/**
 * @file kbd_driver.h
 * @brief Фундаментальный драйвер клавиатуры RAND Elecorner 36.
 *
 * KeyboardDriver — низкоуровневый драйвер, который эмулирует аппаратную
 * клавиатуру с PS/2-подобным интерфейсом. Общается с ядром напрямую
 * через EventBus и предоставляет:
 *
 * - Скан-коды (make/break) для каждой физической клавиши
 * - Трансляцию скан-кода → символа с учётом раскладки
 * - Отслеживание модификаторов (Shift, Ctrl, Alt, CapsLock)
 * - Кольцевой буфер событий клавиш (аппаратный FIFO)
 * - Линейный буфер ввода текста (для терминала)
 * - IRQ-генерацию при нажатии клавиши
 * - Статистику нажатий
 *
 * Архитектура:
 *
 *   GUI (Qt) → injectKeyPress()
 *            ↓
 *   KeyboardDriver
 *     ├─ scancode → KeyEvent       (аппаратный уровень)
 *     ├─ KeyEvent → символ         (трансляция по раскладке)
 *     ├─ keyBuffer_ (ring FIFO)    (аппаратный буфер)
 *     ├─ lineBuffer_ (текст)       (строка ввода)
 *     └─ EventBus.publish(KeyPressed / KeyReleased)
 *            ↓
 *   Kernel / Shell / AppRuntime
 */

#pragma once

#include "types.h"

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <functional>
#include <cstdint>

namespace re36 {

// Предварительные объявления
class EventBus;

// ============================================================================
// Скан-коды
// ============================================================================

/**
 * @enum Scancode
 * @brief Скан-коды виртуальной клавиатуры (PS/2-подобная раскладка).
 */
enum class Scancode : uint16_t {
    // Функциональные
    None        = 0x00,
    Escape      = 0x01,
    F1          = 0x3B,
    F2          = 0x3C,
    F3          = 0x3D,
    F4          = 0x3E,
    F5          = 0x3F,
    F6          = 0x40,
    F7          = 0x41,
    F8          = 0x42,
    F9          = 0x43,
    F10         = 0x44,
    F11         = 0x57,
    F12         = 0x58,

    // Цифры (верхний ряд)
    Key1        = 0x02,
    Key2        = 0x03,
    Key3        = 0x04,
    Key4        = 0x05,
    Key5        = 0x06,
    Key6        = 0x07,
    Key7        = 0x08,
    Key8        = 0x09,
    Key9        = 0x0A,
    Key0        = 0x0B,
    Minus       = 0x0C,
    Equal       = 0x0D,
    Backspace   = 0x0E,

    // Первый ряд букв
    Tab         = 0x0F,
    Q           = 0x10,
    W           = 0x11,
    E           = 0x12,
    R           = 0x13,
    T           = 0x14,
    Y           = 0x15,
    U           = 0x16,
    I           = 0x17,
    O           = 0x18,
    P           = 0x19,
    LeftBracket = 0x1A,
    RightBracket= 0x1B,
    Enter       = 0x1C,

    // Модификаторы
    LeftCtrl    = 0x1D,
    LeftShift   = 0x2A,
    RightShift  = 0x36,
    LeftAlt     = 0x38,
    RightAlt    = 0xE038,
    RightCtrl   = 0xE01D,
    CapsLock    = 0x3A,
    NumLock     = 0x45,
    ScrollLock  = 0x46,

    // Второй ряд букв
    A           = 0x1E,
    S           = 0x1F,
    D           = 0x20,
    F           = 0x21,
    G           = 0x22,
    H           = 0x23,
    J           = 0x24,
    K           = 0x25,
    L           = 0x26,
    Semicolon   = 0x27,
    Apostrophe  = 0x28,
    Grave       = 0x29,  // `
    Backslash   = 0x2B,

    // Третий ряд букв
    Z           = 0x2C,
    X           = 0x2D,
    C           = 0x2E,
    V           = 0x2F,
    B           = 0x30,
    N           = 0x31,
    M           = 0x32,
    Comma       = 0x33,
    Period      = 0x34,
    Slash       = 0x35,

    // Пробел
    Space       = 0x39,

    // Навигация
    Insert      = 0xE052,
    Delete      = 0xE053,
    Home        = 0xE047,
    End         = 0xE04F,
    PageUp      = 0xE049,
    PageDown    = 0xE051,
    ArrowUp     = 0xE048,
    ArrowDown   = 0xE050,
    ArrowLeft   = 0xE04B,
    ArrowRight  = 0xE04D,

    // Специальные
    PrintScreen = 0xE037,
    Pause       = 0xE045
};

// ============================================================================
// Состояние модификаторов
// ============================================================================

/**
 * @struct ModifierState
 * @brief Текущее состояние модификаторных клавиш.
 */
struct ModifierState {
    bool leftShift  = false;
    bool rightShift = false;
    bool leftCtrl   = false;
    bool rightCtrl  = false;
    bool leftAlt    = false;
    bool rightAlt   = false;
    bool capsLock   = false;
    bool numLock    = false;
    bool scrollLock = false;

    /// Любой Shift нажат?
    bool shift() const { return leftShift || rightShift; }
    /// Любой Ctrl нажат?
    bool ctrl() const { return leftCtrl || rightCtrl; }
    /// Любой Alt нажат?
    bool alt() const { return leftAlt || rightAlt; }
    /// Верхний регистр активен (Shift XOR CapsLock)?
    bool uppercase() const { return shift() != capsLock; }
};

// ============================================================================
// Событие клавиши
// ============================================================================

/**
 * @enum KeyAction
 * @brief Тип события клавиши.
 */
enum class KeyAction : uint8_t {
    Press,      ///< Нажатие (make)
    Release,    ///< Отпускание (break)
    Repeat      ///< Автоповтор
};

/**
 * @struct KeyEvent
 * @brief Событие нажатия/отпускания клавиши.
 */
struct KeyEvent {
    Scancode        scancode;           ///< Скан-код клавиши
    KeyAction       action;             ///< Тип события
    char            character   = '\0'; ///< Транслированный символ (или '\0')
    ModifierState   modifiers;          ///< Состояние модификаторов
    uint64_t        tick        = 0;    ///< Тик события
    uint32_t        sequenceNum = 0;    ///< Порядковый номер

    /// Печатный символ?
    bool isPrintable() const { return character >= 32 && character < 127; }
    /// Управляющая клавиша?
    bool isControl() const { return modifiers.ctrl() && character != '\0'; }
};

// ============================================================================
// Раскладка клавиатуры
// ============================================================================

/**
 * @enum KeyboardLayout
 * @brief Раскладка клавиатуры.
 */
enum class KeyboardLayout : uint8_t {
    US_EN,      ///< Английская (US)
    RU          ///< Русская (фонетическая)
};

// ============================================================================
// Статистика клавиатуры
// ============================================================================

/**
 * @struct KeyboardStats
 * @brief Статистика использования клавиатуры.
 */
struct KeyboardStats {
    uint64_t    totalKeyPresses     = 0;
    uint64_t    totalKeyReleases    = 0;
    uint64_t    totalCharacters     = 0;        ///< Печатных символов
    uint64_t    totalInterrupts     = 0;        ///< IRQ от клавиатуры
    uint64_t    bufferOverflows     = 0;        ///< Потерянных событий
    uint64_t    linesEntered        = 0;        ///< Завершённых строк (Enter)
    uint32_t    currentBufferSize   = 0;
    uint32_t    maxBufferSize       = 0;
    double      keysPerSecond       = 0.0;      ///< Средняя скорость набора
};

// ============================================================================
// KeyboardDriver — главный класс
// ============================================================================

/// Максимальный размер аппаратного буфера клавиш
static constexpr uint32_t KBD_BUFFER_SIZE       = 128;
/// Максимальный размер линейного буфера
static constexpr uint32_t KBD_LINE_BUFFER_SIZE  = 1024;
/// IRQ номер клавиатуры
static constexpr uint32_t KBD_IRQ               = 1;
/// Задержка автоповтора (тиков)
static constexpr uint32_t KBD_REPEAT_DELAY      = 30;
/// Интервал автоповтора (тиков)
static constexpr uint32_t KBD_REPEAT_INTERVAL   = 3;

/**
 * @class KeyboardDriver
 * @brief Фундаментальный драйвер клавиатуры.
 *
 * Общается с ядром напрямую через EventBus.
 * На стороне GUI (Qt) вызываются injectKeyPress/injectKeyRelease
 * для эмуляции аппаратных событий.
 */
class KeyboardDriver {
public:
    explicit KeyboardDriver(EventBus& eventBus);
    ~KeyboardDriver();

    KeyboardDriver(const KeyboardDriver&) = delete;
    KeyboardDriver& operator=(const KeyboardDriver&) = delete;

    // ========================================================================
    // Жизненный цикл
    // ========================================================================

    /// Инициализировать драйвер (сбросить буферы, раскладку)
    bool init();

    /// Сбросить драйвер (очистить все буферы и состояние)
    void reset();

    /// Обработать тик (автоповтор, обновление статистики)
    void tick(uint64_t currentTick);

    // ========================================================================
    // Инъекция событий (вызывается из GUI)
    // ========================================================================

    /**
     * Эмулировать нажатие клавиши (make-код).
     * @param scancode Скан-код клавиши
     */
    void injectKeyPress(Scancode scancode);

    /**
     * Эмулировать отпускание клавиши (break-код).
     * @param scancode Скан-код клавиши
     */
    void injectKeyRelease(Scancode scancode);

    /**
     * Ввести строку целиком (удобство для скриптов/тестов).
     * Генерирует события press+release для каждого символа.
     * @param text Строка для ввода
     */
    void injectString(const std::string& text);

    // ========================================================================
    // Чтение буфера (вызывается ядром / shell)
    // ========================================================================

    /**
     * Извлечь следующее событие из аппаратного буфера (FIFO).
     * @return Событие или nullopt если буфер пуст
     */
    std::optional<KeyEvent> pollEvent();

    /**
     * Посмотреть следующее событие без извлечения.
     */
    std::optional<KeyEvent> peekEvent() const;

    /// Есть ли события в буфере?
    bool hasEvents() const;

    /// Количество событий в буфере
    uint32_t eventCount() const;

    // ========================================================================
    // Линейный буфер (для терминала)
    // ========================================================================

    /**
     * Прочитать текущую строку ввода (до нажатия Enter).
     * Не очищает буфер.
     */
    const std::string& getLineBuffer() const;

    /**
     * Прочитать и забрать последнюю завершённую строку.
     * @return Строка или nullopt если нет завершённых строк
     */
    std::optional<std::string> pollLine();

    /// Есть ли завершённые строки?
    bool hasLine() const;

    /// Очистить линейный буфер
    void clearLineBuffer();

    // ========================================================================
    // Раскладка
    // ========================================================================

    /// Установить раскладку
    void setLayout(KeyboardLayout layout);

    /// Получить текущую раскладку
    KeyboardLayout getLayout() const;

    /// Переключить раскладку (EN → RU → EN)
    void toggleLayout();

    // ========================================================================
    // Модификаторы
    // ========================================================================

    /// Текущее состояние модификаторов
    const ModifierState& getModifiers() const;

    // ========================================================================
    // Автоповтор
    // ========================================================================

    /// Включить/отключить автоповтор
    void setRepeatEnabled(bool enabled);
    bool isRepeatEnabled() const;

    /// Настроить задержку и интервал автоповтора
    void setRepeatRate(uint32_t delay, uint32_t interval);

    // ========================================================================
    // Статистика
    // ========================================================================

    /// Получить статистику
    KeyboardStats getStats() const;

    /// Сбросить статистику
    void resetStats();

    // ========================================================================
    // Лог событий (для визуализации)
    // ========================================================================

    /// Последние N событий
    std::vector<KeyEvent> getEventLog(size_t count = 50) const;

    /// Обратный вызов при нажатии клавиши
    using KeyCallback = std::function<void(const KeyEvent&)>;

    /// Зарегистрировать callback
    void registerCallback(KeyCallback cb);

private:
    EventBus&           eventBus_;

    // Состояние
    bool                initialized_ = false;
    ModifierState       modifiers_;
    KeyboardLayout      layout_ = KeyboardLayout::US_EN;

    // Аппаратный буфер клавиш (кольцевой FIFO)
    std::deque<KeyEvent>    keyBuffer_;
    uint32_t                maxBufferSize_ = KBD_BUFFER_SIZE;

    // Линейный буфер
    std::string             lineBuffer_;
    std::deque<std::string> completedLines_;

    // Автоповтор
    bool                repeatEnabled_  = true;
    uint32_t            repeatDelay_    = KBD_REPEAT_DELAY;
    uint32_t            repeatInterval_ = KBD_REPEAT_INTERVAL;
    Scancode            lastKey_        = Scancode::None;
    uint64_t            lastKeyTick_    = 0;
    bool                repeating_      = false;

    // Лог
    std::deque<KeyEvent>    eventLog_;
    static constexpr size_t MAX_EVENT_LOG = 500;

    // Счётчики
    KeyboardStats       stats_;
    uint32_t            nextSequence_   = 1;
    uint64_t            currentTick_    = 0;

    // Callbacks
    std::vector<KeyCallback>    callbacks_;

    // Нажатые клавиши (для отслеживания)
    std::unordered_map<uint16_t, bool>  pressedKeys_;

    // --- Внутренние методы ---

    /// Создать KeyEvent и поместить в буфер
    void processKey(Scancode scancode, KeyAction action);

    /// Обновить состояние модификаторов
    void updateModifiers(Scancode scancode, KeyAction action);

    /// Транслировать скан-код → символ
    char translateScancode(Scancode scancode) const;

    /// Таблица трансляции US-EN
    char translateUS(Scancode scancode) const;

    /// Таблица трансляции RU
    char translateRU(Scancode scancode) const;

    /// Опубликовать событие на EventBus
    void publishKeyEvent(const KeyEvent& evt);

    /// Добавить символ в линейный буфер
    void appendToLineBuffer(const KeyEvent& evt);

    /// Обработать управляющие последовательности (Ctrl+C и т.д.)
    bool handleControlSequence(const KeyEvent& evt);
};

} // namespace re36

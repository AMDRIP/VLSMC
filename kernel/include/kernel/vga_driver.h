/**
 * @file vga_driver.h
 * @brief Фундаментальный драйвер VGA (текстовый режим) RAND Elecorner 36.
 *
 * Эмулирует аппаратный видеобуфер 0xB8000 типичной PC-совместимой
 * системы в текстовом режиме 80x25.
 * Предоставляет прямой доступ к ячейкам памяти (символ + атрибут),
 * а также высокоуровневые функции печати, скроллинга и управления курсором.
 */

#pragma once

#include "types.h"
#include <vector>
#include <string>

namespace re36 {

// Предварительные объявления
class EventBus;

// ============================================================================
// VGA Константы и Типы
// ============================================================================

constexpr uint16_t VGA_WIDTH  = 80;
constexpr uint16_t VGA_HEIGHT = 25;

/**
 * @enum VgaColor
 * @brief 16 стандартных цветов VGA (CGA).
 */
enum class VgaColor : uint8_t {
    Black       = 0,
    Blue        = 1,
    Green       = 2,
    Cyan        = 3,
    Red         = 4,
    Magenta     = 5,
    Brown       = 6,
    LightGray   = 7,
    DarkGray    = 8,
    LightBlue   = 9,
    LightGreen  = 10,
    LightCyan   = 11,
    LightRed    = 12,
    LightMagenta= 13,
    Yellow      = 14,
    White       = 15
};

/**
 * @struct VgaChar
 * @brief Символ в памяти VGA (байт ASCII + байт атрибута).
 */
struct VgaChar {
    char        character;
    uint8_t     attribute;
};

// ============================================================================
// VgaDriver
// ============================================================================

/**
 * @class VgaDriver
 * @brief Низкоуровневый драйвер текстового видеоадаптера.
 */
class VgaDriver {
public:
    explicit VgaDriver(EventBus& eventBus);
    ~VgaDriver();

    VgaDriver(const VgaDriver&) = delete;
    VgaDriver& operator=(const VgaDriver&) = delete;

    // ========================================================================
    // Инициализация
    // ========================================================================

    bool init();
    void reset();

    // ========================================================================
    // Атрибуты и очистка
    // ========================================================================

    /// Сформировать байт атрибута из цвета текста(fg) и цвета фона(bg)
    static uint8_t makeAttribute(VgaColor fg, VgaColor bg);

    /// Возвращает текущий активный атрибут цвета (фон и текст)
    uint8_t getCurrentAttribute() const;

    /// Установить текущий цвет текста и фона
    void setColor(VgaColor fg, VgaColor bg);

    /// Очистить весь экран текущим цветом фона
    void clearScreen();

    // ========================================================================
    // Рисование и Вывод
    // ========================================================================

    /// Записать символ по конкретным координатам (без смещения курсора)
    void putCharAt(char c, uint16_t x, uint16_t y, uint8_t attribute);

    /// Напечатать один символ на текущей позиции курсора и сдвинуть курсор
    void putChar(char c);

    /// Напечатать строку текста начиная с текущей позиции курсора
    void print(const std::string& text);

    // ========================================================================
    // Курсор
    // ========================================================================

    /// Установить позицию аппаратного курсора
    void setCursorPosition(uint16_t x, uint16_t y);

    /// Получить позицию X
    uint16_t getCursorX() const;

    /// Получить позицию Y
    uint16_t getCursorY() const;

    // ========================================================================
    // Управление буфером (для GUI/визуализатора)
    // ========================================================================

    /// Получить сырой буфер экрана
    const std::vector<VgaChar>& getBuffer() const;

private:
    EventBus& eventBus_;
    bool initialized_ = false;

    // Буфер видеопамяти 80 * 25
    std::vector<VgaChar> buffer_;

    // Текущий атрибут по умолчанию
    uint8_t currentAttribute_;

    // Позиция курсора
    uint16_t cursorX_ = 0;
    uint16_t cursorY_ = 0;

    // Внутренние методы
    void scrollUp();
    void publishUpdate();
};

} // namespace re36

/**
 * @file console_driver.h
 * @brief Фундаментальный драйвер системной консоли RAND Elecorner 36.
 *
 * ConsoleDriver предоставляет текстовый интерфейс (TTY) для ОС без GUI.
 * Он общается:
 * - С приложениями: через системные вызовы IoRead / IoWrite (обёрнутые в AppApi).
 * - С клавиатурой: получает готовые строки ввода от KeyboardDriver.
 * - С ядром/монитором: для вывода через EventBus или хранения буфера экрана.
 *
 * Архитектура:
 *
 *   AppApi.print() → syscall(IoWrite, "console") → ConsoleDriver.writeText()
 *   AppApi.readLine() → syscall(IoRead, "console") → ConsoleDriver.readText()
 */

#pragma once

#include "types.h"

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <optional>
#include <functional>

namespace re36 {

// Предварительные объявления
class EventBus;
class InputManager;
class Scheduler;

// ============================================================================
// Статистика консоли
// ============================================================================

/**
 * @struct ConsoleStats
 * @brief Статистика драйвера консоли.
 */
struct ConsoleStats {
    uint64_t totalBytesWritten  = 0;
    uint64_t totalBytesRead     = 0;
    uint64_t totalLinesWritten  = 0;
    uint64_t totalLinesRead     = 0;
    uint32_t currentBufferLines = 0;
    uint32_t maxBufferLines     = 0;
};

// ============================================================================
// ConsoleDriver
// ============================================================================

/**
 * @class ConsoleDriver
 * @brief Драйвер системной консоли.
 */
class ConsoleDriver {
public:
    /**
     * @brief Конструктор.
     * @param eventBus Шина событий для уведомлений (например, обновление экрана).
     * @param inputManager Ссылка на менеджер ввода для чтения событий клавиатуры.
     * @param scheduler Ссылка на планировщик для блокировки процессов в ожидании ввода.
     */
    ConsoleDriver(EventBus& eventBus, class InputManager& inputManager, Scheduler& scheduler);
    ~ConsoleDriver();

    ConsoleDriver(const ConsoleDriver&) = delete;
    ConsoleDriver& operator=(const ConsoleDriver&) = delete;

    // ========================================================================
    // Жизненный цикл
    // ========================================================================

    /// Инициализировать драйвер
    bool init();

    /// Сбросить буферы
    void reset();
    
    /// Обработать тик (проверить наличие строк от клавиатуры, разбудить процессы)
    void tick(uint64_t currentTick);

    // ========================================================================
    // Вывод текста (App -> OS)
    // ========================================================================

    /**
     * @brief Вывести текст на консоль.
     * @param text Текст для вывода (может содержать \n).
     * @param pid PID процесса, который пишет (для логов).
     */
    void writeText(const std::string& text, Pid pid = INVALID_PID);

    // ========================================================================
    // Ввод текста (OS -> App)
    // ========================================================================

    /**
     * @brief Запросить чтение строки из консоли.
     * 
     * Если строка уже введена (в буфере), возвращает её сразу.
     * Если нет — процесс переводится в состояние Waiting, и метод возвращает nullopt.
     * Когда строка будет введена, драйвер разбудит процесс.
     * 
     * @param pid PID читающего процесса
     * @return std::optional<std::string> Строка, если доступна немедленно.
     */
    std::optional<std::string> requestReadLine(Pid pid);

    // ========================================================================
    // Доступ к буферу экрана (для GUI/мониторинга)
    // ========================================================================

    /// Получить все текущие строки на "экране"
    std::vector<std::string> getScreenBuffer() const;

    /// Установить максимальный размер истории (по умолчанию 1000 строк)
    void setMaxHistory(size_t lines);

    /// Очистить экран
    void clearScreen();

    // ========================================================================
    // Статистика
    // ========================================================================

    ConsoleStats getStats() const;

private:
    EventBus&       eventBus_;
    class InputManager& inputManager_;
    Scheduler&      scheduler_;

    bool            initialized_ = false;

    // Экранный буфер
    std::deque<std::string> screenBuffer_;
    std::string             currentLineBuffer_;
    size_t                  maxHistoryLines_ = 1000;

    // Очередь готовых строк (ввод пользователя)
    std::deque<std::string> inputLines_;

    // Очередь процессов, ожидающих ввода (sys_read / readLine)
    std::deque<Pid>         waitingProcesses_;

    // Статистика
    ConsoleStats    stats_;

    // --- Внутренние методы ---

    /// Обработать один символ от клавиатуры (Canonical mode)
    void handleCharInput(const struct KeyEvent& ev);

    /// Добавить символы в экранный буфер (разбивая по \n)
    void appendToScreen(const std::string& text);

    /// Опубликовать событие об изменении экрана
    void publishScreenUpdate();
};

} // namespace re36

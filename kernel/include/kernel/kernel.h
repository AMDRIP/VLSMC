/**
 * @file kernel.h
 * @brief Ядро операционной системы RAND Elecorner 36.
 *
 * Kernel — центральный диспетчер симулятора. Он владеет всеми подсистемами,
 * управляет тиковой моделью времени и предоставляет единый интерфейс
 * системных вызовов.
 *
 * Архитектурные принципы:
 * - Ядро НЕ зависит от Qt (чистый C++).
 * - GUI общается с ядром ТОЛЬКО через syscall().
 * - Подсистемы общаются друг с другом через EventBus.
 * - Каждый тик — один квант работы ОС.
 *
 * @see ARCHITECTURE.md для диаграмм и потоков данных.
 */

#pragma once

#include "types.h"
#include "syscalls.h"
#include "event_bus.h"

#include <memory>
#include <functional>
#include <vector>
#include <string>

namespace re36 {

// ============================================================================
// Предварительные объявления подсистем
// ============================================================================

class Scheduler;        ///< Управление процессами и планирование CPU
class MemoryManager;    ///< Физическая/виртуальная память, подкачка
class FileSystem;       ///< Иноды, каталоги, блоки, права доступа
class IPCManager;       ///< Семафоры, мьютексы, каналы, очереди сообщений
class IOManager;        ///< Устройства, драйверы, прерывания
class UserManager;      ///< Учётные записи, аутентификация, роли
class AppRuntime;       ///< Исполнение пользовательских JS-приложений
class Logger;           ///< Безопасное канальное логирование
class SystemMonitor;    ///< Мониторинг оборудования
class KeyboardDriver;   ///< Фундаментальный драйвер клавиатуры
class ConsoleDriver;    ///< Системная консоль
class VgaDriver;        ///< Драйвер VGA (текстовый режим)

// ============================================================================
// Статистика ядра
// ============================================================================

/**
 * @struct KernelStats
 * @brief Агрегированная статистика работы ядра.
 *
 * Обновляется каждый тик. Используется системным монитором GUI.
 */
struct KernelStats {
    Tick        tickCount       = 0;     ///< Общее количество тиков
    uint32_t    totalProcesses  = 0;     ///< Всего создано процессов за сессию
    uint32_t    activeProcesses = 0;     ///< Активных процессов сейчас
    double      cpuUsage        = 0.0;   ///< Загрузка CPU (0.0–1.0)
    size_t      memoryUsed      = 0;     ///< Использовано памяти (байт)
    size_t      memoryTotal     = 0;     ///< Всего памяти (байт)
    uint64_t    pageFaults      = 0;     ///< Всего промахов страниц
    uint64_t    contextSwitches = 0;     ///< Всего переключений контекста
    uint64_t    syscallCount    = 0;     ///< Всего системных вызовов
    uint64_t    interruptCount  = 0;     ///< Всего обработанных прерываний
    size_t      diskUsed        = 0;     ///< Использовано на диске (байт)
    size_t      diskTotal       = 0;     ///< Всего на диске (байт)
};

// ============================================================================
// Лог-запись инициализации
// ============================================================================

/**
 * @struct BootLogEntry
 * @brief Одна запись из журнала загрузки.
 *
 * Используется экраном загрузки для отображения прогресса
 * инициализации подсистем (BOOT-02).
 */
struct BootLogEntry {
    std::string subsystem;   ///< Имя подсистемы ("Scheduler", "Memory", ...)
    std::string message;     ///< Сообщение ("Инициализация планировщика...")
    bool        success;     ///< Успешно ли
    LogLevel    level;       ///< Уровень серьёзности
};

// ============================================================================
// Callback-типы для связи с GUI
// ============================================================================

/// Callback: ядро отправляет журнал загрузки
using BootProgressCallback = std::function<void(const BootLogEntry& entry)>;

/// Callback: ядро уведомляет об изменении состояния
using StateChangedCallback = std::function<void(KernelState newState)>;

/// Callback: ядро завершило тик (GUI может обновить визуализации)
using TickCallback = std::function<void(Tick tickNumber, const KernelStats& stats)>;

// ============================================================================
// ЯДРО
// ============================================================================

/**
 * @class Kernel
 * @brief Центральный диспетчер операционной системы RAND Elecorner 36.
 *
 * Единственная точка входа из GUI в логику ОС.
 *
 * Типичный жизненный цикл:
 * @code
 *   Kernel kernel;
 *   kernel.setBootProgressCallback([](const BootLogEntry& e) { ... });
 *   kernel.setTickCallback([](Tick t, const KernelStats& s) { ... });
 *
 *   KernelConfig config;
 *   config.totalPhysicalMemory = 2 * 1024 * 1024;  // 2 МБ
 *   config.schedulerAlgorithm = SchedulerAlgorithm::RoundRobin;
 *
 *   kernel.boot(config);   // → KernelState::Booting → KernelState::Running
 *
 *   // Главный цикл (управляется QTimer в GUI)
 *   while (kernel.getState() == KernelState::Running) {
 *       kernel.tick();
 *   }
 *
 *   kernel.shutdown();     // → KernelState::ShuttingDown → KernelState::Halted
 * @endcode
 */
class Kernel {
public:
    Kernel();
    ~Kernel();

    // Некопируемый
    Kernel(const Kernel&) = delete;
    Kernel& operator=(const Kernel&) = delete;

    // ========================================================================
    // Жизненный цикл ядра
    // ========================================================================

    /**
     * Загрузить и инициализировать ядро с заданными параметрами.
     *
     * Последовательность инициализации (KRN-01):
     * 1. EventBus
     * 2. MemoryManager
     * 3. Scheduler
     * 4. FileSystem
     * 5. IOManager
     * 6. IPCManager
     * 7. UserManager
     * 8. AppRuntime
     *
     * @param config Параметры загрузки
     * @return true если все подсистемы загружены успешно
     */
    bool boot(const KernelConfig& config);

    /**
     * Выполнить один квант работы ОС (один тик).
     *
     * Порядок выполнения за тик:
     * 1. eventBus_.processEvents()        — обработка отложенных событий
     * 2. ioManager_.handleInterrupts()    — обработка прерываний
     * 3. scheduler_.schedule()            — выбрать процесс
     * 4. scheduler_.executeCurrentProcess() — выполнить инструкцию
     * 5. memoryManager_.checkPageFaults() — обработка page faults
     * 6. ipcManager_.processMessages()   — доставка IPC-сообщений
     * 7. updateStats()                    — обновить статистику
     * 8. tickCallback_(...)               — уведомить GUI
     */
    void tick();

    /**
     * Остановить ядро.
     *
     * Корректно завершает все процессы, сохраняет состояние ФС,
     * останавливает подсистемы в обратном порядке.
     */
    void shutdown();

    /**
     * Перезагрузить ядро.
     * Эквивалент shutdown() → boot(currentConfig).
     */
    void reboot();

    // ========================================================================
    // Системные вызовы (KRN-02)
    // ========================================================================

    /**
     * Единая точка входа для всех операций с ядром.
     *
     * Все обращения из GUI и пользовательских приложений проходят
     * через этот метод. Каждый вызов маршрутизируется к соответствующей
     * подсистеме и возвращает результат.
     *
     * @param id    Идентификатор системного вызова
     * @param args  Аргументы вызова
     * @return Результат выполнения
     *
     * @see SyscallId для полного перечня вызовов
     * @see SyscallArgs, SyscallResult для структур данных
     */
    SyscallResult syscall(SyscallId id, const SyscallArgs& args = {});

    // ========================================================================
    // Состояние ядра
    // ========================================================================

    /// Текущее состояние ядра
    KernelState getState() const;

    /// Текущий номер тика (сколько квантов прошло с загрузки)
    Tick getTickCount() const;

    /// Агрегированная статистика
    const KernelStats& getStats() const;

    /// Конфигурация, с которой загрузилось ядро
    const KernelConfig& getConfig() const;

    /// Журнал загрузки (для экрана Boot)
    const std::vector<BootLogEntry>& getBootLog() const;

    /// Ядро работает?
    bool isRunning() const { return state_ == KernelState::Running; }

    // ========================================================================
    // Доступ к подсистемам (только для чтения — визуализация)
    // ========================================================================

    /// @note GUI использует эти методы только для визуализации.
    ///       Модификация состояния — ТОЛЬКО через syscall().

    const EventBus&      getEventBus()      const;
    const Scheduler&     getScheduler()     const;
    const MemoryManager& getMemoryManager() const;
    const FileSystem&    getFileSystem()    const;
    const IPCManager&    getIPCManager()    const;
    const IOManager&     getIOManager()     const;
    const UserManager&   getUserManager()   const;
    const AppRuntime&    getAppRuntime()    const;
    const Logger&        getLogger()        const;
          Logger&        getLogger();
    const SystemMonitor& getSystemMonitor() const;
          SystemMonitor& getSystemMonitor();
    const InputManager& getInputManager() const;
          InputManager& getInputManager();
    const KeyboardDriver& getKeyboardDriver() const;
          KeyboardDriver& getKeyboardDriver();
    const ConsoleDriver& getConsoleDriver() const;
          ConsoleDriver& getConsoleDriver();
    const VgaDriver& getVgaDriver() const;
          VgaDriver& getVgaDriver();

    // ========================================================================
    // Callbacks для GUI
    // ========================================================================

    /**
     * Устанавливает callback прогресса загрузки.
     * Вызывается при инициализации каждой подсистемы.
     */
    void setBootProgressCallback(BootProgressCallback callback);

    /**
     * Устанавливает callback смены состояния ядра.
     * Вызывается при переходах: Booting → Running → ShuttingDown → Halted.
     */
    void setStateChangedCallback(StateChangedCallback callback);

    /**
     * Устанавливает callback завершения тика.
     * Вызывается в конце каждого tick() с актуальной статистикой.
     */
    void setTickCallback(TickCallback callback);

    // ========================================================================
    // Управление тиком
    // ========================================================================

    /// Приостановить тиковую систему (ядро остаётся Running, но tick() ничего не делает)
    void pause();

    /// Возобновить тиковую систему
    void resume();

    /// Ядро на паузе?
    bool isPaused() const;

    /// Выполнить один тик в режиме паузы (пошаговый режим для отладки)
    void stepTick();

    /// Установить множитель скорости (1.0 = нормально, 2.0 = удвоенная, 0.5 = замедленная)
    void setSpeedMultiplier(double multiplier);

    /// Текущий множитель скорости
    double getSpeedMultiplier() const;

private:
    // ========================================================================
    // Подсистемы
    // ========================================================================

    std::unique_ptr<EventBus>      eventBus_;
    std::unique_ptr<Scheduler>     scheduler_;
    std::unique_ptr<MemoryManager> memoryManager_;
    std::unique_ptr<FileSystem>    fileSystem_;
    std::unique_ptr<IPCManager>    ipcManager_;
    std::unique_ptr<IOManager>     ioManager_;
    std::unique_ptr<UserManager>   userManager_;
    std::unique_ptr<AppRuntime>    appRuntime_;
    std::unique_ptr<Logger>         logger_;
    std::unique_ptr<SystemMonitor>   systemMonitor_;
    std::unique_ptr<KeyboardDriver>  kbdDriver_;
    std::unique_ptr<ConsoleDriver>   consoleDriver_;
    std::unique_ptr<VgaDriver>       vgaDriver_;

    // ========================================================================
    // Состояние
    // ========================================================================

    KernelState                state_      = KernelState::Uninitialized;
    KernelConfig               config_;
    KernelStats                stats_;
    Tick                       tickCount_  = 0;
    bool                       paused_     = false;
    double                     speedMultiplier_ = 1.0;
    std::vector<BootLogEntry>  bootLog_;

    // ========================================================================
    // Callbacks
    // ========================================================================

    BootProgressCallback       bootProgressCb_;
    StateChangedCallback       stateChangedCb_;
    TickCallback               tickCb_;

    // ========================================================================
    // Внутренние методы
    // ========================================================================

    /// Изменить состояние ядра и уведомить подписчиков
    void setState(KernelState newState);

    /// Записать событие в журнал загрузки и уведомить GUI
    void logBoot(const std::string& subsystem, const std::string& message,
                 bool success, LogLevel level = LogLevel::Info);

    /// Инициализировать одну подсистему с обработкой ошибок
    template<typename T, typename... Args>
    bool initSubsystem(std::unique_ptr<T>& ptr, const std::string& name, Args&&... args);

    /// Обновить агрегированную статистику после тика
    void updateStats();

    // --- Маршрутизация системных вызовов ---

    SyscallResult handleProcessSyscall(SyscallId id, const SyscallArgs& args);
    SyscallResult handleMemorySyscall(SyscallId id, const SyscallArgs& args);
    SyscallResult handleFileSystemSyscall(SyscallId id, const SyscallArgs& args);
    SyscallResult handleIOSyscall(SyscallId id, const SyscallArgs& args);
    SyscallResult handleIPCSyscall(SyscallId id, const SyscallArgs& args);
    SyscallResult handleUserSyscall(SyscallId id, const SyscallArgs& args);
    SyscallResult handleAppSyscall(SyscallId id, const SyscallArgs& args);
    SyscallResult handleVgaSyscall(SyscallId id, const SyscallArgs& args);
    SyscallResult handleSystemSyscall(SyscallId id, const SyscallArgs& args);
};

} // namespace re36

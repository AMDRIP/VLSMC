/**
 * @file system_monitor.h
 * @brief Мониторинг использования оборудования RAND Elecorner 36.
 *
 * SystemMonitor агрегирует метрики со всех подсистем ядра и хранит
 * историю замеров для GUI-визуализации (графики, индикаторы, дашборд).
 *
 * Отслеживаемые показатели:
 * ┌─────────────────────────────────────────────────────┐
 * │  CPU:   usage%, idle%, процессы Ready/Blocked       │
 * │  RAM:   используемые фреймы, page faults/тик,       │
 * │         процент заполнения, фрагментация             │
 * │  Disk:  использованные блоки, IOPS, пропускная      │
 * │         способность (байт/тик)                       │
 * │  IO:    очередь запросов, прерывания/тик,            │
 * │         нагрузка по устройствам                       │
 * │  IPC:   активные семафоры/мьютексы/каналы,           │
 * │         тупики, блокировки                            │
 * │  Proc:  всего/активных/заблокированных/завершённых,  │
 * │         переключений контекста/тик                    │
 * │  Net:   пакетов/тик, задержка (симулированная)       │
 * │  User:  активных сессий, попыток входа               │
 * └─────────────────────────────────────────────────────┘
 *
 * История хранится как ring-буфер снимков (по умолчанию 600 тиков ≈ 10 мин).
 */

#pragma once

#include "types.h"

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <cstdint>

namespace re36 {

// Предварительные объявления
class Scheduler;
class MemoryManager;
class FileSystem;
class IPCManager;
class IOManager;
class UserManager;
class Logger;
class EventBus;

// ============================================================================
// Моментальный снимок CPU
// ============================================================================

struct CpuSnapshot {
    double   usagePercent     = 0.0;    ///< % CPU за последний тик (0–100)
    double   avgUsagePercent  = 0.0;    ///< Скользящее среднее (EMA)
    uint32_t runningCount     = 0;      ///< Процессов в состоянии Running
    uint32_t readyCount       = 0;      ///< Процессов в очереди Ready
    uint32_t blockedCount     = 0;      ///< Заблокированных (IO/IPC/Sleep)
    uint64_t contextSwitches  = 0;      ///< Переключений контекста всего
    uint32_t contextSwitchesPerTick = 0;///< Переключений за последний тик
    Tick     idleTicks        = 0;      ///< Тиков простоя (idle) всего
};

// ============================================================================
// Моментальный снимок памяти
// ============================================================================

struct MemorySnapshot {
    size_t   totalFrames      = 0;      ///< Всего физических фреймов
    size_t   usedFrames       = 0;      ///< Занятых фреймов
    size_t   freeFrames       = 0;      ///< Свободных фреймов
    double   usagePercent     = 0.0;    ///< % использования (0–100)
    uint64_t totalPageFaults  = 0;      ///< Промахов страниц всего
    uint32_t pageFaultsPerTick = 0;     ///< Промахов за последний тик
    double   fragmentationPercent = 0.0;///< % фрагментации
    size_t   totalBytes       = 0;      ///< Объём в байтах (total)
    size_t   usedBytes        = 0;      ///< Использовано байт
};

// ============================================================================
// Моментальный снимок диска
// ============================================================================

struct DiskSnapshot {
    size_t   totalBlocks      = 0;      ///< Всего блоков на диске
    size_t   usedBlocks       = 0;      ///< Занятых блоков
    size_t   freeBlocks       = 0;      ///< Свободных блоков
    double   usagePercent     = 0.0;    ///< % использования
    uint32_t totalFiles       = 0;      ///< Количество файлов
    uint32_t totalDirectories = 0;      ///< Количество каталогов
    uint64_t totalOps         = 0;      ///< Операций чтения/записи всего
    uint32_t opsPerTick       = 0;      ///< Операций за тик (IOPS)
    uint64_t bytesReadTotal   = 0;      ///< Прочитано байт всего
    uint64_t bytesWrittenTotal = 0;     ///< Записано байт всего
};

// ============================================================================
// Моментальный снимок ввода-вывода
// ============================================================================

/// Метрика одного устройства
struct DeviceMetrics {
    std::string name;
    std::string type;           ///< "Disk", "Keyboard", "Display", "Network"
    std::string state;          ///< "Idle", "Busy", "Error"
    uint32_t    queueLength = 0;///< Длина очереди запросов
    uint64_t    totalOps    = 0;///< Обработанных операций всего
    uint64_t    totalBytes  = 0;///< Передано байт всего
};

struct IoSnapshot {
    uint64_t totalInterrupts   = 0;     ///< Прерываний всего
    uint32_t interruptsPerTick = 0;     ///< Прерываний за тик
    uint32_t pendingRequests   = 0;     ///< Запросов в очереди (все устройства)
    uint32_t activeDevices     = 0;     ///< Устройств в состоянии Busy
    uint32_t totalDevices      = 0;     ///< Всего устройств
    std::vector<DeviceMetrics> devices; ///< По каждому устройству
};

// ============================================================================
// Моментальный снимок IPC
// ============================================================================

struct IpcSnapshot {
    uint32_t activeSemaphores  = 0;     ///< Семафоров
    uint32_t activeMutexes     = 0;     ///< Мьютексов
    uint32_t activePipes       = 0;     ///< Каналов
    uint32_t activeQueues      = 0;     ///< Очередей сообщений
    uint32_t blockedProcesses  = 0;     ///< Процессов заблокированных на IPC
    bool     deadlockDetected  = false; ///< Обнаружен тупик?
    uint32_t deadlockedCount   = 0;     ///< Количество процессов в тупике
};

// ============================================================================
// Моментальный снимок процессов
// ============================================================================

struct ProcessSnapshot {
    uint32_t total        = 0;  ///< Всего процессов (включая завершённые)
    uint32_t running      = 0;  ///< Выполняющихся (Running)
    uint32_t ready        = 0;  ///< В очереди (Ready)
    uint32_t blocked      = 0;  ///< Заблокированных (Blocked)
    uint32_t suspended    = 0;  ///< Приостановленных (Suspended)
    uint32_t terminated   = 0;  ///< Завершённых (Terminated)
    uint32_t created      = 0;  ///< Всего создано за сессию
};

// ============================================================================
// Моментальный снимок пользователей
// ============================================================================

struct UserSnapshot {
    uint32_t totalAccounts    = 0;      ///< Всего учётных записей
    uint32_t activeSessions   = 0;      ///< Активных сессий
    uint32_t failedLogins     = 0;      ///< Неудачных попыток (за сессию)
    uint32_t auditEntries     = 0;      ///< Записей в журнале аудита
};

// ============================================================================
// Полный снимок системы
// ============================================================================

/**
 * @struct SystemSnapshot
 * @brief Полный снимок всех метрик оборудования в конкретный тик.
 */
struct SystemSnapshot {
    Tick              tick = 0;
    CpuSnapshot       cpu;
    MemorySnapshot    memory;
    DiskSnapshot      disk;
    IoSnapshot        io;
    IpcSnapshot       ipc;
    ProcessSnapshot   processes;
    UserSnapshot      users;
};

// ============================================================================
// Порог для предупреждений
// ============================================================================

/**
 * @struct AlertThresholds
 * @brief Пороговые значения для автоматических предупреждений.
 */
struct AlertThresholds {
    double   cpuWarning       = 80.0;   ///< CPU% при котором Warning
    double   cpuCritical      = 95.0;   ///< CPU% при котором Critical
    double   memoryWarning    = 75.0;   ///< RAM% Warning
    double   memoryCritical   = 90.0;   ///< RAM% Critical
    double   diskWarning      = 80.0;   ///< Disk% Warning
    double   diskCritical     = 95.0;   ///< Disk% Critical
    uint32_t ioQueueWarning   = 20;     ///< Длина очереди IO
    uint32_t processLimit     = 100;    ///< Лимит процессов
};

/**
 * @enum AlertLevel
 */
enum class AlertLevel : uint8_t {
    Normal   = 0,
    Warning  = 1,
    Critical = 2
};

/**
 * @struct SystemAlert
 * @brief Предупреждение о превышении порога.
 */
struct SystemAlert {
    Tick         tick;
    std::string  subsystem;     ///< "cpu", "memory", "disk", "io", "process"
    AlertLevel   level;
    std::string  message;
    double       currentValue;
    double       threshold;
};

// ============================================================================
// SystemMonitor
// ============================================================================

/**
 * @class SystemMonitor
 * @brief Агрегатор метрик оборудования с историей для GUI.
 *
 * Использование:
 * @code
 *   // В boot():
 *   monitor.init(scheduler, memoryManager, fileSystem, ...);
 *
 *   // В tick():
 *   monitor.collect(currentTick);
 *
 *   // GUI:
 *   auto snap = monitor.getCurrentSnapshot();
 *   auto history = monitor.getHistory(300);  // последние 300 тиков
 *   auto cpuHistory = monitor.getCpuHistory(100);
 * @endcode
 */
class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor();

    SystemMonitor(const SystemMonitor&) = delete;
    SystemMonitor& operator=(const SystemMonitor&) = delete;

    // ========================================================================
    // Инициализация
    // ========================================================================

    /**
     * Привязать подсистемы.
     * Вызывается при boot() ядра ПОСЛЕ создания всех подсистем.
     */
    void init(Scheduler& scheduler, MemoryManager& memory,
              FileSystem& fs, IPCManager& ipc, IOManager& io,
              UserManager& users, Logger& logger, EventBus& eventBus);

    // ========================================================================
    // Сбор метрик
    // ========================================================================

    /**
     * Собрать метрики со всех подсистем.
     * Вызывается ядром каждый тик.
     */
    void collect(Tick currentTick);

    // ========================================================================
    // Текущее состояние
    // ========================================================================

    /// Последний снимок
    const SystemSnapshot& getCurrentSnapshot() const;

    /// Текущий CPU usage (%)
    double getCpuUsage() const;

    /// Текущий RAM usage (%)
    double getMemoryUsage() const;

    /// Текущий Disk usage (%)
    double getDiskUsage() const;

    /// Уровень тревоги по подсистеме
    AlertLevel getCpuAlertLevel() const;
    AlertLevel getMemoryAlertLevel() const;
    AlertLevel getDiskAlertLevel() const;

    // ========================================================================
    // История (для графиков GUI)
    // ========================================================================

    /// Полная история (до maxHistory снимков)
    const std::deque<SystemSnapshot>& getHistory() const;

    /// Последние N снимков
    std::vector<SystemSnapshot> getRecentHistory(size_t count) const;

    /// Только CPU-метрики за последние N тиков
    std::vector<CpuSnapshot> getCpuHistory(size_t count) const;

    /// Только Memory-метрики за последние N тиков
    std::vector<MemorySnapshot> getMemoryHistory(size_t count) const;

    /// Только Disk-метрики за последние N тиков
    std::vector<DiskSnapshot> getDiskHistory(size_t count) const;

    /// Только IO-метрики за последние N тиков
    std::vector<IoSnapshot> getIoHistory(size_t count) const;

    // ========================================================================
    // Пороги и предупреждения
    // ========================================================================

    /// Установить пороги
    void setThresholds(const AlertThresholds& thresholds);

    /// Текущие пороги
    const AlertThresholds& getThresholds() const;

    /// Список активных предупреждений
    std::vector<SystemAlert> getActiveAlerts() const;

    /// История предупреждений (последние N)
    std::vector<SystemAlert> getAlertHistory(size_t count) const;

    // ========================================================================
    // Настройки
    // ========================================================================

    /// Макс. глубина истории (по умолчанию 600 тиков)
    void setMaxHistory(size_t maxSnapshots);
    size_t getMaxHistory() const;

    /// Частота сбора (каждые N тиков, по умолчанию 1)
    void setCollectInterval(uint32_t interval);

private:
    // Ссылки на подсистемы (не владеет ими)
    Scheduler*     scheduler_     = nullptr;
    MemoryManager* memoryManager_ = nullptr;
    FileSystem*    fileSystem_    = nullptr;
    IPCManager*    ipcManager_    = nullptr;
    IOManager*     ioManager_     = nullptr;
    UserManager*   userManager_   = nullptr;
    Logger*        logger_        = nullptr;
    EventBus*      eventBus_      = nullptr;

    // Текущий и предыдущий снимок
    SystemSnapshot current_;
    SystemSnapshot previous_;

    // История
    std::deque<SystemSnapshot> history_;
    size_t maxHistory_ = 600;
    uint32_t collectInterval_ = 1;

    // Пороги
    AlertThresholds thresholds_;
    std::vector<SystemAlert> alerts_;
    size_t maxAlerts_ = 200;

    // EMA (Exponential Moving Average)
    double emaAlpha_ = 0.1;  // сглаживание: 0.1 = медленное

    // Счётчики из предыдущего тика (для delta)
    uint64_t prevContextSwitches_ = 0;
    uint64_t prevPageFaults_      = 0;
    uint64_t prevInterrupts_      = 0;
    uint64_t prevDiskOps_         = 0;

    // --- Внутренние методы сбора ---
    CpuSnapshot     collectCpu();
    MemorySnapshot  collectMemory();
    DiskSnapshot    collectDisk();
    IoSnapshot      collectIo();
    IpcSnapshot     collectIpc();
    ProcessSnapshot collectProcesses();
    UserSnapshot    collectUsers();

    /// Проверить пороги и создать предупреждения
    void checkThresholds(Tick tick);
    void addAlert(Tick tick, const std::string& subsystem,
                  AlertLevel level, const std::string& msg,
                  double value, double threshold);
};

} // namespace re36

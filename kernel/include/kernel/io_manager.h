/**
 * @file io_manager.h
 * @brief Менеджер устройств ввода-вывода RAND Elecorner 36.
 *
 * Управляет виртуальными устройствами, драйверами, очередями
 * запросов и прерываниями.
 * Соответствует требованиям IO-01 — IO-06.
 */

#pragma once

#include "types.h"

#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <memory>
#include <functional>

namespace re36 {

// Предварительные объявления
class EventBus;
class Scheduler;

// ============================================================================
// Запрос ввода-вывода
// ============================================================================

/**
 * @struct IoRequest
 * @brief Один запрос к устройству.
 */
struct IoRequest {
    uint32_t        requestId       = 0;
    DeviceId        deviceId        = 0;
    Pid             requesterPid    = INVALID_PID;
    bool            isRead          = true;         ///< true = чтение, false = запись
    size_t          dataSize        = 0;
    std::string     data;                           ///< Данные для записи
    uint32_t        diskAddress     = 0;            ///< Адрес на диске (для блочных)
    Tick            submittedAt     = 0;            ///< Тик постановки в очередь
    Tick            completedAt     = 0;            ///< Тик завершения
    uint32_t        estimatedTicks  = 1;            ///< Ожидаемое время обработки (тики)
    uint32_t        remainingTicks  = 0;            ///< Осталось тиков до завершения
    bool            completed       = false;
    std::string     result;                         ///< Результат (данные чтения или статус)
};

// ============================================================================
// Прерывание
// ============================================================================

/**
 * @struct Interrupt
 * @brief Прерывание от устройства.
 */
struct Interrupt {
    DeviceId        deviceId;
    uint32_t        requestId;      ///< ID завершённого запроса
    Tick            tick;
    std::string     message;
};

// ============================================================================
// Абстрактный драйвер (IO-05)
// ============================================================================

/**
 * @class IDeviceDriver
 * @brief Абстрактный интерфейс драйвера устройства.
 */
class IDeviceDriver {
public:
    virtual ~IDeviceDriver() = default;

    /// Инициализировать устройство
    virtual bool init() = 0;

    /// Имя устройства
    virtual std::string deviceName() const = 0;

    /// Тип устройства
    virtual DeviceType deviceType() const = 0;

    /// Обработать один тик работы (уменьшить remaining на текущем запросе)
    virtual void tick(IoRequest& currentRequest) = 0;

    /// Рассчитать время обработки запроса (тиков)
    virtual uint32_t estimateTime(const IoRequest& request) const = 0;

    /// Сбросить устройство
    virtual void reset() = 0;
};

// ============================================================================
// Виртуальное устройство (IO-01)
// ============================================================================

/**
 * @struct Device
 * @brief Виртуальное устройство ввода-вывода.
 */
struct Device {
    DeviceId                    id;
    std::string                 name;
    DeviceType                  type;
    DeviceStatus                status      = DeviceStatus::Idle;
    std::unique_ptr<IDeviceDriver> driver;

    // Очередь запросов (IO-02)
    std::queue<IoRequest>       requestQueue;
    IoRequest*                  currentRequest = nullptr; ///< Текущий обрабатываемый запрос

    // Статистика
    uint64_t                    totalRequests   = 0;
    uint64_t                    completedRequests = 0;
    uint64_t                    totalBusyTicks  = 0;
};

// ============================================================================
// Информация об устройстве (для GUI)
// ============================================================================

/**
 * @struct DeviceInfo
 * @brief Публичная информация об устройстве.
 */
struct DeviceInfo {
    DeviceId        id;
    std::string     name;
    DeviceType      type;
    DeviceStatus    status;
    uint32_t        queueSize;
    uint64_t        totalRequests;
    uint64_t        completedRequests;
    double          utilization;    ///< % времени занят (busyTicks / totalTicks)
};

// ============================================================================
// Менеджер устройств
// ============================================================================

/**
 * @class IOManager
 * @brief Управление устройствами и обработка ввода-вывода.
 */
class IOManager {
public:
    explicit IOManager(EventBus& eventBus, Scheduler& scheduler, const KernelConfig& config);
    ~IOManager();

    IOManager(const IOManager&) = delete;
    IOManager& operator=(const IOManager&) = delete;

    // ========================================================================
    // Инициализация
    // ========================================================================

    /**
     * Инициализировать менеджер и создать устройства по умолчанию (IO-06):
     * - Диск (блочное)
     * - Принтер (символьное)
     * - Клавиатура (символьное)
     */
    bool init();

    // ========================================================================
    // Запросы ввода-вывода
    // ========================================================================

    /**
     * Поставить запрос на чтение.
     * Процесс-инициатор переводится в Waiting.
     */
    uint32_t requestRead(DeviceId deviceId, Pid pid, size_t size, uint32_t address = 0);

    /**
     * Поставить запрос на запись.
     * Процесс-инициатор переводится в Waiting.
     */
    uint32_t requestWrite(DeviceId deviceId, Pid pid, const std::string& data, uint32_t address = 0);

    // ========================================================================
    // Обработка (вызывается из Kernel::tick)
    // ========================================================================

    /**
     * Обработать прерывания от устройств.
     * Для каждого устройства:
     * - Если текущий запрос завершён → генерировать прерывание
     * - Если устройство свободно и есть очередь → начать следующий
     * - Если устройство работает → tick() на драйвере
     */
    void handleInterrupts();

    // ========================================================================
    // Планирование дисковых запросов (IO-03)
    // ========================================================================

    /// Сменить алгоритм планирования диска
    void setDiskSchedulingAlgorithm(DiskSchedulingAlgorithm algo);
    DiskSchedulingAlgorithm getDiskSchedulingAlgorithm() const;

    // ========================================================================
    // Запросы (для GUI)
    // ========================================================================

    /// Список всех устройств
    std::vector<DeviceInfo> getDeviceList() const;

    /// Информация об устройстве
    std::optional<DeviceInfo> getDeviceInfo(DeviceId id) const;

    /// Очередь запросов устройства
    std::vector<IoRequest> getDeviceQueue(DeviceId id) const;

    /// Лог прерываний (последние N)
    std::vector<Interrupt> getInterruptLog(size_t count = 100) const;

    /// Общее количество обработанных прерываний
    uint64_t getTotalInterruptCount() const;

private:
    EventBus&                                       eventBus_;
    Scheduler&                                      scheduler_;
    KernelConfig                                    config_;

    // Устройства
    std::unordered_map<DeviceId, std::unique_ptr<Device>> devices_;
    DeviceId                                        nextDeviceId_ = 1;

    // Планирование диска
    DiskSchedulingAlgorithm                         diskAlgo_;

    // Счётчик запросов
    uint32_t                                        nextRequestId_ = 1;

    // Лог прерываний
    std::vector<Interrupt>                          interruptLog_;
    static constexpr size_t MAX_INTERRUPT_LOG = 5000;

    // Текущий тик
    Tick                                            currentTick_ = 0;
    uint64_t                                        totalTicks_  = 0;

    // --- Внутренние методы ---

    /// Создать устройство по умолчанию
    DeviceId addDevice(const std::string& name, DeviceType type,
                       std::unique_ptr<IDeviceDriver> driver);

    /// Выбрать следующий запрос из очереди дискового устройства (IO-03)
    std::optional<IoRequest> selectNextDiskRequest(Device& device);

    /// FCFS выбор
    std::optional<IoRequest> selectFCFS(Device& device);

    /// SSTF выбор
    std::optional<IoRequest> selectSSTF(Device& device, uint32_t currentHead);

    /// SCAN выбор (алгоритм лифта)
    std::optional<IoRequest> selectSCAN(Device& device, uint32_t currentHead, bool& direction);

    /// Записать прерывание
    void recordInterrupt(DeviceId deviceId, uint32_t requestId, const std::string& message);

    /// Текущая позиция головки диска (для планирования)
    uint32_t                                        diskHeadPosition_ = 0;
    bool                                            diskDirection_    = true; // true = вверх
};

} // namespace re36

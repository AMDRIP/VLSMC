/**
 * @file ipc.h
 * @brief Межпроцессное взаимодействие (IPC) RAND Elecorner 36.
 *
 * Реализует механизмы синхронизации и обмена данными
 * между процессами: семафоры, мьютексы, каналы (pipes),
 * очереди сообщений, обнаружение тупиков.
 * Соответствует требованиям IPC-01 — IPC-06.
 */

#pragma once

#include "types.h"

#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <optional>

namespace re36 {

// Предварительные объявления
class EventBus;
class Scheduler;

// ============================================================================
// Семафор (IPC-01)
// ============================================================================

/**
 * @struct Semaphore
 * @brief Счётный семафор.
 */
struct Semaphore {
    SemaphoreId         id          = 0;
    std::string         name;
    int32_t             value       = 1;        ///< Текущее значение
    int32_t             initialValue = 1;       ///< Начальное значение
    Uid                 owner       = ROOT_UID; ///< Создатель
    std::vector<Pid>    waitQueue;              ///< Процессы, ожидающие wait()
    Tick                createdAt   = 0;
};

// ============================================================================
// Мьютекс (IPC-02)
// ============================================================================

/**
 * @struct Mutex
 * @brief Бинарный мьютекс для взаимного исключения.
 */
struct Mutex {
    MutexId             id          = 0;
    std::string         name;
    bool                locked      = false;
    Pid                 lockedBy    = INVALID_PID;  ///< PID захватившего процесса
    Uid                 owner       = ROOT_UID;
    std::vector<Pid>    waitQueue;                  ///< Очередь ожидающих lock()
    Tick                createdAt   = 0;
    Tick                lockedAt    = 0;
};

// ============================================================================
// Канал (IPC-03)
// ============================================================================

/**
 * @struct Pipe
 * @brief Однонаправленный канал между двумя процессами.
 */
struct Pipe {
    PipeId              id          = 0;
    Pid                 writerPid   = INVALID_PID;  ///< Процесс-писатель
    Pid                 readerPid   = INVALID_PID;  ///< Процесс-читатель
    std::queue<std::string> buffer;                 ///< Буфер данных
    size_t              maxBufferSize = 64;          ///< Макс. сообщений в буфере
    bool                writerClosed = false;
    bool                readerClosed = false;
    Tick                createdAt   = 0;
};

// ============================================================================
// Очередь сообщений (IPC-04)
// ============================================================================

/**
 * @struct Message
 * @brief Одно сообщение в очереди.
 */
struct Message {
    Pid                 senderPid;
    std::string         data;
    Tick                sentAt      = 0;
};

/**
 * @struct MessageQueue
 * @brief Именованная очередь сообщений.
 */
struct MessageQueue {
    MessageQueueId      id          = 0;
    std::string         name;
    std::queue<Message> messages;
    size_t              maxSize     = 128;          ///< Макс. сообщений
    Uid                 owner       = ROOT_UID;
    std::vector<Pid>    waitingReceivers;           ///< Ожидающие receive()
    Tick                createdAt   = 0;
};

// ============================================================================
// Обнаружение тупиков (IPC-05)
// ============================================================================

/**
 * @enum ResourceType
 * @brief Тип ресурса для графа ожидания.
 */
enum class ResourceType : uint8_t {
    SemaphoreRes,
    MutexRes,
    PipeRes,
    QueueRes
};

/**
 * @struct ResourceAllocation
 * @brief Информация о выделенном ресурсе (для алгоритма банкира).
 */
struct ResourceAllocation {
    ResourceType    type;
    uint32_t        resourceId;
    Pid             heldBy;             ///< Кто держит
    std::vector<Pid> waitedBy;          ///< Кто ждёт
};

/**
 * @struct DeadlockInfo
 * @brief Результат обнаружения тупика.
 */
struct DeadlockInfo {
    bool                    detected    = false;
    std::vector<Pid>        involvedPids;       ///< Процессы в цикле
    std::vector<ResourceAllocation> resources;   ///< Заблокированные ресурсы
    std::string             description;
};

// ============================================================================
// Журнал IPC
// ============================================================================

/**
 * @struct IPCLogEntry
 * @brief Запись в журнале операций синхронизации.
 */
struct IPCLogEntry {
    Tick            tick;
    Pid             pid;
    std::string     operation;      ///< "sem_wait", "mutex_lock", "pipe_write" и т.д.
    std::string     resourceName;
    bool            success;
    std::string     details;
};

// ============================================================================
// Менеджер IPC
// ============================================================================

/**
 * @class IPCManager
 * @brief Управление межпроцессным взаимодействием.
 */
class IPCManager {
public:
    explicit IPCManager(EventBus& eventBus, Scheduler& scheduler, const KernelConfig& config);
    ~IPCManager();

    IPCManager(const IPCManager&) = delete;
    IPCManager& operator=(const IPCManager&) = delete;

    // ========================================================================
    // Инициализация
    // ========================================================================

    bool init();

    // ========================================================================
    // Семафоры (IPC-01)
    // ========================================================================

    /// Создать семафор
    SemaphoreId createSemaphore(const std::string& name, int32_t initialValue = 1, Uid owner = ROOT_UID);

    /// wait() / P() — уменьшить значение, заблокировать если <= 0
    bool semWait(SemaphoreId id, Pid pid);

    /// signal() / V() — увеличить значение, разбудить ожидающего
    bool semSignal(SemaphoreId id, Pid pid);

    /// Удалить семафор
    bool destroySemaphore(SemaphoreId id);

    /// Информация о семафоре
    std::optional<Semaphore> getSemaphoreInfo(SemaphoreId id) const;

    // ========================================================================
    // Мьютексы (IPC-02)
    // ========================================================================

    /// Создать мьютекс
    MutexId createMutex(const std::string& name, Uid owner = ROOT_UID);

    /// lock() — захватить, заблокировать если занят
    bool mutexLock(MutexId id, Pid pid);

    /// unlock() — освободить, разбудить следующего
    bool mutexUnlock(MutexId id, Pid pid);

    /// Удалить мьютекс
    bool destroyMutex(MutexId id);

    /// Информация о мьютексе
    std::optional<Mutex> getMutexInfo(MutexId id) const;

    // ========================================================================
    // Каналы (IPC-03)
    // ========================================================================

    /// Создать канал между двумя процессами
    PipeId createPipe(Pid writerPid, Pid readerPid);

    /// Записать в канал
    bool pipeWrite(PipeId id, const std::string& data);

    /// Прочитать из канала
    std::optional<std::string> pipeRead(PipeId id);

    /// Закрыть канал
    bool closePipe(PipeId id, Pid pid);

    // ========================================================================
    // Очереди сообщений (IPC-04)
    // ========================================================================

    /// Создать очередь
    MessageQueueId createMessageQueue(const std::string& name, Uid owner = ROOT_UID);

    /// Отправить сообщение
    bool sendMessage(MessageQueueId queueId, Pid senderPid, const std::string& data);

    /// Получить сообщение (nullopt если очередь пуста)
    std::optional<Message> receiveMessage(MessageQueueId queueId, Pid receiverPid);

    /// Удалить очередь
    bool destroyMessageQueue(MessageQueueId id);

    // ========================================================================
    // Обнаружение тупиков (IPC-05)
    // ========================================================================

    /**
     * Проверить наличие тупиков.
     * Строит граф ожидания и ищет циклы.
     */
    DeadlockInfo checkDeadlocks() const;

    // ========================================================================
    // Обработка сообщений (вызывается из Kernel::tick)
    // ========================================================================

    /// Обработать отложенные IPC-операции
    void processMessages();

    // ========================================================================
    // Очистка
    // ========================================================================

    /// Освободить все IPC-ресурсы процесса (при завершении)
    void cleanupProcess(Pid pid);

    // ========================================================================
    // Визуализация
    // ========================================================================

    /// Все семафоры
    std::vector<Semaphore> getAllSemaphores() const;

    /// Все мьютексы
    std::vector<Mutex> getAllMutexes() const;

    /// Граф зависимостей ресурсов
    std::vector<ResourceAllocation> getResourceGraph() const;

    /// Журнал операций (последние N записей)
    std::vector<IPCLogEntry> getLog(size_t count = 100) const;

private:
    EventBus&                                       eventBus_;
    Scheduler&                                      scheduler_;
    KernelConfig                                    config_;

    // Ресурсы
    std::unordered_map<SemaphoreId, Semaphore>      semaphores_;
    std::unordered_map<MutexId, Mutex>              mutexes_;
    std::unordered_map<PipeId, Pipe>                pipes_;
    std::unordered_map<MessageQueueId, MessageQueue> messageQueues_;

    // Счётчики ID
    SemaphoreId                                     nextSemId_   = 1;
    MutexId                                         nextMutexId_ = 1;
    PipeId                                          nextPipeId_  = 1;
    MessageQueueId                                  nextQueueId_ = 1;

    // Журнал
    std::vector<IPCLogEntry>                        log_;
    static constexpr size_t MAX_LOG_SIZE = 5000;

    // Текущий тик (обновляется из processMessages)
    Tick                                            currentTick_ = 0;

    /// Добавить запись в журнал
    void logOperation(Pid pid, const std::string& op, const std::string& resource,
                      bool success, const std::string& details = "");

    /// Построить граф ожидания и проверить циклы
    bool detectCycle(const std::unordered_map<Pid, std::vector<Pid>>& waitGraph,
                     std::vector<Pid>& cycle) const;
};

} // namespace re36

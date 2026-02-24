/**
 * @file scheduler.h
 * @brief Планировщик процессов RAND Elecorner 36.
 *
 * Scheduler управляет жизненным циклом процессов (PROC-02),
 * реализует несколько алгоритмов планирования CPU (PROC-03),
 * и поддерживает переключение алгоритма «на лету» (PROC-04).
 */

#pragma once

#include "types.h"
#include "process.h"

#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>
#include <optional>

namespace re36 {

// Предварительные объявления
class EventBus;

// ============================================================================
// Интерфейс алгоритма планирования
// ============================================================================

/**
 * @class ISchedulingAlgorithm
 * @brief Абстрактный интерфейс для алгоритмов планирования.
 *
 * Каждый алгоритм реализует свою логику выбора следующего процесса
 * из очереди готовых.
 */
class ISchedulingAlgorithm {
public:
    virtual ~ISchedulingAlgorithm() = default;

    /// Имя алгоритма (для логирования)
    virtual std::string name() const = 0;

    /// Тип алгоритма
    virtual SchedulerAlgorithm type() const = 0;

    /**
     * Выбрать следующий процесс для выполнения.
     * @param readyQueue Вектор PID процессов в состоянии Ready
     * @param processes  Таблица процессов (PID → Process)
     * @param currentPid PID текущего процесса (INVALID_PID если CPU свободен)
     * @return PID выбранного процесса или INVALID_PID
     */
    virtual Pid selectNext(
        const std::vector<Pid>& readyQueue,
        const std::unordered_map<Pid, Process>& processes,
        Pid currentPid
    ) = 0;

    /**
     * Нужно ли вытеснить текущий процесс?
     * @return true если текущий процесс должен уступить CPU
     */
    virtual bool shouldPreempt(
        const Process& current,
        const std::vector<Pid>& readyQueue,
        const std::unordered_map<Pid, Process>& processes
    ) = 0;

    /**
     * Уведомить алгоритм о завершении кванта времени.
     * (используется Round Robin)
     */
    virtual void onQuantumExpired(Process& process) {}

    /**
     * Сброс внутреннего состояния алгоритма.
     */
    virtual void reset() {}
};

// ============================================================================
// Планировщик
// ============================================================================

/**
 * @class Scheduler
 * @brief Управление процессами и планирование CPU.
 *
 * Основные обязанности:
 * - Создание / завершение процессов (PROC-05)
 * - Управление состояниями процессов (PROC-02)
 * - Выбор процесса для CPU (PROC-03)
 * - Переключение контекста
 * - Ведение диаграммы Ганта для визуализации
 */
class Scheduler {
public:
    /**
     * @param eventBus Ссылка на шину событий (не владеет)
     * @param config   Конфигурация ядра
     */
    explicit Scheduler(EventBus& eventBus, const KernelConfig& config);
    ~Scheduler();

    // Некопируемый
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    // ========================================================================
    // Инициализация
    // ========================================================================

    /**
     * Инициализировать планировщик.
     * Создаёт алгоритм по умолчанию из конфигурации.
     * @return true при успешной инициализации
     */
    bool init();

    // ========================================================================
    // Управление процессами (PROC-05)
    // ========================================================================

    /**
     * Создать новый процесс.
     * @param name      Имя процесса
     * @param burstTime Требуемое время CPU (тики)
     * @param priority  Приоритет (0 — наивысший)
     * @param parentPid PID родителя (INVALID_PID для корневых)
     * @param owner     UID владельца
     * @return PID созданного процесса
     */
    Pid createProcess(const std::string& name, uint32_t burstTime,
                      uint8_t priority = 5, Pid parentPid = INVALID_PID,
                      Uid owner = ROOT_UID);

    /**
     * Создать процесс с заданным набором инструкций.
     * @param name         Имя
     * @param instructions Программа (набор инструкций)
     * @param priority     Приоритет
     * @param memoryRequired Требуемая память (байт)
     * @param parentPid    PID родителя
     * @param owner        UID владельца
     * @return PID созданного процесса
     */
    Pid createProcess(const std::string& name,
                      std::vector<Instruction> instructions,
                      uint8_t priority = 5,
                      size_t memoryRequired = 4096,
                      Pid parentPid = INVALID_PID,
                      Uid owner = ROOT_UID);

    /**
     * Завершить процесс.
     * @param pid  PID процесса
     * @param code Код завершения
     * @return true если процесс найден и завершён
     */
    bool killProcess(Pid pid, int32_t code = -1);

    /**
     * Приостановить процесс (перевести в Waiting).
     */
    bool suspendProcess(Pid pid);

    /**
     * Возобновить процесс (перевести в Ready).
     */
    bool resumeProcess(Pid pid);

    // ========================================================================
    // Планирование (вызывается из Kernel::tick)
    // ========================================================================

    /**
     * Выбрать следующий процесс для исполнения на CPU.
     * Выполняет переключение контекста при необходимости.
     */
    void schedule();

    /**
     * Выполнить одну инструкцию текущего процесса.
     * @return Тип выполненной инструкции (для обработки ядром)
     */
    std::optional<InstructionType> executeCurrentProcess();

    // ========================================================================
    // Алгоритм планирования (PROC-03, PROC-04)
    // ========================================================================

    /**
     * Сменить алгоритм планирования «на лету».
     * @param algo Новый алгоритм
     */
    void setAlgorithm(SchedulerAlgorithm algo);

    /// Текущий алгоритм
    SchedulerAlgorithm getAlgorithm() const;

    /// Имя текущего алгоритма (для GUI)
    std::string getAlgorithmName() const;

    /**
     * Установить квант времени (для Round Robin).
     * @param quantum Количество тиков
     */
    void setTimeQuantum(uint32_t quantum);

    /// Текущий квант времени
    uint32_t getTimeQuantum() const;

    // ========================================================================
    // Запросы состояния (для визуализации)
    // ========================================================================

    /// Получить процесс по PID (nullptr если не найден)
    const Process* getProcess(Pid pid) const;

    /// PID текущего процесса на CPU
    Pid getCurrentPid() const;

    /// Список всех процессов
    std::vector<ProcessSnapshot> getProcessList() const;

    /// Процессы в состоянии Ready
    std::vector<Pid> getReadyQueue() const;

    /// Процессы в состоянии Waiting
    std::vector<Pid> getWaitingQueue() const;

    /// Количество активных процессов (не Terminated)
    uint32_t getActiveProcessCount() const;

    /// Общее количество созданных процессов
    uint32_t getTotalProcessCount() const;

    /// Количество переключений контекста
    uint64_t getContextSwitchCount() const;

    /// Диаграмма Ганта (последние N записей)
    std::vector<GanttEntry> getGanttChart(size_t maxEntries = 100) const;

    /// Загрузка CPU (0.0 — 1.0): доля тиков с работающим процессом
    double getCpuUsage() const;

    /// Дерево процессов (PID → список дочерних PID)
    std::unordered_map<Pid, std::vector<Pid>> getProcessTree() const;

    // ========================================================================
    // Обработка событий от других подсистем
    // ========================================================================

    /// Разблокировать процесс, ожидающий I/O
    void onIoComplete(Pid pid);

    /// Разблокировать процесс, ожидающий IPC
    void onIpcReady(Pid pid);

private:
    EventBus&                               eventBus_;
    KernelConfig                            config_;

    // Таблица процессов
    std::unordered_map<Pid, Process>        processes_;
    Pid                                     nextPid_ = KERNEL_PID + 1;

    // Очереди
    std::vector<Pid>                        readyQueue_;
    std::vector<Pid>                        waitingQueue_;
    Pid                                     currentPid_ = INVALID_PID;

    // Алгоритм
    std::unique_ptr<ISchedulingAlgorithm>   algorithm_;
    uint32_t                                timeQuantum_;

    // Статистика
    uint64_t                                contextSwitches_ = 0;
    uint64_t                                totalTicks_      = 0;
    uint64_t                                busyTicks_       = 0;
    uint32_t                                totalCreated_    = 0;

    // Диаграмма Ганта
    std::vector<GanttEntry>                 ganttChart_;
    static constexpr size_t MAX_GANTT_SIZE = 1000;

    // --- Внутренние методы ---

    /// Сменить состояние процесса и опубликовать событие
    void changeState(Process& proc, ProcessState newState);

    /// Переключить контекст на другой процесс
    void contextSwitch(Pid newPid);

    /// Добавить запись в диаграмму Ганта
    void recordGantt(Pid pid, Tick start, Tick end);

    /// Создать алгоритм по перечислению
    std::unique_ptr<ISchedulingAlgorithm> createAlgorithm(SchedulerAlgorithm algo);

    /// Удалить pid из очереди
    void removeFromQueue(std::vector<Pid>& queue, Pid pid);
};

} // namespace re36

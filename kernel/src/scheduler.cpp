/**
 * @file scheduler.cpp
 * @brief Реализация планировщика процессов RAND Elecorner 36.
 *
 * Содержит:
 * - 4 алгоритма планирования (FCFS, Round Robin, Priority, Multilevel Queue)
 * - Полное управление жизненным циклом процессов
 * - Переключение контекста
 * - Диаграмма Ганта для визуализации
 */

#include "kernel/scheduler.h"
#include "kernel/event_bus.h"

#include <algorithm>
#include <cassert>
#include <limits>

namespace re36 {

// ============================================================================
// Конкретные алгоритмы планирования
// ============================================================================

// ---------- FCFS (First Come, First Served) ---------------------------------

class FCFSAlgorithm : public ISchedulingAlgorithm {
public:
    std::string name() const override { return "FCFS"; }
    SchedulerAlgorithm type() const override { return SchedulerAlgorithm::FCFS; }

    Pid selectNext(const std::vector<Pid>& readyQueue,
                   const std::unordered_map<Pid, Process>& /*processes*/,
                   Pid currentPid) override {
        // Если текущий процесс работает — не прерываем (non-preemptive)
        if (currentPid != INVALID_PID) {
            return currentPid;
        }
        // Берём первый из очереди
        if (!readyQueue.empty()) {
            return readyQueue.front();
        }
        return INVALID_PID;
    }

    bool shouldPreempt(const Process& /*current*/,
                       const std::vector<Pid>& /*readyQueue*/,
                       const std::unordered_map<Pid, Process>& /*processes*/) override {
        // FCFS — не вытесняющий
        return false;
    }
};

// ---------- Round Robin (Циклический) ----------------------------------------

class RoundRobinAlgorithm : public ISchedulingAlgorithm {
public:
    explicit RoundRobinAlgorithm(uint32_t quantum) : quantum_(quantum) {}

    std::string name() const override { return "Round Robin (q=" + std::to_string(quantum_) + ")"; }
    SchedulerAlgorithm type() const override { return SchedulerAlgorithm::RoundRobin; }

    Pid selectNext(const std::vector<Pid>& readyQueue,
                   const std::unordered_map<Pid, Process>& processes,
                   Pid currentPid) override {
        // Если текущий процесс — проверяем, не истёк ли квант
        if (currentPid != INVALID_PID) {
            auto it = processes.find(currentPid);
            if (it != processes.end() && it->second.quantumRemaining > 0) {
                return currentPid;  // квант ещё не истёк
            }
        }
        // Берём следующий из очереди
        if (!readyQueue.empty()) {
            return readyQueue.front();
        }
        return INVALID_PID;
    }

    bool shouldPreempt(const Process& current,
                       const std::vector<Pid>& readyQueue,
                       const std::unordered_map<Pid, Process>& /*processes*/) override {
        // Вытеснить, если квант истёк и есть другие процессы
        return current.quantumRemaining == 0 && !readyQueue.empty();
    }

    void onQuantumExpired(Process& process) override {
        // Сбросить квант
        process.quantumRemaining = quantum_;
    }

    void setQuantum(uint32_t q) { quantum_ = q; }
    uint32_t getQuantum() const { return quantum_; }

private:
    uint32_t quantum_;
};

// ---------- Priority (Приоритетный, вытесняющий) ----------------------------

class PriorityAlgorithm : public ISchedulingAlgorithm {
public:
    std::string name() const override { return "Priority (Preemptive)"; }
    SchedulerAlgorithm type() const override { return SchedulerAlgorithm::Priority; }

    Pid selectNext(const std::vector<Pid>& readyQueue,
                   const std::unordered_map<Pid, Process>& processes,
                   Pid /*currentPid*/) override {
        if (readyQueue.empty()) {
            return INVALID_PID;
        }

        // Найти процесс с наименьшим приоритетом (0 = наивысший)
        Pid bestPid = readyQueue.front();
        uint8_t bestPriority = 255;

        for (Pid pid : readyQueue) {
            auto it = processes.find(pid);
            if (it != processes.end() && it->second.priority < bestPriority) {
                bestPriority = it->second.priority;
                bestPid = pid;
            }
        }
        return bestPid;
    }

    bool shouldPreempt(const Process& current,
                       const std::vector<Pid>& readyQueue,
                       const std::unordered_map<Pid, Process>& processes) override {
        // Вытеснить, если в очереди есть процесс с более высоким приоритетом
        for (Pid pid : readyQueue) {
            auto it = processes.find(pid);
            if (it != processes.end() && it->second.priority < current.priority) {
                return true;
            }
        }
        return false;
    }
};

// ---------- Multilevel Queue (Многоуровневая очередь) ------------------------

/**
 * Реализация многоуровневой очереди:
 * - Уровень 0: высший приоритет, маленький квант (2 тика)
 * - Уровень 1: средний приоритет, средний квант (4 тика)
 * - Уровень 2: низший приоритет, большой квант (8 тиков) + FCFS внутри
 *
 * Если процесс использует весь квант — понижается на уровень ниже.
 * Если процесс блокируется (I/O) — возвращается на текущий уровень.
 * Периодически (каждые 50 тиков) все процессы повышаются до уровня 0 (aging).
 */
class MultilevelQueueAlgorithm : public ISchedulingAlgorithm {
public:
    MultilevelQueueAlgorithm() = default;

    std::string name() const override { return "Multilevel Feedback Queue"; }
    SchedulerAlgorithm type() const override { return SchedulerAlgorithm::MultilevelQueue; }

    Pid selectNext(const std::vector<Pid>& readyQueue,
                   const std::unordered_map<Pid, Process>& processes,
                   Pid currentPid) override {
        ++ticksSinceBoost_;

        // Aging: периодически повышать все процессы
        if (ticksSinceBoost_ >= AGING_INTERVAL) {
            ticksSinceBoost_ = 0;
            // Пометка: фактический boost делается в schedule() через queueLevel
        }

        // Если текущий — проверяем квант
        if (currentPid != INVALID_PID) {
            auto it = processes.find(currentPid);
            if (it != processes.end() && it->second.quantumRemaining > 0) {
                // Квант не истёк, но может ли кто-то с более высоким уровнем вытеснить?
                for (Pid pid : readyQueue) {
                    auto jt = processes.find(pid);
                    if (jt != processes.end() && jt->second.queueLevel < it->second.queueLevel) {
                        return pid;  // вытеснить текущий
                    }
                }
                return currentPid;
            }
        }

        if (readyQueue.empty()) {
            return INVALID_PID;
        }

        // Выбрать процесс с наименьшим уровнем очереди (наивысший приоритет)
        // При равных уровнях — FCFS (первый в readyQueue)
        Pid bestPid = readyQueue.front();
        uint8_t bestLevel = 255;

        for (Pid pid : readyQueue) {
            auto it = processes.find(pid);
            if (it != processes.end() && it->second.queueLevel < bestLevel) {
                bestLevel = it->second.queueLevel;
                bestPid = pid;
            }
        }
        return bestPid;
    }

    bool shouldPreempt(const Process& current,
                       const std::vector<Pid>& readyQueue,
                       const std::unordered_map<Pid, Process>& processes) override {
        // Вытеснить если квант истёк
        if (current.quantumRemaining == 0) {
            return !readyQueue.empty();
        }
        // Вытеснить если появился процесс на уровне выше
        for (Pid pid : readyQueue) {
            auto it = processes.find(pid);
            if (it != processes.end() && it->second.queueLevel < current.queueLevel) {
                return true;
            }
        }
        return false;
    }

    void onQuantumExpired(Process& process) override {
        // Понизить уровень (если не на самом нижнем)
        if (process.queueLevel < MAX_LEVEL) {
            process.queueLevel++;
        }
        // Назначить квант по новому уровню
        process.quantumRemaining = getQuantumForLevel(process.queueLevel);
    }

    void reset() override {
        ticksSinceBoost_ = 0;
    }

    /// Получить квант для уровня очереди
    static uint32_t getQuantumForLevel(uint8_t level) {
        switch (level) {
            case 0:  return 2;
            case 1:  return 4;
            default: return 8;
        }
    }

    bool shouldBoost() const { return ticksSinceBoost_ >= AGING_INTERVAL; }

private:
    static constexpr uint8_t  MAX_LEVEL       = 2;
    static constexpr uint32_t AGING_INTERVAL   = 50;
    uint32_t                  ticksSinceBoost_ = 0;
};

// ============================================================================
// Scheduler — реализация
// ============================================================================

Scheduler::Scheduler(EventBus& eventBus, const KernelConfig& config)
    : eventBus_(eventBus),
      config_(config),
      timeQuantum_(config.timeQuantum)
{
}

Scheduler::~Scheduler() = default;

// ---- Инициализация ---------------------------------------------------------

bool Scheduler::init() {
    algorithm_ = createAlgorithm(config_.schedulerAlgorithm);
    if (!algorithm_) {
        return false;
    }

    currentPid_ = INVALID_PID;
    nextPid_ = KERNEL_PID + 1;
    contextSwitches_ = 0;
    totalTicks_ = 0;
    busyTicks_ = 0;
    totalCreated_ = 0;
    readyQueue_.clear();
    waitingQueue_.clear();
    processes_.clear();
    ganttChart_.clear();

    return true;
}

// ---- Создание процессов ----------------------------------------------------

Pid Scheduler::createProcess(const std::string& name, uint32_t burstTime,
                              uint8_t priority, Pid parentPid, Uid owner) {
    if (processes_.size() >= config_.maxProcesses) {
        return INVALID_PID;
    }

    Process proc;
    proc.pid            = nextPid_++;
    proc.name           = name;
    proc.parentPid      = parentPid;
    proc.owner          = owner;
    proc.priority       = priority;
    proc.basePriority   = priority;
    proc.burstTime      = burstTime;
    proc.remainingBurst = burstTime;
    proc.createdAt      = totalTicks_;
    proc.state          = ProcessState::New;

    // Создать простую программу: burstTime инструкций CpuWork
    proc.instructions.resize(burstTime, Instruction{InstructionType::CpuWork, 0, 0, ""});
    proc.programCounter = 0;

    // Квант для Round Robin / Multilevel Queue
    proc.quantumRemaining = timeQuantum_;
    proc.queueLevel = 0;

    // Память по умолчанию
    proc.memoryRequired = 4096;

    Pid pid = proc.pid;

    // Добавить в дерево родителя
    if (parentPid != INVALID_PID) {
        auto parentIt = processes_.find(parentPid);
        if (parentIt != processes_.end()) {
            parentIt->second.children.push_back(pid);
        }
    }

    // Вставить процесс в таблицу
    processes_.emplace(pid, std::move(proc));
    totalCreated_++;

    // Перевести в Ready
    changeState(processes_.at(pid), ProcessState::Ready);

    // Добавить в очередь готовых
    readyQueue_.push_back(pid);

    // Событие
    Event evt(EventType::ProcessCreated, totalTicks_, "scheduler");
    evt.with("pid", static_cast<int64_t>(pid))
       .with("name", name)
       .with("priority", static_cast<int64_t>(priority));
    eventBus_.publish(evt);

    return pid;
}

Pid Scheduler::createProcess(const std::string& name,
                              std::vector<Instruction> instructions,
                              uint8_t priority,
                              size_t memoryRequired,
                              Pid parentPid,
                              Uid owner) {
    if (processes_.size() >= config_.maxProcesses) {
        return INVALID_PID;
    }

    Process proc;
    proc.pid            = nextPid_++;
    proc.name           = name;
    proc.parentPid      = parentPid;
    proc.owner          = owner;
    proc.priority       = priority;
    proc.basePriority   = priority;
    proc.burstTime      = static_cast<uint32_t>(instructions.size());
    proc.remainingBurst = proc.burstTime;
    proc.createdAt      = totalTicks_;
    proc.state          = ProcessState::New;
    proc.instructions   = std::move(instructions);
    proc.programCounter = 0;
    proc.memoryRequired = memoryRequired;
    proc.quantumRemaining = timeQuantum_;
    proc.queueLevel     = 0;

    Pid pid = proc.pid;

    if (parentPid != INVALID_PID) {
        auto parentIt = processes_.find(parentPid);
        if (parentIt != processes_.end()) {
            parentIt->second.children.push_back(pid);
        }
    }

    processes_.emplace(pid, std::move(proc));
    totalCreated_++;

    changeState(processes_.at(pid), ProcessState::Ready);
    readyQueue_.push_back(pid);

    Event evt(EventType::ProcessCreated, totalTicks_, "scheduler");
    evt.with("pid", static_cast<int64_t>(pid))
       .with("name", name)
       .with("priority", static_cast<int64_t>(priority));
    eventBus_.publish(evt);

    return pid;
}

// ---- Управление процессами -------------------------------------------------

bool Scheduler::killProcess(Pid pid, int32_t code) {
    auto it = processes_.find(pid);
    if (it == processes_.end()) return false;

    Process& proc = it->second;
    if (proc.state == ProcessState::Terminated) return false;

    proc.exitCode = code;
    proc.terminatedAt = totalTicks_;
    proc.turnaroundTime = static_cast<uint32_t>(totalTicks_ - proc.createdAt);

    // Снять с CPU если работает
    if (currentPid_ == pid) {
        currentPid_ = INVALID_PID;
    }

    // Удалить из очередей
    removeFromQueue(readyQueue_, pid);
    removeFromQueue(waitingQueue_, pid);

    changeState(proc, ProcessState::Terminated);

    // Событие
    Event evt(EventType::ProcessTerminated, totalTicks_, "scheduler");
    evt.with("pid", static_cast<int64_t>(pid))
       .with("name", proc.name)
       .with("exitCode", static_cast<int64_t>(code));
    eventBus_.publish(evt);

    return true;
}

bool Scheduler::suspendProcess(Pid pid) {
    auto it = processes_.find(pid);
    if (it == processes_.end()) return false;

    Process& proc = it->second;
    if (proc.state != ProcessState::Running && proc.state != ProcessState::Ready) {
        return false;
    }

    if (currentPid_ == pid) {
        currentPid_ = INVALID_PID;
    }

    removeFromQueue(readyQueue_, pid);
    changeState(proc, ProcessState::Waiting);
    waitingQueue_.push_back(pid);

    return true;
}

bool Scheduler::resumeProcess(Pid pid) {
    auto it = processes_.find(pid);
    if (it == processes_.end()) return false;

    Process& proc = it->second;
    if (proc.state != ProcessState::Waiting) return false;

    removeFromQueue(waitingQueue_, pid);
    changeState(proc, ProcessState::Ready);
    readyQueue_.push_back(pid);

    return true;
}

// ---- Планирование ----------------------------------------------------------

void Scheduler::schedule() {
    totalTicks_++;

    // Aging для Multilevel Queue
    auto* mlq = dynamic_cast<MultilevelQueueAlgorithm*>(algorithm_.get());
    if (mlq && mlq->shouldBoost()) {
        for (auto& [pid, proc] : processes_) {
            if (proc.state == ProcessState::Ready || proc.state == ProcessState::Running) {
                proc.queueLevel = 0;
                proc.quantumRemaining = MultilevelQueueAlgorithm::getQuantumForLevel(0);
            }
        }
    }

    // Обновить waitingTime для процессов в Ready
    for (Pid pid : readyQueue_) {
        auto it = processes_.find(pid);
        if (it != processes_.end()) {
            it->second.waitingTime++;
        }
    }

    // Если текущий процесс завершён — освободить CPU
    if (currentPid_ != INVALID_PID) {
        auto it = processes_.find(currentPid_);
        if (it != processes_.end()) {
            Process& current = it->second;
            if (current.state == ProcessState::Terminated || !current.hasMoreInstructions()) {
                if (current.state != ProcessState::Terminated) {
                    killProcess(currentPid_, 0);
                }
                currentPid_ = INVALID_PID;
            }
        } else {
            currentPid_ = INVALID_PID;
        }
    }

    // Уменьшить квант текущего процесса
    if (currentPid_ != INVALID_PID) {
        auto it = processes_.find(currentPid_);
        if (it != processes_.end()) {
            Process& current = it->second;
            if (current.quantumRemaining > 0) {
                current.quantumRemaining--;
            }

            // Проверить, нужно ли вытеснить
            if (algorithm_->shouldPreempt(current, readyQueue_, processes_)) {
                // Вернуть текущий в очередь Ready
                changeState(current, ProcessState::Ready);
                readyQueue_.push_back(currentPid_);

                // Сбросить квант
                algorithm_->onQuantumExpired(current);

                currentPid_ = INVALID_PID;
            }
        }
    }

    // Выбрать следующий процесс
    Pid nextPid = algorithm_->selectNext(readyQueue_, processes_, currentPid_);

    if (nextPid != INVALID_PID && nextPid != currentPid_) {
        contextSwitch(nextPid);
    } else if (nextPid == INVALID_PID && currentPid_ != INVALID_PID) {
        // Очередь пуста, текущий продолжает — всё ок
    }

    // Подсчёт busyTicks
    if (currentPid_ != INVALID_PID) {
        busyTicks_++;
    }
}

std::optional<InstructionType> Scheduler::executeCurrentProcess() {
    if (currentPid_ == INVALID_PID) {
        return std::nullopt;  // CPU простаивает (idle)
    }

    auto it = processes_.find(currentPid_);
    if (it == processes_.end()) {
        currentPid_ = INVALID_PID;
        return std::nullopt;
    }

    Process& proc = it->second;

    if (!proc.hasMoreInstructions()) {
        killProcess(currentPid_, 0);
        return InstructionType::Exit;
    }

    const Instruction* instr = proc.currentInstruction();
    if (!instr) {
        return std::nullopt;
    }

    InstructionType instrType = instr->type;

    // Обработка инструкции
    switch (instrType) {
        case InstructionType::CpuWork:
            // Чистые вычисления — просто потребляем тик
            proc.advanceProgramCounter();
            if (proc.remainingBurst > 0) {
                proc.remainingBurst--;
            }
            break;

        case InstructionType::IoRequest:
            // Процесс запрашивает I/O → переходит в Waiting
            proc.advanceProgramCounter();
            changeState(proc, ProcessState::Waiting);
            removeFromQueue(readyQueue_, currentPid_);
            waitingQueue_.push_back(currentPid_);
            proc.ioWaitTime++;
            currentPid_ = INVALID_PID;
            break;

        case InstructionType::MemoryAccess:
            // Обращение к памяти (потенциальный page fault обрабатывается ядром)
            proc.advanceProgramCounter();
            if (proc.remainingBurst > 0) {
                proc.remainingBurst--;
            }
            break;

        case InstructionType::IpcOperation:
            // IPC-операция — может заблокировать процесс
            // Фактическая обработка делегируется ядру
            proc.advanceProgramCounter();
            break;

        case InstructionType::Sleep: {
            // Спяший процесс → Waiting на param1 тиков
            proc.advanceProgramCounter();
            changeState(proc, ProcessState::Waiting);
            removeFromQueue(readyQueue_, currentPid_);
            waitingQueue_.push_back(currentPid_);
            currentPid_ = INVALID_PID;
            break;
        }

        case InstructionType::Exit:
            // Завершение процесса
            killProcess(currentPid_, 0);
            break;
    }

    return instrType;
}

// ---- Алгоритм --------------------------------------------------------------

void Scheduler::setAlgorithm(SchedulerAlgorithm algo) {
    auto newAlgo = createAlgorithm(algo);
    if (newAlgo) {
        algorithm_ = std::move(newAlgo);

        // При смене на Round Robin / MLQ — обновить кванты процессов
        if (algo == SchedulerAlgorithm::RoundRobin) {
            for (auto& [pid, proc] : processes_) {
                if (proc.state == ProcessState::Ready || proc.state == ProcessState::Running) {
                    proc.quantumRemaining = timeQuantum_;
                }
            }
        } else if (algo == SchedulerAlgorithm::MultilevelQueue) {
            for (auto& [pid, proc] : processes_) {
                if (proc.state == ProcessState::Ready || proc.state == ProcessState::Running) {
                    proc.queueLevel = 0;
                    proc.quantumRemaining = MultilevelQueueAlgorithm::getQuantumForLevel(0);
                }
            }
        }
    }
}

SchedulerAlgorithm Scheduler::getAlgorithm() const {
    return algorithm_ ? algorithm_->type() : SchedulerAlgorithm::FCFS;
}

std::string Scheduler::getAlgorithmName() const {
    return algorithm_ ? algorithm_->name() : "Unknown";
}

void Scheduler::setTimeQuantum(uint32_t quantum) {
    timeQuantum_ = quantum;
    // Обновить если текущий алгоритм — Round Robin
    auto* rr = dynamic_cast<RoundRobinAlgorithm*>(algorithm_.get());
    if (rr) {
        rr->setQuantum(quantum);
    }
}

uint32_t Scheduler::getTimeQuantum() const {
    return timeQuantum_;
}

// ---- Запросы состояния -----------------------------------------------------

const Process* Scheduler::getProcess(Pid pid) const {
    auto it = processes_.find(pid);
    return (it != processes_.end()) ? &it->second : nullptr;
}

Pid Scheduler::getCurrentPid() const {
    return currentPid_;
}

std::vector<ProcessSnapshot> Scheduler::getProcessList() const {
    std::vector<ProcessSnapshot> result;
    result.reserve(processes_.size());

    for (const auto& [pid, proc] : processes_) {
        ProcessSnapshot snap;
        snap.pid        = proc.pid;
        snap.name       = proc.name;
        snap.state      = proc.state;
        snap.priority   = proc.priority;
        snap.memoryUsed = proc.memoryAllocated;
        snap.cpuTime    = proc.burstTime - proc.remainingBurst;
        snap.owner      = std::to_string(proc.owner);  // TODO: resolve username
        snap.isSystem   = proc.isSystemProcess;
        result.push_back(snap);
    }

    return result;
}

std::vector<Pid> Scheduler::getReadyQueue() const {
    return readyQueue_;
}

std::vector<Pid> Scheduler::getWaitingQueue() const {
    return waitingQueue_;
}

uint32_t Scheduler::getActiveProcessCount() const {
    uint32_t count = 0;
    for (const auto& [pid, proc] : processes_) {
        if (proc.state != ProcessState::Terminated) {
            count++;
        }
    }
    return count;
}

uint32_t Scheduler::getTotalProcessCount() const {
    return totalCreated_;
}

uint64_t Scheduler::getContextSwitchCount() const {
    return contextSwitches_;
}

std::vector<GanttEntry> Scheduler::getGanttChart(size_t maxEntries) const {
    if (maxEntries == 0 || maxEntries >= ganttChart_.size()) {
        return ganttChart_;
    }
    // Вернуть последние N записей
    return std::vector<GanttEntry>(
        ganttChart_.end() - static_cast<ptrdiff_t>(maxEntries),
        ganttChart_.end()
    );
}

double Scheduler::getCpuUsage() const {
    if (totalTicks_ == 0) return 0.0;
    return static_cast<double>(busyTicks_) / static_cast<double>(totalTicks_);
}

std::unordered_map<Pid, std::vector<Pid>> Scheduler::getProcessTree() const {
    std::unordered_map<Pid, std::vector<Pid>> tree;
    for (const auto& [pid, proc] : processes_) {
        tree[proc.parentPid].push_back(pid);
    }
    return tree;
}

// ---- Обработка событий от подсистем ----------------------------------------

void Scheduler::onIoComplete(Pid pid) {
    resumeProcess(pid);
}

void Scheduler::onIpcReady(Pid pid) {
    resumeProcess(pid);
}

// ============================================================================
// Приватные методы
// ============================================================================

void Scheduler::changeState(Process& proc, ProcessState newState) {
    ProcessState oldState = proc.state;
    proc.state = newState;

    Event evt(EventType::ProcessStateChanged, totalTicks_, "scheduler");
    evt.with("pid", static_cast<int64_t>(proc.pid))
       .with("name", proc.name)
       .with("oldState", static_cast<int64_t>(static_cast<uint8_t>(oldState)))
       .with("newState", static_cast<int64_t>(static_cast<uint8_t>(newState)));
    eventBus_.publish(evt);
}

void Scheduler::contextSwitch(Pid newPid) {
    Tick switchTick = totalTicks_;

    // Записать Ганта для уходящего процесса
    if (currentPid_ != INVALID_PID) {
        auto it = processes_.find(currentPid_);
        if (it != processes_.end()) {
            // Если текущий ещё работает (не завершён и не на ожидании) → в Ready
            if (it->second.state == ProcessState::Running) {
                changeState(it->second, ProcessState::Ready);
                readyQueue_.push_back(currentPid_);
            }
        }
    }

    // Убрать новый из очереди Ready
    removeFromQueue(readyQueue_, newPid);

    // Переключить
    auto it = processes_.find(newPid);
    if (it != processes_.end()) {
        Process& proc = it->second;
        changeState(proc, ProcessState::Running);

        // Установить квант при первом запуске или после вытеснения
        if (proc.quantumRemaining == 0) {
            if (algorithm_->type() == SchedulerAlgorithm::MultilevelQueue) {
                proc.quantumRemaining = MultilevelQueueAlgorithm::getQuantumForLevel(proc.queueLevel);
            } else {
                proc.quantumRemaining = timeQuantum_;
            }
        }
    }

    // Записать запись Ганта
    recordGantt(newPid, switchTick, switchTick);

    contextSwitches_++;
    Pid oldPid = currentPid_;
    currentPid_ = newPid;

    // Событие
    Event evt(EventType::ContextSwitch, totalTicks_, "scheduler");
    evt.with("fromPid", static_cast<int64_t>(oldPid))
       .with("toPid", static_cast<int64_t>(newPid));
    eventBus_.publish(evt);
}

void Scheduler::recordGantt(Pid pid, Tick start, Tick end) {
    // Попробовать продлить последнюю запись если тот же PID
    if (!ganttChart_.empty()) {
        auto& last = ganttChart_.back();
        if (last.pid == pid && last.endTick == start - 1) {
            last.endTick = end;
            return;
        }
        // Обновить endTick последней записи
        if (last.pid == pid && last.endTick == start) {
            last.endTick = end;
            return;
        }
    }

    // Новая запись
    ganttChart_.push_back({pid, start, end});

    // Ограничить размер
    if (ganttChart_.size() > MAX_GANTT_SIZE) {
        ganttChart_.erase(ganttChart_.begin(),
                          ganttChart_.begin() + static_cast<ptrdiff_t>(ganttChart_.size() - MAX_GANTT_SIZE));
    }
}

std::unique_ptr<ISchedulingAlgorithm> Scheduler::createAlgorithm(SchedulerAlgorithm algo) {
    switch (algo) {
        case SchedulerAlgorithm::FCFS:
            return std::make_unique<FCFSAlgorithm>();
        case SchedulerAlgorithm::RoundRobin:
            return std::make_unique<RoundRobinAlgorithm>(timeQuantum_);
        case SchedulerAlgorithm::Priority:
            return std::make_unique<PriorityAlgorithm>();
        case SchedulerAlgorithm::MultilevelQueue:
            return std::make_unique<MultilevelQueueAlgorithm>();
    }
    // Fallback
    return std::make_unique<FCFSAlgorithm>();
}

void Scheduler::removeFromQueue(std::vector<Pid>& queue, Pid pid) {
    queue.erase(
        std::remove(queue.begin(), queue.end(), pid),
        queue.end()
    );
}

} // namespace re36

/**
 * @file ipc.cpp
 * @brief Реализация межпроцессного взаимодействия RAND Elecorner 36.
 */

#include "kernel/ipc.h"
#include "kernel/event_bus.h"

#include <algorithm>

namespace re36 {

IPCManager::IPCManager(EventBus& eventBus, const KernelConfig& config)
    : eventBus_(eventBus), config_(config) {}

IPCManager::~IPCManager() = default;

bool IPCManager::init() {
    semaphores_.clear();
    mutexes_.clear();
    pipes_.clear();
    messageQueues_.clear();
    waitGraph_.clear();
    history_.clear();
    return true;
}

// ---- Семафоры (IPC-01) -----------------------------------------------------

SemaphoreId IPCManager::createSemaphore(const std::string& name, int32_t initial) {
    SemaphoreId id = nextSemId_++;
    Semaphore s;
    s.id = id;
    s.name = name;
    s.value = initial;
    s.initialValue = initial;
    semaphores_[id] = std::move(s);

    recordOp("sem_create", INVALID_PID, "name=" + name + " val=" + std::to_string(initial));
    return id;
}

bool IPCManager::semaphoreWait(SemaphoreId id, Pid pid) {
    auto it = semaphores_.find(id);
    if (it == semaphores_.end()) return false;
    auto& sem = it->second;

    if (sem.value > 0) {
        sem.value--;
        sem.totalAcquires++;
        recordOp("sem_wait_ok", pid, "sem=" + sem.name + " val=" + std::to_string(sem.value));
        return true;
    }

    // Блокировать процесс
    sem.waitingProcesses.push_back(pid);
    sem.totalBlocks++;
    addWaitEdge(pid, id, "semaphore");
    recordOp("sem_wait_blocked", pid, "sem=" + sem.name);

    Event evt(EventType::IpcBlocked, 0, "ipc");
    evt.with("pid", static_cast<int64_t>(pid)).with("resource", sem.name);
    eventBus_.publish(evt);

    return false; // процесс заблокирован
}

bool IPCManager::semaphoreSignal(SemaphoreId id) {
    auto it = semaphores_.find(id);
    if (it == semaphores_.end()) return false;
    auto& sem = it->second;

    if (!sem.waitingProcesses.empty()) {
        Pid woken = sem.waitingProcesses.front();
        sem.waitingProcesses.erase(sem.waitingProcesses.begin());
        removeWaitEdge(woken);
        sem.totalAcquires++;
        recordOp("sem_signal_wake", woken, "sem=" + sem.name);

        Event evt(EventType::IpcUnblocked, 0, "ipc");
        evt.with("pid", static_cast<int64_t>(woken)).with("resource", sem.name);
        eventBus_.publish(evt);

        return true; // вернуть PID для пробуждения (через возврат true, PID в ожидании)
    }

    sem.value++;
    recordOp("sem_signal", INVALID_PID, "sem=" + sem.name + " val=" + std::to_string(sem.value));
    return true;
}

void IPCManager::destroySemaphore(SemaphoreId id) {
    auto it = semaphores_.find(id);
    if (it != semaphores_.end()) {
        for (Pid p : it->second.waitingProcesses) removeWaitEdge(p);
        semaphores_.erase(it);
    }
}

std::optional<Semaphore> IPCManager::getSemaphore(SemaphoreId id) const {
    auto it = semaphores_.find(id);
    return it != semaphores_.end() ? std::optional(it->second) : std::nullopt;
}

// ---- Мьютексы (IPC-02) -----------------------------------------------------

MutexId IPCManager::createMutex(const std::string& name) {
    MutexId id = nextMutId_++;
    Mutex m;
    m.id = id;
    m.name = name;
    m.locked = false;
    m.owner = INVALID_PID;
    mutexes_[id] = std::move(m);
    recordOp("mutex_create", INVALID_PID, "name=" + name);
    return id;
}

bool IPCManager::mutexLock(MutexId id, Pid pid) {
    auto it = mutexes_.find(id);
    if (it == mutexes_.end()) return false;
    auto& mtx = it->second;

    if (!mtx.locked) {
        mtx.locked = true;
        mtx.owner = pid;
        mtx.totalLocks++;
        recordOp("mutex_lock", pid, "mtx=" + mtx.name);
        return true;
    }

    if (mtx.owner == pid) return true; // re-entrant

    mtx.waitingProcesses.push_back(pid);
    mtx.totalContention++;
    addWaitEdge(pid, id, "mutex");
    recordOp("mutex_lock_blocked", pid, "mtx=" + mtx.name + " owner=" + std::to_string(mtx.owner));

    Event evt(EventType::IpcBlocked, 0, "ipc");
    evt.with("pid", static_cast<int64_t>(pid)).with("resource", mtx.name);
    eventBus_.publish(evt);

    return false;
}

bool IPCManager::mutexUnlock(MutexId id, Pid pid) {
    auto it = mutexes_.find(id);
    if (it == mutexes_.end()) return false;
    auto& mtx = it->second;

    if (mtx.owner != pid) return false;

    if (!mtx.waitingProcesses.empty()) {
        Pid next = mtx.waitingProcesses.front();
        mtx.waitingProcesses.erase(mtx.waitingProcesses.begin());
        removeWaitEdge(next);
        mtx.owner = next;
        mtx.totalLocks++;
        recordOp("mutex_unlock_transfer", pid, "mtx=" + mtx.name + " to=" + std::to_string(next));

        Event evt(EventType::IpcUnblocked, 0, "ipc");
        evt.with("pid", static_cast<int64_t>(next));
        eventBus_.publish(evt);
    } else {
        mtx.locked = false;
        mtx.owner = INVALID_PID;
        recordOp("mutex_unlock", pid, "mtx=" + mtx.name);
    }
    return true;
}

void IPCManager::destroyMutex(MutexId id) {
    auto it = mutexes_.find(id);
    if (it != mutexes_.end()) {
        for (Pid p : it->second.waitingProcesses) removeWaitEdge(p);
        mutexes_.erase(it);
    }
}

std::optional<Mutex> IPCManager::getMutex(MutexId id) const {
    auto it = mutexes_.find(id);
    return it != mutexes_.end() ? std::optional(it->second) : std::nullopt;
}

// ---- Каналы (IPC-03) -------------------------------------------------------

PipeId IPCManager::createPipe(const std::string& name, size_t bufSize) {
    PipeId id = nextPipeId_++;
    Pipe p;
    p.id = id;
    p.name = name;
    p.bufferSize = bufSize;
    pipes_[id] = std::move(p);
    recordOp("pipe_create", INVALID_PID, "name=" + name);
    return id;
}

bool IPCManager::pipeWrite(PipeId id, Pid writer, const std::string& data) {
    auto it = pipes_.find(id);
    if (it == pipes_.end() || it->second.closed) return false;
    auto& pipe = it->second;

    if (pipe.buffer.size() + data.size() > pipe.bufferSize) {
        pipe.writerBlocked = writer;
        recordOp("pipe_write_blocked", writer, "pipe=" + pipe.name);
        return false;
    }

    pipe.buffer += data;
    pipe.totalBytesWritten += data.size();
    recordOp("pipe_write", writer, "pipe=" + pipe.name + " bytes=" + std::to_string(data.size()));

    // Разбудить читателя
    if (pipe.readerBlocked != INVALID_PID) {
        Pid reader = pipe.readerBlocked;
        pipe.readerBlocked = INVALID_PID;
        Event evt(EventType::IpcUnblocked, 0, "ipc");
        evt.with("pid", static_cast<int64_t>(reader));
        eventBus_.publish(evt);
    }
    return true;
}

std::optional<std::string> IPCManager::pipeRead(PipeId id, Pid reader, size_t maxBytes) {
    auto it = pipes_.find(id);
    if (it == pipes_.end()) return std::nullopt;
    auto& pipe = it->second;

    if (pipe.buffer.empty()) {
        if (pipe.closed) return std::string();
        pipe.readerBlocked = reader;
        recordOp("pipe_read_blocked", reader, "pipe=" + pipe.name);
        return std::nullopt;
    }

    size_t toRead = std::min(maxBytes, pipe.buffer.size());
    std::string data = pipe.buffer.substr(0, toRead);
    pipe.buffer.erase(0, toRead);
    pipe.totalBytesRead += toRead;
    recordOp("pipe_read", reader, "pipe=" + pipe.name + " bytes=" + std::to_string(toRead));

    // Разбудить писателя
    if (pipe.writerBlocked != INVALID_PID) {
        Pid w = pipe.writerBlocked;
        pipe.writerBlocked = INVALID_PID;
        Event evt(EventType::IpcUnblocked, 0, "ipc");
        evt.with("pid", static_cast<int64_t>(w));
        eventBus_.publish(evt);
    }
    return data;
}

void IPCManager::closePipe(PipeId id) {
    auto it = pipes_.find(id);
    if (it != pipes_.end()) it->second.closed = true;
}

void IPCManager::destroyPipe(PipeId id) { pipes_.erase(id); }

std::optional<Pipe> IPCManager::getPipe(PipeId id) const {
    auto it = pipes_.find(id);
    return it != pipes_.end() ? std::optional(it->second) : std::nullopt;
}

// ---- Очереди сообщений (IPC-04) --------------------------------------------

MsgQueueId IPCManager::createMessageQueue(const std::string& name, size_t maxMsgs) {
    MsgQueueId id = nextMqId_++;
    MessageQueue mq;
    mq.id = id;
    mq.name = name;
    mq.maxSize = maxMsgs;
    messageQueues_[id] = std::move(mq);
    return id;
}

bool IPCManager::sendMessage(MsgQueueId id, Pid sender, const std::string& data, int32_t prio) {
    auto it = messageQueues_.find(id);
    if (it == messageQueues_.end()) return false;
    auto& mq = it->second;

    if (mq.messages.size() >= mq.maxSize) return false;

    Message msg;
    msg.sender = sender;
    msg.data = data;
    msg.priority = prio;
    mq.messages.push_back(std::move(msg));
    mq.totalSent++;

    // Разбудить ожидающего получателя
    if (!mq.waitingReceivers.empty()) {
        Pid rcv = mq.waitingReceivers.front();
        mq.waitingReceivers.erase(mq.waitingReceivers.begin());
        Event evt(EventType::IpcUnblocked, 0, "ipc");
        evt.with("pid", static_cast<int64_t>(rcv));
        eventBus_.publish(evt);
    }
    return true;
}

std::optional<Message> IPCManager::receiveMessage(MsgQueueId id, Pid receiver) {
    auto it = messageQueues_.find(id);
    if (it == messageQueues_.end()) return std::nullopt;
    auto& mq = it->second;

    if (mq.messages.empty()) {
        mq.waitingReceivers.push_back(receiver);
        return std::nullopt;
    }

    Message msg = std::move(mq.messages.front());
    mq.messages.erase(mq.messages.begin());
    mq.totalReceived++;
    return msg;
}

void IPCManager::destroyMessageQueue(MsgQueueId id) { messageQueues_.erase(id); }

std::optional<MessageQueue> IPCManager::getMessageQueue(MsgQueueId id) const {
    auto it = messageQueues_.find(id);
    return it != messageQueues_.end() ? std::optional(it->second) : std::nullopt;
}

// ---- Обнаружение тупиков (IPC-05) ------------------------------------------

bool IPCManager::detectDeadlock() const {
    // DFS по графу ожиданий: найти цикл
    std::unordered_set<Pid> visited;
    std::unordered_set<Pid> inStack;

    for (const auto& [pid, _] : waitGraph_) {
        if (visited.find(pid) == visited.end()) {
            if (dfsDeadlock(pid, visited, inStack)) return true;
        }
    }
    return false;
}

std::vector<Pid> IPCManager::getDeadlockedProcesses() const {
    std::vector<Pid> result;
    std::unordered_set<Pid> visited;
    std::unordered_set<Pid> inStack;

    for (const auto& [pid, _] : waitGraph_) {
        if (visited.find(pid) == visited.end()) {
            std::vector<Pid> path;
            if (dfsDeadlockPath(pid, visited, inStack, path)) {
                result = path;
                break;
            }
        }
    }
    return result;
}

IPCManager::WaitGraphSnapshot IPCManager::getWaitGraph() const {
    WaitGraphSnapshot snap;
    for (const auto& [pid, edge] : waitGraph_) {
        snap.push_back({pid, edge.resourceId, edge.resourceType});
    }
    return snap;
}

// ---- Очистка ---------------------------------------------------------------

void IPCManager::cleanupProcess(Pid pid) {
    // Убрать из ожидания семафоров
    for (auto& [id, sem] : semaphores_) {
        auto& wq = sem.waitingProcesses;
        wq.erase(std::remove(wq.begin(), wq.end(), pid), wq.end());
    }
    // Освободить мьютексы
    for (auto& [id, mtx] : mutexes_) {
        if (mtx.owner == pid) {
            mutexUnlock(id, pid);
        }
        auto& wq = mtx.waitingProcesses;
        wq.erase(std::remove(wq.begin(), wq.end(), pid), wq.end());
    }
    // Каналы
    for (auto& [id, pipe] : pipes_) {
        if (pipe.readerBlocked == pid) pipe.readerBlocked = INVALID_PID;
        if (pipe.writerBlocked == pid) pipe.writerBlocked = INVALID_PID;
    }
    // Очереди
    for (auto& [id, mq] : messageQueues_) {
        auto& wr = mq.waitingReceivers;
        wr.erase(std::remove(wr.begin(), wr.end(), pid), wr.end());
    }
    removeWaitEdge(pid);
}

std::vector<IPCOperation> IPCManager::getHistory(size_t count) const {
    if (count == 0 || count >= history_.size()) return history_;
    return std::vector<IPCOperation>(history_.end() - static_cast<ptrdiff_t>(count), history_.end());
}

// ---- Внутренние ------------------------------------------------------------

void IPCManager::addWaitEdge(Pid pid, uint32_t resId, const std::string& type) {
    waitGraph_[pid] = {resId, type};
}

void IPCManager::removeWaitEdge(Pid pid) {
    waitGraph_.erase(pid);
}

bool IPCManager::dfsDeadlock(Pid pid, std::unordered_set<Pid>& visited,
                              std::unordered_set<Pid>& inStack) const {
    visited.insert(pid);
    inStack.insert(pid);

    auto it = waitGraph_.find(pid);
    if (it != waitGraph_.end()) {
        // Найти владельца ресурса
        Pid owner = findResourceOwner(it->second.resourceId, it->second.resourceType);
        if (owner != INVALID_PID) {
            if (inStack.find(owner) != inStack.end()) return true;
            if (visited.find(owner) == visited.end()) {
                if (dfsDeadlock(owner, visited, inStack)) return true;
            }
        }
    }
    inStack.erase(pid);
    return false;
}

bool IPCManager::dfsDeadlockPath(Pid pid, std::unordered_set<Pid>& visited,
                                   std::unordered_set<Pid>& inStack,
                                   std::vector<Pid>& path) const {
    visited.insert(pid);
    inStack.insert(pid);
    path.push_back(pid);

    auto it = waitGraph_.find(pid);
    if (it != waitGraph_.end()) {
        Pid owner = findResourceOwner(it->second.resourceId, it->second.resourceType);
        if (owner != INVALID_PID) {
            if (inStack.find(owner) != inStack.end()) {
                path.push_back(owner);
                return true;
            }
            if (visited.find(owner) == visited.end()) {
                if (dfsDeadlockPath(owner, visited, inStack, path)) return true;
            }
        }
    }
    inStack.erase(pid);
    path.pop_back();
    return false;
}

Pid IPCManager::findResourceOwner(uint32_t resId, const std::string& type) const {
    if (type == "mutex") {
        auto it = mutexes_.find(resId);
        if (it != mutexes_.end() && it->second.locked) return it->second.owner;
    }
    return INVALID_PID;
}

void IPCManager::recordOp(const std::string& op, Pid pid, const std::string& detail) {
    history_.push_back({op, pid, detail, 0});
    if (history_.size() > 10000) {
        history_.erase(history_.begin(), history_.begin() + 5000);
    }
}

} // namespace re36

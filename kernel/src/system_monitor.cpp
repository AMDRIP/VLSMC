// Не используется, не удалять
/**
 * @file system_monitor.cpp
 * @brief Реализация мониторинга оборудования RAND Elecorner 36.
 */

#include "kernel/system_monitor.h"
#include "kernel/scheduler.h"
#include "kernel/memory_manager.h"
#include "kernel/filesystem.h"
#include "kernel/ipc.h"
#include "kernel/io_manager.h"
#include "kernel/user_manager.h"
#include "kernel/logger.h"
#include "kernel/event_bus.h"

#include <algorithm>
#include <cmath>

namespace re36 {

SystemMonitor::SystemMonitor() = default;
SystemMonitor::~SystemMonitor() = default;

// ============================================================================
// Инициализация
// ============================================================================

void SystemMonitor::init(Scheduler& scheduler, MemoryManager& memory,
                          FileSystem& fs, IPCManager& ipc, IOManager& io,
                          UserManager& users, Logger& logger, EventBus& eventBus) {
    scheduler_     = &scheduler;
    memoryManager_ = &memory;
    fileSystem_    = &fs;
    ipcManager_    = &ipc;
    ioManager_     = &io;
    userManager_   = &users;
    logger_        = &logger;
    eventBus_      = &eventBus;

    history_.clear();
    alerts_.clear();
    current_ = {};
    previous_ = {};
    prevContextSwitches_ = 0;
    prevPageFaults_ = 0;
    prevInterrupts_ = 0;
    prevDiskOps_ = 0;
}

// ============================================================================
// Сбор метрик
// ============================================================================

void SystemMonitor::collect(Tick currentTick) {
    if (!scheduler_) return;

    // Пропуск по интервалу
    if (collectInterval_ > 1 && currentTick % collectInterval_ != 0) return;

    previous_ = current_;

    current_.tick      = currentTick;
    current_.cpu       = collectCpu();
    current_.memory    = collectMemory();
    current_.disk      = collectDisk();
    current_.io        = collectIo();
    current_.ipc       = collectIpc();
    current_.processes = collectProcesses();
    current_.users     = collectUsers();

    // Добавить в историю
    history_.push_back(current_);
    if (history_.size() > maxHistory_) {
        history_.pop_front();
    }

    // Проверить пороги
    checkThresholds(currentTick);
}

// ============================================================================
// Текущее состояние
// ============================================================================

const SystemSnapshot& SystemMonitor::getCurrentSnapshot() const {
    return current_;
}

double SystemMonitor::getCpuUsage() const { return current_.cpu.usagePercent; }
double SystemMonitor::getMemoryUsage() const { return current_.memory.usagePercent; }
double SystemMonitor::getDiskUsage() const { return current_.disk.usagePercent; }

AlertLevel SystemMonitor::getCpuAlertLevel() const {
    if (current_.cpu.usagePercent >= thresholds_.cpuCritical) return AlertLevel::Critical;
    if (current_.cpu.usagePercent >= thresholds_.cpuWarning) return AlertLevel::Warning;
    return AlertLevel::Normal;
}

AlertLevel SystemMonitor::getMemoryAlertLevel() const {
    if (current_.memory.usagePercent >= thresholds_.memoryCritical) return AlertLevel::Critical;
    if (current_.memory.usagePercent >= thresholds_.memoryWarning) return AlertLevel::Warning;
    return AlertLevel::Normal;
}

AlertLevel SystemMonitor::getDiskAlertLevel() const {
    if (current_.disk.usagePercent >= thresholds_.diskCritical) return AlertLevel::Critical;
    if (current_.disk.usagePercent >= thresholds_.diskWarning) return AlertLevel::Warning;
    return AlertLevel::Normal;
}

// ============================================================================
// История
// ============================================================================

const std::deque<SystemSnapshot>& SystemMonitor::getHistory() const {
    return history_;
}

std::vector<SystemSnapshot> SystemMonitor::getRecentHistory(size_t count) const {
    if (count == 0 || count >= history_.size()) {
        return {history_.begin(), history_.end()};
    }
    return {history_.end() - static_cast<ptrdiff_t>(count), history_.end()};
}

std::vector<CpuSnapshot> SystemMonitor::getCpuHistory(size_t count) const {
    std::vector<CpuSnapshot> result;
    size_t start = (count == 0 || count >= history_.size())
                       ? 0 : history_.size() - count;
    for (size_t i = start; i < history_.size(); ++i) {
        result.push_back(history_[i].cpu);
    }
    return result;
}

std::vector<MemorySnapshot> SystemMonitor::getMemoryHistory(size_t count) const {
    std::vector<MemorySnapshot> result;
    size_t start = (count == 0 || count >= history_.size())
                       ? 0 : history_.size() - count;
    for (size_t i = start; i < history_.size(); ++i) {
        result.push_back(history_[i].memory);
    }
    return result;
}

std::vector<DiskSnapshot> SystemMonitor::getDiskHistory(size_t count) const {
    std::vector<DiskSnapshot> result;
    size_t start = (count == 0 || count >= history_.size())
                       ? 0 : history_.size() - count;
    for (size_t i = start; i < history_.size(); ++i) {
        result.push_back(history_[i].disk);
    }
    return result;
}

std::vector<IoSnapshot> SystemMonitor::getIoHistory(size_t count) const {
    std::vector<IoSnapshot> result;
    size_t start = (count == 0 || count >= history_.size())
                       ? 0 : history_.size() - count;
    for (size_t i = start; i < history_.size(); ++i) {
        result.push_back(history_[i].io);
    }
    return result;
}

// ============================================================================
// Пороги
// ============================================================================

void SystemMonitor::setThresholds(const AlertThresholds& thresholds) {
    thresholds_ = thresholds;
}

const AlertThresholds& SystemMonitor::getThresholds() const {
    return thresholds_;
}

std::vector<SystemAlert> SystemMonitor::getActiveAlerts() const {
    // Вернуть только актуальные (за последние 10 тиков)
    std::vector<SystemAlert> active;
    Tick cutoff = current_.tick > 10 ? current_.tick - 10 : 0;
    for (auto it = alerts_.rbegin(); it != alerts_.rend(); ++it) {
        if (it->tick < cutoff) break;
        active.push_back(*it);
    }
    std::reverse(active.begin(), active.end());
    return active;
}

std::vector<SystemAlert> SystemMonitor::getAlertHistory(size_t count) const {
    if (count == 0 || count >= alerts_.size()) return alerts_;
    return {alerts_.end() - static_cast<ptrdiff_t>(count), alerts_.end()};
}

// ============================================================================
// Настройки
// ============================================================================

void SystemMonitor::setMaxHistory(size_t maxSnapshots) {
    maxHistory_ = std::max<size_t>(10, maxSnapshots);
    while (history_.size() > maxHistory_) history_.pop_front();
}

size_t SystemMonitor::getMaxHistory() const { return maxHistory_; }

void SystemMonitor::setCollectInterval(uint32_t interval) {
    collectInterval_ = std::max<uint32_t>(1, interval);
}

// ============================================================================
// Внутренние методы сбора
// ============================================================================

CpuSnapshot SystemMonitor::collectCpu() {
    CpuSnapshot snap;

    auto processes = scheduler_->getProcessList();
    bool cpuBusy = false;

    for (const auto& p : processes) {
        switch (p.state) {
            case ProcessState::Running:
                snap.runningCount++;
                cpuBusy = true;
                break;
            case ProcessState::Ready:
                snap.readyCount++;
                break;
            case ProcessState::Blocked:
                snap.blockedCount++;
                break;
            default:
                break;
        }
    }

    // CPU usage: 100% если есть Running процесс (не idle), 0% если idle
    auto currentPid = scheduler_->getCurrentPid();
    bool isIdle = (currentPid == INVALID_PID);
    // Проверяем, не является ли текущий процесс idle
    if (!isIdle) {
        for (const auto& p : processes) {
            if (p.pid == currentPid && p.name == "idle") {
                isIdle = true;
                break;
            }
        }
    }

    snap.usagePercent = isIdle ? 0.0 : 100.0;
    if (!isIdle) snap.idleTicks = previous_.cpu.idleTicks;
    else snap.idleTicks = previous_.cpu.idleTicks + 1;

    // EMA сглаживание
    snap.avgUsagePercent = emaAlpha_ * snap.usagePercent +
                           (1.0 - emaAlpha_) * previous_.cpu.avgUsagePercent;

    // Context switches (delta)
    snap.contextSwitches = scheduler_->getContextSwitchCount();
    snap.contextSwitchesPerTick = static_cast<uint32_t>(
        snap.contextSwitches - prevContextSwitches_);
    prevContextSwitches_ = snap.contextSwitches;

    return snap;
}

MemorySnapshot SystemMonitor::collectMemory() {
    MemorySnapshot snap;

    auto memSnap = memoryManager_->getSnapshot();
    snap.totalFrames = memSnap.totalFrames;
    snap.usedFrames = memSnap.usedFrames;
    snap.freeFrames = memSnap.totalFrames - memSnap.usedFrames;
    snap.usagePercent = memSnap.totalFrames > 0
        ? (100.0 * memSnap.usedFrames / memSnap.totalFrames)
        : 0.0;

    snap.totalBytes = memSnap.totalFrames * 4096; // 4KB фреймы
    snap.usedBytes = memSnap.usedFrames * 4096;

    // Page faults (delta)
    snap.totalPageFaults = memSnap.totalPageFaults;
    snap.pageFaultsPerTick = static_cast<uint32_t>(
        snap.totalPageFaults - prevPageFaults_);
    prevPageFaults_ = snap.totalPageFaults;

    snap.fragmentationPercent = memSnap.fragmentationPercent;

    return snap;
}

DiskSnapshot SystemMonitor::collectDisk() {
    DiskSnapshot snap;

    auto fsSnap = fileSystem_->getSnapshot();
    snap.totalBlocks = fsSnap.totalBlocks;
    snap.usedBlocks = fsSnap.usedBlocks;
    snap.freeBlocks = fsSnap.totalBlocks - fsSnap.usedBlocks;
    snap.usagePercent = fsSnap.totalBlocks > 0
        ? (100.0 * fsSnap.usedBlocks / fsSnap.totalBlocks)
        : 0.0;
    snap.totalFiles = fsSnap.totalFiles;
    snap.totalDirectories = fsSnap.totalDirectories;

    snap.totalOps = fsSnap.totalOps;
    snap.opsPerTick = static_cast<uint32_t>(snap.totalOps - prevDiskOps_);
    prevDiskOps_ = snap.totalOps;

    snap.bytesReadTotal = fsSnap.bytesRead;
    snap.bytesWrittenTotal = fsSnap.bytesWritten;

    return snap;
}

IoSnapshot SystemMonitor::collectIo() {
    IoSnapshot snap;

    auto allDevices = ioManager_->getAllDevices();
    snap.totalDevices = static_cast<uint32_t>(allDevices.size());

    for (const auto& dev : allDevices) {
        DeviceMetrics dm;
        dm.name = dev.name;
        dm.queueLength = static_cast<uint32_t>(dev.requestQueue.size());
        dm.totalOps = dev.totalOps;
        dm.totalBytes = dev.totalBytesTransferred;

        // Тип
        switch (dev.type) {
            case DeviceType::Disk:     dm.type = "Диск"; break;
            case DeviceType::Keyboard: dm.type = "Клавиатура"; break;
            case DeviceType::Display:  dm.type = "Дисплей"; break;
            case DeviceType::Network:  dm.type = "Сеть"; break;
            default:                   dm.type = "Другое"; break;
        }

        // Состояние
        switch (dev.state) {
            case DeviceState::Idle:  dm.state = "Простой"; break;
            case DeviceState::Busy:  dm.state = "Занят"; snap.activeDevices++; break;
            case DeviceState::Error: dm.state = "Ошибка"; break;
            default:                 dm.state = "?"; break;
        }

        snap.pendingRequests += dm.queueLength;
        snap.devices.push_back(std::move(dm));
    }

    snap.totalInterrupts = ioManager_->getTotalInterrupts();
    snap.interruptsPerTick = static_cast<uint32_t>(
        snap.totalInterrupts - prevInterrupts_);
    prevInterrupts_ = snap.totalInterrupts;

    return snap;
}

IpcSnapshot SystemMonitor::collectIpc() {
    IpcSnapshot snap;

    // Подсчитываем через граф ожиданий
    auto waitGraph = ipcManager_->getWaitGraph();
    snap.blockedProcesses = static_cast<uint32_t>(waitGraph.size());

    snap.deadlockDetected = ipcManager_->detectDeadlock();
    if (snap.deadlockDetected) {
        auto deadlocked = ipcManager_->getDeadlockedProcesses();
        snap.deadlockedCount = static_cast<uint32_t>(deadlocked.size());
    }

    // Подсчёт активных ресурсов — через getHistory (последняя операция)
    // В реальной реализации у IPCManager были бы отдельные count-методы
    // Пока используем снимки; TODO: добавить getActiveCounts() в IPC
    auto history = ipcManager_->getHistory(0);
    for (const auto& op : history) {
        if (op.operation.find("sem_create") != std::string::npos) snap.activeSemaphores++;
        if (op.operation.find("mutex_create") != std::string::npos) snap.activeMutexes++;
        if (op.operation.find("pipe_create") != std::string::npos) snap.activePipes++;
    }

    return snap;
}

ProcessSnapshot SystemMonitor::collectProcesses() {
    ProcessSnapshot snap;

    auto processes = scheduler_->getProcessList();
    snap.total = static_cast<uint32_t>(processes.size());

    for (const auto& p : processes) {
        switch (p.state) {
            case ProcessState::Running:    snap.running++; break;
            case ProcessState::Ready:      snap.ready++; break;
            case ProcessState::Blocked:    snap.blocked++; break;
            case ProcessState::Suspended:  snap.suspended++; break;
            case ProcessState::Terminated: snap.terminated++; break;
            default: break;
        }
    }

    snap.created = snap.total; // общее число созданных за сессию

    return snap;
}

UserSnapshot SystemMonitor::collectUsers() {
    UserSnapshot snap;

    auto accounts = userManager_->getAllAccounts();
    snap.totalAccounts = static_cast<uint32_t>(accounts.size());

    auto sessions = userManager_->getActiveSessions();
    snap.activeSessions = static_cast<uint32_t>(sessions.size());

    auto audit = userManager_->getAuditLog(0);
    snap.auditEntries = static_cast<uint32_t>(audit.size());

    // Подсчёт неудачных попыток
    for (const auto& acc : accounts) {
        snap.failedLogins += acc.failedLogins;
    }

    return snap;
}

// ============================================================================
// Проверка порогов
// ============================================================================

void SystemMonitor::checkThresholds(Tick tick) {
    // CPU
    if (current_.cpu.avgUsagePercent >= thresholds_.cpuCritical) {
        addAlert(tick, "cpu", AlertLevel::Critical,
                 "Критическая загрузка CPU: " +
                 std::to_string(static_cast<int>(current_.cpu.avgUsagePercent)) + "%",
                 current_.cpu.avgUsagePercent, thresholds_.cpuCritical);
    } else if (current_.cpu.avgUsagePercent >= thresholds_.cpuWarning) {
        addAlert(tick, "cpu", AlertLevel::Warning,
                 "Высокая загрузка CPU: " +
                 std::to_string(static_cast<int>(current_.cpu.avgUsagePercent)) + "%",
                 current_.cpu.avgUsagePercent, thresholds_.cpuWarning);
    }

    // Memory
    if (current_.memory.usagePercent >= thresholds_.memoryCritical) {
        addAlert(tick, "memory", AlertLevel::Critical,
                 "Критический уровень памяти: " +
                 std::to_string(static_cast<int>(current_.memory.usagePercent)) + "%",
                 current_.memory.usagePercent, thresholds_.memoryCritical);
    } else if (current_.memory.usagePercent >= thresholds_.memoryWarning) {
        addAlert(tick, "memory", AlertLevel::Warning,
                 "Высокий уровень памяти: " +
                 std::to_string(static_cast<int>(current_.memory.usagePercent)) + "%",
                 current_.memory.usagePercent, thresholds_.memoryWarning);
    }

    // Disk
    if (current_.disk.usagePercent >= thresholds_.diskCritical) {
        addAlert(tick, "disk", AlertLevel::Critical,
                 "Критический уровень диска: " +
                 std::to_string(static_cast<int>(current_.disk.usagePercent)) + "%",
                 current_.disk.usagePercent, thresholds_.diskCritical);
    } else if (current_.disk.usagePercent >= thresholds_.diskWarning) {
        addAlert(tick, "disk", AlertLevel::Warning,
                 "Высокий уровень диска: " +
                 std::to_string(static_cast<int>(current_.disk.usagePercent)) + "%",
                 current_.disk.usagePercent, thresholds_.diskWarning);
    }

    // IO queue
    if (current_.io.pendingRequests >= thresholds_.ioQueueWarning) {
        addAlert(tick, "io", AlertLevel::Warning,
                 "Большая очередь IO: " + std::to_string(current_.io.pendingRequests),
                 current_.io.pendingRequests, thresholds_.ioQueueWarning);
    }

    // Process limit
    uint32_t active = current_.processes.running + current_.processes.ready +
                      current_.processes.blocked + current_.processes.suspended;
    if (active >= thresholds_.processLimit) {
        addAlert(tick, "process", AlertLevel::Warning,
                 "Лимит процессов: " + std::to_string(active) + "/" +
                 std::to_string(thresholds_.processLimit),
                 active, thresholds_.processLimit);
    }

    // Deadlock
    if (current_.ipc.deadlockDetected) {
        addAlert(tick, "ipc", AlertLevel::Critical,
                 "Обнаружен тупик! " + std::to_string(current_.ipc.deadlockedCount) +
                 " процессов заблокированы",
                 current_.ipc.deadlockedCount, 0);
    }

    // Page Fault storm
    if (current_.memory.pageFaultsPerTick > 10) {
        addAlert(tick, "memory", AlertLevel::Warning,
                 "Шторм page faults: " + std::to_string(current_.memory.pageFaultsPerTick) + "/тик",
                 current_.memory.pageFaultsPerTick, 10);
    }
}

void SystemMonitor::addAlert(Tick tick, const std::string& subsystem,
                              AlertLevel level, const std::string& msg,
                              double value, double threshold) {
    // Не дублировать однотипные алерты каждый тик
    if (!alerts_.empty()) {
        const auto& last = alerts_.back();
        if (last.subsystem == subsystem && last.level == level &&
            tick - last.tick < 5) {
            return; // подавить повтор в течение 5 тиков
        }
    }

    alerts_.push_back({tick, subsystem, level, msg, value, threshold});
    if (alerts_.size() > maxAlerts_) {
        alerts_.erase(alerts_.begin(), alerts_.begin() +
                      static_cast<ptrdiff_t>(maxAlerts_ / 2));
    }

    // Логировать
    if (logger_) {
        auto logLevel = (level == AlertLevel::Critical)
                            ? LogLevel::Error : LogLevel::Warning;
        logger_->log(logLevel, "monitor", msg, "kernel");
    }
}

} // namespace re36

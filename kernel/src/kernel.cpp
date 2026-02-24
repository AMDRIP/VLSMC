/**
 * @file kernel.cpp
 * @brief Реализация центрального диспетчера ядра RAND Elecorner 36.
 */

#include "kernel/kernel.h"
#include "kernel/logger.h"
#include "kernel/system_monitor.h"
#include "kernel/input_manager.h"
#include "kernel/kbd_driver.h"
#include "kernel/console_driver.h"
#include "kernel/vga_driver.h"

#include <algorithm>
#include <chrono>

namespace re36 {

Kernel::Kernel()
    : eventBus_(std::make_unique<EventBus>()),
      config_(KernelConfig{}) {}

Kernel::Kernel(const KernelConfig& config)
    : eventBus_(std::make_unique<EventBus>()),
      config_(config) {}

Kernel::~Kernel() {
    if (state_ == KernelState::Running || state_ == KernelState::Paused) {
        shutdown();
    }
}

// ============================================================================
// Жизненный цикл
// ============================================================================

bool Kernel::boot() {
    if (state_ != KernelState::Off && state_ != KernelState::ShutDown) {
        return false;
    }

    state_ = KernelState::Booting;
    currentTick_ = 0;

    logMessage("Ядро RAND Elecorner 36 — загрузка...");

    // Logger создаётся первым (до подсистем) — все могут логировать
    logger_ = std::make_unique<Logger>();
    if (!logger_->init()) { logMessage("ОШИБКА: Логгер"); return false; }
    logMessage("  ✓ Логгер (6 каналов)");

    // Создать подсистемы
    scheduler_     = std::make_unique<Scheduler>(*eventBus_, config_);
    memoryManager_ = std::make_unique<MemoryManager>(*eventBus_, config_);
    fileSystem_    = std::make_unique<FileSystem>(*eventBus_, config_);
    ipcManager_    = std::make_unique<IPCManager>(*eventBus_, config_);
    ioManager_     = std::make_unique<IOManager>(*eventBus_, config_);
    userManager_   = std::make_unique<UserManager>(*eventBus_, config_);
    inputManager_  = std::make_unique<InputManager>(*eventBus_);
    kbdDriver_     = std::make_unique<KeyboardDriver>(*eventBus_, *inputManager_);
    consoleDriver_ = std::make_unique<ConsoleDriver>(*eventBus_, *inputManager_, *scheduler_);
    vgaDriver_     = std::make_unique<VgaDriver>(*eventBus_);

    // Инициализировать в порядке зависимостей
    if (!scheduler_->init()) { logMessage("ОШИБКА: Планировщик"); return false; }
    logMessage("  ✓ Планировщик CPU");

    if (!memoryManager_->init()) { logMessage("ОШИБКА: Память"); return false; }
    logMessage("  ✓ Менеджер памяти");

    if (!fileSystem_->init()) { logMessage("ОШИБКА: ФС"); return false; }
    logMessage("  ✓ Файловая система");

    if (!ipcManager_->init()) { logMessage("ОШИБКА: IPC"); return false; }
    logMessage("  ✓ Межпроцессное взаимодействие");

    if (!ioManager_->init()) { logMessage("ОШИБКА: IO"); return false; }
    logMessage("  ✓ Менеджер устройств");

    if (!userManager_->init()) { logMessage("ОШИБКА: Пользователи"); return false; }
    logMessage("  ✓ Менеджер пользователей");

    if (!inputManager_->init()) { logMessage("ОШИБКА: Ввод"); return false; }
    logMessage("  ✓ Подсистема ввода (InputManager)");

    if (!kbdDriver_->init()) { logMessage("ОШИБКА: Клавиатура"); return false; }
    logMessage("  ✓ Драйвер клавиатуры (IRQ 1)");

    if (!consoleDriver_->init()) { logMessage("ОШИБКА: Консоль"); return false; }
    logMessage("  ✓ Драйвер системной консоли (TTY0)");

    if (!vgaDriver_->init()) { logMessage("ОШИБКА: VGA"); return false; }
    logMessage("  ✓ Драйвер VGA (80x25)");

    // AppRuntime зависит от FileSystem и Scheduler
    appRuntime_ = std::make_unique<AppRuntime>(*eventBus_, *fileSystem_, *scheduler_, config_);
    if (!appRuntime_->init()) { logMessage("ОШИБКА: AppRuntime"); return false; }
    appRuntime_->setKernel(*this);
    logMessage("  ✓ Среда приложений (+ App API)");

    // SystemMonitor создаётся последним — ему нужны все подсистемы
    systemMonitor_ = std::make_unique<SystemMonitor>();
    systemMonitor_->init(*scheduler_, *memoryManager_, *fileSystem_,
                         *ipcManager_, *ioManager_, *userManager_,
                         *logger_, *eventBus_);
    logMessage("  ✓ Монитор оборудования");

    // Прерывания I/O → пробуждение процессов
    ioManager_->registerInterruptCallback(
        [this](const std::string& /*dev*/, Pid pid) {
            scheduler_->onIoComplete(pid);
        }
    );

    // Подписка на завершение процессов → очистка IPC / IO / AppRuntime
    eventBus_->subscribe(EventType::ProcessTerminated,
        [this](const Event& evt) {
            Pid pid = static_cast<Pid>(evt.getInt("pid", INVALID_PID));
            if (pid != INVALID_PID) {
                ipcManager_->cleanupProcess(pid);
                ioManager_->cancelProcessRequests(pid);
                appRuntime_->onProcessTerminated(pid);
                memoryManager_->deallocate(pid);
            }
        }
    );

    // Системный процесс (idle)
    scheduler_->createProcess("idle", 999999, 255, INVALID_PID, ROOT_UID);

    state_ = KernelState::Running;
    logMessage("Ядро загружено. Тик #0");
    logger_->security("kernel_boot", ROOT_UID, "Ядро RAND Elecorner 36 загружено");

    Event evt(EventType::KernelBooted, 0, "kernel");
    eventBus_->publish(evt);

    if (onBoot_) onBoot_();

    return true;
}

void Kernel::tick() {
    if (state_ != KernelState::Running) return;

    currentTick_++;

    // Синхронизировать тик логгера (сброс rate-limiter)
    logger_->setCurrentTick(currentTick_);

    // 1. Планировщик — выбрать процесс
    scheduler_->schedule();

    // 2. Выполнить инструкцию текущего процесса
    auto instrType = scheduler_->executeCurrentProcess();

    // 3. Обработать инструкцию (взаимодействие с подсистемами)
    if (instrType.has_value()) {
        handleInstruction(*instrType, scheduler_->getCurrentPid());
    }

    // 4. Обработать I/O устройства
    ioManager_->tick();

    // 4.5. Обработать клавиатуру
    kbdDriver_->tick(currentTick_);

    // 4.6. Обработать консоль
    consoleDriver_->tick(currentTick_);

    // 5. Обработать страничные ошибки
    memoryManager_->checkPageFaults();

    // 6. Обработать отложенные события
    eventBus_->processEvents();

    // 7. Собрать метрики оборудования
    systemMonitor_->collect(currentTick_);

    // Оповестить GUI
    if (onTick_) onTick_(currentTick_);
}

void Kernel::shutdown() {
    if (state_ == KernelState::Off || state_ == KernelState::ShutDown) return;

    state_ = KernelState::ShuttingDown;
    logMessage("Завершение работы...");

    // Остановить приложения
    if (appRuntime_) appRuntime_->stopAllApps();

    // Завершить процессы
    if (scheduler_) {
        auto list = scheduler_->getProcessList();
        for (auto& snap : list) {
            if (snap.state != ProcessState::Terminated) {
                scheduler_->killProcess(snap.pid, -1);
            }
        }
    }

    state_ = KernelState::ShutDown;
    logMessage("Ядро остановлено.");
    if (logger_) logger_->security("kernel_shutdown", ROOT_UID, "Ядро остановлено");

    Event evt(EventType::KernelShutdown, currentTick_, "kernel");
    eventBus_->publish(evt);

    if (onShutdown_) onShutdown_();
}

// ============================================================================
// Пауза / управление скоростью
// ============================================================================

void Kernel::pause() {
    if (state_ == KernelState::Running) {
        state_ = KernelState::Paused;
        if (onPauseResume_) onPauseResume_(true);
    }
}

void Kernel::resume() {
    if (state_ == KernelState::Paused) {
        state_ = KernelState::Running;
        if (onPauseResume_) onPauseResume_(false);
    }
}

void Kernel::step() {
    if (state_ == KernelState::Paused) {
        KernelState saved = state_;
        state_ = KernelState::Running;
        tick();
        state_ = saved;
    }
}

void Kernel::setSpeedMultiplier(double mult) {
    speedMultiplier_ = std::max(0.1, std::min(mult, 100.0));
}

double Kernel::getSpeedMultiplier() const {
    return speedMultiplier_;
}

// ============================================================================
// Системные вызовы
// ============================================================================

SyscallResult Kernel::syscall(SyscallId id, const SyscallArgs& args) {
    totalSyscalls_++;

    Event evt(EventType::SyscallInvoked, currentTick_, "kernel");
    evt.with("syscallId", static_cast<int64_t>(id));
    eventBus_->publish(evt);

    // Маршрутизация по диапазонам
    if (id >= 100 && id < 200) return handleProcessSyscall(id, args);
    if (id >= 200 && id < 300) return handleMemorySyscall(id, args);
    if (id >= 300 && id < 400) return handleFsSyscall(id, args);
    if (id >= 400 && id < 500) return handleIoSyscall(id, args);
    if (id >= 500 && id < 600) return handleIpcSyscall(id, args);
    if (id >= 600 && id < 700) return handleUserSyscall(id, args);
    if (id >= 700 && id < 800) return handleAppSyscall(id, args);
    if (id >= 800 && id < 900) return handleVgaSyscall(id, args);
    if (id >= 900 && id < 1000) return handleSystemSyscall(id, args);

    return SyscallResult::error(SyscallStatus::InvalidSyscall,
                                "Неизвестный syscall: " + std::to_string(id));
}

// ============================================================================
// Доступ к подсистемам
// ============================================================================

KernelState Kernel::getState() const { return state_; }
Tick Kernel::getCurrentTick() const { return currentTick_; }
EventBus& Kernel::getEventBus() { return *eventBus_; }
Scheduler& Kernel::getScheduler() { return *scheduler_; }
MemoryManager& Kernel::getMemoryManager() { return *memoryManager_; }
FileSystem& Kernel::getFileSystem() { return *fileSystem_; }
IPCManager& Kernel::getIPCManager() { return *ipcManager_; }
IOManager& Kernel::getIOManager() { return *ioManager_; }
UserManager& Kernel::getUserManager() { return *userManager_; }
AppRuntime& Kernel::getAppRuntime() { return *appRuntime_; }
Logger& Kernel::getLogger() { return *logger_; }
SystemMonitor& Kernel::getSystemMonitor() { return *systemMonitor_; }
InputManager& Kernel::getInputManager() { return *inputManager_; }
KeyboardDriver& Kernel::getKeyboardDriver() { return *kbdDriver_; }
ConsoleDriver& Kernel::getConsoleDriver() { return *consoleDriver_; }
VgaDriver& Kernel::getVgaDriver() { return *vgaDriver_; }
uint64_t Kernel::getTotalSyscalls() const { return totalSyscalls_; }

const std::vector<std::string>& Kernel::getBootLog() const { return bootLog_; }

// ============================================================================
// Callbacks
// ============================================================================

void Kernel::setOnBootCallback(std::function<void()> cb) { onBoot_ = std::move(cb); }
void Kernel::setOnShutdownCallback(std::function<void()> cb) { onShutdown_ = std::move(cb); }
void Kernel::setOnTickCallback(std::function<void(Tick)> cb) { onTick_ = std::move(cb); }
void Kernel::setOnPauseResumeCallback(std::function<void(bool)> cb) { onPauseResume_ = std::move(cb); }

// ============================================================================
// Обработка системных вызовов по подсистемам
// ============================================================================

SyscallResult Kernel::handleProcessSyscall(SyscallId id, const SyscallArgs& args) {
    switch (id) {
        case SYS_PROC_CREATE: {
            auto name = args.getString("name", "process");
            auto burst = static_cast<uint32_t>(args.getInt("burst", 10));
            auto prio = static_cast<uint8_t>(args.getInt("priority", 5));
            auto owner = static_cast<Uid>(args.getInt("owner", ROOT_UID));
            Pid pid = scheduler_->createProcess(name, burst, prio, INVALID_PID, owner);
            if (pid == INVALID_PID)
                return SyscallResult::error(SyscallStatus::ResourceLimit, "Лимит процессов");
            return SyscallResult::ok("pid", static_cast<int64_t>(pid));
        }
        case SYS_PROC_EXIT:
        case SYS_PROC_KILL: {
            auto pid = static_cast<Pid>(args.getInt("pid", INVALID_PID));
            auto code = static_cast<int32_t>(args.getInt("code", 0));
            if (!scheduler_->killProcess(pid, code))
                return SyscallResult::error(SyscallStatus::NotFound, "Процесс не найден");
            return SyscallResult::ok();
        }
        case SYS_PROC_LIST: {
            auto list = scheduler_->getProcessList();
            return SyscallResult::ok("count", static_cast<int64_t>(list.size()));
        }
        case SYS_PROC_SET_PRIORITY: {
            // Через scheduler — не реализовано отдельно, TODO
            return SyscallResult::ok();
        }
        default:
            return SyscallResult::error(SyscallStatus::InvalidSyscall, "process");
    }
}

SyscallResult Kernel::handleMemorySyscall(SyscallId id, const SyscallArgs& args) {
    switch (id) {
        case SYS_MEM_ALLOC: {
            auto pid = static_cast<Pid>(args.getInt("pid", 0));
            auto size = static_cast<size_t>(args.getInt("size", 4096));
            auto addr = memoryManager_->allocate(pid, size);
            if (!addr) return SyscallResult::error(SyscallStatus::OutOfMemory, "Нет памяти");
            return SyscallResult::ok("addr", static_cast<int64_t>(*addr));
        }
        case SYS_MEM_FREE: {
            auto pid = static_cast<Pid>(args.getInt("pid", 0));
            memoryManager_->deallocate(pid);
            return SyscallResult::ok();
        }
        case SYS_MEM_ACCESS: {
            auto pid = static_cast<Pid>(args.getInt("pid", 0));
            auto vaddr = static_cast<VirtualAddr>(args.getUint("addr", 0));
            auto write = args.getBool("write", false);
            auto paddr = memoryManager_->accessMemory(pid, vaddr, write);
            if (!paddr) return SyscallResult::error(SyscallStatus::InvalidAddress, "Ошибка доступа");
            return SyscallResult::ok("paddr", static_cast<int64_t>(*paddr));
        }
        case SYS_MEM_SWAP_OUT: {
            auto pid = static_cast<Pid>(args.getInt("pid", 0));
            auto page = static_cast<PageNumber>(args.getInt("page", 0));
            return memoryManager_->swapOutPage(pid, page)
                ? SyscallResult::ok()
                : SyscallResult::error(SyscallStatus::ResourceLimit, "Swap out не удался");
        }
        case SYS_MEM_SWAP_IN: {
            auto pid = static_cast<Pid>(args.getInt("pid", 0));
            auto page = static_cast<PageNumber>(args.getInt("page", 0));
            return memoryManager_->swapInPage(pid, page)
                ? SyscallResult::ok()
                : SyscallResult::error(SyscallStatus::NotFound, "Страница не в swap");
        }
        case SYS_MEM_SWAP_ENABLE: {
            memoryManager_->setSwapEnabled(true);
            return SyscallResult::ok();
        }
        case SYS_MEM_SWAP_DISABLE: {
            memoryManager_->setSwapEnabled(false);
            return SyscallResult::ok();
        }
        case SYS_MEM_SWAP_STATS: {
            auto stats = memoryManager_->getSwapStats();
            return SyscallResult::ok("usedSlots", static_cast<int64_t>(stats.usedSlots));
        }
        default:
            return SyscallResult::error(SyscallStatus::InvalidSyscall, "memory");
    }
}

SyscallResult Kernel::handleFsSyscall(SyscallId id, const SyscallArgs& args) {
    auto uid = static_cast<Uid>(args.getInt("uid", ROOT_UID));
    auto path = args.getString("path");

    switch (id) {
        case SYS_FS_CREATE:
            return fileSystem_->createFile(path, uid)
                ? SyscallResult::ok()
                : SyscallResult::error(SyscallStatus::AlreadyExists, path);
        case SYS_FS_READ: {
            auto content = fileSystem_->readFile(path, uid);
            if (!content) return SyscallResult::error(SyscallStatus::NotFound, path);
            return SyscallResult::ok("content", *content);
        }
        case SYS_FS_WRITE:
            return fileSystem_->writeFile(path, args.getString("content"), uid)
                ? SyscallResult::ok()
                : SyscallResult::error(SyscallStatus::PermissionDenied, path);
        case SYS_FS_DELETE:
            return fileSystem_->deleteFile(path, uid)
                ? SyscallResult::ok()
                : SyscallResult::error(SyscallStatus::NotFound, path);
        case SYS_FS_MKDIR:
            return fileSystem_->makeDirectory(path, uid)
                ? SyscallResult::ok()
                : SyscallResult::error(SyscallStatus::AlreadyExists, path);
        case SYS_FS_RMDIR:
            return fileSystem_->removeDirectory(path, uid)
                ? SyscallResult::ok()
                : SyscallResult::error(SyscallStatus::NotEmpty, path);
        case SYS_FS_LIST: {
            auto entries = fileSystem_->listDirectory(path, uid);
            if (!entries) return SyscallResult::error(SyscallStatus::NotFound, path);
            return SyscallResult::ok("count", static_cast<int64_t>(entries->size()));
        }
        default:
            return SyscallResult::error(SyscallStatus::InvalidSyscall, "fs");
    }
}

SyscallResult Kernel::handleIoSyscall(SyscallId id, const SyscallArgs& args) {
    switch (id) {
        case SYS_IO_READ: {
            auto dev = args.getString("device");
            if (dev == "console") {
                auto pid = static_cast<Pid>(args.getInt("pid", 0));
                auto result = consoleDriver_->requestReadLine(pid);
                if (result) {
                    return SyscallResult::ok("data", *result);
                } else {
                    scheduler_->suspendProcess(pid);
                    return SyscallResult::ok("blocked", true);
                }
            }
            return SyscallResult::error(SyscallStatus::InvalidArg, dev);
        }
        case SYS_IO_WRITE: {
            auto dev = args.getString("device");
            if (dev == "console") {
                auto pid = static_cast<Pid>(args.getInt("pid", 0));
                consoleDriver_->writeText(args.getString("data"), pid);
                return SyscallResult::ok();
            }
            return SyscallResult::error(SyscallStatus::InvalidArg, dev);
        }
        case SYS_IO_REQUEST: {
            auto dev = args.getString("device");
            auto pid = static_cast<Pid>(args.getInt("pid", 0));
            auto type = static_cast<IORequestType>(args.getInt("type", 0));
            auto rid = ioManager_->submitRequest(dev, pid, type, args.getString("data"), 0);
            if (rid == 0) return SyscallResult::error(SyscallStatus::NotFound, "Устройство не найдено");
            return SyscallResult::ok("requestId", static_cast<int64_t>(rid));
        }
        default:
            return SyscallResult::error(SyscallStatus::InvalidSyscall, "io");
    }
}

SyscallResult Kernel::handleIpcSyscall(SyscallId id, const SyscallArgs& args) {
    switch (id) {
        case SYS_IPC_SEM_CREATE: {
            auto sid = ipcManager_->createSemaphore(
                args.getString("name"), static_cast<int32_t>(args.getInt("value", 1)));
            return SyscallResult::ok("id", static_cast<int64_t>(sid));
        }
        case SYS_IPC_SEM_WAIT: {
            auto sid = static_cast<SemaphoreId>(args.getInt("id", 0));
            auto pid = static_cast<Pid>(args.getInt("pid", 0));
            bool ok = ipcManager_->semaphoreWait(sid, pid);
            if (!ok) {
                scheduler_->suspendProcess(pid);
                return SyscallResult::ok("blocked", true);
            }
            return SyscallResult::ok();
        }
        case SYS_IPC_SEM_SIGNAL: {
            auto sid = static_cast<SemaphoreId>(args.getInt("id", 0));
            ipcManager_->semaphoreSignal(sid);
            return SyscallResult::ok();
        }
        case SYS_IPC_MUTEX_CREATE: {
            auto mid = ipcManager_->createMutex(args.getString("name"));
            return SyscallResult::ok("id", static_cast<int64_t>(mid));
        }
        case SYS_IPC_MUTEX_LOCK: {
            auto mid = static_cast<MutexId>(args.getInt("id", 0));
            auto pid = static_cast<Pid>(args.getInt("pid", 0));
            bool ok = ipcManager_->mutexLock(mid, pid);
            if (!ok) scheduler_->suspendProcess(pid);
            return SyscallResult::ok("blocked", !ok);
        }
        case SYS_IPC_MUTEX_UNLOCK: {
            auto mid = static_cast<MutexId>(args.getInt("id", 0));
            auto pid = static_cast<Pid>(args.getInt("pid", 0));
            ipcManager_->mutexUnlock(mid, pid);
            return SyscallResult::ok();
        }
        case SYS_IPC_PIPE_CREATE: {
            auto pid = ipcManager_->createPipe(args.getString("name"));
            return SyscallResult::ok("id", static_cast<int64_t>(pid));
        }
        default:
            return SyscallResult::error(SyscallStatus::InvalidSyscall, "ipc");
    }
}

SyscallResult Kernel::handleUserSyscall(SyscallId id, const SyscallArgs& args) {
    switch (id) {
        case SYS_USER_CREATE: {
            auto uid = userManager_->createUser(
                args.getString("username"), args.getString("password"),
                static_cast<UserRole>(args.getInt("role", 1)),
                static_cast<Uid>(args.getInt("requestor", ROOT_UID)));
            if (!uid) return SyscallResult::error(SyscallStatus::PermissionDenied, "Создание запрещено");
            return SyscallResult::ok("uid", static_cast<int64_t>(*uid));
        }
        case SYS_USER_LOGIN: {
            auto sid = userManager_->login(args.getString("username"), args.getString("password"));
            if (!sid) return SyscallResult::error(SyscallStatus::AuthFailed, "Ошибка входа");
            return SyscallResult::ok("session", static_cast<int64_t>(*sid));
        }
        case SYS_USER_LOGOUT: {
            auto sid = static_cast<SessionId>(args.getInt("session", 0));
            userManager_->logout(sid);
            return SyscallResult::ok();
        }
        default:
            return SyscallResult::error(SyscallStatus::InvalidSyscall, "user");
    }
}

SyscallResult Kernel::handleAppSyscall(SyscallId id, const SyscallArgs& args) {
    switch (id) {
        case SYS_APP_INSTALL: {
            auto appId = appRuntime_->installApp(
                args.getString("path"),
                static_cast<Uid>(args.getInt("uid", ROOT_UID)));
            if (appId.empty()) return SyscallResult::error(SyscallStatus::InvalidArgs, "Ошибка установки");
            return SyscallResult::ok("appId", appId);
        }
        case SYS_APP_UNINSTALL:
            return appRuntime_->uninstallApp(args.getString("appId"),
                                              static_cast<Uid>(args.getInt("uid", ROOT_UID)))
                ? SyscallResult::ok()
                : SyscallResult::error(SyscallStatus::NotFound, "Приложение не найдено");
        case SYS_APP_RUN: {
            auto instId = appRuntime_->runApp(
                args.getString("appId"),
                static_cast<Uid>(args.getInt("uid", ROOT_UID)));
            if (instId == 0) return SyscallResult::error(SyscallStatus::ResourceLimit, "Не удалось запустить");
            return SyscallResult::ok("instanceId", static_cast<int64_t>(instId));
        }
        case SYS_APP_STOP:
            return appRuntime_->stopApp(static_cast<uint32_t>(args.getInt("instanceId", 0)))
                ? SyscallResult::ok()
                : SyscallResult::error(SyscallStatus::NotFound, "Экземпляр не найден");
        case SYS_APP_LIST: {
            auto apps = appRuntime_->getInstalledApps();
            return SyscallResult::ok("count", static_cast<int64_t>(apps.size()));
        }
        default:
            return SyscallResult::error(SyscallStatus::InvalidSyscall, "app");
    }
}

// ============================================================================
// VGA
// ============================================================================

SyscallResult Kernel::handleVgaSyscall(SyscallId id, const SyscallArgs& args) {
    if (!vgaDriver_) return SyscallResult::error(SyscallStatus::NotReady, "VgaDriver");

    switch (id) {
        case SYS_VGA_CLEAR:
            vgaDriver_->clearScreen();
            return SyscallResult::ok();

        case SYS_VGA_SET_CURSOR: {
            if (!args.hasKey("x") || !args.hasKey("y")) return SyscallResult::error(SyscallStatus::InvalidArg, "x or y missing");
            uint16_t x = static_cast<uint16_t>(args.getInt("x"));
            uint16_t y = static_cast<uint16_t>(args.getInt("y"));
            vgaDriver_->setCursorPosition(x, y);
            return SyscallResult::ok();
        }

        case SYS_VGA_SET_COLOR: {
            if (!args.hasKey("fg") || !args.hasKey("bg")) return SyscallResult::error(SyscallStatus::InvalidArg, "fg or bg missing");
            auto fg = static_cast<VgaColor>(args.getInt("fg"));
            auto bg = static_cast<VgaColor>(args.getInt("bg"));
            vgaDriver_->setColor(fg, bg);
            return SyscallResult::ok();
        }

        case SYS_VGA_PRINT: {
            if (!args.hasKey("text")) return SyscallResult::error(SyscallStatus::InvalidArg, "text");
            vgaDriver_->print(args.getString("text"));
            return SyscallResult::ok();
        }

        default:
            return SyscallResult::error(SyscallStatus::InvalidSyscall, "vga");
    }
}

// ============================================================================
// Система
// ============================================================================

SyscallResult Kernel::handleSystemSyscall(SyscallId id, const SyscallArgs& /*args*/) {
    switch (id) {
        case SYS_SYSTEM_INFO:
            return SyscallResult::ok("tick", static_cast<int64_t>(currentTick_));
        case SYS_SYSTEM_SHUTDOWN:
            shutdown();
            return SyscallResult::ok();
        case SYS_SYSTEM_REBOOT:
            shutdown();
            boot();
            return SyscallResult::ok();
        default:
            return SyscallResult::error(SyscallStatus::InvalidSyscall, "system");
    }
}

// ============================================================================
// Внутренние
// ============================================================================

void Kernel::handleInstruction(InstructionType type, Pid pid) {
    switch (type) {
        case InstructionType::IoRequest: {
            // Направить I/O запрос на HDD
            ioManager_->submitRequest("hdd0", pid, IORequestType::Read, "", 512);
            break;
        }
        case InstructionType::MemoryAccess: {
            // Обращение к виртуальной памяти
            auto vaddr = static_cast<VirtualAddr>(pid * 0x10000);
            memoryManager_->accessMemory(pid, vaddr, false);
            break;
        }
        case InstructionType::IpcOperation:
        case InstructionType::CpuWork:
        case InstructionType::Sleep:
        case InstructionType::Exit:
            break; // обработано в scheduler
    }
}

void Kernel::logMessage(const std::string& msg) {
    bootLog_.push_back(msg);
    if (bootLog_.size() > 1000) {
        bootLog_.erase(bootLog_.begin(), bootLog_.begin() + 500);
    }
}

} // namespace re36

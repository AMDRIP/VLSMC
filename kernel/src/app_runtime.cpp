/**
 * @file app_runtime.cpp
 * @brief Реализация среды исполнения приложений RAND Elecorner 36.
 */

#include "kernel/app_runtime.h"
#include "kernel/app_api.h"
#include "kernel/event_bus.h"
#include "kernel/filesystem.h"
#include "kernel/scheduler.h"
#include "kernel/kernel.h"

#include <sstream>
#include <algorithm>

namespace re36 {

AppRuntime::AppRuntime(EventBus& eventBus, FileSystem& fs, Scheduler& scheduler,
                       const KernelConfig& config)
    : eventBus_(eventBus), fileSystem_(fs), scheduler_(scheduler), config_(config) {}

AppRuntime::~AppRuntime() = default;

void AppRuntime::setKernel(Kernel& kernel) {
    kernel_ = &kernel;
}

bool AppRuntime::init() {
    registry_.clear();
    instances_.clear();
    pidToInstance_.clear();
    nextInstanceId_ = 1;

    // Системные каталоги
    if (!fileSystem_.exists("/apps")) {
        fileSystem_.makeDirectory("/apps", ROOT_UID);
    }

    registerBuiltinApps();
    return true;
}

// ---- Управление пакетами ---------------------------------------------------

std::string AppRuntime::installApp(const std::string& packagePath, Uid installerUid) {
    // Прочитать пакет (в симуляции пакет = каталог с manifest.json)
    auto manifestContent = fileSystem_.readFile(packagePath + "/manifest.json", ROOT_UID);
    if (!manifestContent) {
        // Попробовать как файл
        manifestContent = fileSystem_.readFile(packagePath, ROOT_UID);
        if (!manifestContent) return "";
    }

    auto manifest = parseManifest(*manifestContent);
    if (!manifest || !validateManifest(*manifest)) return "";

    if (registry_.find(manifest->id) != registry_.end()) return "";

    // Создать каталог приложения
    std::string installPath = "/apps/" + manifest->id;
    fileSystem_.makeDirectory(installPath, ROOT_UID);

    InstalledApp app;
    app.manifest = *manifest;
    app.installPath = installPath;
    app.installedBy = installerUid;
    app.isSystem = false;

    std::string appId = manifest->id;
    registry_[appId] = std::move(app);

    Event evt(EventType::AppInstalled, 0, "app_runtime");
    evt.with("appId", appId).with("name", manifest->name);
    eventBus_.publish(evt);

    return appId;
}

bool AppRuntime::uninstallApp(const std::string& appId, Uid requestorUid) {
    auto it = registry_.find(appId);
    if (it == registry_.end()) return false;
    if (it->second.isSystem && requestorUid != ROOT_UID) return false;

    // Остановить если работает
    stopAppByAppId(appId);

    // Удалить каталог
    fileSystem_.removeDirectory(it->second.installPath, ROOT_UID);

    registry_.erase(it);

    Event evt(EventType::AppUninstalled, 0, "app_runtime");
    evt.with("appId", appId);
    eventBus_.publish(evt);

    return true;
}

// ---- Запуск / остановка ----------------------------------------------------

uint32_t AppRuntime::runApp(const std::string& appId, Uid launcherUid) {
    auto it = registry_.find(appId);
    if (it == registry_.end()) return 0;

    const auto& manifest = it->second.manifest;

    // Создать процесс в планировщике
    std::vector<Instruction> instructions;
    // Приложение = 100 тактов CPU work по умолчанию (JS-исполнитель на GUI)
    for (int i = 0; i < 100; ++i) {
        instructions.push_back({InstructionType::CpuWork, 0, 0, ""});
    }

    Pid pid = scheduler_.createProcess(
        manifest.name, std::move(instructions), 5, 8192, KERNEL_PID, launcherUid);
    if (pid == INVALID_PID) return 0;

    uint32_t instId = nextInstanceId_++;
    AppInstance inst;
    inst.instanceId = instId;
    inst.appId = appId;
    inst.processPid = pid;
    inst.state = AppInstanceState::Running;
    inst.launchedBy = launcherUid;

    instances_[instId] = std::move(inst);
    pidToInstance_[pid] = instId;

    // Создать AppApi для экземпляра
    if (kernel_) {
        instances_[instId].api = std::make_shared<AppApi>(*kernel_, appId, pid, launcherUid);
    }

    Event evt(EventType::AppStarted, 0, "app_runtime");
    evt.with("appId", appId).with("instanceId", static_cast<int64_t>(instId))
       .with("pid", static_cast<int64_t>(pid));
    eventBus_.publish(evt);

    return instId;
}

bool AppRuntime::stopApp(uint32_t instanceId) {
    auto it = instances_.find(instanceId);
    if (it == instances_.end()) return false;

    auto& inst = it->second;
    inst.state = AppInstanceState::Stopped;

    scheduler_.killProcess(inst.processPid, 0);
    pidToInstance_.erase(inst.processPid);

    Event evt(EventType::AppStopped, 0, "app_runtime");
    evt.with("appId", inst.appId).with("instanceId", static_cast<int64_t>(instanceId));
    eventBus_.publish(evt);

    instances_.erase(it);
    return true;
}

bool AppRuntime::stopAppByAppId(const std::string& appId) {
    std::vector<uint32_t> toStop;
    for (auto& [id, inst] : instances_) {
        if (inst.appId == appId) toStop.push_back(id);
    }
    bool ok = true;
    for (auto id : toStop) {
        if (!stopApp(id)) ok = false;
    }
    return ok;
}

// ---- Запросы ---------------------------------------------------------------

std::vector<AppInfo> AppRuntime::getInstalledApps() const {
    std::vector<AppInfo> result;
    for (auto& [id, app] : registry_) {
        AppInfo info;
        info.id = id;
        info.name = app.manifest.name;
        info.version = app.manifest.version;
        info.author = app.manifest.author;
        info.description = app.manifest.description;
        info.icon = app.manifest.icon;
        info.isInstalled = true;
        info.isRunning = isRunning(id);
        info.isSystem = app.isSystem;
        info.installedAt = app.installedAt;
        result.push_back(info);
    }
    return result;
}

std::optional<AppInfo> AppRuntime::getAppInfo(const std::string& appId) const {
    auto it = registry_.find(appId);
    if (it == registry_.end()) return std::nullopt;
    auto& app = it->second;
    AppInfo info;
    info.id = appId;
    info.name = app.manifest.name;
    info.version = app.manifest.version;
    info.author = app.manifest.author;
    info.description = app.manifest.description;
    info.icon = app.manifest.icon;
    info.isInstalled = true;
    info.isRunning = isRunning(appId);
    info.isSystem = app.isSystem;
    info.installedAt = app.installedAt;
    return info;
}

std::vector<AppInstance> AppRuntime::getRunningInstances() const {
    std::vector<AppInstance> result;
    for (auto& [id, inst] : instances_) result.push_back(inst);
    return result;
}

std::optional<AppInstance> AppRuntime::getInstanceByPid(Pid pid) const {
    auto it = pidToInstance_.find(pid);
    if (it == pidToInstance_.end()) return std::nullopt;
    auto ii = instances_.find(it->second);
    return ii != instances_.end() ? std::optional(ii->second) : std::nullopt;
}

bool AppRuntime::isInstalled(const std::string& appId) const {
    return registry_.find(appId) != registry_.end();
}

bool AppRuntime::isRunning(const std::string& appId) const {
    for (auto& [id, inst] : instances_) {
        if (inst.appId == appId) return true;
    }
    return false;
}

// ---- App API ---------------------------------------------------------------

AppApi* AppRuntime::getAppApi(uint32_t instanceId) {
    auto it = instances_.find(instanceId);
    if (it == instances_.end()) return nullptr;
    return it->second.api.get();
}

AppApi* AppRuntime::getAppApiByPid(Pid pid) {
    auto it = pidToInstance_.find(pid);
    if (it == pidToInstance_.end()) return nullptr;
    auto ii = instances_.find(it->second);
    if (ii == instances_.end()) return nullptr;
    return ii->second.api.get();
}

// ---- Разрешения ------------------------------------------------------------

bool AppRuntime::hasPermission(const std::string& appId, AppPermission perm) const {
    auto it = registry_.find(appId);
    if (it == registry_.end()) return false;
    return it->second.manifest.permissions.count(perm) > 0;
}

bool AppRuntime::checkPermission(const std::string& appId, AppPermission perm) {
    if (hasPermission(appId, perm)) return true;

    Event evt(EventType::PermissionDenied, 0, "app_runtime");
    evt.with("appId", appId);
    eventBus_.publish(evt);
    return false;
}

// ---- Очистка ---------------------------------------------------------------

void AppRuntime::stopAllApps() {
    std::vector<uint32_t> ids;
    for (auto& [id, _] : instances_) ids.push_back(id);
    for (auto id : ids) stopApp(id);
}

void AppRuntime::onProcessTerminated(Pid pid) {
    auto it = pidToInstance_.find(pid);
    if (it == pidToInstance_.end()) return;
    uint32_t instId = it->second;
    auto ii = instances_.find(instId);
    if (ii != instances_.end()) {
        ii->second.state = AppInstanceState::Stopped;
        instances_.erase(ii);
    }
    pidToInstance_.erase(it);
}

// ---- Внутренние ------------------------------------------------------------

std::optional<AppManifest> AppRuntime::parseManifest(const std::string& json) const {
    // Простой парсинг для симулятора (ищем ключевые поля)
    AppManifest m;

    auto extractValue = [&json](const std::string& key) -> std::string {
        auto pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        pos = json.find('"', pos + 1);
        if (pos == std::string::npos) return "";
        auto end = json.find('"', pos + 1);
        if (end == std::string::npos) return "";
        return json.substr(pos + 1, end - pos - 1);
    };

    m.id = extractValue("id");
    m.name = extractValue("name");
    m.version = extractValue("version");
    m.author = extractValue("author");
    m.description = extractValue("description");
    m.entryPoint = extractValue("entry");
    m.icon = extractValue("icon");

    if (m.id.empty() || m.name.empty()) return std::nullopt;
    if (m.version.empty()) m.version = "1.0.0";
    if (m.entryPoint.empty()) m.entryPoint = "main.js";

    return m;
}

bool AppRuntime::validateManifest(const AppManifest& m) const {
    if (m.id.empty() || m.name.empty()) return false;
    if (m.id.find(' ') != std::string::npos) return false;
    return true;
}

void AppRuntime::registerBuiltinApps() {
    auto reg = [this](const std::string& id, const std::string& name,
                      const std::string& desc) {
        InstalledApp app;
        app.manifest.id = id;
        app.manifest.name = name;
        app.manifest.description = desc;
        app.manifest.version = "1.0.0";
        app.manifest.author = "System";
        app.manifest.entryPoint = "main.js";
        app.manifest.permissions = {
            AppPermission::UiWindow,
            AppPermission::FilesystemRead,
            AppPermission::FilesystemWrite,
            AppPermission::ProcessList,
            AppPermission::SystemInfo
        };
        app.installPath = "/apps/" + id;
        app.isSystem = true;
        app.installedBy = ROOT_UID;
        registry_[id] = std::move(app);
    };

    reg("re36.terminal",     "Терминал",          "Командная строка");
    reg("re36.file_manager", "Файловый менеджер", "Управление файлами");
    reg("re36.task_manager", "Диспетчер задач",   "Просмотр процессов");
    reg("re36.settings",     "Настройки",         "Системные параметры");
}

} // namespace re36

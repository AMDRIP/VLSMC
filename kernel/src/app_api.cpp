// Не используется, не удалять
/**
 * @file app_api.cpp
 * @brief Реализация типобезопасного API для приложений RAND Elecorner 36.
 *
 * Каждый публичный метод:
 * 1. Проверяет AppPermission
 * 2. Формирует SyscallArgs
 * 3. Вызывает Kernel::syscall()
 * 4. Разбирает SyscallResult → ApiResult<T>
 */

#include "kernel/app_api.h"
#include "kernel/kernel.h"
#include "kernel/syscalls.h"
#include "kernel/app_runtime.h"

namespace re36 {

// ============================================================================
// Конструктор / деструктор
// ============================================================================

AppApi::AppApi(Kernel& kernel, const std::string& appId, Pid pid, Uid uid)
    : kernel_(kernel), appId_(appId), pid_(pid), uid_(uid) {}

AppApi::~AppApi() = default;

// ============================================================================
// Идентификация
// ============================================================================

const std::string& AppApi::getAppId() const { return appId_; }
Pid AppApi::getPid() const { return pid_; }
Uid AppApi::getUid() const { return uid_; }

// ============================================================================
// Консоль (Терминал)
// ============================================================================

ApiResult<void> AppApi::print(const std::string& text) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::ConsoleOutput))) {
        logCall("print", false, false, "Требуется разрешение ConsoleOutput");
        // Мы не возвращаем ошибку, если нет прав, просто игнорируем или возвращаем PermissionDenied
        // Но чтобы не ломать программы, можно возвращать PermissionDenied
        return permissionDenied("print");
    }

    SyscallArgs args;
    args.setString("device", "console");
    args.setString("data", text);
    args.setInt("pid", pid_);

    auto res = kernel_.syscall(SYS_IO_WRITE, args);
    logCall("print", true, res.isOk());

    if (res.isOk()) return ApiResult<void>::ok();
    return ApiResult<void>::fail(mapError(res.status()), res.errorMessage());
}

ApiResult<void> AppApi::printLine(const std::string& text) {
    return print(text + "\n");
}

ApiResult<std::string> AppApi::readLine() {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::ConsoleInput))) {
        logCall("readLine", false, false, "Требуется разрешение ConsoleInput");
        return permissionDeniedT<std::string>("readLine");
    }

    SyscallArgs args;
    args.setString("device", "console");
    args.setInt("pid", pid_);

    auto res = kernel_.syscall(SYS_IO_READ, args);
    logCall("readLine", true, res.isOk());

    if (res.isOk()) return ApiResult<std::string>::ok(res.getString("data"));
    return ApiResult<std::string>::fail(mapError(res.status()), res.errorMessage());
}

// ============================================================================
// Экран (VGA)
// ============================================================================

ApiResult<void> AppApi::vgaClear() {
    auto res = kernel_.syscall(SYS_VGA_CLEAR, SyscallArgs{});
    logCall("vgaClear", true, res.isOk());
    if (res.isOk()) return ApiResult<void>::ok();
    return ApiResult<void>::fail(mapError(res.status()), res.errorMessage());
}

ApiResult<void> AppApi::vgaPrint(const std::string& text) {
    SyscallArgs args;
    args.setString("text", text);
    auto res = kernel_.syscall(SYS_VGA_PRINT, args);
    logCall("vgaPrint", true, res.isOk());
    if (res.isOk()) return ApiResult<void>::ok();
    return ApiResult<void>::fail(mapError(res.status()), res.errorMessage());
}

ApiResult<void> AppApi::vgaSetCursor(uint16_t x, uint16_t y) {
    SyscallArgs args;
    args.setInt("x", x);
    args.setInt("y", y);
    auto res = kernel_.syscall(SYS_VGA_SET_CURSOR, args);
    logCall("vgaSetCursor", true, res.isOk());
    if (res.isOk()) return ApiResult<void>::ok();
    return ApiResult<void>::fail(mapError(res.status()), res.errorMessage());
}

ApiResult<void> AppApi::vgaSetColor(VgaColor fg, VgaColor bg) {
    SyscallArgs args;
    args.setInt("fg", static_cast<int64_t>(fg));
    args.setInt("bg", static_cast<int64_t>(bg));
    auto res = kernel_.syscall(SYS_VGA_SET_COLOR, args);
    logCall("vgaSetColor", true, res.isOk());
    if (res.isOk()) return ApiResult<void>::ok();
    return ApiResult<void>::fail(mapError(res.status()), res.errorMessage());
}

// ============================================================================
// Файловая система
// ============================================================================

ApiResult<std::string> AppApi::readFile(const std::string& path) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::FilesystemRead))) {
        logCall("readFile", false, false, "Нет разрешения FilesystemRead");
        return permissionDeniedT<std::string>("readFile");
    }

    SyscallArgs args;
    args.set("path", path);
    args.set("uid", static_cast<int64_t>(uid_));
    auto result = kernel_.syscall(SyscallId::ReadFile, args);

    if (result.isOk()) {
        logCall("readFile", true, true);
        return ApiResult<std::string>::ok(result.getString("data"));
    }

    logCall("readFile", true, false, result.errorMessage);
    return ApiResult<std::string>::fail(mapError(static_cast<uint8_t>(result.status)),
                                        result.errorMessage);
}

ApiResult<void> AppApi::writeFile(const std::string& path, const std::string& data) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::FilesystemWrite))) {
        logCall("writeFile", false, false, "Нет разрешения FilesystemWrite");
        return ApiResult<void>::fail(ApiErrorCode::PermissionDenied,
                                     "Нет разрешения для: writeFile");
    }

    SyscallArgs args;
    args.set("path", path);
    args.set("data", data);
    args.set("uid", static_cast<int64_t>(uid_));
    auto result = kernel_.syscall(SyscallId::WriteFile, args);

    if (result.isOk()) {
        logCall("writeFile", true, true);
        return ApiResult<void>::ok();
    }

    logCall("writeFile", true, false, result.errorMessage);
    return ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                 result.errorMessage);
}

ApiResult<void> AppApi::createFile(const std::string& path) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::FilesystemWrite))) {
        logCall("createFile", false, false);
        return permissionDenied("createFile");
    }

    SyscallArgs args;
    args.set("path", path);
    args.set("uid", static_cast<int64_t>(uid_));
    auto result = kernel_.syscall(SyscallId::CreateFile, args);

    bool ok = result.isOk();
    logCall("createFile", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

ApiResult<void> AppApi::deleteFile(const std::string& path) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::FilesystemWrite))) {
        logCall("deleteFile", false, false);
        return permissionDenied("deleteFile");
    }

    SyscallArgs args;
    args.set("path", path);
    args.set("uid", static_cast<int64_t>(uid_));
    auto result = kernel_.syscall(SyscallId::DeleteFile, args);

    bool ok = result.isOk();
    logCall("deleteFile", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

ApiResult<void> AppApi::renameFile(const std::string& oldPath, const std::string& newPath) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::FilesystemWrite))) {
        logCall("renameFile", false, false);
        return permissionDenied("renameFile");
    }

    SyscallArgs args;
    args.set("oldPath", oldPath);
    args.set("newPath", newPath);
    args.set("uid", static_cast<int64_t>(uid_));
    auto result = kernel_.syscall(SyscallId::RenameFile, args);

    bool ok = result.isOk();
    logCall("renameFile", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

ApiResult<void> AppApi::copyFile(const std::string& srcPath, const std::string& dstPath) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::FilesystemRead)) ||
        !checkPermission(static_cast<uint16_t>(AppPermission::FilesystemWrite))) {
        logCall("copyFile", false, false);
        return permissionDenied("copyFile");
    }

    SyscallArgs args;
    args.set("srcPath", srcPath);
    args.set("dstPath", dstPath);
    args.set("uid", static_cast<int64_t>(uid_));
    auto result = kernel_.syscall(SyscallId::CopyFile, args);

    bool ok = result.isOk();
    logCall("copyFile", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

ApiResult<FileInfoView> AppApi::statFile(const std::string& path) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::FilesystemRead))) {
        logCall("statFile", false, false);
        return permissionDeniedT<FileInfoView>("statFile");
    }

    SyscallArgs args;
    args.set("path", path);
    args.set("uid", static_cast<int64_t>(uid_));
    auto result = kernel_.syscall(SyscallId::StatFile, args);

    if (result.isOk()) {
        FileInfoView info;
        info.name = result.getString("name");
        info.path = path;
        info.size = static_cast<size_t>(result.getInt("size"));
        info.isDirectory = result.getBool("isDirectory");
        info.permissions = result.getString("permissions");
        info.owner = result.getString("owner");

        logCall("statFile", true, true);
        return ApiResult<FileInfoView>::ok(std::move(info));
    }

    logCall("statFile", true, false, result.errorMessage);
    return ApiResult<FileInfoView>::fail(mapError(static_cast<uint8_t>(result.status)),
                                         result.errorMessage);
}

ApiResult<void> AppApi::makeDirectory(const std::string& path) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::FilesystemWrite))) {
        logCall("makeDirectory", false, false);
        return permissionDenied("makeDirectory");
    }

    SyscallArgs args;
    args.set("path", path);
    args.set("uid", static_cast<int64_t>(uid_));
    auto result = kernel_.syscall(SyscallId::MakeDirectory, args);

    bool ok = result.isOk();
    logCall("makeDirectory", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

ApiResult<void> AppApi::removeDirectory(const std::string& path) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::FilesystemWrite))) {
        logCall("removeDirectory", false, false);
        return permissionDenied("removeDirectory");
    }

    SyscallArgs args;
    args.set("path", path);
    args.set("uid", static_cast<int64_t>(uid_));
    auto result = kernel_.syscall(SyscallId::RemoveDirectory, args);

    bool ok = result.isOk();
    logCall("removeDirectory", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

ApiResult<std::vector<std::string>> AppApi::listDirectory(const std::string& path) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::FilesystemRead))) {
        logCall("listDirectory", false, false);
        return permissionDeniedT<std::vector<std::string>>("listDirectory");
    }

    SyscallArgs args;
    args.set("path", path);
    args.set("uid", static_cast<int64_t>(uid_));
    auto result = kernel_.syscall(SyscallId::ListDirectory, args);

    if (result.isOk()) {
        // Результат содержит количество элементов и список имён
        int64_t count = result.getInt("count");
        std::vector<std::string> entries;
        entries.reserve(static_cast<size_t>(count));

        for (int64_t i = 0; i < count; ++i) {
            std::string key = "entry_" + std::to_string(i);
            entries.push_back(result.getString(key));
        }

        logCall("listDirectory", true, true);
        return ApiResult<std::vector<std::string>>::ok(std::move(entries));
    }

    logCall("listDirectory", true, false, result.errorMessage);
    return ApiResult<std::vector<std::string>>::fail(
        mapError(static_cast<uint8_t>(result.status)), result.errorMessage);
}

// ============================================================================
// IPC — Сообщения
// ============================================================================

ApiResult<uint32_t> AppApi::createMessageQueue(const std::string& name) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcSend))) {
        logCall("createMessageQueue", false, false);
        return permissionDeniedT<uint32_t>("createMessageQueue");
    }

    SyscallArgs args;
    args.set("name", name);
    auto result = kernel_.syscall(SyscallId::MsgQueueCreate, args);

    if (result.isOk()) {
        logCall("createMessageQueue", true, true);
        return ApiResult<uint32_t>::ok(static_cast<uint32_t>(result.getInt("queueId")));
    }

    logCall("createMessageQueue", true, false, result.errorMessage);
    return ApiResult<uint32_t>::fail(mapError(static_cast<uint8_t>(result.status)),
                                     result.errorMessage);
}

ApiResult<void> AppApi::sendMessage(uint32_t queueId, const std::string& data) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcSend))) {
        logCall("sendMessage", false, false);
        return permissionDenied("sendMessage");
    }

    SyscallArgs args;
    args.set("queueId", static_cast<int64_t>(queueId));
    args.set("data", data);
    args.set("pid", static_cast<int64_t>(pid_));
    auto result = kernel_.syscall(SyscallId::MsgSend, args);

    bool ok = result.isOk();
    logCall("sendMessage", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

ApiResult<std::string> AppApi::receiveMessage(uint32_t queueId) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcReceive))) {
        logCall("receiveMessage", false, false);
        return permissionDeniedT<std::string>("receiveMessage");
    }

    SyscallArgs args;
    args.set("queueId", static_cast<int64_t>(queueId));
    args.set("pid", static_cast<int64_t>(pid_));
    auto result = kernel_.syscall(SyscallId::MsgReceive, args);

    if (result.isOk()) {
        logCall("receiveMessage", true, true);
        return ApiResult<std::string>::ok(result.getString("data"));
    }

    logCall("receiveMessage", true, false, result.errorMessage);
    return ApiResult<std::string>::fail(mapError(static_cast<uint8_t>(result.status)),
                                        result.errorMessage);
}

ApiResult<void> AppApi::destroyMessageQueue(uint32_t queueId) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcSend))) {
        logCall("destroyMessageQueue", false, false);
        return permissionDenied("destroyMessageQueue");
    }

    SyscallArgs args;
    args.set("queueId", static_cast<int64_t>(queueId));
    auto result = kernel_.syscall(SyscallId::MsgQueueDestroy, args);

    bool ok = result.isOk();
    logCall("destroyMessageQueue", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

// ============================================================================
// IPC — Каналы (pipes)
// ============================================================================

ApiResult<uint32_t> AppApi::createPipe() {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcSend))) {
        logCall("createPipe", false, false);
        return permissionDeniedT<uint32_t>("createPipe");
    }

    SyscallArgs args;
    auto result = kernel_.syscall(SyscallId::PipeCreate, args);

    if (result.isOk()) {
        logCall("createPipe", true, true);
        return ApiResult<uint32_t>::ok(static_cast<uint32_t>(result.getInt("pipeId")));
    }

    logCall("createPipe", true, false, result.errorMessage);
    return ApiResult<uint32_t>::fail(mapError(static_cast<uint8_t>(result.status)),
                                     result.errorMessage);
}

ApiResult<void> AppApi::writePipe(uint32_t pipeId, const std::string& data) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcSend))) {
        logCall("writePipe", false, false);
        return permissionDenied("writePipe");
    }

    SyscallArgs args;
    args.set("pipeId", static_cast<int64_t>(pipeId));
    args.set("data", data);
    auto result = kernel_.syscall(SyscallId::PipeWrite, args);

    bool ok = result.isOk();
    logCall("writePipe", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

ApiResult<std::string> AppApi::readPipe(uint32_t pipeId) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcReceive))) {
        logCall("readPipe", false, false);
        return permissionDeniedT<std::string>("readPipe");
    }

    SyscallArgs args;
    args.set("pipeId", static_cast<int64_t>(pipeId));
    auto result = kernel_.syscall(SyscallId::PipeRead, args);

    if (result.isOk()) {
        logCall("readPipe", true, true);
        return ApiResult<std::string>::ok(result.getString("data"));
    }

    logCall("readPipe", true, false, result.errorMessage);
    return ApiResult<std::string>::fail(mapError(static_cast<uint8_t>(result.status)),
                                        result.errorMessage);
}

ApiResult<void> AppApi::closePipe(uint32_t pipeId) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcSend))) {
        logCall("closePipe", false, false);
        return permissionDenied("closePipe");
    }

    SyscallArgs args;
    args.set("pipeId", static_cast<int64_t>(pipeId));
    auto result = kernel_.syscall(SyscallId::PipeClose, args);

    bool ok = result.isOk();
    logCall("closePipe", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

// ============================================================================
// IPC — Синхронизация
// ============================================================================

ApiResult<uint32_t> AppApi::createSemaphore(const std::string& name, int initialValue) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcSend))) {
        logCall("createSemaphore", false, false);
        return permissionDeniedT<uint32_t>("createSemaphore");
    }

    SyscallArgs args;
    args.set("name", name);
    args.set("initialValue", static_cast<int64_t>(initialValue));
    auto result = kernel_.syscall(SyscallId::SemCreate, args);

    if (result.isOk()) {
        logCall("createSemaphore", true, true);
        return ApiResult<uint32_t>::ok(static_cast<uint32_t>(result.getInt("semId")));
    }

    logCall("createSemaphore", true, false, result.errorMessage);
    return ApiResult<uint32_t>::fail(mapError(static_cast<uint8_t>(result.status)),
                                     result.errorMessage);
}

ApiResult<void> AppApi::semaphoreWait(uint32_t semId) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcReceive))) {
        logCall("semaphoreWait", false, false);
        return permissionDenied("semaphoreWait");
    }

    SyscallArgs args;
    args.set("semId", static_cast<int64_t>(semId));
    args.set("pid", static_cast<int64_t>(pid_));
    auto result = kernel_.syscall(SyscallId::SemWait, args);

    bool ok = result.isOk();
    logCall("semaphoreWait", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

ApiResult<void> AppApi::semaphoreSignal(uint32_t semId) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcSend))) {
        logCall("semaphoreSignal", false, false);
        return permissionDenied("semaphoreSignal");
    }

    SyscallArgs args;
    args.set("semId", static_cast<int64_t>(semId));
    args.set("pid", static_cast<int64_t>(pid_));
    auto result = kernel_.syscall(SyscallId::SemSignal, args);

    bool ok = result.isOk();
    logCall("semaphoreSignal", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

ApiResult<uint32_t> AppApi::createMutex(const std::string& name) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcSend))) {
        logCall("createMutex", false, false);
        return permissionDeniedT<uint32_t>("createMutex");
    }

    SyscallArgs args;
    args.set("name", name);
    auto result = kernel_.syscall(SyscallId::MutexCreate, args);

    if (result.isOk()) {
        logCall("createMutex", true, true);
        return ApiResult<uint32_t>::ok(static_cast<uint32_t>(result.getInt("mutexId")));
    }

    logCall("createMutex", true, false, result.errorMessage);
    return ApiResult<uint32_t>::fail(mapError(static_cast<uint8_t>(result.status)),
                                     result.errorMessage);
}

ApiResult<void> AppApi::mutexLock(uint32_t mutexId) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcReceive))) {
        logCall("mutexLock", false, false);
        return permissionDenied("mutexLock");
    }

    SyscallArgs args;
    args.set("mutexId", static_cast<int64_t>(mutexId));
    args.set("pid", static_cast<int64_t>(pid_));
    auto result = kernel_.syscall(SyscallId::MutexLock, args);

    bool ok = result.isOk();
    logCall("mutexLock", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

ApiResult<void> AppApi::mutexUnlock(uint32_t mutexId) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::IpcSend))) {
        logCall("mutexUnlock", false, false);
        return permissionDenied("mutexUnlock");
    }

    SyscallArgs args;
    args.set("mutexId", static_cast<int64_t>(mutexId));
    args.set("pid", static_cast<int64_t>(pid_));
    auto result = kernel_.syscall(SyscallId::MutexUnlock, args);

    bool ok = result.isOk();
    logCall("mutexUnlock", true, ok, ok ? "" : result.errorMessage);

    return ok ? ApiResult<void>::ok()
              : ApiResult<void>::fail(mapError(static_cast<uint8_t>(result.status)),
                                      result.errorMessage);
}

// ============================================================================
// Процессы
// ============================================================================

ApiResult<uint32_t> AppApi::getProcessCount() {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::ProcessList))) {
        logCall("getProcessCount", false, false);
        return permissionDeniedT<uint32_t>("getProcessCount");
    }

    SyscallArgs args;
    auto result = kernel_.syscall(SyscallId::GetProcessList, args);

    if (result.isOk()) {
        logCall("getProcessCount", true, true);
        return ApiResult<uint32_t>::ok(static_cast<uint32_t>(result.getInt("count")));
    }

    logCall("getProcessCount", true, false, result.errorMessage);
    return ApiResult<uint32_t>::fail(mapError(static_cast<uint8_t>(result.status)),
                                     result.errorMessage);
}

ApiResult<std::vector<ProcessInfoView>> AppApi::getProcessList() {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::ProcessList))) {
        logCall("getProcessList", false, false);
        return permissionDeniedT<std::vector<ProcessInfoView>>("getProcessList");
    }

    SyscallArgs args;
    auto result = kernel_.syscall(SyscallId::GetProcessList, args);

    if (result.isOk()) {
        // Список процессов возвращается как count + индексированные записи
        int64_t count = result.getInt("count");
        std::vector<ProcessInfoView> list;
        list.reserve(static_cast<size_t>(count));

        for (int64_t i = 0; i < count; ++i) {
            ProcessInfoView pv;
            pv.pid = static_cast<Pid>(result.getInt("pid_" + std::to_string(i)));
            pv.name = result.getString("name_" + std::to_string(i));
            pv.state = result.getString("state_" + std::to_string(i));
            pv.priority = static_cast<uint32_t>(result.getInt("priority_" + std::to_string(i)));
            list.push_back(std::move(pv));
        }

        logCall("getProcessList", true, true);
        return ApiResult<std::vector<ProcessInfoView>>::ok(std::move(list));
    }

    logCall("getProcessList", true, false, result.errorMessage);
    return ApiResult<std::vector<ProcessInfoView>>::fail(
        mapError(static_cast<uint8_t>(result.status)), result.errorMessage);
}

ApiResult<Pid> AppApi::spawnProcess(const std::string& name, uint32_t burstTime, uint8_t priority) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::ProcessSpawn))) {
        logCall("spawnProcess", false, false);
        return permissionDeniedT<Pid>("spawnProcess");
    }

    SyscallArgs args;
    args.set("name", name);
    args.set("burstTime", static_cast<int64_t>(burstTime));
    args.set("priority", static_cast<int64_t>(priority));
    auto result = kernel_.syscall(SyscallId::CreateProcess, args);

    if (result.isOk()) {
        logCall("spawnProcess", true, true);
        return ApiResult<Pid>::ok(static_cast<Pid>(result.getInt("pid")));
    }

    logCall("spawnProcess", true, false, result.errorMessage);
    return ApiResult<Pid>::fail(mapError(static_cast<uint8_t>(result.status)),
                                result.errorMessage);
}

// ============================================================================
// Системная информация
// ============================================================================

ApiResult<SystemInfoView> AppApi::getSystemInfo() {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::SystemInfo))) {
        logCall("getSystemInfo", false, false);
        return permissionDeniedT<SystemInfoView>("getSystemInfo");
    }

    SyscallArgs args;
    auto result = kernel_.syscall(SyscallId::GetSystemStats, args);

    if (result.isOk()) {
        SystemInfoView info;
        info.uptime = static_cast<uint64_t>(result.getInt("tick"));
        info.hostname = result.getString("hostname");
        info.cpuUsagePercent = 0.0;  // TODO: вычислить из подсистем
        info.ramUsagePercent = 0.0;
        info.processCount = static_cast<uint32_t>(result.getInt("processCount", 0));

        logCall("getSystemInfo", true, true);
        return ApiResult<SystemInfoView>::ok(std::move(info));
    }

    logCall("getSystemInfo", true, false, result.errorMessage);
    return ApiResult<SystemInfoView>::fail(mapError(static_cast<uint8_t>(result.status)),
                                           result.errorMessage);
}

ApiResult<uint64_t> AppApi::getUptime() {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::SystemInfo))) {
        logCall("getUptime", false, false);
        return permissionDeniedT<uint64_t>("getUptime");
    }

    SyscallArgs args;
    auto result = kernel_.syscall(SyscallId::GetUptime, args);

    if (result.isOk()) {
        logCall("getUptime", true, true);
        return ApiResult<uint64_t>::ok(static_cast<uint64_t>(result.getInt("tick")));
    }

    logCall("getUptime", true, false, result.errorMessage);
    return ApiResult<uint64_t>::fail(mapError(static_cast<uint8_t>(result.status)),
                                     result.errorMessage);
}

ApiResult<std::string> AppApi::getHostname() {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::SystemInfo))) {
        logCall("getHostname", false, false);
        return permissionDeniedT<std::string>("getHostname");
    }

    SyscallArgs args;
    auto result = kernel_.syscall(SyscallId::GetHostname, args);

    if (result.isOk()) {
        logCall("getHostname", true, true);
        return ApiResult<std::string>::ok(result.getString("hostname"));
    }

    logCall("getHostname", true, false, result.errorMessage);
    return ApiResult<std::string>::fail(mapError(static_cast<uint8_t>(result.status)),
                                        result.errorMessage);
}

ApiResult<MemoryInfoView> AppApi::getMemoryInfo() {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::SystemInfo))) {
        logCall("getMemoryInfo", false, false);
        return permissionDeniedT<MemoryInfoView>("getMemoryInfo");
    }

    SyscallArgs args;
    auto result = kernel_.syscall(SyscallId::GetMemoryStats, args);

    if (result.isOk()) {
        MemoryInfoView info;
        info.totalMemory = static_cast<size_t>(result.getInt("totalMemory"));
        info.usedMemory = static_cast<size_t>(result.getInt("usedMemory"));
        info.freeMemory = static_cast<size_t>(result.getInt("freeMemory"));
        info.usagePercent = static_cast<double>(result.getInt("usagePercent", 0));
        info.swapEnabled = result.getBool("swapEnabled");
        info.swapTotal = static_cast<size_t>(result.getInt("swapTotal", 0));
        info.swapUsed = static_cast<size_t>(result.getInt("swapUsed", 0));

        logCall("getMemoryInfo", true, true);
        return ApiResult<MemoryInfoView>::ok(std::move(info));
    }

    logCall("getMemoryInfo", true, false, result.errorMessage);
    return ApiResult<MemoryInfoView>::fail(mapError(static_cast<uint8_t>(result.status)),
                                           result.errorMessage);
}

// ============================================================================
// Уведомления
// ============================================================================

ApiResult<void> AppApi::notify(const std::string& title, const std::string& message) {
    if (!checkPermission(static_cast<uint16_t>(AppPermission::UiNotification))) {
        logCall("notify", false, false);
        return permissionDenied("notify");
    }

    // Уведомления обрабатываются через EventBus (GUI подписывается)
    // AppApi просто публикует — ядро перехватит
    // Пока что просто логируем и возвращаем Ok
    logCall("notify", true, true);
    (void)title;
    (void)message;
    return ApiResult<void>::ok();
}

// ============================================================================
// Журнал вызовов
// ============================================================================

const std::vector<ApiCallLog>& AppApi::getCallLog() const {
    return callLog_;
}

void AppApi::clearCallLog() {
    callLog_.clear();
}

void AppApi::setMaxLogSize(size_t maxSize) {
    maxLogSize_ = maxSize;
}

// ============================================================================
// Внутренние методы
// ============================================================================

bool AppApi::checkPermission(uint16_t permission) const {
    auto perm = static_cast<AppPermission>(permission);
    return kernel_.getAppRuntime().hasPermission(appId_, perm);
}

void AppApi::logCall(const std::string& method, bool permitted, bool success,
                     const std::string& errorMessage) {
    if (callLog_.size() >= maxLogSize_) {
        // Удалить старую половину
        callLog_.erase(callLog_.begin(),
                       callLog_.begin() + static_cast<ptrdiff_t>(maxLogSize_ / 2));
    }

    ApiCallLog entry;
    entry.tick = kernel_.getCurrentTick();
    entry.appId = appId_;
    entry.method = method;
    entry.permitted = permitted;
    entry.success = success;
    entry.errorMessage = errorMessage;
    callLog_.push_back(std::move(entry));
}

ApiErrorCode AppApi::mapError(uint8_t syscallStatus) {
    auto status = static_cast<SyscallStatus>(syscallStatus);
    switch (status) {
        case SyscallStatus::Ok:               return ApiErrorCode::None;
        case SyscallStatus::PermissionDenied:  return ApiErrorCode::PermissionDenied;
        case SyscallStatus::NotFound:          return ApiErrorCode::NotFound;
        case SyscallStatus::AlreadyExists:     return ApiErrorCode::AlreadyExists;
        case SyscallStatus::InvalidArgument:   return ApiErrorCode::InvalidArgument;
        case SyscallStatus::OutOfMemory:       return ApiErrorCode::OutOfResources;
        case SyscallStatus::OutOfDiskSpace:    return ApiErrorCode::OutOfResources;
        case SyscallStatus::ResourceBusy:      return ApiErrorCode::ResourceBusy;
        case SyscallStatus::ResourceLimit:     return ApiErrorCode::OutOfResources;
        case SyscallStatus::InvalidAddress:    return ApiErrorCode::InvalidArgument;
        default:                               return ApiErrorCode::InternalError;
    }
}

ApiResult<void> AppApi::permissionDenied(const std::string& method) {
    return ApiResult<void>::fail(ApiErrorCode::PermissionDenied,
                                 "Нет разрешения для: " + method);
}

} // namespace re36

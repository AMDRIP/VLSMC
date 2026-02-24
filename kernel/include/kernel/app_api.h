/**
 * @file app_api.h
 * @brief Типобезопасный API для пользовательских приложений RAND Elecorner 36.
 *
 * AppApi — единственный объект, который видит приложение.
 * Приложение НЕ ЗНАЕТ о syscall, SyscallId, SyscallArgs и т.д.
 * Все вызовы проходят через типобезопасные методы с автоматической
 * проверкой разрешений (AppPermission из манифеста).
 *
 * Архитектура:
 *   Приложение → AppApi → [проверка разрешения] → Kernel::syscall()
 *
 * Соответствует требованиям APP-06 (изоляция приложений).
 */

#pragma once

#include "types.h"
#include "kernel/vga_driver.h"

#include <string>
#include <vector>
#include <optional>

namespace re36 {

// Предварительные объявления (приложение их не видит)
class Kernel;

// ============================================================================
// Код ошибки API
// ============================================================================

/**
 * @enum ApiErrorCode
 * @brief Коды ошибок, видимые приложению.
 *
 * Приложение не видит внутренних статусов ядра (SyscallStatus);
 * оно получает только этот набор ошибок.
 */
enum class ApiErrorCode : uint8_t {
    None,                   ///< Нет ошибки
    PermissionDenied,       ///< Нет нужного разрешения
    NotFound,               ///< Ресурс не найден
    AlreadyExists,          ///< Ресурс уже существует
    InvalidArgument,        ///< Некорректный аргумент
    OutOfResources,         ///< Недостаточно ресурсов (память, диск, swap)
    ResourceBusy,           ///< Ресурс занят
    IoError,                ///< Ошибка ввода-вывода
    InternalError           ///< Внутренняя ошибка ядра
};

// ============================================================================
// Ошибка API
// ============================================================================

/**
 * @struct ApiError
 * @brief Описание ошибки, возвращаемое приложению.
 */
struct ApiError {
    ApiErrorCode    code    = ApiErrorCode::None;
    std::string     message;                        ///< Человекочитаемое сообщение

    /// Нет ошибки?
    bool ok() const { return code == ApiErrorCode::None; }
};

// ============================================================================
// Результат API: ApiResult<T>
// ============================================================================

/**
 * @class ApiResult
 * @brief Обёртка результата: либо значение T, либо ошибка.
 *
 * Использование:
 * @code
 *   auto result = api.readFile("/data/config.txt");
 *   if (result) {
 *       std::string content = result.value();
 *   } else {
 *       std::cerr << result.error().message << "\n";
 *   }
 * @endcode
 */
template <typename T>
class ApiResult {
public:
    /// Успешный результат
    static ApiResult ok(const T& value) {
        ApiResult r;
        r.value_ = value;
        r.error_ = {ApiErrorCode::None, ""};
        return r;
    }

    static ApiResult ok(T&& value) {
        ApiResult r;
        r.value_ = std::move(value);
        r.error_ = {ApiErrorCode::None, ""};
        return r;
    }

    /// Ошибка
    static ApiResult fail(ApiErrorCode code, const std::string& message) {
        ApiResult r;
        r.error_ = {code, message};
        return r;
    }

    /// Успешен?
    explicit operator bool() const { return error_.ok(); }

    /// Значение
    const T& value() const { return value_.value(); }
    T& value() { return value_.value(); }

    /// Ошибка
    const ApiError& error() const { return error_; }

private:
    std::optional<T> value_;
    ApiError         error_;
};

/**
 * @brief Специализация ApiResult<void> для операций без возвращаемого значения.
 */
template <>
class ApiResult<void> {
public:
    static ApiResult ok() {
        ApiResult r;
        r.error_ = {ApiErrorCode::None, ""};
        return r;
    }

    static ApiResult fail(ApiErrorCode code, const std::string& message) {
        ApiResult r;
        r.error_ = {code, message};
        return r;
    }

    explicit operator bool() const { return error_.ok(); }
    const ApiError& error() const { return error_; }

private:
    ApiError error_;
};

// ============================================================================
// Безопасные проекции данных ядра (видимые приложению)
// ============================================================================

/**
 * @struct SystemInfoView
 * @brief Проекция системной информации для приложений.
 *
 * Содержит только ту часть данных, что безопасно показывать
 * пользовательским приложениям.
 */
struct SystemInfoView {
    uint64_t    uptime          = 0;        ///< Тики с момента загрузки
    std::string hostname;                   ///< Имя хоста
    double      cpuUsagePercent = 0.0;      ///< % загрузки CPU
    double      ramUsagePercent = 0.0;      ///< % использования RAM
    uint32_t    processCount    = 0;        ///< Количество процессов
};

/**
 * @struct ProcessInfoView
 * @brief Информация о процессе, видимая приложению.
 */
struct ProcessInfoView {
    Pid         pid;
    std::string name;
    std::string state;      ///< "Готов", "Выполняется", "Ожидание", "Завершён"
    uint32_t    priority = 0;
};

/**
 * @struct FileInfoView
 * @brief Информация о файле, видимая приложению.
 */
struct FileInfoView {
    std::string name;
    std::string path;
    size_t      size        = 0;
    bool        isDirectory = false;
    std::string permissions;    ///< "rwxr-xr-x"
    std::string owner;
};

/**
 * @struct MemoryInfoView
 * @brief Информация о памяти, видимая приложению.
 */
struct MemoryInfoView {
    size_t      totalMemory     = 0;        ///< Всего RAM (байт)
    size_t      usedMemory      = 0;        ///< Использовано RAM
    size_t      freeMemory      = 0;        ///< Свободно RAM
    double      usagePercent    = 0.0;      ///< %
    bool        swapEnabled     = false;    ///< Swap включён?
    size_t      swapTotal       = 0;        ///< Всего swap
    size_t      swapUsed        = 0;        ///< Использовано swap
};

// ============================================================================
// Журнал вызовов API (для аудита и визуализации)
// ============================================================================

/**
 * @struct ApiCallLog
 * @brief Запись о вызове API (для визуализации в GUI).
 */
struct ApiCallLog {
    Tick        tick;
    std::string appId;
    std::string method;         ///< Имя метода ("readFile", "sendMessage")
    bool        permitted;      ///< Разрешение получено?
    bool        success;        ///< Вызов успешен?
    std::string errorMessage;   ///< Сообщение об ошибке (если есть)
};

// ============================================================================
// AppApi — главный класс
// ============================================================================

/**
 * @class AppApi
 * @brief Типобезопасный интерфейс ядра для пользовательских приложений.
 *
 * Приложение получает указатель на AppApi при запуске.
 * Каждый метод:
 * 1. Проверяет нужное AppPermission
 * 2. Формирует SyscallArgs (внутренняя деталь)
 * 3. Вызывает Kernel::syscall()
 * 4. Разбирает SyscallResult → ApiResult<T>
 *
 * Приложение не имеет доступа к Kernel, EventBus, подсистемам напрямую.
 */
class AppApi {
public:
    /**
     * @brief Конструктор (вызывается ядром, не приложением).
     *
     * @param kernel    Ссылка на ядро (скрыта от приложения)
     * @param appId     ID приложения (из манифеста)
     * @param pid       PID процесса приложения
     * @param uid       UID пользователя, запустившего приложение
     */
    AppApi(Kernel& kernel, const std::string& appId, Pid pid, Uid uid);
    ~AppApi();

    // Некопируемый
    AppApi(const AppApi&) = delete;
    AppApi& operator=(const AppApi&) = delete;

    // ========================================================================
    // Идентификация
    // ========================================================================

    /// ID приложения (из манифеста)
    const std::string& getAppId() const;

    /// PID процесса приложения
    Pid getPid() const;

    /// UID пользователя
    Uid getUid() const;

    // ========================================================================
    // Консоль (Терминал)
    // ========================================================================

    /// Вывести текст на консоль (без перевода строки)
    ApiResult<void> print(const std::string& text);

    /// Вывести строку на консоль (с переводом строки)
    ApiResult<void> printLine(const std::string& text = "");

    /// Считать введенную строку из консоли (блокирующий вызов)
    ApiResult<std::string> readLine();

    // ========================================================================
    // Экран (VGA)
    // ========================================================================

    /// Очистить экран
    ApiResult<void> vgaClear();

    /// Напечатать текст в VGA буфер
    ApiResult<void> vgaPrint(const std::string& text);

    /// Установить позицию курсора (0..79, 0..24)
    ApiResult<void> vgaSetCursor(uint16_t x, uint16_t y);

    /// Установить текущий цвет (фон и текст)
    ApiResult<void> vgaSetColor(VgaColor fg, VgaColor bg);

    // ========================================================================
    // Файловая система (требует FilesystemRead / FilesystemWrite)
    // ========================================================================

    /// Прочитать содержимое файла
    ApiResult<std::string> readFile(const std::string& path);

    /// Записать данные в файл (создаёт файл если его нет)
    ApiResult<void> writeFile(const std::string& path, const std::string& data);

    /// Создать пустой файл
    ApiResult<void> createFile(const std::string& path);

    /// Удалить файл
    ApiResult<void> deleteFile(const std::string& path);

    /// Переименовать файл
    ApiResult<void> renameFile(const std::string& oldPath, const std::string& newPath);

    /// Копировать файл
    ApiResult<void> copyFile(const std::string& srcPath, const std::string& dstPath);

    /// Информация о файле
    ApiResult<FileInfoView> statFile(const std::string& path);

    /// Создать каталог
    ApiResult<void> makeDirectory(const std::string& path);

    /// Удалить каталог
    ApiResult<void> removeDirectory(const std::string& path);

    /// Содержимое каталога (имена файлов/каталогов)
    ApiResult<std::vector<std::string>> listDirectory(const std::string& path);

    // ========================================================================
    // IPC — Сообщения (требует IpcSend / IpcReceive)
    // ========================================================================

    /// Создать очередь сообщений
    ApiResult<uint32_t> createMessageQueue(const std::string& name);

    /// Отправить сообщение в очередь
    ApiResult<void> sendMessage(uint32_t queueId, const std::string& data);

    /// Получить сообщение из очереди
    ApiResult<std::string> receiveMessage(uint32_t queueId);

    /// Удалить очередь сообщений
    ApiResult<void> destroyMessageQueue(uint32_t queueId);

    // ========================================================================
    // IPC — Каналы (pipes) (требует IpcSend / IpcReceive)
    // ========================================================================

    /// Создать канал
    ApiResult<uint32_t> createPipe();

    /// Записать в канал
    ApiResult<void> writePipe(uint32_t pipeId, const std::string& data);

    /// Прочитать из канала
    ApiResult<std::string> readPipe(uint32_t pipeId);

    /// Закрыть канал
    ApiResult<void> closePipe(uint32_t pipeId);

    // ========================================================================
    // IPC — Синхронизация (требует IpcSend / IpcReceive)
    // ========================================================================

    /// Создать семафор
    ApiResult<uint32_t> createSemaphore(const std::string& name, int initialValue);

    /// wait() / P()
    ApiResult<void> semaphoreWait(uint32_t semId);

    /// signal() / V()
    ApiResult<void> semaphoreSignal(uint32_t semId);

    /// Создать мьютекс
    ApiResult<uint32_t> createMutex(const std::string& name);

    /// lock()
    ApiResult<void> mutexLock(uint32_t mutexId);

    /// unlock()
    ApiResult<void> mutexUnlock(uint32_t mutexId);

    // ========================================================================
    // Процессы (требует ProcessSpawn / ProcessList)
    // ========================================================================

    /// Получить количество процессов
    ApiResult<uint32_t> getProcessCount();

    /// Получить список процессов (только с ProcessList)
    ApiResult<std::vector<ProcessInfoView>> getProcessList();

    /// Создать дочерний процесс (только с ProcessSpawn)
    ApiResult<Pid> spawnProcess(const std::string& name, uint32_t burstTime, uint8_t priority = 5);

    // ========================================================================
    // Системная информация (требует SystemInfo)
    // ========================================================================

    /// Информация о системе
    ApiResult<SystemInfoView> getSystemInfo();

    /// Время работы (тики)
    ApiResult<uint64_t> getUptime();

    /// Имя хоста
    ApiResult<std::string> getHostname();

    /// Информация о памяти
    ApiResult<MemoryInfoView> getMemoryInfo();

    // ========================================================================
    // Уведомления (требует UiNotification)
    // ========================================================================

    /// Показать уведомление пользователю
    ApiResult<void> notify(const std::string& title, const std::string& message);

    // ========================================================================
    // Журнал вызовов (внутренний, для ядра)
    // ========================================================================

    /// Получить журнал последних вызовов API
    const std::vector<ApiCallLog>& getCallLog() const;

    /// Очистить журнал
    void clearCallLog();

    /// Максимум записей в журнале
    void setMaxLogSize(size_t maxSize);

private:
    Kernel&         kernel_;
    std::string     appId_;
    Pid             pid_;
    Uid             uid_;

    // Журнал вызовов
    std::vector<ApiCallLog>  callLog_;
    size_t                   maxLogSize_ = 200;

    // --- Внутренние методы ---

    /// Проверить разрешение приложения
    bool checkPermission(uint16_t permission) const;

    /// Записать вызов в журнал
    void logCall(const std::string& method, bool permitted, bool success,
                 const std::string& errorMessage = "");

    /// Конвертировать SyscallStatus → ApiErrorCode
    static ApiErrorCode mapError(uint8_t syscallStatus);

    /// Создать ApiResult<void> ошибку отказа в доступе
    static ApiResult<void> permissionDenied(const std::string& method);

    /// Создать ApiResult<T> ошибку отказа в доступе
    template <typename T>
    static ApiResult<T> permissionDeniedT(const std::string& method) {
        return ApiResult<T>::fail(ApiErrorCode::PermissionDenied,
                                  "Нет разрешения для: " + method);
    }
};

} // namespace re36

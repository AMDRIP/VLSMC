/**
 * @file syscalls.h
 * @brief Определения системных вызовов ядра RAND Elecorner 36.
 *
 * Системные вызовы — единственный способ взаимодействия GUI и
 * пользовательских приложений с ядром. Все запросы проходят через
 * метод Kernel::syscall(SyscallId, SyscallArgs) → SyscallResult.
 */

#pragma once

#include "types.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace re36 {

// ============================================================================
// Идентификаторы системных вызовов
// ============================================================================

/**
 * @enum SyscallId
 * @brief Полный перечень системных вызовов ядра.
 *
 * Сгруппированы по подсистемам. Нумерация оставляет промежутки
 * для будущих расширений.
 */
enum class SyscallId : uint16_t {

    // --- Процессы (100–199) ------------------------------------------------
    CreateProcess       = 100,  ///< Создать процесс. args: name, burstTime, priority
    KillProcess         = 101,  ///< Завершить процесс. args: pid
    SuspendProcess      = 102,  ///< Приостановить процесс. args: pid
    ResumeProcess       = 103,  ///< Возобновить процесс. args: pid
    GetProcessInfo      = 104,  ///< Информация о процессе. args: pid
    GetProcessList      = 105,  ///< Список всех процессов
    SetSchedulerAlgo    = 106,  ///< Сменить алгоритм планировщика. args: algorithm
    GetSchedulerAlgo    = 107,  ///< Текущий алгоритм планировщика
    SetProcessPriority  = 108,  ///< Установить приоритет процесса. args: pid, priority

    // --- Память (200–299) --------------------------------------------------
    AllocMemory         = 200,  ///< Выделить память. args: pid, size
    FreeMemory          = 201,  ///< Освободить память. args: pid, addr
    GetMemoryMap        = 202,  ///< Карта физической памяти
    GetPageTable        = 203,  ///< Таблица страниц процесса. args: pid
    GetMemoryStats      = 204,  ///< Статистика памяти (total, used, page faults...)
    SetAllocationAlgo   = 205,  ///< Сменить алгоритм выделения
    SetPageReplacement  = 206,  ///< Сменить алгоритм замещения страниц
    AccessMemory        = 207,  ///< Обращение к памяти. args: pid, addr, write
    SwapOut             = 210,  ///< Выгрузить страницу в swap. args: pid, page
    SwapIn              = 211,  ///< Загрузить страницу из swap. args: pid, page
    SwapEnable          = 212,  ///< Включить подкачку
    SwapDisable         = 213,  ///< Выключить подкачку
    GetSwapStats        = 214,  ///< Статистика swap

    // --- Файловая система (300–399) ----------------------------------------
    CreateFile          = 300,  ///< Создать файл. args: path, permissions
    OpenFile            = 301,  ///< Открыть файл. args: path, mode
    ReadFile            = 302,  ///< Прочитать файл. args: path
    WriteFile           = 303,  ///< Записать в файл. args: path, data
    DeleteFile          = 304,  ///< Удалить файл. args: path
    RenameFile          = 305,  ///< Переименовать. args: oldPath, newPath
    CopyFile            = 306,  ///< Копировать файл. args: srcPath, dstPath
    StatFile            = 307,  ///< Метаданные файла. args: path
    MakeDirectory       = 310,  ///< Создать каталог. args: path
    RemoveDirectory     = 311,  ///< Удалить каталог. args: path
    ListDirectory       = 312,  ///< Содержимое каталога. args: path
    GetWorkingDir       = 313,  ///< Текущий рабочий каталог. args: pid
    SetWorkingDir       = 314,  ///< Сменить рабочий каталог. args: pid, path
    ChangePermissions   = 320,  ///< Сменить права доступа. args: path, permissions
    ChangeOwner         = 321,  ///< Сменить владельца. args: path, uid
    GetDiskStats        = 330,  ///< Статистика диска (total, used, free, inodes)

    // --- I/O и устройства (400–499) ----------------------------------------
    IoRead              = 400,  ///< Чтение с устройства. args: deviceId, size
    IoWrite             = 401,  ///< Запись на устройство. args: deviceId, data
    GetDeviceList       = 402,  ///< Список устройств
    GetDeviceInfo       = 403,  ///< Информация об устройстве. args: deviceId
    GetDeviceQueue      = 404,  ///< Очередь запросов устройства. args: deviceId
    SetDiskScheduling   = 405,  ///< Сменить алгоритм планирования диска

    // --- IPC (500–599) -----------------------------------------------------
    SemCreate           = 500,  ///< Создать семафор. args: name, initialValue
    SemWait             = 501,  ///< wait() / P(). args: semId, pid
    SemSignal           = 502,  ///< signal() / V(). args: semId, pid
    SemDestroy          = 503,  ///< Удалить семафор. args: semId

    MutexCreate         = 510,  ///< Создать мьютекс. args: name
    MutexLock           = 511,  ///< lock(). args: mutexId, pid
    MutexUnlock         = 512,  ///< unlock(). args: mutexId, pid
    MutexDestroy        = 513,  ///< Удалить мьютекс. args: mutexId

    PipeCreate          = 520,  ///< Создать канал. → pipeId
    PipeRead            = 521,  ///< Прочитать из канала. args: pipeId
    PipeWrite           = 522,  ///< Записать в канал. args: pipeId, data
    PipeClose           = 523,  ///< Закрыть канал. args: pipeId

    MsgQueueCreate      = 530,  ///< Создать очередь сообщений. args: name
    MsgSend             = 531,  ///< Отправить сообщение. args: queueId, data
    MsgReceive          = 532,  ///< Получить сообщение. args: queueId
    MsgQueueDestroy     = 533,  ///< Удалить очередь. args: queueId

    CheckDeadlocks      = 590,  ///< Запустить проверку на тупики

    // --- Пользователи и безопасность (600–699) -----------------------------
    Login               = 600,  ///< Войти в систему. args: username, password
    Logout              = 601,  ///< Выйти из системы. args: uid
    GetCurrentUser      = 602,  ///< Текущий пользователь
    AddUser             = 610,  ///< Создать пользователя. args: username, password, role
    DeleteUser          = 611,  ///< Удалить пользователя. args: uid
    ChangePassword      = 612,  ///< Сменить пароль. args: uid, oldPass, newPass
    GetUserList         = 613,  ///< Список пользователей
    SwitchUser          = 620,  ///< Переключить пользователя. args: username, password
    GetAuditLog         = 690,  ///< Получить журнал аудита

    // --- Приложения (700–799) ----------------------------------------------
    InstallApp          = 700,  ///< Установить приложение. args: packagePath
    UninstallApp        = 701,  ///< Удалить приложение. args: appId
    RunApp              = 702,  ///< Запустить приложение. args: appId
    StopApp             = 703,  ///< Остановить приложение. args: appId / pid
    GetInstalledApps    = 704,  ///< Список установленных приложений
    GetAppInfo          = 705,  ///< Информация о приложении. args: appId

    // --- VGA (800–899) -----------------------------------------------------
    VgaClear            = 800,  ///< Очистить экран VGA
    VgaSetCursor        = 801,  ///< Установить позицию. args: x, y
    VgaSetColor         = 802,  ///< Установить цвет. args: fg, bg
    VgaPrint            = 803,  ///< Напечатать текст (raw). args: text

    // --- Система (900–999) -------------------------------------------------
    GetKernelState      = 900,  ///< Состояние ядра
    GetUptime           = 901,  ///< Время работы (тики)
    GetHostname         = 902,  ///< Имя хоста
    GetSystemStats      = 903,  ///< Общая статистика (CPU%, RAM%, processes)
    Shutdown            = 910,  ///< Завершение работы
    Reboot              = 911   ///< Перезагрузка
};

// ============================================================================
// Аргументы системного вызова
// ============================================================================

/**
 * @struct SyscallArgs
 * @brief Аргументы, передаваемые в системный вызов.
 *
 * Используется словарь (string → KernelValue) для гибкости.
 * Каждый вызов определяет, какие ключи ожидаются.
 */
struct SyscallArgs {
    /// Именованные аргументы
    std::unordered_map<std::string, KernelValue> params;

    // --- Удобные хелперы ---

    void set(const std::string& key, int64_t value);
    void set(const std::string& key, uint64_t value);
    void set(const std::string& key, const std::string& value);
    void set(const std::string& key, bool value);

    int64_t     getInt(const std::string& key, int64_t defaultVal = 0) const;
    uint64_t    getUint(const std::string& key, uint64_t defaultVal = 0) const;
    std::string getString(const std::string& key, const std::string& defaultVal = "") const;
    bool        getBool(const std::string& key, bool defaultVal = false) const;

    bool hasKey(const std::string& key) const;
};

// ============================================================================
// Результат системного вызова
// ============================================================================

/**
 * @enum SyscallStatus
 * @brief Статус выполнения системного вызова.
 */
enum class SyscallStatus : uint8_t {
    Ok,                 ///< Успех
    Error,              ///< Общая ошибка
    PermissionDenied,   ///< Недостаточно прав
    NotFound,           ///< Ресурс не найден
    AlreadyExists,      ///< Ресурс уже существует
    InvalidArgument,    ///< Некорректный аргумент
    OutOfMemory,        ///< Недостаточно памяти
    OutOfDiskSpace,     ///< Недостаточно места на диске
    ResourceBusy,       ///< Ресурс занят
    ResourceLimit,      ///< Достигнут предел ресурса
    InvalidAddress,     ///< Некорректный адрес
    NotSupported        ///< Операция не поддерживается
};

// ============================================================================
// Короткие алиасы SYS_* для использования в switch-блоках ядра
// ============================================================================

// Процессы
inline constexpr auto SYS_PROC_CREATE        = SyscallId::CreateProcess;
inline constexpr auto SYS_PROC_KILL          = SyscallId::KillProcess;
inline constexpr auto SYS_PROC_SUSPEND       = SyscallId::SuspendProcess;
inline constexpr auto SYS_PROC_RESUME        = SyscallId::ResumeProcess;
inline constexpr auto SYS_PROC_INFO          = SyscallId::GetProcessInfo;
inline constexpr auto SYS_PROC_LIST          = SyscallId::GetProcessList;
inline constexpr auto SYS_PROC_SET_SCHEDULER = SyscallId::SetSchedulerAlgo;
inline constexpr auto SYS_PROC_GET_SCHEDULER = SyscallId::GetSchedulerAlgo;
inline constexpr auto SYS_PROC_SET_PRIORITY  = SyscallId::SetProcessPriority;

// Память
inline constexpr auto SYS_MEM_ALLOC           = SyscallId::AllocMemory;
inline constexpr auto SYS_MEM_FREE            = SyscallId::FreeMemory;
inline constexpr auto SYS_MEM_MAP             = SyscallId::GetMemoryMap;
inline constexpr auto SYS_MEM_PAGE_TABLE      = SyscallId::GetPageTable;
inline constexpr auto SYS_MEM_STATS           = SyscallId::GetMemoryStats;
inline constexpr auto SYS_MEM_SET_ALLOC       = SyscallId::SetAllocationAlgo;
inline constexpr auto SYS_MEM_SET_REPLACE     = SyscallId::SetPageReplacement;
inline constexpr auto SYS_MEM_ACCESS          = SyscallId::AccessMemory;

// Swap
inline constexpr auto SYS_MEM_SWAP_OUT        = SyscallId::SwapOut;
inline constexpr auto SYS_MEM_SWAP_IN         = SyscallId::SwapIn;
inline constexpr auto SYS_MEM_SWAP_ENABLE     = SyscallId::SwapEnable;
inline constexpr auto SYS_MEM_SWAP_DISABLE    = SyscallId::SwapDisable;
inline constexpr auto SYS_MEM_SWAP_STATS      = SyscallId::GetSwapStats;

// Файловая система
inline constexpr auto SYS_FS_CREATE           = SyscallId::CreateFile;
inline constexpr auto SYS_FS_OPEN             = SyscallId::OpenFile;
inline constexpr auto SYS_FS_READ             = SyscallId::ReadFile;
inline constexpr auto SYS_FS_WRITE            = SyscallId::WriteFile;
inline constexpr auto SYS_FS_DELETE           = SyscallId::DeleteFile;
inline constexpr auto SYS_FS_RENAME           = SyscallId::RenameFile;
inline constexpr auto SYS_FS_COPY             = SyscallId::CopyFile;
inline constexpr auto SYS_FS_STAT             = SyscallId::StatFile;
inline constexpr auto SYS_FS_MKDIR            = SyscallId::MakeDirectory;
inline constexpr auto SYS_FS_RMDIR            = SyscallId::RemoveDirectory;
inline constexpr auto SYS_FS_LISTDIR          = SyscallId::ListDirectory;
inline constexpr auto SYS_FS_GETWD            = SyscallId::GetWorkingDir;
inline constexpr auto SYS_FS_SETWD            = SyscallId::SetWorkingDir;
inline constexpr auto SYS_FS_CHMOD            = SyscallId::ChangePermissions;
inline constexpr auto SYS_FS_CHOWN            = SyscallId::ChangeOwner;
inline constexpr auto SYS_FS_DISK_STATS       = SyscallId::GetDiskStats;

// I/O
inline constexpr auto SYS_IO_READ             = SyscallId::IoRead;
inline constexpr auto SYS_IO_WRITE            = SyscallId::IoWrite;
inline constexpr auto SYS_IO_DEVICE_LIST      = SyscallId::GetDeviceList;
inline constexpr auto SYS_IO_DEVICE_INFO      = SyscallId::GetDeviceInfo;
inline constexpr auto SYS_IO_DEVICE_QUEUE     = SyscallId::GetDeviceQueue;
inline constexpr auto SYS_IO_SET_DISK_SCHED   = SyscallId::SetDiskScheduling;

// IPC
inline constexpr auto SYS_IPC_SEM_CREATE      = SyscallId::SemCreate;
inline constexpr auto SYS_IPC_SEM_WAIT        = SyscallId::SemWait;
inline constexpr auto SYS_IPC_SEM_SIGNAL      = SyscallId::SemSignal;
inline constexpr auto SYS_IPC_SEM_DESTROY     = SyscallId::SemDestroy;
inline constexpr auto SYS_IPC_MUTEX_CREATE    = SyscallId::MutexCreate;
inline constexpr auto SYS_IPC_MUTEX_LOCK      = SyscallId::MutexLock;
inline constexpr auto SYS_IPC_MUTEX_UNLOCK    = SyscallId::MutexUnlock;
inline constexpr auto SYS_IPC_MUTEX_DESTROY   = SyscallId::MutexDestroy;
inline constexpr auto SYS_IPC_PIPE_CREATE     = SyscallId::PipeCreate;
inline constexpr auto SYS_IPC_PIPE_READ       = SyscallId::PipeRead;
inline constexpr auto SYS_IPC_PIPE_WRITE      = SyscallId::PipeWrite;
inline constexpr auto SYS_IPC_PIPE_CLOSE      = SyscallId::PipeClose;
inline constexpr auto SYS_IPC_MQ_CREATE       = SyscallId::MsgQueueCreate;
inline constexpr auto SYS_IPC_MQ_SEND         = SyscallId::MsgSend;
inline constexpr auto SYS_IPC_MQ_RECV         = SyscallId::MsgReceive;
inline constexpr auto SYS_IPC_MQ_DESTROY      = SyscallId::MsgQueueDestroy;
inline constexpr auto SYS_IPC_DEADLOCK        = SyscallId::CheckDeadlocks;

// Пользователи
inline constexpr auto SYS_USER_LOGIN          = SyscallId::Login;
inline constexpr auto SYS_USER_LOGOUT         = SyscallId::Logout;
inline constexpr auto SYS_USER_CURRENT        = SyscallId::GetCurrentUser;
inline constexpr auto SYS_USER_ADD            = SyscallId::AddUser;
inline constexpr auto SYS_USER_DELETE         = SyscallId::DeleteUser;
inline constexpr auto SYS_USER_CHANGE_PASS    = SyscallId::ChangePassword;
inline constexpr auto SYS_USER_LIST           = SyscallId::GetUserList;
inline constexpr auto SYS_USER_SWITCH         = SyscallId::SwitchUser;
inline constexpr auto SYS_USER_AUDIT          = SyscallId::GetAuditLog;

// Приложения
inline constexpr auto SYS_APP_INSTALL         = SyscallId::InstallApp;
inline constexpr auto SYS_APP_UNINSTALL       = SyscallId::UninstallApp;
inline constexpr auto SYS_APP_RUN             = SyscallId::RunApp;
inline constexpr auto SYS_APP_STOP            = SyscallId::StopApp;
inline constexpr auto SYS_APP_LIST            = SyscallId::GetInstalledApps;
inline constexpr auto SYS_APP_INFO            = SyscallId::GetAppInfo;

// VGA
inline constexpr auto SYS_VGA_CLEAR           = SyscallId::VgaClear;
inline constexpr auto SYS_VGA_SET_CURSOR      = SyscallId::VgaSetCursor;
inline constexpr auto SYS_VGA_SET_COLOR       = SyscallId::VgaSetColor;
inline constexpr auto SYS_VGA_PRINT           = SyscallId::VgaPrint;

// Система
inline constexpr auto SYS_SYSTEM_INFO         = SyscallId::GetKernelState;
inline constexpr auto SYS_SYSTEM_UPTIME       = SyscallId::GetUptime;
inline constexpr auto SYS_SYSTEM_HOSTNAME     = SyscallId::GetHostname;
inline constexpr auto SYS_SYSTEM_STATS        = SyscallId::GetSystemStats;
inline constexpr auto SYS_SYSTEM_SHUTDOWN     = SyscallId::Shutdown;
inline constexpr auto SYS_SYSTEM_REBOOT       = SyscallId::Reboot;

/**
 * @struct SyscallResult
 * @brief Результат системного вызова.
 */
struct SyscallResult {
    SyscallStatus status = SyscallStatus::Ok;
    std::string   errorMessage;  ///< Описание ошибки (пусто при Ok)

    /// Данные результата (ключ → значение)
    std::unordered_map<std::string, KernelValue> data;

    // --- Фабричные методы ---

    /// Успешный результат без данных
    static SyscallResult ok();

    /// Успешный результат с одним значением
    static SyscallResult ok(const std::string& key, KernelValue value);

    /// Ошибка
    static SyscallResult error(SyscallStatus status, const std::string& message);

    // --- Удобные геттеры ---

    bool isOk() const { return status == SyscallStatus::Ok; }

    int64_t     getInt(const std::string& key, int64_t defaultVal = 0) const;
    uint64_t    getUint(const std::string& key, uint64_t defaultVal = 0) const;
    std::string getString(const std::string& key, const std::string& defaultVal = "") const;
    bool        getBool(const std::string& key, bool defaultVal = false) const;
};

} // namespace re36

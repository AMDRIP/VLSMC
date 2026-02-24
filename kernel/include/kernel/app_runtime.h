/**
 * @file app_runtime.h
 * @brief Среда исполнения пользовательских приложений RAND Elecorner 36.
 *
 * Управляет жизненным циклом JS-приложений, запускаемых внутри
 * песочницы (QJSEngine). Обеспечивает установку, удаление, запуск,
 * остановку и изоляцию приложений.
 *
 * Этот заголовок описывает интерфейс среды на уровне ядра (C++).
 * Фактическая интеграция с QJSEngine происходит на уровне GUI.
 * Соответствует требованиям APP-01 — APP-06.
 */

#pragma once

#include "types.h"
#include "app_api.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <unordered_set>
#include <memory>

namespace re36 {

// Предварительные объявления
class EventBus;
class FileSystem;
class Scheduler;

// ============================================================================
// Разрешения приложений (APP-05)
// ============================================================================

/**
 * @enum AppPermission
 * @brief Разрешения, которые приложение запрашивает.
 */
enum class AppPermission : uint16_t {
    UiWindow,           ///< Создание окон
    UiNotification,     ///< Показ уведомлений
    FilesystemRead,     ///< Чтение файлов
    FilesystemWrite,    ///< Запись файлов
    ProcessSpawn,       ///< Порождение процессов
    ProcessList,        ///< Просмотр списка процессов
    IpcSend,            ///< Отправка сообщений IPC
    IpcReceive,         ///< Получение сообщений IPC
    SystemInfo,         ///< Запрос системной информации
    Network             ///< Сетевой доступ (зарезервировано)
};

// ============================================================================
// Манифест приложения (APP-02)
// ============================================================================

/**
 * @struct AppManifest
 * @brief Метаданные приложения из manifest.json.
 */
struct AppManifest {
    std::string                     id;             ///< "com.example.myapp"
    std::string                     name;           ///< "Мой Калькулятор"
    std::string                     version;        ///< "1.0.0"
    std::string                     author;
    std::string                     description;
    std::string                     entryPoint;     ///< "main.js"
    std::string                     icon;           ///< Путь к иконке
    uint32_t                        minApiVersion = 1;
    std::unordered_set<AppPermission> permissions;   ///< Запрошенные разрешения

    // Параметры окна по умолчанию
    struct WindowConfig {
        uint32_t    width       = 800;
        uint32_t    height      = 600;
        uint32_t    minWidth    = 200;
        uint32_t    minHeight   = 150;
        bool        resizable   = true;
    } window;
};

// ============================================================================
// Установленное приложение (APP-03)
// ============================================================================

/**
 * @struct InstalledApp
 * @brief Запись установленного приложения в реестре.
 */
struct InstalledApp {
    AppManifest     manifest;
    std::string     installPath;        ///< Путь в виртуальной ФС (/apps/<id>)
    Tick            installedAt = 0;
    Uid             installedBy = ROOT_UID;
    bool            isSystem    = false; ///< Системное приложение (нельзя удалить)
};

// ============================================================================
// Экземпляр работающего приложения (APP-04)
// ============================================================================

/**
 * @enum AppInstanceState
 * @brief Состояние экземпляра приложения.
 */
enum class AppInstanceState : uint8_t {
    Starting,       ///< Загрузка скрипта
    Running,        ///< Работает
    Paused,         ///< На паузе
    Stopping,       ///< Останавливается
    Crashed,        ///< Упало с ошибкой
    Stopped         ///< Остановлено
};

/**
 * @struct AppInstance
 * @brief Работающий экземпляр приложения.
 */
struct AppInstance {
    uint32_t            instanceId;
    std::string         appId;
    Pid                 processPid;         ///< PID процесса в планировщике
    AppInstanceState    state       = AppInstanceState::Starting;
    Tick                startedAt   = 0;
    Tick                stoppedAt   = 0;
    std::string         errorMessage;       ///< Сообщение об ошибке (если crashed)
    Uid                 launchedBy  = ROOT_UID;
    std::shared_ptr<AppApi> api;            ///< Объект API для приложения
};

// ============================================================================
// Информация о приложении (для GUI)
// ============================================================================

/**
 * @struct AppInfo
 * @brief Публичная информация о приложении для GUI.
 */
struct AppInfo {
    std::string         id;
    std::string         name;
    std::string         version;
    std::string         author;
    std::string         description;
    std::string         icon;
    bool                isInstalled;
    bool                isRunning;
    bool                isSystem;
    Tick                installedAt;
};

// ============================================================================
// Среда исполнения приложений
// ============================================================================

/**
 * @class AppRuntime
 * @brief Управление жизненным циклом приложений.
 *
 * На уровне ядра AppRuntime управляет:
 * - Реестром установленных приложений
 * - Установкой / удалением пакетов
 * - Созданием процессов и экземпляров
 * - Проверкой разрешений
 *
 * Собственно исполнение JS-кода происходит на уровне GUI
 * (QJSEngine), куда ядро передаёт путь к скрипту и PID процесса.
 */
class AppRuntime {
public:
    explicit AppRuntime(EventBus& eventBus, FileSystem& fs, Scheduler& scheduler,
                        const KernelConfig& config);
    ~AppRuntime();

    AppRuntime(const AppRuntime&) = delete;
    AppRuntime& operator=(const AppRuntime&) = delete;

    // ========================================================================
    // Связь с ядром
    // ========================================================================

    /**
     * Установить ссылку на ядро.
     * Вызывается Kernel::boot() после создания AppRuntime,
     * чтобы разорвать циклическую зависимость.
     */
    void setKernel(Kernel& kernel);

    // ========================================================================
    // Инициализация
    // ========================================================================

    /**
     * Инициализировать среду: создать каталог /apps,
     * установить встроенные приложения (терминал, менеджер файлов и т.д.)
     */
    bool init();

    // ========================================================================
    // Управление пакетами (APP-03)
    // ========================================================================

    /**
     * Установить приложение из пакета (.vlsmc-pkg).
     * @param packagePath Путь к пакету в виртуальной ФС
     * @param installerUid UID установщика
     * @return ID приложения или пустая строка при ошибке
     */
    std::string installApp(const std::string& packagePath, Uid installerUid = ROOT_UID);

    /**
     * Удалить приложение.
     * @param appId ID приложения
     * @param requestorUid UID запросившего
     */
    bool uninstallApp(const std::string& appId, Uid requestorUid = ROOT_UID);

    // ========================================================================
    // Запуск / остановка (APP-04)
    // ========================================================================

    /**
     * Запустить приложение.
     * Создаёт процесс в планировщике и экземпляр AppInstance.
     * @param appId   ID приложения
     * @param launcherUid UID запускающего
     * @return ID экземпляра или 0 при ошибке
     */
    uint32_t runApp(const std::string& appId, Uid launcherUid = ROOT_UID);

    /**
     * Остановить экземпляр приложения.
     * @param instanceId ID экземпляра
     */
    bool stopApp(uint32_t instanceId);

    /**
     * Остановить все экземпляры приложения.
     * @param appId ID приложения
     */
    bool stopAppByAppId(const std::string& appId);

    // ========================================================================
    // Запросы
    // ========================================================================

    /// Список установленных приложений
    std::vector<AppInfo> getInstalledApps() const;

    /// Информация о приложении
    std::optional<AppInfo> getAppInfo(const std::string& appId) const;

    /// Работающие экземпляры
    std::vector<AppInstance> getRunningInstances() const;

    /// Экземпляр по PID процесса
    std::optional<AppInstance> getInstanceByPid(Pid pid) const;

    /// Приложение установлено?
    bool isInstalled(const std::string& appId) const;

    /// Приложение запущено?
    bool isRunning(const std::string& appId) const;

    // ========================================================================
    // App API
    // ========================================================================

    /**
     * Получить объект API экземпляра приложения.
     * @param instanceId ID экземпляра
     * @return Указатель на AppApi или nullptr если экземпляр не найден
     */
    AppApi* getAppApi(uint32_t instanceId);

    /**
     * Получить объект API по PID процесса.
     * @param pid PID процесса
     * @return Указатель на AppApi или nullptr если не найден
     */
    AppApi* getAppApiByPid(Pid pid);

    // ========================================================================
    // Проверка разрешений (APP-05)
    // ========================================================================

    /**
     * Имеет ли приложение указанное разрешение?
     * @param appId      ID приложения
     * @param permission Запрашиваемое разрешение
     */
    bool hasPermission(const std::string& appId, AppPermission permission) const;

    /**
     * Проверить разрешение и опубликовать событие PermissionDenied при отказе.
     */
    bool checkPermission(const std::string& appId, AppPermission permission);

    // ========================================================================
    // Очистка
    // ========================================================================

    /// Остановить все приложения (при shutdown)
    void stopAllApps();

    /// Очистить экземпляры, связанные с завершённым процессом
    void onProcessTerminated(Pid pid);

private:
    EventBus&                                       eventBus_;
    FileSystem&                                     fileSystem_;
    Scheduler&                                      scheduler_;
    KernelConfig                                    config_;
    Kernel*                                         kernel_ = nullptr;  ///< Устанавливается через setKernel()

    // Реестр установленных приложений
    std::unordered_map<std::string, InstalledApp>   registry_;

    // Работающие экземпляры
    std::unordered_map<uint32_t, AppInstance>        instances_;
    uint32_t                                        nextInstanceId_ = 1;

    // PID → instanceId (для быстрого поиска)
    std::unordered_map<Pid, uint32_t>               pidToInstance_;

    // --- Внутренние методы ---

    /// Разобрать manifest.json
    std::optional<AppManifest> parseManifest(const std::string& jsonContent) const;

    /// Проверить манифест на корректность
    bool validateManifest(const AppManifest& manifest) const;

    /// Зарегистрировать встроенные приложения
    void registerBuiltinApps();
};

} // namespace re36

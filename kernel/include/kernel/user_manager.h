/**
 * @file user_manager.h
 * @brief Управление пользователями и безопасностью RAND Elecorner 36.
 *
 * Учётные записи, аутентификация, роли, журнал аудита.
 * Соответствует требованиям USR-01 — USR-06.
 */

#pragma once

#include "types.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace re36 {

// Предварительные объявления
class EventBus;

// ============================================================================
// Роли пользователей (USR-02)
// ============================================================================

/**
 * @enum UserRole
 * @brief Роль пользователя в системе.
 */
enum class UserRole : uint8_t {
    Root,       ///< Суперпользователь (полные права)
    User        ///< Обычный пользователь
};

// ============================================================================
// Учётная запись (USR-01)
// ============================================================================

/**
 * @struct UserAccount
 * @brief Учётная запись пользователя.
 */
struct UserAccount {
    Uid             uid;
    std::string     username;
    std::string     passwordHash;       ///< Хеш пароля (не открытый текст)
    UserRole        role            = UserRole::User;
    Gid             primaryGroup    = 0;
    std::string     homeDirectory;      ///< "/home/<username>"
    std::string     shell           = "/bin/sh";
    Tick            createdAt       = 0;
    Tick            lastLoginAt     = 0;
    bool            isActive        = true; ///< Аккаунт не заблокирован
};

// ============================================================================
// Публичная информация о пользователе (для GUI)
// ============================================================================

/**
 * @struct UserInfo
 * @brief Информация о пользователе без пароля.
 */
struct UserInfo {
    Uid             uid;
    std::string     username;
    UserRole        role;
    std::string     homeDirectory;
    Tick            createdAt;
    Tick            lastLoginAt;
    bool            isActive;
};

// ============================================================================
// Сессия (USR-03)
// ============================================================================

/**
 * @struct Session
 * @brief Активная пользовательская сессия.
 */
struct Session {
    uint32_t        sessionId;
    Uid             uid;
    std::string     username;
    UserRole        role;
    Tick            loginAt;
};

// ============================================================================
// Журнал аудита (USR-06)
// ============================================================================

/**
 * @enum AuditAction
 * @brief Тип действия в журнале аудита.
 */
enum class AuditAction : uint8_t {
    Login,
    LoginFailed,
    Logout,
    UserCreated,
    UserDeleted,
    PasswordChanged,
    PermissionDenied,
    ProcessKilled,
    FileAccess,
    SudoCommand,
    SwitchUser
};

/**
 * @struct AuditLogEntry
 * @brief Одна запись журнала аудита.
 */
struct AuditLogEntry {
    Tick            tick;
    Uid             uid;
    std::string     username;
    AuditAction     action;
    std::string     details;
    bool            success;
};

// ============================================================================
// Менеджер пользователей
// ============================================================================

/**
 * @class UserManager
 * @brief Управление пользователями, аутентификация, авторизация.
 */
class UserManager {
public:
    explicit UserManager(EventBus& eventBus, const KernelConfig& config);
    ~UserManager();

    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;

    // ========================================================================
    // Инициализация
    // ========================================================================

    /**
     * Инициализировать менеджер.
     * Создаёт пользователя root с паролем из KernelConfig.
     */
    bool init();

    // ========================================================================
    // Аутентификация (USR-03)
    // ========================================================================

    /**
     * Войти в систему.
     * @param username Имя пользователя
     * @param password Пароль (открытый текст, хешируется внутри)
     * @return ID сессии или nullopt при ошибке
     */
    std::optional<uint32_t> login(const std::string& username, const std::string& password);

    /**
     * Выйти из системы.
     * @param sessionId ID сессии
     */
    bool logout(uint32_t sessionId);

    /**
     * Переключить пользователя (su).
     * @param sessionId  Текущая сессия
     * @param username   Целевой пользователь
     * @param password   Пароль целевого пользователя
     * @return Новый ID сессии или nullopt
     */
    std::optional<uint32_t> switchUser(uint32_t sessionId,
                                        const std::string& username,
                                        const std::string& password);

    // ========================================================================
    // Управление аккаунтами (USR-05)
    // ========================================================================

    /**
     * Создать пользователя (только root).
     * @param requestorUid UID запрашивающего
     * @param username     Имя нового пользователя
     * @param password     Пароль
     * @param role         Роль
     * @return UID нового пользователя или nullopt
     */
    std::optional<Uid> addUser(Uid requestorUid, const std::string& username,
                               const std::string& password, UserRole role = UserRole::User);

    /**
     * Удалить пользователя (только root).
     */
    bool deleteUser(Uid requestorUid, Uid targetUid);

    /**
     * Сменить пароль.
     * @param uid       UID пользователя
     * @param oldPass   Старый пароль
     * @param newPass   Новый пароль
     */
    bool changePassword(Uid uid, const std::string& oldPass, const std::string& newPass);

    // ========================================================================
    // Запросы
    // ========================================================================

    /// Информация о текущем пользователе сессии
    std::optional<UserInfo> getCurrentUser(uint32_t sessionId) const;

    /// Информация о пользователе по UID
    std::optional<UserInfo> getUserInfo(Uid uid) const;

    /// Информация о пользователе по имени
    std::optional<UserInfo> getUserInfo(const std::string& username) const;

    /// Список всех пользователей
    std::vector<UserInfo> getUserList() const;

    /// Активные сессии
    std::vector<Session> getActiveSessions() const;

    // ========================================================================
    // Авторизация (USR-04)
    // ========================================================================

    /// Пользователь является root?
    bool isRoot(Uid uid) const;

    /**
     * Может ли пользователь выполнить действие над процессом?
     * Root может всё. Обычный пользователь — только свои процессы.
     */
    bool canKillProcess(Uid requestorUid, Uid processOwnerUid) const;

    /**
     * Может ли пользователь читать/писать файл с данными правами?
     */
    bool canAccessFile(Uid requestorUid, Uid fileOwnerUid,
                       FilePermissions perms, bool read, bool write, bool execute) const;

    // ========================================================================
    // Журнал аудита (USR-06)
    // ========================================================================

    /// Получить записи аудита (последние N)
    std::vector<AuditLogEntry> getAuditLog(size_t count = 200) const;

    /// Получить записи аудита для конкретного пользователя
    std::vector<AuditLogEntry> getAuditLogForUser(Uid uid, size_t count = 100) const;

    // ========================================================================
    // Персистентность
    // ========================================================================

    /// Сохранить пользователей в файл
    bool saveToFile(const std::string& path) const;

    /// Загрузить пользователей из файла
    bool loadFromFile(const std::string& path);

private:
    EventBus&                                   eventBus_;
    KernelConfig                                config_;

    // Пользователи
    std::unordered_map<Uid, UserAccount>        users_;
    Uid                                         nextUid_ = 1; // 0 = root

    // Сессии
    std::unordered_map<uint32_t, Session>       sessions_;
    uint32_t                                    nextSessionId_ = 1;

    // Журнал аудита
    std::vector<AuditLogEntry>                  auditLog_;
    static constexpr size_t MAX_AUDIT_LOG = 10000;

    // Текущий тик
    Tick                                        currentTick_ = 0;

    // --- Внутренние методы ---

    /// Хешировать пароль (простая реализация для симулятора)
    std::string hashPassword(const std::string& password) const;

    /// Проверить пароль
    bool verifyPassword(const std::string& password, const std::string& hash) const;

    /// Записать в журнал аудита
    void audit(Uid uid, const std::string& username, AuditAction action,
               const std::string& details, bool success);

    /// Найти пользователя по имени
    std::optional<Uid> findByUsername(const std::string& username) const;
};

} // namespace re36

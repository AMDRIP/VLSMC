/**
 * @file user_manager.cpp
 * @brief Реализация управления пользователями RAND Elecorner 36.
 */

#include "kernel/user_manager.h"
#include "kernel/event_bus.h"

#include <algorithm>
#include <functional>

namespace re36 {

UserManager::UserManager(EventBus& eventBus, const KernelConfig& config)
    : eventBus_(eventBus), config_(config) {}

UserManager::~UserManager() = default;

bool UserManager::init() {
    accounts_.clear();
    sessions_.clear();
    auditLog_.clear();

    // Создать root
    UserAccount root;
    root.uid = ROOT_UID;
    root.username = "root";
    root.passwordHash = hashPassword("root");
    root.role = UserRole::Root;
    root.homeDir = "/home/root";
    root.enabled = true;
    accounts_[ROOT_UID] = std::move(root);
    nextUid_ = ROOT_UID + 1;

    audit("system", "init", "Менеджер пользователей инициализирован");
    return true;
}

// ---- Управление учётными записями ------------------------------------------

std::optional<Uid> UserManager::createUser(const std::string& username,
                                            const std::string& password,
                                            UserRole role, Uid requestor) {
    if (requestor != ROOT_UID) {
        audit(resolveUsername(requestor), "create_user_denied", username);
        return std::nullopt;
    }
    // Уникальность имени
    for (auto& [id, acc] : accounts_) {
        if (acc.username == username) return std::nullopt;
    }

    Uid uid = nextUid_++;
    UserAccount acc;
    acc.uid = uid;
    acc.username = username;
    acc.passwordHash = hashPassword(password);
    acc.role = role;
    acc.homeDir = "/home/" + username;
    acc.enabled = true;
    accounts_[uid] = std::move(acc);

    audit("root", "create_user", username + " uid=" + std::to_string(uid));

    Event evt(EventType::UserCreated, 0, "user_manager");
    evt.with("uid", static_cast<int64_t>(uid)).with("username", username);
    eventBus_.publish(evt);

    return uid;
}

bool UserManager::deleteUser(Uid uid, Uid requestor) {
    if (requestor != ROOT_UID || uid == ROOT_UID) return false;
    auto it = accounts_.find(uid);
    if (it == accounts_.end()) return false;

    std::string name = it->second.username;
    // Закрыть все сессии
    logoutAll(uid);
    accounts_.erase(it);

    audit("root", "delete_user", name);
    return true;
}

bool UserManager::changePassword(Uid uid, const std::string& oldPass,
                                  const std::string& newPass) {
    auto it = accounts_.find(uid);
    if (it == accounts_.end()) return false;
    if (it->second.passwordHash != hashPassword(oldPass)) {
        audit(it->second.username, "change_password_failed", "Неверный старый пароль");
        return false;
    }
    it->second.passwordHash = hashPassword(newPass);
    audit(it->second.username, "change_password", "OK");
    return true;
}

bool UserManager::setUserRole(Uid uid, UserRole role, Uid requestor) {
    if (requestor != ROOT_UID) return false;
    auto it = accounts_.find(uid);
    if (it == accounts_.end()) return false;
    it->second.role = role;
    audit("root", "set_role", it->second.username + " → " +
          (role == UserRole::Root ? "root" : "user"));
    return true;
}

bool UserManager::enableUser(Uid uid, bool enabled, Uid requestor) {
    if (requestor != ROOT_UID || uid == ROOT_UID) return false;
    auto it = accounts_.find(uid);
    if (it == accounts_.end()) return false;
    it->second.enabled = enabled;
    if (!enabled) logoutAll(uid);
    return true;
}

// ---- Аутентификация --------------------------------------------------------

std::optional<SessionId> UserManager::login(const std::string& username,
                                             const std::string& password) {
    Uid uid = INVALID_UID;
    for (auto& [id, acc] : accounts_) {
        if (acc.username == username) { uid = id; break; }
    }
    if (uid == INVALID_UID) {
        audit(username, "login_failed", "Пользователь не найден");
        Event evt(EventType::LoginFailed, 0, "user_manager");
        evt.with("username", username);
        eventBus_.publish(evt);
        return std::nullopt;
    }

    auto& acc = accounts_[uid];
    if (!acc.enabled) {
        audit(username, "login_failed", "Учётная запись отключена");
        return std::nullopt;
    }
    if (acc.passwordHash != hashPassword(password)) {
        acc.failedLogins++;
        audit(username, "login_failed", "Неверный пароль");
        Event evt(EventType::LoginFailed, 0, "user_manager");
        evt.with("username", username);
        eventBus_.publish(evt);
        return std::nullopt;
    }

    SessionId sid = nextSessionId_++;
    Session session;
    session.id = sid;
    session.uid = uid;
    session.username = username;
    session.active = true;
    sessions_[sid] = std::move(session);

    acc.failedLogins = 0;
    audit(username, "login", "Сессия " + std::to_string(sid));

    Event evt(EventType::UserLoggedIn, 0, "user_manager");
    evt.with("uid", static_cast<int64_t>(uid)).with("username", username)
       .with("session", static_cast<int64_t>(sid));
    eventBus_.publish(evt);

    return sid;
}

bool UserManager::logout(SessionId sid) {
    auto it = sessions_.find(sid);
    if (it == sessions_.end()) return false;
    it->second.active = false;
    audit(it->second.username, "logout", "Сессия " + std::to_string(sid));

    Event evt(EventType::UserLoggedOut, 0, "user_manager");
    evt.with("uid", static_cast<int64_t>(it->second.uid));
    eventBus_.publish(evt);

    sessions_.erase(it);
    return true;
}

bool UserManager::isLoggedIn(Uid uid) const {
    for (auto& [id, s] : sessions_) {
        if (s.uid == uid && s.active) return true;
    }
    return false;
}

std::optional<Session> UserManager::getSession(SessionId sid) const {
    auto it = sessions_.find(sid);
    return it != sessions_.end() ? std::optional(it->second) : std::nullopt;
}

std::vector<Session> UserManager::getActiveSessions() const {
    std::vector<Session> result;
    for (auto& [id, s] : sessions_) {
        if (s.active) result.push_back(s);
    }
    return result;
}

// ---- Авторизация -----------------------------------------------------------

bool UserManager::canKillProcess(Uid uid, Uid processOwner) const {
    if (uid == ROOT_UID) return true;
    return uid == processOwner;
}

bool UserManager::canAccessFile(Uid uid, Uid fileOwner,
                                 const FilePermissions& perms, bool write) const {
    if (uid == ROOT_UID) return true;
    if (uid == fileOwner) {
        return write ? perms.ownerWrite : perms.ownerRead;
    }
    return write ? perms.otherWrite : perms.otherRead;
}

bool UserManager::canManageUsers(Uid uid) const {
    auto it = accounts_.find(uid);
    return it != accounts_.end() && it->second.role == UserRole::Root;
}

// ---- Запросы ---------------------------------------------------------------

std::optional<UserAccount> UserManager::getAccount(Uid uid) const {
    auto it = accounts_.find(uid);
    if (it == accounts_.end()) return std::nullopt;
    UserAccount copy = it->second;
    copy.passwordHash = "***";  // не показывать хеш
    return copy;
}

std::vector<UserAccount> UserManager::getAllAccounts() const {
    std::vector<UserAccount> result;
    for (auto& [id, acc] : accounts_) {
        UserAccount copy = acc;
        copy.passwordHash = "***";
        result.push_back(copy);
    }
    return result;
}

std::optional<Uid> UserManager::resolveUid(const std::string& username) const {
    for (auto& [id, acc] : accounts_) {
        if (acc.username == username) return id;
    }
    return std::nullopt;
}

std::string UserManager::resolveUsername(Uid uid) const {
    auto it = accounts_.find(uid);
    return it != accounts_.end() ? it->second.username : "uid" + std::to_string(uid);
}

// ---- Аудит -----------------------------------------------------------------

std::vector<AuditEntry> UserManager::getAuditLog(size_t count) const {
    if (count == 0 || count >= auditLog_.size()) return auditLog_;
    return std::vector<AuditEntry>(
        auditLog_.end() - static_cast<ptrdiff_t>(count), auditLog_.end());
}

// ---- Внутренние ------------------------------------------------------------

std::string UserManager::hashPassword(const std::string& password) const {
    // Простой хеш для симулятора (djb2)
    uint64_t hash = 5381;
    for (char c : password) {
        hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
    }
    return "hash:" + std::to_string(hash);
}

void UserManager::audit(const std::string& who, const std::string& action,
                         const std::string& detail) {
    auditLog_.push_back({who, action, detail, 0});
    if (auditLog_.size() > 10000) {
        auditLog_.erase(auditLog_.begin(), auditLog_.begin() + 5000);
    }
}

void UserManager::logoutAll(Uid uid) {
    std::vector<SessionId> toRemove;
    for (auto& [id, s] : sessions_) {
        if (s.uid == uid) toRemove.push_back(id);
    }
    for (auto sid : toRemove) {
        sessions_.erase(sid);
    }
}

} // namespace re36

// Не используется, не удалять
/**
 * @file logger.cpp
 * @brief Реализация безопасной подсистемы логирования RAND Elecorner 36.
 */

#include "kernel/logger.h"

#include <algorithm>
#include <numeric>

namespace re36 {

Logger::Logger(const RateLimitConfig& rateConfig)
    : rateConfig_(rateConfig) {}

Logger::~Logger() = default;

// ============================================================================
// Инициализация
// ============================================================================

bool Logger::init() {
    std::lock_guard<std::mutex> lock(mutex_);

    channels_.clear();
    subscriptions_.clear();

    // Стандартные каналы
    auto make = [this](const std::string& name, SecurityLevel sec, LogLevel min, size_t max) {
        LogChannel ch;
        ch.name = name;
        ch.security = sec;
        ch.minLevel = min;
        ch.maxEntries = max;
        channels_[name] = std::move(ch);
    };

    make("kernel",   SecurityLevel::Internal, LogLevel::Info,    5000);
    make("security", SecurityLevel::Audit,    LogLevel::Warning, 10000);
    make("app",      SecurityLevel::Public,   LogLevel::Info,    3000);
    make("debug",    SecurityLevel::Internal, LogLevel::Trace,   2000);
    make("io",       SecurityLevel::Internal, LogLevel::Debug,   2000);
    make("ipc",      SecurityLevel::Internal, LogLevel::Debug,   2000);

    auditChainHash_ = 0;
    nextSeqId_ = 1;
    totalEntries_ = 0;
    droppedEntries_ = 0;

    return true;
}

// ============================================================================
// Каналы
// ============================================================================

bool Logger::createChannel(const std::string& name, SecurityLevel security,
                            LogLevel minLevel, size_t maxEntries) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (channels_.find(name) != channels_.end()) return false;

    LogChannel ch;
    ch.name = name;
    ch.security = security;
    ch.minLevel = minLevel;
    ch.maxEntries = maxEntries;
    channels_[name] = std::move(ch);
    return true;
}

bool Logger::removeChannel(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (name == "security" || name == "kernel") return false; // защищены
    return channels_.erase(name) > 0;
}

bool Logger::setChannelEnabled(const std::string& name, bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = channels_.find(name);
    if (it == channels_.end()) return false;
    if (name == "security" && !enabled) return false; // audit нельзя выключить
    it->second.enabled = enabled;
    return true;
}

std::optional<LogChannel> Logger::getChannel(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = channels_.find(name);
    return it != channels_.end() ? std::optional(it->second) : std::nullopt;
}

std::vector<std::string> Logger::getChannelNames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (auto& [n, _] : channels_) names.push_back(n);
    return names;
}

// ============================================================================
// Запись
// ============================================================================

bool Logger::log(LogLevel level, const std::string& source,
                  const std::string& message, const std::string& channel,
                  Pid pid, Uid uid) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Найти канал
    auto chIt = channels_.find(channel);
    if (chIt == channels_.end()) return false;
    auto& ch = chIt->second;

    // Канал выключен?
    if (!ch.enabled) return false;

    // Уровень ниже минимального?
    if (level < ch.minLevel) return false;

    // Rate limiting
    if (!checkRateLimit(source)) {
        droppedEntries_++;
        return false;
    }

    // Создать запись
    LogEntry entry;
    entry.sequenceId = nextSeqId_++;
    entry.tick = currentTick_;
    entry.level = level;
    entry.source = source;
    entry.channel = channel;
    entry.message = message;
    entry.pid = pid;
    entry.uid = uid;

    // Хеш-цепочка для audit-канала
    if (ch.security == SecurityLevel::Audit) {
        entry.prevHash = auditChainHash_;
        entry.selfHash = computeEntryHash(entry);
        auditChainHash_ = entry.selfHash;
    }

    // Ring-буфер
    ch.entries.push_back(entry);
    if (ch.entries.size() > ch.maxEntries) {
        ch.entries.pop_front();
    }

    totalEntries_++;

    // Оповестить подписчиков (под lock — подписчики не должны долго работать)
    notifySubscribers(entry);

    return true;
}

// --- Короткие обёртки ---

bool Logger::trace(const std::string& src, const std::string& msg, const std::string& ch) {
    return log(LogLevel::Trace, src, msg, ch);
}

bool Logger::debug(const std::string& src, const std::string& msg, const std::string& ch) {
    return log(LogLevel::Debug, src, msg, ch);
}

bool Logger::info(const std::string& src, const std::string& msg, const std::string& ch) {
    return log(LogLevel::Info, src, msg, ch);
}

bool Logger::warn(const std::string& src, const std::string& msg, const std::string& ch) {
    return log(LogLevel::Warning, src, msg, ch);
}

bool Logger::error(const std::string& src, const std::string& msg, const std::string& ch) {
    return log(LogLevel::Error, src, msg, ch);
}

bool Logger::fatal(const std::string& src, const std::string& msg, const std::string& ch) {
    return log(LogLevel::Fatal, src, msg, ch);
}

bool Logger::security(const std::string& action, Uid uid, const std::string& detail) {
    return log(LogLevel::Warning, "security", action + ": " + detail, "security",
               INVALID_PID, uid);
}

void Logger::setCurrentTick(Tick tick) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tick != currentTick_) {
        currentTick_ = tick;
        // Сбросить rate limiter каждый тик
        rateCountPerSource_.clear();
        rateCountTotal_ = 0;
    }
}

// ============================================================================
// Подписки
// ============================================================================

LogSubscriptionId Logger::subscribe(const std::string& channel, LogLevel minLevel,
                                     Uid subscriberUid, LogSubscriber callback) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Проверить доступ (для конкретного канала)
    if (channel != "*") {
        auto chIt = channels_.find(channel);
        if (chIt == channels_.end()) return 0;
        if (!checkReadAccess(chIt->second, subscriberUid)) return 0;
    } else {
        // Подписка на все — нужен Internal уровень
        if (subscriberUid != ROOT_UID) return 0;
    }

    LogSubscriptionId id = nextSubId_++;
    subscriptions_.push_back({id, channel, minLevel, subscriberUid, std::move(callback)});
    return id;
}

void Logger::unsubscribe(LogSubscriptionId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.erase(
        std::remove_if(subscriptions_.begin(), subscriptions_.end(),
                        [id](const LogSubscription& s) { return s.id == id; }),
        subscriptions_.end()
    );
}

// ============================================================================
// Запросы истории
// ============================================================================

std::vector<LogEntry> Logger::getHistory(const std::string& channel, size_t count,
                                          Uid requestorUid) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto chIt = channels_.find(channel);
    if (chIt == channels_.end()) return {};
    if (!checkReadAccess(chIt->second, requestorUid)) return {};

    const auto& entries = chIt->second.entries;
    if (count == 0 || count >= entries.size()) {
        return {entries.begin(), entries.end()};
    }
    return {entries.end() - static_cast<ptrdiff_t>(count), entries.end()};
}

std::vector<LogEntry> Logger::getHistoryByLevel(const std::string& channel,
                                                  LogLevel minLevel, size_t count,
                                                  Uid requestorUid) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto chIt = channels_.find(channel);
    if (chIt == channels_.end()) return {};
    if (!checkReadAccess(chIt->second, requestorUid)) return {};

    std::vector<LogEntry> result;
    for (auto it = chIt->second.entries.rbegin(); it != chIt->second.entries.rend(); ++it) {
        if (it->level >= minLevel) {
            result.push_back(*it);
            if (count > 0 && result.size() >= count) break;
        }
    }
    std::reverse(result.begin(), result.end());
    return result;
}

std::vector<LogEntry> Logger::getHistoryBySource(const std::string& channel,
                                                   const std::string& source,
                                                   size_t count,
                                                   Uid requestorUid) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto chIt = channels_.find(channel);
    if (chIt == channels_.end()) return {};
    if (!checkReadAccess(chIt->second, requestorUid)) return {};

    std::vector<LogEntry> result;
    for (auto it = chIt->second.entries.rbegin(); it != chIt->second.entries.rend(); ++it) {
        if (it->source == source) {
            result.push_back(*it);
            if (count > 0 && result.size() >= count) break;
        }
    }
    std::reverse(result.begin(), result.end());
    return result;
}

// ============================================================================
// Аудит: проверка целостности
// ============================================================================

bool Logger::verifyAuditIntegrity() const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto chIt = channels_.find("security");
    if (chIt == channels_.end()) return true;

    const auto& entries = chIt->second.entries;
    if (entries.empty()) return true;

    uint64_t prevHash = 0;
    for (const auto& entry : entries) {
        // Проверить цепочку
        if (entry.prevHash != prevHash) return false;

        // Проверить хеш самой записи
        uint64_t computed = computeEntryHash(entry);
        if (entry.selfHash != computed) return false;

        prevHash = entry.selfHash;
    }
    return true;
}

// ============================================================================
// Статистика
// ============================================================================

uint64_t Logger::getTotalEntries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return totalEntries_;
}

uint64_t Logger::getDroppedEntries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return droppedEntries_;
}

void Logger::resetRateLimiter() {
    std::lock_guard<std::mutex> lock(mutex_);
    rateCountPerSource_.clear();
    rateCountTotal_ = 0;
}

// ============================================================================
// Внутренние методы
// ============================================================================

bool Logger::checkReadAccess(const LogChannel& channel, Uid uid) const {
    switch (channel.security) {
        case SecurityLevel::Public:
            return true;
        case SecurityLevel::User:
            return uid != INVALID_UID;
        case SecurityLevel::Internal:
            return uid == ROOT_UID;
        case SecurityLevel::Audit:
            return uid == ROOT_UID;
    }
    return false;
}

bool Logger::checkRateLimit(const std::string& source) {
    // Проверить глобальный лимит
    if (rateCountTotal_ >= rateConfig_.maxPerTickTotal + rateConfig_.burstAllowance) {
        return false;
    }

    // Проверить лимит по source
    auto& count = rateCountPerSource_[source];
    if (count >= rateConfig_.maxPerTickPerSource + rateConfig_.burstAllowance) {
        return false;
    }

    count++;
    rateCountTotal_++;
    return true;
}

uint64_t Logger::computeEntryHash(const LogEntry& entry) const {
    // FNV-1a хеш для tamper-evident цепочки
    uint64_t hash = 14695981039346656037ULL; // FNV offset basis

    auto mix = [&hash](uint64_t val) {
        hash ^= val;
        hash *= 1099511628211ULL; // FNV prime
    };

    mix(entry.sequenceId);
    mix(entry.tick);
    mix(static_cast<uint64_t>(entry.level));
    mix(entry.prevHash);
    mix(static_cast<uint64_t>(entry.pid));
    mix(static_cast<uint64_t>(entry.uid));

    // Хеш строк
    for (char c : entry.source)  mix(static_cast<uint64_t>(c));
    for (char c : entry.message) mix(static_cast<uint64_t>(c));
    for (char c : entry.channel) mix(static_cast<uint64_t>(c));

    return hash;
}

void Logger::notifySubscribers(const LogEntry& entry) {
    // Вызывается под lock
    for (const auto& sub : subscriptions_) {
        // Фильтр по каналу
        if (sub.channel != "*" && sub.channel != entry.channel) continue;

        // Фильтр по уровню
        if (entry.level < sub.minLevel) continue;

        // Проверить, имеет ли подписчик доступ (если канал != "*")
        auto chIt = channels_.find(entry.channel);
        if (chIt != channels_.end()) {
            if (!checkReadAccess(chIt->second, sub.subscriberUid)) continue;
        }

        // Вызвать callback
        sub.callback(entry);
    }
}

} // namespace re36

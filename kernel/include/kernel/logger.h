/**
 * @file logger.h
 * @brief Безопасная подсистема логирования RAND Elecorner 36.
 *
 * Logger реализован как набор именованных каналов (LogChannel),
 * на которые можно подписываться. Каждый канал имеет:
 * - Уровень доступа (SecurityLevel) — кто может читать
 * - Минимальный уровень записи (LogLevel)
 * - Ring-буфер сообщений
 * - Список подписчиков с проверкой прав
 *
 * Модель безопасности:
 * ┌────────────────────────────────────────────────┐
 * │  Запись: любой модуль ядра через log()          │
 * │  Чтение: только подписчик с достаточным         │
 * │          SecurityLevel                          │
 * │  Каналы:                                        │
 * │    "kernel"   — SecurityLevel::Internal (ядро)   │
 * │    "security" — SecurityLevel::Audit   (root)    │
 * │    "app"      — SecurityLevel::Public  (все)     │
 * │    "debug"    — SecurityLevel::Internal          │
 * │  Rate limiting: макс. N сообщений/тик на PID    │
 * │  Подписки: каждая привязана к UID                │
 * └────────────────────────────────────────────────┘
 */

#pragma once

#include "types.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <deque>
#include <optional>
#include <chrono>

namespace re36 {

// ============================================================================
// Уровни логирования
// ============================================================================

/**
 * @enum LogLevel
 * @brief Уровень важности сообщения.
 */
enum class LogLevel : uint8_t {
    Trace   = 0,    ///< Детальная трассировка (только debug)
    Debug   = 1,    ///< Отладочная информация
    Info    = 2,    ///< Информационное сообщение
    Warning = 3,    ///< Предупреждение
    Error   = 4,    ///< Ошибка
    Fatal   = 5     ///< Фатальная ошибка
};

/**
 * @brief Строковое представление уровня.
 */
inline const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
    }
    return "?";
}

// ============================================================================
// Уровень доступа к каналу
// ============================================================================

/**
 * @enum SecurityLevel
 * @brief Кто может подписаться на канал.
 */
enum class SecurityLevel : uint8_t {
    Public   = 0,   ///< Любой пользователь
    User     = 1,   ///< Только авторизованные (есть сессия)
    Internal = 2,   ///< Только ядро / root
    Audit    = 3    ///< Только root, записи неизменяемы
};

// ============================================================================
// Запись лога
// ============================================================================

/**
 * @struct LogEntry
 * @brief Одно сообщение в канале.
 *
 * После создания запись иммутабельна — нельзя изменить содержимое.
 * Для аудит-канала записи дополнительно защищены хешем цепочки.
 */
struct LogEntry {
    uint64_t        sequenceId = 0;         ///< Глобальный порядковый номер
    Tick            tick       = 0;         ///< Тик ядра
    LogLevel        level      = LogLevel::Info;
    std::string     source;                 ///< Подсистема-источник ("scheduler", "fs", ...)
    std::string     channel;                ///< Имя канала
    std::string     message;                ///< Текст сообщения
    Pid             pid        = INVALID_PID;  ///< PID процесса (если применимо)
    Uid             uid        = INVALID_UID;  ///< UID пользователя (если применимо)

    /// Хеш предыдущей записи (для Audit-канала — цепочка целостности)
    uint64_t        prevHash   = 0;
    /// Хеш текущей записи
    uint64_t        selfHash   = 0;
};

// ============================================================================
// Подписка на канал
// ============================================================================

/// Тип обратного вызова подписчика
using LogSubscriber = std::function<void(const LogEntry&)>;

/// Идентификатор подписки
using LogSubscriptionId = uint32_t;

/**
 * @struct LogSubscription
 * @brief Привязка подписчика к каналу.
 */
struct LogSubscription {
    LogSubscriptionId   id;
    std::string         channel;        ///< Имя канала ("*" = все)
    LogLevel            minLevel;       ///< Минимальный уровень интереса
    Uid                 subscriberUid;  ///< UID подписчика (для проверки прав)
    LogSubscriber       callback;
};

// ============================================================================
// Канал логирования
// ============================================================================

/**
 * @struct LogChannel
 * @brief Именованный канал с буфером и политикой доступа.
 */
struct LogChannel {
    std::string         name;
    SecurityLevel       security    = SecurityLevel::Public;
    LogLevel            minLevel    = LogLevel::Info;
    size_t              maxEntries  = 5000;    ///< Размер ring-буфера
    std::deque<LogEntry> entries;               ///< Ring-буфер
    bool                enabled     = true;
};

// ============================================================================
// Конфигурация rate-limiting
// ============================================================================

/**
 * @struct RateLimitConfig
 * @brief Ограничение частоты записи.
 */
struct RateLimitConfig {
    uint32_t maxPerTickPerSource = 50;  ///< Макс. сообщений от одного source за тик
    uint32_t maxPerTickTotal     = 200; ///< Макс. сообщений за тик всего
    uint32_t burstAllowance      = 20;  ///< Допустимый «всплеск» сверх лимита
};

// ============================================================================
// Logger
// ============================================================================

/**
 * @class Logger
 * @brief Подсистема логирования с каналами, подписками и безопасностью.
 *
 * Использование ядром:
 * @code
 *   logger.log(LogLevel::Info, "scheduler", "Процесс {} создан", pid);
 *   logger.security("login_failed", uid, "Неверный пароль для user root");
 * @endcode
 *
 * Подписка GUI:
 * @code
 *   auto id = logger.subscribe("kernel", LogLevel::Info, ROOT_UID,
 *       [](const LogEntry& e) { updateUI(e); });
 * @endcode
 */
class Logger {
public:
    explicit Logger(const RateLimitConfig& rateConfig = {});
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // ========================================================================
    // Инициализация
    // ========================================================================

    /**
     * Инициализировать стандартные каналы:
     * - "kernel"   — Internal, Info+
     * - "security" — Audit, Warning+
     * - "app"      — Public, Info+
     * - "debug"    — Internal, Trace+
     * - "io"       — Internal, Debug+
     * - "ipc"      — Internal, Debug+
     */
    bool init();

    // ========================================================================
    // Управление каналами
    // ========================================================================

    /// Создать канал (только root)
    bool createChannel(const std::string& name, SecurityLevel security,
                       LogLevel minLevel, size_t maxEntries = 5000);

    /// Удалить канал (только root, нельзя удалить "security")
    bool removeChannel(const std::string& name);

    /// Включить / выключить канал
    bool setChannelEnabled(const std::string& name, bool enabled);

    /// Получить описание канала
    std::optional<LogChannel> getChannel(const std::string& name) const;

    /// Список всех каналов
    std::vector<std::string> getChannelNames() const;

    // ========================================================================
    // Запись логов
    // ========================================================================

    /**
     * Записать сообщение в канал.
     * @param level    Уровень
     * @param source   Подсистема-источник
     * @param message  Текст
     * @param channel  Канал (по умолчанию "kernel")
     * @param pid      PID (если применимо)
     * @param uid      UID (если применимо)
     * @return true если записано, false если заблокировано rate-limiter
     */
    bool log(LogLevel level, const std::string& source,
             const std::string& message,
             const std::string& channel = "kernel",
             Pid pid = INVALID_PID, Uid uid = INVALID_UID);

    /// Короткие обёртки
    bool trace(const std::string& source, const std::string& msg,
               const std::string& channel = "debug");
    bool debug(const std::string& source, const std::string& msg,
               const std::string& channel = "debug");
    bool info(const std::string& source, const std::string& msg,
              const std::string& channel = "kernel");
    bool warn(const std::string& source, const std::string& msg,
              const std::string& channel = "kernel");
    bool error(const std::string& source, const std::string& msg,
               const std::string& channel = "kernel");
    bool fatal(const std::string& source, const std::string& msg,
               const std::string& channel = "kernel");

    /**
     * Запись в аудит-канал "security".
     * Записи защищены хеш-цепочкой (tamper-evident).
     */
    bool security(const std::string& action, Uid uid, const std::string& detail);

    /// Установить текущий тик (вызывается ядром каждый tick)
    void setCurrentTick(Tick tick);

    // ========================================================================
    // Подписки (чтение)
    // ========================================================================

    /**
     * Подписаться на канал.
     * @param channel       Имя канала ("*" = все каналы)
     * @param minLevel      Минимальный уровень
     * @param subscriberUid UID подписчика (проверяется по SecurityLevel канала)
     * @param callback      Функция-обработчик
     * @return ID подписки или 0 при отказе доступа
     */
    LogSubscriptionId subscribe(const std::string& channel, LogLevel minLevel,
                                 Uid subscriberUid, LogSubscriber callback);

    /// Отписаться
    void unsubscribe(LogSubscriptionId id);

    // ========================================================================
    // Запросы истории
    // ========================================================================

    /**
     * Получить последние записи канала.
     * @param channel    Канал
     * @param count      Макс. количество
     * @param requestorUid UID запрашивающего (проверка доступа)
     * @return Записи или пустой вектор при отказе
     */
    std::vector<LogEntry> getHistory(const std::string& channel, size_t count,
                                      Uid requestorUid) const;

    /**
     * Получить записи, отфильтрованные по уровню.
     */
    std::vector<LogEntry> getHistoryByLevel(const std::string& channel,
                                             LogLevel minLevel, size_t count,
                                             Uid requestorUid) const;

    /**
     * Получить записи конкретного источника.
     */
    std::vector<LogEntry> getHistoryBySource(const std::string& channel,
                                              const std::string& source,
                                              size_t count,
                                              Uid requestorUid) const;

    // ========================================================================
    // Аудит: проверка целостности
    // ========================================================================

    /**
     * Проверить целостность хеш-цепочки в аудит-канале.
     * @return true если цепочка не нарушена
     */
    bool verifyAuditIntegrity() const;

    // ========================================================================
    // Статистика
    // ========================================================================

    /// Общее количество записей
    uint64_t getTotalEntries() const;

    /// Количество отклонённых rate-limiter
    uint64_t getDroppedEntries() const;

    /// Сбросить rate-limiter (в начале нового тика)
    void resetRateLimiter();

private:
    mutable std::mutex                              mutex_;
    std::unordered_map<std::string, LogChannel>     channels_;
    std::vector<LogSubscription>                    subscriptions_;
    LogSubscriptionId                               nextSubId_ = 1;

    // Глобальный счётчик
    uint64_t                                        nextSeqId_ = 1;
    uint64_t                                        totalEntries_ = 0;
    uint64_t                                        droppedEntries_ = 0;
    Tick                                            currentTick_ = 0;

    // Rate limiting
    RateLimitConfig                                 rateConfig_;
    std::unordered_map<std::string, uint32_t>       rateCountPerSource_; ///< source → count за тик
    uint32_t                                        rateCountTotal_ = 0; ///< всего за тик

    // Аудит: последний хеш цепочки
    uint64_t                                        auditChainHash_ = 0;

    // --- Внутренние методы ---

    /// Проверить, может ли UID читать канал
    bool checkReadAccess(const LogChannel& channel, Uid uid) const;

    /// Rate-limiting: можно ли записать?
    bool checkRateLimit(const std::string& source);

    /// Вычислить хеш записи (для аудит-цепочки)
    uint64_t computeEntryHash(const LogEntry& entry) const;

    /// Уведомить подписчиков
    void notifySubscribers(const LogEntry& entry);
};

} // namespace re36

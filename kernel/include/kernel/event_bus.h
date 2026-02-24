/**
 * @file event_bus.h
 * @brief Шина событий ядра RAND Elecorner 36.
 *
 * EventBus обеспечивает слабую связанность между модулями ядра.
 * Подсистемы публикуют события, а подписчики (включая GUI) получают
 * уведомления без прямой зависимости друг от друга.
 */

#pragma once

#include "types.h"

#include <functional>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <any>

namespace re36 {

// ============================================================================
// Событие
// ============================================================================

/**
 * @struct Event
 * @brief Единица данных, передаваемая через шину событий.
 */
struct Event {
    EventType   type;           ///< Тип события
    Tick        tick;           ///< Тик, на котором произошло событие
    std::string source;        ///< Модуль-источник ("scheduler", "memory", ...)

    /// Произвольные данные события (ключ → значение)
    std::unordered_map<std::string, KernelValue> payload;

    // --- Удобные конструкторы ---

    Event() = default;

    Event(EventType type, Tick tick, const std::string& source)
        : type(type), tick(tick), source(source) {}

    /// Добавить данные к событию (цепочка вызовов)
    Event& with(const std::string& key, KernelValue value) {
        payload[key] = std::move(value);
        return *this;
    }
};

// ============================================================================
// Обработчик событий
// ============================================================================

/// Тип callback-функции подписчика
using EventHandler = std::function<void(const Event&)>;

/// Идентификатор подписки (для отписки)
using SubscriptionId = uint32_t;

// ============================================================================
// Шина событий
// ============================================================================

/**
 * @class EventBus
 * @brief Центральная шина событий ядра.
 *
 * Поддерживает два режима:
 * 1. **Синхронный** (по умолчанию): событие обрабатывается немедленно.
 * 2. **Отложенный**: событие ставится в очередь и обрабатывается
 *    в начале следующего тика (Kernel::tick → eventBus.processEvents).
 */
class EventBus {
public:
    EventBus();
    ~EventBus();

    // Некопируемый, неперемещаемый (singleton-подобное поведение)
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    // ---- Подписка ----

    /**
     * Подписаться на определённый тип события.
     * @param type Тип события
     * @param handler Функция-обработчик
     * @return ID подписки для последующей отписки
     */
    SubscriptionId subscribe(EventType type, EventHandler handler);

    /**
     * Подписаться на все события.
     * @param handler Функция-обработчик
     * @return ID подписки
     */
    SubscriptionId subscribeAll(EventHandler handler);

    /**
     * Отменить подписку.
     * @param id ID подписки, полученный от subscribe()
     */
    void unsubscribe(SubscriptionId id);

    // ---- Публикация ----

    /**
     * Опубликовать событие (синхронно — обработчики вызываются немедленно).
     * @param event Событие для публикации
     */
    void publish(const Event& event);

    /**
     * Поставить событие в очередь (отложенная обработка).
     * Будет обработано при вызове processEvents().
     * @param event Событие
     */
    void enqueue(Event event);

    /**
     * Обработать все события из очереди.
     * Вызывается ядром в начале каждого тика.
     */
    void processEvents();

    // ---- Журнал ----

    /**
     * Получить последние N событий из журнала.
     * @param count Количество (0 = все)
     * @return Вектор событий, от старых к новым
     */
    std::vector<Event> getRecentEvents(size_t count = 0) const;

    /**
     * Получить события определённого типа из журнала.
     */
    std::vector<Event> getEventsByType(EventType type, size_t count = 0) const;

    /**
     * Очистить журнал событий.
     */
    void clearHistory();

    /**
     * Общее количество опубликованных событий.
     */
    uint64_t getTotalEventCount() const;

private:
    struct Subscription {
        SubscriptionId      id;
        EventType           type;
        EventHandler        handler;
        bool                allEvents;  ///< true = подписка на все события
    };

    std::vector<Subscription>   subscriptions_;
    std::queue<Event>           pendingEvents_;
    std::vector<Event>          eventHistory_;
    SubscriptionId              nextId_ = 1;
    uint64_t                    totalEventCount_ = 0;

    static constexpr size_t MAX_HISTORY_SIZE = 10000;

    /// Доставить событие всем подходящим подписчикам
    void dispatch(const Event& event);

    /// Добавить событие в журнал (с ограничением размера)
    void recordEvent(const Event& event);

    mutable std::mutex mutex_;
};

} // namespace re36

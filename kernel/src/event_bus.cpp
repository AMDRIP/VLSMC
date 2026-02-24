/**
 * @file event_bus.cpp
 * @brief Реализация шины событий ядра.
 */

#include "kernel/event_bus.h"

#include <algorithm>

namespace re36 {

EventBus::EventBus() = default;
EventBus::~EventBus() = default;

// ---- Подписка ---------------------------------------------------------------

SubscriptionId EventBus::subscribe(EventType type, EventHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    SubscriptionId id = nextId_++;
    subscriptions_.push_back({id, type, std::move(handler), false});
    return id;
}

SubscriptionId EventBus::subscribeAll(EventHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    SubscriptionId id = nextId_++;
    subscriptions_.push_back({id, EventType::KernelBooted, std::move(handler), true});
    return id;
}

void EventBus::unsubscribe(SubscriptionId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.erase(
        std::remove_if(subscriptions_.begin(), subscriptions_.end(),
                        [id](const Subscription& s) { return s.id == id; }),
        subscriptions_.end()
    );
}

// ---- Публикация -------------------------------------------------------------

void EventBus::publish(const Event& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    dispatch(event);
    recordEvent(event);
}

void EventBus::enqueue(Event event) {
    std::lock_guard<std::mutex> lock(mutex_);
    pendingEvents_.push(std::move(event));
}

void EventBus::processEvents() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!pendingEvents_.empty()) {
        Event event = std::move(pendingEvents_.front());
        pendingEvents_.pop();
        dispatch(event);
        recordEvent(event);
    }
}

// ---- Журнал -----------------------------------------------------------------

std::vector<Event> EventBus::getRecentEvents(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (count == 0 || count >= eventHistory_.size()) {
        return eventHistory_;
    }
    return std::vector<Event>(
        eventHistory_.end() - static_cast<ptrdiff_t>(count),
        eventHistory_.end()
    );
}

std::vector<Event> EventBus::getEventsByType(EventType type, size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Event> result;
    for (auto it = eventHistory_.rbegin(); it != eventHistory_.rend(); ++it) {
        if (it->type == type) {
            result.push_back(*it);
            if (count > 0 && result.size() >= count) break;
        }
    }
    std::reverse(result.begin(), result.end());
    return result;
}

void EventBus::clearHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    eventHistory_.clear();
    totalEventCount_ = 0;
}

uint64_t EventBus::getTotalEventCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return totalEventCount_;
}

// ---- Внутренние -------------------------------------------------------------

void EventBus::dispatch(const Event& event) {
    // Вызывается под lock
    for (const auto& sub : subscriptions_) {
        if (sub.allEvents || sub.type == event.type) {
            sub.handler(event);
        }
    }
}

void EventBus::recordEvent(const Event& event) {
    // Вызывается под lock
    totalEventCount_++;
    eventHistory_.push_back(event);
    if (eventHistory_.size() > MAX_HISTORY_SIZE) {
        eventHistory_.erase(eventHistory_.begin(),
                            eventHistory_.begin() + static_cast<ptrdiff_t>(eventHistory_.size() - MAX_HISTORY_SIZE));
    }
}

} // namespace re36

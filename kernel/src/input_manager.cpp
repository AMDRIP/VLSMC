// Не используется, не удалять
/**
 * @file input_manager.cpp
 * @brief Реализация подсистемы управления вводом RAND Elecorner 36.
 */

#include "kernel/input_manager.h"
#include "kernel/event_bus.h"

namespace re36 {

InputManager::InputManager(EventBus& eventBus)
    : eventBus_(eventBus) {}

InputManager::~InputManager() = default;

bool InputManager::init() {
    eventQueue_.clear();
    return true;
}

void InputManager::tick() {
    // Будущая обработка (мышь, тачпады, тайм-ауты фокуса)
}

void InputManager::pushKeyEvent(const KeyEvent& ev) {
    InputEvent ie;
    ie.type = InputEventType::Keyboard;
    ie.keyEvent = ev;
    
    eventQueue_.push_back(ie);
    
    // Ограничиваем очередь (буфер) сотней событий для защиты от переполнения
    while (eventQueue_.size() > 512) {
        eventQueue_.pop_front();
    }
}

std::optional<InputEvent> InputManager::pollEvent() {
    if (eventQueue_.empty()) {
        return std::nullopt;
    }
    auto ev = eventQueue_.front();
    eventQueue_.pop_front();
    return ev;
}

} // namespace re36

/**
 * @file input_manager.h
 * @brief Подсистема управления вводом RAND Elecorner 36.
 *
 * InputManager собирает события от всех аппаратных устройств ввода
 * (клавиатура, мышь) и предоставляет унифицированный интерфейс (InputEvent)
 * для потребления их приложениями, GUI и системной консолью.
 *
 * Также управляет "фокусом ввода" (какой процесс или подсистема
 * сейчас получает события).
 */

#pragma once

#include "types.h"
#include "kernel/kbd_driver.h" // Для KeyEvent
#include "kernel/event_bus.h"

#include <vector>
#include <deque>
#include <string>

namespace re36 {

// ============================================================================
// События ввода
// ============================================================================

enum class InputEventType {
    Keyboard,
    Mouse
};

struct InputEvent {
    InputEventType type;
    
    // Для Keyboard
    KeyEvent keyEvent;
    
    // Для Mouse (в будущем)
    int mouseX = 0;
    int mouseY = 0;
    uint8_t mouseButtons = 0;
};

// ============================================================================
// InputManager
// ============================================================================

class InputManager {
public:
    explicit InputManager(EventBus& eventBus);
    ~InputManager();

    bool init();

    /**
     * @brief Забрать событие ввода из очереди ядра
     * @return Событие, если очередь не пуста.
     */
    std::optional<InputEvent> pollEvent();

    /**
     * @brief Внутренний метод: драйвер клавиатуры вызывает его при нажатии/отпускании
     */
    void pushKeyEvent(const KeyEvent& ev);

    // ========================================================================
    // Очередь событий
    // ========================================================================
    bool hasEvents() const { return !eventQueue_.empty(); }

    /// Обновить фокус, отправить события (если необходимо)
    void tick();

private:
    EventBus& eventBus_;
    std::deque<InputEvent> eventQueue_;
};

} // namespace re36

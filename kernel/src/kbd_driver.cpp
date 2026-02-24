/**
 * @file kbd_driver.cpp
 * @brief Реализация фундаментального драйвера клавиатуры.
 *
 * Эмулирует PS/2-подобную клавиатуру с аппаратным буфером,
 * трансляцией скан-кодов в символы и автоповтором.
 */

#include "kernel/kbd_driver.h"
#include "kernel/event_bus.h"

#include <algorithm>

namespace re36 {

// ============================================================================
// Конструктор / деструктор
// ============================================================================

KeyboardDriver::KeyboardDriver(EventBus& eventBus, InputManager& inputManager)
    : eventBus_(eventBus), inputManager_(inputManager) {}

KeyboardDriver::~KeyboardDriver() = default;

// ============================================================================
// Жизненный цикл
// ============================================================================

bool KeyboardDriver::init() {
    reset();
    initialized_ = true;

    Event evt(EventType::IoCompleted, 0, "kbd_driver");
    evt.with("action", std::string("init"))
       .with("device", std::string("keyboard"));
    eventBus_.publish(evt);

    return true;
}

void KeyboardDriver::reset() {
    keyBuffer_.clear();
    modifiers_ = ModifierState{};
    pressedKeys_.clear();
    lastKey_ = Scancode::None;
    repeating_ = false;
    nextSequence_ = 1;
    stats_ = KeyboardStats{};
}

void KeyboardDriver::tick(uint64_t currentTick) {
    currentTick_ = currentTick;

    // Автоповтор
    if (repeatEnabled_ && lastKey_ != Scancode::None) {
        uint64_t elapsed = currentTick - lastKeyTick_;
        if (!repeating_ && elapsed >= repeatDelay_) {
            repeating_ = true;
            processKey(lastKey_, KeyAction::Repeat);
            lastKeyTick_ = currentTick;
        } else if (repeating_ && elapsed >= repeatInterval_) {
            processKey(lastKey_, KeyAction::Repeat);
            lastKeyTick_ = currentTick;
        }
    }

    // Обновить статистику (примитивная скорость набора)
    if (currentTick > 0 && currentTick % 100 == 0) {
        // Средняя скорость за последние 100 тиков
        stats_.keysPerSecond = static_cast<double>(stats_.totalKeyPresses) /
                               (static_cast<double>(currentTick) / 100.0);
    }
}

// ============================================================================
// Инъекция событий (из GUI)
// ============================================================================

void KeyboardDriver::injectKeyPress(Scancode scancode) {
    if (!initialized_) return;

    auto sc = static_cast<uint16_t>(scancode);
    // Игнорировать повторное нажатие уже нажатой клавиши
    if (pressedKeys_[sc]) return;
    pressedKeys_[sc] = true;

    // Обновить модификаторы
    updateModifiers(scancode, KeyAction::Press);

    // Обработать событие
    processKey(scancode, KeyAction::Press);

    // Запомнить для автоповтора
    lastKey_ = scancode;
    lastKeyTick_ = currentTick_;
    repeating_ = false;

    stats_.totalKeyPresses++;
}

void KeyboardDriver::injectKeyRelease(Scancode scancode) {
    if (!initialized_) return;

    auto sc = static_cast<uint16_t>(scancode);
    pressedKeys_[sc] = false;

    updateModifiers(scancode, KeyAction::Release);
    processKey(scancode, KeyAction::Release);

    // Остановить автоповтор если отпущена повторяемая клавиша
    if (lastKey_ == scancode) {
        lastKey_ = Scancode::None;
        repeating_ = false;
    }

    stats_.totalKeyReleases++;
}

void KeyboardDriver::injectString(const std::string& text) {
    if (!initialized_) return;

    // Простая таблица символ → скан-код для латиницы
    static const std::unordered_map<char, Scancode> charToScancode = {
        {'a', Scancode::A}, {'b', Scancode::B}, {'c', Scancode::C},
        {'d', Scancode::D}, {'e', Scancode::E}, {'f', Scancode::F},
        {'g', Scancode::G}, {'h', Scancode::H}, {'i', Scancode::I},
        {'j', Scancode::J}, {'k', Scancode::K}, {'l', Scancode::L},
        {'m', Scancode::M}, {'n', Scancode::N}, {'o', Scancode::O},
        {'p', Scancode::P}, {'q', Scancode::Q}, {'r', Scancode::R},
        {'s', Scancode::S}, {'t', Scancode::T}, {'u', Scancode::U},
        {'v', Scancode::V}, {'w', Scancode::W}, {'x', Scancode::X},
        {'y', Scancode::Y}, {'z', Scancode::Z},
        {'1', Scancode::Key1}, {'2', Scancode::Key2}, {'3', Scancode::Key3},
        {'4', Scancode::Key4}, {'5', Scancode::Key5}, {'6', Scancode::Key6},
        {'7', Scancode::Key7}, {'8', Scancode::Key8}, {'9', Scancode::Key9},
        {'0', Scancode::Key0},
        {' ', Scancode::Space}, {'\n', Scancode::Enter},
        {'-', Scancode::Minus}, {'=', Scancode::Equal},
        {'[', Scancode::LeftBracket}, {']', Scancode::RightBracket},
        {';', Scancode::Semicolon}, {'\'', Scancode::Apostrophe},
        {'`', Scancode::Grave}, {'\\', Scancode::Backslash},
        {',', Scancode::Comma}, {'.', Scancode::Period},
        {'/', Scancode::Slash}, {'\t', Scancode::Tab}
    };

    for (char ch : text) {
        char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        bool needShift = (ch >= 'A' && ch <= 'Z');

        auto it = charToScancode.find(lower);
        if (it == charToScancode.end()) continue;

        if (needShift) injectKeyPress(Scancode::LeftShift);
        injectKeyPress(it->second);
        injectKeyRelease(it->second);
        if (needShift) injectKeyRelease(Scancode::LeftShift);
    }
}

// ============================================================================
// Чтение буфера
// ============================================================================

std::optional<KeyEvent> KeyboardDriver::pollEvent() {
    if (keyBuffer_.empty()) return std::nullopt;
    KeyEvent evt = keyBuffer_.front();
    keyBuffer_.pop_front();
    stats_.currentBufferSize = static_cast<uint32_t>(keyBuffer_.size());
    return evt;
}

std::optional<KeyEvent> KeyboardDriver::peekEvent() const {
    if (keyBuffer_.empty()) return std::nullopt;
    return keyBuffer_.front();
}

bool KeyboardDriver::hasEvents() const {
    return !keyBuffer_.empty();
}

uint32_t KeyboardDriver::eventCount() const {
    return static_cast<uint32_t>(keyBuffer_.size());
}

// ============================================================================
// ============================================================================
// Раскладка
// ============================================================================

void KeyboardDriver::setLayout(KeyboardLayout layout) {
    layout_ = layout;
}

KeyboardLayout KeyboardDriver::getLayout() const {
    return layout_;
}

void KeyboardDriver::toggleLayout() {
    layout_ = (layout_ == KeyboardLayout::US_EN) ?
              KeyboardLayout::RU : KeyboardLayout::US_EN;

    Event evt(EventType::IoCompleted, 0, "kbd_driver");
    evt.with("action", std::string("layout_change"))
       .with("layout", layout_ == KeyboardLayout::US_EN ?
             std::string("EN") : std::string("RU"));
    eventBus_.publish(evt);
}

// ============================================================================
// Модификаторы
// ============================================================================

const ModifierState& KeyboardDriver::getModifiers() const {
    return modifiers_;
}

// ============================================================================
// Автоповтор
// ============================================================================

void KeyboardDriver::setRepeatEnabled(bool enabled) {
    repeatEnabled_ = enabled;
}

bool KeyboardDriver::isRepeatEnabled() const {
    return repeatEnabled_;
}

void KeyboardDriver::setRepeatRate(uint32_t delay, uint32_t interval) {
    repeatDelay_ = delay;
    repeatInterval_ = interval;
}

// ============================================================================
// Статистика
// ============================================================================

KeyboardStats KeyboardDriver::getStats() const {
    KeyboardStats s = stats_;
    s.currentBufferSize = static_cast<uint32_t>(keyBuffer_.size());
    return s;
}

void KeyboardDriver::resetStats() {
    stats_ = KeyboardStats{};
}

// ============================================================================
// Лог событий
// ============================================================================

std::vector<KeyEvent> KeyboardDriver::getEventLog(size_t count) const {
    size_t n = std::min(count, eventLog_.size());
    return {eventLog_.end() - static_cast<std::ptrdiff_t>(n), eventLog_.end()};
}

void KeyboardDriver::registerCallback(KeyCallback cb) {
    callbacks_.push_back(std::move(cb));
}

// ============================================================================
// Внутренние методы
// ============================================================================

void KeyboardDriver::processKey(Scancode scancode, KeyAction action) {
    // Транслировать скан-код → символ
    char ch = '\0';
    if (action == KeyAction::Press || action == KeyAction::Repeat) {
        ch = translateScancode(scancode);
    }

    // Создать событие
    KeyEvent evt;
    evt.scancode = scancode;
    evt.action = action;
    evt.character = ch;
    evt.modifiers = modifiers_;
    evt.tick = currentTick_;
    evt.sequenceNum = nextSequence_++;

    // Проверить управляющие последовательности (Ctrl+C и т.д.)
    if (action == KeyAction::Press && handleControlSequence(evt)) {
        return; // Управляющая последовательность обработана
    }

    // Добавить в аппаратный буфер
    if (keyBuffer_.size() >= maxBufferSize_) {
        keyBuffer_.pop_front();  // Вытеснить старейшее
        stats_.bufferOverflows++;
    }
    keyBuffer_.push_back(evt);
    stats_.currentBufferSize = static_cast<uint32_t>(keyBuffer_.size());
    if (stats_.currentBufferSize > stats_.maxBufferSize) {
        stats_.maxBufferSize = stats_.currentBufferSize;
    }

    // Добавить в лог
    eventLog_.push_back(evt);
    while (eventLog_.size() > MAX_EVENT_LOG) {
        eventLog_.pop_front();
    }

    // Отправляем в менеджер ввода для потребления системами
    inputManager_.pushKeyEvent(evt);

    // Подсчёт символов
    if (evt.isPrintable() && (action == KeyAction::Press || action == KeyAction::Repeat)) {
        stats_.totalCharacters++;
    }

    // Вызвать callbacks
    for (auto& cb : callbacks_) {
        cb(evt);
    }

    // Опубликовать событие на EventBus (= IRQ)
    publishKeyEvent(evt);
}

void KeyboardDriver::updateModifiers(Scancode scancode, KeyAction action) {
    bool pressed = (action == KeyAction::Press);

    switch (scancode) {
        case Scancode::LeftShift:   modifiers_.leftShift = pressed; break;
        case Scancode::RightShift:  modifiers_.rightShift = pressed; break;
        case Scancode::LeftCtrl:    modifiers_.leftCtrl = pressed; break;
        case Scancode::RightCtrl:   modifiers_.rightCtrl = pressed; break;
        case Scancode::LeftAlt:     modifiers_.leftAlt = pressed; break;
        case Scancode::RightAlt:    modifiers_.rightAlt = pressed; break;
        case Scancode::CapsLock:
            if (pressed) modifiers_.capsLock = !modifiers_.capsLock;
            break;
        case Scancode::NumLock:
            if (pressed) modifiers_.numLock = !modifiers_.numLock;
            break;
        case Scancode::ScrollLock:
            if (pressed) modifiers_.scrollLock = !modifiers_.scrollLock;
            break;
        default:
            break;
    }
}

char KeyboardDriver::translateScancode(Scancode scancode) const {
    switch (layout_) {
        case KeyboardLayout::US_EN:
            return translateUS(scancode);
        case KeyboardLayout::RU:
            return translateRU(scancode);
    }
    return '\0';
}

char KeyboardDriver::translateUS(Scancode scancode) const {
    bool upper = modifiers_.uppercase();
    bool shift = modifiers_.shift();

    switch (scancode) {
        // Буквы
        case Scancode::A: return upper ? 'A' : 'a';
        case Scancode::B: return upper ? 'B' : 'b';
        case Scancode::C: return upper ? 'C' : 'c';
        case Scancode::D: return upper ? 'D' : 'd';
        case Scancode::E: return upper ? 'E' : 'e';
        case Scancode::F: return upper ? 'F' : 'f';
        case Scancode::G: return upper ? 'G' : 'g';
        case Scancode::H: return upper ? 'H' : 'h';
        case Scancode::I: return upper ? 'I' : 'i';
        case Scancode::J: return upper ? 'J' : 'j';
        case Scancode::K: return upper ? 'K' : 'k';
        case Scancode::L: return upper ? 'L' : 'l';
        case Scancode::M: return upper ? 'M' : 'm';
        case Scancode::N: return upper ? 'N' : 'n';
        case Scancode::O: return upper ? 'O' : 'o';
        case Scancode::P: return upper ? 'P' : 'p';
        case Scancode::Q: return upper ? 'Q' : 'q';
        case Scancode::R: return upper ? 'R' : 'r';
        case Scancode::S: return upper ? 'S' : 's';
        case Scancode::T: return upper ? 'T' : 't';
        case Scancode::U: return upper ? 'U' : 'u';
        case Scancode::V: return upper ? 'V' : 'v';
        case Scancode::W: return upper ? 'W' : 'w';
        case Scancode::X: return upper ? 'X' : 'x';
        case Scancode::Y: return upper ? 'Y' : 'y';
        case Scancode::Z: return upper ? 'Z' : 'z';

        // Цифры / символы
        case Scancode::Key1: return shift ? '!' : '1';
        case Scancode::Key2: return shift ? '@' : '2';
        case Scancode::Key3: return shift ? '#' : '3';
        case Scancode::Key4: return shift ? '$' : '4';
        case Scancode::Key5: return shift ? '%' : '5';
        case Scancode::Key6: return shift ? '^' : '6';
        case Scancode::Key7: return shift ? '&' : '7';
        case Scancode::Key8: return shift ? '*' : '8';
        case Scancode::Key9: return shift ? '(' : '9';
        case Scancode::Key0: return shift ? ')' : '0';
        case Scancode::Minus:  return shift ? '_' : '-';
        case Scancode::Equal:  return shift ? '+' : '=';

        // Пунктуация
        case Scancode::LeftBracket:  return shift ? '{' : '[';
        case Scancode::RightBracket: return shift ? '}' : ']';
        case Scancode::Semicolon:    return shift ? ':' : ';';
        case Scancode::Apostrophe:   return shift ? '"' : '\'';
        case Scancode::Grave:        return shift ? '~' : '`';
        case Scancode::Backslash:    return shift ? '|' : '\\';
        case Scancode::Comma:        return shift ? '<' : ',';
        case Scancode::Period:       return shift ? '>' : '.';
        case Scancode::Slash:        return shift ? '?' : '/';

        // Спецклавиши
        case Scancode::Space:     return ' ';
        case Scancode::Enter:     return '\n';
        case Scancode::Tab:       return '\t';
        case Scancode::Backspace: return '\b';
        case Scancode::Escape:    return 0x1B;

        default: return '\0';
    }
}

char KeyboardDriver::translateRU(Scancode scancode) const {
    // Для русской раскладки отображаем на ASCII-совместимые
    // символы (фонетическая раскладка). В реальной ОС были бы
    // юникодные символы, но для симулятора используем ASCII.
    // Переключение раскладки = переключение трансляции.
    // Возвращаем английские буквы с пометкой для GUI.
    // В симуляторе русская раскладка работает идентично английской,
    // но GUI может отображать другие символы на основе layout_.
    return translateUS(scancode);
}

void KeyboardDriver::publishKeyEvent(const KeyEvent& evt) {
    stats_.totalInterrupts++;

    // Определить тип события
    EventType etype;
    switch (evt.action) {
        case KeyAction::Press:
        case KeyAction::Repeat:
            etype = EventType::IoRequestSubmitted;
            break;
        case KeyAction::Release:
            etype = EventType::IoCompleted;
            break;
    }

    Event busEvt(etype, 0, "kbd_driver");
    busEvt.with("irq", static_cast<int64_t>(KBD_IRQ))
          .with("scancode", static_cast<int64_t>(static_cast<uint16_t>(evt.scancode)))
          .with("action", evt.action == KeyAction::Press ? std::string("press") :
                          evt.action == KeyAction::Release ? std::string("release") :
                          std::string("repeat"));

    if (evt.character != '\0') {
        busEvt.with("char", std::string(1, evt.character));
    }

    eventBus_.publish(busEvt);
} // namespace re36

bool KeyboardDriver::handleControlSequence(const KeyEvent& evt) {
    if (!evt.modifiers.ctrl()) return false;

    switch (evt.scancode) {
        case Scancode::C: {
            // Ctrl+C — прервать процесс (SIGINT)
            Event sig(EventType::ProcessTerminated, 0, "kbd_driver");
            sig.with("signal", std::string("SIGINT"))
               .with("source", std::string("keyboard"));
            eventBus_.publish(sig);
            lineBuffer_.clear();
            return true;
        }
        case Scancode::D: {
            // Ctrl+D — конец ввода (EOF)
            Event sig(EventType::IoCompleted, 0, "kbd_driver");
            sig.with("signal", std::string("EOF"))
               .with("source", std::string("keyboard"));
            eventBus_.publish(sig);
            return true;
        }
        case Scancode::L: {
            // Ctrl+L — очистить экран
            Event sig(EventType::IoCompleted, 0, "kbd_driver");
            sig.with("signal", std::string("CLEAR"))
               .with("source", std::string("keyboard"));
            eventBus_.publish(sig);
            return true;
        }
        case Scancode::Z: {
            // Ctrl+Z — приостановить процесс (SIGTSTP)
            Event sig(EventType::ProcessTerminated, 0, "kbd_driver");
            sig.with("signal", std::string("SIGTSTP"))
               .with("source", std::string("keyboard"));
            eventBus_.publish(sig);
            return true;
        }
        default:
            return false;
    }
}

} // namespace re36

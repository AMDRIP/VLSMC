/**
 * @file console_driver.cpp
 * @brief Реализация системной консоли RAND Elecorner 36.
 */

#include "kernel/console_driver.h"
#include "kernel/event_bus.h"
#include "kernel/input_manager.h"
#include "kernel/scheduler.h"

#include <iostream>
#include <sstream>

namespace re36 {

ConsoleDriver::ConsoleDriver(EventBus& eventBus, InputManager& inputManager, Scheduler& scheduler)
    : eventBus_(eventBus),
      inputManager_(inputManager),
      scheduler_(scheduler) {}

ConsoleDriver::~ConsoleDriver() = default;

bool ConsoleDriver::init() {
    reset();
    initialized_ = true;

    Event evt(EventType::IoCompleted, 0, "console_driver");
    evt.with("action", std::string("init"))
       .with("device", std::string("console"));
    eventBus_.publish(evt);

    return true;
}

void ConsoleDriver::reset() {
    screenBuffer_.clear();
    currentLineBuffer_.clear();
    waitingProcesses_.clear();
    stats_ = ConsoleStats{};
    stats_.maxBufferLines = maxHistoryLines_;
}

void ConsoleDriver::tick(uint64_t /*currentTick*/) {
    if (!initialized_) return;

    // Читаем события клавиатуры из InputManager
    while (auto evOpt = inputManager_.pollEvent()) {
        const auto& ev = *evOpt;
        if (ev.type == InputEventType::Keyboard && (ev.keyEvent.action == KeyAction::Press || ev.keyEvent.action == KeyAction::Repeat)) {
            handleCharInput(ev.keyEvent);
        }
    }
}

void ConsoleDriver::writeText(const std::string& text, Pid pid) {
    if (!initialized_) return;

    appendToScreen(text);

    stats_.totalBytesWritten += text.length();

    // Отправляем событие
    Event evt(EventType::IoCompleted, 0, "console_driver");
    evt.with("action", std::string("write"))
       .with("device", std::string("console"))
       .with("pid", static_cast<int64_t>(pid));
    eventBus_.publish(evt);
}

std::optional<std::string> ConsoleDriver::requestReadLine(Pid pid) {
    if (!initialized_) return std::nullopt;

    // Если есть готовая строка - отдаем сразу
    if (!inputLines_.empty()) {
        std::string line = inputLines_.front();
        inputLines_.pop_front();
        return line;
    }

    // Иначе ставим процесс в очередь ожидания
    waitingProcesses_.push_back(pid);
    return std::nullopt;
}

void ConsoleDriver::handleCharInput(const KeyEvent& ev) {
    char c = ev.character;

    if (c == '\b') {
        // Backspace
        if (!currentLineBuffer_.empty()) {
            currentLineBuffer_.pop_back();
            // С точки зрения экранного буфера реализовать "удаление" сложно
            // если мы используем `appendToScreen`, но мы можем перерисовать
            // Для упрощения: просто удаляем символ из внутреннего буфера ввода.
            // (В GUI можно реализовать \b как спецсимвол и отрисовать затирание)
            appendToScreen("\b");
        }
    } else if (c == '\n' || c == '\r') {
        // Enter
        stats_.totalLinesRead++;
        stats_.totalBytesRead += currentLineBuffer_.length();

        appendToScreen("\n");

        inputLines_.push_back(currentLineBuffer_);
        currentLineBuffer_.clear();

        // Если есть ждущие процессы — будим первого (FIFO)
        if (!waitingProcesses_.empty()) {
            Pid pid = waitingProcesses_.front();
            waitingProcesses_.pop_front();

            // Будим процесс, чтобы он повторил системный вызов
            scheduler_.onIoComplete(pid);
        }
    } else if (ev.isPrintable()) {
        // Обычный символ
        if (currentLineBuffer_.size() < 256) {
            currentLineBuffer_ += c;
            appendToScreen(std::string(1, c));
        }
    }
}

void ConsoleDriver::appendToScreen(const std::string& text) {
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!currentLineBuffer_.empty() && text.front() != '\n') {
            currentLineBuffer_ += line;
            screenBuffer_.push_back(currentLineBuffer_);
            currentLineBuffer_.clear();
        } else {
            screenBuffer_.push_back(line);
        }
    }
    
    // Если текст не заканчивался переводом строки, остаток в currentLineBuffer_
    if (!text.empty() && text.back() != '\n') {
        currentLineBuffer_ = screenBuffer_.back();
        screenBuffer_.pop_back();
    }

    while (screenBuffer_.size() > maxHistoryLines_) {
        screenBuffer_.pop_front();
    }
    
    stats_.totalLinesWritten = screenBuffer_.size();
    stats_.currentBufferLines = static_cast<uint32_t>(screenBuffer_.size());

    publishScreenUpdate();
}

void ConsoleDriver::publishScreenUpdate() {
    Event evt(EventType::IoCompleted, 0, "console_driver");
    evt.with("action", std::string("screen_update"));
    eventBus_.publish(evt);
}

std::vector<std::string> ConsoleDriver::getScreenBuffer() const {
    std::vector<std::string> buf(screenBuffer_.begin(), screenBuffer_.end());
    // текущая строка для ввода уже не является просто `currentLineBuffer_` как раньше,
    // потому что эхо добавляет в экранный буфер. Поэтому здесь нам не нужно
    // вручную добавлять currentLineBuffer_. Эхо-символы уже в screenBuffer_.
    return buf;
}

void ConsoleDriver::setMaxHistory(size_t lines) {
    maxHistoryLines_ = lines;
}

void ConsoleDriver::clearScreen() {
    screenBuffer_.clear();
    currentLineBuffer_.clear();
    stats_.currentBufferLines = 0;
    publishScreenUpdate();
}

ConsoleStats ConsoleDriver::getStats() const {
    return stats_;
}

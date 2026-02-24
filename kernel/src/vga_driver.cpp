/**
 * @file vga_driver.cpp
 * @brief Реализация VGA драйвера для текстового режима RAND Elecorner 36.
 */

#include "kernel/vga_driver.h"
#include "kernel/event_bus.h"

namespace re36 {

VgaDriver::VgaDriver(EventBus& eventBus)
    : eventBus_(eventBus),
      buffer_(VGA_WIDTH * VGA_HEIGHT, VgaChar{' ', makeAttribute(VgaColor::LightGray, VgaColor::Black)}),
      currentAttribute_(makeAttribute(VgaColor::LightGray, VgaColor::Black)) {}

VgaDriver::~VgaDriver() = default;

bool VgaDriver::init() {
    reset();
    initialized_ = true;
    publishUpdate();
    return true;
}

void VgaDriver::reset() {
    cursorX_ = 0;
    cursorY_ = 0;
    currentAttribute_ = makeAttribute(VgaColor::LightGray, VgaColor::Black);
    clearScreen();
}

uint8_t VgaDriver::makeAttribute(VgaColor fg, VgaColor bg) {
    // В текстовом режиме цвет описывается байтом:
    // Младшие 4 бита — Foreground (fg)
    // Старшие 4 бита — Background (bg)
    return (static_cast<uint8_t>(bg) << 4) | (static_cast<uint8_t>(fg) & 0x0F);
}

uint8_t VgaDriver::getCurrentAttribute() const {
    return currentAttribute_;
}

void VgaDriver::setColor(VgaColor fg, VgaColor bg) {
    currentAttribute_ = makeAttribute(fg, bg);
}

void VgaDriver::clearScreen() {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i) {
        buffer_[i] = VgaChar{' ', currentAttribute_};
    }
    cursorX_ = 0;
    cursorY_ = 0;
    publishUpdate();
}

void VgaDriver::putCharAt(char c, uint16_t x, uint16_t y, uint8_t attribute) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) return;
    size_t index = y * VGA_WIDTH + x;
    buffer_[index] = VgaChar{c, attribute};
}

void VgaDriver::putChar(char c) {
    if (!initialized_) return;

    if (c == '\n') {
        cursorX_ = 0;
        cursorY_++;
    } else if (c == '\b') {
        if (cursorX_ > 0) {
            cursorX_--;
            putCharAt(' ', cursorX_, cursorY_, currentAttribute_);
        } else if (cursorY_ > 0) {
            cursorY_--;
            cursorX_ = VGA_WIDTH - 1;
            putCharAt(' ', cursorX_, cursorY_, currentAttribute_);
        }
    } else if (c == '\t') {
        cursorX_ = (cursorX_ + 4) & ~3; // Выравнивание до 4
    } else if (c == '\r') {
        cursorX_ = 0;
    } else if (c >= ' ') {
        putCharAt(c, cursorX_, cursorY_, currentAttribute_);
        cursorX_++;
    }

    if (cursorX_ >= VGA_WIDTH) {
        cursorX_ = 0;
        cursorY_++;
    }

    if (cursorY_ >= VGA_HEIGHT) {
        scrollUp();
        cursorY_ = VGA_HEIGHT - 1;
    }
}

void VgaDriver::print(const std::string& text) {
    if (!initialized_) return;

    for (char c : text) {
        putChar(c);
    }
    publishUpdate();
}

void VgaDriver::scrollUp() {
    // Сдвинуть все строки на 1 вверх
    for (size_t y = 1; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            size_t dst = (y - 1) * VGA_WIDTH + x;
            size_t src = y * VGA_WIDTH + x;
            buffer_[dst] = buffer_[src];
        }
    }
    // Очистить последнюю строку пустыми символами с текущим атрибутом
    for (size_t x = 0; x < VGA_WIDTH; ++x) {
        size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        buffer_[index] = VgaChar{' ', currentAttribute_};
    }
}

void VgaDriver::setCursorPosition(uint16_t x, uint16_t y) {
    if (x >= VGA_WIDTH) cursorX_ = VGA_WIDTH - 1; else cursorX_ = x;
    if (y >= VGA_HEIGHT) cursorY_ = VGA_HEIGHT - 1; else cursorY_ = y;
    publishUpdate();
}

uint16_t VgaDriver::getCursorX() const {
    return cursorX_;
}

uint16_t VgaDriver::getCursorY() const {
    return cursorY_;
}

const std::vector<VgaChar>& VgaDriver::getBuffer() const {
    return buffer_;
}

void VgaDriver::publishUpdate() {
    Event evt(EventType::IoCompleted, 0, "vga_driver");
    evt.with("action", std::string("vga_update"));
    eventBus_.publish(evt);
}

} // namespace re36

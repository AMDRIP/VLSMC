#include "kernel/mouse.h"
#include "kernel/pic.h"
#include "libc.h"

namespace re36 {

int32_t MouseDriver::current_x_ = 0;
int32_t MouseDriver::current_y_ = 0;
bool MouseDriver::left_btn_ = false;
bool MouseDriver::right_btn_ = false;
bool MouseDriver::middle_btn_ = false;

uint8_t MouseDriver::packet_[3] = {0};
uint8_t MouseDriver::packet_idx_ = 0;

void MouseDriver::wait_read() {
    int timeout = 100000;
    while (timeout--) {
        if ((inb(0x64) & 1) == 1) return;
    }
}

void MouseDriver::wait_write() {
    int timeout = 100000;
    while (timeout--) {
        if ((inb(0x64) & 2) == 0) return;
    }
}

void MouseDriver::write_command(uint8_t cmd) {
    wait_write();
    outb(0x64, cmd);
}

void MouseDriver::write_data(uint8_t data) {
    wait_write();
    outb(0x60, data);
}

uint8_t MouseDriver::read_data() {
    wait_read();
    return inb(0x60);
}

void MouseDriver::init() {
    printf("[Mouse] Initializing PS/2 Mouse Controller on 0x60/0x64...\n");

    // Enable the auxiliary mouse device
    write_command(0xA8);

    // Tell the keyboard controller that we want to read the compaq status byte
    write_command(0x20);
    uint8_t status = read_data();

    // Enable IRQ12 and disable mouse clock
    status |= (1 << 1); // Enable IRQ12
    status &= ~(1 << 5); // Clear disable mouse clock bit

    // Write the compaq status byte back
    write_command(0x60);
    write_data(status);

    // Tell mouse to use default settings
    write_command(0xD4);
    write_data(0xF6);
    read_data(); // Ack

    // Enable Data Reporting
    write_command(0xD4);
    write_data(0xF4);
    read_data(); // Ack

    printf("[Mouse] PS/2 Mouse successfully initialized.\n");
}

void MouseDriver::handle_interrupt() {
    // 1. Check Status Bit 5 (Mouse Output Buffer Full)
    // If it's not set, the byte doesn't belong to the mouse (e.g. keyboard stroke)
    uint8_t status = inb(0x64);
    if (!(status & 0x20)) {
        return; // Drop fake interrupts
    }

    uint8_t byte = inb(0x60);

    // 2. Synchronize packet!
    if (packet_idx_ == 0) {
        if (!(byte & 0x08)) {
            // Out of sync! The first byte of the PS/2 packet must always have bit 3 set (value 00001000=0x08).
            // Drop it and wait until we find the real first byte.
            return;
        }
    }

    packet_[packet_idx_] = byte;
    packet_idx_++;

    // 3. Packet assembly (3 bytes)
    if (packet_idx_ >= 3) {
        // Parse the packets
        left_btn_ = packet_[0] & 0x01;
        right_btn_ = packet_[0] & 0x02;
        middle_btn_ = packet_[0] & 0x04;

        int8_t dx = packet_[1];
        int8_t dy = packet_[2];

        // Apply X sign extension
        if (packet_[0] & 0x10) {
            dx = (int8_t)(packet_[1] | 0xFFFFFF00);
        }
        
        // Apply Y sign extension
        if (packet_[0] & 0x20) {
            dy = (int8_t)(packet_[2] | 0xFFFFFF00);
        }

        current_x_ += dx;
        current_y_ -= dy; // In PS/2, positive Y means up

        // Just basic bounds (example: 1024x768 hypothetical view)
        if (current_x_ < 0) current_x_ = 0;
        if (current_x_ > 1024) current_x_ = 1024;
        if (current_y_ < 0) current_y_ = 0;
        if (current_y_ > 768) current_y_ = 768;

        packet_idx_ = 0;
    }
}

MouseState MouseDriver::get_state() {
    MouseState state;
    state.x = current_x_;
    state.y = current_y_;
    state.left_btn = left_btn_;
    state.right_btn = right_btn_;
    state.middle_btn = middle_btn_;
    return state;
}

} // namespace re36

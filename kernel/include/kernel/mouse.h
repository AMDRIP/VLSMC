#pragma once

#include <stdint.h>

namespace re36 {

struct MouseState {
    int32_t x;
    int32_t y;
    bool left_btn;
    bool right_btn;
    bool middle_btn;
};

class MouseDriver {
public:
    static void init();
    static void handle_interrupt();

    // Returns the latest known state of the mouse
    static MouseState get_state();

private:
    static void wait_read();
    static void wait_write();
    static void write_command(uint8_t cmd);
    static void write_data(uint8_t data);
    static uint8_t read_data();
    
    static int32_t current_x_;
    static int32_t current_y_;
    static bool left_btn_;
    static bool right_btn_;
    static bool middle_btn_;

    static uint8_t packet_[3];
    static uint8_t packet_idx_;
};

} // namespace re36

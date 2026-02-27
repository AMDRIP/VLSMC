#pragma once

#include <stdint.h>

namespace re36 {

class BgaDriver {
public:
    static void init(uint16_t width, uint16_t height, uint16_t bpp);

    // Basic drawing primitives for testing
    static void put_pixel(uint32_t x, uint32_t y, uint32_t color);
    static void clear_screen(uint32_t color);

    static uint16_t get_width() { return width_; }
    static uint16_t get_height() { return height_; }
    static uint16_t get_bpp() { return bpp_; }
    static uint32_t get_pitch() { return pitch_; }

private:
    static void write_register(uint16_t index, uint16_t val);
    static uint16_t read_register(uint16_t index);

    static uint16_t width_;
    static uint16_t height_;
    static uint16_t bpp_;
    static uint32_t pitch_; // Line width in bytes

    static uint32_t framebuffer_phys_;
    static uint32_t framebuffer_virt_;
    static uint32_t framebuffer_size_;
};

} // namespace re36

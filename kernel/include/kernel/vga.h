#pragma once

#include <stdint.h>

namespace re36 {

#define VGA_WIDTH  320
#define VGA_HEIGHT 200
#define VGA_FBADDR 0xA0000

class VGA {
public:
    static void init_mode13h();
    static void init_text_mode();
    static bool is_graphics();

    static void put_pixel(int x, int y, uint8_t color);
    static uint8_t get_pixel(int x, int y);
    static void clear(uint8_t color);

    static void fill_rect(int x, int y, int w, int h, uint8_t color);
    static void draw_rect(int x, int y, int w, int h, uint8_t color);
    static void draw_line(int x0, int y0, int x1, int y1, uint8_t color);
    static void draw_circle(int cx, int cy, int r, uint8_t color);
    static void fill_circle(int cx, int cy, int r, uint8_t color);

    static void draw_char(int x, int y, char c, uint8_t fg, uint8_t bg);
    static void draw_string(int x, int y, const char* str, uint8_t fg, uint8_t bg);

    static void set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
    static void set_default_palette();

    static void demo();

private:
    static bool graphics_mode_;
    static uint8_t* framebuffer_;

    static void write_regs(const uint8_t* regs);
};

} // namespace re36

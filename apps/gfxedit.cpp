#include "app_api.h"
#include "vesa_driver.h"

extern "C" void __cxa_pure_virtual() {
    vlsmc::App::print("Pure virtual function call!\n");
    vlsmc::App::exit(1);
}

using namespace vlsmc;

namespace {

// Public-domain style 8x8 ASCII font (0..127)
static const uint8_t FONT8X8[128][8] = {
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
{0x36,0x36,0x24,0x00,0x00,0x00,0x00,0x00},{0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00},
{0x18,0x3E,0x03,0x1E,0x30,0x1F,0x18,0x00},{0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00},
{0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00},{0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00},
{0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00},{0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00},
{0x00,0x66,0x3C,0x7F,0x3C,0x66,0x00,0x00},{0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06},{0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00},
{0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00},{0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00},
{0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00},{0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00},
{0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00},{0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00},
{0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00},{0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00},
{0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00},{0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00},
{0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00},{0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00},
{0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00},{0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06},
{0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00},{0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00},
{0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},{0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00},
{0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00},{0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00},
{0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00},{0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00},
{0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00},{0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00},
{0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00},{0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00},
{0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00},{0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
{0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00},{0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00},
{0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00},{0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00},
{0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00},{0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00},
{0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00},{0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00},
{0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00},{0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00},
{0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},{0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00},
{0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00},{0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
{0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00},{0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00},
{0x7F,0x73,0x19,0x0C,0x26,0x63,0x7F,0x00},{0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00},
{0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00},{0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00},
{0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
{0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00},
{0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00},{0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00},
{0x38,0x30,0x30,0x3E,0x33,0x33,0x6E,0x00},{0x00,0x00,0x1E,0x33,0x3F,0x03,0x1E,0x00},
{0x1C,0x36,0x06,0x0F,0x06,0x06,0x0F,0x00},{0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F},
{0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00},{0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00},
{0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E},{0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00},
{0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},{0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00},
{0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00},{0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00},
{0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F},{0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78},
{0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00},{0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00},
{0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00},{0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00},
{0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00},{0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00},
{0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00},{0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F},
{0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00},{0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00},
{0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},{0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00},
{0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00},{0,0,0,0,0,0,0,0},

// Remaining control chars 96..127 unused fallback to empty/simple
{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}
};

static const int CHAR_W = 8;
static const int CHAR_H = 8;

static const int MAX_TEXT = 128 * 1024;
static char text_buf[MAX_TEXT];
static int text_len = 0;
static int cursor = 0;
static int view_line = 0;
static bool modified = false;
static const char* FILE_PATH = "/EDIT.TXT";

static int screen_w = 1024;
static int screen_h = 768;
static uint32_t fb_pixels = 1024 * 768;
static uint32_t fb_bytes = 1024 * 768 * 4;

struct EditorLayout {
    int x;
    int y;
    int w;
    int h;
    int title_h;
    int status_h;
    int gutter_w;
    int text_x;
    int text_y;
    int text_w_px;
    int text_h_px;
    int text_cols;
    int text_rows;
};

static EditorLayout layout;

void mem_move(char* dst, const char* src, int n) {
    if (n <= 0 || dst == src) return;
    if (dst < src) {
        for (int i = 0; i < n; i++) dst[i] = src[i];
    } else {
        for (int i = n - 1; i >= 0; i--) dst[i] = src[i];
    }
}

int str_len(const char* s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}

void append_char(char* dst, int& pos, int max, char c) {
    if (pos >= max - 1) return;
    dst[pos++] = c;
    dst[pos] = '\0';
}

void append_str(char* dst, int& pos, int max, const char* s) {
    for (int i = 0; s && s[i]; i++) append_char(dst, pos, max, s[i]);
}

void append_uint(char* dst, int& pos, int max, uint32_t value) {
    char tmp[12];
    int len = 0;
    if (value == 0) {
        append_char(dst, pos, max, '0');
        return;
    }
    while (value > 0 && len < 11) {
        tmp[len++] = (char)('0' + (value % 10));
        value /= 10;
    }
    for (int i = len - 1; i >= 0; i--) append_char(dst, pos, max, tmp[i]);
}

void uint_to_dec(uint32_t value, char* out) {
    int pos = 0;
    append_uint(out, pos, 12, value);
}

void recompute_layout() {
    int margin = screen_w >= 900 ? 32 : 12;
    layout.x = margin;
    layout.y = margin;
    layout.w = screen_w - margin * 2;
    layout.h = screen_h - margin * 2;

    if (layout.w < 320) {
        layout.x = 4;
        layout.w = screen_w - 8;
    }
    if (layout.h < 220) {
        layout.y = 4;
        layout.h = screen_h - 8;
    }

    layout.title_h = 34;
    layout.status_h = 26;
    layout.gutter_w = 54;
    layout.text_x = layout.x + layout.gutter_w + 10;
    layout.text_y = layout.y + layout.title_h + 10;
    layout.text_w_px = layout.w - layout.gutter_w - 20;
    layout.text_h_px = layout.h - layout.title_h - layout.status_h - 20;
    layout.text_cols = layout.text_w_px / CHAR_W;
    layout.text_rows = layout.text_h_px / CHAR_H;

    if (layout.text_cols < 8) layout.text_cols = 8;
    if (layout.text_rows < 4) layout.text_rows = 4;
}

void fill_rect(uint32_t* fb, int x, int y, int w, int h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > screen_w) w = screen_w - x;
    if (y + h > screen_h) h = screen_h - y;
    if (w <= 0 || h <= 0) return;

    for (int yy = 0; yy < h; yy++) {
        for (int xx = 0; xx < w; xx++) {
            fb[(y + yy) * screen_w + (x + xx)] = color;
        }
    }
}

void draw_rect(uint32_t* fb, int x, int y, int w, int h, uint32_t color) {
    fill_rect(fb, x, y, w, 1, color);
    fill_rect(fb, x, y + h - 1, w, 1, color);
    fill_rect(fb, x, y, 1, h, color);
    fill_rect(fb, x + w - 1, y, 1, h, color);
}

uint32_t dim_color(uint32_t color) {
    uint32_t r = (color >> 16) & 0xFF;
    uint32_t g = (color >> 8) & 0xFF;
    uint32_t b = color & 0xFF;
    r = (r * 5) / 8;
    g = (g * 5) / 8;
    b = (b * 5) / 8;
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

void prepare_backdrop(uint32_t* dst, const uint32_t* shell_snapshot) {
    if (!shell_snapshot) {
        fill_rect(dst, 0, 0, screen_w, screen_h, 0xFF0E1116);
        return;
    }
    for (uint32_t i = 0; i < fb_pixels; i++) {
        dst[i] = dim_color(shell_snapshot[i]);
    }
}

void restore_shell(VesaDriver& drv, DriverContext& ctx, const uint32_t* shell_snapshot) {
    if (!shell_snapshot) return;
    drv.seek(&ctx, 0);
    drv.write(&ctx, shell_snapshot, fb_bytes);
    drv.ioctl(&ctx, VESA_CMD_FLUSH, nullptr);
}

void draw_glyph_at(uint32_t* fb, int px, int py, char ch, uint32_t fg, uint32_t bg) {
    uint8_t raw = (uint8_t)ch;
    uint8_t idx = 0;
    if (raw >= 32 && raw < 128) {
        idx = raw - 32;
    } else if (raw >= 128) {
        idx = '?' - 32;
    }
    const uint8_t* g = FONT8X8[idx];

    for (int y = 0; y < 8; y++) {
        int sy = py + y;
        if (sy < 0 || sy >= screen_h) continue;
        uint8_t bits = g[y];
        for (int x = 0; x < 8; x++) {
            int sx = px + x;
            if (sx < 0 || sx >= screen_w) continue;
            bool on = (bits & (1u << x)) != 0;
            fb[sy * screen_w + sx] = on ? fg : bg;
        }
    }
}

void draw_char(uint32_t* fb, int cx, int cy, char ch, uint32_t fg, uint32_t bg) {
    int px = layout.text_x + cx * CHAR_W;
    int py = layout.text_y + cy * CHAR_H;
    draw_glyph_at(fb, px, py, ch, fg, bg);
}

void draw_text_clipped(uint32_t* fb, int px, int py, const char* s, uint32_t fg, uint32_t bg, int clip_right) {
    int x = px;
    for (int i = 0; s[i]; i++) {
        if (x + CHAR_W > clip_right) return;
        draw_glyph_at(fb, x, py, s[i], fg, bg);
        x += CHAR_W;
    }
}

void draw_text(uint32_t* fb, int px, int py, const char* s, uint32_t fg, uint32_t bg) {
    draw_text_clipped(fb, px, py, s, fg, bg, screen_w);
}

void index_to_linecol(int idx, int& line, int& col) {
    line = 0;
    col = 0;
    for (int i = 0; i < idx && i < text_len; i++) {
        if (text_buf[i] == '\n') { line++; col = 0; }
        else col++;
    }
}

int linecol_to_index(int target_line, int target_col) {
    int line = 0;
    int col = 0;
    for (int i = 0; i < text_len; i++) {
        if (line == target_line && col == target_col) return i;
        if (text_buf[i] == '\n') {
            if (line == target_line) return i;
            line++; col = 0;
        } else {
            col++;
        }
    }
    return text_len;
}

void ensure_cursor_visible() {
    int line, col;
    index_to_linecol(cursor, line, col);
    if (line < view_line) view_line = line;
    if (line >= view_line + layout.text_rows) view_line = line - layout.text_rows + 1;
    if (view_line < 0) view_line = 0;
}

void insert_char(char c) {
    if (text_len >= MAX_TEXT - 1) return;
    mem_move(text_buf + cursor + 1, text_buf + cursor, text_len - cursor);
    text_buf[cursor] = c;
    cursor++;
    text_len++;
    modified = true;
}

void backspace_char() {
    if (cursor <= 0) return;
    mem_move(text_buf + cursor - 1, text_buf + cursor, text_len - cursor);
    cursor--;
    text_len--;
    modified = true;
}

void delete_char() {
    if (cursor >= text_len) return;
    mem_move(text_buf + cursor, text_buf + cursor + 1, text_len - cursor - 1);
    text_len--;
    modified = true;
}

bool load_file() {
    int fd = App::open(FILE_PATH, FMODE_READ);
    if (fd < 0) {
        text_len = 0;
        cursor = 0;
        modified = false;
        return false;
    }

    int got = App::read(fd, text_buf, MAX_TEXT - 1);
    App::close(fd);

    if (got < 0) {
        text_len = 0;
        cursor = 0;
        modified = false;
        return false;
    }

    text_len = got;
    cursor = 0;
    modified = false;
    return true;
}

bool save_file() {
    int fd = App::open(FILE_PATH, FMODE_WRITE);
    if (fd < 0) return false;
    int wr = App::write(fd, text_buf, (uint32_t)text_len);
    App::close(fd);
    if (wr < 0) return false;
    modified = false;
    return true;
}

int count_lines() {
    int lines = 1;
    for (int i = 0; i < text_len; i++) {
        if (text_buf[i] == '\n') lines++;
    }
    return lines;
}

void draw_status(uint32_t* fb, bool save_ok, bool loaded_ok) {
    int cur_line = 0, cur_col = 0;
    index_to_linecol(cursor, cur_line, cur_col);

    char left[128];
    int pos = 0;
    left[0] = '\0';
    append_str(left, pos, 128, FILE_PATH);
    append_str(left, pos, 128, modified ? "  modified" : "  saved");
    append_str(left, pos, 128, loaded_ok ? "  load:ok" : "  load:new");
    append_str(left, pos, 128, save_ok ? "  save:ok" : "  save:--");

    char right[96];
    pos = 0;
    right[0] = '\0';
    append_str(right, pos, 96, "Ln ");
    append_uint(right, pos, 96, (uint32_t)(cur_line + 1));
    append_str(right, pos, 96, ", Col ");
    append_uint(right, pos, 96, (uint32_t)(cur_col + 1));
    append_str(right, pos, 96, "  ");
    append_uint(right, pos, 96, (uint32_t)text_len);
    append_str(right, pos, 96, "b");

    int y = layout.y + layout.h - layout.status_h + 9;
    draw_text_clipped(fb, layout.x + 14, y, left, 0xFFC8D0DA, 0xFF171D24, layout.x + layout.w - 12);

    int right_w = str_len(right) * CHAR_W;
    int right_x = layout.x + layout.w - right_w - 14;
    if (right_x > layout.x + 14) {
        draw_text(fb, right_x, y, right, 0xFF8FC7FF, 0xFF171D24);
    }
}

void draw_title(uint32_t* fb) {
    fill_rect(fb, layout.x, layout.y, layout.w, layout.title_h, 0xFF1E2F46);
    draw_text(fb, layout.x + 14, layout.y + 12, "GFXEDIT.ELF", 0xFFFFFFFF, 0xFF1E2F46);

    const char* hint = "Ctrl+S save  ESC quit  arrows move";
    int hint_w = str_len(hint) * CHAR_W;
    int hint_x = layout.x + layout.w - hint_w - 14;
    if (hint_x > layout.x + 130) {
        draw_text(fb, hint_x, layout.y + 12, hint, 0xFFB8C7D9, 0xFF1E2F46);
    }
}

void draw_editor(uint32_t* fb, const uint32_t* shell_snapshot, bool save_ok, bool loaded_ok) {
    prepare_backdrop(fb, shell_snapshot);

    fill_rect(fb, layout.x + 7, layout.y + 8, layout.w, layout.h, 0x77000000);
    fill_rect(fb, layout.x, layout.y, layout.w, layout.h, 0xFF11161C);
    draw_rect(fb, layout.x, layout.y, layout.w, layout.h, 0xFF4F6680);
    draw_title(fb);

    int body_y = layout.y + layout.title_h;
    int body_h = layout.h - layout.title_h - layout.status_h;
    fill_rect(fb, layout.x, body_y, layout.gutter_w, body_h, 0xFF151B22);
    fill_rect(fb, layout.x + layout.gutter_w, body_y, layout.w - layout.gutter_w, body_h, 0xFF0F141A);
    fill_rect(fb, layout.x, layout.y + layout.h - layout.status_h, layout.w, layout.status_h, 0xFF171D24);

    int cur_line = 0, cur_col = 0;
    index_to_linecol(cursor, cur_line, cur_col);

    int cursor_row = cur_line - view_line;
    if (cursor_row >= 0 && cursor_row < layout.text_rows) {
        fill_rect(fb, layout.x + layout.gutter_w, layout.text_y + cursor_row * CHAR_H, layout.w - layout.gutter_w, CHAR_H, 0xFF182534);
    }

    int total_lines = count_lines();
    for (int row = 0; row < layout.text_rows; row++) {
        int visible_line = view_line + row;
        int py = layout.text_y + row * CHAR_H;

        if (visible_line < total_lines) {
            char num[12];
            uint_to_dec((uint32_t)(visible_line + 1), num);
            int nx = layout.x + layout.gutter_w - (str_len(num) + 1) * CHAR_W;
            draw_text(fb, nx, py, num, visible_line == cur_line ? 0xFFE8F0FA : 0xFF697789, visible_line == cur_line ? 0xFF182534 : 0xFF151B22);
        }

        int idx = linecol_to_index(visible_line, 0);
        for (int col = 0; col < layout.text_cols && idx < text_len && text_buf[idx] != '\n'; col++, idx++) {
            char ch = text_buf[idx];
            if (ch == '\t') ch = ' ';
            draw_char(fb, col, row, ch, 0xFFE8EDF2, visible_line == cur_line ? 0xFF182534 : 0xFF0F141A);
        }
    }

    if (cursor_row >= 0 && cursor_row < layout.text_rows && cur_col >= 0 && cur_col < layout.text_cols) {
        int px = layout.text_x + cur_col * CHAR_W;
        int py = layout.text_y + cursor_row * CHAR_H;
        fill_rect(fb, px, py + 7, 8, 1, 0xFFFFFF00);
    }

    draw_status(fb, save_ok, loaded_ok);
}

} // namespace

int main() {
    VesaDriver drv;
    DriverContext ctx{};
    if (drv.init(&ctx) != 0) {
        App::print("GFXEDIT: VESA init failed\n");
        return 1;
    }

    VesaResolution res{};
    if (drv.ioctl(&ctx, VESA_CMD_GET_RES, &res) != 0 || res.width == 0 || res.height == 0 || res.bpp != 32) {
        App::print("GFXEDIT: requires 32-bit BGA framebuffer\n");
        drv.stop(&ctx);
        return 1;
    }

    screen_w = (int)res.width;
    screen_h = (int)res.height;
    fb_pixels = res.width * res.height;
    fb_bytes = fb_pixels * 4;
    recompute_layout();

    uint32_t* shell_snapshot = (uint32_t*)App::malloc(fb_bytes);
    uint32_t* backbuffer = (uint32_t*)App::malloc(fb_bytes);
    if (!shell_snapshot || !backbuffer) {
        App::print("GFXEDIT: Not enough memory for overlay buffers\n");
        if (backbuffer) App::free(backbuffer);
        if (shell_snapshot) App::free(shell_snapshot);
        drv.stop(&ctx);
        return 1;
    }

    bool shell_ok = false;
    if (drv.seek(&ctx, 0) == 0) {
        int got = drv.read(&ctx, shell_snapshot, fb_bytes);
        shell_ok = (got == (int)fb_bytes);
    }
    if (!shell_ok) {
        for (uint32_t i = 0; i < fb_pixels; i++) shell_snapshot[i] = 0xFF0E1116;
    }

    bool loaded_ok = load_file();
    bool save_ok = false;
    bool needs_redraw = true;

    while (true) {
        if (needs_redraw) {
            draw_editor(backbuffer, shell_snapshot, save_ok, loaded_ok);
            drv.seek(&ctx, 0);
            drv.write(&ctx, backbuffer, fb_bytes);
            needs_redraw = false;
        }

        char c = App::getchar();

        if (c == 27) { // ESC
            break;
        } else if (c == (char)0x80) { // UP
            int line, col; index_to_linecol(cursor, line, col);
            if (line > 0) cursor = linecol_to_index(line - 1, col);
        } else if (c == (char)0x81) { // DOWN
            int line, col; index_to_linecol(cursor, line, col);
            cursor = linecol_to_index(line + 1, col);
        } else if (c == (char)0x82) { // LEFT
            if (cursor > 0) cursor--;
        } else if (c == (char)0x83) { // RIGHT
            if (cursor < text_len) cursor++;
        } else if (c == (char)0x84) { // DELETE
            delete_char();
            save_ok = false;
        } else if (c == 19) { // Ctrl+S
            save_ok = save_file();
        } else if (c == '\b' || c == 127 || c == 8) { // Backspace
            backspace_char();
            save_ok = false;
        } else if (c == '\n' || c == '\r') {
            insert_char('\n');
            save_ok = false;
        } else if (c == '\t') {
            for (int i = 0; i < 4; i++) insert_char(' ');
            save_ok = false;
        } else if (c >= 32 || c == '\t') {
            insert_char(c);
            save_ok = false;
        }

        ensure_cursor_visible();
        needs_redraw = true;
    }

    restore_shell(drv, ctx, shell_snapshot);
    App::free(backbuffer);
    App::free(shell_snapshot);
    drv.stop(&ctx);
    App::print("GFXEDIT: exit\n");
    return 0;
}

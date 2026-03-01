#include "app_api.h"
#include "vesa_driver.h"

void operator delete(void*, unsigned int) {}
void operator delete(void*) {}
extern "C" void __cxa_pure_virtual() {
    vlsmc::App::print("Pure virtual function call!\n");
    vlsmc::App::exit(1);
}

using namespace vlsmc;

namespace {

static const int BOARD_W = 10;
static const int BOARD_H = 20;
static const int CELL = 16;

static const int FIELD_X = 80;
static const int FIELD_Y = 40;

struct Vec2 { int x; int y; };

// [piece][rotation][block]
static const Vec2 PIECES[7][4][4] = {
    // I
    {{{0,1},{1,1},{2,1},{3,1}}, {{2,0},{2,1},{2,2},{2,3}}, {{0,2},{1,2},{2,2},{3,2}}, {{1,0},{1,1},{1,2},{1,3}}},
    // O
    {{{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}}},
    // T
    {{{1,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{2,1},{1,2}}, {{0,1},{1,1},{2,1},{1,2}}, {{1,0},{0,1},{1,1},{1,2}}},
    // S
    {{{1,0},{2,0},{0,1},{1,1}}, {{1,0},{1,1},{2,1},{2,2}}, {{1,1},{2,1},{0,2},{1,2}}, {{0,0},{0,1},{1,1},{1,2}}},
    // Z
    {{{0,0},{1,0},{1,1},{2,1}}, {{2,0},{1,1},{2,1},{1,2}}, {{0,1},{1,1},{1,2},{2,2}}, {{1,0},{0,1},{1,1},{0,2}}},
    // J
    {{{0,0},{0,1},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{1,2}}, {{0,1},{1,1},{2,1},{2,2}}, {{1,0},{1,1},{0,2},{1,2}}},
    // L
    {{{2,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{1,2},{2,2}}, {{0,1},{1,1},{2,1},{0,2}}, {{0,0},{1,0},{1,1},{1,2}}},
};

static const uint32_t COLORS[8] = {
    0xFF111111, // empty
    0xFF00FFFF, // I
    0xFFFFFF00, // O
    0xFFFF00FF, // T
    0xFF00FF00, // S
    0xFFFF0000, // Z
    0xFF0000FF, // J
    0xFFFFA500  // L
};

struct Piece {
    int type;
    int rot;
    int x;
    int y;
};

int board[BOARD_H][BOARD_W];
uint32_t rng_state = 0;

uint32_t next_rand() {
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state;
}

int random_piece() {
    return (int)(next_rand() % 7u);
}

void fill_rect(VesaDriver& drv, int x, int y, int w, int h, uint32_t color) {
    for (int py = 0; py < h; py++) {
        for (int px = 0; px < w; px++) {
            drv.draw_pixel((uint32_t)(x + px), (uint32_t)(y + py), color);
        }
    }
}

void draw_cell(VesaDriver& drv, int gx, int gy, int cell, bool active) {
    int px = FIELD_X + gx * CELL;
    int py = FIELD_Y + gy * CELL;
    uint32_t c = COLORS[cell];

    fill_rect(drv, px, py, CELL - 1, CELL - 1, c);
    if (active) {
        fill_rect(drv, px + 2, py + 2, CELL - 5, 2, 0xFFFFFFFF);
    }
}

bool collides(const Piece& p, int nx, int ny, int nrot) {
    for (int i = 0; i < 4; i++) {
        int x = nx + PIECES[p.type][nrot][i].x;
        int y = ny + PIECES[p.type][nrot][i].y;

        if (x < 0 || x >= BOARD_W || y < 0 || y >= BOARD_H) return true;
        if (board[y][x] != 0) return true;
    }
    return false;
}

void lock_piece(const Piece& p) {
    for (int i = 0; i < 4; i++) {
        int x = p.x + PIECES[p.type][p.rot][i].x;
        int y = p.y + PIECES[p.type][p.rot][i].y;
        if (x >= 0 && x < BOARD_W && y >= 0 && y < BOARD_H) {
            board[y][x] = p.type + 1;
        }
    }
}

int clear_lines() {
    int cleared = 0;
    for (int y = BOARD_H - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < BOARD_W; x++) {
            if (board[y][x] == 0) { full = false; break; }
        }
        if (full) {
            cleared++;
            for (int yy = y; yy > 0; yy--) {
                for (int x = 0; x < BOARD_W; x++) board[yy][x] = board[yy - 1][x];
            }
            for (int x = 0; x < BOARD_W; x++) board[0][x] = 0;
            y++;
        }
    }
    return cleared;
}

void draw_field(VesaDriver& drv, const Piece& active, bool game_over) {
    fill_rect(drv, 0, 0, 1024, 768, game_over ? 0xFF220000 : 0xFF000000);

    fill_rect(drv, FIELD_X - 3, FIELD_Y - 3, BOARD_W * CELL + 6, BOARD_H * CELL + 6, 0xFF555555);
    fill_rect(drv, FIELD_X, FIELD_Y, BOARD_W * CELL, BOARD_H * CELL, 0xFF111111);

    for (int y = 0; y < BOARD_H; y++) {
        for (int x = 0; x < BOARD_W; x++) {
            draw_cell(drv, x, y, board[y][x], false);
        }
    }

    if (!game_over) {
        for (int i = 0; i < 4; i++) {
            int x = active.x + PIECES[active.type][active.rot][i].x;
            int y = active.y + PIECES[active.type][active.rot][i].y;
            if (x >= 0 && x < BOARD_W && y >= 0 && y < BOARD_H) {
                draw_cell(drv, x, y, active.type + 1, true);
            }
        }
    }

    // Side panel bars (simple no-font HUD)
    fill_rect(drv, FIELD_X + BOARD_W * CELL + 24, FIELD_Y, 220, 20, 0xFF333333);
    fill_rect(drv, FIELD_X + BOARD_W * CELL + 24, FIELD_Y + 28, 220, 14, 0xFF666666);
    fill_rect(drv, FIELD_X + BOARD_W * CELL + 24, FIELD_Y + 50, 220, 14, 0xFF444444);
}

void reset_board() {
    for (int y = 0; y < BOARD_H; y++) {
        for (int x = 0; x < BOARD_W; x++) board[y][x] = 0;
    }
}

} // namespace

int main() {
    VesaDriver drv;
    DriverContext ctx{};

    if (drv.init(&ctx) != 0) {
        App::print("TETRIS: VESA init failed\n");
        return 1;
    }

    rng_state = App::uptime() ^ 0xA5A55A5Au;
    if (rng_state == 0) rng_state = 1;

    reset_board();

    Piece cur{random_piece(), 0, 3, 0};
    int lines = 0;
    int score = 0;
    bool game_over = false;

    uint32_t last_tick = App::uptime();
    uint32_t last_drop = last_tick;
    uint32_t drop_ms = 500;

    while (true) {
        uint32_t now = App::uptime();
        if (now - last_tick < 30) {
            App::sleep(1);
            continue;
        }
        last_tick = now;

        // Non-blocking keyboard poll through PS/2 controller status/data ports.
        while ((App::inb(0x64) & 0x01) != 0) {
            uint8_t sc = App::inb(0x60);
            if (sc & 0x80) continue; // ignore key release

            if (sc == 0x10) { // Q
                drv.stop(&ctx);
                App::print("TETRIS: exit\n");
                return 0;
            }

            if (game_over) {
                if (sc == 0x39) { // SPACE restart
                    reset_board();
                    cur = Piece{random_piece(), 0, 3, 0};
                    lines = 0;
                    score = 0;
                    drop_ms = 500;
                    game_over = false;
                }
                continue;
            }

            if (sc == 0x1E) { // A left
                if (!collides(cur, cur.x - 1, cur.y, cur.rot)) cur.x--;
            } else if (sc == 0x20) { // D right
                if (!collides(cur, cur.x + 1, cur.y, cur.rot)) cur.x++;
            } else if (sc == 0x1F) { // S soft drop
                if (!collides(cur, cur.x, cur.y + 1, cur.rot)) cur.y++;
            } else if (sc == 0x11) { // W rotate
                int nr = (cur.rot + 1) & 3;
                if (!collides(cur, cur.x, cur.y, nr)) cur.rot = nr;
            } else if (sc == 0x39) { // SPACE hard drop
                while (!collides(cur, cur.x, cur.y + 1, cur.rot)) cur.y++;
                last_drop = 0;
            }
        }

        if (!game_over && now - last_drop >= drop_ms) {
            last_drop = now;

            if (!collides(cur, cur.x, cur.y + 1, cur.rot)) {
                cur.y++;
            } else {
                lock_piece(cur);
                int c = clear_lines();
                lines += c;
                score += (c == 1 ? 100 : c == 2 ? 300 : c == 3 ? 500 : c >= 4 ? 800 : 0);

                if (lines < 50) {
                    uint32_t speedup = (uint32_t)(lines * 8);
                    drop_ms = (500 > speedup + 120) ? (500 - speedup) : 120;
                }

                cur = Piece{random_piece(), 0, 3, 0};
                if (collides(cur, cur.x, cur.y, cur.rot)) {
                    game_over = true;
                    App::print("TETRIS: GAME OVER (SPACE to restart, Q to quit)\n");
                    (void)score;
                }
            }
        }

        draw_field(drv, cur, game_over);
    }

    return 0;
}

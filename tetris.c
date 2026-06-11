#include <stdbool.h>
#include <stdint.h>
#include <util/delay.h>
#include "screen.h"
#include "input.h"

// rozmiar pojedynczego kwadratu w pikselach
#define CELL_SIZE       8
// wielkość planszy tetris
#define GRID_WIDTH      10
#define GRID_HEIGHT     16
// pozycja planszy na ekranie
#define BOARD_X         24
#define BOARD_Y         8
// panel na next block i score
#define STATUS_X        108
#define PREVIEW_CELL    4
// opóźnienie między klatkami
#define TICK_MS         40
#define GRAVITY_STEPS   8
#define HORIZ_REPEAT    3
#define SOFT_REPEAT     2

// kolory dla bloków i tła
static const uint16_t piece_colors[8] = {
    COLOR_BLACK,
    COLOR_CYAN,
    COLOR_YELLOW,
    COLOR_MAGENTA,
    COLOR_GREEN,
    COLOR_RED,
    COLOR_BLUE,
    COLOR_WHITE
};

// kształty tetromin, zapisane jako 4x4 bitmapy
static const uint16_t piece_shapes[7][4] = {
    {0x0F00, 0x2222, 0x00F0, 0x4444}, // I
    {0x6600, 0x6600, 0x6600, 0x6600}, // O
    {0x0E40, 0x4C40, 0x4E00, 0x4640}, // T
    {0x06C0, 0x4620, 0x06C0, 0x4620}, // S
    {0x0C60, 0x4C80, 0x0C60, 0x4C80}, // Z
    {0x08E0, 0x6440, 0x0E20, 0x44C0}, // J
    {0x02E0, 0x4460, 0x0E80, 0xC440}  // L
};

static uint8_t board[GRID_HEIGHT][GRID_WIDTH];
static uint8_t current_type;
static uint8_t current_rot;
static int8_t current_x;
static int8_t current_y;
static uint8_t next_type;
static uint16_t lines_cleared;
static bool game_over_flag;
static uint16_t random_state = 0xBEEF;
static uint8_t prev_held;
static uint8_t horiz_timer;
static uint8_t soft_timer;
static uint8_t gravity_timer;
static int8_t prev_piece_x;
static int8_t prev_piece_y;
static uint8_t prev_piece_rot;
static uint8_t prev_piece_type;
static bool moving_piece_drawn;
static bool board_redraw_needed;
static uint8_t prev_next_type;
static uint16_t prev_lines_cleared;
static bool preview_drawn;
static bool score_drawn;

static inline uint16_t random16(void)
{
    random_state = (uint16_t)(random_state * 2053u + 13849u);
    return random_state;
}

static inline bool piece_bit(uint8_t type, uint8_t rotation, uint8_t row, uint8_t col)
{
    uint16_t shape = piece_shapes[type][rotation];
    return (shape >> (15 - (row * 4 + col))) & 1u;
}

// sprawdza, czy klocek da się postawić na tej pozycji
static bool can_place(uint8_t type, uint8_t rotation, int8_t x, int8_t y)
{
    for (uint8_t ry = 0; ry < 4; ++ry) {
        for (uint8_t rx = 0; rx < 4; ++rx) {
            if (!piece_bit(type, rotation, ry, rx)) {
                continue;
            }

            int8_t board_x = x + rx;
            int8_t board_y = y + ry;

            if (board_x < 0 || board_x >= GRID_WIDTH || board_y < 0 || board_y >= GRID_HEIGHT) {
                return false;
            }
            if (board[board_y][board_x] != 0) {
                return false;
            }
        }
    }
    return true;
}

// zatwierdza aktywny klocek i zapisuje go na planszy
static void place_piece(void)
{
    for (uint8_t ry = 0; ry < 4; ++ry) {
        for (uint8_t rx = 0; rx < 4; ++rx) {
            if (!piece_bit(current_type, current_rot, ry, rx)) {
                continue;
            }
            int8_t bx = current_x + rx;
            int8_t by = current_y + ry;
            if (by >= 0 && by < GRID_HEIGHT && bx >= 0 && bx < GRID_WIDTH) {
                board[by][bx] = current_type + 1;
            }
        }
    }
}

static uint8_t clear_lines(void)
{
    uint8_t cleared = 0;

    for (int8_t ry = GRID_HEIGHT - 1; ry >= 0; --ry) {
        bool full = true;
        for (uint8_t rx = 0; rx < GRID_WIDTH; ++rx) {
            if (board[ry][rx] == 0) {
                full = false;
                break;
            }
        }

        if (full) {
            ++cleared;
        } else if (cleared > 0) {
            for (uint8_t rx = 0; rx < GRID_WIDTH; ++rx) {
                board[ry + cleared][rx] = board[ry][rx];
            }
        }
    }

    for (uint8_t ry = 0; ry < cleared; ++ry) {
        for (uint8_t rx = 0; rx < GRID_WIDTH; ++rx) {
            board[ry][rx] = 0;
        }
    }

    lines_cleared += cleared;
    return cleared;
}

// rysuje ramkę i tło planszy, musi być przed rysowaniem klocków
static void draw_board_bg(void)
{
    screen_fill_rect(BOARD_X - 1, BOARD_Y - 1, GRID_WIDTH * CELL_SIZE + 2, GRID_HEIGHT * CELL_SIZE + 2, COLOR_GRAY);
    screen_fill_rect(BOARD_X, BOARD_Y, GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE, COLOR_BLACK);
}

// rysuje pojedynczy kwadrat planszy
static void draw_cell(uint8_t x, uint8_t y, uint16_t color)
{
    screen_fill_rect(BOARD_X + x * CELL_SIZE, BOARD_Y + y * CELL_SIZE, CELL_SIZE, CELL_SIZE, color);
}

static void draw_preview_cell(uint8_t x, uint8_t y, uint16_t color)
{
    screen_fill_rect(STATUS_X + x * PREVIEW_CELL, BOARD_Y + y * PREVIEW_CELL, PREVIEW_CELL, PREVIEW_CELL, color);
}

static void draw_piece_state(int8_t x, int8_t y, uint8_t type, uint8_t rot, uint16_t color)
{
    for (uint8_t ry = 0; ry < 4; ++ry) {
        for (uint8_t rx = 0; rx < 4; ++rx) {
            if (!piece_bit(type, rot, ry, rx)) {
                continue;
            }
            int8_t bx = x + rx;
            int8_t by = y + ry;
            if (bx >= 0 && bx < GRID_WIDTH && by >= 0 && by < GRID_HEIGHT) {
                draw_cell((uint8_t)bx, (uint8_t)by, color);
            }
        }
    }
}

static void clear_piece_state(int8_t x, int8_t y, uint8_t type, uint8_t rot)
{
    for (uint8_t ry = 0; ry < 4; ++ry) {
        for (uint8_t rx = 0; rx < 4; ++rx) {
            if (!piece_bit(type, rot, ry, rx)) {
                continue;
            }
            int8_t bx = x + rx;
            int8_t by = y + ry;
            if (bx >= 0 && bx < GRID_WIDTH && by >= 0 && by < GRID_HEIGHT) {
                draw_cell((uint8_t)bx, (uint8_t)by, piece_colors[board[by][bx]]);
            }
        }
    }
}

static void draw_board_cells(void)
{
    for (uint8_t ry = 0; ry < GRID_HEIGHT; ++ry) {
        for (uint8_t rx = 0; rx < GRID_WIDTH; ++rx) {
            draw_cell(rx, ry, piece_colors[board[ry][rx]]);
        }
    }
}

static void draw_status_bg(void)
{
    screen_fill_rect(STATUS_X - 2, BOARD_Y - 2, 18, GRID_HEIGHT * CELL_SIZE + 4, COLOR_BLACK);
}

// rysuje jeden bit wyniku w pionie, LSB na dole
static void draw_score_bit(uint8_t bit, bool on)
{
    uint8_t x = STATUS_X;
    uint8_t y = (uint8_t)(BOARD_Y + GRID_HEIGHT * CELL_SIZE - CELL_SIZE - bit * CELL_SIZE);
    screen_fill_rect(x, y, CELL_SIZE, CELL_SIZE, on ? COLOR_WHITE : COLOR_BLACK);
}

// rysuje cały wynik binarnie tylko raz albo po zmianie
static void draw_score_display(void)
{
    for (uint8_t bit = 0; bit < 8; ++bit) {
        bool on = (lines_cleared >> bit) & 1u;
        draw_score_bit(bit, on);
    }
    prev_lines_cleared = lines_cleared;
    score_drawn = true;
}

static void update_score_display(uint16_t old_score, uint16_t new_score)
{
    for (uint8_t bit = 0; bit < 8; ++bit) {
        bool old_on = (old_score >> bit) & 1u;
        bool new_on = (new_score >> bit) & 1u;
        if (old_on != new_on) {
            draw_score_bit(bit, new_on);
        }
    }
    prev_lines_cleared = new_score;
}

// rysuje tylko nowy podgląd następnego klocka
static void draw_next_preview(bool force)
{
    if (!force && preview_drawn && next_type == prev_next_type) {
        return;
    }

    screen_fill_rect(STATUS_X, BOARD_Y, 4 * PREVIEW_CELL, 4 * PREVIEW_CELL, COLOR_BLACK);
    for (uint8_t ry = 0; ry < 4; ++ry) {
        for (uint8_t rx = 0; rx < 4; ++rx) {
            uint16_t color = piece_bit(next_type, 0, ry, rx) ? piece_colors[next_type + 1] : COLOR_BLACK;
            draw_preview_cell(rx, ry + 1, color);
        }
    }

    preview_drawn = true;
    prev_next_type = next_type;
}

static void render(void)
{
    if (board_redraw_needed) {
        draw_board_bg();
        draw_board_cells();
        board_redraw_needed = false;
        moving_piece_drawn = false;
    }

    if (moving_piece_drawn && (current_type != prev_piece_type || current_x != prev_piece_x || current_y != prev_piece_y || current_rot != prev_piece_rot)) {
        clear_piece_state(prev_piece_x, prev_piece_y, prev_piece_type, prev_piece_rot);
    }

    draw_piece_state(current_x, current_y, current_type, current_rot, piece_colors[current_type + 1]);

    prev_piece_x = current_x;
    prev_piece_y = current_y;
    prev_piece_rot = current_rot;
    prev_piece_type = current_type;
    moving_piece_drawn = true;
}

// startuje nowy klocek na górze planszy
static void spawn_piece(uint8_t type)
{
    current_type = type;
    current_rot = 0;
    current_x = 3;
    current_y = 0;
    if (!can_place(current_type, current_rot, current_x, current_y)) {
        game_over_flag = true;
    }
}

// obracanie aktualnego klocka
static void rotate_piece(void)
{
    uint8_t candidate = (current_rot + 1) & 3;
    if (can_place(current_type, candidate, current_x, current_y)) {
        current_rot = candidate;
        return;
    }
    if (can_place(current_type, candidate, current_x - 1, current_y)) {
        current_x -= 1;
        current_rot = candidate;
        return;
    }
    if (can_place(current_type, candidate, current_x + 1, current_y)) {
        current_x += 1;
        current_rot = candidate;
    }
}

static void move_piece(int8_t dx)
{
    int8_t target_x = current_x + dx;
    if (can_place(current_type, current_rot, target_x, current_y)) {
        current_x = target_x;
    }
}
// rzut klockiem w dół
static bool drop_piece(void)
{
    int8_t target_y = current_y + 1;
    if (can_place(current_type, current_rot, current_x, target_y)) {
        current_y = target_y;
        return true;
    }

    place_piece();
    board_redraw_needed = true;
    uint16_t old_score = prev_lines_cleared;
    uint8_t cleared = clear_lines();
    if (cleared > 0) {
        update_score_display(old_score, lines_cleared);
    }

    uint8_t spawn_type = next_type;
    next_type = (uint8_t)(random16() % 7);
    spawn_piece(spawn_type);
    draw_next_preview(true);
    moving_piece_drawn = false;
    return false;
}

static void hard_drop(void)
{
    while (can_place(current_type, current_rot, current_x, current_y + 1)) {
        current_y += 1;
    }
    place_piece();
    board_redraw_needed = true;
    uint16_t old_score = prev_lines_cleared;
    uint8_t cleared = clear_lines();
    if (cleared > 0) {
        update_score_display(old_score, lines_cleared);
    }

    uint8_t spawn_type = next_type;
    next_type = (uint8_t)(random16() % 7);
    spawn_piece(spawn_type);
    draw_next_preview(true);
    moving_piece_drawn = false;
}

// resetuje całą grę i ustawia pierwszy klocek
static void reset_game(void)
{
    for (uint8_t ry = 0; ry < GRID_HEIGHT; ++ry) {
        for (uint8_t rx = 0; rx < GRID_WIDTH; ++rx) {
            board[ry][rx] = 0;
        }
    }
    lines_cleared = 0;
    gravity_timer = 0;
    horiz_timer = 0;
    soft_timer = 0;
    prev_held = 0;
    game_over_flag = false;
    board_redraw_needed = true;
    next_type = (uint8_t)(random16() % 7);
    spawn_piece((uint8_t)(random16() % 7));
    draw_status_bg();
    draw_score_display();
    draw_next_preview(true);
    moving_piece_drawn = false;
}

// czeka aż użytkownik naciśnie przycisk A żeby zacząć
static void wait_for_start(void)
{
    screen_fill(COLOR_GREEN);
    while (1) {
        input_update();
        if (input_get_pressed() & BTN_A) {
            break;
        }
        _delay_ms(20);
    }
}

// obsługa przycisków i ruchów klocka
static void process_input(void)
{
    input_update();
    uint8_t held = input_get_held();
    uint8_t pressed = input_get_pressed();

    if (pressed & BTN_A) {
        rotate_piece();
    }
    if (pressed & BTN_B) {
        hard_drop();
    }

    if (held & BTN_LEFT) {
        if (!(prev_held & BTN_LEFT) || horiz_timer == 0) {
            move_piece(-1);
            horiz_timer = HORIZ_REPEAT;
        }
    } else if (held & BTN_RIGHT) {
        if (!(prev_held & BTN_RIGHT) || horiz_timer == 0) {
            move_piece(1);
            horiz_timer = HORIZ_REPEAT;
        }
    } else {
        horiz_timer = 0;
    }

    if (held & BTN_DOWN) {
        if (!(prev_held & BTN_DOWN) || soft_timer == 0) {
            drop_piece();
            soft_timer = SOFT_REPEAT;
        }
    } else {
        soft_timer = 0;
    }

    if (horiz_timer > 0) {
        --horiz_timer;
    }
    if (soft_timer > 0) {
        --soft_timer;
    }

    prev_held = held;
}

int main(void)
{
    screen_init();
    input_init();

    wait_for_start();
    reset_game();
    render();

    while (1) {
        process_input();

        if (game_over_flag) {
            screen_fill(COLOR_RED);
            _delay_ms(800);
            wait_for_start();
            reset_game();
            render();
            continue;
        }

        if (++gravity_timer >= GRAVITY_STEPS) {
            gravity_timer = 0;
            drop_piece();
        }

        render();
        _delay_ms(TICK_MS);
    }

    return 0;
}
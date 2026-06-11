#include <stdbool.h>
#include <stdint.h>
#include <util/delay.h>
#include "screen.h"
#include "input.h"

#define BOX_SIZE      8 // rozmiar jednego pola
// wielkość pola gry
#define GRID_WIDTH    (SCREEN_WIDTH / BOX_SIZE) 
#define GRID_HEIGHT   (SCREEN_HEIGHT / BOX_SIZE)
// maksymalna długość węża
#define SNAKE_MAX_LEN (GRID_WIDTH * GRID_HEIGHT)
// czas trwania tiku
#define TICK_MS       200

typedef struct {
    uint8_t x;
    uint8_t y;
} Punkt;

static Punkt snake[SNAKE_MAX_LEN]; // inicjalizacja tablicy weża
static uint16_t snake_length;
static uint16_t snake_head;
static uint16_t snake_tail;
static int8_t direction_x;
static int8_t direction_y;
static Punkt apple;
static uint16_t random_state = 0xBEEF;

// ngl to z czataGPT
static inline uint16_t random16()
{
    random_state = (uint16_t)(random_state * 2053u + 13849u);
    return random_state;
}

// porównuje dwa punkty, podstawa do sprawdzania kolizji i jedzenia jabłka
static inline bool points_equal(const Punkt *a, const Punkt *b)
{
    return a->x == b->x && a->y == b->y;
}

// sprawdza, czy dany punkt jest zajęty przez węża, opcjonalnie ignorując ogon (przy ruchu węża)
static bool snake_contains(const Punkt *p, bool ignore_tail)
{
    for (uint16_t i = 0; i < snake_length; ++i) {
        uint16_t idx = (snake_tail + i) % SNAKE_MAX_LEN;
        if (points_equal(&snake[idx], p)) {
            if (ignore_tail && idx == snake_tail) {
                continue;
            }
            return true;
        }
    }
    return false;
}

static Punkt random_cell(void)
{
    Punkt cell;
    cell.x = (uint8_t)(random16() % GRID_WIDTH);
    cell.y = (uint8_t)(random16() % GRID_HEIGHT);
    return cell;
}

// jabłko jest generowane losowo, ale w miejscu, gdzie nie ma węża
static Punkt spawn_apple(void)
{
    Punkt p;
    do {
        p = random_cell();
    } while (snake_contains(&p, false));
    return p;
}

static void draw_cell(const Punkt *p, uint16_t color)
{
    screen_fill_rect((uint8_t)(p->x * BOX_SIZE), (uint8_t)(p->y * BOX_SIZE), BOX_SIZE, BOX_SIZE, color);
}

static void game_over(void)
{
    screen_fill(COLOR_RED);
    //TODO: dodać tekst jak API będzie obsługiwać
    _delay_ms(800);
    screen_fill(COLOR_BLACK);
}

static void process_direction_input(void)
{
    input_update();
    uint8_t held = input_get_held();

    if ((held & BTN_UP) && direction_y == 0) {
        direction_x = 0;
        direction_y = -1;
    } else if ((held & BTN_DOWN) && direction_y == 0) {
        direction_x = 0;
        direction_y = 1;
    } else if ((held & BTN_LEFT) && direction_x == 0) {
        direction_x = -1;
        direction_y = 0;
    } else if ((held & BTN_RIGHT) && direction_x == 0) {
        direction_x = 1;
        direction_y = 0;
    }
}

static void delay_tick(void)
{
    const uint8_t step = 10; // dzielimy tick na mniejsze kroki, żeby sprawdzać input i utrzymać responsywność
    uint8_t elapsed = 0;

    while (elapsed < TICK_MS) {
        _delay_ms(step);
        elapsed += step;
        process_direction_input(); //sprawdzanie inputu podczas czekania, żeby gra była responsywna
    }
}


static void init_game(void)
{
    screen_fill(COLOR_BLACK);

    snake_length = 4;
    snake_tail = 0;
    snake_head = snake_length - 1;
    direction_x = 0;
    direction_y = 0;

    uint8_t start_x = GRID_WIDTH / 2;
    uint8_t start_y = GRID_HEIGHT / 2;

    for (uint8_t i = 0; i < snake_length; ++i) {
        snake[i].x = start_x - (snake_length - 1 - i); // wąż startuje poziomo, głowa po prawej
        snake[i].y = start_y;
        draw_cell(&snake[i], COLOR_WHITE);
    }

    apple = spawn_apple();
    draw_cell(&apple, COLOR_RED);
}

int main(void)
{
    screen_init();
    input_init();
    init_game();

    while (1)
    {
        process_direction_input();

        if (direction_x == 0 && direction_y == 0) {
            delay_tick();
            continue;
        }

        Punkt current_head = snake[snake_head];
        int16_t next_x = (int16_t)current_head.x + direction_x;
        int16_t next_y = (int16_t)current_head.y + direction_y;

        if (next_x < 0 || next_x >= GRID_WIDTH || next_y < 0 || next_y >= GRID_HEIGHT) {
            game_over();
            init_game();
            continue;
        }

        // obliczanie nowej głowy węża, sprawdzanie kolizji z samym sobą i jedzenia jabłka
        Punkt new_head = {(uint8_t)next_x, (uint8_t)next_y};
        bool eating = points_equal(&new_head, &apple);
        bool collision = snake_contains(&new_head, !eating);

        if (collision) {
            game_over();
            init_game();
            continue;
        }

        snake_head = (snake_head + 1) % SNAKE_MAX_LEN;
        snake[snake_head] = new_head;

        // jeśli nie zjadamy jabłka, to usuwamy ogon , w przeciwnym wypadku wydłużamy węża i generujemy nowe jabłko
        if (!eating) {
            draw_cell(&snake[snake_tail], COLOR_BLACK);
            snake_tail = (snake_tail + 1) % SNAKE_MAX_LEN;
        } else {
            if (snake_length < SNAKE_MAX_LEN) {
                ++snake_length;
            }
            apple = spawn_apple();
            draw_cell(&apple, COLOR_RED);
        }

        draw_cell(&new_head, COLOR_WHITE);
        delay_tick();
    }

    return 0;
}
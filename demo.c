#include <util/delay.h>
#include "screen.h"

// Prosty generator pseudolosowy oparty na algorytmie xorshift32.
static uint32_t random_state = 0xA5A5A5A5;
static uint32_t random_next(void)
{
    uint32_t x = random_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    random_state = x;
    return x;
}

// Zwraca losowy kolor.
static uint16_t random_color(void)
{
    static const uint16_t palette[] = {
        COLOR_RED, COLOR_BLUE, COLOR_GREEN,
        COLOR_CYAN, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE
    };
    return palette[random_next() % (sizeof(palette) / sizeof(palette[0]))];
}

// Demo 1: pełne wypełnienie ekranu trzema kolorami po kolei.
static void demo1(void)
{
    const uint16_t sequence[] = {COLOR_RED, COLOR_BLUE, COLOR_GREEN};
        for (uint8_t i = 0; i < sizeof(sequence) / sizeof(sequence[0]); ++i) {
            screen_fill(sequence[i]); // wypełnij ekran jednym kolorem
            _delay_ms(250); // odczekaj chwilę, aby widać zmianę
        }
}

// Demo 2: losowe prostokąty
static void demo2(void)
{
    screen_fill(COLOR_BLACK);

    for (uint16_t i = 0; i < 1000; ++i) {
        uint32_t value = random_next(); // LETS GO GAMBLING!
        uint8_t width = (value & 0x1F) + 8; // szerokość 8..39
        uint8_t height = ((value >> 5) & 0x1F) + 8; // wysokość 8..39
        uint8_t x = (value >> 10) % (SCREEN_WIDTH - width + 1); // losowe X
        uint8_t y = (value >> 18) % (SCREEN_HEIGHT - height + 1); // losowe Y
        uint16_t color = random_color(); // losowy kolor z palety

        screen_fill_rect(x, y, width, height, color); // rysuj prostokąt
    }
}

// Demo 3: trzy prostokąty odbijające się od krawędzi ekranu.
static void demo3(void)
{
    // Pozycje, prędkości i kolory trzech prostokątów.
    int16_t rect_x[3] = {0, 40, 80};
    int16_t rect_y[3] = {0, 30, 70};
    signed char dx[3] = {1, 2, 1}; // prędkości ruchu w poziomie
    signed char dy[3] = {1, 1, 2}; // prędkości ruchu w pionie
    const uint16_t rect_color[3] = {COLOR_BLUE, COLOR_RED, COLOR_GREEN};
    const uint8_t rect_w[3] = {24, 20, 18};
    const uint8_t rect_h[3] = {16, 18, 22};

    screen_fill(COLOR_BLACK);

    for (uint16_t frame = 0; frame < 500; ++frame) {
        for (uint8_t i = 0; i < 3; ++i) {
            screen_fill_rect((uint8_t)rect_x[i], (uint8_t)rect_y[i], rect_w[i], rect_h[i], COLOR_BLACK); // skasuj poprzednią pozycję

            int16_t next_x = rect_x[i] + dx[i]; // policz nową pozycję
            int16_t next_y = rect_y[i] + dy[i];

            if (next_x < 0 || next_x > SCREEN_WIDTH - rect_w[i]) {
                dx[i] = -dx[i]; // odbicie od lewej/prawej krawędzi
                next_x = rect_x[i] + dx[i];
            }
            if (next_y < 0 || next_y > SCREEN_HEIGHT - rect_h[i]) {
                dy[i] = -dy[i]; // odbicie od góry/dołu
                next_y = rect_y[i] + dy[i];
            }

            rect_x[i] = next_x;
            rect_y[i] = next_y;
            screen_fill_rect((uint8_t)rect_x[i], (uint8_t)rect_y[i], rect_w[i], rect_h[i], rect_color[i]); // narysuj nową pozycję
        }
    }
}

// Demo 4: pojedynczy prostokąt o zmiennym rytmie wykrywania linii, rysowany z pominięciem.
static void demo4(void)
{
    int16_t rect_x = 20;
    int16_t rect_y = 30;
    signed char dx = 2;
    signed char dy = 2;
    const uint8_t rect_w = 24;
    const uint8_t rect_h = 32;
    uint8_t current_skip = 0;

    screen_fill(COLOR_BLACK); // czarne tło

    for (uint16_t frame = 0; frame < 1000; ++frame) {
        uint8_t skip = 0 + (frame / 200); // zmienia tryb pomijania co 200 klatek

        screen_fill_rect_skip((uint8_t)rect_x, (uint8_t)rect_y, rect_w, rect_h, COLOR_BLACK, current_skip); // skasuj stary prostokąt

        int16_t next_x = rect_x + dx;
        int16_t next_y = rect_y + dy;

        if (next_x < 0 || next_x > SCREEN_WIDTH - rect_w) {
            dx = -dx; // odbicie w poziomie
            next_x = rect_x + dx;
        }
        if (next_y < 0 || next_y > SCREEN_HEIGHT - rect_h) {
            dy = -dy; // odbicie w pionie
            next_y = rect_y + dy;
        }

        rect_x = next_x;
        rect_y = next_y;

        screen_fill_rect_skip((uint8_t)rect_x, (uint8_t)rect_y, rect_w, rect_h, COLOR_WHITE, skip); // narysuj prostokąt z nowym skip
        current_skip = skip;
    }
}

static void draw_corner_edge_lines(int16_t x0, int16_t y0, uint16_t color)
{
    const uint8_t steps = 9;

    if (y0 == 0) {
        for (uint8_t i = 0; i <= steps; ++i) {
            int16_t x = (int16_t)((uint32_t)i * (SCREEN_WIDTH - 1) / steps);
            screen_draw_line(x0, y0, x, SCREEN_HEIGHT - 1, color); // rysuj linie do dolnej krawędzi
        }
    } else {
        for (uint8_t i = 0; i <= steps; ++i) {
            int16_t x = (int16_t)((uint32_t)i * (SCREEN_WIDTH - 1) / steps);
            screen_draw_line(x0, y0, x, 0, color); // rysuj linie do górnej krawędzi
        }
    }

    if (x0 == 0) {
        for (uint8_t i = 1; i < steps; ++i) {
            int16_t y = (int16_t)((uint32_t)i * (SCREEN_HEIGHT - 1) / steps);
            screen_draw_line(x0, y0, SCREEN_WIDTH - 1, y, color); // rysuj linie do prawej krawędzi
        }
    } else {
        for (uint8_t i = 1; i < steps; ++i) {
            int16_t y = (int16_t)((uint32_t)i * (SCREEN_HEIGHT - 1) / steps);
            screen_draw_line(x0, y0, 0, y, color); // rysuj linie do lewej krawędzi
        }
    }
}

// Demo 5: Linie
static void demo5(void)
{
    const int16_t corners[4][2] = {
        {0, 0},
        {SCREEN_WIDTH - 1, 0},
        {0, SCREEN_HEIGHT - 1},
        {SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1}
    };

    for (uint8_t pass = 0; pass < 3; ++pass) {
        screen_fill(COLOR_BLACK); // czyść ekran przed każdym przebiegiem

        for (uint8_t corner = 0; corner < 4; ++corner) {
            int16_t x0 = corners[corner][0];
            int16_t y0 = corners[corner][1];

            draw_corner_edge_lines(x0, y0, COLOR_WHITE); // narysuj linie z rogu
            draw_corner_edge_lines(x0, y0, COLOR_BLACK); // usuń linie tym samym kolorem tła
        }
    }
}

// Demo 6: Pong
static void demo6(void)
{
    const uint8_t paddle_w = 32;
    const uint8_t paddle_h = 4;
    const uint8_t ball_size = 4;
    int16_t top_x = (SCREEN_WIDTH - paddle_w) / 2;
    int16_t bottom_x = (SCREEN_WIDTH - paddle_w) / 2;
    int16_t ball_x = SCREEN_WIDTH / 2 - ball_size / 2;
    int16_t ball_y = SCREEN_HEIGHT / 2 - ball_size / 2;
    signed char ball_dx = 1;
    signed char ball_dy = 2;
    const uint16_t color = COLOR_WHITE;

    screen_fill(COLOR_BLACK);

    for (uint16_t frame = 0; frame < 1000; ++frame) {
        screen_fill_rect(top_x, 2, paddle_w, paddle_h, COLOR_BLACK); // skasuj górną paletkę
        screen_fill_rect(bottom_x, SCREEN_HEIGHT - 2 - paddle_h, paddle_w, paddle_h, COLOR_BLACK); // skasuj dolną paletkę
        screen_fill_rect((uint8_t)ball_x, (uint8_t)ball_y, ball_size, ball_size, COLOR_BLACK); // skasuj piłkę
        screen_draw_hline(0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, COLOR_GRAY); // rysuj środkową linię

        int16_t next_x = ball_x + ball_dx; // nowa pozycja piłki w poziomie
        int16_t next_y = ball_y + ball_dy; // nowa pozycja piłki w pionie

        if (next_x < 0 || next_x > SCREEN_WIDTH - ball_size) {
            ball_dx = -ball_dx; // odbicie od lewej/prawej krawędzi
            next_x = ball_x + ball_dx;
        }

        if (next_y <= 2 + paddle_h) {
            if (next_x + ball_size > top_x && next_x < top_x + paddle_w) {
                ball_dy = -ball_dy; // odbicie od górnej paletki
                next_y = ball_y + ball_dy;
            } else {
                ball_dy = -ball_dy; // odbicie od góry, gdy nie trafi w paletkę
                next_y = ball_y + ball_dy;
            }
        }
        if (next_y >= SCREEN_HEIGHT - 2 - paddle_h - ball_size) {
            if (next_x + ball_size > bottom_x && next_x < bottom_x + paddle_w) {
                ball_dy = -ball_dy; // odbicie od dolnej paletki
                next_y = ball_y + ball_dy;
            } else {
                ball_dy = -ball_dy; // odbicie od dołu, gdy nie trafi w paletkę
                next_y = ball_y + ball_dy;
            }
        }

        if (top_x + paddle_w / 2 < next_x + ball_size / 2) {
            top_x += 1; // przesuwaj górną paletkę w kierunku piłki
        } else if (top_x + paddle_w / 2 > next_x + ball_size / 2) {
            top_x -= 1;
        }
        if (bottom_x + paddle_w / 2 < next_x + ball_size / 2) {
            bottom_x += 1; // przesuwaj dolną paletkę w kierunku piłki
        } else if (bottom_x + paddle_w / 2 > next_x + ball_size / 2) {
            bottom_x -= 1;
        }

        if (top_x < 0) {
            top_x = 0; // ograniczenie lewego krańca
        } else if (top_x > SCREEN_WIDTH - paddle_w) {
            top_x = SCREEN_WIDTH - paddle_w; // ograniczenie prawego krańca
        }
        if (bottom_x < 0) {
            bottom_x = 0;
        } else if (bottom_x > SCREEN_WIDTH - paddle_w) {
            bottom_x = SCREEN_WIDTH - paddle_w;
        }

        ball_x = next_x;
        ball_y = next_y;

        screen_fill_rect(top_x, 2, paddle_w, paddle_h, color); // narysuj górną paletkę
        screen_fill_rect(bottom_x, SCREEN_HEIGHT - 2 - paddle_h, paddle_w, paddle_h, color); // narysuj dolną paletkę
        screen_fill_rect((uint8_t)ball_x, (uint8_t)ball_y, ball_size, ball_size, color); // narysuj piłkę
        _delay_ms(15);
    }
}

// Demo 7: Kostka
static void demo7(void)
{
    static const int32_t points[8][3] = {
        {-20, -20, -20}, {20, -20, -20}, {20, 20, -20}, {-20, 20, -20},
        {-20, -20,  20}, {20, -20,  20}, {20, 20,  20}, {-20, 20,  20}
    };
    const uint8_t edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };
    // tablica sinusów (skalowanie przez 256)
    static const int16_t sin_table[64] = {
         0, 25, 50, 74, 98,121,142,162,181,198,213,226,236,245,251,255,
       256,255,251,245,236,226,213,198,181,162,142,121, 98, 74, 50, 25,
         0,-25,-50,-74,-98,-121,-142,-162,-181,-198,-213,-226,-236,-245,-251,-255,
      -256,-255,-251,-245,-236,-226,-213,-198,-181,-162,-142,-121,-98,-74,-50,-25
    };
    const int32_t center_x = SCREEN_WIDTH / 2;
    const int32_t center_y = SCREEN_HEIGHT / 2;
    uint32_t angle = 0;
    int32_t prev_projected[8][2] = {{0}};

    screen_fill(COLOR_BLACK);

    for (uint16_t frame = 0; frame < 300; ++frame) {
        const int32_t sin_a = sin_table[angle & 63];
        const int32_t cos_a = sin_table[(angle + 16) & 63];
        const int32_t sin_b = sin_table[(angle + 8) & 63];
        const int32_t cos_b = sin_table[(angle + 24) & 63];

        int32_t projected[8][2];

        for (uint8_t i = 0; i < 8; ++i) {
            int32_t x = points[i][0];
            int32_t y = points[i][1];
            int32_t z = points[i][2];

            int32_t x1 = (x * cos_a - z * sin_a) >> 8; // rotacja wokół Y
            int32_t z1 = (x * sin_a + z * cos_a) >> 8;
            int32_t y1 = (y * cos_b - z1 * sin_b) >> 8; // rotacja wokół X
            int32_t z2 = (y * sin_b + z1 * cos_b) >> 8;

            projected[i][0] = center_x + x1 + (z2 >> 2);
            projected[i][1] = center_y + y1 - (z2 >> 2);
        }

        if (frame > 0) {
            for (uint8_t e = 0; e < 12; ++e) {
                uint8_t a = edges[e][0];
                uint8_t b = edges[e][1];
                screen_draw_line((int16_t)prev_projected[a][0], (int16_t)prev_projected[a][1],
                                 (int16_t)prev_projected[b][0], (int16_t)prev_projected[b][1], COLOR_BLACK);
            }
        }

        for (uint8_t e = 0; e < 12; ++e) {
            uint8_t a = edges[e][0];
            uint8_t b = edges[e][1];
            screen_draw_line((int16_t)projected[a][0], (int16_t)projected[a][1],
                             (int16_t)projected[b][0], (int16_t)projected[b][1], COLOR_WHITE);
        }

        for (uint8_t i = 0; i < 8; ++i) {
            prev_projected[i][0] = projected[i][0];
            prev_projected[i][1] = projected[i][1];
        }

        angle += 1;
    }
}

int main(void)
{
    screen_init();

    while (1) {
        demo1();
        demo2();
        demo3();
        demo4();
        demo5();
        demo6();
        demo7();
    }
}

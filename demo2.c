#include <util/delay.h>
#include "screen.h"
#include "input.h"

#define BOX_SIZE 12
#define MOVE_SPEED 4

int main(void)
{
    screen_init();
    input_init();

    screen_fill(COLOR_BLACK);

    // środek ekranu
    int16_t x = (SCREEN_WIDTH - BOX_SIZE) / 2;
    int16_t y = (SCREEN_HEIGHT - BOX_SIZE) / 2;

    uint16_t current_color = COLOR_RED;

    screen_fill_rect((uint8_t)x, (uint8_t)y, BOX_SIZE, BOX_SIZE, current_color);

    while (1)
    {
        input_update();
        
        uint8_t held = input_get_held();
        uint8_t pressed = input_get_pressed();

        // ify osobno by nie bolokowały siebe wzajemnie (przykład)
        if (pressed & BTN_A) {
            current_color = COLOR_GREEN;
            screen_fill_rect((uint8_t)x, (uint8_t)y, BOX_SIZE, BOX_SIZE, current_color);
        }
        if (pressed & BTN_B) {
            current_color = COLOR_CYAN;
            screen_fill_rect((uint8_t)x, (uint8_t)y, BOX_SIZE, BOX_SIZE, current_color);
        }

        // wektor ruchu
        int8_t dx = 0;
        int8_t dy = 0;

        // dodawanie do wektora ruchu
        if (held & BTN_UP)    dy -= MOVE_SPEED;
        if (held & BTN_DOWN)  dy += MOVE_SPEED;
        if (held & BTN_LEFT)  dx -= MOVE_SPEED;
        if (held & BTN_RIGHT) dx += MOVE_SPEED;

        // tylko rysuj jeżeli jest ruch
        if (dx != 0 || dy != 0) 
        {
            // usuń stary prostokąt
            screen_fill_rect((uint8_t)x, (uint8_t)y, BOX_SIZE, BOX_SIZE, COLOR_BLACK);

            // przesuń
            x += dx;
            y += dy;

            // ograniczenie do ekranu
            if (x < 0) x = 0;
            if (x > SCREEN_WIDTH - BOX_SIZE) x = SCREEN_WIDTH - BOX_SIZE;
            if (y < 0) y = 0;
            if (y > SCREEN_HEIGHT - BOX_SIZE) y = SCREEN_HEIGHT - BOX_SIZE;

            // narysuj nowy prostokąt
            screen_fill_rect((uint8_t)x, (uint8_t)y, BOX_SIZE, BOX_SIZE, current_color);
        }

        _delay_ms(25);
    }

    return 0;
}
#include "input.h"
#include <avr/io.h>

//przypisanie przycisków
#define PIN_UP    PD2 // Pin D2
#define PIN_DOWN  PD3 // Pin D3
#define PIN_LEFT  PD4 // Pin D4
#define PIN_RIGHT PD5 // Pin D5
#define PIN_A     PD6 // Pin D6
#define PIN_B     PD7 // Pin D7

static uint8_t current_held = 0;
static uint8_t just_pressed = 0;

void input_init(void)
{
    // ustaw piny jako wejscia
    DDRD &= ~((1 << PIN_UP) | (1 << PIN_DOWN) | (1 << PIN_LEFT) | 
              (1 << PIN_RIGHT) | (1 << PIN_A) | (1 << PIN_B));
    
    // ustaw pull-upy
    PORTD |= (1 << PIN_UP) | (1 << PIN_DOWN) | (1 << PIN_LEFT) | 
             (1 << PIN_RIGHT) | (1 << PIN_A) | (1 << PIN_B);
}

void input_update(void)
{
    static uint8_t last_raw = 0; // ostatni odczyt
    static uint8_t validated_state = 0; // ostatni stabilny stan przycisków
    
    uint8_t raw = 0;

    // Odczyt wartości wejściowych (logika odwrócona: pin zwiera do GND po naciśnięciu)
    if (!(PIND & (1 << PIN_UP)))    raw |= BTN_UP;
    if (!(PIND & (1 << PIN_DOWN)))  raw |= BTN_DOWN;
    if (!(PIND & (1 << PIN_LEFT)))  raw |= BTN_LEFT;
    if (!(PIND & (1 << PIN_RIGHT))) raw |= BTN_RIGHT;
    if (!(PIND & (1 << PIN_A)))     raw |= BTN_A;
    if (!(PIND & (1 << PIN_B)))     raw |= BTN_B;

    // debounce
    if (raw == last_raw) 
    {
        uint8_t previous_validated = validated_state;
        validated_state = raw;
        
        // wyznaczaj przyciski które zostały właśnie naciśnięte (wykrywa zbocze)
        just_pressed = (validated_state ^ previous_validated) & validated_state;
        current_held = validated_state;
    } 
    else 
    {
        // Trwa przejście sygnału; odrzuć zmiany, aż stan będzie stabilny
        just_pressed = 0;
    }
    
    last_raw = raw;
}

uint8_t input_get_held(void)
{
    return current_held;
}

uint8_t input_get_pressed(void)
{
    return just_pressed;
}
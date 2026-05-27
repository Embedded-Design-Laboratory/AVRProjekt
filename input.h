#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

// Bitmasks for all 6 buttons
#define BTN_UP    (1 << 0)
#define BTN_DOWN  (1 << 1)
#define BTN_LEFT  (1 << 2)
#define BTN_RIGHT (1 << 3)
#define BTN_A     (1 << 4)
#define BTN_B     (1 << 5)

/**
 * Initializes pins D2 through D7 as inputs with internal pull-ups.
 */
void input_init(void);

/**
 * Reads physical pins and evaluates stable state using non-blocking debouncing.
 * This must be executed once per main loop iteration.
 */
void input_update(void);

/**
 * Retrieves buttons that are actively being held down.
 * Ideal for smooth continuous tasks like directional character movement.
 */
uint8_t input_get_held(void);

/**
 * Retrieves buttons that transitioned from unpressed to pressed in this specific frame.
 * Ideal for single trigger actions like opening menus or changing colors.
 */
uint8_t input_get_pressed(void);

#endif // INPUT_H
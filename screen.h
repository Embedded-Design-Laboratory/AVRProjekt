/**
 * @file screen.h
 * @brief API dla sterownika wyświetlacza ST7735 na ATmega328P.
 */

#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

/** Szerokość ekranu w pikselach. */
#define SCREEN_WIDTH  128
/** Wysokość ekranu w pikselach. */
#define SCREEN_HEIGHT 160

/** Definicje kolorów w formacie RGB565. */
#define COLOR_BLACK   0x0000
#define COLOR_BLUE    0x001F
#define COLOR_GREEN   0x07E0
#define COLOR_CYAN    0x07FF
#define COLOR_RED     0xF800
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW  0xFFE0
#define COLOR_WHITE   0xFFFF
#define COLOR_GRAY    0x8410

/**
 * Inicjalizuje sprzęt wyświetlacza i uruchamia kontroler ST7735.
 * Konfiguruje interfejs SPI, resetuje wyświetlacz i wysyła sekwencję inicjalizacyjną.
 */
void screen_init(void);

/**
 * Resetuje wyświetlacz przy pomocy pinu reset.
 */
void screen_reset(void);

/**
 * Wypełnia cały ekran pojedynczym kolorem RGB565.
 *
 * @param color Wartość koloru RGB565.
 */
void screen_fill(uint16_t color);

/**
 * Wypełnia obszar prostokąta pojedynczym kolorem RGB565.
 *
 * @param x      Pozycja X lewego górnego narożnika.
 * @param y      Pozycja Y lewego górnego narożnika.
 * @param width  Szerokość prostokąta w pikselach.
 * @param height Wysokość prostokąta w pikselach.
 * @param color  Wartość koloru RGB565.
 */
void screen_fill_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color);

/**
 * Rysuje pojedynczy piksel bez użycia ogólnej funkcji prostokąta.
 * Dzięki temu rysowanie linii jest bardziej efektywne.
 *
 * @param x     Pozycja X piksela.
 * @param y     Pozycja Y piksela.
 * @param color Wartość koloru RGB565.
 */
void screen_draw_pixel(int16_t x, int16_t y, uint16_t color);

/**
 * Rysuje poziomą linię o długości `width` rozpoczynającą się w punkcie (x, y).
 *
 * @param x      Pozycja X początku linii.
 * @param y      Pozycja Y początku linii.
 * @param width  Długość linii w pikselach.
 * @param color  Wartość koloru RGB565.
 */
void screen_draw_hline(uint8_t x, uint8_t y, uint8_t width, uint16_t color);

/**
 * Rysuje pionową linię o długości `height` rozpoczynającą się w punkcie (x, y).
 *
 * @param x       Pozycja X początku linii.
 * @param y       Pozycja Y początku linii.
 * @param height  Długość linii w pikselach.
 * @param color   Wartość koloru RGB565.
 */
void screen_draw_vline(uint8_t x, uint8_t y, uint8_t height, uint16_t color);

/**
 * Wypełnia obszar prostokąta pojedynczym kolorem RGB565, używając trybu pomijania linii.
 *
 * skip == 0: normalne pełne wypełnienie
 * skip == 1: rysuje co drugą linię poziomą
 * skip > 2: rysuje jedną linię, a następnie pomija `skip` linii
 *
 * Funkcja zawsze zmniejsza ilość wysyłanych danych pikseli w porównaniu do pełnego wypełnienia.
 *
 * @param x      Pozycja X lewego górnego narożnika.
 * @param y      Pozycja Y lewego górnego narożnika.
 * @param width  Szerokość prostokąta w pikselach.
 * @param height Wysokość prostokąta w pikselach.
 * @param color  Wartość koloru RGB565.
 * @param skip   Wartość trybu pomijania linii.
 */
void screen_fill_rect_skip(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color, uint8_t skip);

/**
 * Rysuje linię między dwoma punktami metodą Bresenhama.
 *
 * @param x0    Współrzędna X punktu początkowego.
 * @param y0    Współrzędna Y punktu początkowego.
 * @param x1    Współrzędna X punktu końcowego.
 * @param y1    Współrzędna Y punktu końcowego.
 * @param color Wartość koloru RGB565.
 */
void screen_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

/**
 * Wysyła pojedynczy bajt polecenia do wyświetlacza.
 *
 * @param cmd Bajt polecenia.
 */
void screen_command(uint8_t cmd);

/**
 * Wysyła pojedynczy bajt danych do wyświetlacza.
 *
 * @param data Bajt danych.
 */
void screen_data(uint8_t data);

#endif // SCREEN_H

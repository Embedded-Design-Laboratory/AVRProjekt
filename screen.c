#include "screen.h"
#include <avr/io.h>
#include <util/delay.h>

#define PIN_SCK   PB5 // linia zegara SPI (SCK), Pin D13
#define PIN_MOSI  PB3 // linia magistrali danych SPI (SDA), Pin D11
#define PIN_CS    PB2 // chip select wyświetlacza (CS), Pin D10
#define PIN_RST   PB1 // reset sprzętowy wyświetlacza (RESET), Pin D9
#define PIN_DC    PB0 // data/command: wybór danych lub polecenia (A0), Pin D8
// LED i VCC do 3.3 V, GND do masy

#define CS_LOW()     (PORTB &= ~(1 << PIN_CS))
#define CS_HIGH()    (PORTB |=  (1 << PIN_CS))
#define RST_LOW()    (PORTB &= ~(1 << PIN_RST))
#define RST_HIGH()   (PORTB |=  (1 << PIN_RST))
#define DC_COMMAND() (PORTB &= ~(1 << PIN_DC))
#define DC_DATA()    (PORTB |=  (1 << PIN_DC))

// Inicjalizacja linii SPI oraz pinów sterujących wyświetlaczem.
static void spi_init(void)
{
    // ustaw SCK, MOSI, CS, RESET i DC jako wyjścia na porcie B
    DDRB |= (1 << PIN_SCK) | (1 << PIN_MOSI) | (1 << PIN_CS) |
            (1 << PIN_RST) | (1 << PIN_DC);
    // włącz stan wysoki dla CS i RESET: wyświetlacz nieaktywny i reset zwolniony
    PORTB |= (1 << PIN_CS) | (1 << PIN_RST);

    // włącz moduł SPI i ustaw mikrokontroler jako master.
    SPCR = (1 << SPE) | (1 << MSTR);
    // wyczyść dodatni rejestr statusu SPI, resetując flagi taktowania i błędów
    SPSR = 0;
}

// Wysyła bajt przez SPI i czeka na zakończenie transmisji.
static inline void spi_write(uint8_t value)
{
    SPDR = value; // wpisz wartość do rejestru SPI i rozpocznij transmisję

    while (!(SPSR & (1 << SPIF))); // czekaj, aż transmisja SPI się zakończy;
}

static inline void screen_select(void)
{
    CS_LOW();
}

static inline void screen_deselect(void)
{
    CS_HIGH();
}

// Reset sprzętowy wyświetlacza przez przyłożenie stanu niskiego do pinu RESET.
void screen_reset(void)
{
    RST_LOW();
    _delay_ms(20);
    RST_HIGH();
    _delay_ms(150);
}

// Wysyła jedno polecenie do kontrolera ST7735.
static inline void screen_write_command(uint8_t cmd)
{
    screen_select();
    DC_COMMAND(); // D/C low = polecenie
    spi_write(cmd); // wyślij bajt polecenia
    screen_deselect(); // zwolnij CS
}

// Wysyła polecenie wraz z dodatkowymi parametrami danych.
static inline void screen_write_command_data(uint8_t cmd, const uint8_t *data, uint8_t length)
{
    screen_select();
    DC_COMMAND(); // wyślij bajt polecenia
    spi_write(cmd);

    if (length && data) {
        DC_DATA(); // przełącz na tryb danych

        while (length >= 4) {
            spi_write(*data++);
            spi_write(*data++);
            spi_write(*data++);
            spi_write(*data++);
            length -= 4;
        }
        while (length--) {
            spi_write(*data++);
        }
    }

    screen_deselect();
}

// Ustawia prostokątny obszar pamięci ekranu do dalszej wysyłki pikseli.
static inline void screen_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t buffer[4];

    screen_select();

    DC_COMMAND(); spi_write(0x2A); // CASET: ustaw zakres kolumn

    buffer[0] = x0 >> 8;
    buffer[1] = x0 & 0xFF;
    buffer[2] = x1 >> 8;
    buffer[3] = x1 & 0xFF;
    DC_DATA(); // dane zakresu kolumn
    spi_write(buffer[0]); spi_write(buffer[1]); spi_write(buffer[2]); spi_write(buffer[3]);

    DC_COMMAND(); spi_write(0x2B); // RASET: ustaw zakres wierszy

    buffer[0] = y0 >> 8;
    buffer[1] = y0 & 0xFF;
    buffer[2] = y1 >> 8;
    buffer[3] = y1 & 0xFF;
    DC_DATA(); // dane zakresu wierszy
    spi_write(buffer[0]); spi_write(buffer[1]); spi_write(buffer[2]); spi_write(buffer[3]);

    screen_deselect(); // zwolnij CS po ustawieniu okna
}

void screen_command(uint8_t cmd)
{
    screen_write_command(cmd);
}

void screen_data(uint8_t data)
{
    screen_select();
    DC_DATA(); // tryb danych
    spi_write(data); // wyślij bajt danych
    screen_deselect();
}

// Wypełnia cały ekran jednym kolorem.
void screen_fill(uint16_t color)
{
    uint32_t pixelCount = (uint32_t)SCREEN_WIDTH * SCREEN_HEIGHT;
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    screen_set_address_window(0, 0,
                              SCREEN_WIDTH - 1,
                              SCREEN_HEIGHT - 1); // ustaw okno na cały ekran
    screen_write_command(0x2C); // rozpocznij zapis pikseli

    screen_select();
    DC_DATA(); // tryb danych

    while (pixelCount >= 4) {
        spi_write(hi); spi_write(lo);
        spi_write(hi); spi_write(lo);
        spi_write(hi); spi_write(lo);
        spi_write(hi); spi_write(lo);
        pixelCount -= 4;
    }
    while (pixelCount--) {
        spi_write(hi);
        spi_write(lo);
    }

    screen_deselect();
}

void screen_fill_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color)
{
    if (width == 0 || height == 0) {
        return;
    }

    uint8_t x0 = x;
    uint8_t y0 = y;
    uint8_t x1 = x0 + width - 1;
    uint8_t y1 = y0 + height - 1;
    uint32_t pixelCount = (uint32_t)width * height;
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    screen_set_address_window(x0, y0, x1, y1); // ustaw okno prostokąta
    screen_write_command(0x2C); // rozpocznij zapis pikseli

    screen_select();
    DC_DATA(); // tryb danych

    // wysyłaj piksele prostokąta w parach hi/lo
    while (pixelCount >= 4) {
        spi_write(hi); spi_write(lo);
        spi_write(hi); spi_write(lo);
        spi_write(hi); spi_write(lo);
        spi_write(hi); spi_write(lo);
        pixelCount -= 4;
    }
    while (pixelCount--) {
        spi_write(hi);
        spi_write(lo);
    }

    screen_deselect();
}

void screen_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }

    uint8_t x0 = (uint8_t)x;
    uint8_t y0 = (uint8_t)y;
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    screen_set_address_window(x0, y0, x0, y0); // ustaw okno dla jednego piksela
    screen_write_command(0x2C); // rozpocznij zapis piksela

    screen_select();
    DC_DATA(); // tryb danych
    spi_write(hi);
    spi_write(lo);
    screen_deselect();
}

void screen_draw_hline(uint8_t x, uint8_t y, uint8_t width, uint16_t color)
{
    if (width == 0) {
        return;
    }

    uint8_t x0 = x;
    uint8_t y0 = y;
    uint8_t x1 = x0 + width - 1;
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    screen_set_address_window(x0, y0, x1, y0); // ustaw okno poziomej linii
    screen_write_command(0x2C); // rozpocznij zapis pikseli

    screen_select();
    DC_DATA(); // tryb danych

    while (width--) {
        spi_write(hi);
        spi_write(lo);
    }

    screen_deselect();
}

void screen_draw_vline(uint8_t x, uint8_t y, uint8_t height, uint16_t color)
{
    if (height == 0) {
        return;
    }

    uint8_t x0 = x;
    uint8_t y0 = y;
    uint8_t y1 = y0 + height - 1;
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint8_t count = height;

    screen_set_address_window(x0, y0, x0, y1); // ustaw okno pionowej linii
    screen_write_command(0x2C); // rozpocznij zapis pikseli

    screen_select();
    DC_DATA(); // tryb danych

    while (count--) {
        spi_write(hi);
        spi_write(lo);
    }

    screen_deselect();
}

// Wypełnia prostokąt kolorem, pomijając co pewną liczbę linii dla przyspieszenia.
// Szybciej rysuje ale trochę meh.
void screen_fill_rect_skip(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color, uint8_t skip)
{
    if (skip == 0) {
        screen_fill_rect(x, y, width, height, color);
        return;
    }

    if (width == 0 || height == 0) {
        return;
    }

    uint8_t x0 = x;
    uint8_t y0 = y;
    uint8_t x1 = x0 + width - 1;
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint8_t lineStep = (skip == 1) ? 2 : (skip + 1);

    for (uint8_t row = 0; row < height; row += lineStep) { // co N-ty wiersz, żeby zmniejszyć liczbę pikseli
        uint8_t current_y = y0 + row;
        uint32_t pixelCount = width;

        screen_set_address_window(x0, current_y, x1, current_y); // ustaw okno jednego wiersza
        screen_write_command(0x2C); // rozpocznij zapis pikseli

        screen_select();
        DC_DATA(); // tryb danych

        while (pixelCount--) {
            spi_write(hi);
            spi_write(lo);
        }

        screen_deselect();
    }
}

// Rysuje linię między dwoma punktami metodą Bresenhama.
void screen_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    int16_t sx = dx >= 0 ? 1 : -1;
    int16_t sy = dy >= 0 ? 1 : -1;
    dx = dx >= 0 ? dx : -dx;
    dy = dy >= 0 ? dy : -dy;
    int16_t err = (dx > dy ? dx : -dy) / 2;
    int16_t e2;

    while (1) {
        screen_draw_pixel(x0, y0, color); // narysuj bieżący punkt linii
        if (x0 == x1 && y0 == y1) {
            break;
        }

        e2 = err; // popraw wartość błędu i wykonaj krok w x/y
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

// Inicjalizuje wyświetlacz ST7735, reset oraz sekwencję konfiguracji.
void screen_init(void)
{
    spi_init();
    screen_reset();

    // Parametry trybu odświeżania i konfiguracji napięć.
    static const uint8_t frctr1[] = {0x01, 0x2C, 0x2D};
    static const uint8_t frctr2[] = {0x01, 0x2C, 0x2D};
    static const uint8_t frctr3[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
    static const uint8_t invctr[]  = {0x07};
    static const uint8_t pwctr1[] = {0xA2, 0x02, 0x84};
    static const uint8_t pwctr2[] = {0xC5};
    static const uint8_t pwctr3[] = {0x0A, 0x00};
    static const uint8_t pwctr4[] = {0x8A, 0x2A};
    static const uint8_t pwctr5[] = {0x8A, 0xEE};
    static const uint8_t vmctr1[] = {0x0E};
    static const uint8_t colmod[] = {0x05};
    static const uint8_t madctl[] = {0xC0};
    static const uint8_t caset[]  = {0x00, 0x00, 0x00, 0x7F};
    static const uint8_t raset[]  = {0x00, 0x00, 0x00, 0x9F};
    static const uint8_t gmctrp1[] = {0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10};
    static const uint8_t gmctrn1[] = {0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10};

    // Reset kontrolera i wyjście ze stanu uśpienia.
    screen_write_command(0x01); // SWRESET
    _delay_ms(150);
    screen_write_command(0x11); // SLPOUT
    _delay_ms(255);

    // Konfiguracja ogranicznika częstotliwości i mocy.
    screen_write_command_data(0xB1, frctr1, sizeof(frctr1));
    screen_write_command_data(0xB2, frctr2, sizeof(frctr2));
    screen_write_command_data(0xB3, frctr3, sizeof(frctr3));
    screen_write_command_data(0xB4, invctr, sizeof(invctr));
    screen_write_command_data(0xC0, pwctr1, sizeof(pwctr1));
    screen_write_command_data(0xC1, pwctr2, sizeof(pwctr2));
    screen_write_command_data(0xC2, pwctr3, sizeof(pwctr3));
    screen_write_command_data(0xC3, pwctr4, sizeof(pwctr4));
    screen_write_command_data(0xC4, pwctr5, sizeof(pwctr5));
    screen_write_command_data(0xC5, vmctr1, sizeof(vmctr1));

    // Wyłącz odwrotność obrazu, ustaw format pikseli i orientację wyświetlacza.
    screen_write_command(0x20); // INVOFF
    screen_write_command_data(0x3A, colmod, sizeof(colmod));
    screen_write_command_data(0x36, madctl, sizeof(madctl));

    // Ustawienia obszaru rysowania (cały ekran).
    screen_write_command_data(0x2A, caset, sizeof(caset));
    screen_write_command_data(0x2B, raset, sizeof(raset));

    // Ustaw krzywe gamma dla jaśniejszych i ciemniejszych tonów.
    screen_write_command_data(0xE0, gmctrp1, sizeof(gmctrp1));
    screen_write_command_data(0xE1, gmctrn1, sizeof(gmctrn1));

    // Włącz tryb normalny i włącz wyświetlacz.
    screen_write_command(0x13); // NORON
    _delay_ms(10);
    screen_write_command(0x29); // DISPON
    _delay_ms(100);
}

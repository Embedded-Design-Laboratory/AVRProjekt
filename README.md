# Konsola
EIT-EP-2024
## Opis
Projekt przedstawia przenośną konsolę do gier opartą o mikrokontroler ATmega328P.
Urządzenie wyposażone jest w kolorowy wyświetlacz TFT LCD 1.8" oraz sześć przycisków sterujących.
Oprogramowanie umożliwia uruchamianie gier kompilowanych jako osobne programy.

### Użyte komponenty
* Mikrokontroler: **ATmega328P**
* Wyświetlacz: **LCD TFT 1.8" SPI ST7735S**
* Zasilanie: **ogniwo Li-Ion 18650**

### Kompilacja gier
Do kompilacji wybranej gry użyj polecenia:
```bash
make <nazwa_gry>
```
Czysty `make` skompiluje `main.c`.

### Gry
| Gra | Kompilacja |
|------|------------|
| Snake | `make snake` |
| Tetris | `make tetris` |

### Model obudowy
[![Model 3D](assets/obudowa.png)](https://sketchfab.com/3d-models/obudowa-2c08534e8cd44891bd327603aee7c3e8)
**Kliknij obraz, aby otworzyć interaktywny model 3D na Sketchfab.**

### Aktualnie używane piny
| Rodzaj      | Pin (w kodzie)  | Port ATmega328P | Pin Arduino | Funkcja           |
| :---------- | :-------------- | :-------------- | :---------- | :---------------- |
| **Ekran**   | `PIN_SCK`       | PB5             | D13         | Zegar SPI         |
| **Ekran**   | `PIN_MOSI`      | PB3             | D11         | Dane SPI (SDA)    |
| **Ekran**   | `PIN_CS`        | PB2             | D10         | Chip Select (CS)  |
| **Ekran**   | `PIN_RST`       | PB1             | D9          | Reset (RST)       |
| **Ekran**   | `PIN_DC`        | PB0             | D8          | Data/Command (A0) |
| **Wejście** | `PIN_UP`        | PD2             | D2          | Góra              |
| **Wejście** | `PIN_DOWN`      | PD3             | D3          | Dół               |
| **Wejście** | `PIN_LEFT`      | PD4             | D4          | Lewo              |
| **Wejście** | `PIN_RIGHT`     | PD5             | D5          | Prawo             |
| **Wejście** | `PIN_A`         | PD6             | D6          | Przycisk "A"      |
| **Wejście** | `PIN_B`         | PD7             | D7          | Przycisk "B"      |
>Przyciski są pull-down, ze wspólną masą,
>**Ekran** ma poziom logiczny 3V


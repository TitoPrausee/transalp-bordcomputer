# GPIO Pin Mapping (ESP32-S3)

Vorläufige Belegung. Änderungen werden hier nachgezogen.

## UART

| Funktion | GPIO | Richtung | Bemerkung |
|---|---|---|---|
| GPS UART RX | GPIO 44 | IN | NEO-M9N TX → ESP RX |
| GPS UART TX | GPIO 43 | OUT | ESP TX → NEO-M9N RX (Konfig) |
| GPS PPS | GPIO 1 | IN (IRQ) | 1PPS Zeitsync |
| OBD-II UART RX | GPIO 18 | IN | (falls K-Line direkt, nicht BT) |
| OBD-II UART TX | GPIO 17 | OUT | |

## I2C (Sensoren)

| Funktion | GPIO | Bemerkung |
|---|---|---|
| I2C SCL | GPIO 9 | MPU6050 + BMP280 |
| I2C SDA | GPIO 8 | shared Bus |
| MPU6050 INT | GPIO 2 | IRQ (Bewegung/Crash) |

## SPI (Display + SD)

| Funktion | GPIO | Bemerkung |
|---|---|---|
| SPI MOSI | GPIO 11 | shared (Display + SD) |
| SPI MISO | GPIO 13 | shared |
| SPI SCLK | GPIO 12 | shared |
| Display CS | GPIO 10 | E-Paper |
| Display DC | GPIO 14 | Data/Command |
| Display RST | GPIO 21 | |
| Display BUSY | GPIO 47 | IRQ, wird für Refresh gewartet |
| SD CS | GPIO 38 | MicroSD |

## ADC

| Funktion | GPIO | Bemerkung |
|---|---|---|
| Akku-Spannung | GPIO 4 | Spannungsteiler 2:1 (max 3.3V) |

## Power Control

| Funktion | GPIO | Bemerkung |
|---|---|---|
| GPS Power EN | GPIO 5 | High-Side MOSFET (GPS ganz aus) |
| Boost EN | GPIO 6 | MT3608 EN-Pin (5V-Rail schalten) |

## Frei / Reserve

GPIO 0, 3, 7, 15, 16, 19, 20, 35, 36, 37, 39, 40, 41, 42, 45, 46, 48

→ Reserve für: Touch-Buttons, LED-Status, zweites Display, Erweiterungen.

## Pin-Konflikte prüfen

- GPIO 0: Boot-Strap (MUSS beim Flash LOW sein) → nicht als Output verwenden
- GPIO 46: Boot-Strap → nur Input
- GPIO 19/20: USB (D+/D-) → frei lassen wenn USB-Debug gebraucht wird
- GPIO 43/44: Default UART0 (USB-CDC) → kann umkonfiguriert werden

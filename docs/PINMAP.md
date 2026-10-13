# ESP32-S3-WROOM-1 Pin-Belegung (PINMAP)

> Bordcomputer Transalp 600 (PD06)
> MCU: ESP32-S3-WROOM-1 (N16R8: 16MB Flash, 8MB Octal PSRAM)
> Stand: Oktober 2026

Diese Datei ist die alleinige Referenz fuer alle Pin-Zuweisungen.
`firmware/main/board_config.h` muss dieser Tabelle entsprechen.

## Vollstaendige Pin-Tabelle

| GPIO  | Funktion         | Bus/Interface    | ADC         | RTC  | Pull     | IRQ  | Zugewiesene Aufgabe                        | Notizen                                           |
|-------|-----------------|------------------|-------------|------|----------|------|-------------------------------------------|---------------------------------------------------|
| 0     | Input           | —                | —           | Ja   | Pull-Up  | Ja  | BOOT_BTN (Boot-Taster)                    | Strapping-Pin: LOW = Download-Mode. Beim Boot HIGH  |
| 1     | Analog Input    | ADC1_CH0         | 12-bit      | Ja   | —        | Nein | BATT_VOLTAGE (Bordspannungsmessung)       | Spannungsteiler 12V→ADC, ADC_ATTEN_DB_12           |
| 2     | Analog Input    | ADC1_CH1         | 12-bit      | Ja   | —        | Nein | COOLANT_TEMP (Kuehlmitteltemperatur NTC)  | NTC-Spannungsteiler, Steinhart-Hart-Auswertung     |
| 3     | Digital Input   | GPIO             | ADC1_CH2    | Ja   | Pull-Up  | Ja  | BTN_BACK (Taster Zurueck)                | Strapping (JTAG-Select). HIGH bei Boot = USB-JTAG. Als Input mit Pull-Up sicher |
| 4     | Analog Input    | ADC1_CH3         | 12-bit      | Ja   | —        | Nein | FUEL_LEVEL (Kraftstoffstand)              | Analoger Tankgeber, Spannungsteiler               |
| 5     | Analog Input    | ADC1_CH4         | 12-bit      | Ja   | —        | Nein | OIL_PRESSURE (Oeldruck)                   | 0.5-4.5V Sensor, ADC_ATTEN_DB_12                  |
| 6     | Digital Input   | GPIO             | —           | Ja   | Pull-Up  | Ja  | BTN_MENU (Taster Menu)                    | Active-LOW, Entprellung in Software               |
| 7     | Digital Input   | GPIO             | —           | Ja   | Pull-Up  | Ja  | BTN_SELECT (Taster Select/Bestaetigen)    | Active-LOW, Entprellung in Software               |
| 8     | Digital I/O     | I2C0 SDA         | ADC1_CH7    | —    | Pull-Up  | Nein | I2C_SDA (MPU6050 + BMP280)                | I2C Bus 0, 400 kHz Fast Mode                      |
| 9     | Digital Output  | I2C0 SCL         | ADC1_CH8    | —    | Pull-Up  | Nein | I2C_SCL (MPU6050 + BMP280)                | I2C Bus 0, 400 kHz Fast Mode                      |
| 10    | Digital Output  | SPI2 (FSPI) CS   | ADC1_CH9    | —    | —        | Nein | TFT_CS (ILI9488 Chip Select)              | Active-LOW, Shared FSPI-Bus                       |
| 11    | Digital Output  | SPI2 (FSPI) MOSI | ADC2_CH0    | —    | —        | Nein | FSPI_MOSI (Shared Bus: TFT + EPD + SD)    | ADC2 nicht nutzbar (BLE aktiv). Digital verwendet |
| 12    | Digital Output  | SPI2 (FSPI) SCK  | ADC2_CH1    | —    | —        | Nein | FSPI_SCK (Shared Bus: TFT + EPD + SD)     | SPI-Takt bis 80 MHz                                |
| 13    | Digital Input   | SPI2 (FSPI) MISO | ADC2_CH2    | —    | —        | Nein | FSPI_MISO (nur SD-Karte Lese-Richtung)    | TFT und EPD sind Write-Only; MISO nur fuer SD      |
| 14    | Digital Output  | GPIO             | —           | Ja   | —        | Nein | TFT_DC (ILI9488 Data/Command)             | HIGH = Data, LOW = Command                         |
| 15    | Digital Input   | PCNT0            | —           | Ja   | Pull-Up  | Ja  | RPM_PULSE (Drehzahl-Sensor)              | Optokoppler + Schmitt-Trigger, PCNT-Eingang       |
| 16    | Digital Input   | PCNT1            | —           | Ja   | Pull-Up  | Ja  | TACHO_PULSE (Tacho/Geschwindigkeit)       | Reed-Kontakt + Pull-Up, PCNT-Eingang              |
| 17    | Digital Output  | GPIO             | —           | Ja   | —        | Nein | TFT_RST (ILI9488 Reset)                   | Active-LOW                                         |
| 18    | Digital Output  | LEDC PWM         | —           | Ja   | —        | Nein | TFT_BL (ILI9488 Backlight)               | PWM-Helligkeitsregelung ueber LEDC                |
| 19    | USB             | USB-OTG D+       | —           | —    | —        | —    | USB_DP (USB-C Data+)                      | Fixed USB-OTG, nicht ummappbar                     |
| 20    | USB             | USB-OTG D-       | —           | —    | —        | —    | USB_DM (USB-C Data-)                      | Fixed USB-OTG, nicht ummappbar                     |
| 21    | Digital Output  | GPIO             | —           | Ja   | —        | Nein | STATUS_LED                                | Status-LED (gruen), Heartbeat/BLE-Status           |
| 22-25 | —               | —                | —           | —    | —        | —    | NICHT VORHANDEN                           | GPIO22-25 existieren auf ESP32-S3 nicht             |
| 26    | —               | SPI0 (intern)    | —           | —    | —        | —    | INTERNAL FLASH CLK                        | Niemals extern verwenden                           |
| 27    | —               | SPI0 (intern)    | —           | —    | —        | —    | INTERNAL FLASH D                          | Niemals extern verwenden                           |
| 28    | —               | SPI0 (intern)    | —           | —    | —        | —    | INTERNAL FLASH D                          | Niemals extern verwenden                           |
| 29    | —               | SPI0 (intern)    | —           | —    | —        | —    | INTERNAL FLASH D                          | Niemals extern verwenden                           |
| 30    | —               | SPI0 (intern)    | —           | —    | —        | —    | INTERNAL FLASH D                          | Niemals extern verwenden                           |
| 31    | —               | SPI0 (intern)    | —           | —    | —        | —    | INTERNAL FLASH D                          | Niemals extern verwenden                           |
| 32    | —               | SPI0 (intern)    | —           | —    | —        | —    | INTERNAL FLASH D                          | Niemals extern verwenden                           |
| 33    | —               | SPI1 (intern)    | —           | —    | —        | —    | INTERNAL PSRAM (Octal)                    | Niemals extern verwenden (N8R2/N16R8)             |
| 34    | —               | SPI1 (intern)    | —           | —    | —        | —    | INTERNAL PSRAM (Octal)                    | Niemals extern verwenden (N8R2/N16R8)             |
| 35    | —               | SPI1 (intern)    | —           | —    | —        | —    | INTERNAL PSRAM (Octal)                    | Niemals extern verwenden (N8R2/N16R8)             |
| 36    | —               | SPI1 (intern)    | —           | —    | —        | —    | INTERNAL PSRAM (Octal)                    | Niemals extern verwenden (N8R2/N16R8)             |
| 37    | —               | SPI1 (intern)    | —           | —    | —        | —    | INTERNAL PSRAM (Octal)                    | Niemals extern verwenden (N8R2/N16R8)             |
| 38    | Digital Output  | LEDC PWM         | —           | —    | —        | Nein | BUZZER (Piezo)                            | Akustische Signale, LEDC-PWM                      |
| 39    | Digital Input   | GPIO             | —           | Ja   | Pull-Up  | Ja  | GPS_PPS (1PPS Zeitsynchronisation)        | RTC-GPIO fuer Deep-Sleep-Wake moeglich             |
| 40    | Digital Output  | SPI2 (FSPI) CS   | —           | —    | —        | Nein | EPD_CS (E-Paper Chip Select)              | Active-LOW, Shared FSPI-Bus                       |
| 41    | Digital Output  | GPIO             | —           | —    | —        | Nein | EPD_DC (E-Paper Data/Command)             | HIGH = Data, LOW = Command                         |
| 42    | Digital Output  | GPIO             | —           | —    | —        | Nein | EPD_RST (E-Paper Reset)                   | Active-LOW                                         |
| 43    | Digital Output  | UART1 TX         | —           | —    | —        | Nein | GPS_TX (MCU → GPS RX, Konfiguration)      | UART1, 115200 Baud (AT6558)                       |
| 44    | Digital Input   | UART1 RX         | —           | —    | —        | Nein | GPS_RX (GPS TX → MCU RX, NMEA-Daten)      | UART1, 115200 Baud (AT6558)                       |
| 45    | —               | —                | —           | —    | —        | —    | NICHT VERWENDET                           | Strapping-Pin (VDDSPI Voltage). Frei halten       |
| 46    | —               | —                | —           | —    | —        | —    | NICHT VERWENDET                           | Strapping-Pin (Boot-Mode). Frei halten             |
| 47    | Digital Output  | SPI2 (FSPI) CS   | —           | —    | —        | Nein | SD_CS (MicroSD Chip Select)               | Active-LOW, Shared FSPI-Bus                       |
| 48    | Digital Input   | GPIO             | —           | Ja   | Pull-Up  | Ja  | MPU6050_INT (IMU Interrupt)              | Bewegungs-/Crash-Erkennung, RTC-GPIO fuer Wake     |

## Pin-Belegung Zusammenfassung

### Digitale Interfaces

| Bus     | Controller | Pins                    | Geraete                | Taktrate  |
|---------|-----------|-------------------------|------------------------|-----------|
| SPI2    | FSPI      | MOSI=11, SCK=12, MISO=13 | ILI9488 TFT, E-Paper 2.13", MicroSD | 40-80 MHz |
| I2C0    | I2C_NUM_0 | SDA=8, SCL=9            | MPU6050 (0x68), BMP280 (0x76) | 400 kHz  |
| UART1   | UART_NUM_1| TX=43, RX=44            | AT6558 GPS             | 115200    |
| USB-OTG | —         | D+=19, D-=20            | USB-C (Power + Serial) | 12 Mbit/s|

### ADC-Kanaele (alle auf ADC1, da ADC2 mit BLE unzuverlaessig)

| GPIO | ADC1-Kanal | Sensor               | Messbereich      | Attenuation |
|------|-----------|----------------------|-------------------|-------------|
| 1    | CH0       | Bordspannung         | 0-15V → 0-2.9V    | 12 dB       |
| 2    | CH1       | Kuehlmitteltemp (NTC)| 0-3.3V (Teiler)  | 12 dB       |
| 4    | CH3       | Kraftstoffstand       | 0-3.3V (Teiler)  | 12 dB       |
| 5    | CH4       | Oeldruck              | 0.5-4.5V → 0-2.9V| 12 dB       |

### Pulse-Counter (PCNT)

| GPIO | PCNT-Kanal | Sensor         | Signalquelle              |
|------|-----------|----------------|---------------------------|
| 15   | PCNT0     | Drehzahl       | Optokoppler (Zuendimpuls)  |
| 16   | PCNT1     | Tacho          | Reed-Kontakt (Vorderrad)  |

### Strapping-Pins (Boot-relevant)

| GPIO  | Funktion beim Boot                | Verwendung                |
|-------|-----------------------------------|---------------------------|
| 0     | Boot-Mode (LOW = Download)        | BOOT_BTN (mit Pull-Up)    |
| 3     | JTAG-Source-Select (HIGH = USB)   | BTN_BACK (Input, Pull-Up)  |
| 45    | VDDSPI Voltage (3.3V)             | Nicht verwendet (frei)     |
| 46    | Boot-Mode                         | Nicht verwendet (frei)     |

GPIO3 wird als Taster-Eingang verwendet. Beim Boot muss dieser Pin HIGH
sein (Pull-Up sicherstellt das). LOW wuerde JTAG auf externe Pins umschalten,
was nicht gewollt ist. Als Input mit Pull-Up ist die Verwendung sicher: der
Taster zieht kurz auf LOW, aber nur wenn der Button gedrueckt wird — nicht
beim Boot.

### Nicht verwendbare Pins

| GPIO-Bereich | Grund                                     |
|-------------|-------------------------------------------|
| 22-25       | Existieren auf ESP32-S3-Silizium nicht    |
| 26-32       | Intern mit SPI-Flash verbunden            |
| 33-37       | Intern mit Octal-PSRAM verbunden (N8R2/N16R8) |

### Freie Pins

Nach obiger Belegung sind alle verfuegbaren GPIOs zugewiesen.
Es gibt keine freien Pins mehr fuer Erweiterungen. Bei Bedarf koennen
die Taster-Pins (GPIO6, GPIO7, GPIO3) multiplexed oder ein I2C-Port-Expander
(PCF8574) fuer zusaetzliche Ein-/Ausgaenge verwendet werden.

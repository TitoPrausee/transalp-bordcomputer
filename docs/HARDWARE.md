# Hardware-Schaltplan: Pin-Layout und Verdrahtungsplan

> Bordcomputer Honda Transalp 600 (PD06)
> MCU: ESP32-S3-WROOM-1 (N16R8: 16MB Flash, 8MB Octal PSRAM)
> Stand: Oktober 2026
> Quelle: docs/research/02-hardware-deep-dive.md, 05-gehaeuse-einbau-emv.md, 11-firmware-architecture.md

Dieses Dokument beschreibt den vollstaendigen Hardware-Schaltplan des
Bordcomputers. Es ist so detailliert, dass eine Platine danach geaetzt
oder bestellt werden kann. Die Pin-Belegung ist in docs/PINMAP.md
 definiert und muss mit firmware/main/board_config.h uebereinstimmen.

---

## 1. ESP32-S3 Pin-Belegung (vollstaendig)

Der ESP32-S3-WROOM-1 in der N16R8-Variante bietet 45 nutzbare GPIO-Pins
(GPIO0 bis GPIO48, mit Luecken bei 22-25 und intern belegten 26-37).
Die vollstaendige Pin-Tabelle mit allen Attributen befindet sich in
docs/PINMAP.md. Hier folgt die kompakte Zusammenfassung der
funktionalen Gruppierung.

### 1.1 Funktionsgruppen

**SPI2 (FSPI) — Shared Bus fuer TFT, E-Paper und SD-Karte:**

| Signal  | GPIO | Richtung | Verwendung                    |
|---------|------|----------|-------------------------------|
| MOSI    | 11   | OUT      | Shared Datenleitung (TFT+EPD+SD) |
| SCK     | 12   | OUT      | Shared Taktleitung            |
| MISO    | 13   | IN       | Nur SD-Karte (TFT/EPD write-only) |
| TFT_CS  | 10   | OUT      | ILI9488 Chip Select           |
| EPD_CS  | 40   | OUT      | E-Paper 2.13" Chip Select     |
| SD_CS   | 47   | OUT      | MicroSD Chip Select           |

**I2C0 — Sensoren (MPU6050 + BMP280):**

| Signal      | GPIO | Richtung | Verwendung               |
|-------------|------|----------|---------------------------|
| I2C_SDA     | 8    | I/O      | Shared I2C Data           |
| I2C_SCL     | 9    | OUT      | Shared I2C Clock          |
| MPU6050_INT | 48   | IN       | IMU Interrupt (Wake/Crash)|

**UART1 — GPS (AT6558):**

| Signal  | GPIO | Richtung | Verwendung               |
|---------|------|----------|---------------------------|
| GPS_TX  | 43   | OUT      | MCU → GPS (Konfiguration)|
| GPS_RX  | 44   | IN       | GPS → MCU (NMEA-Daten)   |
| GPS_PPS | 39   | IN       | 1PPS Zeitsynchronisation  |

**ADC1 — Analoge Sensoren:**

| Signal          | GPIO | ADC1-Kanal | Sensor              |
|-----------------|------|-----------|----------------------|
| BATT_VOLTAGE    | 1    | CH0       | Bordspannung         |
| COOLANT_TEMP    | 2    | CH1       | Kuehlmitteltemp NTC  |
| FUEL_LEVEL      | 4    | CH3       | Kraftstoffstand      |
| OIL_PRESSURE    | 5    | CH4       | Oeldruck             |

Alle analogen Sensoren liegen auf ADC1 (GPIO1-GPIO10), da ADC2
(GPIO11-GPIO20) von BLE arbitriert wird und bei aktivem Bluetooth
unzuverlaessige Werte liefert.

**PCNT — Pulse-Counter fuer Drehzahl und Tacho:**

| Signal       | GPIO | PCNT-Kanal | Sensor              |
|--------------|------|-----------|----------------------|
| RPM_PULSE    | 15   | PCNT0     | Drehzahl (Optokoppler)|
| TACHO_PULSE  | 16   | PCNT1     | Tacho (Reed-Kontakt)  |

**GPIO — Taster, Display-Steuerung, Status:**

| Signal      | GPIO | Richtung | Verwendung               |
|-------------|------|----------|---------------------------|
| BOOT_BTN    | 0    | IN       | Boot-Taster (Strapping)  |
| BTN_MENU    | 6    | IN       | Taster Menu              |
| BTN_SELECT  | 7    | IN       | Taster Select            |
| BTN_BACK    | 3    | IN       | Taster Zurueck (Strapping)|
| TFT_DC      | 14   | OUT      | ILI9488 Data/Command      |
| TFT_RST     | 17   | OUT      | ILI9488 Reset             |
| TFT_BL      | 18   | OUT      | ILI9488 Backlight (PWM)  |
| EPD_DC      | 41   | OUT      | E-Paper Data/Command      |
| EPD_RST     | 42   | OUT      | E-Paper Reset             |
| STATUS_LED  | 21   | OUT      | Status-LED (gruen)        |
| BUZZER      | 38   | OUT      | Piezo-Buzzer (PWM)        |

**USB-OTG (Fixed):**

| Signal | GPIO | Verwendung             |
|--------|------|------------------------|
| USB_DP | 19   | USB-C Data+ (Power+Serial) |
| USB_DM | 20   | USB-C Data-            |

### 1.2 Strapping-Pins

Der ESP32-S3 hat vier Strapping-Pins, deren Zustand beim Boot die
Konfiguration bestimmt:

- GPIO0: LOW = Download-Mode (Flash-Modus). Verwendung als Boot-Taster
  mit Pull-Up: beim normalen Boot ist GPIO0 HIGH, beim Gedrueckt halten
  LOW → Flash-Mode. Sichert gleichzeitig die Boot-Funktion.
- GPIO3: HIGH = USB-JTAG aktiv. Verwendung als BTN_BACK mit Pull-Up:
  beim Boot ist GPIO3 HIGH → USB-JTAG bleibt aktiv. Der Taster zieht
  nur bei Betaetigung kurz auf LOW.
- GPIO45: Bestimmt VDDSPI Voltage (3.3V). Nicht verwendet, muss beim
  Boot LOW sein (intern default).
- GPIO46: Boot-Mode-Bit. Nicht verwendet, intern default.

---

## 2. Verdrahtungsplan

### 2.1 ESP32-S3 MCU (WROOM-1 Modul)

Das WROOM-1-Modul wird als vorgefertigte Baugruppe verwendet. Die
externe Verdrahtung erfolgt ueber die Pin-Header des Moduls bzw. die
PCB-Pads bei einer Custom-Platine.

Stromversorgung des Moduls:
- 3.3V an Pin 3V3 (aus HT7333 LDO)
- GND an Pin GND
- EN-Pin: Pull-Up 10k nach 3.3V, 100nF Kondensator nach GND (Stabilitaet)
- IO0: 10k Pull-Up nach 3.3V (Boot-Mode, siehe Strapping)

### 2.2 ILI9488 TFT Display (SPI)

Das ILI9488 ist ein 3.5" TFT mit 480x320 Pixeln, RGB565-Farbformat.
Es wird ueber den shared SPI2 (FSPI) angeschlossen. Das Display ist
write-only (kein MISO benoetigt).

```
ILI9488 Pin     ESP32-S3 GPIO    Funktion
──────────────────────────────────────────────
VCC             3.3V / 5V        Power (Modul-abhaengig, siehe Note)
GND             GND              Ground
CS              GPIO10           Chip Select (Active-LOW)
DC              GPIO14           Data/Command (HIGH=Data, LOW=Cmd)
RST             GPIO17           Reset (Active-LOW)
BL              GPIO18           Backlight (LEDC PWM, dimmbar)
SDI (MOSI)      GPIO11           SPI MOSI (Shared FSPI)
SCK             GPIO12           SPI SCK (Shared FSPI)
```

Note: Einige ILI9488-Module haben einen eigenen LDO und akzeptieren 5V
am VCC-Pin, andere brauchen 3.3V. Bei 5V-Modulen BL ueber einen
Transistor-Pegelausgang schalten, da GPIO18 nur 3.3V liefert.

SPI-Takt: 40 MHz fuer stabile Anzeige (max. 80 MHz moeglich).
Bei 40 MHz und 480x320x2 Bytes = 307KB pro Frame: theoretisch ~26 FPS.
Mit Dirty-Rect-Tracking (nur geaenderte Bereiche) deutlich mehr.

### 2.3 E-Paper 2.13" (SPI)

Das E-Paper-Display (Display-Modul-Hersteller 2.13", 250x122 Pixel, SSD1680/UC8151D)
wird ebenfalls am shared SPI2 (FSPI) betrieben. Es ist write-only und
hat zusaetzlich ein BUSY-Signal, das aber beim 2.13"-Modul ueber
polling abgefragt werden kann.

```
E-Paper Pin     ESP32-S3 GPIO    Funktion
──────────────────────────────────────────────
VCC             3.3V             Power
GND             GND              Ground
CS              GPIO40           Chip Select (Active-LOW)
DC              GPIO41           Data/Command
RST             GPIO42           Reset (Active-LOW)
BUSY            —                Wird per Software-Polling abgefragt
DIN (MOSI)      GPIO11           SPI MOSI (Shared FSPI)
CLK (SCK)       GPIO12           SPI SCK (Shared FSPI)
```

Der BUSY-Pin wurde eingespart, da der 2.13"-Treiber mit festen
Wartezeiten arbeitet (Full-Refresh: 2s, Partial: 300ms). Bei
Bedarf kann BUSY auf einen freien Pin gemappt werden (z.B. GPIO39,
dann aber PPS verlegen).

SPI-Takt fuer E-Paper: 4-20 MHz (niedriger als TFT, aber das E-Paper
braucht nicht viel Bandbreite bei 250x122 Pixeln).

### 2.4 MicroSD-Karten-Modul (SPI)

Die MicroSD-Karte nutzt denselben SPI2-Bus wie TFT und E-Paper, ist
aber das einzige Geraet, das MISO benoetigt (Lese-Richtung).

```
MicroSD Pin     ESP32-S3 GPIO    Funktion
──────────────────────────────────────────────
VCC             3.3V             Power (NIEMALS 5V!)
GND             GND              Ground
MOSI            GPIO11           SPI MOSI (Shared FSPI)
MISO            GPIO13           SPI MISO (Shared FSPI, nur SD)
SCK             GPIO12           SPI SCK (Shared FSPI)
CS              GPIO47           Chip Select (Active-LOW)
```

Zusaetzlich Pull-Up-Widerstaende (10k) an CS, MISO, MOSI und SCK,
um die SD-Karte beim Einstecken/Hot-Plug zu stabilisieren. Die
ESP32-S3 internen Pull-Ups sind schwach (45k) und reichen nicht
fuer zuverlaessigen SD-Betrieb.

SPI-Takt fuer SD: 400 kHz bei Initialisierung (SD-Protokoll), danach
bis 40 MHz fuer Datentransfer.

### 2.5 GPS-Modul AT6558 (UART)

Das AT6558 GPS-Modul kommuniziert ueber UART1 mit 115200 Baud.
Es unterstuetzt NMEA und bietet einen 1PPS-Ausgang fuer genaue
Zeitsynchronisation.

```
AT6558 Pin       ESP32-S3 GPIO    Funktion
──────────────────────────────────────────────
VCC              3.3V             Power
GND              GND              Ground
TX               GPIO44           GPS TX → MCU UART1 RX
RX               GPIO43           MCU UART1 TX → GPS RX (Konfig)
PPS              GPIO39           1PPS Impuls (IRQ, RTC-GPIO)
```

UART-Konfiguration: 115200 Baud, 8N1. NMEA-Saetze: GGA, RMC, VTG.
Die 1PPS-Leitung erzeugt einen Impuls pro Sekunde mit Mikrosekunden-
Genauigkeit und kann als RTC-Wake-Source verwendet werden.

### 2.6 IMU MPU6050 (I2C)

Der MPU6050 (6-DoF: Accel + Gyro) ist am I2C0-Bus mit Adresse 0x68
(AD0 = LOW). Der Interrupt-Pin meldet Bewegungs- und Crash-Ereignisse.

```
MPU6050 Pin      ESP32-S3 GPIO    Funktion
──────────────────────────────────────────────
VCC              3.3V             Power
GND              GND              Ground
SDA              GPIO8            I2C0 SDA (Shared)
SCL              GPIO9            I2C0 SCL (Shared)
INT              GPIO48           Interrupt (Bewegung/Crash, Wake)
AD0              GND              I2C-Adresse 0x68
```

Der INT-Pin ist RTC-faehig (GPIO48 = RTC_GPIO17) und kann den ESP32-S3
aus dem Deep-Sleep wecken (Wake-on-Motion). Dadurch kann der Bordcomputer
im Parkmodus bei Erschuetterung aufwachen.

### 2.7 BMP280 Luftdruck (I2C)

Der BMP280 misst Luftdruck und Temperatur fuer Hoehenmeter-Logging.
Er teilt sich den I2C0-Bus mit dem MPU6050.

```
BMP280 Pin       ESP32-S3 GPIO    Funktion
──────────────────────────────────────────────
VCC              3.3V             Power
GND              GND              Ground
SDA              GPIO8            I2C0 SDA (Shared)
SCL              GPIO9            I2C0 SCL (Shared)
SDO              GND              I2C-Adresse 0x76
CSB              3.3V             I2C-Mode (nicht SPI)
```

I2C-Adresse: 0x76 (SDO = GND) oder 0x77 (SDO = VCC).
Kein eigener Interrupt-Pin; der BMP280 wird periodisch (1-10 Hz) gepollt.

### 2.8 Drehzahl-Sensor (Interrupt/PCNT)

Die Drehzahlmessung erfolgt ueber Zuendimpulse, die ueber einen
Optokoppler galvanisch getrennt und ueber einen Schmitt-Trigger
entprellt werden.

```
Komponente              ESP32-S3 GPIO    Funktion
──────────────────────────────────────────────
Optokoppler-Ausgang     GPIO15           PCNT0-Eingang (IRQ)
```

Schaltung: Die Zündspule erzeugt Hochspannungsimpulse. Ein induktiver
Abgriff (Pickup-Spule am Zuendkabel) oder direkter Anschluss an das
Pickup-Signal der CDI liefert ein Signal, das ueber einen Vorwiderstand
und eine Zener-Diode auf sicheres Niveau gebracht wird. Der Optokoppler
(PC817 oder 6N137 fuer schnellere Signale) trennt das Motorrad-Netz
galvanisch vom Bordcomputer. Ein nachgeschalteter Schmitt-Trigger
(74HC14) erzeugt saubere Rechteckimpulse fuer den PCNT-Eingang.

### 2.9 Tacho-Sensor (Interrupt/PCNT)

Der Tacho-Sensor verwendet einen Reed-Kontakt am Vorderrad, der bei
jeder Radumdrehung einen Impuls erzeugt. Alternativ kann das
Tacho-Kabel der PD06 abgegriffen werden (mechanischer Tachoantrieb
mit induktivem Aufnehmer).

```
Komponente              ESP32-S3 GPIO    Funktion
──────────────────────────────────────────────
Reed-Kontakt / Pickup   GPIO16           PCNT1-Eingang (IRQ)
```

Reed-Kontakt mit Pull-Up (10k nach 3.3V). Bei jeder Radumdrehung
schliesst der Reed-Kontakt kurz und zieht GPIO16 auf LOW. Der PCNT
zaehlt die Flanken. Aus Radumfang und Impulszaehler wird die
Geschwindigkeit berechnet.

### 2.10 Kuehlmitteltemperatur-Sensor (ADC)

Ein NTC-Thermistor (10k bei 25°C, B3950) im Kuehlmittelkreislauf
misst die Motortemperatur. Der NTC ist Teil eines Spannungsteilers.

```
3.3V ──[10k Referenz-R]──┬── GPIO2 (ADC1_CH1)
                           │
                        [NTC 10k]
                           │
GND ──────────────────────┘
```

Bei 25°C: NTC = 10k → Spannungsteiler 1:1 → 1.65V am ADC.
Bei 100°C: NTC ≈ 1k → 0.3V am ADC.
Bei 0°C: NTC ≈ 33k → 2.5V am ADC.

Auswertung: Steinhart-Hart-Gleichung (siehe firmware-architektur.md,
Abschnitt 3.2). 16x Oversampling fuer Rauschunterdrueckung, EMA-Filter
mit alpha=0.1 fuer Glaettung.

### 2.11 Kraftstoffstand-Sensor (ADC)

Der Kraftstoffstandsensor ist ein variabler Widerstand im Tank
(Honda-typisch 5-90 Ohm). Er wird als Spannungsteiler ausgelsen.

```
3.3V ──[100 Ohm Referenz-R]──┬── GPIO4 (ADC1_CH3)
                               │
                        [Tankgeber 5-90 Ohm]
                               │
GND ──────────────────────────┘
```

Bei leerem Tank (90 Ohm): 3.3V * 90/(100+90) = 1.56V.
Bei vollem Tank (5 Ohm): 3.3V * 5/(100+5) = 0.16V.

Kalibrierung gegen Tankstreifen (Brennweiten-Messung) fuer genaue
Liter-Anzeige. Lookup-Table im Flash fuer nichtlineare Tankgeometrie.

### 2.12 Oeldruck-Sensor (ADC)

Ein analoger Oeldrucksensor (0-10 bar, 0.5-4.5V Ausgang) misst den
Oeldruck. Die Ausgangsspannung wird direkt am ADC gemessen (der
Spannungsbereich 0.5-4.5V muss auf unter 2.9V begrenzt werden, daher
ein Spannungsteiler).

```
Sensor-Ausgang ──[10k]──┬── GPIO5 (ADC1_CH4)
                          │
                       [10k]
                          │
GND ─────────────────────┘
```

Spannungsteiler 1:2: 4.5V → 2.25V am ADC (im 12dB-Bereich).
Auswertung: oil_bar = (adc_voltage * 2 - 0.5) / 4.0 * 10.0

### 2.13 Bordspannung-Messung (ADC)

Die Bordspannung (12-15V) wird ueber einen Spannungsteiler gemessen,
um den Ladezustand der Motorrad-Batterie zu ueberwachen.

```
12V Bordnetz ──[100k]──┬── GPIO1 (ADC1_CH0)
                         │
                      [10k]
                         │
GND ────────────────────┘
```

Spannungsteiler 10:1: 15V → 1.5V am ADC.
Auswertung: vbat = adc_voltage * 10.0 / 1000.0 (in Volt)
Generator-Status: vbat > 13.8V = Generator laeuft.

### 2.14 Taster/Buttons (Menu, Select, Back)

Drei Taster fuer die Menue-Steuerung. Active-LOW mit internem Pull-Up.

```
3.3V ──[Pull-Up intern]──┬── GPIO6 (BTN_MENU)
                          │
                     [Taster]
                          │
GND ──────────────────────┘

(Analog fuer GPIO7/BTN_SELECT und GPIO3/BTN_BACK)
```

Entprellung in Software: Moving-Average oder Debounce-Timer (20ms).
Alle drei Taster koennen als RTC-GPIO den ESP32 aus dem Light-Sleep
wecken.

### 2.15 Status-LED

Eine einfache gruene LED zeigt den Betriebszustand an (Heartbeat,
BLE-Status, Fehler).

```
3.3V ──[330 Ohm]──[LED gruen]── GPIO21 (STATUS_LED)
```

LOW = LED an, HIGH = LED aus (Active-LOW, stromsparend).
PWM moeglich fuer dimmen/helligkeit.

### 2.16 Buzzer/Piezo

Ein Piezo-Buzzer fuer akustische Warnungen (Uebertemperatur, niedriger
Kraftstoffstand).

```
3.3V ──[Piezo Buzzer]── GPIO38 (BUZZER)
```

Angesteuert ueber LEDC-PWM (2-4 kHz, je nach Piezo-Resonanz).

### 2.17 USB-C (Power + Serial)

USB-C bietet Stromversorgung (5V) und serielle Kommunikation
(flashen, Debugging, Diagnose-Interface).

```
USB-C Pin     ESP32-S3 GPIO    Funktion
──────────────────────────────────────────────
VBUS          5V Rail          Power Input (5V)
GND           GND              Ground
D+            GPIO19           USB-OTG Data+
D-            GPIO20           USB-OTG Data-
CC1/CC2       5.1k Pull-Down    USB-C Konfiguration (Device-Mode)
```

Der ESP32-S3 hat einen integrierten USB-Serial-Controller. Ueber
USB-C kann direkt geflasht und debuggt werden, ohne einen externen
USB-UART-Bridge-Chip. Die 5V von VBUS speisen den 5V-Rail, wenn kein
Motorrad-Strom anliegt (Service-Modus).

---

## 3. Bus-Konflikte und Aufloesung

### 3.1 SPI-Bus-Architektur

Alle drei SPI-Geraete (TFT, E-Paper, SD-Karte) teilen sich einen
gemeinsamen SPI2 (FSPI) Bus. Das ist moeglich, weil:

1. Jedes Geraet einen eigenen Chip-Select (CS) hat
2. TFT und E-Paper write-only sind (kein MISO-Bus-Konflikt)
3. Nur die SD-Karte MISO verwendet (Tri-State bei CS HIGH)
4. Die Geraete nicht gleichzeitig aktiv sind

```
         GPIO11 (MOSI) ──┬──── TFT (SDI)
                          ├──── EPD (DIN)
                          └──── SD  (MOSI)

         GPIO12 (SCK)  ──┬──── TFT (SCK)
                          ├──── EPD (CLK)
                          └──── SD  (SCK)

         GPIO13 (MISO) ────── SD  (MISO)    [nur SD!]

CS: GPIO10 ──────────────── TFT_CS (Active-LOW)
CS: GPIO40 ──────────────── EPD_CS (Active-LOW)
CS: GPIO47 ──────────────── SD_CS  (Active-LOW)
```

**Wichtig:** Bei Multidrop-SPI muessen alle CS-Leitungen beim
SPI-Transfer korrekt verwaltet werden. Nur ein Geraet darf CS=LOW
haben. Der ESP-IDF SPI-Treiber macht das automatisch, wenn fuer jedes
Geraet ein eigener spi_device_handle_t mit entsprechendem CS-Pin
angemeldet wird.

**Timing:** TFT und SD koennen am selben Bus betrieben werden, da
SD-Transfers asynchron (nicht zeitkritisch) sind. Das E-Paper wird
nur periodisch (alle 300ms-2s) aktualisiert. Konflikte sind
unwahrscheinlich, aber der SPI-Treiber muss Mutex-geschuetzt sein.

### 3.2 Alternative: Zwei SPI-Busse

Falls der shared-Bus-Ansatz zu Performance-Problemen fuehrt, kann
der ESP32-S3 einen zweiten SPI-Bus (SPI3/HSPI) nutzen:

| Bus   | Geraete           | Pins                    |
|-------|-------------------|-------------------------|
| SPI2  | TFT + E-Paper     | MOSI=11, SCK=12, MISO=13|
| SPI3  | SD-Karte          | MOSI=35, SCK=36, MISO=37, CS=38 |

**Aber:** GPIO35-37 sind bei der N16R8-Variante (Octal-PSRAM) intern
fuer PSRAM belegt und nicht extern verfuegbar. Bei der N8R2-Variante
(Quad-PSRAM, kein Octal) waeren diese Pins frei. Mit N16R8 ist der
Shared-Bus-Ansatz die einzige Option.

### 3.3 I2C-Bus

Der I2C0-Bus ist einfach: zwei Geraete mit unterschiedlichen Adressen.

```
I2C0 Bus:
  SDA (GPIO8) ──┬── MPU6050 (0x68)
                └── BMP280  (0x76)
  SCL (GPIO9) ──┬── MPU6050
                └── BMP280
```

Kein Adress-Konflikt. Beide Geraete koennen beliebig oft angesprochen
werden. Der Bus ist 400-kHz-Fast-Mode-faehig.

Pull-Ups: 4.7k nach 3.3V auf SDA und SCL. Die ESP32-S3 internen
Pull-Ups (ca. 45k) sind zu schwach fuer 400 kHz mit zwei Geraeten.

### 3.4 UART

UART1 ist exklusiv fuer das GPS-Modul. UART0 ist der USB-CDC-Port
fuer Flashing/Debug. Keine Bus-Konflikte.

### 3.5 Konfliktfreie Pins

Die folgende Tabelle zeigt, welche Pins konfliktfrei fuer zusaetzliche
Erweiterungen verfuegbar waeren (aktuell alle zugewiesen):

| GPIO | Status      | Notiz                        |
|------|-------------|------------------------------|
| 0    | Belegt     | BOOT_BTN (Strapping)         |
| 3    | Belegt     | BTN_BACK (Strapping)         |
| 45   | Frei       | Strapping, nicht verwenden   |
| 46   | Frei       | Strapping, nicht verwenden   |

GPIO45 und GPIO46 sind die einzigen physikalisch freien Pins, duerfen
aber wegen Strapping-Bedingungen nicht frei verwendet werden. Der
Bordcomputer hat somit keine freien Pins mehr. Fuer Erweiterungen
wird ein I2C-Port-Expander (PCF8574, Adresse 0x20-0x27) empfohlen.

---

## 4. Power-Management-Schaltung

### 4.1 Uebersicht

```
12V Bordnetz
    │
    ├──[Sicherung 1A]──[TVS SMCJ24A]──[100µF Elko]──┐
    │                                               │
    ├──[Schottky 1N5822] (Rev-Pol) ────────────────┐│
    │                                              ││
    │         ┌────────────────────────────────────┘│
    │         │                                     │
    │    [MP1584EN Buck]                           │
    │    12V → 5V / 2A                             │
    │         │                                     │
    │    5V Rail ──┬── ILI9488 TFT (5V)            │
    │              ├── USB-C VBUS (5V)             │
    │              └── [HT7333 LDO]                 │
    │                   5V → 3.3V / 250mA           │
    │                        │                      │
    │                   3.3V Rail                   │
    │                   ├── ESP32-S3                │
    │                   ├── MPU6050                  │
    │                   ├── BMP280                  │
    │                   ├── AT6558 GPS              │
    │                   ├── SD-Karte                │
    │                   └── E-Paper                 │
    │                                              │
    └──[Spannungsteiler 10:1]── GPIO1 (ADC1_CH0)   │
       Bordspannung messen                          │
```

### 4.2 12V → 5V Step-Down (Buck Converter)

**Bauteil:** MP1584EN oder LM2596

Die Bordspannung der PD06 schwankt zwischen 8V (beim Anlassen) und
15V (hohe Drehzahl). Zusaetzlich treten Lastabwurf-Spikes bis 100V
auf. Der Buck-Converter muss diesen Bereich abdecken.

```
Eingang:        8-28V (MP1584) oder 7-40V (LM2596)
Ausgang:        5.0V ± 0.1V
Max. Strom:     2A (bentigt: ~500mA峰值)
Effizienz:      90-92% (MP1584), 75-82% (LM2596)
Schaltfreq:     1.5 MHz (MP1584), 150 kHz (LM2596)
```

**Schutzbeschaltung am Eingang:**
- Sicherung 1A (Flachsicherung oder PTC resettable) als Erstschutz
- TVS-Diode SMCJ24A (24V Standoff, 38.9V Clamp) gegen Lastabwurf
- 100µF/25V Elektrolyt-Kondensator als Puffer
- 100nF Keramik-Kondensator gegen Hochfrequenz

**Reverse-Polarity-Schutz:**
- P-Kanal MOSFET (IRF4905) als High-Side-Switch
- RDS(on) = 0.02 Ohm → Verlust bei 500mA: 5mW (vernachlaessigbar)
- Alternative: Schottky-Diode 1N5822 (0.3V Drop, 2.75W bei 5A)

### 4.3 5V → 3.3V LDO

**Bauteil:** HT7333 (oder AMS1117-3.3)

```
Eingang:        5V (vom Buck)
Ausgang:        3.3V ± 0.05V
Max. Strom:     250mA (HT7333) oder 1A (AMS1117)
Quiescent:      4µA (HT7333) oder 5mA (AMS1117)
Rauschen:       30µVrms (HT7333)
```

Der HT7333 wird wegen seines niedrigen Rauschens (wichtig fuer
ADC-Genauigkeit) und der geringen Quiescent-Current gewaehlt. Bei
ESP32-S3-Peak-Strom (~500mA fuer 20µs beim WiFi-Transmit) reicht
250mA Dauerstrom mit einem 100µF Puffer-Elko am Ausgang.

**Kondensatoren:**
- Eingang: 10µF Tantal
- Ausgang: 10µF Tantal + 100nF Keramik (nahe am ESP32-S3 3V3-Pin)

### 4.4 Batterie-Backup fuer RTC

Der ESP32-S3 hat eine interne RTC, die im Deep-Sleep weiterlaeuft.
Bei vollstaendigem Stromverlust (Batterie abgeklemmt) geht die
RTC-Zeit verloren.

**Backup-Loesung:**
- Li-Ion 18650 (3400 mAh) als Backup-Akku
- TP4056 Ladung von 5V-Rail
- MT3608 Boost: 3.7V → 5V
- Schottky-Diode (1N5822) fuer Power-Path-ORing

```
5V (Buck) ──[1N5822]──┬── 5V Rail
                       │
TP4056 OUT ──[1N5822]──┘
     ↑
  18650 Akku (3.7V)
     ↓
MT3608 Boost → 5V → [1N5822] → Power-Path
```

Bei laufendem Motor: 5V vom Buck versorgt alles + laedt Akku.
Bei Motor aus: Akku → MT3608 → 5V → versorgt Bordcomputer.
Backup-Laufzeit: ~20h voll aktiv, ~18 Tage mit GPS-Tracking.

### 4.5 Ueberstromschutz

Eine Flachsicherung (1A, ATO/ATC) schuetzt den Bordcomputer-Gesamtstrom.
Zusaetzlich PTC-Resettable-Sicherung (0.5A) als sekundaerer Schutz
fuer Ueberstrom am 3.3V-Rail.

### 4.6 Reverse-Polarity-Schutz

P-Kanal MOSFET (IRF4905) am 12V-Eingang als High-Side-Switch.
Gate wird ueber 10k Pull-Up nach Source (12V) gezogen.
Source = 12V-Eingang, Drain = Buck-Converter-Eingang.
Bei korrekter Polaritaet: MOSFET leitet (V_GS < 0).
Bei Verpolung: V_GS = 0, MOSFET sperrt.

### 4.7 Spannungsteiler fuer Bordspannungsmessung

```
12V Bordnetz ──[100k 1%]──┬── GPIO1 (ADC1_CH0)
                            │
                         [10k 1%]
                            │
GND ────────────────────────┘
```

Teilerverhaeltnis 10:1. Bei 15V Bordnetz: 1.5V am ADC.
1%-Widerstaende fuer genaue Messung (±0.1V bei 15V).
Zusaetzlich 100nF Kondensator am ADC-Pin gegen Rauschen.

---

## 5. Sensor-Konditionierung

### 5.1 Kuehlmitteltemperatur: NTC-Spannungsteiler

```
3.3V ──[10k 1% Ref]──┬── GPIO2 (ADC1_CH1)
                       │
                    [NTC 10k B3950]
                       │
GND ──────────────────┘
```

NTC-Widerstand bei verschiedenen Temperaturen:
- 0°C:  ~33k → 2.55V am ADC
- 25°C: 10k  → 1.65V am ADC
- 50°C: ~3.6k → 0.88V am ADC
- 80°C: ~1.2k → 0.36V am ADC
- 100°C: ~680 Ohm → 0.21V am ADC
- 120°C: ~420 Ohm → 0.13V am ADC

Auswertung: Steinhart-Hart-Gleichung:
1/T = 1/T0 + (1/B) * ln(R/R0)
mit T0 = 298.15K (25°C), R0 = 10k, B = 3950.

16x Oversampling am ADC fuer Rauschminderung. EMA-Filter alpha=0.1
fuer Glaettung. Warngrenze: 110°C (Warnung), 120°C (Kritisch).

### 5.2 Kraftstoffstand: Spannungsteiler mit variablen Widerstand

```
3.3V ──[100 Ohm Ref]──┬── GPIO4 (ADC1_CH3)
                        │
                   [Tankgeber 5-90 Ohm]
                        │
GND ────────────────────┘
```

Honda-typische Tankgeber: 5 Ohm (voll) bis 90 Ohm (leer).
Bei 100 Ohm Referenz:
- Voll (5 Ohm): V = 3.3 * 5/105 = 0.16V
- Halb (47 Ohm): V = 3.3 * 47/147 = 1.05V
- Leer (90 Ohm): V = 3.3 * 90/190 = 1.56V

Kalibrierung: Lookup-Table im Flash, da Tankgeometrie nicht linear
ist. EMA-Filter alpha=0.05 fuer starke Glaettung (Kraftstoffstand
aendert sich langsam). Reserve-Warnung bei < 2 Liter.

### 5.3 Oeldruck: ADC-Signal 0.5-4.5V

```
Sensor-Ausgang (0.5-4.5V) ──[10k]──┬── GPIO5 (ADC1_CH4)
                                     │
                                  [10k]
                                     │
GND ────────────────────────────────┘
```

Spannungsteiler 1:2 am ADC (Sensor 0.5-4.5V → ADC 0.25-2.25V).
Auswertung: oil_bar = (adc_voltage * 2.0 - 0.5) / 4.0 * 10.0

Typische Oeldruecke:
- Leerlauf warm: 0.5-1.5 bar
- Fahrt: 3-5 bar
- Kaltstart: bis 6 bar

Warn-Schwellen:
- < 0.5 bar bei laufendem Motor → Kritisch (Oelmangel)
- > 6 bar → Warnung (Oeldruck-Regler defekt)

### 5.4 Drehzahl: Optokoppler + Schmitt-Trigger

```
Zündspule / CDI Pickup
    │
    ├──[Vorwiderstand 10k]──[Zener 5.1V]──┐
    │                                      │
    │                              [Optokoppler PC817]
    │                              LED-Seite
    │                                      │
    │                              [Transistor-Seite]
    │                              Collector ── 3.3V
    │                              Emitter ───┬── GPIO15 (PCNT0)
    │                                          │
    │                                      [10k Pull-Up]
    │                                          │
    │                              GND ────────┘
    │
    └──[74HC14 Schmitt-Trigger]── GPIO15 (sicher konfiguriert)
```

Funktionsweise:
1. Die CDI-Zuendung erzeugt einen Hochspannungsimpuls pro
   Zuendvorgang (2 Zylinder, 1 Impuls pro 360° Kurbelwellenumlauf)
2. Das Signal wird ueber einen induktiven Pickup am Zuendkabel
   abgegriffen
3. Der Vorwiderstand und die Zener-Diode begrenzen die Spannung
4. Der Optokoppler (PC817) trennt das Motorrad-Netz galvanisch
5. Der Schmitt-Trigger (74HC14) entprellt und erzeugt saubere
   Rechteckimpulse
6. Der PCNT zaehlt die Impulse → Drehzahl = Pulse * 120 / Zylinder

Bei V2-Motor (2 Zylinder): 1 Impuls pro Umdrehung.
Bei 3000 rpm = 50 Impulse/s = 50 Hz.

Alternative fuer hohe Drehzahlen: 6N137 Optokoppler (10 MHz, schnell).

### 5.5 Tacho: Reed-Kontakt + Pull-Up

```
3.3V ──[10k Pull-Up]──┬── GPIO16 (PCNT1)
                       │
                  [Reed-Kontakt]
                       │
GND ───────────────────┘
```

Ein Reed-Kontakt am Vorderrad schliesst bei jeder Radumdrehung
(magnetischer Pickup am Felgen-/Speichenmagnet). Der PCNT zaehlt
die Impulse.

Reifenumfang Transalp 600 (110/80-18):
- Abrollumfang ≈ 2.07 m
- Bei 100 km/h: 100000/3600/2.07 = 13.4 Umdrehungen/s = 13.4 Hz
- Bei 200 km/h (unrealistisch): 26.8 Hz

Reed-Kontakte sind bis ca. 50 Hz zuverlaessig. Fuer hoehere
Geschwindigkeiten oder feinere Aufloesung: Hall-Sensor (z.B. A3144)
mit mehreren Magneten.

RC-Tiefpass (1k + 100nF = 1.6 kHz) am Eingang gegen EMV-Stoerungen.

---

## 6. ASCII-Art Schaltplan

### 6.1 Block-Uebersicht

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        BORDCOMPUTER TRANSALP 600                        │
│                        (Gesamt-Blockdiagramm)                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌──────────┐   ┌───────────────────────────────────────────┐          │
│  │ 12V Bord │   │              ESP32-S3-WROOM-1            │          │
│  │  Netz    │   │           (N16R8, Dual-Core 240MHz)      │          │
│  │          │   │                                           │          │
│  │  8-15V   │   │  GPIO0  ─── BOOT_BTN                     │          │
│  └────┬─────┘   │  GPIO1  ─── BATT_VOLTAGE (ADC)           │          │
│       │         │  GPIO2  ─── COOLANT_TEMP (ADC/NTC)       │          │
│  ┌────▼─────┐   │  GPIO3  ─── BTN_BACK                     │          │
│  │Sicherung │   │  GPIO4  ─── FUEL_LEVEL (ADC)             │          │
│  │   1A     │   │  GPIO5  ─── OIL_PRESSURE (ADC)           │          │
│  └────┬─────┘   │  GPIO6  ─── BTN_MENU                     │          │
│       │         │  GPIO7  ─── BTN_SELECT                   │          │
│  ┌────▼─────┐   │  GPIO8  ─── I2C SDA ──┐                  │          │
│  │TVS Diode│   │  GPIO9  ─── I2C SCL ──┤                  │          │
│  │SMCJ24A  │   │  GPIO10 ─── TFT_CS ───┤ FSPI             │          │
│  └────┬─────┘   │  GPIO11 ─── MOSI ────┤ (Shared          │          │
│       │         │  GPIO12 ─── SCK ────┤  SPI2)           │          │
│  ┌────▼─────┐   │  GPIO13 ─── MISO ───┤                  │          │
│  │MP1584EN │   │  GPIO14 ─── TFT_DC    │                  │          │
│  │  Buck    │   │  GPIO15 ─── RPM_PULSE (PCNT0)          │          │
│  │ 12V→5V  │   │  GPIO16 ─── TACHO_PULSE (PCNT1)         │          │
│  └────┬─────┘   │  GPIO17 ─── TFT_RST                    │          │
│       │5V       │  GPIO18 ─── TFT_BL (PWM)               │          │
│  ┌────▼─────┐   │  GPIO19 ─── USB_DP                      │          │
│  │HT7333    │   │  GPIO20 ─── USB_DM                      │          │
│  │  LDO     │   │  GPIO21 ─── STATUS_LED                  │          │
│  │ 5V→3.3V │   │  GPIO38 ─── BUZZER (PWM)                 │          │
│  └────┬─────┘   │  GPIO39 ─── GPS_PPS (IRQ)              │          │
│       │3.3V     │  GPIO40 ─── EPD_CS ────┤ FSPI          │          │
│  ┌────▼─────────┐  GPIO41 ─── EPD_DC     │               │          │
│  │  Alle Module │  GPIO42 ─── EPD_RST    │               │          │
│  │  versorgt    │  GPIO43 ─── GPS_TX (UART1)             │          │
│  └──────────────┘  GPIO44 ─── GPS_RX (UART1)             │          │
│                     GPIO47 ─── SD_CS ────┤ FSPI          │          │
│                     GPIO48 ─── MPU6050_INT (IRQ)         │          │
│                       └───────────────────────────────────┘          │
│                                                                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │ILI9488   │  │E-Paper   │  │MicroSD   │  │ AT6558   │             │
│  │TFT 3.5"  │  │2.13"     │  │  Slot    │  │  GPS     │             │
│  │SPI (FSPI)│  │SPI(FSPI) │  │SPI(FSPI) │  │ UART1    │             │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘             │
│                                                                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │ MPU6050  │  │ BMP280   │  │ Buttons  │  │ USB-C    │             │
│  │  IMU     │  │  Baro    │  │  x3      │  │ Pwr+Ser  │             │
│  │ I2C      │  │ I2C      │  │ GPIO     │  │ USB-OTG  │             │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘             │
│                                                                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │RPM Sensor│  │Tacho     │  │Coolant   │  │Fuel/Oil  │             │
│  │Opto+PCNT │  │Reed+PCNT │  │NTC+ADC   │  │ADC       │             │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘             │
│                                                                         │
│  ┌──────────┐  ┌──────────┐                                           │
│  │Status LED│  │ Buzzer   │                                           │
│  └──────────┘  └──────────┘                                           │
└─────────────────────────────────────────────────────────────────────────┘
```

### 6.2 Vollstaendiger Schaltplan (ASCII-Art)

```
                             12V Bordnetz (PD06)
                                 │
                    ┌────────────┼────────────────────┐
                    │            │                    │
               ┌────▼────┐  ┌───▼────┐         ┌─────▼─────┐
               │Sicherung│  │TVS Diode│         │ Spannung  │
               │  1A     │  │SMCJ24A │         │ teiler    │
               └────┬────┘  └───┬────┘         │ 100k/10k  │
                    │           │              └─────┬─────┘
               ┌────▼────┐  ┌───▼────┐               │
               │ Reverse │  │100µF   │               │
               │ Polarity│  │ Elko   │               │
               │ IRF4905 │  └───┬────┘               │
               └────┬────┘      │                    │
                    │      ┌────▼────┐               │
                    └──────┤MP1584EN │               │
                           │  Buck   │               │
                           │12V→5V  │               │
                           └────┬────┘               │
                                │ 5V                  │
                    ┌───────────┼───────────┐        │
                    │           │           │        │
               ┌────▼────┐ ┌───▼───┐ ┌────▼────┐    │
               │ 5V Rail │ │USB-C  │ │ HT7333  │    │
               │         │ │ VBUS  │ │  LDO    │    │
               │ TFT(5V) │ └───────┘ │5V→3.3V  │    │
               └─────────┘           └────┬────┘    │
                                          │ 3.3V    │
                    ┌─────────────────────┼─────────┘
                    │                     │
              ┌─────▼─────────────────────▼─────┐
              │      ESP32-S3-WROOM-1           │
              │      (N16R8)                    │
              │                                 │
              │  3V3 ●● GND                     │
              │  EN  ●  IO0 ──[10k PU]──3.3V   │
              │       ●  IO0 ──[BOOT_BTN]──GND │
              └──┬───┬───┬───┬───┬───┬───┬───┬──┘
                 │   │   │   │   │   │   │   │
    ┌────────────┘   │   │   │   │   │   │   └──────────────┐
    │   ┌────────────┘   │   │   │   │   └────────┐         │
    │   │   ┌────────────┘   │   │   └──────────┐ │         │
    │   │   │      ┌──────────┘   └────────────┐ │ │         │
    │   │   │      │             ┌─────────────┘ │         │
    │   │   │      │             │   ┌────────────┘         │
    │   │   │      │             │   │                      │
    ▼   ▼   ▼      ▼             ▼   ▼                      ▼
   ADC1  ADC1  ADC1            ADC1 ADC1                    ADC1
   GPIO1 GPIO2 GPIO4           GPIO5                        GPIO1
   Batt  Cool  Fuel            Oil                          (Batt
                                           ┌─────────────────  Teiler)
   ┌─────┐ ┌──────┐ ┌──────────┐           │
   │10:1 │ │10k   │ │100 Ohm   │           │
   │Teil.│ │ NTC  │ │ Tankgeber│           │
   │Bord │ │10k   │ │ 5-90 Ohm │           │
   │Netz │ │ Ref  │ │ Ref      │           │
   └─────┘ └──────┘ └──────────┘           │
                                           │
   ┌───────────────────────────────────────┘
   │
   │     ┌─────────────────────────────────────────┐
   │     │           FSPI (SPI2) Shared Bus       │
   │     │                                         │
   │     │  MOSI (GPIO11) ──┬── TFT_SDI           │
   │     │                  ├── EPD_DIN           │
   │     │                  └── SD_MOSI           │
   │     │                                         │
   │     │  SCK  (GPIO12) ──┬── TFT_SCK           │
   │     │                  ├── EPD_CLK           │
   │     │                  └── SD_SCK            │
   │     │                                         │
   │     │  MISO (GPIO13) ──── SD_MISO (nur SD)   │
   │     │                                         │
   │     │  CS: GPIO10 ─── TFT_CS                 │
   │     │  CS: GPIO40 ─── EPD_CS                 │
   │     │  CS: GPIO47 ─── SD_CS                  │
   │     └─────────────────────────────────────────┘
   │
   │     ┌─────────────────────────────────────────┐
   │     │           I2C Bus 0                    │
   │     │                                         │
   │     │  SDA (GPIO8) ──┬── MPU6050 SDA          │
   │     │                └── BMP280 SDA           │
   │     │                                         │
   │     │  SCL (GPIO9) ──┬── MPU6050 SCL         │
   │     │                └── BMP280 SCL           │
   │     │                                         │
   │     │  Pull-Ups: 4.7k nach 3.3V auf SDA+SCL │
   │     │  Adressen: MPU6050=0x68, BMP280=0x76   │
   │     └─────────────────────────────────────────┘
   │
   │     ┌─────────────────────────────────────────┐
   │     │           UART1 (GPS)                   │
   │     │                                         │
   │     │  TX (GPIO43) ─── AT6558 RX              │
   │     │  RX (GPIO44) ─── AT6558 TX              │
   │     │  PPS (GPIO39) ─── AT6558 PPS (IRQ)     │
   │     │  Baud: 115200                            │
   │     └─────────────────────────────────────────┘
   │
   │     ┌─────────────────────────────────────────┐
   │     │    Pulse-Counter / Interrupts           │
   │     │                                         │
   │     │  PCNT0 (GPIO15) ── Optokoppler (RPM)   │
   │     │  PCNT1 (GPIO16) ── Reed-Kontakt (Tacho)│
   │     │  INT  (GPIO48) ── MPU6050_INT (IMU)   │
   │     └─────────────────────────────────────────┘
   │
   │     ┌─────────────────────────────────────────┐
   │     │    Display-Steuerung                   │
   │     │                                         │
   │     │  TFT_DC  (GPIO14) ── ILI9488 DC        │
   │     │  TFT_RST (GPIO17) ── ILI9488 RST       │
   │     │  TFT_BL  (GPIO18) ── ILI9488 BL (PWM) │
   │     │  EPD_DC  (GPIO41) ── E-Paper DC        │
   │     │  EPD_RST (GPIO42) ── E-Paper RST       │
   │     └─────────────────────────────────────────┘
   │
   │     ┌─────────────────────────────────────────┐
   │     │    Buttons / Status / USB              │
   │     │                                         │
   │     │  BTN_MENU   (GPIO6)  ──[Taster]── GND  │
   │     │  BTN_SELECT (GPIO7)  ──[Taster]── GND  │
   │     │  BTN_BACK   (GPIO3)  ──[Taster]── GND  │
   │     │  STATUS_LED (GPIO21) ──[330R]──[LED]   │
   │     │  BUZZER     (GPIO38) ──[Piezo]── GND   │
   │     │  USB_DP     (GPIO19) ── USB-C D+       │
   │     │  USB_DM     (GPIO20) ── USB-C D-       │
   │     └─────────────────────────────────────────┘
   │
   │
   └─── Siehe PINMAP.md fuer vollstaendige Pin-Tabelle
```

### 6.3 Power-Path Detail

```
        12V Bordnetz
             │
    ┌────────┴────────┐
    │                 │
┌───▼───┐        ┌───▼────────┐
│ 1A    │        │Spannungs-  │
│Sicher.│        │teiler      │
└───┬───┘        │100k/10k    │
    │            └──────┬─────┘
    │                   │
┌───▼───┐               │
│TVS    │               │
│SMCJ24A│               │
└───┬───┘               │
    │                   │
┌───▼───┐               │
│100µF  │               │
│Elko   │               │
└───┬───┘               │
    │                   │
┌───▼───┐               │
│Reverse│               │
│Pol.   │               │
│IRF4905│               │
└───┬───┘               │
    │                   │
┌───▼───┐               │
│MP1584 │               │
│ Buck  │               │
│12V→5V │               │
└───┬───┘               │
    │ 5V                 │
    ├───► USB-C VBUS     │
    ├───► TFT (5V)      │
    │                   │
┌───▼───┐               │
│HT7333 │               │
│ LDO   │               │
│5V→3.3V│               │
└───┬───┘               │
    │ 3.3V               │
    │        ┌──────────┘
    ├───► ESP32-S3
    ├───► MPU6050
    ├───► BMP280
    ├───► AT6558 GPS
    ├───► SD-Karte
    ├───► E-Paper
    ├───► NTC-Sensoren
    └───► Tankgeber
```

---

## 7. KiCad-Kompatibilitaet

### 7.1 Bauteile-Liste mit Footprint und Bestellnummer

| Bauteil              | Wert / Typ         | Footprint         | SMD/THT | Anzahl | Bestellnummer (Beispiel)  |
|---------------------|--------------------|--------------------|---------|--------|---------------------------|
| ESP32-S3-WROOM-1    | N16R8              | ESP32-S3-WROOM-1   | SMD     | 1      | 1900-ESP32-S3-WROOM-1-N16R8|
| ILI9488 TFT 3.5"    | 480x320 RGB565    | Module-PCB         | THT     | 1      | ILI9488-3.5-SPI           |
| E-Paper 2.13"       | 250x122 SSD1680   | Display-Hersteller-Modul    | THT     | 1      | WS-EPD-213-V2             |
| MicroSD Slot        | Push-Push          | MicroSD-SMD        | SMD     | 1      | DM3AT-SF-PEJM5            |
| AT6558 GPS          | Modul             | GPS-Modul-PCB      | THT     | 1      | AT6558-Breakout           |
| MPU6050             | 6-DoF IMU         | QFN-24 (Modul THT)| SMD/THT | 1      | MPU6050-Module            |
| BMP280              | Baro + Temp       | LGA-8 (Modul THT) | SMD/THT | 1      | BMP280-Module             |
| MP1584EN            | Buck 12V→5V       | SOIC-8             | SMD     | 1      | MP1584EN-SOIC8            |
| HT7333              | LDO 5V→3.3V       | SOT-89             | SMD     | 1      | HT7333-SOT89              |
| IRF4905             | P-MOSFET          | TO-220             | THT     | 1      | IRF4905PBF                |
| PC817               | Optokoppler       | DIP-4              | THT     | 1      | PC817C                    |
| 74HC14              | Schmitt-Trigger   | SOIC-14            | SMD     | 1      | 74HC14D                   |
| TVS SMCJ24A         | Ueberspannung     | SMC                | SMD     | 1      | SMCJ24A                   |
| 1N5822              | Schottky-Diode    | DO-201AD           | THT     | 2      | 1N5822                    |
| LED gruen           | 3mm               | LED D3.0mm         | THT     | 1      | LED-GREEN-3mm             |
| Piezo Buzzer        | 5V                | Buzzer-12mm        | THT     | 1      | BUZZER-5V-12mm            |
| Taster (Menu/Sel/Bk)| 6x6mm Tactile    | TACT-6x6           | THT     | 3      | TACT-6x6-4.3mm            |
| Widerstand 10k      | 1% 0805           | R0805              | SMD     | 6      | RC0805FR-0710KL           |
| Widerstand 100k     | 1% 0805           | R0805              | SMD     | 1      | RC0805FR-07100KL          |
| Widerstand 4.7k     | 1% 0805           | R0805              | SMD     | 2      | RC0805FR-074K7L           |
| Widerstand 330 Ohm  | 1% 0805           | R0805              | SMD     | 1      | RC0805FR-07330RL          |
| Widerstand 100 Ohm  | 1% 0805           | R0805              | SMD     | 1      | RC0805FR-07100RL          |
| Kondensator 100µF   | 25V Elektrolyt   | CAP-RAD-D6.3       | THT     | 2      | UVR1E101MPD               |
| Kondensator 10µF    | 16V Tantal        | TANT-B             | SMD     | 2      | T520B107M006ATE040        |
| Kondensator 100nF   | X7R 0805          | C0805              | SMD     | 6      | CC0805KRX7R9BB104         |
| Sicherung 1A        | Flachsicherung    | FUSE-ATO-MINI      | THT     | 1      | ATO-1A                    |
| USB-C Buchse        | IP67 Panel-Mount  | USB-C-PM           | THT     | 1      | USB-C-IP67-PM             |
| Steckverbinder      | AMP Superseal 6p  | CON-SUPERSEAL-6   | THT     | 4      | AMP-284510-1              |

### 7.2 SMD vs. THT

**SMD-Bauteile (auf der PCB-Hauptplatine):**
- ESP32-S3-WROOM-1 Modul
- Widerstaende (0805)
- Kondensatoren (0805, Tantal)
- MP1584EN, HT7333, 74HC14
- TVS-Diode SMCJ24A
- MicroSD-Slot

**THT-Bauteile (durchsteckmontiert):**
- ILI9488 TFT-Display (auf Header gesteckt)
- E-Paper-Modul (auf Header gesteckt)
- GPS-Modul (auf Header gesteckt)
- MPU6050/BMP280 (Module auf Header)
- Optokoppler PC817, IRF4905, Dioden
- Taster, LED, Buzzer
- Kondensatoren 100µF (Elektrolyt)
- Steckverbinder, USB-C

Die Module (Display, GPS, Sensoren) sind als Breakout-Boards auf
Lochraster-Header gesteckt. Auf einer spaeteren Custom-PCB koennen
diese als SMD-Bauteile direkt aufgelegt werden.

### 7.3 Leiterplatten-Abmessungen-Vorschlag

```
┌─────────────────────────────────────────────┐
│                                             │
│  PCB: 80mm x 50mm (双层, 1.6mm FR4)        │
│                                             │
│  ┌──────────────┐  ┌──────────┐            │
│  │  ESP32-S3    │  │  MP1584  │            │
│  │  WROOM-1     │  │  Buck    │            │
│  │  (18x25.5mm) │  │ (22x15mm)│            │
│  └──────────────┘  └──────────┘            │
│                                             │
│  ┌──────┐ ┌──────┐  ┌──────────┐           │
│  │HT7333│ │74HC14│  │  Power   │           │
│  │ LDO  │ │Schmit│  │  Input  │           │
│  └──────┘ └──────┘  │  +TVS    │           │
│                     └──────────┘           │
│  ┌──────┐ ┌──────┐  ┌──────────┐           │
│  │ SD   │ │ I2C  │  │  ADC     │           │
│  │ Slot │ │Pullup│  │  Teiler  │           │
│  └──────┘ └──────┘  └──────────┘           │
│                                             │
│  ┌────────────────────────────┐             │
│  │  Header (2.54mm Raster)   │             │
│  │  fuer Display/GPS/Sensor  │             │
│  │  Breakout-Module          │             │
│  └────────────────────────────┘             │
│                                             │
└─────────────────────────────────────────────┘

Abmessungen: 80mm x 50mm (zweilagig)
Lagen: Top (Komponenten + Signale), Bottom (GND-Polygone + Power)
Lack: Gruener Loetstopplack, weisser Siebdruck
Kupferstaerke: 35µm (1 oz) fuer Signal, 70µm fuer Power-Rails
Bohrungen: M2.5-Befestigungslöcher in den Ecken
```

**Zusammenfassung:**
- PCB-Groesse: 80 x 50 mm (zweilagig, 1.6mm FR4)
- Passend in ein Gehaeuse von ca. 100 x 60 x 35 mm
- Befestigung: 4x M2.5 mit Gummipuffern (Vibration-Entkopplung)
- Conformal Coating: Polyurethan (UR) nach Bestueckung

---

## Anhang: Quellen und Referenzen

- docs/research/02-hardware-deep-dive.md — Komponenten-Auswahl, Stromschaetzung
- docs/research/05-gehaeuse-einbau-emv.md — EMV, Vibration, Wasserdichtigkeit
- docs/research/11-firmware-architecture.md — Sensor-Pipeline, Treiber-Pins
- docs/PINMAP.md — Vollstaendige Pin-Tabelle (Referenz fuer board_config.h)
- ESP32-S3 Datasheet (MCU-Chiphersteller) — Pin-Mux, ADC-Kanaele, Strapping-Pins
- MP1584 Datasheet (MPS) — Buck-Converter Spezifikation
- HT7333 Datasheet (Holtek) — LDO Spezifikation
- PC817 Datasheet (Sharp) — Optokoppler
- ILI9488 Datasheet (Ilitek) — TFT-Controller
- SSD1680 Datasheet — E-Paper-Controller

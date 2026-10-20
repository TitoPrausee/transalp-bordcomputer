# Lenker-Controller Hardware-Konzept (CONTROLLER-HARDWARE)

> TRANSALP Bordcomputer — Honda Transalp 600 (PD06)
> Firmware-Komponente: `firmware/components/hid_controller/`
> Stand: Oktober 2026

Dieses Dokument beschreibt drei Hardware-Optionen fuer den
Lenker-Controller (Steuerknoepfe am Lenker) sowie Pin-Belegung,
Montage und Firmware-Anbindung.

## Uebersicht

Der Bordcomputer steuert die UI ueber 11 logische Buttons:

| Button-ID           | Event-Bus              | Funktion                         |
|---------------------|------------------------|----------------------------------|
| NAV_UP              | APP_EVENT_CTRL_NAV_UP  | Navigation Hoch                  |
| NAV_DOWN            | APP_EVENT_CTRL_NAV_DOWN| Navigation Runter                |
| NAV_LEFT            | APP_EVENT_CTRL_NAV_LEFT| Navigation Links                 |
| NAV_RIGHT           | APP_EVENT_CTRL_NAV_RIGHT| Navigation Rechts               |
| SELECT              | APP_EVENT_CTRL_SELECT  | Bestaetigen / OK                 |
| BACK                | APP_EVENT_CTRL_BACK    | Zurueck                          |
| HOME               | APP_EVENT_CTRL_HOME    | Hauptmenue                       |
| VOL_UP              | APP_EVENT_CTRL_VOL_UP  | Lauter                           |
| VOL_DOWN            | APP_EVENT_CTRL_VOL_DOWN| Leiser                           |
| MEDIA_NEXT          | APP_EVENT_CTRL_MEDIA_NEXT| Naechster Titel                |
| MEDIA_PREV          | APP_EVENT_CTRL_MEDIA_PREV| Vorheriger Titel               |

Die Firmware-Komponente `hid_controller` unterstuetzt drei
Transport-Pfade, die gleichzeitig aktiv sein koennen:

1. **GPIO** — Direkt verdrahtete Taster (Interrupt-basiert, 50ms Debounce)
2. **I2C** — Custom PCB mit ATmega328P/RP2040 als I2C-Slave (Poll 100ms)
3. **BLE** — Wireless Controller ueber NimBLE GATT Client

Jede der drei Hardware-Optionen nutzt einen oder mehrere dieser Pfade.

---

## Option A — SilverFox C2 (Premium, verdrahtet)

| Eigenschaft     | Wert                                      |
|-----------------|-------------------------------------------|
| Produkt         | SilverFox C2 Handlebar Controller         |
| Preis           | ~235 EUR                                  |
| Schutzart       | IP67 (wetterfest)                         |
| Eingaenge       | 5-Way Joystick + 6 Taster                 |
| Anbindung       | Kabelbaum an ESP32-S3 GPIO                |
| Stromversorgung | 3.3V vom Bordcomputer                     |
| Haltbarkeit     | Made in Finland, Motorrad-spezifiziert    |

### Anbindung

Der SilverFox C2 wird ueber den **GPIO-Pfad** angeschlossen. Da alle
GPIOs des ESP32-S3 bereits belegt sind (siehe [PINMAP.md](PINMAP.md)),
werden die vorhandenen Taster-Pins fuer den Controller gemultiplext:

| Controller-Button | GPIO | board_config.h        | PINMAP-Beschreibung   |
|-------------------|------|-----------------------|-----------------------|
| NAV_UP            | 0    | CTRL_BTN_NAV_UP_PIN   | BOOT_BTN (Strapping)  |
| SELECT            | 7    | CTRL_BTN_SELECT_PIN   | BTN_SELECT            |
| BACK              | 3    | CTRL_BTN_BACK_PIN     | BTN_BACK (Strapping)  |
| HOME             | 6    | CTRL_BTN_HOME_PIN     | BTN_MENU              |

Die restlichen Buttons (NAV_DOWN/LEFT/RIGHT, VOL_UP/DOWN,
MEDIA_NEXT/PREV) werden bei dieser Option ueber einen
**PCF8574 I2C-Port-Expander** (Adresse 0x20) auf dem vorhandenen
I2C-Bus (SDA=GPIO8, SCL=GPIO9) nachgefuehrt.

### Vor- und Nachteile

- **+** IP67 wetterfest, fuer Motorrad-Einsatz entwickelt
- **+** Keine Batterie noetig (kabelgebunden)
- **+** Geringe Latenz (GPIO-Interrupt)
- **-** Hoechster Preis der drei Optionen
- **-** Kabelbaum muss zum Lenker gefuehrt werden

---

## Option B — Chigee CGRC Multi (Budget, wireless)

| Eigenschaft     | Wert                                      |
|-----------------|-------------------------------------------|
| Produkt         | Chigee CGRC Multi Controller              |
| Preis           | ~90 EUR                                   |
| Schutzart      | Keine Angabe (Gehaeuse empfohlen)          |
| Eingaenge       | 8 Taster + Joystick                       |
| Anbindung       | BLE (Wireless)                             |
| Stromversorgung | CR2032 Knopfzelle (Batterie)              |
| Reichweite      | ~5m (BLE 5.0)                              |

### Anbindung

Der Chigee CGRC Multi nutzt den **BLE-Pfad**. Der ESP32-S3 verbindet
sich als NimBLE GATT-Client:

| Parameter           | Wert              | board_config.h       |
|---------------------|-------------------|----------------------|
| Service UUID        | 0xFFE0            | CTRL_BLE_SVC_UUID    |
| Characteristic UUID | 0xFFE1            | CTRL_BLE_CHAR_UUID    |
| Notification-Format  | 1-2 Byte Maske    | (siehe Firmware)     |

Die Button-Maske in der BLE-Notification:

| Byte | Bit | Button        |
|------|-----|---------------|
| 0    | 0   | NAV_UP        |
| 0    | 1   | NAV_DOWN      |
| 0    | 2   | NAV_LEFT      |
| 0    | 3   | NAV_RIGHT     |
| 0    | 4   | SELECT        |
| 0    | 5   | BACK          |
| 0    | 6   | HOME          |
| 0    | 7   | VOL_UP        |
| 1    | 0   | VOL_DOWN     |
| 1    | 1   | MEDIA_NEXT   |
| 1    | 2   | MEDIA_PREV   |

### Vor- und Nachteile

- **+** Kabellos — keine Kabelverbindung zum Lenker noetig
- **+** Einfache Montage (nur Halterung)
- **+** Guenstigste Option
- **-** Batterie-Wechsel noetig (CR2032, ~6 Monate Lebensdauer)
- **-** Hoehere Latenz als GPIO (~20-50ms)
- **-** Wetterfestigkeit nicht garantiert (Gehaeuse erforderlich)

---

## Option C — Custom PCB (Eigenbau, I2C)

| Eigenschaft     | Wert                                      |
|-----------------|-------------------------------------------|
| Design          | Custom PCB mit ATmega328P oder RP2040     |
| Preis           | ~15-30 EUR (Komponenten)                  |
| Schutzart      | Abhaengig vom Gehaeuse                     |
| Eingaenge       | 11 Taster (alle Buttons)                  |
| Anbindung       | I2C-Slave an ESP32-S3                      |
| Stromversorgung | 3.3V vom Bordcomputer (ueber I2C-Kabel)   |

### Anbindung

Das Custom PCB wird als **I2C-Slave** an den vorhandenen I2C-Bus
angeschlossen:

| Parameter        | Wert    | board_config.h     |
|------------------|---------|--------------------|
| I2C-Bus          | I2C0    | I2C_MASTER_NUM     |
| SDA              | GPIO8   | I2C_MASTER_SDA_IO  |
| SCL              | GPIO9   | I2C_MASTER_SCL_IO  |
| Takt             | 400 kHz | I2C_MASTER_FREQ_HZ |
| Slave-Adresse    | 0x42    | CTRL_I2C_ADDR      |
| Poll-Intervall   | 100 ms  | CTRL_I2C_POLL_MS   |
| Register         | 0x00    | (2-Byte Maske)     |

Button-Maske (16-bit, LSB first):

| Bit | Button        |
|-----|---------------|
| 0   | NAV_UP        |
| 1   | NAV_DOWN      |
| 2   | NAV_LEFT      |
| 3   | NAV_RIGHT     |
| 4   | SELECT        |
| 5   | BACK          |
| 6   | HOME          |
| 7   | VOL_UP        |
| 8   | VOL_DOWN     |
| 9   | MEDIA_NEXT   |
| 10  | MEDIA_PREV   |
| 11-15 | reserviert  |

### MCU-Wahl

| MCU        | Taktrate  | Flash | RAM  | I2C-Slave | Bemerkung                  |
|------------|-----------|-------|------|-----------|----------------------------|
| ATmega328P | 8/16 MHz  | 32 KB | 2 KB | Ja        | Klassisch, Arduino-kompatibel |
| RP2040     | 133 MHz   | 2 MB  | 264 KB | Ja      | Dual-Core, leistungsstark    |

### Vor- und Nachteile

- **+** Volle Kontrolle ueber Layout und Taster-Anordnung
- **+** Alle 11 Buttons ueber ein I2C-Kabel (4 Adern: VCC/GND/SDA/SCL)
- **+** Keine Battiere (stromversorgt vom Bordcomputer)
- **+** Kostenguenstig bei Eigenbau
- **-** Entwicklungsaufwand fuer PCB-Design
- **-** Loeten und Bestueckung erforderlich

---

## Pin-Belegung Zusammenfassung

### Direkte GPIO-Taster (Option A — verdrahtet)

Diese Pins werden fuer die direkte GPIO-Anbindung verwendet und
entsprechen den Taster-Pins aus [PINMAP.md](PINMAP.md):

| GPIO | board_config.h        | Controller-Button | Bemerkung                       |
|------|-----------------------|-------------------|---------------------------------|
| 0    | CTRL_BTN_NAV_UP_PIN   | NAV_UP            | Strapping (LOW = Download-Mode)  |
| 3    | CTRL_BTN_BACK_PIN     | BACK              | Strapping (JTAG-Select)         |
| 6    | CTRL_BTN_HOME_PIN     | HOME              | Ehemals BTN_MENU                |
| 7    | CTRL_BTN_SELECT_PIN   | SELECT            | Ehemals BTN_SELECT              |

> **Achtung:** GPIO0 und GPIO3 sind Strapping-Pins. Sie muessen beim
> Boot HIGH sein (interner Pull-Up sicherstellt das). Als Input mit
> Pull-Up und active-LOW-Taster ist die Verwendung sicher, da der
> Taster nur kurz auf LOW zieht — nicht beim Boot-Vorgang.

### I2C-Bus (Option A PCF8574 + Option C Custom PCB)

| Signal | GPIO | board_config.h       | Bus     |
|--------|------|----------------------|---------|
| SDA    | 8    | I2C_MASTER_SDA_IO    | I2C0    |
| SCL    | 9    | I2C_MASTER_SCL_IO    | I2C0    |

Teilnehmer auf dem I2C-Bus:

| Geraet              | Adresse | board_config.h       |
|---------------------|---------|----------------------|
| MPU6050 (IMU)       | 0x68    | MPU6050_I2C_ADDR     |
| BMP280 (Baro)       | 0x76    | BMP280_I2C_ADDR      |
| PCF8574 (Port-Exp)  | 0x20    | CTRL_PCF8574_ADDR    |
| Custom PCB Controller| 0x42   | CTRL_I2C_ADDR        |

### BLE (Option B — wireless)

BLE nutzt den integrierten Antennenpfad des ESP32-S3-WROOM-1.
Keine zusaetzlichen Pins erforderlich. Benoetigt NimBLE mit
Central-Role (`CONFIG_BT_NIMBLE_ROLE_CENTRAL=y` in sdkconfig.defaults).

---

## Montageanleitung PD06 (Honda Transalp 600)

### Montageposition

Der Lenker-Controller wird am linken Lenkerende montiert (gegenueber
der Kupplung). Die Transalp 600 (PD06) hat einen Lenkerdurchmesser
von **22 mm (7/8 Zoll)**.

### Option A — SilverFox C2

1. **Kabelbaum:** Vom Controller zum Bordcomputer-Gehaeuse verlegen.
   Empfohlene Leitung: 4-adrig geschirmt (VCC, GND, 2x Signal).
   Laenge ~1,2 m (Lenker → Cockpit).
2. **GPIO-Anschluss:** Die 4 direkten Taster an GPIO0/3/6/7 an den
   PD06-Header des Bordcomputer-PCBs anschliessen.
3. **PCF8574:** Auf einer kleinen Zusatzplatine im Bordcomputer-
   Gehaeuse verdrahtet, am I2C-Bus (SDA=GPIO8, SCL=GPIO9).
4. **Befestigung:** Schellenmontage am Lenker (22 mm). Kabel
   entlang des Lenkers und durch den Cockpit-Verkleidung verlegt.

### Option B — Chigee CGRC Multi

1. **Halterung:** Die mitgelieferte Lenkerhalterung (22 mm) am linken
   Lenkerende befestigen.
2. **Paarung:** Bordcomputer in den BLE-Pairing-Modus versetzen,
   Controller einschalten. Verbindung wird automatisch hergestellt.
3. **Stromversorgung:** CR2032 in das Controller-Gehaeuse einlegen.
   Lebensdauer ~6 Monate bei taeglicher Nutzung.
4. **Wetterfest:** Fuer Dauereinsatz ein zusaetzliches Silikon-
   Gehaeuse empfohlen.

### Option C — Custom PCB

1. **PCB-Design:** Taster auf einer massgeschneiderten Platine
   (50x30 mm empfohlen) im Lenker-Controller-Gehaeuse.
2. **MCU:** ATmega328P (TQFP-32) oder RP2040 auf der Platine.
   Firmware liest 11 Taster und stellt die Button-Maske als
   I2C-Slave bei Adresse 0x42 bereit.
3. **Kabel:** 4-adriges I2C-Kabel (VCC, GND, SDA, SCL) vom
   Controller zum Bordcomputer. Laenge max. 30 cm (I2C-Bus-
   Kapazitaetslimit). Bei laengeren Strecken P82B96
   I2C-Extender verwenden.
4. **Befestigung:** Gehaeuse mit Schellenmontage am Lenker (22 mm).
5. **Gehaeuse:** 3D-Druck (PETG/ABS) empfohlen. IP54 mit
   Silikondichtung erreichbar.

### Kabelwege PD06

| Strecke                  | Laenge  | Leitung              |
|--------------------------|---------|----------------------|
| Lenker → Cockpit-Gehaeuse| ~1,2 m  | 4-adrig geschirmt    |
| Cockpit → Bordcomputer   | ~0,3 m  | im Gehaeuse verlegt  |

### Wasserfestigkeit

Alle Optionen muessen fuer Motorrad-Einsatz mindestens IP54
(Spritzwasser) erfuellen. Option A (IP67) ist die einzige
werksseitig wasserdichte Loesung. Option C erfordert ein
entsprechendes Gehaeuse mit Dichtung.

---

## Firmware-Integration

### Initialisierung in `main.c`

```c
#include "hid_controller.h"

// Nach sensors_init() und ble_init():
hid_controller_init();
hid_controller_start((1 << CONTROLLER_SRC_GPIO)
                    | (1 << CONTROLLER_SRC_I2C)
                    | (1 << CONTROLLER_SRC_BLE));
```

Je nach Hardware-Option werden die entsprechenden Quellen aktiviert:

| Option | CONTROLLER_SRC_GPIO | CONTROLLER_SRC_I2C | CONTROLLER_SRC_BLE |
|--------|:-------------------:|:------------------:|:------------------:|
| A      | Ja                  | Ja (PCF8574)       | Nein               |
| B      | Nein                | Nein               | Ja                 |
| C      | Nein                | Ja                 | Nein               |

### Debounce

Alle Transport-Pfade nutzen die gemeinsame Debounce-Logik in
`controller_input.c`: 50 ms Mindestabstand zwischen zwei Tastendruecken
desselben Buttons. Rising-Edge-Detection (nur der Druck, nicht das
Loslassen, wird weitergeleitet).

### Event-Bus

Bestaetigte Tastendruecke werden ueber `app_event_dispatch()` auf den
zentralen Event-Loop gesendet (Typen `APP_EVENT_CTRL_*`, definiert in
`app_event_loop.h`). Subscriber (Display, CarPlay-Bridge) reagieren
auf die Events ohne direkte Kopplung an den Controller.

---

## Siehe auch

- [PINMAP.md](PINMAP.md) — Vollstaendige Pin-Belegung des ESP32-S3
- [HARDWARE.md](HARDWARE.md) — Uebergreifende Hardware-Dokumentation
- [FIRMWARE-GUIDE.md](FIRMWARE-GUIDE.md) — Firmware-Entwicklungsleitfaden
- `firmware/main/board_config.h` — Pin-Definitionen und Konstanten
- `firmware/components/hid_controller/` — Firmware-Komponente

# Deep Dive: Software-Stack, ESP-IDF, Treiber, UI & Datenverarbeitung

> Stand: Oktober 2026 — Recherche-Grundlage für die Firmware-Implementierung.
> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986–2000, rein mechanisch, keine ECU.
> MCU: ESP32-S3-WROOM-1, 16MB Flash, 8MB PSRAM, Dual-Core 240MHz.
> Repo: github.com/TitoPrausee/transalp-bordcomputer

## 1. ESP-IDF vs. Arduino-Framework

### 1.1 ESP-IDF (Espressif IoT Development Framework)

ESP-IDF ist das offizielle SDK von Espressif. Es basiert auf FreeRTOS und
bietet direkten Zugriff auf alle Hardware-Peripherals des ESP32-S3.

Vorteile für dieses Projekt:
- FreeRTOS nativ: Tasks, Queues, Semaphores, EventGroups sind Teil des
  Frameworks, kein Wrapper nötig
- Volle Kontrolle über Partition-Layout, OTA, NVS, TWAI, PCNT, I2C-API
- Bessere Performance: kein Arduino-Layer, direkte Register-Zugriffe
- PSRAM-Support: `MALLOC_CAP_SPIRAM` für Framebuffer und große Buffer
- idf.py menuconfig für Feintuning (FreeRTOS-Tickrate, Heap-Size, etc.)
- Production-grade: stabil, gut dokumentiert, aktiv gepflegt

Nachteile:
- Steilere Lernkurve als Arduino
- Weniger vorgefertigte Libraries — viele Treiber müssen selbst geschrieben
  oder als Component integriert werden
- Längere Build-Zeiten

### 1.2 Arduino-Framework (arduino-esp32)

Das Arduino-Framework für ESP32 (arduino-esp32) ist ein Wrapper um ESP-IDF.
Es abstrahiert FreeRTOS, Peripherals und WiFi/BLE hinter einfachen Klassen.

Vorteile:
- Schneller Einstieg, viele Tutorials
- Riesige Library-Auswahl (GxEPD2, Adafruit GFX, TinyGPS++, etc.)
- Einfacher Code für Prototyping

Nachteile:
- Weniger Kontrolle: FreeRTOS-Tasks sind hinter `loop()` versteckt
- Arduino-Layer verbraucht额外 Heap und Flash
- Partition-Layout ist fixed, OTA ist komplizierter
- Library-Konflikte (I2C-Bus-Kollisionen zwischen Libraries)
- Kein direkter Zugriff auf ESP-IDF-Features wie TWAI, PCNT neue API

### 1.3 Arduino-Libraries im ESP-IDF nutzen

Ja, das geht. Der Arduino-Core kann als ESP-IDF Component eingebunden werden:

```bash
# In components/ arduino-esp32 als Submodule
git submodule add https://github.com/espressif/arduino-esp32.git components/arduino
```

In `CMakeLists.txt`:
```cmake
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES arduino)
```

Dann sind Arduino-Klassen wie `SPIClass`, `Wire`, `Adafruit_GFX` verfügbar.
Aber: das zieht den ganzen Arduino-Layer rein, negiert viele ESP-IDF-Vorteile.

Besser: gezielt Arduino-Libraries als ESP-IDF Component portieren.
Beispiel GxEPD2: benötigt Adafruit_GFX + SPI. Das kann man als Component
einbinden und den Rest nativ in ESP-IDF schreiben.

### 1.4 Empfehlung für dieses Projekt

**ESP-IDF als Hauptframework, gezielte Arduino-Components für Display und GPS.**

Begründung:
- Bordcomputer ist ein Embedded-System mit klaren Real-Time-Anforderungen
  (GPS 10Hz, IMU 50Hz, CAN-Frame-Processing)
- TWAI, PCNT, OTA, NVS sind native ESP-IDF-Features — Arduino würde sie
  verdecken oder nur unvollständig unterstützen
- FreeRTOS-Task-Design braucht direkte Kontrolle über Core-Affinity und
  Prioritäten
- GxEPD2 für E-Paper als Component einbinden (die Library ist gut getestet
  und unterstützt alle Waveshare-Panels)
- minmea als C-Library direkt nutzen (kein Arduino-Wrapper nötig)

Quellen:
- ESP-IDF v5.4 Docs: https://docs.espressif.com/projects/esp-idf/en/v5.4.4/esp32s3/
- arduino-esp32 als Component: https://github.com/espressif/arduino-esp32/blob/master/docs/esp-idf_component.md
- GxEPD2: https://github.com/ZinggJM/GxEPD2

## 2. FreeRTOS Task-Design

### 2.1 Task-Architektur

Der ESP32-S3 hat zwei Xtensa LX7 Cores bei 240 MHz. In ESP-IDF v5.x ist
FreeRTOS SMP (Symmetric Multiprocessing) aktiv — Tasks können auf beide
Cores laufen oder per `xTaskCreatePinnedToCore()` fixiert werden.

Wichtig: In ESP-IDF ist `ulStackDepth` in **Bytes**, nicht in Words
(abweichend von Vanilla FreeRTOS).

API-Signatur:
```c
BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t pxTaskCode,      // Task-Funktion
    const char * const pcName,      // Name (für Debug)
    const uint32_t ulStackDepth,    // Stack-Size in BYTES
    void * const pvParameters,      // Parameter
    UBaseType_t uxPriority,         // Priorität (0 = idle, höhere = wichtiger)
    TaskHandle_t * const pxCreatedTask,  // Handle
    const BaseType_t xCoreID        // 0, 1, oder tskNO_AFFINITY
);
```

Quelle: https://github.com/espressif/esp-idf/blob/v5.4.3/components/freertos/esp_additions/include/freertos/idf_additions.h

### 2.2 Task-Layout für den Bordcomputer

| Task | Core | Prio | Stack | Rate | Beschreibung |
|---|---|---|---|---|---|
| `gps_task` | 1 | 5 | 4096 | 10 Hz | UART lesen, NMEA parsen, Queue senden |
| `sensor_task` | 1 | 4 | 4096 | 50 Hz | I2C: MPU6050 + BMP280, Fusion berechnen |
| `can_task` | 1 | 6 | 4096 | Event | TWAI RX-Queue, CAN-Frames dekodieren |
| `display_task` | 0 | 2 | 8192 | 1 Hz | UI rendern, Partial-Refresh |
| `logger_task` | 0 | 3 | 4096 | 5 Hz | Ringbuffer → SD-Karte (Batch-Write) |
| `ble_task` | 0 | 3 | 6144 | Event | NimBLE Stack, GATT-Services |
| `power_task` | 0 | 6 | 2048 | 1 Hz | ADC lesen, Akku-Überwachung, Sleep-Mgmt |
| `obd_task` | 1 | 2 | 4096 | 2 Hz | OBD-II PID-Polling (wenn K-Line verbunden) |

Core-Zuweisung:
- **Core 1**: Real-Time-Tasks (GPS, Sensoren, CAN) — müssen deterministisch laufen
- **Core 0**: UI, BLE, Logger, Power — weniger zeitkritisch, brauchen mehr Stack
- BLE-Stack läuft intern auf Core 0 (NimBLE-Default)

### 2.3 Inter-Task-Kommunikation

**Queues** (Datenübergabe zwischen Tasks):
```c
// GPS-Daten an Display und Logger
QueueHandle_t gps_data_queue = xQueueCreate(8, sizeof(gps_data_t));
// Sensor-Daten an Display und Logger
QueueHandle_t sensor_data_queue = xQueueCreate(16, sizeof(sensor_data_t));
// OBD-Daten an Display und Logger
QueueHandle_t obd_data_queue = xQueueCreate(4, sizeof(obd_data_t));

// Senden aus GPS-Task
gps_data_t data = { ... };
xQueueSend(gps_data_queue, &data, pdMS_TO_TICKS(10));

// Empfangen im Display-Task
gps_data_t gps;
if (xQueueReceive(gps_data_queue, &gps, pdMS_TO_TICKS(100)) == pdPASS) {
    // GPS-Daten aktualisieren
}
```

**EventGroups** (Power-States):
```c
#define BIT_ACTIVE  (1 << 0)
#define BIT_IDLE    (1 << 1)
#define BIT_SLEEP   (1 << 2)
#define BIT_SHUTDOWN (1 << 3)

EventGroupHandle_t power_event_group = xEventGroupCreate();

// Power-Task setzt State
xEventGroupSetBits(power_event_group, BIT_SLEEP);

// Andere Tasks warten auf State-Wechsel
xEventGroupWaitBits(power_event_group, BIT_SHUTDOWN,
                    pdFALSE, pdTRUE, portMAX_DELAY);
```

**Mutex** (SPI-Bus, Display + SD teilen sich Bus):
```c
SemaphoreHandle_t spi_mutex = xSemaphoreCreateMutex();

// Display-Task holt Mutex
xSemaphoreTake(spi_mutex, portMAX_DELAY);
display.refresh(false);  // Partial-Refresh
xSemaphoreGive(spi_mutex);

// Logger-Task holt Mutex
xSemaphoreTake(spi_mutex, portMAX_DELAY);
fwrite(buffer, 1, len, f);
xSemaphoreGive(spi_mutex);
```

**RingBuffer** (Roh-IMU-Daten für Crash-Detection, letzten 5s):
```c
RingbufHandle_t imu_ringbuf = xRingbufferCreate(8192, RINGBUF_TYPE_NOSPLIT);

// Sensor-Task schreibt
uint8_t imu_packet[24]; // 6x int16_t (ax,ay,az,gx,gy,gz)
xRingbufferSend(imu_ringbuf, imu_packet, sizeof(imu_packet), pdMS_TO_TICKS(1));
```

### 2.4 Stack-Size-Schätzung

Stack-Größen in ESP-IDF sind Bytes, nicht Words. Faustregeln:
- Minimaler Task (GPIO, LED): 1024 Bytes
- I2C/Sensor-Polling: 2048–4096 Bytes
- BLE-Stack (NimBLE): 4096–6144 Bytes (NimBLE braucht intern viel Stack)
- Display/Rendering: 6144–8192 Bytes (GxEPD2 nutzt lokale Buffer)
- GPS-Parser (minmea): 4096 Bytes (String-Verarbeitung)

Stack-Verbrauch zur Laufzeit prüfen:
```c
UBaseType_t high_water = uxTaskGetStackHighWaterMark(NULL);
ESP_LOGI(TAG, "Stack high water mark: %u bytes", high_water);
```

`uxTaskGetStackHighWaterMark()` gibt den minimalen verbleibenden Stack in
Bytes zurück. Wenn der Wert nahe 0 ist, droht ein Stack-Overflow.

### 2.5 Watchdog-Timer

ESP-IDF hat zwei Watchdog-Timer: Task-WDT (per Task) und Interrupt-WDT
(global).

```c
// Task beim Watchdog registrieren
esp_task_wdt_add(NULL);

// Regelmäßig Watchdog zurücksetzen (im Task-Loop)
esp_task_wdt_reset();
```

In `menuconfig`:
- `CONFIG_ESP_TASK_WDT_TIMEOUT_S` = 5 (Sekunden, Default)
- `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0` = y
- `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1` = y

Wenn ein Task den WDT nicht rechtzeitig resettet, wird ein Panic ausgelöst
und der ESP32 neugestartet (bei aktivem Core-Dump wird der Dump gespeichert).

### 2.6 Task-Überwachung: was passiert bei Crash?

- Stack-Overflow → Guru Meditation Error → Panic → Neustart
- Watchdog-Timeout → Panic → Neustart
- `CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK` setzt einen Hardware-Watchpoint
  auf das Stack-Ende → sofortiger Debug-Break bei Overflow

Für OTA-Sicherheit: bei wiederholten Crashes nach Update aktiviert ESP-IDF
den automatischen Rollback zur vorherigen Partition (siehe Abschnitt 8).

## 3. GPS-Treiber

### 3.1 UART-Konfiguration für NEO-M9N

Der NEO-M9N kommuniziert per UART. Default-Baudrate ist 38400, für 10Hz
NMEA-Output sollte auf 115200 hochgesetzt werden (UBX-Konfiguration).

```c
#include "driver/uart.h"

#define GPS_UART_NUM    UART_NUM_1
#define GPS_UART_BAUD   115200
#define GPS_UART_RX_PIN 44   // PINMAP: GPS UART RX
#define GPS_UART_TX_PIN 43   // PINMAP: GPS UART TX
#define GPS_BUF_SIZE    1024

void gps_uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate  = GPS_UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(GPS_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART_NUM, GPS_UART_TX_PIN,
                                  GPS_UART_RX_PIN,
                                  UART_PIN_NO_CHANGE,
                                  UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(GPS_UART_NUM, GPS_BUF_SIZE,
                                         GPS_BUF_SIZE, 0, NULL, 0));
}
```

### 3.2 NMEA-Parsing: minmea vs. TinyGPS++ vs. eigener Parser

| Kriterium | minmea | TinyGPS++ | Eigener Parser |
|---|---|---|---|
| Sprache | C99 (pure C) | C++ (Arduino) | C |
| Dynamischer Speicher | Nein | Nein | Nein |
| Floating-Point im Core | Nein (optional) | Ja | Optional |
| Unterstützte Sätze | GGA, GLL, GSA, GSV, RMC, VTG, ZDA, GBS, GST | GGA, GLL, RMC, VTG | Beliebig |
| ESP-IDF tauglich | Ja (nativ) | Nur mit Arduino-Core | Ja |
| Code-Größe | ~1000 Zeilen (1 Datei) | ~2000 Zeilen | Variabel |
| Lizenz | WTFPL | LGPL | Eigen |
| Quelle | github.com/kosma/minmea | github.com/mikalhart/TinyGPSPlus | — |

**Empfehlung: minmea.**

Begründung:
- Pure C, keine Abhängigkeiten, eine Source-Datei (`minmea.c` + `minmea.h`)
- Kein dynamischer Speicher → sicher in FreeRTOS-Tasks
- Kein Floating-Point im Core (vermeidet FPU-Kontext-Switches)
- Läuft direkt unter ESP-IDF, kein Arduino-Wrapper nötig
- Gut getestet (955+ Stars, aktiv gepflegt)

Quelle: https://github.com/kosma/minmea

Beispiel mit minmea:
```c
#include "minmea.h"

void gps_parse_line(const char *line) {
    switch (minmea_sentence_id(line, false)) {
    case MINMEA_SENTENCE_RMC: {
        struct minmea_sentence_rmc frame;
        if (minmea_parse_rmc(&frame, line)) {
            // Position, Geschwindigkeit, Kurs
            float lat = minmea_tocoord(&frame.latitude);
            float lon = minmea_tocoord(&frame.longitude);
            float speed_knots = minmea_tofloat(&frame.speed);
            float speed_kmh = speed_knots * 1.852;
        }
        break;
    }
    case MINMEA_SENTENCE_GGA: {
        struct minmea_sentence_gga frame;
        if (minmea_parse_gga(&frame, line)) {
            int satellites = frame.satellites_tracked;
            float hdop = minmea_tofloat(&frame.hdop);
            float altitude = minmea_tofloat(&frame.altitude);
        }
        break;
    }
    default:
        break;
    }
}
```

ESP-IDF hat auch ein eigenes NMEA-Parser-Beispiel:
`examples/peripherals/uart/nmea0183_parser` — verwendet ESP Event Loop
und UART-Event-Driver. Das ist eine Alternative zu minmea, aber weniger
flexibel und unterstützt nur GGA, GSA, GSV, RMC, GLL, VTG.

Quelle: https://github.com/espressif/esp-idf/tree/master/examples/peripherals/uart/nmea0183_parser

### 3.3 UBX-Protokoll: 10Hz, GGA+RMC aktivieren

NMEA ist textbasiert (ASCII), UBX ist binär und kompakter. Für 10Hz
Update-Rate sollte UBX verwendet werden, um den UART-Durchsatz zu minimieren.

UBX-Frame-Struktur:
```
Sync1 (0xB5) | Sync2 (0x62) | Class | ID | Length (2B) | Payload | CK_A | CK_B
```

Wichtige UBX-Nachrichten für Konfiguration:
- `CFG-RATE` (0x06 0x08): Messrate einstellen
- `CFG-MSG` (0x06 0x01): NMEA-Sätze aktivieren/deaktivieren
- `CFG-PRT` (0x06 0x00): Port-Konfiguration (Baudrate)

10Hz konfigurieren (CFG-RATE, measRate=100ms):
```c
// UBX-CFG-RATE payload: measRate(2) navRate(2) timeRef(2)
uint8_t ubx_cfg_rate[] = {
    0xB5, 0x62,       // Sync
    0x06, 0x08,       // Class=CFG, ID=RATE
    0x06, 0x00,       // Length=6
    0x64, 0x00,       // measRate=100ms (10Hz)
    0x01, 0x00,       // navRate=1
    0x01, 0x00,       // timeRef=GPS
    0x7A, 0x12        // Checksum (berechnen!)
};
uart_write_bytes(GPS_UART_NUM, ubx_cfg_rate, sizeof(ubx_cfg_rate));
```

Checksum-Berechnung:
```c
void ubx_checksum(uint8_t *msg, size_t len, uint8_t *ck_a, uint8_t *ck_b) {
    *ck_a = 0; *ck_b = 0;
    for (size_t i = 2; i < len - 2; i++) { // Skip sync bytes and checksum
        *ck_a += msg[i];
        *ck_b += *ck_a;
    }
}
```

Nur GGA und RMC aktivieren (andere abschalten):
```c
// UBX-CFG-MSG: Class=0xF0 (NMEA), ID=0x00 (GGA), rate=1 (jede Nav-Lösung)
uint8_t enable_gga[] = {
    0xB5, 0x62, 0x06, 0x01, 0x03, 0x00,
    0xF0, 0x00, 0x01,  // GGA, rate=1
    0x00, 0x00         // Checksum (berechnen!)
};
```

Alternativ: SparkFun_u-blox_GNSS_Arduino_Library als Component einbinden.
Diese kann über I2C/SPI/UART konfigurieren und bietet `setNavigationFrequency()`,
`setAutoPVT()`, etc. Muss nicht zwingend Arduino-Core benötigen, wenn man
nur die UBX-Konfiguration portiert.

Quellen:
- https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library
- https://github.com/loginov-rocks/UbxGps (Minimal-Ubx-Library für 10Hz)

### 3.4 PPS-Signal: Input-Capture für Zeitsynchronisation

Der NEO-M9N liefert einen 1PPS-Puls (1 Sekunde, präzise auf ns).
Pin: GPIO 1 (PINMAP, als IRQ konfiguriert).

```c
#include "driver/gptimer.h"
#include "esp_intr_alloc.h"

// GPIO 1 als Input mit Interrupt auf steigende Flanke
gpio_config_t pps_config = {
    .pin_bit_mask = (1ULL << 1),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_POSEDGE,
};
gpio_config(&pps_config);

// ISR-Handler registrieren
gpio_isr_handler_add(1, pps_isr_handler, NULL);
```

In der ISR: Timer-Wert speichern (für Latenz-Kompensation), Event an
Task senden (xQueueSendFromISR).

Anwendung: RTC-Zeit synchronisieren für korrekte GPX-Timestamps.
Die ESP32-S3 RTC hat ~±10ppm Drift, GPS-PPS ist ~±1ns.

### 3.5 Geschwindigkeitsberechnung: Doppler vs. Position-Delta

**Doppler (aus RMC-Satz):**
Der RMC-Satz liefert Speed over Ground in Knoten. Diese basiert auf der
Doppler-Verschiebung des GPS-Signals — sehr präzise (±0.1 m/s).

```c
float speed_kmh = minmea_tofloat(&frame.speed) * 1.852;
```

**Position-Delta:**
```c
// Distanz zwischen zwei GPS-Positionen (Haversine)
double haversine(double lat1, double lon1, double lat2, double lon2) {
    double dlat = (lat2 - lat1) * M_PI / 180.0;
    double dlon = (lon2 - lon1) * M_PI / 180.0;
    double a = sin(dlat/2) * sin(dlat/2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dlon/2) * sin(dlon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return 6371000.0 * c;  // Erdradius in Metern
}
// speed = distance / dt
```

Empfehlung: **Doppler** (RMC-Speed) für Anzeige, Position-Delta nur als
Fallback. Doppler ist genauer bei hohen Geschwindigkeiten und hat weniger
Rauschen.

### 3.6 HDOP/VDOP-Qualitätsprüfung

HDOP (Horizontal Dilution of Precision) aus GGA-Satz:
```c
float hdop = minmea_tofloat(&frame.hdop);
// HDOP < 1.0: exzellent
// HDOP 1-2: gut
// HDOP 2-5: moderat
// HDOP > 5: schlecht — Position verwerfen
```

VDOP aus GSA-Satz (minmea unterstützt GSA):
```c
case MINMEA_SENTENCE_GSA: {
    struct minmea_sentence_gsa frame;
    if (minmea_parse_gsa(&frame, line)) {
        float vdop = minmea_tofloat(&frame.vdop);
        float hdop = minmea_tofloat(&frame.hdop);
        float pdop = minmea_tofloat(&frame.pdop);
    }
    break;
}
```

Qualitäts-Flag für Logger und Display:
```c
typedef enum {
    GPS_FIX_NONE = 0,
    GPS_FIX_2D,
    GPS_FIX_3D,
    GPS_FIX_DGPS,
} gps_fix_quality_t;

// In gps_data_t speichern
gps_data_t data = {
    .fix_quality = (frame.fix_quality == 1) ? GPS_FIX_3D : GPS_FIX_NONE,
    .hdop = hdop,
    .valid = (hdop < 5.0 && frame.fix_quality >= 1),
};
```

### 3.7 Cold-Start-Optimierung: A-GPS über BLE

Assisted GPS (A-GPS) lädt Almanach- und Ephemeridendaten vom Smartphone
und sendet sie per BLE an den GPS-Empfänger. Das verkürzt den Cold-Start
von 30+ Sekunden auf unter 5 Sekunden.

Implementation mit UBX:
- Smartphone-App lädt AssistNow-Data von u-blox-Server
- App sendet Daten per BLE an ESP32
- ESP32 leitet Daten an NEO-M9N weiter (UBX-CFG-MSG als Wrapper)

Alternative: u-blox AssistNow Offline — Ephemeriden für 5 Tage gültig,
können über WiFi (wenn verfügbar) oder BLE übertragen werden.

Quelle: u-blox NEO-M9N Interface Description, Section "AssistNow"

## 4. Display-Treiber & UI

### 4.1 Library-Vergleich: GxEPD2 vs. Adafruit EPD vs. Waveshare

| Kriterium | GxEPD2 | Adafruit EPD | Waveshare-Lib |
|---|---|---|---|
| Panels | 100+ (alle Good Display + Waveshare) | ~10 (Adafruit-eigene) | Nur Waveshare-Boards |
| Partial-Refresh | Ja (differentiell) | Begrenzt | Ja (einfach) |
| Basis | Adafruit_GFX | Adafruit_GFX | Eigen (RAW) |
| Multi-Display | Ja (verschiedene CS) | Nein | Nein |
| ESP32-S3 | Getestet | Getestet | Getestet |
| Pflege | Sehr aktiv (J. Zingg) | Selten Updates | Sporadisch |
| Quelle | github.com/ZinggJM/GxEPD2 | github.com/adafruit/Adafruit_EPD | Waveshare Wiki |

**Empfehlung: GxEPD2.**

Begründung:
- Unterstützt das Waveshare 2.9" b/w Panel (GDEM029T94 / SSD1680) direkt
- Fast Partial Update (differentielle Refresh) für Live-Daten
- Paged Drawing (Picture Loop) reduziert RAM-Bedarf
- Aktive Pflege und Bugfixes
- Kompatibel mit Adafruit_GFX (Fonts, Grafik-Primitive)

Quelle: https://github.com/ZinggJM/GxEPD2

Neue Alternative: `tuanpmt/esp_epaper` — native ESP-IDF Component mit
LVGL-9-Support und smartem Partial-Refresh. Reif für Produktion, aber
weniger Panels unterstützt als GxEPD2.

Quelle: https://github.com/tuanpmt/esp_epaper

### 4.2 E-Paper Partial-Refresh: Funktionsweise und Limits

E-Paper-Displays ändern ihren Zustand durch Anlegen von Spannungsimpulsen.
Full-Refresh: alle Pixel werden Schwarz → Weiß → Schwarz getaktet (flackert),
dauert ~3s. Partial-Refresh: nur geänderte Pixel werden aktualisiert, ~0.3s.

Technisch: Der Controller (z.B. SSD1680) hat zwei RAM-Bänke:
1. Current RAM (0x24): aktueller Bildinhalt
2. Base RAM (0x26): vorheriger Bildinhalt

Bei Partial-Refresh vergleicht der Controller beide Bänke und treibt nur
die Pixel, die sich geändert haben. Das vermeidet das Flackern.

Limits:
- Ghosting: nach vielen Partial-Refreshs bleiben Reste stehen. Abhilfe:
  alle N Updates (z.B. 50) einen Full-Refresh erzwingen
- Temperaturabhängig: bei kalten Temperaturen (<10°C) wird Partial-Refresh
  langsamer und unzuverlässig. LUT (Look-Up Table) muss angepasst werden
- Nur 1-Bit (Schwarz/Weiß): keine Graustufen bei Partial-Refresh
- 3-Farben-Panels (b/w/r): kein Partial-Refresh möglich

Code mit GxEPD2:
```cpp
#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>

// 2.9" b/w Panel
GxEPD2_BW<GxEPD2_290_GDEY029T94, GxEPD2_290_GDEY029T94::HEIGHT> display(
    /*CS=*/ 10, /*DC=*/ 14, /*RST=*/ 21, /*BUSY=*/ 47);

void display_init() {
    display.init(115200, true, 2, false);  // 2ms reset für Waveshare
    display.setPartialWindow(0, 0, display.width(), display.height());
    display.setRotation(1);  // Querformat: 296x128
}

void update_speed(int speed) {
    // Partial-Refresh nur für den Geschwindigkeitsbereich
    display.setPartialWindow(0, 0, 100, 40);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        display.setFont(&FreeMonoBold24pt7b);
        display.setCursor(5, 30);
        display.printf("%d", speed);
    } while (display.nextPage());
}
```

### 4.3 Framebuffer im PSRAM

Das 2.9" Panel (296x128) braucht bei 1-Bit-Tiefe 296×128/8 = 4736 Bytes.
Das ist klein genug für internen SRAM. Aber für differentielles Update
braucht GxEPD2 einen Shadow-Buffer (gleiche Größe). Bei größeren Panels
sollte PSRAM genutzt werden.

```c
// ESP-IDF: PSRAM allozieren
uint8_t *fb = heap_caps_malloc(4736, MALLOC_CAP_SPIRAM);
```

Mit GxEPD2 als Component: die Library nutzt intern `malloc()` für den
Shadow-Buffer. Bei ESP32-S3 mit PSRAM wird das automatisch über
`heap_caps_malloc` mit `MALLOC_CAP_DEFAULT` alloziert — PSRAM ist
eingebunden, wenn in menuconfig aktiviert.

PSRAM aktivieren in menuconfig:
- `CONFIG_SPIRAM_USE_MALLOC` = y
- `CONFIG_SPIRAM_MODE_OCT` = y (Octal-PSRAM für WROOM-1 Module)

### 4.4 LVGL vs. Custom-UI

LVGL (Light and Versatile Graphics Library) ist ein vollwertiges UI-Framework
für Embedded-Systeme. Version 9 unterstützt E-Paper mit speziellen
Partial-Refresh-Strategien.

| Kriterium | LVGL 9 | Custom-UI |
|---|---|---|
| Lernkurve | Mittel | Gering |
| Flexibilität | Hoch (Widgets, Layouts) | Niedrig (manuell) |
| Flash-Verbrauch | ~200-300 KB | ~20-50 KB |
| RAM-Verbrauch | ~50-100 KB (Draw-Buffer) | ~5-10 KB |
| E-Paper-tauglich | Ja (mit Adapter) | Ja (direkt) |
| Animationen | Ja (aber bei E-Paper limitiert) | Manuell |
| Partial-Refresh | Über Adapter (tuanpmt/esp_epaper) | Direkt via GxEPD2 |

**Empfehlung: Custom-UI mit GxEPD2 und Adafruit_GFX-Primitiven.**

Begründung:
- Bordcomputer-UI ist simpel: wenige Werte, große Fonts, klare Hierarchie
- LVGL ist überdimensioniert für ein 296×128 b/w Display
- Partial-Refresh mit GxEPD2 ist direkter und besser kontrollierbar
- Weniger Flash- und RAM-Verbrauch → mehr Platz für Logger und BLE

LVGL wäre interessant, wenn man komplexe UIs mit vielen Widgets braucht
(Scroll-Menüs, Tab-Views). Für einen Bordcomputer reicht Custom-UI.

### 4.5 UI-Design: Anzeige-Layout

Primärer Screen (Tour-Modus):
```
+-------------------------------------------+
|                    |                      |
|     98 km/h        |    4500 rpm          |
|     SPEED          |    RPM   [Gang: 5]   |
|                    |                      |
|--------------------|----------------------|
|                    |                      |
|     340 m          |    22°C   842 m      |
|     HÖHE           |    TEMP   HÖHE       |
|                    |                      |
|--------------------|----------------------|
|  Neigung: +5°  | Akku: 78%  | GPS: 9 SAT |
+-------------------------------------------+
```

Layout-Wechsel:
- **Tour**: Standard (Speed, RPM, Höhe, Temp, Neigung, Akku)
- **Track**: GPS-Fokus (Position, HDOP, Speed, Distanz, Zeit)
- **Config**: Einstellungen (BLE-Pairing, Kalibrierung, Log-Format)

Dark/Light-Mode bei E-Paper = Invert:
```c
// Invert: Schwarz wird Weiß, Weiß wird Schwarz
display.fillScreen(GxEPD_BLACK);  // Hintergrund Schwarz
display.setTextColor(GxEPD_WHITE); // Text Weiß
```

Praktisch bei Dunkelheit: helle Schrift auf dunklem Grund ist weniger
blendend. Umschaltung per Long-Press auf einen Button oder per BLE.

## 5. Sensor-Treiber

### 5.1 I2C-Bus: ESP32-S3 I2C-API

Der ESP32-S3 hat 2 I2C-Controller (I2C_NUM_0 und I2C_NUM_1). Beide Sensoren
(MPU6050 und BMP280) teilen sich einen Bus.

Ab ESP-IDF v5.x wird die neue I2C-API mit Handle-basiertem Design empfohlen:

```c
#include "driver/i2c_master.h"

#define I2C_MASTER_SCL_IO  9    // PINMAP
#define I2C_MASTER_SDA_IO  8    // PINMAP
#define I2C_MASTER_FREQ   400000  // 400kHz Fast Mode

i2c_master_bus_handle_t i2c_bus_handle;

void i2c_init(void) {
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,  // externe 4.7k Pullups
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));
}

// Device-Handle für MPU6050 (Adresse 0x68)
i2c_master_dev_handle_t mpu6050_dev;
void mpu6050_add_to_bus(void) {
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x68,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle,
                                               &dev_config, &mpu6050_dev));
}
```

Mehrere Sensoren am gleichen Bus: jedes Device bekommt einen eigenen Handle.
Der I2C-Master-Treiber kümmert sich um Arbitrierung.

### 5.2 MPU6050: Register-Map und Konfiguration

I2C-Adresse: 0x68 (AD0=LOW) oder 0x69 (AD0=HIGH).

Wichtige Register:

| Register | Adresse | Beschreibung |
|---|---|---|
| SMPLRT_DIV | 0x19 | Sample-Rate-Teiler |
| CONFIG | 0x1A | DLPF-Konfiguration |
| GYRO_CONFIG | 0x1B | Gyro-Bereich (±250/500/1000/2000 °/s) |
| ACCEL_CONFIG | 0x1C | Accel-Bereich (±2/4/8/16 g) |
| FIFO_EN | 0x23 | FIFO-Aktivierung pro Sensor |
| INT_PIN_CFG | 0x37 | Interrupt-Pin-Konfiguration |
| INT_ENABLE | 0x38 | Interrupt-Enable |
| INT_STATUS | 0x3A | Interrupt-Status (read-only) |
| ACCEL_XOUT_H | 0x3B | Accel X high byte |
| ACCEL_XOUT_L | 0x3C | Accel X low byte |
| TEMP_OUT_H | 0x41 | Temperatur high byte |
| GYRO_XOUT_H | 0x43 | Gyro X high byte |
| USER_CTRL | 0x6A | FIFO/DMP-Enable |
| PWR_MGMT_1 | 0x6B | Power-Management, Sleep, Clock |
| WHO_AM_I | 0x75 | Device-ID (0x68 oder 0x70 bei Clones) |

Konfiguration für Bordcomputer:
- Gyro: ±2000 °/s (Motorrad kann schnell drehen)
- Accel: ±4g (ausreichend für Neigung, nicht für Crash)
- DLPF: CONFIG=0x03 (42Hz Gyro / 44Hz Accel) — filtert Motorvibration
- Sample-Rate: 1kHz / (1 + SMPLRT_DIV) → SMPLRT_DIV=19 → 50Hz

```c
void mpu6050_configure(void) {
    // PWR_MGMT_1: wake up, clock=PLL (Gyro X)
    mpu6050_write_reg(0x6B, 0x01);
    vTaskDelay(pdMS_TO_TICKS(100));

    // SMPLRT_DIV = 19 → 1kHz / 20 = 50Hz
    mpu6050_write_reg(0x19, 0x13);

    // CONFIG: DLPF_CFG=3 (42Hz Gyro / 44Hz Accel)
    mpu6050_write_reg(0x1A, 0x03);

    // GYRO_CONFIG: FS_SEL=3 (±2000 °/s), Sensitivity=16.4 LSB/°/s
    mpu6050_write_reg(0x1B, 0x18);

    // ACCEL_CONFIG: AFS_SEL=1 (±4g), Sensitivity=8192 LSB/g
    mpu6050_write_reg(0x1C, 0x08);
}

// Daten lesen (6 Bytes Accel + 2 Bytes Temp + 6 Bytes Gyro = 14 Bytes ab 0x3B)
void mpu6050_read_all(int16_t *ax, int16_t *ay, int16_t *az,
                       int16_t *gx, int16_t *gy, int16_t *gz) {
    uint8_t buf[14];
    mpu6050_read_regs(0x3B, buf, 14);
    *ax = (buf[0]  << 8) | buf[1];
    *ay = (buf[2]  << 8) | buf[3];
    *az = (buf[4]  << 8) | buf[5];
    // buf[6..7] = Temperatur
    *gx = (buf[8]  << 8) | buf[9];
    *gy = (buf[10] << 8) | buf[11];
    *gz = (buf[12] << 8) | buf[13];
}
```

I2C-Read/Write-Hilfsfunktionen:
```c
esp_err_t mpu6050_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(mpu6050_dev, buf, 2, pdMS_TO_TICKS(100));
}

esp_err_t mpu6050_read_regs(uint8_t reg, uint8_t *buf, size_t len) {
    return i2c_master_transmit_receive(mpu6050_dev, &reg, 1,
                                        buf, len, pdMS_TO_TICKS(100));
}
```

### 5.3 Sensor-Fusion: Complementary Filter für Roll/Pitch

Der Complementary Filter kombiniert Gyro (kurzfristig präzise, driftet)
mit Accel (langfristig stabil, aber verrauscht durch Beschleunigung).

Formel:
```
angle = 0.98 * (angle + gyro_rate * dt) + 0.02 * accel_angle
```

Code-Beispiel (50Hz → dt=0.02s):
```c
#define COMP_FILTER_ALPHA  0.98f
#define DT                 0.02f  // 50Hz

static float roll = 0.0f, pitch = 0.0f;

void sensor_fusion_update(int16_t ax, int16_t ay, int16_t az,
                           int16_t gx, int16_t gy, int16_t gz) {
    // Rohwerte in physikalische Einheiten umrechnen
    float accel_x = ax / 8192.0f;  // ±4g → LSB/g
    float accel_y = ay / 8192.0f;
    float accel_z = az / 8192.0f;
    float gyro_x  = gx / 16.4f;   // ±2000°/s → LSB/(°/s)
    float gyro_y  = gy / 16.4f;
    float gyro_z  = gz / 16.4f;

    // Roll aus Accel (Winkel um X-Achse)
    float accel_roll = atan2(accel_y, accel_z) * 180.0f / M_PI;
    // Pitch aus Accel (Winkel um Y-Achse)
    float accel_pitch = atan2(-accel_x,
        sqrt(accel_y * accel_y + accel_z * accel_z)) * 180.0f / M_PI;

    // Complementary Filter
    roll  = COMP_FILTER_ALPHA * (roll  + gyro_x * DT)
            + (1.0f - COMP_FILTER_ALPHA) * accel_roll;
    pitch = COMP_FILTER_ALPHA * (pitch + gyro_y * DT)
            + (1.0f - COMP_FILTER_ALPHA) * accel_pitch;
}
```

Roll = Seitenneigung (links/rechts) — wichtig für Motorrad.
Pitch = Längsneigung (vorn/hinten) — relevant für Gefälle/Steigung.

### 5.4 BMP280: Kalibrierung und Druck→Höhe

I2C-Adresse: 0x76 (oder 0x77, je nach Modul).

Der BMP280 hat werkskalibrierte Koeffizienten in Registern 0x88–0xA1.
Diese müssen einmal beim Start gelesen werden.

```c
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5;
    int16_t  dig_P6, dig_P7, dig_P8, dig_P9;
} bmp280_calib_t;

bmp280_calib_t bmp_calib;

void bmp280_read_calibration(void) {
    uint8_t buf[24];
    bmp280_read_regs(0x88, buf, 24);
    bmp_calib.dig_T1 = (buf[1]  << 8) | buf[0];
    bmp_calib.dig_T2 = (buf[3]  << 8) | buf[2];
    bmp_calib.dig_T3 = (buf[5]  << 8) | buf[4];
    bmp_calib.dig_P1 = (buf[7]  << 8) | buf[6];
    bmp_calib.dig_P2 = (buf[9]  << 8) | buf[8];
    // ... restliche Koeffizienten
}
```

Temperatur- und Druckberechnung (aus BMP280 Datasheet, Section 4):
```c
// Returns temperature in °C * 100 (integer)
int32_t bmp280_compensate_temperature(int32_t adc_T) {
    int32_t var1 = (((adc_T >> 3) - ((int32_t)bmp_calib.dig_T1 << 1)))
                   * ((int32_t)bmp_calib.dig_T2) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)bmp_calib.dig_T1))
                   * ((adc_T >> 4) - ((int32_t)bmp_calib.dig_T1))) >> 12)
                   * ((int32_t)bmp_calib.dig_T3)) >> 14;
    return var1 * 128 + var2;  // t_fine
}

// Returns pressure in Pa (integer)
int32_t bmp280_compensate_pressure(int32_t adc_P, int32_t t_fine) {
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bmp_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)bmp_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)bmp_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)bmp_calib.dig_P3) >> 8)
           + ((var1 * (int64_t)bmp_calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1))
           * ((int64_t)bmp_calib.dig_P1) >> 33;
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)bmp_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bmp_calib.dig_P8) * p) >> 19;
    return (int32_t)((p + var1 + var2 + (int64_t)bmp_calib.dig_P7) >> 8);
}
```

Druck → Höhe (barometrische Formel):
```c
// Höhe über NN in Metern
float bmp280_pressure_to_altitude(int32_t pressure_pa) {
    // P0 = 101325 Pa (Meeresspiegel)
    return 44330.0f * (1.0f - powf(pressure_pa / 101325.0f, 0.1903f));
}
```

### 5.5 ADC-Handling: esp_adc_cal, Akku-Überwachung

Der ESP32-S3 hat zwei 12-bit ADC-Einheiten. Für die Akku-Spannungsmessung
(GPIO 4, Spannungsteiler 2:1) wird ADC1 Channel 4 genutzt.

Neue ADC-API (v5.x, `esp_adc`):
```c
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc_cali_handle;

void adc_init(void) {
    // ADC1 Unit
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    // Channel 4 konfigurieren (GPIO 4)
    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_12,  // bis ~2.5V
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle,
                                                ADC_CHANNEL_4,
                                                &chan_config));

    // eFuse Vref Kalibrierung
    adc_cali_linepoint_val_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .chan = ADC_CHANNEL_4,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_linepoint(&cali_config,
                                                      &adc_cali_handle));
}

float read_battery_voltage(void) {
    int raw, voltage_mv;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &raw));
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw,
                                             &voltage_mv));
    // Spannungsteiler 2:1 → echte Spannung = 2 * measured
    return (float)voltage_mv * 2.0f / 1000.0f;  // Volt
}
```

eFuse Vref: jeder ESP32-S3 hat einen werkskalibrierten Vref-Wert in eFuse
gespeichert. `adc_cali_create_scheme_linepoint` liest diesen automatisch.

### 5.6 Pulse-Counter (PCNT): Drehzahl/Tacho

Der ESP32-S3 hat PCNT-Units für Hardware-Pulse-Counting. Damit kann man
Reed-Kontakte, Hall-Sensoren oder TTL-Pulse zählen ohne CPU-Last.

Neue PCNT-API (v5.x, handle-basiert):

```c
#include "driver/pulse_cnt.h"

pcnt_unit_handle_t pcnt_unit;
pcnt_channel_handle_t pcnt_chan;

void pcnt_init(void) {
    // PCNT-Unit erstellen
    pcnt_unit_config_t unit_config = {
        .low_limit = -1000,
        .high_limit = 1000,
        .accum_count = true,  // interner Akkumulator
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    // Glitch-Filter: 1µs
    pcnt_glitch_filter_config_t filter = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter));

    // Channel erstellen (Pulse-Input auf GPIO)
    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = 3,  // Tacho-Signal-Pin (Reserve)
        .level_gpio_num = -1,  // kein Control-Pin
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_chan));

    // Steigende Flanke → Counter erhöhen
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_HOLD));

    // Watch-Point bei High-Limit → Interrupt
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, 1000));
    // Callback registrieren
    pcnt_event_callbacks_t cbs = {
        .on_reach = pcnt_overflow_handler,
    };
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit,
                                                        &cbs, NULL));

    // Unit starten
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}
```

Drehzahl berechnen:
```c
// Im sensor_task: alle 100ms Counter auslesen
int pulse_count;
pcnt_unit_get_count(pcnt_unit, &pulse_count);

// RPM = (pulse_count / time_window_s) * (60 / pulses_per_revolution)
float rpm = (pulse_count / 0.1f) * (60.0f / PULSES_PER_REV);

// Counter zurücksetzen nach dem Lesen
pcnt_unit_clear_count(pcnt_unit);
```

Quelle: https://docs.espressif.com/projects/esp-idf/en/v5.3.5/esp32s3/api-reference/peripherals/pcnt.html

## 6. Daten-Logging

### 6.1 FATFS-API im ESP-IDF

ESP-IDF bietet FATFS über das VFS (Virtual File System) Interface. Damit
sind Standard-C-Funktionen (fopen, fwrite, fclose) verfügbar.

SD-Karte über SPI mounten (shared mit Display):
```c
#include "esp_vfs_fat_sdspi.h"
#include "driver/spi_common.h"

#define MOUNT_POINT "/sdcard"
#define SD_CS_PIN   38   // PINMAP

sdmmc_card_t *card;
sdmmc_host_t host = SDSPI_HOST_DEFAULT();

void sd_init(void) {
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = 11,  // shared mit Display
        .miso_io_num = 13,
        .sclk_io_num = 12,
        .max_transfer_sz = 4096,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA));

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };
    ESP_ERROR_CHECK(esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host,
        &slot_config, &mount_config, &card));
}
```

Nach dem Mount sind POSIX-Funktionen nutzbar:
```c
FILE *f = fopen("/sdcard/logs/track_001.csv", "w");
fprintf(f, "timestamp,lat,lon,speed,rpm,temp,altitude\n");
fclose(f);
```

Quelle: https://github.com/espressif/esp-idf/blob/master/examples/storage/sd_card/sdspi/README.md

### 6.2 Ringbuffer-Strategie: Batch-Writes

SD-Karten-Schreibzugriffe sind langsam (10-50ms pro Block) und blockierend.
Direkte Schreibzugriffe pro Messwert würden den Sensor-Task blockieren.

Strategie:
1. Sensor/GPS-Daten im RAM sammeln (Ringbuffer oder Array)
2. Alle N Sekunden (z.B. 5s) als Batch auf SD schreiben
3. Schreiben im logger_task (niedrige Priorität)

```c
#define LOG_BATCH_SIZE   64    // Einträge pro Batch
#define LOG_ENTRY_SIZE   sizeof(log_entry_t)

typedef struct {
    uint32_t timestamp;
    float lat, lon;
    float speed;
    int16_t rpm;
    float temp;
    float altitude;
    float roll, pitch;
} log_entry_t;

static log_entry_t log_buffer[LOG_BATCH_SIZE];
static int log_index = 0;

void logger_add_entry(const log_entry_t *entry) {
    // In Buffer schreiben (thread-safe mit Mutex)
    xSemaphoreTake(log_mutex, portMAX_DELAY);
    log_buffer[log_index++] = *entry;
    bool flush_needed = (log_index >= LOG_BATCH_SIZE);
    xSemaphoreGive(log_mutex);

    if (flush_needed) {
        xTaskNotify(logger_task_handle, 0, eNoAction);
    }
}

void logger_task(void *arg) {
    for (;;) {
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

        xSemaphoreTake(log_mutex, portMAX_DELAY);
        int count = log_index;
        log_index = 0;
        // Buffer kopieren für nicht-blockierendes Schreiben
        log_entry_t batch[LOG_BATCH_SIZE];
        memcpy(batch, log_buffer, count * LOG_ENTRY_SIZE);
        xSemaphoreGive(log_mutex);

        // SPI-Mutex holen (shared mit Display)
        xSemaphoreTake(spi_mutex, portMAX_DELAY);
        FILE *f = fopen("/sdcard/logs/track_001.csv", "a");
        if (f) {
            for (int i = 0; i < count; i++) {
                fprintf(f, "%u,%.6f,%.6f,%.1f,%d,%.1f,%.1f,%.1f,%.1f\n",
                    batch[i].timestamp, batch[i].lat, batch[i].lon,
                    batch[i].speed, batch[i].rpm, batch[i].temp,
                    batch[i].altitude, batch[i].roll, batch[i].pitch);
            }
            fflush(f);
            fsync(fileno(f));  //强制 flush auf SD
            fclose(f);
        }
        xSemaphoreGive(spi_mutex);
    }
}
```

### 6.3 CSV-Format

Header:
```
timestamp,lat,lon,speed_kmh,rpm,temp_c,alt_m,roll_deg,pitch_deg,bat_v,hdop,sats
```

Beispiel-Zeile:
```
1728421200,50.934721,11.589344,98.5,4500,22.3,340.5,5.2,-1.8,3.92,1.2,9
```

Timestamp = Unix-Epoch (Sekunden seit 1970-01-01 UTC).

### 6.4 GPX-Export

GPX ist ein XML-Format für GPS-Tracks. Track-Points mit Zeit, Höhe, Speed.

```c
void gpx_write_header(FILE *f) {
    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(f, "<gpx version=\"1.1\" creator=\"Transalp-Bordcomputer\"\n");
    fprintf(f, "  xmlns=\"http://www.topografix.com/GPX/1/1\">\n");
    fprintf(f, "<trk>\n<trkseg>\n");
}

void gpx_write_point(FILE *f, float lat, float lon, float ele,
                      uint32_t timestamp, float speed) {
    // ISO 8601 Timestamp
    time_t t = timestamp;
    struct tm *tm = gmtime(&t);
    char time_str[25];
    strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", tm);

    fprintf(f, "<trkpt lat=\"%.6f\" lon=\"%.6f\">\n", lat, lon);
    fprintf(f, "  <ele>%.1f</ele>\n", ele);
    fprintf(f, "  <time>%s</time>\n", time_str);
    fprintf(f, "  <speed>%.1f</speed>\n", speed);
    fprintf(f, "</trkpt>\n");
}

void gpx_write_footer(FILE *f) {
    fprintf(f, "</trkseg>\n</trk>\n</gpx>\n");
}
```

### 6.5 Binär-Format für High-Rate-Daten

CSV ist lesbar, aber ineffizient für High-Rate-Daten (IMU 50Hz = 50 Zeilen/s).
Binär-Format spart ~70% Speicher.

Format-Design:
```c
typedef struct __attribute__((packed)) {
    uint16_t magic;        // 0xB0BC für Validierung
    uint16_t version;      // Format-Version
    uint32_t start_time;   // Unix-Timestamp des ersten Eintrags
} bin_header_t;

typedef struct __attribute__((packed)) {
    uint16_t dt_ms;        // ms seit letztem Eintrag (max 65s)
    int16_t ax, ay, az;    // Accel raw (±4g)
    int16_t gx, gy, gz;    // Gyro raw (±2000°/s)
    int16_t rpm;           // Drehzahl
    uint8_t temp_c;        // Temperatur (°C, offset 0°C)
    uint8_t flags;         // Bit0: GPS-Fix, Bit1: CAN-Active
} bin_entry_t;  // 14 Bytes pro Eintrag
```

Bei 50Hz: 14 Bytes * 50 = 700 Bytes/s → ~2.5 MB pro Stunde.
CSV bei 50Hz: ~60 Bytes/Zeile * 50 = 3000 Bytes/s → ~10.8 MB pro Stunde.

Konvertierung Binär→CSV offline (Python-Script auf PC).

### 6.6 Dateinamen-Konvention

Format: `YYYYMMDD_HHMMSS.csv` und `.gpx` bzw. `.bin`

```c
void generate_filename(char *buf, size_t len, const char *ext) {
    time_t now;
    time(&now);
    struct tm *tm = localtime(&now);
    strftime(buf, len, "/sdcard/tracks/%Y%m%d_%H%M%S", tm);
    strncat(buf, ext, len - strlen(buf) - 1);
}
```

Beispiel: `/sdcard/tracks/20261013_142015.gpx`

Neue Datei starten bei:
- Neuer Tag (Datum wechselt)
- Reset / Neustart
- Konfiguration: max Einträge pro Datei (z.B. 100000)

### 6.7 SD-Karten-Verschleiß: Wear-Leveling

SD-Karten haben internen Wear-Leveling-Controller (Flash Translation Layer).
Dieser verteilt Schreibzugriffe über die Blöcke. Trotzdem begrenzt:

- TLC-Flash: ~500 Schreibzyklen pro Zelle
- MLC-Flash: ~3000 Zylken
- SLC-Flash: ~100000 Zylken (teuer, selten in Consumer-SD)

Lebensdauer-Abschätzung für 8GB Class-10 SD-Karte:
- Bei 700 Bytes/s (Binär, 50Hz) → 2.5 MB/h → 60 MB/Tag
- 8GB = ~133 Tage bis einmal voll geschrieben
- Bei Wear-Leveling über 8GB: ~500-1000 Zyklen → 133 * 500 = 66500 Tage
- Realistisch: mehrere Jahre Nutzung

Best Practices:
- Class-10 Karten (min. 10 MB/s Schreiben)
- FAT32 (nicht exFAT — ESP-IDF FATFS unterstützt kein exFAT)
- `fsync()` nach jedem Batch (verhindert Datenverlust bei Stromausfall)
+- Dateien nicht zu oft öffnen/schließen (Overhead)

## 7. BLE-Stack

### 7.1 NimBLE-Stack initialisieren

ESP-IDF unterstützt zwei BLE-Host-Stacks: Bluedroid (älter, mehr Features,
mehr RAM) und NimBLE (neuer, kompakter, weniger RAM, nur BLE — kein Classic).

Für den ESP32-S3 (nur BLE, kein Classic) ist NimBLE die bessere Wahl:
- ~50% weniger RAM als Bluedroid
- BT 5.0 kompatibel
- Aktiv gepflegt (Apache MyNewt)
- Leichter zu konfigurieren

In menuconfig:
- `CONFIG_BT_ENABLED` = y
- `CONFIG_BT_NIMBLE_ENABLED` = y
- `CONFIG_BT_NIMBLE_ROLE_PERIPHERAL` = y (GATT-Server)
- `CONFIG_BT_NIMBLE_ROLE_CENTRAL` = n (nur Peripheral nötig)
- `CONFIG_BT_NIMBLE_MAX_CONNECTIONS` = 1
- `CONFIG_BT_NIMBLE_MAX_BONDS` = 3 (Smartphone-Pairings speichern)

Initialisierung:
```c
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"

void ble_init(void) {
    ESP_ERROR_CHECK(nimble_port_init());

    // Sync-Callback setzen
    ble_hs_cfg.sync_cb = ble_on_sync;

    // GAP-Device-Name setzen
    ble_svc_gap_device_name_set("Transalp-BC");

    // GATT-Services initialisieren
    ble_svc_gatt_init();
    // Services registrieren (siehe 7.2)

    // Host-Task starten
    nimble_port_freertos_init(ble_host_task);
}

static void ble_on_sync(void) {
    // Advertising starten
    uint8_t addr_type;
    ble_hs_id_infer_auto(0, &addr_type);
    ble_gap_adv_start(addr_type, NULL, BLE_HS_FOREVER,
                      &adv_params, ble_gap_event_handler, NULL);
}
```

### 7.2 GATT-Service-Design

Vier Services für den Bordcomputer:

**Telemetry-Service** (UUID: 0000A001-0000-1000-8000-00805F9B34FB)
- Characteristics:
  - GPS-Data (notify): lat, lon, speed, alt, sats — 20 Bytes
  - RPM/Speed (notify): rpm, speed_kmh, gear — 8 Bytes
  - Sensors (notify): temp, altitude, roll, pitch — 16 Bytes
  - Battery (read): voltage, percentage — 4 Bytes

**Config-Service** (UUID: 0000A002-0000-1000-8000-00805F9B34FB)
- Characteristics:
  - Display-Mode (read/write): 0=Tour, 1=Track, 2=Config
  - Log-Interval (read/write): 1-60 Sekunden
  - Calibration (write): IMU-Offsets, BMP280-Sea-Level
  - Device-Info (read): Firmware-Version, Uptime

**Navigation-Service** (UUID: 0000A003-0000-1000-8000-00805F9B34FB)
- Characteristics:
  - Next-Turn (write): direction (uint8), distance_m (uint32), street (string)
  - ETA (write): arrival_time (uint32), distance_total (uint32)
  - Route-Active (read/write): bool

**OTA-Service** (UUID: 0000A004-0000-1000-8000-00805F9B34FB)
- Characteristics:
  - OTA-Control (write): start/cancel/verify commands
  - OTA-Data (write): firmware chunks (siehe Abschnitt 8)
  - OTA-Status (notify): progress, errors

Service mit Attribute-Table definieren (NimBLE):
```c
// Telemetry-Service UUID
static const ble_uuid128_t svc_uuid =
    BLE_UUID128_INIT(0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
                     0x00, 0x10, 0x00, 0x00, 0x01, 0xA0, 0x00, 0x00);

static int gps_data_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        // GPS-Daten zusammenbauen
        gps_data_t gps;
        // ... aus Queue lesen ...
        os_mbuf_append(ctxt->om, &gps, sizeof(gps));
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gps_chr_uuid.u,
                .access_cb = gps_data_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            { 0 },  // Terminator
        },
    },
    { 0 },  // Service-Terminator
};

void ble_register_services(void) {
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ESP_ERROR_CHECK(ble_gatts_count_cfg(gatt_services));
    ESP_ERROR_CHECK(ble_gatts_add_svcs(gatt_services));
}
```

Notify (Telemetry an Smartphone pushen):
```c
void ble_notify_gps(gps_data_t *data) {
    if (!conn_handle_active) return;
    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, sizeof(*data));
    ble_gattc_notify(conn_handle, gps_attr_handle, om);
}
```

### 7.3 MTU-Verhandlung und Connection-Parameters

Default MTU: 23 Bytes (ATT_MTU). Für Telemetry ist das zu wenig (20 Bytes
GPS-Daten + 3 Bytes ATT-Header = 23). MTU erhöhen:

```c
static int ble_gap_event_handler(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        conn_handle_active = event->connect.conn_handle;
        // MTU auf 247 verhandeln (max für BLE 5.0)
        ble_att_set_preferred_mtu(247);
        ble_gattc_exchange_mtu(conn_handle_active, NULL, NULL);
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        conn_handle_active = 0;
        // Advertising neu starten
        ble_gap_adv_start(...);
        break;
    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU negotiated: %d", event->mtu.value);
        break;
    }
    return 0;
}
```

Connection-Parameters (Advertising):
```c
static struct ble_gap_adv_params adv_params = {
    .conn_mode = BLE_GAP_CONN_MODE_UND,  // Connectable
    .disc_mode = BLE_GAP_DISC_MODE_GEN,  // General discoverable
    .itvl_min = 0x0020,  // 20ms min interval
    .itvl_max = 0x0040,  // 40ms max interval
};
```

Connection-Parameters nach Connect (für stabile Latenz):
```c
struct ble_gap_upd_params conn_params = {
    .itvl_min = 0x0006,  // 7.5ms
    .itvl_max = 0x0010,  // 20ms
    .latency = 0,
    .supervision_timeout = 0x00C8,  // 2s
};
ble_gap_update_params(conn_handle, &conn_params);
```

### 7.4 BLE-Security: Pairing und Bonding

```c
// In ble_hs_cfg:
ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;  // Just Works (keine Tastatur)
ble_hs_cfg.sm_bonding = 1;   // Bonding aktivieren
ble_hs_cfg.sm_mitm = 0;      // Just Works (kein MITM-Schutz)
ble_hs_cfg.sm_sc = 1;        // Secure Connections (LE Secure Connections)
ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
```

Just Works ist ausreichend für Bordcomputer-Daten (keine sensiblen Daten).
Secure Connections (LE SC) nutzt ECDH (P-256) — sicherer als Legacy Pairing.
Bonding speichert Keys im NVS — beim erneuten Verbinden wird kein Pairing
mehr benötigt.

### 7.5 App-Konzept: welche App spricht mit dem Bordcomputer?

Möglichkeiten:
1. **Custom-App** (Flutter / React Native): volle Kontrolle, eigene UI,
   BLE-Plugin. Aufwendig zu entwickeln.
2. **Web Bluetooth API** (Chrome): Webseite verbindet sich per BLE.
   Einfach zu prototypen, aber nur Chrome/Edge.
3. **Serial Bluetooth Terminal** (App): generisches BLE-Terminal für
   Debugging. Nicht nutzerfreundlich für Endanwender.
4. **Existing Apps**: Apps wie "BLE Scanner" oder "nRF Connect" können
   GATT-Services lesen/schreiben. Gut für Debug, nicht für Produktion.

Empfehlung: **Custom-App in Flutter** mit `flutter_blue_plus` Plugin.
Flutter ist plattformübergreifend (iOS + Android), BLE-Plugin ist reif.
Die App kann:
- Live-Telemetry anzeigen (als Ergänzung zum E-Paper)
- Navigation-Directions an den Bordcomputer senden
- OTA-Firmware-Updates durchführen
- Konfiguration ändern (Display-Mode, Log-Interval, Kalibrierung)
+- Tracks von SD-Karte downloaden (über BLE, langsam aber funktional)

## 8. OTA-Update

### 8.1 ESP-IDF OTA-API

ESP-IDF bietet eine vollständige OTA-API für Firmware-Updates über
Dual-Partitions. Die API (`esp_ota_*`) verwaltet das Schreiben in die
inaktive Partition und das Setzen des Boot-Flags.

Partition-Layout (16MB Flash):
```
# Name,   Type, SubType, Offset,  Size,  Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 4M,
ota_0,    app,  ota_0,   0x410000, 4M,
ota_1,    app,  ota_1,   0x810000, 4M,
otadata,  data, ota,     0xc10000, 0x2000,
```

API-Flow:
```c
#include "esp_ota_ops.h"

esp_err_t ota_start(const uint8_t *data, size_t len) {
    esp_ota_handle_t ota_handle;
    const esp_partition_t *ota_partition = NULL;

    // Inaktive Partition finden
    ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) return ESP_FAIL;

    // OTA-Session starten
    ESP_ERROR_CHECK(esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN,
                                   &ota_handle));

    // Ersten Chunk schreiben
    ESP_ERROR_CHECK(esp_ota_write(ota_handle, data, len));

    // ... weitere Chunks empfangen und schreiben ...

    // OTA abschließen
    ESP_ERROR_CHECK(esp_ota_end(ota_handle));

    // Boot-Flag setzen
    ESP_ERROR_CHECK(esp_ota_set_boot_partition(ota_partition));

    // Neustart
    esp_restart();
    return ESP_OK;
}
```

### 8.2 Dual-Partition-Setup

Der ESP32-S3 bootet von der Partition, die in `otadata` als aktiv markiert
ist. Der Flow:
1. Firmware läuft auf `factory` oder `ota_0`
2. OTA-Update wird in die andere Partition (`ota_1` bzw. `ota_0`) geschrieben
3. `esp_ota_set_boot_partition()` markiert die neue Partition als aktiv
4. `esp_restart()` → Boot von neuer Partition

### 8.3 Rollback-Mechanismus

Bei aktivem Rollback-Schutz wird eine neue Firmware nicht sofort als gültig
markiert. Sie muss sich erst bewähren.

In menuconfig:
- `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` = y

Flow mit Rollback:
```c
// Nach OTA-Update und Neustart:
const esp_partition_t *running = esp_ota_get_running_partition();
esp_ota_img_states_t state;
esp_ota_get_state_partition(running, &state);

if (state == ESP_OTA_IMG_PENDING_VERIFY) {
    // Firmware ist neu — muss verifiziert werden
    if (firmware_self_test_ok()) {
        esp_ota_mark_app_valid_cancel_rollback();
        ESP_LOGI(TAG, "Firmware validated, rollback cancelled");
    } else {
        ESP_LOGE(TAG, "Self-test failed, rolling back");
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }
}
```

Wenn die Firmware sich nicht within eines Timeouts (konfigurierbar) validiert,
startet der Bootloader automatisch die vorherige Partition. Das verhindert
Boot-Loops nach einem defekten Update.

### 8.4 Firmware-Transfer über BLE

Firmware-Image (typisch 1-2 MB) wird in Chunks über BLE übertragen.

Chunk-Größe: MTU - 3 (ATT-Header) = 244 Bytes bei MTU 247.

Protokoll:
```c
#define OTA_CMD_START    0x01
#define OTA_CMD_DATA     0x02
#define OTA_CMD_FINISH   0x03
#define OTA_CMD_CANCEL   0x04
#define OTA_CHUNK_SIZE   240  // MTU 247 - 3 ATT - 4 Header

typedef struct __attribute__((packed)) {
    uint8_t  cmd;
    uint32_t offset;     // Byte-Offset im Firmware-Image
    uint16_t chunk_len;  // Länge der Daten in diesem Chunk
    uint8_t  data[OTA_CHUNK_SIZE];
} ota_packet_t;

// Bei jedem empfangenen Chunk:
void ota_handle_packet(ota_packet_t *pkt) {
    switch (pkt->cmd) {
    case OTA_CMD_START:
        ota_begin();
        break;
    case OTA_CMD_DATA:
        ota_write_chunk(pkt->data, pkt->chunk_len);
        // Fortschritt notify
        ble_notify_ota_progress(pkt->offset);
        break;
    case OTA_CMD_FINISH:
        ota_finish_and_reboot();
        break;
    }
}
```

Fortschrittsanzeige: OTA-Status-Characteristic sendet nach jedem Chunk
den aktuellen Fortschritt (Prozent) an die App.

### 8.5 Signing / Verification

ESP-IDF Secure Boot signiert Firmware-Images. Das verhindert, dass
unkoshere Firmware geflasht wird.

In menuconfig:
- `CONFIG_SECURE_BOOT_V2_ENABLED` = y
- `CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES` = y

 Beim Build wird ein Private Key genutzt, um das Image zu signieren.
 Der Bootloader verifiziert die Signatur beim Booten.

Für dieses Projekt optional — es sei denn, man möchte verhindern, dass
jemand unbemerkt modifizierte Firmware über BLE einspielt. Bei einem
Motorrad-Bordcomputer ist das Risiko gering.

### 8.6 Recovery-Mode

Bei einem Brick (Firmware bootet nicht) gibt es mehrere Recovery-Wege:

1. **USB-Flash**: ESP32-S3 hat USB-OTG. Mit `idf.py -p /dev/cu.usbmodem*
   flash` kann man direkt über USB flashen, unabhängig vom OTA-State.
2. **Rollback**: Wenn Rollback aktiv ist, startet der Bootloader nach
   Timeout die vorherige Partition automatisch.
3. **Factory-Reset**: `factory`-Partition ist immer vorhanden und kann
   über `esp_ota_set_boot_partition(factory_partition)` aktiviert werden
   (z.B. per Long-Press auf einen Button beim Booten).
4. **Boot-Mode**: GPIO 0 auf LOW beim Reset → Download-Mode, flashen
   über USB.

## 9. Konfigurations-Management

### 9.1 NVS (Non-Volatile Storage)

NVS ist ESP-IDF's Key-Value-Store für Konfigurationsdaten. Es nutzt eine
eigene Flash-Partition und bietet Wear-Leveling.

Eigenschaften:
- Key: max. 15 Zeichen (ASCII)
- Values: int8/16/32/64, float, double, string (max 4000 Bytes), blob
- BLOB: max 508 KB
- Namespaces zur Isolierung verschiedener Module
- Wear-Leveling über mehrere Flash-Pages

Quelle: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html

```c
#include "nvs_flash.h"

void config_init(void) {
    // NVS initialisieren
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

// Konfiguration lesen (mit Default-Fallback)
int32_t config_get_log_interval(void) {
    nvs_handle_t handle;
    int32_t value = 5;  // Default
    if (nvs_open("config", NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_i32(handle, "log_int", &value);
        nvs_close(handle);
    }
    return value;
}

// Konfiguration schreiben
void config_set_log_interval(int32_t seconds) {
    nvs_handle_t handle;
    if (nvs_open("config", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_i32(handle, "log_int", seconds);
        nvs_commit(handle);
        nvs_close(handle);
    }
}
```

### 9.2 Konfigurations-Struktur

```c
typedef struct {
    // Pin-Mapping (falls änderbar — meist fix)
    uint8_t gps_rx_pin;
    uint8_t gps_tx_pin;
    uint8_t display_cs_pin;
    // ... etc

    // Kalibrierung
    int16_t gyro_offset_x, gyro_offset_y, gyro_offset_z;
    int16_t accel_offset_x, accel_offset_y, accel_offset_z;
    uint32_t bmp_sea_level_pa;   // QNH-Referenzdruck
    uint16_t adc_vref_mv;        // ADC-Referenzspannung

    // Schwellen
    uint16_t rpm_redline;        // z.B. 8000
    uint16_t rpm_shift_light;    // z.B. 7000
    float battery_low_v;         // z.B. 3.3
    float battery_shutdown_v;    // z.B. 3.0

    // Logging
    uint8_t log_interval_s;       // 1-60s
    uint8_t log_format;          // 0=CSV, 1=Binary, 2=Both
    uint8_t gps_rate_hz;         // 1, 5, 10

    // Display
    uint8_t display_mode;        // 0=Tour, 1=Track, 2=Config
    uint8_t display_invert;      // 0=Normal, 1=Inverted

    // BLE
    char device_name[16];
} device_config_t;
```

Speichern als BLOB:
```c
void config_save_all(const device_config_t *cfg) {
    nvs_handle_t handle;
    nvs_open("config", NVS_READWRITE, &handle);
    nvs_set_blob(handle, "dev_cfg", cfg, sizeof(device_config_t));
    nvs_commit(handle);
    nvs_close(handle);
}

void config_load_all(device_config_t *cfg) {
    nvs_handle_t handle;
    size_t len = sizeof(device_config_t);
    if (nvs_open("config", NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_blob(handle, "dev_cfg", cfg, &len);
        nvs_close(handle);
    }
}
```

### 9.3 Config-Mode: BLE oder Seriell

Konfiguration kann auf zwei Wegen geändert werden:

**BLE**: Über den Config-Service (read/write Characteristics). Die App
schreibt neue Werte, der ESP32 speichert sie in NVS.

**Seriell**: Über USB-CDC mit einem einfachen Text-Kommando-Interface:
```
SET log_int 10
SET rpm_red 8000
SET display_mode 1
GET log_int
SAVE
```

```c
void config_serial_handler(void) {
    char line[128];
    if (fgets(line, sizeof(line), stdin)) {
        char cmd[16], key[32], val[32];
        if (sscanf(line, "%15s %31s %31s", cmd, key, val) == 3) {
            if (strcmp(cmd, "SET") == 0) {
                config_set_by_key(key, val);
            }
        } else if (sscanf(line, "%15s %31s", cmd, key) == 2) {
            if (strcmp(cmd, "GET") == 0) {
                printf("%s = %s\n", key, config_get_by_key(key));
            }
        }
    }
}
```

### 9.4 Default vs. User-Config: Merge-Strategie

Beim ersten Start (keine NVS-Daten) werden Defaults geladen:

```c
static const device_config_t default_config = {
    .gps_rx_pin = 44,
    .gps_tx_pin = 43,
    .display_cs_pin = 10,
    // ... PINMAP-Werte
    .gyro_offset_x = 0,
    .bmp_sea_level_pa = 101325,
    .rpm_redline = 8000,
    .rpm_shift_light = 7000,
    .battery_low_v = 3.3f,
    .battery_shutdown_v = 3.0f,
    .log_interval_s = 5,
    .log_format = 0,  // CSV
    .gps_rate_hz = 10,
    .display_mode = 0,  // Tour
    .display_invert = 0,
    .device_name = "Transalp-BC",
};

void config_init_with_defaults(device_config_t *cfg) {
    *cfg = default_config;
    // Versuche User-Config aus NVS zu laden (überschreibt Defaults)
    nvs_handle_t handle;
    size_t len = sizeof(*cfg);
    if (nvs_open("config", NVS_READONLY, &handle) == ESP_OK) {
        if (nvs_get_blob(handle, "dev_cfg", cfg, &len) != ESP_OK) {
            ESP_LOGW(TAG, "No user config found, using defaults");
        }
        nvs_close(handle);
    }
    // Wenn NVS-Version != Code-Version → Migration oder Defaults
}
```

Bei neuen Firmware-Versionen mit neuen Config-Fields: BLOB-Größe ändert sich.
NVS liefert dann `ESP_ERR_NVS_INVALID_LENGTH` → Defaults laden und neue
Struktur mit den alten Werten füllen (Migration).

## 10. Bestehende Open-Source-Projekte

### 10.1 Motorrad-Bordcomputer-Projekte auf GitHub

Direkte Vorläufer für Motorrad-Bordcomputer sind rar. Die meisten Projekte
sind Auto-OBD-Scanner oder Fahrrad-Computer.

Gefundene Projekte:
- `geeksville/Meshtastic-ESP32` — LoRa-Mesh, keine Bordcomputer-UI
- `Daniel-Roseman/OBD2-Scanner` — OBD-II Reader, keine UI
- `powerbroker/dashboard` — Auto-Dashboard auf Raspberry Pi, nicht ESP32
- `btselm/ESP32-CAN-Dash` — Auto-Dashboard mit ESP32 und CAN

Adaptierbar: `btselm/ESP32-CAN-Dash` — CAN-basiertes Dashboard, kann als
Referenz für TWAI-Integration und Display-Layout dienen.

### 10.2 Freematics OBD-II Telematic Kit

GitHub: `stanleyhuang/Freematics` + `freematics/freematics_telemetry`

Das Freematics-Telematics-Kit ist ein Open-Source-OBD-II-Telematik-System
für Fahrzeuge. Es basiert auf ESP8266/ESP32 und hat:
- OBD-II-Reader (AT-Kommandos für ELM327)
- GPS-Logging
- SD-Karten-Logging
- Telemetry-Display

Adaptierbar:
- OBD-II PID-Decoder (welche PIDs für was)
- GPS-Logging-Format
- SD-Karten-Schreibstrategie
- Display-Layout-Konzepte

Nicht adaptierbar:
- Freematics nutzt ESP8266 (single-core) — Task-Design übertragbar aber
  nicht direkt
- OBD-II über ELM327 (wir nutzen TWAI direkt)
- Arduino-Framework (wir nutzen ESP-IDF)

Quelle: https://github.com/freematics

### 10.3 CAN-Logger-Projekte

GitHub: `autowp/arduino-mcp2515` — MCP2515 CAN-Logger Library
- Gut dokumentiert, MIT-Lizenz
- Kann für MCP2515-Backup-CAN (zweite Schnittstelle) adaptiert werden
- Nicht direkt für TWAI nutzbar (MCP2515 über SPI, TWAI ist nativ)

GitHub: `nopnop2002/esp-idf-mcp2515` — ESP-IDF native MCP2515-Treiber
- Direkt nutzbar als ESP-IDF-Component
- Sehr gut für sekundäre CAN-Schnittstelle über SPI

### 10.4 OpenDashboard / moto-dashboard

OpenDashboard (GitHub): meist Auto-fokussiert, oft auf Raspberry Pi oder
größeren Displays. Nicht direkt adaptierbar für E-Paper.

Relevante Bibliotheken aus der Open-Source-Community:
- `autowp/arduino-mcp2515` — CAN-Treiber (MCP2515)
- `mikalhart/TinyGPSPlus` — GPS-Parser (Alternative zu minmea)
- `kosma/minmea` — GPS-Parser (empfohlen)
- `ZinggJM/GxEPD2` — E-Paper-Treiber (empfohlen)
- `stanleyhuang/ELMDuino` — ELM327-Library (für Bench-Testing)
- `nopnop2002/esp-idf-mcp2515` — ESP-IDF MCP2515-Component
- `tuanpmt/esp_epaper` — ESP-IDF native E-Paper-Component
- `sparkfun/SparkFun_u-blox_GNSS_Arduino_Library` — u-blox GPS-Konfiguration
- `derdoktor667/ESP32_MPU6050` — MPU6050-Treiber mit DLPF-Konfiguration

### 10.5 Was adaptieren?

| Modul | Library | Adaptierung |
|---|---|---|
| GPS-Parser | minmea | Direkt als Component einbinden |
| GPS-Konfiguration | SparkFun_u-blox_GNSS | UBX-Konfiguration portieren |
| E-Paper | GxEPD2 | Als Arduino-Component einbinden |
| MPU6050 | Eigenbau nach Register-Map | I2C-Treiber direkt in ESP-IDF |
| BMP280 | Eigenbau nach Datasheet | I2C-Treiber direkt in ESP-IDF |
| CAN (TWAI) | ESP-IDF native | TWAI-API direkt nutzen |
| CAN (MCP2515 Backup) | nopnop2002/esp-idf-mcp2515 | Als Component |
| SD/FATFS | ESP-IDF native | esp_vfs_fat_sdspi_mount |
| BLE | NimBLE (ESP-IDF) | Direkt, GATT-Services selbst |
| OTA | ESP-IDF native | esp_ota_* API |
| NVS | ESP-IDF native | nvs_open/set/get/commit |
| Logging | ESP-IDF native | ESP_LOG + eigene Batch-Logik |

Zusammenfassend: Der Stack besteht aus ESP-IDF nativen APIs plus drei
externen Components: minmea (GPS), GxEPD2 (E-Paper) und optional
nopnop2002/esp-idf-mcp2515 (sekundärer CAN). Alles andere wird selbst
implementiert — I2C-Sensoren, BLE-Services, OTA-Protokoll, Logger.

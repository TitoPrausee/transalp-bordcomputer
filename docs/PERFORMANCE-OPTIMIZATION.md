# Performance-Optimierung und Profiling-Konzept

> Bordcomputer Transalp 600 (PD06)
> MCU: ESP32-S3-WROOM-1 (N16R8: 16 MB Flash, 8 MB Octal PSRAM)
> Dual-Core 240 MHz, FreeRTOS SMP, ESP-IDF v5.4
> Stand: Oktober 2026
> Quellen: firmware/main/main.c, firmware/main/app_event_loop.c,
> firmware/main/board_config.h, docs/BENCHMARK-SPEC.md,
> docs/ARCHITECTURE.md, firmware/components/

---

## 0. Zusammenfassung

Dieses Dokument beschreibt die Strategien und Massnahmen, mit denen die
in docs/BENCHMARK-SPEC.md definierten Zielwerte erreicht werden. Waehrend
die Benchmark-Spec die _Was_- und _Wie-messen_-Fragen beantwortet,
beantwortet dieses Dokument die _Wie-erreichen_-Frage.

Jeder Abschnitt verweist auf die konkreten Code-Stellen in der Firmware,
erklaert die aktuelle Implementierung, identifiziert Engpaesse und gibt
konkrete Optimierungsvorschlaege mit Code-Snippets, die direkt in die
bestehenden Module integrierbar sind.

Die Priorisierung der Optimierungen folgt der Benchmark-Spec:
Must-Benchmarks haben hoechste Prioritaet, Should-Benchmarks werden
adressiert, wenn die Must-Ziele gesichert sind.

---

## Inhaltsverzeichnis

1. [CPU-Optimierung](#1-cpu-optimierung)
2. [Speicher-Optimierung](#2-speicher-optimierung)
3. [Rendering-Optimierung](#3-rendering-optimierung)
4. [Sensor-Latenz-Optimierung](#4-sensor-latenz-optimierung)
5. [BLE-Durchsatz-Optimierung](#5-ble-durchsatz-optimierung)
6. [Power-Optimierung](#6-power-optimierung)
7. [Profiling-Strategie](#7-profiling-strategie)
8. [Optimierungs-Roadmap](#8-optimierungs-roadmap)

---

## 1. CPU-Optimierung

### 1.1 Aktuelle Task-Prioritaeten und Core-Pinning

Die Firmware nutzt das FreeRTOS SMP auf dem Dual-Core ESP32-S3. Die
Aufgabenteilung folgt dem Prinzip: Core 1 uebernimmt
Echtzeit-Datenakquisition (deterministische Sampling-Raten), Core 0
uebernimmt Praesentation und Kommunikation (best-effort). Diese Trennung
verhindert, dass UI-Rendering oder BLE-Stack-Schwankungen die
Sensor-Sampling-Rate beeinflussen.

Die aktuellen Task-Konstanten sind in `board_config.h` definiert:

```
Core 1 (TASK_CORE_RT = 1, Echtzeit):
  sensor_task      Prio 4  Stack 4096  Rate 50 Hz
  gps_task          Prio 5  Stack 4096  Rate 10 Hz
  diagnostics_task  Prio 2  Stack 4096  Rate 0.2 Hz (alle 5 s)

Core 0 (TASK_CORE_UI = 0, UI/Comms):
  display_task     Prio 2  Stack 8192  Rate 1 Hz (E-Paper) / 30 FPS (TFT)
  ble_task          Prio 3  Stack 6144  Event-getrieben
  power_task        Prio 6  Stack 2048  Rate 1 Hz
  logger_task       Prio 3  Stack 4096  Rate 5 Hz
  ota_task          Prio 4  Stack 8192  Sleep bis Update
  event_loop        Prio 3  Stack 4096  Event-getrieben
```

**Begruendung der Prioritaeten:**

- `power_task` (P6, hoechste Prioritaet auf Core 0): Ein kritischer
  Akku-Zustand (Spannung < 3.2 V) muss sofort zu einem geordneten
  Shutdown fuehren, unabhaengig davon, was andere Tasks gerade tun.
  Die hoechste Prioritaet stellt sicher, dass der Power-Task nie von
  UI- oder BLE-Aktivitaeten blockiert wird.

- `gps_task` (P5, hoechste auf Core 1): NMEA-Daten kommen asynchron
  ueber UART. Der Task muss den UART-Puffer rechtzeitig auslesen, um
  keinen Satz zu verlieren (besonders bei 115200 Baud und 10 Hz
  Update-Rate). Eine hoehe Prioritaet minimiert UART-Overruns.

- `sensor_task` (P4 auf Core 1): 50 Hz Sampling-Rate (20 ms Zyklus).
  Muss deterministic laufen, um_aliasing zu vermeiden. P4 ist hoch
  genug, um von logger/display nicht verdraengt zu werden, aber
  niedriger als GPS, da UART-Overruns kritischer sind als ein
  um 1 ms verschobener Sample.

- `ota_task` (P4 auf Core 0): Nur aktiv waehrend eines Updates. P4
  stellt sicher, dass der OTA-Transfer nicht von BLE-Notify-Sturm
  verdraengt wird, ist aber niedriger als power (Shutdown waehrend
  OTA muss moeglich sein).

- `ble_task` (P3): Event-getrieben. NimBLE laeuft intern in einem
  eigenen Host-Task; `ble_task` behandelt nur anwendungsseitige Events.
  P3 ist ausreichend, da BLE-Events nicht zeitkritischer als
  Sensor-Updates sind.

- `logger_task` (P3): 5 Hz Polling. Schreibt Batches auf SD. P3 reicht,
  da Logging-Verzoegerung nicht kritisch ist (Daten sind bereits im
  Ringbuffer).

- `event_loop` (P3): Zentrale Event-Verteilung. P3 sorgt dafuer, dass
  Events schnell verarbeitet werden, aber nicht vor Power- oder
  OTA-Events.

- `display_task` (P2): Niedrigste Prioritaet auf Core 0. Display-Updates
  sind best-effort und duerfen nicht Sensoren oder BLE stoeren.

- `diagnostics_task` (P2): Periodische Gesundheitspruefung alle 5 s.
  Niedrige Prioritaet, da Diagnose nie zeitkritisch ist.

**Empfohlene Anpassungen:**

Die aktuelle Verteilung ist fuer die Prototyp-Phase gut gewaehlt. Fuer
die Produktions-Phase werden folgende Tuning-Massnahmen empfohlen:

1. `sensor_task` auf P5 erhoehen (gleich mit GPS), da die 50 Hz
   Sampling-Rate bei 8000 U/min (133 Hz RPM-Pulse) ohne Jitter
   laufen muss. Die Benchmark-Spec fordert < 80% CPU-Last pro Task,
   aber bei hoeherer Prioritaet ist der Task seltener unterbrochen.

2. `display_task` bei TFT-Betrieb auf P3 erhoehen (30 FPS Target),
   bei E-Paper-Betrieb bei P2 belassen (1 Hz Refresh reicht).

3. `event_loop` auf P4 erhoehen, damit Event-Verteilung (z.B.
   Crash-Detection -> SOS-Countdown) mit minimaler Latenz erfolgt.

```c
/* board_config.h — Empfohlene Prioritaeten fuer Produktion */
#define TASK_PRIO_SENSOR    5   /* war 4: 50Hz ohne Jitter bei 8000 rpm */
#define TASK_PRIO_GPS       5   /* bleibt: UART-Overrun-Vermeidung */
#define TASK_PRIO_DISPLAY   3   /* war 2: 30 FPS TFT benoetigt hoehere Prio */
#define TASK_PRIO_LOGGER    3   /* bleibt */
#define TASK_PRIO_BLE       3   /* bleibt */
#define TASK_PRIO_POWER     6   /* bleibt: hoechste fuer Shutdown */
#define TASK_PRIO_EVENT_LOOP 4  /* war 3: Crash-Latenz minimieren */
#define TASK_PRIO_OTA       4   /* bleibt */
#define TASK_PRIO_DIAG      2   /* bleibt: nicht zeitkritisch */
```

### 1.2 Task-Pinning und Core-Affinity-Tuning

Alle Tasks werden mit `xTaskCreatePinnedToCore` erstellt und sind fest
an einen Core gebunden. Das Pinning verhindert Core-Migration, die zu
Cache-Invalidierung und Jitter fuehrt.

**Begruendung der Core-Zuordnung:**

- **Core 1 (RT):** `sensor_task` und `gps_task` brauchen deterministische
  Timing. Sie teilen sich keinen Bus mit Core-0-Tasks (I2C und UART
  sind exklusiv Core 1 zugeordnet). Dadurch gibt es keinen
  Bus-Mutex-Konflikt mit Display oder SD.

- **Core 0 (UI):** `display_task`, `logger_task`, `ble_task`,
  `power_task`, `ota_task`, `event_loop` teilen sich den SPI2-Bus
  (Display + SD). Der `spi_mutex` serialisiert den Zugriff. Da diese
  Tasks auf dem gleichen Core laufen, entfaellt die Notwendigkeit
  fuer spinlock-basierte Mutexe.

**Vermeidung von Core-0-Ueberlastung:**

Core 0 traegt die meisten Tasks (6 Tasks + Idle). Bei TFT-Betrieb mit
30 FPS kann die CPU-Last auf Core 0 auf > 60% steigen. Die
Benchmark-Spec fordert < 80% pro Core.

Massnahmen zur Lastverteilung:

1. Der `event_loop` ist leichtgewichtig (nur Dispatch + Callbacks).
   Er kann bei Bedarf auf Core 1 verschoben werden, da er nur
   Queue-Operationen ausfuehrt und keine Bus-Zugaenge braucht:

```c
/* Alternative: event_loop auf Core 1, entlastet Core 0 */
BaseType_t ret = xTaskCreatePinnedToCore(
    app_event_loop_task,
    "event_loop",
    TASK_STACK_EVENT_LOOP,
    NULL,
    TASK_PRIO_EVENT_LOOP,
    &s_loop_task,
    TASK_CORE_RT);  /* Core 1 statt TASK_CORE_UI */
```

2. Der `diagnostics_task` kann auf Core 0 verschoben werden, da er
   nur Queue-Peeks und Heap-Checks macht (keine I2C/SPI-Zugaenge):

```c
xTaskCreatePinnedToCore(diagnostics_task, "diag_chk",
    TASK_STACK_DIAG, NULL, TASK_PRIO_DIAG,
    &s_task_diagnostics, TASK_CORE_UI);  /* Core 0 statt Core 1 */
```

Diese Umverteilung wuerde Core 1 auf `sensor_task` + `gps_task` +
`event_loop` reduzieren und Core 0 um `diagnostics_task` erhoehen.
Da Diagnostics nur alle 5 s laeuft und wenig CPU braucht, ist der
Netto-Effekt eine Entlastung von Core 1 zugunsten von Core 0, was
die Sensor-Jitter-Sensitivitaet verringert.

### 1.3 Interrupt-Last minimieren

Die Firmware verwendet mehrere Interrupt-Quellen. Die Benchmark-Spec
fordert < 80% CPU-Last pro Core, und Interrupts werden ausserhalb der
FreeRTOS-Scheduling-Statistik gemessen. Die ISRs muessen kurz sein,
um den Interrupt-Watchdog (IWDT) nicht auszuloesen und um Jitter in
den Echtzeit-Tasks zu vermeiden.

**Aktive Interrupt-Quellen:**

| Quelle | Pin | ISR-Aufwand | Optimierung |
|---|---|---|---|
| PCNT (RPM) | GPIO16 | Kein ISR (Hardware-Zaehler) | Optimal: null CPU-Last |
| PCNT (Tacho) | GPIO15 | Kein ISR (Hardware-Zaehler) | Optimal: null CPU-Last |
| MPU6050 INT | GPIO48 | Crash-Detection Flag setzen | Sehr kurz: 1 Flag-Set |
| GPS PPS | GPIO39 | Timestamp erfassen | Sehr kurz: 1 Register-Lese |
| Button (Menu/Select/Back/Boot) | GPIO6/7/3/0 | Entprell-Flag setzen | Kurz: Debounce in Task |

**PCNT als wichtigste Optimierung:**

Der RPM-Signal-Pfad nutzt den Hardware Pulse-Counter (PCNT), der Impulse
ohne CPU-Beteiligung zaehlt. Der `sensor_task` liest nur alle 20 ms den
Zaehlerstand und resettiert ihn. Dadurch entstehen bei 8000 U/min
(133 Hz) keinerlei Interrupts fuer die Drehzahlerfassung.

```c
/* rpm.c — Hardware-Zaehler statt ISR */
uint16_t rpm_read(void)
{
    int16_t count = 0;
    pcnt_get_counter_value(pcnt_unit, &count);
    pcnt_counter_clear(pcnt_unit);

    /* RPM = (Pulse / Pulses-per-Rev) * (1000ms / Interval_ms) * 60 */
    float rpm = (count / (float)RPM_PULSES_PER_REV)
              * (1000.0f / 20.0f) * 60.0f;
    return (uint16_t)rpm;
}
```

**MPU6050 Interrupt-Optimierung:**

Der IMU-Interrupt auf GPIO48 wird bei Bewegung oder Crash ausgeloest.
Die ISR darf nur ein Flag setzen und den Rest der Verarbeitung an
den `sensor_task` delegieren:

```c
/* Optimierter ISR: minimaler Code-Pfad */
static void IRAM_ATTR imu_isr_handler(void *arg)
{
    (void)arg;
    /* Nur BaseType-Flag setzen, keine I2C-Kommunikation in ISR */
    BaseType_t hpw = pdFALSE;
    xTaskNotifyFromISR(s_task_sensor, 0x01,
                       eSetBits, &hpw);
    portYIELD_FROM_ISR(hpw);
}
```

Das `IRAM_ATTR` Attribut stellt sicher, dass die ISR im schnellen
Instruction-RAM laeuft und nicht aus dem langsameren Flash geladen
werden muss.

**GPS-PPS-Interrupt:**

Der PPS-Pin (GPIO39) erzeugt 1 Impuls pro Sekunde. Die ISR erfasst nur
den Timestamp mit `esp_timer_get_time()` fuer die Zeitsynchronisation.
Keine weitere Verarbeitung in der ISR.

### 1.4 Yield-Strategien in der Event-Loop

Die zentrale Event-Loop (`app_event_loop.c`) verwendet eine FreeRTOS
Queue mit 32 Slots. Der Event-Loop-Task blockiert auf
`xQueueReceive(s_event_queue, &ev, portMAX_DELAY)` und wacht nur bei
eingehenden Events auf.

**Aktuelle Implementierung:**

```c
/* app_event_loop.c — Blockierende Queue, kein Polling */
while (1) {
    if (xQueueReceive(s_event_queue, &ev, portMAX_DELAY) != pdTRUE) {
        continue;
    }
    /* Subscriber-Tabelle iterieren */
    xSemaphoreTake(s_sub_mutex, portMAX_DELAY);
    for (int i = 0; i < APP_EVENT_MAX_SUBSCRIBERS; i++) {
        if (!s_subs[i].active) continue;
        if (s_subs[i].type_mask & (1U << ev.type)) {
            subscriber_t cb_copy = s_subs[i];
            xSemaphoreGive(s_sub_mutex);
            cb_copy.callback(&ev, cb_copy.user_ctx);
            xSemaphoreTake(s_sub_mutex, portMAX_DELAY);
        }
    }
    xSemaphoreGive(s_sub_mutex);
}
```

**Optimierungspotenzial:**

1. **Mutex-Release zwischen Callbacks vermeiden:** Die aktuelle
   Implementierung gibt den Mutex vor jedem Callback frei und nimmt
   ihn danach wieder. Bei N Subscribern sind das 2N Mutex-Operationen
   pro Event. Optimierung: Subscriber-Tabelle einmal kopieren, dann
   Mutex freigeben und alle Callbacks ohne Mutex-Operationen ausfuehren:

```c
/* Optimiert: einmal kopieren, dann ohne Mutex arbeiten */
typedef struct {
    app_event_callback_t cb;
    void *ctx;
} active_sub_t;

static void app_event_loop_task(void *arg)
{
    app_event_t ev;
    active_sub_t actives[APP_EVENT_MAX_SUBSCRIBERS];

    while (1) {
        if (xQueueReceive(s_event_queue, &ev, portMAX_DELAY) != pdTRUE)
            continue;

        /* Einmal kopieren unter Mutex */
        int n = 0;
        xSemaphoreTake(s_sub_mutex, portMAX_DELAY);
        for (int i = 0; i < APP_EVENT_MAX_SUBSCRIBERS; i++) {
            if (s_subs[i].active &&
                (s_subs[i].type_mask & (1U << ev.type))) {
                actives[n].cb  = s_subs[i].callback;
                actives[n].ctx = s_subs[i].user_ctx;
                n++;
            }
        }
        xSemaphoreGive(s_sub_mutex);

        /* Alle Callbacks ohne Mutex aufrufen */
        for (int i = 0; i < n; i++) {
            actives[i].cb(&ev, actives[i].ctx);
        }
    }
}
```

Diese Optimierung reduziert die Mutex-Operationen von 2N auf 2 pro
Event und ist sicher, da Subscriber-Aenderungen nur waehrend
`app_event_subscribe` / `app_event_unsubscribe` passieren, die
selten sind (nur beim Boot).

2. **Event-Batching:** Wenn mehrere Events schnell hintereinander
   eintreffen (z.B. SENSOR_UPDATE + GPS_UPDATE), koennen sie in einem
   Durchlauf verarbeitet werden:

```c
/* Batch-Modus: alle wartenden Events verarbeiten */
while (xQueueReceive(s_event_queue, &ev, 0) == pdTRUE) {
    /* Callbacks ausfuehren */
    for (int i = 0; i < n_active; i++) {
        actives[i].cb(&ev, actives[i].ctx);
    }
}
/* Dann blockieren auf naechstes Event */
if (xQueueReceive(s_event_queue, &ev, portMAX_DELAY) != pdTRUE)
    continue;
```

3. **Yield bei langer Callback-Kette:** Wenn ein Callback lange dauert
   (z.B. BLE-Notify mit SD-Schreiben), sollte die Event-Loop anderen
   Tasks die CPU ueberlassen. Das passiert automatisch, da die
   Callbacks im Event-Loop-Task laufen (P3), und hoehere Tasks
   (power P6, sensor P4) preemptieren koennen.

### 1.5 Idle-Hook fuer Hintergrund-Aufgaben

FreeRTOS ruft den Idle-Hook auf, wenn kein anderer Task bereit ist.
Der ESP-IDF Idle-Task wird auf beiden Kernen ausgefuehrt. Der Idle-Hook
kann fuer niederprioritaetare Aufgaben genutzt werden, die keinen
eigenen Task rechtfertigen.

**Kandidaten fuer Idle-Hook-Aufgaben:**

1. **LED-Animation / Heartbeat:** Die Status-LED blinkt als
   Heartbeat. Anstatt einen eigenen Task oder Timer zu verwenden,
   kann das Blinken im Idle-Hook erfolgen:

```c
/* Idle-Hook: LED-Heartbeat (nicht blockierend!) */
void vApplicationIdleHook(void)
{
    static uint32_t last_toggle = 0;
    uint32_t now = esp_timer_get_time() / 1000;
    if (now - last_toggle > 500) {
        gpio_set_level(STATUS_LED_GPIO, !gpio_get_level(STATUS_LED_GPIO));
        last_toggle = now;
    }
}
```

2. **Telemetrie-Vorsortierung:** Datensaetze fuer BLE-Telemetrie
   koennen im Idle vorbereitet werden (Serialisierung, Encoding),
   sodass der BLE-Task nur noch den fertigen Buffer senden muss.

3. **Heap-Defragmentierung:** Der ESP-IDF Heap-Allocator fuehrt im
   Idle keine aktive Defragmentierung durch. Eine leichte
   `heap_caps_check_integrity()` im Idle-Hook (alle 10 s) kann
   Corruption frueh erkennen.

**Wichtige Einschraenkung:**

Der Idle-Hook darf **niemals blockieren**. Wenn eine Aufgabe laenger
dauert als ein paar Mikrosekunden, muss sie in einen eigenen Task
ausgelagert werden. Der Idle-Hook dient nur fuer nicht-blockierende,
kurze Aktionen.

```c
/* Konfiguration in sdkconfig.defaults:
 * CONFIG_FREERTOS_USE_IDLE_HOOK=y
 */
```

---

## 2. Speicher-Optimierung

### 2.1 Heap-Fragmentierung verhindern

Der ESP32-S3 hat 512 KB internes SRAM und 8 MB externes PSRAM. FreeRTOS
und ESP-IDF nutzen den internen Heap fuer dynamische Allokationen. Die
Benchmark-Spec fordert:
- Freier Heap nach Boot: >= 50 KB
- Freier Heap nach 24 h: >= 40 KB
- Heap-Fragmentierung: < 20%

Die haeufigsten dynamischen Allokationen in der Firmware sind:

| Allokation | Groesse | Haeufigkeit | Aktuelle Methode |
|---|---|---|---|
| GPS-NMEA-Saetze | ~80 Bytes | 10 Hz | Stack-Buffer |
| Event-Queue-Eintraege | 128 + 8 Bytes | ~20 Hz | Statische Queue |
| Display-Framebuffer | 153 KB | 1x beim Init | PSRAM, statisch |
| BLE-Notify-Payload | 28-201 Bytes | 4 Hz | Stack-Buffer |
| Logger-Ringbuffer | 64 Eintraege | kontinuierlich | Statisch |
| SD-Datei-Handles | ~200 Bytes | pro Datei | FATFS-intern |

**Memory-Pools fuer haeufige Allokationen:**

Die aktuelle Implementierung verwendet bereits Stack-Buffer fuer die
meisten zeitkritischen Pfade, was gut ist. Wenn jedoch zukuenftige
Erweiterungen heap-basierte Puffer benoetigen, sollten Memory-Pools
eingesetzt werden:

1. **GPS-NMEA-Pool:** Statt jedes Mal `malloc(82)` fuer einen NMEA-Satz
   aufzurufen, wird ein Pool von 4 Puffern vor-allokiert:

```c
/* GPS NMEA Memory Pool (statisch, kein malloc zur Laufzeit) */
#define NMEA_POOL_SIZE  4
#define NMEA_MAX_LEN    82

static char s_nmea_pool[NMEA_POOL_SIZE][NMEA_MAX_LEN];
static bool s_nmea_used[NMEA_POOL_SIZE];

static char *nmea_pool_alloc(void)
{
    for (int i = 0; i < NMEA_POOL_SIZE; i++) {
        if (!s_nmea_used[i]) {
            s_nmea_used[i] = true;
            return s_nmea_pool[i];
        }
    }
    return NULL;  /* Pool leer — Caller muss ohne Buffer umgehen */
}

static void nmea_pool_free(char *buf)
{
    if (!buf) return;
    ptrdiff_t idx = (buf - s_nmea_pool[0]) / NMEA_MAX_LEN;
    if (idx >= 0 && idx < NMEA_POOL_SIZE) {
        s_nmea_used[idx] = false;
    }
}
```

2. **Event-Queue mit statischen Events:** Die aktuelle Event-Loop
   verwendet bereits eine statische Queue (`xQueueCreate` mit
   fester Groesse). Die `app_event_t` Struktur enthaelt ein
   128-Byte Payload-Array, das vom Producer in den Stack kopiert
   und vom Dispatcher in die Queue uebertragen wird. Es findet keine
   `malloc`-Operation pro Event statt. Das ist bereits optimal.

3. **Display-Buffer:** Die Framebuffer werden einmalig beim Init mit
   `heap_caps_malloc(TFT_BUF_SIZE, MALLOC_CAP_SPIRAM)` allokiert und
   danach nie freigegeben. Das Double-Buffering nutzt zwei statische
   Pointer (`s_fb_front`, `s_fb_back`), die durch `tft_swap_buffers()`
   getauscht werden. Keine dynamische Allokation beim Rendering.

**Vermeidung von Fragmentierung durch ESP-IDF-Konfiguration:**

```ini
# sdkconfig.defaults — Heap-Optionen
CONFIG_SPIRAM_USE_MALLOC=y          # PSRAM in den Heap einbinden
CONFIG_SPIRAM_MEMTRY_ALLOC_MAX=8192  # Kleine Allokationen im internen SRAM
CONFIG_HEAP_POISONING_DISABLED=y     # Keine Poison-Bytes in Produktion
CONFIG_HEAP_TASK_TRACKING=n          # Deaktiv in Produktion (Overhead)
CONFIG_HEAP_TRACING=n                # Deaktiv in Produktion
```

Durch `CONFIG_SPIRAM_USE_MALLOC=y` werden Allokationen > 8 KB
automatisch in den PSRAM umgeleitet. Das reduziert den Druck auf den
internen SRAM-Heap, wo FreeRTOS und ESP-IDF ihre Kontrollstrukturen
ablegen.

### 2.2 Stack-Groessen pro Task

Die Stack-Groessen sind in `board_config.h` definiert. Die
Benchmark-Spec fordert >= 25% freien Stack pro Task
(`uxTaskGetStackHighWaterMark()`).

**Aktuelle Stack-Konfiguration:**

| Task | Stack | Min. frei (Soll) | Begründung |
|---|---|---|---|
| sensor_task | 4096 B | 1024 B | I2C-Read (14B MPU + BMP), ADC, Filter, Queue |
| gps_task | 4096 B | 1024 B | NMEA-Parser (82B Buffer), UART-Read, Queue |
| display_task | 8192 B | 2048 B | Renderer, Widget-Drawing, SPI-Transaktionen |
| logger_task | 4096 B | 1024 B | FATFS-Aufrufe, fwrite-Buffer, Formatierung |
| ble_task | 6144 B | 1536 B | NimBLE-Callbacks, GATT-Read/Write, Notify |
| power_task | 2048 B | 512 B | ADC-Read, State-Machine, Deep-Sleep |
| event_loop | 4096 B | 1024 B | Event-Dispatch, Callback-Ausfuehrung |
| ota_task | 8192 B | 2048 B | esp_ota_write, HTTP/Flash-Operationen |
| diagnostics_task | 4096 B | 1024 B | Queue-Peek, Heap-Check, DTC-Register |

**Begruendung der Stack-Groessen:**

- `sensor_task` (4096): I2C-Burst-Read liest 14 Bytes (MPU6050) +
  6 Bytes (BMP280). Die Complementary-Filter-Berechnung verwendet
  `float`-Arithmetik. Die Queue-Operation ist leichtgewichtig.
  4 KB bieten ausreichend Reserve.

- `display_task` (8192): Der Renderer zeichnet Widgets, die
  Text-Rendering mit Font-Tabellen durchfuehren. SPI-Transaktionen
  legen `spi_transaction_t` auf den Stack. Der Bounce-Buffer wird
  zusaetzlich allokiert, aber `memcpy` in `tft_push_region` arbeitet
  mit Pointern, nicht mit grossen Stack-Puffern.

- `ble_task` (6144): NimBLE ruft GATT-Callbacks synchron auf. Diese
  Callbacks lesen Queue-Daten, serialisieren sie und senden Notify.
  Die 6 KB beruecksichtigen den NimBLE-Stack-Overhead.

- `ota_task` (8192): OTA-Operationen verwenden `esp_ota_write` und
  SHA-256-Berechnung, die signifikanten Stack brauchen.

**Stack-Overflow-Detection:**

Jeder Task sollte beim Start und periodisch seinen Stack-Wasserstand
pruefen. Die `diagnostics_task` uebernimmt dies:

```c
/* diagnostics_task — Stack-Monitoring */
static void check_stack_watermarks(void)
{
    TaskHandle_t tasks[] = {
        s_task_sensor, s_task_gps, s_task_display,
        s_task_ble, s_task_power, s_task_logger, s_task_ota
    };
    const char *names[] = {
        "sensor", "gps", "display",
        "ble", "power", "logger", "ota"
    };

    for (int i = 0; i < 7; i++) {
        if (tasks[i] == NULL) continue;
        UBaseType_t hw = uxTaskGetStackHighWaterMark(tasks[i]);
        if (hw < 256) {
            ESP_LOGW(TAG, "Stack low: %s = %u words free",
                     names[i], (unsigned)hw);
            dtc_report(DTC_B0102);
        }
    }
}
```

`uxTaskGetStackHighWaterMark()` gibt den minimalen verbleibenden
Stack in Words (4 Bytes auf ESP32-S3) zurueck. Ein Wert < 256 Words
(1024 Bytes) ist kritisch und sollte einen DTC ausloesen.

Die FreeRTOS-Konfiguration muss Stack-Overflow-Hooks aktivieren:

```ini
# sdkconfig.defaults
CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY=y  # Canary-Wert am Stack-Ende
CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK=y     # Hardware-Watchpoint
```

### 2.3 PSRAM-Nutzung

Der ESP32-S3-WROOM-1 N16R8 hat 8 MB Octal PSRAM. PSRAM ist langsamer
als internes SRAM (ca. 40 MHz vs. 240 MHz), aber mit 8 MB ausreichend
gross fuer grosse Puffer, die nicht zeitkritisch sind.

**Aktuelle PSRAM-Allokationen:**

| Komponente | Groesse | Modus | Begruendung |
|---|---|---|---|
| TFT Front-Buffer | 153.600 B (240x320x2) | heap_caps_malloc(SPIRAM) | DMA kann nicht von PSRAM lesen → Bounce-Buffer |
| TFT Back-Buffer | 153.600 B | heap_caps_malloc(SPIRAM) | Paint-Buffer, wird von CPU beschrieben |
| GPX-Track-Buffer | variabel (geplant) | PSRAM | Track-Punkte fuer Export |
| Font-Tabellen | ~120 KB (geplant) | PSRAM (Flash-mapped) | Font-Daten sind im Flash, mmap reicht |

**TFT Double-Buffering mit PSRAM:**

Die Implementierung in `tft_driver.c` allokiert zwei Framebuffer in
PSRAM und einen Bounce-Buffer im internen SRAM:

```c
/* tft_driver.c — PSRAM + Bounce-Buffer Architektur */
s_fb_front = heap_caps_malloc(TFT_BUF_SIZE, MALLOC_CAP_SPIRAM);
s_fb_back  = heap_caps_malloc(TFT_BUF_SIZE, MALLOC_CAP_SPIRAM);
s_bounce   = heap_caps_malloc(TFT_BOUNCE_SIZE,
                                MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
```

Warum der Bounce-Buffer? Der SPI-DMA-Controller kann nicht direkt von
PSRAM lesen (nur von internem SRAM). Der `tft_push_region`-Code
kopiert daher Daten in Chunks von PSRAM in den SRAM-Bounce-Buffer und
startet dann den DMA-Transfer:

```c
/* tft_push_region — Bounce-Buffer Transfer */
for (uint16_t row = 0; row < h; row += TFT_BOUNCE_LINES) {
    uint16_t lines = MIN(h - row, TFT_BOUNCE_LINES);
    size_t copy_bytes = (size_t)w * lines * 2;

    memcpy(s_bounce,
           &s_fb_front[(y + row) * TFT_WIDTH + x],
           copy_bytes);

    spi_transaction_t t = {0};
    t.length    = copy_bytes * 8;
    t.tx_buffer = s_bounce;
    spi_device_polling_transmit(s_spi, &t);
}
```

Der Bounce-Buffer fasst 40 Zeilen (TFT_BOUNCE_LINES = 40), was
240 * 40 * 2 = 19.200 Bytes entspricht. Bei 240x320 Pixel sind 8
Bounce-Durchlaeufe pro Vollbild noetig.

**GPX-Track-Buffer in PSRAM:**

Fuer Track-Exporte (GPX-Datei) werden Trackpunkte zwischengespeichert.
Bei einer 24-Stunden-Tour mit 1 Hz GPS sind das ~86.400 Punkte.
Ein GPX-Trackpoint benoetigt ca. 80-120 Bytes. Bei 100 Bytes pro Punkt
sind das 8,6 MB — das passt knapp in die 8 MB PSRAM, aber zusammen mit
den Framebuffern (2x 153 KB = 307 KB) ist Platz ausreichend.

```c
/* GPX-Track-Buffer in PSRAM (geplante Implementierung) */
#define GPX_MAX_POINTS  86400
#define GPX_POINT_SIZE  100  /* Bytes pro Trackpoint */

typedef struct {
    float lat;
    float lon;
    float alt;
    float speed;
    uint32_t timestamp;
} gpx_point_t;

static gpx_point_t *s_gpx_buffer = NULL;
static uint32_t s_gpx_count = 0;

void gpx_buffer_init(void)
{
    s_gpx_buffer = heap_caps_malloc(
        GPX_MAX_POINTS * sizeof(gpx_point_t),
        MALLOC_CAP_SPIRAM);
    if (s_gpx_buffer) {
        ESP_LOGI(TAG, "GPX buffer: %u bytes in PSRAM",
                 GPX_MAX_POINTS * sizeof(gpx_point_t));
    }
}
```

**Font-Tabellen:**

Font-Daten sind im Flash gespeichert und werden via Memory-Mapping
(XIP — eXecute In Place) zugegriffen. Es ist nicht noetig, sie in PSRAM
zu kopieren. Der Flash ist mit 16 MB ausreichend gross, und das
Memory-Mapping hat keinen Performance-Nachteil gegenueber PSRAM
(beide sind langsamer als SRAM).

**PSRAM-Konfiguration:**

```ini
# sdkconfig.defaults — PSRAM
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=1              # Octal PSRAM Mode
CONFIG_SPIRAM_SPEED_80M=1             # 80 MHz PSRAM-Takt
CONFIG_SPIRAM_USE_MALLOC=y            # malloc kann PSRAM nutzen
CONFIG_SPIRAM_MEMTRY_ALLOC_MAX=8192   # <8KB → internes SRAM
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=4096  # <4KB immer intern
```

Die Kombination aus `MEMTRY_ALLOC_MAX` und `MALLOC_ALWAYSINTERNAL`
stellt sicher, dass kleine, zeitkritische Allokationen (z.B.
FreeRTOS-Queue-Knoten) im internen SRAM bleiben, waehrend grosse
Puffer in den PSRAM ausgelagert werden.

---

## 3. Rendering-Optimierung

### 3.1 TFT: Dirty-Rect-Updates statt Full-Screen-Refresh

Das TFT-Display (240x320 Pixel, RGB565) benoetigt 153.600 Bytes pro
Vollbild. Bei 40 MHz SPI-Takt entspricht das einer theoretischen
Uebertragungszeit von ~15 ms (~65 FPS). Mit Dirty-Rect-Tracking wird
die effektive Refresh-Rate deutlich erhoeht.

**Aktuelle Implementierung:**

Der Renderer in `renderer.c` verwaltet eine Dirty-Rect-Liste mit bis
zu 16 Eintraegen (`MAX_DIRTY_RECTS = 16`). Widgets rufen
`renderer_invalidate(x, y, w, h)` auf, um Bereiche als dirty zu
markieren. Beim `renderer_flush(partial)` werden ueberlappende Rechtecke
vereinigt und nur die betroffenen Regionen per `tft_push_region()`
uebertragen.

```c
/* renderer.c — Dirty-Rect Compositor */
static void dirty_merge(void)
{
    bool merged = true;
    while (merged && s_dirty.count > 1) {
        merged = false;
        for (int i = 0; i < s_dirty.count - 1; i++) {
            for (int j = i + 1; j < s_dirty.count; j++) {
                if (rects_overlap(&s_dirty.rects[i],
                                  &s_dirty.rects[j])) {
                    s_dirty.rects[i] = rect_merge(
                        &s_dirty.rects[i], &s_dirty.rects[j]);
                    s_dirty.rects[j] = s_dirty.rects[s_dirty.count - 1];
                    s_dirty.count--;
                    j--;
                    merged = true;
                }
            }
        }
    }
}
```

**Performance-Analyse:**

Der Merge-Algorithmus ist O(n²) mit n <= 16. Bei 16 Rechtecken sind das
maximal 256 Vergleiche — vernachlaessigbar. Bei einem typischen
Dashboard-Update (4 Zahlenbereiche, je ~80x40 Pixel) sind nur
25.600 Bytes statt 153.600 Bytes zu uebertragen (Faktor 6 weniger).

Bei 40 MHz SPI-Takt:
- Vollbild: 153.600 Bytes → ~15 ms → ~65 FPS
- Typisches Dirty-Rect: 25.600 Bytes → ~2,5 ms → ~400 FPS (theoretisch)
- Realistisch mit Overhead: ~5-8 ms → ~120-200 FPS

Die Benchmark-Spec fordert > 30 FPS (Vollbild) und > 60 FPS (Dirty-Rect).
Beide Ziele sind mit der aktuellen Architektur erreichbar.

**Optimierungspotenzial:**

1. **Display-Task auf 30 FPS umstellen:** Die aktuelle Implementierung
   in `display.c` verwendet 1 Hz (`vTaskDelay(1000)`) fuer E-Paper.
   Bei TFT-Betrieb muss auf 30 FPS umgestellt werden:

```c
/* display.c — 30 FPS fuer TFT, 1 Hz fuer E-Paper */
void display_task(void *arg)
{
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t tft_period = pdMS_TO_TICKS(33);  /* 30 FPS */
    const TickType_t epd_period = pdMS_TO_TICKS(1000); /* 1 Hz */

    while (1) {
        /* Daten pullen */
        bool have_gps = (gps_data_queue &&
            xQueuePeek(gps_data_queue, &gps, 0) == pdTRUE);
        bool have_sens = (sensor_data_queue &&
            xQueuePeek(sensor_data_queue, &sens, 0) == pdTRUE);

        /* Nur zeichnen wenn dirty oder neue Daten */
        renderer_clear();
        /* ... Widgets zeichnen ... */
        renderer_flush(true);

        /* Period: TFT=33ms, E-Paper=1000ms */
        TickType_t period = (s_active_display == DISPLAY_TYPE_TFT)
                          ? tft_period : epd_period;
        vTaskDelayUntil(&last_wake, period);
    }
}
```

2. **Dirty-Rect-Granularitaet verbessern:** Die aktuelle Implementierung
   in `main.c` ruft `renderer_invalidate(0, 0, TFT_WIDTH, TFT_HEIGHT)`
   auf, was das gesamte Display als dirty markiert. Das entspricht einem
   Vollbild-Refresh und macht das Dirty-Rect-System wirkungslos.

```c
/* main.c — Aktuell: Full-Screen Invalidation (suboptimal) */
static void on_gps_update(const app_event_t *ev, void *ctx)
{
    renderer_invalidate(0, 0, TFT_WIDTH, TFT_HEIGHT);  /* ← zu gross */
}

/* Optimiert: Nur betroffene Widget-Bereiche invalidieren */
static void on_gps_update(const app_event_t *ev, void *ctx)
{
    /* Speed-Anzeige: 80x40 Pixel bei (5, 10) */
    renderer_invalidate(5, 10, 80, 40);
    /* Altitude-Anzeige: 60x30 Pixel bei (180, 10) */
    renderer_invalidate(180, 10, 60, 30);
}
```

3. **Partial-Frame-Buffer fuer betroffene Regionen:** Statt das gesamte
   Back-Buffer zu loeschen (`renderer_clear()`), sollte nur der
   betroffene Bereich geloescht und neu gezeichnet werden:

```c
/* display.c — Selektives Clear statt Full-Clear */
void display_task(void *arg)
{
    while (1) {
        /* Nur dirty-Bereiche neu zeichnen */
        bool have_gps = (gps_data_queue &&
            xQueuePeek(gps_data_queue, &gps, 0) == pdTRUE);
        bool have_sens = (sensor_data_queue &&
            xQueuePeek(sensor_data_queue, &sens, 0) == pdTRUE);

        /* Vorherige Werte mit neuen vergleichen */
        static uint16_t prev_speed = 0xFFFF;
        static uint16_t prev_rpm = 0xFFFF;

        if (have_gps && (uint16_t)gps.speed_kmh != prev_speed) {
            /* Nur Speed-Bereich neu zeichnen */
            renderer_fill_rect(5, 10, 80, 40, COLOR_BLACK);
            widgets_draw_speed((uint16_t)gps.speed_kmh, 5, 10, 4);
            renderer_invalidate(5, 10, 80, 40);
            prev_speed = (uint16_t)gps.speed_kmh;
        }

        if (have_sens && sens.rpm != prev_rpm) {
            renderer_fill_rect(60, 50, 180, 16, COLOR_BLACK);
            widgets_draw_rpm_bar(sens.rpm, 8000, 60, 50, 180, 16);
            renderer_invalidate(60, 50, 180, 16);
            prev_rpm = sens.rpm;
        }

        renderer_flush(true);
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}
```

Diese Strategie reduziert die Anzahl der zu uebertragenden Bytes
drastisch. Nur geaenderte Widget-Bereiche werden neu gezeichnet und
uebertragen.

### 3.2 E-Paper: Differential-Update-Strategien

Das E-Paper-Display (296x128 Pixel, 1-bit monochrom) wird ueber den
selben SPI2-Bus wie TFT und SD angesteuert, aber mit 4 MHz Takt (vs.
40 MHz fuer TFT). E-Paper-Updates sind physisch langsam, da die
E-Ink-Pigmente Zeit fuer die Umschaltung brauchen.

**Aktuelle Implementierung:**

Der Renderer unterstuetzt bereits Partial- und Full-Refresh:

```c
/* renderer.c — E-Paper Refresh-Logik */
void renderer_flush(bool partial)
{
    if (s_active_display == DISPLAY_TYPE_EPAPER) {
        if (partial && s_dirty.count <= 4) {
            /* Partial refresh fuer kleine Aenderungen */
            for (int i = 0; i < s_dirty.count; i++) {
                epaper_display_partial(
                    epaper_get_framebuffer(),
                    s_dirty.rects[i].x, s_dirty.rects[i].y,
                    s_dirty.rects[i].w, s_dirty.rects[i].h);
            }
        } else {
            /* Full refresh bei grossen Aenderungen */
            epaper_display_frame(epaper_get_framebuffer());
        }
    }
}
```

**Strategie fuer Differential-Updates:**

1. **Partial-Refresh fuer kleine Widgets:** Uhrzeit, Trip-Zaehler,
   und Tankstand aktualisieren sich haeufig, aber nur in kleinen
   Bereichen (z.B. 40x20 Pixel fuer die Uhrzeit). Partial-Refresh
   dauert <= 300 ms (Benchmark-Should-Ziel) vs. <= 2,0 s fuer
   Full-Refresh.

2. **Full-Refresh nur bei Screen-Wechsel:** Wenn der Anzeigemodus
   gewechselt wird (z.B. TOUR → NAV), ist ein Full-Refresh noetig,
   da sich grosse Teile des Displays aendern. Full-Refresh dauert
   <= 2,0 s (Benchmark-Must-Ziel).

3. **Ghosting-Reduktion:** Nach 50 Partial-Refreshes sollte ein
   Full-Refresh ausgefuehrt werden, um Ghosting zu entfernen:

```c
/* Ghosting-Counter: alle 50 Partial-Refreshes ein Full-Refresh */
static uint8_t s_partial_count = 0;
#define GHOSTING_REFRESH_INTERVAL  50

void renderer_flush(bool partial)
{
    if (s_active_display == DISPLAY_TYPE_EPAPER) {
        bool do_full = !partial || s_dirty.count > 4
                     || s_partial_count >= GHOSTING_REFRESH_INTERVAL;

        if (do_full) {
            epaper_display_frame(epaper_get_framebuffer());
            s_partial_count = 0;
        } else {
            for (int i = 0; i < s_dirty.count; i++) {
                epaper_display_partial(/* ... */);
            }
            s_partial_count++;
        }
    }
}
```

### 3.3 Animationen: Frame-Rate-Adaption bei hoher Last

Die Benchmark-Spec fordert < 80% CPU-Last pro Task. Bei hohen
Sensor-Raten (50 Hz) und gleichzeitigem TFT-Rendering (30 FPS) kann
die Last auf Core 0 steigen. Animationen sind der erste Kandidat fuer
Last-Reduktion.

**Aktuelle Implementierung:**

Der Renderer verwaltet eine Animation-Queue mit bis zu 8 Animationen
(`MAX_ANIMATIONS = 8`). Die `renderer_anim_tick(now_ms)`-Funktion
ruft fuer jede aktive Animation ihren Callback auf.

**Frame-Rate-Adaption:**

```c
/* Animation-Frame-Rate an CPU-Last anpassen */
typedef enum {
    ANIM_FPS_FULL = 60,    /* Normalbetrieb */
    ANIM_FPS_REDUCED = 15, /* Bei CPU-Last > 70% */
    ANIM_FPS_OFF = 0,      /* Bei CPU-Last > 85% */
} anim_fps_mode_t;

static anim_fps_mode_t s_anim_mode = ANIM_FPS_FULL;
static uint32_t s_last_anim_tick = 0;

void renderer_anim_tick_adaptive(uint32_t now_ms)
{
    uint32_t min_interval;
    switch (s_anim_mode) {
    case ANIM_FPS_REDUCED: min_interval = 67; break;  /* 15 FPS */
    case ANIM_FPS_OFF:     return;                     /* deaktiviert */
    default:               min_interval = 16; break;  /* 60 FPS */
    }

    if (now_ms - s_last_anim_tick < min_interval) return;
    s_last_anim_tick = now_ms;

    renderer_anim_tick(now_ms);
}

/* CPU-Last-basierte Anpassung (vom diagnostics_task gesetzt) */
void renderer_set_anim_mode(anim_fps_mode_t mode)
{
    if (mode != s_anim_mode) {
        ESP_LOGI(TAG, "Animation FPS mode: %d -> %d",
                 s_anim_mode, mode);
        s_anim_mode = mode;
    }
}
```

**Trigger fuer Modus-Wechsel:**

Der `diagnostics_task` ueberwacht die CPU-Last mit
`vTaskGetRunTimeStats()` und setzt den Animationsmodus:

```c
/* diagnostics_task — CPU-Last-Monitoring */
#define CPU_LOAD_THRESHOLD_REDUCE  70  /* % */
#define CPU_LOAD_THRESHOLD_DISABLE 85  /* % */

static void check_cpu_load_and_adapt(void)
{
    TaskStatus_t task_stats[16];
    UBaseType_t n = uxTaskGetSystemState(task_stats, 16, NULL);

    uint32_t total_runtime = 0;
    for (int i = 0; i < n; i++) {
        total_runtime += task_stats[i].ulRunTimeCounter;
    }

    /* Core 0 Last schaetzen (display + ble + logger + power + idle) */
    uint32_t core0_runtime = 0;
    for (int i = 0; i < n; i++) {
        if (task_stats[i].xCoreID == 0) {
            core0_runtime += task_stats[i].ulRunTimeCounter;
        }
    }

    uint32_t load_pct = (total_runtime > 0)
        ? (core0_runtime * 100 / total_runtime) : 0;

    if (load_pct > CPU_LOAD_THRESHOLD_DISABLE) {
        renderer_set_anim_mode(ANIM_FPS_OFF);
    } else if (load_pct > CPU_LOAD_THRESHOLD_REDUCE) {
        renderer_set_anim_mode(ANIM_FPS_REDUCED);
    } else {
        renderer_set_anim_mode(ANIM_FPS_FULL);
    }
}
```

---

## 4. Sensor-Latenz-Optimierung

### 4.1 RPM-Signal: Minimale Latenz vom Impuls bis Display-Update

Die Benchmark-Spec fordert eine RPM-Latenz < 200 ms vom Frequenzsprung
bis zum sichtbaren Display-Update. Der Signal-Pfad ist:

1. Pickup-Sensor → Optokoppler → PCNT-Hardware-Zaehler (0 ms CPU)
2. `sensor_task` liest PCNT alle 20 ms (max 20 ms Latenz)
3. Filterung (Median/EMA) ( < 1 ms)
4. `xQueueOverwrite(sensor_data_queue)` ( < 1 ms)
5. `display_task` liest Queue und rendert ( < 33 ms bei 30 FPS)
6. SPI-Transfer zum Display ( < 15 ms bei Dirty-Rect)

**Gesamtlatenz (schaetzung):** 20 + 1 + 1 + 33 + 15 = ~70 ms

Damit wird das < 200 ms Ziel mit Reserve erreicht.

**Hardware-Timer fuer RPM-Zaehlung:**

Die aktuelle Implementierung nutzt bereits den PCNT-Hardware-Zaehler,
der Impulse ohne CPU-Beteiligung zaehlt. Der `sensor_task` liest nur
alle 20 ms den Zaehlerstand. Es gibt kein Software-Polling oder
ISR-basiertes Zaehlen.

```c
/* rpm.c — PCNT-Initialisierung (vereinfacht) */
void rpm_init(void)
{
    pcnt_config_t cfg = {
        .pulse_gpio_num = RPM_PCNT_PIN,
        .ctrl_gpio_num  = PCNT_PIN_NOT_USED,
        .channel        = PCNT_CHANNEL_0,
        .unit           = PCNT_UNIT_0,
        .pos_mode       = PCNT_COUNT_INC,   /* Steigende Flanke zaehlen */
        .neg_mode       = PCNT_COUNT_DIS,
        .lctrl_mode     = PCNT_MODE_KEEP,
        .hctrl_mode     = PCNT_MODE_KEEP,
        .counter_h_lim  = RPM_PCNT_HIGH_LIMIT,
        .counter_l_lim  = 0,
    };
    pcnt_unit_config(&cfg);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);
}
```

**Optimierung der RPM-Berechnung:**

Die aktuelle Berechnung erfolgt im `sensor_task` alle 20 ms. Bei
niedrigen Drehzahlen (Leerlauf ~1300 U/min = ~21 Hz) liefert der PCNT
in 20 ms nur ca. 0-1 Pulse, was zu einer quantisierten Anzeige fuehrt.

Loesung: Laengeres Messfenster bei niedrigen Drehzahlen:

```c
/* rpm.c — Adaptives Messfenster */
uint16_t rpm_read_adaptive(void)
{
    int16_t count = 0;
    pcnt_get_counter_value(PCNT_UNIT_0, &count);
    pcnt_counter_clear(PCNT_UNIT_0);

    /* Bei niedrigen Drehzahlen: 100 ms Fenster */
    /* Bei hohen Drehzahlen: 20 ms Fenster */
    uint32_t window_ms = (count < 5) ? 100 : 20;

    float rpm = (count / (float)RPM_PULSES_PER_REV)
              * (1000.0f / window_ms) * 60.0f;
    return (uint16_t)rpm;
}
```

Dies erfordert, dass der `sensor_task` den RPM-Lesezyklus anpasst: bei
niedrigen Drehzahlen wird RPM nur alle 100 ms (jeder 5. Tick) gelesen,
bei hohen alle 20 ms.

### 4.2 GPS: NMEA-Parsing-Optimierung

Das GPS-Modul kommuniziert ueber UART1 mit NMEA 0183-Saetzen. Die
Benchmark-Spec fordert >= 1 Hz Update-Rate (Standard) und >= 5 Hz
(High-Rate, Should-Ziel).

**Aktuelle Implementierung:**

Der `gps_task` liest in einer Schleife UART-Daten mit 100 ms Timeout
und parst NMEA-Saetze. Die Baudrate ist 38400 (Default), per UBX auf
115200 erhoehbar.

**Optimierungsstrategien:**

1. **Nur RMC- und GGA-Saetze parsen:** NMEA-Module senden verschiedene
   Satz-Typen (GGA, GLL, GSA, GSV, RMC, VTG, ZDA). Fuer den Bordcomputer
   sind nur RMC (Position, Speed, Course, Zeit) und GGA (Hoehe,
   Satelliten, Fix-Qualitaet) relevant. Alle anderen Saetze sollten
   verworfen werden, sobald der Praefix erkannt ist:

```c
/* nmea_parser.c — Fruehzeitige Filterung */
bool nmea_should_parse(const char *sentence)
{
    /* Nur $GNRMC und $GNGGA parsen */
    if (strncmp(sentence, "$GNRMC", 6) == 0) return true;
    if (strncmp(sentence, "$GPGGA", 6) == 0) return true;
    if (strncmp(sentence, "$GPRMC", 6) == 0) return true;
    if (strncmp(sentence, "$GPGGA", 6) == 0) return true;
    return false;
}

void gps_parse_line(const char *line)
{
    if (!nmea_should_parse(line)) return;  /* Verwerfen */

    if (line[3] == 'R') {
        /* RMC: Position, Speed, Course */
        nmea_parse_rmc(line, &g_gps_data);
    } else if (line[3] == 'G') {
        /* GGA: Hoehe, Satelliten, Fix */
        nmea_parse_gga(line, &g_gps_data);
    }

    xQueueOverwrite(gps_data_queue, &g_gps_data);
    app_event_dispatch_simple(APP_EVENT_GPS_UPDATE);
}
```

2. **UART-Event-getrieben statt Polling:** Die aktuelle Implementierung
   verwendet `uart_read_bytes` mit 100 ms Timeout (Polling). Besser
   ist ein UART-Event-Queue, der den Task nur bei eintreffenden Daten
   aufweckt:

```c
/* gps_task.c — Event-getriebener UART-Empfang */
void gps_task(void *arg)
{
    uart_event_t event;
    char rxbuf[256];

    while (1) {
        if (xQueueReceive(gps_uart_queue, &event, portMAX_DELAY)) {
            if (event.type == UART_DATA) {
                int len = uart_read_bytes(GPS_UART_NUM,
                    (uint8_t *)rxbuf, event.size, 0);
                if (len > 0) {
                    rxbuf[len] = '\0';
                    nmea_parse_buffer(rxbuf, len);
                }
            }
        }
    }
}
```

3. **Minmea als leichtgewichtige Alternative:** Die minmea-Bibliothek
   ist eine minimale NMEA-Parser-Implementierung mit geringem
   Code-Footprint und minimaler Stack-Nutzung. Sie ist eine gute
   Alternative zu vollwertigen NMEA-Bibliotheken:

```c
/* minmea-Integration (geplant) */
#include "minmea.h"

void nmea_parse_rmc(const char *line, gps_data_t *gd)
{
    struct minmea_sentence_rmc frame;
    if (minmea_parse_rmc(&frame, line)) {
        gd->lat = minmea_tocoord(&frame.latitude);
        gd->lon = minmea_tocoord(&frame.longitude);
        gd->speed_kmh = minmea_tofloat(&frame.speed) * 3.6f;
        gd->course_deg = minmea_tofloat(&frame.course);
        gd->timestamp_ms = get_millis();
    }
}
```

4. **Baudrate auf 115200 erhoehen:** Bei 5 Hz oder 10 Hz Update-Rate
   steigt das NMEA-Datenvolumen. Bei 38400 Baud und 10 Hz sind pro
   Sekunde ~1000 Bytes zu uebertragen (10 Saetze a ~100 Bytes). Das
   entspricht ~10.000 Bits/s, was knapp unter der 38400 Baud-Grenze
   liegt. Bei 115200 Baud gibt es ausreichend Reserve.

```c
/* UBX-Konfiguration fuer 115200 Baud und 5 Hz (geplant) */
void gps_configure_high_rate(void)
{
    /* UBX-CFG-PRT: Setze UART-Baudrate auf 115200 */
    uint8_t cfg_prt[] = {
        0xB5, 0x62, 0x06, 0x00, /* UBX, CFG, PRT */
        0x14, 0x00,              /* Length */
        0x01, 0x00, 0x00, 0x00,  /* Port: UART1 */
        0xD0, 0x08, 0x00, 0x00,  /* Baudrate: 115200 */
        /* ... weitere Felder ... */
    };
    uart_write_bytes(GPS_UART_NUM, (const char *)cfg_prt, sizeof(cfg_prt));
    vTaskDelay(pdMS_TO_TICKS(100));
    uart_set_baudrate(GPS_UART_NUM, 115200);
}
```

### 4.3 IMU: Sensor-Fusion effizient implementieren

Der IMU (MPU6050, 6-Achsen) liefert Beschleunigung und Gyroskop-Daten.
Die Firmware verwendet einen Complementary-Filter fuer Roll/Pitch und
eine Schwellwert-basierte Crash-Detection.

**Complementary-Filter statt Kalman:**

Die Firmware verwendet bereits den Complementary-Filter (definiert in
`filters.h` / `filters.c`). Das ist die richtige Wahl fuer ein
eingebettetes System:

| Filter | CPU-Last | Speicher | Genauigkeit | Komplexitaet |
|---|---|---|---|---|
| Complementary | Sehr gering | ~24 Bytes | Gut fuer Neigung | Trivial |
| Kalman 1D | Mittel | ~40 Bytes | Sehr gut | Hoch |
| Madgwick | Hoch | ~200 Bytes | Sehr gut (9-DoF) | Komplex |

```c
/* filters.h — Complementary-Filter (bereits implementiert) */
typedef struct {
    float alpha;   /* Gewichtung: 0.95 typisch */
    float angle;   /* Aktueller Winkel */
    float dt;      /* Zeitintervall [s] */
} complementary_filter_t;

float comp_filter_update(complementary_filter_t *cf,
                         float gyro_rate, float accel_angle)
{
    /* alpha * (angle + gyro*dt) + (1-alpha) * accel_angle */
    cf->angle = cf->alpha * (cf->angle + gyro_rate * cf->dt)
              + (1.0f - cf->alpha) * accel_angle;
    return cf->angle;
}
```

Der Complementary-Filter benoetigt nur 2 Multiplikationen und 2
Additionen pro Update — vernachlaessigbare CPU-Last im Vergleich zu
einem Kalman-Filter, der Matrix-Operationen erfordert.

**Crash-Detection Optimierung:**

Die Crash-Detection vergleicht die Gesamtbeschleunigung mit einem
Schwellwert (> 3g). Die Berechnung ist trivial:

```c
/* imu.c — Crash-Detection */
float accel_magnitude(float ax, float ay, float az)
{
    /* sqrt(ax² + ay² + az²) — ohne sqrt-Bibliothek:
     * Verwende Approximation oder schnelle inverse sqrt */
    float sum = ax*ax + ay*ay + az*az;

    /* Quake fast inverse sqrt fuer Geschwindigkeit */
    /* (oder sqrtf fuer Einfachheit) */
    return sqrtf(sum);
}

bool imu_check_crash(float ax, float ay, float az)
{
    float mag = accel_magnitude(ax, ay, az);
    return mag > 3.0f;  /* 3g Schwellwert */
}
```

Fuer minimale Latenz kann die Crash-Detection direkt im
`sensor_task`-Zyklus erfolgen, ohne Filter-Verzoegerung. Bei 50 Hz
Sampling-Rate ist die maximale Crash-Latenz 20 ms (ein Zyklus), was
das < 500 ms Benchmark-Ziel deutlich unterschreitet.

---

## 5. BLE-Durchsatz-Optimierung

### 5.1 MTU-Verhandlung

Die Benchmark-Spec fordert eine MTU von 247 Bytes (Should-Ziel). Die
aktuelle Implementierung in `ble_service.c` setzt bereits
`BLE_MTU_SIZE = 247`:

```c
/* ble_service.c — MTU-Konfiguration */
#define BLE_MTU_SIZE  247

/* Bei Connect: MTU-Exchange anfordern */
ble_att_set_preferred_mtu(BLE_MTU_SIZE);
int rc = ble_gattc_exchange_mtu(s_conn_handle);
```

Bei einer MTU von 247 Bytes sind 244 Bytes Payload pro Notification
moeglich (247 - 3 Bytes ATT-Header). Die Telemetry-Struktur
(`TLM_LIVE`) ist 28 Bytes, was in einer einzigen Notification passt.

**Optimierung:**

Bei der Initialisierung kann die maximale MTU angefordert werden.
Der ESP32-S3 unterstuetzt bis zu 512 Bytes MTU:

```c
/* Erhoehte MTU fuer grossen Durchsatz (z.B. OTA, Track-Export) */
#define BLE_MTU_SIZE_MAX  512

/* Beim Connect: Maximale MTU anfordern */
ble_att_set_preferred_mtu(BLE_MTU_SIZE_MAX);
int rc = ble_gattc_exchange_mtu(s_conn_handle);
```

Bei 512 Bytes MTU sind 509 Bytes Payload pro Notification moeglich.
Das verdoppelt den Durchsatz fuer OTA-Updates und Track-Exporte.

### 5.2 Connection-Parameter

Die aktuellen Connection-Parameter sind in `ble_service.c` definiert:

```c
#define BLE_CONN_INTERVAL_MIN   12   /* 15 ms (12 * 1.25 ms) */
#define BLE_CONN_INTERVAL_MAX   24   /* 30 ms */
#define BLE_CONN_LATENCY        0
#define BLE_CONN_TIMEOUT        400  /* 4 s (400 * 10 ms) */
```

**Begruendung:**

- **Interval 15-30 ms:** Schnelle Telemetrie-Updates (4 Hz = 250 ms).
  Bei 15 ms Intervall sind 16 Connection-Events pro Telemetrie-Update
  moeglich — mehr als ausreichend fuer Telemetrie + Nav + Media.

- **Latency 0:** Keine Latenz-Schoenung. Jedes Connection-Event wird
  genutzt. Das ist wichtig fuer Echtzeit-Telemetrie und
  Phone-Notifications, die sofort zugestellt werden muessen.

- **Supervision Timeout 4 s:** Schneller Disconnect bei echtem
  Abbruch. Bei 5 s (Benchmark-Spec Should) oder 4 s (aktuell) wird
  die Verbindung schnell getrennt und kann neu aufgebaut werden.

**Optimierung fuer Durchsatz:**

 Fuer maximalen Durchsatz (OTA-Update, Track-Export) sollte das
 Connection-Interval auf das Minimum (7.5 ms) gesetzt werden:

```c
/* High-Throughput Connection Parameters (fuer OTA) */
#define BLE_CONN_INTERVAL_FAST_MIN  6    /* 7.5 ms (6 * 1.25 ms) */
#define BLE_CONN_INTERVAL_FAST_MAX  12   /* 15 ms */
#define BLE_CONN_TIMEOUT_FAST        500  /* 5 s */

/* Bei OTA-Start: auf schnelle Parameter wechseln */
void ble_set_connection_fast(void)
{
    struct ble_gap_upd_params params = {
        .itvl_min = BLE_CONN_INTERVAL_FAST_MIN,
        .itvl_max = BLE_CONN_INTERVAL_FAST_MAX,
        .latency = 0,
        .supervision_timeout = BLE_CONN_TIMEOUT_FAST,
    };
    ble_gap_update_params(ble_get_conn_handle(), &params);
}
```

Bei 7.5 ms Intervall und 509 Bytes Payload (MTU 512) sind theoretisch
bis zu 68 kB/s moeglich (509 / 0.0075). Realistisch sind 20-40 kB/s
wegen Protocol-Overhead.

### 5.3 Notify statt Indicate

Die aktuelle Implementierung verwendet fuer alle Telemetrie-Daten
`BLE_GATT_CHR_F_NOTIFY` (Notify). Das ist korrekt:

- **Notify:** Ungesichert, keine Bestaetigung vom Client. Hoeherer
  Durchsatz, aber keine Garantie fuer Zustellung.
- **Indicate:** Gesichert, Client muss bestaetigen. Niedrigerer
  Durchsatz, aber zuverlaessig.

 Fuer Echtzeit-Telemetrie (4 Hz, 28 Bytes) ist Notify richtig:
die Daten sind zeitkritisch, und ein verpasstes Update wird beim
naechsten Zyklus ohnehin ueberschrieben. Die Benchmark-Spec fordert
0 Verluste von 100 Notifys — bei guter Verbindung und 15 ms Intervall
ist das problemlos erreichbar.

Die einzige Ausnahme ist die DTC-Liste (0x7001), die sowohl Notify
als auch Indicate unterstuetzt. Fuer kritische Fehlercodes sollte
Indicate verwendet werden, um Zustellung zu garantieren:

```c
/* DTC-List: Indicate fuer garantierte Zustellung */
{ .uuid = &chr_dtc_list.u,
  .access_cb = ble_gatt_access_cb,
  .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY |
           BLE_GATT_CHR_F_INDICATE,
  .val_handle = &ble_val_handles[CHR_H_DTC_LIST] },
```

### 5.4 Batch-Updates: Mehrere Werte pro Notification

Die `TLM_LIVE`-Characteristic (0x5001) fasst bereits alle
Echtzeit-Werte in einer 28-Byte-Notification zusammen:

```
TLM_LIVE (28 Bytes, 4 Hz):
  speed(1) rpm(2) gear(1) coolant(1) ambient(1) fuel(1)
  voltage(1) lat(4) lon(4) alt(2) heading(2) sats(1) hdop(1)
  + padding/reserved
```

Das ist bereits optimal: Statt 13 separate Notifications zu senden,
werden alle Werte in einer einzigen Notification gebuendelt.

**Zusaetzliche Batch-Optimierung fuer Logger-Export:**

Beim Log-Export ueber BLE (0x7004) sollten mehrere Log-Eintraege pro
Notification gepackt werden. Bei einer MTU von 247 Bytes und 50 Bytes
pro Log-Eintrag passen 4 Eintraege in eine Notification:

```c
/* ble_callbacks.c — Batch-Log-Export */
#define LOG_ENTRY_SIZE  50
#define LOG_BATCH_SIZE  4  /* 4 * 50 = 200 Bytes < 244 Bytes MTU */

void ble_send_log_batch(const log_entry_t *entries, uint8_t count)
{
    uint8_t buf[LOG_BATCH_SIZE * LOG_ENTRY_SIZE];
    uint8_t n = (count < LOG_BATCH_SIZE) ? count : LOG_BATCH_SIZE;

    for (int i = 0; i < n; i++) {
        serialize_log_entry(&buf[i * LOG_ENTRY_SIZE], &entries[i]);
    }

    ble_gatts_notify(ble_get_conn_handle(),
                     ble_val_handles[CHR_H_LOG_EXPORT],
                     buf, n * LOG_ENTRY_SIZE);
}
```

---

## 6. Power-Optimierung

### 6.1 Light-Sleep zwischen Sensor-Reads

Der `sensor_task` laeuft mit 50 Hz (20 ms Zyklus). Zwischen den
Sample-Zyklen schlaeft der Task mit `vTaskDelay(20)`. In dieser Zeit
koennte die CPU in den Light-Sleep gehen, um Strom zu sparen.

**Implementierung:**

```c
/* sdkconfig.defaults — Light-Sleep aktivieren */
CONFIG_PM_ENABLE=y
CONFIG_FREERTOS_USE_TICKLESS_IDLE=y
CONFIG_PM_SLP_DEFAULT_PARAMS_OPT=y
```

```c
/* main.c — Power-Management initialisieren */
#include "esp_pm.h"

void app_main(void)
{
    /* Power-Management konfigurieren */
    esp_pm_config_esp32s3_t pm_config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 80,       /* Reduziert bei Idle */
        .light_sleep_enable = true,
    };
    esp_pm_configure(&pm_config);

    /* ... restlicher Init ... */
}
```

Mit `light_sleep_enable = true` geht die CPU in den Light-Sleep, wenn
alle Tasks schlafen. Die CPU-Frequenz wird automatisch auf 80 MHz
reduziert. Beim naechsten Wakeup (Timer oder Interrupt) kehrt sie
sofort zu 240 MHz zurueck.

**Schaetzung der Einsparung:**

| Zustand | CPU-Frequenz | Strom (ESP32-S3) |
|---|---|---|
| Aktiv (240 MHz) | 240 MHz | ~80 mA |
| Light-Sleep (80 MHz) | 80 MHz | ~30 mA |
| Deep-Sleep | RTC only | ~10 µA |

Bei 50 Hz Sampling und 5 ms aktiver Zeit pro Zyklus sind 15 ms
Idle-Zeit. Mit Light-Sleep wuerde der Durchschnittsverbrauch des
ESP32-S3 von ~80 mA auf ~65 mA sinken.

### 6.2 WiFi-Modul deaktivieren wenn nicht OTA-aktiv

Der ESP32-S3 hat ein integriertes WiFi-Modul, das nicht verwendet wird
(BLE ist aktiv, WiFi nicht). Das WiFi-Modul sollte komplett
deaktiviert werden:

```c
/* sdkconfig.defaults — WiFi deaktivieren */
CONFIG_ESP_WIFI_ENABLED=n     # WiFi nicht initialisieren
```

Wenn WiFi fuer OTA-Updates benoetigt wird (Alternative zu BLE-OTA),
sollte es nur waehrend des Updates aktiviert werden:

```c
/* OTA via WiFi (geplant) */
void ota_start_wifi(void)
{
    /* WiFi nur fuer OTA aktivieren */
    esp_netif_init();
    esp_event_loop_create_default();
    /* ... WiFi Connect ... */
}

void ota_stop_wifi(void)
{
    /* WiFi nach OTA deaktivieren */
    esp_wifi_stop();
    esp_wifi_deinit();
}
```

WiFi im Idle verbraucht ~20-40 mA, auch wenn nicht verbunden. Durch
Deaktivierung wird dieser Verbrauch komplett eliminiert.

### 6.3 Display-Backlight PWM fuer Dimming

Die aktuelle Implementierung in `tft_driver.c` unterstuetzt bereits
PWM-basiertes Backlight-Dimming:

```c
/* tft_driver.c — Backlight PWM */
void tft_set_backlight(uint8_t percent)
{
    if (percent > 100) percent = 100;
    uint32_t duty = (uint32_t)percent * 1023 / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, TFT_BACKLIGHT_CH, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, TFT_BACKLIGHT_CH);
    s_backlight_pct = percent;
}
```

Das Backlight wird mit 5 kHz PWM und 10-bit Aufloesung betrieben.
Bei 20% Duty (DIMMED-Mode) sinkt der TFT-Strom von ~50-70 mA auf
~10-15 mA.

**Auto-Dimming:**

Der `power_task` kann das Backlight basierend auf Inaktivitaet
dimmen:

```c
/* power_manager.c — Auto-Dimming */
#define IDLE_DIM_TIMEOUT_MS  30000   /* 30 s Inaktivitaet → DIMMED */
#define IDLE_OFF_TIMEOUT_MS  60000   /* 60 s → Display aus */

void power_task(void *arg)
{
    while (1) {
        uint32_t now = get_millis();
        uint32_t idle_time = now - s_last_interaction_ms;

        if (idle_time > IDLE_OFF_TIMEOUT_MS) {
            /* Display aus */
            tft_set_backlight(0);
            power_set_state(POWER_STATE_IDLE);
        } else if (idle_time > IDLE_DIM_TIMEOUT_MS) {
            /* Display dimmen */
            tft_set_backlight(20);
            power_set_state(POWER_STATE_DIMMED);
        } else {
            /* Display voll */
            tft_set_backlight(config.display_brightness);
            power_set_state(POWER_STATE_ACTIVE);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### 6.4 Deep-Sleep bei Spannung aus

Die Benchmark-Spec fordert < 5 mA im DEEP_SLEEP-Zustand. Die aktuelle
Implementierung in `power.c` / `power_manager.c` unterstuetzt
`power_enter_deep_sleep()`:

```c
/* main.c — Shutdown → Deep Sleep */
static void on_shutdown(const app_event_t *ev, void *ctx)
{
    ESP_LOGW(TAG, "Shutdown event received, entering deep sleep");
    event_bus_set(EVT_SHUTDOWN);
    power_enter_deep_sleep(0);
}
```

**Wake-Quellen fuer Deep-Sleep:**

```c
/* power_manager.c — Deep-Sleep mit Wake-Source */
void power_enter_deep_sleep(uint32_t sleep_us)
{
    /* SD-Flush vor Sleep */
    logger_flush();

    /* GPIO-Wake: Zuendung AN (GPIO an Zündungsleitung) */
    esp_sleep_enable_ext0_wakeup(IGNITION_GPIO, 1);

    /* GPIO-Wake: BLE (nur bei Bonding moeglich) */
    /* (BLE kann im Deep-Sleep nicht empfangen —
     *  nur ext0/ext1/Timer/ULP koennen wecken) */

    /* Timer-Wake: Alle 10 Min fuer GPS-Log */
    if (sleep_us == 0) {
        sleep_us = 10 * 60 * 1000000ULL;  /* 10 Minuten */
    }
    esp_sleep_enable_timer_wakeup(sleep_us);

    ESP_LOGI(TAG, "Entering deep sleep for %llu s", sleep_us / 1000000ULL);
    esp_deep_sleep_start();
}
```

**Stromverbrauch im Deep-Sleep:**

| Komponente | Strom im Deep-Sleep |
|---|---|
| ESP32-S3 (RTC only) | ~10 µA |
| GPS-Modul (Backup-Mode) | ~5 mA |
| MPU6050 (Sleep-Mode) | ~10 µA |
| BMP280 (Forced-Mode) | ~0.3 µA |
| SD-Karte (Power-Off) | ~200 µA |
| MT3608 (Quiescent) | ~0.3 mA |
| Gesamt | ~5.5 mA |

Das Benchmark-Ziel von < 5 mA wird knapp verfehlt. Um es zu erreichen,
muss das GPS-Modul komplett abgeschaltet werden (ueber den
`GPS_POWER_EN_PIN`):

```c
/* GPS komplett ausschalten im Deep-Sleep */
gpio_set_level(GPS_POWER_EN_PIN, 0);  /* GPS-Modul stromlos */
```

Mit GPS aus sinkt der Deep-Sleep-Strom auf ~0.5 mA — deutlich unter
dem 5 mA Ziel.

### 6.5 Peripherie selektiv: IMU-ACC suspendieren bei Stillstand

Wenn das Fahrzeug steht (GPS-Speed = 0 km/h fuer > 60 s), koennen
die Sensoren in einen Stromsparmodus versetzt werden:

```c
/* sensors.c — Selektive Sensor-Suspendierung */
static bool s_sensors_suspended = false;

void sensor_task(void *arg)
{
    while (1) {
        /* ... normale Sensor-Lese ... */

        /* Stillstand-Erkennung */
        static uint32_t last_movement_ms = 0;
        if (data.speed_kmh > 1.0f || data.accel_g > 0.5f) {
            last_movement_ms = get_millis();
            if (s_sensors_suspended) {
                imu_wake();
                s_sensors_suspended = false;
            }
        } else {
            if (!s_sensors_suspended &&
                (get_millis() - last_movement_ms) > 60000) {
                /* IMU in Sleep-Mode versetzen (ACC suspendiert) */
                imu_suspend();
                s_sensors_suspended = true;
                ESP_LOGI(TAG, "IMU suspended (stillstand)");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/* imu.c — MPU6050 Sleep-Mode */
void imu_suspend(void)
{
    /* PWR_MGMT_1: SLEEP bit setzen */
    uint8_t data = 0x40;  /* Bit 6 = SLEEP */
    i2c_master_write_reg(MPU6050_I2C_ADDR, 0x6B, &data, 1);
}

void imu_wake(void)
{
    /* PWR_MGMT_1: SLEEP bit loeschen, CLKSEL=0 */
    uint8_t data = 0x00;
    i2c_master_write_reg(MPU6050_I2C_ADDR, 0x6B, &data, 1);
    vTaskDelay(pdMS_TO_TICKS(10));  /* Stabilisierungszeit */
}
```

Im Sleep-Mode sinkt der Stromverbrauch des MPU6050 von ~3.5 mA auf
~10 µA. Die Crash-Detection ist in diesem Modus nicht aktiv, aber
da das Fahrzeug steht, ist das akzeptabel.

---

## 7. Profiling-Strategie

### 7.1 esp_timer_get_time() fuer kritische Pfade

`esp_timer_get_time()` liefert Mikrosekunden-aufloesung seit Boot. Es
ist die primaere Profiling-Methode fuer kritische Code-Pfade.

**Instrumentierung der Event-Loop:**

```c
/* app_event_loop.c — Latenz-Messung pro Event */
static void app_event_loop_task(void *arg)
{
    app_event_t ev;
    while (1) {
        if (xQueueReceive(s_event_queue, &ev, portMAX_DELAY) != pdTRUE)
            continue;

        int64_t dispatch_time = (int64_t)ev.timestamp_ms * 1000;
        int64_t process_time = esp_timer_get_time();
        int64_t latency_us = process_time - dispatch_time;

        if (latency_us > 50000) {  /* > 50 ms Latenz */
            ESP_LOGW(TAG, "Event %d latency: %lld us",
                     (int)ev.type, latency_us);
        }

        /* ... Subscriber-Callback ausfuehren ... */

        int64_t done_time = esp_timer_get_time();
        int64_t callback_us = done_time - process_time;

        if (callback_us > 10000) {  /* > 10 ms fuer Callback */
            ESP_LOGD(TAG, "Event %d callback: %lld us",
                     (int)ev.type, callback_us);
        }
    }
}
```

**Instrumentierung des Display-Task:**

```c
/* display.c — Frame-Zeit-Messung */
void display_task(void *arg)
{
    while (1) {
        int64_t frame_start = esp_timer_get_time();

        /* ... Rendering ... */

        int64_t render_done = esp_timer_get_time();
        renderer_flush(true);
        int64_t flush_done = esp_timer_get_time();

        uint32_t render_ms = (render_done - frame_start) / 1000;
        uint32_t flush_ms  = (flush_done - render_done) / 1000;

        if (render_ms + flush_ms > 33) {  /* > 30 FPS Frame-Zeit */
            ESP_LOGD(TAG, "Frame slow: render=%u ms, flush=%u ms",
                     render_ms, flush_ms);
        }

        vTaskDelay(pdMS_TO_TICKS(33));
    }
}
```

### 7.2 Trace-Punkte in der Event-Loop

Zusaetzlich zu `esp_timer_get_time()` koennen strukturierte
Trace-Punkte in der Event-Loop gesetzt werden. Diese werden in einem
Ringbuffer gesammelt und bei Bedarf ausgegeben:

```c
/* Trace-System (geplante Implementierung) */
#define TRACE_BUF_SIZE  64

typedef struct {
    uint32_t event_type;
    int64_t  dispatch_us;
    int64_t  process_us;
    int64_t  callback_us;
} trace_entry_t;

static trace_entry_t s_trace_buf[TRACE_BUF_SIZE];
static volatile uint8_t s_trace_idx = 0;

void trace_event(uint32_t type, int64_t dispatch, int64_t process,
                 int64_t callback)
{
    uint8_t idx = s_trace_idx;
    s_trace_buf[idx].event_type  = type;
    s_trace_buf[idx].dispatch_us = dispatch;
    s_trace_buf[idx].process_us = process;
    s_trace_buf[idx].callback_us = callback;
    s_trace_idx = (idx + 1) % TRACE_BUF_SIZE;
}

void trace_dump(void)
{
    ESP_LOGI(TAG, "=== Trace Dump ===");
    for (int i = 0; i < TRACE_BUF_SIZE; i++) {
        trace_entry_t *t = &s_trace_buf[i];
        if (t->event_type == 0) continue;
        ESP_LOGI(TAG, "  evt=%u dispatch=%lldms cb=%lldms",
                 t->event_type,
                 (t->process_us - t->dispatch_us) / 1000,
                 t->callback_us / 1000);
    }
}
```

### 7.3 Heap-Monitoring

Die `diagnostics_task` ueberwacht bereits die Heap-Fragmentierung:

```c
/* main.c — diagnostics_task (bereits implementiert) */
uint32_t free_heap = esp_get_free_heap_size();
uint32_t min_heap  = esp_get_minimum_free_heap_size();
if (min_heap > 0 && free_heap > 0) {
    uint32_t fragmentation_pct =
        100 - (min_heap * 100 / free_heap);
    if (fragmentation_pct > 50) {
        ESP_LOGW(TAG, "Heap fragmentation high: %u%%",
                 (unsigned)fragmentation_pct);
    }
}
```

**Erweitertes Heap-Monitoring:**

```c
/* diagnostics_task — Erweiterte Heap-Statistiken */
static void check_heap_status(void)
{
    /* Interner Heap */
    uint32_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    uint32_t min_internal  = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
    uint32_t largest_internal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);

    /* PSRAM */
    uint32_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    uint32_t largest_psram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

    /* Fragmentierung */
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    uint32_t total_allocated = info.total_allocated_bytes;
    uint32_t total_free = info.total_free_bytes;
    uint32_t frag_pct = (total_free > 0)
        ? 100 - (info.largest_free_block * 100 / total_free)
        : 0;

    ESP_LOGI(TAG, "Heap: int=%uKB (min=%uKB, largest=%uKB, frag=%u%%), "
             "psram=%uKB (largest=%uKB)",
             free_internal / 1024,
             min_internal / 1024,
             largest_internal / 1024,
             frag_pct,
             free_psram / 1024,
             largest_psram / 1024);

    /* DTC bei kritischen Werten */
    if (free_internal < 40 * 1024) {
        dtc_report(DTC_B0101);  /* Heap niedrig */
    }
    if (frag_pct > 50) {
        ESP_LOGW(TAG, "Heap fragmentation critical: %u%%", frag_pct);
    }
}
```

### 7.4 Per-Task CPU-Last mit vTaskGetRunTimeStats()

Die Benchmark-Spec fordert < 80% CPU-Last pro Task. Die Messung
erfolgt mit `vTaskGetRunTimeStats()`:

```c
/* diagnostics_task — CPU-Last-Statistik */
#define STATS_TASK_COUNT  16

static void print_cpu_stats(void)
{
    TaskStatus_t *stats = pvPortMalloc(STATS_TASK_COUNT * sizeof(TaskStatus_t));
    if (!stats) return;

    UBaseType_t n = uxTaskGetSystemState(stats, STATS_TASK_COUNT, NULL);
    uint32_t total_runtime = 0;

    /* Gesamtlaufzeit berechnen */
    for (int i = 0; i < n; i++) {
        total_runtime += stats[i].ulRunTimeCounter;
    }

    ESP_LOGI(TAG, "=== CPU Runtime Stats ===");
    ESP_LOGI(TAG, "Task            Core  Prio  CPU%%   Stack-free");
    for (int i = 0; i < n; i++) {
        if (total_runtime == 0) continue;
        uint32_t pct = stats[i].ulRunTimeCounter * 100 / total_runtime;
        ESP_LOGI(TAG, "%-15s  %d    %d     %2u%%   %u",
                 stats[i].pcTaskName,
                 stats[i].xCoreID,
                 stats[i].uxCurrentPriority,
                 pct,
                 stats[i].usStackHighWaterMark);
    }

    vPortFree(stats);
}
```

**Konfiguration fuer Runtime-Statistiken:**

```ini
# sdkconfig.defaults
CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
CONFIG_FREERTOS_USE_TRACE_FACILITY=y
CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS=y
```

Der Timer fuer Runtime-Statistiken wird automatisch vom ESP-IDF
konfiguriert (typischerweise ein 16-bit Timer mit 10-40 kHz).

### 7.5 Logging-Level: VERBOSE fuer Debug, WARNING fuer Produktion

Die ESP-IDF-Logging-Infrastruktur unterstuetzt dynamische
Log-Level-Aenderung zur Laufzeit. Das ist wichtig, da `ESP_LOGV`
(VERBOSE) und `ESP_LOGD` (DEBUG) erheblichen Overhead erzeugen
(String-Formatierung + UART-Ausgabe).

**Konfiguration:**

```ini
# sdkconfig.defaults — Logging
CONFIG_LOG_DEFAULT_LEVEL_WARN=y       # Default: WARNING
CONFIG_LOG_MAXIMUM_LEVEL_VERBOSE=y    # Max: VERBOSE (fuer Debug-Builds)
CONFIG_LOG_COLORS=n                    # Keine ANSI-Farben (Overhead)
CONFIG_LOG_TIMESTAMP=y                 # Timestamp in Log-Zeile
```

**Runtime-Steuerung:**

```c
/* Logging-Level zur Laufzeit aendern */
void set_log_level_debug(void)
{
    esp_log_level_set("*", ESP_LOG_VERBOSE);  /* Alle Module VERBOSE */
}

void set_log_level_production(void)
{
    esp_log_level_set("*", ESP_LOG_WARNING);
    esp_log_level_set("main", ESP_LOG_INFO);
    esp_log_level_set("event_loop", ESP_LOG_INFO);
    esp_log_level_set("sensors", ESP_LOG_INFO);
}

/* Per BLE-Kommando aenderbar (via Config-Service) */
void ble_handle_log_level(uint8_t level)
{
    esp_log_level_t lvl;
    switch (level) {
    case 0:  lvl = ESP_LOG_NONE;    break;
    case 1:  lvl = ESP_LOG_ERROR;   break;
    case 2:  lvl = ESP_LOG_WARN;    break;
    case 3:  lvl = ESP_LOG_INFO;    break;
    case 4:  lvl = ESP_LOG_DEBUG;   break;
    case 5:  lvl = ESP_LOG_VERBOSE; break;
    default: return;
    }
    esp_log_level_set("*", lvl);
}
```

**Performance-Auswirkung:**

| Log-Level | UART-Ausgaben/s | CPU-Overhead |
|---|---|---|
| VERBOSE | ~200-500 | 5-10% |
| DEBUG | ~50-100 | 1-3% |
| INFO | ~10-20 | < 1% |
| WARNING | ~1-5 | vernachlaessigbar |
| ERROR | ~0-1 | vernachlaessigbar |

In der Produktion sollte WARNING als Default verwendet werden. DEBUG
nur bei aktiver Fehlersuche, VERBOSE nur bei tiefgreifendem Profiling.

---

## 8. Optimierungs-Roadmap

### 8.1 Phase 1: Sofortmassnahmen (Must-Benchmarks)

Diese Massnahmen sind fuer das Erreichen der Must-Benchmark-Ziele
kritisch und sollten mit hoechster Prioritaet umgesetzt werden:

1. **Dirty-Rect-Invalidate präzisieren:** `main.c` invalidiert derzeit
   das gesamte Display bei jedem GPS/Sensor-Update. Aenderung auf
   bereichsspezifische Invalidation (nur betroffene Widget-Bereiche).

2. **Display-Task auf 30 FPS umstellen:** `display.c` verwendet 1 Hz
   (E-Paper-Modus). Fuer TFT-Boards muss die Periodendauer auf 33 ms
   geaendert werden.

3. **Selektives Clear statt Full-Clear:** `renderer_clear()` loescht
   den gesamten Back-Buffer. Stattdessen nur dirty-Bereiche loeschen.

4. **Heap-Monitoring aktivieren:** Die `diagnostics_task` muss
   zusaetzlich PSRAM und largest-free-block ueberwachen.

5. **Stack-High-Water-Mark-Check:** Jeder Task muss periodisch
   `uxTaskGetStackHighWaterMark()` pruefen und bei < 25% einen DTC
   setzen.

### 8.2 Phase 2: Should-Benchmarks

Diese Massnahmen sichern die Should-Benchmark-Ziele:

1. **UART-Event-Queue fuer GPS:** Polling durch Event-getriebenen
   Empfang ersetzen.

2. **NMEA-Filterung:** Nur RMC und GGA parsen, andere Saetze verwerfen.

3. **BLE Connection-Parameter fuer OTA-Modus:** Schnelle Parameter
   (7.5 ms Intervall) waehrend OTA-Updates.

4. **E-Paper Ghosting-Counter:** Alle 50 Partial-Refreshes ein
   Full-Refresh erzwingen.

5. **Animation Frame-Rate-Adaption:** CPU-Last-basierte Reduktion
   der Animations-FPS.

### 8.3 Phase 3: Power-Optimierung

Diese Massnahmen reduzieren den Stromverbrauch und sichern die
Power-Benchmark-Ziele:

1. **Light-Sleep aktivieren:** `esp_pm_configure` mit
   `light_sleep_enable = true`.

2. **GPS-Modul im Deep-Sleep ausschalten:** `GPS_POWER_EN_PIN` auf
   LOW setzen.

3. **IMU-Suspendierung bei Stillstand:** MPU6050 Sleep-Mode nach 60 s
   Inaktivitaet.

4. **Auto-Dimming:** Backlight-Reduktion nach 30 s Inaktivitaet.

5. **WiFi deaktivieren:** `CONFIG_ESP_WIFI_ENABLED=n` in
   sdkconfig.defaults.

### 8.4 Phase 4: Profiling-Infrastruktur

Diese Massnahmen bauen die Profiling-Infrastruktur fuer
kontinuierliches Monitoring auf:

1. **Trace-Ringbuffer:** Strukturierte Latenz-Messung in Event-Loop.

2. **CPU-Stats per BLE:** `vTaskGetRunTimeStats()`-Ergebnisse ueber
   Diagnose-Service uebertragen.

3. **Heap-Stats per BLE:** `heap_caps_get_info()`-Ergebnisse ueber
   Diagnose-Service.

4. **Log-Level-Steuerung per BLE:** Dynamische Aenderung des
   Log-Levels ueber Config-Service.

5. **Benchmark-Kommandos:** `benchmark tft 100`, `benchmark cpu 10`,
   `benchmark heap` etc. ueber serielle Konsole (siehe
   docs/HARDWARE-TEST-BENCH.md).

---

## Anhang: Konfigurations-Referenz

### sdkconfig.defaults — Performance-relevante Optionen

```ini
# CPU / Clock
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y

# FreeRTOS
CONFIG_FREERTOS_HZ=1000
CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY=y
CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK=y
CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
CONFIG_FREERTOS_USE_TRACE_FACILITY=y
CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS=y
CONFIG_FREERTOS_USE_IDLE_HOOK=y
CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM=n

# PSRAM
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=1
CONFIG_SPIRAM_SPEED_80M=1
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_MEMTRY_ALLOC_MAX=8192
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=4096

# Power Management
CONFIG_PM_ENABLE=y
CONFIG_FREERTOS_USE_TICKLESS_IDLE=y
CONFIG_PM_SLP_DEFAULT_PARAMS_OPT=y

# WiFi (deaktiviert)
CONFIG_ESP_WIFI_ENABLED=n

# Logging
CONFIG_LOG_DEFAULT_LEVEL_WARN=y
CONFIG_LOG_MAXIMUM_LEVEL_VERBOSE=y
CONFIG_LOG_COLORS=n
CONFIG_LOG_TIMESTAMP=y

# Heap
CONFIG_HEAP_POISONING_DISABLED=y
CONFIG_HEAP_TASK_TRACKING=n
CONFIG_HEAP_TRACING=n
```

### Benchmark-Zielwerte — Quick Reference

| # | Metrik | Zielwert | Optimierung |
|---|---|---|---|
| 1 | TFT-FPS (Vollbild) | > 30 FPS | Dirty-Rect, Bounce-Buffer |
| 2 | TFT-FPS (Dirty-Rect) | > 60 FPS | Praezise Invalidation |
| 3 | E-Paper Full-Refresh | <= 2,0 s | Nur bei Screen-Wechsel |
| 4 | E-Paper Partial-Refresh | <= 300 ms | Dirty-Rect mit <= 4 Rechtecken |
| 5 | RPM-Latenz | < 200 ms | PCNT, 20 ms Task, 30 FPS Display |
| 6 | Temperatur-Latenz | < 500 ms | 2 Hz ADC-Sampling, EMA-Filter |
| 7 | BLE-Durchsatz | > 10 kbps | MTU 247+, Notify, Batch |
| 8 | BLE-Latenz | < 100 ms | 15 ms Interval, Latency 0 |
| 9 | GPS-Update-Rate | >= 1 Hz | NMEA-Filter, 115200 Baud |
| 10 | Power ACTIVE | < 150 mA | Light-Sleep, Auto-Dim |
| 11 | Power DEEP_SLEEP | < 5 mA | GPS aus, IMU-Sleep |
| 12 | Boot-Zeit | < 3 s | Init-Reihenfolge optimieren |
| 13 | CPU-Last pro Task | < 80% | Prioritaeten, Core-Balance |
| 14 | Freier Heap | >= 50 KB | Memory-Pools, PSRAM-Auslagerung |
| 15 | Heap-Fragmentierung | < 20% | Statische Pools, keine malloc-Schleifen |

---

*Dieses Dokument beschreibt die Strategien zur Erreichung der in
docs/BENCHMARK-SPEC.md definierten Zielwerte. Bei Aenderungen an der
Architektur oder Task-Konfiguration muss dieses Dokument aktualisiert
werden. Code-Snippets sind als Orientierungshilfe zu verstehen und
muessen an die jeweilige Modul-Implementierung angepasst werden.*

# Tachokalibrierung: VSS-basierte Geschwindigkeitsmessung und Auto-Kalibrierung

> Stand: 16.10.2026
> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986-2000, rein mechanisch, keine ECU
> Plattform: ESP32-S3-WROOM-1 (N8R2: 8MB Flash, 2MB PSRAM), ESP-IDF v5.4, FreeRTOS SMP
> Vorherige Dokumente: 01-14

## 0. Einleitung

Die Honda Transalp 600 (PD06) hat einen mechanischen Tacho, der ueber einen
Tachowellenantrieb am Vorderrad angetrieben wird. Der Bordcomputer erfasst
die Geschwindigkeit derzeit primär ueber GPS (NEO-M9N, 10 Hz). GPS ist
praezise im Freien, hat aber Luecken in Tuneln, unter Bruecken und in
Schluchten.

Eine VSS-basierte (Vehicle Speed Sensor) Geschwindigkeitsmessung ergaenzt
das GPS: Ein Hall-Sensor oder Reed-Kontakt am Vorderrad zaehlt Umdrehungen.
Aus Radumfang und Zaehlfrequenz berechnet der Bordcomputer die Geschwindigkeit.
Diese ist unabhaengig von GPS-Signal und liefert kontinuierliche Werte.

Die Herausforderung: Der Radumfang muss genau bekannt sein. Reifengroesse,
Reifendruck und Reifenverschleiss beeinflussen den effektiven Umfang.
Eine Tachokalibrierung gleicht diese Faktoren aus.

Dieses Dokument spezifiziert:

1. Radumfang- und Uebersetzungsfaktor konfigurierbar machen
2. GPS-basierte Auto-Kalibrierung (Vergleich GPS-Speed vs VSS-Speed)
3. Neue Config-Felder in device_config_t
4. Kalibrierungs-Wizard in der Companion-App

---

## 1. Grundlagen

### 1.1 Bestehende Geschwindigkeitsmessung

Der Bordcomputer erfasst die Geschwindigkeit derzeit ausschliesslich ueber GPS:

```
GPS-Task (Core 1, 10Hz)
  -> NMEA-Parser: $GPRMC oder $GPVTG
  -> gps_data_t.speed_kmh
  -> gps_data_queue
```

Es gibt keinen VSS-Sensor (Vehicle Speed Sensor) in der aktuellen
Hardware. Die Pin-Belegung in board_config.h definiert keinen VSS-Pin.
Der OBD-UART (UART_NUM_2, GPIO17/18) ist fuer OBD-II reserviert, nicht
fuer VSS.

### 1.2 Bestehende Gang-Erkennung und Uebersetzungen

Die Gang-Erkennung (gear_indicator.c) verwendet bereits feste
Uebersetzungswerte:

```c
#define PRIMARY_RATIO        2.733f    /* Kurbelwelle -> Kupplung */
#define FINAL_RATIO          2.933f    /* Sekundaer: 15/44 Zaehne */
#define WHEEL_CIRCUMFERENCE  1.96f     /* 130/80-17 Reifen, ~1.96m */

static const float s_gear_ratios[6] = {
    2.571f, 1.750f, 1.316f, 1.045f, 0.893f, 0.800f
};
```

Diese Werte sind als Compile-Zeit-Konstanten (#define) hart codiert.
Eine Aenderung erfordert Re-Kompilierung der Firmware. Die Tacho-
kalibrierung macht diese Werte zur Laufzeit konfigurierbar.

### 1.3 VSS-Optionen fuer die PD06

Die PD06 hat eine mechanische Tachowelle am Vorderrad. Optionen fuer
einen elektronischen VSS:

**Option A: Hall-Sensor am Vorderrad**
- Magnet auf einer Bremsscheibenschraube
- Hall-Sensor (z.B. A3144 oder TLE4905L) am Gabelbock
- 1 Puls pro Radumdrehung
- Sehr genau, wetterfest verpackbar
- Hardware-Aufwand: ~5 EUR (Magnet + Hall-Sensor + Kabel)

**Option B: Reed-Kontakt am Tachowellenantrieb**
- Reed-Kontakt in der Naehe des Tachowellen-Antriebs am Vorderrad
- Magnet auf der Antriebswelle
- Mehrere Pulse pro Radumdrehung moeglich (Anzahl abhaengig von
  Positionierung)
- Einfacher einzubauen, aber weniger robust gegen Vibration

**Option C: Abgriff der Tachowelle (Induktiv)**
- Induktiver Sensor am Tachowellenkabel
- Keine mechanische Modifikation am Rad
- Komplexer abzugleichen, Signal nicht standardisiert

**Empfehlung:** Option A (Hall-Sensor am Vorderrad). Genauigkeit,
Robustheit und einfache Berechnung (1 Puls = 1 Radumdrehung) ueberwiegen
den minimal hoeheren Einbau-Aufwand.

### 1.4 Radumfangsberechnung

Der Reifenumfang haengt von Reifengroesse und Reifendruck ab.

Standard-Reifengroesse fuer die PD06: 130/80-17

Der Umfang wird berechnet aus:
- Reifendurchmesser = Felgendurchmesser + 2 * Reifenhoehe
- Reifenhoehe = Breite * Querschnittsverhaeltnis
- Felgendurchmesser = 17 Zoll = 431.8 mm
- Breite = 130 mm, Querschnitt = 80%
- Reifenhoehe = 130 * 0.80 = 104 mm
- Gesamtdurchmesser = 431.8 + 2 * 104 = 639.8 mm
- Umfang = pi * 639.8 = 2009 mm (~2.01 m)

Der aktuell codierte Wert ist 1.96 m. Der Unterschied von 5 cm kann auf
Reifenverschleiss oder eine abweichende Reifengroesse zurueckzufuehren
sein. Bei 100 km/h (= 27.8 m/s) entspricht ein 5 cm Fehler in einer
Umdrehung bei 2.01 m Umfang: 2.5% Abweichung = 2.5 km/h.

---

## 2. Architektur

### 2.1 VSS-Sensor-Integration

```
Vorderrad
  |
  +-- Magnet auf Bremsscheibenschraube
  |
  v
Hall-Sensor (GPIO XX, PCNT-Channel)
  |
  | 1 Puls pro Radumdrehung
  v
+-----------------------------------+
| Bordcomputer: VSS-Task            |
| (Core 1, Prio 4, neben RPM)       |
|                                   |
| PCNT: Zaehlt Pulse im Messfenster |
| (100ms = 10 Hz Update-Rate)       |
|                                   |
| speed_kmh = pulse_count *          |
|   wheel_circumference_mm * 36 /   |
|   (1000 * sample_period_s)        |
|                                   |
| Smoothing: Moving Average (5)    |
+-----------------------------------+
  |
  v
sensor_data_t.speed_kmh (VSS-basiert)
  |
  +---> Display: Tacho-Anzeige
  +---> Logger: CSV/GPX
  +---> Gang-Erkennung: gear_indicator_calc()
```

### 2.2 Geschwindigkeits-Quellen und Fallback

Der Bordcomputer hat zwei Geschwindigkeitsquellen:

```
Quelle         Genauigkeit    Update-Rate   Verfuegbarkeit
------------------------------------------------------------
GPS (NEO-M9N)  ±1-2 km/h     10 Hz         Freie Sicht noetig
VSS (Hall)     ±0.5 km/h     10 Hz         Immer (bei Bewegung)
```

Strategie (Sensor-Fusion):
1. Wenn GPS-Fix vorhanden (has_fix + valid) -> GPS-Speed als Primaerwert
2. Wenn GPS keinen Fix hat -> VSS-Speed als Fallback
3. Wenn beide verfuegbar -> VSS als Display-Wert (praeziser bei
   konstanter Geschwindigkeit), GPS fuer Kalibrierung
4. Wenn keiner verfuegbar -> speed_kmh = 0 (Stand)

### 2.3 Auto-Kalibrierung

Die Auto-Kalibrierung vergleicht GPS-Speed mit VSS-Speed und berechnet
einen Korrekturfaktor. Voraussetzung:

- GPS-Fix mit hoher Qualitaet (HDOP < 2.0, Fix >= 3D)
- Konstante Geschwindigkeit (>30 km/h, <120 km/h, aenderung <2 km/h/s)
- Dauer: mindestens 30 Sekunden konstante Fahrt
- Gerade Strecke (Kursaenderung <5 Grad in 10s)

Ablauf:
1. Kalibrierungs-Task sammelt parallele GPS/VSS-Werte fuer 30s
2. Berechnet Durchschnitt von GPS-Speed und VSS-Speed
3. Korrekturfaktor = avg_gps_speed / avg_vss_speed
4. Neuer Radumfang = konfigurierter Umfang * Korrekturfaktor
5. Speichern in config (NVS)
6. Benachrichtigung an Nutzer ueber Companion-App

```
GPS-Speed: 87 km/h (avg ueber 30s)
VSS-Speed: 84 km/h (avg ueber 30s)
Korrekturfaktor = 87 / 84 = 1.0357
Neuer Umfang = 1960mm * 1.0357 = 2030mm
Abweichung: +3.6% (Tacho zeigte zu wenig)
```

### 2.4 Pin-Belegung fuer VSS

Neuer Pin fuer VSS-Hall-Sensor. Der PCNT (Pulse Counter) des ESP32-S3
hat 4 Einheiten mit je 2 Channels. Unit 0 ist fuer RPM belegt
(GPIO16). Unit 0 Channel 1 ist frei.

```c
/* board_config.h: Neue Definitionen */
#define VSS_PCNT_UNIT        PCNT_UNIT_0
#define VSS_PCNT_CHANNEL     PCNT_CHANNEL_1
#define VSS_PCNT_PIN         45       /* Hall-Sensor Input */
#define VSS_PULSES_PER_REV   1        /* 1 Magnet, 1 Puls/Umdrehung */
```

GPIO 45 ist frei in der aktuellen board_config.h. Alternativ koennen
GPIO 4, 5, oder 7 verwendet werden (alle frei, nicht-Strapping).

---

## 3. Datenfluss

### 3.1 VSS-Messzyklus

```
VSS-Task (Core 1, Prio 4)
  Schleife alle 100ms:
    1. PCNT-Wert auslesen (pulse_count seit letztem Reset)
    2. PCNT zuruecksetzen
    3. speed_kmh berechnen:
       speed_ms = pulse_count * wheel_circumference_m / sample_period_s
       speed_kmh = speed_ms * 3.6
    4. Moving Average (Window 5)
    5. In sensor_data_t.speed_kmh schreiben (mit Mutex)
    6. Wenn Kalibrierung aktiv: Wert + GPS-Wert in Kalibrierungs-Puffer
```

### 3.2 Kalibrierungs-Datenfluss

```
Companion-App            Bordcomputer
     |                        |
     |  "Auto-Kalibrierung    |
     |   starten"             |
     |  -> Write CHR_CONFIG_  |
     |     WRITE (0x6002)     |
     |     {"cmd":"calib_start"}
     |                        |
     |                        |  Kalibrierungs-Task aktiv:
     |                        |  - Sammelt 30s GPS+VSS parallel
     |                        |  - Prueft Bedingungen (HDOP, const spd)
     |                        |
     |  <- Notify CHR_CONFIG_ |
     |     READ (0x6001)      |
     |     {"calib_progress": 50}
     |                        |
     |  <- Notify CHR_CONFIG_ |
     |     READ (0x6001)      |
     |     {"calib_progress": 100,
     |      "calib_result": {
     |        "gps_avg": 87.3,
     |        "vss_avg": 84.1,
     |        "correction": 1.038,
     |        "new_circumference_mm": 2035,
     |        "old_circumference_mm": 1960
     |      }}
     |                        |
     |  "Akzeptieren"         |
     |  -> Write CHR_CONFIG_  |
     |     WRITE (0x6002)     |
     |     {"cmd":"calib_accept"}
     |                        |
     |                        |  NVS: wheel_circumference_mm = 2035
     |                        |  config_save_all()
     |                        |
     |  <- Notify CHR_CONFIG_ |
     |     READ: {"calib_done": true}
     |                        |
```

### 3.3 Manuelles Setup (ohne Auto-Kalibrierung)

Der Nutzer kann die Werte auch manuell eingeben:

```
Companion-App -> Settings -> Kalibrierung

  Reifengroesse: [130/80-17] (Dropdown)
  -> Berechnet: Umfang = 2009 mm (automatisch)
  -> Eingabe: [2009] mm (editierbar)

  Rad-Uebersetzung: [2.933] (15/44, vorausgefuellt)
  Primaer-Uebersetzung: [2.733] (vorausgefuellt)

  VSS-Pulse pro Umdrehung: [1]

  [Speichern] -> Write CHR_CONFIG_WRITE
```

---

## 4. API-Design

### 4.1 Neue Config-Felder

Erweiterung von `device_config_t` in `storage.h` und `config.c`:

```c
/* storage.h: Neue Felder in device_config_t */
typedef struct {
    /* ... bestehende Felder ... */

    /* Tacho-Kalibrierung (neu) */
    uint16_t wheel_circumference_mm;    /* Default: 2009 (130/80-17) */
    float    final_drive_ratio;         /* Default: 2.933f (15/44) */
    float    primary_drive_ratio;       /* Default: 2.733f */
    uint8_t  vss_pulses_per_rev;        /* Default: 1 */
    uint8_t  speed_source;              /* 0=GPS, 1=VSS, 2=Auto (Fusion) */
    float    vss_calibration_factor;    /* Auto-Kalibrierung: 1.0 = default */
    uint16_t vss_pin;                   /* GPIO fuer VSS-Sensor */
} device_config_t;

/* config_key_t: Neue Schluessel */
typedef enum {
    /* ... bestehende Schluessel ... */
    CFG_KEY_WHEEL_CIRCUMFERENCE,
    CFG_KEY_FINAL_DRIVE_RATIO,
    CFG_KEY_PRIMARY_DRIVE_RATIO,
    CFG_KEY_VSS_PULSES_PER_REV,
    CFG_KEY_SPEED_SOURCE,
    CFG_KEY_VSS_CALIB_FACTOR,
    CFG_KEY_VSS_PIN,
} config_key_t;
```

Config-Version muss erhoeht werden: `CONFIG_VERSION` von 3 auf 4.
Migration in `config_migrate()`:

```c
if (old_version < 4) {
    /* v3 -> v4: Tacho-Kalibrierung hinzugefuegt */
    s_config.wheel_circumference_mm = 2009;  /* 130/80-17 */
    s_config.final_drive_ratio = 2.933f;      /* 15/44 */
    s_config.primary_drive_ratio = 2.733f;
    s_config.vss_pulses_per_rev = 1;
    s_config.speed_source = 2;                /* Auto-Fusion */
    s_config.vss_calibration_factor = 1.0f;
    s_config.vss_pin = 45;                    /* GPIO 45 */
}
```

### 4.2 Firmware: VSS-Modul

Neue Datei: `firmware/components/sensors/vss.c` + Erweiterung `sensors.h`

```c
/* vss.c -- Vehicle Speed Sensor via PCNT */

#include "sensors.h"
#include "board_config.h"
#include "esp_log.h"
#include "driver/pcnt.h"

static const char *TAG = "vss";

#define VSS_SAMPLE_PERIOD_MS  100
#define VSS_SMOOTH_WINDOW    5

static float s_vss_buf[VSS_SMOOTH_WINDOW];
static uint8_t s_vss_buf_idx = 0;
static uint8_t s_vss_buf_count = 0;

/* Kalibrierungs-Zustand */
static bool s_calib_active = false;
static float s_calib_gps_sum = 0;
static float s_calib_vss_sum = 0;
static uint16_t s_calib_samples = 0;

void vss_init(void)
{
    /* PCNT Channel 1 auf Unit 0 konfigurieren */
    pcnt_config_t cfg = {
        .pulse_gpio_num = s_config.vss_pin,
        .ctrl_gpio_num  = PCNT_PIN_NOT_USED,
        .channel        = PCNT_CHANNEL_1,
        .unit           = PCNT_UNIT_0,
        .pos_mode       = PCNT_COUNT_INC,
        .neg_mode       = PCNT_COUNT_DIS,
        .lctrl_mode     = PCNT_MODE_KEEP,
        .hctrl_mode     = PCNT_MODE_KEEP,
        .counter_h_lim  = 10000,
        .counter_l_lim  = -10000,
    };
    pcnt_unit_config(&cfg);
    pcnt_set_filter_value(PCNT_UNIT_0, 100);
    pcnt_filter_enable(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);

    gpio_set_direction(s_config.vss_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(s_config.vss_pin, GPIO_PULLUP_ONLY);

    memset(s_vss_buf, 0, sizeof(s_vss_buf));
    ESP_LOGI(TAG, "VSS initialized (pin=%d, pulses/rev=%d, circumference=%umm)",
             s_config.vss_pin, s_config.vss_pulses_per_rev,
             s_config.wheel_circumference_mm);
}

float vss_read_speed_kmh(void)
{
    int16_t count;
    pcnt_counter_clear(PCNT_UNIT_0);

    vTaskDelay(pdMS_TO_TICKS(VSS_SAMPLE_PERIOD_MS));

    pcnt_get_counter_value(PCNT_UNIT_0, &count);

    /* speed = count * circumference / time */
    float circumference_m = s_config.wheel_circumference_mm / 1000.0f;
    float period_s = VSS_SAMPLE_PERIOD_MS / 1000.0f;
    float pulses_per_rev = (float)s_config.vss_pulses_per_rev;
    if (pulses_per_rev < 1) pulses_per_rev = 1;

    float speed_ms = (count * circumference_m) /
                     (period_s * pulses_per_rev);
    float speed_kmh = speed_ms * 3.6f;

    /* Kalibrierungsfaktor anwenden */
    speed_kmh *= s_config.vss_calibration_factor;

    /* Moving Average */
    s_vss_buf[s_vss_buf_idx] = speed_kmh;
    s_vss_buf_idx = (s_vss_buf_idx + 1) % VSS_SMOOTH_WINDOW;
    if (s_vss_buf_count < VSS_SMOOTH_WINDOW) s_vss_buf_count++;

    float sum = 0;
    for (uint8_t i = 0; i < s_vss_buf_count; i++)
        sum += s_vss_buf[i];
    return sum / s_vss_buf_count;
}

/* Auto-Kalibrierung */
void vss_calib_start(void)
{
    s_calib_active = true;
    s_calib_gps_sum = 0;
    s_calib_vss_sum = 0;
    s_calib_samples = 0;
    ESP_LOGI(TAG, "Calibration started");
}

calib_result_t vss_calib_update(float gps_speed, float vss_speed)
{
    if (!s_calib_active) return CALIB_INACTIVE;

    s_calib_gps_sum += gps_speed;
    s_calib_vss_sum += vss_speed;
    s_calib_samples++;

    if (s_calib_samples >= 300) {  /* 30s bei 10Hz */
        s_calib_active = false;
        float avg_gps = s_calib_gps_sum / s_calib_samples;
        float avg_vss = s_calib_vss_sum / s_calib_samples;

        if (avg_vss < 5.0f) {
            return CALIB_FAILED_LOW_SPEED;
        }

        float factor = avg_gps / avg_vss;
        s_config.vss_calibration_factor = factor;

        calib_result_t result = {
            .status = CALIB_SUCCESS,
            .gps_avg = avg_gps,
            .vss_avg = avg_vss,
            .correction = factor,
            .new_circumference_mm =
                (uint16_t)(s_config.wheel_circumference_mm * factor),
            .old_circumference_mm = s_config.wheel_circumference_mm,
        };
        return result;
    }

    calib_result_t result = {
        .status = CALIB_RUNNING,
        .progress_pct = (uint8_t)(s_calib_samples * 100 / 300),
    };
    return result;
}

void vss_calib_accept(void)
{
    /* Radumfang aktualisieren */
    s_config.wheel_circumference_mm =
        (uint16_t)(s_config.wheel_circumference_mm *
                   s_config.vss_calibration_factor);
    s_config.vss_calibration_factor = 1.0f;  /* Faktor zuruecksetzen */
    config_save_all();
    ESP_LOGI(TAG, "Calibration accepted: circumference=%umm",
             s_config.wheel_circumference_mm);
}
```

### 4.3 Companion-App: Kalibrierungs-Wizard

Neue Datei: `companion-app/src/screens/CalibrationScreen.tsx`

Wizard mit 4 Schritten:

```
Schritt 1: Reifengroesse
+-----------------------------------+
|  Tacho-Kalibrierung               |
|                                   |
|  Reifengroesse auswaehlen:        |
|  [130/80-17  (Standard PD06)]    |
|  [120/90-17]                      |
|  [110/80-18]                      |
|  [Custom: Umfang ___ mm]         |
|                                   |
|  Berechneter Umfang: 2009 mm      |
|                                   |
|              [Weiter]             |
+-----------------------------------+

Schritt 2: Uebersetzungen
+-----------------------------------+
|  Uebersetzungsdaten               |
|                                   |
|  Primaer-Uebersetzung: 2.733      |
|  Sekundaer-Uebersetzung: 2.933    |
|  (15/44 Zaehne)                   |
|                                   |
|  Diese Werte stimmen fuer die     |
|  PD06 ab Werk. Nur aendern, wenn   |
|  ein anderer Zahnkraensatz verbaut |
|  ist.                              |
|                                   |
|              [Weiter]             |
+-----------------------------------+

Schritt 3: Auto-Kalibrierung
+-----------------------------------+
|  GPS-basierte Kalibrierung        |
|                                   |
|  Bedingungen:                     |
|  - GPS-Fix mit HDOP < 2.0         |
|  - Geschwindigkeit 30-120 km/h    |
|  - Konstante Fahrt fuer 30s       |
|  - Gerade Strecke                 |
|                                   |
|  GPS-Status: Fix (HDOP 1.2)      |
|  Speed: 0 km/h                    |
|                                   |
|  [Kalibrierung starten]           |
|                                   |
|  Fortschritt: [====50%====]      |
|  GPS-Avg: 87.3  VSS-Avg: 84.1    |
|  Korrektur: +3.8%                 |
|                                   |
|  [Akzeptieren]  [Abbrechen]       |
+-----------------------------------+

Schritt 4: Bestaetigung
+-----------------------------------+
|  Kalibrierung abgeschlossen       |
|                                   |
|  Alter Radumfang: 1960 mm         |
|  Neuer Radumfang: 2035 mm         |
|  Korrekturfaktor: 1.038           |
|                                   |
|  Die Werte wurden gespeichert.    |
|  Der Tacho zeigt jetzt korrekt.   |
|                                   |
|  [Fertig]                         |
+-----------------------------------+
```

### 4.4 JSON-Config-Erweiterung

`config_export_json()` in config.c wird erweitert:

```json
{
  "calibration": {
    "gyro_offset": [0, 0, 0],
    "accel_offset": [0, 0, 0],
    "bmp_sea_level_pa": 101325,
    "adc_vref_mv": 3300,
    "wheel_circumference_mm": 2009,
    "final_drive_ratio": 2.933,
    "primary_drive_ratio": 2.733,
    "vss_pulses_per_rev": 1,
    "speed_source": 2,
    "vss_calibration_factor": 1.0,
    "vss_pin": 45
  }
}
```

### 4.5 Reifengroesse-Tabelle

Vordefinierte Reifengroessen fuer den Wizard:

```typescript
const TIRE_SIZES = [
  { label: '130/80-17 (Standard PD06)', circumference_mm: 2009 },
  { label: '120/90-17',                  circumference_mm: 1973 },
  { label: '110/80-18',                  circumference_mm: 2020 },
  { label: '130/70-18',                 circumference_mm: 2001 },
  { label: '90/90-21 (Front)',          circumference_mm: 2015 },
  { label: 'Custom',                    circumference_mm: 0 },
];
```

Die Umfaenge werden berechnet nach:
`Umfang = (Felgendurchmesser + 2 * Breite * Querschnitt) * pi`

---

## 5. Risiken

### 5.1 Technische Risiken

| Risiko | Wahrscheinlichkeit | Auswirkung | Mitigation |
|---|---|---|---|
| VSS-Sensor entfernt bei Reifenwechsel | mittel | hoch | Plausibilitaetscheck: speed=0 bei GPS>5km/h -> Warnung |
| Hall-Sensor versagt bei Regen | niedrig | mittel | IP65-Gehaeuse, Wasserabweisende Montage |
| Magnet loest sich durch Vibration | mittel | hoch | Loctite auf Gewinde, Ueberpruefung bei Service |
| GPS-Kalibrierung bei schlechtem Signal | mittel | mittel | HDOP-Check, Mindestanzahl Satelliten (>=7) |
| PCNT-Overflow bei hohen Geschwindigkeiten | niedrig | niedrig | counter_h_lim=10000, Reset alle 100ms |
| Reifendruck-Aenderung beeinflusst Umfang | hoch | niedrig | Delta <1%, Akzeptabel fuer Tacho |

### 5.2 Kalibrierungs-Risiken

| Risiko | Auswirkung | Mitigation |
|---|---|---|
| Kalibrierung bei beschleunigter Fahrt | falscher Faktor | Bedingung: Speed-Delta <2 km/h/s |
| Kalibrierung in Kurve (GPS ungenau) | falscher Faktor | Bedingung: Kursaenderung <5 Grad/10s |
| Nutzer akzeptiert falschen Faktor | Tacho falsch | Bestaetigungs-Dialog mit Vergleichswerten |
| VSS-Pulse durch Reibschnickspross fehlen | Unterzahlung | Plausibilitaetscheck VSS vs GPS, Warnung |

### 5.3 Architektur-Risiken

| Risiko | Auswirkung | Mitigation |
|---|---|---|
| Config-Version-Aenderung bricht alte Geraete | Migration | config_migrate() v3->v4 fuellt Defaults |
| Gear-Indicator nutzt noch Compile-Konstanten | Inkonsistenz | Refactoring: gear_indicator.c auf config-Werte umstellen |
| PCNT-Unit shared mit RPM | Ressourcenkonflikt | Unit 0 hat 2 Channels: Ch0=RPM, Ch1=VSS. Kein Konflikt. |

---

## 6. Aufwandsschaetzung

### 6.1 Firmware

| Aufgabe | Aufwand |
|---|---|
| VSS-Modul: vss.c (PCNT, Speed-Berechnung) | 4h |
| vss.h Header + sensors.h Integration | 1h |
| board_config.h: VSS-Pin und Konstanten | 1h |
| device_config_t: Neue Felder + Migration v4 | 2h |
| config.c: set_value/get_value fuer neue Keys | 2h |
| config.c: JSON-Export/Import erweitern | 2h |
| Sensor-Fusion: GPS/VSS-Auswahl-Logik in sensor_task | 3h |
| gear_indicator.c: Refactoring auf Config-Werte | 2h |
| Kalibrierungs-Task (30s Messung, Berechnung) | 3h |
| BLE-Callback: Kalibrierungs-Kommandos | 2h |
| Unit-Tests: VSS-Speed-Berechnung, Kalibrierung | 3h |
| **Zwischensumme Firmware** | **25h** |

### 6.2 Companion-App

| Aufgabe | Aufwand |
|---|---|
| CalibrationScreen.tsx (4-Schritt-Wizard) | 6h |
| Reifengroesse-Dropdown + Umfang-Rechner | 2h |
| Kalibrierungs-Fortschritts-Anzeige | 2h |
| Config-Write/Read fuer neue Felder | 2h |
| Settings: VSS-Optionen (Pin, Source) | 2h |
| Unit-Tests | 2h |
| **Zwischensumme App** | **16h** |

### 6.3 Hardware (Dokumentation)

| Aufgabe | Aufwand |
|---|---|
| VSS-Montage-Anleitung (Hall-Sensor am Vorderrad) | 2h |
| Pinmap-Aktualisierung | 1h |
| Schaltplan-Ergaenzung | 1h |
| **Zwischensumme Hardware** | **4h** |

### 6.4 Gesamt

| Bereich | Aufwand |
|---|---|
| Firmware | 25h |
| Companion-App | 16h |
| Hardware-Doku | 4h |
| Integrationstests | 3h |
| **Gesamt** | **48h** |

---

## 7. Implementierungs-Plan

### Phase 1: VSS-Hardware und Firmware

1. `firmware/components/sensors/vss.c` + `vss.h` implementieren
2. `board_config.h` um VSS-Definitionen erweitern
3. `device_config_t` erweitern (CONFIG_VERSION=4, Migration)
4. `config.c` um neue Keys und JSON-Felder erweitern
5. `sensor_task` um VSS-Integration und Sensor-Fusion erweitern
6. `gear_indicator.c` von Compile-Konstanten auf Config-Werte umstellen
7. Unit-Tests fuer VSS-Berechnung und Kalibrierung

### Phase 2: Auto-Kalibrierung

1. Kalibrierungs-Task implementieren (30s Messung, Bedingungen pruefen)
2. BLE-Callbacks fuer calib_start, calib_accept Kommandos
3. Notify-Characteristics fuer Kalibrierungs-Fortschritt
4. Integrationstest mit GPS + VSS

### Phase 3: Companion-App

1. `CalibrationScreen.tsx` als 4-Schritt-Wizard implementieren
2. Reifengroesse-Tabelle und Umfang-Rechner
3. Fortschritts- und Ergebnis-Anzeige
4. Settings: VSS-Pin und Speed-Source konfigurierbar
5. TabNavigator um Kalibrierungs-Entry erweitern

### Phase 4: Hardware-Dokumentation

1. Montage-Anleitung fuer Hall-Sensor am Vorderrad
2. Pinmap und Schaltplan aktualisieren
3. BOM um VSS-Komponenten erweitern

---

## 8. Abhaengigkeiten

- **Config-Migration:** CONFIG_VERSION muss von 3 auf 4 erhoeht werden.
  Bestehende Geraete mit NVS-Config v3 werden beim naechsten Boot
  automatisch migriert. `config_migrate()` fuellt die neuen Felder
  mit Defaults.

- **gear_indicator.c Refactoring:** Die aktuelle Implementierung
  verwendet #define-Konstanten fuer Uebersetzungen und Radumfang.
  Diese muessen auf `s_config.*` Werte umgestellt werden, damit die
  Kalibrierung auch die Gang-Erkennung korrigiert.

- **PCNT-Ressource:** PCNT Unit 0 Channel 0 ist fuer RPM belegt.
  Channel 1 auf derselben Unit ist fuer VSS vorgesehen. Beide
  koennen unabhaengig konfiguriert und ausgelesen werden.

- **BLE-GATT:** Die neuen Kommandos laufen ueber den bestehenden
  Config-Service (0x6000). Es sind keine neuen Characteristics
  noetig -- `CHR_CONFIG_WRITE` (0x6002) nimmt JSON-Kommandos,
  `CHR_CONFIG_READ` (0x6001) liefert JSON-Antworten.

---

## 9. Referenzen

- SAE J1979: Vehicle Speed PID 0x0D (fuer OBD-II-kompatible Fahrzeuge)
- Honda PD06 Service Manual: Primaer-Uebersetzung 2.733, Sekundaer 2.933 (15/44)
- Reifengroesse 130/80-17: Umfang = pi * (431.8 + 2*104) = 2009 mm
- Firmware: `firmware/components/sensors/gear_indicator.c` (Uebersetzungs-Konstanten)
- Firmware: `firmware/components/sensors/rpm.c` (PCNT-Pattern fuer RPM, analog fuer VSS)
- Firmware: `firmware/components/storage/config.c` (device_config_t, JSON-Export)
- Firmware: `firmware/main/board_config.h` (Pin-Definitionen)
- Firmware: `firmware/components/gps/include/gps.h` (gps_data_t mit speed_kmh)
- Companion-App: `companion-app/src/screens/SettingsScreen.tsx`
- Doku: `docs/research/01-sensoren-fahrzeugdaten.md` (Geschwindigkeit, Hall-Sensor)
- Doku: `docs/research/04-pd06-technische-daten.md` (Uebersetzungen, Reifengroesse)

---

*Dieses Dokument spezifiziert die Tachokalibrierung fuer den Transalp Bordcomputer.
VSS-basierte Geschwindigkeitsmessung als GPS-Ergaenzung, Auto-Kalibrierung per GPS-Vergleich.
Gesamtaufwand: 48h. Phase 1 Firmware (25h), Phase 2 Kalibrierung (in Firmware), Phase 3 App (16h), Phase 4 Doku (4h).*

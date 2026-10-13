# E-Paper Widget-Katalog & Display-Screen-Referenz

> **Phase 5 — Referenzdokument fuer Entwickler**
> Bordcomputer Transalp 600 (PD06)
> MCU: ESP32-S3-WROOM-1 (N16R8: 16 MB Flash, 8 MB Octal PSRAM), Dual-Core 240 MHz
> Framework: ESP-IDF v5.4 mit FreeRTOS SMP
> Displays: 2.9" E-Paper b/w (296x128, SSD1680, 1-Bit) + 2.4" TFT (240x320, ILI9341, RGB565)
> Stand: Oktober 2026
> Quellen: `firmware/components/display/`, `docs/ARCHITECTURE.md`, `docs/research/10-ui-ux-design.md`, `docs/PERFORMANCE-OPTIMIZATION.md`

---

## Inhaltsverzeichnis

1. [Widget-System Uebersicht](#1-widget-system-uebersicht)
2. [Widget-Katalog](#2-widget-katalog)
3. [Screen-Layouts](#3-screen-layouts)
4. [Widget-Entwicklungs-Guide](#4-widget-entwicklungs-guide)
5. [Farbtiefe & Rendering](#5-farbtiefe--rendering)

---

## 1. Widget-System Uebersicht

### 1.1 Architektur: Wie Widgets registriert, gelayoutet und gerendert werden

Das Bordcomputer-Display-Subsystem folgt einer vierschichtigen Rendering-Pipeline.
Widgets sind die oberste Schicht — sie kapseln jeweils einen visuellen
Informationsbaustein (z.B. Geschwindigkeit, Drehzahl, Temperatur) und zeichnen
in einen gemeinsamen Framebuffer ueber die Renderer-Paint-Engine.

```
  Widget Tree        Layout Engine       Paint Engine
  +----------+       +-------------+     +----------------+
  | Screen   |  -->  | Position    | --> | Draw Prims     |
  |  + RPM   |       | Size        |     | Rect/Circle/   |
  |  + Speed |       | Bounds      |     | Text/Line/Arc  |
  |  + Gear  |       +-------------+     +----------------+
  +----------+                                    |
                                                   v
  +----------+       +-------------+     +----------------+
  | SPI/DMA  | <--  | Compositor  | <-- | Dirty-Rect     |
  | Blit     |       | Merge Rects |     | Tracker        |
  +----------+       +-------------+     +----------------+
```

Die Pipeline laeuft im `display_task` auf Core 0. Pro Frame-Zyklus:

1. **Daten pullen** — `display_task` liest non-blocking aus `gps_data_queue`
   und `sensor_data_queue` (siehe `display.c:33-36`).
2. **Framebuffer loeschen** — `renderer_clear()` setzt den gesamten Buffer
   auf Schwarz (TFT) bzw. Weiss (E-Paper).
3. **Widgets zeichnen** — Jedes Widget ruft Paint-Primitives
   (`renderer_fill_rect`, `renderer_draw_line`, `renderer_draw_circle`)
   auf, die in den Back-Buffer schreiben und Dirty-Rects markieren.
4. **Flush** — `renderer_flush(true)` merged Dirty-Rects, uebertraegt
   geaenderte Regionen via SPI/DMA an das Display (Partial-Refresh bei
   E-Paper, Dirty-Region-Push bei TFT).

Widgets sind **stateless**: Sie nehmen einen Wert und zeichnen ihn. Alle
Animationen (RPM-Balken-Traegheit, Speed-Rolling, Gang-Slide) werden
ueber die Renderer-Animation-Queue abgewickelt, nicht innerhalb des Widgets
selbst (siehe `widgets.c:8-9`).

### 1.2 Widget-Basisklasse (C-Struktur)

In der aktuellen Implementierung sind Widgets als freie Funktionen
implementiert (z.B. `widgets_draw_speed()`, `widgets_draw_rpm_bar()`).
Die folgende Struktur definiert das einheitliche Widget-Interface, das fuer
die Registrierung in einer Widget-Registry verwendet wird:

```c
/* Widget-Typ-Enum: identifiziert jeden Widget-Typ */
typedef enum {
    WIDGET_SPEED_GAUGE = 0,
    WIDGET_RPM_BAR,
    WIDGET_TEMP_GAUGE,
    WIDGET_FUEL_GAUGE,
    WIDGET_GEAR_INDICATOR,
    WIDGET_VOLTAGE_DISPLAY,
    WIDGET_CLOCK_DISPLAY,
    WIDGET_ODOMETER_DISPLAY,
    WIDGET_TRIP_DISPLAY,
    WIDGET_GPS_STATUS,
    WIDGET_IMU,
    WIDGET_ALTITUDE,
    WIDGET_COMPASS,
    WIDGET_WARNING_INDICATOR,
    WIDGET_BLE_STATUS,
    WIDGET_TOUR_STATS,
    WIDGET_TURN_BY_TURN,
    WIDGET_WEATHER,
    WIDGET_TYPE_COUNT
} widget_type_t;

/* Refresh-Strategie: bestimmt, wie oft das Widget aktualisiert wird */
typedef enum {
    REFRESH_FULL = 0,       /* Full-Refresh (3s, bei Screen-Wechsel) */
    REFRESH_PARTIAL,        /* Partial-Refresh (0.3s, bei Wert-Aenderung) */
    REFRESH_VALUE_BASED,    /* Nur bei signifikanter Aenderung */
} refresh_strategy_t;

/* Widget-Basisklasse */
typedef struct {
    widget_type_t      type;          /* Widget-Typ */
    const char        *name;          /* Anzeigename (fuer Diagnose) */
    rect_t             bounds;        /* Position & Groesse (x,y,w,h) */
    refresh_strategy_t refresh;       /* Refresh-Strategie */
    uint16_t           update_rate_ms;/* Update-Intervall in ms */
    void             (*draw_fn)(const void *data, uint16_t x, uint16_t y);
                                    /* Render-Funktion */
    bool               visible;       /* Sichtbar im aktuellen Layout? */
    bool               dirty;         /* Wert seit letztem Render geaendert? */
} widget_t;

/* Widget-Registry: statisches Array, alle Widgets */
static widget_t s_widget_registry[WIDGET_TYPE_COUNT];
```

Jedes Widget hat eine `draw_fn`, die einen Daten-Pointer und seine
Position erhaelt. Der `display_task` iteriert ueber alle sichtbaren
Widgets und ruft deren `draw_fn` auf.

### 1.3 Rendering-Pipeline: Widget -> Framebuffer -> E-Paper Update

Die Rendering-Pipeline besteht aus folgenden Schritten:

**Schritt 1 — Widget-Render:**
Ein Widget ruft Paint-Primitives auf, die in den Back-Buffer schreiben:

```c
/* Beispiel: SpeedGauge zeichnet in den Framebuffer */
void widgets_draw_speed(uint16_t speed_kmh, uint16_t x, uint16_t y,
                        uint8_t font_size)
{
    int digit_w = (SEG_W + SEG_SPACING) * font_size;
    int total_w = digit_w * 3;
    int total_h = SEG_H * font_size;

    /* Hintergrund loeschen */
    renderer_fill_rect(x, y, total_w, total_h, COLOR_BLACK);

    /* Zahl zeichnen */
    draw_seg_number(x, y, speed_kmh, 3, COLOR_FG_PRIMARY, font_size);

    /* Label */
    draw_text5x7(x, y + total_h + 2, "km/h", COLOR_FG_SECONDARY, 1);

    /* Dirty-Rect markieren */
    renderer_invalidate(x, y, total_w, total_h + 10);
}
```
*(Quelle: `firmware/components/display/widgets.c:194-212`)*

**Schritt 2 — Dirty-Rect-Tracking:**
Jeder Paint-Aufruf markiert den betroffenen Bereich als "dirty". Der
Renderer sammelt bis zu 16 Dirty-Rects (`MAX_DIRTY_RECTS`, siehe
`display.h:90`). Ueberlappende Rects werden vom Compositor zu einer
Bounding-Box zusammengefasst (`renderer.c:143-162`).

**Schritt 3 — Compositor:**
`renderer_flush()` ruft `dirty_merge()` auf, das eine O(n^2)-Schleife
ueber alle Dirty-Rects laeuft und ueberlappende zusammenfuehrt:

```c
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
*(Quelle: `firmware/components/display/renderer.c:143-162`)*

**Schritt 4 — Blit (E-Paper):**
Bei E-Paper werden bis zu 4 Dirty-Rects per Partial-Refresh uebertragen;
bei mehr als 4 Rects erfolgt ein Full-Refresh:

```c
if (partial && s_dirty.count <= 4) {
    for (int i = 0; i < s_dirty.count; i++) {
        epaper_display_partial(epaper_get_framebuffer(),
            s_dirty.rects[i].x, s_dirty.rects[i].y,
            s_dirty.rects[i].w, s_dirty.rects[i].h);
    }
} else {
    epaper_display_frame(epaper_get_framebuffer());
}
```
*(Quelle: `firmware/components/display/renderer.c:451-464`)*

### 1.4 Update-Strategien: Full-Refresh vs. Partial-Refresh

Das E-Paper-Display (SSD1680, 296x128, 1-Bit) unterstuetzt zwei
Update-Modi:

| Modus | Dauer | Verwendung | Ghosting |
|---|---|---|---|
| Full-Refresh | ~3 s | Screen-Wechsel, Ghosting-Reset | Nein |
| Partial-Refresh | ~0.3 s | Wert-Aenderungen (< 4 Rects) | Ja (kumulativ) |

**Ghosting-Schutz:** Nach 50 Partial-Refreshs wird automatisch ein
Full-Refresh erzwungen. Bei Temperaturen unter 10 C wird die Schwelle
auf 20 reduziert, da Partial-Refresh bei Kaelte unzuverlaessig wird.

**Wertbasierte Refresh-Trigger:** Der Refresh wird nicht zeitbasiert
ausgeloest, sondern nur wenn sich ein Wert signifikant geaendert hat:

| Widget | Trigger |
|---|---|
| SpeedGauge | Aenderung >= 1 km/h |
| RpmBar | Aenderung >= 100 rpm |
| FuelGauge | Aenderung >= 5% |
| ClockDisplay | Jede volle Minute |
| AltitudeWidget | Aenderung >= 5 m |
| TempGauge | Aenderung >= 1 C |

Dies reduziert die Refresh-Anzahl drastisch. Bei konstanter Geschwindigkeit
aktualisiert sich das E-Paper kaum — das spart Strom und minimiert Ghosting.

### 1.5 Memory-Layout: Wie Widgets im RAM liegen

| Puffer | Groesse | Speicherort | Beschreibung |
|---|---|---|---|
| E-Paper Framebuffer | 4.736 Bytes (296x128/8) | PSRAM | 1-Bit, aktueller Frame |
| E-Paper Prev-Buffer | 4.736 Bytes | PSRAM | 1-Bit, vorheriger Frame (Diff) |
| TFT Framebuffer (Front) | 153.600 Bytes (240x320x2) | PSRAM | RGB565, auf Display |
| TFT Framebuffer (Back) | 153.600 Bytes | PSRAM | RGB565, Paint-Buffer |
| TFT Bounce-Buffer | 19.200 Bytes (240x40x2) | Internes SRAM | DMA-faehig |
| Dirty-Rect-Liste | 64 Bytes (16 x 4 Bytes) | Internes SRAM | Stack-static |
| Widget-Registry | ~512 Bytes | Internes SRAM | Static array |
| Font-Data (5x7) | 480 Bytes (96 x 5) | Flash (const) | Bitmap-Glyphen |
| 7-Seg-Patterns | 70 Bytes (10 x 7) | Flash (const) | Bitmap-Glyphen |

Die E-Paper-Buffer liegen im PSRAM, weil sie zwar klein sind, aber
gemeinsam mit dem TFT-Framebuffer allokiert werden. Der PSRAM hat
8 MB, die Display-Buffer belegen insgesamt ~316 KB — kein Engpass.

Der Bounce-Buffer (internes SRAM, DMA-faehig) ist fuer den TFT
zwingend: SPI-DMA kann nicht direkt von PSRAM lesen, daher werden
Daten in Bloecken von 40 Zeilen in den SRAM kopiert und von dort per
DMA an das Display uebertragen (siehe `tft_driver.c:382-395`).

---

## 2. Widget-Katalog

### 2.1 SpeedGauge — Tacho mit 7-Segment-Digitalanzeige

**Zweck:** Zeigt die aktuelle Geschwindigkeit als grosse 7-Segment-Zahl.
Primaeres Widget, zentral auf dem Bildschirm positioniert.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | 3 Ziffern x 24px = ~78px breit, 28px hoch | Bei font_size=4 |
| Position | Zentriert, X=5 Y=10 (E-Paper) | Siehe `display.c:42` |
| Update-Rate | 1 Hz (E-Paper), 10 Hz (TFT) | GPS-Update-Rate |
| Refresh-Typ | Partial (bei >= 1 km/h Aenderung) | Wertbasiert |

**Datenquelle:** `gps_data_t.speed_kmh` (GPS ueber UART, 10 Hz)

**Rendering-Funktion:**

```c
void widgets_draw_speed(uint16_t speed_kmh, uint16_t x, uint16_t y,
                        uint8_t font_size);
```

Die Funktion verwendet das 7-Segment-Font-System (`seg_digits[10][7]`,
siehe `widgets.c:36-47`). Jede Ziffer ist 5 Pixel breit und 7 Pixel hoch,
mit einem Scale-Faktor multipliziert. Bei `font_size=4` ist jede Ziffer
20x28 Pixel, drei Ziffern ergeben 60+2=62 Pixel Gesamtbreite.

**Abhaengigkeiten:** `renderer_fill_rect`, `renderer_invalidate`,
`gps_data_queue`

**E-Paper-Variante:** Speed in 48px Bold (font_size ~ 7), zentriert in
der oberen Haelfte. Keine Animation (statisch, aktualisiert bei
>= 1 km/h Aenderung).

---

### 2.2 RpmBar — Drehzahl-Balken mit Shift-Light-Zone

**Zweck:** Horizontaler Balken, der sich von links nach rechts fuellt.
Farbcodierung kommuniziert die Laststufe. Shift-Light-Marker bei 85%.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | 180x16 px (E-Paper), 440x24 px (TFT) | Variabel |
| Position | X=60 Y=50 (E-Paper), X=10 Y=0 (TFT, voll breit) | |
| Update-Rate | 50 Hz (Sensor-Task) | Echtzeit |
| Refresh-Typ | Partial (bei >= 100 rpm Aenderung) | Wertbasiert |

**Datenquelle:** `sensor_data_t.rpm` (PCNT0, Pulse-Counter, 50 Hz)

**Rendering-Funktion:**

```c
void widgets_draw_rpm_bar(uint16_t rpm, uint16_t max_rpm,
                          uint16_t x, uint16_t y, uint16_t w, uint16_t h);
```

**Farbverlauf (TFT, RGB565):**

| Drehzahl-Bereich | Farbe | RGB565 | Bedeutung |
|---|---|---|---|
| 0-40% | Gruen | `COLOR_ACCENT_GREEN` (0x3660) | Normaler Fahrbetrieb |
| 40-70% | Gelb | `COLOR_ACCENT_YELLOW` (0xFFE0) | Mittlere Last |
| 70-85% | Orange | `COLOR_ACCENT_ORANGE` (0xFB20) | Sportlicher Bereich |
| 85-100% | Rot | `COLOR_ACCENT_RED` (0xF800) | Shift-Light aktiv |
| 95-100% | Blinkend Rot | `COLOR_ACCENT_RED` (blink) | Drehzahlbegrenzer |

*(Quelle: `widgets.c:240-249`)*

**E-Paper-Variante:** Auf dem 1-Bit-Display entfaellt der Farbverlauf.
Stattdessen wird die Warnzone durch eine Schraffur kommuniziert: der
Balken ist durchgezogen im Normalbereich und schraffiert ab 85%. Eine
zusaetzliche Markierungslinie am Rotationsmaximum dient als "rote Linie".

**Shift-Light-Indikator:** Bei >= 85% erscheint ein roter Kreis
(`renderer_fill_circle`) rechts oben ueber dem Balken
(siehe `widgets.c:258-260`).

**Animation (TFT):** Zweiphasen-Modell — Aufwaerts schnell (120ms,
ease_out), Abwaerts mit Traegheit (250ms, ease_in_out) plus Overshoot
bei starkem Abfall (150ms, ease_out_back). Auf E-Paper: keine
Animation, statische Updates.

---

### 2.3 TempGauge — Temperaturanzeige mit Warn-Zonen

**Zweck:** Kuehlmitteltemperatur mit Farbcodierung fuer Warn-Zonen.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | 50x28 px | Bei font_size=2 |
| Position | X=5 Y=90 (E-Paper) | Siehe `display.c:49` |
| Update-Rate | 50 Hz (Sensor) | EMA-gefiltert |
| Refresh-Typ | Partial (bei >= 1 C Aenderung) | Wertbasiert |

**Datenquelle:** `sensor_data_t.coolant_temp_c` (NTC 10k B3950 ueber
ADC1_CH6, Steinhart-Hart-Gleichung)

**Rendering-Funktion:**

```c
void widgets_draw_temp(int8_t temp_c, uint16_t x, uint16_t y);
```

**Farbcodierung:**

| Temperatur | Farbe | RGB565 | Bedeutung |
|---|---|---|---|
| < 40 C | Gruen | `COLOR_ACCENT_GREEN` (0x3660) | Kalt (Normal) |
| 40-100 C | Gelb | `COLOR_ACCENT_YELLOW` (0xFFE0) | Betriebstemperatur |
| 100-115 C | Orange | `COLOR_ACCENT_ORANGE` (0xFB20) | Warnung |
| > 115 C | Rot | `COLOR_ACCENT_RED` (0xF800) | Kritisch (DTC P0202) |

*(Quelle: `widgets.c:327-335`)*

**Abhaengigkeiten:** `renderer_fill_rect`, `draw_text5x7`,
`sensor_data_queue`

---

### 2.4 FuelGauge — Tankanzeige mit Low-Fuel-Warning

**Zweck:** Kraftstoffstand als vertikaler Balken mit Farbcodierung.
Tankkapazitaet: 17 L, Reserve bei 3 L.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | 40x32 px | Balken + Label |
| Position | X=20 Y=90 (E-Paper) | Siehe `display.c:50` |
| Update-Rate | 50 Hz (Sensor) | EMA-gefiltert |
| Refresh-Typ | Partial (bei >= 5% Aenderung) | Wertbasiert |

**Datenquelle:** `sensor_data_t.fuel_level_l` (ADC1_CH2, 6-Punkt-Kennlinie)

**Rendering-Funktion:**

```c
void widgets_draw_fuel(uint8_t liters, uint8_t max_liters,
                       uint16_t x, uint16_t y);
```

**Farbcodierung:**

| Fuellstand | Farbe | Bedeutung |
|---|---|---|
| > 50% | Gruen (`COLOR_ACCENT_GREEN`) | Normal |
| 25-50% | Gelb (`COLOR_ACCENT_YELLOW`) | Niedrig |
| < 25% | Rot (`COLOR_ACCENT_RED`) | Reserve (DTC P0501) |

*(Quelle: `widgets.c:378-385`)*

**E-Paper-Variante:** Vertikaler Balken, Fuellung von unten nach oben.
Bei < 25% erscheint zusaetzlich ein kleines "RES" Label (blinkend auf
TFT, statisch auf E-Paper).

---

### 2.5 GearIndicator — Gang-Anzeige

**Zweck:** Aktuell eingelegter Gang als grosse Ziffer in einem Rahmen.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | 24x28 px (Box) + Label | |
| Position | X=200 Y=50 (E-Paper) | Siehe `display.c:48` |
| Update-Rate | 50 Hz (Sensor) | Speed/RPM-Ratio |
| Refresh-Typ | Partial (bei Gangwechsel) | Event-basiert |

**Datenquelle:** `sensor_data_t.gear` (berechnet aus Speed/RPM-Ratio,
siehe `gear_indicator.c`)

**Rendering-Funktion:**

```c
void widgets_draw_gear(uint8_t gear, uint16_t x, uint16_t y);
```

Der Gang wird als einzelner Buchstabe dargestellt: 0 = 'N' (Leerlauf,
in `COLOR_ACCENT_AMBER`), 1-6 als Ziffer (in `COLOR_FG_PRIMARY`).
Der Buchstabe wird mit Scale 3 (15x21 px) in der 24x28-Box zentriert
(siehe `widgets.c:296-303`).

**Animation (TFT):** Beim Schalten slidet die alte Gang-Zahl nach links
heraus (150ms, ease_in), die neue slidet von rechts herein mit leichtem
Bounce (150ms, ease_out_back, 8% Overshoot). Auf E-Paper: statischer
Update bei Gangwechsel.

---

### 2.6 VoltageDisplay — Bordspannung

**Zweck:** Akku-/Bordspannung in Volt mit Generator-Status-Indikator.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | ~36x20 px | 5-Zeichen-String |
| Position | Status-Leiste, rechts (TFT) | Y=220-262 |
| Update-Rate | 1 Hz (Power-Task) | |
| Refresh-Typ | Partial (bei >= 0.1V Aenderung) | Wertbasiert |

**Datenquelle:** `sensor_data_t.battery_v` (ADC1_CH0, Spannungsteiler
10:1). Generator-Status: `sensor_data_t.generator_ok` (Bordspannung
> 13.8V = Generator laeuft).

**Rendering (Pseudocode):**

```c
void widgets_draw_voltage(float voltage, bool generator_ok,
                          uint16_t x, uint16_t y)
{
    char volt_str[8];
    snprintf(volt_str, sizeof(volt_str), "%.1fV", voltage);

    uint16_t color = (voltage < 11.5f || voltage > 14.8f)
                     ? COLOR_ACCENT_RED : COLOR_FG_PRIMARY;
    draw_text5x7(x, y, volt_str, color, 1);

    /* Generator-Indikator */
    if (generator_ok) {
        draw_text5x7(x, y + 8, "GEN", COLOR_ACCENT_GREEN, 1);
    }
    renderer_invalidate(x, y, 36, 16);
}
```

**Warn-Schwellen:** < 11.5V = DTC P0401 (Bordspannung kritisch),
rot blinkend. > 14.8V = Ueberladung, rot.

---

### 2.7 ClockDisplay — Uhrzeit

**Zweck:** Aktuelle Uhrzeit im 24-Stunden-Format.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | ~60x14 px (E-Paper), 60x24 px (TFT) | |
| Position | Zentriert, Y=120 (E-Paper), Y=262 (TFT) | |
| Update-Rate | 1 Hz (System-Tick) | |
| Refresh-Typ | Partial (jede volle Minute) | Wertbasiert |

**Datenquelle:** FreeRTOS-Systemzeit (`xTaskGetTickCount()`), synchronisiert
mit GPS-PPS-Signal (GPIO39, 1PPS).

**Rendering (Pseudocode):**

```c
void widgets_draw_clock(uint16_t x, uint16_t y, uint8_t scale)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
    uint8_t hh = (now / 3600) % 24;
    uint8_t mm = (now / 60) % 60;

    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02u:%02u", hh, mm);
    draw_text5x7(x, y, time_str, COLOR_FG_PRIMARY, scale);
    renderer_invalidate(x, y, 36, 7 * scale);
}
```

---

### 2.8 OdometerDisplay — Gesamtkilometerstand

**Zweck:** Gesamte zurueckgelegte Strecke seit Inbetriebnahme.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | ~60x14 px | |
| Position | Status-Leiste | |
| Update-Rate | 1 Hz | |
| Refresh-Typ | Partial (bei >= 1 km Aenderung) | Wertbasiert |

**Datenquelle:** Persistiert in NVS (`device_config_t`-Blob). Berechnet
aus GPS-Distanz (kumuliert `gps_data_t.speed_kmh` ueber Zeit).

---

### 2.9 TripDisplay — Trip A/B mit Reset

**Zweck:** Zwei unabhaengige Trip-Zaehler (A und B), zuruecksetzbar.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | ~60x14 px pro Trip | |
| Position | Status-Leiste | |
| Update-Rate | 1 Hz | |
| Refresh-Typ | Partial | Wertbasiert |

**Datenquelle:** GPS-Distanz, kumuliert. Reset ueber Menu-Taster.

**Rendering (Pseudocode):**

```c
void widgets_draw_trip(float trip_a_km, float trip_b_km,
                       uint16_t x, uint16_t y)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "A %.1f  B %.1f", trip_a_km, trip_b_km);
    draw_text5x7(x, y, buf, COLOR_FG_SECONDARY, 1);
    renderer_invalidate(x, y, 90, 8);
}
```

---

### 2.10 GpsStatusWidget — GPS-Fix, Satelliten, HDOP

**Zweck:** GPS-Status mit Satellitenanzahl und horizontaler Praezision.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | ~50x20 px | |
| Position | Status-Leiste, 4. Spalte (TFT) | Y=220 |
| Update-Rate | 10 Hz (GPS) | |
| Refresh-Typ | Partial (bei Sats-Aenderung) | Wertbasiert |

**Datenquelle:** `gps_data_t.satellites`, `gps_data_t.fix`,
`gps_data_t.hdop`

**Rendering (Pseudocode):**

```c
void widgets_draw_gps_status(uint8_t sats, bool fix, float hdop,
                             uint16_t x, uint16_t y)
{
    uint16_t color = fix ? COLOR_ACCENT_GREEN : COLOR_FG_TERTIARY;
    char sats_str[12];
    snprintf(sats_str, sizeof(sats_str), "%u SAT", sats);
    draw_text5x7(x, y, sats_str, color, 1);

    if (fix) {
        char hdop_str[8];
        snprintf(hdop_str, sizeof(hdop_str), "%.1f", hdop);
        draw_text5x7(x, y + 8, hdop_str, COLOR_FG_SECONDARY, 1);
    }
    renderer_invalidate(x, y, 50, 16);
}
```

**Farbcodierung:** Gruen bei 3D-Fix, Gelb bei 2D-Fix, Grau bei keinem Fix.

---

### 2.11 ImuWidget — Neigung/Beschleunigung (Roll/Pitch)

**Zweck:** Roll- und Nickwinkel des Motorrads in Grad.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | ~60x14 px pro Wert | |
| Position | Y=240 (TFT, Diagnose-Screen) | |
| Update-Rate | 50 Hz (Sensor) | Complementary-Filter |
| Refresh-Typ | Partial (bei >= 1 Aenderung) | Wertbasiert |

**Datenquelle:** `sensor_data_t.roll_deg`, `sensor_data_t.pitch_deg`
(MPU6050, Complementary-Filter)

**Rendering (Pseudocode):**

```c
void widgets_draw_imu(float roll, float pitch, uint16_t x, uint16_t y)
{
    char buf[24];
    snprintf(buf, sizeof(buf), "R %+4.1f  P %+4.1f", roll, pitch);
    draw_text5x7(x, y, buf, COLOR_ACCENT_CYAN, 1);
    renderer_invalidate(x, y, 60, 8);
}
```

---

### 2.12 AltitudeWidget — Hoehe (barometrisch + GPS)

**Zweck:** Aktuelle Hoehe ueber NN.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | 60x24 px | |
| Position | X=180 Y=10 (E-Paper) | Siehe `display.c:43` |
| Update-Rate | 50 Hz (BMP280) | |
| Refresh-Typ | Partial (bei >= 5m Aenderung) | Wertbasiert |

**Datenquelle:** `sensor_data_t.altitude_m` (BMP280, barometrisch).
Falls GPS verfuegbar: `gps_data_t.altitude_m` als Fallback.

**Rendering-Funktion:**

```c
void widgets_draw_altitude(float altitude_m, uint16_t x, uint16_t y);
```

*(Quelle: `firmware/components/display/widgets.c:517-532`)*

Zeigt den Hoehenwert in Metern als Text in `COLOR_ACCENT_CYAN` mit
Scale 2, gefolgt von einem "ALT" Label in `COLOR_FG_TERTIARY`.

---

### 2.13 CompassWidget — Kompass mit Heading

**Zweck:** Kurs/Heading-Anzeige in Grad mit Himmelsrichtung.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | ~50x20 px | |
| Position | Status-Leiste | |
| Update-Rate | 10 Hz (GPS) | |
| Refresh-Typ | Partial (bei >= 5 Aenderung) | Wertbasiert |

**Datenquelle:** `gps_data_t.course_deg` (GPS-Heading)

**Rendering (Pseudocode):**

```c
void widgets_draw_compass(float heading, uint16_t x, uint16_t y)
{
    const char *dirs[] = {"N","NO","O","SO","S","SW","W","NW"};
    uint8_t idx = ((int)(heading + 22.5f) / 45) % 8;
    char buf[16];
    snprintf(buf, sizeof(buf), "%3.0f %s", heading, dirs[idx]);
    draw_text5x7(x, y, buf, COLOR_FG_PRIMARY, 1);
    renderer_invalidate(x, y, 50, 8);
}
```

---

### 2.14 WarningIndicator — Warn-Icons

**Zweck:** Ueberlagerungs-Banner fuer kritische Warnungen
(Uebertemperatur, Oeldruck, Bordspannung, Kraftstoff).

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | Voll breit x 20 px (oben) | |
| Position | Y=0 (Top-Overlay) | |
| Update-Rate | Event-basiert | Bei DTC-Trigger |
| Refresh-Typ | Full (Overlay ein/aus) | |

**Datenquelle:** `warning_type_t`-Enum (siehe `display.h:42-49`):
`WARNING_OVERTEMP`, `WARNING_LOW_VOLTAGE`, `WARNING_LOW_FUEL`,
`WARNING_NO_GPS`, `WARNING_BLE_LOST`, `WARNING_GENERIC`.

**Rendering-Funktion:**

```c
void widgets_draw_warning(warning_type_t type, const char *msg);
```

*(Quelle: `firmware/components/display/widgets.c:477-497`)*

Zeichnet einen roten Balken voll breit oben auf dem Display, mit
einem weissen Kreis-Icon (Ausrufezeichen) und dem Warnungs-Text
in weiss, Scale 2.

**Verhalten:**
- Kritische Warnungen bleiben sichtbar, bis der Zustand sich
  normalisiert (nicht automatisch ausgeblendet).
- Auf TFT: Balken pulsiert (1 Hz, Hintergrund wechselt rot/dunkelrot).
- Auf E-Paper: statischer roter (schwarzer) Balken, kein Pulsieren.

---

### 2.15 BleStatusWidget — BLE-Verbindungsstatus

**Zweck:** Anzeige der Bluetooth-Verbindung zur Companion-App.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | ~80x14 px (Icon + Text) | |
| Position | Status-Leiste, links | Y=280 (TFT) |
| Update-Rate | Event-basiert | Bei Connect/Disconnect |
| Refresh-Typ | Partial | |

**Datenquelle:** Event-Bus (`EVT_BLE_CONNECTED` / `EVT_BLE_DISCONNECTED`)

**Rendering (Pseudocode):**

```c
void widgets_draw_ble_status(bool connected, uint16_t x, uint16_t y)
{
    uint16_t color = connected ? COLOR_ACCENT_BLUE : COLOR_FG_TERTIARY;
    const char *text = connected ? "Verbunden" : "Getrennt";
    draw_text5x7(x + 12, y, text, color, 1);

    /* BT-Icon (vereinfacht: kleines Rechteck mit Blitz) */
    renderer_draw_line(x, y, x + 8, y + 7, color);
    renderer_draw_line(x + 8, y, x, y + 7, color);
    renderer_fill_rect(x + 3, y + 2, 3, 4, color);
    renderer_invalidate(x, y, 80, 8);
}
```

---

### 2.16 TourStatsWidget — Tour-Statistiken

**Zweck:** Zusammenfassung nach Tour-Ende: Distanz, Dauer, Durchschnitts-
geschwindigkeit, Hoehenmeter.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | Vollbild | Eigener Screen |
| Position | Screen-Zentriert | |
| Update-Rate | Einmalig bei Tour-Ende | |
| Refresh-Typ | Full | |

**Datenquelle:** Tour-Log von SD-Karte (`/sdcard/logs/`), aggregiert
im `logger_task`.

**Rendering (Pseudocode):**

```c
typedef struct {
    float distance_km;
    uint32_t duration_s;
    float avg_speed_kmh;
    float max_speed_kmh;
    uint16_t max_rpm;
    float alt_start_m;
    float alt_max_m;
    float ascent_m;
    float descent_m;
    float fuel_used_l;
} tour_stats_t;

void widgets_draw_tour_stats(const tour_stats_t *stats, uint16_t x,
                             uint16_t y)
{
    /* Hero-Bereich: Distanz + Dauer */
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f", stats->distance_km);
    draw_text5x7(x + 20, y, buf, COLOR_FG_PRIMARY, 4);  /* gross */
    draw_text5x7(x + 20, y + 30, "Distanz (km)",
                 COLOR_FG_SECONDARY, 1);

    snprintf(buf, sizeof(buf), "%uh %umin",
             stats->duration_s / 3600,
             (stats->duration_s / 60) % 60);
    draw_text5x7(x + 120, y, buf, COLOR_FG_PRIMARY, 4);
    draw_text5x7(x + 120, y + 30, "Dauer",
                 COLOR_FG_SECONDARY, 1);

    /* Sekundaer-Statistiken */
    snprintf(buf, sizeof(buf), "Durchschnitt  %.0f km/h",
             stats->avg_speed_kmh);
    draw_text5x7(x, y + 60, buf, COLOR_FG_SECONDARY, 1);

    snprintf(buf, sizeof(buf), "Max-Speed     %.0f km/h",
             stats->max_speed_kmh);
    draw_text5x7(x, y + 70, buf, COLOR_FG_SECONDARY, 1);

    /* Hoehe */
    snprintf(buf, sizeof(buf), "Start %.0fm  Max %.0fm  +%+.0f  %.0f",
             stats->alt_start_m, stats->alt_max_m,
             stats->ascent_m, stats->descent_m);
    draw_text5x7(x, y + 90, buf, COLOR_FG_SECONDARY, 1);

    renderer_invalidate(x, y, 200, 110);
}
```

**Animation (TFT):** Count-Up-Animation — Werte zaehlen von 0 auf den
Endwert in 800ms mit ease_out. Hoehen-Balken wachsen gestaffelt von
unten nach oben (100ms Versatz pro Balken, 300ms pro Balken).

---

### 2.17 TurnByTurnWidget — Navigations-Anweisung

**Zweck:** Navigations-Pfeil mit Distanz und Strassenname, empfangen
ueber BLE Navigation-Service (0x1001).

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | 40x52 px (Pfeil + Text) | |
| Position | Zentriert, Y=10-150 (TFT) | |
| Update-Rate | 1-2 s (BLE Notify) | |
| Refresh-Typ | Partial (bei Maneuver-Aenderung) | Event-basiert |

**Datenquelle:** BLE Navigation-Service, Characteristic 0x1001
(`NAV_MANEUVER`): type(1B), distance(4B), street(var).

**Rendering-Funktion:**

```c
void widgets_draw_nav_arrow(uint16_t heading, uint16_t next_turn,
                            uint16_t x, uint16_t y);
```

*(Quelle: `firmware/components/display/widgets.c:405-471`)*

Der Pfeil richtet sich nach der relativen Differenz zwischen aktuellem
Heading und naechstem Maneuver:

| Relativer Winkel | Pfeil-Richtung |
|---|---|
| < 45 | Geradeaus (Pfeil nach oben) |
| 45 bis 135 | Rechts |
| -45 bis -135 | Links |
| > 135 | Wende (U-Turn, Pfeil nach unten) |

Auf dem TFT werden die Pfeile als Polygone gezeichnet und morphen
beim Maneuver-Wechsel ueber 300ms mit ease_in_out. Auf E-Paper wird
ein einfacher Pfeil aus Rechtecken gezeichnet (siehe Code).

**Naechster Maneuver-Countdown (TFT):**
- Bei < 500m: Pfeil vergroessert sich (100x100 -> 140x140, 800ms).
- Bei < 100m: Distanz-Zahl in 36px Bold eingeblendet.
- Bei < 30m: Pfeil pulsiert (1 Hz, 10% Groessenschwankung).

---

### 2.18 WeatherWidget — Temperatur & Wetter-Icon

**Zweck:** Aktuelle Wetterdaten, empfangen ueber BLE Weather-Service
(0x4001), geliefert von der Companion-App.

**Parameter:**

| Parameter | Wert | Beschreibung |
|---|---|---|
| Typische Groesse | ~40x20 px | |
| Position | Status-Leiste oder Mini-Widget | |
| Update-Rate | 15 min (BLE Notify) | |
| Refresh-Typ | Full (seltene Updates) | |

**Datenquelle:** BLE Weather-Service, Characteristic 0x4001
(`WX_CURRENT`): temp(i16), humidity(u8), condition(u8), wind(u8),
feels(i16), rain(u8).

**Rendering (Pseudocode):**

```c
/* Wetter-Condition-Codes */
enum {
    WX_CLEAR = 0, WX_CLOUDY, WX_OVERCAST, WX_RAIN,
    WX_THUNDERSTORM, WX_SNOW, WX_FOG,
};

void widgets_draw_weather(int16_t temp_c, uint8_t condition,
                          uint16_t x, uint16_t y)
{
    char temp_str[8];
    snprintf(temp_str, sizeof(temp_str), "%dC", temp_c);
    draw_text5x7(x, y, temp_str, COLOR_FG_PRIMARY, 1);

    /* Wetter-Icon (vereinfacht als Text-Symbol) */
    const char *icons[] = {"Sonne","Wolk","Bedeckt","Regen",
                           "Gewitt","Schnee","Nebel"};
    if (condition < 7) {
        draw_text5x7(x, y + 8, icons[condition],
                     COLOR_FG_SECONDARY, 1);
    }
    renderer_invalidate(x, y, 40, 16);
}
```

**Offline-Verhalten:** Bei BLE-Verbindungsabbruch bleiben die letzten
Wetterdaten 15 Minuten gueltig, danach zeigt das Widget "Wetter Offline".

---

## 3. Screen-Layouts

### 3.1 Standard-Fahr-Layout (Instrument-Screen)

Das Standard-Layout beim Fahren ohne aktive Navigation. Alle
Kerninstrumente sind sichtbar.

**TFT (240x320):**

```
+--------------------------------------------------+
|                                                  |
|  [RPM-Balken: ======== 4500 =====    ]           |
|  [Shift Light |    Redline ]                      |
|                                                  |
|                    98                            |
|                   km/h                            |
|                                                  |
|                                                  |
|  [G:5]    [22 C]   [1/2]    [GPS 9]   [3.92V]    |
|                                                  |
|                    14:23                         |
|                                                  |
|  [BT Verbunden]    [TOUR]    [186 km heute]      |
+--------------------------------------------------+
```

**E-Paper (296x128):**

```
+------------------------------------------+
|  98 km/h   |   4500 rpm   G:5            |
|            |   [####  ----]              |
|------------|-----------------------------|
|  22 C   1/2   340m   9 SAT   12.4V      |
|  14:23                    TOUR           |
+------------------------------------------+
```

**Widget-Positionierung (E-Paper, 296x128):**

| Widget | X | Y | Groesse |
|---|---|---|---|
| SpeedGauge | 5 | 10 | font_size=4 (~62x28) |
| RpmBar | 60 | 50 | 180x16 |
| GearIndicator | 200 | 50 | 24x28 |
| TempGauge | 5 | 90 | 50x20 |
| FuelGauge | 20 | 90 | 40x20 |
| AltitudeWidget | 180 | 10 | 60x20 |
| ClockDisplay | 100 | 120 | 60x14 |
| BleStatusWidget | 5 | 120 | 80x8 |

*(Quelle: `firmware/components/display/display.c:42-50`)*

**Layout-Logik:** Der `display_task` ruft pro Frame alle sichtbaren
Widgets auf. Die Reihenfolge bestimmt die Z-Order (spaeter gezeichnete
Widgets liegen oben). Das WarningOverlay wird zuletzt gezeichnet, um
alle anderen Widgets zu ueberdecken.

---

### 3.2 Diagnose-Layout

Der Diagnose-Screen ist fuer den Stillstand (Werkstatt). Er zeigt
Sensor-Status, DTC-Liste und Self-Test-Ergebnisse.

```
+--------------------------------------------------+
|  DIAGNOSE                            [SELF-TEST] |
|--------------------------------------------------|
|                                                  |
|  SENSOREN:                                       |
|    GPS        OK    9 SAT  HDOP 1.2              |
|    IMU        OK    Roll: +5.2                   |
|    Baro       OK    842 m   1013 hPa              |
|    Temp       OK    22 C                          |
|    Fuel       OK    1/2   45%                     |
|    RPM        OK    4500                          |
|    Volt       OK    12.4V                         |
|                                                  |
|  FEHLERCODES:                                    |
|    [Keine aktiven Fehler]                        |
|                                                  |
|  LETZTE SELBST-TEST:  13.10.2026 14:20          |
|    Alle Systeme: OK                              |
+--------------------------------------------------+
```

**Layout-Logik:** Der Diagnose-Screen wird manuell ueber den Menu-Taster
angewaehlt (`DISPLAY_MODE_CONFIG`). Er ist nicht Teil des
automatischen Layout-Wechsels. Der Self-Test-Button triggert
`selftest_run()` und jede Sensor-Zeile wird nacheinander hervorgehoben.

---

### 3.3 Tour-Statistik-Layout

Wird beim Tour-Ende angezeigt (Zuendung aus oder manueller Tour-Stop).

```
+--------------------------------------------------+
|  TOUR-STATISTIK                      13.10.2026  |
|--------------------------------------------------|
|         142 km         3h 24min                  |
|        Distanz         Dauer                     |
|                                                  |
|  Durchschnitt       68 km/h                      |
|  Max-Speed         128 km/h                       |
|  Max-RPM          7800                           |
|                                                  |
|  Hoehe                                             |
|    Start 412m  Max 1247m  +950m  -820m           |
|    [Balkendiagramm]                              |
|                                                  |
|  Verbrauch        ca. 5.2 L  (34 km/L)          |
|                                                  |
|  [GPX exportieren]   [Statistik teilen]         |
+--------------------------------------------------+
```

*(Quelle: `mockups/screens/tour-statistik.html`)*

**Layout-Logik:** Der Tour-Statistik-Screen ersetzt den
Instrument-Screen vollstaendig. Er wird beim Tour-Ende fuer 30 Sekunden
angezeigt, danach kehrt das System in den Instrument-Screen zurueck
(oder in den Idle/Sleep-Modus, falls die Zuendung aus ist).

**E-Paper-Variante:** Auf dem E-Paper wird eine reduzierte Statistik
angezeigt: Distanz, Dauer, Durchschnitt. Keine Count-Up-Animation.

---

### 3.4 Boot/Splash-Screen

Die Boot-Animation laeuft waehrend der System-Initialisierung
(typisch 1.5-3 Sekunden).

**Sequenz (TFT):**

1. **0-200ms:** Bildschirm schwarz. Ein einzelner Punkt in der Mitte.
2. **200-600ms:** Punkt waechst zu einem Kreis (Radius 0->30px, ease_out).
3. **600-1000ms:** Senkrechter Strich waechst nach unten (2px, 40px lang).
4. **1000-1400ms:** Text "TRANSALP BORDCOMPUTER" fade-t ein (12px).
5. **1400-1800ms:** Horizontale Linie faehrt links nach rechts (200ms).
6. **1800-2000ms:** Alles fade-t aus, Haupt-Screen fade-t ein.

**E-Paper-Variante:** Statischer Splash-Screen mit Text
"TRANSALP BORDCOMPUTER" und einer horizontalen Trennlinie. Ein
Full-Refresh beim Boot (kein Ghosting von vorherigem Inhalt).

**Layout-Logik:** Der Splash-Screen wird in `display_init()` gezeichnet,
bevor der `display_task` startet. Er verwendet einen eigenen
Render-Zyklus (nicht die normale 1Hz/30FPS-Loop).

---

### 3.5 Warning-Overlay-Layout

Warnungen erscheinen als Top-Banner, das den Instrument-Screen
ueberlagert, ohne ihn zu ersetzen.

```
+--------------------------------------------------+
| [!] WARNUNG: UEBERTEMPERATUR 120C              |  <- Overlay (Y=0-20)
|--------------------------------------------------|
|                                                  |
|                    98                            |
|                   km/h                            |
|                                                  |
|  [G:5]    [22 C]   [1/2]    [GPS 9]   [3.92V]    |
|                                                  |
+--------------------------------------------------+
```

**Layout-Logik:** Das WarningOverlay wird nach allen anderen Widgets
gezeichnet (hoechste Z-Order). Es ist 20px hoch und voll breit. Bei
mehreren aktiven Warnungen stapeln sie sich vertikal. Kritische
Warnungen bleiben sichtbar, bis der Zustand behoben ist.

*(Quelle: `firmware/components/display/widgets.c:477-497`)*

---

### 3.6 Layout-Wechsel-Logik (Automatik vs. Manuel)

Der Bordcomputer unterstuetzt vier Display-Modi (siehe `display.h:35-39`):

```c
typedef enum {
    DISPLAY_MODE_TOUR = 0,   /* Standard: Speed, RPM, Hoehe, Temp */
    DISPLAY_MODE_TRACK,      /* GPS-Fokus: Position, HDOP, Distanz */
    DISPLAY_MODE_NAV,        /* Navigations-Modus */
    DISPLAY_MODE_CONFIG,     /* Einstellungen: BLE, Kalibrierung */
} display_mode_t;
```

**Automatischer Wechsel:**

| Trigger | Modus | Bedingung |
|---|---|---|
| BLE Nav-Service aktiv | `DISPLAY_MODE_NAV` | NAV_STATUS != inactive |
| Tour-Ende | Tour-Statistik-Screen | Zuendung aus oder manueller Stop |
| 60s keine Interaktion | Power-IDLE | Display aus, GPS+Logger an |
| Akku < 3.2V | Shutdown | Sofort, alle Zustaende |

**Manueller Wechsel:** Ueber drei Taster (Menu, Select, Back):
- Menu-Taster: Zyklisch durch die Modi (Tour -> Track -> Config -> Tour).
- Select-Taster: In Config-Modus, Wert aendern.
- Back-Taster: Zurueck zum vorherigen Modus.

**Uebergangs-Animation (TFT):** Slide-Transition, 250ms, ease_in_out.
Vorwaerts (naechster Modus) slidet von rechts, rueckwaerts von links.
Auf E-Paper: kein Slide, direkter Full-Refresh beim Modus-Wechsel.

---

## 4. Widget-Entwicklungs-Guide

### 4.1 Ein neues Widget erstellen (Schritt fuer Schritt)

**Schritt 1 — Widget-Typ registrieren:**

In `display.h`, Enum `widget_type_t` einen neuen Eintrag hinzufuegen:

```c
typedef enum {
    /* ... existing widgets ... */
    WIDGET_MY_NEW_WIDGET,
    WIDGET_TYPE_COUNT  /* muss letzter Eintrag bleiben */
} widget_type_t;
```

**Schritt 2 — Draw-Funktion implementieren:**

In `widgets.c` eine neue Funktion hinzufuegen. Die Signatur folgt dem
Muster `void widgets_draw_<name>(<parameter>, uint16_t x, uint16_t y)`.

```c
void widgets_draw_my_widget(int16_t value, uint16_t x, uint16_t y)
{
    /* 1. Hintergrund loeschen */
    renderer_fill_rect(x, y, 60, 20, COLOR_BLACK);

    /* 2. Wert formatieren und zeichnen */
    char buf[12];
    snprintf(buf, sizeof(buf), "%d", value);
    draw_text5x7(x, y, buf, COLOR_FG_PRIMARY, 2);

    /* 3. Label hinzufuegen */
    draw_text5x7(x, y + 16, "LABEL", COLOR_FG_TERTIARY, 1);

    /* 4. Dirty-Rect markieren */
    renderer_invalidate(x, y, 60, 24);
}
```

**Schritt 3 — Header-Deklaration:**

In `display.h` im Widget-API-Abschnitt:

```c
void widgets_draw_my_widget(int16_t value, uint16_t x, uint16_t y);
```

**Schritt 4 — In `display_task` einbinden:**

In `display.c`, innerhalb der `while(1)`-Schleife:

```c
if (have_sens) {
    /* ... existing widgets ... */
    widgets_draw_my_widget(sens.my_value, 100, 90);
}
```

**Schritt 5 — CMakeLists.txt anpassen (falls neue .c-Datei):**

Wenn das Widget in einer separaten Datei steht, diese in
`firmware/components/display/CMakeLists.txt` registrieren.

### 4.2 Widget-Registry: Wie registrieren

Die Widget-Registry (`s_widget_registry`) ist ein statisches Array mit
`WIDGET_TYPE_COUNT` Eintraegen. Ein Widget wird beim Systemstart
registriert:

```c
void widget_registry_init(void)
{
    /* SpeedGauge */
    s_widget_registry[WIDGET_SPEED_GAUGE] = (widget_t){
        .type         = WIDGET_SPEED_GAUGE,
        .name         = "Speed",
        .bounds       = {5, 10, 62, 28},
        .refresh      = REFRESH_VALUE_BASED,
        .update_rate_ms = 1000,   /* 1 Hz */
        .draw_fn      = (void*)widgets_draw_speed_wrapper,
        .visible      = true,
        .dirty        = true,
    };

    /* RPM Bar */
    s_widget_registry[WIDGET_RPM_BAR] = (widget_t){
        .type         = WIDGET_RPM_BAR,
        .name         = "RPM",
        .bounds       = {60, 50, 180, 16},
        .refresh      = REFRESH_PARTIAL,
        .update_rate_ms = 20,     /* 50 Hz */
        .draw_fn      = (void*)widgets_draw_rpm_bar_wrapper,
        .visible      = true,
        .dirty        = true,
    };

    /* ... weitere Widgets ... */
}
```

Der `display_task` iteriert dann ueber die Registry:

```c
for (int i = 0; i < WIDGET_TYPE_COUNT; i++) {
    widget_t *w = &s_widget_registry[i];
    if (!w->visible || !w->dirty) continue;

    /* Daten holen */
    /* w->draw_fn(data, w->bounds.x, w->bounds.y); */
    w->dirty = false;
}
```

### 4.3 Testing: Widgets isoliert testen

Widgets koennen isoliert getestet werden, indem der `display_task`
durch einen Test-Task ersetzt wird, der Mock-Daten an die Widget-Funktionen
uebergibt:

```c
/* Test-Task: simuliert Widget-Rendering mit Mock-Daten */
void widget_test_task(void *arg)
{
    renderer_init(DISPLAY_TYPE_EPAPER);
    renderer_clear();

    /* Test: SpeedGauge mit verschiedenen Werten */
    for (int speed = 0; speed <= 200; speed += 10) {
        renderer_clear();
        widgets_draw_speed(speed, 5, 10, 4);
        renderer_flush(true);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    /* Test: RpmBar mit Shift-Light */
    for (int rpm = 0; rpm <= 8000; rpm += 500) {
        renderer_clear();
        widgets_draw_rpm_bar(rpm, 8000, 60, 50, 180, 16);
        renderer_flush(true);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    /* Test: Warning-Overlay */
    renderer_clear();
    widgets_draw_speed(98, 5, 10, 4);
    widgets_draw_warning(WARNING_OVERTEMP, "UEBERTEMPERATUR 120C");
    renderer_flush(false);  /* Full refresh */
}
```

Dieser Test-Task kann in `main.c` statt des normalen `display_task`
gestartet werden, um Widgets ohne Sensor-Hardware zu verifizieren.

### 4.4 Performance-Guidelines (Update-Rate, Memory)

| Richtlinie | Empfehlung | Begruendung |
|---|---|---|
| Update-Rate | <= 1 Hz fuer E-Paper | Partial-Refresh dauert 0.3s |
| Dirty-Rect | So klein wie moeglich | Jedes Rect kostet SPI-Bandbreite |
| Font-Scale | <= 4 auf E-Paper | Groessere Fonts = groessere Dirty-Rects |
| Clear vor Draw | Immer Bereich loeschen | Vermeidet Ghost-Artefakte |
| String-Laenge | <= 16 Zeichen | Verhindert Buffer-Overflow |
| Float-Formatierung | snprintf, nicht printf | printf ist nicht reentrant |

**Memory-Budget pro Widget:**

Ein typisches Widget braucht:
- 0 Bytes RAM (stateless, zeichnet nur in Framebuffer)
- ~100-500 Bytes Flash (Code + String-Konstanten)
- ~64-200 Bytes Dirty-Rect (1-3 Rects pro Update)

Bei 18 Widgets sind das insgesamt ~2-9 KB Flash und < 4 KB Dirty-Rects
— unkritisch bei 16 MB Flash und 8 MB PSRAM.

### 4.5 Partial-Refresh-Strategien pro Widget-Typ

| Widget-Typ | Refresh-Strategie | Begruendung |
|---|---|---|
| SpeedGauge | Partial, wertbasiert (>= 1 km/h) | Haeufige, kleine Aenderungen |
| RpmBar | Partial, wertbasiert (>= 100 rpm) | Sehr haeufig, aber kleine Rects |
| TempGauge | Partial, wertbasiert (>= 1 C) | Langsame Aenderungen |
| FuelGauge | Partial, wertbasiert (>= 5%) | Sehr langsame Aenderungen |
| GearIndicator | Partial, event-basiert | Seltene Aenderungen (Schalten) |
| ClockDisplay | Partial, jede Minute | Minimale Refresh-Rate |
| AltitudeWidget | Partial, wertbasiert (>= 5m) | Mittlere Aenderungsrate |
| WarningIndicator | Full (Overlay ein/aus) | Vollflaechige Aenderung |
| TourStatsWidget | Full (einmalig) | Neuer Screen |
| TurnByTurnWidget | Partial, event-basiert | Bei Maneuver-Wechsel |
| WeatherWidget | Full (selten) | 15-Minuten-Intervall |

**Ghosting-Reset:** Unabhaengig von der Widget-Strategie wird nach 50
Partial-Refreshs ein Full-Refresh erzwungen. Dies ist eine globale
Zaehlervariable im Renderer, nicht pro Widget.

---

## 5. Farbtiefe & Rendering

### 5.1 E-Paper: 1-Bit (schwarz/weiss)

Das E-Paper-Display (SSD1680, 296x128) verwendet einen 1-Bit-Framebuffer:
jedes Pixel ist entweder schwarz oder weiss. Ein Byte kodiert 8 Pixel.

**Framebuffer-Layout:**

```c
#define EPD_BUF_SIZE  ((EPD_WIDTH * EPD_HEIGHT) / 8)  /* 4736 Bytes */

/* Bit-Werte: 1 = weiss, 0 = schwarz */
/* Byte-Reihenfolge: Zeilenweise, MSB first */
/* Pixel (x,y): byte = y * (EPD_WIDTH/8) + x/8, bit = 0x80 >> (x%8) */
```

*(Quelle: `firmware/components/display/epaper_driver.c:31`)*

**Pixel setzen (im Renderer):**

```c
if (color == COLOR_WHITE) {
    fb[byte_idx] |= bit;   /* 1 = white */
} else {
    fb[byte_idx] &= ~bit;  /* 0 = black */
}
```

*(Quelle: `firmware/components/display/renderer.c:216-219`)*

**Einschraenkungen:**
- Keine Graustufen (kein Anti-Aliasing moeglich)
- Keine Farbe (Farbcodierung wird durch Muster ersetzt)
- Keine Transparenz
- Keine Animationen (statische Updates)

**Strategie fuer Farb-Codes auf E-Paper:**
- Gruen/Gelb/Orange/Rot -> alle "schwarz" (durchgezogen)
- Warnung: Schraffur statt Farbe
- "Rote Linie" (Redline): schwarze Linie mit Pfeil-Symbol
- Status-Indikator: ausgefuellt = aktiv, leer = inaktiv

### 5.2 TFT: 16-Bit RGB565

Das TFT-Display (ILI9341, 240x320) verwendet einen 16-Bit-Framebuffer
im RGB565-Format: 5 Bit Rot, 6 Bit Gruen, 5 Bit Blau.

**Framebuffer-Layout:**

```c
#define TFT_BUF_SIZE  ((TFT_WIDTH * TFT_HEIGHT) * 2)  /* 153600 Bytes */

/* Pixel (x,y): uint16_t an Position y * TFT_WIDTH + x */
/* Byte-Reihenfolge: Little-Endian (low byte first) */
```

*(Quelle: `firmware/components/display/tft_driver.c:32`)*

**Farb-Hilfsfunktion:**

```c
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
```

*(Quelle: `firmware/components/display/include/display.h:52-55`)*

**Farbpalette (RGB565-Konstanten):**

| Token | RGB565 | Verwendung |
|---|---|---|
| `COLOR_BLACK` | 0x0000 | Hintergrund, Void |
| `COLOR_WHITE` | 0xFFFF | Primärer Text, Hauptwerte |
| `COLOR_BG_PANEL` | 0x1084 | Panel-Hintergrund |
| `COLOR_BG_RAISED` | 0x2104 | Erhoehte Elemente |
| `COLOR_FG_PRIMARY` | 0xFFFF | Hauptwerte |
| `COLOR_FG_SECONDARY` | 0x8C71 | Sekundaerwerte, Labels |
| `COLOR_FG_TERTIARY` | 0x4208 | Deaktiviert |
| `COLOR_ACCENT_RED` | 0xF800 | Warnungen, Redline |
| `COLOR_ACCENT_GREEN` | 0x3660 | Erfolg, GPS-Fix |
| `COLOR_ACCENT_BLUE` | 0x041F | Info, Navigation, BT |
| `COLOR_ACCENT_YELLOW` | 0xFFE0 | Shift-Light, Hinweis |
| `COLOR_ACCENT_ORANGE` | 0xFB20 | RPM-Mittelbereich |
| `COLOR_ACCENT_AMBER` | 0xFC80 | RPM-Start, Leerlauf |
| `COLOR_ACCENT_CYAN` | 0x4CDA | Telemetrie, Hoehe |

*(Quelle: `firmware/components/display/include/display.h:58-73`)*

### 5.3 Widget-Rendering fuer beide Display-Typen

Der Renderer abstrahiert den Display-Typ. Widgets rufen Paint-Primitives
auf, die je nach aktivem Display in den richtigen Framebuffer schreiben:

```c
void renderer_draw_pixel(int x, int y, uint16_t color)
{
    /* ... clip check ... */

    if (s_active_display == DISPLAY_TYPE_TFT) {
        tft_draw_pixel((uint16_t)x, (uint16_t)y, color);
    }
    if (s_active_display == DISPLAY_TYPE_EPAPER) {
        uint8_t *fb = epaper_get_framebuffer();
        if (fb) {
            uint16_t byte_idx = y * (EPD_WIDTH / 8) + x / 8;
            uint8_t bit = 0x80 >> (x % 8);
            if (color == COLOR_WHITE) {
                fb[byte_idx] |= bit;   /* white */
            } else {
                fb[byte_idx] &= ~bit;  /* black */
            }
        }
    }
    dirty_add(x, y, 1, 1);
}
```

*(Quelle: `firmware/components/display/renderer.c:188-224`)*

**Wichtiger Hinweis:** Auf dem E-Paper wird jede Farbe, die nicht
`COLOR_WHITE` (0xFFFF) ist, als schwarz interpretiert. Das bedeutet,
dass die gesamte Farbpalette auf dem E-Paper zu Schwarz/Weiss
kollabiert. Widgets, die Farbe zur Informationsuebermittlung verwenden
(z.B. RpmBar, TempGauge), muessen auf dem E-Paper zusaetzliche
Muster/Symbole verwenden, um die gleiche Information zu vermitteln.

### 5.4 Anti-Aliasing-Strategien

**TFT (RGB565):**

Echtes Anti-Aliasing ist auf dem ESP32-S3 mit 30 FPS nicht realistisch
fuer flaechige Renders. Die Strategie ist:

1. **Font-Anti-Aliasing:** Die 5x7-Bitmap-Fonts haben kein
   Anti-Aliasing. Bei hoeheren Scale-Faktoren (Scale 3-4) werden die
   Pixel einfach vergroessert, was zu treppenstufenartigen Kanten
   fuehrt. Abhilfe: bei finales Design 4-bit-gefilterte Bitmap-Fonts
   verwenden (mit `lv_font_conv` generiert).

2. **Linien:** Der Bresenham-Algorithmus (`renderer_draw_line`,
   siehe `renderer.c:254-270`) erzeugt harte Kanten. Fuer diagonale
   Linien ist das akzeptabel; fuer Kreise wird der
   Mittelpunkt-Algorithmus verwendet (`renderer_draw_circle`,
   siehe `renderer.c:272-290`), der ebenfalls hart ist.

3. **Kreise:** `renderer_fill_circle` verwendet eine sqrt-freie
   Annaeherung (Scanline mit Chord-Berechnung, siehe
   `renderer.c:292-304`).

**E-Paper (1-Bit):**

Anti-Aliasing ist auf einem 1-Bit-Display prinzipiell nicht moeglich.
Die Strategie ist:

1. **Hochskalierung:** Fonts werden mit ganzzahligen Scale-Faktoren
   skaliert (1x, 2x, 3x, 4x). Keine gebrochenen Faktoren — das
   verhindert Subpixel-Artefakte.

2. **Dithering (geplant):** Fuer Graustufen-Simulation koennte
   Floyd-Steinberg-Dithering verwendet werden. Dies ist in der
   aktuellen Implementierung nicht realisiert, aber der 1-Bit-Framebuffer
   unterstuetzt es prinzipiell.

3. **Kontrast-Maximierung:** Da es keine Abstufungen gibt, ist
   hoechster Kontrast (Schwarz auf Weiss) die beste Strategie.
   Alle Texte und Linien sind maximal 1px dick (bei Scale 1) oder
   werden als gefuellte Rechtecke gezeichnet.

---

## Anhang

### A. Verzeichnis der Quelldateien

| Datei | Zeilen | Beschreibung |
|---|---|---|
| `firmware/components/display/include/display.h` | 191 | Oeffentliches Interface, Farb-Tabelle, Typen |
| `firmware/components/display/display.c` | 59 | `display_task`, 1 Hz Refresh-Zyklus |
| `firmware/components/display/renderer.c` | 468 | Framebuffer, Dirty-Rect, Animation-Queue |
| `firmware/components/display/tft_driver.c` | 406 | ILI9341 TFT-Treiber (SPI, DMA, Double-Buffer) |
| `firmware/components/display/epaper_driver.c` | 378 | SSD1680 E-Paper-Treiber (SPI, Partial-Refresh) |
| `firmware/components/display/widgets.c` | 532 | UI-Widgets: Speed, RPM, Gear, Temp, Fuel, Nav, Warning, Altitude |

### B. Widget-Quick-Reference

| # | Widget | Draw-Funktion | Datenquelle | Refresh |
|---|---|---|---|---|
| 1 | SpeedGauge | `widgets_draw_speed()` | GPS speed_kmh | Partial, >= 1 km/h |
| 2 | RpmBar | `widgets_draw_rpm_bar()` | sensor_data_t.rpm | Partial, >= 100 rpm |
| 3 | TempGauge | `widgets_draw_temp()` | coolant_temp_c | Partial, >= 1 C |
| 4 | FuelGauge | `widgets_draw_fuel()` | fuel_level_l | Partial, >= 5% |
| 5 | GearIndicator | `widgets_draw_gear()` | sensor_data_t.gear | Partial, event |
| 6 | VoltageDisplay | (geplant) | battery_v | Partial, >= 0.1V |
| 7 | ClockDisplay | (geplant) | xTaskGetTickCount | Partial, /min |
| 8 | OdometerDisplay | (geplant) | NVS | Partial, >= 1 km |
| 9 | TripDisplay | (geplant) | GPS kumuliert | Partial |
| 10 | GpsStatusWidget | (geplant) | gps_data_t | Partial, sats change |
| 11 | ImuWidget | (geplant) | roll_deg, pitch_deg | Partial, >= 1 deg |
| 12 | AltitudeWidget | `widgets_draw_altitude()` | altitude_m | Partial, >= 5m |
| 13 | CompassWidget | (geplant) | gps_data_t.course_deg | Partial, >= 5 deg |
| 14 | WarningIndicator | `widgets_draw_warning()` | warning_type_t | Full, event |
| 15 | BleStatusWidget | (geplant) | EVT_BLE_* | Partial, event |
| 16 | TourStatsWidget | (geplant) | tour_stats_t | Full, einmalig |
| 17 | TurnByTurnWidget | `widgets_draw_nav_arrow()` | BLE NAV_MANEUVER | Partial, event |
| 18 | WeatherWidget | (geplant) | BLE WX_CURRENT | Full, 15 min |

### C. 7-Segment-Font-Referenz

Der 7-Segment-Font (`seg_digits[10][7]`) ist in `widgets.c:36-47`
definiert. Jede Ziffer ist ein Array von 7 Zeilen mit 5-Bit-Mustern:

```
Segment-Layout:
 aaa
f   b
f   b
 ggg
e   c
e   c
 ddd

Bit-Reihenfolge pro Zeile (MSB first): 4 3 2 1 0
```

Beispiel Ziffer "0" (alle Segmente ausser g):
```
0x1F = 11111  (aaaaa)
0x11 = 10001  (f   b)
0x11 = 10001  (f   b)
0x11 = 10001  (f   b)
0x11 = 10001  (f   b)
0x11 = 10001  (f   b)
0x1F = 11111  (ddddd)
```

### D. 5x7-Bitmap-Font-Referenz

Der 5x7-Font (`font5x7[96][5]`) ist in `widgets.c:50-118` definiert.
Er deckt ASCII 32-127 ab (Space bis DEL). Jedes Zeichen ist 5 Spalten
breit, 7 Zeilen hoch, 1 Byte pro Spalte (LSB = oberste Zeile).

**Zeichenbreite:** 5 Pixel + 1 Pixel Spacing = 6 Pixel pro Zeichen.
Bei Scale 2: 12 Pixel, Scale 3: 18 Pixel, Scale 4: 24 Pixel.

---

*Dieses Dokument ist die Referenz fuer das E-Paper-Widget-System.
Bei Aenderungen an Widgets muss diese Datei aktualisiert werden.
Code-Aenderungen muessen mit der Implementierung in
`firmware/components/display/` uebereinstimmen.*

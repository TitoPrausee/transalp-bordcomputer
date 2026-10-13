# PD06 Sensoren — Fahrzeugdaten direkt abgreifen

> Deep Dive: Honda XL600V Transalp (PD06), Baujahr 1986–2000.
> Rein mechanische Maschine: kein CAN-Bus, kein OBD-II, keine ECU im modernen Sinn.
> Zündung: CDI (Kondensatorzündung), Vergaser (kein EFI).
> Ziel: Jeder Sensor mit konkreter Schaltung, Signalcharakteristik und ESP32-Anbindung.

## Fahrzeugdaten

| Eigenschaft | Wert |
|---|---|
| Motor | 583cc V-Twin, 52°, flüssigkeitsgekühlt, 4-Takt, SOHC, 3 Ventile/Zyl |
| Leistung | 37 kW (50 PS) @ 8000 rpm |
| Drehmoment | 52 Nm @ 6000 rpm |
| Getriebe | 5-Gang |
| Zündung | CDI (Capacitive Discharge Ignition), doppelt (wasted spark) |
| Generator | 310 W @ 5000 rpm (H/J 1987-88), 350 W @ 5000 rpm (K-P 1989-93), 310 W @ 5000 rpm (R-X 1994-99) |
| Regler/Gleichrichter | 13.5–15.5 V @ 5000 rpm, SCR-Typ, 25A, 3-Phase |
| Batterie | 12V / 12Ah |
| Tank | 18 Liter |
| Öldruck | 4.5 bar @ 6000 rpm (warm, 80°C Öltemp) |
| Übersetzungen | I=2.500, II=1.722, III=1.333, IV=1.111, V=0.961 (ab Modell T) |
| Tachoantrieb | Mechanisch, Bowdenzug vom Vorderrad |

Quellen: [motorcyclespecs.co.za XL600V 1988](https://motorcyclespecs.co.za/model/Honda/honda_xl600v_88.htm), [motoinst.ru Africa Twin/Transalp specs](https://motoinst.ru/en/Honda/Africa-Twin/XRV750/electric/equipment/115153), [bikeswiki.com XL600V](https://bikeswiki.com/Honda_XL600V_Transalp), [Honda Service Manual XL600/650V](https://www.moto4.ru/downloads/Service_Repare_Manual_Honda_XL600_650V_Transalp_XRV750_AfricaTwin_87_07_st_font_fin.pdf)

---

## 1. Drehzahl (RPM)

### 1.1 CDI-Zündung der PD06 — Funktionsweise

Die PD06 nutzt eine **CDI (Capacitive Discharge Ignition)**. Das Prinzip:

1. Die **Ladespule** (Teil der Lichtmaschine/Stator) erzeugt AC-Spannung, die über eine Diode gleichgerichtet wird und einen Kondensator (~0.47–2µF) auf lädt (typ. 210–240V am Ladekondensator).
2. Der **Impulsgeber** (Pickup-Spule/Pulse Generator) am Rotor erzeugt ein Signal, das Drehzahl und OT-Position (oberer Totpunkt) enthält.
3. Ein **ICC** (Ignition Control Circuit, Spezial-IC) vergleicht zwei Spannungsrampen: eine feste Referenzrampe und eine drehzahlabhängige Rampe. Am Schnittpunkt beider wird der Thyristor (SCR) gezündet.
4. Der Thyristor entlädt den Kondensator über die Primärwicklung der Zündspule → Hochspannung → Zündfunke.

Die PD06 hat **zwei Zündspulen** (V-Twin, ein Zylinder pro Spule) und läuft im **wasted-spark**-Modus: jeder Zylinder bekommt pro Umdrehung einen Funken, aber nur jeder zweite ist relevant (Kompressionstakt). Der pickup-seitige Vorgang erzeugt pro Umdrehung einen Trigger.

Die Frühzündung ist drehzahlabhängig: bei ~1300 rpm ca. 8° vor OT, bei ~4000 rpm ca. 28° vor OT.

Quellen: [rolf-grauer.de CDI XL600 Analyse](https://www.rolf-grauer.de/cdixl600.pdf), [scribd.com Honda XL600 CDI Wiring](https://www.scribd.com/doc/56944238/Cdi-Xl600-en), [cdi-shop.de Transalp CDI](https://cdi-shop.de/en/shop/honda-transalp-xl600v-pd06/cdi100-d), [NXp AN4006 CDI Reference Design](https://www.nxp.com/docs/en/application-note/AN4006.pdf)

### 1.2 Pickup-Sensor — Signalcharakteristik

Der Pickup-Sensor (Impulsgeber) ist eine kleine Induktivspule, die am Rotor positioniert ist. Ein Magnet auf dem Rotor (oder eine Zacke/»Cam«) passiert die Spule und induziert einen Spannungsimpuls:

- Beim Annähern: **positiver Spannungsimpuls** (Anstieg)
- Wenn Zacke und Spule genau gegenüberstehen: **Nulldurchgang**
- Beim Entfernen: **negativer Spannungsimpuls** (Abfall)

Am OT-Referenzpunkt liegt der Peak des negativen Impulses.

**Spannung und Frequenz:**

| Drehzahl | Peak-to-Peak Spannung | Frequenz |
|---|---|---|
| ~1300 rpm (Leerlauf) | ~28 V pp | ~21.7 Hz |
| ~4400 rpm | ~45 V pp | ~73 Hz |
| ~9500 rpm (Redline) | ~60+ V pp | ~158 Hz |

Die Impulsspannung steigt mit der Drehzahl (induktiver Sensor = AC-Generator). Der Nulldurchgang ist der Referenzpunkt für die CDI.

⚠️ Ein AC-Voltmeter zeigt nur ~2V bei Leerlauf, weil es Durchschnittswerte von kurzen Impulsen mit viel Pause dazwischen misst. Ein Oszilloskop ist Pflicht.

Quellen: [drriders.com Ignition Pulse Measurements](https://drriders.com/ignition-pulse-measurements-t7955.html), [NXp AN4006](https://www.nxp.com/docs/en/application-note/AN4006.pdf)

### 1.3 Signal abgreifen ohne CDI zu stören

⚠️ **Sicherheitskritischer Eingriff.** Fehler hier können zu Aussetzern, Fehlzündungen oder Motorstillstand führen.

**Ansatz A: Optokoppler parallel zum Pickup-Signal**

Der Pickup hat zwei Leitungen (Blau/Gelb und Grün/Weiß an der PD06). Wir greifen das Signal **hochohmig** ab:

```
Pickup+ (Blau/Gelb) ──[470kΩ]──┐
                               ├──[Optokoppler LED]
Pickup- (Grün/Weiß) ───────────┘
```

Ein PC817 oder 6N137 Optokoppler mit Vorwiderstand im Bereich 100kΩ–470kΩ. Die Eingangsbelastung muss so gering sein, dass die CDI-Signalform nicht verzerrt wird. Der Widerstand dimensioniert sich aus der maximalen Pick-up-Spannung (~60V pp) und dem Optokoppler-LED-Strom (1–5mA):

```
R = V_peak / I_LED = 30V / 2mA = 15kΩ (Minimum, sichere Seite)
```

In der Praxis ist ein Vorwiderstand von 22kΩ–47kΩ ausreichend, um das Signal zuverlässig zu triggern und die CDI nicht zu belasten.

⚠️ Vor dem endgültigen Einbau muss mit Oszilloskop verifiziert werden, dass die CDI-Signalform am CDI-Eingang unverändert bleibt.

**Ansatz B: Induktivsensor am Zündkabel (CTA / GT101)**

Ein GT101 oder ein eigener Induktivsensor wird außen auf das Zündkabel geklemmt. Dieser erzeugt pro Zündfunken einen Impuls. Vorteil: **komplett galvanisch getrennt**, kein Eingriff in die CDI-Verkabelung. Nachteil: man bekommt einen Funke pro Zündung, muss aber wissen, ob man im Kompressions- oder Ausstoßtakt ist (wasted spark).

```
Zündkabel ──[GT101 Klammsensor]──[Signal-Conditioning]── ESP32 GPIO
```

Das GT101-Signal ist ein kurzer Spannungsimpuls (~1–10V je nach Position). Ein Komparator (z.B. LM393) macht daraus einen sauberen Rechteck.

**Ansatz C: Hall-Sensor am Zündkabel**

Ein Hall-Sensor (z.B. A3144 oder ACS712) misst das Magnetfeld des Zündstroms im Kabel. Funktioniert, ist aber empfindlicher gegenüber Positionierung und EMV.

### 1.4 ESP32: RPM-Messung

Drei Ansätze, empfohlen in dieser Reihenfolge:

**Option 1: PCNT (Pulse Counter) — empfohlen**

Der ESP32 hat 8 Hardware-Pulse-Counter-Units (PCNT). Diese zählen Impulse unabhängig von der CPU, was sie ideal für RPM macht:

```c
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"

pcnt_unit_handle_t rpm_pcnt_unit = NULL;
pcnt_channel_handle_t rpm_pcnt_chan = NULL;

void rpm_pcnt_init(void) {
    pcnt_unit_config_t unit_config = {
        .low_limit = -1,
        .high_limit = 10000,
    };
    pcnt_new_unit(&unit_config, &rpm_pcnt_unit);

    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = GPIO_NUM_36,  // Optokoppler-Ausgang
        .level_gpio_num = -1,
    };
    pcnt_new_channel(rpm_pcnt_unit, &chan_config, &rpm_pcnt_chan);

    pcnt_channel_set_edge_action(rpm_pcnt_chan,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_HOLD);

    // Glitch-Filter: ignoriert Pulse < 1µs
    pcnt_glitch_filter_config_t filter = { .max_glitch_ns = 1000 };
    pcnt_channel_set_glitch_filter(rpm_pcnt_chan, &filter);

    pcnt_unit_enable(rpm_pcnt_unit);
    pcnt_unit_clear_count(rpm_pcnt_unit);
    pcnt_unit_start(rpm_pcnt_unit);
}

float rpm_read(void) {
    int16_t count = 0;
    pcnt_unit_clear_count(rpm_pcnt_unit);
    vTaskDelay(pdMS_TO_TICKS(500)); // 500ms Messfenster
    pcnt_get_counter_value(rpm_pcnt_unit, &count);

    // PD06: 1 Pickup-Puls pro Umdrehung (wasted spark = 1 Trigger/Rev)
    // count = Pulse in 500ms → rpm = count * 2 * 60
    return (float)count * 2.0f * 60.0f;
}
```

Quelle: [ESP-IDF PCNT Doku](https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32/api-reference/peripherals/pcnt.html)

**Option 2: mcpwm Input Capture**

Der ESP32 MCPWM-Peripheral kann Signal-Frequenzen direkt messen (Input-Capture). Besser für niedrige Frequenzen, da er Zeitstempel nimmt statt zu zählen. Geeigneter für Langsamlauf (Anlassen), aber komplexer einzurichten als PCNT.

**Option 3: GPIO Interrupt**

Einfachster Ansatz, aber CPU-lastig und anfällig für Bounce. Nur für Prototyping geeignet.

```c
volatile uint32_t rpm_pulse_count = 0;

void IRAM_ATTR rpm_isr_handler(void *arg) {
    rpm_pulse_count++;
}
```

### 1.5 RPM-Berechnung

Die PD06 hat **1 Pickup-Trigger pro Kurbelwellenumdrehung** (der Rotor hat eine Referenzzacke). Die RPM-Formel:

```
RPM = (Pulse im Messfenster) × 60 / (Messfenster in Sekunden)
```

Bei 500ms Messfenster:
```
RPM = count × 120
```

Bei 1s Messfenster:
```
RPM = count × 60
```

Kompromiss: 500ms gibt aktualisierte Werte alle 0.5s, bei 9000rpm = 150 Pulse pro Fenster → Auflösung ±2 RPM. Gut genug.

### 1.6 Signal-Glättung und Störschutz

Zündfunken erzeugen **massive EMV** (elektromagnetische Interferenzen). Maßnahmen:

1. **Optokoppler** zwingend — galvanische Trennung zwischen Zündkreis und ESP32
2. **Twisted-Pair** Kabel vom Pickup zum Optokoppler
3. **PCNT Glitch-Filter** auf 1µs (ignoriert Noise-Spikes)
4. **Software-Median-Filter**: 5 Werte nehmen, Median bilden
5. **Spannungsversorgung** des ESP32 über separaten DC/DC-Wandler mit LC-Filter
6. **Abstand**: Zündspulen und Hochspannungskabel so weit wie möglich vom ESP32

```c
// Median-Filter für RPM
#define RPM_BUF_SIZE 5
static float rpm_buffer[RPM_BUF_SIZE] = {0};
static int rpm_buf_idx = 0;

float rpm_filter(float new_rpm) {
    rpm_buffer[rpm_buf_idx] = new_rpm;
    rpm_buf_idx = (rpm_buf_idx + 1) % RPM_BUF_SIZE;

    float sorted[RPM_BUF_SIZE];
    memcpy(sorted, rpm_buffer, sizeof(rpm_buffer));
    // Bubble-Sort für 5 Elemente reicht
    for (int i = 0; i < RPM_BUF_SIZE - 1; i++)
        for (int j = 0; j < RPM_BUF_SIZE - i - 1; j++)
            if (sorted[j] > sorted[j+1]) {
                float tmp = sorted[j];
                sorted[j] = sorted[j+1];
                sorted[j+1] = tmp;
            }
    return sorted[RPM_BUF_SIZE / 2]; // Median
}
```

---

## 2. Geschwindigkeit (Tacho)

### 2.1 Situationsanalyse

Die PD06 hat eine **mechanische Tachowelle** (Bowdenzug-Antrieb am Vorderrad). Es gibt kein elektronisches Geschwindigkeitssignal ab Werk.

Der Standard-Antrieb bei japanischen Motorrädern ist **2240:60** — das heißt, die Tachowelle dreht sich bei 60 mph (96.6 km/h) mit 2240 Umdrehungen pro Minute. Bei km/h entspricht das ca. **2322 Rev/km** (2240 / 0.966 = 2322.9).

Quelle: [reddit.com/r/motorcycles Aftermarket Speedometer](https://reddit.com/r/motorcycles/comments/1xldes/aftermarket_speedometer_help)

### 2.2 Option A: Reed-Kontakt am Tachoantrieb

Ein Reed-Kontakt (magnetisch betätigter Schalter) wird am Tachoantrieb positioniert. Ein kleiner Magnet auf der Tachowelle oder am Getriebeausgang des Tachos aktiviert den Reed bei jeder Umdrehung.

**Vorteile:** Sehr einfach, minimal invasiv (keine mechanische Änderung am Rad)
**Nachteile:** Magnet muss passend angebracht werden, mechanische Belastung des Tachos

Schaltung:

```
3.3V ──[10kΩ Pull-Up]──┬──[Reed-Kontakt]── GND
                        │
                        └── ESP32 GPIO (mit internem Pull-Up)
```

Ein PCNT-Unit zählt die Pulse. Bei 60 km/h dreht die Welle mit ~2322 Rev/km → bei 60km/h (1 km/min) = 2322/60 ≈ 38.7 Pulse/s. Bei 60 km/h mit 1 Puls/Revolution = ~39 Hz.

Geschwindigkeit:
```
speed_kmh = (pulses × 60) / (2322 × messzeit_s)
```

Beispiel: 39 Pulse in 1s → (39 × 60) / 2322 = 1.007 km/h? Nein, das wäre bei 1 km/h. Richtig:
```
speed_kmh = (pulses / messzeit_s) × (3600 / 2322) = pulses/s × 1.5504
```

Bei 39 Pulse/s → 60.5 km/h. Korrekt.

### 2.3 Option B: GPS-Speed (NEO-M9N) — empfohlen für Touren-Logging

Der u-blox NEO-M9N liefert hochpräzise Geschwindigkeit über GNSS (GPS + GLONASS + Galileo + BeiDou).

**Specs:**
- Geschwindigkeitsgenauigkeit: 0.05 m/s (0.18 km/h)
- Update-Rate: bis 25 Hz
- Positionsgenauigkeit: 2.0 m CEP
- Schnittstelle: UART, I2C, SPI
- Versorgung: 3.3V

Quellen: [u-blox NEO-M9N DataSheet](https://content.u-blox.com/sites/default/files/NEO-M9N-00B_DataSheet_UBX-19014285.pdf), [u-blox NEO-M9N Product Summary](https://content.u-blox.com/sites/default/files/NEO-M9N_ProductSummary_UBX-19027207.pdf)

**Vorteile:** Kein mechanischer Eingriff, präzise, liefert auch Position/Track-Logging
**Nachteile:** Signal bei Tunnel/Baumallee möglich, <1s Fix beim Start, benötigt Antenne mit Sicht nach oben

ESP32-Code (NMEA über UART):

```c
#include "driver/uart.h"
#include "minmea.h"

#define GPS_UART UART_NUM_1
#define GPS_TX_PIN 17  // nicht benötigt wenn nur Empfang
#define GPS_RX_PIN 16  // NEO-M9N TX → ESP32 RX
#define BUF_SIZE 1024

void gps_init(void) {
    uart_config_t cfg = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_NONE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(GPS_UART, &cfg);
    uart_set_pin(GPS_UART, GPS_TX_PIN, GPS_RX_PIN, -1, -1);
    uart_driver_install(GPS_UART, BUF_SIZE * 2, 0, 0, NULL, 0);
}

float gps_read_speed_kmh(void) {
    char line[128];
    int len = uart_read_bytes(GPS_UART, (uint8_t*)line, sizeof(line)-1, pdMS_TO_TICKS(100));
    if (len <= 0) return -1.0f;
    line[len] = '\0';

    if (minmea_sentence_id(line) == MINMEA_SENTENCE_RMC) {
        struct minmea_sentence_rmc frame;
        if (minmea_parse_rmc(&frame, line)) {
            // speed in knots → km/h
            return minmea_tofloat(&frame.speed) * 1.852f;
        }
    }
    return -1.0f;
}
```

### 2.4 Option C: Hall-Sensor am Vorderrad

Ein Magnet (z.B. Neodym 5mm) wird an der Bremsscheibe oder Speiche angebracht, ein Hall-Sensor (A3144, TLE4905L) an der Gabel.

**Vorteile:** Sehr robust, wasserdicht, bewährte Methode (Fahrradtacho-Prinzip)
**Nachteile:** Montageaufwand, 1 Puls pro Radumdrehung = grobe Auflösung bei niedriger Geschwindigkeit

Bei einem 19" Vorderrad (Transalp typisch 110/80R19) ist der Abrollumfang ca. 2.15m. Bei 1 Puls/Umdrehung:

```
speed_kmh = (pulses/s) × 2.15 × 3.6 = pulses/s × 7.74
```

Bei 60 km/h → 7.75 Pulse/s. Bei 1s Messfenster → ±0.46 km/h Auflösung. Okay.

Verbesserung: 4 Magnete → 4 Pulse/Rev → 4× bessere Auflösung.

### 2.5 Genauigkeitsvergleich

| Methode | Genauigkeit | Update-Rate | Montageaufwand | EMV-Anfällig |
|---|---|---|---|---|
| Reed am Tacho | ±2 km/h | 1s | Mittel | Nein |
| GPS NEO-M9N | ±0.2 km/h | 5–25 Hz | Gering (nur Antenne) | Ja (Tunnel) |
| Hall am Rad (1 Magnet) | ±0.5 km/h | 1s | Hoch | Nein |
| Hall am Rad (4 Magnete) | ±0.1 km/h | 0.25s | Hoch | Nein |

**Empfehlung:** GPS NEO-M9N als primäre Quelle (genau, kein Eingriff), Hall-Sensor als Backup/Plausibilitätscheck.

---

## 3. Kühlmitteltemperatur

### 3.1 PD06 Bestand

Die PD06 hat zwei temperaturbezogene Komponenten:

1. **Thermoschalter** (am Thermostatgehäuse): schaltet den Kühlerventilator bei ~103°C ein. Reiner ON/OFF-Schalter.
2. **Temperaturfühler für Tankuhr** (falls vorhanden): analoger Geber am Thermostatgehäuse. Liefert Widerstandswert für die Temperaturanzeige im Tacho.

Quelle: [dirtymotorcycleadventures.com Transalp Coolant System](https://www.dirtymotorcycleadventures.com/2011/looking-over-the-coolant-system-on-the-transalp/), [xrv.org.uk Radiator Fan Problems](https://xrv.org.uk/threads/radiator-fan-problems.170473)

Die Tankuhr der PD06 ist relativ simpel. Der Sensor im Thermostatgehäuse hat einen Widerstand, der mit steigender Temperatur abnimmt (NTC-Verhalten).

### 3.2 Sensor-Nachrüstung

⚠️ Kühlmitteleingriffe erfordern Sorgfalt — Kühlverlust, Lufteinschlüsse, falsches Kühlmittelgemisch können Motorschäden verursachen.

**Option A: Inlinesensor im Kühlerschlauch**

Ein Temperatursensor mit Schlauchanschluss (Inline-Fitting, z.B. 1/8 NPT oder M10) wird in den oberen Kühl Schlauch zwischen Thermostat und Kühler gesetzt. Messpunkt ist direkt im heißen Kühlmittelstrom.

**Option B: Adapter am Thermostatgehäuse**

Ein T-Stück oder Adaptergewinde am Thermostatgehäuse. Vorteil: misst dort, wo Honda auch misst.

**Option C: Abgriff des vorhandenen Sensors**

Falls die Tankuhr bereits einen analogen Sensor hat, kann dessen Widerstand hochohmig gemessen werden. Die Tankuhr darf nicht beeinträchtigt werden → Buffer-Verstärker (Operationsverstärker als Spannungsfolger) verwenden.

### 3.3 Sensorvergleich

| Sensor | Typ | Bereich | Genauigkeit | Preis |
|---|---|---|---|---|
| NTC 10kΩ (B3950) | Thermistor | -40 bis +150°C | ±0.5°C | ~1€ |
| PT1000 | RTD (PTC) | -200 bis +850°C | ±0.1°C | ~5–15€ |
| KTY81-210 | Silizium-PTC | -55 bis +150°C | ±2°C | ~2€ |
| DS18B20 | Digital | -55 bis +125°C | ±0.5°C | ~2–5€ |

**Empfehlung: NTC 10kΩ B3950**

Vorteile: billig, robust, ausreichend genau (±0.5°C), simple Schaltung, automotive-tauglich im geschlossenen Gehäuse.

### 3.4 Schaltung: Spannungsteiler → ADC

```
3.3V ──[10kΩ Referenz]──┬── ADC Pin (GPIO34, ADC1_CH6)
                        │
                    [NTC 10kΩ]
                        │
                       GND
```

Der NTC hängt nach Masse, der Referenzwiderstand nach 3.3V. Die Spannung am ADC-Pin steigt mit sinkender Temperatur (NTC-Widerstand steigt → höhere Spannung am Teilerpunkt).

⚠️ Verwende **ADC1** Pins (GPIO32–39), nicht ADC2 — ADC2 ist deaktiviert, sobald WiFi aktiv ist.

### 3.5 ADC-Kalibrierung mit esp_adc_cal

Der ESP32 ADC hat eine nominelle Referenzspannung von 1100 mV, aber die tatsächliche variiert zwischen 1000 und 1200 mV. Die `esp_adc_cal` Bibliothek nutzt factory-calibrierte eFuse-Werte, um das zu kompensieren.

Quelle: [ESP-IDF ADC Calibration](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/adc_calibration.html)

```c
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_cali.h"

adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc_cali_handle;

void temp_adc_init(void) {
    // ADC1 Init
    adc_oneshot_unit_init_cfg_t init_cfg = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&init_cfg, &adc1_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_11,    // bis ~3.1V
        .bitwidth = ADC_BITWIDTH_12, // 12-bit: 0–4095
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &chan_cfg); // GPIO34

    // Kalibrierung
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
        .default_vref = 1100, // Fallback falls eFuse nicht gebrannt
    };
    adc_cali_create_scheme_line_fitting(&cali_cfg, &adc_cali_handle);
}

float temp_read_celsius(void) {
    int raw = 0;
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &raw);

    int voltage_mv = 0;
    adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage_mv);

    // Spannungsteiler: V = 3.3V × R_ntc / (R_ref + R_ntc)
    // → R_ntc = R_ref × V / (Vcc - V)
    float v = voltage_mv / 1000.0f;
    float r_ref = 10000.0f; // 10kΩ
    float r_ntc = r_ref * v / (3.3f - v);

    // Steinhart-Hart für NTC 10k B3950
    // 1/T = 1/T0 + (1/B) × ln(R/R0)
    float t0 = 298.15f;    // 25°C in Kelvin
    float r0 = 10000.0f;  // 10kΩ @ 25°C
    float b = 3950.0f;

    float temp_k = 1.0f / (1.0f/t0 + (1.0f/b) * logf(r_ntc / r0));
    return temp_k - 273.15f; // Celsius
}
```

### 3.6 Mittelwertbildung und Linearisierung

Der ESP32 ADC ist rauscharm, aber bei hohen Werten (nahe 4095) nicht-linear. Maßnahmen:

1. **Multisampling**: 16 Samples nehmen, Mittelwert bilden
2. **100nF Keramikkondensator** am ADC-Pin gegen GND (Bypass)
3. **Software-Glättung**: exponentieller gleitender Mittelwert

```c
float temp_smoothed = 0;
float temp_alpha = 0.1f; // Glättungsfaktor

float temp_read_filtered(void) {
    // 16× oversampling
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) {
        int raw;
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &raw);
        sum += raw;
        vTaskDelay(1);
    }
    int avg_raw = sum / 16;

    int voltage_mv;
    adc_cali_raw_to_voltage(adc_cali_handle, avg_raw, &voltage_mv);

    // ... (Umwandlung wie oben)

    temp_smoothed = temp_alpha * new_temp + (1 - temp_alpha) * temp_smoothed;
    return temp_smoothed;
}
```

### 3.7 Referenzwerte

| Zustand | Erwartete Temperatur |
|---|---|
| Kaltstart (20°C Umgebung) | 20–30°C |
| Betriebstemperatur | 85–105°C |
| Ventilator ein | ~103°C (Thermoschalter) |
| Warnbereich | >115°C |
| Kritisch | >120°C |

Quellen: [xrv.org.uk Radiator Fan](https://xrv.org.uk/threads/radiator-fan-problems.170473), [unimotors.ro Temp Sensor](https://unimotors.ro/en/Temperature-sensor-Honda-Africa-Twin-CB-CBF-Pantheon-Transalp-Shadow-RMS_p_133678.html)

---

## 4. Kraftstoffstand

### 4.1 PD06 Tankgeber

Die PD06 hat einen konventionellen Tankgeber mit Schwimmer und Potentiometer (variabler Widerstand). Honda verwendet typischerweise Widerstandsbereiche im Stil japanischer Motorräder.

Es gibt keine öffentliche Honda-Doku mit exakten Widerstandswerten für die PD06. Die gängigen Bereiche für japanische Motorräder dieser Ära sind:

- **10–190Ω** (Europäischer Standard, häufig bei Honda)
- **0–90Ω** (GM-Standard, gelegentlich bei älteren Honda-Modellen)
- **33–240Ω** (Aftermarket-Standard)

⚠️ Der Widerstandsbereich muss am Fahrzeug gemessen werden. Vorgehen:

1. Tank leeren, Stecker am Geber abziehen
2. Widerstand zwischen Geber-Pin und Masse messen (Tank leer)
3. Tank vollmachen, erneut messen (Tank voll)
4. Werte notieren → das ist der individuelle Bereich

Quellen: [AutoMeter Fuel Sender Guide](https://www.autometer.com/blog/understanding-fuel-level-sender-resistance-ranges/), [Summit Racing Help](https://help.summitracing.com/knowledgebase/article/SR-03679/en-us), [androidexperto.com Fuel Sender Ranges](https://androidexperto.com/inverting-resistance-and-range-of-fuel-level-sender)

### 4.2 Abgriff ohne Tankuhr zu stören

Die Tankuhr der PD06 misst den Widerstand des Gebers durch einen Stromkreis. Wenn wir einen ADC parallel schalten, verändern wir den Gesamtwiderstand → die Tankuhr zeigt falsch an.

**Lösung: Operationsverstärker als Spannungsfolger (Buffer)**

Ein OPV mit Verstärkung 1 (Spannungsfolger) hat einen Eingangswiderstand von >1MΩ und belastet den Geber nicht. Ausgang geht zum ESP32 ADC.

Schaltung (Beispiel mit LM358, Single-Supply 3.3V):

```
                    Tankgeber
3.3V ──[1kΩ]──┬──[Variable R]── GND
               │
               └──[OPV LM358 (+)]──[OPV Output]──[2.2kΩ]── ESP32 ADC (GPIO35)

LM358 (-) ── Output (Feedback, Spannungsfolger)
LM358 VCC: 5V (vom Regler, nicht 3.3V — LM358 braucht min 3V Headroom)
```

Wenn der Tankgeber z.B. zwischen 10Ω und 190Ω schwankt, mit einem 1kΩ Referenzwiderstand nach 3.3V:

- Bei 10Ω (leer): V = 3.3 × 10/(1000+10) = 0.033V
- Bei 190Ω (voll): V = 3.3 × 190/(1000+190) = 0.526V

Der Spannungsbereich ist sehr klein (0.033–0.526V). Das nutzt nur 16% des ADC-Bereichs aus. Besser: Vorwiderstand so wählen, dass der Bereich maximal ausgenutzt wird.

Optimaler Referenzwiderstand: R_ref = sqrt(R_min × R_max) = sqrt(10 × 190) = ~43Ω. Damit:

- Bei 10Ω: V = 3.3 × 10/(43+10) = 0.623V
- Bei 190Ω: V = 3.3 × 190/(43+190) = 2.690V

Das nutzt ~63% des ADC-Bereichs aus. Viel besser.

⚠️ Wenn der Tankgeber auf Masse geschaltet ist (eine Seite liegt an GND), muss der Spannungsteiler anders aufgebaut werden: R_ref von 3.3V zum Geber, Geber nach GND, ADC-Pin am Knotenpunkt.

### 4.3 ESP32-Code

```c
#define FUEL_ADC_CHANNEL ADC_CHANNEL_7  // GPIO35

// Kalibrierung: am Fahrzeug messen!
#define FUEL_R_EMPTY 10.0f    // Ω bei leer
#define FUEL_R_FULL  190.0f   // Ω bei voll
#define FUEL_R_REF   43.0f    // Referenzwiderstand

float fuel_read_percent(void) {
    int raw = 0;
    // 16× oversampling
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) {
        adc_oneshot_read(adc1_handle, FUEL_ADC_CHANNEL, &raw);
        sum += raw;
        vTaskDelay(1);
    }
    int avg_raw = sum / 16;

    int voltage_mv;
    adc_cali_raw_to_voltage(adc_cali_handle, avg_raw, &voltage_mv);
    float v = voltage_mv / 1000.0f;

    // R_geber = R_ref × V / (Vcc - V)
    float r_sender = FUEL_R_REF * v / (3.3f - v);

    // Linear mappen (vereinfacht — NTC-artige Kennlinie possible)
    float percent = (r_sender - FUEL_R_EMPTY) / (FUEL_R_FULL - FUEL_R_EMPTY) * 100.0f;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    return percent;
}
```

### 4.4 Kalibrierung gegen Tankstreifen

Die Widerstandskennlinie des Schwimmers ist nicht zwingend linear (Potentiometer-Abgriff am Schwimmerarm). Kalibrierung:

1. Tank komplett leeren → ADC-Wert notieren = 0%
2. Tank in 2-Liter-Schritten vollmachen → jeweils ADC-Wert notieren
3. Tabelle anlegen: Liter → ADC-Wert
4. Im Code: Lookup-Table oder Interpolation

```c
// Kalibrierungstabelle (Beispiel — am Fahrzeug ermitteln!)
typedef struct { float liters; float adc_raw; } fuel_cal_point_t;
static const fuel_cal_point_t fuel_cal[] = {
    { 0.0f,  120 },  // leer
    { 4.0f,  280 },
    { 8.0f,  450 },
    {12.0f,  620 },
    {16.0f,  780 },
    {18.0f,  860 },  // voll
};
#define FUEL_CAL_SIZE (sizeof(fuel_cal)/sizeof(fuel_cal[0]))

float fuel_read_liters(void) {
    int raw = read_avg_adc(FUEL_ADC_CHANNEL, 16);
    // Lineare Interpolation
    if (raw <= fuel_cal[0].adc_raw) return fuel_cal[0].liters;
    if (raw >= fuel_cal[FUEL_CAL_SIZE-1].adc_raw) return fuel_cal[FUEL_CAL_SIZE-1].liters;

    for (int i = 0; i < FUEL_CAL_SIZE - 1; i++) {
        if (raw >= fuel_cal[i].adc_raw && raw <= fuel_cal[i+1].adc_raw) {
            float t = (float)(raw - fuel_cal[i].adc_raw) /
                      (fuel_cal[i+1].adc_raw - fuel_cal[i].adc_raw);
            return fuel_cal[i].liters + t * (fuel_cal[i+1].liters - fuel_cal[i].liters);
        }
    }
    return 0;
}
```

### 4.5 Reichweitenberechnung

Aus Kraftstoffstand (Liter) und mittlerem Verbrauch (L/100km):

```c
float fuel_range_km(float liters, float avg_consumption_l100) {
    if (avg_consumption_l100 <= 0) return 0;
    return (liters / avg_consumption_l100) * 100.0f;
}
```

Mittlerer Verbrauch wird aus Sprit-Mengen und km-Stand bei Tankstopps berechnet (manuelle Eingabe oder aus GPS-Distanz).

### 4.6 Alternative: Ultraschall-Sensor im Tank

Ein Ultraschall-Sensor (z.B. JSN-SR04T, wasserdicht) misst den Abstand zur Kraftstoffoberfläche. Präziser als der Schwimmer, aber:

- Kraftstoffdämpfe können Sensor beeinflussen
- Montage am Tankdeckel oder Einfüllstutzen
- Blast-Schutz (Ex-Schutz) beachten — Kraftstoffdämpfe sind explosiv!

⚠️ Ultraschall im Tank ist experimentell. Der vorhandene Tankgeber + OPV-Buffer ist die sichere, bewährte Variante.

---

## 5. Öldruck

### 5.1 PD06 Öldruckschalter

Die PD06 hat einen einfachen **Öldruckschalter** (ON/OFF). Er öffnet/schließt bei einem Schwellwert von ca. 0.2–0.5 bar. Unterhalb des Schwellwerts schließt er (Warnleuchte an), darüber öffnet er (Warnleuchte aus).

Das ist eine reine Warnfunktion — keine analoge Druckmessung.

**Öldruck-Sollwerte (aus Service Manual):**
- 4.5 bar @ 6000 rpm (Motor warm, Öl 80°C)

Quelle: [Honda Service Manual XL600/650V](https://www.moto4.ru/downloads/Service_Repare_Manual_Honda_XL600_650V_Transalp_XRV750_AfricaTwin_87_07_st_font_fin.pdf)

### 5.2 Auslesen des Schalters

Der Schalter ist ein-ein-poliger Schließer nach Masse. Bei ausgeschaltetem Motor (kein Druck) liegt Masse am Signalpin. Bei laufendem Motor (Druck > Schwellwert) öffnet der Schalter.

```
12V ──[10kΩ Pull-Up]──┬──[Öldruckschalter]── GND
                       │
                       └── Optokoppler → ESP32 GPIO
```

⚠️ Die Motorrad-Masse ist nicht zwingend identisch mit der ESP32-Masse. Ein Optokoppler (PC817) trennt galvanisch.

```c
// Öldruckschalter: LOW = kein Druck (Warnung), HIGH = Druck OK
#define OIL_PRESSURE_GPIO 14  // mit internem Pull-Up

bool oil_pressure_ok(void) {
    return gpio_get_level(OIL_PRESSURE_GPIO) == 1; // Pull-Up: HIGH = Schalter offen = Druck OK
}
```

### 5.3 Aufrüstung auf analogen Öldrucksensor

Für eine echte Druckanzeige wird ein **analoger Sender** nachgerüstet. Bewährt: VDO 0–10 bar, 0.5–4.5V Ausgang, 1/8-27 NPT Gewinde.

| Eigenschaft | Wert |
|---|---|
| Messbereich | 0–10 bar |
| Ausgangsspannung | 0.5–4.5V (ratiometrisch) |
| Versorgung | 5V |
| Gewinde | 1/8-27 NPT |
| Genauigkeit | ±1.5% FS |

⚠️ Der Sensor muss an einen Ölleitungsanschluss. Die PD06 hat den Öldruckschalter am Motorblock — dort kann ein T-Stück eingesetzt werden: ein Anschluss für den Originalschalter (Warnleuchte bleibt funktionsfähig), einer für den analogen Sensor.

Quellen: [Autodoc Oil Pressure Switch](https://www.autodoc.co.uk/car-parts/oil-pressure-switch-sender-unit-valve-10578), [Arduino Forum VDO Oil Pressure](https://forum.arduino.cc/t/vdo-oil-pressure-interfacing-with-arduino/391373)

### 5.4 Schaltung: 5V-Sensor an ESP32 (3.3V ADC)

Der VDO-Sensor liefert 0.5–4.5V. Der ESP32-ADC verträgt max 3.3V. Ein Spannungsteiler skaliert runter:

```
VDO Signal ──[10kΩ]──┬── ESP32 ADC (GPIO33, ADC1_CH5)
                      │
                   [22kΩ]
                      │
                     GND
```

Spannungsteiler-Verhältnis: 22/(10+22) = 0.6875

- Bei 0.5V (0 bar): 0.344V → ADC ~428
- Bei 4.5V (10 bar): 3.094V → ADC ~3847 (nah an Maximum, aber sicher)

Besser: 12kΩ und 22kΩ → Verhältnis 22/34 = 0.647 → 0.324V bis 2.912V. Etwas mehr Sicherheitsmargin.

### 5.5 ESP32-Code

```c
#define OIL_PRESSURE_CHANNEL ADC_CHANNEL_5  // GPIO33

float oil_pressure_read_bar(void) {
    int raw;
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) {
        adc_oneshot_read(adc1_handle, OIL_PRESSURE_CHANNEL, &raw);
        sum += raw;
        vTaskDelay(1);
    }
    int avg_raw = sum / 16;

    int voltage_mv;
    adc_cali_raw_to_voltage(adc_cali_handle, avg_raw, &voltage_mv);

    // Spannungsteiler rückrechnen: V_sensor = V_adc × (R1+R2)/R2
    float v_divider_ratio = (10.0f + 22.0f) / 22.0f;  // 1.4545
    float v_sensor = (voltage_mv / 1000.0f) * v_divider_ratio;

    // VDO Kennlinie: 0.5V = 0 bar, 4.5V = 10 bar
    float pressure = ((v_sensor - 0.5f) / 4.0f) * 10.0f;
    if (pressure < 0) pressure = 0;
    return pressure;
}
```

### 5.6 Warn-Schwellen

| Zustand | Öldruck | Aktion |
|---|---|---|
| Leerlauf warm | 0.5–1.5 bar | Normal |
| Fahrbetrieb 6000 rpm warm | 4.5 bar | Normal (Service Manual) |
| Unter 0.5 bar bei laufendem Motor | <0.5 bar | ⚠️ WARNUNG — Motor stoppen! |
| Über 6 bar | >6 bar | ⚠️ Überdruck — Ölpumpe/Filter prüfen |

Quelle: [Honda Service Manual](https://www.moto4.ru/downloads/Service_Repare_Manual_Honda_XL600_650V_Transalp_XRV750_AfricaTwin_87_07_st_font_fin.pdf), [preOBD Oil Pressure Thresholds](https://github.com/preobd/preOBD/blob/main/docs/README.md)

---

## 6. Ganganzeige

### 6.1 Situation

Die PD06 hat **keine Ganganzeige** ab Werk. 5-Gang-Getriebe ohne Sensor.

### 6.2 Option A: Schaltwalzen-Positionssensor

Die Schaltwalze im Getriebe hat diskrete Positionen pro Gang. Ein Positionssensor (Potentiometer oder Hall-Sensor-Array) an der Schaltwalze kann die Position erfassen.

**Vorteile:** Sehr genau, direkte Messung
**Nachteile:** Mechanischer Eingriff ins Getriebegehäuse, Dichtigkeit, Custom-Montage

⚠️ Eingriffe ins Getriebegehäuse sind sicherheitskritisch. Beschädigungen können zu Ölverlust oder Getriebeschaden führen.

### 6.3 Option B: Mikroschalter an Schaltwelle

Ein Schalter pro Gangposition, mechanisch über Nocken oder Hebel betätigt. Fünf Schalter für fünf Gänge + einer für Leerlauf.

**Vorteile:** Sehr robust, digitaler Input
**Nachteile:** Montageaufwand, Verschleiß, kalibriert werden muss die Mechanik

```c
// 5 GPIOs für Gänge 1–5, 1 für Leerlauf
#define GEAR1_GPIO 25
#define GEAR2_GPIO 26
#define GEAR3_GPIO 27
#define GEAR4_GPIO 32
#define GEAR5_GPIO 33
#define NEUTRAL_GPIO 4

int gear_read(void) {
    if (gpio_get_level(NEUTRAL_GPIO) == 0) return 0; // Leerlauf
    if (gpio_get_level(GEAR1_GPIO) == 0) return 1;
    if (gpio_get_level(GEAR2_GPIO) == 0) return 2;
    if (gpio_get_level(GEAR3_GPIO) == 0) return 3;
    if (gpio_get_level(GEAR4_GPIO) == 0) return 4;
    if (gpio_get_level(GEAR5_GPIO) == 0) return 5;
    return -1; // Fehler
}
```

### 6.4 Option C: Berechnung aus Drehzahl × Geschwindigkeit — empfohlen

Wenn wir RPM und Geschwindigkeit haben, lässt sich der Gang rechnerisch bestimmen. Prinzip: für jeden Gang gibt es ein festes Übersetzungsverhältnis zwischen Raddrehzahl und Motordrehzahl.

**Übersetzungskette:**
- Primärübersetzung (Kurbelwelle → Kupplung): typisch ~2.5–3.0 bei der PD06
- Gangübersetzung (aus Service Manual): I=2.500, II=1.722, III=1.333, IV=1.111, V=0.961
- Sekundärübersetzung (Kettenantrieb): Kettenrad vorn/hinten, typisch 15/43 bei PD06
- Abrollumfang Reifen: ~2.15m bei 110/80R19

Gesamtübersetzung pro Gang = Primär × Gang × Sekundär

Die Motordrehzahl pro km/h ergibt sich aus:
```
RPM = speed_kmh × Gesamtübersetzung × 1000 / (60 × Abrollumfang)
```

```c
// PD06 Übersetzungen (ab Modell T)
// Primär: ~2.571, Sekundär: 15/43 = 2.867
// Abrollumfang: 2.15m
#define PRIMARY_RATIO 2.571f
#define FINAL_RATIO   2.867f
#define WHEEL_CIRCUM  2.15f  // Meter

static const float gear_ratios[5] = {2.500f, 1.722f, 1.333f, 1.111f, 0.961f};

int gear_detect(float rpm, float speed_kmh) {
    if (speed_kmh < 5.0f || rpm < 800.0f) return 0; // Leerlauf/Stand

    float best_match = 999.0f;
    int best_gear = 0;

    for (int g = 0; g < 5; g++) {
        float total = PRIMARY_RATIO * gear_ratios[g] * FINAL_RATIO;
        float expected_rpm = speed_kmh * total * 1000.0f / (60.0f * WHEEL_CIRCUM);
        float diff = fabsf(rpm - expected_rpm) / expected_rpm; // relative Abweichung

        if (diff < best_match) {
            best_match = diff;
            best_gear = g + 1;
        }
    }
    // Nur wenn Abweichung < 10% — sonst Leerlauf
    if (best_match > 0.10f) return 0;
    return best_gear;
}
```

**Vorteile:** Kein mechanischer Eingriff, nutzt vorhandene Sensoren (RPM + Speed)
**Nachteile:** Ungenau bei Kupplung getreten, Schaltvorgang, rollout. Braucht Sekunden nach Schaltvorgang zur Stabilisierung.

### 6.5 Option D: IMU-basierte Schalt-Erkennung

Ein IMU (MPU6050, BMX055) erkennt die Längsbeschleunigung. Beim Schalten gibt es eine kurze Pause in der Beschleunigung (Kupplung). Das ist aber nicht gangspezifisch.

**Vorteile:** Zusätzliche Daten (Neigung, Beschleunigung) für Bremssysteme und Fahrdynamik
**Nachteile:** Kann den Gang nicht direkt identifizieren, nur Schaltvorgänge

### 6.6 Leerlaufschalter

Die PD06 hat einen **Leerlaufschalter** am Getriebe (schaltet bei Leerlauf nach Masse). Dieser ist einfach auszulesen:

```c
#define NEUTRAL_GPIO 4  // mit Pull-Up
bool is_neutral(void) {
    return gpio_get_level(NEUTRAL_GPIO) == 0; // LOW = Leerlauf aktiv
}
```

**Empfehlung:** Option C (Berechnung) als primäre Methode, Option B (Mikroschalter) für maximale Zuverlässigkeit falls gewünscht, Leerlaufschalter immer aktiv.

---

## 7. Spannungsversorgung / Lichtmaschine

### 7.1 PD06 Lichtmaschine

Die PD06 hat eine Dreiphasen-Wechselstromlichtmaschine (stator) mit externem Regler/Gleichrichter.

**Leistung nach Baujahr:**

| Modelljahr | Leistung |
|---|---|
| XL600V H/J (1987–1988) | 310 W @ 5000 rpm |
| XL600V K–P (1989–1993) | 350 W @ 5000 rpm |
| XL600V R–X (1994–1999) | 310 W @ 5000 rpm |

**Regler/Gleichrichter:**
- Typ: SCR (Thyristor), dreiphasig
- Ausgang: 13.5–15.5 V @ 5000 rpm
- Maximalstrom: 25A
- OEM-Teilenummer: 31600-MS6-921 / 31600-MM9-000

Quellen: [motoinst.ru Transalp Specs](https://motoinst.ru/en/Honda/Africa-Twin/XRV750/electric/equipment/115153), [motorcyclespecs.co.za XL600V](https://motorcyclespecs.co.za/model/Honda/honda_xl600v_88.htm), [oro2u Regulator](https://oro2u.com/regulator-xlv600-transalp-31600-ms6-921-aha6050-road-dze2054)

### 7.2 Verfügbarer Strom für Bord-Elektronik

Bei 310W und 13.5–15.5V Batteriespannung fließen max ~20–23A. Abzüglich:

| Verbraucher | Strom (12V) |
|---|---|
| Abblendlicht | ~4.5A (55W H4) |
| Bremslicht | ~0.5A (6W) |
| Blinker (pro Seite) | ~1.7A (2× 10W) |
| Kennzeichenbeleuchtung | ~0.3A |
| Zündung (CDI) | ~2A (von Ladespule, nicht Batterie) |
| **Summe Standard** | ~7A |

**Verfügbar für Bordcomputer:** 310W/14V = ~22A, minus 7A Standard = **~15A** (180W) freie Kapazität.

Das ist reichlich für einen ESP32 + Sensoren + Display + GPS:
- ESP32: ~0.2–0.5A (je nach WiFi/BLE-Last)
- GPS NEO-M9N: ~0.1A
- Display (ILI9341): ~0.2A
- Sensoren gesamt: ~0.2A
- **Gesamt Bordcomputer: ~0.7A** (8.4W)

Selbst mit 350W-Generator bleibt massiv Reserve.

### 7.3 DC/DC-Wandler

Der Regler liefert 13.5–15.5V an die Batterie. Für den ESP32 brauchen wir stabile 3.3V bzw. 5V.

**Schema:**

```
Batterie (12V) ──[Sicherung 5A]──[DC/DC Step-Down LM2596]── 5V ──[AMS1117 LDO]── 3.3V
```

- **LM2596** (oder MP1584): Step-Down von 12V auf 5V, 3A, Schaltregler (hoher Wirkungsgrad ~85%)
- **AMS1117-3.3**: LDO von 5V auf 3.3V, linear (low noise für ADC)
- **Sicherung**: 5A flink, nah an der Batterie

⚠️ Direkt am Regler abzapfen ist nicht empfohlen — der Regler regelt die Ausgangsspannung der Batterie nach. Abgriff immer an der Batterie (gesichert).

### 7.4 Bordspannung als Datenpunkt

Die Bordspannung ist ein nützlicher Datenpunkt:

- **14.0–15.0V**: Generator läuft, Batterie wird geladen
- **12.5–13.8V**: Batterie entlädt sich (Generator zu schwach oder Last zu hoch)
- **<12.0V**: Batterie tief, Gefahr der Tiefenentladung
- **>15.5V**: Regler defekt — Überladung, Batterie gasst aus

```c
// Spannungsteiler: 12V → 3.3V ADC
// R1=100kΩ (nach Batterie+), R2=33kΩ (nach GND)
// Verhältnis: 33/(100+33) = 0.248 → 15V → 3.72V (zu hoch!)
// Besser: R1=120kΩ, R2=33kΩ → 0.216 → 15V → 3.24V (passt)
#define VBAT_DIVIDER_RATIO ((120.0f + 33.0f) / 33.0f)  // 4.636

float battery_voltage(void) {
    int raw;
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) {
        adc_oneshot_read(adc1_handle, VBAT_CHANNEL, &raw);
        sum += raw;
    }
    int avg = sum / 16;
    int mv;
    adc_cali_raw_to_voltage(adc_cali_handle, avg, &mv);
    return (mv / 1000.0f) * VBAT_DIVIDER_RATIO;
}
```

---

## 8. Blinker / Licht / Hupe (Status lesen)

### 8.1 Sinnhaftigkeit

Status-Signale sind für einen Bordcomputer nützlich für:

- **Turn-by-Turn-Navigation**: Blinker-Status als Trigger für Richtungswechsel
- **Bremslicht-Status**: Log-Eintrag bei Bremsung (Unfalldaten-Schreiber)
- **Fernlicht-Status**: Touren-Logging (wann wurde Fernlicht benutzt)
- **Hupe**: (optional, eher spielerisch)

### 8.2 Signale und Spannungen

Alle Signale sind 12V-Gleichstrom (Batteriespannung). Sie sind nicht gepulst, außer Blinkern (getaktet durch Blinkgeber, typisch 1–1.5Hz, 50% Duty).

| Signal | Kabelfarbe (typisch Honda) | Spannung aktiv |
|---|---|---|
| Abblendlicht | Blau/Weiß (Lo/Hi-Beam Schalter) | 12V dauerhaft bei Licht an |
| Fernlicht | Blau | 12V bei Fernlicht |
| Blinker links | Orange | 12V getaktet (~1Hz) |
| Blinker rechts | Hellblau | 12V getaktet (~1Hz) |
| Bremslicht (hinten) | Grün/Gelb | 12V bei Hebel gezogen |
| Hupe | Schwarz/Grün | 12V bei Hupe gedrückt |

⚠️ Kabelfarben können je nach Baujahr und Markt variieren. Immer mit Schaltplan verifizieren.

### 8.3 Schaltung: Optokoppler-Eingang

Galvanische Trennung ist wichtig. Motorrad-Elektrik ist zwar unempfindlich, aber Lastspitzen (Anlasser, Zündfunken) erzeugen Spannungsspitzen, die den ESP32 zerstören können.

```
Signal (12V) ──[47kΩ]──[Optokoppler PC817 LED+]──[LED-]── GND (Motorrad)

PC817 Collector ── 3.3V (Pull-Up 10kΩ)
PC817 Emitter  ── GND (ESP32)
               └── ESP32 GPIO (mit Pull-Up)
```

Widerstandsdimensionierung für PC817:
- Input-LED-Vorwiderstand: R = (12V - 1.2V) / 5mA = 2.16kΩ → 2.2kΩ Standardwert. Bei Blinkern (getastet) ist das unkritisch. Für evtl. auftretende Spannungsspitzen (Lastabwurf kann >60V erzeugen) ist ein 47kΩ Widerstand mit Z-Diode (12V) als Schutz empfehlenswert.
- Pull-Up: 10kΩ am Kollektor nach 3.3V.

Sicherer Ansatz mit Z-Diode als Überspannungsschutz:

```
Signal (12V) ──[47kΩ]──┬──[PC817 LED+]──[LED-]── GND
                       │
                  [12V Z-Diode nach GND]  (parallel zur LED, Kathode zum Signal)
```

### 8.4 ESP32-Code: Digital-Input mit Entprellung

```c
#include "driver/gpio.h"

#define TURN_LEFT_GPIO  15
#define TURN_RIGHT_GPIO 2
#define HIGH_BEAM_GPIO  0
#define BRAKE_GPIO      4
#define HORN_GPIO       13

void signal_inputs_init(void) {
    int64_t config_pins[] = {TURN_LEFT_GPIO, TURN_RIGHT_GPIO, HIGH_BEAM_GPIO, BRAKE_GPIO, HORN_GPIO};
    for (int i = 0; i < 5; i++) {
        gpio_config_t io = {
            .pin_bit_mask = (1ULL << config_pins[i]),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io);
    }
}

// Blinker-Erkennung mit Software-Entprellung
typedef struct {
    int gpio;
    bool current_state;
    bool last_state;
    uint32_t last_change_ms;
} debounced_input_t;

bool read_debounced(debounced_input_t *input) {
    bool raw = gpio_get_level(input->gpio) == 0; // LOW = aktiv (Optokoppler zieht runter)
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (raw != input->last_state) {
        input->last_change_ms = now;
        input->last_state = raw;
    }
    // 50ms Entprellzeit
    if (now - input->last_change_ms > 50) {
        input->current_state = raw;
    }
    return input->current_state;
}
```

### 8.5 Blinker-Taktung erkennen

Blinker sind getaktet (~1Hz, 50% Duty). Das ist eine Besonderheit: ein einfacher digitaler Input zeigt nur den momentanen Zustand. Um den Blinker als »aktiv« zu erkennen (nicht nur der aktuelle High/Low-Pegel), muss die Taktung erfasst werden.

```c
// Blinker aktiv erkennen: Pegelwechsel innerhalb der letzten 1.5s?
typedef struct {
    int gpio;
    uint32_t last_edge_ms;
    bool is_active;
} turn_signal_t;

void turn_signal_update(turn_signal_t *sig) {
    bool level = gpio_get_level(sig->gpio) == 0;
    static bool last_level[5] = {0};

    if (level != last_level[sig->gpio]) {
        sig->last_edge_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        last_level[sig->gpio] = level;
    }

    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    sig->is_active = (now - sig->last_edge_ms) < 1500; // Flanke in letzter 1.5s?
}
```

---

## GPIO-Zusammenfassung

Empfohlene Pin-Zuweisung für den ESP32 (ADC1 für alle analogen Signale, da WiFi ADC2 blockiert):

| Funktion | GPIO | Peripheral | Hinweis |
|---|---|---|---|
| RPM (Optokoppler) | 36 | PCNT | Input-only, kein Pull-Up intern |
| GPS UART RX | 16 | UART1 | NEO-M9N TX |
| GPS UART TX | 17 | UART1 | (optional für Konfiguration) |
| Kühlmitteltemp (NTC) | 34 | ADC1_CH6 | Input-only |
| Kraftstoffstand (OPV) | 35 | ADC1_CH7 | Input-only |
| Öldruck analog (VDO) | 33 | ADC1_CH5 | Spannungsteiler |
| Bordspannung | 32 | ADC1_CH4 | Spannungsteiler |
| Öldruckschalter (digital) | 14 | GPIO | Mit Pull-Up |
| Leerlaufschalter | 4 | GPIO | Mit Pull-Up |
| Blinker links | 15 | GPIO | Optokoppler |
| Blinker rechts | 2 | GPIO | Optokoppler ( onboard LED) |
| Fernlicht | 0 | GPIO | Optokoppler (Boot-Strap!) |
| Bremslicht | 12 | GPIO | Optokoppler |
| Hupe | 13 | GPIO | Optokoppler |
| Display SPI | 18/19/23/5/21 | SPI | VSPI |
| I2C (IMU) | 21/22 | I2C | SDA/SCL |

⚠️ GPIO 0 ist Boot-Strap-Pin — muss beim Boot HIGH sein. Optokoppler mit Pull-Up ist sicher, solange beim Boot das Signal inaktiv ist (Fernlicht aus).

---

## Quellenverzeichnis

### Fahrzeugdaten & Service Manual
- [Honda XL600/650V & XRV750 Service Manual (PDF)](https://www.moto4.ru/downloads/Service_Repare_Manual_Honda_XL600_650V_Transalp_XRV750_AfricaTwin_87_07_st_font_fin.pdf)
- [motorcyclespecs.co.za XL600V 1988](https://motorcyclespecs.co.za/model/Honda/honda_xl600v_88.htm)
- [motorcyclespecs.co.za XL600V 1990](https://motorcyclespecs.co.za/model/Honda/honda_xl600v_90.htm)
- [motorcyclespecs.co.za XL600V 1998](https://motorcyclespecs.co.za/model/Honda/honda_xl600v_98.htm)
- [bikeswiki.com XL600V Transalp](https://bikeswiki.com/Honda_XL600V_Transalp)
- [motoinst.ru Transalp Electric Equipment](https://motoinst.ru/en/Honda/Africa-Twin/XRV750/electric/equipment/115153)
- [Wikipedia Honda Transalp](https://en.wikipedia.org/wiki/Honda_Transalp)

### CDI & Zündung
- [rolf-grauer.de CDI XL600 Analyse](https://www.rolf-grauer.de/cdixl600.pdf)
- [scribd.com Honda XL600 CDI Wiring & Function](https://www.scribd.com/doc/56944238/Cdi-Xl600-en)
- [cdi-shop.de Transalp CDI](https://cdi-shop.de/en/shop/honda-transalp-xl600v-pd06/cdi100-d)
- [NXp AN4006 CDI Reference Design](https://www.nxp.com/docs/en/application-note/AN4006.pdf)
- [drriders.com Ignition Pulse Measurements](https://drriders.com/ignition-pulse-measurements-t7955.html)

### Lichtmaschine & Regler
- [oro2u.com Regulator XLV600](https://oro2u.com/regulator-xlv600-transalp-31600-ms6-921-aha6050-road-dze2054)
- [rmstator-europe.com XV600V Transalp Parts](https://rmstator-europe.com/en/1454-xlv600v-transalp)
- [madhornets.com Stator + Regulator](https://madhornets.com/magneto-coil-stator-voltage-regulator-gasket-assy-fit-for-honda-xlv400-xl400v-transalp-400-92-94-xlv600-xl600v-transalp-600-87-99)

### Öldruck
- [Autodoc Oil Pressure Switches](https://www.autodoc.co.uk/car-parts/oil-pressure-switch-sender-unit-valve-10578)
- [Arduino Forum: VDO Oil Pressure interfacing](https://forum.arduino.cc/t/vdo-oil-pressure-interfacing-with-arduino/391373)
- [preOBD Oil Pressure Thresholds](https://github.com/preobd/preOBD/blob/main/docs/README.md)
- [SmartBoat Innovations: Oil Pressure ESP32](https://smartboatinnovations.com/oil-pressure-monitor-esp32-ina219)

### Kraftstoffstand
- [AutoMeter Fuel Sender Guide](https://www.autometer.com/blog/understanding-fuel-level-sender-resistance-ranges/)
- [Summit Racing: Measure Fuel Sender](https://help.summitracing.com/knowledgebase/article/SR-03679/en-us)
- [androidexperto.com Fuel Sender Ranges](https://androidexperto.com/inverting-resistance-and-range-of-fuel-level-sender)

### Kühlmitteltemperatur
- [dirtymotorcycleadventures.com Coolant System](https://www.dirtymotorcycleadventures.com/2011/looking-over-the-coolant-system-on-the-transalp/)
- [xrv.org.uk Radiator Fan Problems](https://xrv.org.uk/threads/radiator-fan-problems.170473)
- [unimotors.ro Temperature Sensor](https://unimotors.ro/en/Temperature-sensor-Honda-Africa-Twin-CB-CBF-Pantheon-Transalp-Shadow-RMS_p_133678.html)

### GPS & Geschwindigkeit
- [u-blox NEO-M9N DataSheet](https://content.u-blox.com/sites/default/files/NEO-M9N-00B_DataSheet_UBX-19014285.pdf)
- [u-blox NEO-M9N Product Summary](https://content.u-blox.com/sites/default/files/NEO-M9N_ProductSummary_UBX-19027207.pdf)
- [SparkFun Community: NEO-M9N Speed Accuracy](https://community.sparkfun.com/t/neo-m9n-speed-accuracy-problems/44868)
- [reddit.com/r/motorcycles Aftermarket Speedometer](https://reddit.com/r/motorcycles/comments/1xldes/aftermarket_speedometer_help)
- [GitHub: MotoNav-CYD ESP32 GPS Speedometer](https://github.com/markseder/MotoNav-CYD)

### ESP32 & ADC
- [ESP-IDF ADC Calibration](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/adc_calibration.html)
- [ESP-IDF PCNT (Pulse Counter)](https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32/api-reference/peripherals/pcnt.html)
- [nedesico/ESP32_Thermistor](https://github.com/nedesico/ESP32_Thermistor)
- [ESPHome NTC Calibration](https://gist.github.com/wujku/1fecfcfde1494db23956cfb5dcdbe54f)

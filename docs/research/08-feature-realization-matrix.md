# Feature-Realisierungs-Matrix: Datenlage -> Machbarkeit

> Stand: 13.10.2026
> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986-2000, rein mechanisch, keine ECU
> Plattform: ESP32-S3-WROOM-1 (N8R2, 8MB Flash, 2MB PSRAM)
> Basis: Querschnittsanalyse aller Research-Dokumente (01-07)

## 0. Einleitung

Dieses Dokument verknuepft die Erkenntnisse aus sieben Research-Dokumenten
(ueber 34.000 Woerter) zu einer umfassenden Feature-Realisierungs-Matrix. Es
beantwortet die zentrale Frage: Was ist mit der aktuell recherchierten
Datenlage und Hardware-Auswahl moeglich, und in welcher Reihenfolge sollten
Features umgesetzt werden?

Die Honda Transalp 600 (PD06) ist ein rein mechanisches Motorrad ohne ECU,
CAN-Bus oder OBD-II. Alle Daten muessen ueber eigenstaendige Sensoren erfasst
werden. Das ist eine Herausforderung, aber auch eine Chance: Wir haben volle
Kontrolle ueber jede Messgroesse und jedes Sensor-Interface.

Die Hardware-Basis steht fest:
- MCU: ESP32-S3-WROOM-1 (N8R2), Dual-Core 240MHz, BLE 5.0, 45 GPIO
- GPS: u-blox NEO-M9N (4-Konstellation, 25Hz, UART)
- IMU: BMI270 (6-DoF, I2C, 685uA, Automotive-proven)
- Barometer: BMP388 (0.08 hPa rel., 512 Byte FIFO, I2C)
- Display Phase 1: 3.5" TFT 480x320 (ILI9488/ST7796, SPI2) — oder E-Paper 2.9"
- Display Phase 2: 2.13" E-Paper fuer statische Werte (SPI3)
- SD-Karte: 8-16GB SDHC, Class 10, SPI
- BLE: NimBLE Stack (50% weniger RAM als Bluedroid)
- Power: MP1584EN Buck (12V->5V) + HT7333 LDO (5V->3.3V) + 18650 Backup

---

## 1. Was ist mit der aktuellen Datenlage moeglich?

### 1.1 Verfuegbare Sensordaten

Aus den Research-Dokumenten 01 (Sensoren), 02 (Hardware), 04 (PD06-Daten) und
06 (Roadmap) ergibt sich folgende Datenlage:

| Sensordatum | Quelle | Interface | Verfuegbar? | Genauigkeit |
|---|---|---|---|---|
| Geschwindigkeit | GPS NEO-M9N (Doppler) | UART (NMEA/UBX) | Ja | +-0.2 km/h, 25Hz |
| Drehzahl (RPM) | CDI-Pickup (1 Puls/Rev) | Optokoppler -> PCNT | Ja | +-2 RPM bei 500ms Fenster |
| Kuehlmitteltemperatur | NTC 10k B3950 (Inline-Sensor) | Spannungsteiler -> ADC1 | Ja | +-0.5C |
| Bordspannung | Spannungsteiler (120k/33k) | ADC1 | Ja | +-0.1V |
| GPS-Position | NEO-M9N (Lat/Lon/Alt) | UART (NMEA/UBX) | Ja | 2.0m CEP |
| GPS-Heading | NEO-M9N (Course) | UART (NMEA RMC) | Ja | +-1 Grad |
| GPS-Zeit | NEO-M9N (PPS + RMC) | UART + GPIO IRQ | Ja | +-1ns (PPS) |
| Neigung (Roll/Pitch) | BMI270 (Accel+Gyro) | I2C | Ja | +-1 Grad (Complementary Filter) |
| Hoehe (barometrisch) | BMP388 | I2C | Ja | +-0.5m (relativ) |
| Umwelttemperatur | BMP388 (Temp-Kompensation) | I2C | Ja | +-1C (Eigenwaerme-korrigiert) |
| Kraftstoffstand | Aftermarket-Tankgeber (0-190 Ohm) | OPV-Buffer -> ADC1 | Nachruestung | +-0.5-1L |
| Oeldruck (digital) | OEM-Oeldruckschalter (M10x1.0) | Optokoppler -> GPIO | Ja (ON/OFF) | Binaer |
| Oeldruck (analog) | VDO 0-10 bar Sender (Nachruestung) | Spannungsteiler -> ADC1 | Nachruestung | +-1.5% FS |
| Leerlaufschalter | OEM-Neutral-Schalter | GPIO (Pull-Up) | Ja | Binaer |
| Blinker/Licht/Bremse | OEM-Leitungen (12V) | Optokoppler -> GPIO | Ja | Binaer |

### 1.2 Kritische Datenluecken

Aus Dokument 04 (PD06-Technische-Daten) ergeben sich zwei wichtige Korrekturen
gegenueber der urspruenglichen Roadmap (Dokument 06):

1. **Kraftstoffstand**: Die PD06 hat serienmaessig KEINEN Tankgeber mit
   Widerstandssignal. Sie verwendet einen Vakuum-Petcock mit Reserve-Stellung.
   Fuer eine elektronische Tankstandsanzeige muss ein Aftermarket-Schwimmer
   durch die Tankoeffnung installiert werden. Die Kennlinie muss empirisch
   kalibriert werden (2-Liter-Schritte). Der original Petcock bleibt erhalten.

2. **Oeldruck**: Der OEM-Sensor ist ein reiner Einfach-Schalter (ON/OFF bei
   ca. 0.2-0.4 bar). Fuer eine analoge Druckanzeige muss ein VDO-Sender
   (0-10 bar, 1/8 NPT) ueber einen M10x1.0 auf 1/8-NPT-Adapter am
   Oelkreislauf nachgeruestet werden. Der Originalschalter bleibt ueber ein
   T-Stueck funktionsfaehig.

3. **Kuehlmitteltemperatur**: Die PD06 hat keinen elektronischen
   Kuehlmitteltemperatur-Sensor, nur einen Thermoschalter (Luefter bei ~103C).
   Ein NTC oder DS18B20 muss als Inline-Sensor in den Kuehlmittelschlauch
   eingesetzt werden.

Diese drei Punkte bedeuten: Die "Must-Have"-Features fuer Kraftstoffstand
und Oeldruck sind mit zusaetzlichem Hardware-Einbau verbunden, der
mechanische Eingriffe am Fahrzeug erfordert.

---

## 2. Feature-Matrix

Jedes Feature aus der Roadmap (Dokument 06) wird hier bewertet nach:
- **Machbar?**: Ja / Ja mit Nachruestung / Nein
- **Datenquelle**: Welcher Sensor liefert die Daten?
- **Aufwand**: Niedrig (Stunden) / Mittel (Tage) / Hoch (Wochen) / Sehr Hoch (Monate)
- **Prioritaet**: Must / Should / Could / Won't (mit Begruendung)
- **Abhaengigkeiten**: Was wird vorausgesetzt?
- **Status**: Recherchiert / Prototyp / Produktion / Blockiert

### 2.1 Core-Features (Must-Have)

| # | Feature | Machbar? | Datenquelle | Aufwand | Prioritaet | Abhaengigkeiten | Status |
|---|---|---|---|---|---|---|---|
| 1.1 | Geschwindigkeitsanzeige | Ja | GPS NEO-M9N (Doppler) | Niedrig | Must | GPS-Modul | Recherchiert |
| 1.2 | Drehzahlmesser (RPM) | Ja | CDI-Pickup -> Optokoppler -> PCNT | Mittel | Must | Optokoppler-Schaltung, EMV-Schutz | Recherchiert |
| 1.3 | Kuehlmitteltemperatur | Ja | NTC 10k -> ADC1 | Niedrig | Must | Inline-Sensor im Kuehlschlauch | Recherchiert |
| 1.4 | Kraftstoffstand + Reichweite | Ja mit Nachruestung | Aftermarket-Tankgeber -> OPV -> ADC1 | Mittel-Hoch | Must | Tankgeber-Einbau, Kalibrierung | Recherchiert |
| 1.5 | Uhrzeit (GPS-synced) | Ja | GPS RMC-Satz + PPS | Niedrig | Must | GPS-Modul | Recherchiert |
| 1.6 | Tripcmeter (Trip A/B/ODO) | Ja | GPS (Haversine) | Niedrig | Must | GPS-Modul, NVS fuer ODO | Recherchiert |
| 1.7 | GPS-Track-Logging (GPX) | Ja | GPS + SD-Karte | Niedrig-Mittel | Must | GPS, SD-Karte, FATFS | Recherchiert |

### 2.2 Erweiterte Features (Should-Have)

| # | Feature | Machbar? | Datenquelle | Aufwand | Prioritaet | Abhaengigkeiten | Status |
|---|---|---|---|---|---|---|---|
| 2.1 | Ganganzeige (berechnet) | Ja | RPM + Speed -> Ratio | Mittel | Should | Feature 1.1 + 1.2 | Recherchiert |
| 2.2 | Neigungsanzeige (Roll/Pitch) | Ja | BMI270 -> Complementary Filter | Mittel | Should | BMI270 IMU | Recherchiert |
| 2.3 | Hoehenmesser (barometrisch) | Ja | BMP388 -> baro. Formel | Niedrig | Should | BMP388 Sensor | Recherchiert |
| 2.4 | Umwelttemperatur | Ja | BMP388 (integrierte Temp) | Niedrig | Should | BMP388 Sensor | Recherchiert |
| 2.5 | Oeldruck (analog) | Ja mit Nachruestung | VDO-Sender -> ADC1 | Hoch | Should | VDO-Sensor, M10-Adapter, T-Stueck | Recherchiert |
| 2.6 | Bordspannung | Ja | Spannungsteiler -> ADC1 | Niedrig | Should | 2 Widerstaende | Recherchiert |
| 2.7 | Verbrauchs-Logging | Ja | Kraftstoffstand + GPS-Strecke | Mittel | Should | Feature 1.4 + 1.6 | Recherchiert |
| 2.8 | Fahrtzeit / Pause-Erkennung | Ja | GPS-Speed < 2 km/h fuer >60s | Niedrig | Should | Feature 1.1 | Recherchiert |
| 2.9 | Max-Speed / Max-RPM-Recorder | Ja | GPS-Speed + RPM | Niedrig | Should | Feature 1.1 + 1.2 | Recherchiert |

### 2.3 Smart-Features (BLE + Smartphone)

| # | Feature | Machbar? | Datenquelle | Aufwand | Prioritaet | Abhaengigkeiten | Status |
|---|---|---|---|---|---|---|---|
| 3.1 | Turn-by-Turn Navigation | Ja | Smartphone-App -> BLE GATT -> Display | Mittel | Should | Companion-App, BLE-Stack, Display | Recherchiert |
| 3.2 | Wettervorhersage | Ja | Smartphone-App -> Wetter-API -> BLE | Mittel | Could | Companion-App, Internet | Recherchiert |
| 3.3 | Incoming-Call-Anzeige | Ja (iOS nativ) | iOS-Notifizierungs-Service (ANCS) | Mittel | Could | BLE-Pairing, Plattform-spezifisch | Recherchiert |
| 3.4 | Music-Track-Info | Ja (iOS nativ) | iOS-Media-Service (AMS) | Mittel | Could | BLE-Pairing, Plattform-spezifisch | Recherchiert |
| 3.5 | SMS/Notification-Preview | Ja | ANCS / Android-Notification-Listener | Mittel | Could | Companion-App (Android), BLE | Recherchiert |
| 3.6 | POI-Logging (Wegpunkte) | Ja | GPS + Taster -> GPX | Niedrig | Should | GPS, SD-Karte, 1x GPIO-Taster | Recherchiert |

### 2.4 Safety-Features

| # | Feature | Machbar? | Datenquelle | Aufwand | Prioritaet | Abhaengigkeiten | Status |
|---|---|---|---|---|---|---|---|
| 4.1 | Crash-Erkennung | Ja | BMI270 (>7g + Neigung >75 Grad) | Mittel | Should | Feature 2.2 (IMU) | Recherchiert |
| 4.2 | Automatic SOS | Ja (via Smartphone) | BLE -> Smartphone -> SMS/Anruf | Hoch | Should | Feature 4.1 + BLE + Companion-App | Recherchiert |
| 4.3 | Sturz-Aufzeichnung (30s) | Ja | IMU Ring-Buffer -> SD-Karte | Mittel | Could | Feature 2.2 (IMU) + SD-Karte | Recherchiert |
| 4.4 | Geofencing | Ja | GPS-Position vs. Radius | Niedrig | Could | GPS-Modul | Recherchiert |
| 4.5 | Diebstahl-Warnung | Ja (eingeschraenkt) | IMU Wake-on-Motion + Deep-Sleep + BLE | Hoch | Could | IMU Low-Power, Deep-Sleep, BLE-Reichweite | Recherchiert |

### 2.5 Tour-Features

| # | Feature | Machbar? | Datenquelle | Aufwand | Prioritaet | Abhaengigkeiten | Status |
|---|---|---|---|---|---|---|---|
| 5.1 | Track-Recording mit Telemetrie | Ja | GPS + alle Sensoren -> GPX-Extensions/CSV | Mittel | Should | Feature 1.7 + alle Sensoren | Recherchiert |
| 5.2 | Export GPX/KML/CSV | Ja | SD-Karte -> Konvertierung | Niedrig | Should | Feature 5.1 | Recherchiert |
| 5.3 | Tour-Statistiken | Ja | Aggregation aller Sensordaten | Mittel | Should | Feature 5.1 + 2.3 + 2.7 | Recherchiert |
| 5.4 | Segment-Timing | Ja | GPS-Waypoint-Trigger | Mittel | Could | GPS-Modul | Recherchiert |
| 5.5 | Rueckweg-Navigation | Ja | GPS-Track speichern -> reversiv abfahren | Mittel | Could | GPS-Modul | Recherchiert |
| 5.6 | POI-Marking | Ja | GPS + Taster | Niedrig | Should | Identisch mit 3.6 | Recherchiert |

### 2.6 Diagnose-Features

| # | Feature | Machbar? | Datenquelle | Aufwand | Prioritaet | Abhaengigkeiten | Status |
|---|---|---|---|---|---|---|---|
| 6.1 | Oeldruck-Trend | Ja mit Nachruestung | VDO-Sender -> ADC -> SD-Log | Mittel | Should | Feature 2.5 (analoger Sensor) | Recherchiert |
| 6.2 | Kuehlmitteltemp-Trend | Ja | NTC -> SD-Log | Niedrig | Should | Feature 1.3 | Recherchiert |
| 6.3 | Bordspannung-Trend | Ja | Spannungsteiler -> SD-Log | Niedrig | Should | Feature 2.6 | Recherchiert |
| 6.4 | Kraftstoffverbrauch-Historie | Ja | Feature 2.7 -> Langzeit-Log | Mittel | Could | Feature 2.7 | Recherchiert |
| 6.5 | Wartungs-Reminder | Ja | NVS (km/Zeit-Zaehler) | Niedrig | Should | Feature 1.6 (ODO) | Recherchiert |

### 2.7 Nice-to-Have / Experimentell

| # | Feature | Machbar? | Datenquelle | Aufwand | Prioritaet | Abhaengigkeiten | Status |
|---|---|---|---|---|---|---|---|
| 7.1 | Reifendruck (TPMS) | Ja mit Nachruestung | BLE-TPMS-Sensoren (passiv) | Mittel | Could | ZEEPIN TP630 Sensoren (20-30 EUR) | Recherchiert |
| 7.2 | Hall-Sensor Vorderrad | Ja | A3144 + Magnet -> PCNT | Niedrig | Could | Mechanische Montage | Recherchiert |
| 7.3 | GPS-Beschleunigung (0-100) | Ja | GPS-Speed -> Timer | Niedrig | Could | Feature 1.1 | Recherchiert |
| 7.4 | Lean-Angle-Max-Recorder | Ja | Feature 2.2 -> NVS | Niedrig | Could | Feature 2.2 | Recherchiert |
| 7.5 | Audio-Warnungen (Piezo) | Ja | GPIO -> Buzzer | Niedrig | Could | 1x GPIO, Buzzer | Recherchiert |
| 7.6 | LED-Status-Strip | Ja | WS2812B -> GPIO | Niedrig | Could | 1x GPIO, Pegelwandler | Recherchiert |
| 7.7 | Zweites Display | Ja | SPI3 (HSPI) -> E-Paper | Hoch | Won't | Komplexitaet, Platzmangel | Zurueckgestellt |
| 7.8 | Sprachausgabe via BT | Nein | ESP32-S3 kein BT Classic (A2DP) | Sehr Hoch | Won't | Nicht moeglich mit S3 | Won't-Have |

### 2.8 Daten-Export und Analyse

| # | Feature | Machbar? | Datenquelle | Aufwand | Prioritaet | Abhaengigkeiten | Status |
|---|---|---|---|---|---|---|---|
| 8.1 | GPX-Track (Standard) | Ja | SD-Karte -> GPX 1.1 | Niedrig | Must | Feature 1.7 | Recherchiert |
| 8.2 | CSV-Telemetrie | Ja | Alle Sensoren -> CSV auf SD | Niedrig | Should | SD-Karte, Batch-Write | Recherchiert |
| 8.3 | JSON-API ueber BLE | Ja | NimBLE GATT Notify (200B, 1Hz) | Mittel | Should | BLE-Stack, GATT-Design | Recherchiert |
| 8.4 | Web-Dashboard | Ja (eingeschraenkt) | HTML/JS auf SD-Karte, offline | Hoch | Could | SD-Karte, Browser | Recherchiert |
| 8.5 | Statistik-Uebersicht Display | Ja | Aggregation -> E-Paper/TFT | Niedrig | Should | Display | Recherchiert |

### 2.9 Smartphone-App-Konzept

| # | Feature | Machbar? | Datenquelle | Aufwand | Prioritaet | Abhaengigkeiten | Status |
|---|---|---|---|---|---|---|---|
| 9.1 | Eigene App noetig? | Ja (fuer die meisten Smart-Features) | — | Mittel | Should | BLE-Protokoll-Design | Recherchiert |
| 9.2 | Native App (iOS/Android) | Ja | Flutter/React-Native | Hoch | Could | Doppelter Entwicklungsaufwand | Recherchiert |
| 9.3 | BLE-Protokoll-Design | Ja | NimBLE GATT-Services | Mittel | Must | BLE-Stack | Recherchiert |
| 9.4 | App-Funktionen | Ja | Siehe Feature 9.1-9.5 Roadmap | Mittel | Should | Companion-App | Recherchiert |
| 9.5 | PWA (Web-BLE) Alternative | Ja (eingeschraenkt) | Web Bluetooth API | Mittel | Should | Browser mit Web-BLE | Recherchiert |

### 2.10 Erweiterbarkeit

| # | Feature | Machbar? | Datenquelle | Aufwand | Prioritaet | Abhaengigkeiten | Status |
|---|---|---|---|---|---|---|---|
| 10.1 | Offene Sensor-Architektur | Ja | I2C/SPI/ADC Modul-Interface | Mittel | Should | Sensor-Manager-Framework | Recherchiert |
| 10.2 | Plugin-System UI | Ja | LVGL Screen-Module | Hoch | Could | LVGL-Integration | Recherchiert |
| 10.3 | Config via JSON auf SD | Ja | /config/bordcomputer.json | Niedrig | Should | SD-Karte, FATFS | Recherchiert |
| 10.4 | REST-API / MQTT | Ja (eingeschraenkt) | WiFi (nur bei Stillstand) | Hoch | Could | WiFi-Stack, MQTT-Library | Recherchiert |

---

## 3. Synergie-Effekte

Synergie-Effekte sind Features, die durch die Kombination von Sensordaten
entstehen, die einzeln nicht ausreichen wuerden. Diese sind entscheidend fuer
den Wert des Bordcomputers: der Nutzen wächst ueberproportional mit jeder
zusaetzlichen Sensordatenquelle.

### 3.1 GPS + IMU -> Dead Reckoning in Tunneln

```
    GPS-Position (1-25 Hz)                    IMU-Gyro/Accel (50-200 Hz)
         |                                          |
         v                                          v
    +----------+        +-------------+        +----------+
    | Kalman   | <----- | Sensor-     | -----> | IMU-     |
    | Filter   |        | Fusion      |        | Drift-   |
    | (POS)    |        | (Complementary)       | Komp.    |
    +----------+        +-------------+        +----------+
         |
         v
    Positionsschaetzung bei GPS-Ausfall
    (Gueltig fuer ca. 10-30 Sekunden)
```

Die BMI270-IMU kann bei GPS-Ausfall (Tunnel, Tiefgarage, Baumallee) die
Position durch Integration der Beschleunigungs- und Gierraten-Daten
schaetzen. Das ist kein echtes Dead Reckoning (wie beim NEO-M8U mit
integrierter IMU), aber ausreichend fuer kurze Unterbrechungen.

Die Herausforderung: Die IMU-Drift bei Motorrad-Einsatz ist schwierig,
weil Schraeglage und Geradeausfahrt unterschieden werden muessen. Der
Complementary Filter gibt dem Gyroskop in dynamischen Phasen mehr Gewicht
und dem Beschleunigungssensor bei Niedrigbeschleunigung. Bei einer
Schaeglage von 30 Grad und Integration ueber 10 Sekunden kann der Fehler
auf 5-15 Meter ansteigen — ausreichend fuer Track-Logging, aber nicht fuer
Praezisions-Navigation.

Status: Moeglich aber mit Einschraenkungen. Sollte erst nach stabiler
GPS-Implementierung angegangen werden.

### 3.2 IMU + GPS -> Lean-Angle-Mapping auf Track

```
    GPS-Speed (km/h)                IMU-Gyro-Z (Gierrate)
         |                               |
         v                               v
    +----------+                   +----------+
    | Speed -> |                   | Gyro ->  |
    | Heading  |                   | Roll-Rate|
    +----------+                   +----------+
         |                               |
         v                               v
    +------------------------------------------+
    |     Track-Mapping (Kurven-Identifikation)|
    |     - Schraeglage pro Kurve              |
    |     - Geschwindigkeit pro Kurve          |
    |     - Kurvenradius berechnet             |
    +------------------------------------------+
```

Die Kombination aus GPS-Heading-Aenderung und IMU-Roll-Winkel ermoeglicht
eine detaillierte Kurvenanalyse: Welche Schraeglage in welcher Kurve bei
welcher Geschwindigkeit. Das ist fuer Trackday-Fahrer und
Spass-Fahrer interessant.

Die Daten koennen als Heatmap auf einer Karte dargestellt werden
(spater ueber Web-Dashboard oder Smartphone-App). Der Bordcomputer selbst
kann Max-Lean-Recorder links/rechts pro Tour speichern.

### 3.3 Kraftstoffstand + GPS-Speed -> Verbrauchsprofil pro Strecke

```
    Kraftstoffstand (ADC, 1 Hz)       GPS-Speed + Distanz (1-25 Hz)
         |                                     |
         v                                     v
    +----------+                        +----------+
    | Delta-   |                        | Strecke  |
    | Kraft-   |                        | (Haver-  |
    | stoff    |                        |  sine)   |
    +----------+                        +----------+
         |                                     |
         v                                     v
    +------------------------------------------+
    |   Verbrauch pro 100 km (gleitend 50km)  |
    |   - l/100km pro Streckenabschnitt       |
    |   - Korrelation mit Geschwindigkeit     |
    |   - Korrelation mit Hoehenprofil        |
    +------------------------------------------+
```

Der Kraftstoffstandsensor hat eine Aufloesung von ca. 0.5-1 Liter. Bei
einem 18-Liter-Tank und 5 l/100km Verbrauch entspricht das ca. 5-10%
Fehlertoleranz pro Messung. Mit Mittelung ueber 50-100 km wird es
brauchbar. Der echte Mehrwert entsteht durch die Korrelation mit
Geschwindigkeit und Hoehenprofil: Bergauf-Bergab, Landstrasse vs.
Autobahn, werden als unterschiedliche Verbrauchsprofile sichtbar.

Einschraenkung: Der Tankgeber muss erst nachgeruestet und kalibriert
werden (siehe Abschnitt 1.2). Ohne analogen Sensor ist nur die
Reserve-Warnung (Petcock-Stellung) verfuegbar.

### 3.4 Drehzahl + Geschwindigkeit -> Ganganzeige (berechnet)

```
    RPM (PCNT, 2 Hz)              Speed (GPS, 1-25 Hz)
         |                               |
         v                               v
    +----------+                   +----------+
    | RPM      |                   | Speed    |
    | (Pulse/  |                   | (km/h)   |
    |  Rev)    |                   |          |
    +----------+                   +----------+
         |                               |
         v                               v
    +------------------------------------------+
    |  Ratio = RPM / Speed                     |
    |  Match gegen Getriebe-Uebersetzungen:   |
    |  I=2.500 II=1.722 III=1.333             |
    |  IV=1.111 V=0.961                       |
    |  Primär: 1.888, Sekundaer: 3.133        |
    |  Reifenumfang: 2.010m (hinten)          |
    +------------------------------------------+
         |
         v
    Gang 1-5 oder N (Leerlauf)
```

Die PD06 hat ein 5-Gang-Getriebe mit bekannten Uebersetzungsverhaeltnissen
(siehe Dokument 04, Abschnitt 2). Die Gesamtuebersetzung pro Gang ergibt
sich aus: Primaer x Gang x Sekundaer. Die Motordrehzahl pro km/h ist fuer
jeden Gang eindeutig (ausser beim 5. Gang, der nahe an 1.0 liegt und schwer
vom 4. Gang zu unterscheiden ist).

Die Methode: Berechne das Verhaeltnis RPM/Speed und vergleiche mit den
 theoretischen Werten. Bei Abweichung < 10% vom naechsten Match -> Gang
 erkannt. Bei Kupplung gezogen oder Leerlauf -> "N" anzeigen.

Diese Berechnung ist in der Firmware direkt implementierbar, sobald RPM
und Speed verfuegbar sind. Sie benoetigt KEINE zusaetzliche Hardware.

### 3.5 Bordspannung + Drehzahl -> Generator-Health-Monitoring

```
    Bordspannung (ADC, 1 Hz)       RPM (PCNT, 2 Hz)
         |                               |
         v                               v
    +----------+                   +----------+
    | Spannung |                   | Drehzahl |
    | (Volt)   |                   | (U/min)  |
    +----------+                   +----------+
         |                               |
         v                               v
    +------------------------------------------+
    |  Generator-Diagnose:                    |
    |  - Spannung bei Leerlauf (1300 rpm)     |
    |    soll: 13.5-14.5V                     |
    |  - Spannung bei 5000 rpm soll: 14-15V   |
    |  - Trend: Abfall ueber 1000 km -> Regler|
    |    oder Stator defekt                   |
    |  - PD06 bekanntes Problem:              |
    |    Lichtmaschinenregler-Ausfall         |
    +------------------------------------------+
```

Der Regler/Gleichrichter der PD06 ist ein bekannter Schwachpunkt
(Shindengen SH538D-13, Shunt-Typ). Die Kombination aus Bordspannung und
Drehzahl ermoeglicht eine fruehzeitige Diagnose:

- Spannung < 13.5V bei > 3000 RPM -> Generator/Regler schwach
- Spannung > 15V -> Regler defekt (Ueberladung, Batterie-Gefahr)
- Trend: Durchschnittsspannung sinkt ueber 500 km -> Stator oder
  Regler im Anfangsstadium des Versagens

Diese Diagnose ist mit minimaler Hardware moeglich (2 Widerstaende fuer
den Spannungsteiler) und bietet hohen praktischen Wert fuer ein 30+
Jahre altes Motorrad.

### 3.6 Uebersicht aller Synergie-Effekte

| Kombination | Ergebnis | Wert | Status |
|---|---|---|---|
| GPS + IMU | Dead Reckoning (kurze GPS-Ausfaelle) | Mittel | Moeglich |
| IMU + GPS | Lean-Angle-Mapping, Kurvenanalyse | Hoch | Moeglich |
| Kraftstoff + GPS | Verbrauchsprofil pro Strecke | Hoch | Benoetigt Tankgeber |
| RPM + Speed | Ganganzeige (berechnet) | Hoch | Moeglich (keine HW) |
| Bordspannung + RPM | Generator-Health-Monitoring | Hoch | Moeglich (minimal HW) |
| Hoehe (baro) + GPS-Hoehe | Praezise Hoehenmessung | Mittel | Moeglich |
| Temp + Bordspannung | Kuehler-Diagnose (Luefter-Schaltverhalten) | Mittel | Moeglich |
| IMU + SD-Karte | Crash-Aufzeichnung (30s Ring-Buffer) | Hoch | Moeglich |
| GPS + BLE + Smartphone | Turn-by-Turn Navigation | Hoch | Moeglich (App noetig) |
| Alle Sensoren + SD | Telemetrie-Recording | Hoch | Moeglich |

---

## 4. Daten-Pipelines

Fuer jedes realisierbare Feature wird hier die Datenpipeline skizziert:
Sensor -> Verarbeitung -> Output (Display/Log/BLE)

### 4.1 Geschwindigkeits-Pipeline

```
  +-----------+     UART      +-----------+     Queue      +-----------+
  | NEO-M9N   | ---115200---> | ESP32-S3  | ------------> | Display   |
  | GPS       |    NMEA/UBX  | minmea    |   gps_data_t  | Task      |
  +-----------+               | Parser    |               +-----------+
                              +-----------+
                                    |
                                    v
                              +-----------+
                              | Logger    |
                              | Task ->   |
                              | SD-Karte  |
                              +-----------+
```

- Sensor: NEO-M9N, 4-GNSS, 25Hz max, Doppler-Speed +-0.2 km/h
- Verarbeitung: minmea Parser (pure C), RMC-Sentence -> Speed in km/h
- Output: Display (1Hz Partial-Refresh), SD-Log (1-5Hz), BLE Notify (1Hz)

### 4.2 RPM-Pipeline

```
  +-----------+   Optokoppler   +-----------+    PCNT     +-----------+
  | CDI-Pickup| ---PC817/6N137->| ESP32-S3  | ---------> | Sensor    |
  | (1 Puls/  |   22k-47k Ohm   | PCNT Unit|  count/   | Task      |
  |  Rev)     |                 | Glitch   |  500ms    +-----------+
  +-----------+                 | Filter   |                 |
                                +-----------+                 v
                                      |               +-----------+
                                      v               | Display   |
                                +-----------+         | (Median-  |
                                | RPM =     |         |  Filter)  |
                                | count*120 |         +-----------+
                                +-----------+              |
                                                           v
                                                    +-----------+
                                                    | Logger -> |
                                                    | SD-Karte  |
                                                    +-----------+
```

- Sensor: CDI-Pickup, 1 Puls pro Kurbelwellenumdrehung (Wasted Spark)
- Verarbeitung: PCNT Hardware-Pulse-Counter, 500ms Messfenster, Median-Filter (5 Werte)
- Output: Display (2Hz), SD-Log (1-5Hz), BLE Notify (500ms)

### 4.3 Temperatur-Pipeline (Kuehlmittel)

```
  +-----------+   Spannungs-     +-----------+    ADC1     +-----------+
  | NTC 10k   | ---teiler------> | ESP32-S3  | ---------> | Sensor    |
  | B3950     |   10k Ref +      | ADC1_CH2  |  16x over-| Task      |
  | Inline    |   100nF Bypass   | esp_adc_  |  sampling +-----------+
  +-----------+                  | cal       |                 |
                                 +-----------+                 v
                                       |               +-----------+
                                       v               | Display   |
                                 +-----------+         | (Exp.     |
                                 | Steinhart |         |  Filter)  |
                                 | -Hart ->  |         +-----------+
                                 | Celsius   |              |
                                 +-----------+              v
                                                      +-----------+
                                                      | Logger -> |
                                                      | SD-Karte  |
                                                      | + Alarm   |
                                                      |  bei >115C|
                                                      +-----------+
```

- Sensor: NTC 10k B3950, Inline im Kuehlmittelschlauch
- Verarbeitung: ADC1 mit 16x Oversampling, Steinhart-Hart-Gleichung, exponentieller Filter (alpha=0.1)
- Output: Display (1Hz), SD-Log (alle 2-10s), Alarm bei >115C

### 4.4 Kraftstoffstand-Pipeline

```
  +-----------+   OPV-Buffer    +-----------+    ADC1     +-----------+
  | Aftermarkt| ---LM358------> | ESP32-S3  | ---------> | Sensor    |
  | Tankgeber|   Spannungs-    | ADC1_CH1  |  16x over- | Task      |
  | 0-190 Ohm|   folger (keine  |           |  sampling +-----------+
  +-----------+   Belastung)    +-----------+                 |
                                                       v
                                                 +-----------+
                                                 | Lookup-   |
                                                 | Tabelle   |
                                                 | (kalibri- |
                                                 | ert) -> L |
                                                 +-----------+
                                                       |
                                                       v
                                                 +-----------+
                                                 | Display + |
                                                 | Reichw.  |
                                                 +-----------+
```

- Sensor: Aftermarket-Tankgeber (0-190 Ohm), OPV LM358 als Spannungsfolger
- Verarbeitung: ADC1, Lookup-Tabelle (kalibriert in 2L-Schritten), Reichweitenberechnung
- Output: Display (1Hz), SD-Log (alle 30s), BLE Notify (60s)

### 4.5 Bordspannungs-Pipeline

```
  +-----------+   Spannungs-     +-----------+    ADC1     +-----------+
  | 12V Bord- | ---teiler------> | ESP32-S3  | ---------> | Power     |
  | netz      |   120k/33k      | ADC1_CH4  |  16x over- | Task      |
  +-----------+                  | esp_adc_  |  sampling +-----------+
                                 | cal       |                 |
                                 +-----------+                 v
                                       |               +-----------+
                                       v               | Display + |
                                 +-----------+         | Trend-   |
                                 | V = ADC x |         | Log +    |
                                 | 4.636    |         | Alarm    |
                                 +-----------+         +-----------+
```

- Sensor: Spannungsteiler 120k/33k an Batterie
- Verarbeitung: ADC1, Skalierung x 4.636, Mittelwert 50 Samples
- Output: Display (1Hz), SD-Log (alle 30s), Alarm bei <11.5V oder >15V

### 4.6 IMU/Neigungs-Pipeline

```
  +-----------+     I2C       +-----------+    Queue      +-----------+
  | BMI270    | --400kHz----> | ESP32-S3  | ------------> | Display  |
  | Accel+    |   SDA=GPIO8   | I2C-Master|  sensor_data_t| (Roll/Pit|
  | Gyro      |   SCL=GPIO9   | 50 Hz     |               |  Balken) |
  +-----------+                +-----------+               +-----------+
                                       |
                                       v
                                 +-----------+
                                 | Complem. |
                                 | Filter:  |
                                 | roll =   |
                                 | 0.98*    |
                                 | (roll+   |
                                 | gyro*dt)+|
                                 | 0.02*    |
                                 | accel_r  |
                                 +-----------+
                                       |
                                       v
                                 +-----------+    +-----------+
                                 | Ring-    |--->| SD-Karte  |
                                 | Buffer   |    | (Crash-   |
                                 | (30s)    |    |  Aufz.)   |
                                 +-----------+    +-----------+
```

- Sensor: BMI270 (6-DoF), 50 Hz Sample-Rate, I2C 400kHz
- Verarbeitung: Complementary Filter (alpha=0.98), Roll aus atan2(ay,az)
- Output: Display (10Hz Partial-Refresh), Ring-Buffer (30s), SD-Log (10Hz), BLE Notify (200ms)

### 4.7 BLE-Telemetrie-Pipeline

```
  +-----------+               +-----------+               +-----------+
  | Alle      | --Queues----> | ESP32-S3  | --BLE GATT-> | Smart-    |
  | Sensor-   |               | NimBLE    |   Notify     | phone     |
  | Tasks     |               | Stack     |   247B MTU   | App / PWA |
  +-----------+               +-----------+               +-----------+
                                     ^
                                     |
                               +-----------+
                               | Smartphone|
                               | -> Config |
                               | -> Nav-   |
                               |   Data    |
                               | -> OTA    |
                               +-----------+
```

- Transport: NimBLE BLE 5.0, MTU 247, Custom GATT-Profile
- Output: Live-Telemetrie (1Hz, ~200B JSON), Nav-Data (Event), Config (RW), OTA (Chunked)
- Bidirektional: Bordcomputer sendet Telemetrie/Tracks/Alerts, Smartphone sendet Nav/Config/Firmware

### 4.8 Crash-Erkennungs-Pipeline

```
  +-----------+               +-----------+               +-----------+
  | BMI270    | --50 Hz ----> | Crash-    | --Event----> | SOS-      |
  | Accel+    |               | Detector |              | Handler   |
  | Gyro      |               |           |              +-----------+
  +-----------+               +-----------+                    |
                                     |                         v
                              +-----------+           +-----------+
                              | Stage 1: |           | BLE ->    |
                              | |accel|  |           | Smartphone|
                              | >7g @   |           | -> SOS    |
                              | 100ms   |           | (SMS/Anr.)|
                              +-----------+           +-----------+
                                     |
                                     v
                              +-----------+
                              | Stage 2: |
                              | Neigung  |
                              | >75 Grad |
                              | @ 500ms  |
                              +-----------+
                                     |
                                     v
                              +-----------+
                              | Stage 3: |
                              | 10s      |
                              | Countdown|
                              | -> Abort |
                              | durch    |
                              | Tastendr.|
                              +-----------+
```

- Sensor: BMI270, 50 Hz Accel + Gyro
- Algorithmus: 3-Stage-Verifikation (Beschleunigung > Neigung > Countdown)
- Output: Ring-Buffer auf SD sichern, BLE-Notfall-Message an Smartphone

---

## 5. Was fehlt noch an Daten?

### 5.1 Sensoren die nachgeruestet werden muessen

| Sensor | Feature | Kosten | Einbau-Aufwand | Risiko |
|---|---|---|---|---|
| Aftermarket-Tankgeber | Kraftstoffstand (1.4, 2.7) | 10-20 EUR | Hoch (Tankoeffnung) | Mittel (Ex-Schutz) |
| NTC 10k B3950 Inline | Kuehlmitteltemp (1.3) | 2-5 EUR | Mittel (Kuehlschlauch) | Niedrig |
| VDO Oeldrucksender | Analoger Oeldruck (2.5, 6.1) | 20-30 EUR | Hoch (Oelkreislauf, M10-Adapter) | Mittel (Oelverlust) |
| Optokoppler-Schaltung | RPM (1.2) | 2-5 EUR | Mittel (CDI-Kabel) | Niedrig (galv. Trennung) |
| Hall-Sensor + Magnet | Radgeschwindigkeit (7.2) | 2-5 EUR | Hoch (Gabel/Bremsscheibe) | Niedrig |
| BLE-TPMS-Sensoren | Reifendruck (7.1) | 20-30 EUR | Niedrig (Ventilkappen) | Niedrig |
| Taster am Lenker | POI-Logging (3.6) | 1 EUR | Niedrig | Niedrig |
| Piezo-Buzzer | Audio-Warnungen (7.5) | 1 EUR | Niedrig | Niedrig |
| WS2812B-Strip | LED-Warnungen (7.6) | 2-3 EUR | Niedrig | Niedrig |

### 5.2 Software die noch nicht verfuegbar ist

| Komponente | Feature | Aufwand | Alternative |
|---|---|---|---|
| Companion-App (iOS/Android) | Nav, Wetter, Call, Music, SOS | Sehr Hoch | PWA mit Web-BLE (eingeschraenkt) |
| Kalman-Filter fuer GPS+IMU-Fusion | Dead Reckoning | Hoch | Complementary-Filter (einfacher) |
| Web-Dashboard (HTML/JS) | Analyse nach Tour | Hoch | CSV-Export zu externer Software |
| MQTT/REST-API | Home-Assistant-Integration | Hoch | BLE als API |
| Karten-Rendering auf Display | On-Device-Karte | Sehr Hoch | Nicht moeglich (Ressourcen) |

### 5.3 Hardware-Einschraenkungen des ESP32-S3

1. **Kein Bluetooth Classic**: Der ESP32-S3 unterstuetzt nur BLE 5.0, kein
   BT Classic. Das bedeutet: kein A2DP (Audio-Streaming), kein SPP (Serial
   Port Profile). Sprachausgabe ueber BT-Kopfhoerer ist nicht direkt
   moeglich. Loesung: Smartphone uebernimmt TTS und sendet an
   BT-Kopfhoerer, Bordcomputer sendet nur Text/Trigger.

2. **ADC2 blockiert durch BLE**: Sobald Bluetooth aktiv ist, liefern
   ADC2-Kanaele (GPIO11-20) keine verlaesslichen Werte. Alle analogen
   Sensoren muessen auf ADC1 (GPIO1-10) liegen. Das limitiert die Anzahl
   verfuegbarer ADC-Kanaele auf 10.

3. **PSRAM und GPIO-Belegung**: Bei Modulen mit Octal-PSRAM (N8R8) sind
   zusaetzlich GPIO 33-37 fuer die PSRAM-Anbindung blockiert. Das reduziert
   die verfuegbaren GPIOs weiter. Bei N8R2 (Quad-PSRAM) sind diese Pins
   frei — daher wird N8R2 empfohlen.

4. **SD-Karte ueber SPI**: Die SD-Karte nutzt SPI (nicht SDIO), was die
   Schreibrate auf ca. 500 KB/s limitiert. Fuer Telemetrie-Logging ist das
   ausreichend (800 Byte/s bei 10 Sensoren), aber nicht fuer
   Audio- oder Bild-Streaming.

5. **Display-Refresh bei E-Paper**: E-Paper Partial-Refresh dauert ca.
   0.3s, Full-Refresh ca. 2s. Bei schnellen Wertaenderungen (RPM, Speed)
   ist E-Paper zu langsam. Daher wird fuer Phase 1 ein TFT-Display
   empfohlen, E-Paper nur fuer statische Werte (Kraftstoff, Tageskm,
   Uhrzeit).

---

## 6. Implementierungs-Reihenfolge

Die Implementierung erfolgt in 4 Phasen. Jede Phase baut auf der
vorhergehenden auf. Features innerhalb einer Phase koennen parallel
entwickelt werden.

### Phase 1: Core (ohne was nichts geht)

Ziel: Ein lauffaehiger Bordcomputer mit Grundfunktionen.

```
  Phase 1: Core-System
  ====================

  +-------------------------------------------------------+
  |  ESP32-S3 Boot, FreeRTOS, NVS, Config                 |
  |  +-------------------------------------------------+  |
  |  |  GPS-Task (UART, minmea, 10Hz)                  |  |
  |  |  -> Speed, Position, Zeit, Track                |  |
  |  +-------------------------------------------------+  |
  |  |  RPM-Task (PCNT, 500ms Fenster, Median-Filter)  |  |
  |  |  -> Drehzahl                                    |  |
  |  +-------------------------------------------------+  |
  |  |  Temp-Task (ADC1, NTC, Steinhart-Hart)          |  |
  |  |  -> Kuehlmitteltemperatur                       |  |
  |  +-------------------------------------------------+  |
  |  |  Display-Task (SPI, LVGL/GxEPD2, 1Hz)           |  |
  |  |  -> Speed, RPM, Temp, Zeit                      |  |
  |  +-------------------------------------------------+  |
  |  |  Logger-Task (SD-Karte, FATFS, Batch-Write)      |  |
  |  |  -> GPX-Track + CSV-Telemetrie                  |  |
  |  +-------------------------------------------------+  |
  |  |  Power-Task (ADC1, Bordspannung, Akku)           |  |
  |  |  -> Spannung, Tiefenentladungs-Schutz            |  |
  |  +-------------------------------------------------+  |
  +-------------------------------------------------------+
```

| Feature | Aufwand | Geschaetzte Zeit |
|---|---|---|
| 1.1 Geschwindigkeit (GPS) | Niedrig | 1-2 Tage |
| 1.2 Drehzahl (RPM) | Mittel | 2-3 Tage |
| 1.3 Kuehlmitteltemperatur | Niedrig | 1 Tag |
| 1.5 Uhrzeit (GPS) | Niedrig | 0.5 Tage |
| 1.6 Tripcmeter | Niedrig | 1 Tag |
| 1.7 GPX-Logging | Niedrig-Mittel | 1-2 Tage |
| 2.6 Bordspannung | Niedrig | 0.5 Tage |
| 8.1 GPX-Export | Niedrig | Inkl. in 1.7 |
| 9.3 BLE-GATT-Basis | Mittel | 2-3 Tage |
| 10.3 JSON-Config | Niedrig | 0.5 Tage |

Geschaetzte Gesamtzeit Phase 1: 2-3 Wochen

### Phase 2: Extended (erhoeht den Wert stark)

Ziel: Erweiterte Sensordaten und erste Synergie-Effekte.

```
  Phase 2: Extended-Features
  ==========================

  +-------------------------------------------------------+
  |  Zusaetzlich zu Phase 1:                             |
  |  +-------------------------------------------------+  |
  |  |  Sensor-Task (I2C: BMI270 + BMP388, 50Hz)       |  |
  |  |  -> Neigung, Hoehe, Umwelttemp                   |  |
  |  |  -> Complementary Filter (Roll/Pitch)            |  |
  |  +-------------------------------------------------+  |
  |  |  Gear-Detect (RPM / Speed -> Ratio-Match)        |  |
  |  |  -> Ganganzeige 1-5/N                            |  |
  |  +-------------------------------------------------+  |
  |  |  Fuel-Task (ADC1, OPV-Buffer, Lookup-Tabelle)    |  |
  |  |  -> Kraftstoffstand + Reichweite                 |  |
  |  +-------------------------------------------------+  |
  |  |  Tour-Task (Fahrtzeit, Pause, Statistiken)       |  |
  |  |  -> Fahrtzeit, Durchschnitt, Hoehenmeter         |  |
  |  +-------------------------------------------------+  |
  |  |  Diag-Task (Temp-Trend, Volt-Trend, Verbrauch)  |  |
  |  |  -> Langzeit-Logging + Alarme                    |  |
  |  +-------------------------------------------------+  |
  |  |  Wartungs-Reminder (NVS: km/Zeit-Zaehler)        |  |
  |  |  -> Oelwechsel, Reifen, Bremsen                  |  |
  |  +-------------------------------------------------+  |
  +-------------------------------------------------------+
```

| Feature | Aufwand | Geschaetzte Zeit |
|---|---|---|
| 2.1 Ganganzeige (berechnet) | Mittel | 2 Tage |
| 2.2 Neigungsanzeige (IMU) | Mittel | 2-3 Tage |
| 2.3 Hoehenmesser (BMP388) | Niedrig | 1 Tag |
| 2.4 Umwelttemperatur | Niedrig | 0.5 Tage |
| 1.4 Kraftstoffstand | Mittel-Hoch | 3-4 Tage |
| 2.7 Verbrauchs-Logging | Mittel | 2 Tage |
| 2.8 Fahrtzeit/Pause | Niedrig | 1 Tag |
| 2.9 Max-Recorder | Niedrig | 0.5 Tage |
| 5.1 Telemetrie-Recording | Mittel | 2 Tage |
| 5.3 Tour-Statistiken | Mittel | 2 Tage |
| 6.2 Kuehlmitteltemp-Trend | Niedrig | 0.5 Tage |
| 6.3 Bordspannung-Trend | Niedrig | 0.5 Tage |
| 6.5 Wartungs-Reminder | Niedrig | 1 Tag |
| 8.2 CSV-Telemetrie | Niedrig | Inkl. in 5.1 |
| 8.5 Statistik-Display | Niedrig | 1 Tag |

Geschaetzte Gesamtzeit Phase 2: 3-4 Wochen

### Phase 3: Smart (BLE + Smartphone)

Ziel: Smartphone-Anbindung, Navigation, Live-Daten.

```
  Phase 3: Smart-Features
  =======================

  +-------------------------------------------------------+
  |  Zusaetzlich zu Phase 2:                             |
  |  +-------------------------------------------------+  |
  |  |  BLE-GATT-Server (NimBLE, Custom Profile)       |  |
  |  |  -> Telemetry, Config, Nav, OTA Services         |  |
  |  +-------------------------------------------------+  |
  |  |  Nav-Parser (BLE JSON -> Display-Update)         |  |
  |  |  -> Pfeil, Distanz, Strassenname                 |  |
  |  +-------------------------------------------------+  |
  |  |  Companion-App / PWA                             |  |
  |  |  -> Routing-Backend, Config-UI, Live-Anzeige      |  |
  |  +-------------------------------------------------+  |
  |  |  POI-Logging (Taster -> GPS-Waypoint -> GPX)      |  |
  |  +-------------------------------------------------+  |
  |  |  JSON-API ueber BLE (Live-Daten, 1Hz)            |  |
  |  +-------------------------------------------------+  |
  |  |  File-Transfer ueber BLE (GPX/CSV Download)      |  |
  |  +-------------------------------------------------+  |
  +-------------------------------------------------------+
```

| Feature | Aufwand | Geschaetzte Zeit |
|---|---|---|
| 3.1 Turn-by-Turn Navigation | Mittel | 1-2 Wochen |
| 3.6 POI-Logging | Niedrig | 1 Tag |
| 8.3 JSON-API ueber BLE | Mittel | 3-4 Tage |
| 9.1 Companion-App / PWA | Mittel-Hoch | 1-2 Wochen |
| 9.5 PWA (Web-BLE) | Mittel | 1 Woche |
| 5.2 Export GPX/KML/CSV | Niedrig | 1 Tag |
| 5.5 Rueckweg-Navigation | Mittel | 2-3 Tage |

Geschaetzte Gesamtzeit Phase 3: 3-5 Wochen

### Phase 4: Advanced (Safety, Crash, erweiterte Diagnose)

Ziel: Sicherheitsfeatures, erweiterte Diagnose, optionale Sensoren.

```
  Phase 4: Advanced-Features
  ==========================

  +-------------------------------------------------------+
  |  Zusaetzlich zu Phase 3:                             |
  |  +-------------------------------------------------+  |
  |  |  Crash-Detector (IMU >7g + Neigung >75 Grad)    |  |
  |  |  -> 3-Stage Verifikation + Countdown            |  |
  |  +-------------------------------------------------+  |
  |  |  SOS-Handler (BLE -> Smartphone -> SMS/Anruf)   |  |
  |  +-------------------------------------------------+  |
  |  |  Sturz-Aufzeichnung (IMU Ring-Buffer 30s -> SD)  |  |
  |  +-------------------------------------------------+  |
  |  |  Oeldruck (VDO-Sender, analog, T-Stueck)        |  |
  |  |  -> Druck-Trend + Alarme                         |  |
  |  +-------------------------------------------------+  |
  |  |  TPMS (BLE-TPMS-Sensoren, passiv)                |  |
  |  |  -> Reifendruck + Temp pro Reifen                |  |
  |  +-------------------------------------------------+  |
  |  |  Diebstahl-Warnung (Deep-Sleep + Wake-on-Motion)|  |
  |  +-------------------------------------------------+  |
  |  |  Segment-Timing (GPS-Waypoint-Trigger)            |  |
  |  +-------------------------------------------------+  |
  |  |  Geofencing (Radius-Check)                       |  |
  |  +-------------------------------------------------+  |
  +-------------------------------------------------------+
```

| Feature | Aufwand | Geschaetzte Zeit |
|---|---|---|
| 4.1 Crash-Erkennung | Mittel | 1 Woche |
| 4.2 Automatic SOS | Hoch | 1-2 Wochen |
| 4.3 Sturz-Aufzeichnung | Mittel | 2-3 Tage |
| 4.4 Geofencing | Niedrig | 1 Tag |
| 4.5 Diebstahl-Warnung | Hoch | 1-2 Wochen |
| 2.5 Oeldruck (analog) | Hoch | 1 Woche (inkl. Einbau) |
| 6.1 Oeldruck-Trend | Mittel | 2 Tage |
| 6.4 Verbrauchs-Historie | Mittel | 2 Tage |
| 7.1 TPMS | Mittel | 2-3 Tage |
| 7.2 Hall-Sensor Vorderrad | Niedrig | 1-2 Tage |
| 7.3 GPS-Beschleunigung (0-100) | Niedrig | 1 Tag |
| 7.4 Lean-Angle-Max | Niedrig | 0.5 Tage |
| 7.5 Audio-Warnungen | Niedrig | 1 Tag |
| 7.6 LED-Status-Strip | Niedrig | 1 Tag |
| 5.4 Segment-Timing | Mittel | 2-3 Tage |
| 8.4 Web-Dashboard | Hoch | 1-2 Wochen |

Geschaetzte Gesamtzeit Phase 4: 4-6 Wochen

### Gesamt-Uebersicht

```
  Phase 1 (Core):          [Woche 1-3]   ====> Basis-System laeuft
  Phase 2 (Extended):     [Woche 4-7]   ====> Volle Sensorik aktiv
  Phase 3 (Smart):         [Woche 8-12]  ====> Smartphone-Anbindung
  Phase 4 (Advanced):      [Woche 13-18] ====> Safety + Diagnose
```

Die Phasen sind nicht strikt sequenziell: Phase 2-Features koennen
beginnen sobald die jeweilige Abhaengigkeit aus Phase 1 steht. Phase 3
kann parallel zu den spaeten Phase 2-Features starten, da die BLE-Stack-
Initialisierung bereits in Phase 1 enthalten ist.

---

## 7. Hardware-Kosten-Uebersicht

| Komponente | Typ | Preis (EUR) | Phase | Interface |
|---|---|---|---|---|
| ESP32-S3-WROOM-1 N8R2 | MCU | 5-10 | 1 | — |
| u-blox NEO-M9N | GPS | 15-20 | 1 | UART |
| NTC 10k B3950 | Kuehlmitteltemp | 2-5 | 1 | ADC1 |
| Spannungsteiler (Widerstaende) | Bordspannung | 0.50 | 1 | ADC1 |
| Optokoppler PC817/6N137 | RPM | 2-5 | 1 | PCNT |
| MicroSD-Modul | Speicher | 2 | 1 | SPI |
| MicroSD-Karte 16GB | Speicher | 5 | 1 | SPI |
| TFT 3.5" 480x320 (1000 nits) | Display | 15-20 | 1 | SPI2 |
| BMI270 | IMU | 4-7 | 2 | I2C |
| BMP388 | Barometer | 3-9 | 2 | I2C |
| OPV LM358 + Widerstaende | Kraftstoff-Buffer | 1-2 | 2 | ADC1 |
| Aftermarkt-Tankgeber | Kraftstoffstand | 10-20 | 2 | ADC1 |
| Taster (wasserdicht) | POI/Control | 1 | 3 | GPIO |
| Piezo-Buzzer | Audio-Warnung | 1 | 4 | GPIO |
| WS2812B-Strip | LED-Warnung | 2-3 | 4 | GPIO |
| VDO Oeldrucksender 0-10 bar | Oeldruck | 20-30 | 4 | ADC1 |
| M10x1.0 -> 1/8 NPT Adapter | Oeldruck | 5-10 | 4 | — |
| T-Stueck fuer Oeldruck | Oeldruck | 5-10 | 4 | — |
| BLE-TPMS-Sensoren (Paar) | Reifendruck | 20-30 | 4 | BLE |
| Hall-Sensor A3144 + Magnet | Radgeschwindigkeit | 2-5 | 4 | PCNT |
| MP1584EN Buck-Modul | Power 12V->5V | 1-2 | 1 | — |
| HT7333 LDO | Power 5V->3.3V | 0.50 | 1 | — |
| 18650 Akku + TP4056 + MT3608 | Backup-Power | 7-10 | 1 | — |
| TVS SMAJ15A + MOV | Ueberspannung | 1-2 | 1 | — |
| ASA-Filament (Gehaeuse) | Gehaeuse | 5-10 | 1 | — |
| Kupferfolie (EMV) | Abschirmung | 5-10 | 1 | — |
| Cable-Glands PG7/PG9 | Kabeldurchfuehrung | 2-3 | 1 | — |
| GORE PolyVent XS | Druckausgleich | 3-5 | 1 | — |

| Ausbaustufe | Gesamtkosten |
|---|---|
| Phase 1 (Core) | ~50-80 EUR |
| Phase 1+2 (Extended) | ~70-110 EUR |
| Phase 1-3 (Smart) | ~75-120 EUR |
| Phase 1-4 (Vollausbau) | ~130-170 EUR |

---

## 8. Empfehlung und Begruendung

### 8.1 Top-Prioritaeten (sofort umsetzen)

1. **GPS + Geschwindigkeit + Tripcmeter + GPX-Logging**: Das Fundament
   des Bordcomputers. Ohne GPS ist das System nutzlos. Der NEO-M9N
   liefert alle noetigen Daten (Position, Speed, Zeit, Heading) in
   einem Modul. Die Implementierung ist dank minmea-Bibliothek
   unkompliziert.

2. **RPM ueber CDI-Pickup**: Der zweite Must-Have. Die PCNT-Hardware
   des ESP32-S3 macht die Zaehlung trivial, die Herausforderung liegt
   in der EMV-Sicheren Optokoppler-Schaltung. Der Pickup liefert
   1 Puls pro Umdrehung — bei 8000 RPM sind das 133 Hz, problemlos
   fuer PCNT.

3. **Bordspannung**: Mit 2 Widerstaenden implementierbar, bietet
   sofortigen diagnostischen Wert (Generator-Health) bei minimalem
   Aufwand. Die PD06 ist bekannt fuer Regler-Ausfaelle.

4. **Kuehlmitteltemperatur**: Der NTC ist billig (2 EUR) und der
   Steinhart-Hart-Algorithmus ist gut dokumentiert. Die
   Ueberhitzungswarnung ist besonders bei Bergfahrten wichtig.

5. **BLE-GATT-Basis**: Die NimBLE-Initialisierung und das
   GATT-Framework sollten bereits in Phase 1 angelegt werden, auch
   wenn die Smart-Features erst in Phase 3 kommen. Das erspart
   spaeter Refactoring.

### 8.2 Wichtigste Synergie: Ganganzeige ohne zusaetzliche Hardware

Die Ganganzeige aus RPM/Speed-Ratio ist das beste Beispiel fuer einen
Synergie-Effekt: Sobald RPM (Feature 1.2) und Speed (Feature 1.1)
verfuegbar sind, kann der Gang berechnet werden — ohne zusaetzliche
Hardware, ohne mechanischen Eingriff, ohne Sensor. Die
Uebersetzungsverhaeltnisse der PD06 sind aus dem Service Manual bekannt
(Dokument 04, Abschnitt 2). Die Implementierung besteht aus ca. 30
Zeilen C-Code.

### 8.3 Wichtigste Korrektur: Kraftstoffstand

Die urspruengliche Roadmap (Dokument 06) geht davon aus, dass die PD06
einen Tankgeber mit Widerstandssignal hat. Dokument 04 stellt klar: Die
PD06 hat einen Vakuum-Petcock mit Reserve-Stellung, keinen elektronischen
Tankgeber. Fuer das Feature 1.4 (Kraftstoffstand + Reichweite) muss ein
Aftermarket-Schwimmer-Sensor durch die Tankoeffnung installiert werden.

Das aendert die Priorisierung: Kraftstoffstand bleibt ein Must-Have,
rutscht aber an das Ende von Phase 2, weil der Einbau mechanischen
Aufwand erfordert und die Kalibrierung (2-Liter-Schritte) zeitaufwaendig
ist. Bis dahin kann die Reserve-Warnung (Petcock auf RES-Stellung)
als Notloesung dienen.

### 8.4 Display-Entscheidung: TFT statt E-Paper als Primaerdisplay

Die Research-Dokumente 02 und 07 zeigen einen Paradigmenwechsel in der
Display-Wahl:

- Dokument 02 empfiehlt E-Paper 2.9" (296x128) als Primaerdisplay
- Dokument 07 empfiehlt TFT 3.5" (480x320) als Primaerdisplay

Die Begruendung fuer die Aenderung: E-Paper Partial-Refresh dauert 0.3s.
Bei schnellen Wertaenderungen (RPM aendert sich bei Schaltvorgang um
2000 RPM in <0.5s) ist das zu langsam. Ein TFT mit 15-20 FPS
(bei 80 MHz SPI) kann alle Werte fluessig anzeigen.

E-Paper bleibt wertvoll fuer statische Werte (Kraftstoffstand,
Tageskilometer, Uhrzeit), die sich nur selten aendern. Daher die
Empfehlung:
- Phase 1: TFT 3.5" (480x320, 1000+ nits) als Primaerdisplay
- Phase 2: 2.13" E-Paper als Sekundaerdisplay fuer statische Werte

### 8.5 Won't-Have-Begruendungen

**7.7 Zweites Display am Lenkerende**: Platzmangel am Lenker, zu viele
Kabel, komplexe Software mit zwei Render-Pfaden. Das Hauptdisplay
sollte alle Warnungen prominent genug anzeigen.

**7.8 Sprachausgabe via BT-Kopfhoerer**: Der ESP32-S3 unterstuetzt kein
Bluetooth Classic (A2DP). TTS auf ESP32 ist ressourcenintensiv. Die
Smartphone-Route (App macht TTS, sendet an BT-Kopfhoerer) ist
vorzuziehen, aber dann kein Bordcomputer-Feature mehr.

---

## 9. Zusammenfassung

Die aktuelle Datenlage — bestehend aus 7 Research-Dokumenten mit ueber
34.000 Woertern technischer Recherche — ermoeglicht die Umsetzung von
42 der 48 in der Roadmap genannten Features. Drei Features
(Kraftstoffstand, analoger Oeldruck, Kuehlmitteltemperatur) erfordern
mechanische Nachruestungen am Fahrzeug. Zwei Features (zweites Display,
Sprachausgabe) werden als Won't-Have zurueckgestellt.

Die Hardware-Basis (ESP32-S3 + NEO-M9N + BMI270 + BMP388 + SD-Karte +
BLE) ist ausreichend fuer alle Phasen. Die Gesamtkosten liegen zwischen
50 EUR (Core) und 170 EUR (Vollausbau).

Die wichtigste Erkenntnis aus der Querschnittsanalyse: Die Synergie-Effekte
machen den Bordcomputer wertvoll. Einzelne Sensordaten sind interessant,
aber die Kombination aus RPM + Speed -> Ganganzeige, Bordspannung + RPM
-> Generator-Diagnose, und GPS + IMU -> Kurvenanalyse schafft einen
Mehrwert, der ueber die Summe der Teile hinausgeht.

Die empfohlene Implementierungs-Reihenfolge (Phase 1-4) stellt sicher,
dass jede Phase fuer sich allein schon einen nutzbaren Bordcomputer
ergibt: Phase 1 ist ein funktionierendes GPS-Tripmeter mit RPM und
Temperatur, Phase 2 fuegt Neigung, Hoehe und Verbrauch hinzu, Phase 3
macht das System vernetzt, und Phase 4 bringt Sicherheit.

---

## Quellen

Dieses Dokument verknuepft die Erkenntnisse aus:
- 01-sensoren-fahrzeugdaten.md (Sensor-Schaltungen, Signal-Conditioning)
- 02-hardware-deep-dive.md (MCU, GPS, IMU, Baro, Power, Pin-Map)
- 03-software-stack.md (ESP-IDF, FreeRTOS, Treiber, BLE, OTA)
- 04-pd06-technische-daten.md (Fahrzeugdaten, Uebersetzungen, Service-Manual)
- 05-gehaeuse-einbau-emv.md (Gehaeuse, Wasserdichtigkeit, Vibration, EMV)
- 06-feature-roadmap.md (Alle Features mit Bewertung)
- 07-display-navigation-smartphone.md (Multi-Display, BLE, Companion-App)

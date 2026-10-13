# Master-Plan: Konkrete Umsetzungs-Roadmap mit Kosten-Nutzen-Analyse

> Stand: 13.10.2026
> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986-2000, rein mechanisch, keine ECU
> Plattform: ESP32-S3-WROOM-1 (N8R2)
> Vorausgehende Dokumente: 01 bis 08 in docs/research/

## 0. Zusammenfassung

Dieser Master-Plan bündelt die Erkenntnisse aus den Forschungsdokumenten 01 bis 07
in einen konkreten, kosteneffizienten Umsetzungsplan. Er definiert drei
Ausbau-Stufen (Basic, Standard, Premium), einen fünfphasigen Entwicklungsplan
über zehn Wochen, ein vollständiges Diagnose- und Support-Konzept, eine
Test-Strategie und eine Risikomatrix.

Das Endprodukt soll sich anfühlen wie ein Premium-Produkt eines kalifornischen
Technologieherstellers: reibungslos, durchdacht, schön, intuitiv. Qualität
steht an vorderster Front — keine Kompromisse bei Verarbeitung, Design und
Funktion. Die Architektur ist modular: jede Komponente ist unabhängig
austauschbar, jeder Sensor ist ein eigenes Software-Modul, jede Erweiterung
fügt sich nahtlos in das bestehende System ein.

Die Gesamtkosten für den Vollausbau (Premium-Stufe) liegen bei ca. 195 EUR
für alle Hardware-Komponenten. Die Basic-Stufe ist ab ca. 72 EUR realisierbar.
Das Preis-Leistungs-Verhältnis ist bei der Standard-Stufe am besten: sie deckt
alle Must- und die wichtigsten Should-Features ab für ca. 125 EUR.

---

## 1. Kosten-Nutzen-Analyse

### 1.1 Bill of Materials (BOM)

Die folgende Tabelle listet alle Komponenten mit konkreten Preisen in EUR auf.
Preise sind Richtwerte (Stand 2026) und beziehen sich auf Einzelstückpreise bei
AliExpress, Amazon, Mouser oder DigiKey. Bei Stückzahlen ab 10 fallen die Preise
typischerweise um 20-40%.

Jede Komponente hat einen Budget- und einen Premium-Vorschlag. Budget-Teile
sind ausreichend für die Basic-Stufe, Premium-Teile für die Premium-Stufe.
Die Empfehlung gibt an, welche Option das beste Preis-Leistungs-Verhältnis
bietet — "Premium" bedeutet nicht immer "teuer", sondern "bester Wert".

#### 1.1.1 Kern-System

| # | Komponente | Budget-Option (EUR) | Premium-Option (EUR) | Empfehlung | Interface |
|---|---|---|---|---|---|
| 1 | MCU-Modul | ESP32-S3-DevKitC-1 (5) | ESP32-S3-WROOM-1 N8R2 Custom PCB (18) | Premium (Custom PCB ab Phase 2) | — |
| 2 | GPS-Modul | ATGM336H (3) | u-blox NEO-M9N (30) | Premium | UART |
| 3 | GPS-Antenne | Passive Patch (2) | Active Patch 25x25 mit Magnetfuß (8) | Premium | — |
| 4 | Display (primär) | ILI9341 2.4" 320x240 (6) | Waveshare 3.5" IPS 1000 nits 480x320 (18) | Premium | SPI |
| 5 | Display (sekundär) | — | E-Paper 2.13" 250x122 (12) | Standard-Stufe | SPI |
| 6 | SD-Karten-Modul | MicroSD Breakout (2) | MicroSD Breakout (2) | Budget (kein Unterschied) | SPI |
| 7 | MicroSD-Karte | 8GB Class 10 NoName (3) | SanDisk Ultra 16GB Class 10 (6) | Premium | — |

#### 1.1.2 Sensoren

| # | Komponente | Budget-Option (EUR) | Premium-Option (EUR) | Empfehlung | Interface |
|---|---|---|---|---|---|
| 8 | IMU | MPU6050 (2) | BMI270 (5) | Premium | I2C |
| 9 | Barometer | BMP280 (2) | BMP388 (5) | Premium | I2C |
| 10 | Kühlmitteltemp | NTC 10k Thermistor (1) | DS18B20 wasserdicht (3) | Premium | 1-Wire |
| 11 | Öldrucksensor | — (Originalschalter, digital) | VDO 0-5 bar analog (25) | Standard-Stufe | ADC |
| 12 | Kraftstoffstand | Spannungsteiler am Originalgeber (1) | Spannungsteiler + OpAmp (3) | Premium | ADC |
| 13 | Bordspannung | Spannungsteiler 100k/33k (1) | Spannungsteiler + RC-Filter (2) | Budget (reicht) | ADC |
| 14 | Zündimpuls (RPM) | PC817 Optokoppler (1) | PC817 + Zener + RC-Filter (3) | Premium | GPIO/IRQ |
| 15 | Umwelttemp | BMP280 (entfallen) | DS18B20 separat (3) | Standard-Stufe | 1-Wire |
| 16 | Hall-Sensor (Rad) | A3144 (1) | A3144 + Magnet (2) | Could-Feature | GPIO/PCNT |

#### 1.1.3 Power-Management

| # | Komponente | Budget-Option (EUR) | Premium-Option (EUR) | Empfehlung | Interface |
|---|---|---|---|---|---|
| 17 | Buck-Converter 12V->5V | LM2596 Modul (2) | MP1584EN Modul (2) + TVS-Diode SMAJ15A (1) | Premium | — |
| 18 | LDO 5V->3.3V | AMS1117-3.3 (0.15) | HT7333 (0.50) | Premium | — |
| 19 | Backup-Akku | — | 18650 NCR18650B 3400mAh (6) | Standard-Stufe | — |
| 20 | Akku-Lademodul | — | TP4056 mit Schutz (0.50) | Standard-Stufe | — |
| 21 | Boost-Converter 3.7V->5V | — | MT3608 (0.80) | Standard-Stufe | — |
| 22 | Schottky-Diode Power-Path | — | 1N5822 (0.10) | Standard-Stufe | — |

#### 1.1.4 Konnektivität & Erweiterung

| # | Komponente | Budget-Option (EUR) | Premium-Option (EUR) | Empfehlung | Interface |
|---|---|---|---|---|---|
| 23 | BLE | ESP32-S3 eingebaut (0) | ESP32-S3 eingebaut (0) | — | BLE 5.0 |
| 24 | Piezo-Buzzer | Passiv (1) | Aktiv + Passiv kombiniert (2) | Budget | GPIO |
| 25 | LED-Status-Strip | — | WS2812B 4-LED (3) | Could-Feature | GPIO |
| 26 | TPMS-Sensoren | — | ZEEPIN TP630 Paar (25) | Could-Feature | BLE |
| 27 | Relais-Modul 4-Kanal | SRD-12V (4) | SRD-12V + Optokoppler (5) | Standard-Stufe | GPIO |
| 28 | MOSFET PWM Heizgriffe | IRF520 (2) | IRLZ44N + TC4427 Gate-Driver (4) | Premium | GPIO/PWM |

#### 1.1.5 Gehäuse & Mechanik

| # | Komponente | Budget-Option (EUR) | Premium-Option (EUR) | Empfehlung | Interface |
|---|---|---|---|---|---|
| 29 | Gehäuse Material | PETG (3 Filament) | ASA (5 Filament) | Premium | — |
| 30 | O-Ring Dichtung | NBR (1) | Silikon (2) | Premium | — |
| 31 | Kabeldurchführungen | PG7 x4 (4) | PG7 x4 + PG9 x2 (6) | Premium | — |
| 32 | Druckausgleichsmembran | — | GORE PolyVent XS (8) | Premium | — |
| 33 | Lenkerhalterung | U-Bolt Schelle (2) | RAM-Mount Kugelgelenk (8) | Premium | — |
| 34 | Schrauben/Muttern | M3 Sortiment (2) | M3 Edelstahl (3) | Budget | — |
| 35 | Sicherungen + Halter | Inline 5A (1) | ATO Sicherungskasten 6-fach (5) | Standard | — |

### 1.2 Gesamtkostenschätzung: Drei Ausbau-Stufen

#### Basic-Stufe (Phase 1-2)

Funktionierendes Kernsystem mit allen Must-Features: Geschwindigkeit, RPM,
Kühlmitteltemperatur, Kraftstoffstand, Uhrzeit, Tripcmeter, GPX-Logging.
Keine Backup-Akku-Versorgung, kein BLE, keine Smartphone-Integration,
kein Öldruck, kein TFT-Display, DevKitC-1 auf Perfboard.

| Kategorie | Summe (EUR) |
|---|---|
| Kern-System (DevKit, ATGM336H, ILI9341, SD) | 19 |
| Sensoren (MPU6050, BMP280, NTC, PC817, Kraftstoff/Spannungsteiler) | 7 |
| Power (LM2596, AMS1117) | 2 |
| Gehäuse (PETG, NBR, PG7, U-Bolt, Sicherung) | 9 |
| Verbrauchsmaterial (Kabel, Widerstände, Kondensatoren) | 5 |
| **Gesamt Basic** | **~42** |
| Puffer 15% für Versand, Preisschwankungen | 6 |
| **Budget Basic (inkl. Puffer)** | **~48** |

Hinweis: Die Basic-Stufe ist ein Prototyp auf Perfboard/DevKit. Sie ist
voll funktionsfähig, aber nicht wasserdicht, nicht vibrationssicher und
nicht für den Dauereinsatz am Motorrad gedacht. Sie dient der
Software-Entwicklung und dem Funktionstest.

#### Standard-Stufe (Phase 1-4)

Vollwertiges Bordcomputer-System mit allen Must- und Should-Features:
BLE-Anbindung, Smartphone-Integration, TFT-Display, Backup-Akku,
Öldruck, E-Paper-Zweitdisplay, Relais-Steuerung. Custom PCB oder
saubere Perfboard-Lösung.

| Kategorie | Summe (EUR) |
|---|---|
| Kern-System (WROOM-1 N8R2, NEO-M9N, Waveshare 3.5" IPS, SD) | 56 |
| Sensoren (BMI270, BMP388, DS18B20 x2, VDO Öldruck, PC817+, OpAmp) | 42 |
| Power (MP1584+TVS, HT7333, 18650, TP4056, MT3608, Diode) | 11 |
| Konnektivität (Buzzer, Relais, IRLZ44N+TC4427) | 8 |
| Gehäuse (ASA, Silikon, PG7+PG9, GORE Vent, RAM-Mount) | 28 |
| Verbrauchsmaterial | 5 |
| **Gesamt Standard** | **~150** |
| Puffer 15% | 23 |
| **Budget Standard (inkl. Puffer)** | **~173** |

#### Premium-Stufe (Vollausbau, Phase 1-5)

Alles aus Standard plus: TPMS, LED-Status-Strip, Hall-Sensor am Vorderrad,
Crash-Detection mit SOS, zweiteilige Custom-PCB-Lösung, professionelle
Gehäuse-Fertigung mit GORE-Vent, alle Nice-to-Have-Features.

| Kategorie | Summe (EUR) |
|---|---|
| Standard-Stufe (siehe oben) | 150 |
| TPMS-Sensoren (ZEEPIN TP630) | 25 |
| LED-Status-Strip (WS2812B) | 3 |
| Hall-Sensor + Magnete | 2 |
| E-Paper-Zweitdisplay (2.13") | 12 |
| Custom PCB (JLCPCB 5-Stück, Bestückung) | 20 |
| Premium-Komponenten (Edelstahl, zusätzliche Sicherungen) | 5 |
| **Gesamt Premium** | **~217** |
| Puffer 15% | 33 |
| **Budget Premium (inkl. Puffer)** | **~250** |

### 1.3 Kosten-Nutzen-Bewertung: Was lohnt sich?

Die folgende Bewertung ordnet jede Komponente nach ihrem Kosten-Nutzen-Verhältnis.
"Nutzen" ist die Funktionssteigerung für den Rider. "Kosten" sind die
Mehrpreise gegenüber der Budget-Option.

#### Sehr hohes Kosten-Nutzen-Verhältnis (unbedingt machen)

| Investition | Mehrpreis (EUR) | Nutzen | Begründung |
|---|---|---|---|
| NEO-M9N statt ATGM336H | +27 | 4-Konstellation, 25Hz, 2m CEP | Der wichtigste Sensor. 27 EUR für 10x besseres GPS ist unschlagbar. |
| BMI270 statt MPU6050 | +3 | 10x weniger Strom, weniger Rauschen | 3 EUR für einen massiven Sprung in Qualität. |
| BMP388 statt BMP280 | +3 | FIFO, 0.5m Höhenauflösung | 3 EUR für 2x bessere Höhenmessung. |
| DS18B20 statt NTC | +2 | Kalibriert, digital, wasserdicht | 2 EUR für 0.5°C Genauigkeit und keine Kalibrierung nötig. |
| HT7333 statt AMS1117 | +0.35 | 100x weniger Rauschen | Fast kostenlos, aber entscheidend für ADC-Qualität. |
| ASA statt PETG | +2 | UV-beständig, 100°C TG | 2 EUR für ein Gehäuse, das nicht in der Sonne schmilzt. |
| Silikon-O-Ring statt NBR | +1 | -40 bis +200°C, UV-stabil | 1 EUR für dauerhafte Dichtigkeit. |
| GORE PolyVent | +8 | Kondenswasser-Vermeidung | Verhindert Korrosion. Einmalige Investition. |
| MP1584+TVS statt LM2596 | +1 | Höhere Effizienz, Lastabwurf-Schutz | 1 EUR für 10% weniger Wärme und Schutz gegen 40V-Spikes. |
| SanDisk Ultra statt NoName | +3 | Zuverlässigkeit | SD-Kartenausfall = Datenverlust. 3 EUR für Sicherheit. |

#### Hohes Kosten-Nutzen-Verhältnis (empfohlen)

| Investition | Mehrpreis (EUR) | Nutzen | Begründung |
|---|---|---|---|
| Waveshare 3.5" IPS 1000 nits | +12 | Sonnenlesbarkeit | 1000 nits ist das Minimum für direktes Sonnenlicht am Lenker. |
| E-Paper 2.13" als Zweitdisplay | +12 | Statische Werte immer sichtbar | TFT aus = Kraftstoff/Trip immer noch da. Low Power. |
| 18650 Backup-Akku | +8 | Power-Loss-Schutz, Diebstahlschutz | 8 EUR für Datensicherheit und Tiefgaragen-Tracking. |
| VDO Öldrucksensor | +25 | Analoge Öldruckanzeige | Teuerster Einzelposten. Lohnt sich für Motoren über 50.000 km. |
| IRLZ44N+TC4427 für Heizgriffe | +2 | Stufenlose PWM-Regelung | 2 EUR für komfortable Heizgriff-Steuerung. |

#### Mittleres Kosten-Nutzen-Verhältnis (optional)

| Investition | Mehrpreis (EUR) | Nutzen | Begründung |
|---|---|---|---|
| TPMS-Sensoren | +25 | Reifendrucküberwachung | Nützlich, aber Kontrolle beim Tanken reicht auch. |
| WS2812B LED-Strip | +3 | Visuelle Warnungen | Schick, aber Display+Buzzer reicht auch. |
| Hall-Sensor am Vorderrad | +2 | GPS-Fallback bei Tunnel | Selten nötig. GPS-Abdeckung ist gut. |
| RAM-Mount Kugelgelenk | +6 | Einstellbare Neigung | Nicer to have, U-Bolt funktioniert auch. |
| Custom PCB | +20 | Kompaktheit, Professionalität | Ab Phase 2 sinnvoll. Perfboard funktioniert für Prototyp. |

#### Wo kann man sparen ohne Qualität zu verlieren?

| Komponente | Budget-Option reicht | Begründung |
|---|---|---|
| SD-Karten-Modul | 2 EUR Breakout | Es gibt keinen Qualitätsunterschied bei SPI-Breakouts. |
| Bordspannung-Spannungsteiler | 1 EUR Widerstände | Präzisionswiderstände sind Verschwendung. |
| Piezo-Buzzer | 1 EUR passiv | Aktiv hat keine Vorteile. PWM auf passiv ist flexibler. |
| M3-Schrauben | 2 EUR Sortiment | Edelstahl ist nice-to-have, nicht nötig. |
| Relais-Modul | 4 EUR Standard | Optokoppler ist bereits onboard bei den meisten Modulen. |

### 1.4 Kosten-Entscheidungs-Matrix: Welche Stufe für wen?

| Kriterium | Basic | Standard | Premium |
|---|---|---|---|
| Preis (inkl. Puffer) | ~48 EUR | ~173 EUR | ~250 EUR |
| Must-Features | Alle | Alle | Alle |
| Should-Features | Keine | Alle | Alle |
| Could-Features | Keine | Keine | Alle sinnvollen |
| Wasserdicht | Nein | Ja (IP67) | Ja (IP68 mit GORE-Vent) |
| Smartphone-Integration | Nein | Ja (BLE, PWA) | Ja (BLE, native App-Option) |
| Backup-Akku | Nein | Ja (18650) | Ja (18650) |
| Crash-Detection | Nein | Software-ready | Voll implementiert |
| Dauereinsatz am Motorrad | Nein (Prototyp) | Ja | Ja |
| Empfehlung für | Entwickler-Phase | Erste Saison | Langzeit-Einsatz |

**Klare Empfehlung:** Die Standard-Stufe ist das beste Preis-Leistungs-
Verhältnis. Sie deckt alle Must- und Should-Features ab, ist
wasserdicht, hat Backup-Akku und Smartphone-Integration. Die
Basic-Stufe ist ein Entwicklungs-Prototyp, die Premium-Stufe ist
für Enthusiasten, die das Maximum herausholen wollen.

---

## 2. Phasen-Plan

Der Entwicklungsplan umfasst fünf Phasen über zehn Wochen. Jede Phase
baut auf der vorherigen auf. Am Ende jeder Phase steht ein funktions-
fähiger Meilenstein, der unabhängig testbar ist.

### 2.1 Phasen-Übersicht

| Phase | Zeitraum | Fokus | Meilenstein | Stufe |
|---|---|---|---|---|
| 1 | Woche 1-2 | Core-System | GPS-Track-Logger läuft | Basic |
| 2 | Woche 3-4 | Fahrzeug-Sensoren | Alle Must-Sensoren liefern Daten | Basic+ |
| 3 | Woche 5-6 | UI/UX & Display | Premium-Design auf TFT | Standard |
| 4 | Woche 7-8 | Connectivity | BLE + Smartphone-App | Standard |
| 5 | Woche 9-10 | Safety & Diagnose | Crash-Detection + DTC-System | Premium |

### 2.2 Gantt-Chart (ASCII)

```
Phase / Woche          W1   W2   W3   W4   W5   W6   W7   W8   W9   W10
═══════════════════════════════════════════════════════════════════════════
                       │    │    │    │    │    │    │    │    │    │
P1: Core-System        ├────┤    │    │    │    │    │    │    │    │
  ESP32 Setup          ████ │    │    │    │    │    │    │    │    │
  GPS-Treiber               ████│    │    │    │    │    │    │    │
  SD-Logging                ████│    │    │    │    │    │    │    │
  Basis-Display             ████│    │    │    │    │    │    │    │
                       │    │    │    │    │    │    │    │    │    │
P2: Fahrzeug-Sensoren            ├────┤    │    │    │    │    │    │
  RPM (Zündimpuls)              ████ │    │    │    │    │    │    │
  Kühlmitteltemp                ████ │    │    │    │    │    │    │
  Kraftstoffstand                   ████│    │    │    │    │    │
  Öldruck                            ████│    │    │    │    │    │
  Bordspannung                       ████│    │    │    │    │    │
                       │    │    │    │    │    │    │    │    │    │
P3: UI/UX & Display                     ├────┤    │    │    │    │
  LVGL-Integration                      ████ │    │    │    │    │
  Dashboard-Layout                           ████│    │    │    │
  Animationen/Übergänge                      ████│    │    │    │
  E-Paper-Zweitdisplay                       ████│    │    │    │
                       │    │    │    │    │    │    │    │    │    │
P4: Connectivity                                  ├────┤    │    │
  BLE-GATT-Server                                 ████ │    │    │
  Companion-App (PWA)                                  ████│    │
  Turn-by-Turn Navigation                              ████│    │
  OTA-Update                                           ████│    │
                       │    │    │    │    │    │    │    │    │    │
P5: Safety & Diagnose                                           ├────┤
  Crash-Detection                                              ████ │
  DTC-System                                                        │
  Self-Test                                                         │
  Wartungs-Intervalle                                               │
  QS-Tests & Benchmarks                                             │
═══════════════════════════════════════════════════════════════════════════
  Meilenstein:         M1         M2         M3         M4         M5
```

### 2.3 Phasen-Detail

#### Phase 1 (Woche 1-2): Core-System

**Ziel:** Ein lauffähiger ESP32-S3 mit GPS, SD-Logging und Basis-Display.
Am Ende der Phase 1 kann der Bordcomputer GPS-Tracks aufzeichnen und
auf einem Display anzeigen.

**Woche 1: Setup und GPS**

- ESP-IDF-Projektstruktur aufsetzen (nicht Arduino — volle Kontrolle)
- FreeRTOS-Task-Architektur definieren: GPS-Task, Display-Task,
  Logger-Task, Main-Task
- GPS-Treiber implementieren: UART-Initialisierung für NEO-M9N,
  NMEA-Parsing (GGA, RMC, VTG), UBX-NAV-PVT-Parsing für 5-10 Hz Update
- NVS-Konfiguration: settings.json auf SD-Karte laden, Parameter
  in NVS persistieren
- Erste Display-Ausgabe: Text auf ILI9341 oder Waveshare 3.5" IPS
  (Geschwindigkeit, GPS-Fix-Status, Sat-Anzahl, Uhrzeit)

**Woche 2: SD-Logging und Basis-Funktionen**

- SD-Karten-Treiber mit FATFS einrichten (SPI-Mode, SPI3/HSPI)
- Ringbuffer-basiertes Logging: 4KB Buffer, Batch-Write alle 10 Sekunden
- GPX-Track-Writer: /tracks/YYYYMMDD_HHMMSS.gpx
- CSV-Telemetrie-Writer: /telemetry/YYYYMMDD_HHMMSS.csv
- Tripcmeter: Haversine-Distanz aus GPS-Positionen, Trip A/B, ODO in NVS
- Pause-Erkennung: Geschwindigkeit < 2 km/h für > 60 Sekunden = Pause
- Uhrzeit: GPS-Zeit mit CET/CEST-Zeitzonenkonvertierung

**Meilenstein M1:** Bordcomputer zeichnet GPX-Tracks auf, zeigt
Geschwindigkeit und Uhrzeit an. Tests mit Fahrrad oder zu Fuß.

**Abhängigkeiten:** Keine (erste Phase).

**Risiken:** SPI-Pin-Konflikte zwischen Display und SD-Karte. Lösung:
getrennte SPI-Busse (SPI2 für Display, SPI3 für SD). UBX-Protokoll-
Dokumentation ggf. lückenhaft — auf NMEA als Fallback zurückfallen.

#### Phase 2 (Woche 3-4): Fahrzeug-Sensoren

**Ziel:** Alle Must-Sensoren liefern valide Daten. Der Bordcomputer
zeigt Geschwindigkeit, RPM, Kühlmitteltemperatur, Kraftstoffstand
und Bordspannung an.

**Woche 3: RPM und Kühlmitteltemperatur**

- RPM-Sensor: PC817 Optokoppler am Zündkabel, Hardware-Interrupt
  (FALLING-Edge), Moving-Average-Filter (5-10 Samples), Entprellung
  (Mindestabstand 2000 µs)
- Signal-Konditionierung prüfen: Zener-Diode + RC-Filter zum Schutz
  vor HV-Spikes der Zündspule
- Kühlmitteltemperatur: DS18B20 in T-Stück im Kühlmittelschlauch,
  DallasTemperature-Bibliothek, Lesen alle 1-2 Sekunden
- Alarm-Schwellen: >110°C Warnung, >115°C Kritisch
- Bordspannung: Spannungsteiler 100k/33k an ADC1, Mittelwert über
  50 Samples, Warnung bei < 11.5V oder > 15V

**Woche 4: Kraftstoffstand und Öldruck**

- Kraftstoffstand: ADC-Lesung am Originalgeber, Kennlinien-Kalibrierung
  (Widerstand -> Füllstand, nicht-linear), Reichweitenberechnung
  aus Verbrauch (gleitender Durchschnitt über 50 km)
- Öldruck: Falls analoger VDO-Sensor verbaut: ADC-Lesung,
  Widerstand-zu-Druck-Umrechnung, Warnung bei < 0.5 bar Leerlauf
  oder < 1.5 bar bei höherer Drehzahl
- Falls nur Originalschalter: Warnung bei Öldruckabfall (digital)
- Trend-Logging: Öldruck, Kühlmitteltemp, Bordspannung alle 10-30
  Sekunden auf SD-Karte
- Ganganzeige (Should): Verhältnis RPM/Geschwindigkeit, Lern-Modus
  für 5 Gänge, NVS-Speicherung

**Meilenstein M2:** Alle Must-Sensoren liefern valide Daten. Erste
Testfahrt mit voller Sensorik. Vergleich mit Referenzwerten
(Tacho, Thermometer).

**Abhängigkeiten:** Phase 1 (Task-Architektur, SD-Logging muss laufen).

**Risiken:** Zündspulen-EMI stört andere Sensoren. Lösung: abgeschirmtes
Kabel für RPM-Pickup, räumliche Trennung, RC-Filter. Kraftstoffgeber-
Kennlinie muss empirisch kalibriert werden (Tank leeren/füllen in
Schritten). Öldrucksensor-Einbau erfordert mechanische Modifikation
am Ölkreislauf — ggf. auf Phase 5 verschieben wenn zu aufwendig.

#### Phase 3 (Woche 5-6): UI/UX und Display

**Ziel:** Premium-Design auf dem TFT-Display. Die UI fühlt sich an wie
ein Produkt aus einem kalifornischen Technologie-Labor: reibungslos,
durchdacht, schön, intuitiv. Animationen sind dezent, Übergänge sind
flüssig, Schriftarten sind scharf und gut lesbar.

**Woche 5: LVGL-Integration und Dashboard-Layout**

- LVGL (Light and Versatile Graphics Library) in ESP-IDF-Projekt
  einbinden, esp_lvgl_port für esp_lcd-Treiber
- PSRAM-Framebuffer mit Bounce-Buffer für DMA-Transfers
- Draw-Buffer: 1/4 der Bildschirmgröße in internem SRAM (38 KB)
- Dashboard-Screen: Speed groß zentral, RPM als Balken oder
  digital, Gang, Temperatur, Kraftstoff, Bordspannung, Uhrzeit
- Farbschema: dunkler Hintergrund, helle Schrift, Akzentfarbe
  für Warnungen (Bernstein/Rot), positive Werte in Weiß
- Typografie: große, fette Schrift für Hauptwerte, kleinere
  proportionale Schrift für Sekundärwerte

**Woche 6: Animationen, E-Paper-Zweitdisplay, UI-State-Machine**

- Screen-Wechsel mit dezenten Übergängen (Slide, Fade) — nicht
  abrupt, nicht zu langsam (200-300 ms)
- UI-State-Machine: Dashboard, Navigation, Tour-Statistik, Diagnose,
  Settings — Wechsel über physikalische Taster am Lenker
- E-Paper-Zweitdisplay (2.13") über SPI3: statische Werte (Kraftstoff,
  Trip, ODO, Uhrzeit, Außentemperatur), Partial-Refresh alle 0.3s
- Warnungs-Overlay: bei kritischen Werten dimmt das Dashboard und
  zeigt eine großflächige Warnung an (z.B. "KÜHLTEMPKRITISCH 115°C")
- Tour-Ende-Zusammenfassung: Distanz, Fahrtzeit, Durchschnitts-
  geschwindigkeit, Höhenmeter, Max-Speed, Max-RPM, Verbrauch,
  Max-Schräglage
- Neigungsanzeige: Roll/Pitch aus BMI270 mit Complementary Filter,
  Anzeige als Balken oder Zahlenwert (-45 bis +45 Grad)

**Meilenstein M3:** Premium-Dashboard läuft auf TFT, E-Paper zeigt
statische Werte. UI fühlt sich rund an. Erste längere Testfahrt.

**Abhängigkeiten:** Phase 1 (Basis-Display), Phase 2 (Sensordaten für
Dashboard). BMI270 und BMP388 müssen initialisiert sein für Neigung
und Höhe.

**Risiken:** LVGL auf ESP32-S3 mit PSRAM und zwei Displays kann
Performance-Probleme haben. Lösung: Draw-Buffer-Größe tunen,
Partial-Refresh maximieren, ggf. nur ein Display gleichzeitig
refreshen. Bounce-Buffer-Größe experimentell optimieren.

#### Phase 4 (Woche 7-8): Connectivity

**Ziel:** BLE-Verbindung zum Smartphone. Companion-App (PWA) zeigt
Live-Daten, sendet Navigationsanweisungen, ermöglicht OTA-Updates.

**Woche 7: BLE-GATT-Server und PWA**

- NimBLE-Stack initialisieren (nicht Bluedroid — 50% weniger RAM)
- Custom GATT-Service-Struktur (siehe Dokument 07, Abschnitt 2.3):
  - Bordcomputer Data (Live-Telemetrie, Notify, 1 Hz, ~200 Bytes JSON)
  - Navigation (Write: Maneuver, Distanz, Straßenname)
  - Config (Read/Write: Schwellwerte, Layout, Display-Helligkeit)
  - File Transfer (Read: GPX/CSV-Dateien, chunked)
  - Device Info (Standard 0x180A)
- MTU-Aushandlung: esp_ble_gatt_set_local_mtu(247), App muss
  requestMtu(247) aufrufen
- PWA (Progressive Web App) mit Web Bluetooth API:
  - Auto-Discovery (scan for service UUID)
  - Live-Daten-Anzeige (Speed, RPM, Temp, etc.)
  - Konfigurations-Interface (Schwellwerte, Layout)
  - Datei-Browser für GPX/CSV-Export
  - Installierbar als PWA (Home-Screen-Icon)

**Woche 8: Navigation und OTA**

- Turn-by-Turn Navigation: Companion-App nutzt native Maps-API
  (iOS MapKit / Android Maps-API), berechnet Route, sendet
  nur die nächste Anweisung als JSON über BLE an Bordcomputer
- Navigations-UI auf TFT: großer Pfeil (links/rechts/geradeaus/
  Rundkehr), Distanz zum Maneuver, Straßenname (scrollend bei
  langen Namen), nächstes Maneuver als Vorschau
- Back-Track-Navigation: Bordcomputer zeichnet Route auf, kann
  umgekehrt zurücknavigieren ohne Smartphone
- OTA-Update über BLE: Dual-Partition-Layout (ota_0, ota_1),
  Chunk-basiertes Flashen (512 Byte Chunks), Checksum-Verify,
  Boot-Partition-Set, Rollback bei Boot-Fehler
- Crash-Notification (BLE-Push an App bei Crash-Erkennung, vorbereitet
  für Phase 5)

**Meilenstein M4:** Smartphone ist verbunden. PWA zeigt Live-Daten.
Turn-by-Turn funktioniert. OTA-Update getestet.

**Abhängigkeiten:** Phase 1-3 (Daten müssen da sein, Display muss
anzeigen können). BLE-Hardware ist im ESP32-S3 eingebaut.

**Risiken:** iOS Safari unterstützt kein Web Bluetooth (Workaround:
Bluefy Browser). Lösung: PWA für Android, Bluefy für iOS, oder native
App als späterer Schritt. BLE-MTU-Aushandlung kann auf manchen
Smartphones fehlschlagen — Fallback auf 20-Byte-Payload mit Chunking.

#### Phase 5 (Woche 9-10): Safety und Diagnose

**Ziel:** Crash-Detection, DTC-System, Self-Test, Wartungs-Intervalle.
Das System ist robust, selbstdiagnosefähig und sicher.

**Woche 9: Crash-Detection und DTC-System**

- Crash-Detection: BMI270-Beschleunigung überwachen (100 Hz),
  Crash-Kriterium: Beschleunigung > 7g für > 100 ms UND Neigung > 75°
  für > 500 ms. Multi-Stage-Verifikation:
  1. Beschleunigungsschwelle überschritten -> Verdacht
  2. Neigung für 3-5 Sekunden bestätigt -> Crash bestätigt
  3. Countdown 10 Sekunden -> Abbruch durch Tastendruck (Fehlalarm)
  4. Wenn nicht abgebrochen: SOS an Smartphone-App senden
- Sturz-Aufzeichnung: Ring-Buffer mit 30 Sekunden IMU-Daten (50 Hz,
  1500 Samples), bei Crash auf SD-Karte sichern (CSV)
- DTC-System (Diagnostic Trouble Codes): Eigenes Fehlersystem,
  siehe Abschnitt 3.2 für Details
- Self-Test: Hardware-Selbsttest beim Boot, siehe Abschnitt 3.5

**Woche 10: Wartungs-Intervalle, QS-Tests, Benchmarks**

- Wartungs-Intervall-Anzeige: Kilometer/Tage seit letzter Wartung,
  konfigurierbare Intervalle (Ölwechsel alle 6000 km / 12 Monate,
  Reifen, Bremsflüssigkeit, Kettensatz), Warnung bei 90% des
  Intervalls, manuelles Reset nach Wartung, NVS-Persistenz
- Qualitätsicherung: Unit-Tests, Integration-Tests, HIL-Tests,
  Performance-Benchmarks (siehe Abschnitt 4)
- Code-Review-Checkliste durchgehen (siehe Abschnitt 4.4)
- Dokumentation: README aktualisieren, PINMAP, ARCHITECTURE,
  Inbetriebnahme-Anleitung

**Meilenstein M5:** Vollständiges System mit Safety, Diagnose und QS.
Bereit für den Dauereinsatz.

**Abhängigkeiten:** Alle vorherigen Phasen. Crash-Detection braucht
BMI270 (Phase 2). SOS braucht BLE (Phase 4).

**Risiken:** Crash-Detection-Algorithmus kann Fehlalarme auslösen
(Schlaglöcher, hartes Bremsen). Lösung: Multi-Stage-Verifikation mit
Neigungs-Bestätigung und Countdown. DTC-System kann komplex werden —
auf die wichtigsten 20-30 Codes beschränken.

---

## 3. Support-Schnittstellen

### 3.1 Diagnose-Zugang für Reparateure und Techniker

Ein Reparateur oder Techniker muss auf drei Wegen auf Fehler und
Diagnosedaten zugreifen können:

**Weg 1: Seriell (USB-C)**

Der ESP32-S3 hat USB-OTG. Über USB-C verbunden, bietet er einen
seriellen Diagnose-Modus, der ohne spezielle Software funktioniert
(Terminal-Programm wie minicom, screen, PuTTY). Befehle:

```
> diag status          # System-Status, aktive DTCs
> diag dtc list       # Alle gespeicherten Fehlercodes
> diag dtc clear      # Fehlercodes löschen (nach Reparatur)
> diag sensor all     # Alle Sensorwerte live
> diag sensor <name>   # Spezifischer Sensor (z.B. coolant)
> diag log tail        # Letzte 50 Log-Zeilen
> diag log dump        # Komplettes Log auf SD
> diag selftest        # Hardware-Self-Test auslösen
> diag battery         # Akku-Status, Ladezyklen
> diag config show     # Aktuelle Konfiguration
> diag config set <k> <v>  # Parameter setzen
> diag stats           # Betriebsstunden, km, Fahrten
```

Vorteil: Universell, keine App nötig, funktioniert an jedem Laptop.
Nachteil: Kabelverbindung nötig, Motorrad muss zugänglich sein.

**Weg 2: BLE über Smartphone-App**

Die Companion-App hat einen Diagnose-Modus, der über BLE auf die
gleichen Daten zugreift wie der serielle Weg. Zusätzlich:

- Visuelle Aufbereitung der Sensorwerte (Diagramme, Trend-Kurven)
- DTC-Liste mit Beschreibung und empfohlener Lösung
- Konfigurations-Interface für Schwellwerte und Intervalle
- Log-Dateien als Download (GPX, CSV, DTC-Log)
- Firmware-Update über OTA

Vorteil: Komfortabel, visuell, mobil. Nachteil: Smartphone muss
gepaart sein, App muss installiert sein.

**Weg 3: SD-Karte direkt**

Die SD-Karte enthält alle Diagnosedaten in strukturierten Dateien.
Sie kann entnommen und an einem PC ausgelesen werden:

```
/dtc/dtc_log.json          # Alle Fehlercodes mit Zeitstempel
/diag/diag_snapshot.json   # Letzter Diagnose-Snapshot
/config/bordcomputer.json  # Konfiguration
/logs/system.log           # System-Log
/telemetry/                # Telemetrie-CSVs
/tracks/                   # GPX-Tracks
/maintenance/maint_log.json # Wartungs-Historie
```

Vorteil: Keine Elektronik nötig, nur ein Kartenleser. Nachteil:
Keine Live-Daten, nur historisch.

**Empfehlung:** Weg 2 (BLE) als primäre Diagnose-Schnittstelle für
den Rider und Weg 1 (seriell) als Fallback für Techniker ohne App.
Weg 3 (SD-Karte) als letzte Möglichkeit und für Langzeit-Archivierung.

### 3.2 DTC-System (Diagnostic Trouble Codes)

Das Bordcomputer hat ein eigenes DTC-System, das an bewährte
Automobil-Standards angelehnt ist, aber auf die spezifischen Sensoren
und Komponenten dieses Projekts zugeschnitten ist.

#### 3.2.1 Nummerierungsschema

Jeder DTC ist 5-stellig: Ein Buchstabe (Systembereich) gefolgt von
4 Ziffern (Fehlercode).

```
Format: XNNNN

X = Systembereich:
  P = Powertrain (Motor, Getriebe, Antrieb)
  S = Sensoren (Sensorik)
  C = Connectivity (BLE, GPS)
  D = Display (TFT, E-Paper)
  E = Electrical (Stromversorgung, Akku)
  B = Bordcomputer intern (Speicher, Task-Crash)

NNNN = 4-stelliger Fehlercode:
  0001-0099: Hard-Fehler (Komponente defekt, Funktion nicht verfügbar)
  0100-0199: Soft-Fehler (Wert außerhalb des Plausibilitätsbereichs)
  0200-0299: Warn-Fehler (Wert nähert sich kritischer Schwelle)
  0300-0399: Kommunikationsfehler (Timeout, Verbindungsabbruch)
  0400-0499: Konfigurationsfehler (ungültige Einstellung)
```

#### 3.2.2 Fehlercode-Verzeichnis

| DTC | System | Beschreibung | Schweregrad | Aktion |
|---|---|---|---|---|
| P0001 | Powertrain | RPM-Signal verloren (kein Interrupt) | Warnung | Fallback: 0 RPM anzeigen |
| P0002 | Powertrain | RPM unplausibel (> 12000) | Warnung | Sensor prüfen, EMI? |
| P0100 | Sensoren | Kühlmitteltemp-Sensor nicht gefunden (1-Wire) | Kritisch | Sensor/Verkabelung prüfen |
| P0101 | Sensoren | Kühlmitteltemp unplausibel (< -20°C oder > 150°C) | Kritisch | Sensor defekt? |
| P0102 | Sensoren | Kraftstoffstand-Signal verloren (ADC = 0) | Warnung | Verkabelung prüfen |
| P0103 | Sensoren | Kraftstoffstand unplausibel (> 100% oder < 0%) | Warnung | Kalibrierung nötig |
| P0104 | Sensoren | Öldruck-Signal verloren | Kritisch | Sensor/Verkabelung prüfen |
| P0105 | Sensoren | Öldruck kritisch niedrig (< 0.5 bar) | Kritisch | Motor stoppen! |
| P0106 | Sensoren | Bordspannung kritisch niedrig (< 11.5V) | Kritisch | Generator/Regler prüfen |
| P0107 | Sensoren | Bordspannung kritisch hoch (> 15V) | Kritisch | Regler defekt! Überladung |
| P0108 | Sensoren | BMI270 IMU nicht gefunden (I2C) | Warnung | Neigungs-/Crash-Detection inaktiv |
| P0109 | Sensoren | BMP388 Barometer nicht gefunden (I2C) | Warnung | Höhenmessung inaktiv |
| P0110 | Sensoren | BMI270 unplausibel (Neigung > 90° bei Stillstand) | Warnung | Kalibrierung nötig |
| P0200 | Sensoren | Kühlmitteltemperatur Warnschwelle (> 110°C) | Warnung | Kühler prüfen |
| P0201 | Sensoren | Kühlmitteltemperatur kritisch (> 115°C) | Kritisch | Motor stoppen! |
| P0202 | Sensoren | Kraftstoffreserve (< 3 Liter) | Warnung | Tanken! |
| P0203 | Sensoren | Bordspannung Warnung (< 12V bei laufendem Motor) | Warnung | Generator prüfen |
| P0204 | Sensoren | Bordspannung Warnung (> 14.8V) | Warnung | Regler prüfen |
| C0300 | Connectivity | GPS-Fix verloren (kein Fix > 60 Sekunden) | Warnung | Antenne prüfen, Tunnel? |
| C0301 | Connectivity | GPS-Modul antwortet nicht (UART-Timeout) | Kritisch | Verkabelung/Modul prüfen |
| C0302 | Connectivity | BLE-Verbindung verloren | Info | Smartphone außer Reichweite |
| C0303 | Connectivity | BLE-Stack-Fehler | Warnung | Neustart erwägen |
| D0300 | Display | TFT-Display nicht erreichbar (SPI-Timeout) | Kritisch | Verkabelung prüfen |
| D0301 | Display | E-Paper nicht erreichbar (SPI-Timeout) | Warnung | Verkabelung prüfen |
| E0300 | Electrical | SD-Karte nicht erkannt | Kritisch | Karte prüfen/tauschen |
| E0301 | Electrical | SD-Schreibfehler (Disk Full / I/O Error) | Kritisch | Karte voll? tauschen |
| E0302 | Electrical | Backup-Akku niedrig (< 20%) | Warnung | Laden |
| E0303 | Electrical | Backup-Akku kritisch (< 5%) | Kritisch | System geht bald aus |
| E0304 | Electrical | Brownout-Detektor ausgelöst | Warnung | Spannungsversorgung prüfen |
| E0305 | Electrical | Überstrom erkannt | Kritisch | Kurzschluss? |
| B0001 | Bordcomputer | Heap-Fragmentierung kritisch (< 10KB frei) | Warnung | Neustart |
| B0002 | Bordcomputer | Task-Watchdog-Timeout | Kritisch | Task hängt, Neustart |
| B0003 | Bordcomputer | NVS-Lesefehler | Warnung | Konfiguration verloren |
| B0004 | Bordcomputer | Flash-Schreibfehler | Kritisch | Speicher defekt? |
| B0005 | Bordcomputer | OTA-Update fehlgeschlagen | Warnung | Rollback auf alte Version |

#### 3.2.3 DTC-Logging

Jeder DTC wird beim Auftreten und beim Verschwinden protokolliert:

```json
{
  "dtc": "P0105",
  "severity": "critical",
  "description": "Öldruck kritisch niedrig (< 0.5 bar)",
  "timestamp_start": "2026-10-13T14:23:15Z",
  "timestamp_end": null,
  "value_at_trigger": 0.3,
  "threshold": 0.5,
  "odometer": 45823,
  "engine_hours": 312.5,
  "context": {
    "rpm": 3200,
    "coolant_temp": 92,
    "voltage": 13.8
  }
}
```

Die DTC-Log-Datei (/dtc/dtc_log.json) ist ein JSON-Array aller
DTC-Ereignisse. Sie wird beim Boot geladen und beim Auftreten neuer
Fehler erweitert. Bei Erreichen einer konfigurierbaren Maximalgröße
(Standard: 500 Einträge) wird der älteste Eintrag entfernt.

### 3.3 Log-Files: Strukturierte Fehlerprotokolle

Das System führt drei Arten von Log-Files:

**1. System-Log (/logs/system.log)**

Klartext-Log mit Zeitstempel und Log-Level. Rotiert bei 1 MB
(behält die letzten 1 MB, älteres wird verworfen).

```
[2026-10-13 14:23:01] INFO  Boot complete, firmware v1.2.0
[2026-10-13 14:23:01] INFO  GPS: searching for fix...
[2026-10-13 14:23:24] INFO  GPS: fix acquired, 9 satellites
[2026-10-13 14:23:25] INFO  SD: 16GB SanDisk Ultra, FAT32
[2026-10-13 14:23:25] INFO  BLE: advertising started
[2026-10-13 14:25:12] WARN  P0203: Bordspannung < 12V (11.8V)
[2026-10-13 14:25:15] INFO  P0203: resolved (12.2V)
[2026-10-13 14:30:00] INFO  Trip A: 12.3 km, Fahrtzeit 0:07
[2026-10-13 14:35:02] WARN  C0300: GPS-Fix verloren (Tunnel)
[2026-10-13 14:35:45] INFO  C0300: resolved (8 satellites)
```

**2. DTC-Log (/dtc/dtc_log.json)**

Strukturiertes JSON mit allen DTC-Ereignissen (siehe 3.2.3).

**3. Telemetrie-Log (/telemetry/YYYYMMDD_HHMMSS.csv)**

Pro Fahrt eine CSV-Datei mit allen Sensordaten:

```
timestamp,lat,lon,alt,speed,heading,rpm,gear,roll,pitch,temp_coolant,
temp_ambient,volt,fuel_level,oil_pressure
```

Sampling-Rate: 1 Hz für GPS, 10 Hz für IMU/RPM. Die Datei wird beim
Boot erstellt und beim Abschalten (Power-Loss-Interrupt) geschlossen.

### 3.4 Remote-Diagnose über Smartphone-App

Die Companion-App bietet einen Remote-Diagnose-Modus:

- **Live-Daten-Stream:** Alle Sensordaten in Echtzeit (1 Hz über
  BLE-Notify), visualisiert als Dashboard oder Diagramm
- **DTC-Liste:** Aktive und historische Fehlercodes mit Beschreibung,
  Schweregrad, Zeitpunkt des Auftretens, Kontext-Werte
- **DTC-Löschung:** Nach Reparatur können Fehlercodes gelöscht werden
  (Bestätigung erforderlich)
- **Log-Download:** System-Log, DTC-Log und Telemetrie-Dateien über
  BLE-File-Transfer herunterladen (chunked, 244 Bytes pro Paket)
- **Konfiguration:** Schwellwerte, Warn-Intervalle, Display-Layout
  per App ändern und an Bordcomputer übertragen
- **Firmware-Update:** OTA-Update über BLE, mit Fortschrittsanzeige
  und automatischem Rollback bei Fehler
- **Fernauslesbarkeit:** Bei gepaartem Smartphone in Reichweite
  (ca. 30m) können alle Daten live abgefragt werden — ideal für
  Werkstattbesuche, ohne das Gehäuse zu öffnen

Szenario: Der Rider bemerkt eine Warnung auf dem Display. Er hält an,
öffnet die App, sieht den DTC "P0203: Bordspannung < 12V", liest die
Kontextwerte (Spannung, RPM, Temperatur zum Zeitpunkt des Fehlers),
und kann entscheiden: weiterfahren oder Werkstatt aufsuchen. In der
Werkstatt kann der Techniker ohne App-Öffnung über USB-C die gleichen
Daten abrufen.

### 3.5 Self-Test: Hardware-Selbsttest beim Boot

Beim jedem Boot durchläuft der Bordcomputer einen automatischen
Self-Test, der alle kritischen Hardware-Komponenten prüft. Der
Self-Test dauert ca. 2-3 Sekunden und wird auf dem Display angezeigt.

**Self-Test-Sequenz:**

| Schritt | Prüfung | Dauer | Bestanden-Kriterium |
|---|---|---|---|
| 1 | ESP32-S3 CPU und Heap | 100 ms | Heap > 50KB frei, kein Stack-Overflow |
| 2 | Flash-Speicher | 50 ms | Lesbar, Partitionstabelle gültig |
| 3 | NVS (Non-Volatile Storage) | 50 ms | Lesbar, Konfiguration ladbar |
| 4 | SD-Karte | 300 ms | Erkannt, FAT32, schreibbar |
| 5 | GPS-Modul (UART) | 500 ms | Antwortet auf UBX-Poll (auch ohne Fix) |
| 6 | BMI270 IMU (I2C) | 100 ms | I2C-Adresse 0x68 erreichbar, Who-Am-I korrekt |
| 7 | BMP388 Barometer (I2C) | 100 ms | I2C-Adresse 0x76 erreichbar, Who-Am-I korrekt |
| 8 | DS18B20 (1-Wire) | 200 ms | Gerät gefunden, Temperatur im Plausibilitätsbereich |
| 9 | TFT-Display (SPI) | 200 ms | SPI-antwortet, Backlight steuerbar |
| 10 | E-Paper-Display (SPI) | 200 ms | BUSY-Pin reagiert, Init-Sequenz ok |
| 11 | ADC-Kanäle (Bordspannung, Kraftstoff) | 100 ms | Werte im Plausibilitätsbereich |
| 12 | Backup-Akku (ADC) | 100 ms | Spannung > 3.0V (nicht tiefentladen) |
| 13 | BLE-Stack | 200 ms | NimBLE initialisiert, Advertising startbar |
| 14 | RPM-Sensor (GPIO) | 100 ms | Pin als Input konfiguriert, Pull-up ok |
| 15 | Relais/MOSFET-Pins | 50 ms | Alle Outputs auf definiertem Safe-State |

**Gesamtdauer:** ca. 2.1 Sekunden.

**Ergebnis-Anzeige:**

```
┌─────────────────────────────────┐
│  SELF-TEST                      │
│                                 │
│  CPU + Heap ......... OK        │
│  Flash + NVS ........ OK        │
│  SD-Karte ........... OK  16GB  │
│  GPS-Modul ......... OK  (no fix)│
│  IMU BMI270 ........ OK        │
│  Baro BMP388 ....... OK        │
│  Temp DS18B20 ...... OK  18°C   │
│  TFT-Display ....... OK        │
│  E-Paper ........... OK        │
│  ADC Sensoren ...... OK        │
│  Akku .............. OK  87%   │
│  BLE ............... OK        │
│  RPM-Sensor ........ OK        │
│  Relais ............ OK        │
│                                 │
│  System bereit.                  │
│  0 Fehler, 0 Warnungen.         │
└─────────────────────────────────┘
```

Bei Fehler: entsprechende Zeile wird rot markiert und der DTC wird
sofort gesetzt. Kritische Fehler (SD-Karte, Display) zeigen eine
Warnung an und blockieren den Start, bis bestätigt wird.

### 3.6 Wartungs-Intervall-Anzeige

Der Bordcomputer verfolgt Betriebsstunden und Kilometerstand und
erinnert den Rider an anstehende Wartungsarbeiten.

**Konfigurierbare Wartungs-Intervalle:**

| Wartung | Standard-Intervall | Einheit | Warnung bei |
|---|---|---|---|
| Ölwechsel | 6000 km oder 12 Monate | km/Tage | 90% (5400 km / 328 Tage) |
| Reifen | 10000 km oder 5 Jahre | km/Tage | 90% |
| Bremsflüssigkeit | 24 Monate | Tage | 90% (657 Tage) |
| Kettensatz | 25000 km | km | 90% |
| Luftfilter | 12000 km | km | 90% |
| Zündkerzen | 12000 km | km | 90% |
| Gabelöl | 20000 km oder 2 Jahre | km/Tage | 90% |
| Kühlmittel | 24 Monate | Tage | 90% |

**Implementierung:**

- Kilometerstand aus GPS/Trip (ODO, persistent in NVS)
- Betriebsstunden aus Betriebsstunden-Zähler (hochgezählt bei
  laufendem Motor, erkannt an RPM > 0 oder Bordspannung > 13V)
- Wartungs-Log: /maintenance/maint_log.json mit allen durchgeführten
  Wartungen (Datum, Kilometerstand, Art, Notiz)
- Reset nach durchgeführter Wartung: manueller Reset über App oder
  serielles Interface (diag maint reset <type>)
- Anzeige: Status-Icon auf Dashboard, Detail-Seite mit allen
  Intervallen und Farb-Codierung (grün = ok, gelb = Warnung,
  rot = überfällig)

**Anzeige auf dem Display:**

```
┌─────────────────────────────────┐
│  WARTUNG                         │
│                                 │
│  Ölwechsel       4.200 / 6.000  │
│  ████████████░░░░░░░░  70%  OK  │
│                                 │
│  Reifen          7.100 / 10.000 │
│  █████████████░░░░░░░  71%  OK  │
│                                 │
│  Bremsflüssigk.  548 / 730 Tag  │
│  ███████████████░░░░░  75%  OK  │
│                                 │
│  Kettensatz     22.300 / 25.000 │
│  ██████████████████░░  89%  !   │
│                                 │
└─────────────────────────────────┘
```

---

## 4. Qualitätssicherung

### 4.1 Test-Strategie

Die Test-Strategie umfasst drei Ebenen: Unit-Tests, Integration-Tests
und Hardware-in-the-Loop (HIL) Tests. Jede Ebene hat klare Ziele und
Abdeckungsanforderungen.

#### 4.1.1 Unit-Tests für Treiber

Unit-Tests prüfen einzelne Software-Module isoliert. Sie laufen auf
dem Entwicklungsrechner (nicht auf dem ESP32) mit einem Mock-Framework
für Hardware-Zugriffe.

**Getestete Module:**

| Modul | Testfälle | Abdeckung |
|---|---|---|
| GPS-Treiber (NMEA-Parsing) | 15 | GGA, RMC, VTG, fehlerhafte Sätze, leere Daten, Checksum-Fehler |
| GPS-Treiber (UBX-NAV-PVT) | 10 | Binäres Parsing, Little-Endian, Feld-Grenzen |
| Haversine-Distanz | 8 | Bekannte Distanzen, antipodale Punkte, Null-Distanz, Pol-Nähe |
| DTC-System | 12 | Set/Clear, Persistenz, Schweregrad-Filterung, Log-Größen-Limit |
| Konfigurations-Parser (JSON) | 10 | Gültige/ungültige JSONs, Default-Werte, Typ-Konvertierung |
| Time-Zone-Konvertierung | 8 | CET/CEST, DST-Übergang, Schaltjahre |
| Moving-Average-Filter | 6 | Leerer Buffer, voller Buffer, Overflow |
| Complementary-Filter (Roll/Pitch) | 8 | Stillstand, konstante Neigung, Drift-Verhalten |
| Kraftstoff-Kennlinie | 6 | Leerer Tank, voller Tank, nicht-lineare Bereiche |
| Wartungs-Intervall-Logik | 8 | km-basiert, zeitbasiert, kombiniert, Reset |

**Framework:** Unity-Test-Framework (C, kompakt, läuft nativ auf PC
mit GCC und auf dem ESP32 mit ESP-IDF). CI-Pipeline führt Unit-Tests
bei jedem Commit aus.

#### 4.1.2 Integration-Tests für Task-System

Integration-Tests prüfen das Zusammenspiel mehrerer Module und
FreeRTOS-Tasks. Sie laufen auf dem ESP32-S3 (echte Hardware oder
QEMU-Emulation).

**Test-Szenarien:**

| Test | Beschreibung | Erfolgs-Kriterium |
|---|---|---|
| GPS -> Logger -> SD | GPS-Daten werden korrekt geloggt | CSV enthält GPS-Werte im richtigen Format |
| GPS -> Display | GPS-Geschwindigkeit erscheint auf Display | Wert aktualisiert sich alle 1 Sekunde |
| RPM-Interrupt -> Display | RPM-Wert reagiert auf Interrupts | Anzeige folgt innerhalb 200 ms |
| BLE-Notify -> App | Live-Daten werden über BLE gesendet | App empfängt 1 Hz Updates |
| Crash-Detection -> SOS | Crash wird erkannt und SOS ausgelöst | Countdown startet, SOS nach 10s |
| Power-Loss -> Flush | Bei Power-Loss wird SD-Flush abgeschlossen | Letzte Daten sind auf SD |
| OTA-Update | Firmware wird über BLE geflasht | Neustart mit neuer Version, Rollback bei Fehler |
| Self-Test beim Boot | Alle Komponenten werden geprüft | Self-Test-Report korrekt |
| DTC-Setzen bei Fehler | Sensor-Ausfall löst DTC aus | DTC in Log und auf Display |
| Dual-Display | TFT und E-Paper aktualisieren sich | Keine SPI-Kollision, beide zeigen korrekte Daten |

#### 4.1.3 Hardware-in-the-Loop (HIL)

HIL-Tests verwenden Sensor-Simulationen, um reproduzierbare Tests
unter definierten Bedingungen zu ermöglichen. Statt echter Sensoren
werden simulierte Signale erzeugt.

**HIL-Setup:**

Ein zweiter ESP32 (oder ein Raspberry Pi) fungiert als Signal-
Generator und erzeugt:

- **GPS:** Simulierte NMEA/UBX-Sätze über UART (definierte Positionen,
  Geschwindigkeiten, Sat-Anzahl). Test-Routen als GPX-Dateien abspielbar.
- **RPM:** PWM-Signal mit definierter Frequenz am RPM-Pin (simuliert
  Zündimpulse von 800-8000 RPM)
- **Kühlmitteltemp:** Widerstandsnetzwerk mit schaltbaren Widerständen
  (simuliert 20°C, 80°C, 110°C, 120°C)
- **Kraftstoffstand:** Spannungsteiler mit Potentiometer (simuliert
  0-100% Füllstand)
- **Bordspannung:** Labornetzteil mit einstellbarer Spannung (10-16V)
- **IMU:** Stattdaten über I2C (simulierte Neigungswerte, Crash-Beschleunigung)
- **SD-Karte:** Echte SD-Karte (keine Simulation nötig)
- **BLE:** Echte BLE-Verbindung (keine Simulation möglich)

**HIL-Test-Szenarien:**

| Szenario | Simulierte Bedingung | Erwartetes Verhalten |
|---|---|---|
| Normalfahrt | 80 km/h, 4000 RPM, 85°C, 13.8V | Dashboard zeigt alle Werte korrekt |
| Überhitzung | Kühlmitteltemp steigt auf 115°C | Warnung, DTC P0201, Buzzer |
| Kraftstoffreserve | Füllstand fällt auf 2 Liter | Warnung, DTC P0202 |
| Generator-Ausfall | Bordspannung fällt auf 11.5V | Warnung, DTC P0106 |
| GPS-Verlust | GPS-Fix verloren (Tunnel-Simulation) | DTC C0300, Fallback auf letzte Werte |
| Crash-Simulation | Beschleunigungsspike > 7g + Neigung > 75° | Crash-Detection, Countdown, SOS |
| Sensor-Ausfall | DS18B20 vom I2C getrennt | DTC P0100, Display zeigt "---" |
| SD-Karte voll | SD-Karte mit 1KB freiem Platz | DTC E0301, Logging stoppt sauber |
| Power-Loss | Bordspannung plötzlich weg | Letzte Daten auf SD gesichert |
| OTA-Update | Firmware-Datei über BLE gesendet | Update erfolgreich oder Rollback |

### 4.2 Performance-Benchmarks

| Metrik | Zielwert | Messmethode | Priorität |
|---|---|---|---|
| Display-Refresh-Rate (TFT) | >= 15 FPS (Full-Screen), >= 30 FPS (Partial) | esp_lcd-FPS-Counter, LVGL-Stat | Must |
| Display-Refresh-Rate (E-Paper) | >= 2 s (Full), >= 0.3 s (Partial) | Zeitmessung zwischen Init und BUSY-Clear | Should |
| BLE-Latenz (Notify) | < 100 ms (End-to-End) | Timestamp App vs. Bordcomputer | Must |
| SD-Schreibrate | >= 300 KB/s (4KB-Buffer) | FATFS-Benchmark, fwrite von 1MB | Must |
| GPS-Update-Rate | >= 5 Hz (UBX-NAV-PVT) | Timestamp-Differenz der PVT-Pakete | Must |
| Heap frei (laufend) | >= 50 KB | esp_get_free_heap_size() alle 10s | Must |
| Task-Watchdog | Kein Timeout in 24h | FreeRTOS-Task-Statistiken | Must |
| Boot-Zeit (bis Self-Test) | < 5 s | Zeit vom Power-On bis Self-Test-Start | Should |
| Self-Test-Dauer | < 3 s | Zeitmessung Self-Test-Sequenz | Should |
| BLE-Connect-Zeit | < 5 s | Zeit von Advertising bis Connected | Should |
| OTA-Update-Dauer | < 30 s (2 MB Firmware) | Zeit vom Start bis Reboot | Should |
| Crash-Detection-Latenz | < 500 ms | Zeit vom Impact bis SOS-Countdown | Must |

### 4.3 Code-Review-Checkliste

Jeder Merge-Request wird gegen die folgende Checkliste geprüft:

**Architektur und Design**

- [ ] Neue Module folgen dem Sensor-Interface-Pattern (init/read/config)
- [ ] Tasks haben definierte Stack-Größen und Prioritäten
- [ ] Keine blockierenden Operationen in High-Priority-Tasks (keine delay(),
  kein Warten auf I2C/SPI ohne Timeout in GPS-Task)
- [ ] Shared State ist durch Mutex oder Atomic geschützt
- [ ] Keine magic numbers — Konstanten in config.h oder settings.json

**Sicherheit und Robustheit**

- [ ] Alle externen Eingaben werden validiert (NMEA, UBX, BLE, JSON-Config)
- [ ] Buffer-Größen sind definiert und werden nicht überschritten (snprintf
  statt sprintf, strlcpy statt strcpy)
- [ ] I2C/SPI/UART-Transaktionen haben Timeouts
- [ ] Fehler werden geloggt und als DTC gemeldet, nicht stillschweigend ignoriert
- [ ] Keine assert() in Produktions-Code — stattdessen Fehlerbehandlung
- [ ] NVS-Schreibzugriffe sind minimal (Flash-Lebensdauer)

**Motorrad-Spezifisch**

- [ ] Keine Ablenkung während der Fahrt (keine pop-ups, die Bestätigung
  erfordern, bei Geschwindigkeit > 5 km/h)
- [ ] Kritische Warnungen sind sofort erkennbar (groß, kontrastreich, mit Ton)
- [ ] Display-Updates sind schnell genug (kein Einfrieren bei schnellen
  Geschwindigkeitsänderungen)
- [ ] Power-Loss-Handler sichert offene Dateien

**Code-Stil**

- [ ] Funktionen sind < 80 Zeilen (außer Daten-Tabellen)
- [ ] Variablen haben aussagekräftige Namen
- [ ] Kommentare erklären "warum", nicht "was"
- [ ] Keine toten Code-Pfade
- [ ] Konsistente Einrückung (4 Spaces, keine Tabs)

### 4.4 CI/CD-Pipeline

Die CI-Pipeline führt bei jedem Push und Pull-Request folgende Schritte aus:

1. **Lint:** clang-format Check, cppcheck statische Analyse
2. **Unit-Tests:** Unity-Tests auf PC (GCC), müssen alle bestehen
3. **Build:** ESP-IDF-Kompilierung für ESP32-S3 (ohne Flash)
4. **Größe:** Flash- und RAM-Verbrauch muss unter definierten Limits liegen
5. **Dokumentation:** Doxygen-Generierung, README-Check

Die Pipeline läuft auf GitHub Actions. Ein Merge in den main-Branch
erfordert eine erfolgreiche Pipeline-Ausführung.

---

## 5. Risiken und Mitigation

### 5.1 Risikomatrix

| Risiko | Wahrscheinlichkeit | Auswirkung | Risikowert | Mitigation |
|---|---|---|---|---|
| Zündspulen-EMI stört Elektronik | Hoch | Hoch | Kritisch | Abschirmung, RC-Filter, räumliche Trennung, Optokoppler |
| Wassereintritt ins Gehäuse | Mittel | Hoch | Hoch | IP67-Gehäuse, GORE-Vent, Silikon-O-Ringe, Cable-Glands |
| Vibration beschädigt Lötstellen/Anschlüsse | Hoch | Mittel | Hoch | Custom PCB statt Perfboard, Schraubklemmen, Kabelbinder, Schrumpfschlauch |
| SD-Kartenausfall | Mittel | Mittel | Mittel | Marken-SD-Karte, Ringbuffer, Power-Loss-Flush, Backup in NVS |
| GPS-Signalverlust (Tunnel, Wald, Tall) | Mittel | Niedrig | Niedrig | NEO-M9N (4-Konstellation), Hall-Sensor-Fallback (optional) |
| Display-Ausfall (TFT) | Niedrig | Hoch | Mittel | E-Paper als redundante Anzeige für kritische Werte |
| Backup-Akku versagt | Niedrig | Niedrig | Niedrig | TP4056-Schutz, Brownout-Detektor, sauberes Shutdown |
| Generator/Regler der PD06 versagt | Mittel | Hoch | Hoch | Bordspannungs-Überwachung, Warnung bei < 12V, redundante Sensorwerte |
| BLE-Verbindung instabil | Mittel | Niedrig | Niedrig | NimBLE (stabiler als Bluedroid), Auto-Reconnect, Fallback auf seriell |
| Flash-Speicher erschöpft (NVS-Wear) | Niedrig | Mittel | Niedrig | Minimale NVS-Schreibzugriffe, Wear-Leveling, Konfiguration auf SD |
| OTA-Update schlägt fehl | Niedrig | Mittel | Niedrig | Dual-Partition mit Rollback, Checksum-Verify, Auto-Recovery |
| Fehlalarm Crash-Detection | Mittel | Niedrig | Niedrig | Multi-Stage-Verifikation, Neigungs-Bestätigung, 10s Countdown |
| Überspannung (Lastabwurf 40V) | Mittel | Hoch | Hoch | TVS-Diode SMAJ15A, 5A Sicherung, MP1584 mit 28V Max (oder TPS5430 mit 36V) |
| Kondenswasser im Gehäuse | Mittel | Mittel | Mittel | GORE PolyVent (Druckausgleich), IP67-Dichtung, keine Waagerechten Flächen |

### 5.2 Hardware-Failures: Notfall-Betrieb bei Teilausfall

Der Bordcomputer ist so konzipiert, dass er bei Ausfall einzelner
Komponenten in einem degraded mode weiterläuft. Jede Komponente hat
einen definierten Fallback.

**Display-Ausfall (TFT):**

- Primär: TFT zeigt alle Daten
- Fallback 1: E-Paper-Zweitdisplay übernimmt Kerninstrumente
  (Speed, RPM, Gang, Kraftstoff, Warnungen)
- Fallback 2: BLE-Notify an Smartphone-App, App zeigt Dashboard
- Fallback 3: Serieller Output über USB-C (für Diagnose)
- DTC: D0300 (TFT nicht erreichbar)

**GPS-Ausfall:**

- Primär: NEO-M9N liefert Position, Geschwindigkeit, Höhe
- Fallback 1: Hall-Sensor am Vorderrad für Geschwindigkeit
  (falls installiert)
- Fallback 2: Letzte bekannte Geschwindigkeit einfrieren, mit
  Hinweis "GPS: kein Signal" auf Display
- Auswirkung: Tripcmeter stoppt, Track-Logging pausiert
- DTC: C0300 (GPS-Fix verloren) oder C0301 (GPS-Modul nicht
  erreichbar)

**SD-Karten-Ausfall:**

- Primär: SD-Karte für Logging, Tracks, Config, OTA
- Fallback 1: NVS für kritische Daten (ODO, Wartungs-Intervalle,
  DTC-Log der letzten 20 Einträge)
- Fallback 2: BLE-Notify der Live-Daten an Smartphone-App, App loggt
- Auswirkung: Kein GPX-Logging, keine Telemetrie-CSV, keine
  Langzeit-Trends
- DTC: E0300 (SD nicht erkannt) oder E0301 (SD-Schreibfehler)

**RPM-Sensor-Ausfall:**

- Primär: PC817 am Zündkabel
- Fallback: RPM-Anzeige auf "---" setzen, Ganganzeige inaktiv
- Auswirkung: Keine Drehzahl- und Ganganzeige, Crash-Detection
  verliert RPM-Kontext (arbeitet mit IMU weiter)
- DTC: P0001 (RPM-Signal verloren)

**Backup-Akku-Ausfall:**

- Primär: 18650 versorgt System bei Motor aus
- Fallback: System läuft nur bei laufendem Motor (Bordnetz)
- Auswirkung: Kein Power-Loss-Schutz, keine Tiefgaragen-Logs,
  Diebstahl-Warnung inaktiv
- DTC: E0302 (Akku niedrig) oder E0303 (Akku kritisch)

**BLE-Ausfall:**

- Primär: BLE für Smartphone-Anbindung
- Fallback: Keine Smartphone-Integration (kein Navigation, keine
  App-Diagnose, kein OTA über BLE)
- Auswirkung: Bordcomputer funktioniert als Standalone-Gerät
  weiter, alle Core-Features bleiben verfügbar
- DTC: C0303 (BLE-Stack-Fehler)

**Kompletter ESP32-Ausfall:**

- Nicht zu mitigieren — der Bordcomputer ist tot
- Prävention: Brownout-Detektor, Watchdog, Überspannungsschutz
- Alle Daten bis zum Ausfall sind auf SD-Karte gesichert (falls
  der letzte Flush vor dem Ausfall stattfand)

### 5.3 Backup-Strategien: Redundanz

**Redundante Anzeige (zweites Display):**

Das E-Paper-Zweitdisplay ist nicht nur ein Nice-to-Have, sondern
eine Redundanz. Wenn das TFT ausfällt (SPI-Kabelbruch, Treiber-
Absturz), zeigt das E-Paper weiterhin Speed, Kraftstoff, Warnungen.
Da E-Paper bistabil ist, bleibt der letzte Inhalt selbst bei
vollständigem Stromausfall sichtbar.

**Redundante Geschwindigkeitsmessung:**

GPS ist primär. Der optionale Hall-Sensor am Vorderrad ist sekundär.
Bei GPS-Ausfall übernimmt der Hall-Sensor. Ohne Hall-Sensor wird die
letzte bekannte Geschwindigkeit eingefroren.

**Redundante Stromversorgung:**

Das 12V-Bordnetz ist primär (bei laufendem Motor). Der 18650-Akku
ist sekundär. Bei Bordnetz-Ausfall (Generator/Regler defekt) läuft
der Bordcomputer auf Akku weiter und warnt den Rider. Die
Schottky-Diode im Power-Path verhindert Rückstrom.

**Redundante Datenspeicherung:**

SD-Karte ist primär. NVS (interner Flash) ist sekundär für kritische
Daten (ODO, Wartungs-Intervalle, DTC-Log). Bei SD-Ausfall bleiben
die wichtigsten persistenten Daten erhalten. BLE-Notify an die App
ist tertiär — die App kann Live-Daten loggen, wenn die SD-Karte
ausfällt.

### 5.4 EMV-Schutz (Elektromagnetische Verträglichkeit)

Die PD06 hat eine CDI-Zündung (Capacitive Discharge Ignition), die
signifikante elektromagnetische Störungen erzeugt. Die Zündspulen
erzeugen Hochspannungsspitzen, die den Bordcomputer stören können.

**Schutzmaßnahmen:**

1. **Abschirmung:** Abgeschirmtes Kabel für RPM-Pickup am Zündkabel,
   Schirmung an Masse am Bordcomputer-Gehäuse
2. **Galvanische Trennung:** PC817 Optokoppler zwischen Zündkreis
   und Bordcomputer-Elektronik
3. **RC-Filter:** Tiefpass am RPM-GPIO (1kΩ + 100nF, Grenzfrequenz
   ca. 1.6 kHz — lässt 8000 RPM = 133 Hz durch, blockt HF-Störungen)
4. **Zener-Diode:** 3.3V Zener parallel zum Optokoppler-Ausgang als
   Überspannungsschutz
5. **Niederohmiger Pull-up:** 220Ω statt 10kΩ, um Störungen zu
   unterdrücken
6. **Räumliche Trennung:** RPM-Kabel so kurz wie möglich, weit weg
   von anderen Sensorleitungen
7. **Ferrit-Kern:** Optional Ferrit-Ringkern am RPM-Kabel für
   HF-Dämpfung
8. **Masse-Führung:** Sternförmige Masseführung, keine Schleifen,
   um induktive Einkopplung zu vermeiden
9. **Lastabwurf-Schutz:** TVS-Diode SMAJ15A am Power-Input, 5A
   Sicherung vor dem Buck-Converter
10. **Filterkondensatoren:** 100µF Elko + 100nF Kerko am Power-Input
    für Transienten-Dämpfung

---

## 6. Architektur-Entscheidungen

### 6.1 ESP-IDF statt Arduino

Die Entscheidung fällt auf ESP-IDF (nicht Arduino) für die
Firmware-Entwicklung. Begründung:

- Volle Kontrolle über FreeRTOS-Task-Prioritäten, Stack-Größen,
  CPU-Affinität (Core 0 / Core 1)
- Direkter Zugriff auf esp_lcd-Treiber für Display-DMA
- NimBLE statt Bluedroid als BLE-Stack (50% weniger RAM)
- OTA mit Dual-Partition und Rollback nativ unterstützt
- Build-System (CMake + idf.py) ist robust und CI-tauglich
- Arduino-ESP32 als Framework-Layer über ESP-IDF ist möglich,
  aber nicht empfohlen für ein Projekt dieser Komplexität

### 6.2 FreeRTOS-Task-Architektur

| Task | Core | Priorität | Stack | Frequenz | Beschreibung |
|---|---|---|---|---|---|
| GPS-Task | 1 | 5 (hoch) | 4 KB | 10 Hz | UART-Lesen, NMEA/UBX-Parsing |
| Sensor-Task | 1 | 4 | 4 KB | 1-100 Hz | I2C/ADC/1-Wire lesen |
| Display-Task | 0 | 3 | 8 KB | 30 Hz | LVGL-Render, SPI-Transfer |
| Logger-Task | 0 | 2 | 4 KB | 10 Hz | SD-Schreiben (Ringbuffer-Flush) |
| BLE-Task | 0 | 3 | 6 KB | Event | NimBLE-Callbacks, GATT-Server |
| Main-Task | 0 | 1 | 4 KB | 1 Hz | Statemachine, Koordination |
| Crash-Task | 1 | 7 (höchste) | 2 KB | 100 Hz | IMU-Monitoring, Crash-Detection |

Begründung der Core-Zuweisung: Core 1 übernimmt zeitkritische
Sensoren (GPS, IMU), Core 0 übernimmt UI und Konnektivität. So
blockiert ein langsamer SPI-Transfer nicht die GPS-Auslesung.

### 6.3 Software-Architektur: Modulares Sensor-Interface

Jeder Sensor implementiert ein einheitliches Interface:

```c
typedef struct {
    const char* name;
    esp_err_t (*init)(void);
    esp_err_t (*read)(sensor_data_t* data);
    esp_err_t (*config)(const char* key, const char* value);
    void (*deinit)(void);
} sensor_driver_t;
```

Der Sensormanager registriert alle Treiber beim Boot und ruft
`read()` in der Sensor-Task zyklisch auf. Neue Sensoren können
hinzugefügt werden, ohne bestehenden Code zu ändern — nur ein
neues Modul mit dem Interface implementieren und registrieren.

### 6.4 Konfigurations-Verwaltung

Die Konfiguration liegt als JSON auf der SD-Karte
(/config/bordcomputer.json) und wird beim Boot in NVS geladen.
Änderungen über BLE (App) oder serielles Interface werden
sowohl in NVS als auch in der JSON-Datei gespeichert.

Das JSON-Format ist menschenlesbar und kann direkt auf der
SD-Karte editiert werden (Kartenleser am PC). Bei ungültigem
JSON fallen die Werte auf kompilierte Defaults zurück.

---

## 7. Lieferanten und Einkaufsstrategie

### 7.1 Lieferanten-Übersicht

| Komponente | Lieferant | Begründung |
|---|---|---|
| ESP32-S3-DevKitC-1 | Mouser / DigiKey | Original, nicht Clone |
| ESP32-S3-WROOM-1 (N8R2) | Mouser / DigiKey | Für Custom PCB |
| u-blox NEO-M9N | Mouser / DigiKey | Original (Clones sind oft Fakes) |
| BMI270, BMP388 | Mouser / DigiKey | Original-Sensoren (nicht Clone) |
| DS18B20 | Mouser / DigiKey | Original (Fakes im Umlauf) |
| Displays (Waveshare) | Waveshare Store / Amazon | Original |
| VDO Öldrucksensor | KFZ-Teileshop / eBay | Spezifisches Bauteil |
| Passive Bauteile (R, C) | Mouser / DigiKey | Großpackungen |
| Gehäuse-Filament (ASA) | Prusa / Filament-Shop | Qualität |
| GORE PolyVent | Gore direkt / Distributor | Spezialprodukt |
| SD-Karte | SanDisk direkt / Amazon | Markenprodukt |
| Custom PCB | JLCPCB / PCBWay | 5 Stück ab 2 EUR + Bestückung |
| Steckverbinder | Molex / RS Components | Vibrationssicher |

### 7.2 Einkaufs-Strategie

1. **Kritische Komponenten (GPS, MCU, Sensoren):** Nur bei
   autorisierten Distributoren (Mouser, DigiKey). Clones von
   AliExpress sind oft Fakes (besonders NEO-M9N und DS18B20).
2. **Mechanische Teile (Gehäuse, Halterung):** 3D-Druck selbst,
   Filament von Qualitätshersteller. GORE-Vent als Spezialprodukt
   direkt von Gore.
3. **Kabel, Widerstände, Kondensatoren:** Großpackungen bei
   Mouser/DigiKey. Versandkosten amortisieren sich bei
   Mehrfachbestellungen.
4. **Custom PCB:** JLCPCB für Prototyp (5 Stück, 2-EUR-Basispreis,
   Bestückung ca. 15 EUR). Lieferzeit ca. 2 Wochen.
5. **Puffer:** 15% Puffer für Versand, Preisschwankungen und
   versehentlich bestellte falsche Teile einplanen.

### 7.3 Fälschungs-Warnung

Besonders folgende Komponenten werden häufig gefälscht:

| Komponente | Fälschungs-Risiko | Erkennung |
|---|---|---|
| u-blox NEO-M9N | Sehr hoch (AliExpress unter 15 EUR) | Original nur bei Mouser/DigiKey, Markierung lasergraviert |
| DS18B20 | Hoch | Fakes liefern falsche Werte, CRC-Fehler; Test: Eiswasser = 0°C |
| ESP32-S3 | Mittel | DevKitC-1 original hat Espressif-Logo und USB-Bridge CH343 |
| SanDisk SD-Karte | Hoch (bei Amazon Marketplace) | Originalverpackung, Serial auf sandisk.com verifizieren |

---

## 8. Roadmap nach der ersten Version

### 8.1 Was nach Phase 5 kommt

Nach Abschluss von Phase 5 ist der Bordcomputer voll funktionsfähig.
Die folgenden Erweiterungen sind für zukünftige Versionen denkbar,
aber nicht Teil des 10-Wochen-Plans:

| Erweiterung | Beschreibung | Aufwand | Priorität |
|---|---|---|---|
| Native App (React Native / Flutter) | Statt PWA für iOS und Android | 4-6 Wochen | Medium |
| Wetter-API-Integration | Live-Wetter über Smartphone-App | 1 Woche | Low |
| Incoming-Call (ANCS) | Anrufername auf Display | 1 Woche | Medium |
| Music-Info (AMS) | Artist/Titel auf Display | 1 Woche | Low |
| Diebstahl-Warnung | IMU Wake-on-Motion, BLE-Push | 2 Wochen | Medium |
| TPMS-Integration | Reifendruck über BLE-Beacons | 1 Woche | Low |
| Web-Dashboard | Statische HTML-Datei auf SD, Telemetrie-Visualisierung | 2 Wochen | Low |
| REST-API / MQTT | WiFi-Verbindung, Home Assistant Integration | 2 Wochen | Low |
| Segment-Timing | GPS-basierte Sektor-Zeiten | 1 Woche | Low |
| Plugin-System UI | Benutzerdefinierte Screens per JSON | 3 Wochen | Low |

### 8.2 Langfristige Vision

Der Bordcomputer ist als offene Plattform konzipiert. Die modulare
Architektur (Sensor-Interface, Task-System, BLE-GATT-Profile,
JSON-Konfiguration) erlaubt Dritten, eigene Sensoren, Displays und
Apps zu entwickeln. Die Dokumentation in docs/research/ ist die
Grundlage für eine Community, die das System erweitern kann.

Das Endziel ist ein Bordcomputer, der sich anfühlt wie ein Produkt
eines kalifornischen Technologieherstellers — aber von jedem
nachgebaut und angepasst werden kann, der löten kann und einen
3D-Drucker besitzt.

---

## 9. Quellenverzeichnis

Dieser Master-Plan baut auf den Forschungsdokumenten 01 bis 07 auf.
Alle detaillierten Quellen finden sich in den jeweiligen Dokumenten:

- 01-sensoren-fahrzeugdaten.md: Sensor-Anzapfung an der PD06
- 02-hardware-deep-dive.md: MCU, GPS, Display, IMU, Power-Management
- 03-software-stack.md: ESP-IDF, FreeRTOS, Treiber, BLE, OTA
- 04-pd06-technische-daten.md: Motor, Getriebe, Elektrik, Fahrwerk
- 05-gehaeuse-einbau-emv.md: Gehäuse, Wasserdichtigkeit, EMV
- 06-feature-roadmap.md: Vollständige Feature-Roadmap
- 07-display-navigation-smartphone.md: Multi-Display, Smartphone, Navigation

Zusätzliche Quellen für diesen Master-Plan:

- ESP-IDF Dokumentation: https://docs.espressif.com/projects/esp-idf/
- LVGL Dokumentation: https://docs.lvgl.io/
- FreeRTOS Dokumentation: https://www.freertos.org/
- NimBLE: https://github.com/h2zero/NimBLE-Arduino
- GxEPD2 (E-Paper): https://github.com/ZinggJM/GxEPD2
- JLCPCB: https://jlcpcb.com/
- GORE Protective Vents: https://www.gore.com/products/protective-vents

---

## 10. Anhang

### 10.1 BOM Zusammenfassung (Standard-Stufe)

Diese Tabelle ist die Einkaufsliste für die empfohlene Standard-Stufe.

| # | Komponente | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) | Lieferant |
|---|---|---|---|---|---|---|
| 1 | ESP32-S3-WROOM-1 N8R2 | 8MB Flash, 2MB PSRAM | 1 | 8 | 8 | Mouser |
| 2 | u-blox NEO-M9N | GPS, 4-GNSS, 25Hz | 1 | 30 | 30 | Mouser |
| 3 | Active Patch Antenne 25x25 | GPS-Antenne mit Verstärker | 1 | 8 | 8 | Mouser |
| 4 | Waveshare 3.5" IPS | 480x320, 1000 nits, ILI9488 | 1 | 18 | 18 | Waveshare |
| 5 | E-Paper 2.13" | 250x122, SSD1680 | 1 | 12 | 12 | Waveshare |
| 6 | MicroSD Breakout | SPI, Standard | 1 | 2 | 2 | Amazon |
| 7 | SanDisk Ultra 16GB | microSD, Class 10 | 1 | 6 | 6 | Amazon |
| 8 | BMI270 | IMU, 6-Achsen, I2C | 1 | 5 | 5 | Mouser |
| 9 | BMP388 | Barometer, I2C | 1 | 5 | 5 | Mouser |
| 10 | DS18B20 | wasserdicht, 1-Wire | 2 | 3 | 6 | Mouser |
| 11 | VDO Öldrucksensor | 0-5 bar, 10-180 Ohm | 1 | 25 | 25 | KFZ-Shop |
| 12 | PC817 Optokoppler | RPM-Pickup | 2 | 0.50 | 1 | Mouser |
| 13 | Zener-Diode 3.3V | Überspannungsschutz | 2 | 0.20 | 0.40 | Mouser |
| 14 | MP1584EN Modul | Buck 12V->5V | 1 | 2 | 2 | Amazon |
| 15 | TVS-Diode SMAJ15A | Lastabwurf-Schutz | 2 | 0.50 | 1 | Mouser |
| 16 | HT7333 LDO | 5V->3.3V, low noise | 1 | 0.50 | 0.50 | Mouser |
| 17 | 18650 NCR18650B | 3400 mAh Li-Ion | 1 | 6 | 6 | Conrad |
| 18 | TP4056 Modul | Lademodul mit Schutz | 1 | 0.50 | 0.50 | Amazon |
| 19 | MT3608 Modul | Boost 3.7V->5V | 1 | 0.80 | 0.80 | Amazon |
| 20 | 1N5822 Schottky | Power-Path ORing | 2 | 0.10 | 0.20 | Mouser |
| 21 | Piezo-Buzzer | Passiv | 1 | 1 | 1 | Amazon |
| 22 | Relais-Modul 4-Kanal | SRD-12V, Optokoppler | 1 | 4 | 4 | Amazon |
| 23 | IRLZ44N MOSFET | Heizgriffe PWM | 1 | 1.50 | 1.50 | Mouser |
| 24 | TC4427 Gate-Driver | 3.3V->12V Gate-Drive | 1 | 2 | 2 | Mouser |
| 25 | ASA Filament 1kg | UV-beständig, 100°C Tg | 1 | 5 | 5 | Prusa |
| 26 | Silikon-O-Ring | Dichtung, -40 bis +200°C | 2 | 1 | 2 | Amazon |
| 27 | PG7 Cable-Gland | IP67, für Sensorkabel | 4 | 1 | 4 | Amazon |
| 28 | PG9 Cable-Gland | IP67, für Starkstrom | 2 | 1.50 | 3 | Amazon |
| 29 | GORE PolyVent XS | Druckausgleich, IP68 | 1 | 8 | 8 | Gore |
| 30 | RAM-Mount Kugelgelenk | Lenkerhalterung 22mm | 1 | 8 | 8 | Amazon |
| 31 | ATO Sicherungskasten | 6-fach, wasserdicht | 1 | 5 | 5 | Amazon |
| 32 | Verbrauchsmaterial | Kabel, Widerstände, Kondensatoren | — | 5 | 5 | Mouser |
| | | | | **Gesamt** | **~181** | |
| | | | | Puffer 15% | 27 | |
| | | | | **Budget** | **~208** | |

Hinweis: Die tatsächlichen Preise können je nach Quelle und
Bestellmenge um +/- 20% variieren. Die Puffer von 15% deckt
Versandkosten und Preisschwankungen ab. Bei Bestellung mehrerer
Komponenten bei einem Lieferanten (Mouser, DigiKey) fallen
Versandkosten nur einmal an.

### 10.2 Pin-Belegungs-Referenz

Die vollständige Pin-Belegung ist in 02-hardware-deep-dive.md
(Abschnitt 10) dokumentiert. Hier die Zusammenfassung der
wesentlichen Pins:

```
SPI2 (FSPI) — TFT-Display:
  MOSI=GPIO11, SCK=GPIO12, CS=GPIO10, DC=GPIO9, RST=GPIO14, BUSY=GPIO13

SPI3 (HSPI) — SD-Karte + E-Paper:
  MOSI=GPIO35, SCK=GPIO36, MISO=GPIO37, CS_SD=GPIO38, CS_EPD=GPIO39

I2C Bus 0 — Sensoren:
  SDA=GPIO8, SCL=GPIO9
  BMI270 (0x68), BMP388 (0x76)

UART0 — GPS:
  RX=GPIO43, TX=GPIO44

ADC1 — Analoge Sensoren:
  GPIO1=VBAT, GPIO2=FUEL, GPIO3=TEMP_ENGINE

PCNT — Pulse Counter:
  GPIO4=TACHO (Rad), GPIO5=RPM (Zündung)

GPIO — Steuerung:
  GPIO6=HEAT_GRIP (PWM), GPIO7=AUX_LIGHT, GPIO15=USB_POWER, GPIO16=AUX_SOCKET
  GPIO17=STATUS_LED, GPIO0=BOOT_BTN
```

Hinweis: Bei Verwendung des E-Paper-Zweitdisplays auf SPI3 teilt
es den Bus mit der SD-Karte über unterschiedliche CS-Pins. Die
SPI-Takt-Frequenz muss auf den langsamsten Teilnehmer abgestimmt
sein (E-Paper typisch 10-20 MHz, SD-Karte bis 40 MHz).

### 10.3 Meilenstein-Definitionen

| Meilenstein | Ende | Kriterium | Test |
|---|---|---|---|
| M1 | Woche 2 | GPS-Track-Logger läuft | 1 km zu Fuß gehen, GPX-Datei auf PC auswerten |
| M2 | Woche 4 | Alle Must-Sensoren liefern Daten | Testfahrt, Vergleich mit Tacho/Thermometer |
| M3 | Woche 6 | Premium-UI auf TFT | Testfahrt mit voller UI, Subjective-Quality-Review |
| M4 | Woche 8 | BLE + Smartphone-App | App verbindet, zeigt Live-Daten, Navigation funktioniert |
| M5 | Woche 10 | Safety + Diagnose | HIL-Tests bestanden, Crash-Test simuliert, Self-Test ok |

### 10.4 Glossar

| Begriff | Bedeutung |
|---|---|
| ADC | Analog-to-Digital Converter |
| BLE | Bluetooth Low Energy |
| BOM | Bill of Materials (Stückliste) |
| CDI | Capacitive Discharge Ignition (Kapazitive Zündung) |
| CEP | Circular Error Probable (GPS-Genauigkeitsmaß) |
| DTC | Diagnostic Trouble Code (Fehlercode) |
| EMV | Elektromagnetische Verträglichkeit |
| EMI | Electromagnetic Interference |
| ESP-IDF | Espressif IoT Development Framework |
| GATT | Generic Attribute Profile (BLE) |
| GPX | GPS Exchange Format (XML-Track-Format) |
| HIL | Hardware-in-the-Loop |
| IMU | Inertial Measurement Unit |
| I2C | Inter-Integrated Circuit (Bus) |
| LDO | Low Dropout Regulator |
| LVGL | Light and Versatile Graphics Library |
| MOSFET | Metal-Oxide-Semiconductor Field-Effect Transistor |
| MTU | Maximum Transmission Unit (BLE-Paketgröße) |
| NimBLE | BLE-Stack für ESP32 (alternative zu Bluedroid) |
| NVS | Non-Volatile Storage (ESP32 Flash) |
| OTA | Over-The-Air (Firmware-Update) |
| PD06 | Honda Transalp 600 (interne Honda-Bezeichnung) |
| PCNT | Pulse Counter (ESP32-Peripherie) |
| PSRAM | Pseudo-Static RAM (externer RAM) |
| PWA | Progressive Web App |
| RTC | Real Time Clock |
| SPI | Serial Peripheral Interface (Bus) |
| TCI | Transistor Coil Ignition (Transistor-Zündung) |
| TFT | Thin-Film Transistor (Display-Technologie) |
| TPMS | Tire Pressure Monitoring System |
| TVS | Transient Voltage Suppressor (Überspannungsschutz) |
| UART | Universal Asynchronous Receiver-Transmitter |
| UBX | u-blox Binary Protocol |
| ULP | Ultra-Low-Power Coprozessor (ESP32) |

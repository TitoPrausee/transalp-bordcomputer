# Hardware-Test-Bench & Signal-Simulator-Spezifikation

> Bordcomputer Transalp 600 (PD06)
> MCU: ESP32-S3-WROOM-1 (N16R8: 16MB Flash, 8MB Octal PSRAM)
> Stand: Oktober 2026
> Quellen: docs/TEST-STRATEGY.md, docs/HARDWARE.md, docs/PINMAP.md,
> docs/BOM.md, docs/research/04-pd06-technische-daten.md,
> firmware/main/board_config.h

---

## 0. Zusammenfassung

Dieses Dokument spezifiziert den Aufbau einer Hardware-Test-Bench fuer den
Transalp Bordcomputer. Die Test-Bench ermoeglicht die vollstaendige
Funktionspruefung aller Subsysteme ohne das Motorrad. Statt echter
Fahrzeugsignale werden elektronische Simulatoren eingesetzt, die die
Sensor-signale des PD06 (Drehzahl, Geschwindigkeit, Kuehlmitteltemperatur,
Tank-stand, Bordspannung, Oeldruck, IMU, GPS) in definierter Weise nachbilden.

Die Test-Bench besteht aus drei Ebenen:

1. **Signal-Simulatoren** — aktive und passive Bauteile, die die
   Motorsensor-signale erzeugen (Funktionsgenerator fuer RPM,
   Widerstandsdekade fuer NTC, Spannungsteiler fuer analoge Sensoren,
   aufgezeichnete NMEA-Daten fuer GPS).

2. **Mess-Schnittstellen** — Test-Points auf dem PCB, Strommess-Shunts
   fuer Power-Profiling, Oszilloskop-Messpunkte fuer Signal-Integritaet.

3. **Automatisierung** — ein serieller Test-Controller, der ueber USB-C
   Befehle an den Bordcomputer sendet, Messwerte ausliest und
   Pass/Fail-Entscheidungen trifft.

Die Test-Bench ist so konzipiert, dass sie mit handelsueblichen
Laborgeraeten und preisguenstigen Modulen aufgebaut werden kann. Alle
Spezifikationen beziehen sich auf die in docs/HARDWARE.md und
docs/PINMAP.md dokumentierte Hardware.

---

## 1. Test-Bench Architektur

### 1.1 Uebersicht

Die Test-Bench umgibt den Bordcomputer (Device Under Test, DUT) mit
Signalquellen, Messgeraeten und einem Steuerrechner. Der DUT wird auf
einem Steckbrett oder in einem Test-Rig montiert, das alle
Sensorleitungen, Stromversorgung und Kommunikationsschnittstellen
zusammenfuehrt.

```
┌──────────────────────────────────────────────────────────────────────┐
│                       HARDWARE TEST-BENCH                            │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌─────────────┐    USB-C     ┌──────────────────┐                 │
│  │ Steuer-PC   │◄────────────►│  Bordcomputer     │                 │
│  │ (Test-      │  Serial +   │  (ESP32-S3 DUT)   │                 │
│  │  Runner)    │  Flashing   │                    │                 │
│  └──────┬──────┘             │  GPIO1  ◄── Batt   │                 │
│         │                    │  GPIO2  ◄── Temp   │                 │
│         │                    │  GPIO4  ◄── Fuel   │                 │
│         │                    │  GPIO5  ◄── Oil    │                 │
│         │                    │  GPIO15 ◄── RPM     │                 │
│         │                    │  GPIO16 ◄── Tacho  │                 │
│         │                    │  GPIO8/9 ◄── I2C    │                 │
│         │                    │  GPIO44 ◄── GPS RX  │                 │
│         │                    │  GPIO48 ◄── IMU INT │                 │
│         │                    └─────────┬──────────┘                 │
│         │                              │                              │
│         ▼                              ▼                              │
│  ┌──────────────────────────────────────────────────┐               │
│  │              SIGNAL-SIMULATOREN                    │               │
│  │                                                    │               │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐       │               │
│  │  │ RPM-Sim  │  │Temp-Sim  │  │ Fuel-Sim  │       │               │
│  │  │Funktions-│  │Widerst.- │  │Poti/     │       │               │
│  │  │generator │  │dekade    │  │DAC       │       │               │
│  │  └──────────┘  └──────────┘  └──────────┘       │               │
│  │                                                    │               │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐       │               │
│  │  │ Oil-Sim  │  │Batt-Sim  │  │ Tacho-Sim│       │               │
│  │  │Poti/DAC  │  │Labornetz-│  │Funktions-│       │               │
│  │  │          │  │teil      │  │generator │       │               │
│  │  └──────────┘  └──────────┘  └──────────┘       │               │
│  │                                                    │               │
│  │  ┌──────────┐  ┌──────────┐                       │               │
│  │  │ IMU-Rig  │  │ GPS-Sim  │                       │               │
│  │  │MPU6050   │  │NMEA-     │                       │               │
│  │  │Neigung   │  │Replay    │                       │               │
│  │  └──────────┘  └──────────┘                       │               │
│  └──────────────────────────────────────────────────┘               │
│                                                                      │
│  ┌──────────────────────────────────────────────────┐               │
│  │              MESS-INSTRUMENTE                      │               │
│  │                                                    │               │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐       │               │
│  │  │Strommess-│  │Oszillo-  │  │Logik-    │       │               │
│  │  │geraet    │  │skop     │  │analysator│       │               │
│  │  │(µA range)│  │(4-Kanal) │  │(8-Kanal) │       │               │
│  │  └──────────┘  └──────────┘  └──────────┘       │               │
│  └──────────────────────────────────────────────────┘               │
└──────────────────────────────────────────────────────────────────────┘
```

### 1.2 Steuerrechner (Test-Runner)

Der Steuerrechner ist ein Standard-PC oder Laptop mit USB-Anschluss.
Er kommuniziert mit dem DUT ueber USB-C (USB-CDC-Serial) und fuehrt
folgende Aufgaben aus:

- **Firmware-Flash:** Ueber `esptool.py` oder `idf.py flash` wird die
  zu testende Firmware auf den ESP32-S3 geflasht.
- **Test-Steuerung:** Ueber die serielle Konsole werden Test-Befehle
  gesendet (z.B. `selftest`, `benchmark tft`, `benchmark ble`).
- **Messwert-Erfassung:** Die serielle Ausgabe des DUT wird mitge-
  schnitten und ausgewertet (JSON-Format fuer automatisierte Parsing).
- **Messgeraete-Ansteuerung:** Bei Bedarf steuert der Steuerrechner
  auch die angeschlossenen Messgeraete (Oszilloskop ueber USB-TMC,
  Strommessgeraet ueber VCP).
- **Ergebnis-Dokumentation:** Test-Ergebnisse werden in einer
  CSV/JSON-Datei gespeichert und mit Build-Nummer und Datum versehen.

Die Automatisierung verwendet ein Python-Testskript, das die
serielle Schnittstelle oeffnet, Befehle sendet, auf Antworten wartet
und Pass/Fail entscheidet. Das Skript kann in den CI/CD-Pipeline
integriert werden.

### 1.3 Test-Rig Mechanik

Das Test-Rig ist eine mechanische Vorrichtung, die folgende Komponenten
aufnimmt:

- **DUT-Platine:** Der Bordcomputer (DevKit oder Custom-PCB) wird auf
  einer Acrylplatte (200x150mm, 5mm) mit M2.5-Abstandshaltern
  befestigt.
- **Signal-Generator-Board:** Ein separates Breadboard oder
  Perfboard (160x100mm) nimmt die Simulator-Schaltungen auf
  (Widerstandsdekaden, Potentiometer, DAC-Module).
- **Verdrahtung:** Alle Verbindungen zwischen DUT und Simulatoren
  erfolgen ueber steckbare Jumper-Wire (Dupont, 2.54mm). Jede Leitung
  ist beschriftet.
- **IMU-Test-Rig:** Eine kleine Neigungsvorrichtung (Goniometer oder
  einfacher Winkelblock) fuer definierte Winkel von 0-90 Grad, auf
  der der IMU-Sensor montiert wird.
- **Stromversorgung:** Ein Labornetzteil (0-30V, 0-3A) mit
  einstellbarer Spannung simuliert das 12V Bordnetz. Digitalmultimeter
  mit µA-Aufloesung fuer Strommessung im Deep-Sleep.

---

## 2. Signal-Simulatoren

### 2.1 RPM-Simulator (Drehzahl)

Die Drehzahlmessung erfolgt ueber Zuendimpulse am PCNT0-Eingang
(GPIO15). Das PD06 hat einen V2-Motor mit 1 Pickup-Puls pro
Kurbelwellenumdrehung. Die Frequenz entspricht:

    f = n_mot / 60

Bei Leerlauf (1300 U/min): f = 21,7 Hz
Bei Nenndrehzahl (8000 U/min): f = 133,3 Hz
Bei Rotationsbereich (max 9000 U/min): f = 150 Hz

Der Simulator muss Rechteckimpulse im Bereich 1-150 Hz erzeugen, die
den Optokoppler-Eingang (GPIO15, PCNT0) treiben. Da der Optokoppler
und Schmitt-Trigger bereits auf dem PCB sitzen, wird das Signal direkt
an den PCNT-Eingang eingespeist (3.3V-Pegel).

**Option A: Funktionsgenerator (empfohlen)**

Ein handelsueblicher Funktionsgenerator (Direkt-Digital-Synthesis)
mit Rechteckausgang (3.3V, TTL-Pegel) wird direkt an GPIO15
angeschlossen. Vorteile: hoechste Genauigkeit, einstellbare Frequenz
und Tastverhaeltnis, stoerungsarmer Ausgang.

Einstellbare Parameter:
- Frequenz: 1 Hz bis 200 Hz (entspricht 60 bis 12000 U/min)
- Amplitude: 3.3V (TTL-Pegel)
- Tastverhaeltnis: 50% (Impulsbreite irrelevant fuer PCNT, der
  Flanken zaehlt)
- Ausgangsimpedanz: 50 Ohm (Standard)

**Option B: Zweiter ESP32 als PWM-Generator**

Ein zweiter ESP32-S3-DevKit erzeugt PWM-Signale ueber den LEDC-
Peripherieblock. Vorteil: programmierbare Frequenzverlaeufe (Rampen,
Spruenge, Rauschen). Nachteil: etwas geringere Frequenzgenauigkeit.

Belegung des zweiten ESP32:
- GPIO (beliebig) -> Jumper-Wire -> DUT GPIO15 (RPM_PULSE)
- GND -> GND (gemeinsame Masse zwingend erforderlich)

Test-Frequenzen (PD06-spezifisch):

| Drehzahl [U/min] | Frequenz [Hz] | Beschreibung |
|---|---|---|
| 0 | 0 | Motor aus |
| 1300 | 21,7 | Leerlauf (PD06 Service-Manual: 1300 ± 100) |
| 2000 | 33,3 | Schonbereich |
| 4000 | 66,7 | Fahrbetrieb |
| 6000 | 100,0 | Drehmomentmaximum |
| 8000 | 133,3 | Nenndrehzahl (37 kW / 50 PS) |
| 9000 | 150,0 | Roter Bereich (CDI-Begrenzung) |

### 2.2 Tacho-Simulator (Geschwindigkeit)

Die Geschwindigkeitsmessung erfolgt ueber einen Reed-Kontakt oder
Hall-Sensor am Vorderrad (PCNT1, GPIO16). Der PD06 hat einen
Reifenumfang vorne von 2,185 m (90/90-21). Bei 100 km/h ergibt sich:

    n_rad = 100000 / 3600 / 2,185 = 12,72 Hz

Der Simulator muss Impulse im Bereich 0-30 Hz erzeugen (0 bis ca.
235 km/h). Die Implementierung erfolgt wie beim RPM-Simulator ueber
einen Funktionsgenerator oder einen zweiten ESP32 mit PWM-Ausgang.

Test-Frequenzen (PD06-spezifisch):

| Geschw. [km/h] | Frequenz [Hz] | Beschreibung |
|---|---|---|
| 0 | 0 | Stand |
| 30 | 3,82 | Stadtverkehr |
| 50 | 6,36 | innerorts |
| 80 | 10,18 | Landstrasse |
| 100 | 12,72 | Landstrasse |
| 130 | 16,54 | Autobahn |
| 163 | 20,74 | Hoechstgeschwindigkeit (Werksangabe) |

### 2.3 Kuehlmitteltemperatur-Simulator

Die Kuehlmitteltemperatur wird ueber einen NTC-Thermistor (10k bei
25 Grad C, B3950) im Spannungsteiler gemessen. Der ADC liest die
Spannung am Teilerpunkt (GPIO2, ADC1_CH1).

Statt einer temperaturregelten Fluessigkeit zu verwenden, wird eine
Widerstandsdekade eingesetzt, die den NTC-Widerstand bei verschiedenen
Temperaturen simuliert.

NTC-Widerstandswerte (B3950, R25=10k):

| Temperatur [Grad C] | NTC-Widerstand [Ohm] | ADC-Spannung [V] |
|---|---|---|
| 0 | 33000 | 2,52 |
| 20 | 12500 | 1,83 |
| 25 | 10000 | 1,65 |
| 40 | 5300 | 1,15 |
| 60 | 2470 | 0,66 |
| 80 | 1250 | 0,37 |
| 95 | 740 | 0,23 |
| 100 | 680 | 0,21 |
| 110 | 520 | 0,16 |
| 120 | 420 | 0,13 |

PD06-relevante Temperaturpunkte:
- Thermostat oeffnet bei 80-84 Grad C
- Thermostat voll offen bei 95 Grad C
- Kuehlerluefter schaltet bei 98-102 Grad C ein, bei 93-97 Grad C aus
- Uebertemperatur-Warnung: 110 Grad C
- Kritische Uebertemperatur: 120 Grad C (DTC gesetzt)

**Implementierung:** Widerstandsdekade (1 Ohm bis 100k Ohm,
1% Genauigkeit) oder ein programmierbares Widerstands-Modul
(Digital Potentiometer, z.B. MCP4151, 0-100k Ohm, 257 Stufen).
Der Simulator wird anstelle des NTC in den Spannungsteiler
eingeschleift (zwei Klemmen: ADC-Pin und GND).

### 2.4 Kraftstoffstand-Simulator

Der Kraftstoffstandsensor ist ein variabler Widerstand im Tank
(Honda-typisch 5-90 Ohm oder Aftermarket 0-190 Ohm). Er wird als
Spannungsteiler mit 100 Ohm Referenzwiderstand ausgelesen (GPIO4,
ADC1_CH3).

Der Simulator ist ein einstellbarer Widerstand (Potentiometer oder
Widerstandsdekade) im Bereich 0-200 Ohm.

Kraftstoffstand vs. Widerstand (Aftermarket 0-190 Ohm):

| Tankfuellung | Widerstand [Ohm] | ADC-Spannung [V] | Liter (18L Tank) |
|---|---|---|---|
| Leer (0%) | 190 | 1,56 | 0 |
| 25% | 143 | 1,37 | 4,5 |
| 50% | 95 | 1,18 | 9,0 |
| 75% | 48 | 0,66 | 13,5 |
| Voll (100%) | 5 | 0,16 | 18,0 |

PD06-spezifisch: Der Tank hat 18,0 Liter Volumen mit 2,9-3,5 Liter
Reserve. Die Reserve-Warnung wird bei unter 3 Liter ausgeloest.

### 2.5 Bordspannungs-Simulator

Die Bordspannung wird ueber einen Spannungsteiler (100k/10k, 10:1)
an GPIO1 (ADC1_CH0) gemessen. Der Simulator ist das Labornetzteil,
das die 12V-Versorgung des Bordcomputers liefert. Durch Verstellen
der Ausgangsspannung werden verschiedene Betriebszustaende simuliert.

Test-Szenarien (PD06-spezifisch):

| Bordspannung [V] | ADC-Wert [V] | Beschreibung |
|---|---|---|
| 8,0 | 0,80 | Anlassen (Spannungseinbruch) |
| 11,5 | 1,15 | Batterie schwach |
| 12,0 | 1,20 | Motor aus, Batterie voll |
| 13,0 | 1,30 | Batterie voll geladen |
| 13,8 | 1,38 | Generator laeuft (Ladebeginn) |
| 14,2 | 1,42 | Normaler Ladebetrieb |
| 15,0 | 1,50 | Ueberladung (Regler defekt) |
| 0,0 | 0,00 | Spannungsausfall (Power-Loss-Test) |

Generator-Spezifikation PD06: 310W (V-H bis V-J, V-R bis V-X) bzw.
350W (V-K bis V-P) bei 5000 U/min. Ladespannung 13,5-15,5 V.

### 2.6 Oeldruck-Simulator

Der Oeldrucksensor (0-10 bar, 0,5-4,5V Ausgang) wird ueber einen
Spannungsteiler (1:2) an GPIO5 (ADC1_CH4) gemessen. Der Simulator
ist ein einstellbares Gleichspannungsquelle (0-2,5V) oder ein
Potentiometer im Spannungsteiler.

Test-Szenarien (PD06-spezifisch):

| Oeldruck [bar] | Sensor-Spannung [V] | ADC-Spannung [V] | Beschreibung |
|---|---|---|---|
| 0,0 | 0,50 | 0,25 | Warnschalter aktiv (< 0,2-0,4 bar) |
| 0,5 | 0,75 | 0,38 | Leerlauf warm |
| 1,0 | 1,00 | 0,50 | Leerlauf heiss |
| 4,5 | 2,75 | 1,38 | Fahrbetrieb @ 6000 U/min |
| 6,0 | 3,25 | 1,63 | Kaltstart (max) |
| 10,0 | 4,50 | 2,25 | Sensor-Maximum |

PD06 Service-Manual: Oeldruck 190-588 kPa (1,9-5,9 bar) bei 8000
U/min. Warnschalter schaltet bei ca. 0,2-0,4 bar ab. Oelfuellmenge
2,10 Liter (Oelwechsel) bzw. 2,30 Liter (mit Filter).

### 2.7 IMU-Simulator (Beschleunigung/Neigung)

Der IMU (MPU6050, I2C Adresse 0x68) misst Beschleunigung und
Drehrate. Fuer den Test wird ein physisches Test-Rig verwendet, das
den IMU in definierte Winkel bringt.

**Test-Rig Aufbau:**

Eine Acrylplatte (100x80mm, 5mm) wird mit einer Schraubklemme an
einem Goniometer (Winkelmesser mit Skala, 0-90 Grad, 1 Grad
Aufloesung) befestigt. Der IMU wird mittig auf der Platte
montiert. Durch Drehen des Goniometers werden definierte
Neigungswinkel eingestellt.

Test-Winkel:

| Winkel [Grad] | Roll/Pitch | Erwartete Accel [g] | Beschreibung |
|---|---|---|---|
| 0 | 0 | ax=0, ay=0, az=1,0 | Ebene, Motorrad steht |
| 10 | 10 | az=0,98, ax=0,17 | Leichte Schraege |
| 20 | 20 | az=0,94, ax=0,34 | Maessige Schraege |
| 30 | 30 | az=0,87, ax=0,50 | Steile Schraege |
| 45 | 45 | az=0,71, ax=0,71 | Sehr steil (extrem) |
| 90 | 90 | az=0, ax=1,0 | Seitenlage (gefallen) |

Zusaetzliche Tests:
- **Crash-Simulation:** Schelle auf das Test-Rig (manueller Schlag
  mit definiertem Impuls, z.B. 5g fuer 50ms). Prueft, ob die
  Crash-Erkennung anspricht und der SOS-Countdown startet.
- **Vibration:** Ein kleines Vibrationsmotor (z.B. aus Handy oder
  Spielzeug) wird auf das Test-Rig geklebt und mit verschiedenen
  Frequenzen (50-200 Hz) betrieben. Prueft, ob die
  Complementary-Filterung die Vibration herausfiltert.

### 2.8 GPS-Simulator

Das GPS-Modul kommuniziert ueber UART1 (115200 Baud, GPIO43 TX /
GPIO44 RX) mit NMEA-Saetzen (GGA, RMC, VTG). Der 1PPS-Ausgang
(GPIO39) liefert einen Impuls pro Sekunde.

**Option A: NMEA-Replay ueber UART (empfohlen)**

Der Steuerrechner oder ein zweiter ESP32 spielt aufgezeichnete
NMEA-Daten ueber UART1 ein. Die NMEA-Daten werden aus einer
vorher aufgezeichneten Fahrt oder aus einem GPS-Simulator-Tool
exportiert.

Beispiel-NMEA-Strom (mit variablen Geschwindigkeiten):

    $GPRMC,095500,A,5055.1234,N,01137.5678,E,045.3,180.0,131024,,,A*6A
    $GPGGA,095500,5055.1234,N,01137.5678,E,1,08,1.2,280.5,M,47.0,M,,*5C
    $GPRMC,095501,A,5055.1240,N,01137.5680,E,055.1,180.0,131024,,,A*69
    $GPGGA,095501,5055.1240,N,01137.5680,E,1,08,1.1,280.3,M,47.0,M,,*5D

Implementierung: Ein Python-Skript auf dem Steuerrechner oeffnet
den seriellen Port (USB-Serial-Adapter an GPIO44) und sendet die
NMEA-Saetze mit 1 Hz (oder konfigurierbarer Rate). Der 1PPS-Pin
kann ueber einen weiteren GPIO des Steuerrechners oder des zweiten
ESP32 simuliert werden (1 Hz Rechteck, 100ms HIGH).

**Option B: GPS-Simulator-Modul**

Ein dediziertes GPS-Simulator-Modul (z.B. basierend auf einem
SDR oder einem u-blox-Modul im Simulator-Modus) erzeugt echte
RF-Signale. Vorteil: realistischstes Test-Szenario. Nachteil: sehr
teuer (Laborgeraete ab 2000 EUR) und nicht fuer die Routinepruefung
erforderlich.

**Test-Szenarien:**

| Szenario | Geschwindigkeit | Sat-Anzahl | Beschreibung |
|---|---|---|---|
| Stand | 0 km/h | 8-12 | 3D-Fix, keine Bewegung |
| Stadt | 30-50 km/h | 6-10 | Maessige Multipath |
| Landstrasse | 80-100 km/h | 8-12 | Offenes Feld |
| Tunnel | 0 km/h | 0 | Signalverlust (Fix lost) |
| Kaltstart | — | — | Ohne Ephemeriden, Fix-Zeit messen |
| Warmstart | — | — | Mit Ephemeriden, Fix-Zeit messen |

### 2.9 BMP280 Barometer-Simulator

Der BMP280 (I2C Adresse 0x76) misst Luftdruck und Temperatur. Da
der BMP280 digital ist (keine analoge Schnittstelle), kann der
Sensor nicht direkt simuliert werden. Stattdessen wird der Sensor
physisch in eine Druckkammer (z.B. einen luftdichten Behaelter mit
Handpumpe) gebracht.

Alternativ: Ein zweiter BMP280 wird als Referenz betrieben, und die
Druckdifferenz zwischen beiden wird verglichen. Fuer die
Hoehenmeter-Berechnung wird der Meeresspiegeldruck (1013,25 hPa)
als Referenz verwendet.

Test-Werte:

| Hoehe [m ue. NN] | Druck [hPa] | Beschreibung |
|---|---|---|
| 0 | 1013,25 | Meeresspiegel |
| 200 | 990 | Flachland |
| 500 | 955 | Mittelgebirge |
| 1000 | 899 | Gebirge (z.B. Thueringer Wald) |
| 2000 | 795 | Hochgebirge (Alpenpass) |
| 2762 | 722 | Hoechster Pass der Transalp-Route (Stelvio) |

---

## 3. Test-Points und Mess-Schnittstellen

### 3.1 Test-Points auf dem PCB

Um Signal-Integritaet und Spannungspegel waehrend des Tests
ueberpruefen zu koennen, werden Test-Points auf dem PCB
vorgesehen. Diese sind als 1mm-Lotpads (oder 2.54mm Header-Pins)
ausgefuehrt und auf dem Siebdruck mit TP-Nummern beschriftet.

| TP-Nummer | Signal | GPIO | Messzweck |
|---|---|---|---|
| TP01 | 3.3V Rail | — | Spannungsversorgung pruefen |
| TP02 | 5V Rail | — | Buck-Converter Ausgang |
| TP03 | 12V Input | — | Bordspannung vor Buck |
| TP04 | GND | — | Referenzmasse |
| TP05 | BATT_VOLTAGE | GPIO1 | ADC-Spannung Bordspannung |
| TP06 | COOLANT_TEMP | GPIO2 | ADC-Spannung NTC |
| TP07 | FUEL_LEVEL | GPIO4 | ADC-Spannung Tankgeber |
| TP08 | OIL_PRESSURE | GPIO5 | ADC-Spannung Oeldruck |
| TP09 | RPM_PULSE | GPIO15 | PCNT0-Eingang (Rechteck) |
| TP10 | TACHO_PULSE | GPIO16 | PCNT1-Eingang (Rechteck) |
| TP11 | I2C_SDA | GPIO8 | I2C-Bus Data |
| TP12 | I2C_SCL | GPIO9 | I2C-Bus Clock |
| TP13 | GPS_TX | GPIO43 | UART1 TX (MCU zu GPS) |
| TP14 | GPS_RX | GPIO44 | UART1 RX (GPS zu MCU) |
| TP15 | GPS_PPS | GPIO39 | 1PPS-Impuls |
| TP16 | TFT_CS | GPIO10 | SPI Chip-Select TFT |
| TP17 | EPD_CS | GPIO40 | SPI Chip-Select E-Paper |
| TP18 | SD_CS | GPIO47 | SPI Chip-Select SD-Karte |
| TP19 | MOSI | GPIO11 | SPI MOSI (Shared) |
| TP20 | SCK | GPIO12 | SPI SCK (Shared) |
| TP21 | MISO | GPIO13 | SPI MISO (nur SD) |
| TP22 | STATUS_LED | GPIO21 | LED-Ausgang |
| TP23 | BUZZER | GPIO38 | Buzzer-Ausgang |
| TP24 | MPU6050_INT | GPIO48 | IMU-Interrupt |
| TP25 | EN | — | ESP32-S3 Enable/Reset |

Jeder Test-Point ist so platziert, dass er mit einer Oszilloskop-Sonde
(10:1, 1 MOhm Eingang) oder einem Logikanalysator erreicht werden
kann, ohne andere Bauteile zu beruehren.

### 3.2 Strommessung fuer Power-Profiling

Die Stromaufnahme des Bordcomputers wird in verschiedenen
Power-States gemessen. Dafuer wird ein Strommess-Shunt (0,1 Ohm,
1%, 3W) in die 12V-Versorgungsleitung eingeschleift. Der
Spannungsabfall ueber dem Shunt wird mit einem Digitalmultimeter
(mV-Bereich) oder Oszilloskop gemessen.

    I = U_Shunt / R_Shunt

Bei R_Shunt = 0,1 Ohm: 1 mV = 10 mA.

Alternativ: Verwendung eines USB-Strommessgeraets (z.B. für die
3.3V-Schiene) oder eines Labor-Strommessgeraets mit µA-Aufloesung.

Messpunkte fuer Power-Profiling:

| Messpunkt | Position | Methode |
|---|---|---|
| 12V Gesamtstrom | Vor Buck-Converter | Shunt 0,1 Ohm + Multimeter |
| 5V Rail | Nach Buck-Converter | USB-Strommesser |
| 3.3V Rail | Nach LDO | Shunt 0,1 Ohm + Multimeter |
| ESP32-S3 Strom | 3.3V am Modul | Labor-Strommesser (µA) |

### 3.3 Oszilloskop-Messpunkte fuer Signal-Integritaet

Ein 4-Kanal-Oszilloskop (mindestens 20 MHz Bandbreite, 100 MSa/s)
wird fuer folgende Messungen verwendet:

| Messung | Kanal | Test-Point | Trigger |
|---|---|---|---|
| RPM-Signal | CH1 | TP09 (GPIO15) | Steigende Flanke |
| Tacho-Signal | CH2 | TP10 (GPIO16) | Steigende Flanke |
| SPI SCK | CH3 | TP20 (GPIO12) | Fallende Flanke |
| SPI MOSI | CH4 | TP19 (GPIO11) | CS fallend (TP16) |
| I2C SDA | CH1 | TP11 (GPIO8) | SCL steigend (TP12) |
| GPS PPS | CH1 | TP15 (GPIO39) | Steigende Flanke |
| ADC-Coolant | CH1 | TP06 (GPIO2) | Freilauf |
| ADC-Fuel | CH2 | TP07 (GPIO4) | Freilauf |
| Backlight PWM | CH1 | GPIO18 | Steigende Flanke |

Signal-Integritaets-Kriterien:
- RPM-Rechteck: Steilheit > 1V/µs, kein Prellen, keine
 overshoots > 0,5V
- SPI-SCK: symmetrischer Takt, Jitter < 5 ns, keine
  Flankenschwingungen
- I2C: Steigende Flanke < 1 µs (bei 400 kHz), kein Bus-Contention
- ADC-Spannung: Rauschen < 10 mV peak-to-peak (bei 16x
  Oversampling)

### 3.4 Logikanalysator

Ein 8-Kanal-Logikanalysator (z.B. 24 MSa/s, 8 Kanal) wird fuer die
parallele Beobachtung mehrerer digitaler Signale verwendet:

| Kanal | Signal | Zweck |
|---|---|---|
| D0 | RPM_PULSE (GPIO15) | Zaehlen der Impulse |
| D1 | TACHO_PULSE (GPIO16) | Zaehlen der Impulse |
| D2 | GPS_PPS (GPIO39) | Zeitstempel-Referenz |
| D3 | MPU6050_INT (GPIO48) | IMU-Event-Erfassung |
| D4 | STATUS_LED (GPIO21) | Systemzustand |
| D5 | TFT_CS (GPIO10) | Display-Aktivitaet |
| D6 | SD_CS (GPIO47) | SD-Karten-Aktivitaet |
| D7 | BOOT_BTN (GPIO0) | Benutzereingabe |

Der Logikanalysator erfasst die zeitlichen Beziehungen zwischen
Signalen (z.B. Latenz zwischen RPM-Puls und Display-Update) und
erlaubt die Protocol-Decodierung von SPI, I2C und UART.

---

## 4. Automatisierung

### 4.1 Serielle Testschnittstelle

Der Bordcomputer bietet ueber USB-C (USB-CDC) eine serielle
Konsole mit Befehlsinterpreter. Diese Schnittstelle wird fuer
automatisierte Tests verwendet. Der Steuerrechner sendet
Text-Befehle, der Bordcomputer antwortet mit JSON-formatierten
Ergebnissen.

Unterstützte Test-Befehle:

| Befehl | Parameter | Antwort |
|---|---|---|
| `selftest` | — | Self-Test-Ergebnis als JSON |
| `benchmark tft` | Frames-Anzahl | FPS-Wert, Renderzeit |
| `benchmark epd` | Anzahl Updates | Update-Zeit |
| `benchmark ble` | Bytes senden | Durchsatz, Latenz |
| `benchmark sd` | Bytes schreiben | Schreibrate |
| `benchmark boot` | — | Boot-Zeit |
| `benchmark heap` | — | Freier Heap, PSRAM |
| `benchmark cpu` | Messdauer [s] | CPU-Last pro Task |
| `benchmark power` | State | Stromwert (falls Sensor) |
| `sensor read all` | — | Alle Sensorwerte als JSON |
| `sensor read <name>` | Sensor-Name | Einzelner Sensorwert |
| `dtc list` | — | Alle DTCs |
| `dtc clear` | — | DTCs loeschen |
| `gps status` | — | Fix-Status, Sat-Anzahl |
| `ble advertise on` | — | BLE-Advertising starten |
| `power state <s>` | active/dimmed/suspend/sleep | Power-State setzen |
| `log enable` | — | Logging aktivieren |
| `log disable` | — | Logging stoppen |
| `version` | — | Firmware-Version, Build-Datum |

Beispiel-Dialog:

```
>> selftest
<< {"cmd":"selftest","status":"ok","modules":[
     {"name":"ADC","ok":true},
     {"name":"I2C_MPU6050","ok":true},
     {"name":"I2C_BMP280","ok":true},
     {"name":"SPI_TFT","ok":true},
     {"name":"SPI_EPD","ok":true},
     {"name":"SPI_SD","ok":true},
     {"name":"UART_GPS","ok":true},
     {"name":"BLE","ok":true}
    ],"duration_ms":1247}

>> sensor read all
<< {"cmd":"sensor_read","values":{
     "rpm":3250,"speed":65.3,"coolant_temp":82,
     "fuel_level":8.5,"oil_pressure":3.2,
     "batt_voltage":14.1,"roll":2.3,"pitch":1.1,
     "altitude":280,"gps_fix":true,"gps_sats":9
    }}

>> benchmark tft 100
<< {"cmd":"benchmark_tft","frames":100,
     "total_ms":3120,"fps":32.05,
     "result":"PASS","target_fps":30}
```

### 4.2 Python-Test-Runner

Der automatisierte Test-Runner ist ein Python-Skript, das:

1. Die serielle Verbindung herstellt (pyserial, 115200 Baud).
2. Eine Konfigurationsdatei mit Test-Szenarien einliest (YAML).
3. Fuer jedes Szenario die Signal-Simulatoren einstellt (bei
   fernsteuerbaren Geraeten).
4. Den Test-Befehl an den DUT sendet.
5. Die JSON-Antwort auswertet und mit den Zielwerten vergleicht.
6. Das Ergebnis (PASS/FAIL) mit Zeitstempel speichert.

Test-Szenarien werden in einer YAML-Datei definiert:

    test_suite: "hw_bench_v1"
    tests:
      - id: TB-RPM-001
        name: "RPM Leerlauf"
        simulator:
          type: function_generator
          channel: rpm
          frequency_hz: 21.7
        dut_command: "sensor read rpm"
        expect:
          rpm: 1300
          tolerance_pct: 2
        timeout_s: 5

      - id: TB-TEMP-001
        name: "Kuehlmittel 95C"
        simulator:
          type: resistance_box
          channel: coolant
          resistance_ohm: 740
        dut_command: "sensor read coolant_temp"
        expect:
          coolant_temp: 95
          tolerance_c: 3
        timeout_s: 5

Der Test-Runner generiert einen Bericht im CSV-Format mit Spalten:
Test-ID, Name, Status (PASS/FAIL), Messwert, Zielwert, Toleranz,
Zeitstempel, Dauer.

### 4.3 CI/CD-Integration

Fuer die kontinuierliche Integration wird der Test-Runner in die
Build-Pipeline eingebunden. Nach jedem Firmware-Build:

1. Kompilierung der Firmware (ESP-IDF, idf.py build).
2. Flash auf den DUT (esptool.py).
3. Warte auf Boot (3 Sekunden).
4. Ausfuehrung des Self-Tests (`selftest` Befehl).
5. Ausfuehrung der automatisierten Benchmark-Suite.
6. Auswertung: wenn ein Test FAIL, wird der Build als fehlerhaft
   markiert.
7. Bericht wird als Artefakt gespeichert (JSON + CSV).

Nicht-automatisierbare Tests (manuelle Pruefung der Displays,
IMU-Neigung, physische Inspektion) werden in einer separaten
Checkliste gefuehrt und bei jedem Release manuell abgehakt.

---

## 5. Test-Prozeduren fuer Subsysteme

### 5.0 PCB-Inbetriebnahme (Bring-up) — Erst-Inbetriebnahme nach Bestueckung

Die Prozeduren §5.1–§5.10 setzen ein bereits bootfaehiges, lauffaehiges DUT
voraus ("Self-Test bestanden"). Fuer eine **frisch bestueckte Leiterplatte** ist
zuvor die gestaffelte Inbetriebnahme (Bring-up) noetig. Die detaillierte
Schritt-fuer-Schritt-Anleitung steht in
[`docs/ASSEMBLY-GUIDE.md`](ASSEMBLY-GUIDE.md) §5–§7; hier die
Test-Bench-spezifischen Eckpunkte.

#### 5.0.1 Pre-Power Kontinuitaets- und Kurzschlusstest

**Vorbedingung:** PCB bestueckt, nicht bestromt. Labornetzteil aus.
Testpunkte TP01 (3,3V) / TP02 (5V) / TP03 (12V) / TP04 (GND) — siehe §3.1.

| Schritt | Messung (Multimeter Widerstand) | Soll (unbestromt) |
|---|---|---|
| 1 | 12V ↔ GND (TP03↔TP04) | > 10 kΩ (ideal ∞) |
| 2 | 5V ↔ GND (TP02↔TP04) | > 1 kΩ |
| 3 | 3,3V ↔ GND (TP01↔TP04) | > 500 Ω |
| 4 | 12V ↔ 5V (TP03↔TP02) | > 10 kΩ |
| 5 | 5V ↔ 3,3V (TP02↔TP01) | > 1 kΩ |

Pass/Fail: alle Widerstaende im gruenen Bereich. Bei Abweichung: Kurzschluss
lokalisieren, Reflow pruefen, erst danach weiter mit Phase 1.

#### 5.0.2 Smoke-Test / Strombegrenztes Erst-Einschalten

**Vorbedingung:** §5.0.1 bestanden. Labornetzteil 12 V, Strombegrenzung 50 mA.

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | Netzteil an 12V-Eingang (TP03 / AMP Superseal 2-polig) | — |
| 2 | Strombegrenzung 50 mA, einschalten | Stromaufnahme < 50 mA |
| 3 | 5V-Schiene messen (TP02) | 5,0 V ± 0,1 V |
| 4 | 3,3V-Schiene messen (TP01) | 3,3 V ± 0,1 V |
| 5 | Strombegrenzung auf 200 mA hochregeln | Schienen stabil, kein Overcurrent |

Pass/Fail: beide Schienen im Toleranzbereich, kein Ueberstrom.
Bei Ueberstrom → sofort aus, Kurzschluss suchen. Siehe Debug-Tree ASSEMBLY-GUIDE §10.

#### 5.0.3 USB-C Erst-Flash & Boot-Verifikation

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | USB-C (Service-Port) an PC | Serieller Port erkannt |
| 2 | Bootloader: BOOT(GPIO0) + RESET | ESP32 im Download-Mode |
| 3 | `idf.py -p /dev/cu.usbmodem* flash monitor` | Flash + Monitor startet |
| 4 | Boot-Log beobachten | keine Panic/Backtrace, selftest OK |
| 5 | STATUS_LED (TP22) | Heartbeat-Blinken |

Pass/Fail: Firmware geflasht, Boot ohne Exception, STATUS_LED blinkt.

#### 5.0.4 I2C-Bus-Scan

**Vorbedingung:** §5.0.3 bestanden (MCU laeuft). Ueber serielle Test-Schnittstelle (§4.1):

```
i2c-scan
```

| Erwartete Adresse | Geraet |
|---|---|
| 0x68 | IMU (BMI270/MPU6050, AD0=LOW) |
| 0x76 | Barometer (BMP390/BMP388) |

Pass/Fail: beide Adressen gefunden. Fehlt eine → Verdrahtung SDA=8/SCL=9,
Pull-Ups 4,7 kΩ, VCC pruefen (siehe Debug-Tree ASSEMBLY-GUIDE §10).

#### 5.0.5 Gestaffeltes Bring-up (Uebersicht)

Vollstaendige Reihenfolge siehe ASSEMBLY-GUIDE §6:

1. Phase 1 — Power-Sektion (strombegrenzt, §5.0.2)
2. Phase 2 — ESP32-S3 Flash & Boot (§5.0.3)
3. Phase 3 — I2C-Bus-Scan (§5.0.4)
4. Phase 4 — SPI-Geraete (TFT, E-Paper, MicroSD) enumerieren
5. Phase 5 — Peripherie (ADC-Sensoren, RPM/Tacho, GPS, BLE, Buttons)

Erst nach Abschluss aller Phasen Übergang zu den funktionalen Subsystem-Tests §5.1–§5.15.

---

### 5.1 RPM/Tacho (PCNT) Test-Prozedur

**Vorbedingung:** DUT eingeschaltet, Self-Test bestanden,
Funktionsgenerator an GPIO15 (RPM) und GPIO16 (Tacho) angeschlossen.

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | Setze Frequenz auf 0 Hz (Motor aus) | RPM = 0, Speed = 0 km/h |
| 2 | Setze Frequenz auf 21,7 Hz (Leerlauf) | RPM = 1300 ± 26, Speed = 0 |
| 3 | Setze Frequenz auf 66,7 Hz (4000 U/min) | RPM = 4000 ± 80 |
| 4 | Setze Frequenz auf 133,3 Hz (8000 U/min) | RPM = 8000 ± 160 |
| 5 | Setze Frequenz auf 150 Hz (9000 U/min) | RPM = 9000 ± 180, roter Bereich |
| 6 | Tacho: 12,72 Hz (100 km/h) | Speed = 100,0 ± 2 km/h |
| 7 | Tacho: 20,74 Hz (163 km/h) | Speed = 163,0 ± 3 km/h |
| 8 | Frequenzsprung 21,7 -> 133,3 Hz | Latenz bis Display < 200 ms |
| 9 | Frequenz auf 0 Hz zurueck | RPM faellt auf 0 innerhalb 1s |

Pass/Fail: Alle Werte innerhalb Toleranz, Latenz < 200 ms.
Automatisierbar: Ja (fernsteuerbarer Funktionsgenerator erforderlich).

### 5.2 ADC-Sensor Test-Prozedur (Temperatur, Tank, Oeldruck)

**Vorbedingung:** DUT eingeschaltet, Widerstandsdekade an GPIO2
und Potentiometer an GPIO4/GPIO5 angeschlossen.

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | NTC-Widerstand 10000 Ohm (25 C) | Coolant = 25 ± 2 C |
| 2 | NTC-Widerstand 740 Ohm (95 C) | Coolant = 95 ± 3 C |
| 3 | NTC-Widerstand 420 Ohm (120 C) | Coolant = 120 ± 3 C, DTC gesetzt |
| 4 | NTC-Widerstand 33000 Ohm (0 C) | Coolant = 0 ± 2 C |
| 5 | Tank: 190 Ohm (leer) | Fuel = 0 ± 5%, Reserve-Warnung |
| 6 | Tank: 95 Ohm (halbvoll) | Fuel = 50 ± 5% |
| 7 | Tank: 5 Ohm (voll) | Fuel = 100 ± 5% |
| 8 | Oeldruck: 0,25V ADC (0 bar) | Oil = 0 bar, Warnung |
| 9 | Oeldruck: 1,38V ADC (4,5 bar) | Oil = 4,5 ± 0,3 bar |
| 10 | Oeldruck: 2,25V ADC (10 bar) | Oil = 10 ± 0,5 bar |

Pass/Fail: Alle Werte innerhalb Toleranz, DTC bei Uebertemperatur.
Automatisierbar: Teilweise (Digital-Poti fernsteuerbar, manuelle
Widerstandsdekade nur manuell).

### 5.3 Bordspannung Test-Prozedur

**Vorbedingung:** DUT am Labornetzteil, Multimeter am 12V-Eingang.

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | Netzteil 12,0V (Motor aus) | Batt = 12,0 ± 0,2V |
| 2 | Netzteil 13,8V (Generator) | Batt = 13,8 ± 0,2V, "Generator aktiv" |
| 3 | Netzteil 14,2V (Normalfahrt) | Batt = 14,2 ± 0,2V |
| 4 | Netzteil 8,0V (Anlassen) | Batt = 8,0 ± 0,2V, keine Resets |
| 5 | Netzteil 15,0V (Ueberladung) | Batt = 15,0 ± 0,2V, Warnung |
| 6 | Netzteil 0V (Power-Loss) | DTC gesetzt, SD-Daten gesichert |

Pass/Fail: Alle Werte innerhalb Toleranz, Power-Loss-Handling OK.
Automatisierbar: Ja (fernsteuerbares Labornetzteil).

### 5.4 Display Test-Prozedur

**Vorbedingung:** DUT eingeschaltet, TFT und E-Paper verbunden.

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | TFT: Vollbild-Update (480x320) | Aktualisierung sichtbar, < 33ms |
| 2 | TFT: 100 Frames Full-Refresh | FPS > 30, gemessen per `benchmark tft 100` |
| 3 | TFT: Backlight dimmen (PWM 0-100%) | Helligkeit linear veraenderlich |
| 4 | E-Paper: Full-Refresh | Update-Zeit < 2s, kein Ghosting |
| 5 | E-Paper: Partial-Refresh (10x) | Update-Zeit < 300ms pro Refresh |
| 6 | TFT + E-Paper gleichzeitig | Keine SPI-Kollision, korrekte Darstellung |
| 7 | TFT + SD-Karten-Schreiben gleichzeitig | Keine SPI-Kollision, keine Framedrops |

Pass/Fail: FPS > 30, E-Paper < 2s, keine SPI-Kollisionen.
Automatisierbar: Ja (FPS per `benchmark`-Befehl, visuelle Pruefung manuell).

### 5.5 BLE Test-Prozedur

**Vorbedingung:** DUT eingeschaltet, BLE advertising aktiv,
Companion-App oder BLE-Test-Tool auf dem Steuerrechner.

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | BLE-Scan auf dem Steuerrechner | Bordcomputer gefunden, RSSI > -80 dBm |
| 2 | Verbindungsaufbau | Verbunden in < 5s |
| 3 | MTU-Aushandlung (247 Bytes) | MTU = 247, DLE aktiv |
| 4 | Telemetry-Notify (1 Hz, 100 Updates) | Alle empfangen, Latenz < 100 ms |
| 5 | Write-Command (Konfiguration) | Bestaetigung < 50 ms |
| 6 | Durchsatz-Test (100 KB senden) | > 10 kbps sustained |
| 7 | Verbindungsabbruch und Wiederverbindung | Automatisches Reconnect < 10s |

Pass/Fail: Alle Latenzen und Durchsaetze innerhalb Zielwerte.
Automatisierbar: Ja (Python-BLE-Test-Tool).

### 5.6 GPS Test-Prozedur

**Vorbedingung:** DUT eingeschaltet, NMEA-Replay-Tool aktiv.

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | NMEA-Stream starten (Stand) | Fix in < 30s (Kaltstart simuliert) |
| 2 | NMEA: 0 km/h, 8 Satelliten | GPS-Fix = true, Speed = 0 |
| 3 | NMEA: 50 km/h | Speed = 50 ± 1 km/h |
| 4 | NMEA: 100 km/h | Speed = 100 ± 2 km/h |
| 5 | NMEA-Stream stoppen (Tunnel) | Fix = false nach 5s, DTC gesetzt |
| 6 | 1PPS-Signal pruefen | 1 Hz Rechteck, Jitter < 1 ms |
| 7 | GPS-Konfiguration pruefen | Update-Rate 1 Hz (oder 5 Hz konfiguriert) |

Pass/Fail: Fix-Zeit < 30s, Geschwindigkeit innerhalb Toleranz.
Automatisierbar: Ja (NMEA-Replay per Skript).

### 5.7 IMU Test-Prozedur

**Vorbedingung:** DUT eingeschaltet, IMU auf Test-Rig montiert.

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | Rig auf 0 Grad (ebene) | Roll = 0 ± 2, Pitch = 0 ± 2 |
| 2 | Rig auf 10 Grad Neigung | Pitch = 10 ± 2 |
| 3 | Rig auf 20 Grad Neigung | Pitch = 20 ± 2 |
| 4 | Rig auf 45 Grad Neigung | Pitch = 45 ± 3 |
| 5 | Rig auf 90 Grad (Seitenlage) | Roll = 90 ± 3, Crash-Erkennung? |
| 6 | Crash-Simulation (5g Schlag) | Crash-DTC gesetzt, SOS-Countdown |
| 7 | Vibrations-Test (50-200 Hz) | Roll/Pitch stabil, Rauschen < 2 Grad |

Pass/Fail: Winkel innerhalb Toleranz, Crash-Erkennung ansprechend.
Automatisierbar: Nein (manuelle Winkelstellung, manueller Schlag).

### 5.8 SD-Karten Logging Test-Prozedur

**Vorbedingung:** DUT eingeschaltet, MicroSD-Karte eingesetzt,
Logging aktiviert.

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | SD-Karte mounten | Erfolg, DTC frei |
| 2 | GPX-Log starten, 60s aufzeichnen | Datei vorhanden, > 50 KB |
| 3 | Schreibrate messen (`benchmark sd`) | > 50 kbps sustained |
| 4 | SD-Karte entfernen waehrend Betrieb | DTC C0101 gesetzt, Fallback RAM-Buffer |
| 5 | SD-Karte wieder einsetzen | Remount, Logging fortgesetzt |
| 6 | Power-Loss waehrend Schreiben | Letzte Daten konsistent, keine Korruption |
| 7 | SD-Karte voll (< 1 KB frei) | DTC C0103, Logging sauber gestoppt |

Pass/Fail: Schreibrate > 50 kbps, Fehlerbehandlung OK.
Automatisierbar: Teilweise (Schreibrate per Befehl, Power-Loss manuell).

### 5.9 Power-Consumption Test-Prozedur

**Vorbedingung:** DUT am Labornetzteil, µA-Multimeter in
Strommessschleife.

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | Power-State: ACTIVE | Strom < 150 mA @ 12V |
| 2 | Power-State: DIMMED | Strom < 80 mA @ 12V |
| 3 | Power-State: SUSPENDED | Strom < 20 mA @ 12V |
| 4 | Power-State: DEEP_SLEEP | Strom < 5 mA @ 12V |
| 5 | Wake from DEEP_SLEEP (GPIO0) | Boot < 3s, alle Module aktiv |
| 6 | Wake from DEEP_SLEEP (IMU-INT) | Boot < 3s, Crash-Check |

Pass/Fail: Alle Stromwerte unter Zielwerten, Wake-Zeit < 3s.
Automatisierbar: Ja (Power-State per `power state` Befehl, Strom
per Multimeter mit serieller Schnittstelle).

### 5.10 Boot-Zeit Test-Prozedur

**Vorbedingung:** DUT ausgeschaltet, Oszilloskop an GPIO21
(STATUS_LED) fuer Trigger.

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | Strom einschalten (12V) | — |
| 2 | Warte bis STATUS_LED ersten Herzschlag zeigt | Zeit messen |
| 3 | Warte bis TFT erstes Bild zeigt | Zeit messen |
| 4 | Warte bis Self-Test abgeschlossen | Zeit messen, < 3s gesamt |
| 5 | Pruefe `version` Ausgabe | Firmware-Version korrekt |

Pass/Fail: Boot bis Self-Test < 3s.
Automatisierbar: Ja (Zeitmessung per Oszilloskop oder `benchmark boot`).

### 5.11 Taster Test-Prozedur

**Vorbedingung:** DUT bootfaehig (§5.0.3). Taster: BOOT(GPIO0), MENU(GPIO6),
SELECT(GPIO7), BACK(GPIO3) — alle Active-LOW mit internem Pull-Up.

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | `button-test` ueber serielle Schnittstelle | Test-Modus aktiv |
| 2 | BOOT-Taster druecken | Event "BOOT" protokolliert |
| 3 | MENU-Taster druecken | Event "MENU" |
| 4 | SELECT-Taster druecken | Event "SELECT" |
| 5 | BACK-Taster druecken | Event "BACK" |

Pass/Fail: jeder Taster erzeugt genau ein Event pro Druck (entprellt).
Hinweis: GPIO3 ist Strapping-Pin (JTAG-Select) — beim Boot muss er HIGH sein
(Pull-Up). Bei Fehlfunktion Pull-Up und Boot-Verhalten pruefen.

### 5.12 Buzzer Test-Prozedur

**Vorbedingung:** DUT bootfaehig. Buzzer an GPIO38 (LEDC PWM, TP23).

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | `buzzer-test` senden | Piezo toont (ca. 2 kHz) |
| 2 | Pegel per Oszilloskop an TP23 messen | PWM-Signal sichtbar |
| 3 | `buzzer-test stop` | Ton endet |

Pass/Fail: akustisches Signal hoerbar, PWM an TP23 messbar.

### 5.13 Status-LED Test-Prozedur

**Vorbedingung:** DUT bootfaehig. STATUS_LED an GPIO21 (TP22).

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | Boot abwarten | Heartbeat-Blinken (gruen) |
| 2 | BLE verbinden | LED wechselt zu BLE-Status (dauerhaft/steady) |
| 3 | BLE trennen | zurueck zu Heartbeat |
| 4 | `led-test` | definiertes Blinkmuster |

Pass/Fail: LED zeigt Heartbeat und BLE-Status korrekt.
Hinweis: PINMAP weist GPIO21 der STATUS_LED zu; board_config.h verwendet GPIO21
aktuell als DISPLAY_RST — Diskrepanz siehe Pinmap-Verifikationsbericht
`docs/research/pinmap-firmware-verification.md`.

### 5.14 MOSFET / Relais Test-Prozedur

**Vorbedingung:** DUT bootfaehig. Gate-Driver TC4427, MOSFET IRLZ44N,
Optokoppler PC817 (Heizgriff/Hilfslicht-Steuerung).

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | `output-test heat on` | Heizgriff-Ausgang schaltet (MOSFET durch) |
| 2 | Drain-Spannung MOSFET messen | niedrig (Durchlass) |
| 3 | `output-test heat off` | Ausgang aus, Drain = Versorgungsspannung |
| 4 | `output-test aux on/off` | Hilfslicht-Ausgang schaltet |
| 5 | Optokoppler-LED (PC817) pruefen | leuchtet bei aktiviertem Kanal |

Pass/Fail: alle Ausgaenge schalten sauber, keine Kurzschlüsse, Rueckweg sauber.
Achtung: induktive Lasten mit Freilaufdiode (1N5408) — Diode pruefen.

### 5.15 Barometer (BMP390/BMP388) Test-Prozedur

**Vorbedingung:** DUT bootfaehig, I2C-Scan (§5.0.4) findet Adresse 0x76.

| Schritt | Aktion | Erwartetes Ergebnis |
|---|---|---|
| 1 | `baro-read` ueber serielle Schnittstelle | Druck + Temperatur plausibel |
| 2 | Hoehe ableiten | ~ Hoehe des Standorts ± 50 m |
| 3 | Lüftungsöffnung im Gehaeuse freimachen | Wert stabil |
| 4 | Mit Referenzbarometer vergleichen | Abweichung < 1 hPa |

Pass/Fail: Druck ~960–1013 hPa (je Hoehe), Temperatur plausibel, stabil.
Hinweis: BMP388 ist "Obsolete" — Ersatz BMP390 (BOM §8.2). I2C-Adresse 0x76/0x77
je Modul-Belegung verifizieren.

---

## 6. Test-Matrix

### 6.1 Automatisierbare vs. Manuelle Tests

| Test-ID | Subsystem | Test-Name | Automatisierbar | Frequenz |
|---|---|---|---|---|
| TB-RPM-001 | RPM | Leerlauf-Frequenz | Ja (FG) | Jeder Build |
| TB-RPM-002 | RPM | Volllast-Frequenz | Ja (FG) | Jeder Build |
| TB-RPM-003 | RPM | Rotationsbereich | Ja (FG) | Release |
| TB-RPM-004 | RPM | Latenz bei Sprung | Ja (FG + LA) | Release |
| TB-TACHO-001 | Tacho | 50 km/h | Ja (FG) | Jeder Build |
| TB-TACHO-002 | Tacho | 100 km/h | Ja (FG) | Jeder Build |
| TB-TACHO-003 | Tacho | Max-Speed | Ja (FG) | Release |
| TB-TEMP-001 | Temperatur | 25 C (NTC) | Ja (Digital-Poti) | Jeder Build |
| TB-TEMP-002 | Temperatur | 95 C (Betriebstemp.) | Ja (Digital-Poti) | Jeder Build |
| TB-TEMP-003 | Temperatur | 120 C (Uebertemp.) | Ja (Digital-Poti) | Release |
| TB-FUEL-001 | Tank | Leer | Ja (Poti) | Jeder Build |
| TB-FUEL-002 | Tank | Halbvoll | Ja (Poti) | Jeder Build |
| TB-FUEL-003 | Tank | Voll | Ja (Poti) | Release |
| TB-OIL-001 | Oeldruck | 0 bar (Warnung) | Ja (DAC) | Release |
| TB-OIL-002 | Oeldruck | 4,5 bar | Ja (DAC) | Release |
| TB-BATT-001 | Bordspannung | 12V (Motor aus) | Ja (NT) | Jeder Build |
| TB-BATT-002 | Bordspannung | 14,2V (Ladebetrieb) | Ja (NT) | Jeder Build |
| TB-BATT-003 | Bordspannung | 8V (Anlassen) | Ja (NT) | Release |
| TB-BATT-004 | Bordspannung | Power-Loss | Manuell | Release |
| TB-DISP-001 | TFT | FPS > 30 | Ja (Befehl) | Jeder Build |
| TB-DISP-002 | E-Paper | Full-Refresh < 2s | Ja (Befehl) | Jeder Build |
| TB-DISP-003 | TFT+E-Paper | SPI-Kollision | Manuell | Release |
| TB-BLE-001 | BLE | Connect < 5s | Ja (Python) | Jeder Build |
| TB-BLE-002 | BLE | Notify-Latenz < 100ms | Ja (Python) | Jeder Build |
| TB-BLE-003 | BLE | Durchsatz > 10 kbps | Ja (Python) | Release |
| TB-GPS-001 | GPS | Kaltstart-Fix < 30s | Ja (NMEA-Replay) | Release |
| TB-GPS-002 | GPS | Geschwindigkeit korrekt | Ja (NMEA-Replay) | Jeder Build |
| TB-GPS-003 | GPS | Tunnel (Signalverlust) | Ja (NMEA-Replay) | Release |
| TB-IMU-001 | IMU | Neigung 0-45 Grad | Manuell | Release |
| TB-IMU-002 | IMU | Crash-Erkennung | Manuell | Release |
| TB-SD-001 | SD | Schreibrate > 50 kbps | Ja (Befehl) | Jeder Build |
| TB-SD-002 | SD | Power-Loss Write | Manuell | Release |
| TB-SD-003 | SD | Karte voll | Manuell | Release |
| TB-POWER-001 | Power | ACTIVE < 150 mA | Ja (Befehl+MM) | Release |
| TB-POWER-002 | Power | DIMMED < 80 mA | Ja (Befehl+MM) | Release |
| TB-POWER-003 | Power | SUSPENDED < 20 mA | Ja (Befehl+MM) | Release |
| TB-POWER-004 | Power | DEEP_SLEEP < 5 mA | Ja (Befehl+MM) | Release |
| TB-BOOT-001 | Boot | Zeit < 3s | Ja (Befehl) | Jeder Build |
| TB-HEAP-001 | Heap | Frei > 50 KB nach Boot | Ja (Befehl) | Jeder Build |
| TB-HEAP-002 | Heap | Frei > 40 KB nach 24h | Ja (Befehl) | Release |

### 6.2 Regression-Test-Plan

Bei jedem Build (Continuous Integration) werden folgende Tests
automatisch ausgefuehrt:

1. Self-Test (`selftest` Befehl): Alle Module erkannt, Dauer < 3s.
2. Boot-Zeit (`benchmark boot`): < 3s bis Self-Test-Start.
3. TFT-FPS (`benchmark tft 100`): > 30 FPS.
4. E-Paper-Update (`benchmark epd 5`): < 2s pro Full-Refresh.
5. SD-Schreibrate (`benchmark sd`): > 50 kbps.
6. Heap-Check (`benchmark heap`): > 50 KB frei nach Boot.
7. CPU-Last (`benchmark cpu 10`): Kein Task > 80% pro Core.
8. BLE-Connect (`ble advertise on` + Python-Connect): < 5s.
9. RPM-Read (`sensor read rpm` mit 21,7 Hz): 1300 ± 26.
10. Temp-Read (`sensor read coolant_temp` mit 10k Ohm): 25 ± 2 C.

Bei jedem Release (vor Auslieferung) zusaetzlich:
- Alle manuellen Tests (IMU-Neigung, Crash-Simulation, Power-Loss).
- Alle Release-Frequenz-Tests aus der Test-Matrix.
- Langzeit-Stabilitaetstest (1h Lauf, Heap-Monitoring).
- SD-Karten-Voll-Test und Korruptionstest.

---

## 7. Test-Environment Schema

### 7.1 Verkabelungs-Uebersicht

```
┌────────────┐   USB-C    ┌──────────────────┐
│ Steuer-PC  │◄──────────►│  Bordcomputer     │
│            │            │  (DUT)            │
│  pyserial  │            │                   │
│  pytest    │            │  GPIO15 ◄── RPM   │
│  esptool   │            │  GPIO16 ◄── Tacho  │
└────────────┘            │  GPIO2  ◄── Temp  │
                          │  GPIO4  ◄── Fuel  │
┌────────────┐            │  GPIO5  ◄── Oil   │
│Funktions-  │── RPM ────►│  GPIO1  ◄── Batt  │
│generator   │── Tacho ──►│  GPIO8/9◄── I2C   │
│            │            │  GPIO44◄── GPS RX │
└────────────┘            │  GPIO48◄── IMU INT│
                          └─────────┬─────────┘
┌────────────┐                      │
│Widerstands-│── Temp ───►          │
│dekade /    │── Fuel ───►          │
│Digital-Poti│── Oil ────►          │
└────────────┘                      │
                                    │
┌────────────┐                      │
│Labornetz-  │── 12V ────►──────────┘
│teil +      │
│Multimeter  │
└────────────┘

┌────────────┐   UART     ┌──────────┐
│NMEA-Replay │── GPS ────►│  DUT     │
│(Python)    │── PPS ────►│          │
└────────────┘            └──────────┘

┌────────────┐   I2C      ┌──────────┐
│IMU-Test-Rig│── SDA/SCL─►│  DUT     │
│(MPU6050 +  │── INT ────►│          │
│ Goniometer)│            └──────────┘
└────────────┘

┌────────────┐   SPI/ADC  ┌──────────┐
│Oszilloskop │── Probes──►│  DUT     │
│4-Kanal     │            │  TPs     │
└────────────┘            └──────────┘

┌────────────┐   Digital  ┌──────────┐
│Logik-       │── Clips──►│  DUT     │
│analysator   │            │  TPs     │
│8-Kanal     │            └──────────┘
└────────────┘
```

### 7.2 GND-Verbindung

Alle Geraete muessen eine gemeinsame Masse (GND) haben. Der
Steuerrechner bekommt GND ueber USB-C (USB-GND). Funktionsgenerator,
Labornetzteil und alle Simulator-Module werden mit separaten
GND-Kabeln an den DUT-GND angeschlossen (Sternpunkt am DUT).

### 7.3 Stoerungsarme Messung

- Alle Signalleitungen so kurz wie moeglich (< 30 cm).
- Funktiongenerator-Ausgang mit 50 Ohm abschließen, wenn der
  Eingang hochohmig ist (mindestens 10 kOhm am PCNT).
- Oszilloskop-Sonden mit 10:1-Tastkopf (reduziert Kapazitive
  Belastung auf ~10 pF).
- GND-Krokodilklemmen so kurz wie moeglich (Schleifen vermeiden,
  um Einkopplung zu reduzieren).
- Bei PWM-Messungen (Backlight, Buzzer) keine direkten Kabel-
  Verlaengerungen, sondern Tastkopf direkt am Test-Point.

---

## 8. Benchmark-Tool Referenz

Die in der Test-Bench verwendeten Befehle korrespondieren mit den
Benchmark-Spezifikationen in docs/BENCHMARK-SPEC.md. Die dort
definierten Zielwerte sind die Pass/Fail-Kriterien fuer die
hier beschriebenen Test-Prozeduren.

| Benchmark | Befehl | Zielwert (Dokument) |
|---|---|---|
| TFT Rendering | `benchmark tft N` | BENCHMARK-SPEC.md 1.1 |
| E-Paper Update | `benchmark epd N` | BENCHMARK-SPEC.md 1.2 |
| Sensor-Latenz | `benchmark latency` | BENCHMARK-SPEC.md 2 |
| BLE-Durchsatz | `benchmark ble N` | BENCHMARK-SPEC.md 3 |
| GPS-Update-Rate | `benchmark gps` | BENCHMARK-SPEC.md 4 |
| Power-Consumption | `benchmark power S` | BENCHMARK-SPEC.md 5 |
| Boot-Zeit | `benchmark boot` | BENCHMARK-SPEC.md 6 |
| SD-Schreibrate | `benchmark sd N` | BENCHMARK-SPEC.md 7 |
| CPU-Last | `benchmark cpu N` | BENCHMARK-SPEC.md 8 |
| Speicherverbrauch | `benchmark heap` | BENCHMARK-SPEC.md 9 |

---

## Anhang: Quellen und Referenzen

- docs/TEST-STRATEGY.md — Test-Pyramide, HIL-Tests, Benchmark-Zielwerte
- docs/HARDWARE.md — Schaltplan, Pin-Belegung, Power-Management
- docs/PINMAP.md — Vollstaendige Pin-Tabelle
- docs/BOM.md — Komponenten-Liste, Bezugsquellen
- docs/research/04-pd06-technische-daten.md — PD06 technische Daten
- firmware/main/board_config.h — Pin-Definitionen, Konstanten
- docs/BENCHMARK-SPEC.md — Benchmark-Zielwerte und Messmethoden

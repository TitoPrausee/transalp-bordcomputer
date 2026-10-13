# PCB-Layout-Konzept: Layer-Stackup, Routing-Regeln, EMV-Optimierung

> Stand: Oktober 2026
> Projekt: Transalp Bordcomputer (Honda XL600V Transalp PD06)
> Plattform: ESP32-S3-WROOM-1 (N8R2), 8 MB Flash, 2 MB PSRAM
> Vorgaenger-Dokumente: docs/research/02-hardware-deep-dive.md,
>   docs/research/05-gehaeuse-einbau-emv.md, docs/research/11-firmware-architecture.md
> Dieses Dokument ist ein Design-Konzept, kein KiCad-File. Es ist so praechise
> geschrieben, dass ein PCB-Designer damit direkt arbeiten kann.

---

## 1. PCB-Spezifikation

### 1.1 Board-Abmessungen

Das PCB muss in das Gehaeuse passen, das in 05-gehaeuse-einbau-emv.md
beschrieben ist. Das Gehaeuse wird aus ASA im FDM-Verfahren gefertigt
und am Lenker (22 mm Durchmesser) oder an einer Crossbar montiert.
Das Gesamtgeraet soll nicht mehr als 400-500 g wiegen.

Empfohlene Platinengroesse: 80 mm x 50 mm (4.000 mm^2).

Diese Groesse wurde wie folgt begruendet:

- Der ESP32-S3-WROOM-1 hat Modulabmessungen von 18 x 25,5 mm.
  Die Antenne erfordert einen Freiraum von mindestens 15 mm am
  Modulende (keine Kupferflaechen, keine Bauteile in diesem Bereich).
- Das 3.5" IPS-Display (Waveshare) hat eine Modulgroesse von ca.
  85 x 50 mm. Das Display wird nicht auf dem Haupt-PCB montiert,
  sondern ueber ein FPC-Kabel angeschlossen. Das Haupt-PCB braucht
  daher nur den Display-Connector (FPC 0.5 mm Pitch, 12-polig).
- Der E-Paper-Zweitdisplay (2.13") wird ebenfalls per FPC angebunden.
- Das GPS-Modul (NEO-M9N, 16 x 12,2 mm) wird auf einem separaten
  Daughterboard montiert (bessere Antennenpositionierung) und ueber
  ein 4-poliges Flachbandkabel (UART) angeschlossen.

Das 80x50 mm PCB bietet ausreichend Platz fuer:

| Komponente              | Flaeche (mm^2) | Anzahl |
|-------------------------|----------------|--------|
| ESP32-S3-WROOM-1        | 18 x 25,5     | 1      |
| Buck-Converter (MP1584) | 22 x 15       | 1      |
| LDO HT7333 (SOT-89)     | 4,5 x 4,5     | 1      |
| TP4056 Lademodul        | 17 x 12       | 1      |
| MT3608 Boost-Modul      | 17 x 11       | 1      |
| I2C-Sensoren (BMI270)   | 3 x 3         | 1      |
| I2C-Sensoren (BMP388)   | 2 x 2         | 1      |
| MicroSD-Slot            | 15 x 15       | 1      |
| USB-C IP67-Buchse       | 12 x 9        | 1      |
| Optokoppler (PC817)     | 5 x 5         | 2      |
| Ferrite-Beads (0805)    | 2 x 1,25      | 4      |
| Entkopplungs-Cs (0805)  | 2 x 1,25      | ~15    |
| Steckverbinder (Sensor) | 10 x 8        | 2      |
| Freiflaeche (Routing)   | Rest          | —      |

Alternative Abmessung fuer kompakteren Prototyp (ohne eingebaute
Module, stattdessen auf Breakout-Boards verdrahtet): 60 x 40 mm.
Dies ist ausreichend fuer den WROOM-1, die Sensoren und Stecker,
allerdings muessen Buck/Boost/Lade-Module als externe Breakouts
angeschlossen werden.

### 1.2 Layer-Count: 2-Layer vs. 4-Layer

| Kriterium              | 2-Layer                      | 4-Layer                           |
|------------------------|------------------------------|-----------------------------------|
| Kosten (5 Stueck, JLC) | ca. 2-5 EUR                 | ca. 8-15 EUR                      |
| Lieferzeit             | 24-48h                      | 3-5 Tage                          |
| GND-Plane              | Nur Teilflaechen moeglich    | Durchgehende GND-Plane auf L2     |
| Power-Plane            | Als Polygone (gesplittet)    | Eigene Power-Plane auf L3         |
| Signal-Integritaet     | Laengerer Return-Path        | Kurzer Return-Path ueber GND-Plane |
| EMV                    | Schlechter (keine durchgehende GND-Plane) | Gut (durchgehende GND-Plane)   |
| Routing-Aufwand        | Hoch ( viele Vias)           | Niedriger (Signal auf L1/L4, Power/GND auf L2/L3) |
| Vibration              | Gleich                       | Gleich (gleiche Dicke)            |
| Plattendicke           | 1,6 mm                       | 1,6 mm                            |

Empfehlung: 4-Layer.

Ein 4-Layer-Board kostet ca. 5-10 EUR mehr als ein 2-Layer-Board bei
einer Kleinserie von 5 Stueck. Dafuer bietet es eine durchgehende
GND-Plane, die fuer die EMV-Stabilitaet in der Motorradumgebung
(Zuendung, Lichtmaschine) entscheidend ist. Der ADC braucht eine
rauscharme Masse, und die SPI-Signale zum Display und zur SD-Karte
profitieren von einer unmittelbar darunterliegenden GND-Plane.

Zusaetzlich reduziert eine 4-Layer-Platine die Routing-Komplexitaet
massiv: alle Signalleitungen koennen auf L1 und L4 gefuehrt werden,
waehrend L2 und L3 fuer Power und GND reserviert sind. Das minimiert
die Anzahl der Vias und verbessert die Signal-Integritaet.

Fuer einen reinen Prototyp (Phase 1) ist ein 2-Layer-Board akzeptabel.
Ab Phase 2 (Production, Dauereinsatz am Motorrad) ist 4-Layer Pflicht.

### 1.3 Material und Dicke

- Material: FR4 (Standard-Epoxidharz, Tg >= 130 Grad C)
- Plattendicke: 1,6 mm (Standard, fuer JLCPCB/PCBWay ohne Aufpreis)
- Kupferdicke: 1 oz (35 um) fuer Signal-Layer und GND/Power-Plane

1 oz Kupfer reicht fuer alle Strombahnen dieses Projekts, da der
maximale Dauerstrom auf dem 12V-Pfad bei 3A (Heizgriffe ueber MOSFET,
nicht ueer das PCB gefuehrt) bzw. 2A (5V-Rail vom Buck-Converter)
liegt. Die Strombahnen fuer diese Leistungen werden als Polygone
grossflaechig ausgefuehrt, nicht als einzelne Traces.

Alternative: 2 oz (70 um) fuer L1 und L4 (Signal-Layer) bei geringem
Mehraufwand (~3-5 EUR Aufpreis). 2 oz erlaubt schmalere Power-Traces
und verbessert die Waermeableitung vom ESP32-S3. Empfehlung:
2 oz fuer L1 und L4 ab Phase 2 (Production).

Die innere GND- und Power-Plane (L2, L3) bleiben bei 1 oz, da
durchgehende Kupferflaechen ohnehin einen sehr niedrigen
Flaechenwiderstand haben.

### 1.4 Oberflaechenfinish

- HASL (Hot Air Solder Leveling): billig, aber uneben, nicht fuer
  feine Pitch-Komponenten geeignet.
- ENIG (Electroless Nickel Immersion Gold): eben, lagerfaehig, fuer
  feine Pitch-Komponenten (USB-C, WROOM-1 Pad-Raster). Empfohlen.

Aufpreis fuer ENIG bei JLCPCB: ca. 3-5 EUR fuer 5 Stueck. Da das
Board einen USB-C-Connector (0,5 mm Pitch) und den WROOM-1
(1,27 mm Pitch Pad-Raster) tragt, ist ENIG die sichere Wahl.

---

## 2. Layer-Stackup (4-Layer)

### 2.1 Stackup-Definition

```
Schicht        Dicke      Material             Funktion
═════════════════════════════════════════════════════════════════
L1 (Top)       35 um      Kupfer, 1 oz/2 oz    Signale + Bauteile
Prepreg        ~200 um    FR4 (1080+2116)      Dielektrikum
L2 (Inner 1)   35 um      Kupfer, 1 oz          GND-Plane (durchgehend)
Core           ~1.000 um  FR4                   Kern
L3 (Inner 2)   35 um      Kupfer, 1 oz          Power-Plane (5V + 3.3V)
Prepreg        ~200 um    FR4 (1080+2116)      Dielektrikum
L4 (Bottom)    35 um      Kupfer, 1 oz/2 oz    Signale + Bauteile
═════════════════════════════════════════════════════════════════
Gesamt:        1,6 mm
```

### 2.2 Begruendung fuer jeden Layer

**L1 (Top): Signale und Bauteile**

Auf der Top-Layer werden alle Bauteile plaziert (SMD-Bauteile
auf der Oberseite). Die Signalfuehrung erfolgt hier mit kurzen
Traces zwischen Bauteilen. Vias verbinden die Top-Layer-Signale
mit der Bottom-Layer (L4), wenn ein Signal die Ebene wechseln muss.

Besonderheiten auf L1:

- ESP32-S3-WROOM-1 in der Platinenmitte, mit 15 mm Antennen-Freiraum
  am einen Ende (kein Kupfer, keine Bauteile, kein GND-Polygon)
- Kritische Signale (SPI-Clock, Crystal) so kurz wie moeglich, um
  Induktivitaet und Kapazitaet zu minimieren
- Decoupling-Caps direkt an den VCC-Pins der ICs, max. 2 mm
  Abstand zum Pin

**L2 (Inner 1): GND-Plane (durchgehend)**

Die innere Masse-Ebene ist durchgehend, ohne Slots oder
Unterbrechungen unter kritischen ICs. Sie dient als:

- Niedrigimpedanter Return-Pfad fuer alle Signale (Signal-Integritaet)
- EMV-Schirmung (Stoerstrahlung von der Zuendung/Lichtmaschine wird
  ueber die GND-Plane abgeleitet)
- Waerme-Verteilung (Kupfer leitet Waerme vom ESP32-S3 weg)
- Referenzpotential fuer ADC-Messungen (rauscharme Masse)

Einzige Unterbrechungen: Vias (Antipads). Keine Slots, keine
Power-Traces, die die GND-Plane durchschneiden wuerden.

**L3 (Inner 2): Power-Plane (5V + 3.3V geteilt)**

Die innere Power-Ebene ist in zwei Bereiche geteilt:

- Ca. 60% der Flaeche: 3.3V (fuer ESP32-S3, Sensoren, SD-Karte)
- Ca. 40% der Flaeche: 5V (fuer USB-Power, Backup-Boost, Display-Backlight)

Die Trennlinie verlaeuft so, dass die 3.3V-Seite unter dem
ESP32-S3 und den Sensoren liegt, die 5V-Seite unter dem
Buck-Converter und den Power-Komponenten.

Alternativ: Die Power-Plane als durchgehende 3.3V-Ebene und die
5V-Versorgung als breite Polygone auf L1 und L4. Dies ist
einfacher zu routen und vermeidet Split-Plane-Probleme, hat aber
den Nachteil, dass die 5V-Versorgung nicht als Ebene verfuegbar ist.

Empfehlung: Durchgehende 3.3V-Ebene auf L3. Die 5V-Versorgung als
breite Polygone (min. 2 mm) auf L1 und L4. Dies gibt eine saubere
GND-Plane (L2) und eine saubere 3.3V-Plane (L3), die 80% der
Platine abdeckt. Der 5V-Pfad ist kurz (vom Buck-Converter zum
USB-Connector und zum Display-Connector) und kann als Polygon
gefuehrt werden.

**L4 (Bottom): Signale und Bauteile**

Die Bottom-Layer traegt SMD-Bauteile auf der Unterseite (passive
Komponenten, kleinere ICs). Sie wird auch fuer Signal-Routing
verwendet, wenn ein Signal auf L1 nicht mehr weitergefuehrt werden
kann.

Besonderheiten auf L4:

- GND-Polygon als Teilflaeche (ergaenzt die GND-Plane auf L2)
- Power-Polygone fuer 5V und 12V (kurze Pfade zu den Steckverbindern)
- Keine kritischen Signale (EMV: die Unterseite ist dem Gehaeuse
  zugewandt, Kupferfolie kann hier als zusaetzliche Abschirmung
  aufgeklebt werden)

### 2.3 Impedanz-Kontrolle

Fuer dieses Projekt ist keine kontrollierte Impedanz erforderlich.
Die einzigen hochfrequenten Signale sind:

- USB-C D+/D- (Full-Speed, 12 Mbps): keine Impedanz-Anforderung
- SPI-Signale (max. 80 MHz fuer Display, 40 MHz fuer SD-Karte):
  kurze Leitungen (< 50 mm), keine differenziellen Paare
- GPS-UART (115200 Baud): niederfrequent, unkritisch

Die GND-Plane auf L2 sorgt fuer einen guten Return-Path, ohne dass
eine kontrollierte Impedanz spezifiziert werden muss.

---

## 3. Komponenten-Placement

### 3.1 Placement-Prinzipien

1. ESP32-S3-WROOM-1 zentral auf der Platine, Antenne am
   Plaetchenrand (15 mm Freiraum)
2. Power-Komponenten (Buck-Converter, LDO, TP4056, MT3608) am
   einen Ende der Platine (nahe USB-C-Eingang und 12V-Eingang)
3. SPI-Bus-Komponenten gruppiert: Display-Connector und SD-Slot
   auf der gleichen Seite, kurze Leitungen
4. Sensor-Eingaenge (ADC, I2C, Pulse-Counter) nahe den
   Stecker-Pins am Platinenrand
5. Abstandskriterium: min. 0,5 mm zwischen SMD-Bauteilen fuer
   Lotpaste und Bestueckung, 1,0 mm fuer handloetbare Abzuege
6. Waerme: ESP32-S3 und Buck-Converter haben Waermeabgabe.
   Diese Bauteile nicht direkt nebeneinander plazieren.
   GND-Polygon unter dem ESP32-S3 als Waerme-Spreader.

### 3.2 ASCII-Placement-Uebersicht

```
    12V-EINGANG                                     SENSOREN
    │  USB-C(IP67)                                   │  │
    │  │  FUSE  TVS                                  │  │
    │  │  │    │                                     │  │
    v  v  v    v                                     v  v
┌──────────────────────────────────────────────────────────────┐
│  ┌─────────┐  ┌──────────┐                                    │
│  │ MP1584  │  │ TP4056   │                          ┌───────┐│
│  │ Buck    │  │ Charger  │                          │ J2    ││
│  │ 12V→5V  │  │ 5V→18650 │         ┌──────────────┐  │ Sensor││
│  └─────────┘  └──────────┘         │              │  │ Conn  ││
│  ┌─────┐  ┌──────────┐             │   ESP32-S3   │  └───────┘│
│  │HT733│  │ MT3608   │             │   WROOM-1    │            │
│  │ LDO │  │ Boost    │             │   N8R2       │  ┌───────┐│
│  │5V→3V│  │ 3.7V→5V  │             │              │  │ J3    ││
│  └─────┘  └──────────┘             │   (Antenne)  │  │ Relais││
│  ┌──────────────┐                  └──────────────┘  │ Conn  ││
│  │ 18650 Holder │                  ┌──────────┐       └───────┘│
│  │  (auf L4)     │                  │  SD-Slot │                │
│  └──────────────┘                  │  (SPI3)  │                │
│                                     └──────────┘                │
│  ┌────────┐  ┌─────┐  ┌──────────┐  ┌──────────┐               │
│  │ FERR   │  │PC817│  │ BMI270   │  │ BMP388   │   ┌───────┐   │
│  │ BEADS  │  │ x2  │  │ IMU      │  │ Baro     │   │ FPC   │   │
│  └────────┘  └─────┘  └──────────┘  └──────────┘   │ Disp  │   │
│                                                   └───────┘   │
│  ┌──────┐   ┌──────┐                                          │
│  │TC4427│   │IRLZ44│       (Conformal Coating Bereich)         │
│  │Gate  │   │MOSFET│                                         │
│  │Driver│   │      │                                         │
│  └──────┘   └──────┘                                         │
│                                                              │
│  ←─── 5V / 3.3V Power Rail (Polygon unter Bauteilen) ───→   │
└──────────────────────────────────────────────────────────────┘
    ↑                                    ↑
    POWER-DOMAINE                        SIGNAL-DOMAINE
    (Buck, LDO, Charger, Boost)          (MCU, Sensoren, SPI, I2C)
```

### 3.3 Placement-Details

**ESP32-S3-WROOM-1:**

- Zentral auf der Platine, ca. bei X=35 mm, Y=20 mm (Mitte des 80x50 mm Boards)
- Modulausrichtung: Antenne zeigt zum Plaetchenrand (keine Bauteile,
  keine Kupferflaechen im Antennenbereich, 15 mm Freiraum)
- Die 45 GPIO-Pins sind auf dem Modulrand herausgefuehrt. Die
  Pin-Reihen sind mit 1,27 mm Pitch herausgefuehrt.
- Decoupling-Caps direkt neben den VCC-Pins auf der Top-Layer

**Buck-Converter (MP1584EN):**

- Am linken Platinenrand, nahe dem 12V-Eingang und USB-C
- Input-Kondensatoren (100 uF Elko + 100 nF Kerko) direkt am Input-Pin
- Output-Kondensatoren (22 uF + 100 nF) direkt am Output-Pin
- Induktivitaet (22 uH, 3A) so nah wie moeglich am IC
- Feedback-Teiler (R1/R2) kurze Traces zum FB-Pin

**LDO (HT7333, SOT-89):**

- Direkt neben dem Buck-Converter-Output
- Input-Cap (10 uF) und Output-Cap (10 uF) direkt an den Pins
- Ausgang mit kurzer Trace zur 3.3V-Power-Plane

**TP4056 + MT3608 (Backup-Power):**

- Neben dem Buck-Converter
- TP4056-Input vom 5V-Rail (vom Buck)
- MT3608-Input vom Akku (B+)
- Schottky-Dioden (1N5822) fuer Power-Path-ORing nahe der Ausgaenge
- 18650-Akkuhalter auf der Bottom-Layer (L4), ueber die Power-Sektion

**I2C-Sensoren (BMI270, BMP388):**

- Neben dem ESP32-S3, kurze I2C-Leitungen (< 20 mm)
- Beide Sensoren auf der gleichen Seite des MCU, um den I2C-Bus
  kurz zu halten
- BMI270 auf Schaumstoff-Pad oder Silikontoepfchen zur
  Vibrationsentkopplung (siehe 05-gehaeuse-einbau-emv.md Abschnitt 3.2)
- BMP388 unmittelbar daneben, beide auf der gleichen I2C-Adresse
  (BMI270: 0x68, BMP388: 0x76)
- I2C-Pull-Ups (4,7 kOhm) direkt am SDA/SCL-Pin des ESP32-S3

**SD-Karten-Slot:**

- Auf der gleichen Seite wie der ESP32-S3, SPI3-Bus
- CS-Pin (GPIO38) direkt neben dem Slot
- SPI-Leitungen (MOSI, SCK, MISO) als Bus gefuehrt (parallele Traces)
- SD-Karte ist ein haeufiges EMV-Problem: Clock-Signal (SCK) kann
  stoeren. Serienwiderstand (22 Ohm) im SCK-Pfad empfohlen.

**Display-Connector (FPC):**

- Am rechten Platinenrand, nahe der MCU
- 12-poliger FPC-Connector (0,5 mm Pitch) fuer das 3.5" IPS-Display
- SPI2-Leitungen (MOSI=GPIO11, SCK=GPIO12, CS=GPIO10, DC=GPIO9)
  als kurze, parallele Traces zum Connector
- Display-Backlight-Power (5V) als separates Polygon

**USB-C-Buchse (IP67):**

- Am linken Platinenrand, nahe dem Buck-Converter
- USB-C D+/D- direkt zum ESP32-S3 USB-OTG-Pins (GPIO19/GPIO20)
- USB-VBUS (5V) ueber Ferrite-Bead zum 5V-Rail

**Steckverbinder (Sensoren, Relais):**

- Am rechten und oberen Platinenrand
- AMP Superseal oder Deusch DTM (IP67, rastverbinder)
- ADC-Eingaenge (GPIO1-3) mit RC-Filter (1 kOhm + 100 nF) direkt
  am Stecker-Pin
- Pulse-Counter-Eingaenge (GPIO4-5) mit Optokoppler (PC817)
  und Pull-Up direkt am Stecker

---

## 4. Routing-Regeln

### 4.1 Trace-Width fuer Power

Alle Berechnungen nach IPC-2221 (Generic Standard on Printed Board Design).
Formel: I = k * dT^0.44 * A^0.725, wobei k=0.048 (aeussere Layer) bzw.
0.024 (innere Layer), I in Ampere, dT in Grad Celsius, A in mil^2.

Bei 1 oz Kupfer (35 um) und einer Temperaturerhoehung von 10 K:

| Schiene       | Strom | Layer    | Kupfer | Breite (berechnet) | Empfohlene Breite |
|---------------|-------|----------|--------|--------------------|--------------------|
| 12V Eingang   | 3.0 A | Top (ext) | 1 oz  | 1.37 mm            | 2.0 mm (Polygon)   |
| 12V Eingang   | 3.0 A | Top (ext) | 2 oz  | 0.68 mm            | 1.5 mm (Polygon)   |
| 5V Rail       | 2.0 A | Top (ext) | 1 oz  | 0.78 mm            | 1.5 mm (Polygon)   |
| 5V Rail       | 2.0 A | Top (ext) | 2 oz  | 0.39 mm            | 1.0 mm (Polygon)   |
| 3.3V Rail     | 0.5 A | Top (ext) | 1 oz  | 0.12 mm            | 0.5 mm             |
| 3.3V Rail     | 1.0 A | Top (ext) | 1 oz  | 0.30 mm            | 0.5 mm             |
| 3.3V Plane    | 0.5 A | Inner    | 1 oz  | 0.30 mm            | Plane (durchgeh.)  |
| Heizgriff PWM | 3.0 A | Top (ext) | 2 oz  | 0.45 mm (dT=20K)  | 1.5 mm             |

Erlaeuterung: Die berechneten Werte sind Mindestbreiten fuer eine
Temperaturerhoehung von 10 K. In der Praxis werden Power-Traces
grosszuegiger dimensioniert, um Temperatur-Spikes abzufangen und
Spannungsabfaelle zu minimieren. Die empfohlenen Breiten haben einen
Sicherheitsfaktor von ca. 1.5x bis 2x.

Die 12V-Versorgung fuer die Heizgriffe und das Zusatzscheinwerfer
wird nicht ueer das PCB gefuehrt. Diese Lasten werden ueber
Relais/MOSFET direkt von der 12V-Bordnetz-Leitung geschaltet.
Das PCB fuehrt nur die Steuersignale (GPIO ueber TC4427 Gate-Driver
zum IRLZ44N MOSFET).

Die 5V-Versorgung vom Buck-Converter zum 5V-Rail wird als
breites Polygon (min. 2 mm bei 1 oz) auf L1 gefuehrt.

Die 3.3V-Versorgung vom HT7333 LDO zur 3.3V-Plane (L3) wird ueber
einen kurzen, breiten Trace (min. 0.5 mm) und mehrere Vias
(min. 3 Vias, 0.3 mm Drill) zur inneren Ebene gefuehrt.

### 4.2 Trace-Width fuer Signale

| Signal-Typ          | Frequenz    | Breite    | Begruendung                          |
|---------------------|-------------|-----------|--------------------------------------|
| Standard-GPIO       | DC - 1 MHz  | 0.15 mm   | Ausreichend fuer kurze Leitungen     |
| SPI-Signale         | bis 80 MHz  | 0.15 mm   | Kurze Leitungen (< 50 mm), GND-Plane |
| I2C (SDA/SCL)       | 100-400 kHz | 0.15 mm   | Niederfrequent, unkritisch           |
| UART (GPS)          | 115.2 kbaud | 0.15 mm   | Niederfrequente Leitung              |
| USB-C D+/D-         | 12 Mbps     | 0.15 mm   | Full-Speed, keine Diff-Pair-Notwendigk. |
| ADC-Eingaenge       | DC - 1 kHz  | 0.15 mm   | Niederfrequent, Guard-Ring wichtiger|

Alle Signal-Traces werden mit 0.15 mm (6 mil) Breite gefuehrt. Das
ist der Standard fuer JLCPCB/PCBWay ohne Aufpreis (min. 5 mil).

### 4.3 Via-Size und Pitch

| Via-Typ       | Drill  | Pad    | Verwendung                          |
|---------------|--------|--------|-------------------------------------|
| Signal-Via    | 0.2 mm | 0.4 mm | Alle Signale (GPIO, SPI, I2C)       |
| Power-Via     | 0.3 mm | 0.6 mm | 3.3V und 5V Uebergang zu innerer Eb.|
| Thermal-Via   | 0.3 mm | 0.6 mm | Waermeableitung ESP32-S3 GND-Pad     |
| Stitching-Via | 0.2 mm | 0.4 mm | GND-Stitching alle 5-10 mm          |

Via-Pitch: min. 0.5 mm zwischen Via-Pads (Rand zu Rand).

Stromkapazitaet einzelner Vias (1 oz Plating, ca. 25 um Seitenwand):

- 0.2 mm Drill: ca. 0.3 A
- 0.3 mm Drill: ca. 0.5 A
- 0.4 mm Drill: ca. 0.8 A
- 0.6 mm Drill: ca. 1.5 A

Fuer die 3.3V- und 5V-Power-Uebergaenge werden mindestens 3 Vias
(0.3 mm Drill) parallel verwendet, um den Strom zu verteilen und
den Spannungsabfall zu minimieren.

### 4.4 Differential Pair Routing (USB-C D+/D-)

Obwohl USB-C D+/D- bei Full-Speed (12 Mbps) keine kontrollierte
Impedanz erfordert, werden die beiden Leitungen als Paar gefuehrt:

- Beide Traces gleich lang (Toleranz +/- 0.5 mm)
- Beide Traces gleich breit (0.15 mm)
- Abstand zueinander: 0.15 mm (eng gekoppelt)
- Abstand zu anderen Signalen: min. 0.3 mm
- Keine Vias im D+/D- Pfad, wenn moeglich
- D+/D- auf der gleichen Layer (L1 oder L4)
- GND-Plane direkt darunter (L2) als Referenz

Fuer USB-C VBUS (5V Power): Ferrite-Bead (600 Ohm @ 100 MHz) in
Reihe, 4.7 uF Bulk-Cap nach dem Bead.

### 4.5 SPI-Signal-Routing

Der Bordcomputer hat zwei SPI-Busse:

**SPI2 (FSPI) - TFT-Display:**
- MOSI=GPIO11, SCK=GPIO12, CS=GPIO10, DC=GPIO9
- Leitungen als paralleler Bus gefuehrt (gleiche Laenge +/- 5 mm)
- SCK mit Serienwiderstand 22 Ohm zur Reduzierung von Overshoot
- CS mit Pull-Up 10 kOhm am ESP32-Pin
- DC direkt an GPIO, kein Pull-up noetig
- Display-Connector nahe dem ESP32-S3, Leitungslaenge < 30 mm

**SPI3 (HSPI) - SD-Karte + E-Paper:**
- MOSI=GPIO35, SCK=GPIO36, MISO=GPIO37, CS_SD=GPIO38, CS_EPD=GPIO39
- Beide Slaves am gleichen Bus (ueber unterschiedliche CS-Pins)
- SCK mit Serienwiderstand 22 Ohm
- MISO mit Serienwiderstand 33 Ohm (Damping)
- CS_SD und CS_EPD mit Pull-Up 10 kOhm
- SD-Karte und E-Paper-Connector auf der gleichen Seite des PCB

Gleiche Laenge fuer MOSI/SCK/CS ist bei SPI nicht zwingend erforderlich,
da SPI einzelne Leitungen sind (kein Clock-Forwarding wie bei DDR-SDRAM).
Eine gleiche Laenge ist dennoch empfehlenswert, um Skew zwischen den
Datenleitungen zu minimieren. Bei 80 MHz SPI-Clock und 30 mm
Leitungslaenge ist der Skew vernachlaessigbar (< 0.2 ns).

### 4.6 I2C-Pull-Up-Placement

- Pull-Up-Widerstaende (4.7 kOhm) fuer SDA und SCL
- Direkt am ESP32-S3-Pin (nicht am Sensor-Ende)
- Wenn der I2C-Bus aus dem PCB herausgefuehrt wird (fuer externe
  Sensoren), einen zweiten Satz Pull-Ups am Steckverbinder platzieren
  (10 kOhm, schwaecher als die internen)
- Bei laengeren I2C-Leitungen (> 30 cm): Pull-Up auf 2.2 kOhm reduzieren,
  um Flankensteilheit zu erhoehen und Kapazitaet zu treiben
- Ferrite-Bead (600 Ohm @ 100 MHz) in Reihe mit SDA/SCL, wenn der
  I2C-Bus aus dem Gehaeuse herausgefuehrt wird

### 4.7 Crystal-Oscillator-Routing

Der ESP32-S3-WROOM-1 hat einen internen Crystal-Oscillator. Es ist
kein externer Crystal erforderlich. Das Modul hat bereits einen
40 MHz Crystal integriert.

Falls ein externer RTC-Crystal (32.768 kHz) fuer genauere
Zeitmessung verwendet wird (optional):

- Crystal so nah wie moeglich am ESP32-S3 (XIN/XOUT-Pins)
- Leitungen so kurz wie moeglich (< 5 mm)
- Beide Crystal-Leitungen gleich lang
- Load-Capacitors (12-22 pF) direkt am Crystal
- Keine anderen Signale in der Naehe des Crystals
- GND-Polygon unter dem Crystal und den Load-Caps
- Keine Vias im Crystal-Pfad

---

## 5. EMV-Optimierung

### 5.1 Decoupling-Caps

Jeder IC-Versorgungspin bekommt einen 100 nF Keramikkondensator
(X7R-Dielektrikum, 0805-SMD), der moeglichst nah am Pin plaziert
wird (max. 2 mm Abstand zum Pin). Der Cap wird direkt zwischen
VCC und dem naechsten GND-Via plaziert, um den Return-Path kurz
zu halten.

Zusaetzlich bekommt jeder IC einen 10 uF Bulk-Kondensator (Tantal
oder Keramik X5R/X7R, 0805 oder 1206) als lokalen Energiespeicher
fuer Stromspitzen.

Spezifische Decoupling-Anforderungen:

| IC                  | VCC-Pins | 100 nF Caps | 10 uF Bulk |
|---------------------|----------|-------------|------------|
| ESP32-S3-WROOM-1    | 3V3, EN  | 3 x 100 nF  | 1 x 10 uF  |
| BMI270              | VDDIO    | 1 x 100 nF  | —          |
| BMP388              | VDDIO    | 1 x 100 nF  | —          |
| SD-Karten-Slot      | VDD      | 1 x 100 nF  | 1 x 10 uF  |
| MP1584EN (Buck)     | VIN, VOUT| 1 x 100 nF je | 1 x 100 uF (Elko) |
| TP4056              | VCC, BAT | 1 x 100 nF  | 1 x 10 uF  |
| MT3608              | VIN, VOUT| 1 x 100 nF  | 1 x 22 uF  |

Der ESP32-S3 hat besonders hohe Stromspitzen (TX-Burst bis 500 mA
fuer 20 us). Der 10 uF Bulk-Cap am WROOM-1 VCC-Pin faengt diese
Spitzen ab. Die HT7333 LDO ist nur fuer 250 mA Dauerstrom
spezifiziert; die Spikes werden vom Bulk-Cap geliefert.

### 5.2 GND-Plane

Die GND-Plane auf L2 ist die wichtigste EMV-Massnahme. Sie muss
durchgehend sein, ohne Slots oder Unterbrechungen unter kritischen ICs.

Regeln fuer die GND-Plane:

- Keine Slots: Eine GND-Plane mit einem Slot verhaelt sich wie
  zwei getrennte Planes. Return-Stroeme muessen um den Slot herum
  fliessen, was die Schleifenflaeche vergroessert und EMV
  verschlechtert.
- Keine Power-Traces auf L2: Die GND-Plane ist ausschliesslich
  fuer Masse reserviert. Power wird auf L1, L3 oder L4 gefuehrt.
- Stitching-Vias: Alle 5-10 mm wird ein GND-Via von L1 nach L4
  gesetzt (Stitching), um die GND-Plane auf allen Layern zu
  verbinden und Hochfrequenz-Stoerstroeme kurzzuschliessen.
- Keine Signal-Leitungen auf L2, die die GND-Plane durchschneiden.
- Unter dem ESP32-S3: GND-Polygon mit Thermal-Vias (min. 4 Vias),
  um Waerme vom WROOM-1 in die GND-Plane abzuleiten.

Sternpunkt-Masse: Die 12V-Eingangsmasse (Bordnetz), die 5V-Masse
(Buck-Converter) und die 3.3V-Masse (LDO) laufen an einem einzigen
Punkt zusammen, nahe dem 12V-Eingangsverbinder. Von dort aus
verteilt sich die Masse ueber die GND-Plane. Dies vermeidet
Masseschleifen zwischen Power-Domaene und Signal-Domaene.

### 5.3 Sensor-Leitungen

Analoge Sensorleitungen (ADC-Eingaenge) sind besonders empfaenglich
fuer EMV-Stoerungen. Massnahmen:

**Geschirmte Kabel:**

- Externe Sensoren (Kuehlmitteltemp NTC, Oeldruck VDO, Kraftstoffgeber)
  werden mit abgeschirmten Kabeln (geflochtener Schirm, SFTP) angeschlossen
- Schirm an EINEM Ende auf Masse legen (Gehaeuseseite / PCB-GND am
  Steckverbinder), um Masseschleifen zu vermeiden
- Schirmwirkung eines geflochtenen Schirms: 80-90% Abschirmung bei HF

**Verdrillte Adern (Twisted Pair):**

- Fuer analoge Sensorsignale: verdrillte Adern reduzieren induktive
  Einkopplung um 20-40 dB
- Besonders wichtig fuer NTC-Sensoren mit langen Leitungen (> 30 cm)
- Bei Sensorleitungen im Motorrad: verdrilltes Paar aus Signal und
  Masse, nicht nur Signalleitung allein

**Maximale Leitungslaenge:**

- ADC-Sensorleitungen: max. 50 cm (mit RC-Filter und Abtastung)
- I2C-Sensorleitungen: max. 30 cm (innerhalb des Gehaeuses)
  (Externe I2C-Geraete sollten ueber einen dedizierten I2C-Extender
  wie PCA9615 angeschlossen werden, falls > 30 cm)
- SPI-Sensorleitungen: max. 20 cm (nur innerhalb des Gehaeuses)
- UART (GPS): max. 100 cm (mit Ferritkern am GPS-Kabel)

**RC-Filter an ADC-Eingaengen:**

Jeder ADC-Eingang bekommt ein RC-Tiefpassfilter direkt am Steckverbinder:

- R = 1 kOhm, C = 100 nF -> Grenzfrequenz ca. 1.6 kHz
- Das filtert hochfrequente Stoerungen (Zuendfunken > 100 kHz) heraus
- Laesst niederfrequente Signale (Kuehlmitteltemp, Bordspannung) passieren
- Der Widerstand begrenzt zusaetzlich den Strom bei Ueberspannung am
  ADC-Pin

### 5.4 SPI-Leitungen: Serienwiderstaende

Die SPI-Clock-Leitung (SCK) kann bei 80 MHz Overshoot und Ringing
erzeugen, besonders wenn die Leitung lang ist (> 50 mm) oder
mehrere Slaves am Bus haengen.

Massnahmen:

- Serienwiderstand 22 Ohm in der SCK-Leitung, direkt am ESP32-S3-Pin
- Serienwiderstand 33 Ohm in der MISO-Leitung (Damping)
- Bei MOSI: optional 22 Ohm, wenn der Display-Treiber empfindlich ist
- Die Widerstaende werden als 0805-SMD-Bauteile direkt am ESP32-S3-Pin
  plaziert

Diese Serienwiderstaende dienen der Impedanz-Anpassung: Sie dmpen
die Reflexionen an den Leitungsenden und reduzieren Overshoot.
Der Wert von 22 Ohm ist ein bewaehrter Standard fuer SPI-Busse mit
kurzen Leitungen (< 50 cm).

### 5.5 Ferrite-Beads

Ferrite-Beads werden eingesetzt, um Hochfrequenz-Stoerungen auf
Power-Leitungen zu filtern. Sie wirken als frequenzabhaengiger
Widerstand: bei DC (0 Hz) haben sie nahezu 0 Ohm, bei 100 MHz
typisch 600 Ohm.

Einsatzstellen:

| Position                      | Ferrite-Bead              | Zweck                          |
|-------------------------------|---------------------------|--------------------------------|
| USB-C VBUS -> 5V-Rail         | 600 Ohm @ 100 MHz, 2A     | Stoerungen vom USB-Host filtern|
| 5V-Rail -> Display-Backlight  | 600 Ohm @ 100 MHz, 2A     | Display-Schaltstoerungen iso. |
| 3.3V -> ESP32-S3 VDDA         | 600 Ohm @ 100 MHz, 500 mA | ADC-Rauschen reduzieren        |
| 3.3V -> GPS-Modul (separat)   | 600 Ohm @ 100 MHz, 500 mA | GPS-Stoerungen isolieren       |

Waehlung des Ferrite-Beads:

- DC-Widerstand (DCR): < 0.1 Ohm (um Spannungsabfall zu minimieren)
- Nennstrom: min. 2x des Betriebsstroms
- Impedanz bei 100 MHz: 600 Ohm typisch (Bereich 300-1000 Ohm)
- Gehaeuse: 0805 (Standard-SMD, leicht zu loeten)

Ferrite-Beads niemals parallel zu Kondensatoren ohne Lastwiderstand
schalten (kann LC-Resonanz erzeugen und Stoerungen verstaerken).

### 5.6 Guard-Rings um ADC-Eingaenge

Guard-Rings sind Masse-Flaechen, die ADC-Eingaenge umschliessen
und Leckstroeme und Stoerungen abweisen.

Implementierung:

```
                    ┌─────────────────────┐
                    │     GND-Polygon     │
                    │  ┌───────────────┐  │
   ADC-Pin ─────────┤  │  (Guard-Ring)  │  ├──────── RC-Filter ──── Sensor
                    │  │               │  │
                    │  └───────────────┘  │
                    │     GND-Polygon     │
                    └─────────────────────┘
```

- Ein GND-Ring (min. 0.3 mm breit) um den ADC-Pin auf L1
- Der Ring ist ueber Stitching-Vias mit der GND-Plane (L2) verbunden
- Der ADC-Pin wird in der Mitte des Rings herausgefuehrt
- Die ADC-Leitung verlaeuft innerhalb des GND-Rings bis zum
  Steckverbinder
- Der GND-Ring schirmt den ADC-Eingang gegen benachbarte Signale ab
  und reduziert Leckstroeme

Dies ist besonders wichtig fuer den Batterie-Monitor (GPIO1) und
den Kraftstoffstand (GPIO2), da diese ueber Spannungsteiler mit
hohen Widerstandswerten (10 kOhm) arbeiten und daher empfindlich
auf Leckstroeme reagieren.

---

## 6. Motorrad-spezifische Anforderungen

### 6.1 Vibration: Bauteil-Auswahl

Die Honda Transalp 600 (PD06) mit ihrem 52-Grad V2-Motor erzeugt
5-10 g RMS am Lenker (siehe 05-gehaeuse-einbau-emv.md Abschnitt 3.2).
Dies ist deutlich mehr als in einer Auto-ECU-Umgebung (< 1 g).

Bauteil-Auswahl fuer Vibrationstauglichkeit:

- **SMD bevorzugt**: SMD-Bauteile haben keine losen Drhte, keine
  schweren Gehaeuse und eine geringe Masse. Sie sind von Natur aus
  vibrationsunempfindlicher als THT-Bauteile.
- **Keine schweren THT-Teile**: THT-Bauteile mit grosser Masse
  (z. B. grosse Elkos > 100 uF) sollten vermieden werden. Wenn
  THT noetig ist, muss das Bauteil mit Schrauben oder Kleber
  gesichert werden.
- **Keine Sockel**: IC-Sockel (DIL, PLCC) sind tabu. Sie lockern
  sich bei Vibration. ICs werden direkt geloetet.
- **Bauteilgroesse**: Kleinere SMD-Packages sind besser. 0603 ist
  besser als 0805, 0805 ist besser als 1206. Aber: 0603 ist
  schwerer zu loeten. Empfehlung: 0805 fuer passive Bauteile
  (Kompromiss aus Vibrationstauglichkeit und Loetbarkeit).
- **Elkos**: SMD-Tantal oder SMD-Keramik statt THT-Elkos. Wenn ein
  grosser Elko (100 uF) am Power-Eingang noetig ist: SMD-Tantal
  mit mindestens 2 Loetpunkten pro Seite.
- **Steckverbinder**: Rastverbinder (mit Verriegelung) oder
  Schraubgewinde. Kein JST-XH ohne Verriegelung.

### 6.2 Sockel vermeiden

Der ESP32-S3-WROOM-1 wird direkt auf das PCB geloetet. Keine
Sockel fuer ICs oder Module. Ein Sockel ist die erste mechanische
Schwachstelle bei Vibration.

Falls der WROOM-1 fuer Wartung austauschbar sein muss: Er wird auf
das PCB geloetet (Loetstation mit Heissluft), nicht gesockelt.
Alternativ kann das gesamte PCB als austauschbare Einheit konzipiert
werden (Steckverbinder fuer alle externen Anschluesse, das PCB
wird als Ganzes getauscht).

### 6.3 Conformal Coating (Schutzlack)

Wie in 05-gehaeuse-einbau-emv.md Abschnitt 4.3 ausfuehrlich
beschrieben, ist Conformal Coating fuer das PCB Pflicht.

Empfehlung: Polyurethan (UR)

- Temperaturbereich: -65 bis +130 Grad C (deckt Motorradbereich ab)
- Chemikalien-Resistenz: Sehr gut (Kraftstoff, Oel, Reinigungsmittel)
- Auftragsmethode: Pinsel oder Spruehdose (2-3 duenne Schichten)
  fuer Prototyp ausreichend
- Dicke: 25-75 um pro Schicht, 2-3 Schichten = 50-225 um gesamt

Vor dem Coating:

- Maskieren: Stecker, Loetpads, USB-C-Buchse, HF-Antennenbereich
  des WROOM-1 nicht beschichten
- Reinigen: PCB mit Isopropanol (IPA) entfetten, Feuchtigkeit
  entfernen
- Aushaerten: 24 h Raumtemperatur oder 60 min bei 80 Grad C

Coating-Bereiche:

- Gesamtes PCB, ausser: Steckverbinder, USB-C-Buchse, SD-Karten-Slot,
  Antennenbereich des WROOM-1, test points
- Besonders wichtig: LNahte um Steckverbinder, LNahte am
  Display-Connector, ungeschuetzte Leiterbahnen am Platinenrand

### 6.4 Waermemanagement

Der ESP32-S3 kann bei Volllast (240 MHz, BLE aktiv, Display-Refresh)
bis zu 500 mW abgeben. Bei 70 Grad C Innentemperatur im Gehaeuse
(Sommer, Sonne) kommt das Modul an sein +85 Grad C Limit.

Waermemanagement-Massnahmen:

1. **GND-Polygon unter dem WROOM-1**: Die Kupferflaeche unter dem
   Modul dient als Waerme-Spreader. Mindestens 100 mm^2
   GND-Flaeche auf L1, direkt unter dem Modul.

2. **Thermal-Vias**: 4-8 Vias (0.3 mm Drill) unter dem WROOM-1,
   die die GND-Flaeche auf L1 mit der GND-Plane auf L2 verbinden.
   Dies leitet die Waerme in die innere Kupferflaeche, die eine
   viel groessere thermische Masse hat.

3. **Keine Hitzestau-Bauteile neben dem WROOM-1**: Der Buck-Converter
   (MP1584) erzeugt ebenfalls Waerme (200-500 mW). Er wird auf der
   gegenueberliegenden Seite des PCB plaziert, nicht direkt neben
   dem ESP32-S3.

4. **Gehaeuse hellfarbig**: Ein weisses oder hellgraues ASA-Gehaeuse
   reflektiert Sonnenstrahlung und bleibt 10-15 Grad C kuehler als
   ein schwarzes Gehaeuse.

5. **Keine aktive Kuehlung**: Ein Luefter ist am Motorrad unbrauchbar
   (Staub, Wasser, Vibration). Das PCB muss passiv gekuehlt werden.
   Die Kupferflaechen des PCB sind der einzige Kuehlkoerper.

6. **Deep-Sleep zwischen Updates**: Die Firmware nutzt Deep-Sleep
   (7 uA) zwischen Sensor-Updates und Display-Refreshes. Bei 10 Hz
   Sensor-Update und 0.3 s Display-Partial-Refresh ist der ESP32-S3
   nur ca. 10% der Zeit aktiv. Dies reduziert die mittlere
   Waermeabgabe auf ca. 50 mW.

7. **Alternative: +105 Grad C Variante**: Falls die +85 Grad C
   Grenze erreicht wird, kann der ESP32-WROOM-32E mit +105 Grad C
   Flash-Chip verwendet werden (35 Grad C Margin statt 15 Grad C).
   Fuer den ESP32-S3 ist dies zum aktuellen Zeitpunkt nicht
   verfuegbar; das +85 Grad C Modul ist die einzige Option.

### 6.5 Stecker-Auswahl

Alle Steckverbinder muessen vibrations- und wasserfest sein (min. IP67).

| Anschluss        | Stecker-Typ           | IP-Rating | Kontakte | Strom  |
|------------------|-----------------------|----------|----------|--------|
| 12V Bordnetz     | AMP Superseal 2-polig| IP67     | 2        | 5A     |
| Sensoren (ADC)   | AMP Superseal 3-polig | IP67     | 3        | 3A     |
| RPM (Optokoppler)| Deusch DTM 2-polig   | IP68     | 2        | 3A     |
| Relais-Steuerung | AMP Superseal 4-polig| IP67     | 4        | 3A     |
| USB-C Service    | IP67 USB-C Panel-Mount| IP67     | —        | 3A     |
| Display (FPC)    | FFC/FPC intern        | —        | 12       | —      |
| GPS (UART)       | JST-SM 4-polig       | IP65     | 4        | 3A     |

Die internen Verbindungen (Display-FPC, GPS-Kabel) koennen mit
JST-SM oder FFC gemacht werden, da sie innerhalb des wasserdichten
Gehaeuses liegen. Alle externen Anschluesse muessen IP67 haben.

Vermeiden: JST-XH (keine Verriegelung, loest sich bei Vibration).

---

## 7. Kosten-Schaetzung

### 7.1 PCB-Fertigung

Preise fuer 5 Stueck (Standard-Kleinserie bei JLCPCB, Stand 2026):

| Option                    | 2-Layer    | 4-Layer    |
|---------------------------|------------|------------|
| Basispreis (5 Stueck)    | 2 EUR      | 8 EUR      |
| ENIG-Oberflaeche          | +3 EUR     | +3 EUR     |
| 2 oz Kupfer (L1/L4)      | +3 EUR     | +3 EUR     |
| Conformal Coating (Selektiv) | +5 EUR | +5 EUR     |
| Versand (DHL Express)     | 15 EUR     | 15 EUR     |
| **Gesamt**                | **28 EUR** | **34 EUR** |

Hinweis: Die Preise sind Richtwerte und koennen je nach Optionen,
Lieferzeit und Waehrungskurs schwanken. JLCPCB bietet oft Rabatte
fuer Neukunden an. PCBWay hat aehnliche Preise.

Fuer den Prototyp (Phase 1) ist ein 2-Layer-Board ohne ENIG
(HASL reicht fuer Phase 1) ausreichend: ca. 5-10 EUR fuer 5 Stueck.

Fuer die Production (Phase 2+) ist das 4-Layer-Board mit ENIG
und Conformal Coating empfohlen: ca. 34 EUR fuer 5 Stueck.

### 7.2 Bauteile-Kosten (BOM)

Die vollstaendige BOM ist in 09-master-plan.md Abschnitt 10.1
dokumentiert. Hier die Zusammenfassung der PCB-relevanten Bauteile:

| Bauteil                    | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) |
|----------------------------|-------|-------------------|-------------------|
| ESP32-S3-WROOM-1 N8R2      | 1     | 8                 | 8                 |
| MP1584EN Modul             | 1     | 2                 | 2                 |
| HT7333 LDO                 | 1     | 0,50              | 0,50              |
| TP4056 Modul               | 1     | 0,50              | 0,50              |
| MT3608 Modul               | 1     | 0,80              | 0,80              |
| 18650 Akkuhalter            | 1     | 0,50              | 0,50              |
| 1N5822 Schottky            | 2     | 0,10              | 0,20              |
| BMI270 (IMU)               | 1     | 5                 | 5                 |
| BMP388 (Baro)              | 1     | 5                 | 5                 |
| PC817 Optokoppler           | 2     | 0,50              | 1                 |
| Ferrite-Beads (0805)       | 4     | 0,20              | 0,80              |
| Decoupling-Caps (0805)     | 15    | 0,05              | 0,75              |
| Pull-Up-Widerstaende (0805)| 10    | 0,02              | 0,20              |
| Serienwiderstaende (0805)  | 6     | 0,02              | 0,12              |
| Zener-Diode 3.3V            | 2     | 0,20              | 0,40              |
| TVS-Diode SMAJ15A           | 2     | 0,50              | 1                 |
| USB-C IP67 Buchse           | 1     | 2                 | 2                 |
| MicroSD-Slot                | 1     | 1                 | 1                 |
| FPC-Connector 12-polig      | 1     | 0,50              | 0,50              |
| AMP Superseal Stecker       | 4     | 1,50              | 6                 |
| Deusch DTM Stecker          | 1     | 3                 | 3                 |
| PCB (4-Layer, ENIG, 5 St.) | 1/5   | 6,80              | 6,80              |
| Conformal Coating (UR)      | 1     | 5                 | 5                 |
| Verbrauchsmaterial          | —     | —                 | 3                 |
| **Zwischensumme**           |       |                   | **~54**           |
| Puffer 15%                  |       |                   | 8                 |
| **Gesamt PCB-BOM**          |       |                   | **~62**           |

Diese Kosten sind nur fuer das PCB und die direkt darauf plazierten
Bauteile. Module wie GPS (NEO-M9N, 30 EUR), Display (3.5" IPS, 18 EUR),
E-Paper (12 EUR), SD-Karte (6 EUR), Akku (6 EUR) und Gehaeuse-Material
kommen zusaetzlich hinzu (siehe 09-master-plan.md fuer die
Gesamtkosten-Schaetzung der Standard-Stufe: ~173 EUR inklusive Puffer).

### 7.3 Zusammenbau: DIY vs. bestueckt

| Option              | Kosten (zusaetzlich zur BOM) | Aufwand | Qualitaet |
|---------------------|------------------------------|---------|------------|
| DIY (Handloeten)    | 0 EUR (Werkzeug vorhanden)   | 4-8 h   | Mittel (Loetfehler moeglich) |
| DIY (Heissluft)     | 0 EUR (Heissluftstation)     | 2-4 h   | Gut        |
| JLCPCB Bestueckung  | 15-25 EUR                    | 0 h     | Sehr gut (maschinell) |
| Lokaler Dienstleister| 30-50 EUR                    | 0 h     | Sehr gut   |

Empfehlung:

- Phase 1 (Prototyp): DIY-Loeten. Das DevKitC-1 auf Perfboard
  braucht kein Custom PCB.
- Phase 2 (Production): JLCPCB-Bestueckung fuer die SMD-Bauteile
  (WROOM-1, passive Bauteile, Sensoren). THT-Bauteile (Steckverbinder,
  groessere Elkos) selbst nachloeten. Das spart Kosten und ist
  flexibel.

JLCPCB bietet Bestueckung fuer Bauteile an, die in ihrem Katalog
gefuehrt werden. Nicht alle Bauteile (z. B. BMI270, BMP388)
sind bei JLCPCB verfuegbar. Diese muessen separat bestellt und
nachgeloetet werden. Die Bestueckungskosten bei JLCPCB fuer
ein Board dieser Komplexitaet (ca. 50 Bauteile) liegen bei
15-25 EUR fuer 5 Stueck.

---

## 8. Zusammenfassung der Design-Entscheidungen

| Parameter              | Wert                         | Begruendung                       |
|------------------------|------------------------------|-----------------------------------|
| Board-Groesse          | 80 x 50 mm                   | Passt ins Gehaeuse, ausreichend Platz |
| Layer-Count            | 4-Layer                      | Durchgehende GND-Plane fuer EMV   |
| Material               | FR4, 1.6 mm                  | Standard, kein Aufpreis           |
| Kupferdicke            | 1 oz (L2/L3), 2 oz (L1/L4)  | 2 oz fuer Power-Traces und Kuehlung|
| Oberflaeche            | ENIG                         | Ebene Pads fuer feinen Pitch      |
| GND-Plane (L2)         | Durchgehend, keine Slots     | EMV, Signal-Integritaet           |
| Power-Plane (L3)       | 3.3V durchgehend, 5V als Polygon | Einfaches Routing            |
| Conformal Coating      | Polyurethan, 2-3 Schichten  | Feuchtigkeitsschutz, Chemikalien  |
| Steckverbinder         | AMP Superseal, Deusch DTM   | IP67, vibrationsicher             |
| Antennen-Freiraum      | 15 mm am WROOM-1             | BLE-Reichweite                    |
| Decoupling             | 100 nF je VCC-Pin, 10 uF je IC| Rauschminimierung               |
| Ferrite-Beads          | USB-Power, Display, ADC      | HF-Filterung                      |
| SPI-Serienwiderstaende | 22-33 Ohm auf SCK/MISO      | Impedanz-Anpassung                |
| ADC-Guard-Rings        | Um alle ADC-Eingaenge        | Leckstrom-Reduktion                |
| Vibration              | Nur SMD, keine Sockel        | Vibrationstauglichkeit            |

---

## 9. Naechste Schritte fuer den PCB-Designer

1. **Schematic erstellen**: Alle Komponenten und Verbindungen
   nach der Pin-Belegung in 02-hardware-deep-dive.md Abschnitt 10
   erfassen. Pin-Mux-Checks durchfuehren (ADC2-Pins fuer analoge
   Sensoren vermeiden: ADC1 GPIO1-10 verwenden).

2. **Footprints waehlen**: WROOM-1-Footprint aus dem
   Espressif-KiCad-Library. USB-C IP67-Footprint vom Hersteller.
   Alle passiven Bauteile als 0805-SMD.

3. **Placement nach Abschnitt 3**: Komponenten nach dem
   ASCII-Placement-Diagramm plazieren. Kritisch: kurze Leitungen
   fuer SPI, I2C und Decoupling-Caps.

4. **Routing nach Abschnitt 4**: Power-Traces als Polygone,
   Signal-Traces 0.15 mm, Vias 0.2/0.3 mm. GND-Plane auf L2
   unangetastet lassen.

5. **DRC-Check**: Design Rule Check in KiCad ausfuehren.
   Min. Clearance: 0.15 mm (6 mil). Min. Trace: 0.15 mm.
   Min. Drill: 0.2 mm. Min. Annular Ring: 0.1 mm.

6. **Gerber-Export**: Alle Layer exportieren (L1, L2, L3, L4,
   Silk Top/Bottom, Solder Mask Top/Bottom, Drill, Outline).

7. **JLCPCB-Bestellung**: 4-Layer, ENIG, 1.6 mm, 2 oz auf L1/L4
   (falls verfuegbar), 5 Stueck.

8. **Bestueckung**: SMD-Bauteile per Heissluft oder JLCPCB-Bestueckung.
   THT-Bauteile (Steckverbinder, Elkos) von Hand nachloeten.

9. **Test**: PCB mit Multimeter auf Kurzschluesse pruefen (besonders
   12V zu 5V zu 3.3V zu GND). Danach schrittweise Inbetriebnahme:
   zuerst Power-Sektion (Buck, LDO), dann MCU, dann Sensoren,
   dann Display, dann Peripherie.

10. **Conformal Coating**: Nach erfolgreichem Funktionstest.
    Maskieren, reinigen, 2-3 Schichten Polyurethan auftragen,
    24 h aushaerten lassen.

---

## 10. Quellen

- IPC-2221: Generic Standard on Printed Board Design
  (Trace-Width-Berechnung)
- 02-hardware-deep-dive.md: Pin-Belegung, Komponenten-Auswahl
- 05-gehaeuse-einbau-emv.md: Gehaeuse-Abmessungen, EMV-Massnahmen,
  Conformal Coating, Vibration
- 09-master-plan.md: BOM, Kosten-Schaetzung
- 11-firmware-architecture.md: SPI-Bus-Struktur, Task-Architektur
- Espressif ESP32-S3 Datasheet:
  https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- JLCPCB Capabilities:
  https://jlcpcb.com/capabilities/pcb-capabilities
- Analog Devices Ferrite Bead Selection Guide:
  https://www.analog.com/en/design-notes/selecting-the-right-ferrite-bead-for-emc-compliance.html
- Texas Instruments Load Dump Application Note:
  https://www.ti.com/lit/an/slvaef9/slvaef9.pdf

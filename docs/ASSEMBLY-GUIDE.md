# PCB-Bestückungs- und Inbetriebnahme-Guide

> **Parent:** TOT-16 TRANSALP Bordcomputer
> **Sprint:** 2 — Integration & Production Readiness
> **Board:** ESP32-S3-WROOM-1, 80 × 50 mm, 4-Layer, ENIG, 1.6 mm
> Stand: Oktober 2026

Dieser Guide führt Schritt für Schritt durch die Bestückung der Leiterplatte
und die gestaffelte Inbetriebnahme (Bring-up). Er schließt die Lücke zwischen
dem Bestell-BOM ([`docs/BOM.md`](BOM.md)) und dem funktionalen Test-Bench
([`docs/HARDWARE-TEST-BENCH.md`](HARDWARE-TEST-BENCH.md)).

Quellen: [`docs/PCB-LAYOUT.md`](PCB-LAYOUT.md) §3 (Placement), §6 (Motorrad-spezifisch),
§9 (Nächste Schritte); [`docs/CASE-DESIGN.md`](CASE-DESIGN.md) §6 (Verschraubung);
[`docs/PINMAP.md`](PINMAP.md) (Pin-Referenz).

## Inhaltsverzeichnis

1. [Vorbereitung](#1-vorbereitung)
2. [Bare-PCB-Eingangsprüfung](#2-bare-pcb-eingangspruefung)
3. [Bestückungsreihenfolge](#3-bestueckungsreihenfolge)
4. [Abschnittsbestückung](#4-abschnittsbestueckung)
5. [Pre-Power Kontinuitäts- und Kurzschluss-Test](#5-pre-power-kontinuitaets--und-kurzschlusstest)
6. [Gestaffelte Inbetriebnahme (Bring-up)](#6-gestaffelte-inbetriebnahme-bring-up)
7. [Subsystem-Funktionstests](#7-subsystem-funktionstests)
8. [Conformal Coating](#8-conformal-coating)
9. [Gehäuse-Montage](#9-gehaeuse-montage)
10. [Fehler-Isolation / Debug-Tree](#10-fehler-isolation--debug-tree)

---

## 1. Vorbereitung

### 1.1 Werkzeug

- Lötkolben mit feiner Spitze (0,4 mm) + Heißluftstation (SMD-Reflow)
- Entlötpumpe / Entlötlitze
- Multimeter (Durchgangs- und Spannungsmessung)
- Labornetzteil mit einstellbarer Strombegrenzung (0–15 V, 0–3 A)
- Pinzetten (fein, antimagnetisch), Lupe/Mikroskop
- ESD-Matte und ESD-Armband
- Reinigungsalkohol (Isopropanol), flux-Entferner, weiche Bürste
- Wärmeleitpaste / Kapton-Tape zum Abdecken beim Coating

### 1.2 Referenzdokumente

| Dokument                | Zweck                                            |
|-------------------------|--------------------------------------------------|
| `docs/BOM.md`           | Bauteile-Liste und Bezugsquellen                 |
| `docs/PCB-LAYOUT.md`    | Placement, Routing-Regeln, Stackup               |
| `docs/PINMAP.md`        | Alle Pin-Zuweisungen (alleinige Referenz)         |
| `docs/HARDWARE-TEST-BENCH.md` | Funktionale Test-Prozeduren               |
| `hardware/gerber/`       | Gerber-Files und Fertigungsspezifikation         |

### 1.3 ESD- und Arbeitsplatz-Regeln

- ESD-Matte geerdet, Bauteile in ESD-Beuteln lagern
- Keine Bauteile auf ungeschützter Plastikoberfläche
- Feuchtigkeitsempfindliche Bauteile (MSL): ESP32-S3-WROOM-1 und BMP388 vor dem
  ReflowBacken trocknen, falls aus geöffneter Vakuumverpackung (4 h bei 125 °C oder
  gemäß MSL-Klasse in der Datenblatt-Tabelle)

---

## 2. Bare-PCB-Eingangsprüfung

Vor der Bestückung das nackte PCB auf Sicht prüfen:

- [ ] Board-Abmessungen 80 × 50 mm, Kanten sauber gefräst
- [ ] 4× M3-Befestigungslöcher (3,2 mm) an (5,5) (75,5) (5,45) (75,45) mm vorhanden
- [ ] Lötstopplack ohne Blasen/Risse, Kupferflächen oxidfrei
- [ ] Bohrungen nicht verstopft, Durchkontaktierungen (Vias) intakt
- [ ] Silk-Screen lesbar (Platzierungszonen POWER / MCU / I2C / DISPLAY / SD / GPS UART)
- [ ] Multimeter-Durchgang: keine Kurzschlüsse zwischen den Netzen
  12V ↔ GND, 5V ↔ GND, 3V3 ↔ GND (auf dem nackten PCB ~∞, mindestens > 1 MΩ)

---

## 3. Bestückungsreihenfolge

Allgemeine Regel (PCB-LAYOUT.md §9): **SMD zuerst (Reflow/Heißluft), danach THT
von Hand löten.** Innerhalb SMD: flache Bauteile zuerst (0805-Passive), dann
höhere Bauteile, zuletzt Module.

Empfohlene Reihenfolge:

1. **Power-Sektion** (linker Board-Rand) — siehe Abschnitt 4.1
2. **ESP32-S3-WROOM-1** (Mitte) — siehe Abschnitt 4.2
3. **I2C-Sensoren** BMI270 + BMP388 — Abschnitt 4.3
4. **SPI-Bus-Geräte** (MicroSD, Display-FPC-Connector, E-Paper-Header) — Abschnitt 4.4
5. **GPS-Interface** (UART-Header, PPS) — Abschnitt 4.5
6. **THT-Stecker und Elkos** — Abschnitt 4.6
7. **Optokoppler / MOSFET / Gate-Driver** — Abschnitt 4.7

> **Hinweis zu Bauteil-Seiten:** L1 (Top) trägt die meisten SMD-Bauteile. Der
> 18650-Halter ist auf L4 (Bottom) über der Power-Sektion. PCB-LAYOUT.md §3.3
> gibt die genaue Platzierung.

---

## 4. Abschnittsbestückung

### 4.1 Power-Sektion (12V → 5V → 3.3V + Lade-/Boost-Logik)

Bauteile (siehe `docs/BOM.md` §1.4 / §1.6):

| Bauteil             | Funktion              | Hinweis                                |
|---------------------|-----------------------|----------------------------------------|
| Sicherung + Halter  | 12V-Eingangsschutz   | THT, nahe Board-Kante                 |
| TVS-Diode           | Überspannungsschutz   | Nahe Sicherung                          |
| MP1584 Buck         | 12V → 5V              | Ausrichtung laut Datenblatt            |
| HT7333 LDO (SOT-89) | 5V → 3.3V             | Direkt neben Buck-Ausgang              |
| TP4056              | 5V → 18650-Ladung     | Neben Buck                             |
| MT3608 Boost        | 3.7V → 5V             | Neben Buck                             |
| Ferrite Beads       | Entkopplung           | An VBUS und Sensor-Leitungen           |
| Elkos 100µF/25V     | Bulk-Caps Power       | THT, Polarity beachten!                |

- Entkopplungs-Caps (100 nF, 10 µF) direkt an jedem VCC-Pin, max. 2 mm Abstand
  (PCB-LAYOUT.md §5.1)
- 5V-Polygon als breite Fläche (min. 2 mm) vom Buck zur Schiene
- 3.3V über min. 3 Vias (0,3 mm) zur inneren Plane (L3)

### 4.2 ESP32-S3-WROOM-1 (Mitte, X≈35/Y≈20 mm)

- Modul mit Antenne zum Board-Rand, **15 mm Antennen-Keepout** frei halten
  (kein Kupfer, keine Bauteile, kein Lötstopplack im Antennenbereich)
- Entkopplungs-Caps direkt an VCC-Pins
- Thermal-Vias (4–8×, 0,3 mm) unter dem Modul für GND/Wärme
- Reflow-Profil gemäß WROOM-1 Datenblatt (Peak 245 °C, TAL > 30 s)
- Nach dem Reflow: Modul-Ausrichtung und Pin-1-Markierung prüfen

### 4.3 I2C-Sensoren (BMI270 + BMP388)

- Neben dem ESP32-S3, I2C-Leitungen < 20 mm
- Beide Sensoren auf derselben Seite des MCU
- **BMI270** auf Schaumstoff-Pad / Silikon-Verguss für Vibrationsentkopplung
- Pull-Ups 4,7 kΩ am ESP32-Pin (nicht am Sensor-Ende)
- BMP388: Lüftungsöffnung im Gehäuse für barometrischen Druck beachten

### 4.4 SPI-Bus-Geräte

Shared FSPI-Bus: MOSI=GPIO11, SCK=GPIO12, MISO=GPIO13 (siehe `docs/PINMAP.md`)

| Gerät              | CS-Pin  | DC-Pin | Hinweis                        |
|--------------------|---------|--------|--------------------------------|
| TFT (ILI9488)      | GPIO10  | GPIO14 | FPC-Connector 12-polig 0,5 mm  |
| E-Paper 2.9"       | GPIO40  | GPIO41 | RST=GPIO42, BUSY separat       |
| MicroSD            | GPIO47  | —      | MISO nur für SD (TFT/EPD write-only) |

- SCK Serienwiderstand 22 Ω, CS Pull-Up 10 kΩ
- Bus-Leitungen parallel, gleiche Länge (±5 mm), < 30 mm

### 4.5 GPS-Interface

GPS-Modul (NEO-M9N / AT6558) ist auf einer separaten Tochterplatine
(PCB-LAYOUT.md §3.3). Auf dem Main-PCB nur:

- 4-poliger JST-SM-Stecker (UART TX=43, RX=44, VCC, GND)
- GPS_PPS an GPIO39 (RTC-GPIO für Deep-Sleep-Wake)

### 4.6 THT-Stecker und Elkos

Nach allen SMD-Bauteilen von Hand löten:

- IP67-Stecker: USB-C (Service), AMP Superseal (12V, Sensoren, Relais), Deusch DTM (RPM)
- Display-FPC-Connector (12-polig, 0,5 mm)
- Elkos (Polarität!), 18650-Halter (Bottom)

### 4.7 Optokoppler / MOSFET / Gate-Driver

- PC817 Optokoppler ×2 (RPM/Tacho-Isolation)
- TC4427 Gate-Driver, IRLZ44N MOSFET (Heizgriff/Hilfslicht-Steuerung)
- Schottky-Dioden 1N5822 (Power-Path-ORing)

---

## 5. Pre-Power Kontinuitäts- und Kurzschlusstest

**Vor dem ersten Einschalten zwingend!** Test-Points TP01–TP04
(`docs/HARDWARE-TEST-BENCH.md` §3.1): 3.3V / 5V / 12V / GND.

Mit dem Multimeter im Durchgangsmodus (Widerstand) messen:

| Messung        | Soll (unbestromt)     | Aktion bei Abweichung            |
|----------------|-----------------------|----------------------------------|
| 12V ↔ GND      | > 10 kΩ (ideal ∞)    | Kurzschluss suchen, Reflow prüfen |
| 5V ↔ GND       | > 1 kΩ               | Buck/MOSFET auf Kurzschluss      |
| 3.3V ↔ GND     | > 500 Ω              | LDO/Modul prüfen                 |
| 12V ↔ 5V       | > 10 kΩ              | Buck-Diode prüfen                |
| 5V ↔ 3.3V      | > 1 kΩ               | LDO-Verdrahtung prüfen           |

Nur wenn alle Widerstände im grünen Bereich sind → weiter mit Bring-up.

---

## 6. Gestaffelte Inbetriebnahme (Bring-up)

> **Sicherheitsregel:** Erstes Einschalten immer am strombegrenzten Labornetzteil,
> NICHT direkt an 12V Bordnetz oder USB. Strombegrenzung niedrig ansetzen und
> hochregeln, bis die Schiene stabil ist.

### Phase 1 — Power-Sektion (strombegrenzt)

1. Labornetzteil auf **12.0 V, Strombegrenzung 50 mA** einstellen
2. An 12V-Eingang (TP03 / AMP Superseal 2-polig) anschließen
3. Stromaufnahme prüfen: soll < 50 mA sein (Leerlauf, MCU noch nicht bootend)
4. Bei Überstrom → **sofort aus**, Kurzschluss lokalisieren
5. Wenn stabil: Spannungen messen:
   - TP02 (5V): **5.0 V ± 0.1 V** (MP1584 Buck)
   - TP01 (3.3V): **3.3 V ± 0.1 V** (HT7333 LDO)
   - GND-Bezug TP04
6. Strombegrenzung auf 200 mA hochregeln, erneut prüfen

### Phase 2 — ESP32-S3 Flash & Boot

1. USB-C (Service-Port) an den PC anschließen
2. Bootloader-Modus: BOOT-Taster (GPIO0) gedrückt halten, RESET, dann loslassen
3. Flashen mit esptool:
   ```bash
   cd firmware
   idf.py -p /dev/cu.usbmodem* flash monitor
   ```
4. Boot-Log beobachten: `selftest`-Ausgabe, keine Panic/Backtrace
5. STATUS_LED (GPIO21) blinkt als Heartbeat

### Phase 3 — I2C-Bus-Scan

```bash
# über die serielle Test-Schnittstelle (HARDWARE-TEST-BENCH.md §4.1)
i2c-scan
```

Erwartete Adressen:

| Adresse | Gerät   |
|---------|---------|
| 0x68    | BMI270 (IMU), AD0=LOW |
| 0x76    | BMP388 (Barometer)    |

Fehlen Adressen → Verdrahtung/Pull-Ups prüfen (siehe Debug-Tree §10).

### Phase 4 — SPI-Geräte enumerieren

- TFT initialisiert (Bildschirm zeigt Boot-Screen)
- E-Paper Refresh sichtbar
- MicroSD-Karte erkannt (Log-Datei wird geschrieben)

### Phase 5 — Peripherie

- ADC-Sensoren (Temperatur, Tank, Öldruck) plausible Werte
- RPM/Tacho (PCNT) reagiert auf Testsignale (Test-Bench §2.1/2.2)
- GPS (UART) NMEA-Daten empfangen
- BLE Advertisement sichtbar
- Buttons reagieren (MENU/SELECT/BACK/BOOT)

---

## 7. Subsystem-Funktionstests

Nach erfolgreichem Bring-up die detaillierten Prozeduren aus
`docs/HARDWARE-TEST-BENCH.md` §5 durchführen:

- §5.1 RPM/Tacho (PCNT)
- §5.2 ADC-Sensoren (Temperatur, Tank, Öldruck)
- §5.3 Bordspannung
- §5.4 Display
- §5.5 BLE
- §5.6 GPS
- §5.7 IMU
- §5.8 SD-Karten-Logging
- §5.9 Power-Consumption
- §5.10 Boot-Zeit

Ergänzend zu den bestehenden Prozeduren jetzt zusätzlich testen:

- **Taster:** BOOT(GPIO0)/MENU(6)/SELECT(7)/BACK(3) — jeder Taster löst Event aus
- **Buzzer (GPIO38):** akustisches Signal bei Warnung
- **Status-LED (GPIO21):** Heartbeat + BLE-Status
- **MOSFET/Relais:** Heizgriff/Hilfslicht schaltet bei Aktivierung
- **Barometer:** BMP388 liefert plausible Höhe/Druck (zusätzlich zu §2.9-Simulator)

---

## 8. Conformal Coating

Nach erfolgreichem Funktionstest (PCB-LAYOUT.md §6.3):

1. **Maskieren:** alle Stecker, USB-C, SD-Slot, Antennenbereich, Testpunkte,
   BMP388-Drucköffnung mit Kapton-Tape abdecken
2. **Reinigen:** Isopropanol, flux-Reste entfernen, trocknen lassen
3. **Auftragen:** Polyurethan (UR), 25–75 µm pro Schicht, 2–3 dünne Schichten
4. **Aushärten:** 24 h bei Raumtemperatur oder 60 min @ 80 °C
5. **Prüfen:** gleichmäßige Schicht, keine Brücken, Masken entfernen

Temperaturbereich: −65 bis +130 °C. Beschichtung nach 2–3 Saisons erneuern.

---

## 9. Gehäuse-Montage

Siehe `docs/CASE-DESIGN.md` und `hardware/case/README.md` (3D-Druck-Dateien).

1. PCB in die Führungsschienen der Basisschale einschieben (Toleranz 0,2 mm Spiel)
2. 4× M3 × 8 mm Senkkopfschrauben mit Gummipuffer in die Schraubsäulen
3. Display-Modul in PETG-Rahmen, auf 0,5 mm Silikon-Pad, 4× M2,5 × 6 mm
4. Silikon-O-Ring (2 mm) in Nut, Deckel aufsetzen (Schuppungsprinzip: Naht unten)
5. 4× M3 × 12 mm Senkkopfschrauben + Federscheiben verschrauben
6. Kabeldurchführungen (PG7/PG9) an Unterseite, O-Ring zwischen Durchführung und Wand
7. Druckausgleichsmembran (ePTFE, M6) an Unterseite vertikal
8. Kugelgelenk-Halterung an Crossbar (22 mm), 2× M6 × 20 mm U-Bolt
9. Vibrationsdämpfer (Gummi-Metall, Shore 50–60A) zwischen Adapter und Gehäuse

---

## 10. Fehler-Isolation / Debug-Tree

| Symptom                          | Erste Prüfung                                  |
|----------------------------------|------------------------------------------------|
| Keine 5V-Schiene                 | Sicherung, TVS, MP1584-Buck, 12V-Anschluss     |
| Keine 3.3V-Schiene               | HT7333-LDO, Eingangsspannung, Kurzschluss 3V3  |
| ESP32 bootet nicht               | USB-C-Verbindung, Strapping-Pins (GPIO0/3/45/46), Flash via esptool |
| I2C-Adresse fehlt (0x68/0x76)    | SDA=8/SCL=9 Verdrahtung, Pull-Ups 4,7 kΩ, VCC  |
| TFT zeigt nichts                 | CS=10/DC=14/RST=17/BL=18, SPI-Bus, FPC-Connector |
| SD-Karte nicht erkannt           | CS=47, MISO=13, Karte formatiert (FAT32)        |
| GPS keine NMEA-Daten             | UART RX=44/TX=43, Baudrate, GPS-Tochterplatine  |
| RPM zeigt 0                     | PCNT0 GPIO15, Optokoppler, Testsignal          |
| BLE nicht sichtbar               | Antennen-Keepout frei? Cu-Abschirmung zu nah?   |
| Selbsttest schlägt fehl          | Serielles Log lesen, betroffenes Modul isolieren |

Bei Architektur-relevanten Pin-Problemen → `docs/PINMAP.md` als alleinige
Referenz konsultieren und mit `firmware/main/board_config.h` abgleichen
(siehe Pinmap-Verifikationsbericht `docs/research/pinmap-firmware-verification.md`).

---

*Bei Fragen zur Firmware-Architektur an @klaus (CTO); bei Ressourcenbedarf an
@horst (CEO). Test-Fixtures und Simulator-Koordination mit @greta (QA).*

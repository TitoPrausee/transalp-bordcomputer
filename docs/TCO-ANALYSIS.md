# Total Cost of Ownership & Budget-Analyse

> Stand: Oktober 2026
> Projekt: Transalp Bordcomputer (Honda XL600V Transalp PD06)
> Plattform: ESP32-S3-WROOM-1 (N8R2), 8 MB Flash, 2 MB PSRAM
> Vorausgehende Dokumente: docs/BOM.md, docs/research/09-master-plan.md,
>   docs/PCB-LAYOUT.md, docs/CASE-DESIGN.md, docs/ROADMAP.md
> Alle Preise in EUR, Stand Oktober 2026, inklusive deutscher MwSt. (19%)
> bei Endkunden-Quellen, netto bei B2B-Lieferanten.

---

## Inhaltsverzeichnis

1. [Einmalige Kosten](#1-einmalige-kosten)
2. [Wiederkehrende Kosten](#2-wiederkehrende-kosten)
3. [Kosten-Szenarien](#3-kosten-szenarien)
4. [Kosten-Nutzen-Analyse](#4-kosten-nutzen-analyse)
5. [Skalierungs-Kosten](#5-skalierungs-kosten)
6. [Wartungs- und Lebenszyklus-Kosten](#6-wartungs-und-lebenszyklus-kosten)
7. [Risiko-Kosten](#7-risiko-kosten)
8. [Zusammenfassung](#8-zusammenfassung)

---

## 1. Einmalige Kosten

Die einmaligen Kosten umfassen alle Ausgaben, die vor der Inbetriebnahme
des Bordcomputers anfallen. Sie setzen sich zusammen aus den Bauteilkosten
(BOM), den Werkzeug-Kosten und gegebenenfalls externer Entwicklungszeit.

### 1.1 BOM-Kosten aufgeschluesselt nach Kategorie

Die BOM-Kosten basieren auf der detaillierten Stückliste in docs/BOM.md.
Alle Preise sind Einzelstueckpreise (1 Stueck) in EUR. Die Aufschluesselung
erfolgt fuer die Standard-Konfiguration, die alle Must- und Should-Features
abdeckt.

#### 1.1.1 MCU und Peripherie

| Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) |
|---|---|---|---|---|
| ESP32-S3-WROOM-1 N8R2 | 8MB Flash, 2MB PSRAM, BLE 5.0, 45 GPIO | 1 | 5,40 | 5,40 |
| USB-C Buchse IP67 | Panel-Mount, wasserdicht, mit Dichtung | 1 | 3,50 | 3,50 |
| MicroSD-Karten-Slot | Push-Push, SMD, 8-polig, SPI-Mode | 1 | 1,20 | 1,20 |
| MicroSD-Karte 16GB | Class 10 / U1, FAT32 | 1 | 6,00 | 6,00 |
| FPC-Connector 12-polig | 0,5mm Pitch, fuer Display-Anbindung | 1 | 0,50 | 0,50 |
| **Zwischensumme MCU** | | | | **16,60** |

Die MCU-Sektion ist mit 16,60 EUR vergleichsweise guenstig. Der ESP32-S3
bietet einen enormen Funktionsumfang (BLE 5.0, WiFi, Dual-Core 240 MHz,
USB-OTG) bei einem Preis unter 6 EUR. Der Grossteil der Kosten entfaellt
auf Speichermedien (SD-Karte und Slot: 7,20 EUR).

#### 1.1.2 Displays (TFT + E-Paper)

| Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) |
|---|---|---|---|---|
| TFT-Display 3,5" IPS | 480x320 Pixel, SPI, 1000 nits Backlight | 1 | 18,00 | 18,00 |
| E-Paper-Display 2,13" | 250x122 Pixel, monochrom | 1 | 12,00 | 12,00 |
| Acrylglas-Abdeckung | 2mm PMMA, konvex, hydrophob | 1 | 2,00 | 2,00 |
| **Zwischensumme Displays** | | | | **32,00** |

Die Displays sind mit 32,00 EUR ein wesentlicher Kostenfaktor (11,3% der
BOM). Das TFT-Display ist der teuerste Einzelbauteil nach dem GPS-Modul
und dem Oeldrucksensor. Das E-Paper-Display ist fuer die Sonnenlicht-
Lesbarkeit statischer Werte (Geschwindigkeit, GPS-Info) essenziell.

#### 1.1.3 Sensoren

| Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) |
|---|---|---|---|---|
| GPS-Modul (4-Konstellation) | GNSS: GPS+GLONASS+Galileo+BeiDou, 25Hz, 2,0m CEP | 1 | 24,60 | 24,60 |
| GPS-Antenne | Active Patch 25x25mm, 28dB Gain, Magnetfuss | 1 | 8,00 | 8,00 |
| IMU (6-DoF) | Accel+Gyro, 685µA, I2C/SPI, 2KB FIFO | 1 | 5,00 | 5,00 |
| Barometer | 300-1250 hPa, ±0,03 hPa rel., I2C/SPI | 1 | 3,50 | 3,50 |
| Kuehlmittel-Temperatur | Digital, 1-Wire, ±0,5°C, wasserdicht | 1 | 3,00 | 3,00 |
| Oeldrucksensor | Analog 0-5 bar, M10x1, wasserdicht | 1 | 25,00 | 25,00 |
| Hall-Sensor (Rad) | Unipolar, 4,5-24V, TO-92 + Magnet 5mm | 1 | 2,00 | 2,00 |
| Bordspannungs-Messung | Spannungsteiler 100k/33k, 1%, 0805 SMD | 1 | 0,10 | 0,10 |
| Kraftstoffstand-Messung | Spannungsteiler + Operationsverstaerker | 1 | 1,00 | 1,00 |
| **Zwischensumme Sensoren** | | | | **72,20** |

Die Sensor-Sektion ist mit 72,20 EUR die teuerste Kategorie (25,4% der
BOM). Dominierend sind das GPS-Modul (24,60 EUR) und der Oeldrucksensor
(25,00 EUR) mit zusammen 49,60 EUR. Der Oeldrucksensor ist ein
Fahrzeugspezifisches Bauteil, das analoge Signale liefert und im
Gegensatz zu Digitalsensoren aufwaendiger auszuwerten ist. Das
GPS-Modul mit 4-Konstellation-Empfang und 25Hz Update-Rate rechtfertigt
seinen Preis durch deutlich bessere Performance als Budget-Alternativen
(mit nur 2 Konstellationen und 1-10Hz).

#### 1.1.4 Mechanische Teile (Gehaeuse, Stecker, Kabel)

| Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) |
|---|---|---|---|---|
| ASA-Filament 1kg | 1,75mm, UV-stabil, hell | 1 | 25,00 | 25,00 |
| PETG-Filament 500g | Fuer Display-Rahmen / Innenteile | 1 | 12,00 | 12,00 |
| Silikon-O-Ring | 2mm Cord, Shore 60A, -40 bis +200°C | 1 | 2,00 | 2,00 |
| EPDM-Schaumstoff | Selbstklebend, 5mm, Sekundaerdichtung | 1 | 3,00 | 3,00 |
| Kabelverschraubungen PG7 | PA66, IP67, fuer Sensorkabel | 4 | 0,80 | 3,20 |
| Kabelverschraubungen PG9 | PA66, IP67, fuer Sensorkabel | 2 | 1,00 | 2,00 |
| Druckausgleichsmembran | ePTFE, IP68, M6x0,75 | 1 | 8,00 | 8,00 |
| Kupferfolie | Selbstklebend, 0,05mm, 50mm breit, EMV | 1 | 8,00 | 8,00 |
| Conformal Coating | Polyurethan, 50ml Spruehdose, UR-Typ | 1 | 12,00 | 12,00 |
| Acrylglas-Platte | 2mm, transparent, 100x80mm | 1 | 2,00 | 2,00 |
| Stecker IP67 (2/3/3/4-pol) | Wasserdicht, Rastverbinder | 4 | 2,50 | 10,00 |
| JST-SM Stecker 4-polig | Wasserdicht, IP65, fuer GPS | 1 | 1,50 | 1,50 |
| Sensor-Kabel abgeschirmt | 4-adrig, AWG24, S/UTP, pro Meter | 3 | 1,50 | 4,50 |
| Ferritkern | Klappferrit, >100 Ohm @ 100MHz | 4 | 0,80 | 3,20 |
| Schrauben, Abstandshalter | M3/M2,5 Edelstahl, Federscheiben | — | — | 2,96 |
| Kabelbinder, Schrumpfschlauch | UV-beständig, Polyolefin | — | — | 2,40 |
| 18650-Akkuhalter | PCB-Mount, fuer 1x 18650 | 1 | 0,50 | 0,50 |
| **Zwischensumme Mechanik** | | | | **102,76** |

Die mechanische Sektion ist mit 102,76 EUR die groesste Kostenkategorie
(36,2% der BOM). Dies ist typisch fuer ein Motorrad-Projekt: die
Umweltanforderungen (IP67, Vibration, UV, Temperatur) erfordern
spezielle Materialien (ASA, Silikon, ePTFE, Conformal Coating) und
wasserdichte Steckverbinder. Das ASA-Filament allein kostet 25 EUR, die
Conformal Coating 12 EUR und die Kupferfolie fuer EMV-Abschirmung 8 EUR.
Bei Verwendung eines einfacheren Gehaeuses (PETG, ohne Coating, ohne
Kupferfolie) koennen ca. 40 EUR eingespart werden, allerdings auf
Kosten der Wasserdichtigkeit und EMV-Tauglichkeit.

#### 1.1.5 PCB-Herstellung

| Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) |
|---|---|---|---|---|
| PCB 4-Layer | 80x50mm, FR4, 1,6mm, ENIG, 2oz/1oz Kupfer | 5 (1 verwendet) | 6,80 | 6,80 |
| PCB-Bestueckung | SMD-Bestueckung, ~50 Bauteile | 1 | 20,00 | 20,00 |
| Loetpaste | Sn42/Bi57/Ag1, bleifrei, 50g Spritze | 1 | 8,00 | 8,00 |
| **Zwischensumme PCB** | | | | **34,80** |

Die PCB-Herstellung kostet 34,80 EUR. Der 4-Layer-PCB mit ENIG-
Oberflaeche ist fuer EMV-Stabilitaet in der Motorradumgebung (Zuendung,
Lichtmaschine) erforderlich. Die SMD-Bestueckung durch den externen
Dienstleister sichert maschinelle Praezision; THT-Bauteile (Steckverbinder,
Elkos) werden manuell nachgeloetet.

#### 1.1.6 Power-Management und aktive Bauelemente

| Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) |
|---|---|---|---|---|
| Buck-Converter 12V->5V | 1,5MHz, 3A, 4,5-28V Input, 90% Effizienz | 1 | 1,50 | 1,50 |
| LDO 5V->3,3V | 250mA, 30µVrms Rauschen, SOT-89 | 1 | 0,50 | 0,50 |
| Backup-Akku 18650 | Li-Ion, 3400mAh, 3,7V, geschuetzt | 1 | 6,00 | 6,00 |
| Akku-Lademodul | Mit Schutz, 5V Input, 1A Ladestrom | 1 | 0,50 | 0,50 |
| Boost-Converter 3,7V->5V | 2A, bis 28V Output, 17x11mm | 1 | 0,80 | 0,80 |
| Schottky-Diode | 3A, 40V, DO-201AD, Power-Path ORing | 2 | 0,10 | 0,20 |
| TVS-Diode | 15V Standoff, 24,4V Clamp, 400W, SMA | 2 | 0,50 | 1,00 |
| Sicherung 5A | Flachsicherung ATO/ATC, mit Halter | 1 | 1,00 | 1,00 |
| Elko 100µF | 25V, Low-ESR, SMD/THT | 1 | 0,30 | 0,30 |
| Gate-Driver | 1,5A, 2-Kanal, Non-Inverting, SOIC-8 | 1 | 1,50 | 1,50 |
| MOSFET | N-Channel, 55V, 25A, Logic-Level, TO-220 | 1 | 1,50 | 1,50 |
| Optokoppler | 1-Kanal, Transistor-Ausgang, DIP-4 | 2 | 0,50 | 1,00 |
| Freilaufdiode | 3A, 1000V, DO-201AD | 2 | 0,10 | 0,20 |
| Relais-Modul 4-Kanal | SRD-12V, 10A, mit Optokoppler | 1 | 4,00 | 4,00 |
| Passive Bauelemente | Widerstaende, Kondensatoren, Ferrite, LED | — | — | 2,55 |
| **Zwischensumme Power/aktiv** | | | | **23,55** |

Die Power-Management-Sektion kostet 23,55 EUR. Die Module (Buck, Boost,
Lader) sind guenstig (unter 3 EUR gesamt), aber der Backup-Akku (6 EUR)
und das Relais-Modul (4 EUR) sind die groessten Posten. Die
TVS-Dioden und die Sicherung schuetzen das System vor Ueberspannung
aus dem Bordnetz (Load-Dump, Spannungsspitzen durch Lichtmaschine).

#### 1.1.7 BOM-Gesamtuebersicht (Standard-Konfiguration)

| Kategorie | Kosten (EUR) | Anteil |
|---|---|---|
| MCU und Peripherie | 16,60 | 5,8% |
| Displays (TFT + E-Paper + Abdeckung) | 32,00 | 11,3% |
| Sensoren (GPS + IMU + Baro + Temp + Oeldruck + Hall + Bordspannung + Kraftstoff) | 72,20 | 25,4% |
| Mechanische Teile (Gehaeuse + Stecker + Kabel + Dichtungen) | 102,76 | 36,2% |
| PCB (Fertigung + Bestueckung + Loetpaste) | 34,80 | 12,3% |
| Power-Management und aktive Bauelemente | 23,55 | 8,3% |
| Verbrauchsmaterial | 5,00 | 1,8% |
| **BOM-Zwischensumme** | **284,01** | **100%** |
| Contingency +15% | 42,60 | — |
| **BOM Gesamt Standard** | **326,61** | — |

### 1.2 Werkzeug-Kosten

Werkzeug-Kosten fallen einmalig an und werden auf die Anzahl der
gebauten Geraete umgelegt. Bei einem Einzelstueck sind sie erheblich;
bei einer Kleinserie von 10 Stueck sinkt der anteilige Werkzeug-Kosten
auf ein Zehntel.

| Posten | Beschreibung | Kosten (EUR) | Bemerkung |
|---|---|---|---|
| Loetstation | Heissluft-Station mit Temperaturkontrolle | 40,00 | SMD-Loeten, Reflow |
| Loetpasten-Schablone | Edelstahl-Schablone fuer Custom PCB | 8,00 | Einmal pro PCB-Revision |
| Isopropanol-Reiniger | 1 Liter, fuer PCB-Reinigung vor Coating | 5,00 | Verbrauchsmaterial |
| Pinsel / Spruehdose | Fuer Conformal Coating-Auftrag | 3,00 | Verbrauchsmaterial |
| Multimeter | Digital, Spannungs- und Durchgangspruefung | 20,00 | Grundausstattung |
| Abisolierzange | Fuer Kabelverarbeitung | 10,00 | Einmalig |
| Crimpzangen-Set | Fuer wasserdichte Rastverbinder | 25,00 | Einmalig |
| SD-Kartenleser | USB, fuer Log-Auswertung | 5,00 | Einmalig |
| 3D-Drucker | FDM-Drucker fuer Gehaeuse | 200,00 | Falls nicht vorhanden |
| Schraubendreher-Set | Inbus 2,0/2,5/4,0mm | 8,00 | Einmalig |
| Heissluftpistole | Fuer Schrumpfschlauch | 15,00 | Einmalig |
| **Gesamt Tooling** | | **339,00** | |
| **Ohne 3D-Drucker** | | **139,00** | Falls bereits vorhanden |

Der 3D-Drucker ist mit 200 EUR der groesste Werkzeug-Posten. Falls er
bereits vorhanden ist (was bei vielen Bastlern der Fall ist), reduzieren
sich die Tooling-Kosten auf 139 EUR. Ein 3D-Drucker wird fuer das
ASA-Gehaeuse benoetigt; ohne ihn muss das Gehaeuse extern gefertigt
werden (Dienstleister: 30-60 EUR pro Gehaeuse).

### 1.3 Entwicklungszeit (falls fremdvergeben)

Die Entwicklung des Bordcomputers umfasst Hardware-Design,
Firmware-Entwicklung, Companion-App und Dokumentation. Bei
Fremdvergabe an einen externen Dienstleister waeren folgende
Stundensaetze und Aufwaende anzusetzen:

| Aufgabe | Aufwand (Stunden) | Stundensatz (EUR/h) | Kosten (EUR) |
|---|---|---|---|
| PCB-Design (Schematic + Layout) | 20-30 | 60-80 | 1.200-2.400 |
| Firmware-Entwicklung (Treiber, BLE, OTA) | 60-80 | 60-80 | 3.600-6.400 |
| Companion-App (React Native, BLE) | 30-50 | 60-80 | 1.800-4.000 |
| Gehaeuse-Design (3D-Modellierung) | 8-12 | 50-70 | 400-840 |
| Dokumentation und Test | 15-20 | 50-70 | 750-1.400 |
| **Gesamt extern** | **133-192 h** | | **7.750-15.040** |

Bei Eigenleistung (DIY) entfallen diese Kosten. Die im Rahmen dieses
Projekts erstellten Dokumente (Research, Design-Konzepte, Firmware-
Skeleton) repraesentieren bereits einen erheblichen Teil dieser
Entwicklungsleistung und stehen als Open-Source zur Verfuegung.
Die verbleibende Implementierungszeit fuer den Eigenbau wird auf
40-60 Stunden geschaetzt (Loeten, Zusammenbau, Firmware-Flash,
Inbetriebnahme, Test).

---

## 2. Wiederkehrende Kosten

Wiederkehrende Kosten fallen waehrend des Betriebs des Bordcomputers an.
Sie sind im Vergleich zu den einmaligen Kosten gering, muessen aber fuer
eine vollstaendige TCO-Betrachtung erfasst werden.

### 2.1 Stromverbrauch

Der Bordcomputer wird aus dem 12V-Bordnetz des Motorrads versorgt. Der
Stromverbrauch wird vollstaendig vom Bordnetz gedeckt — es entstehen
keine zusaetzlichen Stromkosten.

Typischer Stromverbrauch des Systems:

| Betriebszustand | Stromaufnahme (mA) @ 12V | Leistung (W) | Dauer pro Fahrt (h) |
|---|---|---|---|
| Aktiv (Display + GPS + BLE + Sensoren) | 150-200 | 1,8-2,4 | 2-4 |
| Deep-Sleep (Motor aus, GPS-Logging) | 5-10 | 0,06-0,12 | 12-20 |
| Display-Off (Motor laeuft, Display aus) | 80-120 | 1,0-1,4 | — |

Bei einer typischen Saison von 5.000 km und einer durchschnittlichen
Geschwindigkeit von 60 km/h ergibt sich eine Fahrzeit von ca. 83 Stunden.
Bei 2,4 W maximaler Leistung entspricht das einem Energieverbrauch von
ca. 0,2 kWh pro Saison. Bei einem Strompreis von 0,35 EUR/kWh entstehen
Stromkosten von **0,07 EUR pro Saison** — vernachlaessigbar.

Der Backup-Akku (18650, 3400 mAh) wird aus dem Bordnetz geladen. Bei
einer Akkulebensdauer von 3-5 Jahren (typisch fuer Li-Ion bei
regelmassiger Teillast) fallen Austauschkosten von 6 EUR alle 3-5
Jahre an (siehe Abschnitt 2.3).

### 2.2 SD-Karten-Verschleiss

Die SD-Karte ist das einzige Verschleissteil mit signifikanter
Belastung. Der Bordcomputer schreibt kontinuierlich Sensor- und
GPS-Daten auf die SD-Karte:

| Datenquelle | Schreibrate | Dauer pro Fahrt | Daten pro Fahrt |
|---|---|---|---|
| GPS-Track (GPX) | 5 KB/s (1 Hz, 10 Felder) | 2-4 h | 36-72 MB |
| Sensor-Log (CSV) | 2 KB/s (10 Hz, 8 Werte) | 2-4 h | 14-29 MB |
| Diagnose-Log (DTC) | sporadisch | 2-4 h | < 1 MB |
| **Gesamt pro Fahrt** | | | **50-100 MB** |

Bei 83 Fahrten pro Saison (5.000 km / 60 km/h) entstehen ca. 4-8 GB
Daten pro Saison. Eine 16GB-Karte hat eine typische Lebensdauer von
10.000 Schreibzyklen (bei Wear-Levelung). Bei 8 GB pro Saison und
einer Karte mit 14 GB nutzbarem Speicher (Fat32 + Reserve) ergibt das
ca. 1.750 Zyklen pro Saison — die Karte haelt theoretisch 5-6 Jahre.

In der Praxis ist die Lebensdauer kuerzer, da:
- Billige SD-Karten oft nur 500-1.000 Zyklen erreichen
- Temperaturschwankungen (-20 bis +70 Grad C im Motorrad) die
  Lebensdauer reduzieren
- Spannungsspitzen aus dem Bordnetz den Controller beschaedigen koennen

**Ersatz-Kosten:** 6,00 EUR fuer eine 16GB-Karte, alle 2-3 Jahre.
Ueber 5 Jahre: 2 Austausche = 12,00 EUR.

### 2.3 Akku CR2032 fuer RTC

Der ESP32-S3 hat eine interne RTC, die ueber den 18650-Backup-Akku
gepuffert wird. Es ist keine zusaetzliche CR2032 noetig. Der 18650-Akku
selbst hat eine Lebensdauer von 3-5 Jahren bei regelmaessiger Nutzung
(Teilladung, keine Tiefentladung dank Schutzschaltung).

**Ersatz-Kosten:** 6,00 EUR alle 3-5 Jahre. Ueber 5 Jahre: 1 Austausch
= 6,00 EUR.

### 2.4 Wartung: Firmware-Updates

Firmware-Updates werden ueber OTA (Over-The-Air) via BLE oder USB-C
durchgefuehrt. Sie sind kostenlos und erfordern keinen Werkstattbesuch.
Der Zeitaufwand fuer den Nutzer betraegt 5-15 Minuten pro Update.

Fuer den Entwickler/Betreiber fallen Kosten fuer die Wartung der
Firmware-Infrastruktur an (Git-Repository, Build-Server, Release-
Management). Diese sind bei einem Open-Source-Projekt mit
Community-Beitraegen marginal und werden hier nicht weiter
betrachtet.

### 2.5 Versicherung

Der Einbau eines nachgeruesteten Bordcomputers hat keine Auswirkung
auf die Kfz-Versicherung oder Garantie des Motorrads. Das Geraet wird
als Zubehoer behandelt (aehnlich wie ein Navi oder Smartphone-Halterung).
Eine separate Versicherung fuer das Geraet ist nicht ueblich; bei
Diebstahl oder Beschaeidigung greift gegebenenfalls die
Hausratversicherung oder eine Geraeteversicherung.

### 2.6 Zusammenfassung wiederkehrende Kosten (5 Jahre)

| Kostenart | Jaehrlich (EUR) | Ueber 5 Jahre (EUR) |
|---|---|---|
| Strom | 0,07 | 0,35 |
| SD-Karte (alle 2-3 J.) | 2,00-3,00 | 12,00 |
| Backup-Akku (alle 3-5 J.) | 1,20-2,00 | 6,00 |
| Firmware-Wartung | 0,00 | 0,00 |
| Versicherung | 0,00 | 0,00 |
| **Gesamt wiederkehrend** | **3,27-5,07** | **18,35** |

Die wiederkehrenden Kosten sind mit unter 20 EUR ueber 5 Jahre
vernachlaessigbar gering.

---

## 3. Kosten-Szenarien

Drei Szenarien werden verglichen, die unterschiedliche Ausbaustufen
und Budgets abdecken. Jedes Szenario baut auf dem vorherigen auf.

### 3.1 Szenario: Minimal

Ein funktionierendes Kernsystem mit den wichtigsten Grundfunktionen:
Geschwindigkeit, Drehzahl, Kuehlmitteltemperatur, TFT-Display,
SD-Karten-Logging. Kein E-Paper, kein GPS mit 4-Konstellation,
kein BLE-Datenexport, kein Backup-Akku, keine Relais-Steuerung,
keine Oeldruckmessung. DevKitC-1 auf Perfboard, PETG-Gehaeuse.

| Kategorie | Kosten (EUR) |
|---|---|
| MCU (DevKitC-1, ILI9341 2,4", Budget-GPS, SD) | 19,00 |
| Sensoren (Budget-IMU, Budget-Baro, NTC, Optokoppler) | 7,00 |
| Power (Buck-Modul, LDO) | 2,00 |
| Gehaeuse (PETG, NBR, PG7, U-Bolt, Sicherung) | 9,00 |
| Mechanik (Perfboard, Jumper-Wire, Verbrauchsmaterial) | 10,00 |
| Filament PETG 500g | 12,00 |
| Kabelverschraubungen PG7 | 3,20 |
| Sonstiges | 7,65 |
| **BOM Zwischensumme** | **69,85** |
| Contingency +15% | 10,48 |
| **Gesamt Minimal** | **80,33** |

Einschraenkungen: Nicht wasserdicht (PETG-Gehaeuse ohne Conformal
Coating), nicht vibrationssicher (Perfboard + Steckverbindungen),
keine Smartphone-Anbindung, kein Backup bei Power-Loss, kein
E-Paper-Display. Geeignet fuer Software-Entwicklung und
Funktionstest auf dem Schreibtisch, nicht fuer Dauereinsatz am
Motorrad.

### 3.2 Szenario: Standard

Vollwertiges Bordcomputer-System mit allen Must- und Should-Features:
BLE-Anbindung, Smartphone-Integration, TFT-Display 3,5" IPS,
E-Paper-Zweitdisplay, GPS mit 4-Konstellation, Backup-Akku, Oeldruck,
Relais-Steuerung, PWM-Heizgriffe. Custom PCB (4-Layer, ENIG),
ASA-Gehaeuse mit Conformal Coating.

| Kategorie | Kosten (EUR) |
|---|---|
| MCU und Peripherie | 16,60 |
| Displays (TFT + E-Paper + Abdeckung) | 32,00 |
| Sensoren (alle) | 72,20 |
| Power-Management und aktive Bauelemente | 23,55 |
| Passive Bauelemente | 2,55 |
| PCB (Fertigung + Bestueckung + Loetpaste) | 34,80 |
| Gehaeuse (Filament + Dichtungen + Coating) | 62,00 |
| Kabel + Stecker + Ferritkerne | 18,10 |
| Mechanik (Schrauben, Abstandshalter, Kabelbinder) | 5,36 |
| Verbrauchsmaterial | 5,00 |
| Werkzeug (anteilig, ohne 3D-Drucker) | 0,00 |
| Reserve-Bauteile (Paket) | 0,00 |
| **BOM Zwischensumme** | **272,16** |
| Versandkosten (alle Lieferanten) | 11,85 |
| **BOM + Versand** | **284,01** |
| Contingency +15% | 42,60 |
| **Gesamt Standard** | **326,61** |

### 3.3 Szenario: Premium

Alles aus Standard-Konfiguration plus: TPMS-Sensoren, LED-Status-Strip,
Hall-Sensor am Vorderrad, zusaetzliche Sensoren, professionelle
Gehaeuse-Fertigung, Premium-Komponenten (Edelstahlschrauben,
zusaetzliche Sicherungen).

| Zusatz-Bauteil | Spezifikation | Kosten (EUR) |
|---|---|---|
| TPMS-Sensoren | Reifendruck-/Temperatursensor, BLE, Paar | 25,00 |
| LED-Status-Strip | Adressierbar, 4 LEDs, RGB | 3,00 |
| Hall-Sensor + Magnet | Unipolar + Neodym 5mm | 2,00 |
| E-Paper-Display 2,9" | 296x128, zusaetzlich zu 2,13" | 15,00 |
| Umwelt-Temperatursensor | Digital, 1-Wire, extern | 3,00 |
| ATO Sicherungskasten | 6-fach, mit Deckel | 5,00 |
| Edelstahl-Schrauben | M3/M2,5 Sortiment | 3,00 |
| Kugelgelenk-Halterung | Neigungsverstellbar, 22mm | 8,00 |
| Reserve-Bauteile | 10% der kritischen Bauteile | 15,00 |
| **Zusatz-Summe** | | **79,00** |
| Contingency +15% | | 11,85 |
| **Gesamt Premium (inkl. Standard)** | | **417,46** |

### 3.4 Vergleichstabelle der Szenarien

| Kennzahl | Minimal | Standard | Premium |
|---|---|---|---|
| BOM-Kosten (EUR) | 69,85 | 284,01 | 363,01 |
| Contingency +15% (EUR) | 10,48 | 42,60 | 54,45 |
| **Gesamt BOM (EUR)** | **80,33** | **326,61** | **417,46** |
| Werkzeug (EUR, ohne 3D-Drucker) | 139,00 | 139,00 | 139,00 |
| Werkzeug (EUR, mit 3D-Drucker) | 339,00 | 339,00 | 339,00 |
| Reserve-Bauteile (EUR) | 0,00 | 56,60 | 56,60 |
| **TCO Jahr 1 (mit 3D-Drucker)** | **419,33** | **665,61** | **756,46** |
| **TCO Jahr 1 (ohne 3D-Drucker)** | **219,33** | **465,61** | **556,46** |
| **TCO Jahr 5 (ohne 3D-Drucker)** | **237,68** | **483,96** | **574,81** |
| Features abgedeckt | Must (teilw.) | Must + Should | Must + Should + Could |
| Wasserdicht | Nein (IP54) | Ja (IP67) | Ja (IP67) |
| Dauereinsatz tauglich | Nein | Ja | Ja |
| GPS-Genauigkeit | 5-10m (2 Konst.) | 2m (4 Konst.) | 2m (4 Konst.) |
| Display-Anzahl | 1 (TFT) | 2 (TFT + E-Paper) | 3 (TFT + 2x E-Paper) |
| Smartphone-Anbindung | Nein | Ja (BLE) | Ja (BLE) |
| Backup bei Power-Loss | Nein | Ja (18650) | Ja (18650) |
| Relais-Steuerung | Nein | Ja (4-Kanal) | Ja (4-Kanal) |

---

## 4. Kosten-Nutzen-Analyse

### 4.1 ROI: Welche Funktionen ersetzen teurere kommerzielle Loesungen?

Der Bordcomputer ersetzt oder ergaenzt mehrere kommerzielle Produkte,
die fuer Motorradfahrer typischerweise separat beschafft werden:

| Funktion | Kommerzielle Loesung | Preis kommerziell (EUR) | Bordcomputer | Mehrpreis (EUR) |
|---|---|---|---|---|
| GPS-Navigation / Track-Logging | Motorrad-Navi (IPX7) | 250-500 | Enthalten | 0 |
| OBD2 / Fahrzeugdiagnose | OBD2-Adapter + App | 30-80 | Enthalten (adaptiert) | 0 |
| Reifendruckkontrolle (TPMS) | Externe TPMS-Displays | 40-120 | Enthalten (Premium) | 25 |
| Bordspannungs-Anzeige | Separate Voltmeter | 15-30 | Enthalten | 0 |
| Oeldruck-Anzeige | Separate Oeldruckmeter | 40-80 | Enthalten | 25 |
| Kuehlmitteltemp. (zusaetzl.) | Separate Temp-Anzeige | 20-40 | Enthalten | 3 |
| Neigung/Lean-Angle-Recording | separate Action-Cam-App | 50-100 | Enthalten | 5 |
| Heizgriff-Steuerung | Integrierte Heizgriffe | 60-120 | Enthalten | 4 |
| **Kommerzieller Gegenwert** | | **505-970** | | |
| **Bordcomputer-Kosten (Standard)** | | | **327** | |
| **Einsparung** | | | **178-643** | |

Der ROI ist eindeutig positiv: die Standard-Konfiguration kostet 327 EUR
und ersetzt kommerzielle Loesungen im Wert von 505-970 EUR. Das
entspricht einer Einsparung von 38% bis 66% gegenueber kommerziellen
Einzelprodukten.

### 4.2 Vergleich mit kommerziellen Motorrad-Bordcomputern

Kommerzielle Motorrad-Bordcomputer / Navigationsgeraete liegen in
folgenden Preisbereichen (Stand 2026):

| Produktkategorie | Preisbereich (EUR) | Funktionsumfang |
|---|---|---|
| Einfaches Tacho-Display | 50-100 | Geschw., RPM, Temp, Uhrzeit |
| Motorrad-Navi (Mittelklasse) | 250-400 | GPS-Navi, Track, Wetter |
| Motorrad-Navi (Premium) | 500-800 | GPS, BLE, Wetter, Musik, Anrufe |
| Vollausbau-Display-System | 800-1.500 | Alles integriert, TFT-Touchscreen |
| Adventure-Display-System | 1.200-2.000 | Offroad-Modi, Topo-Karten, TPMS |

Der Transalp Bordcomputer (Standard: 327 EUR, Premium: 417 EUR) bietet
einen Funktionsumfang, der zwischen Mittelklasse-Navi und
Vollausbau-Display-System liegt, zu einem Bruchteil des Preises.
Zusaetzlich bietet er Funktionen, die kommerzielle Geraete nicht haben:
Oeldruck-Anzeige, OBD2-Diagnose, BLE-Smartphone-Integration mit
Custom-App, DIY-Reparierbarkeit und Open-Source-Firmware.

### 4.3 Mehrwert durch Diagnose-Funktion

Die Diagnose-Funktion (DTC-Auslesung, Self-Test, Fehlerspeicher)
bietet einen erheblichen Mehrwert durch Werkstatt-Kosten-Einsparung:

| Diagnose-Art | Werkstatt-Kosten (EUR) | Bordcomputer | Einsparung (EUR) |
|---|---|---|---|
| Fehlerspeicher auslesen | 20-40 | Kostenlos | 20-40 |
| Oeldruck pruefen | 30-60 | Kostenlos (Echtzeit) | 30-60 |
| Kuehlmitteltemp. pruefen | 20-40 | Kostenlos (Echtzeit) | 20-40 |
| Bordspannung pruefen | 15-30 | Kostenlos (Echtzeit) | 15-30 |
| Sensor-Werte live auslesen | 30-50 | Kostenlos | 30-50 |
| **Pro Werkstattbesuch** | | | **115-220** |

Bei 2 Werkstattbesuchen pro Saison (Inspektion + gelegentliche
Diagnose) ergibt sich eine jaehrliche Einsparung von 230-440 EUR.
Ueber 5 Jahre: 1.150-2.200 EUR. Das uebersteigt die Anschaffungskosten
des Bordcomputers um ein Vielfaches.

### 4.4 Mehrwert durch GPS-Logging

Die GPS-Track-Aufzeichnung bietet folgenden Mehrwert:

| Funktion | Kommerzieller Gegenwert (EUR) | Bordcomputer |
|---|---|---|
| Track-Recording (GPX-Export) | 30-50 (App-Abos ueber 5 J.) | Kostenlos |
| Routen-Planung (Companion-App) | 20-40 (App) | Kostenlos |
| Statistik (km, h, max. Geschw.) | 10-20 (App) | Kostenlos |
| **Gesamt-Mehrwert** | **60-110** | Kostenlos |

Zusaetzlich bietet das GPS-Logging Sicherheitsfunktionen:
- Crash-Erkennung mit automatischer SOS-Positionierung (ueber Companion-App)
- Letzte bekannte Position bei Notfall
- Fahrtenbuch-Funktion (Steuerlich absetzbar)

---

## 5. Skalierungs-Kosten

Die Kosten pro Geraet sinken signifikant bei hoeherer Stueckzahl.
Dies gilt fuer Bauteile (Mengenrabatte), PCB (Panelisierung) und
Versand (Verteilung auf mehrere Geraete).

### 5.1 Stueckkosten-Reduktion bei Kleinserie (1, 5, 10, 25 Stueck)

| Variante | 1 Stueck | 5 Stueck | 10 Stueck | 25 Stueck |
|---|---|---|---|---|
| **Minimal** | | | | |
| BOM pro Geraet | 69,85 | 55,88 | 48,90 | 42,40 |
| Versand pro Geraet | 15,00 | 5,00 | 3,50 | 2,20 |
| PCB/Perfboard | 2,00 | 1,60 | 1,40 | 1,10 |
| Gesamt pro Geraet | 86,85 | 62,48 | 53,80 | 45,70 |
| Contingency +15% | 13,03 | 9,37 | 8,07 | 6,86 |
| **Gesamt pro Geraet** | **99,88** | **71,85** | **61,87** | **52,56** |
| **Standard** | | | | |
| BOM pro Geraet | 284,01 | 213,01 | 177,51 | 148,50 |
| PCB (anteilig) | 26,80 | 6,80 | 5,36 | 3,40 |
| Bestueckung | 20,00 | 16,00 | 14,00 | 11,00 |
| Versand pro Geraet | 25,00 | 8,00 | 5,50 | 3,20 |
| Gesamt pro Geraet | 355,81 | 243,81 | 202,37 | 166,10 |
| Contingency +15% | 53,37 | 36,57 | 30,36 | 24,92 |
| **Gesamt pro Geraet** | **409,18** | **280,38** | **232,73** | **191,02** |
| **Premium** | | | | |
| BOM pro Geraet | 363,01 | 272,26 | 226,88 | 185,20 |
| PCB (anteilig) | 26,80 | 6,80 | 5,36 | 3,40 |
| Bestueckung | 20,00 | 16,00 | 14,00 | 11,00 |
| Versand pro Geraet | 30,00 | 9,00 | 6,00 | 3,50 |
| Gesamt pro Geraet | 439,81 | 302,06 | 252,24 | 202,10 |
| Contingency +15% | 65,97 | 45,31 | 37,84 | 30,32 |
| **Gesamt pro Geraet** | **505,78** | **347,37** | **290,08** | **232,42** |

### 5.2 PCB-Panelisierung: Kosten pro cm² bei hoeherer Stueckzahl

Das PCB hat eine Groesse von 80 x 50 mm = 4.000 mm² = 40 cm². Bei
einem Standard-Panel von 100 x 100 mm (PCB-Fertiger Standard) passen 4
Boards pro Panel (mit Rails).

| Stueckzahl | Boards pro Panel | Panels | PCB-Kosten/Board (EUR) | Kosten/cm² (EUR) |
|---|---|---|---|---|
| 5 (1 Panel) | 5 (mit Panel-Rails) | 1 | 6,80 | 0,170 |
| 10 (2 Panels) | 5 | 2 | 4,50 | 0,113 |
| 25 (5 Panels) | 5 | 5 | 2,80 | 0,070 |
| 50 (Panel-Optimiert) | 8 (optimiert) | 7 | 1,90 | 0,048 |
| 100 (Panel-Optimiert) | 8 | 13 | 1,30 | 0,033 |

Bei 100 Stueck sinkt der PCB-Preis auf 1,30 EUR pro Board (0,033 EUR/cm²).
Das ist eine Reduktion von 81% gegenueber dem Einzelstueck (6,80 EUR).

### 5.3 Gemeinkosten-Anteil

Gemeinkosten (Werkzeug, Versand-Pauschalen, Entwicklungszeit) sinken
pro Geraet bei steigender Stueckzahl:

| Gemeinkosten | 1 Stueck | 5 Stueck | 10 Stueck | 25 Stueck |
|---|---|---|---|---|
| Werkzeug (mit 3D-Drucker) | 339,00 | 67,80 | 33,90 | 13,56 |
| Werkzeug (ohne 3D-Drucker) | 139,00 | 27,80 | 13,90 | 5,56 |
| Reserve-Bauteile | 56,60 | 11,32 | 5,66 | 2,26 |
| Versand (alle Lieferanten) | 41,75 | 8,35 | 4,18 | 1,67 |
| **Gemeinkosten gesamt** | **437,35** | **47,47** | **23,74** | **9,49** |

Bei 25 Stueck betragen die Gemeinkosten nur noch 9,49 EUR pro Geraet
(ohne 3D-Drucker), was vernachlaessigbar ist gegenueber den BOM-Kosten
von ca. 148 EUR (Standard-Variante).

### 5.4 Skalierungs-Szenario: 5, 10, 25 Stueck Gesamt-TCO

| Szenario | Stueckzahl | Kosten pro Geraet (EUR) | Gesamtkosten (EUR) |
|---|---|---|---|
| Standard (ohne 3D-Drucker) | 1 | 465,61 | 465,61 |
| Standard (ohne 3D-Drucker) | 5 | 307,75 | 1.538,76 |
| Standard (ohne 3D-Drucker) | 10 | 256,62 | 2.566,20 |
| Standard (ohne 3D-Drucker) | 25 | 200,51 | 5.012,76 |
| Premium (ohne 3D-Drucker) | 1 | 556,46 | 556,46 |
| Premium (ohne 3D-Drucker) | 5 | 374,76 | 1.873,80 |
| Premium (ohne 3D-Drucker) | 10 | 313,98 | 3.139,80 |
| Premium (ohne 3D-Drucker) | 25 | 241,82 | 6.045,60 |

---

## 6. Wartungs- und Lebenszyklus-Kosten

### 6.1 Erwartete Lebensdauer pro Komponente

| Komponente | Erwartete Lebensdauer (Jahre) | Ausfallursache | Ersatzkosten (EUR) |
|---|---|---|---|
| ESP32-S3-WROOM-1 | 8-10 | Flash-Verschleiss, Ueberspannung | 5,40 |
| TFT-Display 3,5" | 5-8 | Backlight-Ausfall, Pixel-Ausfall | 18,00 |
| E-Paper-Display 2,13" | 7-10 | Glas-Substrat-Riss (Vibration) | 12,00 |
| GPS-Modul | 8-10 | Antennen-Beschädigung | 24,60 |
| IMU / Barometer | 8-10 | Loeftverbindung (Vibration) | 8,50 |
| SD-Karte | 2-3 | Schreibzyklen, Temperatur | 6,00 |
| Backup-Akku 18650 | 3-5 | Kapazitaetsverlust (Li-Ion) | 6,00 |
| Conformal Coating | 5-7 | Abrieb, Risse, Alterung | 12,00 |
| ASA-Gehaeuse | 8-10 | UV-Alterung, mechanisch | 25,00 |
| Silikon-O-Ring | 3-5 | UV-Alterung, Kompression | 2,00 |
| Kabelverschraubungen | 5-7 | Dichtungsversagen | 5,20 |
| Relais-Modul | 5-8 | Kontaktbrand (Schaltzyklen) | 4,00 |
| MOSFET / Gate-Driver | 8-10 | Thermische Belastung | 3,00 |
| PCB (Leiterbahnen) | 10+ | Mechanisch, Korrosion | 34,80 |
| Optokoppler | 8-10 | LED-Degradation (CTR-Verlust) | 1,00 |

### 6.2 Ersatzteil-Kosten ueber 5 Jahre

Unter der Annahme, dass die typischen Verschleissteile innerhalb von
5 Jahren ausgetauscht werden muessen:

| Komponente | Austausch-Zyklus | Anzahl in 5 J. | Kosten (EUR) |
|---|---|---|---|
| SD-Karte 16GB | Alle 2-3 Jahre | 2 | 12,00 |
| Backup-Akku 18650 | Alle 3-5 Jahre | 1 | 6,00 |
| Silikon-O-Ring | Alle 3-5 Jahre | 1 | 2,00 |
| EPDM-Schaumstoff | Alle 3-5 Jahre | 1 | 3,00 |
| Reserve-Bauteile (Paket) | Variabel | — | 56,60 |
| **Gesamt Ersatzteile (5 J.)** | | | **79,60** |

### 6.3 Firmware-Wartung: Zeitbedarf

| Aufgabe | Aufwand pro Jahr (Stunden) | Ueber 5 Jahre (Stunden) |
|---|---|---|
| Bugfixes und Sicherheitsupdates | 5-10 | 25-50 |
| Neue Features (Custom-Wuensche) | 10-20 | 50-100 |
| Komponenten-Austausch (Treiber) | 2-5 | 10-25 |
| Companion-App-Updates | 5-10 | 25-50 |
| Build- und Release-Management | 2-3 | 10-15 |
| **Gesamt** | **24-48** | **120-240** |

Bei einem internen Stundensatz von 40-60 EUR/h ergeben sich
Firmware-Wartungskosten von 4.800-14.400 EUR ueber 5 Jahre. Bei
einem Open-Source-Projekt mit Community-Beitraegen liegt der
eigentliche Aufwand fuer den Hauptentwickler deutlich niedriger
(20-40 Stunden/Jahr = 4.000-12.000 EUR ueber 5 Jahre).

### 6.4 SD-Karte: Ersatz alle 2-3 Jahre

Die SD-Karte ist das Verschleissteil mit der hoechsten
Austauschfrequenz. Begruendung:

- Schreibrate: 50-100 MB pro Fahrt, 4-8 GB pro Saison
- Schreibzyklen: Bei 10.000 Zyklen (Qualitaetskarte) und 14 GB
  nutzbarem Speicher: ca. 1.750 Zyklen/Saison → 5-6 Jahre theoretisch
- Realitaet: 2-3 Jahre wegen Temperaturschwankungen, Spannungsspitzen
  und Qualitaetsunterschieden
- Kosten: 6,00 EUR pro Austausch

Empfehlung: Hochwertige SD-Karte mit MLC-NAND (10.000+ Zyklen)
verwenden. Wear-Leveling wird vom SD-Controller uebernommen.
Zusaetzlich: Log-Rotation in der Firmware implementieren, um
Schreibzyklen zu minimieren (z.B. nur jede 10. Sekunde loggen statt
jede Sekunde).

---

## 7. Risiko-Kosten

### 7.1 Display-Ausfall

| Risiko | Wahrscheinlichkeit | Kosten (EUR) | Massnahme |
|---|---|---|---|
| TFT-Backlight-Ausfall | Mittel (5-10% in 5 J.) | 18,00 | Display tauschen (15 min) |
| TFT-Pixel-Ausfall | Gering (< 2%) | 18,00 | Display tauschen (15 min) |
| E-Paper-Glas-Riss (Vibration) | Mittel (5-10% in 5 J.) | 12,00 | Display tauschen (20 min) |
| FPC-Kabel-Bruch | Gering (< 5%) | 0,50 | Kabel tauschen (10 min) |
| Acrylglas-Abdeckung Riss | Mittel (5-10%) | 2,00 | Abdeckung tauschen (5 min) |

Erwarteter Schaden (Wahrscheinlichkeit x Kosten):
- TFT-Ausfall: 7,5% x 18 = 1,35 EUR
- E-Paper-Riss: 7,5% x 12 = 0,90 EUR
- FPC-Kabel: 2,5% x 0,50 = 0,01 EUR
- Abdeckung: 7,5% x 2 = 0,15 EUR
- **Erwartete Risiko-Kosten Displays (5 J.): 2,41 EUR**

### 7.2 PCB-Defekt

| Risiko | Wahrscheinlichkeit | Kosten (EUR) | Massnahme |
|---|---|---|---|
| Loetverbindung-Vibration | Mittel (5-10%) | 34,80 (PCB) + 20,00 (Bestueckung) | PCB neu bestuecken |
| ESP32-S3 Ausfall (Ueberspannung) | Gering (2-5%) | 5,40 | Modul nachloeten |
| PCB-Leiterbahn-Schaden | Sehr gering (< 1%) | 54,80 | Komplettes PCB tauschen |
| Conformal Coating-Versagen | Gering (< 5%) | 12,00 | Neu beschichten |

Erwarteter Schaden:
- Loetverbindung: 7,5% x 54,80 = 4,11 EUR
- ESP32-Ausfall: 3,5% x 5,40 = 0,19 EUR
- Leiterbahn: 0,5% x 54,80 = 0,27 EUR
- Coating: 2,5% x 12 = 0,30 EUR
- **Erwartete Risiko-Kosten PCB (5 J.): 4,87 EUR**

### 7.3 Wasserschaden

| Risiko | Wahrscheinlichkeit | Kosten (EUR) | Massnahme |
|---|---|---|---|
| Dichtungs-Versagen (O-Ring) | Mittel (10-15% in 5 J.) | 2,00 (O-Ring) + evt. 54,80 (PCB) | Dichtung + PCB tauschen |
| Cable-Gland-Versagen | Gering (5%) | 5,20 (Glands) + evt. 54,80 (PCB) | Glands + PCB tauschen |
| USB-C-Buchse-Wassereintritt | Gering (3%) | 3,50 (Buchse) + evt. 54,80 (PCB) | Buchse + PCB tauschen |
| Kondensat-Korrosion | Mittel (5-10%) | 12,00 (Coating) + evt. 54,80 (PCB) | Neu beschichten |
| Voll-Wasserschaden (Furtdurchfahrt) | Sehr gering (< 2%) | 284,01 (komplette BOM) | Kompletter Neuaufbau |

Erwarteter Schaden:
- O-Ring-Versagen: 12,5% x 56,80 = 7,10 EUR
- Cable-Gland: 2,5% x 60,00 = 1,50 EUR
- USB-C: 1,5% x 58,30 = 0,87 EUR
- Kondensat: 7,5% x 66,80 = 5,01 EUR
- Vollschaden: 1% x 284,01 = 2,84 EUR
- **Erwartete Risiko-Kosten Wasserschaden (5 J.): 17,32 EUR**

### 7.4 Zusammenfassung Risiko-Kosten

| Risiko-Kategorie | Erwartete Kosten (5 J., EUR) |
|---|---|
| Display-Ausfall | 2,41 |
| PCB-Defekt | 4,87 |
| Wasserschaden | 17,32 |
| Sonstiges (GPS, Sensoren, Akku) | 5,00 |
| **Gesamt erwartete Risiko-Kosten** | **29,60** |

Die Risiko-Kosten sind mit knapp 30 EUR ueber 5 Jahren sehr gering.
Die Conformal Coating und das IP67-Gehaeuse bieten einen effektiven
Schutz. Die groesste einzelne Risiko-Kostenposition ist der
Wasserschaden (17,32 EUR), was die Wichtigkeit der regelmaessigen
Dichtungs-Wartung unterstreicht.

### 7.5 Versicherung / Eigenleistung-Abwaegung

| Option | Kosten | Deckung | Empfehlung |
|---|---|---|---|
| Keine Versicherung | 0 EUR | Keine Deckung bei Diebstahl/Beschädigung | Nur bei geringem Restwert |
| Hausratversicherung (Zusatz) | 10-20 EUR/Jahr | Deckt Diebstahl und Beschädigung | Empfohlen bei Premium-Konfiguration |
| Geraeteversicherung | 30-50 EUR/Jahr | Volldeckung inkl. Wasserschaden | Nur bei kommerzieller Nutzung |
| Eigenleistung (Reserve-Bauteile) | 56,60 EUR einmalig | Selbst-Reparatur bei Ausfall | Empfohlen fuer alle Konfigurationen |

Empfehlung: Eigenleistung mit Reserve-Bauteilen (56,60 EUR) als
Standard-Strategie. Zusaetzlich Hausratversicherung-Zusatz fuer
die Premium-Konfiguration (417 EUR Materialwert).

---

## 8. Zusammenfassung

### 8.1 TCO-Gesamtuebersicht (5 Jahre, Standard-Konfiguration)

| Kostenart | Betrag (EUR) | Anteil |
|---|---|---|
| BOM (einmalig) | 326,61 | 62,1% |
| Werkzeug (einmalig, ohne 3D-Drucker) | 139,00 | 26,4% |
| Reserve-Bauteile (einmalig) | 56,60 | 10,8% |
| Wiederkehrende Kosten (5 Jahre) | 18,35 | 3,5% |
| Risiko-Kosten (erwartet, 5 Jahre) | 29,60 | 5,6% |
| **TCO Gesamt (5 Jahre, ohne 3D-Drucker)** | **570,16** | **100%** |

Bei vorhandenem 3D-Drucker erhoeht sich die TCO um 200 EUR
(3D-Drucker-Kosten) auf 770,16 EUR. Bei einer Kleinserie von 5
Stueck verringert sich die TCO pro Geraet auf ca. 350-400 EUR.

### 8.2 TCO im Vergleich

| Szenario | TCO 5 Jahre (EUR) | Kommerzieller Gegenwert (EUR) | ROI |
|---|---|---|---|
| Minimal | 379,33 | 200-300 | -26% bis +26% |
| Standard | 570,16 | 505-970 | -13% bis +70% |
| Premium | 661,01 | 700-1.500 | +6% bis +127% |

Die Standard- und Premium-Konfigurationen bieten einen positiven ROI
ueber 5 Jahre. Die Minimal-Konfiguration ist nur positiv, wenn man den
Werkstatt-Einsparungen (230-440 EUR/Jahr) einbezieht:

| Szenario | TCO 5 J. | Werkstatt-Einsparung 5 J. | Netto-Kosten 5 J. |
|---|---|---|---|
| Minimal | 379,33 | 1.150-2.200 | -770 bis -1.821 |
| Standard | 570,16 | 1.150-2.200 | -580 bis -1.630 |
| Premium | 661,01 | 1.150-2.200 | -489 bis -1.539 |

Unter Einbeziehung der Werkstatt-Einsparungen ist der Bordcomputer in
allen Konfigurationen ueber 5 Jahre kostenlos oder sogar profitabel.

### 8.3 Empfehlung

Die **Standard-Konfiguration** bietet das beste Preis-Leistungs-
Verhaeltnis:

- BOM: 326,61 EUR (einschliesslich 15% Contingency)
- Werkzeug: 139,00 EUR (ohne 3D-Drucker, falls vorhanden)
- TCO 5 Jahre: 570,16 EUR
- Ersetzt kommerzielle Produkte: 505-970 EUR
- Werkstatt-Einsparung 5 Jahre: 1.150-2.200 EUR
- **Netto-Gewinn nach 5 Jahren: 580-1.630 EUR**

Die Standard-Konfiguration deckt alle Must- und Should-Features ab,
ist wasserdicht (IP67), vibrationssicher (4-Layer-PCB, SMD, keine
Sockel) und fuer den Dauereinsatz am Motorrad konzipiert. Die
Premium-Konfiguration lohnt sich nur fuer Nutzer, die Could-Features
(TPMS, dual E-Paper, Kugelgelenk-Halterung) nutzen moechten.

### 8.4 Kostensenkungs-Potenziale

| Massnahme | Einsparung (EUR) | Nachteil |
|---|---|---|
| Budget-GPS-Modul (2 Konst.) statt 4-Konst. | 21,60 | Schlechtere Genauigkeit (5-10m statt 2m) |
| Kein E-Paper-Display | 12,00 | Schlechtere Sonnenlicht-Lesbarkeit |
| PETG statt ASA-Gehaeuse | 13,00 | UV-Empfindlichkeit, geringere Temperaturgrenze |
| Kein Conformal Coating | 12,00 | Feuchtigkeitsschutz verringert |
| Keine Kupferfolie (EMV) | 8,00 | Erhoehtes EMV-Risiko (GPS-Ausfaelle) |
| Budget-IMU statt Premium | 3,00 | Mehr Rauschen, hoeherer Stromverbrauch |
| 2-Layer statt 4-Layer PCB | 4,80 | Schlechtere EMV, schwierigeres Routing |
| Bestueckung DIY (statt Dienstleister) | 20,00 | Hoeherer Zeitaufwand, Qualitaetsrisiko |
| **Maximale Einsparung** | **94,40** | Deutlich verringerte Zuverlaessigkeit |

Die maximale Einsparung von 94,40 EUR (29% der BOM) ist moeglich,
fuehrt aber zu einem System, das nicht fuer den Dauereinsatz am
Motorrad geeignet ist. Die Empfehlung ist, bei den EMV-relevanten
Komponenten (4-Layer-PCB, Conformal Coating, Kupferfolie) keine
Einsparungen vorzunehmen.

---

## Quellen

- docs/BOM.md: Komponenten-Preise, Lieferanten-Analyse, BOM-Varianten
- docs/research/09-master-plan.md: BOM mit Budget/Premium-Optionen,
  Kosten-Nutzen-Bewertung
- docs/PCB-LAYOUT.md: PCB-Fertigungskosten, Tooling-Kosten
- docs/CASE-DESIGN.md: Gehaeuse-Materialkosten, 3D-Druck-Parameter
- docs/ROADMAP.md: Projekt-Phasen und Fortschritt
- Marktpreise fuer Motorrad-Elektronik und Navigationsgeraete (Stand 2026)
- IPC-2221: Generic Standard on Printed Board Design (Trace-Width)
- IEC 60529: IP-Schutzklassen Definitionen
- SD-Karten-Spezifikation: Schreibzyklen (MLC: 10.000, TLC: 1.000-3.000)
- Li-Ion-Akku-Lebensdauer: 300-500 Zyklen, 3-5 Jahre Kalenderlebensdauer

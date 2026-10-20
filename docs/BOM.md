# Bill of Materials (BOM) & Sourcing-Konzept

> Stand: 16. Oktober 2026
> Projekt: Transalp Bordcomputer (Honda XL600V Transalp PD06)
> Plattform: ESP32-S3-WROOM-1 (N8R2), 8 MB Flash, 2 MB PSRAM
> Vorausgehende Dokumente: docs/research/02-hardware-deep-dive.md,
>   docs/research/09-master-plan.md, docs/PCB-LAYOUT.md, docs/CASE-DESIGN.md
> Alle Preise in EUR, Stand Oktober 2026, Einzelstückpreise sofern nicht
> anders vermerkt. Preise inklusive deutscher MwSt. (19%) bei Endkunden-Quellen,
> netto bei B2B-Lieferanten (Elektronik-Distributor DE #1, Elektronik-Distributor DE #2, B2B-Distributor US #1, B2B-Distributor US #2, Distributor EU).

---

## Inhaltsverzeichnis

1. [Komponenten-Uebersicht](#1-komponenten-uebersicht)
2. [Bezugsquellen-Analyse](#2-bezugsquellen-analyse)
3. [BOM-Varianten](#3-bom-varianten)
4. [Kosten-Analyse](#4-kosten-analyse)
5. [Risiko-Bewertung](#5-risiko-bewertung)
6. [Beschaffungs-Strategie](#6-beschaffungs-strategie)

---

## 1. Komponenten-Uebersicht

Die folgende Tabelle listet alle Bauteile des Transalp Bordcomputers auf.
Jedes Bauteil ist mit Spezifikation, Menge, Preis, Bezugsquelle und
Lieferzeit versehen. Die Preise sind Einzelstückpreise (1 Stueck) in EUR
und repraesentieren den Stand Oktober 2026. Bei hoeheren Stueckzahlen
fallen die Preise typischerweise um 20-40%.

### 1.1 MCU und Peripherie

| # | Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) | Bezugsquelle | Lieferzeit | Alternativen |
|---|---|---|---|---|---|---|---|---|
| 1 | ESP32-S3-WROOM-1 N8R2 | 8MB Flash, 2MB PSRAM, BLE 5.0, 45 GPIO, PCB Trace Antenne, LGA Modul 18x25,5mm | 1 | 5,40 | 5,40 | B2B-Distributor US #1 / B2B-Distributor US #2 | 3-5 Tage | DevKitC-1 (13,80 EUR), ESP32-S3-WROOM-1U (IPEX) |
| 2 | USB-C Buchse IP67 | Panel-Mount, wasserdicht, IP67, mit Dichtungsmembran | 1 | 3,50 | 3,50 | Elektronik-Distributor DE #1 / Online-Marktplatz | 2-4 Tage | USB-B wasserdicht, internes USB-C |
| 3 | MicroSD-Karten-Slot | Push-Push, SMD, 8-polig, fuer SPI-Mode | 1 | 1,20 | 1,20 | Distributor CN / Elektronik-Distributor DE #1 | 5-10 Tage | Wirelle SD-Breakout (China-Marktplatz, 0,50 EUR) |
| 4 | MicroSD-Karte 16GB | Class 10 / U1, Markenprodukt, FAT32 | 1 | 6,00 | 6,00 | Online-Marktplatz / Elektronik-Distributor DE #1 | 2-3 Tage | 8GB Class 10 NoName (3,00 EUR) |
| 5 | FPC-Connector 12-polig | 0,5mm Pitch, fuer Display-Anbindung | 1 | 0,50 | 0,50 | Distributor CN / B2B-Distributor US #2 | 5-10 Tage | JST-SM Kabel (0,80 EUR) |

### 1.2 Display

| # | Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) | Bezugsquelle | Lieferzeit | Alternativen |
|---|---|---|---|---|---|---|---|---|
| 6 | TFT-Display 3,5" IPS | 480x320 Pixel, SPI, IPS-Panel, 1000 nits Backlight | 1 | 18,00 | 18,00 | Display-Modul-Hersteller / Online-Marktplatz | 3-7 Tage | ILI9341 2,4" (6,00 EUR), ST7789 2,0" |
| 7 | E-Paper-Display 2,13" | 250x122 Pixel, monochrom, SSD1680/UC8151D Treiber | 1 | 12,00 | 12,00 | Display-Modul-Hersteller / Online-Marktplatz | 3-7 Tage | E-Paper 2,9" 296x128 (15,00 EUR), 1,54" |
| 8 | Acrylglas-Abdeckung | 2mm PMMA, konvex, hydrophob beschichtet | 1 | 2,00 | 2,00 | Baumarkt / Online-Marktplatz | 1-3 Tage | Polycarbonat 2mm |

### 1.3 Sensoren

| # | Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) | Bezugsquelle | Lieferzeit | Alternativen |
|---|---|---|---|---|---|---|---|---|---|
| 9 | GPS-Modul NEO-M9N | 4-Konstellation GNSS (GPS+GLONASS+Galileo+BeiDou), 25Hz, 2,0m CEP, UART, 12,2x16mm SMD | 1 | 24,60 | 24,60 | Distributor EU / Spezial-Distributor | 3-7 Tage | ATGM336H (3,00 EUR), NEO-M8N (15,00 EUR) |
| 10 | GPS-Antenne | Active Patch 25x25mm, Magnetfuss, 3-5V, 28dB Gain, 1,5m Kabel | 1 | 8,00 | 8,00 | Online-Marktplatz / Elektronik-Distributor DE #1 | 2-4 Tage | Passive Patch (2,00 EUR), Chip-Antenne |
| 11 | IMU BMI270 | 6-DoF (Accel+Gyro), 685µA, I2C/SPI, 2KB FIFO, 2,5x3,0mm LGA-14 | 1 | 5,00 | 5,00 | B2B-Distributor US #1 / B2B-Distributor US #2 / China-Marktplatz | 5-10 Tage | MPU6050 (2,00 EUR), LSM6DS33 (5,00 EUR) |
| 12 | Barometer BMP390 | 300-1250 hPa, ±0,03 hPa rel., 0,25m Aufloesung, I2C/SPI, 2x2mm LGA-10 | 1 | 3,50 | 3,50 | B2B-Distributor US #1 / B2B-Distributor US #2 | 5-10 Tage | BMP388 (OBSOLETE), BMP280 (2,00 EUR), DPS310 |
| 13 | Kuehlmittel-Temp. | DS18B20 wasserdicht, digital, 1-Wire, ±0,5°C, -55 bis +125°C, Edelstahlrohr | 1 | 3,00 | 3,00 | Online-Marktplatz / Elektronik-Distributor DE #1 | 2-4 Tage | NTC 10k Thermistor (1,00 EUR) |
| 14 | Oeldrucksensor | Analog 0-5 bar, 10-180 Ohm, M10x1 Gewinde, wasserdicht | 1 | 25,00 | 25,00 | Online-Marktplatz / Spezial-Distributor | 5-10 Tage | Original-Oeldruckschalter (digital, 0 EUR) |
| 15 | Hall-Sensor (Rad) | A3144 Unipolar, 4,5-24V, TO-92, + Magnet 5mm | 1 | 2,00 | 2,00 | Elektronik-Distributor DE #1 / China-Marktplatz | 3-5 Tage | Reed-Schalter (1,00 EUR), A1101 |
| 16 | Bordspannungs-Messung | Spannungsteiler 100k/33k, 1%, 0805 SMD | 1 | 0,10 | 0,10 | Elektronik-Distributor DE #1 / Distributor CN | 2-3 Tage | Praezisions-Spannungsteiler (0,50 EUR) |
| 17 | Kraftstoffstand-Messung | Spannungsteiler 10k/10k + Operationsverstaerker (MCP6001) | 1 | 1,00 | 1,00 | Elektronik-Distributor DE #1 / Distributor CN | 2-3 Tage | Direkter Spannungsteiler (0,10 EUR) |

### 1.4 Power-Management

| # | Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) | Bezugsquelle | Lieferzeit | Alternativen |
|---|---|---|---|---|---|---|---|---|---|
| 18 | Buck-Converter 12V->5V | MP1584EN Modul, 1,5MHz, 3A, 4,5-28V Input, 90-92% Effizienz, 22x15mm | 1 | 1,50 | 1,50 | China-Marktplatz / Online-Marktplatz | 7-14 Tage | LM2596 Modul (2,00 EUR), TPS5430 (6,00 EUR) |
| 19 | LDO 5V->3,3V | HT7333, 250mA, 30µVrms Rauschen, 4µA Quiescent, SOT-89 | 1 | 0,50 | 0,50 | Elektronik-Distributor DE #1 / Distributor CN | 2-5 Tage | AMS1117-3.3 (0,15 EUR), MCP1700-3302E (1,00 EUR) |
| 20 | Backup-Akku 18650 | Li-Ion, 3400mAh, 3,7V nominal, geschuetzt mit PCB | 1 | 6,00 | 6,00 | Online-Marktplatz / Elektronik-Distributor DE #1 | 2-4 Tage | LiPo 1000mAh (4,00 EUR), LiFePO4 |
| 21 | Akku-Lademodul | TP4056 Modul mit Schutz (DW01A), 5V Input, 1A Ladestrom, Micro-USB oder Type-C | 1 | 0,50 | 0,50 | China-Marktplatz / Online-Marktplatz | 7-14 Tage | IP5306 Modul (3,00 EUR) |
| 22 | Boost-Converter 3,7V->5V | MT3608 Modul, 2A, bis 28V Output, 17x11mm | 1 | 0,80 | 0,80 | China-Marktplatz / Online-Marktplatz | 7-14 Tage | XL6009 Modul (1,50 EUR) |
| 23 | Schottky-Diode | 1N5822, 3A, 40V, DO-201AD, fuer Power-Path ORing | 2 | 0,10 | 0,20 | Elektronik-Distributor DE #1 / Distributor CN | 2-3 Tage | 1N5819 (1A, 0,05 EUR), SS34 SMD |
| 24 | TVS-Diode | SMAJ15A, 15V Standoff, 24,4V Clamp, 400W, SMA | 2 | 0,50 | 1,00 | Elektronik-Distributor DE #1 / B2B-Distributor US #1 | 2-5 Tage | P6KE15A, SMBJ15A |
| 25 | Sicherung 5A | Flachsicherung ATO/ATC, 5A, mit Inline-Halter | 1 | 1,00 | 1,00 | Elektronik-Distributor DE #1 / Online-Marktplatz | 2-3 Tage | PTC 3A resettable (0,80 EUR) |
| 26 | Elko 100µF | 100µF, 25V, Low-ESR, SMD Tantal oder THT | 1 | 0,30 | 0,30 | Elektronik-Distributor DE #1 / Distributor CN | 2-3 Tage | 2x 47µF parallel |

### 1.5 Passive Bauelemente

| # | Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) | Bezugsquelle | Lieferzeit | Alternativen |
|---|---|---|---|---|---|---|---|---|---|
| 27 | Widerstaende 0805 | diverse Werte (10k, 33k, 1k, 22, 33, 4,7k Ohm), 1%, 0,125W | 20 | 0,02 | 0,40 | Elektronik-Distributor DE #1 / Distributor CN | 2-3 Tage | Sortiment 0603 (0,01 EUR/Stueck) |
| 28 | Kondensatoren 0805 | 100nF X7R, 25V, Decoupling | 15 | 0,05 | 0,75 | Elektronik-Distributor DE #1 / Distributor CN | 2-3 Tage | 0603 100nF (0,03 EUR) |
| 29 | Bulk-Kondensatoren | 10µF X5R, 0805 und 22µF X5R, 1206 | 4 | 0,15 | 0,60 | Elektronik-Distributor DE #1 / Distributor CN | 2-3 Tage | Tantal 10µF (0,30 EUR) |
| 30 | Ferrite-Beads | 600 Ohm @ 100MHz, 2A, 0805, BLM18AG601 | 4 | 0,20 | 0,80 | B2B-Distributor US #1 / Elektronik-Distributor DE #1 | 2-5 Tage | 300 Ohm Bead, 1000 Ohm Bead |
| 31 | LED Status | LED, gruen oder blau, 0805 SMD, 3,2V, 20mA | 1 | 0,10 | 0,10 | Elektronik-Distributor DE #1 / Distributor CN | 2-3 Tage | WS2812B RGB (0,75 EUR) |
| 32 | Zener-Diode 3,3V | 3,3V, 500mW, SOT-23 oder DO-35 | 2 | 0,20 | 0,40 | Elektronik-Distributor DE #1 / Distributor CN | 2-3 Tage | TVS 3,3V SOT-23 |
| 33 | I2C-Pull-Ups | 4,7k Ohm, 0805 (bereits in Widerstaende enthalten) | 2 | 0,00 | 0,00 | — | — | — |

### 1.6 MOSFET und Relais-Steuerung

| # | Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) | Bezugsquelle | Lieferzeit | Alternativen |
|---|---|---|---|---|---|---|---|---|---|
| 34 | Gate-Driver TC4427 | 1,5A, 2-Kanal, 4,5-18V, Non-Inverting, SOIC-8 | 1 | 1,50 | 1,50 | B2B-Distributor US #1 / B2B-Distributor US #2 | 5-10 Tage | UCC27524, MIC4427 |
| 35 | MOSFET IRLZ44N | N-Channel, 55V, 25A, Rds(on) 22mOhm @ Vgs=5V, TO-220 | 1 | 1,50 | 1,50 | Elektronik-Distributor DE #1 / B2B-Distributor US #1 | 2-5 Tage | IRF520 (1,50 EUR, nicht Logic-Level), AO3400 SMD |
| 36 | Optokoppler PC817 | 1-Kanal, Transistor-Ausgang, DIP-4, CTR 50-600% | 2 | 0,50 | 1,00 | Elektronik-Distributor DE #1 / Distributor CN | 2-5 Tage | TLP185 SMD, 6N137 (schneller) |
| 37 | Freilaufdiode 1N5408 | 3A, 1000V, DO-201AD, fuer induktive Lasten | 2 | 0,10 | 0,20 | Elektronik-Distributor DE #1 / Distributor CN | 2-3 Tage | 1N4007 (1A), Schottky 1N5822 |
| 38 | Relais-Modul 4-Kanal | SRD-12V, 10A, mit Optokoppler, Active-LOW | 1 | 4,00 | 4,00 | Online-Marktplatz / China-Marktplatz | 3-7 Tage | Einzelrelais SRD-12V (1,00 EUR/Stueck) |

### 1.7 Mechanische Teile

| # | Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) | Bezugsquelle | Lieferzeit | Alternativen |
|---|---|---|---|---|---|---|---|---|---|
| 39 | Stiftleisten Header | 2,54mm Raster, gerade, 40-polig, breakable | 2 | 0,50 | 1,00 | Elektronik-Distributor DE #1 / Online-Marktplatz | 2-3 Tage | Buchsenleisten, Raster 2,0mm |
| 40 | Abstandshalter | M3, 10mm, Sechskant, Messing, innengewindet | 4 | 0,30 | 1,20 | Elektronik-Distributor DE #1 / Online-Marktplatz | 2-3 Tage | Kunststoff-Abstandshalter (0,15 EUR) |
| 41 | Schrauben M3x8 | Senkkopf, Edelstahl, Inbus | 4 | 0,05 | 0,20 | Baumarkt / Online-Marktplatz | 1-3 Tage | Stahl verzinkt (0,03 EUR) |
| 42 | Schrauben M3x12 | Senkkopf, Edelstahl, Inbus, fuer Deckel | 4 | 0,05 | 0,20 | Baumarkt / Online-Marktplatz | 1-3 Tage | Stahl verzinkt (0,03 EUR) |
| 43 | Schrauben M2,5x6 | Senkkopf, Edelstahl, Inbus, fuer Display-Rahmen | 4 | 0,05 | 0,20 | Baumarkt / Online-Marktplatz | 1-3 Tage | M2x6 (0,04 EUR) |
| 44 | Federscheiben M3 | DIN 127B, Edelstahl, fuer Vibrationssicherung | 8 | 0,02 | 0,16 | Elektronik-Distributor DE #1 / Online-Marktplatz | 2-3 Tage | Zahnscheiben DIN 6798 |
| 45 | Schrauben M6x20 | Inbus, Edelstahl, fuer Lenkerhalterung (U-Bolt) | 2 | 0,10 | 0,20 | Baumarkt / Online-Marktplatz | 1-3 Tage | — |
| 46 | 18650-Akkuhalter | PCB-Mount, fuer 1x 18650, mit Kontakt | 1 | 0,50 | 0,50 | Elektronik-Distributor DE #1 / Distributor CN | 2-5 Tage | Akku-Klebeband (0,20 EUR) |

### 1.8 Gehaeuse

| # | Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) | Bezugsquelle | Lieferzeit | Alternativen |
|---|---|---|---|---|---|---|---|---|---|
| 47 | ASA-Filament | 1,75mm, 1kg Spule, hell (weiss/hellgrau), UV-stabil | 1 | 25,00 | 25,00 | Online-Marktplatz / Filament-versand | 2-4 Tage | PETG (20,00 EUR, nur Prototyp) |
| 48 | PETG-Filament | 1,75mm, 500g, fuer Display-Rahmen / Innenteile | 1 | 12,00 | 12,00 | Online-Marktplatz / Filament-versand | 2-4 Tage | PLA (10,00 EUR, nur Prototyp) |
| 49 | Silikon-O-Ring | 2mm Cordstaerke, Shore 60A, -40 bis +200°C, UV-stabil, Umfangsdichtung | 1 | 2,00 | 2,00 | Online-Marktplatz / Spezial-Distributor | 2-4 Tage | NBR O-Ring (1,00 EUR, weniger temperaturbestaendig) |
| 50 | EPDM-Schaumstoff | Selbstklebend, 5mm, Shore 10-30A, fuer Sekundaerdichtung | 1 | 3,00 | 3,00 | Online-Marktplatz / Baumarkt | 2-4 Tage | Geschlossenzelliger Schaum (2,00 EUR) |
| 51 | Kabelverschraubungen PG7 | PA66, IP67, mit NBR-O-Ring, fuer Sensorkabel, 3-6,5mm Kabel-Ø | 4 | 0,80 | 3,20 | Elektronik-Distributor DE #1 / Online-Marktplatz | 2-4 Tage | M12 Kabelverschraubung (0,90 EUR) |
| 52 | Kabelverschraubungen PG9 | PA66, IP67, mit NBR-O-Ring, fuer Sensorkabel, 4-8mm Kabel-Ø | 2 | 1,00 | 2,00 | Elektronik-Distributor DE #1 / Online-Marktplatz | 2-4 Tage | M16 Kabelverschraubung |
| 53 | Druckausgleichsmembran | ePTFE, IP68, M6x0,75 Gewinde, -40 bis +125°C | 1 | 8,00 | 8,00 | Online-Marktplatz / Spezial-Distributor | 3-7 Tage | Loche mit ePTFE-Patch (3,00 EUR) |
| 54 | Kupferfolie | Selbstklebend, 0,05mm, 50mm breit, 1m Rolle, fuer EMV-Abschirmung | 1 | 8,00 | 8,00 | Online-Marktplatz / Baumarkt | 2-4 Tage | Alufolie (2,00 EUR, weniger effektiv) |
| 55 | Conformal Coating | Polyurethan, 50ml Spruehdose, UR-Typ, -65 bis +130°C | 1 | 12,00 | 12,00 | Online-Marktplatz / Spezial-Distributor | 3-7 Tage | Acryl-Coating (8,00 EUR), Silikon-Coating |
| 56 | Acrylglas-Platte | 2mm, transparent, 100x80mm, fuer Display-Abdeckung | 1 | 2,00 | 2,00 | Baumarkt | 1-3 Tage | Polycarbonat 2mm (3,00 EUR) |

### 1.9 Kabel und Stecker

| # | Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) | Bezugsquelle | Lieferzeit | Alternativen |
|---|---|---|---|---|---|---|---|---|---|
| 57 | Sensor-Kabel verdrillt | 4-adrig, AWG24, abgeschirmt (S/UTP), pro Meter | 3 | 1,50 | 4,50 | Online-Marktplatz / Elektronik-Distributor DE #1 | 2-4 Tage | Unabgeschirmt 4-adrig (0,80 EUR/m) |
| 58 | JST-SM Stecker 4-polig | Wasserdicht, IP65, mit Rasterverriegelung, fuer GPS | 1 | 1,50 | 1,50 | Online-Marktplatz / China-Marktplatz | 3-7 Tage | AMP Superseal 3-polig (2,50 EUR) |
| 59 | AMP Superseal 2-polig | IP67, 3A, fuer 12V Bordnetz | 1 | 2,50 | 2,50 | Online-Marktplatz / Spezial-Distributor | 3-7 Tage | Deutch DTM 2-polig (3,50 EUR) |
| 60 | AMP Superseal 3-polig | IP67, 3A, fuer ADC-Sensoren | 2 | 2,50 | 5,00 | Online-Marktplatz / Spezial-Distributor | 3-7 Tage | Deutch DTM 3-polig |
| 61 | AMP Superseal 4-polig | IP67, 3A, fuer Relais-Steuerung | 1 | 2,50 | 2,50 | Online-Marktplatz / Spezial-Distributor | 3-7 Tage | Deutch DTM 4-polig |
| 62 | Ferritkern | Klappferrit, Material 43/73, >100 Ohm @ 100MHz, fuer Kabel | 4 | 0,80 | 3,20 | Online-Marktplatz / Elektronik-Distributor DE #1 | 2-4 Tage | Ringkern (0,50 EUR, nicht klappbar) |
| 63 | Kabelbinder | 100mm, schwarz, UV-beständig, natur (PA66) | 20 | 0,02 | 0,40 | Baumarkt / Online-Marktplatz | 1-3 Tage | — |
| 64 | Schrumpfschlauch | 3mm, 1m, schwarz, Polyolefin, 2:1 Schrumpfrate | 1 | 2,00 | 2,00 | Online-Marktplatz / Baumarkt | 1-3 Tage | — |

### 1.10 PCB-Fertigung

| # | Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) | Bezugsquelle | Lieferzeit | Alternativen |
|---|---|---|---|---|---|---|---|---|---|
| 65 | PCB 4-Layer | 80x50mm, FR4, 1,6mm, ENIG, 2oz Kupfer L1/L4, 1oz L2/L3 | 5 | 6,80 | 6,80 | JLCPCB / PCBWay | 5-8 Tage | 2-Layer (2,00 EUR/5 Stueck) |
| 66 | PCB-Bestueckung | SMD-Bestueckung durch JLCPCB, ~50 Bauteile | 1 | 20,00 | 20,00 | JLCPCB | 5-8 Tage | DIY Heissluft (0 EUR), lokaler Dienstleister (35 EUR) |
| 67 | Loetpaste | Sn42/Bi57/Ag1, bleifrei, 50g Spritze | 1 | 8,00 | 8,00 | Online-Marktplatz / Elektronik-Distributor DE #1 | 2-4 Tage | Sn63/Pb37 bleihaltig (6,00 EUR) |

### 1.11 CarPlay-Display und Lenker-Controller

Diese Komponenten erweitern den Bordcomputer um CarPlay
(externes 5" Display) und einen Lenker-Controller zur Bedienung
von Display und CarPlay. Sie sind optional und werden im
CarPlay-Upgrade-Kit bzw. Lenker-Controller-Kit angeboten.

| # | Bauteil | Spezifikation | Menge | Einzelpreis (EUR) | Gesamtpreis (EUR) | Bezugsquelle | Lieferzeit | Alternativen |
|---|---|---|---|---|---|---|---|---|
| 68 | CarPlay-Display EKIY 5" | 5" IPS, 854x480, IPX7, BT5.0, Wireless CarPlay + Android Auto, 12V | 1 | 90,00 | 90,00 | China-Marktplatz (Suchlink) | 7-14 Tage | Jiuyin 5" (65 EUR), KEMIMOTO 5" (120 EUR) |
| 69 | CarPlay-Display Jiuyin 5" (Budget) | 5" IPS, IPX7, BT, GPS, Wireless CarPlay | 1 | 65,00 | 65,00 | China-Marktplatz (Suchlink) | 7-14 Tage | EKIY 5" (90 EUR) |
| 70 | CarPlay-Display KEMIMOTO 5" (Premium) | 5", 700 nits, IP67, Auto-Brightness, Handschuh-Touch | 1 | 120,00 | 120,00 | Online-Marktplatz | 2-4 Tage | Chigee AIO-5 EVO (370 EUR) |
| 71 | Lenker-Controller SilverFox C2 | CNC-Alu, IP67, Joystick+Switch, M12-Stecker, BLE+Kabel, 22mm, Made in Finland | 1 | 235,00 | 235,00 | SilverFox (silverfoxcontrollers.com) | 1-2 Wochen | Chigee CGRC (90 EUR), Custom PCB (25 EUR) |
| 72 | Lenker-Controller Chigee CGRC Multi (Wireless) | BLE 5.0, CR2032, 6 Tasten+Joystick, 30-35mm Montage | 1 | 90,00 | 90,00 | SportsBikeShop (sportsbikeshop.co.uk) | 5-10 Tage | SilverFox C2 (235 EUR) |
| 73 | Custom Controller PCB | ATmega328P/RP2040, I2C, 6 Tasten+Joystick, 22mm Montage | 1 | 25,00 | 25,00 | JLCPCB + China-Marktplatz | 5-14 Tage | SilverFox C2 (235 EUR) |
| 74 | Lenker-Halterung 22mm | Aluminium, fuer 5" Display, schwingungsgeedaempft | 1 | 15,00 | 15,00 | China-Marktplatz | 7-14 Tage | 3D-Druck ASA (5 EUR) |
| 75 | M12-Kabel 4-polig | Wasserdicht, IP67, 1,5m, fuer SilverFox C2 | 1 | 12,00 | 12,00 | Elektronik-Distributor DE #1 | 2-5 Tage | JST-SM 4-polig (1,50 EUR) |

**Auswahlkriterien:**

- **CarPlay-Display:** EKIY 5" als Standard (bestes Preis-Leistungs-
  Verhaeltnis, IPX7 reicht mit Regenschutz). KEMIMOTO 5" als Premium-
  Option (IP67, 700 nits, Handschuh-Touch). Jiuyin 5" als Budget-
  Alternative. Chigee AIO-5 EVO nur fuer Kunden mit Dashcam-Wunsch.
- **Lenker-Controller:** SilverFox C2 als Premium (IP67, Made in Finland,
  M12-Stecker, BLE+Kabel). Chigee CGRC als Budget (wireless, CR2032).
  Custom PCB als Bastler-Option (I2C, 25 EUR).
- **Verbindung:** SilverFox C2 unterstuetzt gleichzeitig Kabel (GPIO)
  und BLE. Chigee CGRC nur BLE. Custom PCB nur I2C.

---

## 2. Bezugsquellen-Analyse

Die folgende Analyse bewertet fuer jede Bauteil-Kategorie die verfuegbaren
Lieferanten hinsichtlich Preis, Lieferzeit, Mindestbestellmenge und
Verfuegbarkeitsrisiko. Die Lieferanten werden mit ihrer jeweiligen Staerke
und Schwaeche beschrieben.

### 2.1 Lieferanten-Profil

**Elektronik-Distributor DE #1**

Deutscher Online-Haendler fuer elektronische Bauelemente und Werkzeug.
Staerken: grosse Auswahl an passiven Bauelementen (Widerstaende,
Kondensatoren), Halbleiter von herkoemmlichen Herstellern, schnelle
Lieferung innerhalb Deutschlands (1-3 Werktage), kein Mindestbestellwert,
guenstige Versandkosten (ab 4,95 EUR). Schwaeche: eingeschraenkte Auswahl
bei Spezial-ICs (z.B. BMI270, BMP390 nicht immer verfuegbar), keine
SMD-Bauteile in kleinen Mengen (meist Mindestmenge 10 Stueck pro Wert).

Einsatz im Projekt: passive Bauelemente (Widerstaende, Kondensatoren,
Ferrite-Beads), Standard-Halbleiter (HT7333, TC4427, IRLZ44N, PC817),
Kabelverschraubungen, Abstandshalter, Schrauben.

**Elektronik-Distributor DE #2**

Deutscher Elektronik-Versandhaendler mit breitem Sortiment.
Staerken: schnelle Lieferung (1-3 Werktage), grosse Auswahl an
Werkzeug und Verbrauchsmaterial, auch Kleinmengen ab 1 Stueck.
Schwaeche: Preise oft 30-50% hoeher als Elektronik-Distributor DE #1 bei identischen
Bauelementen, beschraenkte Auswahl bei SMD-Bauteilen.

Einsatz im Projekt: nur fuer Bauteile, die bei Elektronik-Distributor DE #1 nicht verfuegbar
sind, oder fuer schnelle Ersatzbeschaffung am Wochenende.

**B2B-Distributor US #1**

Globaler B2B-Distributor fuer Halbleiter und elektronische Bauelemente.
Staerken: riesige Auswahl an Halbleitern direkt vom Hersteller, aktuelle
Datenblaetter, technische Support-Dokumente, keine Mindestbestellmenge
(ab 1 Stueck), Live-Stock-Anzeige. Schwaeche: Versandkosten ab 6,90 EUR
(netto, B2B), Preise in USD mit Wechselkursrisiko, Versand aus
Niederlande (2-4 Werktage).

Einsatz im Projekt: ESP32-S3-WROOM-1-N8R2 (3,30 EUR netto), BMI270
(3,40 EUR netto), BMP390 (3,50 EUR netto), TC4427, Spezial-ICs.

**B2B-Distributor US #2**

US-amerikanischer B2B-Distributor, vergleichbar mit B2B-Distributor US #1.
Staerken: noch groessere Auswahl als B2B-Distributor US #1 bei Spezialbauteilen,
exzellente Such- und Filterfunktion, Live-Stock, keine Mindestmenge.
Schwaeche: Preise oft in USD, Wechselkursrisiko, Versand aus USA
mit zusaetzlicher Einfuhrabwicklung (3-7 Werktage).

Einsatz im Projekt: Alternative fuer B2B-Distributor US #1 bei Nicht-Verfuegbarkeit,
FPC-Connector.

**Distributor EU**

Polnischer B2B-Elektronik-Distributor mit Europa-Logistik.
Staerken: guenstige Preise fuer u-blox GPS-Module (NEO-M9N 20,32 EUR
netto), gute Auswahl an Steckverbindern, keine Mindestbestellmenge,
Versand aus Polen (2-5 Werktage). Schwaeche: Website auf
Polnisch/Englisch, deutschsprachiger Support beschraenkt.

Einsatz im Projekt: NEO-M9N GPS-Modul, AMP Superseal Stecker.

**Distributor CN**

Chinesischer Online-Haendler fuer elektronische Bauelemente.
Staerken: sehr guenstige Preise fuer SMD-Bauteile und PCB-Bestueckung
(100nF Kondensator 0,01 EUR, 10kOhm Widerstand 0,005 EUR), Integration
mit JLCPCB fuer Bestueckung. Schwaeche: Versand aus China (7-14 Tage),
Mindestbestellwert 10 USD, zollpflichtig bei Warenwert > 150 EUR.

Einsatz im Projekt: SMD-Passivbauelemente in groesseren Mengen,
FPC-Connector, MicroSD-Slot.

**JLCPCB (jlcpcb.com)**

Chinesischer PCB-Hersteller und Bestueckungs-Dienstleister.
Staerken: sehr guenstige PCB-Preise (4-Layer ab 8 EUR fuer 5 Stueck),
integrierte SMD-Bestueckung (ab 8 EUR fuer 5 Boards), kurze
Lieferzeit (5-8 Tage inkl. Versand). Schwaeche: Versand aus China,
Zollabwicklung bei Warenwert > 150 EUR, nicht alle Bauteile im
Bestueckungs-Katalog verfuegbar (z.B. BMI270, BMP390).

Einsatz im Projekt: PCB-Fertigung, SMD-Bestueckung.

**China-Marktplatz**

Chinesischer B2C-Marktplatz fuer Elektronik und Verbrauchsgueter.
Staerken: sehr guenstige Preise fuer Module (MP1584EN 0,50 EUR,
TP4056 0,30 EUR, MT3608 0,50 EUR), riesige Auswahl an Breakout-Boards.
Schwaeche: lange Lieferzeit (7-21 Tage), Qualitaetsschwankungen,
keine Gewaehrleistung, Clones und Fakes verbreitet (besonders bei
GPS-Modulen).

Einsatz im Projekt: Module (MP1584, TP4056, MT3608), Hall-Sensor,
SMD-Bauteile in grossen Mengen, nur fuer Prototyp-Phase.

**Online-Marktplatz (Online-Marktplatz)**

Deutscher Online-Marktplatz. Staerken: schnelle Lieferung (1-3 Tage
mit Prime), breite Auswahl an Verbrauchsmaterial, Filament, Werkzeug,
Kabel. Schwaeche: Preise fuer elektronische Bauelemente oft 50-100%
ueber Fachhandel, beschraenkte Auswahl bei SMD-Bauteilen.

Einsatz im Projekt: Filament (ASA, PETG), Schrumpfschlauch,
Kabelbinder, Kupferfolie, Conformal Coating, Acrylglas, Ersatz bei
dringenden Bauteilen.

### 2.2 Preisvergleich ausgewaehlter Bauteile

Die folgende Tabelle vergleicht die Preise ausgewaehlter Bauteile
ueber verschiedene Lieferanten, um die guenstigste Quelle zu
identifizieren.

| Bauteil | B2B-Distributor US #1 | B2B-Distributor US #2 | Distributor EU | Elektronik-Distributor DE #1 | China-Marktplatz | Beste Quelle |
|---|---|---|---|---|---|---|
| ESP32-S3-WROOM-1 N8R2 | 3,30 EUR | 5,40 EUR | n.v. | n.v. | 2,50 EUR* | B2B-Distributor US #1 (original, garantiert) |
| NEO-M9N-00B | 14,40 EUR* | 15,72 EUR* | 24,60 EUR** | n.v. | 12,00 EUR* | Distributor EU (original, EU-Lager) |
| BMI270 (Bare IC) | 3,40 EUR | 3,90 EUR | n.v. | n.v. | 5,00 EUR** | B2B-Distributor US #1 (original) |
| BMP390 (Bare IC) | 3,50 EUR | 3,50 EUR | n.v. | n.v. | n.v. | B2B-Distributor US #1 / B2B-Distributor US #2 |
| MP1584EN Modul | n.v. | n.v. | n.v. | 1,80 EUR | 0,50 EUR | China-Marktplatz (Prototyp) |
| HT7333 LDO | 0,50 EUR | 0,60 EUR | n.v. | 0,50 EUR | 0,20 EUR* | Elektronik-Distributor DE #1 (schnell) |
| TC4427 Gate-Driver | 1,40 EUR | 1,50 EUR | n.v. | 1,50 EUR | 0,80 EUR* | B2B-Distributor US #1 |
| IRLZ44N | 1,50 EUR | 1,60 EUR | n.v. | 1,50 EUR | 0,80 EUR* | Elektronik-Distributor DE #1 |
| ASA Filament 1kg | n.v. | n.v. | n.v. | n.v. | n.v. | Online-Marktplatz (25 EUR) |
| PCB 4-Layer (5 St.) | n.v. | n.v. | n.v. | n.v. | n.v. | JLCPCB (6,80 EUR/Board) |

(*) = Preis aus USD umgerechnet mit Kurs 1 USD = 0,92 EUR
(**) = Preis inkl. MwSt., fuer Endkunden
n.v. = nicht verfuegbar

### 2.3 Verfuegbarkeitsrisiko pro Bauteil-Kategorie

**Geringes Risiko (Multi-Source, grosse Lagerbestaende):**
- Passive Bauelemente (Widerstaende, Kondensatoren, Ferrite-Beads):
  verfuegbar bei Elektronik-Distributor DE #1, Elektronik-Distributor DE #2, B2B-Distributor US #1, B2B-Distributor US #2, Distributor CN, China-Marktplatz.
  Kein Verfuegbarkeitsrisiko.
- Standard-Halbleiter (HT7333, AMS1117, PC817, 1N5822, IRLZ44N):
  verfuegbar bei allen grossen Distributoren.
- Mechanische Teile (Schrauben, Abstandshalter, Kabelbinder):
  verfuegbar bei Elektronik-Distributor DE #1, Online-Marktplatz, Baumarkt.
- Module (MP1584EN, TP4056, MT3608): verfuegbar bei China-Marktplatz, Online-Marktplatz,
  Elektronik-Distributor DE #1. Kein Risiko, aber Qualitaetsschwankungen bei China-Marktplatz.

**Mittleres Risiko (Single-Source bei einigen Lieferanten):**
- ESP32-S3-WROOM-1-N8R2: verfuegbar bei B2B-Distributor US #1 und B2B-Distributor US #2, aber
  beide beziehen vom MCU-Chiphersteller. Bei Lieferengpaessen
  (aktuelle Vorlaufzeit bei B2B-Distributor US #2: 8 Wochen) koennen beide
  betroffen sein. Alternative: DevKitC-1 als Prototyp-Ersatz.
- NEO-M9N GPS-Modul: verfuegbar bei Distributor EU, B2B-Distributor US #1, B2B-Distributor US #2. China-Marktplatz-
  Clones unter 15 EUR sind oft Fakes (u-blox Clone oder ATGM336H
  relabeled). Originalmodul bei Distributor EU (24,60 EUR) ist die sicherste
  Quelle.
- ASA-Filament: verfuegbar bei verschiedenen Anbietern auf Online-Marktplatz und
  Filament-versand. Keine engpaesse, aber Qualitaetsschwankungen.

**Hohes Risiko (Single-Source, Obsolescence, lange Lieferzeiten):**
- BMP388: bei B2B-Distributor US #2 als OBSOLETE markiert! Direkter Ersatz ist
  der BMP390 (Pin-kompatibel, verbesserte Spezifikation). Der BMP388
  ist bei einigen Distributoren noch auf Lager, aber nicht mehr in
  Produktion. EMPFEHLUNG: BMP390 statt BMP388 verwenden.
- BMI270: verfuegbar bei B2B-Distributor US #1 (60.904 auf Lager) und B2B-Distributor US #2
  (67.748 auf Lager). Jedoch bei B2B-Distributor US #2 als "Limited Supply"
  markiert. Breakout-Boards (China-Marktplatz) als Alternative.
- AMP Superseal Stecker: verfuegbar bei Spezial-Distributor und Online-Marktplatz.
  Jedoch nicht bei Elektronik-Distributor DE #1/Elektronik-Distributor DE #2. Bei Lieferengpaessen
  Alternative: Deutch DTM Serie (teurer, aber besser verfuegbar).
- Druckausgleichsmembran (ePTFE): Spezialprodukt, verfuegbar bei
  ausgewaehlten Anbietern. Alternative: ePTFE-Patch mit Loch.

---

## 3. BOM-Varianten

Drei Ausbaustufen werden definiert, um unterschiedliche Budgets und
Funktionsumfaenge abzudecken. Jede Variante baut auf der vorherigen
auf.

### 3.1 Minimal-Konfiguration (Prototyp)

Funktionierendes Kernsystem mit den wichtigsten Grundfunktionen:
Geschwindigkeit, Drehzahl, Kuehlmitteltemperatur, TFT-Display,
SD-Karten-Logging. Kein E-Paper, kein GPS, kein BLE-Datenexport,
kein Backup-Akku, keine Relais-Steuerung, keine Oeldruckmessung.
DevKitC-1 auf Perfboard, PETG-Gehaeuse.

| # | Bauteil | Menge | Preis (EUR) |
|---|---|---|---|
| 1 | ESP32-S3-DevKitC-1 N8R2 | 1 | 13,80 |
| 2 | TFT-Display ILI9341 2,4" 320x240 | 1 | 6,00 |
| 3 | ATGM336H GPS-Modul (Budget) | 1 | 3,00 |
| 4 | Passive GPS-Antenne | 1 | 2,00 |
| 5 | MPU6050 IMU Breakout | 1 | 2,00 |
| 6 | BMP280 Barometer Breakout | 1 | 2,00 |
| 7 | NTC 10k Thermistor (Kuehlmittel) | 1 | 1,00 |
| 8 | MicroSD-Breakout + 8GB Karte | 1 | 5,00 |
| 9 | LM2596 Buck-Modul 12V->5V | 1 | 2,00 |
| 10 | AMS1117-3.3 LDO | 1 | 0,15 |
| 11 | PC817 Optokoppler (RPM) | 1 | 0,50 |
| 12 | Spannungsteiler (Bordspannung) | 1 | 0,10 |
| 13 | Kraftstoffstand-Spannungsteiler | 1 | 0,10 |
| 14 | Piezo-Buzzer passiv | 1 | 1,00 |
| 15 | LED Status gruen | 1 | 0,10 |
| 16 | Perfboard 100x80mm | 1 | 2,00 |
| 17 | Jumper-Wire Sortiment | 1 | 3,00 |
| 18 | PETG-Filament 500g | 1 | 12,00 |
| 19 | NBR O-Ring (Dichtung) | 1 | 1,00 |
| 20 | PG7 Kabelverschraubungen | 4 | 3,20 |
| 21 | U-Bolt Lenkerhalterung 22mm | 1 | 2,00 |
| 22 | Inline-Sicherung 5A | 1 | 1,00 |
| 23 | Verbrauchsmaterial (Kabel, Widerstaende, Kondensatoren) | — | 5,00 |
| — | **Zwischensumme** | | **69,85** |
| — | Contingency +15% | | 10,48 |
| — | **Gesamt Minimal** | | **80,33** |

Einschraenkungen: Nicht wasserdicht (PETG-Gehaeuse ohne Conformal
Coating), nicht vibrationssicher (Perfboard + Steckverbindungen),
keine Smartphone-Anbindung, kein Backup bei Power-Loss, kein
E-Paper-Display. Geeignet fuer Software-Entwicklung und
Funktionstest auf dem Schreibtisch, nicht fuer Dauereinsatz am
Motorrad.

### 3.2 Standard-Konfiguration

Vollwertiges Bordcomputer-System mit allen Must- und Should-Features:
BLE-Anbindung, Smartphone-Integration, TFT-Display 3,5" IPS,
E-Paper-Zweitdisplay, GPS NEO-M9N, Backup-Akku, Oeldruck,
Relais-Steuerung, PWM-Heizgriffe. Custom PCB (4-Layer, ENIG),
ASA-Gehaeuse mit Conformal Coating.

| # | Bauteil | Menge | Preis (EUR) |
|---|---|---|---|
| 1 | ESP32-S3-WROOM-1 N8R2 (Bare Modul) | 1 | 5,40 |
| 2 | TFT-Display 3,5" IPS 480x320 | 1 | 18,00 |
| 3 | E-Paper-Display 2,13" 250x122 | 1 | 12,00 |
| 4 | NEO-M9N GPS-Modul (original) | 1 | 24,60 |
| 5 | GPS-Antenne Active Patch 25x25mm | 1 | 8,00 |
| 6 | BMI270 IMU (Bare IC) | 1 | 5,00 |
| 7 | BMP390 Barometer (Bare IC) | 1 | 3,50 |
| 8 | DS18B20 wasserdicht (Kuehlmittel) | 1 | 3,00 |
| 9 | Aftermarket Oeldrucksensor 0-5 bar | 1 | 25,00 |
| 10 | MicroSD-Slot + 16GB Karte | 1 | 7,20 |
| 11 | MP1584EN Buck-Modul + TVS SMAJ15A | 1+2 | 2,50 |
| 12 | HT7333 LDO | 1 | 0,50 |
| 13 | 18650 Backup-Akku 3400mAh | 1 | 6,00 |
| 14 | TP4056 Lademodul | 1 | 0,50 |
| 15 | MT3608 Boost-Modul | 1 | 0,80 |
| 16 | 1N5822 Schottky-Diode | 2 | 0,20 |
| 17 | TC4427 Gate-Driver | 1 | 1,50 |
| 18 | IRLZ44N MOSFET | 1 | 1,50 |
| 19 | PC817 Optokoppler | 2 | 1,00 |
| 20 | 1N5408 Freilaufdiode | 2 | 0,20 |
| 21 | Relais-Modul 4-Kanal SRD-12V | 1 | 4,00 |
| 22 | USB-C Buchse IP67 | 1 | 3,50 |
| 23 | FPC-Connector 12-polig | 1 | 0,50 |
| 24 | Passive Bauelemente (R, C, Ferrite, LED) | — | 2,55 |
| 25 | PCB 4-Layer ENIG (5 St., 1 verwendet) | 1 | 6,80 |
| 26 | PCB-Bestueckung (JLCPCB) | 1 | 20,00 |
| 27 | ASA-Filament 1kg | 1 | 25,00 |
| 28 | PETG-Filament 500g | 1 | 12,00 |
| 29 | Silikon-O-Ring 2mm | 1 | 2,00 |
| 30 | EPDM-Schaumstoff | 1 | 3,00 |
| 31 | PG7 + PG9 Kabelverschraubungen | 6 | 5,20 |
| 32 | Druckausgleichsmembran ePTFE | 1 | 8,00 |
| 33 | Kupferfolie EMV-Abschirmung | 1 | 8,00 |
| 34 | Conformal Coating UR | 1 | 12,00 |
| 35 | Acrylglas-Abdeckung 2mm | 1 | 2,00 |
| 36 | AMP Superseal Stecker (2+3+3+4-pol) | 4 | 10,00 |
| 37 | JST-SM Stecker 4-polig (GPS) | 1 | 1,50 |
| 38 | Ferritkerne (Klappferrit) | 4 | 3,20 |
| 39 | Sensor-Kabel abgeschirmt (3m) | 3 | 4,50 |
| 40 | Schrauben, Abstandshalter, Federscheiben | — | 2,96 |
| 41 | Kabelbinder, Schrumpfschlauch | — | 2,40 |
| 42 | Sicherung 5A + Halter | 1 | 1,00 |
| 43 | Loetpaste | 1 | 8,00 |
| 44 | Verbrauchsmaterial | — | 5,00 |
| — | **Zwischensumme** | | **284,01** |
| — | Contingency +15% | | 42,60 |
| — | **Gesamt Standard** | | **326,61** |

Hinweis: Die Standard-Konfiguration enthaelt bereits alle Komponenten
fuer den Dauereinsatz am Motorrad. Die Preise sind hoeher als in der
master-plan.md-Kalkulation, weil hier Einzelstueckpreise inkl. MwSt.
und Versandkosten-Pauschalen verwendet werden, waehrend der Master-Plan
teils Netto-Preise (B2B) ohne Versand verwendet.

### 3.3 Premium-Konfiguration (Vollausbau)

Alles aus Standard-Konfiguration plus: TPMS-Sensoren, LED-Status-Strip,
Hall-Sensor am Vorderrad, zusaetzliche Sensoren, professionelle
Gehaeuse-Fertigung, Premium-Komponenten (Edelstahlschrauben,
zusaetzliche Sicherungen). Diese Konfiguration ist fuer den
ambitionierten Nutzer, der alle Features nutzen moechte.

| Zusatz-Bauteil | Spezifikation | Menge | Preis (EUR) |
|---|---|---|---|
| TPMS-Sensoren | Reifendruck-/Temperatursensor, BLE, Paar | 1 Paar | 25,00 |
| WS2812B LED-Strip | Adressierbar, 4 LEDs, RGB | 1 | 3,00 |
| Hall-Sensor + Magnet | A3144 + Neodym 5mm | 1 Set | 2,00 |
| E-Paper-Display 2,9" | 296x128, zusaetzlich zu 2,13" | 1 | 15,00 |
| DS18B20 Umwelt-Temp. | Separater Sensor, extern | 1 | 3,00 |
| ATO Sicherungskasten | 6-fach, mit Deckel | 1 | 5,00 |
| Edelstahl-Schrauben | M3/M2,5 Sortiment | 1 | 3,00 |
| Premium-Kugelgelenk-Halterung | Neigungsverstellbar, 22mm | 1 | 8,00 |
| Reserve-Bauteile | 10% der kritischen Bauteile | — | 15,00 |
| — | **Zusatz-Summe** | | **79,00** |
| — | Contingency +15% | | 11,85 |
| — | **Gesamt Premium (inkl. Standard)** | | **417,46** |

### 3.4 Complete-Konfiguration (Produktifizierung)

Die Complete-Konfiguration entspricht dem Produktangebot "Transalp
Bordcomputer Kit PD06 Complete" (siehe docs/PRODUCT-CATALOG.md).
Sie besteht aus der Standard-Konfiguration plus CarPlay-Display und
Lenker-Controller. Dies ist die Vollausstattung fuer den Endkunden,
die ohne weitere Beschaffung montiert werden kann.

| Zusatz-Bauteil | Spezifikation | Menge | Preis (EUR) |
|---|---|---|---|
| CarPlay-Display EKIY 5 Zoll | 5 Zoll IPS, IPX7, Wireless CarPlay + Android Auto | 1 | 90,00 |
| Lenker-Controller SilverFox C2 | CNC-Alu, IP67, Joystick+Switch, M12, BLE+Kabel | 1 | 235,00 |
| Lenker-Halterung 22mm | Aluminium, schwingungsgeedaempft, fuer 5 Zoll Display | 1 | 15,00 |
| M12-Kabel 4-polig | Wasserdicht, IP67, 1,5m | 1 | 12,00 |
| — | Zusatz-Summe | | 352,00 |
| — | Contingency +15% | | 52,80 |
| — | Gesamt Complete (inkl. Standard) | | 731,41 |

Varianten-Uebersicht CarPlay/Controller:

Die Complete-Konfiguration kann mit unterschiedlichen CarPlay-Displays
und Controllern kombiniert werden. Die folgende Tabelle zeigt die
Preisauswirkung der verschiedenen Optionen:

| Variante | CarPlay-Display | Controller | Display-Preis | Controller-Preis | Gesamt Complete (EUR) |
|---|---|---|---|---|---|
| Standard | EKIY 5 Zoll (IPX7) | SilverFox C2 (IP67) | 90,00 | 235,00 | 731,41 |
| Budget | Jiuyin 5 Zoll (IPX7) | Custom PCB (I2C) | 65,00 | 25,00 | 474,41 |
| Premium | KEMIMOTO 5 Zoll (IP67, 700 nits) | SilverFox C2 (IP67) | 120,00 | 235,00 | 761,41 |
| Wireless | EKIY 5 Zoll (IPX7) | Chigee CGRC (BLE, CR2032) | 90,00 | 90,00 | 586,41 |

Hinweis zur Preisgestaltung: Die Materialkosten des Complete Kits
(Standard-Konfiguration) liegen bei ca. 406 EUR (ohne CarPlay/Controller).
Der Verkaufspreis von 549 EUR im Produktkatalog beinhaltet
Materialkosten, Arbeitsaufwand Montage/Test (80 EUR) und Gewinnmarge
(63 EUR). CarPlay-Display und Controller sind als separate Kits
preisein ausgewiesen (CarPlay-Upgrade-Kit: 129 EUR,
Lenker-Controller-Kit: ab 49 EUR).

---

## 4. Kosten-Analyse

### 4.1 BOM-Kosten pro Variante

| Variante | BOM-Kosten (EUR) | Contingency +15% | Gesamt (EUR) |
|---|---|---|---|
| Minimal | 69,85 | 10,48 | 80,33 |
| Standard | 284,01 | 42,60 | 326,61 |
| Premium | 363,01 | 54,45 | 417,46 |
| Complete (Standard+CarPlay+Controller) | 636,01 | 95,40 | 731,41 |

### 4.2 Tooling-Kosten

Tooling-Kosten sind einmalige Kosten fuer Werkzeug, Material und
Infrastruktur, die nicht pro Geraet anfallen, sondern vorab
investiert werden muessen.

| Posten | Beschreibung | Kosten (EUR) |
|---|---|---|
| Loetstation | Heissluft-Station mit Temperaturkontrolle | 40,00 |
| Loetpasten-Schablone | Fuer Custom PCB (JLCPCB, Edelstahl) | 8,00 |
| Isopropanol-Reiniger | 1 Liter, fuer PCB-Reinigung vor Coating | 5,00 |
| Pinsel / Spruehdose | Fuer Conformal Coating-Auftrag | 3,00 |
| Multimeter | Digital, fuer Spannungs- und Durchgangspruefung | 20,00 |
| Abisolierzange | Fuer Kabelverarbeitung | 10,00 |
| Crimpzangen-Set | Fuer AMP Superseal und JST-SM | 25,00 |
| SD-Kartenleser | USB, fuer Log-Auswertung | 5,00 |
| 3D-Drucker | FDM-Drucker fuer Gehaeuse (falls nicht vorhanden) | 200,00 |
| Schraubendreher-Set | Inbus 2,0/2,5/4,0mm | 8,00 |
| Heissluftpistole | Fuer Schrumpfschlauch | 15,00 |
| **Gesamt Tooling** | | **339,00** |

Hinweis: Der 3D-Drucker ist der groesste Posten. Falls bereits ein
3D-Drucker vorhanden ist, entfallen 200 EUR. Die restlichen
Tooling-Kosten betragen dann 139 EUR.

### 4.3 Gesamtkosten-Durchschuss bei 1, 5, 10 Stueck

Die folgende Tabelle zeigt die Gesamtkosten pro Geraet bei
verschiedenen Stueckzahlen. Hoeherere Stueckzahlen reduzieren
die Kosten durch:
- Rabatte bei Bauteil-Mengen (typisch 20-40% ab 10 Stueck)
- Geringere PCB-Kosten pro Stueck (5 Stueck sind Minimum bei JLCPCB)
- Geringere Versandkosten pro Geraet
- Bessere Ausnutzung des Filaments (1 kg ASA reicht fuer ~3 Gehaeuse)

| Variante | 1 Stueck | 5 Stueck | 10 Stueck |
|---|---|---|---|
| **Minimal** | | | |
| BOM | 69,85 | 55,88 | 48,90 |
| PCB/Perfboard | 2,00 | 1,60 | 1,40 |
| Versand gesamt | 15,00 | 25,00 | 35,00 |
| Versand pro Geraet | 15,00 | 5,00 | 3,50 |
| Gesamt pro Geraet | 86,85 | 62,48 | 53,80 |
| Contingency +15% | 13,03 | 9,37 | 8,07 |
| **Gesamt pro Geraet** | **99,88** | **71,85** | **61,87** |
| | | | |
| **Standard** | | | |
| BOM | 284,01 | 213,01 | 177,51 |
| PCB (5 St., anteilig) | 26,80 | 6,80 | 5,36 |
| Bestueckung | 20,00 | 16,00 | 14,00 |
| Versand gesamt | 25,00 | 40,00 | 55,00 |
| Versand pro Geraet | 25,00 | 8,00 | 5,50 |
| Gesamt pro Geraet | 355,81 | 243,81 | 202,37 |
| Contingency +15% | 53,37 | 36,57 | 30,36 |
| **Gesamt pro Geraet** | **409,18** | **280,38** | **232,73** |
| | | | |
| **Premium** | | | |
| BOM | 363,01 | 272,26 | 226,88 |
| PCB (5 St., anteilig) | 26,80 | 6,80 | 5,36 |
| Bestueckung | 20,00 | 16,00 | 14,00 |
| Versand gesamt | 30,00 | 45,00 | 60,00 |
| Versand pro Geraet | 30,00 | 9,00 | 6,00 |
| Gesamt pro Geraet | 439,81 | 302,06 | 252,24 |
| Contingency +15% | 65,97 | 45,31 | 37,84 |
| **Gesamt pro Geraet** | **505,78** | **347,37** | **290,08** |

Erlaeuterung zur Berechnung:
- Bei 1 Stueck: Alle Bauteile werden in Einzelmenge gekauft, PCB
  wird trotzdem als 5-Set bestellt (JLCPCB Minimum), Versand
  voll dem einzelnen Geraet zugeordnet.
- Bei 5 Stueck: PCB-Kosten werden auf 5 Geraete verteilt, Bauteile
  teilweise in 5er-Mengen gekauft (20% Rabatt), Versand auf 5
  Geraete verteilt.
- Bei 10 Stueck: Bauteile in 10er-Mengen (30% Rabatt), PCB in
  zwei 5-Set Bestellungen, Versand auf 10 Geraete verteilt.

### 4.4 Contingency-Analyse

Die Contingency von +15% deckt folgende Risiken ab:

| Risiko | Anteil | Beschreibung |
|---|---|---|
| Preisschwankungen | 5% | Wechselkursschwankungen (USD/EUR), saisonale Preiserhoehungen |
| Verfuegbarkeitsprobleme | 4% | Ersatz-Bestellung bei nicht verfuegbaren Bauteilen, teurere Alternative |
| Versand- und Zollkosten | 3% | Unerwartete Zollgebuehren bei China-Importen, Express-Versand |
| Beschaffung von Ersatzteilen | 2% | Beschädigte Bauteile, Fehlbestueckung, Nachbestellung |
| Fertigungsfehler | 1% | PCB-Fehler, Loetfehler, Fehldrucke beim 3D-Druck |

Bei Projekten mit hohem Anteil an China-Importen (China-Marktplatz, JLCPCB,
Distributor CN) kann die Contingency auf +20% erhoeht werden, da
Zollabwicklungen und verlaengerte Lieferzeiten haeufiger zu
Nachbestellungen fuehren.

### 4.5 Kosten-Verteilung nach Kategorie (Standard-Variante)

| Kategorie | Kosten (EUR) | Anteil |
|---|---|---|
| Display (TFT + E-Paper + Abdeckung) | 32,00 | 11,3% |
| GPS (Modul + Antenne) | 32,60 | 11,5% |
| Sensoren (IMU + Baro + Temp + Oeldruck) | 36,50 | 12,9% |
| Power (Buck + LDO + Akku + Ladung + Boost) | 11,50 | 4,1% |
| MCU + Speicher | 19,10 | 6,7% |
| MOSFET + Relais + Optokoppler | 8,40 | 3,0% |
| PCB (Fertigung + Bestueckung + Loetpaste) | 34,80 | 12,3% |
| Gehaeuse (Filament + Dichtungen + Coating) | 80,00 | 28,2% |
| Kabel + Stecker + Ferritkerne | 18,10 | 6,4% |
| Verbrauchsmaterial + Mechanik | 10,96 | 3,9% |
| **Gesamt** | **284,01** | **100%** |

Auffaelligkeit: Das Gehaeuse (28,2%) ist die teuerste Einzelkategorie.
Dies liegt an den Kosten fuer Conformal Coating (12 EUR), ASA-Filament
(25 EUR) und Kupferfolie (8 EUR). Bei Verwendung eines einfacheren
Gehaeuses (PETG, ohne Coating, ohne Kupferfolie) koennen ca. 40 EUR
eingespart werden, allerdings auf Kosten der Wasserdichtigkeit und
EMV-Tauglichkeit.

---

## 5. Risiko-Bewertung

### 5.1 Discontinued / Hard-to-Source Bauteile

**BMP388 -- OBSOLETE (Kritisch)**

Der BMP388 wird bei B2B-Distributor US #2 als "Obsolete" markiert (Stand Oktober
2026). Das bedeutet, dass das Bauteil nicht mehr produziert wird
und nur noch Lagerbestaende verfuegbar sind. Die bei B2B-Distributor US #2
angegebene Alternative ist der BMP390 (Part No. 828-BMP390CT-ND).

Der BMP390 ist der direkte Nachfolger des BMP388 und ist:
- Pin-kompatibel (10-WFLGA, 2x2mm)
- Interface-kompatibel (I2C / SPI, gleiche Register-Struktur)
- Verbesserte Spezifikation: ±0,03 hPa rel. Genauigkeit (vs.
  ±0,08 hPa beim BMP388), 0,25m Hoehenaufloesung
- Gleiche Spannungsversorgung (1,65V - 3,6V)
- Gleicher Stromverbrauch (3,4 µA bei 1 Hz)

EMPFEHLUNG: BMP390 statt BMP388 in der gesamten BOM verwenden.
Die Firmware-Bibliothek (BMP3xx-Library (Open-Source)) unterstuetzt beide Sensoren
mit derselben API. Keine Code-Aenderungen erforderlich.

**BMP388 Lagerbestaende (Stand Oktober 2026):**
- B2B-Distributor US #2: 0 auf Lager (Obsolete)
- B2B-Distributor US #1: geringe Restbestaende (pruefen)
- Distributor EU: nicht gelistet

**NEO-M9N -- Clone-Risiko (Hoch)**

Auf China-Marktplatz werden GPS-Module unter der Bezeichnung "NEO-M9N" ab
12 EUR angeboten. Diese sind haeufig Fakes:
- Echte u-blox NEO-M9N-00B Module kosten ab 20 EUR (Distributor EU netto)
- Clones verwenden oft den ATGM336H Chip und relabeln das Modul
- Die Konfigurationssoftware u-center erkennt keine echten
  u-blox-Module bei Fakes

EMPFEHLUNG: NEO-M9N ausschliesslich bei autorisierten Distributoren
(Distributor EU, B2B-Distributor US #1, B2B-Distributor US #2, Spezial-Distributor) kaufen. Bei China-Marktplatz nur von
Verkaeufern mit >98% positiver Bewertung und Verifizierung.

**ESP32-S3-WROOM-1 N8R2 -- Lieferengpaesse (Mittel)**

Bei B2B-Distributor US #2 betraegt die Vorlaufzeit (Standard Lead Time) 8 Wochen,
was auf Produktionsengpaesse beim MCU-Chiphersteller hindeutet.
Bei B2B-Distributor US #1 ist das Modul jedoch mit 3,60 USD (ca. 3,30 EUR netto)
sofort verfuegbar (Stand Oktober 2026). Das DevKitC-1 ist bei
beiden Distributoren mit >1.000 Stueck auf Lager.

EMPFEHLUNG: Modul bei B2B-Distributor US #1 bestellen (kuerzeste Lieferzeit).
Als Backup: DevKitC-1 (13,80 EUR) fuer Prototyp-Phase.

### 5.2 Bauteile mit langen Lieferzeiten (> 4 Wochen)

| Bauteil | Lieferzeit | Grund | Alternative |
|---|---|---|---|
| ESP32-S3-WROOM-1 N8R2 (B2B-Distributor US #2) | 8 Wochen | Produktionstrueckstand | B2B-Distributor US #1 (3-5 Tage), DevKitC-1 |
| NEO-M9N (bei Nicht-Verfuegbarkeit) | 4-6 Wochen | Import aus Asien | ATGM336H (sofort, 3 EUR), NEO-M8N |
| AMP Superseal Stecker | 2-3 Wochen | Spezial-Distributor | Deutch DTM (Elektronik-Distributor DE #1, 2-3 Tage) |
| JLCPCB PCB + Bestueckung | 5-8 Tage | Produktion + Versand aus China | PCBWay (aehnlich schnell), lokaler Dienstleister |
| China-Marktplatz Module (MP1584, TP4056, MT3608) | 7-21 Tage | Versand aus China | Online-Marktplatz Prime (2-3 Tage, +50% Preis) |

### 5.3 Substitute fuer kritische Bauteile

Die folgende Tabelle listet fuer jedes kritische Bauteil die verfuegbaren
Substitute mit ihrer Kompatibilitaet und den Vor- und Nachteilen.

| Krit. Bauteil | Substitut 1 | Substitut 2 | Kompatibilitaet |
|---|---|---|---|
| BMP390 (Baro) | BMP388 (obsolet!) | BMP280 (2 EUR, weniger genau) | Pin-kompatibel? BMP280 nein, BMP388 ja. SW: BMP3xx-Library (Open-Source) fuer BMP388/390, BMP280 braucht eigene Lib. |
| BMI270 (IMU) | MPU6050 (2 EUR, veraltet) | LSM6DS33 (5 EUR) | Pin-kompatibel? Nein. SW: eigene Treiber fuer jeden Sensor. |
| NEO-M9N (GPS) | NEO-M8N (15 EUR, 2 Konst.) | ATGM336H (3 EUR, 2 Konst.) | Pin-kompatibel? NEO-M8/M9 ja (LCC-Package). ATGM336H nein. SW: NMEA-Parser kompatibel, UBX nur u-blox. |
| ESP32-S3-WROOM-1 N8R2 | ESP32-S3-DevKitC-1 (13,80 EUR) | ESP32 Classic (nicht empfohlen) | DevKitC-1 ist groesser aber pin-kompatibel. Classic: kein BLE 5.0, weniger GPIO. |
| HT7333 LDO | AMS1117-3.3 (0,15 EUR) | MCP1700-3302E (1,00 EUR) | Pin-kompatibel? AMS1117 ja (SOT-89), MCP1700 ja. SW: keine Aenderung. |
| MP1584EN Buck | LM2596 (2 EUR) | TPS5430 (6 EUR) | Pin-kompatibel? Nein. SW: keine Aenderung. Mechanisch: gleiche Modulgroesse bei LM2596. |
| IRLZ44N MOSFET | IRF520 (1,50 EUR, nicht Logic) | AO3400 SMD (0,10 EUR, nur 30V/5A) | Pin-kompatibel? IRF520 ja (TO-220), AO3400 nein (SOT-23). SW: keine Aenderung. |

### 5.4 Single-Source vs. Multi-Source Analyse

| Bauteil | Source-Anzahl | Risiko | Massnahme |
|---|---|---|---|
| ESP32-S3-WROOM-1 N8R2 | 2 (B2B-Distributor US #1, B2B-Distributor US #2) | Mittel | DevKitC-1 als Backup |
| NEO-M9N | 3 (Distributor EU, B2B-Distributor US #1, B2B-Distributor US #2) | Mittel | ATGM336H als Backup |
| BMI270 | 2 (B2B-Distributor US #1, B2B-Distributor US #2) | Mittel | MPU6050 als Backup |
| BMP390 | 2 (B2B-Distributor US #1, B2B-Distributor US #2) | Mittel | BMP280 als Backup |
| MP1584EN Modul | 3+ (China-Marktplatz, Online-Marktplatz, Elektronik-Distributor DE #1) | Gering | Keine Massnahme noetig |
| HT7333 | 3+ (Elektronik-Distributor DE #1, B2B-Distributor US #1, Distributor CN) | Gering | AMS1117 als Backup |
| TC4427 | 2 (B2B-Distributor US #1, B2B-Distributor US #2) | Mittel | MIC4427 als Backup |
| ASA-Filament | 3+ (Online-Marktplatz, Filament-versand) | Gering | PETG als Prototyp-Backup |
| AMP Superseal | 1-2 (Spezial-Distributor, Online-Marktplatz) | Hoch | Deutch DTM als Backup |
| JLCPCB PCB | 2 (JLCPCB, PCBWay) | Gering | Lokaler Dienstleister als Backup |
| Conformal Coating UR | 2 (Online-Marktplatz, Spezial-Distributor) | Gering | Acryl-Coating als Backup |

---

## 6. Beschaffungs-Strategie

### 6.1 Empfohlene Bestellreihenfolge

Die Bestellreihenfolge ist so gewaehlt, dass Bauteile mit der laengsten
Lieferzeit zuerst bestellt werden. Dies minimiert die Gesamtzeit bis
zum Aufbau.

**Phase 1: Langlaufende Bestellungen (Woche 1)**

Diese Bauteile haben Lieferzeiten von > 1 Woche und sollten
unverzueglich bestellt werden.

1. PCB-Fertigung bei JLCPCB: 5-8 Tage fuer 4-Layer mit ENIG und
   Bestueckung. Sofort am Anfang bestellen, da alle anderen Schritte
   vom PCB abhaengen.
2. NEO-M9N GPS-Modul bei Distributor EU: 3-7 Tage. Originalmodul, nicht bei
   China-Marktplatz bestellen (Clone-Risiko).
3. ESP32-S3-WROOM-1 N8R2 bei B2B-Distributor US #1: 3-5 Tage. Sofort verfuegbar,
   aber bei Bestellung > 10 Stueck laengere Lieferzeit.
4. BMI270 und BMP390 bei B2B-Distributor US #1: 5-10 Tage. Original-ICs, nicht bei
   China-Marktplatz (Clones, falsche Specs).
5. TC4427 Gate-Driver bei B2B-Distributor US #1: 5-10 Tage.
6. AMP Superseal Stecker bei Spezial-Distributor: 2-3 Wochen.

**Phase 2: Mittellaufende Bestellungen (Woche 2)**

Diese Bauteile haben Lieferzeiten von 2-5 Tagen.

7. Passive Bauelemente (Widerstaende, Kondensatoren, Ferrite-Beads)
   bei Elektronik-Distributor DE #1: 2-3 Tage.
8. Standard-Halbleiter (HT7333, IRLZ44N, PC817, 1N5822, 1N5408,
   Zener, TVS) bei Elektronik-Distributor DE #1: 2-5 Tage.
9. Module (MP1584EN, TP4056, MT3608) bei China-Marktplatz: 7-14 Tage.
   Alternative: Online-Marktplatz Prime (+50% Preis, aber 2-3 Tage).
10. USB-C IP67 Buchse bei Elektronik-Distributor DE #1: 2-4 Tage.
11. Sensor-Kabel abgeschirmt und JST-SM Stecker bei Online-Marktplatz: 2-4 Tage.
12. Ferritkerne bei Online-Marktplatz: 2-4 Tage.

**Phase 3: Kurzlaufende Bestellungen (Woche 3)**

Diese Bauteile sind schnell verfuegbar (1-3 Tage).

13. Displays (TFT 3,5" IPS, E-Paper 2,13") bei Display-Modul-Hersteller/Online-Marktplatz:
    3-7 Tage.
14. DS18B20, NTC, Hall-Sensor bei Elektronik-Distributor DE #1/Online-Marktplatz: 2-4 Tage.
15. Aftermarket Oeldrucksensor bei Online-Marktplatz/Spezial-Distributor: 5-10 Tage.
16. 18650 Akku bei Online-Marktplatz: 2-4 Tage.
17. SD-Karte 16GB bei Online-Marktplatz: 2-3 Tage.
18. ASA- und PETG-Filament bei Online-Marktplatz: 2-4 Tage.
19. Kupferfolie, Conformal Coating, Acrylglas bei Online-Marktplatz: 2-4 Tage.
20. Kabelbinder, Schrumpfschlauch, Schrauben bei Online-Marktplatz/Baumarkt:
    1-3 Tage.

### 6.2 Sammelbestellung vs. Einzelbestellung

**Sammelbestellung (empfohlen fuer Prototyp-Phase):**

Vorteile:
- Reduzierte Versandkosten (1 Versandpauschale statt 10+).
- Alle Bauteile kommen gleichzeitig an.
- Bessere Uebersicht ueber den Bestellstatus.

Nachteile:
- Ein nicht verfuegbares Bauteil verzoegert ggf. die gesamte
  Bestellung (bei einem Lieferanten).
- Keine teilweise Auslieferung moeglich.

Empfohlene Lieferanten-Gruppierung:

| Lieferant | Bauteile | Versand (EUR) | Lieferzeit |
|---|---|---|---|
| JLCPCB | PCB + Bestueckung | 15,00 | 5-8 Tage |
| B2B-Distributor US #1 | ESP32-S3, BMI270, BMP390, TC4427, Ferrite-Beads | 6,90 | 3-5 Tage |
| Distributor EU | NEO-M9N | 8,00 | 3-7 Tage |
| Elektronik-Distributor DE #1 | Passive, Standard-ICs, Kabelverschraubungen, Abstandshalter | 4,95 | 2-3 Tage |
| China-Marktplatz | MP1584, TP4056, MT3608, Hall-Sensor | 0,00 (gratis) | 7-21 Tage |
| Online-Marktplatz | Displays, Akku, Filament, Coating, Kupferfolie, Kabel | 0,00 (Prime) | 1-3 Tage |
| Spezial-Distributor | AMP Superseal, Oeldrucksensor, Druckausgleichsmembran | 6,90 | 3-10 Tage |
| — | **Gesamt Versand** | **41,75** | |

**Einzelbestellung (empfohlen fuer Ersatzbeschaffung):**

Bei nicht verfuegbaren Bauteilen oder dringenden Ersatzbedarf ist
die Einzelbestellung bei Online-Marktplatz Prime (1-2 Tage Lieferung) die
schnellste Option, auch wenn die Preise 30-100% ueber dem
Fachhandel liegen.

### 6.3 Zoll- und Versandkosten

**JLCPCB / Distributor CN (China-Import):**

Bei Bestellungen unter 150 EUR Warenwert fallen keine Zollgebuehren
an (Zollfreigrenze der EU). Es fallen jedoch Einfuhrumsatzsteuer
(19%) ab 22 EUR Warenwert an, die vom Versanddienstleister
eingezogen wird. Bei JLCPCB wird die Einfuhrumsatzsteuer in der
Regel vom Versanddienstleister (DHL Express) abgewickelt.

Bei Warenwert ueber 150 EUR faellt Zoll an (Rate je nach
Warencode, typisch 0-4% fuer elektronische Bauteile).

Empfehlung: Bestellungen bei JLCPCB/Distributor CN unter 150 EUR halten,
um Zollgebuehren zu vermeiden. Bei hoeherem Warenwert: auf
mehrere Bestellungen aufteilen (z.B. PCB separat von Bauteilen).

**China-Marktplatz (China-Import):**

China-Marktplatz-Bestellungen unter 150 EUR sind zollfrei. Die
Einfuhrumsatzsteuer wird bereits im Checkout von China-Marktplatz
erhoben (IOSS-Verfahren), so dass keine Nachzahlung beim
Zoll anfaellt. Lieferzeit 7-21 Tage.

**B2B-Distributor US #1 / B2B-Distributor US #2 / Distributor EU (EU-Versand):**

B2B-Distributor US #1 versandt aus den Niederlanden, B2B-Distributor US #2 aus den USA
(mit EU-Lager), Distributor EU aus Polen. Bei B2B-Konten (USt-IdNr.)
werden Preise netto berechnet. Versandkosten: 6,90-8,00 EUR.
Keine Zollgebuehren innerhalb der EU.

**Elektronik-Distributor DE #1 / Elektronik-Distributor DE #2 / Online-Marktplatz (Inland):**

Versand aus Deutschland, 1-3 Tage Lieferzeit. Versandkosten:
4,95 EUR (Elektronik-Distributor DE #1), gratis bei Elektronik-Distributor DE #2 ab 25 EUR, gratis bei
Online-Marktplatz Prime.

### 6.4 Empfohlene Lagerhaltung (Reserve-Bauteile)

Fuer den Dauereinsatz am Motorrad sollten folgende Reserve-Bauteile
vorgehalten werden, da Ausfaelle durch Vibration, Feuchtigkeit oder
Spannungsspitzen wahrscheinlich sind:

| Bauteil | Reserve-Menge | Grund | Kosten (EUR) |
|---|---|---|---|
| ESP32-S3-WROOM-1 N8R2 | 1 | Single-Point-of-Failure, nicht vor Ort verfuegbar | 5,40 |
| NEO-M9N GPS-Modul | 1 | Spezialbauteil, lange Lieferzeit | 24,60 |
| BMI270 | 1 | SMD, vibrationsempfindlich | 5,00 |
| BMP390 | 1 | SMD, fehleranfaellig bei Loeftung | 3,50 |
| MP1584EN Modul | 2 | China-Import, lange Lieferzeit | 1,00 |
| HT7333 LDO | 2 | Ueberspannung kann Zerstoerung verursachen | 1,00 |
| TVS-Diode SMAJ15A | 2 | Schuetzt gegen Ueberspannung, kann durchbrennen | 1,00 |
| PC817 Optokoppler | 2 | Zündimpulse koennen IC beschaedigen | 1,00 |
| IRLZ44N MOSFET | 1 | Induktive Lasten koennen MOSFET beschaedigen | 1,50 |
| MicroSD-Karte 16GB | 1 | Verschleissteil, begrenzte Schreibzyklen | 6,00 |
| Silikon-O-Ring | 2 | Verschleissteil, altert mit UV-Einfluss | 4,00 |
| PG7 Kabelverschraubung | 2 | Mechanisches Verschleissteil | 1,60 |
| — | **Gesamt Reserve** | | **56,60** |

Die Reserve-Bauteile sollten in einem trockenen, kuehlen Schrank
gelagert werden. SMD-Bauteile in versiegelten Tueten mit
Silica-Gel (Feuchtigkeitsschutz, MSL-Klassifizierung beachten).

### 6.5 Lieferanten-Empfehlung pro Bauteil-Kategorie

| Kategorie | Primaere Quelle | Sekundaere Quelle | Bemerkung |
|---|---|---|---|
| MCU / Spezial-ICs | B2B-Distributor US #1 | B2B-Distributor US #2 | Original, garantierte Qualitaet |
| GPS-Modul | Distributor EU | B2B-Distributor US #1 | Distributor EU hat guenstigste Preise fuer u-blox in der EU |
| Passive Bauelemente | Elektronik-Distributor DE #1 | Distributor CN | Elektronik-Distributor DE #1 schnell, Distributor CN guenstig bei Mengen |
| Module (Buck, Boost, Charger) | China-Marktplatz | Online-Marktplatz | China-Marktplatz am guenstigsten, Online-Marktplatz am schnellsten |
| Standard-Halbleiter | Elektronik-Distributor DE #1 | B2B-Distributor US #1 | Elektronik-Distributor DE #1 schnell und guenstig fuer Standard-Teile |
| Kabel / Stecker / Verschraubungen | Online-Marktplatz | Elektronik-Distributor DE #1 | Online-Marktplatz fuer schnelle Beschaffung |
| Filament / 3D-Druck-Material | Online-Marktplatz | Filament-versand | Grosse Auswahl, schnelle Lieferung |
| PCB-Fertigung | JLCPCB | PCBWay | JLCPCB guenstigster Anbieter mit Bestueckung |
| Spezial-Mechanik (AMP Superseal, Oeldruck) | Spezial-Distributor | Online-Marktplatz | Spezial-Distributor hat groessere Auswahl |
| Werkzeug / Verbrauchsmaterial | Online-Marktplatz | Baumarkt | Online-Marktplatz fuer Bequemlichkeit, Baumarkt fuer Sofortbedarf |
| CarPlay-Display | China-Marktplatz | Online-Marktplatz | EKIY/Jiuyin ueber China-Marktplatz, KEMIMOTO ueber Online-Marktplatz |
| Lenker-Controller | SilverFox (silverfoxcontrollers.com) | SportsBikeShop | SilverFox C2 direkt, Chigee CGRC ueber SportsBikeShop |
| Custom Controller PCB | JLCPCB | China-Marktplatz | PCB bei JLCPCB, Bauteile bei China-Marktplatz |

---

## 7. Quellen

- ESP32-S3-WROOM-1-N8R2: B2B-Distributor US #1 (3,60 USD ≈ 3,30 EUR netto),
  B2B-Distributor US #2 (5,88 USD ≈ 5,40 EUR). Beide Stand Oktober 2026.
- NEO-M9N-00B: Distributor EU (20,32 EUR netto / 24,59 EUR brutto),
  Spezial-Distributor (21,77 EUR). Stand Oktober 2026.
- BMI270: B2B-Distributor US #1 (3,71 USD ≈ 3,40 EUR netto), B2B-Distributor US #2 (4,23 USD).
  Stand Oktober 2026.
- BMP388: B2B-Distributor US #2 Status "Obsolete". BMP390 als direkter Ersatz,
  B2B-Distributor US #2 (2,43 GBP ≈ 2,85 EUR netto). Stand Oktober 2026.
- BMP390: B2B-Distributor US #1 / B2B-Distributor US #2, 3,50 EUR netto. Stand Oktober 2026.
- MP1584EN Modul: China-Marktplatz (0,48-1,26 EUR), LaskaKit (1,31 EUR).
  Stand Oktober 2026.
- JLCPCB PCB-Fertigung: 4-Layer ab 8 USD / 5 Stueck. Stand 2026.
- Display-Modul-Hersteller 3.5" IPS Display: 480x320, SPI, Online-Marktplatz / Display-Modul-Hersteller.
  Stand Oktober 2026.
- CarPlay-Display EKIY 5 Zoll: China-Marktplatz (Suchlink), ca. 90 EUR. Stand Oktober 2026.
- CarPlay-Display Jiuyin 5 Zoll: China-Marktplatz (Suchlink), ca. 65 EUR. Stand Oktober 2026.
- CarPlay-Display KEMIMOTO 5 Zoll: Online-Marktplatz, ca. 120 EUR. Stand Oktober 2026.
- CarPlay-Display Chigee AIO-5 EVO: Chigee (chigee.com), ca. 370 EUR. Stand Oktober 2026.
- Lenker-Controller SilverFox C2: SilverFox (silverfoxcontrollers.com), ca. 235 EUR. Stand Oktober 2026.
- Lenker-Controller Chigee CGRC Multi: SportsBikeShop (sportsbikeshop.co.uk), ca. 90 EUR. Stand Oktober 2026.
- docs/research/02-hardware-deep-dive.md: Komponenten-Auswahl und
  Spezifikationen
- docs/research/09-master-plan.md: BOM mit Budget/Premium-Optionen
- docs/PCB-LAYOUT.md: PCB-relevante Bauteile und Kosten
- docs/CASE-DESIGN.md: Gehaeuse-Materialien und Mechanik
- docs/PRODUCT-CATALOG.md: Produktkatalog mit 5 Produktvarianten
- docs/SHOPPING-LIST.md: Einkaufsliste mit China-Marktplatz-Links

## 8. Manufacturing Readiness & Verification

> Stand: 20. Oktober 2026 · Verifiziert durch: Wolfgang (@wolfgang) · Issue: TOT-30

Diese Sektion dokumentiert den Fertigungs-Reifegrad der BOM fuer die
PCB-Produktifizierung (Sprint 2). Sie ergaenzt die sourcing-Strategie aus
Abschnitt 2 mit Hersteller-Teilenummern (MPN), damit ein Contract
Manufacturer (CM) die Bauteile eindeutig identifizieren und bestellen kann.

### 8.1 Lieferanten-Anonymisierung

Die BOM und `docs/SHOPPING-LIST.md` verwenden bewusst Pseudonyme
("B2B-Distributor US #1", "Elektronik-Distributor DE #1", "China-Marktplatz",
"Online-Marktplatz", "Spezial-Distributor", "Distributor EU/CN",
"Display-Modul-Hersteller"). Dadurch werden keine einzelnen Händler bevorzugt
(oeffentliches Repo). Fuer eine CM-Bestellung muessen diese auf reale
Lieferanten gemappt werden — empfohlene reale Quellen je Kategorie:

| Pseudonym                  | Reale Empfehlung (Beispiel)                                  |
|----------------------------|--------------------------------------------------------------|
| B2B-Distributor US #1/#2   | Mouser, Digi-Key                                            |
| Elektronik-Distributor DE #1 | Conrad, Voelkner, BerryBase                               |
| China-Marktplatz           | LCSC                                                        |
| Online-Marktplatz          | Generic online marketplace                                  |
| Spezial-Distributor        | Spezialisierte Motorrad-Elektronik-Haendler                |
| Distributor EU             | Mouser EU, Farnell                                          |
| Distributor CN             | LCSC                                                        |
| Display-Modul-Hersteller  | Waveshare, BuyDisplay (dealextreme)                          |

### 8.2 MPN-Referenztabelle (Kernbauteile)

Hersteller-Teilenummern fuer die SMT-Bestueckung. Diese MPN sind die
Bestellgrundlage fuer Pick&Place bei JLCPCB/LCSC-Assembly.

| Bauteil                  | Hersteller   | MPN                        | Footprint      | BOM-Preis |
|--------------------------|--------------|----------------------------|----------------|-----------|
| ESP32-S3-WROOM-1         | Espressif    | ESP32-S3-WROOM-1-N8R2*     | Module         | 5,40 EUR  |
| IMU                      | OEM          | BMI270                     | LGA-14         | 5,00 EUR  |
| Barometer                | OEM          | BMP390 (BMP388 obsolete)   | LGA-10         | 3,50 EUR  |
| GPS-Modul                | u-blox        | NEO-M9N-00B               | Module         | 24,60 EUR |
| Buck-Regler              | Monolithic Power | MP1584EN                | SOIC-8 / Modul | 1,50 EUR  |
| LDO 3,3V                 | Holtek       | HT7333-3.3                | SOT-89         | 0,50 EUR  |
| Li-Ion-Lader             | NanJing Top Power | TP4056                 | SOP-8 / Modul  | 0,50 EUR  |
| Boost-Regler             | Aerosemi     | MT3608                     | SOT-23-6 / Modul | 0,80 EUR |
| Optokoppler              | Sharp        | PC817C                     | DIP-4 / SMD    | 0,50 EUR  |
| Gate-Driver              | Microchip    | TC4427VOA                  | SOIC-8         | 1,50 EUR  |
| MOSFET                   | Infineon     | IRLZ44N                    | TO-220         | 1,50 EUR  |
| Schottky-Diode           | Multicomp/Vis | 1N5822                   | DO-201AD       | 0,10 EUR  |
| TVS-Diode                | Littelfuse   | SMAJ15A                    | SMA            | 0,50 EUR  |
| Freilaufdiode            | Multicomp    | 1N5408                     | DO-201AD       | 0,10 EUR  |
| Kuehlmitteltemp-Sensor   | Maxim (ADI)  | DS18B20                    | TO-92 wasserd. | 3,00 EUR  |
| Ferrite Bead             | Murata       | BLM18AG601SN1              | 0805           | 0,20 EUR  |

*Konsolidierung noetig: BOM nennt N8R2 (8MB Flash/2MB PSRAM), PINMAP.md und
HARDWARE-TEST-BENCH.md nennen N16R8 (16MB Flash/8MB Octal PSRAM) — siehe §8.4.

### 8.3 Preis-Verifizierungsstatus

| Kategorie               | Preise vorhanden | Datum-Stempel | URL/Link |
|-------------------------|:---:|:---:|:---:|
| Kern-Elektronik         | ja  | Oktober 2026 | nein (Pseudonym) |
| Fahrzeug-Sensoren       | ja  | Oktober 2026 | nein (Pseudonym) |
| Power-Management        | ja  | Oktober 2026 | nein (Pseudonym) |
| Passive Bauelemente     | ja  | Oktober 2026 | nein (Pseudonym) |
| PCB-Fertigung           | ja  | 2026         | jlcpcb.com       |
| Gehaeuse/Mechanik       | ja  | Oktober 2026 | nein (Pseudonym) |
| CarPlay/Controller      | ja  | Oktober 2026 | teils (chigee/silverfox/sportsbikeshop) |

Preise sind als Einzelstueckpreise (Stand Oktober 2026) verifiziert und aktuell.
Direkte Produkt-URLs fehlen bewusst (Pseudonym-Konvention); die
MPN-Tabelle §8.2 ermoeglicht die eindeutige Bestellung bei jedem Distributor.

### 8.4 Offene Diskrepanzen (vor CM-Handoff loesen)

1. **MCU-Variante N8R2 vs. N16R8** — BOM/SHOPPING-LIST: N8R2 (8MB/2MB);
   `PINMAP.md`/`HARDWARE-TEST-BENCH.md`: N16R8 (16MB/8MB Octal PSRAM).
   Beeinflusst Flash/PSRAM-Konfiguration und Bestellung. Klärung mit @klaus.
2. **Barometer BMP388 (obsolete) → BMP390** — bereits als Ersatz hinterlegt,
   Footprint und I2C-Adresse (0x76/0x77) verifizieren.
3. **IMU MPU6050 vs. BMI270** — Firmware (`board_config.h`) referenziert
   MPU6050 (0x68), BOM/Layout verwenden BMI270. Treiber und I2C-Adresse
   abstimmen. Siehe Pinmap-Verifikationsbericht
   `docs/research/pinmap-firmware-verification.md`.
4. **GPS AT6558 vs. NEO-M9N** — PINMAP.md nennt AT6558, Firmware/BOM NEO-M9N.
   Baudrate und Protokoll (NMEA/UBX) abklaeren.
5. **Referenz-Designatoren (R1, C1, U1 ...)** fehlen — werden im KiCad-Schema
   vergeben (PCB-LAYOUT.md §9 Schritt 1). Fuer die CM-Pick&Place-Datei
   (BOM/Position/Centroid) noetig.
6. **MSL/Verpackung** (Tape&Reel, Feuchtigkeitsklasse) — fuer JLCPCB-Assembly
   bei den Modulen (WROOM-1, NEO-M9N) anzugeben.

### 8.5 Empfehlung

Preise und Bezugsquellen sind verifiziert und aktuell (Oktober 2026). Fuer den
fertigen CM-Handoff muessen (a) die Lieferanten-Pseudonyme auf reale Distributoren
gemappt, (b) die MCU/Sensor-Diskrepanzen geklaert und (c) die Referenzdesignatoren
im KiCad-Schema vergeben werden. Die MPN-Tabelle §8.2 schließt die wesentliche
Luecke ("welches Bauteil genau?").

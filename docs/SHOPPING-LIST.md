# Einkaufsliste — Standard-Konfiguration

> Stand: Oktober 2026
> Variante: Standard (Dauereinsatz am Motorrad, ~327 EUR)
> Alle Preise in EUR inkl. MwSt., Einzelstückpreise

---

## Kern-Elektronik

| # | Bauteil | Spezifikation | Menge | Preis (EUR) | Quelle |
|---|---|---|---|---|---|
| 1 | ESP32-S3-WROOM-1 N8R2 | 8MB Flash, 2MB PSRAM, BLE 5.0, 45 GPIO | 1 | 5,40 | Mouser |
| 2 | NEO-M9N GPS-Modul | 4-Konstellation GNSS, 25Hz, UART, original | 1 | 24,60 | TME |
| 3 | GPS-Antenne | Active Patch 25x25mm, Magnetfuß, 28dB Gain | 1 | 8,00 | Online-Marktplatz |
| 4 | TFT-Display 3,5" IPS | 480x320, SPI, 1000 nits Backlight | 1 | 18,00 | Waveshare/Online-Marktplatz |
| 5 | E-Paper-Display 2,13" | 250x122, monochrom, SSD1680/UC8151D | 1 | 12,00 | Waveshare/Online-Marktplatz |
| 6 | BMI270 IMU | 6-DoF (Accel+Gyro), I2C/SPI, 2KB FIFO | 1 | 5,00 | Mouser |
| 7 | BMP390 Barometer | ±0,03 hPa, 0,25m Auflösung, I2C/SPI | 1 | 3,50 | Mouser |
| 8 | MicroSD-Slot + 16GB Karte | Push-Push SMD, Class 10, FAT32 | 1 | 7,20 | Online-Marktplatz/Reichelt |
| 9 | USB-C Buchse IP67 | Panel-Mount, wasserdicht, mit Dichtung | 1 | 3,50 | Reichelt |
| 10 | FPC-Connector 12-polig | 0,5mm Pitch, für Display-Anbindung | 1 | 0,50 | LCSC/Digi-Key |

## Fahrzeug-Sensoren (PD06)

| # | Bauteil | Spezifikation | Menge | Preis (EUR) | Quelle |
|---|---|---|---|---|---|
| 11 | DS18B20 wasserdicht | Kühlmitteltemp, 1-Wire, ±0,5°C, -55 bis +125°C, Edelstahlrohr | 1 | 3,00 | Online-Marktplatz |
| 12 | Aftermarket Öldrucksensor | Analog 0-5 bar, 10-180 Ohm, M10x1, wasserdicht | 1 | 25,00 | Online-Marktplatz |
| 13 | Hall-Sensor A3144 + Magnet | Unipolar, 4,5-24V, TO-92, + Neodym 5mm | 1 | 2,00 | Reichelt |
| 14 | PC817 Optokoppler | 1-Kanal, Transistor-Ausgang, DIP-4 (für CDI/Zündung) | 2 | 1,00 | Reichelt |
| 15 | Kraftstoffstand-Messung | Spannungsteiler 10k/10k + MCP6001 OpAmp | 1 | 1,00 | Reichelt |
| 16 | Bordspannungs-Messung | Spannungsteiler 100k/33k, 1%, 0805 SMD | 1 | 0,10 | Reichelt |

## Power-Management

| # | Bauteil | Spezifikation | Menge | Preis (EUR) | Quelle |
|---|---|---|---|---|---|
| 17 | MP1584EN Buck-Modul | 12V→5V, 1,5MHz, 3A, 4,5-28V Input, 22x15mm | 1 | 1,50 | AliExpress |
| 18 | HT7333 LDO | 5V→3,3V, 250mA, 30µVrms Rauschen, SOT-89 | 1 | 0,50 | Reichelt |
| 19 | 18650 Li-Ion Akku | 3400mAh, 3,7V, geschützt mit PCB (Backup) | 1 | 6,00 | Online-Marktplatz |
| 20 | TP4056 Lademodul | 5V Input, 1A Ladestrom, USB-C, mit Schutz | 1 | 0,50 | AliExpress |
| 21 | MT3608 Boost-Modul | 3,7V→5V, 2A, bis 28V Output, 17x11mm | 1 | 0,80 | AliExpress |
| 22 | TVS-Diode SMAJ15A | 15V Standoff, 24,4V Clamp, 400W, SMA | 2 | 1,00 | Reichelt |
| 23 | Schottky-Diode 1N5822 | 3A, 40V, DO-201AD, für Power-Path ORing | 2 | 0,20 | Reichelt |
| 24 | Sicherung 5A | Flachsicherung ATO/ATC + Inline-Halter | 1 | 1,00 | Reichelt |
| 25 | Elko 100µF | 25V, Low-ESR, SMD Tantal oder THT | 1 | 0,30 | Reichelt |

## Steuerung

| # | Bauteil | Spezifikation | Menge | Preis (EUR) | Quelle |
|---|---|---|---|---|---|
| 26 | Relais-Modul 4-Kanal | SRD-12V, 10A, mit Optokoppler, Active-LOW | 1 | 4,00 | Online-Marktplatz |
| 27 | IRLZ44N MOSFET | N-Channel, 55V, 25A, Logic-Level, TO-220 | 1 | 1,50 | Reichelt |
| 28 | TC4427 Gate-Driver | 1,5A, 2-Kanal, 4,5-18V, Non-Inverting, SOIC-8 | 1 | 1,50 | Mouser |
| 29 | 1N5408 Freilaufdiode | 3A, 1000V, DO-201AD, für induktive Lasten | 2 | 0,20 | Reichelt |

## Passive Bauelemente

| # | Bauteil | Spezifikation | Menge | Preis (EUR) | Quelle |
|---|---|---|---|---|---|
| 30 | Widerstände 0805 | 10k, 33k, 1k, 22, 33, 4,7k Ohm, 1%, 0,125W | 20 | 0,40 | Reichelt |
| 31 | Kondensatoren 0805 | 100nF X7R, 25V, Decoupling | 15 | 0,75 | Reichelt |
| 32 | Bulk-Kondensatoren | 10µF X5R 0805, 22µF X5R 1206 | 4 | 0,60 | Reichelt |
| 33 | Ferrite-Beads | 600 Ohm @ 100MHz, 2A, 0805, BLM18AG601 | 4 | 0,80 | Mouser |
| 34 | LED Status | Grün oder Blau, 0805 SMD, 3,2V, 20mA | 1 | 0,10 | Reichelt |
| 35 | Zener-Diode 3,3V | 500mW, SOT-23 oder DO-35 | 2 | 0,40 | Reichelt |

## PCB

| # | Bauteil | Spezifikation | Menge | Preis (EUR) | Quelle |
|---|---|---|---|---|---|
| 36 | PCB 4-Layer | 80x50mm, FR4, 1,6mm, ENIG, 2oz Cu L1/L4 (5 Stk.) | 1 | 6,80 | JLCPCB |
| 37 | PCB-Bestückung | SMD-Bestückung durch JLCPCB, ~50 Bauteile | 1 | 20,00 | JLCPCB |
| 38 | Lötpaste | Sn42/Bi57/Ag1, bleifrei, 50g Spritze | 1 | 8,00 | Online-Marktplatz |

## Gehäuse & Mechanik

| # | Bauteil | Spezifikation | Menge | Preis (EUR) | Quelle |
|---|---|---|---|---|---|
| 39 | ASA-Filament | 1,75mm, 1kg, UV-stabil, hell (Gehäuse) | 1 | 25,00 | Online-Marktplatz |
| 40 | PETG-Filament | 1,75mm, 500g (Innenrahmen/Display-Rahmen) | 1 | 12,00 | Online-Marktplatz |
| 41 | Silikon-O-Ring | 2mm Cordstärke, Shore 60A, -40 bis +200°C | 1 | 2,00 | Online-Marktplatz |
| 42 | EPDM-Schaumstoff | Selbstklebend, 5mm, Sekundärdichtung | 1 | 3,00 | Online-Marktplatz |
| 43 | PG7 Kabelverschraubung | PA66, IP67, NBR-O-Ring, 3-6,5mm Ø | 4 | 3,20 | Reichelt |
| 44 | PG9 Kabelverschraubung | PA66, IP67, NBR-O-Ring, 4-8mm Ø | 2 | 2,00 | Reichelt |
| 45 | Druckausgleichsmembran | ePTFE, IP68, M6x0,75, -40 bis +125°C | 1 | 8,00 | Online-Marktplatz |
| 46 | Kupferfolie | Selbstklebend, 0,05mm, 50mm breit, 1m (EMV) | 1 | 8,00 | Online-Marktplatz |
| 47 | Conformal Coating | Polyurethan, UR-Typ, 50ml Sprühdose | 1 | 12,00 | Online-Marktplatz |
| 48 | Acrylglas-Platte | 2mm, transparent, 100x80mm (Display-Abdeckung) | 1 | 2,00 | Baumarkt |
| 49 | Abstandshalter M3 | 10mm, Sechskant, Messing, Innengewinde | 4 | 1,20 | Reichelt |
| 50 | Schrauben M3x8 + M3x12 | Senkkopf, Edelstahl, Inbus | 8 | 0,40 | Baumarkt |
| 51 | Schrauben M2,5x6 | Senkkopf, Edelstahl, Inbus (Display-Rahmen) | 4 | 0,20 | Baumarkt |
| 52 | Federscheiben M3 | DIN 127B, Edelstahl, Vibrationssicherung | 8 | 0,16 | Reichelt |
| 53 | 18650-Akkuhalter | PCB-Mount, für 1x 18650 | 1 | 0,50 | Reichelt |

## Kabel & Stecker

| # | Bauteil | Spezifikation | Menge | Preis (EUR) | Quelle |
|---|---|---|---|---|---|
| 54 | Sensor-Kabel | 4-adrig, AWG24, abgeschirmt (S/UTP), pro Meter | 3 | 4,50 | Online-Marktplatz |
| 55 | AMP Superseal 2-polig | IP67, 3A (12V Bordnetz) | 1 | 2,50 | Spezialversand |
| 56 | AMP Superseal 3-polig | IP67, 3A (ADC-Sensoren) | 2 | 5,00 | Spezialversand |
| 57 | AMP Superseal 4-polig | IP67, 3A (Relais-Steuerung) | 1 | 2,50 | Spezialversand |
| 58 | JST-SM Stecker 4-polig | Wasserdicht, IP65 (GPS) | 1 | 1,50 | Online-Marktplatz |
| 59 | Ferritkerne | Klappferrit, Material 43/73, >100 Ohm @ 100MHz | 4 | 3,20 | Online-Marktplatz |
| 60 | Kabelbinder | 100mm, schwarz, UV-beständig | 20 | 0,40 | Baumarkt |
| 61 | Schrumpfschlauch | 3mm, 1m, schwarz, Polyolefin, 2:1 | 1 | 2,00 | Online-Marktplatz |

---

## Zusammenfassung

| Kategorie | Preis (EUR) |
|---|---|
| Kern-Elektronik | 88,30 |
| Fahrzeug-Sensoren | 32,10 |
| Power-Management | 11,80 |
| Steuerung | 7,20 |
| Passive Bauelemente | 3,05 |
| PCB | 34,80 |
| Gehäuse & Mechanik | 87,66 |
| Kabel & Stecker | 21,60 |
| **Zwischensumme** | **286,51** |
| Contingency +15% | 42,98 |
| **Gesamt (Standard)** | **329,49** |

---

## Bestellreihenfolge

### Phase 1: Langlaufend (Woche 1)
1. PCB-Fertigung bei JLCPCB (5-8 Tage)
2. NEO-M9N bei TME (3-7 Tage, kein AliExpress — Clone-Risiko)
3. ESP32-S3-WROOM-1 bei Mouser (3-5 Tage)
4. BMI270 + BMP390 bei Mouser (5-10 Tage)
5. TC4427 bei Mouser (5-10 Tage)
6. AMP Superseal bei Spezialversand (2-3 Wochen)

### Phase 2: Mittellaufend (Woche 2)
7. Passive Bauelemente bei Reichelt (2-3 Tage)
8. Standard-Halbleiter bei Reichelt (2-5 Tage)
9. Module bei AliExpress: MP1584EN, TP4056, MT3608 (7-14 Tage)
10. USB-C IP67 bei Reichelt (2-4 Tage)
11. Sensor-Kabel + JST-SM bei Online-Marktplatz (2-4 Tage)
12. Ferritkerne bei Online-Marktplatz (2-4 Tage)

### Phase 3: Kurzlaufend (Woche 3)
13. Displays bei Waveshare/Online-Marktplatz (3-7 Tage)
14. DS18B20, Hall-Sensor bei Reichelt/Online-Marktplatz (2-4 Tage)
15. Öldrucksensor bei Online-Marktplatz (5-10 Tage)
16. 18650 Akku bei Online-Marktplatz (2-4 Tage)
17. SD-Karte bei Online-Marktplatz (2-3 Tage)
18. ASA/PETG-Filament bei Online-Marktplatz (2-4 Tage)
19. Kupferfolie, Conformal Coating, Acrylglas bei Online-Marktplatz (2-4 Tage)
20. Kabelbinder, Schrumpfschlauch, Schrauben (1-3 Tage)

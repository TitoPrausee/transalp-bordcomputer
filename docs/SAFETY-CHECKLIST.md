# Safety & Compliance Checklist

> Stand: Oktober 2026
> Sprint: 2 — Integration & Production Readiness
> Ticket: TOT-32
> Fahrzeug: XL600V Transalp (PD06), Baujahr 1986-1999

---

## 1. Zweck

Diese Checklist stellt sicher, dass der TRANSALP Bordcomputer den
Sicherheits- und Compliance-Anforderungen fuer den Einsatz auf einem
Motorrad entspricht. Sie deckt die Bereiche Elektrik, Thermik,
Vibration und Witterung ab und muss vor jedem Feldtest abgezeichnet
werden.

**Freigabe:** Die Checklist muss von @wolfgang (Hardware) und @klaus
(CTO/Firmware) signiert werden, bevor das System auf dem Motorrad
getestet wird.

---

## 2. Elektrische Sicherheit

### 2.1 Bordnetz

| ID | Pruefpunkt | Sollwert | Methode | Status |
|----|-----------|---------|---------|--------|
| E-01 | Bordspannung Bereich | 11.5V – 15.0V | Multimeter an Zigaretteanzuender | [ ] |
| E-02 | Spannungseinbruch beim Anlassen | > 10.5V (min 2s Ueberbrueckung) | Oszilloskop, LiPo-Puffer | [ ] |
| E-03 | Ueberspannungsschutz | Abschaltung bei > 15.5V | Pruefung mit einstellbarem Netzteil | [ ] |
| E-04 | Verpolungsschutz | Diode/Schottky am Eingang | Durchgangspruefung | [ ] |
| E-05 | Sicherung Hauptsicherung | 5A flink, nah an Batterie | Sichtpruefung | [ ] |
| E-06 | DC/DC-Wandler Effizienz | > 85% bei 12V->5V | Messung mit Last | [ ] |
| E-07 | LDO 3.3V Rauschen | < 50mV pp | Oszilloskop am 3.3V-Ausgang | [ ] |
| E-08 | Stromaufnahme (Betrieb) | < 300mA @ 12V | Amperemeter | [ ] |
| E-09 | Stromaufnahme (Deep Sleep) | < 5mA @ 12V | Amperemeter | [ ] |
| E-10 | LiPo-Pufferakku | 3.7V 2000mAh, Ueberbrueckung > 2s | Entlade-Test | [ ] |

### 2.2 Verkabelung

| ID | Pruefpunkt | Sollwert | Methode | Status |
|----|-----------|---------|---------|--------|
| E-11 | Kabelquerschnitt Hauptversorgung | >= 1.5 mm² | Messschieber | [ ] |
| E-12 | Kabelquerschnitt Sensoren | >= 0.5 mm² | Messschieber | [ ] |
| E-13 | Steckverbindungen vergossen | Schrumpfschlauch+Kleber | Sichtpruefung | [ ] |
| E-14 | Kabelbaum gegen Vibration gesichert | Kabelbinder alle 10cm | Sichtpruefung | [ ] |
| E-15 | Keine blanken Leitungen | Alle Krimpverbindungen isoliert | Sichtpruefung | [ ] |
| E-16 | Massepunkt gemeinsamer Sternpunkt | Am Rahmen, lackfreier Kontakt | Durchgangsmessung | [ ] |
| E-17 | Abschirmlung Sensorleitungen | Einseitig an Masse | Durchgangsmessung | [ ] |

### 2.3 Sensoren

| ID | Pruefpunkt | Sollwert | Methode | Status |
|----|-----------|---------|---------|--------|
| E-18 | RPM Optokoppler isoliert | Galvanische Trennung bestaetigt | Durchgangspruefung | [ ] |
| E-19 | Kuehlmittel-NTC | 10k NTC, im Schlauch isoliert | Messung bei 25C und 90C | [ ] |
| E-20 | Kraftstoff-Schwimmer | 0-5V Signal, linear | Pruefung am ADC | [ ] |
| E-21 | IMU (MPU6050) I2C-Adresse | 0x68 (AD0=GND) | I2C-Scan | [ ] |
| E-22 | GPS UART-Pegel | 3.3V TTL, nicht RS232 | Oszilloskop | [ ] |

---

## 3. Thermische Sicherheit

### 3.1 Betriebsbedingungen

| ID | Pruefpunkt | Sollwert | Methode | Status |
|----|-----------|---------|---------|--------|
| T-01 | ESP32-S3 CPU-Temperatur | < 85C im Dauerbetrieb | Diagnose DTC_D0402 | [ ] |
| T-02 | Gehaeuse-Innentemperatur | < 70C bei 50C Umgebung | Infrarot-Thermometer | [ ] |
| T-03 | Display-TFT-Temperatur | < 60C (Spezifikation ILI9341) | Infrarot-Thermometer | [ ] |
| T-04 | DC/DC-Wandler-Temperatur | < 100C (Thermal Shutdown 150C) | Infrarot-Thermometer | [ ] |
| T-05 | Kuehlmittel-Ueberwachung | Warnung bei > 110C, Kritisch bei > 120C | DTC S0102/S0103 | [ ] |

### 3.2 Waermemanagement

| ID | Pruefpunkt | Sollwert | Methode | Status |
|----|-----------|---------|---------|--------|
| T-06 | Belueftungsoeffnungen | Vorhanden, nicht verdeckt | Sichtpruefung | [ ] |
| T-07 | Waermeleitpad ESP32-S3 | Auf Gehaeuseboden geklebt | Sichtpruefung (geoeffnet) | [ ] |
| T-08 | Abstand heisse Bauteile | > 5mm zwischen DC/DC und ESP | Sichtpruefung | [ ] |
| T-09 | Kaltstart bei -10C | System startet, Display reagiert | Klimakammer/Kuehlschrank | [ ] |
| T-10 | Heissstart bei +50C | System laeuft stabil 30min | Warmluft/Sonnenschein | [ ] |
| T-11 | Thermal-Throttling ESP32-S3 | CPU-Frequenz reduziert bei > 80C | Log-Auswertung | [ ] |

---

## 4. Vibrations- und Stossfestigkeit

### 4.1 Mechanische Befestigung

| ID | Pruefpunkt | Sollwert | Methode | Status |
|----|-----------|---------|---------|--------|
| V-01 | Gehaeuse-Befestigung | 4x M5 am Lenkerhalter | Sichtpruefung, Zugprobe | [ ] |
| V-02 | Gehaeuse-Material | ASA 3D-Druck, Wandstaerke >= 3mm | Messschieber | [ ] |
| V-03 | Dichtungsring Gehaeusedeckel | EPDM O-Ring oder Schaumstoff | Sichtpruefung | [ ] |
| V-04 | PCB-Befestigung | 4x Abstandsbolzen M2.5 | Sichtpruefung | [ ] |
| V-05 | Display-Befestigung | Klebstoff/Doppelband, rahmengefuellt | Sichtpruefung | [ ] |
| V-06 | Akku-Befestigung | Klettband oder Schaumstoff | Sichtpruefung | [ ] |

### 4.2 Vibrations-Test

| ID | Pruefpunkt | Sollwert | Methode | Status |
|----|-----------|---------|---------|--------|
| V-07 | Vibrations-Test (Sinus) | 5-500 Hz, 5g, 1h pro Achse | Shaker-Tisch | [ ] |
| V-08 | Stosstest (Drop) | 1m Fall auf Beton (gehaeuse leer) | Fall-Test | [ ] |
| V-09 | Motorvibration (Feld) | 30 min Fahrt bei 6000 RPM | Probefahrt | [ ] |
| V-10 | Schotterpiste | 10 km bei 30 km/h | Probefahrt | [ ] |
| V-11 | Stecker-Sicherheit | Kein Loesen bei Vibration | Nach Test pruefen | [ ] |
| V-12 | Display-Pixel-Fehler | Keine toten Pixel nach Vibration | Sichtpruefung | [ ] |
| V-13 | SD-Karte loest sich nicht | Click-Mechanismus intakt | Nach Test pruefen | [ ] |

---

## 5. Witterungs- und Umweltschutz

### 5.1 Wasserdichtigkeit

| ID | Pruefpunkt | Sollwert | Methode | Status |
|----|-----------|---------|---------|--------|
| W-01 | IP-Schutzklasse | IP65 (Spritzwasser) min., IP67 (Tauchbad) opt. | Zertifikat/Test | [ ] |
| W-02 | Gehaeusedichtung | Vollstaendig aufliegend, keine Luecken | Sichtpruefung | [ ] |
| W-03 | Kabeldurchfuehrungen | Kabelverschraubungen mit Dichtung | Sichtpruefung | [ ] |
| W-04 | Regentest (30 min) | Kein Wassereintritt, keine Funktionseinschraenkung | Beregnungsanlage | [ ] |
| W-05 | Kondensat-Test | Kein Kondensat im Gehaeuse nach 24h | Sichtpruefung (geoeffnet) | [ ] |
| W-06 | Display lesbar bei Regen | Keine Beeintraechtigung der Anzeige | Sichtpruefung bei Regen | [ ] |

### 5.2 Staub

| ID | Pruefpunkt | Sollwert | Methode | Status |
|----|-----------|---------|---------|--------|
| W-07 | Staubdichtigkeit | IP6x (staubdicht) | Staubkammer-Test | [ ] |
| W-08 | Display-Spaltmasse | Kein Staub in Spalten | Sichtpruefung nach Test | [ ] |
| W-09 | Tasten/Controller | Kein Staub unter Tasten | Funktionstest nach Test | [ ] |

### 5.3 UV- und Temperaturwechsel

| ID | Pruefpunkt | Sollwert | Methode | Status |
|----|-----------|---------|---------|--------|
| W-10 | UV-Bestaendigkeit Gehaeuse | ASA-Material (UV-stabil) | Materialzertifikat | [ ] |
| W-11 | UV-Bestaendigkeit Kabel | Schwarze Isolierung (UV-stabil) | Materialzertifikat | [ ] |
| W-12 | Temperaturwechsel (-10C bis +50C) | 5 Zyklen, keine Risse/Verformung | Klimakammer | [ ] |
| W-13 | Kondensat bei Wechsel | Keine Funktionseinschraenkung | Funktionstest nach Wechsel | [ ] |

---

## 6. Funktionale Sicherheit

### 6.1 Notfall-Verhalten

| ID | Pruefpunkt | Sollwert | Methode | Status |
|----|-----------|---------|---------|--------|
| F-01 | Spannungsausfall-Ueberbrueckung | System laeuft > 2s bei 0V Bordnetz | Netzteil-Test | [ ] |
| F-02 | Brownout-Detection | ESP32-S3 Brownout bei 3.0V, DTC D0502 | Log-Auswertung | [ ] |
| F-03 | Watchdog-Reset | System startet innerhalb 5s neu | Test-Reset ausloesen | [ ] |
| F-04 | Crash-Erkennung | IMU |accel| > 3g -> DTC, Log, Shutdown-Timer | Simulator-Test | [ ] |
| F-05 | Not-Aus (Display) | Anzeige im Display bei kritischen Warnungen | DTC-Trigger-Test | [ ] |
| F-06 | OTA-Fail-Safe | Rollback bei fehlgeschlagenem Update | OTA-Test mit bad Image | [ ] |

### 6.2 Datenintegritaet

| ID | Pruefpunkt | Sollwert | Methode | Status |
|----|-----------|---------|---------|--------|
| F-07 | SD-Karte Schreibfehler | DTC C0104 wird gesetzt | SD-Karte entfernen waehrend Betrieb | [ ] |
| F-08 | NVS-Persistenz | Konfiguration ueberlebt Reboot | Konfiguration schreiben, neu starten | [ ] |
| F-09 | GPS-Track-Integritaet | GPX-Datei ohne Luecken nach Abbruch | Fahrt abbrechen, Datei pruefen | [ ] |
| F-10 | BLE-Verbindungsabbruch | Reconnect innerhalb 5s | Bluetooth trennen | [ ] |

---

## 7. Pruef- und Freigabeprotokoll

### 7.1 Vor jeder Feldtest-Serie

| Pruefpunkt | Ja/Nein | Datum | Pruefer |
|-----------|---------|-------|---------|
| Elektrische Sicherheit (Kapitel 2) | [ ] | | |
| Thermische Sicherheit (Kapitel 3) | [ ] | | |
| Vibrationsfestigkeit (Kapitel 4) | [ ] | | |
| Witterungsschutz (Kapitel 5) | [ ] | | |
| Funktionale Sicherheit (Kapitel 6) | [ ] | | |

### 7.2 Freigabe

| Rolle | Name | Unterschrift | Datum |
|-------|------|-------------|-------|
| Hardware-Ingenieur | @wolfgang | | |
| CTO / Firmware | @klaus | | |
| QA / Test Engineer | @greta | | |

---

## 8. Referenzen

- `docs/FIELDTEST-PROTOCOL.md` — Detailliertes Feldtest-Protokoll
- `docs/HARDWARE-TEST-BENCH.md` — Bench-Test-Aufbau
- `docs/HARDWARE.md` — Hardware-Spezifikation
- `docs/CASE-DESIGN.md` — Gehaeuse-Design
- `docs/SECURITY-ANALYSIS.md` — Sicherheitsanalyse
- `firmware/components/diagnostics/include/diagnostics.h` — DTC-Codes
- `firmware/test/integration/README.md` — Integration-Test-Suite

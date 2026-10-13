# Recherche: Transalp 600 (PD06) Bordcomputer

> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986–2000
> Rein mechanische Maschine: kein CAN-Bus, kein OBD-II, keine ECU
> Alle Fahrzeugdaten werden über direkte Sensor-Anzapfung erfasst

## Dokumente

| # | Datei | Thema | Wörter |
|---|---|---|---|
| 01 | [`01-sensoren-fahrzeugdaten.md`](01-sensoren-fahrzeugdaten.md) | Drehzahl, Tacho, Kühlmitteltemp, Kraftstoffstand, Öldruck, Ganganzeige, Bordspannung — direkte Sensor-Anzapfung an der PD06 mit Schaltungen, Signalcharakteristik und ESP32-Code | ~5.750 |
| 02 | [`02-hardware-deep-dive.md`](02-hardware-deep-dive.md) | ESP32-S3, GPS-Module, E-Paper/TFT-Displays, IMU, Barometer, Power-Management (Buck/Boost/LDO), SD-Logging, Bluetooth — Modulnamen, Preise, Pin-Belegungen | ~1.100 |
| 03 | [`03-software-stack.md`](03-software-stack.md) | ESP-IDF vs. Arduino, FreeRTOS Task-Design, GPS-Treiber, Display-Treiber, Sensor-Treiber, Daten-Logging, BLE-Stack, OTA-Update, NVS-Config, Open-Source-Projekte | ~7.400 |
| 04 | [`04-pd06-technische-daten.md`](04-pd06-technische-daten.md) | Motor-/Getriebedaten (Leistung, Drehmoment, Übersetzungen), Elektrik (Lichtmaschine, Regler, Batterie), Fahrwerk, Tank/Kraftstoff, Service-Manual-Quellen, Community-Wissen | ~5.000 |
| 05 | [`05-gehaeuse-einbau-emv.md`](05-gehaeuse-einbau-emv.md) | 3D-Druck-Gehäuse, Wasserdichtigkeit (IP-Ratings), Vibrationen/Stöße, Temperaturbereich, EMV-Schutz (Zündfunken, Abschirmung), Lastabwurf-Schutz, Stromversorgung im Motorrad-Umfeld | ~5.600 |
| 06 | [`06-feature-roadmap.md`](06-feature-roadmap.md) | Vollständige Feature-Roadmap: Core-Features, Smart-Features (BLE+Smartphone), Safety (Crash-Detection, SOS), Tour-Features, Diagnose, Nice-to-have, Daten-Export, App-Konzept | ~6.300 |
| 07 | [`07-display-navigation-smartphone.md`](07-display-navigation-smartphone.md) | Multi-Display-Architektur, Smartphone-Integration per BLE (CarPlay-Alternative), Turn-by-Turn Navigation UI, Companion-App-Konzept, GATT-Service-Design | ~2.400 |

**Gesamt: ~33.500 Wörter** Forschungsmaterial

## Wichtige Erkenntnisse

### Fahrzeug
- Die PD06 hat **kein OBD-II, keinen CAN-Bus, keine ECU** — alle Daten müssen über eigene Sensoren erfasst werden
- Drehzahl: Pickup-Signal an der CDI abgreifen (Optokoppler) oder Induktivsensor am Zündkabel
- Tacho: mechanische Welle — alternativ GPS-Speed oder Hall-Sensor am Vorderrad
- Kühlmitteltemp: zusätzlicher Sensor im Kühlschlauch (KTY81/PT1000/NTC)
- Kraftstoffstand: Widerstandsgeber im Tank (5-90Ω, Honda-typisch)
- Öldruck: Schalter (ON/OFF) oder analoger Sensor nachrüsten

### Hardware
- ESP32-S3 als Kernplattform (Dual-Core, BLE, PSRAM, TWAI, ausreichend GPIO)
- E-Paper für Instrumente (sonnenlesbar, low power) + TFT für Navigation (dynamisch, Farbe)
- Power: 12V Bordnetz → Buck → 5V → LDO → 3.3V, LiPo-Backup für Power-Loss

### Software
- ESP-IDF mit FreeRTOS (nicht Arduino) für volle Kontrolle
- Modulare Task-Architektur: GPS, Display, Sensoren, Logger, BLE, Power
- OTA-Update über BLE mit Dual-Partition-Rollback

### Smartphone-Integration
- Kein echtes CarPlay (MFi-Lizenz nötig) — eigene Companion-App als Alternative
- App nutzt native Maps-API (iOS MapKit / Android Google Maps)
- BLE-GATT-Protokoll: Navigation, Media, Phone, Weather, Telemetry Services
- Turn-by-Turn: großer Pfeil + Distanz + Straßenname auf Display

## Steuerung / Write
- PD06 hat keinen CAN-Bus → klassische Relais-Schaltung für Zubehör
- Relais-Module für: Zusatzlicht, Heizgriffe, USB-Ladung
- ⚠️ Kein Eingriff in sicherheitsrelevante Systeme (Bremsen, Fahrwerk)

## Quellen
Alle Dokumente enthalten Quell-Links zu Foren, Service-Manuals, GitHub-Repos und Datenblättern.

# Recherche: Transalp 600 (PD06) Bordcomputer

> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986–2000
> Rein mechanische Maschine: kein CAN-Bus, kein OBD-II, keine ECU
> Alle Fahrzeugdaten werden über direkte Sensor-Anzapfung erfasst

## Uebersicht

Dieses Verzeichnis enthaelt das gesamte Forschungsmaterial fuer das
Transalp 600 Bordcomputer-Projekt. Die Dokumente decken alle Aspekte
des Projekts ab: von der Sensor-Anzapfung an der PD06 ueber die
Hardware-Auswahl bis hin zur Software-Architektur und Feature-Roadmap.

Die Recherche bildet die Grundlage fuer alle nachfolgenden Design-,
Implementierungs- und Test-Entscheidungen. Jedes Dokument ist
selbstaendig lesbar, verweist aber auf verwandte Themen.

## Dokumente

| # | Datei | Thema | Woerter |
|---|---|---|---|
| 01 | [`01-sensoren-fahrzeugdaten.md`](01-sensoren-fahrzeugdaten.md) | Drehzahl, Tacho, Kuehlmitteltemp, Kraftstoffstand, Oeldruck, Ganganzeige, Bordspannung — direkte Sensor-Anzapfung an der PD06 mit Schaltungen, Signalcharakteristik und ESP32-Code | ~5.750 |
| 02 | [`02-hardware-deep-dive.md`](02-hardware-deep-dive.md) | ESP32-S3, GPS-Module, E-Paper/TFT-Displays, IMU, Barometer, Power-Management (Buck/Boost/LDO), SD-Logging, Bluetooth — Modulnamen, Preise, Pin-Belegungen | ~6.600 |
| 03 | [`03-software-stack.md`](03-software-stack.md) | ESP-IDF vs. Arduino, FreeRTOS Task-Design, GPS-Treiber, Display-Treiber, Sensor-Treiber, Daten-Logging, BLE-Stack, OTA-Update, NVS-Config, Open-Source-Projekte | ~7.400 |
| 04 | [`04-pd06-technische-daten.md`](04-pd06-technische-daten.md) | Motor-/Getriebedaten (Leistung, Drehmoment, Uebersetzungen), Elektrik (Lichtmaschine, Regler, Batterie), Fahrwerk, Tank/Kraftstoff, Service-Manual-Quellen, Community-Wissen | ~5.000 |
| 05 | [`05-gehaeuse-einbau-emv.md`](05-gehaeuse-einbau-emv.md) | 3D-Druck-Gehaeuse, Wasserdichtigkeit (IP-Ratings), Vibrationen/Stoesse, Temperaturbereich, EMV-Schutz (Zuendfunken, Abschirmung), Lastabwurf-Schutz, Stromversorgung im Motorrad-Umfeld | ~5.600 |
| 06 | [`06-feature-roadmap.md`](06-feature-roadmap.md) | Vollstaendige Feature-Roadmap: Core-Features, Smart-Features (BLE+Smartphone), Safety (Crash-Detection, SOS), Tour-Features, Diagnose, Nice-to-have, Daten-Export, App-Konzept | ~6.300 |
| 07 | [`07-display-navigation-smartphone.md`](07-display-navigation-smartphone.md) | Multi-Display-Architektur, Smartphone-Integration per BLE (CarPlay-Alternative), Turn-by-Turn Navigation UI, Companion-App-Konzept, GATT-Service-Design | ~2.400 |
| 08 | [`08-feature-realization-matrix.md`](08-feature-realization-matrix.md) | Feature-Realisierungs-Matrix: Datenlage zu Machbarkeit, Priorisierung und Aufwandsschaetzung aller Features | ~6.900 |
| 09 | [`09-master-plan.md`](09-master-plan.md) | Master-Plan: Umsetzungs-Roadmap mit Kosten-Nutzen-Analyse, BOM, 3 Ausbau-Stufen, 5-Phasen-Plan (10 Wochen), DTC-System, Support-Schnittstellen, QS-Strategie, Risikomatrix | ~9.800 |
| 10 | [`10-ui-ux-design.md`](10-ui-ux-design.md) | UI/UX Design-Konzept: Visual Language, Farbschema, Typografie, Animations-Konzept, Screen-Layouts, Interaction-Design | ~8.300 |
| 11 | [`11-firmware-architecture.md`](11-firmware-architecture.md) | Firmware-Architektur: Rendering-Engine, Animations-Framework, BLE-Stack, Task-Modell, Event-Loop, Modulare Komponenten-Struktur | ~9.400 |
| 12 | [`12-diagnose-support.md`](12-diagnose-support.md) | Diagnose, Support und Wartbarkeit: Reparaturfreundliche Architektur, DTC-System, Self-Test, Kalibrierung, Remote-Support, Wartungs-Intervalle | ~7.600 |
| 13 | [`13-connectivity-app.md`](13-connectivity-app.md) | Connectivity und Companion-App: Nahtlose Smartphone-Integration, BLE-Protokoll, App-Architektur, Screen-Konzepte, Mock-Implementierung | ~7.900 |

**Gesamt: ~89.500 Woerter** Forschungsmaterial

## Wichtige Erkenntnisse

### Fahrzeug
- Die PD06 hat **kein OBD-II, keinen CAN-Bus, keine ECU** — alle Daten muessen ueber eigene Sensoren erfasst werden
- Drehzahl: Pickup-Signal an der CDI abgreifen (Optokoppler) oder Induktivsensor am Zuendkabel
- Tacho: mechanische Welle — alternativ GPS-Speed oder Hall-Sensor am Vorderrad
- Kuehlmitteltemp: zusaetzlicher Sensor im Kuehlschlauch (KTY81/PT1000/NTC)
- Kraftstoffstand: Widerstandsgeber im Tank (5-90 Ohm, Honda-typisch)
- Oeldruck: Schalter (ON/OFF) oder analoger Sensor nachruesten

### Hardware
- ESP32-S3 als Kernplattform (Dual-Core, BLE, PSRAM, TWAI, ausreichend GPIO)
- E-Paper fuer Instrumente (sonnenlesbar, low power) + TFT fuer Navigation (dynamisch, Farbe)
- Power: 12V Bordnetz → Buck → 5V → LDO → 3.3V, LiPo-Backup fuer Power-Loss

### Software
- ESP-IDF mit FreeRTOS (nicht Arduino) fuer volle Kontrolle
- Modulare Task-Architektur: GPS, Display, Sensoren, Logger, BLE, Power
- OTA-Update ueber BLE mit Dual-Partition-Rollback

### Smartphone-Integration
- Kein echtes CarPlay (MFi-Lizenz noetig) — eigene Companion-App als Alternative
- App nutzt native Maps-API (mobile BS A / mobile BS B)
- BLE-GATT-Protokoll: Navigation, Media, Phone, Weather, Telemetry Services
- Turn-by-Turn: grosser Pfeil + Distanz + Strassenname auf Display

## Steuerung / Write
- PD06 hat keinen CAN-Bus → klassische Relais-Schaltung fuer Zubehoer
- Relais-Module fuer: Zusatzlicht, Heizgriffe, USB-Ladung
- Kein Eingriff in sicherheitsrelevante Systeme (Bremsen, Fahrwerk)

## Dokument-Struktur

Jedes Forschungsdokument folgt einem einheitlichen Aufbau:
1. **Einleitung** — Problemstellung und Kontext
2. **Technische Analyse** — detaillierte Untersuchung mit Schaltungen, Code, Tabellen
3. **Entscheidung** — begruendete Wahl mit Alternativen-Vergleich
4. **Implementierungs-Hinweise** — konkrete Tipps fuer die Umsetzung
5. **Quellenverzeichnis** — Links zu Foren, Datenblaettern, Repos

## Quellen
Alle Dokumente enthalten Quell-Links zu Foren, Service-Manuals, Repos und Datenblaettern.

## Recherche-Methodik

Die Forschungsdokumente wurden in einem iterativen Prozess erstellt:

1. **Phase 0a — Fahrzeuganalyse**: Zunaechst wurden alle technischen Daten
   der PD06 gesammelt: Motor, Getriebe, Elektrik, Fahrwerk. Dabei wurde
   klar bestaetigt, dass die PD06 keine elektronische Steuerung hat — kein
   CAN-Bus, kein OBD-II, keine ECU. Alle Fahrzeugdaten muessen ueber
   direkte Sensor-Anzapfung erfasst werden. Dies ist die zentrale
   architektonische Randbedingung fuer das gesamte Projekt.

2. **Phase 0b — Hardware-Recherche**: Basierend auf den Fahrzeugdaten
   wurde die Mikrocontroller-Plattform evaluiert. Die Wahl fiel auf den
   ESP32-S3 wegen Dual-Core, BLE 5, PSRAM-Unterstuetzung und ausreichend
   GPIO-Pins. Zusaetzlich wurden Display-Technologien (E-Paper fuer
   Instrumente, TFT fuer Navigation), GPS-Module, IMU, Barometer und
   Power-Management-Komponenten untersucht.

3. **Phase 0c — Software-Architektur**: Die Software-Entscheidung fiel
   auf ESP-IDF mit FreeRTOS statt Arduino, um volle Kontrolle ueber
   Task-Scheduling, Speicher-Management und Treiber-Integration zu
   haben. Die Task-Architektur wurde modular entworfen: GPS, Display,
   Sensoren, Logger, BLE und Power laufen als eigenstaendige FreeRTOS-Tasks
   mit klar definierten Kommunikationspfaden ueber einen Event-Bus.

4. **Phase 0d — Feature-Definition**: In der Feature-Roadmap wurden alle
   moeglichen Funktionen kategorisiert: Core-Features (Speed, RPM, Temp,
   Fuel, GPS, Logging), Smart-Features (Navigation, Media, Phone per BLE),
   Safety-Features (Crash-Detection, SOS, Diebstahlwarnung) und
   Tour-Features (Track-Recording, Statistiken, Segment-Timing).

5. **Phase 0e — Realisierungs-Matrix**: Jedes Feature wurde auf
   Machbarkeit, Aufwand und PD06-Relevanz geprueft. Die Matrix dient als
   Grundlage fuer die Priorisierung und Phasen-Planung.

## Abhaengigkeiten zwischen Dokumenten

Die Forschungsdokumente bauen aufeinander auf:

- **01 (Sensoren)** definiert die Signalquellen, die in **11 (Firmware)**
  und **09 (Master-Plan)** weiterverarbeitet werden
- **02 (Hardware)** legt die Komponenten fest, die in **05 (Gehaeuse)**
  und **09 (Master-Plan)** verbaut werden
- **03 (Software-Stack)** und **11 (Firmware-Architektur)** beschreiben
  die Implementierungsebene, die in **12 (Diagnose)** um
  Wartbarkeits-Aspekte erweitert wird
- **06 (Feature-Roadmap)** und **08 (Realisierungs-Matrix)** definieren
  den Funktionsumfang, der in **10 (UI/UX)** und **13 (Connectivity)**
  in Benutzeroberflaeche und App-Integration uebersetzt wird
- **07 (Display/Navigation)** verbindet Hardware-Display-Auswahl mit
  Software-Rendering und Smartphone-Integration
- **04 (Technische Daten)** ist die Referenz fuer alle PD06-spezifischen
  Spezifikationen und wird von allen anderen Dokumenten zitiert

## Weiterfuehrende Hinweise

Die Forschungsdokumente sind als Living Documents konzipiert. Bei neuen
Erkenntnissen aus dem Feldtest oder aus Community-Feedback werden sie
aktualisiert. Die Versionierung erfolgt ueber das Git-Repository, sodass
Aenderungen jederzeit nachvollziehbar sind.

Besonderer Wert wurde auf die Beruecksichtigung der spezifischen
Eigenschaften der PD06 gelegt: die mechanische Tacho-Welle, das
CDI-Zaendsystem, die Lichtmaschine mit Regler/Gleichrichter und der
Widerstandsgeber im Tank. Diese PD06-spezifischen Details unterscheiden
das Projekt von generischen Motorrad-Bordcomputer-Projekten und sind
grundlegend fuer eine korrekte Implementierung.

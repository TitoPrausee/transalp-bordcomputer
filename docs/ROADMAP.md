# Projekt-Roadmap: Transalp Bordcomputer

> Zuletzt aktualisiert: 13.10.2026
> Status: Aktiv — 6 Worker + 1 Projektmanager (PM) auf Mac mini

## Projektziel
Vollständiger Open-Source Bordcomputer für die Honda Transalp 600 (PD06).
ESP32-S3 basiert, mit GPS, Multi-Display, Fahrzeug-Sensoren, BLE-Smartphone-Integration,
Diagnose-System und Premium-UI/UX mit Animationen.

## Phasen

### Phase 0: Recherche (ABGESCHLOSSEN)
- [x] PD06 Sensoren (Drehzahl, Tacho, Temp, Kraftstoff, Öldruck, Gang)
- [x] Hardware Deep Dive (ESP32, GPS, Display, Power)
- [x] Software-Stack (ESP-IDF, FreeRTOS, Treiber, BLE, OTA)
- [x] PD06 Technische Daten (Motor, Getriebe, Elektrik)
- [x] Gehäuse, Einbau, EMV
- [x] Feature-Roadmap (maximale Funktionalität)
- [x] Display/Navigation/Smartphone-Integration

### Phase 1: Analyse & Planung (LAUFEND)
- [ ] Feature-Realisierungs-Matrix (Datenlage → Machbarkeit)
- [ ] Master-Plan (BOM, Kosten, Phasen, Support-Schnittstellen)
- [ ] Premium UI/UX Design-Konzept (Animationen, Layouts, Screens)
- [ ] Firmware-Architektur (Rendering-Engine, Pipelines, BLE-Stack)
- [ ] Diagnose & Support (DTC-System, Self-Test, Remote-Diagnose)
- [ ] Connectivity & Companion-App (GATT-Services, App-Architektur)

### Phase 2: Design & Prototyp (GEPLANT)
- [ ] UI-Mockups (HTML/SVG Prototypen für jeden Screen)
- [ ] Hardware-Schaltplan (KiCad)
- [ ] PCB-Layout
- [ ] Gehäuse-Design (Blender/FreeCAD, 3D-Druck)
- [ ] Companion-App Prototyp (React Native)

### Phase 3: Implementierung (GEPLANT)
- [ ] Core-Firmware (GPS, Display, Sensoren, Logger)
- [ ] Rendering-Engine + Animations-Framework
- [ ] BLE-Stack + GATT-Services
- [ ] Diagnose-System (DTC, Self-Test, Konsole)
- [ ] Companion-App (Dashboard, Navigation, Diagnose)

### Phase 4: Integration & Test (GEPLANT)
- [ ] Hardware-Assembly
- [ ] Motorrad-Einbau
- [ ] Feldtest
- [ ] Performance-Optimierung
- [ ] Dokumentation (User-Manual, Installationsanleitung)

## Offene Fragen
- Welche TFT-Displays sind sonnenlesar genug?
- Companion-App: React Native vs. Flutter?
- Ist ein zweiter ESP32 als Display-Controller sinnvoll?

## Worker-Team
| Profil | Modell | Rolle |
|---|---|---|
| transalp-pm | glm-5.2 | Projektmanager (Cronjob alle 15min) |
| worker | glm-5.2 | Allrounder |
| transalp-plan | glm-5.2 | Planung, Kosten, Roadmap |
| transalp-ui | glm-5.2 | UI/UX, Design, Animationen |
| transalp-firmware | glm-5.2 | Firmware, Treiber, Rendering |
| transalp-diag | glm-5.2 | Diagnose, Support, Wartbarkeit |

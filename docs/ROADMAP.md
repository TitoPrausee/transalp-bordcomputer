# Projekt-Roadmap: Transalp Bordcomputer

> Zuletzt aktualisiert: 13.10.2026 (PM-Runde 7)
> Status: Aktiv — 5 Worker-Profile + 1 Projektmanager (PM) auf Mac mini

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

**7 Research-Dokumente, ~39.000 Wörter in docs/research/**

### Phase 1: Analyse & Planung (ABGESCHLOSSEN)
- [x] Feature-Realisierungs-Matrix (Datenlage -> Machbarkeit) — **worker** (t_13aa20d8) → `08-feature-realization-matrix.md` (6.845 Wörter)
- [x] Master-Plan (BOM, Kosten, Phasen, Support-Schnittstellen) — **transalp-plan** (t_66381dae) → `09-master-plan.md` (9.825 Wörter)
- [x] Premium UI/UX Design-Konzept (Animationen, Layouts, Screens) — **transalp-ui** (t_62aba3bf) → `10-ui-ux-design.md` (8.278 Wörter)
- [x] Firmware-Architektur (Rendering-Engine, Pipelines, BLE-Stack) — **transalp-firmware** (t_379223a5) → `11-firmware-architecture.md` (9.411 Wörter)
- [x] Diagnose & Support (DTC-System, Self-Test, Remote-Diagnose) — **transalp-diag** (t_b4d53b0f) → `12-diagnose-support.md` (7.593 Wörter)
- [x] Connectivity & Companion-App (GATT-Services, App-Architektur) — **worker** (t_2c7740fa) → `13-connectivity-app.md` (7.948 Wörter)

**6 Analyse-Dokumente, ~50.000 Wörter — alle auf main gemerged**

### Phase 1.5: Qualitätsbereinigung (ABGESCHLOSSEN)
- [x] Markennamen-Entfernung in 9 Dokumenten (46 Verweise entfernt) — **worker** (t_91456da9)
  - Verifiziert: grep über docs/ zeigt 0 Markennamen im Prosa
  - Commit ebb501a auf main gepusht

### Phase 2: Design & Prototyp (ABGESCHLOSSEN)
- [x] PCB-Layout-Konzept (Layer-Stackup, Routing-Regeln, EMV-Optimierung) — **transalp-plan** (t_c366755f) → `docs/PCB-LAYOUT.md` (5.993 Wörter)
- [x] Test-Strategie & Benchmark-Konzept (Unit-Tests, HIL, Performance) — **worker** (t_47f87b72) → `docs/TEST-STRATEGY.md` (12.965 Wörter)
- [x] Gehäuse-Design (3D-Druck-Konzept, Montage, Wasserfestigkeit) — **worker** (t_1766aa73) → `docs/CASE-DESIGN.md` (5.877 Wörter)
- [x] Companion-App Prototyp (React Native Grundgerüst, BLE) — **transalp-diag** (t_330d64ce) → `companion-app/` (28 Dateien, 14.957 Zeilen TypeScript)
- [x] UI-Mockups (HTML/SVG Prototypen für jeden Screen) — **transalp-ui** (t_d1e10140) → `mockups/` (35 Dateien, 8 Screens + 24 SVG Icons)
- [x] Hardware-Schaltplan (Pin-Layout, Verdrahtungsplan) — **transalp-firmware** (t_cc1f3b71) → `docs/HARDWARE.md` (5.487 Wörter) + `docs/PINMAP.md` (1.609 Wörter)

### Phase 2.5: Erweiterung (ABGESCHLOSSEN)
- [x] Firmware-Skeleton (ESP32-S3 ESP-IDF Projektstruktur) — **worker** (t_09187ad4) → `firmware/` (52 Dateien, 8 Komponenten mit FreeRTOS Tasks)
- [x] Bill of Materials (BOM) & Sourcing-Konzept — **transalp-plan** (t_9b9760ec) → `docs/BOM.md` (7.528 Wörter)
- [x] User-Manual & Installationsanleitung für PD06 — **worker** (t_d71d39c3) → `docs/USER-MANUAL.md` (8.942 Wörter)

### Phase 3: Implementierung (LÄUFT)
- [x] Integrierte System-Architektur (ARCHITECTURE.md) — **transalp-ui** (t_6f1b1c07) → `docs/ARCHITECTURE.md` (8.845 Wörter)
- [x] Rendering-Engine & Display-Treiber (TFT + E-Paper) — **transalp-firmware** (t_adf3f5ea) → `firmware/components/display/` (1.854 Zeilen, 6 Dateien)
- [ ] Sensor-Driver (RPM, Temp, Fuel, Gear, IMU) — **worker** (t_e5d0d7c5) → running (Retry nach Timeout)
- [x] GPS-Treiber & NMEA-Parser — **transalp-plan** (t_6a3a0821) → `firmware/components/gps/` (886 Zeilen: nmea_parser.c 522, gps_task.c 260, gps.h 104) — gemerged (commit ff640bd)
- [x] BLE-Stack & GATT-Server — **transalp-diag** (t_33c9e335) → `firmware/components/ble/` (1.796 Zeilen: ble_callbacks.c 793, ble_service.c 617, ble_internal.h 139, gatt_uuids.h 166, ble.h 72, ble.c 9) — cherry-picked (commit c8483e8)
- [ ] Storage & Logger (SD-Karte, GPX, NVS Config) — **transalp-firmware** (t_85d92039) → running
- [ ] Power Management & Battery Monitoring — **transalp-ui** (t_1111e7f1) → running
- [x] OTA-Update System & Firmware-Engineering Guide — **worker** (t_4daa19c4) → `firmware/components/ota/` (1.609 Zeilen: ota_update.c 986, ota_secure_boot.c 269, ota_update.h 354) + `docs/FIRMWARE-GUIDE.md` (5.366 Wörter) — gemerged (commit 19bab571)
- [ ] Diagnose-System (DTC, Self-Test, Konsole) — **transalp-diag** (t_8454dd54) → running
- [ ] Companion-App Integration (Dashboard, Navigation, Diagnose) — **worker** (t_968d6126) → running

### Phase 4: Integration & Test (GEPLANT)
- [ ] Hardware-Assembly
- [ ] Motorrad-Einbau
- [ ] Feldtest
- [ ] Performance-Optimierung
- [ ] Test-Strategie und Benchmarks
- [ ] Dokumentation (User-Manual, Installationsanleitung)

## PM-Protokoll

### Runde 1 (13.10.2026 ~18:44)
- 7 Research-Tasks für Phase 0 erstellt und dispatched
- Alle 7 abgeschlossen — 39.000 Wörter in 7 Dokumenten
- 7 Phase-1-Tasks erstellt mit `--skill github-auth`
- **FEHLER**: Skill `github-auth` existiert nicht in Worker-Profilen -> alle 7 Tasks crashten
- Branch `research/display-nav-smartphone` lokal gemerged

### Runde 2 (13.10.2026 ~19:30)
- **Root Cause behoben**: 7 blockierte Tasks archiviert (alle crashten wegen `Unknown skill: github-auth`)
- 6 neue Tasks mit korrektem `--skill github` neu erstellt
- Worker-Zuweisung:
  - worker: Feature-Realisierungs-Matrix + Connectivity & Companion-App
  - transalp-plan: Master-Plan mit Kosten-Nutzen-Analyse
  - transalp-ui: Premium UI/UX Design-Konzept
  - transalp-firmware: Firmware-Architektur
  - transalp-diag: Diagnose & Support
- Alle 6 Tasks erfolgreich dispatched — alle laufen (keine Crashes)
- `--max-retries 3` gesetzt für bessere Fehlertoleranz
- ROADMAP.md aktualisiert

### Runde 3 (13.10.2026 ~20:10)
- **Alle 6 Phase-1-Tasks abgeschlossen** — 0 Running, 0 Blocked
- 6 Branches in main gemerged: feature-matrix, master-plan, ui-ux-design, firmware-arch, diagnose-support, connectivity-app
- Alle Dokumente auf main gepusht (Commit d54ef4c)
- **Qualitäts-Review durchgeführt:**
  - Wortzahl alle 13 Dokumente: 89.381 Wörter gesamt
  - Markennamen-Check: 9 Dateien mit 46 Verweisen auf Marken-/Plattform-Namen
  - Dokument 07 zu kurz: 2.421 Wörter (Minimum war 3.000)
- **Neue Tasks erstellt:** Markennamen-Bereinigung + 6 Phase-2-Tasks
- Metriken und ROADMAP aktualisiert

### Runde 4 (13.10.2026 ~20:35)
- **5 fertige Tasks reviewt und gemerged:**
  - Markennamen-Bereinigung (t_91456da9): 8 Dateien bereinigt, 0 verbleibend — VERIFIZIERT
  - PCB-Layout-Konzept (t_c366755f): `docs/PCB-LAYOUT.md` 5.993 Wörter — gemerged
  - Test-Strategie (t_47f87b72): `docs/TEST-STRATEGY.md` 12.965 Wörter — gemerged
  - Gehäuse-Design (t_1766aa73): `docs/CASE-DESIGN.md` 5.877 Wörter — gemerged
  - Companion-App Prototyp (t_330d64ce): `companion-app/` 28 Dateien, 14.957 Zeilen TypeScript — gemerged
  - UI-Mockups (t_d1e10140): `mockups/` 35 Dateien, 8 HTML/SVG Screens — gemerged
- **4 Branches in main gemerged:** design/pcb-layout, design/case-design, prototype/companion-app, mockups/ui-screens
- Auf origin gepusht (Commits 0c2af50 → 8a41b59)
- **Markennamen-Check:** 0 verbleibende Marken im Prosa — Phase 1.5 ABGESCHLOSSEN
- **3 neue Tasks erstellt und dispatched (Phase 2.5):**
  - Firmware-Skeleton (ESP32-S3 Projektstruktur) → worker (t_09187ad4)
  - BOM & Sourcing-Konzept → transalp-plan (t_9b9760ec)
  - User-Manual & Installationsanleitung → worker (t_d71d39c3)
- Review-Kommentare auf allen fertigen Tasks hinterlegt

### Runde 5 (13.10.2026 ~21:00)
- **3 fertige Tasks reviewt und gemerged:**
  - Firmware-Skeleton (t_09187ad4): `firmware/` 52 Dateien, 8 Komponenten — gemerged
  - BOM & Sourcing-Konzept (t_9b9760ec): `docs/BOM.md` 7.528 Wörter — gemerged
  - User-Manual (t_d71d39c3): `docs/USER-MANUAL.md` 8.942 Wörter — gemerged
- **3 Branches in main gemerged:** firmware/skeleton, docs/bom, docs/user-manual
- **Merge-Konflikt in ROADMAP.md aufgelöst**
- **Phase 2 als ABGESCHLOSSEN markiert**
- **Phase 2.5 als ABGESCHLOSSEN markiert**
- **5 Phase-3-Tasks erstellt und dispatched:**
  - Sensor-Driver → worker (t_e5d0d7c5)
  - Rendering-Engine & Display-Treiber → transalp-firmware (t_adf3f5ea)
  - GPS-Treiber & NMEA-Parser → transalp-plan (t_6a3a0821)
  - BLE-Stack & GATT-Server → transalp-diag (t_33c9e335)
  - Integrierte System-Architektur → transalp-ui (t_6f1b1c07)

### Runde 6 (13.10.2026 ~21:30)
- **3 fertige Tasks reviewt und gemerged:**
  - Rendering-Engine & Display-Treiber (t_adf3f5ea): `firmware/components/display/` 1.854 Zeilen über 6 Dateien — fast-forward gemerged
  - Integrierte System-Architektur (t_6f1b1c07): `docs/ARCHITECTURE.md` 8.845 Wörter — gemerged
  - Hardware-Schaltplan (t_cc1f3b71): `docs/HARDWARE.md` 5.487 Wörter + `docs/PINMAP.md` 1.609 Wörter — cherry-picked
- **Auf origin gepusht (Commit 10a6f86)**
- **3 neue Phase-3-Tasks erstellt und dispatched:**
  - Storage & Logger → transalp-firmware (t_85d92039)
  - Power Management & Battery Monitoring → transalp-ui (t_1111e7f1)
  - OTA-Update System & Firmware-Engineering Guide → worker (t_4daa19c4)
- **6 Tasks running**
- Review-Kommentare auf allen 3 gemergten Tasks hinterlegt

### Runde 7 (13.10.2026 ~22:00)
- **3 fertige Tasks reviewt und gemerged:**
  - GPS-Treiber & NMEA-Parser (t_6a3a0821): `firmware/components/gps/` 886 Zeilen (nmea_parser.c 522, gps_task.c 260, gps.h 104) — bereits in main (commit ff640bd), 29 Unit-Tests, 4 NMEA-Sentence-Types
  - OTA-Update System & Firmware-Guide (t_4daa19c4): `firmware/components/ota/` 1.609 Zeilen + `docs/FIRMWARE-GUIDE.md` 5.366 Wörter — gemerged (commit 19bab571)
  - BLE-Stack & GATT-Server (t_33c9e335): `firmware/components/ble/` 1.796 Zeilen, 8 GATT-Services — cherry-picked (commit c8483e8)
- **Markennamen-Bereinigung:** 3 Verweise in FIRMWARE-GUIDE.md entfernt (Apple Silicon, Google Style Guide, Intel)
- **Auf origin gepusht (Commits ff640bd → c8483e8)**
- **2 neue Phase-3-Tasks erstellt und dispatched:**
  - Diagnose-System (DTC, Self-Test, Konsole) → transalp-diag (t_8454dd54)
  - Companion-App Integration (Dashboard, Navigation, Diagnose) → worker (t_968d6126)
- **5 Tasks running:** Sensor-Driver, Storage/Logger, Power-Management, Diagnose-System, Companion-App
- Review-Kommentare auf allen 3 gemergten Tasks hinterlegt

## Offene Fragen
- Welche TFT-Displays sind sonnenlesbar genug?
- Companion-App: React Native vs. Flutter? (Master-Plan empfiehlt React Native)
- Ist ein zweiter ESP32 als Display-Controller sinnvoll?
- Pin-Konflikte bei Dual-SPI (FSPI + HSPI) mit SD-Karte? → Gelöst: Shared FSPI Bus mit CS-Multiplexing
- Dokument 07 erweitern oder mit 08 zusammenführen?

## Worker-Team
| Profil | Modell | Rolle |
|---|---|---|
| transalp-pm | glm-5.2 | Projektmanager (Cronjob) |
| worker | glm-5.2 | Allrounder |
| transalp-plan | glm-5.2 | Planung, Kosten, Roadmap |
| transalp-ui | glm-5.2 | UI/UX, Design, Animationen |
| transalp-firmware | glm-5.2 | Firmware, Treiber, Rendering |
| transalp-diag | glm-5.2 | Diagnose, Support, Wartbarkeit |

## Metriken
| Metrik | Wert |
|---|---|
| Research-Dokumente | 13 (+ README) |
| Wörter gesamt (Research) | 89.476 |
| Design-Dokumente | 10 (ARCHITECTURE, BOM, CASE-DESIGN, FIRMWARE-GUIDE, HARDWARE, PCB-LAYOUT, PINMAP, ROADMAP, TEST-STRATEGY, USER-MANUAL) |
| Wörter gesamt (Design-Docs) | 63.994 |
| Wörter gesamt (alle Docs) | 153.470 |
| Code-Prototypen | 3 (firmware/ 54 Dateien, companion-app/ 28 Dateien, mockups/ 35 Dateien) |
| Firmware C/H-Zeilen | 7.727 (9 Komponenten: ble, diagnostics, display, gps, ota, power, sensors, storage, utils) |
| Tasks done | 24 |
| Tasks running | 5 |
| Tasks blocked | 0 |
| Phase 0 Fortschritt | 100% ✅ |
| Phase 1 Fortschritt | 100% ✅ |
| Phase 1.5 Fortschritt | 100% ✅ |
| Phase 2 Fortschritt | 100% ✅ (6/6 done) |
| Phase 2.5 Fortschritt | 100% ✅ (3/3 done) |
| Phase 3 Fortschritt | ~50% (5/9 done, 4 running) |

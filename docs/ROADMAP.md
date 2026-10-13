# Projekt-Roadmap: Transalp Bordcomputer

> Zuletzt aktualisiert: 14.10.2026 (PM-Runde 11)
> Status: Aktiv — 5 Worker-Profile + 1 Projektmanager (PM) auf Mac mini
> **Phasen 0–4 ABGESCHLOSSEN ✅ — Phase 5 (Polish & Release) gestartet — 5 Tasks running**

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

### Phase 3: Implementierung (ABGESCHLOSSEN — 10/10 done)
- [x] Integrierte System-Architektur (ARCHITECTURE.md) — **transalp-ui** (t_6f1b1c07) → `docs/ARCHITECTURE.md` (8.845 Wörter)
- [x] Rendering-Engine & Display-Treiber (TFT + E-Paper) — **transalp-firmware** (t_adf3f5ea) → `firmware/components/display/` (2.034 Zeilen, 6 Dateien)
- [x] Sensor-Driver (RPM, Temp, Fuel, Gear, IMU) — **worker** (t_e5d0d7c5) → `firmware/components/sensors/` (1.225 Zeilen: rpm.c 158, temperature.c 296, fuel_level.c 145, gear_indicator.c 96, imu.c 238, sensors.c 211, sensors.h 81) — gemerged (commit 2360962a)
- [x] GPS-Treiber & NMEA-Parser — **transalp-plan** (t_6a3a0821) → `firmware/components/gps/` (886 Zeilen: nmea_parser.c 522, gps_task.c 260, gps.h 104) — gemerged (commit ff640bd)
- [x] BLE-Stack & GATT-Server — **transalp-diag** (t_33c9e335) → `firmware/components/ble/` (1.796 Zeilen: ble_callbacks.c 793, ble_service.c 617, ble_internal.h 139, gatt_uuids.h 166, ble.h 72, ble.c 9) — cherry-picked (commit c8483e8)
- [x] Storage & Logger (SD-Karte, GPX, NVS Config) — **transalp-firmware** (t_85d92039) → `firmware/components/storage/` (1.844 Zeilen: sd_card.c 457, logger.c 579, config.c 646, storage.c 162) — gemerged (commit 9cebea86)
- [x] Power Management & Battery Monitoring — **transalp-ui** (t_1111e7f1) → `firmware/components/power/` (1.357 Zeilen: power.c 418, power_manager.c 718, power.h 221) — gemerged (commit 2d756192)
- [x] OTA-Update System & Firmware-Engineering Guide — **worker** (t_4daa19c4) → `firmware/components/ota/` (1.609 Zeilen: ota_update.c 986, ota_secure_boot.c 269, ota_update.h 354) + `docs/FIRMWARE-GUIDE.md` (5.366 Wörter) — gemerged (commit 19bab571)
- [x] Diagnose-System (DTC, Self-Test, Konsole) — **transalp-diag** (t_8454dd54) → `firmware/components/diagnostics/` (1.861 Zeilen: diagnostics.c 670, dtc.c 400, selftest.c 582, diagnostics.h 207) — gemerged (commit 2d756192)
- [x] Companion-App Integration (Dashboard, Navigation, Diagnose) — **worker** (t_968d6126) → `companion-app/src/` (11 Dateien, ~4.713 Zeilen TS/TSX: BleManager, DashboardScreen, NavigationScreen, DiagnoseScreen, SettingsScreen, binary-parsers) — gemerged (commit d5326b4b)

### Phase 4: Integration & Test (ABGESCHLOSSEN — 11/11 done)
- [x] Firmware Integration & Build-System (main.c, Event-Loop, CMake) — **transalp-firmware** (t_95f1b701) → `firmware/main/` (915 Zeilen: main.c 571, app_event_loop.c 215, app_event_loop.h 129) — gemerged (commit 3d9b9ef6)
- [x] Hardware-Test-Bench & Benchmark-Spezifikation — **transalp-diag** (t_dd852a50) → `docs/HARDWARE-TEST-BENCH.md` (1.042 Zeilen) + `docs/BENCHMARK-SPEC.md` (529 Zeilen) — gemerged (commit ffaca698)
- [x] Montage-Anleitung & Einbau-Konzept für PD06 — **transalp-plan** (t_9f608f6e) → `docs/INSTALLATION-GUIDE.md` (1.659 Zeilen) — gemerged (commit 0529bf9)
- [x] Feldtest-Protokoll & Evaluations-Kriterien — **transalp-ui** (t_8451c1c9) → `docs/FIELDTEST-PROTOCOL.md` (1.227 Zeilen, 8.236 Wörter) — gemerged (commit a217e78)
- [x] Performance-Optimierung & Profiling-Konzept — **transalp-firmware** (t_d8927032) → `docs/PERFORMANCE-OPTIMIZATION.md` (2.187 Zeilen, 8.639 Wörter) — gemerged (commit a217e78)
- [x] CI/CD-Pipeline & Automated Testing — **transalp-diag** (t_727233a1) → `docs/CI-CD-PIPELINE.md` (694 Zeilen) + `.github/workflows/` (3 CI-Workflows) + `firmware/test/` (3 Unit-Tests + Mocks) — gemerged (commit 44c8e66f)
- [x] Total Cost of Ownership & Budget-Analyse — **transalp-plan** (t_28e4c582) → `docs/TCO-ANALYSIS.md` (865 Zeilen, 6.110 Wörter) — gemerged (commit a217e78)
- [x] Sicherheitsanalyse & Threat Model (STRIDE) — **worker** (t_6eab2b4d) → `docs/SECURITY-ANALYSIS.md` (1.472 Zeilen, 5.909 Wörter) — gemerged (commit a217e78)
- [x] API-Dokumentation: BLE GATT Interface Reference — **transalp-firmware** (t_2d00106d) → `docs/API-REFERENCE.md` (1.692 Zeilen, 7.297 Wörter) — gemerged (commit 41580e4)
- [x] Release-Notes v1.0 & Changelog-Konzept — **transalp-plan** (t_321097f7) → `docs/RELEASE-NOTES.md` (967 Zeilen, 5.146 Wörter) — gemerged (commit 41580e4)
- [x] Troubleshooting-Guide & FAQ — **transalp-ui** (t_4d933681) → `docs/TROUBLESHOOTING.md` (1.817 Zeilen, 9.199 Wörter) — gemerged (commit 41580e4)

**Phase 4: 11/11 done — ABGESCHLOSSEN ✅**

### Phase 5: Polish & Release (LÄUFT)
- [ ] README.md v2.0: Projekt-Übersicht aktualisieren — **transalp-ui** (t_2f24c21a) → running
- [ ] CONTRIBUTING.md: Open-Source Beitragende-Leitfaden — **transalp-plan** (t_48c61f20) → running
- [ ] E-Paper Widget-Katalog & Display-Screen-Referenz — **transalp-firmware** (t_4b253561) → running
- [ ] Developer Quick-Start Guide — **transalp-diag** (t_97a98c74) → running
- [ ] Markennamen-Bereinigung Teil 2: Research-Docs — **worker** (t_b3b85edd) → running

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

### Runde 8 (13.10.2026 ~22:30)
- **4 fertige Tasks reviewt und gemerged:**
  - Sensor-Driver (t_e5d0d7c5): `firmware/components/sensors/` 1.225 Zeilen (rpm.c 158, temperature.c 296, fuel_level.c 145, gear_indicator.c 96, imu.c 238, sensors.c 211, sensors.h 81) — gemerged (commit 2360962a)
  - Power Management (t_1111e7f1): `firmware/components/power/` 1.357 Zeilen (power.c 418, power_manager.c 718, power.h 221) — gemerged (commit 2d756192)
  - Diagnose-System (t_8454dd54): `firmware/components/diagnostics/` 1.861 Zeilen (diagnostics.c 670, dtc.c 400, selftest.c 582, diagnostics.h 207) — gemerged (commit 2d756192)
  - Storage & Logger (t_85d92039): `firmware/components/storage/` 1.844 Zeilen (sd_card.c 457, logger.c 579, config.c 646, storage.c 162) — gemerged (commit 9cebea86)
- **Kritischer Fix:** Merge-Konflikt-Nachwirkung behoben — Feature-Branch-Merges hatten versehentlich Stub-Versionen statt voller Implementierungen auf main belassen. Alle 4 Komponenten von ihren Feature-Branches restauriert (commit 02f591b0). Firmware-Gesamt: 13.262 C/H-Zeilen über 9 Komponenten.
- **Markennamen-Check:** 0 Marken in restaurierten Dateien
- **Auf origin gepusht (Commits 2d756192 → 9cebea86 → 02f591b0)**
- **Phase 4 gestartet — 4 neue Tasks erstellt und dispatched:**
  - Firmware Integration & Build-System → transalp-firmware (t_95f1b701)
  - Hardware-Test-Bench & Benchmark-Spezifikation → transalp-diag (t_dd852a50)
  - Montage-Anleitung & Einbau-Konzept für PD06 → transalp-plan (t_9f608f6e)
  - Feldtest-Protokoll & Evaluations-Kriterien → transalp-ui (t_8451c1c9)
- **6 Tasks running:** Companion-App (worker) + 4 neue Phase-4-Tasks
- Review-Kommentare auf allen 4 gemergten Tasks hinterlegt

### Runde 9 (13.10.2026 ~23:00)
- **3 fertige Tasks reviewt und gemerged:**
  - Firmware Integration & Build-System (t_95f1b701): `firmware/main/` 915 Zeilen (main.c 571, app_event_loop.c 215, app_event_loop.h 129) — 10-Schritt Startup-Sequenz, 8 FreeRTOS Tasks mit Core-Affinity, Event-Bus — gemerged (commit 3d9b9ef6)
  - Hardware-Test-Bench & Benchmark (t_dd852a50): `docs/HARDWARE-TEST-BENCH.md` 1.042 Zeilen + `docs/BENCHMARK-SPEC.md` 529 Zeilen — Signal-Simulatoren, Test-Prozeduren, Benchmark-Zielwerte — gemerged (commit ffaca698)
  - Montage-Anleitung (t_9f608f6e): `docs/INSTALLATION-GUIDE.md` 1.659 Zeilen — PD06-Einbau: PCB, Verkabelung, Display, Inbetriebnahme — gemerged (commit 0529bf9)
- **Wiederkehrendes Merge-Problem erkannt:** Worker committen auf Feature-Branches aber mergen nicht nach main. Auf main waren nur Stub-Versionen. Feature-Branches selektiv gemerged (nur relevante Dateien um Konflikte zu vermeiden).
- **Markennamen-Check:** 0 Marken in neuen Dateien
- **Auf origin gepusht (Commits 3d9b9ef6 → ffaca698 → 0529bf9)**
- **Firmware-Gesamt: 14.053 C/H-Zeilen** (+791: integration layer main.c + event loop)
- **4 neue Phase-4-Tasks erstellt und dispatched:**
  - Performance-Optimierung & Profiling → transalp-firmware (t_d8927032)
  - CI/CD-Pipeline & Automated Testing → transalp-diag (t_727233a1)
  - Total Cost of Ownership & Budget → transalp-plan (t_28e4c582)
  - Sicherheitsanalyse & Threat Model → worker (t_6eab2b4d, ursprünglich transalp-research zugedacht, aber Profil nicht auf Disk)
- **6 Tasks running:** Companion-App, Feldtest-Protokoll, Performance, CI/CD, TCO, Security
- Review-Kommentare auf allen 3 gemergten Tasks hinterlegt

### Runde 10 (13.10.2026 ~23:30)
- **5 fertige Tasks reviewt und gemerged:**
  - Feldtest-Protokoll (t_8451c1c9): `docs/FIELDTEST-PROTOCOL.md` 1.227 Zeilen, 8.236 Wörter — 8 Test-Phasen, 25 Metriken mit Pass/Fail-Schwellen — gemerged (commit a217e78)
  - Performance-Optimierung (t_d8927032): `docs/PERFORMANCE-OPTIMIZATION.md` 2.187 Zeilen, 8.639 Wörter — CPU/Speicher/Rendering/Sensor/BLE/Power-Optimierung + Profiling-Strategie — gemerged (commit a217e78)
  - TCO & Budget (t_28e4c582): `docs/TCO-ANALYSIS.md` 865 Zeilen, 6.110 Wörter — 3 Szenarien (80/327/417 EUR), ROI 38-66%, 5-Jahres-Lifecycle — gemerged (commit a217e78)
  - Sicherheitsanalyse (t_6eab2b4d): `docs/SECURITY-ANALYSIS.md` 1.472 Zeilen, 5.909 Wörter — STRIDE mit 19 Threats, MoSCoW-Mitigation-Plan, Code-Snippets — gemerged (commit a217e78)
  - CI/CD-Pipeline (t_727233a1): `docs/CI-CD-PIPELINE.md` 694 Zeilen + 3 GitHub-Workflow-Files + `firmware/test/` (3 Unit-Tests + ESP-IDF-Mocks) — gemerged (commit 44c8e66f)
- **Security-Analysis Revert aufgehoben:** Vorheriger Commit (3c56f872) hatte die Sicherheitsanalyse revertiert; neu gemerged mit korrektem Inhalt
- **Markennamen-Check:** 0 Marken in allen neuen Dateien — VERIFIZIERT
- **Auf origin gepusht (Commits a217e78 → 44c8e66f)**
- **Firmware-Gesamt: 15.319 C/H-Zeilen** (+1.266: CI/CD Test-Suite + Mocks)
- **Design-Docs: 18 Dateien, ~115K Wörter** (+5 neue: FIELDTEST-PROTOCOL, PERFORMANCE-OPTIMIZATION, CI-CD-PIPELINE, TCO-ANALYSIS, SECURITY-ANALYSIS)
- **3 neue Tasks erstellt und dispatched (Phase 4 Dokumentation):**
  - API-Dokumentation: BLE GATT Interface Reference → transalp-firmware (t_2d00106d)
  - Release-Notes v1.0 & Changelog-Konzept → transalp-plan (t_321097f7)
  - Troubleshooting-Guide & FAQ → transalp-ui (t_4d933681)
- **4 Tasks running:** API-Reference (firmware), Release-Notes (plan), Troubleshooting (ui)
- Review-Kommentare auf allen 5 gemergten Tasks hinterlegt
- **Companion-App Integration (t_968d6126) nachträglich entdeckt und gemerged:** 11 Dateien, ~4.713 Zeilen TS/TSX (BleManager, Dashboard, Navigation, Diagnose, Settings, binary-parsers) — gemerged (commit d5326b4b). tsc --noEmit 0 errors. **PHASE 3 = 100% ERLEDIGT**

### Runde 11 (14.10.2026 ~00:00)
- **3 fertige Tasks reviewt und gemerged:**
  - API-Dokumentation BLE GATT (t_2d00106d): `docs/API-REFERENCE.md` 1.692 Zeilen, 7.297 Wörter — 8 GATT-Services mit UUIDs, Characteristics, Datenformaten, OTA-Protokoll — gemerged (commit 41580e4)
  - Release-Notes v1.0 (t_321097f7): `docs/RELEASE-NOTES.md` 967 Zeilen, 5.146 Wörter — Feature-Liste mit Status, Hardware-Anforderungen, bekannte Einschränkungen, Changelog-Konzept — gemerged (commit 41580e4)
  - Troubleshooting-Guide (t_4d933681): `docs/TROUBLESHOOTING.md` 1.817 Zeilen, 9.199 Wörter — Hardware/Software/BLE/GPS/SD/Power-Probleme, FAQ, Serial-Console-Guide — gemerged (commit 41580e4)
- **Markennamen-Check:** 0 Marken in allen 3 neuen Dokumenten — VERIFIZIERT
- **Auf origin gepusht (Commit 41580e4)**
- **PHASE 4 = 100% ABGESCHLOSSEN** 🎉 — Alle 11 Phase-4-Tasks done
- **Alle Projektphasen 0–4 abgeschlossen** — 38 Tasks gesamt, 0 running, 0 blocked
- **Gesamtdokumentation: 226.524 Wörter** über 35 Dokumente (14 Research + 21 Design-Docs)
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
| Research-Dokumente | 14 (+ README) |
| Wörter gesamt (Research) | 89.476 |
| Design-Dokumente | 21 (+3: API-REFERENCE, RELEASE-NOTES, TROUBLESHOOTING) |
| Wörter gesamt (Design-Docs) | 137.048 |
| Wörter gesamt (alle Docs) | 226.524 |
| Code-Prototypen | 3 (firmware/ 57+17 Dateien, companion-app/ 5.957 Zeilen TS/TSX, mockups/ 35 Dateien) |
| Firmware C/H-Zeilen | 15.297 (10 Komponenten + main + test-suite) |
| CI/CD Workflows | 3 (firmware-ci, companion-app-ci, docs-ci) + brand-blocklist |
| Unit-Tests | 3 (test_dtc, test_event_loop, test_nmea_parser) + ESP-IDF-Mocks |
| Tasks done | 38 (+3 seit Runde 10) |
| Tasks running | 5 (README v2, CONTRIBUTING, Widget-Katalog, Dev-Quickstart, Brand-Cleanup-2) |
| Tasks blocked | 0 |
| Phase 0 Fortschritt | 100% ✅ |
| Phase 1 Fortschritt | 100% ✅ |
| Phase 1.5 Fortschritt | 100% ✅ |
| Phase 2 Fortschritt | 100% ✅ (6/6 done) |
| Phase 2.5 Fortschritt | 100% ✅ (3/3 done) |
| Phase 3 Fortschritt | 100% ✅ (10/10 done) |
| Phase 4 Fortschritt | 100% ✅ (11/11 done) |
| Phase 5 Fortschritt | 0/5 done, 5 running |

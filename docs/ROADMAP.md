# Projekt-Roadmap: Transalp Bordcomputer

> Zuletzt aktualisiert: 13.10.2026 (PM-Runde 4)
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

### Phase 2: Design & Prototyp (LAUFEND)
- [x] PCB-Layout-Konzept (Layer-Stackup, Routing-Regeln, EMV-Optimierung) — **transalp-plan** (t_c366755f) → `docs/PCB-LAYOUT.md` (5.993 Wörter)
- [x] Test-Strategie & Benchmark-Konzept (Unit-Tests, HIL, Performance) — **worker** (t_47f87b72) → `docs/TEST-STRATEGY.md` (12.965 Wörter)
- [x] Gehäuse-Design (3D-Druck-Konzept, Montage, Wasserfestigkeit) — **worker** (t_1766aa73) → `docs/CASE-DESIGN.md` (5.877 Wörter)
- [ ] UI-Mockups (HTML/SVG Prototypen für jeden Screen) — **transalp-ui** (t_d1e10140) → running
- [ ] Hardware-Schaltplan (Pin-Layout, Verdrahtungsplan) — **transalp-firmware** (t_cc1f3b71) → running
- [ ] Companion-App Prototyp (React Native Grundgerüst, BLE) — **transalp-diag** (t_330d64ce) → running (prototype/companion-app Branch erstellt)

### Phase 2.5: Erweiterung (LAUFEND — neu in Runde 4)
- [ ] Firmware-Skeleton (ESP32-S3 ESP-IDF Projektstruktur) — **worker** (t_09187ad4) → running
- [ ] Bill of Materials (BOM) & Sourcing-Konzept — **transalp-plan** (t_9b9760ec) → running
- [ ] User-Manual & Installationsanleitung für PD06 — **worker** (t_d71d39c3) → running

### Phase 3: Implementierung (GEPLANT)
- [ ] Core-Firmware (GPS, Display, Sensoren, Logger)
- [ ] Rendering-Engine + Animations-Framework
- [ ] BLE-Stack + GATT-Services
- [ ] Diagnose-System (DTC, Self-Test, Konsole)
- [ ] Companion-App (Dashboard, Navigation, Diagnose)
- [ ] OTA-Update-Pipeline

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
- **4 neue fertige Tasks reviewt:**
  - Markennamen-Bereinigung (t_91456da9): 8 Dateien bereinigt, 0 verbleibend — VERIFIZIERT
  - PCB-Layout-Konzept (t_c366755f): `docs/PCB-LAYOUT.md` 5.993 Wörter — gemerged
  - Test-Strategie (t_47f87b72): `docs/TEST-STRATEGY.md` 12.965 Wörter — gemerged
  - Gehäuse-Design (t_1766aa73): `docs/CASE-DESIGN.md` 5.877 Wörter — gemerged
- **3 Branches in main gemerged:** design/pcb-layout, design/case-design (inkl. test-strategy)
- Auf origin gepusht (Commit 0c2af50)
- **Markennamen-Check:** 0 verbleibende Marken im Prosa — Phase 1.5 ABGESCHLOSSEN
- **3 Tasks noch running:** UI-Mockups, Hardware-Schaltplan, Companion-App Prototyp
- **3 neue Tasks erstellt und dispatched (Phase 2.5):**
  - Firmware-Skeleton (ESP32-S3 Projektstruktur) → worker (t_09187ad4)
  - BOM & Sourcing-Konzept → transalp-plan (t_9b9760ec)
  - User-Manual & Installationsanleitung → worker (t_d71d39c3)
- Companion-App Prototyp auf Branch `prototype/companion-app` hat erste React Native Struktur
- Review-Kommentare auf allen 4 fertigen Tasks hinterlegt

## Offene Fragen
- Welche TFT-Displays sind sonnenlesbar genug?
- Companion-App: React Native vs. Flutter? (Master-Plan empfiehlt React Native)
- Ist ein zweiter ESP32 als Display-Controller sinnvoll?
- Pin-Konflikte bei Dual-SPI (FSPI + HSPI) mit SD-Karte?
- Dokument 07 erweitern oder mit 08 zusammenführen?

## Worker-Team
| Profil | Modell | Rolle |
---|---|---|
| transalp-pm | glm-5.2 | Projektmanager (Cronjob) |
| worker | glm-5.2 | Allrounder |
| transalp-plan | glm-5.2 | Planung, Kosten, Roadmap |
| transalp-ui | glm-5.2 | UI/UX, Design, Animationen |
| transalp-firmware | glm-5.2 | Firmware, Treiber, Rendering |
| transalp-diag | glm-5.2 | Diagnose, Support, Wartbarkeit |

## Metriken
| Metrik | Wert |
|---|---|
| Research-Dokumente | 13 |
| Wörter gesamt (Research) | 89.476 |
| Design-Dokumente | 3 (PCB-LAYOUT, CASE-DESIGN, TEST-STRATEGY) |
| Wörter gesamt (Design) | 24.835 |
| Wörter gesamt (alle Docs) | 114.311 |
| Tasks gesamt (done) | 18 (7 Phase 0 + 7 Phase 1 + 4 Phase 1.5/2) |
| Tasks running | 6 |
| Tasks blocked | 0 |
| Phase 0 Fortschritt | 100% |
| Phase 1 Fortschritt | 100% |
| Phase 1.5 Fortschritt | 100% |
| Phase 2 Fortschritt | 50% (3/6 done, 3 running) |

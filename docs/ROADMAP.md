# Projekt-Roadmap: Transalp Bordcomputer

> Zuletzt aktualisiert: 13.10.2026 (PM-Runde 3)
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

### Phase 1.5: Qualitätsbereinigung (LAUFEND)
- [ ] Markennamen-Entfernung in 9 Dokumenten (46 Verweise auf Apple/Google/Samsung/iOS/Android)
- [ ] Dokument 07 zu kurz (2.421 Wörter, Minimum 3.000) — erweitern oder zusammenführen

### Phase 2: Design & Prototyp (GEPLANT — Tasks in Erstellung)
- [ ] UI-Mockups (HTML/SVG Prototypen für jeden Screen) — **transalp-ui**
- [ ] Hardware-Schaltplan (Pin-Layout, Verdrahtungsplan, KiCad-kompatibel) — **transalp-firmware**
- [ ] PCB-Layout-Konzept (Layer-Stackup, Routing-Regeln, EMV-Optimierung) — **transalp-plan**
- [ ] Companion-App Prototyp (React Native Grundgerüst, BLE-Verbindung) — **transalp-diag**
- [ ] Test-Strategie & Benchmark-Konzept (Unit-Tests, HIL, Performance) — **worker**
- [ ] Gehäuse-Design (3D-Druck-Konzept, Montage, Wasserfestigkeit) — **worker**

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
  - Markennamen-Check: 9 Dateien mit 46 Verweisen auf Apple/Google/Samsung/iOS/Android
  - Dokument 07 zu kurz: 2.421 Wörter (Minimum war 3.000)
- **Neue Tasks erstellt:** Markennamen-Bereinigung + 6 Phase-2-Tasks
- Metriken und ROADMAP aktualisiert

## Offene Fragen
- Welche TFT-Displays sind sonnenlesbar genug?
- Companion-App: React Native vs. Flutter? (Master-Plan empfiehlt React Native)
- Ist ein zweiter ESP32 als Display-Controller sinnvoll?
- Pin-Konflikte bei Dual-SPI (FSPI + HSPI) mit SD-Karte?
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
| Research-Dokumente | 13 |
| Wörter gesamt (Research) | 89.381 |
| Tasks gesamt (done) | 14 (7 Phase 0 + 7 Phase 1) |
| Tasks archiviert | 7 (blockiert, durch Skill-Fehler ersetzt) |
| Phase 0 Fortschritt | 100% |
| Phase 1 Fortschritt | 100% |
| Phase 1.5 Fortschritt | 0% (Markennamen-Bereinigung ausstehend) |
| Phase 2 Fortschritt | 0% (Tasks in Erstellung) |

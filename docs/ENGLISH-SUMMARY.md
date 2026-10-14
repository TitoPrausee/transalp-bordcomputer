# English Project Summary & Translation Guide

> Transalp Bordcomputer — Open-Source Onboard Computer for the Honda Transalp 600 (PD06)
> Platform: ESP32-S3-WROOM-1, ESP-IDF v5.4, FreeRTOS SMP
> License: PolyForm Noncommercial 1.0.0
> Date: October 2026

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Feature Summary](#2-feature-summary)
3. [Document Index](#3-document-index)
4. [Translation Guide](#4-translation-guide)
5. [Quick Start](#5-quick-start)

---

## 1. Project Overview

The Transalp Bordcomputer is a complete, open-source onboard computer
designed for the Honda XL600V Transalp 600 (PD06), a dual-sport
motorcycle produced from 1986 to 2000. The PD06 is a purely mechanical
machine: it has no Electronic Control Unit (ECU), no CAN bus, no
OBD-II diagnostic port, and no electronic engine management. Every
vehicle parameter must therefore be captured through independent
sensors and direct signal tapping. This project documents the entire
process of building such a system from scratch, from initial research
through to a working firmware, companion app, and hardware design.

### Why This Project Exists

Modern motorcycles come equipped with sophisticated onboard
electronics — digital dashboards, GPS, Bluetooth connectivity,
diagnostic systems, and over-the-air firmware updates. The PD06, being
a product of the late 1980s and 1990s, has none of these. Owners who
want modern instrumentation face a choice: buy an expensive
aftermarket solution that may not integrate well with the bike's
mechanical nature, or build their own. This project provides a
complete, documented, open-source alternative.

The system is built around the ESP32-S3, a powerful dual-core
microcontroller running at 240 MHz with 16 MB of flash and 8 MB of
PSRAM. The firmware is developed using ESP-IDF (the official IoT
Development Framework) v5.4 with FreeRTOS in Symmetric Multi-Processing
(SMP) mode, enabling true parallel task execution across both CPU
cores. The project does not use the Arduino framework — it targets
ESP-IDF directly for full control over hardware peripherals, task
scheduling, and power management.

### Target Vehicle: Honda Transalp 600 (PD06)

The Honda Transalp 600 (PD06) is a 583cc V-twin dual-sport motorcycle.
Its mechanical simplicity makes it an ideal candidate for a custom
onboard computer: all sensors can be added without conflicting with
existing electronic systems, since there are none. The project
addresses the specific sensor configuration of the PD06:

- **Engine RPM** is measured by tapping the ignition pickup signal via
  an optocoupler connected to the CDI (Capacitor Discharge Ignition)
  unit, read by the ESP32-S3 pulse counter peripheral (PCNT0).
- **Speed** is measured via a Hall-effect sensor or reed contact at
  the front wheel, using a second pulse counter (PCNT1) with
  configurable wheel circumference.
- **Coolant temperature** uses an NTC 10k thermistor in the coolant
  hose, read through an ADC with Steinhart-Hart equation conversion.
- **Fuel level** is read from the existing tank sender (5-90 ohm) via
  a voltage divider and ADC with a 6-point calibration curve.
- **Oil pressure** is measured by an analog pressure sensor.
- **Gear indication** is derived from the speed-to-RPM ratio,
  calibrated for the PD06's 5-speed transmission.

### Technology Stack

The project employs a modern, professional technology stack:

- **Microcontroller:** ESP32-S3-WROOM-1 (dual-core 240 MHz, 16 MB
  flash, 8 MB PSRAM, BLE 5.0)
- **Firmware Framework:** ESP-IDF v5.4 with FreeRTOS SMP
- **Build System:** CMake + Ninja (ESP-IDF native)
- **GPS:** Multi-constellation GNSS module (10 Hz, 72-channel),
  NMEA parsing over UART
- **Displays:** Dual-display architecture — a 3.5-inch TFT (480x320,
  RGB565, ILI9488 controller, SPI with DMA) for real-time telemetry
  at 30 FPS, and a 2.13-inch E-Paper display (monochrome,
  partial-refresh) for sunlight-readable static values
- **Sensors:** 6-axis IMU (gyroscope + accelerometer, MPU6050,
  I2C), barometric pressure sensor (BMP280, I2C) for altitude and
  ambient temperature
- **Wireless:** NimBLE BLE stack with a custom GATT server (8
  services, 33 characteristics), LE Secure Connections with ECDH
  P-256 pairing and MITM protection
- **Storage:** MicroSD card (FAT32, SPI) for CSV telemetry logging
  at 5 Hz and GPX track export at 1 Hz; NVS (Non-Volatile Storage)
  for device configuration as a BLOB
- **Companion App:** React Native application (TypeScript, Expo SDK
  52, strict mode) for both mobile platforms, featuring a BLE client
  that connects to the onboard computer for navigation, media
  control, diagnostics, and firmware updates
- **Power Management:** LiPo 18650 backup battery (3400 mAh) with
  TP4056 charging, MT3608 boost converter, and HT7333 LDO regulator;
  deep-sleep mode at approximately 0.4 mA with wake-on-motion,
  wake-on-ignition, and BLE wake sources

### Open-Source, Non-Commercial

The project is licensed under the PolyForm Noncommercial License
1.0.0. This permits private use, educational purposes, modification,
and sharing under the same license. Commercial use, marketing, and
rebranding under a different name are prohibited. All documentation,
firmware source code, companion app source code, hardware schematics,
and UI mockups are publicly available on the project repository.

### Project Scale

The project was developed across seven phases (Phases 0-6), involving
49 tasks completed by six worker profiles and one project manager. The
result is over 260,000 words of documentation across 49 documents,
approximately 15,300 lines of C/H firmware code across 10 components,
nearly 6,000 lines of TypeScript in the companion app, 35 HTML/SVG UI
mockup files, and a CI/CD pipeline with 4 GitHub Actions workflows
and 3 unit tests with ESP-IDF mocks.

---

## 2. Feature Summary

All major features of the Transalp Bordcomputer with brief
descriptions. For full details, refer to the German documentation
linked in the Document Index below.

### Real-Time Telemetry

- **Engine RPM** — Measured via pulse counter (PCNT0) with optocoupler
  on ignition coil, 50 Hz sampling, median filter. (Stable)
- **Speed** — Measured via reed contact at front wheel (PCNT1),
  configurable wheel circumference. (Stable)
- **Coolant Temperature** — NTC 10k thermistor with ADC, Steinhart-Hart
  equation, 16x oversampling. (Stable)
- **Fuel Level** — ADC with 6-point calibration curve, EMA filter.
  (Beta)
- **Gear Indication** — Derived from speed/RPM ratio for 5-speed
  transmission. (Beta)
- **Board Voltage** — Voltage divider (100k/33k), generator status
  detection (>13.8V). (Stable)
- **Oil Pressure** — Analog sensor (0-5 bar, M10x1 thread). (Beta)
- **Ambient Temperature** — Sourced from barometric pressure sensor.
  (Stable)

### GPS Tracking

- **NMEA Parser** — Parses GGA, RMC, VTG sentences at 10 Hz over UART.
  (Stable)
- **Position & Speed** — Latitude, longitude, altitude, speed, heading,
  satellite count. (Stable)
- **GPS Fix Detection** — Event bus notification on fix acquisition and
  loss. (Stable)
- **GPX Track Export** — GPS track saved as GPX file at 1 Hz on SD
  card. (Stable)
- **1PPS Time Sync** — 1PPS signal on GPIO for time synchronization.
  (Experimental)

### Dual-Display System

- **TFT Driver** — ILI9488, 480x320, RGB565, SPI2 at 40-80 MHz with
  DMA. (Stable)
- **E-Paper Driver** — 2.13 inch, monochrome, partial refresh. (Beta)
- **Framebuffer in PSRAM** — Double buffering (front + back buffer).
  (Stable)
- **Dirty-Rect Tracking** — Maximum 16 rectangles, overlapping merged,
  reducing SPI transfer by factor 12. (Stable)
- **Rendering Pipeline** — Widget tree to layout to paint to compositor
  to dirty-rect to SPI/DMA blit. (Stable)
- **UI Widgets** — Speed, RPM bar, gear, navigation arrow, altitude,
  tank, temperature. (Stable)
- **Animation Engine** — Tween/spring physics, timeline updates at
  60 Hz. (Experimental)

### BLE & Companion App

- **NimBLE GATT Server** — 8 custom services, 33 characteristics,
  MTU 247 (BLE 5.0 maximum). (Stable)
- **LE Secure Connections** — ECDH P-256 pairing, MITM protection,
  bonding persisted in NVS. (Stable)
- **Telemetry Service** — Live data at 4 Hz (28 bytes), snapshot,
  track export. (Stable)
- **Navigation Service** — Turn-by-turn, route, road name, status.
  (Beta)
- **Media Service** — Now playing, control, volume. (Beta)
- **Phone Service** — Incoming call, call status, notifications.
  (Beta)
- **Weather Service** — Current conditions, forecast, alerts. (Beta)
- **Config Service** — Read/write, export/import, bond management.
  (Stable)
- **Diagnose Service** — DTC list, sensor status, self-test, log
  export. (Stable)
- **OTA Service** — Control, 4 KB chunks, SHA-256 verification. (Stable)
- **Auto-Reconnect** — Exponential backoff (1s to 30s max). (Stable)
- **Degrading Mode** — Reduced notify rates when RSSI < -90 dBm.
  (Stable)

### Diagnostics & OTA

- **DTC Table** — 16 entries in RAM, 4 categories (Powertrain, Sensors,
  Comms, Body). (Stable)
- **Boot Self-Test** — 7 hardware checks, OTA rollback verification.
  (Stable)
- **Remote Diagnostics** — DTC list and sensor status via BLE. (Stable)
- **Boot-Loop Detection** — Triggers DTC and rollback after 3 resets in
  60 seconds. (Stable)
- **Dual-Partition OTA** — ota_0 / ota_1 (3 MB each) with factory
  fallback. (Stable)
- **OTA via BLE** — 4 KB chunks, write-without-response, CRC16-CCITT.
  (Stable)
- **OTA via HTTP** — TLS with certificate bundle, range-request resume.
  (Beta)
- **OTA via SD Card** — Firmware file from FAT-formatted SD card.
  (Stable)
- **SHA-256 Verification** — Hash of flashed partition compared with
  expected hash. (Stable)
- **RSA-3072 Signature** — Secure Boot V2 signature check (placeholder
  key in v1.0). (Experimental)
- **Automatic Rollback** — Self-test after boot, reverts to previous
  partition on failure. (Stable)

### Power Management & Sensors

- **Power State Machine** — ACTIVE, IDLE, SLEEP, SHUTDOWN with
  automatic transitions. (Stable)
- **Battery Monitoring** — ADC, voltage divider 2:1, generator status.
  (Stable)
- **Deep-Sleep** — Approximately 0.4 mA, RTC active, survives weeks.
  (Stable)
- **Wake Sources** — Ignition (GPIO), IMU interrupt, BLE connection
  request, RTC timer. (Stable)
- **Backup Battery** — 18650 Li-Ion, 3400 mAh, TP4056 charging module.
  (Stable)
- **IMU Roll/Pitch** — Complementary filter from gyro + accel, 50 Hz.
  (Stable)
- **Crash Detection** — Triggers at >3g, sets event and DTC. (Beta)
- **Barometric Pressure** — Altitude, air pressure, ambient temperature
  via BMP280. (Stable)

### Data Logging

- **SD Card Mount** — FAT32, Class 10, via SPI2 (shared with display),
  mutex-protected. (Stable)
- **CSV Telemetry** — 5 Hz logging, 64-entry ring buffer, batch write.
  (Stable)
- **GPX Track Export** — 1 Hz GPS points, auto-generated filename.
  (Stable)
- **DTC Log** — Critical DTCs persisted to SD card. (Stable)
- **NVS Configuration** — Device config as BLOB, JSON export/import via
  BLE. (Stable)

### Control

- **Relay Control** — 4-channel relay module for auxiliary lights,
  heated grips, USB charging, accessories. (Stable)

### Safety

- **Crash Detection** — Automatic emergency SOS position broadcast via
  companion app on >3g impact. (Beta)
- **Theft Warning** — IMU-based motion detection when parked. (Beta)

---

## 3. Document Index

All project documents listed with English descriptions, grouped by
category. Links point to the actual (German-language) documents.

### Research Documents (Phase 0)

| # | Document | English Description | Words |
|---|----------|-------------------|-------|
| 01 | [01-sensoren-fahrzeugdaten.md](research/01-sensoren-fahrzeugdaten.md) | Vehicle sensors: RPM, speed, coolant temperature, fuel level, oil pressure, gear indication, board voltage — direct sensor tapping on the PD06 with circuit diagrams, signal characteristics, and ESP32 code | ~5,750 |
| 02 | [02-hardware-deep-dive.md](research/02-hardware-deep-dive.md) | Hardware deep dive: ESP32-S3, GPS modules, E-Paper/TFT displays, IMU, barometer, power management (buck/boost/LDO), SD logging, Bluetooth — module selection, prices, pin assignments | ~6,600 |
| 03 | [03-software-stack.md](research/03-software-stack.md) | Software stack: ESP-IDF vs. Arduino-Framework, FreeRTOS task design, GPS driver, display driver, sensor driver, data logging, BLE stack, OTA update, NVS config, open-source projects | ~7,400 |
| 04 | [04-pd06-technische-daten.md](research/04-pd06-technische-daten.md) | PD06 technical data: engine and transmission specs (power, torque, ratios), electrical system (alternator, regulator, battery), chassis, tank and fuel, service manual sources | ~5,000 |
| 05 | [05-gehaeuse-einbau-emv.md](research/05-gehaeuse-einbau-emv.md) | Enclosure, installation, and EMC: 3D-printed case design, water resistance (IP ratings), vibration and shock, temperature range, EMC protection (ignition interference, shielding), load-dump protection, power supply in motorcycle environment | ~5,600 |
| 06 | [06-feature-roadmap.md](research/06-feature-roadmap.md) | Feature roadmap: core features, smart features (BLE + smartphone), safety (crash detection, SOS), tour features, diagnostics, nice-to-have, data export, app concept | ~6,300 |
| 07 | [07-display-navigation-smartphone.md](research/07-display-navigation-smartphone.md) | Display and navigation: multi-display architecture, smartphone integration via BLE, turn-by-turn navigation UI, companion app concept, GATT service design | ~2,400 |
| 08 | [08-feature-realization-matrix.md](research/08-feature-realization-matrix.md) | Feature realization matrix: data availability to feasibility mapping for all features, prioritization with MoSCoW method | ~6,850 |
| 09 | [09-master-plan.md](research/09-master-plan.md) | Master plan: implementation roadmap with cost-benefit analysis, BOM, 3 expansion stages, 5-phase plan (10 weeks), DTC system, support interfaces, QA strategy, risk matrix | ~9,800 |
| 10 | [10-ui-ux-design.md](research/10-ui-ux-design.md) | UI/UX design concept: premium animations, layouts, screens, color palette, widget design, interaction patterns | ~8,300 |
| 11 | [11-firmware-architecture.md](research/11-firmware-architecture.md) | Firmware architecture: rendering engine, pipelines, BLE stack, task model, component design, data flow | ~9,400 |
| 12 | [12-diagnose-support.md](research/12-diagnose-support.md) | Diagnostics and support: DTC system, self-test, remote diagnostics, troubleshooting framework | ~7,600 |
| 13 | [13-connectivity-app.md](research/13-connectivity-app.md) | Connectivity and companion app: GATT services, app architecture, BLE protocol, screen designs | ~7,900 |
| — | [research/README.md](research/README.md) | Research index with summary of findings across all 13 research documents | ~440 |

### Design Documents — Hardware

| Document | English Description | Words |
|----------|-------------------|-------|
| [HARDWARE.md](HARDWARE.md) | Hardware schematic: circuit diagram, pin mapping, power supply design, sensor connections, wiring plan | ~5,500 |
| [PINMAP.md](PINMAP.md) | GPIO assignment for the ESP32-S3: all pins with function and signal, complete pin map | ~1,600 |
| [PCB-LAYOUT.md](PCB-LAYOUT.md) | PCB layout concept: layer stackup, routing rules, EMC optimization, component placement, design constraints | ~6,000 |
| [BOM.md](BOM.md) | Bill of materials: all 67 components across 10 categories with prices, sourcing options, suppliers | ~7,500 |
| [CASE-DESIGN.md](CASE-DESIGN.md) | Enclosure design: 3D-printed case concept, assembly, water resistance, mounting, material selection | ~5,900 |

### Design Documents — Firmware

| Document | English Description | Words |
|----------|-------------------|-------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Integrated system architecture: 8 subsystems, data flows, FreeRTOS task model, BLE protocol, data model, power management | ~8,800 |
| [FIRMWARE-GUIDE.md](FIRMWARE-GUIDE.md) | Firmware engineering guide: build system, component structure, debugging, testing, OTA, release management | ~5,400 |
| [API-REFERENCE.md](API-REFERENCE.md) | BLE GATT interface reference: 8 services with UUIDs, 33 characteristics, data formats, OTA protocol, binary protocol specification | ~7,300 |
| [WIDGET-CATALOG.md](WIDGET-CATALOG.md) | E-Paper widget catalog: 18 widgets, 6 screen layouts, development guide for custom widgets | ~6,600 |

### Design Documents — Testing & Quality

| Document | English Description | Words |
|----------|-------------------|-------|
| [TEST-STRATEGY.md](TEST-STRATEGY.md) | Test strategy and benchmark concept: unit tests, hardware-in-the-loop (HIL), performance testing, test pyramid, coverage targets | ~13,000 |
| [HARDWARE-TEST-BENCH.md](HARDWARE-TEST-BENCH.md) | Hardware test bench specification: signal simulators, test procedures, measurement setup, pass/fail criteria | ~6,200 |
| [BENCHMARK-SPEC.md](BENCHMARK-SPEC.md) | Benchmark specification: target values for CPU, memory, rendering, BLE, sensor, and power performance | ~3,400 |
| [FIELDTEST-PROTOCOL.md](FIELDTEST-PROTOCOL.md) | Field test protocol: 8 test phases, 25 metrics with pass/fail thresholds, real-world evaluation criteria | ~8,200 |
| [PERFORMANCE-OPTIMIZATION.md](PERFORMANCE-OPTIMIZATION.md) | Performance optimization and profiling: CPU, memory, rendering, sensor, BLE, and power optimization strategies | ~8,600 |
| [SECURITY-ANALYSIS.md](SECURITY-ANALYSIS.md) | Security analysis and threat model (STRIDE): 19 identified threats, MoSCoW mitigation plan, secure coding guidelines, code snippets | ~5,900 |

### Design Documents — Operations & Maintenance

| Document | English Description | Words |
|----------|-------------------|-------|
| [CI-CD-PIPELINE.md](CI-CD-PIPELINE.md) | CI/CD pipeline: 4 GitHub Actions workflows, unit tests, brand-name check, build stages, artifacts, release automation | ~3,100 |
| [INSTALLATION-GUIDE.md](INSTALLATION-GUIDE.md) | Installation guide for PD06: PCB mounting, wiring, display installation, commissioning, PD06-specific adjustments | ~8,800 |
| [TROUBLESHOOTING.md](TROUBLESHOOTING.md) | Troubleshooting guide and FAQ: hardware, software, BLE, GPS, SD card, power issues, serial console guide | ~9,200 |
| [TCO-ANALYSIS.md](TCO-ANALYSIS.md) | Total cost of ownership: 3 configuration scenarios (80/327/417 EUR), ROI 38-66%, 5-year lifecycle analysis | ~6,100 |

### Design Documents — User Documentation

| Document | English Description | Words |
|----------|-------------------|-------|
| [USER-MANUAL.md](USER-MANUAL.md) | User manual: operation, displays, app usage, tours, maintenance, configuration, everyday use | ~8,900 |
| [RELEASE-NOTES.md](RELEASE-NOTES.md) | Release notes v1.0: feature list with maturity status, hardware requirements, known limitations, security notes, changelog concept | ~5,100 |

### Design Documents — Developer Resources

| Document | English Description | Words |
|----------|-------------------|-------|
| [DEVELOPER-QUICKSTART.md](DEVELOPER-QUICKSTART.md) | Developer quick-start guide: from zero to running build in under 30 minutes — prerequisites, clone, firmware build, app build, tests, debugging, CI/CD | ~1,950 |

### Project Management Documents

| Document | English Description | Words |
|----------|-------------------|-------|
| [ROADMAP.md](ROADMAP.md) | Project roadmap with all phases (0-5), PM protocol, metrics, worker team, open questions | ~2,700 |

### Root-Level Documents

| Document | English Description |
|----------|-------------------|
| [README.md](../README.md) | Project overview, architecture diagram, project structure, documentation index, feature list, hardware components, CI/CD, metrics |
| [CONTRIBUTING.md](../CONTRIBUTING.md) | Contributing guide: how to contribute, code standards, CI/CD pipeline, review process, license, issue and PR templates |

**Total: 49 documents, approximately 262,000 words.**

---

## 4. Translation Guide

This project is primarily documented in German. This section
provides guidance for contributors who wish to translate documents
into English or other languages.

### 4.1 How to Contribute Translations

1. **Check existing translations.** Before starting, search the
   repository for any existing or in-progress translation work.
   English translations should be placed alongside the original German
   document with a language suffix, for example:
   `docs/ARCHITECTURE.en.md` or `docs/en/ARCHITECTURE.md`.

2. **Open an issue.** Create a GitHub issue using the feature request
   template, indicating which document you intend to translate and
   into which language. This prevents duplicate work.

3. **Fork and branch.** Follow the standard contribution workflow
   described in `CONTRIBUTING.md`. Create a branch with the prefix
   `docs/translate/`, for example: `docs/translate/architecture-en`.

4. **Translate.** Keep the original document structure, heading
   hierarchy, and formatting. Do not restructure or reorganize the
   content — translation only, no editorial changes. Preserve all
   code blocks, tables, and diagrams as-is. Only the prose should be
   translated.

5. **Verify brand-name compliance.** After translation, run the
   brand-name check locally to ensure no brand names have been
   introduced:

   ```bash
   while IFS= read -r pattern; do
       [ -z "$pattern" ] && continue
       grep -rinE "$pattern" docs/ && echo "VIOLATION: $pattern"
   done < .github/brand-blocklist.txt
   ```

6. **Submit a pull request.** Reference the translation issue in the
   PR description. The CI pipeline will automatically check brand-name
   compliance, markdown formatting, and word count.

### 4.2 Translation Priority List

Documents are prioritized for translation based on their importance
for international contributors and users. The following priority list
defines which documents should be translated first:

**Priority 1 — Entry Points (translate first):**

1. `README.md` — The project overview is the first document anyone
   reads. An English version is essential for project discoverability.
2. `docs/ARCHITECTURE.md` — The integrated system architecture is the
   primary technical reference for developers.
3. `docs/DEVELOPER-QUICKSTART.md` — The quick-start guide gets new
   contributors running within 30 minutes.
4. `CONTRIBUTING.md` — The contributing guide is essential for anyone
   who wants to participate.
5. `docs/RELEASE-NOTES.md` — Release notes provide the feature overview
   and known limitations.

**Priority 2 — Core Technical References:**

6. `docs/FIRMWARE-GUIDE.md` — Firmware engineering guide for build,
   debug, and OTA.
7. `docs/API-REFERENCE.md` — BLE GATT interface reference for app
   developers.
8. `docs/HARDWARE.md` — Hardware schematic and pin mapping.
9. `docs/PINMAP.md` — GPIO assignment (short but critical).
10. `docs/CI-CD-PIPELINE.md` — CI/CD pipeline documentation.

**Priority 3 — User-Facing Documentation:**

11. `docs/USER-MANUAL.md` — User manual for end users.
12. `docs/INSTALLATION-GUIDE.md` — Installation guide for PD06.
13. `docs/TROUBLESHOOTING.md` — Troubleshooting guide and FAQ.

**Priority 4 — Specialized References (translate on request):**

14. `docs/BOM.md` — Bill of materials.
15. `docs/SECURITY-ANALYSIS.md` — Security analysis and threat model.
16. `docs/TEST-STRATEGY.md` — Test strategy.
17. `docs/PERFORMANCE-OPTIMIZATION.md` — Performance optimization.
18. `docs/TCO-ANALYSIS.md` — Total cost of ownership.
19. Research documents (`docs/research/01` through `13`) — Translate
    individually as needed.

### 4.3 Translation Conventions

#### General Rules

- **Preserve structure.** Keep the original heading hierarchy,
  numbered lists, tables, and code blocks exactly as in the source.
- **Do not restructure.** A translation is a faithful rendering of
  the source content into another language, not a rewrite. If the
  source has a section ordering that seems suboptimal, propose that
  as a separate issue, not in the translation PR.
- **Keep code and identifiers untranslated.** Variable names,
  function names, CLI commands, file paths, and code snippets remain
  in their original form.
- **Preserve links.** All internal document links should point to
  the original (German) document unless an English translation exists,
  in which case link to both.
- **Add a translation header.** At the top of each translated file,
  include a note indicating the source language and the date of
  translation:

  ```
  > This is an English translation of docs/ARCHITECTURE.md.
  > The original German document is authoritative.
  > Last synchronized: YYYY-MM-DD
  ```

- **Synchronize.** When the original German document is updated, the
  translation should be updated as well. Open an issue if a
  translation falls behind.

#### Terminology Glossary (German to English)

The following glossary defines the standard English translations for
recurring German terms in this project. Translators should use these
mappings consistently across all documents.

**Core Terms:**

| German | English | Notes |
|--------|---------|-------|
| Bordcomputer | Onboard computer | The central device of this project |
| Fahrzeugdaten | Vehicle data | Data captured from the motorcycle |
| Sensor-Anzapfung | Sensor tapping | Direct signal interception |
| Zuendung | Ignition | |
| Zuendspule | Ignition coil | |
| Bordspannung | Board voltage | Motorcycle's 12V electrical system voltage |
| Bordnetz | Electrical system / onboard network | The motorcycle's 12V power network |
| Schaltplan | Circuit diagram / schematic | |
| Stueckliste | Bill of materials (BOM) | |
| Gehaeuse | Enclosure / case | |
| Kuehlmitteltemperatur | Coolant temperature | |
| Kraftstoffstand | Fuel level | |
| Oeldruck | Oil pressure | |
| Drehzahl | Engine speed (RPM) | |
| Tacho | Speedometer | |
| Tankgeber | Fuel tank sender | Resistance-based fuel level sensor |
| Lichtmaschine | Alternator | |
| Gang-Anzeige | Gear indication | |
| Freertos task | FreeRTOS task | Keep "FreeRTOS" in English |
| Zustandsmaschine | State machine | |
| Hochspannung | High voltage | |
| Sicherung | Fuse | |

**Hardware Terms:**

| German | English | Notes |
|--------|---------|-------|
| Leiterplatte | Printed circuit board (PCB) | |
| Pin-Belegung | Pin assignment / pin mapping | |
| Schichtaufbau | Layer stackup | PCB layer structure |
| Verdrahtungsplan | Wiring plan | |
| Stromversorgung | Power supply | |
| Spannungsteiler | Voltage divider | |
| Optokoppler | Optocoupler | Galvanic isolation component |
| Relais | Relay | |
| Heizgriffe | Heated grips | |
| Zusatzlicht | Auxiliary light | |
| Sicherung | Fuse | |
| Stecker | Connector | |
| Kabelbaum | Wiring harness | |
| Ladebuchse | Charging port | |
| Akku | Battery | Rechargeable battery |
| Backup-Akku | Backup battery | The 18650 LiPo cell |
| Schutzdiode | Protection diode | |

**Software & Firmware Terms:**

| German | English | Notes |
|--------|---------|-------|
| Treiber | Driver | Hardware driver |
| Firmware | Firmware | Same in both languages |
| Komponente | Component | ESP-IDF component |
| Task-Prioritaet | Task priority | FreeRTOS scheduling priority |
| Protokoll | Protocol | |
| Kennlinie | Calibration curve | Sensor response curve |
| Filter | Filter | Signal filter (EMA, median, etc.) |
| Ereignis | Event | |
| Protokollierung | Logging | Data recording |
| Konfiguration | Configuration | |
| Menuefuehrung | Menu navigation | |
| Bildschirm | Screen / display | |
| Bildschirm-Layout | Screen layout | |

**Motorcycle & Vehicle Terms:**

| German | English | Notes |
|--------|---------|-------|
| Motorrad | Motorcycle | |
| Vorderrad | Front wheel | |
| Huelse | Hose / sleeve | Kuehlschlauch = coolant hose |
| Kuehlschlauch | Coolant hose | |
| Oelpumpe | Oil pump | |
| Getriebe | Transmission / gearbox | |
| Kupplung | Clutch | |
| Lenker | Handlebar | |
| Tachowelle | Speedometer cable | Mechanical speedometer drive |
| Zündkerze | Spark plug | |
| Verbrennungsmotor | Internal combustion engine | |
| Zylinder | Cylinder | |

**Process & Project Terms:**

| German | English | Notes |
|--------|---------|-------|
| Beitragende | Contributors | |
| Beitragende-Leitfaden | Contributing guide | |
| Schnellstart | Quick start | |
| Fehlerbehebung | Troubleshooting | |
| Wartung | Maintenance | |
| Inbetriebnahme | Commissioning | First startup |
| Freigabe | Release | |
| Versionsnummer | Version number | |
| Aenderungsprotokoll | Changelog | |
| Richtlinie | Guideline / policy | |
| Blockliste | Blocklist | Brand-name blocklist |
| Markennamen | Brand names | |

---

## 5. Quick Start

This is a condensed English summary of the
[Developer Quick-Start Guide](DEVELOPER-QUICKSTART.md) (German). It
covers the five key commands to get from a fresh clone to a running
firmware and companion app build.

### Prerequisites

- ESP-IDF v5.4 with toolchain installed (one-time setup)
- Python 3.8+
- Node.js 18+ (for companion app)
- Git 2.x
- GCC (for host unit tests only)

ESP-IDF installation (one-time):

```bash
mkdir -p ~/esp && cd ~/esp
git clone -b v5.4 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32s3
```

### Step 1: Clone the Repository

```bash
git clone https://github.com/TitoPrausee/transalp-bordcomputer.git
cd transalp-bordcomputer
```

### Step 2: Build the Firmware

```bash
. ~/esp/esp-idf/export.sh
cd firmware
idf.py set-target esp32s3
idf.py build
```

Expected build time: 3-5 minutes (first build), under 30 seconds
(incremental). The firmware binary will be at
`firmware/build/transalp_bordcomputer.bin` (approximately 1.2-1.8 MB).

To flash and monitor (requires an ESP32-S3 board connected via USB):

```bash
idf.py -p /dev/cu.usbmodem* flash monitor
```

### Step 3: Build the Companion App

```bash
cd companion-app
npm install
npm start
```

This starts the Metro bundler. Scan the QR code with the Expo app on
your phone, or press `a` for the Android emulator or `i` for the iOS
simulator.

TypeScript type check:

```bash
npm run lint
```

### Step 4: Run the Unit Tests

```bash
cd firmware/test
git clone https://github.com/ThrowTheSwitch/Unity /tmp/unity
mkdir build && cd build
cmake -DUNITY_DIR=/tmp/unity ..
make
ctest
```

Three test modules are available: `test_nmea_parser` (GPS NMEA
parsing), `test_dtc` (diagnostic trouble code system), and
`test_event_loop` (event bus).

### Step 5: View the UI Mockups

```bash
open mockups/index.html
```

The mockups directory contains 8 HTML/CSS screens (boot, instrument,
navigation, diagnose, settings, split, tour statistics, warning) and
24 SVG icons that show the intended UI design for both the onboard
display and the companion app.

### Common Issues

| Problem | Solution |
|---------|----------|
| `idf.py: command not found` | Run `. ~/esp/esp-idf/export.sh` |
| Build fails: `ESP_ERR_NVS` | Run `idf.py erase-flash`, then reflash |
| BLE does not connect | Check NimBLE config, read advertising logs |
| SD card not detected | Check SPI pins in `firmware/main/board_config.h` |
| Display stays white | Check backlight pin (`TFT_BL_PIN`) |
| `npm install` fails | Run `rm -rf node_modules && npm install` |

For full troubleshooting, see [Troubleshooting Guide](TROUBLESHOOTING.md).

### Where to Go Next

| Topic | Document |
|-------|----------|
| System architecture, task model | `docs/ARCHITECTURE.md` |
| Firmware engineering (build, debug, OTA) | `docs/FIRMWARE-GUIDE.md` |
| BLE GATT API (services, characteristics) | `docs/API-REFERENCE.md` |
| CI/CD pipeline (all stages, artifacts) | `docs/CI-CD-PIPELINE.md` |
| Hardware mounting and wiring | `docs/INSTALLATION-GUIDE.md` |
| Troubleshooting and FAQ | `docs/TROUBLESHOOTING.md` |
| Pin assignment ESP32-S3 | `docs/PINMAP.md` |
| Performance optimization | `docs/PERFORMANCE-OPTIMIZATION.md` |
| Research documents | `docs/research/README.md` |

---

## License

This project is licensed under the **PolyForm Noncommercial License
1.0.0**. See [LICENSE](../LICENSE) for the full text.

- Permitted: Private and educational use, modification, sharing under
  the same license
- Prohibited: Commercial use, marketing, rebranding

---

*This document serves as the English entry point for the Transalp
Bordcomputer project. The original documentation is in German. For
authoritative content, always refer to the original German documents
linked throughout this summary.*

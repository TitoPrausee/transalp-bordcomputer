# Beitragende-Leitfaden (Contributing Guide)

> Transalp Bordcomputer — Open-Source Bordcomputer fuer die Honda Transalp 600 (PD06)
> Plattform: ESP32-S3-WROOM-1 (N16R8), ESP-IDF v5.4, FreeRTOS SMP
> Lizenz: PolyForm Noncommercial 1.0.0
> Stand: Oktober 2026

---

## Inhaltsverzeichnis

1. [Willkommen](#1-willkommen)
2. [Erste Schritte](#2-erste-schritte)
3. [Beitragstypen](#3-beitragstypen)
4. [Code-Standards](#4-code-standards)
5. [CI/CD Pipeline](#5-cicd-pipeline)
6. [Review-Prozess](#6-review-prozess)
7. [Lizenz](#7-lizenz)

---

## 1. Willkommen

Der Transalp Bordcomputer ist ein Open-Source-Projekt, das einen
vollstaendigen Bordcomputer fuer die Honda XL600V Transalp (PD06,
Baujahr 1986-2000) entwickelt. Die PD06 ist eine rein mechanische
Maschine ohne CAN-Bus, OBD-II oder elektronische Motorsteuerung.
Alle Fahrzeugdaten werden ueber eigenstaendige Sensoren und direkte
Signal-Anzapfung erfasst. Das System basiert auf einem ESP32-S3-MCU
mit FreeRTOS, GPS-Tracking, Multi-Display (E-Paper + TFT),
BLE-Smartphone-Integration und umfassendem Power-Management.

Wir freuen uns ueber jeden Beitrag. Wir suchen:

- **Bug-Reports** — Fehler finden und strukturiert melden
- **Features** — Neue Funktionen vorschlagen und implementieren
- **Dokumentation** — Guides, Tutorials, Uebersetzungen, Diagramme
- **Tests** — Unit-Tests, Integrationstests, Hardware-Tests
- **Hardware** — Schaltplan-Verbesserungen, Gehaeuse-Designs,
  Sensor-Kalibrierung

### Code of Conduct

Wir erwarten von allen Beitragenden ein respektvolles, konstruktives
und inklusives Verhalten. Jede Form von Diskriminierung, Belaestigung
oder toxischem Verhalten wird nicht toleriert. Begegne anderen mit
Respekt, akzeptiere konstruktive Kritik und fokussiere dich auf das,
was fuer das Projekt am besten ist.

---

## 2. Erste Schritte

### 2.1 Repo klonen

```bash
git clone https://github.com/TitoPrausee/transalp-bordcomputer.git
cd transalp-bordcomputer
```

### 2.2 Entwicklungsumgebung

Dieses Projekt hat zwei Build-Targets: die Firmware (ESP32-S3) und
die Companion-App (React Native).

#### Firmware (ESP-IDF v5.x)

Die Firmware basiert auf dem ESP-IDF (Espressif IoT Development
Framework) in der Version v5.4. Die komplette Einrichtung des
ESP-IDF, inklusive Cross-Compiler und Build-Tools, ist in
[`docs/FIRMWARE-GUIDE.md`](docs/FIRMWARE-GUIDE.md) beschrieben.

Kurzanleitung:

```bash
# ESP-IDF klonen und einrichten
mkdir -p ~/esp && cd ~/esp
git clone -b v5.4 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3

# In jedem neuen Terminal aktivieren
. ~/esp/esp-idf/export.sh
```

Voraussetzungen:
- Python 3.9+
- CMake 3.16+
- Ninja Build
- Git

#### Companion-App (Node.js / React Native)

Die Companion-App ist eine Expo-basierte React-Native-Anwendung mit
TypeScript. Sie ist in [`companion-app/`](companion-app/) abgelegt.

```bash
# Node.js 18.x oder 20.x installieren
cd companion-app
npm install        # Abhaengigkeiten installieren
npm start          # Metro Bundler starten
```

Voraussetzungen:
- Node.js 18.x oder 20.x
- npm
- Android SDK (fuer APK-Build) oder iOS-Toolchain (fuer iOS-Build)

### 2.3 Build-Anleitung

#### Firmware bauen

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
```

Nach dem Build sind folgende Binaries in `firmware/build/` verfuegbar:

| Datei | Beschreibung |
|---|---|
| `transalp_bordcomputer.bin` | Haupt-Firmware (Offset 0x10000) |
| `bootloader.bin` | Bootloader (Offset 0x0) |
| `partition_table.bin` | Partitionstabelle (Offset 0x8000) |

Die Firmware hat neun Komponenten (`ble`, `diagnostics`, `display`,
`gps`, `ota`, `power`, `sensors`, `storage`, `utils`), die alle ueber
die Top-Level `CMakeLists.txt` mit `EXTRA_COMPONENT_DIRS` automatisch
gebaut werden. Details zur Komponenten-Struktur siehe
[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

#### Companion-App bauen

```bash
cd companion-app
npm run lint       # TypeScript-Check (tsc --noEmit)
npm run android    # Android-Build (erfordert Android SDK)
```

### 2.4 Flashen und Monitor

```bash
cd firmware
idf.py -p /dev/cu.usbmodem* flash monitor
```

Auf Linux ist das Device meistens `/dev/ttyUSB0`, auf macOS
`/dev/cu.usbmodem*`.

### 2.5 Tests ausfuehren

#### Firmware Unit-Tests

Die Unit-Tests laufen auf dem Host-PC mit GCC und dem
Unity-Test-Framework. Eine Mock-Schicht bildet die ESP-IDF-Hardware-
Abstraktion nach, sodass keine echte Hardware fuer die Tests
benoetigt wird.

```bash
cd firmware/test
gcc -I ../components/*/include -I mocks \
    -I /tmp/unity/src test_*.c mocks/*.c \
    /tmp/unity/src/unity.c -o test_runner -lm -lpthread
./test_runner
```

Verfuegbare Test-Module:

| Test | Beschreibung |
|---|---|
| `test_nmea_parser.c` | NMEA-Parser: GGA, RMC, VTG, Checksum |
| `test_event_loop.c` | Event-Bus: Set/Clear/Wait |
| `test_dtc.c` | DTC-System: Registrierung, Zaehler, Clear |

Details zur Test-Strategie, Mock-Architektur und Coverage-Zielen
siehe [`docs/CI-CD-PIPELINE.md`](docs/CI-CD-PIPELINE.md) Abschnitt 6.

#### Companion-App Tests

```bash
cd companion-app
npm run lint        # tsc --noEmit (Type-Check)
```

Wenn Jest-Tests vorhanden sind (in `companion-app/__tests__/`):

```bash
npx jest
```

### 2.6 Track-Export-Tool

```bash
python3 tools/export_track.py /sdcard/logs/SESSION_20261013_140000.csv -o track.gpx
```

---

## 3. Beitragstypen

### 3.1 Bug Reports

Bevor ein Bug-Report erstellt wird, bitte die vorhandenen Issues
durchsuchen, um Duplikate zu vermeiden:

```bash
gh issue list --search "dein-suchbegriff"
```

Ein guter Bug-Report enthaelt folgende Informationen:

- **Zusammenfassung**: Kurze Beschreibung des Fehlers
- **Schritte zur Reproduktion**: Nummerierte, nachvollziehbare Anleitung
- **Erwartetes Verhalten**: Was haette passieren sollen
- **Tatsaechliches Verhalten**: Was ist passiert
- **Logs**: Serielle Konsole, App-Log oder Crash-Dump
- **Hardware**: Welches MCU-Board, welche Sensoren, welche Firmware-Version
- **Umgebung**: ESP-IDF-Version, Betriebssystem, Companion-App-Version

Nutze das Bug-Report-Template unter
[`.github/ISSUE_TEMPLATE/bug_report.md`](.github/ISSUE_TEMPLATE/bug_report.md),
falls vorhanden.

### 3.2 Feature Requests

Feature Requests sollten den Use-Case beschreiben, nicht nur eine
Loesung. Erklaere, warum das Feature fuer das Projekt oder fuer
andere Nutzer der PD06 wertvoll ist.

Dabei ist die PD06-Relevanz zu beachten: Da die PD06 eine rein
mechanische Maschine ohne CAN-Bus oder ECU ist, muessen alle
Fahrzeugdaten ueber eigenstaendige Sensoren erfasst werden. Ein
Feature-Request sollte erklaeren, wie die Datenquelle fuer das
Feature aussehen wuerde.

Nutze das Feature-Request-Template unter
[`.github/ISSUE_TEMPLATE/feature_request.md`](.github/ISSUE_TEMPLATE/feature_request.md),
falls vorhanden.

### 3.3 Code-Beitraege

#### Fork, Branch, PR Workflow

1. Fork das Repository auf GitHub
2. Klone deinen Fork und setze das Upstream-Repo:

```bash
git remote add upstream https://github.com/TitoPrausee/transalp-bordcomputer.git
git remote set-url --push upstream no-push
```

3. Erstelle einen Feature-Branch vom `main`-Branch:

```bash
git checkout main
git pull upstream main
git checkout -b feature/dein-topic
```

4. Implementiere deine Aenderungen
5. Commite mit konventionellen Commit-Messages (siehe unten)
6. Push zu deinem Fork und oeffne einen Pull Request gegen `main`

#### Branch-Naming

| Praefix | Verwendung | Beispiel |
|---|---|---|
| `feature/` | Neue Funktion | `feature/canbus-emulation` |
| `fix/` | Bug-Fix | `fix/nmea-checksum-overflow` |
| `docs/` | Dokumentation | `docs/contributing` |
| `firmware/` | Firmware-Treiber oder Komponente | `firmware/gps-driver` |
| `test/` | Tests | `test/ble-integration` |
| `refactor/` | Refactoring ohne Funktionsaenderung | `refactor/display-task` |

#### Commit-Messages (Konventionelle Commits)

Commit-Messages folgen dem [Conventional Commits](https://www.conventionalcommits.org/)
Standard:

```
type(scope): subject

type:    feat, fix, docs, style, refactor, test, chore, ci, build, perf
scope:   firmware, app, docs, ci, ble, gps, sensors, display, storage, ...
subject: kurze Beschreibung im Imperativ
```

Beispiele:

```
feat(gps): NMEA-Parser um GSA-Satz erweitern
fix(ble): Verbindungabbruch bei leerem RX-Buffer behandeln
docs(ci): CI/CD-Pipeline Dokumentation aktualisieren
test(sensors): Unit-Tests fuer MPU6050-Filter hinzufuegen
chore(deps): ESP-IDF auf v5.4 aktualisieren
```

Die Changelog-Generierung der Release-Pipeline liest diese Commit-Types
automatisch aus. Jeder Type wird zu einem Abschnitt im Changelog:

- `feat` → Neuerungen
- `fix` → Fehlerbehebungen
- `docs` → Dokumentation
- Alle anderen → Wartung

Details zur Changelog-Generierung siehe
[`docs/CI-CD-PIPELINE.md`](docs/CI-CD-PIPELINE.md) Abschnitt 5.2.

---

## 4. Code-Standards

### 4.1 Firmware (C)

#### ESP-IDF Coding-Standard

Die Firmware folgt dem offiziellen ESP-IDF Coding-Standard.
Die wichtigsten Regeln:

- 4 Spaces Einrueckung, keine Tabs
- maximal 120 Zeichen pro Zeile
- Funktions- und Variablennamen in `snake_case`
- Header-Include-Guards in der Form `#ifndef _MODULE_H_` /
  `#define _MODULE_H_` / `#endif`
- `const`-Korrektur fuer alle Parameter, die nicht modifiziert werden
- Kein `malloc` in ISR-Kontext (Interrupt Service Routines)
- FreeRTOS-Primitives (Queues, Mutexe, EventGroups) fuer
  Thread-Synchronisation verwenden

Der Coding-Standard wird durch `clang-format` in der CI-Pipeline
erzwungen. Die Konfiguration sollte mit dem ESP-IDF-Standard
uebereinstimmen.

#### Statische Analyse

Die CI-Pipeline fuehrt zwei statische Analyse-Tools aus:

- **cppcheck**: Prueft alle `.c`- und `.h`-Dateien in `firmware/`
  mit `--enable=all --error-exitcode=1`. Errors brechen den Build ab,
  Warnings sind tolerierbar.
- **clang-format**: Dry-Run mit `--Werror`. Wenn der Code nicht dem
  Formatierungs-Standard entspricht, schlaegt der Build fehl.

Zusaetzlich wird ein Header-Include-Guard-Check durchgefuehrt, der
sicherstellt, dass jede `.h`-Datei einen Include-Guard enthaelt.

#### Test-Abdeckung

Neue Komponenten oder signifikante Aenderungen an bestehenden
Komponenten muessen mit Unit-Tests abgedeckt sein. Die
Coverage-Ziele (aus [`docs/CI-CD-PIPELINE.md`](docs/CI-CD-PIPELINE.md)
Abschnitt 6.4):

| Bereich | Coverage-Ziel |
|---|---|
| Core-Treiber (RPM, Temp, Fuel, GPS, Gear) | > 80% |
| Filter-Algorithmen (Kalman, Complementary, Moving-Avg) | > 90% |
| DTC-System (Set/Clear/Persistenz) | > 85% |
| Konfigurations-Parser (JSON) | > 80% |

Die Tests werden mit GCC auf dem Host-PC kompiliert. Eine
Mock-Schicht (`firmware/test/mocks/`) bildet FreeRTOS-Primitives
ueber POSIX-Threads nach. Details siehe
[`docs/CI-CD-PIPELINE.md`](docs/CI-CD-PIPELINE.md) Abschnitt 6.3.

### 4.2 Companion-App (TypeScript)

#### ESLint und Prettier

Die Companion-App verwendet TypeScript im `strict`-Modus
(`tsconfig.json` hat `strict: true`). Der wichtigste Quality-Gate in
der CI ist `tsc --noEmit`, der alle Typ-Fehler frueh erfasst.

```bash
cd companion-app
npm run lint        # tsc --noEmit
```

Konventionen:
- TypeScript `strict` mode ist Pflicht
- `import`-Statements werden sortiert (automatisch konfigurierbar)
- Kein `any` — explizite Typen verwenden
- React Native Best Practices folgen (Functional Components, Hooks)
- State-Management ueber Zustand-Store

Die App ist Expo-basiert (`companion-app/package.json` mit
`expo ~52.0.0`, `react 18.3.1`, `react-native 0.76.5`).
Die Projektstruktur nutzt Path-Aliase (`@/*` → `src/*`).

#### BLE-Integration

Die App verwendet `react-native-ble-plx` fuer die BLE-Kommunikation
mit dem Bordcomputer. Das GATT-Protokoll mit allen Services und
Characteristics ist in [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
Abschnitt 5 dokumentiert.

### 4.3 Dokumentation

#### Sprache und Formatierung

- Alle Dokumentation wird auf Deutsch verfasst (Projekt-Standard)
- Markdown mit H2/H3-Ueberschriften strukturieren
- Code-Bloecke muessen eine Sprache angeben (```bash, ```c, etc.)
- Keine Tabs, nur Spaces
- Maximal eine Leerzeile am Dateiende
- Ueberschriften muessen inkrementell steigen (kein Sprung von H1 zu H4)

Diese Regeln werden durch `markdownlint` in der Docs-CI-Pipeline
erzwungen.

#### Mindestlaenge

Konzept- und Research-Dokumente muessen eine Mindestwoerterzahl
erfuellen, die in der CI-Pipeline automatisch geprueft wird:

| Dokument-Typ | Pfad | Mindest-Woerter |
|---|---|---|
| Konzept-Dokumente | `docs/*.md` (ohne Unterverzeichnis) | 500 |
| Research-Dokumente | `docs/research/*.md` | 1000 |

#### Markennamen-Blocklist

Dieses Open-Source-Projekt darf keine geschuetzten Markennamen von
Fremdherstellern enthalten. Die CI-Pipeline prueft automatisch alle
`.md`- und `.yml`/`.yaml`-Dateien gegen eine Blocklist.

Die Blocklist befindet sich in
[`.github/brand-blocklist.txt`](.github/brand-blocklist.txt) und
enthaelt Regex-Patterns fuer:

- Smartphone- und Computerhersteller
- Suchmaschinen-Anbieter
- Social-Media-Plattformen
- Betriebssystem-Markennamen fuer mobile Plattformen

Wenn ein Markenname gefunden wird, schlaegt der `brand-check`-Job
fehl und der Pull Request kann nicht gemerged werden. Um einen neuen
Markennamen zur Blocklist hinzuzufuegen, fuege eine Zeile mit dem
entsprechenden Regex-Pattern zur Blocklist-Datei hinzu.

**Wichtig**: Dies gilt auch fuer Code-Kommentare, Variablennamen und
Pull-Request-Beschreibungen. Verwende stattdessen generische
Beschreibungen (z.B. "mobile Betriebssysteme" statt eines
Markennamens).

#### Referenziere existierende Dokumente

Wiederhole nicht Inhalte, die bereits in anderen Dokumenten
beschrieben sind. Verlinke stattdessen auf das jeweilige Dokument.
Die wichtigsten Referenzdokumente:

| Dokument | Inhalt |
|---|---|
| [`README.md`](README.md) | Projekt-Uebersicht, Schnellstart |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | System-Architektur, Task-Modell, BLE-Protokoll |
| [`docs/FIRMWARE-GUIDE.md`](docs/FIRMWARE-GUIDE.md) | Build, Flash, Debug, Test, Release, OTA |
| [`docs/CI-CD-PIPELINE.md`](docs/CI-CD-PIPELINE.md) | CI/CD-Pipeline, Test-Strategie, Release |
| [`docs/SECURITY-ANALYSIS.md`](docs/SECURITY-ANALYSIS.md) | Threat Model, Secure Coding |
| [`docs/HARDWARE.md`](docs/HARDWARE.md) | Schaltplan, Pin-Mapping, Stromversorgung |
| [`docs/PINMAP.md`](docs/PINMAP.md) | GPIO-Belegung ESP32-S3 |

---

## 5. CI/CD Pipeline

Alle Pull Requests muessen die CI-Tests bestehen, bevor sie gemerged
werden koennen. Die Pipeline besteht aus drei unabhaengigen
Workflows, die durch Dateipfad-Filter getriggert werden.

### 5.1 firmware-ci

Datei: [`.github/workflows/firmware-ci.yml`](.github/workflows/firmware-ci.yml)

Trigger: Push/PR auf `firmware/**` oder die Workflow-Datei selbst.

| Job | Beschreibung |
|---|---|
| `esp-idf-build` | Kompiliert alle Komponenten mit ESP-IDF v5.2, v5.3, v5.4 (Matrix) |
| `unit-tests` | Kompiliert und fuehrt Unit-Tests auf Host-PC mit GCC + Unity aus |
| `static-analysis` | cppcheck, clang-format Dry-Run, Header-Include-Guard-Check |

Quality-Gates:
- Flash-Verbrauch muss unter 4 MB liegen (16 MB Flash / 4 Partitionen)
- RAM-Verbrauch muss unter 320 KB liegen
- Alle Unit-Tests muessen bestehen (100% Pass-Rate)
- cppcheck darf keine Errors melden

### 5.2 companion-app-ci

Datei: [`.github/workflows/companion-app-ci.yml`](.github/workflows/companion-app-ci.yml)

Trigger: Push/PR auf `companion-app/**`.

| Job | Beschreibung |
|---|---|
| `lint` | `tsc --noEmit` (TypeScript strict mode Type-Check) |
| `build` | Metro Bundler Build, APK-Build (falls Android SDK verfuegbar) |
| `test` | Jest-Unit-Tests (falls konfiguriert) |

### 5.3 docs-ci

Datei: [`.github/workflows/docs-ci.yml`](.github/workflows/docs-ci.yml)

Trigger: Push/PR auf `docs/**`.

| Job | Beschreibung |
|---|---|
| `markdown-lint` | Prueft Markdown-Formatierung (Tabs, Leerzeilen, Ueberschriften) |
| `brand-check` | Durchsucht alle `.md`/`.yml`-Dateien nach Markennamen (Blocklist) |
| `link-check` | Validiert interne und externe Links mit `lychee` |
| `word-count` | Prueft Mindestwoerterzahl fuer Konzept- und Research-Dokumente |

Die Brand-Blocklist wird bei jedem PR automatisch geprueft. Der
`brand-check`-Job liest die Patterns aus
[`.github/brand-blocklist.txt`](.github/brand-blocklist.txt) und
durchsucht das gesamte Repository. Ein einziger Treffer blockiert den
Merge.

### 5.4 Release-Pipeline

Releases werden durch Git-Tags im Format `vMAJOR.MINOR.PATCH`
ausgeloest (Semantic Versioning). Der Release-Workflow baut Firmware
und App, generiert einen Changelog aus den Commit-Messages und laedt
alle Artifacts als GitHub-Release hoch.

Details siehe [`docs/CI-CD-PIPELINE.md`](docs/CI-CD-PIPELINE.md)
Abschnitt 5.

---

## 6. Review-Prozess

### 6.1 Ablauf

1. Der Pull Request wird von mindestens einem Maintainer reviewt
2. Alle CI-Checks muessen gruen sein
3. Der Brand-Check darf keine Markennamen finden
4. Code-Qualitaet und PD06-Korrektheit werden geprueft

### 6.2 Review-Kriterien

| Kriterium | Beschreibung |
|---|---|
| CI gruen | Alle relevanten Jobs (Firmware, App, Docs) muessen bestehen |
| Keine Markennamen | Brand-Check muss 0 Violations melden |
| Code-Qualitaet | Lesbarkeit, Wartbarkeit, keine Code-Duplikation |
| PD06-Korrektheit | Funktionalitaet muss zur PD06-Plattform passen |
| Test-Abdeckung | Neue Funktionen muessen getestet sein |
| Dokumentation | Signifikante Aenderungen muessen dokumentiert sein |

### 6.3 Was Maintainer pruefen

- Entspricht der Code den in Abschnitt 4 definierten Standards?
- Sind die Tests ausreichend und decken sie Edge-Cases ab?
- Ist die Dokumentation (falls erforderlich) aktualisiert worden?
- Enthaelt der Code keine Markennamen (wird automatisch geprueft)?
- Sind keine Secrets, Tokens oder PII im Code oder in der
  PR-Beschreibung enthalten?

### 6.4 Aenderungen anfordern

Wenn Aenderungen angefordert werden, bitte die Review-Kommentare
adressieren, die Aenderungen committen und pushen. Die CI-Pipeline
wird automatisch neu gestartet. Der Reviewer wird erneut benachrichtigt.

---

## 7. Lizenz

Dieses Projekt steht unter der **PolyForm Noncommercial License
1.0.0**. Der vollstaendige Lizenztext befindet sich in
[`LICENSE`](LICENSE).

Durch das Einreichen eines Beitrags (Code, Dokumentation, Tests,
Assets) stimmst du zu, dass dein Beitrag unter derselben Lizenz wie
das Projekt veroeffentlicht wird.

Zusammenfassung der Lizenz:

- Erlaubt: Private und Bildungsnutzung, Modifikation, Weiterverbreitung
- Verboten: Kommerzielle Nutzung, Vermarktung, Rebranding

---

## Quick Reference

| Aktion | Befehl |
|---|---|
| Repo klonen | `git clone https://github.com/TitoPrausee/transalp-bordcomputer.git` |
| ESP-IDF aktivieren | `. ~/esp/esp-idf/export.sh` |
| Firmware bauen | `cd firmware && idf.py set-target esp32s3 && idf.py build` |
| Firmware flashen | `idf.py -p /dev/cu.usbmodem* flash monitor` |
| Unit-Tests (Firmware) | `cd firmware/test && gcc -I ../components/*/include -I mocks ... test_*.c -o test_runner && ./test_runner` |
| App installieren | `cd companion-app && npm install` |
| App linten | `cd companion-app && npm run lint` |
| App starten | `cd companion-app && npm start` |
| Branch erstellen | `git checkout -b feature/dein-topic` |
| PR oeffnen | `gh pr create --base main` |

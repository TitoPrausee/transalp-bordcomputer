# CI/CD-Pipeline & Automated Testing Konzept

> Bordcomputer Transalp 600 (PD06)
> MCU: ESP32-S3-WROOM-1 (N16R8: 16 MB Flash, 8 MB Octal PSRAM)
> Framework: ESP-IDF v5.4 mit FreeRTOS SMP
> Stand: Oktober 2026
> Quellen: docs/TEST-STRATEGY.md, docs/HARDWARE-TEST-BENCH.md,
> docs/BENCHMARK-SPEC.md, docs/FIRMWARE-GUIDE.md,
> firmware/CMakeLists.txt, firmware/main/CMakeLists.txt,
> firmware/components/*/CMakeLists.txt, companion-app/package.json

---

## 0. Zusammenfassung

Dieses Dokument beschreibt die Continuous-Integration- und
Continuous-Delivery-Pipeline (CI/CD) fuer den Transalp Bordcomputer.
Die Pipeline automatisiert den gesamten Weg vom Code-Commit bis zum
release-faehigen Firmware-Binary und der Companion-App. Sie besteht aus
drei parallelen Workflow-Dateien, die unabhaengig voneinander durch
Dateipfad-Filter getriggert werden: Firmware-CI, Companion-App-CI und
Docs-CI. Jede Pipeline-Stufe hat klare Pass/Fail-Kriterien, und ein
Pull-Request kann erst gemerged werden, wenn alle relevanten Jobs
gruen sind.

Die Pipeline-Architektur folgt einer klassischen Stufen-Struktur
(Lint, Build, Unit Test, Integration, Deploy) und ist so ausgelegt,
dass sie auf GitHub-Hosted-Runnern laeuft, ohne dass spezialisierte
Hardware (ESP32-S3) im CI verfuegbar sein muss. Unit-Tests laufen
daher auf dem Host-PC mit GCC und der Unity-Test-Bibliothek, die
echte Hardware-Integration wird in einem separaten, manuell
getriggerten Workflow-Step angeboten, der einen Self-Hosted-Runner
mit angeschlossener Hardware voraussetzt.

Die Release-Strategie verwendet Semantic Versioning (SemVer) mit
automatischer Changelog-Generierung. Tag-Commits im Format vMAJOR.MINOR.PATCH
loesen einen Release-Workflow aus, der Firmware-Binaries und
App-Builds als Release-Artifacts hochlaedt und ein Tag-basiertes
Release auf der Repository-Seite erstellt.

---

## 1. Pipeline-Architektur

### 1.1 Stufen-Modell

Die Pipeline ist in fuenf Stufen gegliedert, die sequentiell
durchlaufen werden. Jede Stufe kann bei Fehlschlag den weiteren
Fortschritt blockieren (Fail-Fast). Die Stufen sind:

```
    Push / Pull Request
           |
           v
  +------------------+
  | 1. Lint          |  clang-format, cppcheck, tsc, eslint, markdown-lint
  +------------------+
           |
           v
  +------------------+
  | 2. Build         |  ESP-IDF CMake, Metro Bundler
  +------------------+
           |
           v
  +------------------+
  | 3. Unit Test     |  Unity (C), Jest (TS)
  +------------------+
           |
           v
  +------------------+
  | 4. Integration   |  (nur PR) Firmware-Integration, App-Integration
  +------------------+
           |
           v
  +------------------+
  | 5. Deploy        |  (nur Tag) Release-Artifacts, Changelog
  +------------------+
```

| Stufe | Firmware-CI | Companion-App-CI | Docs-CI |
|---|---|---|---|
| Lint | clang-format, cppcheck, header-guard-check | tsc --noEmit, eslint | markdown-lint, brand-check, link-check, word-count |
| Build | ESP-IDF CMake (alle Komponenten) | Metro Bundler + APK-Build | entfaellt |
| Unit Test | Unity: NMEA, Event-Bus, DTC | Jest: Komponenten, BLE-Mock | entfaellt |
| Integration | Syntax-Check, Header-Include-Guards | entfaellt | entfaellt |
| Deploy | firmware-binary Artifact | app-build Artifact | entfaellt |

Die Firmware besteht aus neun Komponenten (ble, diagnostics, display,
gps, ota, power, sensors, storage, utils) und dem main-Modul. Die
Top-Level `CMakeLists.txt` definiert `EXTRA_COMPONENT_DIRS` als
`components`. Das `main/CMakeLists.txt` registriert `main.c` und
fordert alle Komponenten als REQUIRES (gps, display, sensors,
storage/logger, ble, power, ota). Die Komponenten registrieren
jeweils ihre Quelldateien und Abhaengigkeiten ueber
`idf_component_register()`.

### 1.2 Trigger-Strategie

Die Pipelines werden durch verschiedene Ereignisse ausgeloest:

| Ereignis | Firmware-CI | App-CI | Docs-CI | Release |
|---|---|---|---|---|
| Push auf main | wenn firmware/** geaendert | wenn companion-app/** geaendert | wenn docs/** geaendert | nein |
| Pull Request | wenn firmware/** im Diff | wenn companion-app/** im Diff | wenn docs/** im Diff | nein |
| Tag v*.*.* | immer | immer | nein | ja |

Die Pfad-Filter stellen sicher, dass nicht jede Aenderung alle
drei Pipelines startet. Eine Aenderung an einem Dokument loest
beispielsweise nicht den Firmware-Build aus, und umgekehrt.
Dies spart Runner-Minuten und beschleunigt das Feedback.

### 1.3 Build-Matrix

Die Firmware-CI testet gegen eine Matrix von ESP-IDF-Versionen
und Toolchain-Varianten, um Kompatibilitaet sicherzustellen:

| Dimension | Werte |
|---|---|
| ESP-IDF | v5.2, v5.3, v5.4 |
| Target | esp32s3 |
| Compiler | gcc (ESP-IDF integriert) |

Die Companion-App-CI testet gegen:

| Dimension | Werte |
|---|---|
| Node.js | 18.x, 20.x |
| Paket-Manager | npm |

### 1.4 Artifact-Management

Alle Build-Artifacts werden mit actions/upload-artifact@v4
hochgeladen und fuer 30 Tage aufbewahrt (90 Tage fuer Release-Artifacts).

| Artifact | Inhalt | Aufbewahrung |
|---|---|---|
| firmware-binary | build/transalp_bordcomputer.bin, build/bootloader.bin, build/partition_table.bin, build/ota_data_initial.bin | 30 Tage |
| firmware-size-report | build/size.json (Flash/RAM-Analyse) | 30 Tage |
| unit-test-results | test_results.xml (JUnit-Format) | 30 Tage |
| cppcheck-report | cppcheck.xml (Checkstyle-Format) | 30 Tage |
| app-build | android/app/build/outputs/apk/release/app-release.apk | 30 Tage |
| coverage-report | coverage/ (HTML + lcov) | 30 Tage |
| release-firmware | firmware.bin + bootloader + partitions (Tag-Release) | 90 Tage |
| release-app | app-release.apk (Tag-Release) | 90 Tage |

---

## 2. Firmware-CI Pipeline

### 2.1 Workflow-Datei

Die Datei `.github/workflows/firmware-ci.yml` definiert drei Jobs:
`esp-idf-build`, `unit-tests` und `static-analysis`. Alle drei Jobs
laufen parallel, da sie voneinander unabhaengig sind. Nur der
`esp-idf-build`-Job erzeugt das Firmware-Artifact, das spaeter fuer
Release-Deployments verwendet wird.

Trigger:
- Push auf `firmware/**` und `main`
- Pull Requests, die `firmware/**` oder `.github/workflows/firmware-ci.yml`
  betreffen

### 2.2 Job: esp-idf-build

Dieser Job kompiliert die komplette Firmware mit ESP-IDF. Die
Build-Matrix testet gegen ESP-IDF v5.2, v5.3 und v5.4. Der Build
verwendet die offizielle `espressif/esp-idf-ci-action`, die das
komplette ESP-IDF-Environment inklusive Toolchain, CMake und Ninja
einrichtet.

Schritte:
1. Checkout (mit Submodules, falls vorhanden)
2. ESP-IDF Setup ueber `espressif/esp-idf-ci-action@v1`
3. Konfiguration der Target auf `esp32s3`
4. `idf.py build` kompiliert alle Komponenten (ble, diagnostics, display,
   gps, ota, power, sensors, storage, utils) und das main-Modul
5. Syntax-Check mit `gcc -fsyntax-only` fuer alle .c-Dateien (als
   zusaetzliche Sicherheitsmassnahme, auch wenn idf.py bereits
   kompiliert)
6. Artifact-Upload: firmware-binary (alle relevanten .bin-Dateien)
7. Size-Report: Flash- und RAM-Verbrauch wird als JSON-Artifact
   gespeichert

Die Top-Level `CMakeLists.txt` definiert `EXTRA_COMPONENT_DIRS` als
`components`, was bedeutet, dass alle neun Komponenten automatisch
gebaut werden. Die `main/CMakeLists.txt` registriert `main.c`
und fordert alle Komponenten als REQUIRES.

### 2.3 Job: unit-tests

Dieser Job kompiliert und fuehrt die Unit-Tests auf dem Host-PC
mit GCC aus. Die Tests verwenden das Unity-Test-Framework und
eine Mock-Schicht, die die ESP-IDF-Hardware-Abstraktion nachbildet.

Test-Module:
- `test_nmea_parser.c`: Testet den NMEA-Parser mit bekannten Saetzen
  (GGA, RMC, VTG), prueft Koordinaten-Konvertierung, Speed-Berechnung
  (Knoten zu km/h), Checksum-Validierung und Fehlerbehandlung
- `test_event_loop.c`: Testet den Event-Bus mit Bit-Set/Clear,
  Wait-Timeout, Clear-on-Exit und der Eindeutigkeit der Event-Konstanten
- `test_dtc.c`: Testet das DTC-System mit Code-Registrierung,
  Zaehler-Inkrement, Clear-Operation und Abfrage

Die Mock-Schicht ist minimal: sie bildet FreeRTOS-Primitives
(Queues, Mutexe, EventGroups) ueber POSIX-Threads nach und stellt
Ring-Buffer fuer I2C/SPI/UART-Transfers bereit. Die gleiche
Test-Logik kann spaeter als Integration-Test auf echter Hardware
laufen, indem die Mocks durch die echten Treiber ersetzt werden.

Die Test-Binaries werden mit GCC kompiliert:
```
gcc -I firmware/components/*/include -I firmware/test/mocks \
    -I /tmp/unity/src firmware/test/test_*.c firmware/test/mocks/*.c \
    /tmp/unity/src/unity.c -o test_runner -lm -lpthread
```

JUnit-XML-Output wird generiert und als Artifact hochgeladen,
sodass GitHub die Test-Ergebnisse im PR-UI anzeigt.

### 2.4 Job: static-analysis

Dieser Job fuehrt statische Code-Analyse durch, ohne die Firmware
zu kompilieren. Er laeuft unabhaengig vom ESP-IDF-Build und
benoetigt nur GCC und cppcheck.

Schritte:
1. `cppcheck` fuer alle .c- und .h-Dateien in `firmware/` mit
   `--enable=all --error-exitcode=1 --suppress=missingIncludeSystem`
2. `clang-format` Dry-Run mit `--Werror` fuer alle .c- und .h-Dateien
3. Header-Include-Guards-Verifikation: ein Skript prueft, dass jede
   .h-Datei `#ifndef _..._H_` / `#define _..._H_` / `#endif` enthaelt

Der cppcheck-Report wird im Checkstyle-XML-Format als Artifact
gespeichert und kann von Code-Review-Tools weiterverarbeitet werden.

### 2.5 Abhaengigkeiten und Gate-Logik

Ein Pull-Request, der den `main`-Branch betrifft, erfordert, dass
alle drei Jobs (esp-idf-build, unit-tests, static-analysis)
erfolgreich sind. Die Branch-Protection-Rules auf dem Repository
sind so konfiguriert, dass ein Merge blockiert wird, wenn einer
dieser Jobs fehlschlaegt.

Zusaetzliche Quality-Gates:
- Flash-Verbrauch muss unter 4 MB liegen (16 MB Flash / 4 Partitionen)
- RAM-Verbrauch muss unter 320 KB liegen
- Alle Unit-Tests muessen bestehen (100% Pass-Rate)
- cppcheck darf keine Errors melden (Warnings sind tolerierbar)

---

## 3. Companion-App CI Pipeline

### 3.1 Workflow-Datei

Die Datei `.github/workflows/companion-app-ci.yml` definiert drei
Jobs: `lint`, `build` und `test`. Die App ist als Expo/React-Native-
Projekt mit TypeScript organisiert.

Trigger:
- Push auf `companion-app/**` und `main`
- Pull Requests, die `companion-app/**` betreffen

### 3.2 Job: lint

Der Lint-Job prueft TypeScript-Typsicherheit und Code-Stil:
1. `tsc --noEmit` kompiliert das TypeScript ohne Ausgabedateien
   (reiner Typ-Check)
2. ESLint prueft den Code-Stil (wenn konfiguriert, sonst wird
   dieser Schritt uebersprungen)

Der `tsc --noEmit`-Check ist der wichtigste Gate, da er
Typfehler frueh erfasst, die erst zur Laufzeit zu Abstuerzen
fuehren wuerden. Die tsconfig.json hat `strict: true` aktiviert.

### 3.3 Job: build

Der Build-Job erzeugt ein APK fuer die Release-Konfiguration:
1. Node.js 20.x Setup
2. npm ci (clean install aus package-lock.json)
3. Metro Bundler Build (JavaScript-Bundle-Validierung)
4. APK-Build in Release-Konfiguration (falls Android-SDK verfuegbar)

Da die App Expo-basiert ist, wird der Metro-Bundler-Check als
Mindestanforderung ausgefuehrt. Der vollstaendige APK-Build
erfordert das Android-SDK und wird nur ausgefuehrt, wenn
die `ANDROID_HOME`-Umgebungsvariable gesetzt ist.

Das fertige APK wird als Artifact hochgeladen.

### 3.4 Job: test

Der Test-Job fuehrt Jest-Unit-Tests aus:
1. Node.js 20.x Setup
2. npm ci
3. Jest-Test-Runner fuer Komponenten-Tests und BLE-Mock-Tests

Die Test-Suite deckt folgende Bereiche ab:
- BLE-Verbindungs-Logik (MockBleProvider: Connect, Disconnect,
  Telemetry-Empfang, DTC-Deserialisierung)
- UI-Komponenten (SpeedDisplay, RpmBar, NavArrow)
- State-Management (Zustand-Store: Telemetry-Updates, Config-Laden)
- Binary-Parser (Deserialisierung der BLE-Binary-Payloads)

Die Jest-Konfiguration und Test-Dateien werden im
`companion-app/__tests__/` Verzeichnis erwartet. Wenn Jest nicht
installiert ist (nicht in den devDependencies), wird der Job mit
einem Hinweis uebersprungen, anstatt zu fehlschlagen.

---

## 4. Docs CI Pipeline

### 4.1 Workflow-Datei

Die Datei `.github/workflows/docs-ci.yml` definiert vier Jobs:
`markdown-lint`, `brand-check`, `link-check` und `word-count`.

Trigger:
- Push auf `docs/**` und `main`
- Pull Requests, die `docs/**` betreffen

### 4.2 Job: markdown-lint

Prueft die Markdown-Formatierung aller .md-Dateien in `docs/`.
Verwendet `markdownlint-cli2` mit einer minimalen Konfiguration,
die folgende Regeln durchsetzt:
- Keine Tabs (nur Spaces)
- maximal eine Leerzeile am Dateiende
- Ueberschriften muessen inkrementell steigen (kein Sprung von H1 zu H4)
- Code-Bloecke muessen eine Sprache angegeben haben

### 4.3 Job: brand-check

Dies ist ein projekt spezifischer Quality-Gate. Der Bordcomputer ist
ein Open-Source-Projekt und darf keine geschuetzten Markennamen von
Fremdherstellern enthalten. Ein Python-Skript durchsucht alle .md-
und .yml/.yaml-Dateien im Repository mit einem Regex nach folgenden
Markennamen und schlaegt fehl, wenn einer gefunden wird:

- Smartphone- und Computerhersteller
- Suchmaschinen-Anbieter
- Social-Media-Plattformen
- Betriebssystem-Markennamen fuer mobile Plattformen

Die Liste der ueberprueften Begriffe ist in einem zentralen
Regex-Pattern definiert und kann erweitert werden. Ausnahmen
koennen durch Kommentare im Code markiert werden.

### 4.4 Job: link-check

Validiert interne Links in der Markdown-Dokumentation. Verwendet
`lychee` (Rust-basierter Link-Checker), der:
- Interne relative Links (docs/foo.md) auf Existenz prueft
- Externe Links (https://...) auf Erreichbarkeit prueft (mit Retry)
- Anker-Links (#section) gegen die Ueberschriften im Zieldokument
  validiert

Externe Links werden mit einem Timeout von 20 Sekunden pro Link
geprueft. Zeitweilig unerreichbare externe Quellen werden als
Warning, nicht als Error markiert.

### 4.5 Job: word-count

Stellt sicher, dass Research- und Konzept-Dokumente eine Mindestlaenge
haben. Dies verhindert, dass ein Dokument-Commit nur aus einem Stub
besteht. Die Mindestwoerterzahl betraegt:
- Konzept-Dokumente (docs/*.md, ohne Unterverzeichnis): 500 Woerter
- Research-Dokumente (docs/research/*.md): 1000 Woerter
- Alle anderen: keine Mindestanforderung

Ein Python-Skript zaehlt die Woerter (durch Leerzeichen getrennte
Tokens) und schlaegt fehl, wenn ein Dokument unter dem Limit liegt.

---

## 5. Release-Strategie

### 5.1 Semantic Versioning

Alle Releases folgen Semantic Versioning (SemVer) mit dem Format
`vMAJOR.MINOR.PATCH`:

- **MAJOR**: Brechende Aenderungen (API-Break, Partition-Layout-Aenderung)
- **MINOR**: Neue Features (abwaerts-kompatibel)
- **PATCH**: Bug-Fixes (abwaerts-kompatibel)

Ein Release wird durch einen Git-Tag ausgeloest, z.B. `git tag v1.2.0`.
Der Tag-Push startet den Release-Workflow, der:

1. Firmware baut (ESP-IDF, Release-Konfiguration)
2. Companion-App baut (APK, Release-Signing)
3. Changelog aus Commit-Messages generiert (Conventional Commits)
4. Release-Artifacts hochlaedt
5. Ein Tag-Release auf der Repository-Seite erstellt

### 5.2 Changelog-Generierung

Die Changelog-Generierung basiert auf Conventional Commits. Die
Commit-Message-Konvention ist:

```
type(scope): subject

type:    feat, fix, docs, style, refactor, test, chore, ci, build, perf
scope:   firmware, app, docs, ci, ble, gps, sensors, display, storage, ...
subject: kurze Beschreibung
```

Der Changelog wird mit `git-cliff` oder einem aehnlichen Tool
automatisch aus der Git-Historie seit dem letzten Tag generiert.
Jeder Type wird zu einem Abschnitt im Changelog:
- `feat` -> Neuerungen
- `fix` -> Fehlerbehebungen
- `docs` -> Dokumentation
- Alle anderen werden unter "Wartung" zusammengefasst

### 5.3 Release-Artifacts

Ein Release enthaelt folgende Artifacts:

| Datei | Beschreibung |
|---|---|
| transalp_bordcomputer.bin | Haupt-Firmware-Binary (Flash-Offset 0x10000) |
| bootloader.bin | Bootloader (Flash-Offset 0x0) |
| partition_table.bin | Partitionstabelle (Flash-Offset 0x8000) |
| ota_data_initial.bin | OTA-Daten (Flash-Offset 0xF000) |
| companion-app.apk | Companion-App fuer mobile Geraete |
| CHANGELOG.md | Generierter Changelog fuer diese Version |
| release-notes.md | Automatisch generierte Release-Notes |

### 5.4 OTA-Update-Pipeline

Die Firmware unterstuetzt OTA-Updates ueber BLE. Die CI-Pipeline
bereitet OTA-Binaries vor, indem sie das Firmware-Binary fuer den
OTA-Upload vorverarbeitet (Signatur-Verifikation, falls konfiguriert).
Das OTA-Binary wird als separater Release-Artifact hochgeladen und
kann ueber die Companion-App an den Bordcomputer gepusht werden.

---

## 6. Test-Struktur

### 6.1 Verzeichnisstruktur

Die Test-Dateien liegen unter `firmware/test/`:

```
firmware/test/
  CMakeLists.txt          Test-Build-Konfiguration
  test_nmea_parser.c      NMEA-Parser Unit-Tests
  test_event_loop.c       Event-Loop Unit-Tests
  test_dtc.c              DTC-System Unit-Tests
```

Die Test-CMakeLists.txt ist so konfiguriert, dass sie entweder
als ESP-IDF-Test-Komponente (fuer Target-Tests) oder als
Host-Build (fuer Unit-Tests mit GCC) verwendet werden kann.

### 6.2 Unity-Test-Framework

Unity ist ein kompaktes C-Test-Framework mit folgenden Makros:

- `TEST_ASSERT_EQUAL(expected, actual)` — Integer-Gleichheit
- `TEST_ASSERT_EQUAL_FLOAT(expected, actual)` — Float-Gleichheit
- `TEST_ASSERT_TRUE(cond)` / `TEST_ASSERT_FALSE(cond)`
- `TEST_ASSERT_WITHIN(delta, expected, actual)` — Toleranzbereich
- `TEST_ASSERT_EQUAL_STRING(expected, actual)` — String-Gleichheit
- `setUp()` / `tearDown()` — Vor/Nach jedem Test

Jede Test-Datei definiert eine `setUp()` und `tearDown()` Funktion
sowie eine `main()` Funktion, die `UNITY_BEGIN()` aufruft, die
einzelnen Tests mit `RUN_TEST(test_function)` registriert und
mit `UNITY_END()` abschliesst.

### 6.3 Mock-Strategie

Da die Unit-Tests auf dem Host-PC mit GCC kompiliert werden,
muessen alle ESP-IDF-Abhaengigkeiten gemockt werden. Die Mock-Schicht
besteht aus:

- **freertos_mock**: Bildet FreeRTOS-Primitives (Queues, Mutexe,
  EventGroups, Tasks) ueber POSIX-Threads nach
- **esp_log_mock**: ESP_LOG* Makros werden zu printf/leer
- **esp_timer_mock**: esp_timer_get_time() gibt einen konfigurierbaren
  Wert zurueck
- **nvs_mock**: NVS-Lesen/Schreiben ueber eine RAM-basierte
  Key-Value-Map

Die Mocks werden in den Test-Dateien selbst definiert (inline),
sodass keine separate Mock-Bibliothek gepflegt werden muss. Dies
erleichtert die Wartung und haelt die Tests selbstaendig.

### 6.4 Coverage-Messung

Die Coverage-Messung fuer C-Code erfolgt mit gcov. Die GCC-Kompilierung
der Tests wird mit `-fprofile-arcs -ftest-coverage` erweitert, und
nach dem Test-Lauf wird `gcovr` aufgerufen, um einen HTML-Report
zu generieren. Der Report wird als Artifact hochgeladen.

Coverage-Targets (aus docs/TEST-STRATEGY.md):
- Core-Treiber (RPM, Temp, Fuel, GPS, Gear): > 80%
- Filter-Algorithmen (Kalman, Complementary, Moving-Avg): > 90%
- DTC-System (Set/Clear/Persistenz): > 85%
- Konfigurations-Parser (JSON): > 80%

Ein Pull-Request, der die Coverage unter das Ziel senkt, wird
automatisch blockiert (ueber eine Coverage-Check-Aktion).

---

## 7. CI/CD-Konfigurationsdetails

### 7.1 Runner-Spezifikationen

Alle Jobs laufen auf `ubuntu-latest` (GitHub-Hosted Runner). Die
spezifischen Anforderungen sind:

| Job | RAM | Disk | Dauer (geschaetzt) |
|---|---|---|---|
| esp-idf-build | 7 GB | 14 GB | ~5 min |
| unit-tests | 4 GB | 4 GB | ~2 min |
| static-analysis | 4 GB | 4 GB | ~2 min |
| app-lint | 4 GB | 4 GB | ~1 min |
| app-build | 7 GB | 14 GB | ~5 min |
| app-test | 4 GB | 4 GB | ~2 min |
| docs-checks | 2 GB | 2 GB | ~1 min |

Die ESP-IDF-Builds sind am ressourcenintensivsten, da das
 komplette Framework (inklusive aller Abhaengigkeiten) heruntergeladen
und kompiliert werden muss. Die Runner-Caching-Strategie speichert
das ESP-IDF-Verzeichnis und den Build-Cache zwischen, um die
Build-Zeit von ~10 min auf ~5 min zu reduzieren.

### 7.2 Caching-Strategie

Folgende Caches werden verwendet:

| Cache | Key | Wiederherstellung |
|---|---|---|
| ESP-IDF | esp-idf-${{ matrix.idf_version }} | modules pfad |
| npm | node-${{ hashFiles('companion-app/package-lock.json') }} | ~/.npm |
| pip | pip-${{ hashFiles('**/requirements.txt') }} | ~/.cache/pip |
| Build | build-${{ matrix.idf_version }}-${{ hashFiles('firmware/**/*.c') }} | build/ |

Die Cache-Keys sind versions-spezifisch, sodass ein Wechsel der
ESP-IDF-Version einen Cache-Miss erzeugt und den Cache neu aufbaut.

### 7.3 Fail-Fast und Retry-Logik

Die Pipeline verwendet `fail-fast: false` in der Build-Matrix,
sodass ein Fehler in einer ESP-IDF-Version nicht die anderen
Versionen abbricht. Dies ist wichtig, um Kompatibilitaets-
Informationen fuer alle Versionen zu sammeln.

Flaky-Test-Mitigation:
- Unit-Tests werden bis zu 2x wiederholt bei Timeout
- Externe Link-Checks haben 3 Retries mit exponentiellem Backoff
- ESP-IDF-Setup hat einen automatischen Retry bei
  Netzwerkfehlern (integriert in die Action)

### 7.4 Concurrency-Control

GitHub Actions Concurrency wird verwendet, um ueberfluessige
Laufe abzubrechen. Wenn ein neuer Push auf denselben Branch erfolgt,
waehrend ein alter Lauf noch aktiv ist, wird der alte Lauf
abgebrochen:

```yaml
concurrency:
  group: firmware-${{ github.ref }}
  cancel-in-progress: true
```

Dies spart Runner-Minuten und stellt sicher, dass immer nur der
aktuellste Commit getestet wird.

---

## 8. Sicherheitsaspekte

### 8.1 Secrets-Management

Folgende Secrets werden in der Pipeline verwendet:

| Secret | Verwendung | Job |
|---|---|---|
| RELEASE_TOKEN | Release-Upload | Release-Workflow |
| ANDROID_KEYSTORE | APK-Signing | app-build (Release) |
| ANDROID_KEY_ALIAS | APK-Signing | app-build (Release) |
| ANDROID_KEY_PASSWORD | APK-Signing | app-build (Release) |

Keine Secrets werden in Logs oder Artifact-Namen exponiert.
Die Repository-Secret-Verwaltung von GitHub stellt sicher, dass
Secrets nur in den Workflow-Schritten verfuegbar sind, die sie
explizit anfordern.

### 8.2 Permissions

Die Workflows verwenden das Principle-of-Least-Privilege:

```yaml
permissions:
  contents: read     # Standard: nur Lesezugriff
```

Nur der Release-Workflow erfordert `contents: write` fuer das
Erstellen von Tags und Releases. Alle anderen Jobs arbeiten mit
Lesezugriff.

### 8.3 Dependency-Scanning

Die Pipeline fuehrt ein automatisches Dependency-Scanning durch:
- `npm audit` fuer die Companion-App
- `pip-audit` fuer Python-Abhaengigkeiten (falls vorhanden)
- GitHub Dependabot ist aktiviert und prueft woechentlich auf
  bekannte Schwachstellen (npm, github-actions, pip)

### 8.4 Security-Scan-Workflows (seit Sprint 0.4.0)

Zusaetzlich zu den Build-Pipelines enthaelt die CI drei dedizierte
Security-Scan-Workflows:

| Workflow | Datei | Scope | Trigger | Report |
|---|---|---|---|---|
| CodeQL | `.github/workflows/codeql.yml` | C/C++ + JS/TS, security-extended | push, PR, weekly | SARIF -> Security Tab |
| Semgrep | `.github/workflows/semgrep.yml` | C + TypeScript + OWASP | push, PR | SARIF -> Security Tab |
| Trivy | `.github/workflows/trivy.yml` | Filesystem + Firmware-Binary | push, PR | SARIF -> Security Tab |

Alle drei Workflows laden SARIF-Reports in den GitHub Security Tab hoch
und machen Sicherheitsfundstellen direkt im Pull Request sichtbar.
CodeQL wird zusaetzlich woechentlich ausgefuehrt, um auch ohne
Code-Aenderungen neue Schwachstellen durch aktualisierte Query-Suiten
zu erkennen.

Siehe auch [SECURITY.md](../SECURITY.md) fuer den Security-Report-Process
und die Responsible-Disclosure-Zeitleiste.

---

## 9. KPIs und Metriken

### 9.1 Pipeline-KPIs

| KPI | Zielwert | Messintervall |
|---|---|---|
| CI-Gesamtlaufzeit (Firmware) | < 10 min | Pro Run |
| CI-Gesamtlaufzeit (App) | < 8 min | Pro Run |
| CI-Gesamtlaufzeit (Docs) | < 3 min | Pro Run |
| Unit-Test-Ausfuehrung | < 5 s | Pro Run |
| Build-Erfolgsrate | > 95% | Wochen |
| Flaky-Test-Rate | < 2% | Wochen |
| Mean-Time-to-Feedback | < 10 min | Pro PR |
| Release-Bereitstellungszeit | < 30 min | Pro Release |

### 9.2 Qualitaets-KPIs

| KPI | Zielwert | Quelle |
|---|---|---|
| Unit-Test-Coverage (Core) | > 80% | gcov |
| Unit-Test-Pass-Rate | 100% | Unity-Output |
| cppcheck Errors | 0 | cppcheck-Report |
| TypeScript-Errors | 0 | tsc --noEmit |
| Markdown-Lint-Errors | 0 | markdownlint |
| Brand-Check-Violations | 0 | brand-check |
| Broken-Links | 0 | lychee |

---

## 10. Erweiterungsplan

### 10.1 Phase 1 (aktuell)

Die aktuelle Implementierung deckt ab:
- Firmware-Build (ESP-IDF v5.x)
- Host-basierte Unit-Tests (Unity + GCC + Mocks)
- Statische Analyse (cppcheck, clang-format)
- Companion-App Lint + Build + Test
- Docs-Qualitaetspruefung (Markdown-Lint, Brand-Check, Links, Word-Count)
- Release-Pipeline (Tag-basiert)

### 10.2 Phase 2 (geplant)

- Self-Hosted-Runner mit angeschlossenem ESP32-S3 fuer
  Hardware-Integration-Tests
- QEMU-Emulation fuer Target-Tests ohne echte Hardware
- Coverage-Gate (PR wird blockiert, wenn Coverage sinkt)
- Benchmark-Tracking (Performance-Zeitreihe, Vergleich mit
  docs/BENCHMARK-SPEC.md Zielwerten)

### 10.3 Phase 3 (visionaer)

- OTA-Staging: Automatisches OTA-Update an Test-Geraete nach
  Merge in main
- Canary-Release: Schrittweise Auslieferung an eine Untermenge
  von Geraeten
- Automatisches Rollback bei erhoehter Fehlerrate

---

## 11. Quellen

- GitHub Actions Dokumentation: https://docs.github.com/en/actions
- ESP-IDF CI Action: https://github.com/espressif/esp-idf-ci-action
- Unity Test-Framework: https://github.com/ThrowTheSwitch/Unity
- cppcheck: https://cppcheck.sourceforge.io/
- clang-format: https://clang.llvm.org/docs/ClangFormat.html
- Semantic Versioning: https://semver.org/
- Conventional Commits: https://www.conventionalcommits.org/
- gcov/gcovr: https://gcovr.com/
- lychee Link-Checker: https://github.com/lycheeverse/lychee
- markdownlint: https://github.com/DavidAnson/markdownlint
- Expo Framework: https://docs.expo.dev/
- Jest Test-Framework: https://jestjs.io/

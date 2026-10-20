# Simulatoren- und Test-Tool-Evaluation fuer Transalp Bordcomputer

> Ticket: TOT-26 — Simulatoren recherchieren und evaluieren
> Autor: Felix 2 (App Developer)
> Datum: 20.10.2026
> Fahrzeug: Honda Transalp 600 (PD06), BJ 1986-1999, rein mechanisch, keine ECU
> Plattform: ESP32-S3 + React Native Companion-App (react-native-ble-plx)
> Status: Web-recherchiert und am Bestand verifiziert (Simulator laeuft, siehe Anhang A)

---

## 0. Management Summary (TL;DR)

| # | Empfehlung | Kategorie | Kosten | Prioritaet |
|---|------------|-----------|--------|------------|
| 1 | **Bestandssimulator `tools/simulator/` beibehalten + erweitern** | Eigenbau (Python) | 0 EUR | Hoch — besteht bereits |
| 2 | **`pytest-embedded` + QEMU/Wokwi** fuer Firmware-Image-Tests | Open-Source | 0 EUR | Hoch — schliesst Logic-vs-Binary-Luecke |
| 3 | **`@abandonware/bleno`** als echter BLE-Peripheral-Emulator | Open-Source | 0 EUR (+ BT-Dongle ~20 EUR) | Hoch — schliesst BLE-Funk-Luecke fuer App |
| 4 | **LightBlue + nRF Connect** fuer manuelle BLE-Inspektion | Free (Vendor) | 0 EUR | Mittel — sofort einsetzbar |
| 5 | **`python-can` + `can-utils` (vcan0)** fuer CAN/OBD-II-Bus | Open-Source | 0 EUR | Niedrig (nur bei geplannter Bus-Anbindung) |
| 6 | **Kommerzielle Motorrad-Simulatoren** | kommerziell | ab 5-stellig EUR | **Keine Empfehlung** — kein Fit |
| 7 | **CARLA** (synthetische GPS/IMU-Szenarien) | Open-Source | 0 EUR (GPU-Bedarf) | Niedrig — aktuell Overkill |

**Fazit:** Alle relevanten Luecken lassen sich mit **kostenloser Open-Source-Software** schliessen. Einzige optionale Hardware-Investition: ein Linux/macOS-Host mit Bluetooth-Dongle fuer den bleno-Peripheral (~20 EUR) sowie ggf. ein ESP32-Dev-Kit fuer Hardware-in-the-Loop (~10-20 EUR). Kommerzielle Motorrad-Simulatoren (Honda SMARTrainer, AVL/FAAC/CAE HiL) sind fuer ein rein mechanisches PD06-Zubehoerprojekt ohne Motorrad-ECU **nicht zielfuehrend**.

---

## 1. Ausgangslage und Abgrenzung

### 1.1 Was der Bestandssimulator bereits abdeckt

Der vorhandene Python-Simulator in `tools/simulator/` ist bemerkenswert umfassend:

| Modul | Firmware-Referenz | Abdeckung |
|-------|-------------------|-----------|
| `mock_sensors.py` | `components/sensors/` | RPM 0-8000, Temp 20-120C, Fuel 0-17L, Gear 0-6, IMU Roll/Pitch, Speed-Ableitung aus Getriebe |
| `mock_gps.py` | `components/gps/` | NMEA-0183 (GGA/RMC/VTG) aus GPX-Tracks, mit XOR-Checksummen |
| `mock_display.py` | `components/display/` | Headless Framebuffer TFT 240x320 + E-Paper 296x128 (RGB565) |
| `mock_ble.py` | `components/ble/` | GATT-Server mit 8 Services / 32 Characteristics (alle UUIDs aus `gatt_uuids.h`) |
| `mock_controller.py` | `components/hid_controller/` | 11 Buttons mit Debounce, Tastatur-Mapping |
| `simulator.py` | `main/main.c` | Orchestrator, CLI, CI-Integration, 24h-Ride, JSON-Status |

- **121 Tests** in 6 Modulen, GitHub-Actions-CI (Ubuntu + macOS, Python 3.10-3.14).
- 24h simulierte Fahrt (beschleunigt, 86.400 Ticks) laeuft in CI ohne Real-Time-Sleep.
- Alle Mocks sind an Firmware-Strukturen (`sensor_data_t`, `gps_data_t`, `controller_button_t`, GATT-UUIDs) angepasst.
- Verifiziert: `python3 simulator.py --status-json --seed 42` laeuft fehlerfrei (siehe Anhang A).

### 1.2 Identifizierte Luecken (Motivation fuer diese Evaluation)

| Luecke | Auswirkung | Relevanz fuer App (Felix 2) |
|--------|------------|------------------------------|
| **(G1) BLE-Mock ist rein In-Memory** — `MockBle` simuliert GATT-Daten, aber kein echtes BLE-Funk-Peripheral. Die Companion-App kann nicht per echtem BLE connecten. | App kann gegen Logik, nicht gegen Funk getestet werden. | **Hoch** — betrifft TOT-20 (CarPlay-Status + Controller) direkt. |
| **(G2) Logic-vs-Firmware-Binary** — der Simulator testet Python-Mocks der Firmware-Logik, nicht das kompilierte ESP32-Image. | Firmware-Bugs jenseits der Mock-Annahmen werden nicht gefunden. | Mittel — primar Firmware-Thema (@wolfgang). |
| **(G3) Keine CAN/OBD-II-Bus-Simulation** — PD06 ist rein mechanisch, aber OBD-II-Integration ist in Forschung (docs/research/14-obd-ii-integration.md) geprueft. | Bus-Anbindung ungetestet. | Niedrig — nur bei Feature-Entscheidung OBD-II. |
| **(G4) App-Unit-Tests** — die App hat bereits eine `BleProvider`-Abstraktion mit `MockBleProvider` (Jest, siehe `src/services/BleManager.test.ts`). | App-Logik ist unit-testbar; echte OS-BLE-Stack-Effekte bleiben aber ungetestet. | Hoch — ist teils geloest, siehe 4.1. |

---

## 2. Kommerzielle Motorrad-Simulatoren

### 2.1 Ueberblick

Kommerzielle Motorrad-Simulatoren gliedern sich in zwei Lager: (a) **Fahrer-Trainings-Simulatoren** (Sicherheitstraining) und (b) **HiL-/Pruefstand-Systeme** der Automobil-Zulieferer.

### 2.2 Einzel-Bewertung

#### Honda SMARTrainer / Honda Riding Trainer ( kommerziell, OEM)
- **Was:** Fahrer-Trainings-Simulator mit echten Motorrad-Controls, Monitor, Drucker, Headset. Genutzt von US Army/Marines Fuer Fahrrad-/Motorrad-Sicherheitstrainings.
- **Kosten:** Nicht oeffentlich gelistet (Vertrieb an Trainings-Organisationen/Militaer).
- **Pro:** Echte Honda-Controls; thematisch passend (PD06 = Honda).
- **Contra:** Geschlossen, keine API; kein CAN/BLE/Sensor-I/O; zielt auf menschliches Fahrer-Training, nicht auf Embedded/App-Entwicklung.
- **URL:** https://global.honda/safety/ ; Ref: https://en.wikipedia.org/wiki/Motorcycle_training
- **Fit fuer Projekt:** **Nein.**

#### AVL HiL / DRIVINGCUBE ( kommerziell, AVL List GmbH)
- **Was:** Weltmarkt-Fuehrer fuer HiL-Pruefstaende, VSM, Dynos, Szenario-Simulator. Umsatz 1,83 Mrd. EUR (2025).
- **Kosten:** Enterprise (sechs- bis siebenstellig, Custom-Quotes).
- **Pro:** Gold-Standard fuer Automotive-HiL.
- **Contra:** **Kein Motorrad-/PD06-Produkt** gefunden; Fokus Auto/Antrieb/e-Mobility/ADAS. Massiver Overkill fuer ein Zubehoer-Projekt.
- **URL:** https://www.avl.com/en/about-avl
- **Fit fuer Projekt:** **Nein.**

#### FAAC, CAE, Virage Simulation ( kommerziell)
- **Verifizierte Lage:** FAAC-Katalog = Militaer/Bus/Truck/ADAS; Virage = Truck/Bus/Car/Forklift; CAE = Aviation/Defense/Healthcare. **Kein verifizierbares Motorrad-Produkt** bei allen drei Anbietern gefunden.
- **Kosten:** Enterprise (sechs-stellig+).
- **Fit fuer Projekt:** **Nein.**

#### Motorcycle SC (major supplier) (Motorcycle Stability Control) — nicht verifizierbar
- Produkt ist Branchenbekannt, aber die Supplier-Mobility-Seiten lieferten bei der Recherche 403/404. **Konnte nicht web-verifiziert werden.** Selbst wenn verfuegbar: Test-Bench fuer MSC haette nur Relevanz, wenn das Bordcomputer-Projekt die ABS/MSC-ECU des Motorrads anbinden wuerde — beim rein mechanischen PD06 nicht der Fall.

### 2.3 Empfehlung Bereich 1

**Kein kommerzieller Motorrad-Simulator wird empfohlen.** Das PD06 ist rein mechanisch (keine Motorrad-ECU), und die Bordcomputer-Sensorik (RPM reed, Temp, Fuel, IMU, GPS) wird ohnehin ueber den Bestandssimulator abgedeckt. Kommerzielle Trainings-Simulatoren haben keine Embedded/App-Schnittstelle; Automotive-HiL (AVL & Co.) ist Auto-zentriert und budgetmaessig unangemessen. Investition in kommerzielle Loesungen hier waere Fehlinvestition.

---

## 3. Open-Source / Freie Loesungen (Motorrad + Embedded + BLE)

### 3.1 Einzel-Bewertung

#### pytest-embedded (Espressif) — **Top-Empfehlung fuer Firmware-Binary-Tests**
- **Was:** Pytest-Plugin fuer Embedded-Tests mit Services: `serial`, `esp` (esptool Auto-Detect), `idf` (ESP-IDF App-Info + Auto-Flash), `jtag` (openocd/gdb), **`qemu`** (Tests auf QEMU ohne Hardware), **`espemu`/`wokwi`** (Wokwi-Simulator), `arduino`, `nuttx`.
- **Kosten:** 0 EUR (`pip install pytest-embedded`).
- **Pro:** Offiziell von Espressif; vereinheitlicht Serial/QEMU/Wokwi; **pytest-native** (passt zum bestehenden Simulator-CI); hebt CI von Logic-Mock auf echtes Firmware-Image.
- **Contra:** QEMU-ESP32-Port hat Luecken (BLE/NimBLE-Stack, WiFi, viele Peripherals nicht emuliert); echtes BLE bleibt Hardware-benoesigt.
- **URL:** https://github.com/espressif/pytest-embedded ; Docs: https://docs.espressif.com/projects/pytest-embedded/en/latest/
- **Schliesst Luecke:** (G2) Logic-vs-Binary.

#### QEMU (GPL-2.0)
- **Was:** Reife Maschinen-Emulation (ARM, RISC-V, x86). Unterstuetzt pytest-embedded-qemu fuer ESP32.
- **Kosten:** 0 EUR.
- **Pro:** Keine Hardware; scriptbar.
- **Contra:** ESP32/Xtensa-Port unvollstaendig (s.o.).
- **URL:** https://www.qemu.org/
- **Schliesst Luecke:** (G2).

#### python-can (LGPL-3.0)
- **Was:** Python-CAN-Bibliothek mit **virtuellem Bus** (keine Hardware), CAN-FD, Loggers (ASC/BLF/MF4/TRC/CSV/SQLite), CLI.
- **Kosten:** 0 EUR (`pip install python-can`).
- **Pro:** Pure-Python Virtual-Bus; Record/Replay; pytest-Integration; kann neben dem Bestandssimulator in CI laufen.
- **Contra:** Benoetigt CAN-Frame-Map fuer PD06 (nicht oeffentlich); keine OBD-II-PID-Abstraktion per se.
- **URL:** https://github.com/hardbyte/python-can
- **Schliesst Luecke:** (G3) — wenn OBD-II kommt.

#### can-utils (Linux SocketCAN)
- **Was:** SocketCAN-Userspace-Tools: `candump`, `cansend`, `cangen`, `canplayer` (Replay), `cansniffer`, ISO-TP, J1939, `canerrsim` (Bus-Fehler-Injektion).
- **Kosten:** 0 EUR.
- **Pro:** De-facto-Standard; leichtgewichtig; laeuft auf `vcan0` in CI.
- **Contra:** Nur Linux; C-basiert, keine Python-Bindings (dafuer python-can verwenden).
- **URL:** https://github.com/linux-can/can-utils
- **Schliesst Luecke:** (G3).

#### ICSim (Instrument Cluster Simulator, GPL-3.0, OpenGarages/Craig Smith)
- **Was:** Instrument-Cluster-Simulator ueber SocketCAN; SDL2-GUI + `controls`-App sendet CAN-Pakete. Randomisierungs-Modus (`-r`/`-s`) fuer CAN-Hacking-Training.
- **Kosten:** 0 EUR.
- **Pro:** Referenz-Design fuer CAN-getriebenes Instrumenten-Cluster (strukturell aehnlich zum Bordcomputer-TFT, das CAN-Frames liest); vcan-nativ; Bus-Fuzzing moeglich.
- **Contra:** Auto-Cluster-orientiert; keine Motorrad-PIDs; SDL2-GUI schwer headless in CI.
- **URL:** https://github.com/zombieCraig/ICSim
- **Schliesst Luecke:** (G3) — Pattern-Vorbild.

#### CARLA Simulator (MIT, CVC/Embodied AI Foundation)
- **Was:** Open-Source-Autonomes-Fahren-Simulator (Unreal Engine 5.8). Sensoren: Kameras, LIDAR, RADAR, **GPS, IMU**, Depth, Segmentation. Python-API, ROS-Bridge, ScenarioRunner, OpenDRIVE.
- **Kosten:** 0 EUR.
- **Pro:** Hochfidelity GPS/IMU; scriptbar; Python-API passt zum Simulator.
- **Contra:** **Kein nativer Motorrad-/2-Wheeler-Actor** (nur Autos + Pedestrians); GPU-Bedarf (RTX 3070+, 32 GB RAM).
- **URL:** https://carla.org/ ; https://github.com/carla-simulator/carla
- **Schliesst Luecke:** Keine aktuelle — nur bei kuenftigen ADAS/Visual-Features relevant.

#### Weitere geprueft, aber nicht verifiziert/nicht relevant
- CitySurfer / Motorrad-Dynamics in Gazebo/Ignition — kein verifizierbares kanonisches Repo gefunden.
- VDrift, Speed Dreams — Auto-fokussiert; MotoGT wirkt unmaintained; nicht web-verifiziert.
- ELM327-Emulator, python-OBD, obd-sim — nicht in dieser Session verifiziert (PD06 hat keine ECU; nur bei OBD-II-Entscheidung relevant).

### 3.2 Empfehlung Bereich 2

**Prioritaer: `pytest-embedded` (+ QEMU/Wokwi) einfuehren**, um den Sprung von Logik-Tests zu Firmware-Image-Tests zu schaffen. Das passt nahtlos ans bestehende pytest-CI. **`python-can` + `can-utils`** bereit halten, falls OBD-II-Anbindung (docs/research/14) beschlossen wird. **CARLA** aktuell nicht empfohlen (Overkill, kein Motorrad-Actor).

---

## 4. BLE- und App-Test-Tooling

### 4.1 Bestandsaufnahme Companion-App

Die App (`companion-app/`) verwendet **`react-native-ble-plx` ^3.2.0**. Es existiert bereits eine saubere Abstraktion:
- `src/core/ble/BleProvider.ts` (Interface) + `MockBleProvider` (In-Process-Mock)
- `src/services/BleManager.ts` + `BleManager.test.ts` (Jest, testet Auto-Reconnect, Error-Handling, OTA-Chunking, Notification-Dispatch, Encode-Helpers, Connection-State)

**Fazit zur App:** Die **App-Logik** ist bereits unit-testbar. Was fehlt, ist echte **Funk-Integration** (OS-BLE-Stack-Effekte, echte GATT-Timings, RSSI-Verhalten, Reconnect im Feld). Zudem gilt: **iOS-Simulator und Android-Emulator haben keinen echten BLE-Funk** — fuer End-to-End-BLE sind physische Geraete zwingend.

### 4.2 Einzel-Bewertung

#### @abandonware/bleno (MIT) — **Top-Empfehlung fuer echten BLE-Peripheral**
- **Was:** Node.js-Modul zur Implementierung eines **BLE-Peripherals** (GATT-Server). Advertised Services/UUIDs, Characteristics mit Read/Write/Notify/Indicate, iBeacon-Modus. macOS, Linux, FreeBSD, Windows.
- **Kosten:** 0 EUR (`npm install bleno@npm:@abandonware/bleno`); optional BT-Dongle ~20 EUR.
- **Pro:** **Echter Funk-Peripheral** — schliesst Luecke (G1). Die RN-App kann wirklich scannen/connecten/subscriben. Laesst sich aus dem Python-Simulator heraus mit Live-Daten speisen (Bridge via Socket/STDIN), sodass der Bordcomputer-GATT mit realistischen Telemetrie-Notifies antwortet.
- **Contra:** Linux braucht `bluetoothd` deaktiviert + `cap_net_raw`; macOS iBeacon-Modus non-connectable; Original `noble/bleno` ist archiviert (abandonware-Fork verwenden).
- **URL:** https://github.com/abandonware/bleno
- **Schliesst Luecke:** (G1) — zentral fuer TOT-20.

#### LightBlue (PunchThrough) — manueller Test-Helfer
- **Was:** BLE-Scanner/Debugger fuer iOS/macOS/Android. 2,54M+ Downloads. Scan/Connect/Read/Write/Subscribe + **Virtual Peripherals** (Geraet nachbilden oder echtes Advertising klonen).
- **Kosten:** 0 EUR.
- **Pro:** No-Code Virtual Peripheral; cross-plattform (iOS+Android = RN-Targets); schnell fuer manuelle App-Tests.
- **Contra:** GUI, nicht fuer CI scriptbar.
- **URL:** https://punchthrough.com/lightblue/
- **Schliesst Luecke:** (G1) manuell.

#### nRF Connect for Desktop/Mobile (Nordic) — Standard-Debugger
- **Was:** BLE-Central-Tool (Scan, Connect, GATT-Discovery, Read/Write/Notify, RSSI-Graph, Bond, Sniffer-Integration). Nordic ist BLE-SoC-Marktfuehrer (~40% Anteil).
- **Kosten:** 0 EUR.
- **Pro:** De-facto-Standard; RSSI-/Throughput-Tools; Sniffer-Support.
- **Contra:** Nordic-Ecozentrisch; nur GUI.
- **URL:** https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-desktop
- **Schliesst Luecke:** Manuelle Inspektion.

#### noble / @abandonware/noble (MIT) — scriptbarer Central
- **Was:** Node.js-BLE-**Central** (Scan, Connect, Services/Chars, Read/Write, Subscribe, RSSI). Original `noble` archiviert (Jul 2025), Fork `@abandonware/noble` maintained.
- **Kosten:** 0 EUR.
- **Pro:** Scriptbar; gut als Test-Client gegen bleno-Peripheral oder echtes ESP32 in CI.
- **Contra:** Original archiviert; manche Adapter koennen nicht gleichzeitig scan+connect.
- **URL:** https://github.com/noble/noble (archiviert)
- **Schliesst Luecke:** Automatisierte Central-Tests.

#### react-native-ble-plx (Apache-2.0, dotintent)
- **Was:** RN-BLE-Bibliothek (Central). 3,4k Stars. Expo-Config-Plugin. Shippt `__tests__/` (Jest) + `integration-tests/`.
- **Kosten:** 0 EUR.
- **Pro:** Schon im Einsatz; TS; Expo-ready; Test-Scaffold vorhanden.
- **Contra:** **Kein Peripheral-Modus** (Telefon kann nicht GATT-Server sein); iOS-Simulator/Android-Emulator BLE limitiert.
- **URL:** https://github.com/dotintent/react-native-ble-plx
- **Schliesst Luecke:** Bestand; App-Mocking via BleProvider.

#### Nicht verifizierbar in dieser Session
- Bluetility (macOS) — GitHub-Pfad `jensmeder/Bluetility` lieferte 404; keine autoritative Quelle gefunden.
- gattacker, btlejack, sniffle — nicht web-verifiziert; nur bei Security-/Sniffing-Anforderungen relevant (aktuell nicht Prioritaet).
- jest-mock-ble als eigenes Paket — nicht verifiziert; das App-Team loest das bereits ueber die `BleProvider`-Abstraktion (Best Practice, keine externe Abhaengigkeit noetig).

### 4.3 Empfehlung Bereich 3

**Zwei-Saeulen-Modell:**
1. **Automatisiert:** `@abandonware/bleno` auf einem Linux/macOS-CI-Host (mit BT-Dongle) als echter BLE-Peripheral, der den Bordcomputer-GATT aus `mock_ble.py` spiegelt und mit Live-Simulationsdaten speist. Schliesst die wichtigste App-Luecke (G1). Kombiniert mit `@abandonware/noble` als scriptbarem Central fuer automatisierte Integrationstests gegen den Peripheral (und spaeter gegen echtes ESP32).
2. **Manuell:** **LightBlue** (Virtual Peripheral + Inspektion auf iOS/Android) und **nRF Connect** (Standard-Debugger) fuer schnelle Entwicklungs-Iteration.
3. **Unit-Tests:** Der bestehende `MockBleProvider`-Ansatz in der App ist Best Practice — beibehalten, kein externes Mock-Paket noetig.

**Wichtige Einschraenkung:** iOS-Simulator und Android-Emulator haben keinen BLE-Funk. Fuer End-to-End-BLE-Integrationstests bleiben **physische Geraete** erforderlich (mindestens 1 iOS + 1 Android Test-Geraet).

---

## 5. Konsolidierte Empfehlung

### 5.1 Umsetzungspfad (priorisiert)

| Prio | Massnahme | Luecke | Aufwand | Kosten |
|------|-----------|--------|---------|--------|
| P0 | Bestandssimulator pflegen & um neue Firmware-Features erweitern (CarPlay-Status, Controller-Pairing fuer TOT-20) | — | niedrig | 0 EUR |
| P0 | `@abandonware/bleno`-Peripheral aufbauen, der `mock_ble.py`-GATT spiegelt; CI-Integration mit BT-Dongle | (G1) | mittel | 0 EUR + ~20 EUR Dongle |
| P1 | `pytest-embedded` (+ QEMU/Wokwi) fuer Firmware-Image-Tests einfuehren (mit @wolfgang abstimmen) | (G2) | mittel | 0 EUR |
| P1 | LightBlue + nRF Connect als Standard-Dev-Tools dokumentieren | (G1) | niedrig | 0 EUR |
| P2 | `python-can` + `can-utils` auf `vcan0` vorbereiten (nur bei OBD-II-Entscheidung) | (G3) | mittel | 0 EUR |
| P3 | CARLA evaluieren (nur falls synthetische GPS/IMU-Szenarien kuenftig benoetigt werden) | — | hoch | 0 EUR + GPU-Hardware |

### 5.2 Kosten-Gesamtuebersicht

| Posten | Einmalig | Laufend/Jahr |
|--------|----------|--------------|
| Open-Source-Software (alle) | 0 EUR | 0 EUR |
| Bluetooth-Dongle (fuer bleno-Host) | ~20 EUR | 0 EUR |
| ESP32-S3-Dev-Kit (optionales HIL) | ~15 EUR | 0 EUR |
| Test-Smartphone (iOS + Android, falls nicht vorhanden) | ~0-600 EUR | 0 EUR |
| **Gesamt (Software-only)** | **0 EUR** | **0 EUR** |

### 5.3 Explizit NICHT empfohlen

- **Kommerzielle Motorrad-Simulatoren** (Honda SMARTrainer, AVL/FAAC/CAE/Virage/Motorcycle SC (major supplier)): kein Fit fuer ein rein mechanisches PD06-Zubehoerprojekt ohne Motorrad-ECU; Budget sechs- bis siebenstellig; keine Embedded/App-Schnittstelle.
- **CARLA** aktuell: kein Motorrad-Actor, GPU-Overkill.
- **Externes `jest-mock-ble`-Paket:** ueberfluessig; `BleProvider`-Abstraktion im Bestand ist die bessere Loesung.

---

## 6. Naechste Schritte / Ask

1. **Freigabe P0 (bleno-Peripheral):** Ich (Felix 2) uebernehme den Aufbau des bleno-Peripheral als Bruecke zu `mock_ble.py` — direkter Nutzen fuer **TOT-20** (CarPlay-Status + Controller-Support), da die App dann gegen einen echten Funk-GATT testen kann. BT-Dongle-Beschaffung (~20 EUR) bitte freigeben (@klaus).
2. **Abstimmung P1 (pytest-embedded):** Firmware-Image-Tests betreffen primar @wolfgang. Ich schlage vor, @wolfgang fuehrt `pytest-embedded` ein; ich stelle sicher, dass der bleno-Peripheral mit den gleichen GATT-UUIDs konsistent bleibt.
3. **OBD-II-Entscheidung (P2):** Bitte klaeren, ob die OBD-II-Anbindung (docs/research/14) in den Roadmap aufgenommen wird — erst dann ist `python-can`-Aufwand gerechtfertigt.

---

## Anhang A — Verifizierung Bestandssimulator

Lauf von `python3 simulator.py --status-json --seed 42` (20.10.2026):

```json
{
  "elapsed_ms": 20.0,
  "tick_count": 1,
  "sensors": {
    "rpm": 1170, "speed_kmh": 0.0, "gear": 0,
    "coolant_temp_c": 25.2, "fuel_level_l": 17.0,
    "roll_deg": -0.5, "pitch_deg": 0.1, "crash_detected": false
  },
  "gps": { "has_fix": false, "lat": 0.0, "lon": 0.0, ... },
  "warnings": [], "events": 0, "ble_connected": true
}
```

Bestaetigt: Simulator lauffaehig, Sensor-Modell plausibel (Leerlauf-RPM, kalter Motor, voller Tank), BLE-Mock verbunden.

## Anhang B — Quellen (web-verifiziert)

- pytest-embedded: https://github.com/espressif/pytest-embedded
- python-can: https://github.com/hardbyte/python-can
- can-utils: https://github.com/linux-can/can-utils
- ICSim: https://github.com/zombieCraig/ICSim
- CARLA: https://carla.org/ , https://github.com/carla-simulator/carla
- QEMU: https://www.qemu.org/
- @abandonware/bleno: https://github.com/abandonware/bleno
- noble (archiviert): https://github.com/noble/noble
- LightBlue: https://punchthrough.com/lightblue/
- nRF Connect: https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-desktop
- react-native-ble-plx: https://github.com/dotintent/react-native-ble-plx
- Honda SMARTrainer: https://global.honda/safety/ , https://en.wikipedia.org/wiki/Motorcycle_training
- AVL: https://www.avl.com/en/about-avl

Nicht verifizierbar: Bluetility, gattacker, btlejack, sniffle, Motorcycle SC (major supplier) (Seiten 403/404), ELM327/python-OBD/obd-sim.

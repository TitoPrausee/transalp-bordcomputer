# Release-Notes v1.0 & Changelog-Konzept

> Transalp Bordcomputer — Erstes offizielles Release
> Version: 1.0.0
> Datum: 13.10.2026
> Plattform: ESP32-S3-WROOM-1 (N8R2), ESP-IDF v5.4, FreeRTOS SMP
> Fahrzeug: Honda XL600V Transalp 600 (PD06), Baujahr 1986-2000
> Repository: github.com/TitoPrausee/transalp-bordcomputer

---

## Inhaltsverzeichnis

1. [Release-Notes v1.0](#1-release-notes-v10)
2. [Changelog-Konzept](#2-changelog-konzept)
3. [Release-Prozess](#3-release-prozess)
4. [Roadmap-Ausblick](#4-roadmap-ausblick)

---

## 1. Release-Notes v1.0

### 1.1 Highlights

Das v1.0-Release markiert den Abschluss der Kernentwicklung des Transalp
Bordcomputers. Nach sieben Forschungsphasen, sechs Analysephasen und einer
vollstaendigen Implementierungsphase steht erstmals ein funktionsfaehiges
Gesamtsystem zur Verfuegung, das alle Kern-Features abdeckt.

**Was macht dieses Release besonders?**

- Komplettes Open-Source-Bordcomputer-System fuer ein Motorrad ohne
  elektronische Motorsteuerung. Die PD06 ist ein rein mechanisches Fahrzeug
  aus den Baujahren 1986 bis 2000 — sie hat keine ECU, keinen CAN-Bus und
  keine standardisierte Diagnoseschnittstelle. Der Bordcomputer erfasst alle
  Fahrzeugdaten ueber eigenstaendige Sensoren, die nachgeruestet werden.

- Dual-Display-Architektur mit einem 3,5 Zoll TFT-Hauptdisplay (480x320
  Pixel, IPS-Panel, 1000 nits Backlight) und einem optionalen 2,13 Zoll
  E-Paper-Sekundaerdisplay (250x122 Pixel, monochrom). Das TFT-Display
  zeigt die Echtzeit-Telemetrie mit 30 FPS, das E-Paper-Display zeigt
  statische Werte bei direktem Sonnenlicht.

- Vollstaendige BLE-Integration mit einem GATT-Server ueber 8 Custom-Services
  und 33 Characteristics. Die Companion-App verbindet sich per BLE und
  bietet Navigation, Mediensteuerung, Diagnose und Firmware-Updates.

- Integriertes Diagnose-System mit Diagnostic Trouble Codes (DTC),
  automatischem Self-Test beim Boot und Remote-Diagnose ueber die
  Companion-App. Das System ersetzt einen separaten OBD2-Adapter und
  bietet zusaetzlich Echtzeit-Sensordaten, die kommerzielle Diagnosegeraete
  nicht liefern.

- OTA-Update-Faehigkeit mit drei Transportwegen (BLE, HTTP, SD-Karte),
  Dual-Partition-Layout mit automatischem Rollback und SHA-256-Verifikation.

- Daten-Logging auf SD-Karte mit CSV-Telemetrie-Logging (5 Hz) und
  GPX-Track-Export (1 Hz GPS-Punkte). Eine typische 4-Stunden-Tour
  erzeugt 50-100 MB an Log-Daten.

- Power-Management mit Deep-Sleep, Backup-Akku (18650, 3400 mAh) und
  automatischer Zustandsmaschine (ACTIVE, IDLE, SLEEP, SHUTDOWN).

- Ueber 9.100 Zeilen C/H-Firmware-Code ueber 9 Komponenten, ergaenzt durch
  eine React-Native-Companion-App mit knapp 6.000 Zeilen TypeScript und
  35 UI-Mockup-Dateien.

**Unterstuetztes Fahrzeug:** Honda XL600V Transalp 600 (PD06), Baujahr
1986-2000. Das System ist auf die Sensorkonfiguration der PD06 abgestimmt:
Drehzahl ueber Zuendimpuls (Optokoppler, PCNT), Geschwindigkeit ueber
Reed-Kontakt am Vorderrad, Kuehlmitteltemperatur ueber NTC/DS18B20,
Kraftstoffstand ueber ADC-Kennlinie, Oeldruck ueber analogen Sensor.

### 1.2 Features

Die folgende Tabelle listet alle implementierten Features mit ihrem
jeweiligen Reifegrad auf. Die Status-Klassifizierung orientiert sich an
der Praxis: Stable bedeutet produktionsbereit und im Test verifiziert,
Beta bedeutet funktionsfaehig aber mit bekannten Einschraenkungen,
Experimental bedeutet prototypisch implementiert und noch nicht fuer den
Dauereinsatz empfohlen.

#### 1.2.1 Echtzeit-Telemetrie

| Feature | Beschreibung | Status |
|---|---|---|
| Drehzahl (RPM) | Drehzahlmessung ueber Pulse-Counter (PCNT0), Optokoppler an Zuendspule, 50 Hz Sampling, Median-Filter | Stable |
| Geschwindigkeit | Geschwindigkeit ueber Reed-Kontakt am Vorderrad (PCNT1), Radumfang konfigurierbar | Stable |
| Kuehlmitteltemperatur | Kuehlmitteltemperatur ueber NTC 10k B3950 + ADC, Steinhart-Hart-Gleichung, 16x Oversampling | Stable |
| Kraftstoffstand | Kraftstoffstand ueber ADC + 6-Punkt-Kennlinie, EMA-Filter | Beta |
| Gang-Anzeige | Gang-Erkennung aus Speed/RPM-Ratio, 5-Gang-Getriebe der PD06 | Beta |
| Bordspannung | Bordspannung ueber Spannungsteiler (100k/33k), Generator-Status (>13,8V) | Stable |
| Oeldruck | Oeldruck ueber analogen Sensor (0-5 bar, M10x1), linearer ADC | Beta |
| Umgebungstemperatur | Umgebungstemperatur ueber Barometer-Sensor (BMP280/BMP390) | Stable |

Die Echtzeit-Telemetrie wird mit 50 Hz auf Core 1 abgetastet (sensor_task),
gefiltert (Moving-Average, EMA, Complementary-Filter, Steinhart-Hart) und
ueber FreeRTOS-Queues an Display (Core 0) und Logger weitergegeben. Die
BLE-Notify-Rate fuer Live-Telemetrie betraegt 4 Hz (250 ms Intervall).

#### 1.2.2 GPS-Tracking und GPX-Export

| Feature | Beschreibung | Status |
|---|---|---|
| NMEA-Parser | Parser fuer GGA-, RMC- und VTG-Saetze ueber UART1, 10 Hz Update-Rate | Stable |
| GPS-Position | Breiten-/Laengengrad, Hoehe, Geschwindigkeit, Kurs, Satellitenanzahl | Stable |
| GPS-Fix-Erkennung | Event-Bus-Bit EVT_GPS_FIX bei erstem Fix, EVT_GPS_LOST_FIX bei Verlust | Stable |
| GPX-Track-Export | GPS-Track als GPX-Datei auf SD-Karte (/sdcard/tracks/), 1 Hz Punkt-Rate | Stable |
| 1PPS-Zeitsynchronisation | 1PPS-Signal an GPIO39 fuer Zeitsynchronisation (IRQ-faehig) | Experimental |

Das GPS-Modul wird ueber UART1 (GPIO43 TX, GPIO44 RX) mit 38400 Baud
angeschlossen. Die Default-Baudrate ist auf 115200 per UBX-Konfiguration
erhoehbar. Der Parser unterstuetzt NMEA 0183 Standard-Saetze und ist
kompatibel mit Modulen verschiedener Hersteller (AT6558, NEO-M9N und
aehnliche). Bei einem 4-Konstellation-Empfaenger (GPS+GLONASS+Galileo+
BeiDou) wird eine Genauigkeit von ca. 2,0 m CEP erreicht.

#### 1.2.3 Dual-Display (TFT + E-Paper)

| Feature | Beschreibung | Status |
|---|---|---|
| TFT-Treiber (ILI9488) | 3,5 Zoll IPS, 480x320 Pixel, RGB565, SPI2 bei 40-80 MHz, DMA | Stable |
| E-Paper-Treiber | 2,13 Zoll, 250x122 Pixel, monochrom, Partial-Refresh, SPI2 | Beta |
| Framebuffer im PSRAM | Double-Buffering (Front + Back), je ~300 KB, Bounce-Buffer im SRAM | Stable |
| Dirty-Rect-Tracking | Maximal 16 Rechtecke, ueberlappende werden vereinigt, Faktor 12 weniger Uebertragung | Stable |
| Rendering-Pipeline | Widget-Tree -> Layout -> Paint -> Compositor -> Dirty-Rect -> SPI/DMA-Blit | Stable |
| UI-Widgets | Speed, RPM-Bar, Gear, Nav-Arrow, Altitude, Tank, Temperatur | Stable |
| Animations-Engine | Tween/Spring-Physics, Timeline-Updates mit 60 Hz (anim_task) | Experimental |

Das TFT-Display ist das Hauptdisplay und zeigt die Echtzeit-Telemetrie
mit 30 FPS an. Der display_task laeuft auf Core 0 und pullt GPS- und
Sensor-Queues. Das E-Paper-Display ist optional und fuer statische Werte
bei direktem Sonnenlicht gedacht. Beide Displays teilen sich den SPI2-Bus
(FSPI) mit der MicroSD-Karte, Multiplexing erfolgt ueber separate
Chip-Select-Leitungen und ein Mutex (spi_mutex).

#### 1.2.4 BLE-Verbindung zur Companion-App

| Feature | Beschreibung | Status |
|---|---|---|
| NimBLE GATT-Server | 8 Custom-Services, 33 Characteristics, MTU 247 (BLE 5.0 Maximum) | Stable |
| LE Secure Connections | ECDH P-256 Pairing, MITM-Schutz, Bonding persistent in NVS | Stable |
| Telemetry-Service (0x5000) | Live-Telemetrie (4 Hz, 28 Byte), Snapshot, Track-Export | Stable |
| Navigation-Service (0x1000) | Turn-by-Turn, Route, Road, Status, Command | Beta |
| Media-Service (0x2000) | Now Playing, Control, Volume | Beta |
| Phone-Service (0x3000) | Incoming Call, Call Status, Command, Notifications | Beta |
| Weather-Service (0x4000) | Current, Forecast, Alert | Beta |
| Config-Service (0x6000) | Read/Write, Export/Import, Bond Management | Stable |
| Diagnose-Service (0x7000) | DTC List, Sensor Status, Self-Test, Log Export | Stable |
| OTA-Service (0x8000) | Control, Data (4 KB Chunks), Verify (SHA-256) | Stable |
| Auto-Reconnect | Exponentieller Backoff (1s, 2s, 4s, 8s, 16s, max 30s) | Stable |
| DEGRADING-Modus | Bei RSSI < -90 dBm werden Notify-Raten reduziert | Stable |

Die BLE-Kommunikation verwendet einen projekt-spezifischen 128-bit
Base-UUID (6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e). Die 16-bit-ID
identifiziert den Service oder die Characteristic. Standard-Services
(Generic Access 0x1800, Device Information 0x180A) verwenden
Bluetooth-SIG-Standard-UUIDs. Die maximale Anzahl von Bonds ist auf 4
begrenzt (Haupttelefon, Zweitgeraet, Werkstatt, Reserve).

#### 1.2.5 Diagnose-System (DTC, Self-Test)

| Feature | Beschreibung | Status |
|---|---|---|
| DTC-Tabelle | 16 Eintraege im RAM, Kategorien P/S/C/B, Report/Clear/Get-Active | Stable |
| Self-Test beim Boot | 7 Hardware-Checks (Best-Effort), OTA-Rollback-Verifikation | Stable |
| DTC-Kategorien | Powertrain (P0xxx), Sensors (S0xxx), Comms (C0xxx), Body (B0xxx) | Stable |
| DTC-Persistenz | Kritische DTCs zusaetzlich auf SD-Karte (/sdcard/logs/dtc.log) | Stable |
| Remote-Diagnose | DTC-Liste und Sensor-Status ueber BLE abrufbar | Stable |
| Boot-Loop-Erkennung | Bei >3 Resets in 60s wird DTC B0401 gesetzt und Rollback ausgeloest | Stable |

Das Diagnose-System implementiert 16 DTC-Codes in vier Kategorien:
Powertrain (RPM-Signal, Kuehlmitteltemperatur, Oeldruck, Bordspannung,
Kraftstoffstand), Sensors (IMU, Barometer, GPS, Crash), Comms (BLE
Advertising, SD-Karte) und Body (OTA, Watchdog, Boot-Loop). Beim Boot
werden 7 Hardware-Module geprueft. Bei OTA-Rollback-Schutz muss der
Self-Test erfolgreich sein, sonst wird automatisch die vorherige
Firmware-Partition gestartet.

#### 1.2.6 OTA-Update-Faehigkeit

| Feature | Beschreibung | Status |
|---|---|---|
| Dual-Partition-Layout | ota_0 / ota_1 je 3 MB, factory-Partition als Notfall-Backup | Stable |
| OTA ueber BLE | 4 KB Chunks, Write Without Response, CRC16-CCITT pro Chunk | Stable |
| OTA ueber HTTP | TLS mit Certificate-Bundle, Range-Request-Resume | Beta |
| OTA ueber SD-Karte | Firmware-Datei von FAT-formatierter SD-Karte lesen | Stable |
| SHA-256-Verifikation | Hash der geflashten Partition wird mit erwartetem Hash verglichen | Stable |
| RSA-3072-Signatur | Secure-Boot-Signaturpruefung (Platzhalter-Key in v1.0) | Experimental |
| Anti-Rollback | eFuse-Version-Counter (Stub-Implementierung in v1.0) | Experimental |
| Automatischer Rollback | Self-Test nach Boot, bei Fehlschlag Zurueckkehr zur alten Partition | Stable |

Das OTA-System ist als eigenstaendige Komponente unter
firmware/components/ota/ implementiert. Die State-Machine durchlaeuft
die Zustaende IDLE -> DOWNLOADING -> VERIFYING -> FLASHING -> REBOOTING
-> PENDING_VERIFY. Bei PENDING_VERIFY muss der Self-Test erfolgreich
sein, sonst wird der Rollback ausgeloest. Die RSA-Signaturpruefung ist
in v1.0 als Platzhalter implementiert und fuer die Produktionssicherung
(Secure Boot V2) vorgesehen.

#### 1.2.7 Daten-Logging auf SD-Karte

| Feature | Beschreibung | Status |
|---|---|---|
| SD-Karten-Mount | FAT32, Class 10, ueber SPI2 (shared mit Display), spi_mutex | Stable |
| CSV-Telemetrie-Logging | 5 Hz, Ringbuffer mit 64 Eintraegen, Batch-Write | Stable |
| GPX-Track-Export | 1 Hz GPS-Punkte, /sdcard/tracks/YYYYMMDD_HHMMSS.gpx | Stable |
| DTC-Log | Kritische DTCs in /sdcard/logs/dtc.log | Stable |
| Konfiguration in NVS | device_config_t als BLOB, JSON-Export/Import ueber BLE | Stable |
| Dateinamen-Konvention | /sdcard/logs/YYYYMMDD_HHMMSS.csv | Stable |

Das Log-Format ist CSV mit folgendem Schema:
timestamp,lat,lon,speed_kmh,rpm,temp_c,alt_m,roll_deg,pitch_deg

Bei 5 Hz Logging-Rate und einer typischen 4-Stunden-Tour entstehen
ca. 14-29 MB CSV-Daten und 36-72 MB GPX-Track-Daten (50-100 MB gesamt).
Bei einer Saison von 5.000 km (ca. 83 Fahrten) fallen ca. 4-8 GB Daten
an — eine 16 GB SD-Karte hat eine theoretische Lebensdauer von 5-6
Jahren, in der Praxis 2-3 Jahre.

#### 1.2.8 Power-Management mit Deep-Sleep

| Feature | Beschreibung | Status |
|---|---|---|
| Power-State-Machine | ACTIVE, IDLE, SLEEP, SHUTDOWN mit automatischen Uebergaengen | Stable |
| Akku-Ueberwachung | ADC1_CH4, Spannungsteiler 2:1, Generator-Status (>13,8V) | Stable |
| Deep-Sleep | ~0,4 mA Stromverbrauch, nur RTC aktiv, Wochen ueberlebend | Stable |
| Wakeup-Quellen | Zuendung (GPIO), IMU-Interrupt, BLE-Verbindungsanfrage, RTC-Timer | Stable |
| Backup-Akku | 18650 Li-Ion, 3400 mAh, TP4056-Lademodul, MT3608-Boost | Stable |
| Idle-Modus | Display aus, GPS + Logger aktiv (Track-Aufzeichnung ohne Display) | Stable |
| Shutdown | Bei Akku < 3,2V, sauberes SD-Flush, geordnetes Power-Off | Stable |

Die Power-State-Machine wechselt nach 60s ohne Display-Interaktion und
keiner BT-Verbindung von ACTIVE nach IDLE, nach 5min ohne Bewegung
(IMU < Schwellwert) von IDLE nach SLEEP. Aus dem SLEEP-Modus wird bei
IMU-Interrupt, BT-Verbindungsanfrage oder Zuendung AN wieder in den
ACTIVE-Zustand gewechselt. Bei Akku < 3,2V erfolgt sofortiger SHUTDOWN
aus jedem Zustand.

#### 1.2.9 IMU-basierte Fahrdynamik

| Feature | Beschreibung | Status |
|---|---|---|
| Roll/Pitch-Winkel | Complementary-Filter aus Gyro + Accel, 50 Hz Update | Stable |
| Beschleunigung | Gesamtbeschleunigung in g, fuer Fahrdynamik-Anzeige | Stable |
| Crash-Detection | Bei >3g wird EVT_CRASH_DETECTED gesetzt, DTC S0401 | Beta |
| MPU6050-Treiber | I2C0, 400 kHz, 14-Byte Burst-Read, INT an GPIO48 | Stable |
| BMP280-Barometer | I2C0, Umgebungstemperatur, Hoehe, Luftdruck | Stable |

Die IMU (6-Achsen, Accel + Gyro) liefert Roll- und Nickwinkel ueber
einen Complementary-Filter, der die Drift des Gyros mit dem langzeit-
stabilen Accel korrigiert. Die Crash-Detection loest bei einer
Gesamtbeschleunigung von ueber 3g aus und setzt das Event-Bus-Bit
EVT_CRASH_DETECTED, was in der Companion-App eine SOS-Positionierung
ermoeglichen kann. Der Barometer (BMP280) liefert zusaetzlich Hoehe
und Luftdruck fuer die Hoehen-Anzeige.

### 1.3 Hardware

#### 1.3.1 Benoetigte Hardware-Komponenten

Die vollstaendige Stueckliste mit 67 Bauteilen ueber 10 Kategorien ist
in docs/BOM.md dokumentiert. Die Hauptkomponenten sind:

- MCU: ESP32-S3-WROOM-1 (N8R2), 8 MB Flash, 2 MB PSRAM, BLE 5.0, 45 GPIO
- TFT-Display: 3,5 Zoll IPS, 480x320 Pixel, SPI, 1000 nits Backlight
- E-Paper-Display: 2,13 Zoll, 250x122 Pixel, monochrom (optional)
- GPS-Modul: 4-Konstellation GNSS, 25 Hz, 2,0 m CEP, UART
- IMU: 6-Achsen (Accel + Gyro), I2C, 685 uA
- Barometer: 300-1250 hPa, +-0,03 hPa rel., I2C
- Kuehlmitteltemperatur: Digital, 1-Wire, +-0,5 Grad C, wasserdicht
- Oeldrucksensor: Analog 0-5 bar, M10x1, wasserdicht
- MicroSD-Karte: 16 GB, Class 10, FAT32
- Backup-Akku: 18650 Li-Ion, 3400 mAh, 3,7V

#### 1.3.2 Minimale vs. Voll-Konfiguration

Es werden drei Ausbaustufen definiert:

**Minimal-Konfiguration (~80 EUR):** Funktionierendes Kernsystem mit
ESP32-S3-DevKitC-1, ILI9341 2,4 Zoll TFT, Budget-GPS, MPU6050- und
BMP280-Breakouts auf Perfboard in PETG-Gehaeuse. Kein E-Paper, kein
BLE-Datenexport, kein Backup-Akku, nicht wasserdicht. Geeignet fuer
Software-Entwicklung und Schreibtisch-Test.

**Standard-Konfiguration (~327 EUR):** Vollwertiges System mit Custom-PCB
(4-Layer, ENIG), 3,5 Zoll IPS-TFT, 2,13 Zoll E-Paper, 4-Konstellation-GPS,
Backup-Akku 18650, Oeldrucksensor, Relais-Steuerung, ASA-Gehaeuse mit
Conformal Coating, IP67. Wasserdicht, vibrationssicher, fuer Dauereinsatz
am Motorrad konzipiert. Deckt alle Must- und Should-Features ab.

**Premium-Konfiguration (~417 EUR):** Alles aus Standard plus TPMS-Sensoren,
LED-Status-Strip, zusaetzliches 2,9 Zoll E-Paper, Kugelgelenk-Halterung,
Reserve-Bauteile. Deckt zusaetzlich Could-Features ab.

Die detaillierte Kostenaufschluesselung inklusive Werkzeug-Kosten,
Skalierungseffekten und TCO-Analyse ueber 5 Jahre ist in
docs/TCO-ANALYSIS.md dokumentiert.

#### 1.3.3 PD06-spezifische Anpassungen

Die PD06 ist ein rein mechanisches Motorrad ohne elektronische
Motorsteuerung. Daher werden alle Sensorsignale eigenstaendig erfasst:

- Drehzahl: Optokoppler an der Zuendspule, Pulse-Counter (PCNT0) an GPIO15
- Geschwindigkeit: Reed-Kontakt am Vorderrad, Pulse-Counter (PCNT1) an GPIO16
- Kuehlmitteltemperatur: NTC oder DS18B20 im Kuehlmittelkreislauf
- Kraftstoffstand: Spannungsteiler am Tankgeber-Signal
- Oeldruck: Analoger Drucksensor an Oelpumpe (M10x1 Gewinde)
- Bordspannung: Spannungsteiler am 12V-Bordnetz

Die Kalibrierung ist fahrzeugspezifisch: Radumfang (für Geschwindigkeit),
Pulse-pro-Umdrehung (fuer RPM), Kraftstoff-Kennlinie (6 Stuetzpunkte)
und Temperaturoffset werden in der device_config_t-Struktur in NVS
gespeichert und sind ueber die Companion-App einstellbar.

### 1.4 Firmware

#### 1.4.1 ESP-IDF Version

Das Projekt basiert auf ESP-IDF v5.4 (dem offiziellen SDK fuer den ESP32-S3).
ESP-IDF liefert FreeRTOS im SMP-Modus (Symmetric Multi-Processing) fuer
den Dual-Core ESP32-S3, Treiber fuer alle Peripherie und die
Build-Infrastruktur auf Basis von CMake und Ninja.

Die Build-Konfiguration erfolgt ueber sdkconfig.defaults mit drei
Build-Varianten: Debug (-O0, Assertions an, Log-Level VERBOSE), Release
(-O2, Assertions aus, Log-Level INFO) und Production (-Os, Secure Boot
V2, Flash-Encryption, Log-Level WARNING).

#### 1.4.2 FreeRTOS Task-Struktur

Die Firmware verwendet 7 FreeRTOS-Tasks, die mit xTaskCreatePinnedToCore
an feste Cores gepinnt sind:

| Task | Core | Prio | Stack | Rate | Beschreibung |
|---|---|---|---|---|---|
| sensor_task | 1 | 5 | 4096 | 50 Hz | IMU, BMP280, ADC, PCNT abfragen, filtern, Queue |
| gps_task | 1 | 4 | 4096 | 10 Hz | NMEA ueber UART lesen, parsen, Queue |
| display_task | 0 | 2 | 8192 | 30 FPS | Layout, Paint, Swap, Blit (Dirty-Rect) |
| anim_task | 0 | 4 | 2048 | 60 Hz | Tween/Spring-Engine, Timeline-Updates |
| ble_task | 0 | 3 | 6144 | Event | Anwendungsseitige BLE-Event-Behandlung |
| logger_task | 0 | 3 | 4096 | 5 Hz | Ringbuffer-Batch, SD-Schreiben |
| power_task | 0 | 6 | 2048 | 1 Hz | Akku-Ueberwachung, Sleep-Entscheidung |

Die Kernphilosophie ist die strikte Trennung von Datenakquisition
(Core 1, echtzeitkritisch) und Praesentation/Kommunikation (Core 0,
best-effort). Die power_task hat die hoechste Prioritaet (6), damit
ein kritischer Akku-Zustand sofort zu einem geordneten Shutdown fuehrt.

#### 1.4.3 Unterstuetzte Displays

Das Display-Subsystem unterstuetzt folgende Controller:

- ILI9488 (3,5 Zoll TFT, 480x320, RGB565, SPI2 bei 40-80 MHz mit DMA)
- SSD1680/UC8151D (2,13 Zoll E-Paper, 250x122, 1-bit, Partial-Refresh)
- ST7789 (1,3 Zoll TFT, 240x240, kompilierbar ueber Kconfig)
- 2,9 Zoll E-Paper (296x128, zusaetzlich in Premium-Konfiguration)

Die Display-Auswahl erfolgt ueber Kconfig (CONFIG_BC_DISPLAY_TYPE) und
ermoeglicht Board-Varianten ohne Code-Aenderungen.

#### 1.4.4 Build-Anleitung

Die detaillierte Build-Anleitung ist in docs/FIRMWARE-GUIDE.md
dokumentiert. Die wesentlichen Schritte:

```
1. ESP-IDF v5.4 installieren:
   mkdir -p ~/esp && cd ~/esp
   git clone -b v5.4 --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf && ./install.sh esp32s3
   . ~/esp/esp-idf/export.sh

2. Firmware bauen:
   cd firmware
   idf.py menuconfig   # Konfiguration anpassen
   idf.py build         # Production-Build

3. Flashen und Monitor:
   idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

### 1.5 Companion-App

#### 1.5.1 Plattform-Support

Die Companion-App ist eine React-Native-Applikation mit einer einzigen
Codebasis fuer beide mobilen Betriebssysteme (iOS und Android). Sie
verwendet Expo als Build- und Entwicklungsumgebung. Die App ist als
Prototyp implementiert (Version 0.1.0) mit funktionsfaehigem BLE-Stack
und 5 Haupt-Screens (Dashboard, Navigation, Diagnose, Settings, Tours).

#### 1.5.2 BLE-Anforderungen

- BLE-Bibliothek: react-native-ble-plx (volle GATT-API, Connection-
  Parameter-Kontrolle)
- Mindestens BLE 4.2 am Smartphone (fuer LE Secure Connections)
- BLE 5.0 empfohlen (fuer MTU 247 und hoeheren Throughput)
- Standortberechtigung erforderlich (BLE-Scan erfordert auf manchen
  Betriebssystemen Standortzugriff)
- Die App ist GATT-Client, der Bordcomputer ist GATT-Server

#### 1.5.3 Installationshinweise

Die Companion-App befindet sich in der Prototyp-Phase und ist noch nicht
in den offiziellen App-Stores verfuegbar. Fuer die Entwicklung und fuer
Beta-Tester wird die App aus dem Quellcode gebaut:

```
1. Repository klonen
2. cd companion-app
3. npm install
4. npx expo start
5. QR-Code scannen oder Simulator starten
```

Die App enthaelt einen MockBleProvider fuer die Entwicklung ohne
physischen Bordcomputer. Dieser erzeugt Testdaten und ermoeglicht die
UI-Entwicklung unabhaengig von der Hardware.

### 1.6 Bekannte Einschraenkungen

#### 1.6.1 Was funktioniert noch nicht?

- Companion-App-Integration: Die App ist als Prototyp implementiert,
  aber die Integration mit dem Bordcomputer (Dashboard, Navigation,
  Diagnose) ist noch nicht vollstaendig. Ein separater Task
  (Companion-App Integration) laeuft.

- Secure Boot V2 ist in der Standard-Konfiguration deaktiviert. Die
  RSA-Signaturpruefung in ota_secure_boot.c verwendet einen
  Platzhalter-Key und faellt im Dev-Modus auf Hash-Only zurueck. Fuer
  den Produktionseinsatz muss Secure Boot V2 aktiviert und ein echter
  RSA-3072-Key eingebettet werden.

- Flash-Encryption ist deaktiviert. NVS-Inhalte (Konfiguration,
  Bonding-Keys) sind unverschluesselt und bei physischem Zugriff
  auslesbar.

- Admin-PIN-System: Kritische Write-Characteristics (Config-Write,
  Bond-Management, Self-Test, OTA-Control) haben keine zusaetzliche
  Authentifizierung ueber das Pairing hinaus.

- Feldtest-Protokoll: Es liegt noch kein Feldtest-Protokoll vor. Das
  System wurde auf dem Schreibtisch getestet, aber noch nicht im
  Dauereinsatz am Motorrad verifiziert.

- CI/CD-Pipeline: Die automatisierte Build- und Test-Pipeline ist in
  Entwicklung, aber noch nicht vollstaendig. Pre-Commit-Hooks und
  GitHub Actions Workflows sind definiert, aber noch nicht vollstaendig
  integriert.

#### 1.6.2 Bekannte Bugs und Workarounds

- SPI-Bus-Konflikt: Bei gleichzeitigem Zugriff auf TFT-Display und
  SD-Karte kann es zu kurzen Display-Ausfällern kommen. Workaround:
  Das spi_mutex serialisiert den Zugriff; in der Praxis ist die
  Beeintraechtigung minimal (<1 ms pro Frame).

- GPS-Kaltstart: Beim ersten Start nach Stromausfall dauert der GPS-Fix
  typischerweise 30-60 Sekunden (Kaltstart). Bei Warmstart sind es
  <5 Sekunden. Workaround: Backup-Akku haelt RTC und GPS-Backup am
  Leben, Warmstart ist die Regel.

- E-Paper Ghosting: Bei haeufigem Partial-Refresh kann Ghosting
  (nachleuchtende alte Inhalte) auftreten. Workaround: Alle 50 Updates
  einen Full-Refresh durchfuehren (in der Firmware konfigurierbar).

- BLE-Reconnect nach Deep-Sleep: Nach dem Aufwachen aus dem Deep-Sleep
  kann der erste BLE-Reconnect-Versuch fehlschlagen. Workaround:
  Auto-Reconnect mit exponentiellem Backoff ist implementiert und
  stellt die Verbindung innerhalb von 1-5 Sekunden automatisch her.

#### 1.6.3 Hardware-Limits

- TFT-Display bei direktem Sonnenlicht: Das TFT-Display mit 1000 nits
  Backlight ist bei direkter Sonneneinstrahlung schwer lesbar. Das
  E-Paper-Display kompensiert dies fuer statische Werte. Fuer
  vollstaendige Sonnenlicht-Tauglichkeit ist ein transflektives
  Display zu evaluieren.

- SD-Karten-Lebensdauer: Die SD-Karte ist das primaere
  Verschleissteil. Bei 4-8 GB Daten pro Saison betraegt die
  Lebensdauer 2-3 Jahre (bei Qualitaetskarten mit MLC-NAND).
  Empfehlung: Hochwertige Class-10-Karte mit MLC-NAND verwenden.

- BLE-Reichweite: Die effektive Reichweite betraegt 10-30 m je nach
  Umgebung. Bei schlechtem Empfang (RSSI < -90 dBm) schaltet das
  System in den DEGRADING-Modus und reduziert die Notify-Raten.

- GPS-Signal im Gebaeude: GPS funktioniert nur im Freien oder in der
  Naehe von Fenstern. In Garagen oder Tunneln ist kein GPS-Fix
  moeglich. Die letzte bekannte Position wird im Ring-Buffer
  zwischengespeichert.

### 1.7 Sicherheits-Hinweise

Das Sicherheitskonzept ist detailliert in docs/SECURITY-ANALYSIS.md
dokumentiert. Fuer das v1.0-Release gelten folgende Hinweise:

#### 1.7.1 BLE-Pairing sollte aktiviert werden

Das BLE-Pairing mit LE Secure Connections (ECDH P-256) ist in der
Firmware konfiguriert und bietet MITM-Schutz. Ohne Pairing sind keine
Telemetrie-Daten ueber BLE abrufbar (Notifications nur an verbundene
und subscribed Clients). Das Pairing sollte beim ersten Verbinden
durchgefuehrt und das Bonding persistent gespeichert werden.

Die maximale Anzahl von Bonds ist auf 4 begrenzt. Bei Verlust eines
Geraets sollten alle Bonds ueber den Bond-Management-Service (0x6005)
geloescht und die verbleibenden Geraete neu gepairt werden.

#### 1.7.2 Default-PIN aendern

Das Admin-PIN-System ist fuer v1.1 geplant (siehe Sicherheitsanalyse
Abschnitt 4). In v1.0 sind kritische Write-Characteristics nach
erfolgreichem Pairing ohne zusaetzliche Authentifizierung zugreifbar.
Bis zur Implementierung des Admin-PIN-Systems sollte das Pairing nur
mit vertrauenswuerdigen Geraeten durchgefuehrt werden.

#### 1.7.3 OTA nur aus vertrauenswuerdiger Quelle

Firmware-Updates sollten ausschliesslich aus vertrauenswuerdigen Quellen
installiert werden. In v1.0 ist die RSA-Signaturpruefung als Platzhalter
implementiert — es wird nur der SHA-256-Hash verifiziert, nicht die
kryptografische Signatur. Fuer den Produktionseinsatz muss Secure Boot V2
aktiviert werden, damit der Bootloader nur signierte Firmware-Images
ausfuehrt.

Die drei OTA-Transportwege haben unterschiedliche Sicherheitsniveaus:
- SD-Karte: Erfordert physischen Zugriff (niedrigstes Risiko)
- BLE: Verschluesselt durch Pairing (mittleres Risiko)
- HTTP: TLS mit Certificate-Bundle (mittleres Risiko, hoechste
  Reichweite)

---

## 2. Changelog-Konzept

### 2.1 Versionierungsschema: Semantic Versioning

Das Projekt verwendet Semantic Versioning (SemVer) im Format
MAJOR.MINOR.PATCH:

- MAJOR: Inkompatible API-Aenderungen (z.B. BLE-Protokoll-Versionswechsel,
  Aenderung der GATT-Service-UUIDs, Breaking-Change in der
  Konfigurationsstruktur)
- MINOR: Neue Features, abwaertskompatibel (z.B. neuer Sensor-Support,
  neue Companion-App-Screens, zusaetzliche BLE-Characteristics)
- PATCH: Bug-Fixes, abwaertskompatibel (z.B. Display-Flicker-Fix,
  BLE-Reconnect-Verbesserung, Sensor-Kalibrierung-Korrektur)

Beispiel: 1.2.3 = Major 1, Minor 2, Patch 3

Die Versionsnummer wird in der Root-CMakeLists.txt als PROJECT_VER
gesetzt. Alternativ kann die Version automatisch aus dem neuesten
Git-Tag extrahiert werden:

```cmake
execute_process(
    COMMAND git describe --tags --always
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(PROJECT_VER ${GIT_VERSION})
```

ESP-IDF setzt die Version automatisch in die esp_app_desc_t-Struktur,
die zur Laufzeit ueber esp_app_get_description() abrufbar ist.

### 2.2 Changelog-Format: Keep a Changelog

Das Projekt folgt dem "Keep a Changelog"-Format (keepachangelog.com).
Alle wesentlichen Aenderungen werden in der Datei CHANGELOG.md
dokumentiert. Das Format sieht vor, dass fuer jede Version ein Abschnitt
mit einem Datum und den folgenden Kategorien erstellt wird:

**Kategorien:**

- Added: Neue Features und Funktionalitaeten
- Changed: Aenderungen an bestehenden Features
- Deprecated: Features, die in einer zukuenftigen Version entfernt werden
- Removed: Features, die in dieser Version entfernt wurden
- Fixed: Bug-Fixes und Fehlerbehebungen
- Security: Sicherheitsrelevante Aenderungen

**Beispiel-Eintrag:**

```markdown
## [1.1.0] - 2026-11-01

### Added
- Admin-PIN-System fuer kritische BLE-Write-Characteristics
- Secure Boot V2 mit RSA-3072-Signaturverifikation
- Flash-Encryption zum Schutz von NVS und Bonding-Keys

### Changed
- OTA-Signaturpruefung: Dev-Fallback entfernt, fail-closed
- Log-Level in Production-Build auf ERROR reduziert

### Fixed
- SPI-Bus-Konflikt bei gleichzeitigem Display- und SD-Karten-Zugriff
- GPS-Kaltstart-Zeit um 40% reduziert (warm-start optimization)

### Security
- BLE-Rate-Limiting fuer Pairing-Versuche implementiert
- NVS-Encryption aktiviert
```

### 2.3 Pre-Release-Tags

Vor der finalen Veroeffentlichung einer Version werden Pre-Release-Tags
verwendet, um die Reife des Releases zu kennzeichnen:

- alpha: Fruehe Testphase, Features koennen unvollstaendig sein,
  API kann sich noch aendern. Beispiel: 1.1.0-alpha.1
- beta: Feature-complete, aber mit bekannten Bugs. Fuer Beta-Tester
  zur Untersuchung im realen Einsatz. Beispiel: 1.1.0-beta.2
- rc (Release Candidate): Finaler Kandidat, keine bekannten
  kritischen Bugs. Fuer letzte Verifikation vor dem Release.
  Beispiel: 1.1.0-rc.1

Pre-Release-Versionen werden nicht als "latest" markiert und erscheinen
mit einem Hinweis auf der GitHub-Release-Seite.

### 2.4 Release-Zyklus

Der Release-Zyklus ist feature-driven, das heisst es gibt keine festen
Release-Termine. Ein neues Release wird erstellt, wenn:

- Eine ausreichende Anzahl neuer Features oder Bug-Fixes
  akkumuliert ist (typischerweise alle 2-4 Wochen)
- Ein kritischer Security-Fix erforderlich ist (Sofort-Release)
- Ein Meilenstein der Roadmap erreicht ist (z.B. Phase-Abschluss)

Kritische Security-Fixes werden als PATCH-Release ASAP veroeffentlicht,
ohne auf weitere Features zu warten.

---

## 3. Release-Prozess

### 3.1 Release-Checkliste

Die folgende ASCII-Tabelle definiert die verbindliche Checkliste fuer
jedes Release. Jeder Schritt muss abgeschlossen sein, bevor das
Release veroeffentlicht wird.

```
+----+----------------------------------------------+----------+
| #  | Schritt                                      | Status   |
+----+----------------------------------------------+----------+
| 1  | Code-Freeze: Keine neuen Features auf         | [ ] Done |
|    | Release-Branch                                |          |
+----+----------------------------------------------+----------+
| 2  | Versionsnummer in CMakeLists.txt aktualisieren| [ ] Done |
|    | (PROJECT_VER)                                 |          |
+----+----------------------------------------------+----------+
| 3  | CHANGELOG.md mit neuem Abschnitt ergaenzen    | [ ] Done |
+----+----------------------------------------------+----------+
| 4  | Alle Unit-Tests gruen (idf.py test)           | [ ] Done |
+----+----------------------------------------------+----------+
| 5  | cppcheck ohne neue Warnings                   | [ ] Done |
+----+----------------------------------------------+----------+
| 6  | clang-tidy ohne neue Warnings                 | [ ] Done |
+----+----------------------------------------------+----------+
| 7  | clang-format auf allen Dateien ausgefuehrt    | [ ] Done |
+----+----------------------------------------------+----------+
| 8  | Release-Build erfolgreich (Production-Konfig) | [ ] Done |
+----+----------------------------------------------+----------+
| 9  | Secure-Boot-Signatur verifiziert              | [ ] Done |
|    | (falls Production)                             |          |
+----+----------------------------------------------+----------+
| 10 | OTA-Update von SD-Karte getestet              | [ ] Done |
+----+----------------------------------------------+----------+
| 11 | OTA-Update ueber BLE getestet                 | [ ] Done |
+----+----------------------------------------------+----------+
| 12 | Rollback-Test: Neue Firmware mit absichtlichem| [ ] Done |
|    | Self-Test-Fail -> automatischer Rollback      |          |
+----+----------------------------------------------+----------+
| 13 | Flash-Verschluesselung getestet               | [ ] Done |
|    | (falls Production)                             |          |
+----+----------------------------------------------+----------+
| 14 | Companion-App-Kompatibilitaet geprueft       | [ ] Done |
+----+----------------------------------------------+----------+
| 15 | RELEASE-NOTES.md aktualisiert                  | [ ] Done |
+----+----------------------------------------------+----------+
| 16 | Brand-Name-Check: grep ueber alle .md/.yml    | [ ] Done |
|    | Dateien gegen brand-blocklist.txt             |          |
+----+----------------------------------------------+----------+
| 17 | Git-Tag gesetzt:                               | [ ] Done |
|    | git tag -a v1.x.y -m "Release v1.x.y"         |          |
+----+----------------------------------------------+----------+
| 18 | GitHub-Release mit Binaries erstellt          | [ ] Done |
+----+----------------------------------------------+----------+
| 19 | Release-Branch in main gemerged               | [ ] Done |
+----+----------------------------------------------+----------+
| 20 | Push zum Remote (main + tags)                | [ ] Done |
+----+----------------------------------------------+----------+
```

### 3.2 Branch-Strategie fuer Releases

Die Branch-Strategie folgt einem Feature-Branch-Workflow mit
dedizierten Release-Branches:

- main: Stabiler Haupt-Branch. Jeder Commit auf main ist ein
  potentiell releasable Zustand. Tags werden auf main gesetzt.
- feature/*: Feature-Branches fuer neue Funktionalitaet. Werden per
  Pull Request in main gemerged.
- firmware/*: Firmware-spezifische Branches (Treiber, Komponenten).
- docs/*: Dokumentations-Branches.
- release/vX.Y.Z: Release-Branch fuer die Vorbereitung eines Releases.
  Wird von main abgespalten, wenn der Code-Freeze beginnt. Nur
  Bug-Fixes, keine neuen Features. Nach dem Release wird der
  Release-Branch in main zurueckgemerged.
- hotfix/vX.Y.Z: Hotfix-Branch fuer kritische Bug-Fixes nach dem
  Release. Wird von main abgespalten, der Fix wird auf main
  angewendet und der Branch geloescht.

### 3.3 Git-Tagging-Konvention

Git-Tags folgen dem Semantic-Versioning-Format mit einem "v"-Praefix:

```
v1.0.0          — Erstes stabiles Release
v1.0.1          — Bug-Fix-Release
v1.1.0          — Feature-Release
v1.1.0-alpha.1  — Pre-Release (alpha)
v1.1.0-beta.1   — Pre-Release (beta)
v1.1.0-rc.1     — Release Candidate
v2.0.0          — Major-Release (Breaking Changes)
```

Tags werden als annotierte Tags erstellt:

```bash
git tag -a v1.0.0 -m "Release v1.0.0 - Erstes stabiles Release

Highlights:
- Dual-Display (TFT + E-Paper)
- BLE GATT-Server mit 8 Services
- GPS-Tracking und GPX-Export
- Diagnose-System (DTC, Self-Test)
- OTA-Update (BLE, HTTP, SD-Karte)
- Daten-Logging auf SD-Karte
- Power-Management mit Deep-Sleep
- IMU-basierte Fahrdynamik"
```

Tags werden immer zum Remote gepusht:

```bash
git push origin v1.0.0
```

### 3.4 Artifact-Erstellung

Bei einem Release werden folgende Artifacts erstellt:

**Firmware-Binaries:**

- transalp_bordcomputer.bin — Firmware-Image (signiert bei Secure Boot)
- bootloader.bin — Bootloader (Secure-Boot-aktiviert bei Production)
- partition-table.bin — Partition Table
- bootloader/bootloader.bin — Bootloader-Binary

**Companion-App:**

- APK-Datei fuer Android (companion-app/build/app/outputs/apk/)
- IPA-Datei fuer iOS (ueber Expo-EAS-Build, Beta-Tester)

**Dokumentation:**

- RELEASE-NOTES.md — Diese Datei
- CHANGELOG.md — Aenderungsprotokoll
- Archiv der relevanten docs/ als PDF-Bundle (optional)

Die Firmware-Binaries werden automatisch von GitHub Actions erstellt,
wenn ein Tag gepusht wird (siehe docs/FIRMWARE-GUIDE.md Abschnitt 4.3).
Der Workflow baut die Production-Konfiguration, signiert die Firmware
(bei aktiviertem Secure Boot) und laedt die Binaries als Release-Assets
hoch.

### 3.5 Release-Veroeffentlichung (GitHub Releases)

Die Release-Veroeffentlichung erfolgt ueber GitHub Releases:

1. Git-Tag auf main setzen und pushen
2. GitHub Actions erstellt automatisch ein Draft-Release mit den
   Firmware-Binaries
3. Release-Notes aus CHANGELOG.md werden automatisch extrahiert
4. Draft-Release pruefen, Release-Notes ergaenzen
5. Als "Latest Release" veroeffentlichen (fuer stabile Releases) oder
   als "Pre-Release" markieren (fuer alpha/beta/rc)
6. Companion-App-Builds hinzufuegen (APK, ggf. IPA)
7. Dokumentation als zusaetzliche Assets hinzufuegen (optional)

Die GitHub-Release-Seite ist die primaere Veroeffentlichungs-Plattform.
Benutzer koennen von dort die Firmware-Binaries herunterladen und ueber
SD-Karte oder USB flashen. Die Companion-App wird zusaetzlich in den
App-Stores veroeffentlicht (geplant ab v1.1).

---

## 4. Roadmap-Ausblick

### 4.1 Was ist fuer v1.1 geplant?

v1.1 ist das erste Wartungs- und Sicherheits-Release. Der Fokus liegt
auf der Schliessung der in der Sicherheitsanalyse identifizierten
Schwachstellen und der Verbesserung der Produktionsreife.

**Geplante Features fuer v1.1:**

- Admin-PIN-System: Implementierung einer zusaetzlichen
  Authentifizierungsebene fuer kritische BLE-Write-Characteristics
  (Config-Write, Bond-Management, Self-Test, OTA-Control). Die PIN
  wird in NVS gespeichert (Flash-Encryption geschuetzt), mit
  Rate-Limiting (3 Fehlversuche -> 60s Lockout) und Session-Timeout
  (5 Minuten).

- Secure Boot V2: Aktivierung der RSA-3072-Signaturverifikation im
  Bootloader. Einbettung eines echten Public-Keys zur Build-Zeit.
  Entfernung des Dev-Fallbacks in ota_secure_boot.c (fail-closed).

- Flash-Encryption: Aktivierung der Flash-Verschluesselung zum Schutz
  von NVS-Inhalten (Konfiguration, Bonding-Keys). NVS-Encryption
  aktivieren.

- Rate-Limiting fuer BLE-Verbindungen: Begrenzung der
  Pairing-Versuche (max. 5 in 60 Sekunden, 30s Cooldown).

- Connection-Log in NVS: Ring-Buffer (32 Eintraege) fuer
  BLE-Verbindungsereignisse (Connect/Disconnect mit Timestamp und
  gekuerzter Peer-Adresse).

- Erweiterter OTA-Self-Test: Zusaetzliche Pruefung von SD-Karte,
  I2C-Sensoren und BLE-Stack beim Boot.

- Anti-Rollback-Counter: Aktivierung des eFuse-Version-Counters zum
  Schutz vor Firmware-Downgrade-Angriffen.

- Performance-Optimierung: Implementierung der in
  docs/PERFORMANCE-OPTIMIZATION.md identifizierten Optimierungen
  ( Dirty-Rect-Optimierung, SPI-DMA-Pipelining, Heap-Fragmentierung).

**Schaetzung:** v1.1 wird voraussichtlich 2-3 Wochen nach v1.0
veroeffentlicht.

### 4.2 Was ist fuer v2.0 geplant?

v2.0 ist das erste Major-Release nach v1.0 und enthaelt
inkompatible API-Aenderungen und neue Kern-Features.

**Geplante Features fuer v2.0:**

- Vollstaendige Companion-App-Integration: Die React-Native-App wird
  aus dem Prototyp-Stadium in den Produktionsstatus ueberfuehrt.
  Vollstaendige Implementierung aller 5 Haupt-Screens (Dashboard,
  Navigation, Diagnose, Settings, Tours) mit echter BLE-Kommunikation.
  Veroeffentlichung in den App-Stores.

- CI/CD-Pipeline: Vollstaendige Automatisierung von Build, Test und
  Release ueber GitHub Actions. Automatische Firmware-Signierung,
  automatische Release-Erstellung bei Git-Tag, automatische
  Companion-App-Builds.

- Zweite ESP32 als Display-Controller: Entlastung des Haupt-MCU von
  der Display-Rendering-Last durch einen dedizierten
  Display-Controller. Ermöglicht hoehere Frame-Raten und komplexere
  UI-Animationen.

- WLAN-Connectivity (optional): Fuer OTA-Updates ohne Smartphone und
  fuer Cloud-Synchronisation von Touren-Logs. Das WLAN-Modul des
  ESP32-S3 wird aktiviert, aber nur fuer Updates und Synchronisation,
  nicht fuer Echtzeit-Kommunikation.

- Verbesserung der Sonnenlicht-Lesbarkeit: Evaluation eines
  transflektiven TFT-Displays oder eines groesseren E-Paper-Displays
  als Hauptdisplay.

- Erweiterte Diagnose: Implementierung weiterer DTC-Codes,
  erweiterte Sensor-Plausibilitaetspruefung und Trend-Analyse
  (z.B. Kraftstoffverbrauch ueber Zeit, Temperaturtrend).

- Community-Features: Plugin-System fuer eigene UI-Widgets und
  eigene Sensor-Treiber. Konfigurations-Import/Export als JSON-Datei.

**Schaetzung:** v2.0 wird voraussichtlich Q2 2027 veroeffentlicht.

### 4.3 Community-Beitraege und Feature-Requests

Das Projekt ist Open-Source und begruesst Community-Beitraege. Die
folgenden Kanaele stehen zur Verfuegung:

- GitHub Issues: Fuer Bug-Reports und Feature-Requests. Vor dem
  Einreichen eines Issues bitte die vorhandenen Issues durchsuchen,
  um Duplikate zu vermeiden.

- GitHub Pull Requests: Fuer Code-Beitraege. PRs muessen die
  Code-Review-Checkliste aus docs/FIRMWARE-GUIDE.md Abschnitt 6.5
  erfuellen (keine Warnungen, keine Markennamen, Tests fuer neue
  Funktionen, clang-format ausgefuehrt).

- GitHub Discussions: Fuer Fragen, Ideen und Diskussionen ueber die
  Architektur und zukuenftige Features.

**Richtlinien fuer Feature-Requests:**

1. Beschreibe den Use-Case konkret (welches Problem wird geloest?)
2. Beschreibe die erwartete Loesung oder einen Vorschlag
3. Gib an, ob du bereit bist, selbst beizutragen
4. Beruecksichtige die Architektur (modular, komponentenbasiert)

Feature-Requests werden nach MoSCoW priorisiert (Must, Should, Could,
Wont). Kritische Features mit breiter Unterstuetzung werden in die
Roadmap fuer das naechste Release aufgenommen.

**Code-Beitrag-Richtlinien:**

- Folge den Coding-Standards (MISRA-C orientiert, snake_case,
  Header-Guards, Include-Reihenfolge)
- Keine Markennamen in Code oder Kommentaren (siehe
  .github/brand-blocklist.txt)
- Schreibe Unit-Tests fuer neue Funktionen
- Dokumentiere oeffentliche API-Aenderungen im Header (Doxygen-Stil)
- Verwende ESP_LOGx statt printf in Firmware-Code
- Verwende Mutexe fuer allen Shared State

---

## Anhang: Versions-Historie

| Version | Datum | Beschreibung |
|---|---|---|
| 1.0.0 | 2026-10-13 | Erstes stabiles Release. Alle Kern-Features implementiert: Echtzeit-Telemetrie, GPS-Tracking, Dual-Display, BLE, Diagnose, OTA, Logging, Power-Management, IMU. Firmware: 9 Komponenten, 9.100+ C/H-Zeilen. Companion-App: Prototyp, 5.900+ TypeScript-Zeilen. 18 Dokumentationsdateien, 67.000+ Woerter. |

---

## Quellen und Referenzen

- docs/ROADMAP.md — Projekt-Roadmap mit Phasen und Fortschritt
- docs/ARCHITECTURE.md — System-Architektur, BLE-Protokoll, Datenmodell
- docs/TCO-ANALYSIS.md — Kostenanalyse ueber 5 Jahre
- docs/BOM.md — Stueckliste mit 67 Bauteilen
- docs/USER-MANUAL.md — Benutzerhandbuch und Installationsanleitung
- docs/FIRMWARE-GUIDE.md — Firmware-Engineering-Guide
- docs/SECURITY-ANALYSIS.md — Sicherheitsanalyse und Threat Model (STRIDE)
- docs/research/09-master-plan.md — Master-Plan mit Kosten-Nutzen-Analyse
- docs/INSTALLATION-GUIDE.md — Montage-Anleitung fuer PD06
- docs/HARDWARE.md — Hardware-Schaltplan und Pin-Layout
- docs/PINMAP.md — Pin-Belegung
- docs/PERFORMANCE-OPTIMIZATION.md — Performance-Optimierung und Profiling
- docs/CI-CD-PIPELINE.md — CI/CD-Pipeline und Automated Testing
- docs/FIELDTEST-PROTOCOL.md — Feldtest-Protokoll und Evaluations-Kriterien
- Semantic Versioning: semver.org
- Keep a Changelog: keepachangelog.com
- ESP-IDF Documentation: docs.espressif.com

---

*Dieses Dokument wird mit jedem Release aktualisiert. Aenderungen an
den Release-Notes muessen mit der CHANGELOG.md synchronisiert werden.
Bei Aenderungen am Release-Prozess muss auch docs/FIRMWARE-GUIDE.md
Abschnitt 8 (Release-Management) aktualisiert werden.*

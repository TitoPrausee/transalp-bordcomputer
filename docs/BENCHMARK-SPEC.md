# Benchmark-Spezifikation

> Bordcomputer Transalp 600 (PD06)
> MCU: ESP32-S3-WROOM-1 (N16R8: 16MB Flash, 8MB Octal PSRAM)
> Stand: Oktober 2026
> Quellen: docs/TEST-STRATEGY.md, docs/HARDWARE.md, docs/PINMAP.md,
> firmware/main/board_config.h

---

## 0. Zusammenfassung

Dieses Dokument definiert verbindliche Benchmark-Zielwerte fuer alle
Leistungs- und Ressourcen-Metriken des Bordcomputers. Jede Metrik hat
einen Zielwert, eine Messmethode, eine Prioritaet (Must / Should /
Could) und ein Pass/Fail-Kriterium. Ein Feature gilt erst als fertig,
wenn alle zugehoerigen Must-Benchmarks innerhalb der Zielwerte liegen.

Die Benchmarks korrespondieren mit den Test-Prozeduren in
docs/HARDWARE-TEST-BENCH.md. Die dort definierten Befehle (z.B.
`benchmark tft 100`) liefern die Messwerte, die hier gegen die
Zielwerte geprueft werden.

Prioritaeten:
- **Must:** Muss eingehalten werden. Feature ist ohne diese Metrik
  nicht einsatzbereit.
- **Should:** Sollte eingehalten werden. Abweichungen sind
  tolerierbar, beeinflussen aber die Nutzererfahrung.
- **Could:** Nice-to-have. Keine Blockade bei Nichteinhaltung.

---

## 1. Rendering-Performance

### 1.1 TFT-Refresh-Rate

Das TFT-Display (ILI9488, 480x320 Pixel, RGB565) wird ueber den
shared SPI2-Bus (FSPI) mit 40 MHz Taktfrequenz angesteuert. Ein
Vollbild-Update erfordert 480 * 320 * 2 = 307.200 Bytes. Bei 40 MHz
SPI-Takt entspricht das einer theoretischen Uebertragungszeit von
~15,4 ms (~65 FPS). Mit Dirty-Rect-Tracking (nur geaenderte
Bereiche) wird die effektive Refresh-Rate deutlich hoeher.

| Metrik | Zielwert | Messmethode | Prioritaet |
|---|---|---|---|
| TFT-FPS (Vollbild-Refresh) | > 30 FPS | `benchmark tft 100`: 100 Full-Frame-Updates, Zeit messen, FPS = 100 / Zeit_s | Must |
| TFT-FPS (Dirty-Rect, typ. UI) | > 60 FPS | `benchmark tft dirty 100`: 100 Updates mit kleinen Rechtecken (RPM, Speed) | Should |
| TFT-Frame-Zeit (Einzelbild) | < 33 ms | Zeit fuer ein einzelnes spi_device_queue_trans() | Must |
| SPI-Bus-Auslastung (Display) | < 50% der Buszeit | Zeitmessung: SPI-Transfers / Gesamtzeit im 1s-Fenster | Should |

Berechnung der FPS:

    FPS = Anzahl_Frames / Gesamtzeit_Sekunden

Der Bordcomputer sendet nach jedem Frame einen Zeitstempel ueber die
serielle Konsole. Der Steuerrechner misst die Differenz zwischen
erstem und letztem Frame und berechnet die FPS.

Pass-Kriterium: FPS > 30 bei Vollbild-Refresh.
Fail-Kriterium: FPS <= 30 oder visuelle Ruckler waehrend des Tests.

### 1.2 E-Paper-Update-Zeit

Das E-Paper-Display (250x122 Pixel, monochrom, SSD1680/UC8151D)
wird ebenfalls ueber FSPI angesteuert. E-Paper-Updates sind deutlich
langsamer als TFT-Updates, da die physische Umschaltung der
E-Ink-Pigmente Zeit erfordert.

| Metrik | Zielwert | Messmethode | Prioritaet |
|---|---|---|---|
| Full-Refresh-Zeit | <= 2,0 s | `benchmark epd 5`: 5 Full-Refreshes, Durchschnitt bilden | Must |
| Partial-Refresh-Zeit | <= 300 ms | `benchmark epd partial 10`: 10 Partial-Refreshes, Durchschnitt | Should |
| Ghosting nach 50 Refreshes | Kaum sichtbar | Visuelle Pruefung nach 50 Partial-Refreshes | Should |

Pass-Kriterium: Full-Refresh <= 2,0 s, keine sichtbaren
Bildueberbleibsel (Ghosting) nach Partial-Refresh.
Fail-Kriterium: Full-Refresh > 2,0 s oder starkes Ghosting.

PD06-Relevanz: Das E-Paper zeigt statische Informationen (Trip,
Kilometerstand, Uhrzeit). Full-Refresh wird nur bei Seitenwechsel
benoetigt (alle 30-60 s), Partial-Refresh fuer kleine Aktualisierungen
(Uhrzeit, Tankstand).

---

## 2. Sensor-Latenz

Die Sensor-Latenz ist die Zeit vom physikalischen Event (z.B.
Drehzahl-aenderung) bis zum sichtbaren Display-Update. Sie setzt sich
zusammen aus:

1. Sensor-Erfassung (PCNT-Zaehler, ADC-Sample)
2. Filterung (EMA, Oversampling)
3. Event-Bus-Publikation
4. Renderer-Update
5. SPI-Transfer zum Display

| Metrik | Zielwert | Messmethode | Prioritaet |
|---|---|---|---|
| RPM-Latenz (Event bis Display) | < 200 ms | Funktionsgenerator: Frequenzsprung, Oszilloskop an GPIO15 und TFT_CS, Zeit bis naechstes Frame | Must |
| Temperatur-Latenz (NTC-Aenderung bis Display) | < 500 ms | Widerstandsdekade: Sprung, Zeit bis Display-Wert aktualisiert | Must |
| Geschwindigkeits-Latenz | < 200 ms | Funktionsgenerator: Frequenzsprung an PCNT1, Zeit bis Display | Must |
| Tankstand-Latenz | < 1 s | Poti-Sprung, Zeit bis Display (langsame Filterung alpha=0,05) | Should |
| Oeldruck-Latenz | < 500 ms | DAC-Sprung, Zeit bis Display | Should |
| Bordspannungs-Latenz | < 500 ms | Labornetzteil-Sprung, Zeit bis Display | Should |
| Crash-Erkennungs-Latenz | < 500 ms | IMU-Spike (5g), Zeit bis SOS-Countdown startet | Must |

Messmethode fuer RPM-Latenz (detailliert):

1. Funktionsgenerator auf 21,7 Hz (Leerlauf) einstellen.
2. Oszilloskop CH1 an GPIO15 (RPM_PULSE), CH2 an GPIO10 (TFT_CS).
3. Frequenzsprung auf 133,3 Hz (8000 U/min) ausloesen.
4. Zeit messen von Frequenzsprung bis naechster TFT_CS-Aktivitaet
   (Display-Update).
5. Wiederhole 10x, Durchschnitt bilden.

Pass-Kriterium: Alle Must-Latenzen innerhalb Zielwerte.
Fail-Kriterium: Eine Must-Latenz ueberschritten.

PD06-Relevanz: Bei 1300 U/min Leerlauf und 8000 U/min Nenndrehzahl
muss die RPM-Anzeige schnell genug reagieren, um dem Fahrer
Rueckmeldung zu geben. Eine Latenz > 200 ms waere bei schnellen
Drehzahlveraenderungen (z.B. beim Anlassen) stoerend.

---

## 3. BLE-Durchsatz und Latenz

Der BLE-Stack (NimBLE) kommuniziert mit der Companion-App. Die
metriken messen die effektive Uebertragungsrate und die Latenz fuer
Echtzeit-Daten (Telemetry, DTC-Updates).

| Metrik | Zielwert | Messmethode | Prioritaet |
|---|---|---|---|
| BLE-Durchsatz (sustained) | > 10 kbps | `benchmark ble 102400`: 100 KB senden, Zeit messen, kbps = 102400 * 8 / Zeit_s / 1000 | Must |
| BLE-Latenz (Notify, End-to-End) | < 100 ms | Timestamp auf Bordcomputer, Empfangs-Timestamp auf App, Differenz | Must |
| BLE-Connect-Zeit | < 5 s | Zeit von Advertising-Start bis Connected-Event | Should |
| BLE-MTU | 247 Bytes | MTU-Aushandlung, Wert pruefen | Should |
| BLE-Reconnect-Zeit | < 10 s | Nach Verbindungsabbruch, Zeit bis automatisch wieder verbunden | Should |
| BLE-Notify-Verlust | 0 von 100 | 100 Telemetry-Notifies senden, alle auf App empfangen | Must |

Messmethode fuer Durchsatz:

1. BLE-Verbindung herstellen, MTU=247, DLE aktiv.
2. Befehl `benchmark ble 102400` senden.
3. Bordcomputer sendet 100 KB in Chunks zu 244 Bytes (MTU-3).
4. Zeit messen vom ersten bis letzten empfangenen Byte.
5. kbps = (102400 * 8) / Zeit_s / 1000.

Pass-Kriterium: Durchsatz > 10 kbps, Latenz < 100 ms, 0 Verluste.
Fail-Kriterium: Durchsatz <= 10 kbps oder Latenz >= 100 ms.

PD06-Relevanz: Die Telemetry-Daten (RPM, Speed, Temperatur, etc.)
werden mit 1 Hz an die App gesendet. Bei 100 Bytes pro Update
entspricht das 800 bps. Der Sollwert von 10 kbps bietet ausreichend
Reserve fuer Log-Export und OTA-Updates.

---

## 4. GPS-Update-Rate

Das GPS-Modul kommuniziert ueber UART1 (115200 Baud) mit NMEA-Saetzen.
Die Update-Rate bestimmt, wie oft pro Sekunde ein neuer Positionsfix
verfuegbar ist.

| Metrik | Zielwert | Messmethode | Prioritaet |
|---|---|---|---|
| GPS-Update-Rate (Standard) | >= 1 Hz | `benchmark gps`: Timestamp-Differenz der PVT-Pakete ueber 60s | Must |
| GPS-Update-Rate (High-Rate) | >= 5 Hz | UBX-NAV-PVT bei 5Hz-Konfiguration, gleiche Messung | Should |
| GPS-Fix-Time (Kaltstart) | < 30 s | Zeit von Power-On bis 3D-Fix (ohne Ephemeriden) | Must |
| GPS-Fix-Time (Warmstart) | < 5 s | Zeit von Power-On bis 3D-Fix (mit Ephemeriden) | Should |
| GPS-PPS-Jitter | < 1 ms | Oszilloskop an GPIO39 (PPS), Jitter ueber 60 Pulse | Should |
| GPS-Speed-Genauigkeit | ± 2 km/h | NMEA-Replay mit bekannter Geschwindigkeit, Abweichung messen | Must |

Messmethode fuer Update-Rate:

1. NMEA-Replay starten (oder echtes GPS-Signal).
2. Befehl `benchmark gps` senden.
3. Bordcomputer erfasst fuer 60 Sekunden alle eintreffenden PVT-Pakete
   mit Timestamp.
4. Update-Rate = Anzahl_Pakete / 60.
5. Ausgabe: durchschnittliche und minimale Update-Rate.

Pass-Kriterium: Update-Rate >= 1 Hz (Standard), Fix-Time < 30 s.
Fail-Kriterium: Update-Rate < 1 Hz oder Fix-Time > 30 s.

PD06-Relevanz: Die Transalp faehrt typischerweise 50-130 km/h. Bei 1 Hz
GPS-Update entspricht das 14-36 m zwischen Updates. Fuer die
Streckenprotokollierung ist das ausreichend. Bei 5 Hz (Should) werden
2,8-7,2 m Aufloesung erreicht, was fuer Kurvenanalyse nuetzlich ist.

---

## 5. Power-Consumption pro State

Der Bordcomputer hat vier Power-States. Die Stromaufnahme wird an der
12V-Versorgung gemessen (vor dem Buck-Converter), um den realen
Bordnetz-Verbrauch zu erfassen.

| Power-State | Zielwert | Messmethode | Prioritaet |
|---|---|---|---|
| ACTIVE | < 150 mA @ 12V | `power state active`, Strom messen nach 5s Stabilisierung | Must |
| DIMMED | < 80 mA @ 12V | `power state dimmed` (Backlight 20%, BLE an, Sensoren an) | Must |
| SUSPENDED | < 20 mA @ 12V | `power state suspend` (Display aus, BLE an, GPS an, Sensoren aus) | Must |
| DEEP_SLEEP | < 5 mA @ 12V | `power state sleep` (nur RTC + Wake-Source aktiv) | Must |

Zusaetzliche Power-Metriken:

| Metrik | Zielwert | Messmethode | Prioritaet |
|---|---|---|---|
| 3.3V-Rail Strom (ACTIVE) | < 250 mA | Shunt am LDO-Ausgang, Multimeter | Should |
| 5V-Rail Strom (ACTIVE) | < 500 mA | USB-Strommesser am Buck-Ausgang | Should |
| Wake-Zeit (DEEP_SLEEP zu ACTIVE) | < 3 s | Zeit von Wake-Trigger bis Display aktiv | Should |
| Backup-Akku-Laufzeit | > 20 h | Voll aktiv vom 18650-Akku (3400 mAh) | Could |

Messmethode:

1. Labornetzteil auf 12,0V einstellen.
2. Shunt (0,1 Ohm) in 12V-Leitung, Multimeter im mV-Bereich.
3. Befehl `power state <state>` senden.
4. 5 Sekunden warten (Stabilisierung).
5. Strom messen: I = U_Shunt / 0,1 Ohm.
6. Fuer DEEP_SLEEP: µA-Messbereich am Multimeter.

Pass-Kriterium: Alle Stromwerte unter den Zielwerten.
Fail-Kriterium: Ein Must-State ueberschreitet den Zielwert.

Stromaufschluesselung ACTIVE (Schaetzung, 12V-Eingang):

| Komponente | Strom @ 12V |
|---|---|
| ESP32-S3 (CPU aktiv, BLE an) | 40-60 mA |
| TFT 3,5" (Backlight 100%) | 50-70 mA |
| E-Paper 2,13" (idle) | 1-2 mA |
| GPS-Modul | 10-15 mA |
| IMU + Baro (I2C) | 1-3 mA |
| SD-Karte (idle) | 0,5-1 mA |
| Buck-Converter Verlust | 5-10 mA |
| Gesamt (schaetzung) | 108-161 mA |

Der Zielwert von 150 mA ist anspruchsvoll. Das TFT-Backlight ist der
groesste Verbraucher. Im DIMMED-Mode (Backlight 20%) sinkt der
TFT-Strom auf ca. 10-15 mA, was den Gesamtverbrauch auf < 80 mA
reduziert.

PD06-Relevanz: Die Lichtmaschine der Transalp liefert 310W (spaete
Modelle) bzw. 350W (1989-1993) bei 5000 U/min. Bei 12V entspricht
das 25,8 A bzw. 29,2 A. Der Bordcomputer verbraucht < 150 mA (1,8W),
was < 1% der Generatorleistung entspricht. Im Leerlauf (1300 U/min)
sinkt die Generatorleistung auf ca. 80W (6,7A), hier ist der
Bordcomputer-Verbrauch immer noch vernachlaessigbar.

---

## 6. Boot-Zeit

Die Boot-Zeit ist die Zeit vom Power-On bis zur Betriebsbereitschaft
(Self-Test abgeschlossen, Display aktiv).

| Metrik | Zielwert | Messmethode | Prioritaet |
|---|---|---|---|
| Boot-Zeit (bis Self-Test-Start) | < 3 s | `benchmark boot`: Zeit von Power-On bis Self-Test-Start | Must |
| Self-Test-Dauer | < 3 s | Zeitmessung Self-Test-Sequenz (I2C-Scan, SPI-Check, ADC-Check) | Should |
| Boot bis Display aktiv | < 5 s | Visuell: Zeit von Power-On bis erstes TFT-Bild | Should |
| OTA-Update-Dauer | < 30 s (2MB Firmware) | Zeit von Start bis Reboot bei BLE-Transfer | Could |

Messmethode:

1. Bordcomputer ausschalten (Strom entfernen).
2. Oszilloskop CH1 an 12V-Versorgung (Trigger: steigende Flanke).
3. Oszilloskop CH2 an GPIO21 (STATUS_LED, Trigger: erste Flanke =
   Boot-Start) oder TFT_CS (erstes Display-Update).
4. Strom einschalten.
5. Zeit messen von 12V-Anstieg bis STATUS_LED/TFT_CS-Aktivitaet.
6. Alternative: `benchmark boot` Befehl misst intern mit esp_timer.

Boot-Sequenz (detailliert):

| Phase | Dauer (Soll) | Beschreibung |
|---|---|---|
| ROM-Boot | 50-100 ms | ESP32-S3 ROM-Bootloader, Ladefrom Flash |
| ESP-IDF Init | 200-500 ms | FreeRTOS-Start, Heap-Init, SPI-Flash-Mount |
| Board-Init | 200-500 ms | GPIO, I2C, SPI, ADC, UART Konfiguration |
| Self-Test | 500-2000 ms | Module-Scan (I2C, SPI, UART, BLE) |
| Display-Init | 100-300 ms | TFT-Init, E-Paper-Init, erstes Rendering |
| Gesamt | < 3 s | Bis Self-Test-Start |

Pass-Kriterium: Boot < 3s bis Self-Test-Start.
Fail-Kriterium: Boot >= 3s.

PD06-Relevanz: Beim Einschalten der Zuendung (Schluessel auf ON)
sollte der Bordcomputer schnell genug booten, dass der Fahrer nicht
wartet. 3 Sekunden ist ein akzeptabler Wert (aehnlich wie ein
Smartphone-Display).

---

## 7. SD-Karten Schreibrate

Die SD-Karte nutzt den shared SPI2-Bus (FSPI) und ist das einzige
Geraet, das MISO verwendet. Die Schreibrate ist kritisch fuer das
GPX-Logging (GPS-Track-Aufzeichnung) und die Telemetrie-Protokollierung.

| Metrik | Zielwert | Messmethode | Prioritaet |
|---|---|---|---|
| SD-Schreibrate (sustained) | > 50 kbps | `benchmark sd 102400`: 100 KB schreiben, Zeit messen | Must |
| SD-Schreibrate (Peak) | >= 300 KB/s | FATFS-Benchmark mit 4KB-Buffer, `benchmark sd 4096 256` | Should |
| SD-Leserate | >= 200 KB/s | 100 KB lesen, Zeit messen | Should |
| SD-Mount-Zeit | < 2 s | Zeit von SD-Init bis gemountet | Should |

Messmethode fuer sustained Schreibrate:

1. Befehl `benchmark sd 102400` senden.
2. Bordcomputer oeffnet eine Testdatei, schreibt 100 KB in
   512-Byte-Bloecken (typische GPX-Log-Groesse).
3. Zeit messen von erstem bis letztem fwrite().
4. Schreibrate = 102400 Bytes / Zeit_s.
5. Ausgabe in KB/s und kbps (KB/s * 8).

Pass-Kriterium: Schreibrate > 50 kbps (6,25 KB/s) sustained.
Fail-Kriterium: Schreibrate <= 50 kbps.

PD06-Relevanz: Ein typischer GPX-Trackpoint benoetigt ca. 200 Bytes
(Koordinaten, Hoehe, Zeit, Geschwindigkeit). Bei 1 Hz GPS-Update
entspricht das 200 Bytes/s = 1,6 kbps. Der Sollwert von 50 kbps
bietet 30-fache Reserve. Bei 5 Hz GPS-Update und zusaetzlichen
Sensor-Daten (RPM, Temperatur) steigt der Bedarf auf ca. 2 KB/s
(16 kbps), was immer noch gut abgedeckt ist.

SD-Karte Initialisierung:
- SPI-Takt waehrend Init: 400 kHz (SD-Protokoll-Vorschrift).
- SPI-Takt nach Init: bis 40 MHz fuer Datentransfer.
- Filesystem: FAT32, Cluster-Groesse 32 KB.
- Empfohlene Karte: Class 10 / U1, 16 GB.

---

## 8. FreeRTOS Task-CPU-Last

Der ESP32-S3 hat zwei Kerne (Core 0 und Core 1). Die Tasks sind
fest zugewiesen:

- Core 1 (TASK_CORE_RT): Sensor-Task, GPS-Task (Echtzeit).
- Core 0 (TASK_CORE_UI): Display-Task, BLE-Task, Logger-Task,
  Power-Task (UI/Comms).

| Metrik | Zielwert | Messmethode | Prioritaet |
|---|---|---|---|
| CPU-Last pro Task | < 80% eines Cores | `benchmark cpu 10`: vTaskGetRunTimeStats() ueber 10s | Must |
| CPU-Last Core 1 (RT) gesamt | < 80% | Summe aller Core-1-Tasks | Must |
| CPU-Last Core 0 (UI) gesamt | < 80% | Summe aller Core-0-Tasks | Should |
| Task-Watchdog | Kein Timeout in 24h | 24h-Lauf, Watchdog-Event pruefen | Must |

Task-Liste und Soll-Last:

| Task | Core | Stack | Soll-Last | Beschreibung |
|---|---|---|---|---|
| sensor_task | 1 | 4096 | < 20% | ADC-Sampling (100 Hz), PCNT-Auslesen, Filterung |
| gps_task | 1 | 4096 | < 15% | NMEA-Parsing, UART-RX (115200 Baud) |
| display_task | 0 | 8192 | < 40% | TFT-Rendering, E-Paper-Update, SPI-Transfer |
| logger_task | 0 | 4096 | < 10% | SD-Schreiben (puffern + flush) |
| ble_task | 0 | 6144 | < 20% | NimBLE-Stack, GATT-Notify, Advertising |
| power_task | 0 | 2048 | < 5% | Power-State-Machine, Spannungsueberwachung |
| IDLE (beide Cores) | > 20% | — | Restkapazitaet |

Messmethode:

1. Befehl `benchmark cpu 10` senden.
2. Bordcomputer aktiviert Runtime-Statistiken (vTaskGetRunTimeStats).
3. Sammelt 10 Sekunden lang CPU-Zeit pro Task.
4. Ausgabe: Task-Name, CPU-Zeit [%], Core-Zuordnung.
5. Vergleich mit Soll-Last und < 80%-Grenze.

Pass-Kriterium: Kein Task > 80% seines Cores.
Fail-Kriterium: Ein Task >= 80%, was auf einen Engpass oder
Endlosschleife hinweist.

PD06-Relevanz: Bei 8000 U/min Nenndrehzahl muss der Sensor-Task
schnell genug sein, um die 133,3 Hz RPM-Pulse zu zaehlen und zu
filtern. Bei 100 Hz ADC-Sampling und EMA-Filterung sollte die
CPU-Last unter 20% liegen. Der Display-Task ist der
ressourcenintensivste Task (SPI-Transfers fuer 480x320 Pixel).

---

## 9. Speicherverbrauch

Der ESP32-S3-WROOM-1 N16R8 hat 16 MB Flash und 8 MB Octal PSRAM.
Der interne SRAM betraegt 512 KB. FreeRTOS verwendet den Heap
fuer dynamische Allokationen.

| Metrik | Zielwert | Messmethode | Prioritaet |
|---|---|---|---|
| Freier Heap (nach Boot) | >= 50 KB | `benchmark heap`: esp_get_free_heap_size() nach Boot | Must |
| Freier Heap (laufend, 24h) | >= 40 KB | esp_get_free_heap_size() alle 10s, Minimum ueber 24h | Must |
| Freier PSRAM (nach Boot) | >= 1,5 MB | heap_caps_get_free_size(MALLOC_CAP_SPIRAM) | Should |
| Stack-Wasserstand (pro Task) | >= 25% frei | uxTaskGetStackHighWaterMark() pro Task | Should |
| Heap-Fragmentierung | < 20% | heap_caps_get_info(), fragmented / total | Should |

Messmethode:

1. Befehl `benchmark heap` senden nach Boot.
2. Bordcomputer gibt aus:
   - Freier Heap (intern): esp_get_free_heap_size()
   - Freier PSRAM: heap_caps_get_free_size(MALLOC_CAP_SPIRAM)
   - Groesster freier Block: heap_caps_get_largest_free_block()
   - Stack-Wasserstand pro Task: uxTaskGetStackHighWaterMark()
3. Fuer 24h-Test: alle 10s Messung, Minimum protokollieren.

Pass-Kriterium: Freier Heap >= 50 KB nach Boot, >= 40 KB nach 24h.
Fail-Kriterium: Freier Heap < 50 KB nach Boot oder < 40 KB nach 24h
(weist auf Memory-Leak hin).

Stack-Sollwerte pro Task:

| Task | Stack-Groesse | Min. frei (Soll) | Beschreibung |
|---|---|---|---|
| sensor_task | 4096 B | 1024 B | ADC, Filter, I2C |
| gps_task | 4096 B | 1024 B | NMEA-Parser, UART |
| display_task | 8192 B | 2048 B | Renderer, SPI, Widgets |
| logger_task | 4096 B | 1024 B | FATFS, fwrite |
| ble_task | 6144 B | 1536 B | NimBLE, GATT |
| power_task | 2048 B | 512 B | Power-State, ADC |

PD06-Relevanz: Der Bordcomputer muss ueber Stunden hinweg stabil
laufen (Tagesausfluege, mehrtageige Touren). Ein Memory-Leak wuerde
nach 8-12 Stunden zum Absturz fuehren. Der 24h-Test mit
Heap-Monitoring ist der wichtigste Stabilitaetsindikator.

---

## 10. Benchmark-Zusammenfassung

### 10.1 Alle Must-Benchmarks

| # | Metrik | Zielwert | Dokument-Abschnitt |
|---|---|---|---|
| 1 | TFT-FPS (Vollbild) | > 30 FPS | 1.1 |
| 2 | E-Paper Full-Refresh | <= 2,0 s | 1.2 |
| 3 | RPM-Latenz | < 200 ms | 2 |
| 4 | Temperatur-Latenz | < 500 ms | 2 |
| 5 | Geschwindigkeits-Latenz | < 200 ms | 2 |
| 6 | Crash-Erkennungs-Latenz | < 500 ms | 2 |
| 7 | BLE-Durchsatz | > 10 kbps | 3 |
| 8 | BLE-Notify-Latenz | < 100 ms | 3 |
| 9 | BLE-Notify-Verlust | 0 von 100 | 3 |
| 10 | GPS-Update-Rate | >= 1 Hz | 4 |
| 11 | GPS-Fix-Time (Kaltstart) | < 30 s | 4 |
| 12 | GPS-Speed-Genauigkeit | ± 2 km/h | 4 |
| 13 | Power ACTIVE | < 150 mA @ 12V | 5 |
| 14 | Power DIMMED | < 80 mA @ 12V | 5 |
| 15 | Power SUSPENDED | < 20 mA @ 12V | 5 |
| 16 | Power DEEP_SLEEP | < 5 mA @ 12V | 5 |
| 17 | Boot-Zeit | < 3 s | 6 |
| 18 | SD-Schreibrate | > 50 kbps | 7 |
| 19 | CPU-Last pro Task | < 80% pro Core | 8 |
| 20 | Task-Watchdog | Kein Timeout in 24h | 8 |
| 21 | Freier Heap (nach Boot) | >= 50 KB | 9 |
| 22 | Freier Heap (24h) | >= 40 KB | 9 |

### 10.2 Alle Should-Benchmarks

| # | Metrik | Zielwert |
|---|---|---|
| 1 | TFT-FPS (Dirty-Rect) | > 60 FPS |
| 2 | E-Paper Partial-Refresh | <= 300 ms |
| 3 | BLE-Connect-Zeit | < 5 s |
| 4 | BLE-MTU | 247 Bytes |
| 5 | GPS-Update-Rate (High) | >= 5 Hz |
| 6 | GPS-Fix-Time (Warmstart) | < 5 s |
| 7 | GPS-PPS-Jitter | < 1 ms |
| 8 | 3.3V-Rail Strom | < 250 mA |
| 9 | 5V-Rail Strom | < 500 mA |
| 10 | Wake-Zeit | < 3 s |
| 11 | Self-Test-Dauer | < 3 s |
| 12 | Boot bis Display | < 5 s |
| 13 | SD-Schreibrate (Peak) | >= 300 KB/s |
| 14 | SD-Leserate | >= 200 KB/s |
| 15 | SD-Mount-Zeit | < 2 s |
| 16 | CPU-Last Core 0 gesamt | < 80% |
| 17 | Freier PSRAM | >= 1,5 MB |
| 18 | Stack frei pro Task | >= 25% |
| 19 | Heap-Fragmentierung | < 20% |
| 20 | SPI-Bus-Auslastung | < 50% |

### 10.3 Benchmark-Verfahren

Alle Benchmarks werden mit der in docs/HARDWARE-TEST-BENCH.md
definierten Test-Bench durchgefuehrt. Die Messwerte werden ueber
die serielle Konsole (USB-CDC) als JSON ausgegeben und vom
Python-Test-Runner ausgewertet.

Ergebnis-Format (JSON):

    {
      "benchmark": "tft_fps",
      "timestamp": "2026-10-13T22:30:00Z",
      "value": 32.05,
      "unit": "fps",
      "target": 30.0,
      "comparison": ">",
      "result": "PASS",
      "duration_ms": 3120,
      "firmware_version": "1.0.0-dev",
      "build_hash": "abc1234"
    }

Die Benchmarks werden in zwei Stufen ausgefuehrt:

1. **CI/CD (jeder Build):** Alle Must-Benchmarks, die keinen
   physischen Simulator benoetigen (TFT-FPS, Boot-Zeit, Heap,
   CPU-Last, SD-Schreibrate). Dauer: ca. 2 Minuten.

2. **Release-Test (vor Auslieferung):** Alle Must- und
   Should-Benchmarks mit allen Signal-Simulatoren. Dauer: ca.
   30 Minuten. Langzeit-Tests (24h Heap, Watchdog) separat.

---

## Anhang: Quellen und Referenzen

- docs/TEST-STRATEGY.md — Test-Pyramide, Benchmark-Zielwerte (Kapitel 5)
- docs/HARDWARE.md — Schaltplan, Power-Management, Sensor-Konditionierung
- docs/PINMAP.md — Pin-Belegung, ADC-Kanaele, PCNT-Kanaele
- docs/BOM.md — Komponenten-Liste, Stromschaetzungen
- docs/research/04-pd06-technische-daten.md — PD06 Daten (RPM, Tank, Oeldruck)
- firmware/main/board_config.h — Task-Stacks, Prioritaeten, Core-Pinning
- docs/HARDWARE-TEST-BENCH.md — Test-Bench-Aufbau, Test-Prozeduren


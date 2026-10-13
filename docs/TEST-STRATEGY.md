# Test-Strategie & Benchmark-Konzept

> Stand: Oktober 2026
> Plattform: ESP32-S3-WROOM-1 (N8R2: 8MB Flash, 2MB PSRAM), ESP-IDF v5.4, FreeRTOS SMP
> Fahrzeug: Transalp 600 (PD06), rein mechanisch, keine ECU
> Vorgaenger-Dokumente: 01-sensoren, 02-hardware, 03-software-stack, 04-pd06-daten, 05-gehaeuse-emv, 06-feature-roadmap, 07-display-navigation, 08-feature-matrix, 09-master-plan, 10-ui-ux-design, 11-firmware-architecture, 12-diagnose-support, 13-connectivity-app

---

## 0. Zusammenfassung

Dieses Dokument definiert die vollstaendige Test-Strategie fuer den Bordcomputer.
Es deckt alle drei Softwareschichten ab: Firmware (ESP-IDF/C), Companion-App
(Flutter/Dart) und die System-Integration (Firmware + Hardware + App als
Gesamtsystem). Zusaetzlich werden Hardware-in-the-Loop-Tests (HIL) und
Performance-Benchmarks mit konkreten Zielwerten definiert.

Die Strategie folgt einer klassischen Test-Pyramide: eine breite Basis aus
schnellen, isolierten Unit-Tests (Host-Seite, GCC), eine mittlere Schicht aus
Integration-Tests (Target-Seite, ESP32-S3 oder QEMU), und eine schmale Spitze
aus System- und HIL-Tests, die reale Fahr- und Fehlerszenarien abbilden. Jede
Schicht hat unterschiedliche Ausfuehrungszeit, Kosten und
Rueckmeldungsgeschwindigkeit. Die Pyramide ist bewusst so gestaffelt, dass die
meisten Fehler in den schnellen, billigen unteren Schichten gefunden werden und
nur die echten Integrationseffekte in den teuren oberen Schichten getestet
werden muessen.

Alle Test-Cases sind als Tabellen formuliert mit Test-ID, Komponente,
Vorbedingung, Schritten, erwartetem Ergebnis und Status. Die Status-Spalte ist
im Lieferzustand leer und wird waehrend der Implementierung ausgefuellt. Die
Benchmark-Zielwerte sind verbindlich: Ein Feature gilt erst als fertig, wenn
alle zugehoerigen Tests gruen sind und die Benchmarks innerhalb der Zielwerte
liegen.

Die CI/CD-Pipeline auf GitHub Actions fuehrt Unit-Tests bei jedem Commit aus,
Integration-Tests bei jedem Pull-Request, und System-Tests nach Merge in den
Main-Branch. Coverage-Targets sind >80% fuer Core-Treiber, >70% fuer
Anwendungslogik und >60% fuer UI-Code.

---

## 1. Test-Pyramide

```
                         /\
                        /  \
                       /    \
                      /  HIL \
                     /--------\
                    / System   \
                   /   Tests    \
                  /--------------\
                 / Integration   \
                /    Tests       \
               /------------------\
              /     Unit Tests     \
             /  (Host, GCC, schnell) \
            /________________________\
```

### 1.1 Ebenen-Uebersicht

| Ebene | Umgebung | Ausfuehrungszeit | Kosten | Abdeckung |
|---|---|---|---|---|
| Unit-Tests | Host-PC (GCC, native) | < 5s | Sehr niedrig | Treiber-Funktionen, Filter, Parser, DTC-Logik |
| Integration-Tests | ESP32-S3 (echte HW) oder QEMU | 10-60s | Mittel | Task-Interaktionen, Sensor-Pipeline, BLE-Stack |
| System-Tests | ESP32-S3 (echte HW, alle Komponenten) | 1-10min | Hoch | End-to-End Szenarien (Boot, Navigation, Diagnose) |
| HIL-Tests | ESP32-S3 + Sensor-Simulator | 5-30min | Sehr hoch | Reproduzierbare Crash- und Fehlerszenarien |

### 1.2 Verhaeltnis der Ebenen

Die Verteilung der Test-Faelle folgt der Regel: ca. 70% Unit-Tests, 20%
Integration-Tests, 7% System-Tests, 3% HIL-Tests. Diese Verteilung stellt
sicher, dass die meisten Fehler in den schnellen Unit-Tests gefunden werden, die
bei jedem Commit in unter 5 Sekunden laufen. Integration-Tests pruefen das
Zusammenspiel mehrerer Module und laufen in einer Minute. System- und HIL-Tests
sind teuer in der Ausfuehrung und Wartung, decken aber Szenarien ab, die in den
unteren Schichten nicht abzubilden sind.

### 1.3 Mock-Strategie

Unit-Tests auf dem Host-PC koennen nicht auf echte Hardware zugreifen. Daher
wird eine Mock-Schicht verwendet, die die Hardware-Abstraktion des ESP-IDF
nachbildet. Die Mocks sind bewusst minimal: sie simulieren Register-Zugriffe,
I2C/SPI/UART-Transfers und FreeRTOS-Primitives, aber keine echte
Peripherie-Logik. Die Mock-Schicht ist in einer separaten Header-Datei
definiert und wird nur fuer Unit-Tests gelinkt. Die gleiche Test-Logik kann
spaeter als Integration-Test auf echter Hardware laufen, indem die Mocks durch
die echten Treiber ersetzt werden.

| Mock-Komponente | Simuliert | Implementierung |
|---|---|---|
| i2c_mock | I2C-Transfers, ACK/NACK, Bus-Hang | Ring-Buffer mit konfigurierbaren Antworten |
| spi_mock | SPI-Transfers, CS-Pins | Ring-Buffer mit konfigurierbaren Antworten |
| uart_mock | UART-RX/TX (fuer GPS-NMEA) | Buffer mit vordefinierten NMEA-Saetzen |
| adc_mock | ADC-Werte | Konfigurierbare Werte pro Kanal |
| gpio_mock | GPIO-Lesung, Interrupts | Simulierte Pulse fuer RPM |
| nvs_mock | NVS-Lesen/Schreiben | RAM-basierte Key-Value-Map |
| freertos_mock | Queues, Mutexe, Semaphoren | Host-seitige Thread-Primitives (pthreads) |
| esp_timer_mock | Zeitstempel | Monotonic-Clock |

### 1.4 Test-Verzeichnisstruktur

```
test/
  unit/                    Unit-Tests (Host, GCC)
    test_rpm.c
    test_temp.c
    test_fuel.c
    test_gear.c
    test_gps_parse.c
    test_dtc.c
    test_kalman.c
    test_complementary.c
    test_config_parse.c
    test_moving_avg.c
    test_haversine.c
    test_timezone.c
    test_maintenance.c
    test_log_ring.c
    test_calib_curve.c
  integration/             Integration-Tests (ESP32-S3)
    test_task_pipeline.c
    test_spi_conflict.c
    test_ble_stress.c
    test_sd_load.c
    test_watchdog.c
    test_crash_detection.c
    test_ota_rollback.c
    test_selftest_boot.c
    test_display_dual.c
    test_dtc_pipeline.c
  system/                  System-Tests (ESP32-S3, vollstaendig)
    test_boot_sequence.c
    test_navigation_e2e.c
    test_diagnose_console.c
    test_power_loss.c
    test_sensor_degraded.c
  hil/                     HIL-Tests (ESP32-S3 + Simulator)
    test_normal_drive.c
    test_overheat.c
    test_fuel_reserve.c
    test_generator_fail.c
    test_gps_loss.c
    test_crash_sim.c
    test_sensor_disconnect.c
    test_sd_full.c
    test_power_loss_hil.c
    test_ota_hil.c
  app/                     Companion-App Tests (Flutter)
    test_ble_connect.dart
    test_ble_reconnect.dart
    test_ui_render.dart
    test_mock_data.dart
  mocks/                   Mock-Implementierungen
    i2c_mock.c
    spi_mock.c
    uart_mock.c
    adc_mock.c
    gpio_mock.c
    nvs_mock.c
    freertos_mock.c
    esp_timer_mock.c
  fixtures/                Test-Daten
    nmea/                  NMEA-Test-Saetze
    gpx/                   GPX-Referenz-Tracks
    json/                  Config-JSONs
    csv/                   Telemetrie-Referenz-CSVs
```

---

## 2. Firmware Unit-Tests

Unit-Tests pruefen einzelne Software-Module isoliert. Sie laufen auf dem
Entwicklungsrechner (nicht auf dem ESP32) mit GCC und dem Unity-Test-Framework.
Hardware-Zugriffe werden ueber die Mock-Schicht simuliert. Jeder Test ist
unabhaengig und hat keinen Seiteneffekt auf andere Tests. Die Ausfuehrungszeit
fuer die gesamte Unit-Test-Suite liegt unter 5 Sekunden.

### 2.1 RPM-Berechnung (Pulse-Counter -> RPM)

Der RPM-Sensor erzeugt einen Interrupt pro Zündimpuls. Bei einem V2-Motor mit
einem Zündimpuls pro Umdrehung entspricht eine Pulsmessung ueber ein Zeitfenster
T direkt der Drehzahl: RPM = Pulse * (60 / T). Bei zwei Zündimpulsen pro Umdrehung
(bei V2 mit beiden Zylindern in einem Hub) gilt RPM = Pulse * (60 / T) / 2. Die
Implementierung muss Grenzfaelle korrekt behandeln: null Pulse (Motor aus),
maximale Drehzahl (Redline) und Pulse-Overflow (Counter-Ueberlauf bei sehr
hohen Drehzahlen oder langen Messfenstern).

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| UT-RPM-001 | RPM | Null-Pulse (Motor aus) | PCNT-Counter = 0, Zeitfenster = 1s | Berechne RPM aus 0 Pulsen | RPM = 0, kein DTC | |
| UT-RPM-002 | RPM | Normaldrehzahl (3000 RPM) | PCNT-Counter = 50 Pulse, Zeitfenster = 1s, Faktor = 1 (1 Impuls/Umdr.) | Berechne RPM | RPM = 3000 | |
| UT-RPM-003 | RPM | Hoechstdrehzahl (Redline 8000) | PCNT-Counter = 134 Pulse, Zeitfenster = 1s, Faktor = 1 | Berechne RPM | RPM = 8040 | |
| UT-RPM-004 | RPM | Ueber-Redline (12000, unplausibel) | PCNT-Counter = 200 Pulse, Zeitfenster = 1s | Berechne RPM, pruefe Plausibilitaet | RPM = 12000, DTC P0101 (WARN) wird gesetzt | |
| UT-RPM-005 | RPM | Counter-Overflow (uint16 ueberlauf) | PCNT-Counter = 65535, dann +10 Pulse | Berechne RPM nach Overflow | Overflow wird erkannt, RPM korrekt berechnet (nicht negativ) | |
| UT-RPM-006 | RPM | Sehr kurzes Zeitfenster (10ms) | PCNT-Counter = 1, Zeitfenster = 10ms | Berechne RPM | RPM = 6000, Toleranzbereich eingehalten | |
| UT-RPM-007 | RPM | Moving-Average ueber 5 Samples | 5 Messungen: 3000, 3100, 2900, 3050, 3000 RPM | Berechne geglaetteten RPM-Wert | Average = 3010, Jitter reduziert | |
| UT-RPM-008 | RPM | Signal-Verlust (kein Interrupt >2s) | Kein Interrupt fuer 3 Sekunden | Pruefe Timeout | RPM = 0, DTC P0102 (ERROR) wird gesetzt | |
| UT-RPM-009 | RPM | Entprellung (Bounce < 2000us) | Zwei Interrupts innerhalb 1ms | Pruefe Entprell-Logik | Zweiter Interrupt wird ignoriert | |
| UT-RPM-010 | RPM | 2-Impuls-pro-Umdrehung-Modus | PCNT-Counter = 100, Faktor = 2 | Berechne RPM | RPM = 3000 (100 * 60 / 2) | |

### 2.2 Temperatursensor (NTC-Widerstand -> Temperatur)

Der Kuehlmitteltemperatursensor ist ein NTC-Thermistor (10k bei 25C). Der
Widerstand wird ueber einen Spannungsteiler am ADC gemessen. Die Umrechnung von
ADC-Wert -> Widerstand -> Temperatur erfolgt ueber die Steinhart-Hart-Gleichung.
Alternativ wird ein digitaler Sensor (DS18B20, 1-Wire) verwendet, der die
Temperatur direkt in Grad Celsius liefert. Die Tests pruefen beide Pfade.

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| UT-TEMP-001 | Temp (NTC) | Raumtemperatur (25C) | ADC-Wert entspricht 10k Ohm bei 25C | Berechne Temperatur aus ADC | Temp = 25C +/- 1C | |
| UT-TEMP-002 | Temp (NTC) | Betriebstemperatur (85C) | ADC-Wert entspricht ~1.5k Ohm bei 85C | Berechne Temperatur | Temp = 85C +/- 2C | |
| UT-TEMP-003 | Temp (NTC) | Ueberhitzung (110C) | ADC-Wert entspricht ~600 Ohm bei 110C | Berechne Temperatur, pruefe Schwelle | Temp = 110C, DTC S0102 (WARN) | |
| UT-TEMP-004 | Temp (NTC) | Kritisch (120C) | ADC-Wert entspricht ~400 Ohm bei 120C | Berechne Temperatur, pruefe Schwelle | Temp = 120C, DTC S0103 (CRITICAL) | |
| UT-TEMP-005 | Temp (NTC) | Kurzschluss (ADC = max) | ADC-Wert = 4095 (Widerstand ~0) | Berechne Temperatur, pruefe Plausibilitaet | Temp unplausibel, DTC S0101 (ERROR) | |
| UT-TEMP-006 | Temp (NTC) | Unterbrechung (ADC = 0) | ADC-Wert = 0 (Widerstand unendlich) | Berechne Temperatur, pruefe Plausibilitaet | Temp unplausibel, DTC S0101 (ERROR) | |
| UT-TEMP-007 | Temp (NTC) | Steinhart-Hart-Koeffizienten | Bekannter NTC (10k, 3950K Beta) | Berechne Temperatur bei 5 verschiedenen Widerstaenden | Alle Werte innerhalb +/-1C der Referenztabelle | |
| UT-TEMP-008 | Temp (DS18B20) | DS18B20 liest 25C | 1-Wire-Geraet liefert 0x0190 (25.0C) | Wandle Rohwert in Celsius | Temp = 25.0C | |
| UT-TEMP-009 | Temp (DS18B20) | DS18B20 liest -10C | 1-Wire-Geraet liefert negativen Rohwert | Wandle Rohwert in Celsius | Temp = -10.0C (Vorzeichen korrekt) | |
| UT-TEMP-010 | Temp (DS18B20) | DS18B20 nicht gefunden | 1-Wire-Scan findet kein Geraet | Pruefe Sensor-Verfuegbarkeit | DTC S0101 (ERROR) wird gesetzt | |

### 2.3 Kraftstoffstand (ADC-Wert -> Liter)

Der Kraftstoffstand wird ueber einen Widerstandsgeber im Tank gemessen, der
nicht-linear ist. Die Umrechnung erfolgt ueber eine Kalibrierungskurve mit bis
zu 8 Stuetzpunkten. Die Tests pruefen die Interpolation zwischen Stuetzpunkten
und die Grenzfaelle (leer, voll, ausserhalb des Bereichs).

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| UT-FUEL-001 | Fuel | Leertank (0%) | ADC = 200mV (EMPTY-Kalibrierungspunkt) | Interpoliere Fuellstand | Fuel = 0%, DTC S0202 (WARN Reserve) | |
| UT-FUEL-002 | Fuel | Volltank (100%) | ADC = 2900mV (FULL-Kalibrierungspunkt) | Interpoliere Fuellstand | Fuel = 100% | |
| UT-FUEL-003 | Fuel | Halber Tank (50%) | ADC = Mitte zwischen EMPTY und FULL | Interpoliere Fuellstand | Fuel = 50% +/- 5% | |
| UT-FUEL-004 | Fuel | Reserve (<10%) | ADC nahe EMPTY | Interpoliere, pruefe Schwelle | Fuel < 10%, DTC S0202 (WARN) | |
| UT-FUEL-005 | Fuel | Signal verloren (ADC = 0) | ADC = 0 (Sensor getrennt) | Pruefe Plausibilitaet | DTC S0201 (ERROR) wird gesetzt | |
| UT-FUEL-006 | Fuel | Ausserhalb Bereich (ADC > max) | ADC > FULL-Kalibrierungspunkt | Pruefe Plausibilitaet | Fuel auf 100% begrenzt, DTC S0201 (WARN) | |
| UT-FUEL-007 | Fuel | Nicht-lineare Kurve (4 Stuetzpunkte) | Kalibrierung: 0L=200mV, 5L=800mV, 12L=1800mV, 18L=2900mV | Interpoliere bei 1000mV und 2000mV | 1000mV ~6L, 2000mV ~13.5L (Interpolation korrekt) | |
| UT-FUEL-008 | Fuel | Reichweitenberechnung | Verbrauch = 4L/100km, Fuel = 8L | Berechne Reichweite | Range = 200 km | |

### 2.4 Ganganzeige (Speed/RPM -> Gang)

Die Ganganzeige berechnet den eingelegten Gang aus dem Verhaeltnis von
Drehzahl zu Geschwindigkeit. Da das Getriebe der PD06 5 Gange hat (plus
Leerlauf), werden 6 Zustaende unterschieden. Die Schaltpunkte werden in einem
Lern-Modus erfasst und in NVS gespeichert.

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| UT-GEAR-001 | Gear | Leerlauf (Speed=0, RPM=1200) | Speed = 0 km/h, RPM = 1200 | Berechne Gang | Gang = N (Neutral) | |
| UT-GEAR-002 | Gear | 1. Gang (20 km/h, 3000 RPM) | Speed = 20, RPM = 3000 | Berechne Gang | Gang = 1 | |
| UT-GEAR-003 | Gear | 2. Gang (40 km/h, 3000 RPM) | Speed = 40, RPM = 3000 | Berechne Gang | Gang = 2 | |
| UT-GEAR-004 | Gear | 3. Gang (60 km/h, 3000 RPM) | Speed = 60, RPM = 3000 | Berechne Gang | Gang = 3 | |
| UT-GEAR-005 | Gear | 4. Gang (80 km/h, 3000 RPM) | Speed = 80, RPM = 3000 | Berechne Gang | Gang = 4 | |
| UT-GEAR-006 | Gear | 5. Gang (100 km/h, 3000 RPM) | Speed = 100, RPM = 3000 | Berechne Gang | Gang = 5 | |
| UT-GEAR-007 | Gear | Plausibilitaet (Gang > 6) | Speed = 200, RPM = 1000 | Berechne Gang, pruefe Plausibilitaet | DTC P0301 (WARN) wird gesetzt | |
| UT-GEAR-008 | Gear | Schaltpunkt-Lernmodus | 5 bekannte Speed/RPM-Paare | Lerne Schaltpunkte, speichere in NVS | Schaltpunkte korrekt gespeichert, Gang-Erkennung danach korrekt | |
| UT-GEAR-009 | Gear | Kupplung gezogen (RPM faellt, Speed konstant) | Speed = 60, RPM faellt von 3000 auf 1500 | Berechne Gang | Gang bleibt bei 3 (RPM-Abfall ignoriert) | |
| UT-GEAR-010 | Gear | Anhalten (Speed -> 0, RPM -> Leerlauf) | Speed faellt auf 0, RPM auf 1200 | Berechne Gang | Gang = N | |

### 2.5 GPS-Parser (NMEA-Sentences)

Der GPS-Treiber parst NMEA-Saetze (GGA, RMC, VTG) und optional das
binaere UBX-Protokoll. Die Tests verwenden vordefinierte NMEA-Strings aus dem
fixtures-Verzeichnis und pruefen das Parsing auf Korrektheit und
Fehlertoleranz.

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| UT-GPS-001 | GPS | GGA gueltig (Fix, 9 Sats) | NMEA: `$GNGGA,142301.0,5056.1234,N,01135.2345,E,1,09,1.2,245.3,M,47.1,M,,*47` | Parse GGA | Lat=50.93539, Lon=11.58724, Sats=9, Alt=245.3 | |
| UT-GPS-002 | GPS | GGA ohne Fix | NMEA: `$GNGGA,142301.0,,,,,0,00,99.0,,,,,,*66` | Parse GGA | Fix-Status = 0, keine Position | |
| UT-GPS-003 | GPS | RMC gueltig (Speed+Heading) | NMEA: `$GNRMC,142301.0,A,5056.1234,N,01135.2345,E,45.3,270.0,131026,,,A*56` | Parse RMC | Speed=45.3 kn = 83.9 km/h, Heading=270 | |
| UT-GPS-004 | GPS | RMC void (kein Fix) | NMEA: `$GNRMC,142301.0,V,,,,,,,131026,,,N*53` | Parse RMC | Status=V, keine Daten | |
| UT-GPS-005 | GPS | VTG (Speed km/h) | NMEA: `$GNVTG,270.0,T,268.5,M,83.9,K,45.3,N*13` | Parse VTG | Speed=83.9 km/h, Heading=270 | |
| UT-GPS-006 | GPS | Checksum-Fehler | NMEA mit falscher Checksum | Parse, pruefe Checksum | Satz wird verworfen, Fehler gezaehlt | |
| UT-GPS-007 | GPS | Unvollstaendiger Satz | NMEA: `$GNGGA,142301.0,5056` (abgeschnitten) | Parse | Satz wird verworfen, kein Absturz | |
| UT-GPS-008 | GPS | Leerer Satz | NMEA: nur `$\r\n` | Parse | Satz wird verworfen, kein Absturz | |
| UT-GPS-009 | GPS | Unbekannter Satz-Typ | NMEA: `$GPXYZ,...` | Parse | Satz wird ignoriert, kein Absturz | |
| UT-GPS-010 | GPS | Multi-Satz-Buffer | Drei Saetze in einem Buffer (GGA+RMC+VTG) | Parse alle drei | Alle drei Saetze korrekt geparst, keine Reste im Buffer | |
| UT-GPS-011 | GPS | UBX-NAV-PVT gueltig | Binaerer UBX-Puffer mit gueltigem PVT | Parse UBX | Position, Speed, Heading, Sats korrekt extrahiert | |
| UT-GPS-012 | GPS | UBX mit falscher Laenge | UBX-Puffer mit fehlerhafter Laenge | Parse, pruefe Checksum | Satz wird verworfen | |
| UT-GPS-013 | GPS | GPS-Modul antwortet nicht | Keine NMEA-Daten fuer >5s | Pruefe Timeout | DTC S0501 (WARN) wird gesetzt | |
| UT-GPS-014 | GPS | GPS-Fix verloren (Sats < 4) | Sats faellt von 8 auf 3 | Pruefe Fix-Qualitaet | DTC S0502 (WARN) wird gesetzt | |
| UT-GPS-015 | GPS | GPS-Fix wiederhergestellt | Sats steigt von 3 auf 8 | Pruefe Fix-Qualitaet | DTC S0503 (INFO), S0502 wird gecleart | |

### 2.6 DTC-System (Diagnostic Trouble Codes)

Das DTC-System verwaltet Fehlercodes: generieren (set), speichern (NVS + SD),
loeschen (clear) und priorisieren (Severity). Die Tests pruefen die
Datenstruktur-Integritaet, die Persistenz und die Auto-Clear-Logik.

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| UT-DTC-001 | DTC | DTC setzen (neu) | DTC-Tabelle leer | dtc_set(S0102, WARN) | DTC in Tabelle, Status=ACTIVE, count=1, first_occurrence=now | |
| UT-DTC-002 | DTC | DTC wiederholt setzen (count++) | S0102 bereits ACTIVE | dtc_set(S0102, WARN) erneut | count=2, last_occurrence aktualisiert, NVS nur bei count%10==0 geschrieben | |
| UT-DTC-003 | DTC | DTC loeschen (clear) | S0102 ACTIVE | dtc_clear(S0102) | Status=HISTORY, nicht aktiv | |
| UT-DTC-004 | DTC | DTC alle loeschen | 5 DTCs aktiv | dtc_clear_all() | Alle DTCs Status=HISTORY, NVS und SD aktualisiert | |
| UT-DTC-005 | DTC | Auto-Clear nach N Zyklen | S0102 ACTIVE, auto_clear_cycles=10 | 10x gueltige Lesung | DTC wird nach 10 Zyklen auf HISTORY gesetzt | |
| UT-DTC-006 | DTC | Auto-Clear abgebrochen | S0102 ACTIVE, 9x gueltig, dann 1x ungueltig | Pruefe Auto-Clear-Zaehler | Zaehler zurueckgesetzt, DTC bleibt ACTIVE | |
| UT-DTC-007 | DTC | Severity-Priorisierung | P0402 (CRITICAL) und S0102 (WARN) aktiv | Sortiere DTCs nach Severity | CRITICAL vor WARN in der Anzeige | |
| UT-DTC-008 | DTC | NVS-Persistenz (Speichern+Laden) | 3 DTCs aktiv | Speichere in NVS, loesche RAM, lade aus NVS | Alle 3 DTCs korrekt geladen | |
| UT-DTC-009 | DTC | NVS voll (>32 DTCs) | 32 DTCs gespeichert | Versuche 33. DTC zu setzen | Aeltester HISTORY-DTC wird verworfen, neuer DTC gespeichert | |
| UT-DTC-010 | DTC | SD-Log-Eintrag | DTC wird gesetzt | Pruefe dtc.log auf SD-Mock | Eintrag im Format `timestamp|code|sev|status|count|desc` | |
| UT-DTC-011 | DTC | DTC-Export als JSON | 2 DTCs aktiv | Exportiere als JSON | Gueltiges JSON mit allen Feldern | |
| UT-DTC-012 | DTC | DTC-Liste als Binary (BLE) | 3 DTCs aktiv | Serialisiere als Binary | 3x12=36 Bytes, alle Felder korrekt | |

### 2.7 Kalman-Filter (GPS-Speed-Glaettung)

Der Kalman-Filter glaettet die GPS-Geschwindigkeitsmessung, um kurzfristige
Ausreisser und Rauschen zu reduzieren. Die Tests pruefen das Konvergenzverhalten,
die Rauschunterdrueckung und das Verhalten bei schnell wechselnden
Geschwindigkeiten.

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| UT-KAL-001 | Kalman | Konstante Geschwindigkeit (50 km/h) | 20 Messungen: alle 50 km/h | Filtere mit Kalman | Output konvergiert gegen 50 km/h, Varianz -> 0 | |
| UT-KAL-002 | Kalman | Rauschen um 50 km/h | 20 Messungen: 48-52 km/h (Gauss) | Filtere mit Kalman | Output glaettet auf ~50 km/h, Jitter < 1 km/h | |
| UT-KAL-003 | Kalman | Beschleunigung (0 -> 100) | 20 Messungen: 0, 5, 10, ... 100 km/h | Filtere mit Kalman | Output folgt Beschleunigung mit geringer Verzoegerung (<200ms) | |
| UT-KAL-004 | Kalman | Bremsung (100 -> 0) | 20 Messungen: 100, 95, 90, ... 0 km/h | Filtere mit Kalman | Output folgt Bremsung, keine Overshoots | |
| UT-KAL-005 | Kalman | Ausreisser (Single Spike) | Messungen: 50, 50, 200, 50, 50 km/h | Filtere mit Kalman | Ausreisser wird unterdrueckt, Output bleibt bei ~50 | |
| UT-KAL-006 | Kalman | GPS-Fix verloren | Messungen stoppen nach 10 Werten | Pruefe Verhalten bei fehlenden Updates | Letzter Wert wird gehalten, Varianz steigt | |

### 2.8 Complementary-Filter (IMU Roll/Pitch)

Der Complementary-Filter kombiniert Gyroskop- und Beschleunigungssensor-Daten,
um Roll- und Neigungswinkel zu berechnen. Das Gyro ist kurzzeitig genau aber
driftet, der Accel ist langfristig stabil aber rauschbehaftet. Der Filter
nutzt beide互补: Gyro fuer schnelle Aenderungen, Accel fuer langsame
Korrektur.

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| UT-COMP-001 | Complementary | Stillstand (0 Grad) | IMU: ax=0, ay=0, az=1g, gx=gy=gz=0 | Filtere 100 Zyklen | Roll=0, Pitch=0 | |
| UT-COMP-002 | Complementary | Konstante Neigung 30 Grad | IMU: ax=sin(30), az=cos(30)*1g | Filtere 100 Zyklen | Roll=30 Grad +/- 1 Grad | |
| UT-COMP-003 | Complementary | Konstante Neigung -30 Grad | IMU: ax=-sin(30), az=cos(30)*1g | Filtere 100 Zyklen | Roll=-30 Grad +/- 1 Grad | |
| UT-COMP-004 | Complementary | Pitch-Neigung 20 Grad | IMU: ay=sin(20), az=cos(20)*1g | Filtere 100 Zyklen | Pitch=20 Grad +/- 1 Grad | |
| UT-COMP-005 | Complementary | Gyro-Drift (kein Accel) | Gyro: konstant 0.1 rad/s, Accel: 0 | Filtere 1000 Zyklen | Roll steigt erst, wird dann durch Accel korrigiert | |
| UT-COMP-006 | Complementary | Schnelle Aenderung (Wobble) | Gyro: sinusfoermig 10 Hz, Accel: entsprechend | Filtere 100 Zyklen | Output folgt Gyro, Accel-Korrektur gering | |
| UT-COMP-007 | Complementary | IMU unplausibel (>5g) | Accel: az=5.5g (unmoeglich bei Stillstand) | Pruefe Plausibilitaet | DTC S0402 (WARN) wird gesetzt | |
| UT-COMP-008 | Complementary | IMU kalibriert (Offsets) | 1000 Samples bei Stillstand -> Offsets berechnen | Wende Offsets an, filtere | Roll/Pitch naeher an 0 als ohne Kalibrierung | |

### 2.9 Weitere Unit-Tests

Zusaetzlich zu den oben detailliert beschriebenen Modulen werden folgende
Software-Module mit Unit-Tests abgedeckt:

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| UT-CFG-001 | Config | Gueltiges JSON parsen | config.json mit 25 Schluesseln | Parse JSON | Alle 25 Werte korrekt geladen | |
| UT-CFG-002 | Config | Ungueltiges JSON (Syntax-Fehler) | config.json mit fehlendem Komma | Parse JSON | Default-Werte werden verwendet, Fehler geloggt | |
| UT-CFG-003 | Config | Fehlender Schluessel | config.json ohne `display.brightness` | Parse JSON, lese Schluessel | Default-Wert (80) wird verwendet | |
| UT-CFG-004 | Config | Typ-Konvertierung | JSON-Wert als String, erwartet als int | Konvertiere | Typ-Konvertierung erfolgreich oder Fehler geloggt | |
| UT-CFG-005 | Config | NVS-Speichern und Laden | Konfiguration geaendert | Speichere in NVS-Mock, lade zurueck | Alle Werte korrekt | |
| UT-HAV-001 | Haversine | Bekannte Distanz (Jena -> Weimar) | Zwei GPS-Positionen, Distanz ~20km | Berechne Haversine | Distanz = 20.0 km +/- 0.1 km | |
| UT-HAV-002 | Haversine | Null-Distanz | Gleiche Position | Berechne | Distanz = 0 | |
| UT-HAV-003 | Haversine | Antipodale Punkte | Position und exakter Gegenpunkt | Berechne | Distanz ~20015 km | |
| UT-HAV-004 | Haversine | Pol-Naehe | Zwei Punkte bei 89 Grad Breite | Berechne | Distanz korrekt (keine Division durch Null) | |
| UT-TZ-001 | Timezone | CET -> UTC | 14:23 CET (Winter) | Konvertiere zu UTC | 13:23 UTC | |
| UT-TZ-002 | Timezone | CEST -> UTC | 14:23 CEST (Sommer) | Konvertiere zu UTC | 12:23 UTC | |
| UT-TZ-003 | Timezone | DST-Uebergang (Sommer -> Winter) | 31.10.2026 02:30 CEST | Pruefe DST-Status | DST endet, Uhr wird zurueckgestellt | |
| UT-TZ-004 | Timezone | Schaltjahr (29.02.) | 29.02.2028 | Pruefe Gueltigkeit | Datum gueltig | |
| UT-MA-001 | Moving-Avg | Leerer Buffer | 0 Samples | Berechne Average | Average = 0 oder Fehler | |
| UT-MA-002 | Moving-Avg | Voller Buffer (5 Samples) | 5 Samples: 10, 20, 30, 40, 50 | Berechne Average | Average = 30 | |
| UT-MA-003 | Moving-Avg | Buffer-Rotation (6. Sample) | Buffer voll, neues Sample = 60 | Berechne Average | Average = 40 (aeltestes verworfen) | |
| UT-MA-004 | Moving-Avg | Overflow-Schutz | 5 Samples nahe INT16_MAX | Berechne Average | Kein Overflow, korrekter Durchschnitt | |
| UT-MA-005 | Moving-Avg | Negative Werte | Samples: -10, -20, -30 | Berechne Average | Average = -20 | |
| UT-MA-006 | Moving-Avg | Alle gleichen Werte | 5x den Wert 42 | Berechne Average | Average = 42, Varianz = 0 | |
| UT-LOG-001 | Log-Ring | Buffer leer | Ring-Buffer initial | Lese | Keine Daten | |
| UT-LOG-002 | Log-Ring | Buffer teilweise gefuellt | 3 Eintraege in 8KB-Buffer | Lese alle | 3 Eintraege zurueck | |
| UT-LOG-003 | Log-Ring | Buffer voll (Wrap-Around) | >8KB Eintraege | Lese | Aelteste Eintraege werden ueberschrieben | |
| UT-LOG-004 | Log-Ring | Mutex-Schutz | ZweiThreads schreiben gleichzeitig | Schreibe parallel | Keine Korruption, keine Race-Condition | |
| UT-MAINT-001 | Maintenance | km-basiertes Intervall | ODO=5400, Intervall=6000km | Berechne Fortschritt | 90%, Warnung wird ausgegeben | |
| UT-MAINT-002 | Maintenance | zeitbasiertes Intervall | 328 Tage, Intervall=365 Tage | Berechne Fortschritt | 90%, Warnung | |
| UT-MAINT-003 | Maintenance | kombiniertes Intervall | 5400km/328 Tage, Intervall=6000km/365T | Berechne | 90% (welcher zuerst erreicht wird) | |
| UT-MAINT-004 | Maintenance | Reset nach Wartung | Intervall bei 90% | Reset | Zaehler zurueckgesetzt, 0% | |
| UT-MAINT-005 | Maintenance | Betriebsstunden-Zaehler | 500h Betrieb | Pruefe Intervall | Warnung bei 500h | |
| UT-CAL-001 | Calibration | 2-Punkt-Kalibrierung (Fuel) | EMPTY=200mV, FULL=2900mV | Interpoliere bei 1500mV | 50% (linear) | |
| UT-CAL-002 | Calibration | 8-Punkt-Kalibrierung | 8 Stuetzpunkte definiert | Interpoliere zwischen Punkten | Interpolation korrekt, CRC gueltig | |
| UT-CAL-003 | Calibration | CRC-Pruefung | Kalibrierungs-Daten geaendert | Pruefe CRC | CRC-Fehler erkannt, DTC D0601 (ERROR) | |

---

## 3. Integration-Tests

Integration-Tests pruefen das Zusammenspiel mehrerer Module und FreeRTOS-Tasks.
Sie laufen auf dem ESP32-S3 (echte Hardware oder QEMU-Emulation) und
verwenden echte FreeRTOS-Primitives, echte I2C/SPI/UART-Busse (mit echten oder
simulierten Peripherie-Geraeten) und echte NVS/SD-Karte.

Im Gegensatz zu Unit-Tests, die isolierte Funktionen pruefen, pruefen
Integration-Tests die Datenfluesse zwischen Tasks: Sensor-Task liest Daten,
stellt sie in eine Queue, Display-Task liest aus der Queue und rendert,
Logger-Task schreibt auf SD-Karte. Fehler in dieser Kette (falsche
Queue-Groesse, Priority-Inversion, Buffer-Overflow) werden nur hier sichtbar.

### 3.1 FreeRTOS-Task-Interaktionen

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| IT-TASK-001 | Tasks | GPS -> Logger -> SD | GPS-Task laeuft, Logger-Task laeuft, SD gemountet | Empfange GPS-Daten, warte 10s | CSV-Datei enthaelt GPS-Werte im richtigen Format | |
| IT-TASK-002 | Tasks | GPS -> Display | GPS-Task laeuft, Display-Task laeuft | Empfange GPS-Speed, warte 2s | Display zeigt aktuelle Geschwindigkeit, aktualisiert alle 1s | |
| IT-TASK-003 | Tasks | RPM-Interrupt -> Display | RPM-GPIO konfiguriert, Display-Task laeuft | Simuliere RPM-Pulse, warte 1s | Display zeigt RPM-Wert, Latenz < 200ms | |
| IT-TASK-004 | Tasks | BLE-Notify -> App | BLE-Stack initialisiert, App verbunden | Sende Live-Daten via Notify | App empfängt 1 Hz Updates, Format korrekt | |
| IT-TASK-005 | Tasks | Crash-Detection -> SOS | Crash-Task laeuft, BLE verbunden | Simuliere Crash (IMU-Spike + Neigung) | Countdown startet, SOS nach 10s (wenn nicht abgebrochen) | |
| IT-TASK-006 | Tasks | Power-Loss -> SD-Flush | Logger-Task laeuft, SD gemountet | Loese Power-Loss-Interrupt aus | Letzte Daten auf SD gesichert, Datei korrekt geschlossen | |
| IT-TASK-007 | Tasks | Sensor-Task -> DTC-Setzen | Sensor-Task laeuft, DTC-Engine initialisiert | Simuliere Sensor-Ausfall (Temp unplausibel) | DTC wird gesetzt, Display zeigt Warnung, BLE-Notify | |
| IT-TASK-008 | Tasks | Self-Test beim Boot | System frisch gestartet | Warte bis Self-Test abgeschlossen | Self-Test-Report korrekt, DTCs fuer fehlende Module gesetzt | |
| IT-TASK-009 | Tasks | OTA-Update (BLE) | BLE verbunden, OTA-Partition vorhanden | Sende Firmware ueber BLE in Chunks | Update erfolgreich, Neustart mit neuer Version | |
| IT-TASK-010 | Tasks | Dual-Display (TFT + E-Paper) | Beide Displays konfiguriert | Aktualisiere beide Displays gleichzeitig | Keine SPI-Kollision, beide zeigen korrekte Daten | |

### 3.2 SPI-Bus-Konflikte

Der Bordcomputer verwendet zwei SPI-Busse: SPI2 (FSPI) fuer das TFT-Display
und SPI3 (HSPI) fuer SD-Karte und E-Paper-Display. Auf SPI3 teilen sich SD-Karte
und E-Paper den Bus ueber unterschiedliche CS-Pins. Bei gleichzeitigem Zugriff
kann es zu Konflikten kommen, die durch einen Mutex geloest werden muessen.

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| IT-SPI-001 | SPI | Gleichzeitiger Zugriff TFT + SD | SPI2 (TFT) und SPI3 (SD) aktiv | Schreibe gleichzeitig auf beide Busse | Keine Kollision, beide Transfers erfolgreich | |
| IT-SPI-002 | SPI | Gleichzeitiger Zugriff SD + E-Paper | SPI3 geteilt (SD + E-Paper) | Schreibe gleichzeitig auf SD und E-Paper | Mutex verhindert Kollision, beide erfolgreich (sequentiell) | |
| IT-SPI-003 | SPI | SPI-Timeout (Display nicht erreichbar) | TFT CS-Pin auf HIGH gezogen (Display weg) | Versuche Display-Init | Timeout nach 500ms, DTC B0101 (ERROR) | |
| IT-SPI-004 | SPI | SPI-Timeout (SD-Karte nicht erreichbar) | SD-Karte entfernt | Versuche SD-Mount | Timeout, DTC C0101 (ERROR), Fallback auf RAM-Buffer | |
| IT-SPI-005 | SPI | SPI-Taktrate-Konflikt | SD-Karte bei 40MHz, E-Paper bei 10MHz | Wechsle zwischen SD und E-Paper | Taktrate wird korrekt umgeschaltet, keine Datenkorruption | |

### 3.3 BLE-Notification-Stress

Der BLE-Stack (NimBLE) muss mehrere Characteristics gleichzeitig benachrichtigen
koennen: Telemetry (1 Hz), DTC-Updates (ereignisgesteuert), Self-Test-Ergebnisse
(bei Trigger), Log-Export (Stream). Die Tests pruefen die Stabilitaet unter
Last.

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| IT-BLE-001 | BLE | Single-Char-Notify (Telemetry 1Hz) | App verbunden, MTU=247 | Sende 100 Telemetry-Updates | Alle Updates empfangen, Latenz < 100ms | |
| IT-BLE-002 | BLE | Multi-Char-Notify (Telemetry + DTC) | App verbunden | Sende Telemetry und DTC-Update gleichzeitig | Beide Updates empfangen, keine Ueberlagerung | |
| IT-BLE-003 | BLE | Stress: 5 Characteristics gleichzeitig | App verbunden, 5 aktive Notifies | Sende auf allen 5 gleichzeitig | Alle 5 empfangen, keine Verbindungsabbrueche | |
| IT-BLE-004 | BLE | MTU-Aushandlung | App fordert MTU=247 | Aushandlung | MTU=247 akzeptiert, Payload bis 244 Bytes | |
| IT-BLE-005 | BLE | Fallback MTU=23 | App fordert nur MTU=23 | Sende Telemetry | Chunking auf 20-Byte-Payload, Funktion gegeben | |
| IT-BLE-006 | BLE | Log-Export-Stream | App fordert system.log (50KB) | Streame in 240B Chunks | Alle Chunks empfangen, Datei rekonstruierbar | |
| IT-BLE-007 | BLE | Verbindungsabbruch waehrend Notify | App verbunden, Notify aktiv | Trenne App waehrend Notify | BLE-Stack bleibt stabil, DTC C0202 (WARN), Auto-Reconnect | |
| IT-BLE-008 | BLE | Advertising-Start/Stop | BLE initialisiert | Starte/Stoppe Advertising 10x | Kein Stack-Fehler, Advertising zuverlaessig | |

### 3.4 SD-Karte unter Last

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| IT-SD-001 | SD | Schreiben unter Last | SD gemountet, alle Tasks aktiv | Schreibe 1MB Telemetrie bei voller Sensor-Last | Schreibrate > 100 KB/s sustained, keine Datenverluste | |
| IT-SD-002 | SD | Log-Rotation | system.log bei 1MB | Loese Rotation aus | Alte Datei umbenannt, neue erstellt, alte >30 Tage geloescht | |
| IT-SD-003 | SD | SD-Karte voll | SD mit <1KB frei | Schreibe weiter | DTC C0103 (ERROR), Logging stoppt sauber, keine Korruption | |
| IT-SD-004 | SD | Schreibfehler (CRC) | SD-Karte defekt simuliert | Schreibe Daten | DTC C0104 (WARN), Retry wird ausgefuehrt | |
| IT-SD-005 | SD | Power-Loss waehrend Schreiben | SD aktiv, Schreiben laeuft | Unterbreche Spannung | Letzte完整 flushte Daten konsistent, keine korrupte Datei | |

### 3.5 Watchdog

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| IT-WDG-001 | Watchdog | Task-Timeout -> Reset | Watchdog aktiv fuer alle Tasks | Blockiere einen Task (Endlosschleife) | Watchdog loest nach Timeout aus, System reset | |
| IT-WDG-002 | Watchdog | Task normal (kein Reset) | Watchdog aktiv | Alle Tasks feed den Watchdog | Kein Reset, System laeuft stabil | |
| IT-WDG-003 | Watchdog | 24h-Stabilitaet | System vollstaendig | Lasse 24h laufen | Kein Watchdog-Reset, kein Heap-Verlust >10KB | |
| IT-WDG-004 | Watchdog | DTC bei Watchdog-Reset | System nach Watchdog-Reset | Pruefe DTC-Log | DTC D0501 (ERROR) gesetzt, Reset-Ursache dokumentiert | |

---

## 4. Hardware-in-the-Loop (HIL)

HIL-Tests verwenden einen zweiten ESP32 (oder einen kleinen
Mikrocontroller) als Sensor-Simulator, der definierte Signale erzeugt und an
den Bordcomputer sendet. Dies ermoeglicht reproduzierbare Tests unter
kontrollierten Bedingungen, die mit echten Sensoren nicht moeglich waeren
(z.B. Ueberhitzung, Crash, Sensor-Ausfall).

### 4.1 HIL-Setup

```
+-------------------+        +-------------------+
|  Simulator-ESP32  |        |  Bordcomputer     |
|  (zweiter ESP32)  |        |  (ESP32-S3)       |
|                   |        |                   |
|  PWM-Ausgang  --->|------->|  RPM-GPIO         |
|  Poti/DAC      --->|------->|  ADC (Fuel/Temp)  |
|  NMEA-UART    --->|------->|  GPS-UART         |
|  I2C-Master   --->|------->|  I2C (IMU/Baro)   |
|  GPIO-Relais  <---|<-------|  Relais-Pins      |
|  BLE-Client   <---|<-------|  BLE (NimBLE)     |
|                   |        |                   |
|  Test-Steuerung   |        |  Self-Test, DTC   |
|  (Automatisierung)|        |  Diagnose         |
+-------------------+        +-------------------+
```

Der Simulator-ESP32 erzeugt:

- **RPM:** PWM-Signal mit definierter Frequenz am RPM-Pin (simuliert
  Zündimpulse von 800-12000 RPM)
- **Kuehlmitteltemperatur:** Widerstandsnetzwerk mit schaltbaren Widerstaenden
  oder DAC-Ausgang (simuliert 20C, 80C, 110C, 120C)
- **Kraftstoffstand:** Potentiometer oder DAC-Ausgang (simuliert 0-100%
  Fuellstand)
- **Bordspannung:** Labornetzteil mit einstellbarer Spannung (10-16V)
- **GPS:** Simulierte NMEA/UBX-Saetze ueber UART (definierte Positionen,
  Geschwindigkeiten, Sat-Anzahl). Test-Routen als GPX-Dateien abspielbar.
- **IMU:** Statdaten ueber I2C (simulierte Neigungswerte, Crash-Beschleunigung)
- **SD-Karte:** Echte SD-Karte (keine Simulation noetig)
- **BLE:** Echte BLE-Verbindung (Smartphone oder Simulator-ESP32 als Client)

### 4.2 Automatisierte Test-Sequenzen

Der Simulator-ESP32 fuehrt vordefinierte Test-Sequenzen aus, die als
JSON-Skripte beschrieben sind. Jede Sequenz besteht aus einer Liste von
Aktionen (Signal setzen, Warten, Signal aendern) und Erwartungen (welcher
DTC gesetzt werden soll, welcher Display-Wert zu sehen sein soll).

```json
{
  "name": "Ueberhitzung-Szenario",
  "description": "Kuehlmitteltemperatur steigt auf 120C",
  "actions": [
    {"step": 0, "set": "temp_c", "value": 85, "comment": "Normal"},
    {"step": 1, "wait_ms": 2000, "expect_dtc": null},
    {"step": 2, "set": "temp_c", "value": 110, "comment": "Warnung"},
    {"step": 3, "wait_ms": 2000, "expect_dtc": "S0102"},
    {"step": 4, "set": "temp_c", "value": 120, "comment": "Kritisch"},
    {"step": 5, "wait_ms": 2000, "expect_dtc": "S0103"},
    {"step": 6, "set": "temp_c", "value": 85, "comment": "Normalisierung"},
    {"step": 7, "wait_ms": 5000, "expect_dtc": null}
  ]
}
```

### 4.3 HIL-Test-Szenarien

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| HIL-001 | System | Normalfahrt | HIL-Setup aktiv | Simuliere 80 km/h, 4000 RPM, 85C, 13.8V fuer 60s | Dashboard zeigt alle Werte korrekt, keine DTCs | |
| HIL-002 | System | Ueberhitzung | HIL-Setup aktiv | Temp steigt auf 115C | Warnung, DTC S0103 (CRITICAL), Buzzer | |
| HIL-003 | System | Kraftstoffreserve | HIL-Setup aktiv | Fuel faellt auf 2L | Warnung, DTC S0202 (WARN) | |
| HIL-004 | System | Generator-Ausfall | HIL-Setup aktiv | Spannung faellt auf 11.5V | Warnung, DTC P0401 (WARN) | |
| HIL-005 | System | GPS-Verlust (Tunnel) | HIL-Setup aktiv, GPS-Fix vorhanden | GPS-Signal stoppen | DTC S0502 (WARN), Fallback auf letzte Werte | |
| HIL-006 | System | Crash-Simulation | HIL-Setup aktiv | IMU: 7g Spike + 75 Grad Neigung | Crash-Detection, Countdown 10s, SOS (wenn BLE verbunden) | |
| HIL-007 | System | Sensor-Ausfall (Temp) | HIL-Setup aktiv | Trenne Temp-Sensor (ADC=0) | DTC S0101 (ERROR), Display zeigt "---" | |
| HIL-008 | System | SD-Karte voll | HIL-Setup aktiv, SD <1KB frei | Schreibe weiter Telemetrie | DTC C0103 (ERROR), Logging stoppt sauber | |
| HIL-009 | System | Power-Loss | HIL-Setup aktiv, SD aktiv | Unterbreche Bordspannung | Letzte Daten auf SD gesichert, Datei geschlossen | |
| HIL-010 | System | OTA-Update | HIL-Setup aktiv, BLE verbunden | Sende Firmware ueber BLE | Update erfolgreich, Neustart, alte Version bei Crash | |
| HIL-011 | System | Fehlalarm Crash (Schlagloch) | HIL-Setup aktiv | IMU: 7g Spike, aber keine Neigung | Crash-Verdacht, aber keine Bestaetigung (Neigung fehlt), kein SOS | |
| HIL-012 | System | Fehlalarm Crash (Bremsen) | HIL-Setup aktiv | IMU: starke Verzoegerung (1g) | Kein Crash-Verdacht (unter Schwelle) | |
| HIL-013 | System | Langzeitstabilitaet (1h) | HIL-Setup aktiv | Simuliere 1h Fahrt | Keine Abstuerze, Heap stabil, keine Watchdog-Resets | |
| HIL-014 | System | Boot-Sequenz | HIL-Setup aktiv | Power-On, warte bis Self-Test | Self-Test abgeschlossen in <3s, alle Module erkannt | |
| HIL-015 | System | Dual-Display unter Last | HIL-Setup aktiv, beide Displays | Simuliere schnelle Werteaenderungen | TFT und E-Paper aktualisieren korrekt, keine SPI-Kollision | |

---

## 5. Performance-Benchmarks

Performance-Benchmarks messen die Systemleistung unter definierten Bedingungen
und vergleichen sie mit Zielwerten. Jede Metrik hat einen Zielwert, eine
Messmethode und eine Prioritaet (Must = muss eingehalten werden, Should =
sollte eingehalten werden, Could = nice-to-have). Ein Feature gilt erst als
fertig, wenn alle Must-Benchmarks innerhalb der Zielwerte liegen.

### 5.1 Benchmark-Zielwerte

| Metrik | Zielwert | Messmethode | Prioritaet |
|---|---|---|---|
| Display-Refresh-Rate (TFT, Partial) | >= 30 FPS | esp_lcd-FPS-Counter, LVGL-Statistik | Must |
| Display-Refresh-Rate (TFT, Full-Screen) | >= 15 FPS | esp_lcd-FPS-Counter | Should |
| Frame-Render-Time | < 33 ms pro Frame | esp_timer_get_time() um lv_timer_handler() | Must |
| Display-Refresh-Rate (E-Paper, Partial) | <= 0.3 s | Zeitmessung Init bis BUSY-Clear | Should |
| Display-Refresh-Rate (E-Paper, Full) | <= 2.0 s | Zeitmessung Init bis BUSY-Clear | Should |
| BLE-Latenz (Notify) | < 50 ms End-to-End | Timestamp App vs. Bordcomputer | Must |
| BLE-Connect-Zeit | < 5 s | Zeit von Advertising bis Connected | Should |
| SD-Schreibrate | > 100 KB/s sustained | FATFS-Benchmark, fwrite von 1MB | Must |
| SD-Schreibrate (Peak) | >= 300 KB/s | FATFS-Benchmark mit 4KB-Buffer | Should |
| GPS-Update-Rate | >= 5 Hz | Timestamp-Differenz der PVT-Pakete | Must |
| GPS-Update-Rate (High-Rate) | >= 10 Hz | UBX-NAV-PVT bei 10Hz-Konfiguration | Should |
| GPS-Fix-Time (Kaltstart) | < 30 s | Zeit von Power-On bis 3D-Fix | Must |
| GPS-Fix-Time (Warmstart) | < 5 s | Zeit von Power-On bis 3D-Fix (mit Ephemeriden) | Should |
| Boot-Zeit (bis Self-Test-Start) | < 3 s | Zeit von Power-On bis Self-Test startet | Must |
| Self-Test-Dauer | < 3 s | Zeitmessung Self-Test-Sequenz | Should |
| OTA-Update-Dauer | < 30 s (2MB Firmware) | Zeit von Start bis Reboot | Should |
| Crash-Detection-Latenz | < 500 ms | Zeit von Impact bis SOS-Countdown | Must |
| Freier Heap (nach Boot) | >= 50 KB | esp_get_free_heap_size() nach Boot | Must |
| Freier Heap (laufend, 24h) | >= 40 KB | esp_get_free_heap_size() alle 10s, Minimum | Must |
| Freier PSRAM (nach Boot) | >= 1.5 MB | heap_caps_get_free_size(MALLOC_CAP_SPIRAM) | Should |
| CPU-Load (pro Task) | < 80% pro Core | FreeRTOS-Task-Statistiken (vTaskGetRunTimeStats) | Should |
| Task-Watchdog | Kein Timeout in 24h | FreeRTOS-Task-Statistiken | Must |
| I2C-Bus-Auslastung | < 30% der Buszeit | Zeitmessung I2C-Transfers / Gesamtzeit | Should |
| SPI-Bus-Auslastung (Display) | < 50% der Buszeit | Zeitmessung SPI-Transfers / Gesamtzeit | Should |
| ADC-Leserate | >= 100 Hz pro Kanal | Zeitmessung adc_read() | Must |
| NVS-Schreibrate | < 1 Schreibzugriff pro Minute ( Durchschnitt) | Zaehler in NVS-Wrapper | Should |
| JSON-Config-Ladezeit | < 500 ms | Zeitmessung von fopen bis alle Werte geladen | Should |

### 5.2 Benchmark-Test-Cases

| Test-ID | Metrik | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| BM-001 | Display-FPS | TFT-Partial-Refresh-Rate | Display-Task laeuft, LVGL aktiv | Rendere 100 Frames, messe Zeit | FPS >= 30 | |
| BM-002 | Display-FPS | TFT-Full-Screen-Refresh | Display-Task laeuft | Rendere 10 Full-Screen-Updates | FPS >= 15 | |
| BM-003 | Render-Time | Frame-Render-Time | Display-Task laeuft | Messe Zeit pro lv_timer_handler()-Aufruf | < 33 ms | |
| BM-004 | E-Paper | Partial-Refresh-Zeit | E-Paper konfiguriert | Loese Partial-Refresh aus, messe Zeit | <= 0.3 s | |
| BM-005 | E-Paper | Full-Refresh-Zeit | E-Paper konfiguriert | Loese Full-Refresh aus, messe Zeit | <= 2.0 s | |
| BM-006 | BLE | Notify-Latenz | App verbunden | Sende Notify, messe Zeit bis Empfang | < 50 ms | |
| BM-007 | BLE | Connect-Zeit | BLE advertising | Verbinde App, messe Zeit | < 5 s | |
| BM-008 | SD | Schreibrate sustained | SD gemountet | Schreibe 1MB in 4KB-Bloecken | > 100 KB/s | |
| BM-009 | SD | Schreibrate peak | SD gemountet | Schreibe 4KB in einem fwrite | >= 300 KB/s (Peak) | |
| BM-010 | GPS | Update-Rate | GPS-Modul konfiguriert | Messe Zeit zwischen 10 PVT-Paketen | >= 5 Hz (Intervall <= 200ms) | |
| BM-011 | GPS | Fix-Time Kaltstart | GPS-Modul kalt (keine Ephemeriden) | Power-On, warte bis 3D-Fix | < 30 s | |
| BM-012 | GPS | Fix-Time Warmstart | GPS-Modul warm (Ephemeriden vorhanden) | Power-On, warte bis 3D-Fix | < 5 s | |
| BM-013 | Boot | Boot-Zeit bis Self-Test | System aus | Power-On, messe bis Self-Test startet | < 3 s | |
| BM-014 | Self-Test | Self-Test-Dauer | System gebootet | Starte Self-Test, messe Dauer | < 3 s | |
| BM-015 | OTA | Update-Dauer | BLE verbunden, Firmware bereit | Sende 2MB Firmware, messe Zeit | < 30 s | |
| BM-016 | Crash | Crash-Detection-Latenz | Crash-Task laeuft | Simuliere Crash, messe bis Countdown | < 500 ms | |
| BM-017 | Heap | Freier Heap nach Boot | System frisch gebootet | Lese esp_get_free_heap_size() | >= 50 KB | |
| BM-018 | Heap | Freier Heap nach 24h | System laeuft 24h | Lese esp_get_free_heap_size() | >= 40 KB | |
| BM-019 | PSRAM | Freier PSRAM nach Boot | System frisch gebootet | Lese heap_caps_get_free_size(SPIRAM) | >= 1.5 MB | |
| BM-020 | CPU | CPU-Load pro Core | System vollstaendig | Lese vTaskGetRunTimeStats() | < 80% pro Core | |
| BM-021 | Watchdog | 24h-Stabilitaet | System vollstaendig | Lasse 24h laufen | Kein Watchdog-Reset | |
| BM-022 | I2C | I2C-Bus-Auslastung | I2C aktiv (IMU + Baro) | Messe Transferzeit / Gesamtzeit | < 30% | |
| BM-023 | SPI | SPI-Bus-Auslastung (Display) | Display aktiv | Messe Transferzeit / Gesamtzeit | < 50% | |
| BM-024 | ADC | ADC-Leserate | ADC konfiguriert | Lese 1000x, messe Zeit | >= 100 Hz pro Kanal | |
| BM-025 | NVS | NVS-Schreibfrequenz | System laeuft 1h | Zaehle NVS-Schreibzugriffe | < 60 in 1h | |
| BM-026 | JSON | Config-Ladezeit | config.json auf SD | Messe Zeit von fopen bis geladen | < 500 ms | |

---

## 6. Companion-App Tests

Die Companion-App (Flutter/Dart) hat ihre eigene Test-Suite, die auf Jest-aehnlichen
Frameworks basiert. Die Tests decken BLE-Kommunikation, UI-Rendering und
Datenvalidierung ab. Sie laufen auf dem Host-PC (nicht auf dem Smartphone) und
verwenden Mock-BLE-Verbindungen.

### 6.1 BLE-Verbindungs-Tests

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| AT-BLE-001 | BLE | Connect | App gestartet, Bordcomputer advertising | Verbinde via BLE | Verbindung erfolgreich innerhalb 5s | |
| AT-BLE-002 | BLE | Disconnect | Verbunden | Trenne Verbindung | App zeigt "Getrennt", Auto-Reconnect startet | |
| AT-BLE-003 | BLE | Reconnect (Auto) | Getrennt | Warte, bis Bordcomputer wieder erreichbar | App verbindet automatisch innerhalb 10s | |
| AT-BLE-004 | BLE | MTU-Aushandlung | Verbunden | Fordere MTU=247 an | MTU=247 akzeptiert, oder Fallback auf 23 | |
| AT-BLE-005 | BLE | Telemetry empfangen | Verbunden | Warte auf Telemetry-Notify | App empfängt 1 Hz Updates, alle Felder korrekt | |
| AT-BLE-006 | BLE | DTC-Liste empfangen | Verbunden, DTCs aktiv | Lese DTC-List-Characteristic | Alle DTCs korrekt deserialisiert | |
| AT-BLE-007 | BLE | Navigation empfangen | Verbunden | App sendet Maneuver via Write | Bordcomputer bestaetigt Empfang, Display zeigt Pfeil | |
| AT-BLE-008 | BLE | OTA-Update starten | Verbunden, Firmware bereit | Starte OTA via Write | Bordcomputer beginnt Empfang, Fortschritt wird angezeigt | |
| AT-BLE-009 | BLE | Verbindung verloren waehrend OTA | OTA laeuft (50%) | Trenne Verbindung | App zeigt Fehler, OTA kann neu gestartet werden | |
| AT-BLE-010 | BLE | Diagnose-Report abrufen | Verbunden | Lese Diagnose-Report-Characteristic | JSON-Report korrekt geparst und angezeigt | |

### 6.2 UI-Render-Performance

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| AT-UI-001 | UI | Dashboard-Render | App geoeffnet, Mock-Daten | Rendere Dashboard 100x | Alle Renders < 16ms (60 FPS) | |
| AT-UI-002 | UI | Screen-Wechsel | App geoeffnet | Wechsle 10x zwischen Screens | Jeder Wechsel < 300ms, Animation fluessig | |
| AT-UI-003 | UI | Telemetry-Update-Render | App geoeffnet, 1Hz Mock-Daten | Rendere 100 Updates | Kein Frame-Drop, CPU-Last < 30% | |
| AT-UI-004 | UI | DTC-Warning-Overlay | App geoeffnet, Mock-DTC CRITICAL | Zeige Warning-Overlay | Overlay erscheint < 200ms nach DTC-Empfang | |
| AT-UI-005 | UI | Listen-Scrolling (DTC-Liste) | 100 Mock-DTCs in Liste | Scrolle durch Liste | 60 FPS, kein Stottern | |

### 6.3 Mock-Daten-Validierung

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| AT-MOCK-001 | Mock | Mock-Daten plausibel | Mock-Telemetry aktiv | Pruefe alle Werte gegen Plausibilitaetsbereich | Alle Werte im erwarteten Bereich | |
| AT-MOCK-002 | Mock | Mock-DTC-Liste | Mock-DTCs (5 verschiedene) | Validiere Deserialisierung | Alle 5 DTCs korrekt deserialisiert | |
| AT-MOCK-003 | Mock | Mock-GPX-Track | Mock-Track mit 1000 Punkten | Lade und rendere Track | Track korrekt dargestellt, keine Abstuerze | |
| AT-MOCK-004 | Mock | Mock-Config | Mock-Config mit 25 Schluesseln | Lade und wende an | Alle Werte korrekt, UI aktualisiert | |

---

## 7. Test-Framework-Empfehlung

### 7.1 Firmware: Unity + ESP-IDF Test

**Unity** ist ein kompaktes C-Test-Framework, das sowohl auf dem Host-PC (mit
GCC) als auch auf dem ESP32 (mit ESP-IDF) laeuft. Es bietet Assertions,
Test-Gruppierung und automatisierte Test-Runner. Unity ist der De-facto-Standard
fuer Embedded-C-Testing und ist in ESP-IDF integriert (`idf.py create-test`,
`idf.py test`).

**Vorteile:**
- Sehr kompakt (eine Header-Datei, keine Abhaengigkeiten)
- Laeuft nativ auf PC (GCC) fuer schnelle Unit-Tests
- Laeuft auf ESP32 (ESP-IDF) fuer Integration-Tests auf echter Hardware
- CI-tauglich: Test-Output als JUnit-XML fuer GitHub Actions
- Grosse Community, gute Dokumentation

**Nachteile:**
- Keine Mock-Generierung (Mocks muessen manuell geschrieben werden)
- Kein Parameterized-Testing out-of-the-box (ueber Macros nachbaubar)

**Empfehlung:** Unity fuer Unit-Tests (Host) und Integration-Tests (Target).
Mock-Schicht manuell implementieren (siehe Abschnitt 1.3).

### 7.2 Companion-App: Flutter Test + Integration Test

Die Companion-App ist in Flutter geschrieben. Flutter bringt ein eigenes
Test-Framework mit (`flutter test` fuer Widget-Tests, `flutter integration_test`
fuer Integration-Tests auf dem Geraet). Diese sind ausreichend und erfordern
keine zusaetzlichen Abhaengigkeiten.

**Vorteile:**
- Eingebaut in Flutter, keine zusaetzlichen Abhaengigkeiten
- Widget-Tests laufen auf dem Host (schnell, kein Geraet noetig)
- Integration-Tests auf dem Smartphone oder Emulator
- Goldens-Testing fuer UI-Regression

### 7.3 CI/CD: GitHub Actions

Die CI-Pipeline laeuft auf GitHub Actions und wird bei jedem Push und
Pull-Request ausgeloest. Sie besteht aus folgenden Jobs:

| Job | Trigger | Dauer (geschaetzt) | Beschreibung |
|---|---|---|---|
| lint | Jeder Push/PR | ~30s | clang-format Check, cppcheck statische Analyse |
| unit-tests | Jeder Push/PR | ~1min | Unity-Tests auf PC (GCC), JUnit-XML-Output |
| build | Jeder Push/PR | ~3min | ESP-IDF-Kompilierung fuer ESP32-S3 (ohne Flash) |
| size-check | Jeder Push/PR | ~10s | Flash- und RAM-Verbrauch muss unter Limits liegen |
| integration-tests | PR (nicht Push) | ~5min | Integration-Tests auf ESP32 (oder QEMU, falls verfuegbar) |
| app-tests | Jeder Push/PR | ~2min | Flutter Widget-Tests auf Host |
| docs | Jeder Push/PR | ~30s | Doxygen-Generierung, README-Check |

Ein Merge in den `main`-Branch erfordert eine erfolgreiche Pipeline-Ausfuehrung
aller Must-Jobs (lint, unit-tests, build, size-check).

**Workflow-Datei** (`.github/workflows/ci.yml`):

```yaml
name: CI
on: [push, pull_request]
jobs:
  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: clang-format check
        run: find src test -name '*.c' -o -name '*.h' | xargs clang-format --dry-run --Werror
      - name: cppcheck
        run: cppcheck --enable=all --error-exitcode=1 src/

  unit-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install Unity
        run: git clone https://github.com/ThrowTheSwitch/Unity.git /tmp/unity
      - name: Build and run unit tests
        run: |
          gcc -I/src -I/test/mocks -I/tmp/unity/src test/unit/*.c test/mocks/*.c /tmp/unity/src/unity.c -o test_runner -lm
          ./test_runner -j test_results.xml
      - uses: actions/upload-artifact@v4
        with:
          name: test-results
          path: test_results.xml

  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: ESP-IDF Setup
        uses: espressif/esp-idf-ci-action@v1
        with:
          esp_idf_version: v5.4.4
      - name: Build
        run: idf.py build
      - name: Size check
        run: |
          python tools/check_size.py build/bordcomputer.bin --max-flash 4194304 --max-ram 327680

  app-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: subosito/flutter-action@v2
        with:
          flutter-version: '3.24.0'
      - name: Run tests
        run: cd app && flutter test
```

### 7.4 Coverage-Targets

| Modul-Kategorie | Coverage-Ziel | Prioritaet |
|---|---|---|
| Core-Treiber (RPM, Temp, Fuel, GPS, Gear) | > 80% | Must |
| Filter-Algorithmen (Kalman, Complementary, Moving-Avg) | > 90% | Must |
| DTC-System (Set/Clear/Persistenz) | > 85% | Must |
| Konfigurations-Parser (JSON) | > 80% | Must |
| Sensor-Interface (init/read/config) | > 70% | Should |
| BLE-Stack (GATT, Notify, OTA) | > 60% | Should |
| Display-Rendering (LVGL) | > 50% | Could |
| Companion-App (Flutter) | > 60% | Should |
| Integration-Tests (Task-Pipeline) | > 50% der Szenarien | Should |

Coverage wird mit `gcov` (GCC) fuer C-Code und `flutter test --coverage` fuer
Dart gemessen. Die CI-Pipeline generiert einen Coverage-Report und vergleicht
ihn mit den Zielen. Ein Pull-Request, der die Coverage unter das Ziel senkt,
wird automatisch blockiert.

---

## 8. Test-Plan als Tabelle

Der vollstaendige Test-Plan fasst alle Test-Cases aus den Abschnitten 2-6 in
einer Tabelle zusammen. Die Status-Spalte ist im Lieferzustand leer und wird
waehrend der Implementierung ausgefuellt: "PASS" = Test bestanden, "FAIL" =
Test fehlgeschlagen, "SKIP" = Test uebersprungen (noch nicht implementiert),
"N/A" = Test nicht anwendbar.

Legende fuer Test-ID-Praefixe:
- UT = Unit Test (Host, GCC)
- IT = Integration Test (ESP32-S3)
- ST = System Test (ESP32-S3, vollstaendig)
- HIL = Hardware-in-the-Loop (ESP32-S3 + Simulator)
- BM = Benchmark (ESP32-S3)
- AT = App Test (Flutter)

### 8.1 Firmware Unit-Tests (Abschnitt 2)

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| UT-RPM-001 | RPM | Null-Pulse (Motor aus) | PCNT=0, T=1s | Berechne RPM | RPM=0, kein DTC | |
| UT-RPM-002 | RPM | Normaldrehzahl (3000) | PCNT=50, T=1s | Berechne RPM | RPM=3000 | |
| UT-RPM-003 | RPM | Redline (8040) | PCNT=134, T=1s | Berechne RPM | RPM=8040 | |
| UT-RPM-004 | RPM | Ueber-Redline (12000) | PCNT=200, T=1s | Berechne+Pruefe | RPM=12000, DTC P0101 | |
| UT-RPM-005 | RPM | Counter-Overflow | PCNT=65535+10 | Berechne RPM | Overflow erkannt, korrekt | |
| UT-RPM-006 | RPM | Kurzes Zeitfenster (10ms) | PCNT=1, T=10ms | Berechne RPM | RPM=6000, Toleranz ok | |
| UT-RPM-007 | RPM | Moving-Average (5 Samples) | 5 Werte | Berechne Avg | Avg=3010, Jitter reduziert | |
| UT-RPM-008 | RPM | Signal-Verlust (>2s) | Kein Interrupt 3s | Pruefe Timeout | RPM=0, DTC P0102 | |
| UT-RPM-009 | RPM | Entprellung (<2000us) | 2 Interrupts <1ms | Pruefe Debounce | 2. Interrupt ignoriert | |
| UT-RPM-010 | RPM | 2-Impuls-pro-Umdr. | PCNT=100, Faktor=2 | Berechne RPM | RPM=3000 | |
| UT-TEMP-001 | Temp | NTC 25C | ADC=10k@25C | Berechne Temp | 25C+/-1C | |
| UT-TEMP-002 | Temp | NTC 85C | ADC=1.5k@85C | Berechne Temp | 85C+/-2C | |
| UT-TEMP-003 | Temp | NTC 110C (Warnung) | ADC=600 Ohm | Berechne+Pruefe | 110C, DTC S0102 | |
| UT-TEMP-004 | Temp | NTC 120C (Kritisch) | ADC=400 Ohm | Berechne+Pruefe | 120C, DTC S0103 | |
| UT-TEMP-005 | Temp | NTC Kurzschluss | ADC=4095 | Pruefe Plausibilitaet | DTC S0101 | |
| UT-TEMP-006 | Temp | NTC Unterbrechung | ADC=0 | Pruefe Plausibilitaet | DTC S0101 | |
| UT-TEMP-007 | Temp | Steinhart-Hart 5 Punkte | Bekannter NTC | Berechne bei 5 Werten | Alle +/-1C der Tabelle | |
| UT-TEMP-008 | Temp | DS18B20 25C | 1-Wire: 0x0190 | Wandle Rohwert | 25.0C | |
| UT-TEMP-009 | Temp | DS18B20 -10C | 1-Wire: negativer Rohwert | Wandle Rohwert | -10.0C | |
| UT-TEMP-010 | Temp | DS18B20 nicht gefunden | 1-Wire: kein Geraet | Pruefe | DTC S0101 | |
| UT-FUEL-001 | Fuel | Leertank (0%) | ADC=200mV | Interpoliere | 0%, DTC S0202 | |
| UT-FUEL-002 | Fuel | Volltank (100%) | ADC=2900mV | Interpoliere | 100% | |
| UT-FUEL-003 | Fuel | Halber Tank (50%) | ADC=Mitte | Interpoliere | 50%+/-5% | |
| UT-FUEL-004 | Fuel | Reserve (<10%) | ADC nahe EMPTY | Interpoliere+Pruefe | <10%, DTC S0202 | |
| UT-FUEL-005 | Fuel | Signal verloren (ADC=0) | ADC=0 | Pruefe | DTC S0201 | |
| UT-FUEL-006 | Fuel | Ausserhalb Bereich | ADC>2900mV | Pruefe | 100% begrenzt, DTC S0201 | |
| UT-FUEL-007 | Fuel | Nicht-lineare Kurve (4 Pts) | 4 Stuetzpunkte | Interpoliere | Werte korrekt | |
| UT-FUEL-008 | Fuel | Reichweitenberechnung | Verbrauch=4L/100km, Fuel=8L | Berechne Range | 200 km | |
| UT-GEAR-001 | Gear | Leerlauf | Speed=0, RPM=1200 | Berechne Gang | N | |
| UT-GEAR-002 | Gear | 1. Gang | 20/3000 | Berechne | 1 | |
| UT-GEAR-003 | Gear | 2. Gang | 40/3000 | Berechne | 2 | |
| UT-GEAR-004 | Gear | 3. Gang | 60/3000 | Berechne | 3 | |
| UT-GEAR-005 | Gear | 4. Gang | 80/3000 | Berechne | 4 | |
| UT-GEAR-006 | Gear | 5. Gang | 100/3000 | Berechne | 5 | |
| UT-GEAR-007 | Gear | Plausibilitaet (>6) | 200/1000 | Berechne+Pruefe | DTC P0301 | |
| UT-GEAR-008 | Gear | Schaltpunkt-Lernmodus | 5 bekannte Paare | Lerne+Speichere | Schaltpunkte ok | |
| UT-GEAR-009 | Gear | Kupplung gezogen | 60, RPM faellt | Berechne | Gang bleibt 3 | |
| UT-GEAR-010 | Gear | Anhalten | Speed->0, RPM->1200 | Berechne | N | |
| UT-GPS-001 | GPS | GGA gueltig | Fix, 9 Sats | Parse GGA | Lat/Lon/Sats/Alt korrekt | |
| UT-GPS-002 | GPS | GGA ohne Fix | Status=0 | Parse GGA | Keine Position | |
| UT-GPS-003 | GPS | RMC gueltig | Speed+Heading | Parse RMC | Speed+Heading korrekt | |
| UT-GPS-004 | GPS | RMC void | Status=V | Parse RMC | Keine Daten | |
| UT-GPS-005 | GPS | VTG | Speed km/h | Parse VTG | Speed korrekt | |
| UT-GPS-006 | GPS | Checksum-Fehler | Falsche Checksum | Parse+Pruefe | Satz verworfen | |
| UT-GPS-007 | GPS | Unvollstaendig | Abgeschnitten | Parse | Verworfen, kein Absturz | |
| UT-GPS-008 | GPS | Leerer Satz | Nur `$\r\n` | Parse | Verworfen, kein Absturz | |
| UT-GPS-009 | GPS | Unbekannter Typ | `$GPXYZ` | Parse | Ignoriert, kein Absturz | |
| UT-GPS-010 | GPS | Multi-Satz-Buffer | GGA+RMC+VTG | Parse alle | Alle korrekt geparst | |
| UT-GPS-011 | GPS | UBX-NAV-PVT | Binaerer Puffer | Parse UBX | Alle Felder korrekt | |
| UT-GPS-012 | GPS | UBX falsche Laenge | Fehlerhafte Laenge | Parse+Pruefe | Verworfen | |
| UT-GPS-013 | GPS | Modul antwortet nicht | Keine Daten >5s | Pruefe Timeout | DTC S0501 | |
| UT-GPS-014 | GPS | Fix verloren (Sats<4) | Sats 8->3 | Pruefe Fix | DTC S0502 | |
| UT-GPS-015 | GPS | Fix wiederhergestellt | Sats 3->8 | Pruefe Fix | DTC S0503, S0502 gecleart | |
| UT-DTC-001 | DTC | Setzen (neu) | Tabelle leer | dtc_set(S0102) | ACTIVE, count=1 | |
| UT-DTC-002 | DTC | Setzen (count++) | S0102 ACTIVE | dtc_set erneut | count=2 | |
| UT-DTC-003 | DTC | Loeschen (clear) | S0102 ACTIVE | dtc_clear | HISTORY | |
| UT-DTC-004 | DTC | Alle loeschen | 5 aktiv | dtc_clear_all | Alle HISTORY | |
| UT-DTC-005 | DTC | Auto-Clear nach 10 Zyklen | S0102 ACTIVE | 10x gueltig | HISTORY nach 10 Zyklen | |
| UT-DTC-006 | DTC | Auto-Clear abgebrochen | 9x gueltig, 1x ungueltig | Pruefe Zaehler | Zaehler reset, bleibt ACTIVE | |
| UT-DTC-007 | DTC | Severity-Prioritaet | P0402+S0102 | Sortiere | CRITICAL vor WARN | |
| UT-DTC-008 | DTC | NVS-Persistenz | 3 DTCs aktiv | Speichere+Lade | Alle 3 korrekt | |
| UT-DTC-009 | DTC | NVS voll (>32) | 32 DTCs | Setze 33. | Aeltester HISTORY verworfen | |
| UT-DTC-010 | DTC | SD-Log-Eintrag | DTC wird gesetzt | Pruefe dtc.log | Format korrekt | |
| UT-DTC-011 | DTC | JSON-Export | 2 DTCs aktiv | Exportiere | Gueltiges JSON | |
| UT-DTC-012 | DTC | Binary-Export (BLE) | 3 DTCs aktiv | Serialisiere | 36 Bytes, korrekt | |
| UT-KAL-001 | Kalman | Konstant 50 km/h | 20x 50 km/h | Filtere | Konvergiert zu 50 | |
| UT-KAL-002 | Kalman | Rauschen um 50 | 20x 48-52 (Gauss) | Filtere | ~50, Jitter<1 | |
| UT-KAL-003 | Kalman | Beschleunigung 0->100 | 20x steigend | Filtere | Folge, <200ms Verzoegerung | |
| UT-KAL-004 | Kalman | Bremsung 100->0 | 20x fallend | Filtere | Folge, kein Overshoot | |
| UT-KAL-005 | Kalman | Ausreisser (Spike) | 50,50,200,50,50 | Filtere | ~50, Spike unterdrueckt | |
| UT-KAL-006 | Kalman | Fix verloren | Stoppe nach 10 | Pruefe | Letzter Wert gehalten | |
| UT-COMP-001 | Comp-Filter | Stillstand 0 Grad | ax=0,az=1g,g=0 | Filtere 100x | Roll=0, Pitch=0 | |
| UT-COMP-002 | Comp-Filter | Neigung 30 Grad | ax=sin30,az=cos30 | Filtere 100x | Roll=30+/-1 | |
| UT-COMP-003 | Comp-Filter | Neigung -30 Grad | ax=-sin30 | Filtere 100x | Roll=-30+/-1 | |
| UT-COMP-004 | Comp-Filter | Pitch 20 Grad | ay=sin20 | Filtere 100x | Pitch=20+/-1 | |
| UT-COMP-005 | Comp-Filter | Gyro-Drift | Gyro=0.1rad/s, Accel=0 | Filtere 1000x | Accel korrigiert Drift | |
| UT-COMP-006 | Comp-Filter | Schnelle Aenderung (Wobble) | 10Hz sinus | Filtere 100x | Folge Gyro | |
| UT-COMP-007 | Comp-Filter | IMU unplausibel (>5g) | az=5.5g | Pruefe Plausibilitaet | DTC S0402 | |
| UT-COMP-008 | Comp-Filter | IMU kalibriert | 1000 Samples Stillstand | Kalibriere+Filtere | Naeher an 0 | |
| UT-CFG-001 | Config | Gueltiges JSON | 25 Schluessel | Parse | Alle korrekt | |
| UT-CFG-002 | Config | Ungueltiges JSON | Syntax-Fehler | Parse | Defaults, Fehler geloggt | |
| UT-CFG-003 | Config | Fehlender Schluessel | brightness fehlt | Parse+Lese | Default (80) | |
| UT-CFG-004 | Config | Typ-Konvertierung | String->int | Konvertiere | Korrekt oder Fehler | |
| UT-CFG-005 | Config | NVS Speichern+Laden | Config geaendert | Speichere+Lade | Alle korrekt | |
| UT-HAV-001 | Haversine | Jena->Weimar (~20km) | 2 GPS-Positionen | Berechne | 20.0+/-0.1 km | |
| UT-HAV-002 | Haversine | Null-Distanz | Gleiche Position | Berechne | 0 | |
| UT-HAV-003 | Haversine | Antipodale Punkte | Position+Gegenpunkt | Berechne | ~20015 km | |
| UT-HAV-004 | Haversine | Pol-Naehe | 89 Grad Breite | Berechne | Korrekt, keine Div0 | |
| UT-TZ-001 | Timezone | CET->UTC (Winter) | 14:23 CET | Konvertiere | 13:23 UTC | |
| UT-TZ-002 | Timezone | CEST->UTC (Sommer) | 14:23 CEST | Konvertiere | 12:23 UTC | |
| UT-TZ-003 | Timezone | DST-Uebergang | 31.10. 02:30 CEST | Pruefe DST | DST endet | |
| UT-TZ-004 | Timezone | Schaltjahr | 29.02.2028 | Pruefe | Gueltig | |
| UT-MA-001 | Moving-Avg | Leerer Buffer | 0 Samples | Berechne | 0 oder Fehler | |
| UT-MA-002 | Moving-Avg | Voller Buffer (5) | 10,20,30,40,50 | Berechne | 30 | |
| UT-MA-003 | Moving-Avg | Rotation (6. Sample) | +60 | Berechne | 40 (aeltestes raus) | |
| UT-MA-004 | Moving-Avg | Overflow-Schutz | Nahe INT16_MAX | Berechne | Kein Overflow | |
| UT-MA-005 | Moving-Avg | Negative Werte | -10,-20,-30 | Berechne | -20 | |
| UT-MA-006 | Moving-Avg | Alle gleich | 5x 42 | Berechne | 42, Varianz=0 | |
| UT-LOG-001 | Log-Ring | Buffer leer | Initial | Lese | Keine Daten | |
| UT-LOG-002 | Log-Ring | Teilweise gefuellt | 3 Eintraege | Lese | 3 Eintraege | |
| UT-LOG-003 | Log-Ring | Voll (Wrap-Around) | >8KB | Lese | Aelteste ueberschrieben | |
| UT-LOG-004 | Log-Ring | Mutex-Schutz | 2 Threads schreiben | Schreibe parallel | Keine Korruption | |
| UT-MAINT-001 | Maintenance | km-Intervall (90%) | ODO=5400, Int=6000 | Berechne | 90%, Warnung | |
| UT-MAINT-002 | Maintenance | Zeit-Intervall (90%) | 328T, Int=365T | Berechne | 90%, Warnung | |
| UT-MAINT-003 | Maintenance | Kombiniert | 5400km/328T | Berechne | 90% (mindestens) | |
| UT-MAINT-004 | Maintenance | Reset | Bei 90% | Reset | 0% | |
| UT-MAINT-005 | Maintenance | Betriebsstunden | 500h Betrieb | Pruefe | Warnung | |
| UT-CAL-001 | Calibration | 2-Punkt (Fuel) | EMPTY=200, FULL=2900 | Interpoliere@1500 | 50% | |
| UT-CAL-002 | Calibration | 8-Punkt | 8 Stuetzpunkte | Interpoliere | Korrekt, CRC ok | |
| UT-CAL-003 | Calibration | CRC-Pruefung | Daten geaendert | Pruefe CRC | Fehler, DTC D0601 | |

### 8.2 Integration-Tests (Abschnitt 3)

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| IT-TASK-001 | Tasks | GPS->Logger->SD | GPS+Logger aktiv | 10s warten | CSV mit GPS-Werten | |
| IT-TASK-002 | Tasks | GPS->Display | GPS+Display aktiv | GPS-Speed senden | Display zeigt Speed, 1Hz | |
| IT-TASK-003 | Tasks | RPM->Display | RPM+Display aktiv | Pulse simulieren | RPM-Anzeige, <200ms Latenz | |
| IT-TASK-004 | Tasks | BLE->App | BLE+App verbunden | Telemetry senden | App erhaelt 1Hz | |
| IT-TASK-005 | Tasks | Crash->SOS | Crash-Task+BLE | Crash simulieren | Countdown, SOS nach 10s | |
| IT-TASK-006 | Tasks | Power-Loss->Flush | Logger+SD | Power-Loss | Daten gesichert | |
| IT-TASK-007 | Tasks | Sensor->DTC | Sensor+DTC-Engine | Sensor-Ausfall | DTC gesetzt, Display+Buzzer | |
| IT-TASK-008 | Tasks | Self-Test Boot | System frisch | Warten | Report korrekt | |
| IT-TASK-009 | Tasks | OTA (BLE) | BLE+OTA-Partition | Firmware senden | Update+Neustart | |
| IT-TASK-010 | Tasks | Dual-Display | TFT+E-Paper | Beide aktualisieren | Keine Kollision | |
| IT-SPI-001 | SPI | TFT+SD gleichzeitig | SPI2+SPI3 | Parallel schreiben | Beide ok | |
| IT-SPI-002 | SPI | SD+E-Paper gleichzeitig | SPI3 geteilt | Parallel schreiben | Mutex, sequentiell ok | |
| IT-SPI-003 | SPI | TFT-Timeout | CS=HIGH | Init versuchen | Timeout, DTC B0101 | |
| IT-SPI-004 | SPI | SD-Timeout | SD entfernt | Mount versuchen | DTC C0101, RAM-Fallback | |
| IT-SPI-005 | SPI | Taktrate-Konflikt | 40MHz+10MHz | Wechseln | Korrekt umgeschaltet | |
| IT-BLE-001 | BLE | Single-Char 1Hz | App+MTU=247 | 100 Notifys | Alle empfangen, <100ms | |
| IT-BLE-002 | BLE | Multi-Char | App verbunden | Telemetry+DTC | Beide empfangen | |
| IT-BLE-003 | BLE | 5-Char-Stress | App+5 Notifies | Alle senden | Alle ok, kein Abbruch | |
| IT-BLE-004 | BLE | MTU=247 | App fordert MTU | Aushandeln | 247 akzeptiert | |
| IT-BLE-005 | BLE | MTU=23 Fallback | App nur MTU=23 | Telemetry senden | Chunking, funktionsfaehig | |
| IT-BLE-006 | BLE | Log-Export 50KB | App verbunden | Streamen | Alle Chunks, rekonstruierbar | |
| IT-BLE-007 | BLE | Abbruch waehrend Notify | App+Notify | Trennen | Stabil, DTC C0202, Reconnect | |
| IT-BLE-008 | BLE | Advertising Start/Stop | BLE init | 10x Start/Stop | Kein Fehler | |
| IT-SD-001 | SD | Schreiben unter Last | SD+alle Tasks | 1MB schreiben | >100 KB/s, kein Verlust | |
| IT-SD-002 | SD | Log-Rotation | system.log=1MB | Rotation | Umbenannt, neu, alte geloescht | |
| IT-SD-003 | SD | SD voll | <1KB frei | Weiter schreiben | DTC C0103, sauber Stopp | |
| IT-SD-004 | SD | Schreibfehler (CRC) | SD defekt | Schreiben | DTC C0104, Retry | |
| IT-SD-005 | SD | Power-Loss waehrend Write | SD aktiv | Spannung weg | Daten konsistent | |
| IT-WDG-001 | Watchdog | Timeout->Reset | WD aktiv | Task blockieren | Reset | |
| IT-WDG-002 | Watchdog | Normal (kein Reset) | WD aktiv | Alle feeden | Stabil | |
| IT-WDG-003 | Watchdog | 24h-Stabilitaet | Vollstaendig | 24h laufen | Kein Reset, Heap stabil | |
| IT-WDG-004 | Watchdog | DTC nach Reset | Nach WD-Reset | DTC-Log pruefen | DTC D0501 gesetzt | |

### 8.3 System-Tests (Abschnitt 3, erweitert)

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| ST-BOOT-001 | System | Boot-Sequenz komplett | System aus | Power-On, warte | Self-Test <3s, alle Module erkannt, Display aktiv | |
| ST-BOOT-002 | System | Boot ohne SD-Karte | SD-Karte entfernt | Power-On | System bootet, DTC C0101, RAM-Fallback | |
| ST-BOOT-003 | System | Boot ohne IMU | IMU getrennt | Power-On | System bootet, DTC S0401, Neigung inaktiv | |
| ST-BOOT-004 | System | Boot ohne GPS | GPS getrennt | Power-On | System bootet, DTC S0501, Speed=0 | |
| ST-NAV-001 | System | Navigation E2E | BLE+App verbunden | App sendet Route | Display zeigt Pfeil+Distanz+Strasse | |
| ST-NAV-002 | System | Back-Track | Tour aufgezeichnet | Aktiviere Back-Track | Route wird zurueck navigiert | |
| ST-DIAG-001 | System | Diagnose-Konsole (seriell) | USB-C verbunden | Sende Befehle | Alle Befehle korrekt beantwortet | |
| ST-DIAG-002 | System | Diagnose ueber BLE-App | App verbunden | Oeffne Diagnose-Tab | DTCs, Self-Test, Config ueber App verfuegbar | |
| ST-DIAG-003 | System | DTC-Report exportieren | App verbunden | Exportiere Report | JSON-Report korrekt, alle Felder | |
| ST-PWR-001 | System | Power-Loss E2E | System laeuft | Unterbreche Spannung | SD-Flush, Datei geschlossen, Neustart ok | |
| ST-PWR-002 | System | Brownout | Spannung schwankt | Loese Brownout aus | DTC D0502, System stabil | |
| ST-DEG-001 | System | Degraded Mode (GPS weg) | System laeuft | GPS abtrennen | Fallback, letzte Werte, DTC S0502 | |
| ST-DEG-002 | System | Degraded Mode (Display weg) | System laeuft | TFT abtrennen | E-Paper uebernimmt, DTC B0101, BLE-App | |
| ST-DEG-003 | System | Degraded Mode (SD weg) | System laeuft | SD abtrennen | NVS-Fallback, DTC C0101, BLE-Logging | |

### 8.4 HIL-Tests (Abschnitt 4)

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| HIL-001 | HIL | Normalfahrt | HIL aktiv | 80km/h, 4000RPM, 85C, 13.8V, 60s | Dashboard korrekt, keine DTCs | |
| HIL-002 | HIL | Ueberhitzung | HIL aktiv | Temp->115C | Warnung, DTC S0103, Buzzer | |
| HIL-003 | HIL | Kraftstoffreserve | HIL aktiv | Fuel->2L | DTC S0202 | |
| HIL-004 | HIL | Generator-Ausfall | HIL aktiv | Spannung->11.5V | DTC P0401 | |
| HIL-005 | HIL | GPS-Verlust (Tunnel) | HIL aktiv+Fix | GPS stoppen | DTC S0502, Fallback | |
| HIL-006 | HIL | Crash-Simulation | HIL aktiv | 7g+75 Grad | Crash, Countdown, SOS | |
| HIL-007 | HIL | Sensor-Ausfall (Temp) | HIL aktiv | Temp abtrennen | DTC S0101, "---" | |
| HIL-008 | HIL | SD-Karte voll | HIL+SD<1KB | Weiter schreiben | DTC C0103, sauber Stopp | |
| HIL-009 | HIL | Power-Loss | HIL aktiv | Spannung weg | Daten gesichert | |
| HIL-010 | HIL | OTA-Update | HIL+BLE | Firmware senden | Update+Neustart | |
| HIL-011 | HIL | Fehlalarm (Schlagloch) | HIL aktiv | 7g, keine Neigung | Verdacht, kein SOS | |
| HIL-012 | HIL | Fehlalarm (Bremsen) | HIL aktiv | 1g Verzoegerung | Kein Crash-Verdacht | |
| HIL-013 | HIL | Langzeit (1h) | HIL aktiv | 1h Fahrt | Stabil, kein Absturz | |
| HIL-014 | HIL | Boot-Sequenz | HIL aktiv | Power-On | Self-Test <3s | |
| HIL-015 | HIL | Dual-Display unter Last | HIL+2 Displays | Schnelle Aenderungen | Beide korrekt | |

### 8.5 Benchmark-Tests (Abschnitt 5)

| Test-ID | Metrik | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| BM-001 | FPS | TFT-Partial-Refresh | Display aktiv | 100 Frames | >= 30 FPS | |
| BM-002 | FPS | TFT-Full-Screen | Display aktiv | 10 Full-Updates | >= 15 FPS | |
| BM-003 | Render | Frame-Render-Time | Display aktiv | Messe pro Frame | < 33 ms | |
| BM-004 | E-Paper | Partial-Refresh | E-Paper aktiv | 1 Refresh | <= 0.3 s | |
| BM-005 | E-Paper | Full-Refresh | E-Paper aktiv | 1 Refresh | <= 2.0 s | |
| BM-006 | BLE | Notify-Latenz | App verbunden | Senden+Messen | < 50 ms | |
| BM-007 | BLE | Connect-Zeit | Advertising | Verbinden+Messen | < 5 s | |
| BM-008 | SD | Schreibrate sustained | SD gemountet | 1MB schreiben | > 100 KB/s | |
| BM-009 | SD | Schreibrate peak | SD gemountet | 4KB schreiben | >= 300 KB/s | |
| BM-010 | GPS | Update-Rate | GPS konfiguriert | 10 PVT messen | >= 5 Hz | |
| BM-011 | GPS | Fix-Time Kaltstart | GPS kalt | Power-On bis Fix | < 30 s | |
| BM-012 | GPS | Fix-Time Warmstart | GPS warm | Power-On bis Fix | < 5 s | |
| BM-013 | Boot | Boot-Zeit | System aus | Power-On bis Self-Test | < 3 s | |
| BM-014 | Self-Test | Dauer | Gebootet | Self-Test starten | < 3 s | |
| BM-015 | OTA | Update-Dauer | BLE+Firmware | 2MB senden | < 30 s | |
| BM-016 | Crash | Detection-Latenz | Crash-Task aktiv | Crash simulieren | < 500 ms | |
| BM-017 | Heap | Frei nach Boot | Frisch gebootet | Heap lesen | >= 50 KB | |
| BM-018 | Heap | Frei nach 24h | 24h Lauf | Heap lesen | >= 40 KB | |
| BM-019 | PSRAM | Frei nach Boot | Frisch gebootet | PSRAM lesen | >= 1.5 MB | |
| BM-020 | CPU | Load pro Core | Vollstaendig | RunTimeStats | < 80% pro Core | |
| BM-021 | Watchdog | 24h-Stabilitaet | Vollstaendig | 24h laufen | Kein Reset | |
| BM-022 | I2C | Bus-Auslastung | I2C aktiv | Transferzeit/Gesamt | < 30% | |
| BM-023 | SPI | Display-Bus-Auslastung | Display aktiv | Transferzeit/Gesamt | < 50% | |
| BM-024 | ADC | Leserate | ADC konfiguriert | 1000x lesen | >= 100 Hz | |
| BM-025 | NVS | Schreibfrequenz | 1h Lauf | Zaehlen | < 60/h | |
| BM-026 | JSON | Config-Ladezeit | config.json | fopen bis geladen | < 500 ms | |

### 8.6 Companion-App Tests (Abschnitt 6)

| Test-ID | Komponente | Test-Name | Vorbedingung | Schritte | Erwartetes Ergebnis | Status |
|---|---|---|---|---|---|---|
| AT-BLE-001 | BLE | Connect | App+BC advertising | Verbinden | Erfolg <5s | |
| AT-BLE-002 | BLE | Disconnect | Verbunden | Trennen | "Getrennt", Reconnect startet | |
| AT-BLE-003 | BLE | Reconnect (Auto) | Getrennt | Warten | Auto-Reconnect <10s | |
| AT-BLE-004 | BLE | MTU-Aushandlung | Verbunden | MTU=247 fordern | 247 oder Fallback 23 | |
| AT-BLE-005 | BLE | Telemetry empfangen | Verbunden | Warten auf Notify | 1Hz, alle Felder | |
| AT-BLE-006 | BLE | DTC-Liste empfangen | Verbunden | DTC-List lesen | Alle DTCs korrekt | |
| AT-BLE-007 | BLE | Navigation senden | Verbunden | Maneuver Write | BC bestaetigt, Display=Pfeil | |
| AT-BLE-008 | BLE | OTA starten | Verbunden+Firmware | OTA Write | Empfang startet, Fortschritt | |
| AT-BLE-009 | BLE | Abbruch waehrend OTA | OTA 50% | Trennen | Fehler, neu startbar | |
| AT-BLE-010 | BLE | Diagnose-Report | Verbunden | Report lesen | JSON korrekt, angezeigt | |
| AT-UI-001 | UI | Dashboard-Render | App+Mock | 100x rendern | <16ms pro Frame | |
| AT-UI-002 | UI | Screen-Wechsel | App geoeffnet | 10x wechseln | <300ms, fluessig | |
| AT-UI-003 | UI | Telemetry-Update | App+1Hz Mock | 100 Updates | Kein Frame-Drop, <30% CPU | |
| AT-UI-004 | UI | DTC-Warning-Overlay | App+Mock-DTC | Overlay zeigen | <200ms nach DTC | |
| AT-UI-005 | UI | Listen-Scrolling | 100 Mock-DTCs | Scrollen | 60 FPS, kein Stottern | |
| AT-MOCK-001 | Mock | Daten plausibel | Mock-Telemetry | Pruefe Werte | Alle im Bereich | |
| AT-MOCK-002 | Mock | DTC-Liste | 5 Mock-DTCs | Deserialisieren | Alle 5 korrekt | |
| AT-MOCK-003 | Mock | GPX-Track | 1000 Punkte | Laden+Rendern | Korrekt, kein Absturz | |
| AT-MOCK-004 | Mock | Config | 25 Schluessel | Laden+Anwenden | Alle korrekt | |

---

## 9. Test-Automatisierung und CI/CD-Integration

### 9.1 Automatisierungspyramide

Die Test-Automatisierung folgt derselben Pyramide wie die Tests selbst:

1. **Unit-Tests** laufen vollautomatisch bei jedem Commit. Kein menschlicher
   Eingriff erforderlich. Ausfuehrung: <5s.
2. **Integration-Tests** laufen automatisch bei jedem Pull-Request auf einem
   ESP32-S3-Host (oder QEMU, falls verfuegbar). Ausfuehrung: ~5min.
3. **System-Tests** laufen automatisch nach Merge in den Main-Branch.
   Ausfuehrung: ~10min.
4. **HIL-Tests** sind halb-automatisch: Der Simulator-ESP32 wird manuell
   mit dem Bordcomputer verbunden, die Test-Sequenzen laufen dann
   automatisch. Ausfuehrung: ~30min.
5. **App-Tests** laufen vollautomatisch bei jedem Commit (Widget-Tests) und
   bei Pull-Requests (Integration-Tests auf Emulator).

### 9.2 GitHub Actions Pipeline

Die Pipeline besteht aus separaten Jobs, die parallel laufen, wo moeglich:

```
Push/PR
  |
  +---> lint (clang-format, cppcheck)
  +---> unit-tests (GCC, Unity)
  +---> app-tests (Flutter, Widget-Tests)
  |
  +---> build (ESP-IDF, ESP32-S3)
  |       |
  |       +---> size-check (Flash/RAM Limits)
  |
  +---> integration-tests (nur PR, ESP32-S3)
  |
  +---> docs (Doxygen)
```

Ein Merge in `main` erfordert: lint, unit-tests, build, size-check, app-tests.
Integration-Tests werden bei PRs empfohlen, aber nicht zwingend (Hardware-Verfuegbarkeit).

### 9.3 Test-Reporting

Alle Test-Ergebnisse werden als JUnit-XML exportiert und von GitHub Actions
als Test-Report angezeigt. Coverage wird als HTML-Report generiert und als
Artifact gespeichert. Bei Fehlschlagen eines Must-Jobs wird der Merge
automatisch blockiert.

| Report | Format | Speicherort | Aufbewahrung |
|---|---|---|---|
| Unit-Test-Ergebnisse | JUnit-XML | GitHub Actions Artifacts | 30 Tage |
| Integration-Test-Ergebnisse | JUnit-XML | GitHub Actions Artifacts | 30 Tage |
| Coverage-Report (C) | HTML (gcov/genhtml) | GitHub Actions Artifacts | 30 Tage |
| Coverage-Report (Dart) | lcov | GitHub Actions Artifacts | 30 Tage |
| Build-Size-Report | JSON (Größenanalyse) | GitHub Actions Artifacts | 90 Tage |
| Performance-Benchmark-Ergebnisse | JSON (Zeitreihe) | Repository (docs/benchmarks/) | Dauerhaft |

### 9.4 Benchmark-Tracking

Performance-Benchmarks werden nicht nur einmalig gemessen, sondern als
Zeitreihe getrackt. Nach jedem Merge in `main` wird ein Benchmark-Lauf
durchgefuehrt, und die Ergebnisse werden in einer JSON-Datei gespeichert.
Trends (Verschlechterung der Performance) werden so frueh erkannt.

```json
{
  "date": "2026-10-13",
  "commit": "5be3d0b",
  "benchmarks": {
    "display_fps_partial": 33.2,
    "display_fps_full": 16.1,
    "render_time_ms": 30.1,
    "ble_notify_latency_ms": 42,
    "sd_write_rate_kbps": 145,
    "gps_update_rate_hz": 5.0,
    "boot_time_s": 2.8,
    "free_heap_kb": 52,
    "crash_detection_latency_ms": 380
  }
}
```

### 9.5 Test-Wartung

Tests werden mit dem Code gewartet. Wer eine neue Funktion hinzufuegt, muss
auch die zugehoerigen Tests hinzufuegen. Wer einen Bug fixt, muss einen
Test hinzufuegen, der den Bug reproduziert (Regression-Test). Die
Code-Review-Checkliste (siehe Dokument 09, Abschnitt 4.3) enthaelt den Punkt
"Tests hinzugefuegt?".

| Wartungs-Aufgabe | Trigger | Verantwortlich |
|---|---|---|
| Neue Tests fuer neue Features | Feature-Merge | Feature-Autor |
| Regression-Tests fuer Bug-Fixes | Bug-Fix-Merge | Bug-Fix-Autor |
| Test-Update bei API-Aenderungen | API-Breaking-Change | API-Autor |
| Mock-Update bei Hardware-Aenderung | Hardware-Revision | Treiber-Autor |
| Benchmark-Review | Monatlich | QA-Verantwortlicher |
| HIL-Setup-Verifikation | Quartal | QA-Verantwortlicher |

---

## 10. Test-Metriken und KPIs

Um die Qualitaet des Test-Prozesses selbst zu messen, werden folgende KPIs
getrackt:

| KPI | Zielwert | Messintervall | Beschreibung |
|---|---|---|---|
| Unit-Test-Coverage (Core) | > 80% | Pro PR | Abdeckung der Core-Treiber |
| Unit-Test-Pass-Rate | 100% | Pro PR | Alle Unit-Tests muessen bestehen |
| Integration-Test-Pass-Rate | > 95% | Pro PR | Toleranz fuer instabile Tests |
| Bug-Escape-Rate | < 10% | Monatlich | Bugs, die in Produktion gefunden wurden |
| Test-Ausfuehrungszeit (Unit) | < 5s | Pro Run | Geschwindigkeit der Unit-Tests |
| Test-Ausfuehrungszeit (CI) | < 15min | Pro PR | Gesamtzeit der CI-Pipeline |
| Mean-Time-to-Detect (MTTD) | < 1 Tag | Pro Bug | Zeit von Bug-Entstehung bis Entdeckung |
| Mean-Time-to-Fix (MTTF) | < 3 Tage | Pro Bug | Zeit von Entdeckung bis Fix |
| Flaky-Test-Rate | < 2% | Woechentlich | Tests, die nicht deterministisch sind |
| HIL-Test-Abdeckung | > 80% der Szenarien | Pro Release | Abgedeckte HIL-Szenarien |

---

## 11. Risiko- und Mitigations-Strategie fuer Tests

| Risiko | Wahrscheinlichkeit | Auswirkung | Mitigation |
|---|---|---|---|
| QEMU-Emulation unvollstaendig (BLE, SPI-DMA) | Hoch | Mittel | Integration-Tests auf echter Hardware laufen lassen, QEMU nur fuer Unit-aehnliche Integration-Tests |
| HIL-Setup instabil (Verkabelung, Kontakte) | Mittel | Hoch | Steckverbinder mit Rueckhaltekraft, HIL-Setup vor jedem Test verifizieren |
| Flaky-Tests durch Timing (FreeRTOS) | Mittel | Mittel | Zeitliche Toleranzen in Tests, Retry-Logik, Tests mehrfach laufen lassen |
| Mock-Abweichung von echter Hardware | Mittel | Hoch | Mocks gegen echte Hardware-Referenzwerte validieren, Integration-Tests als Sicherheitsnetz |
| Benchmark-Variabilitaet (Temperatur, Versorgung) | Niedrig | Niedrig | Benchmarks bei Raumtemperatur, stabile Versorgungsspannung, Mehrfachmessung |
| CI-Hardware-Verfuegbarkeit (ESP32-S3 Runner) | Mittel | Mittel | Fallback auf QEMU, Integration-Tests als Should nicht Must bei PRs |
| Test-Schuld (Tests werden nicht gepflegt) | Mittel | Hoch | Code-Review-Pflicht fuer Tests, Coverage-Gate in CI |

---

## 12. Quellen

- ESP-IDF Test-Framework: https://docs.espressif.com/projects/esp-idf/en/v5.4.4/esp32s3/api-guides/unit-tests.html
- Unity Test-Framework: https://github.com/ThrowTheSwitch/Unity
- Flutter Testing: https://docs.flutter.dev/testing
- GitHub Actions: https://docs.github.com/en/actions
- FreeRTOS Task-Statistics: https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/03-Run-time-stats
- LVGL Performance-Monitoring: https://docs.lvgl.io/master/overview/performance.html
- NimBLE Testing: https://mynewt.apache.org/latest/tutorials/ble/ble_svc_gap/ble_svc_gap.html
- cppcheck: https://cppcheck.sourceforge.io/
- gcov/genhtml: https://gcc.gnu.org/onlinedocs/gcc/Gcov.html
- FATFS-Benchmark: http://elm-chan.org/fsw/ff/00index_e.html

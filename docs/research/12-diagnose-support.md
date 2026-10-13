# Diagnose, Support & Wartbarkeit: Reparaturfreundliche Architektur

> Stand: 13.10.2026
> Fahrzeug: Transalp 600 (PD06), Baujahr 1986-2000, rein mechanisch, keine ECU
> Plattform: ESP32-S3-WROOM-1 (N8R2: 8MB Flash, 2MB PSRAM), ESP-IDF v5.4, FreeRTOS SMP
> Vorherige Dokumente: 01-sensoren, 02-hardware, 03-software-stack, 04-pd06-daten, 05-gehaeuse-emv, 06-feature-roadmap, 07-display-navigation

## 0. Einleitung

Ein Bordcomputer an einem rein mechanischen Motorrad ist ein Zusatzsystem, das
vom Fahrer oder einer Werkstatt nicht einfach "ausgelesen" werden kann wie ein
OBD-II-faehiges Fahrzeug. Die PD06 hat keine ECU, keinen CAN-Bus, keine
Standard-Diagnoseschnittstelle. Jede Sensordatenquelle ist eine eigene
Loetverbindung, jeder Fehlerzustand eine eigene Logik. Das bedeutet: Das
Diagnosesystem muss komplett selbst aufgebaut werden, vom Fehlercode-Format bis
zur seriellen Konsole.

Dieses Dokument definiert die vollstaendige Diagnose-, Support- und
Wartbarkeitsarchitektur des Bordcomputers. Es deckt ab:

1. Serielle Diagnose-Konsole (USB-C) mit Befehlsreferenz
2. BLE-Diagnose-Service (GATT-Characteristics fuer DTC, Self-Test, Config)
3. Eigenes DTC-Fehlercode-System (inspiriert von OBD-II, angepasst an dieses Projekt)
4. Strukturiertes Log-System mit Ring-Buffer, SD-Karten-Persistenz und Rotation
5. Hardware-Self-Test-Routine beim Boot und auf manuellen Trigger
6. Kalibrierungs-System fuer jeden Sensor
7. Modularitaet: Software erkennt fehlende Hardware und degradiert graceful
8. Remote-Support ueber die Companion-App
9. Wartungs-Intervalle und Betriebsstunden-Zaehler

Das uebergeordnete Ziel: Ein Techniker oder Reparateur kann jederzeit Fehler
auslesen, Software-Updates einspielen, Hardware-Komponenten testen,
Kalibrierungen anpassen und Diagnosen sowohl lokal (seriell) als auch remote
(ueber Smartphone per BLE) durchfuehren.

---

## 1. Diagnose-Schnittstellen

Es gibt drei Wege, auf das Diagnosesystem zuzugreifen. Alle drei Wege sprechen
mit derselben Diagnose-Engine im Hintergrund, unterscheiden sich aber in
Eingabeformat und Zielgruppe.

### 1.1 Serielle Diagnose-Konsole (USB-C)

Der ESP32-S3 hat ein nativen USB-OTG-Controller. Der USB-C-Port am Geraet
stellt zwei virtuelle Schnittstellen bereit:

```
USB-C
  |
  +-- CDC0: ESP-ROM Bootloader (esptool.py flash, monitor)
  +-- CDC1: Diagnose-Konsole (Eingabe: ASCII-Terminal, 115200 8N1)
```

Die Diagnose-Konsole laeuft als FreeRTOS-Task (`diag_console_task`) mit
niedriger Prioritaet (Prio 2) auf Core 1. Sie liest zeilenorientiert vom
USB-CDC-1-Interface, parst Befehle und fuehrt sie aus. Die Ausgabe geht
unmittelbar zurueck an den selben Kanal. Ein Techniker oeffnet ein
Terminalprogramm (screen, minicom, PuTTY, TeraTerm) und tippt Befehle.

Die Konsole ist jederzeit verfuegbar, auch waehrend der Fahrt (USB-C an
Laptop/Powerbank angeschlossen). Die Ausgabe ist nicht-fluechtig: sie geht
nicht an das Display, sondern nur an die serielle Schnittstelle.

```
+------------------+
|  Terminal (PC)   |
|  screen /dev/... |
+--------+---------+
         | USB-C
         v
+--------+---------+
|  ESP32-S3 USB    |
|  CDC1 (Diag)     |
+--------+---------+
         |
         v
+--------+---------+
|  diag_console    |  FreeRTOS Task, Core 1, Prio 2
|  (Befehlsparser) |
+--------+---------+
         |
    +----+----+----+----+----+
    v         v    v    v    v
  DTC     Sensor  Self Config Log  OTA
  Module  Module  Test        Mod  Mod
```

#### Befehls-Parser

Der Parser ist ein einfacher Token-Splitter. Eingabe: ASCII-Zeile, terminiert
durch `\r\n` oder `\n`. Token-Trenner: Leerzeichen. Der erste Token ist der
Befehl, die restlichen Tokens sind Argumente.

```c
typedef struct {
    const char *cmd;       // Befehlsname
    const char *help;      // Kurzbeschreibung
    int (*handler)(int argc, char **argv);  // Handler
} diag_command_t;

static const diag_command_t commands[] = {
    {"dtc",     "DTC-Verwaltung",        cmd_dtc},
    {"sensor",  "Sensor-Werte lesen",   cmd_sensor},
    {"selftest", "Self-Test starten",   cmd_selftest},
    {"config",  "Konfiguration",        cmd_config},
    {"log",     "Log-Verwaltung",       cmd_log},
    {"ota",     "Firmware-Update",      cmd_ota},
    {"display", "Display-Test",         cmd_display},
    {"calibrate","Kalibrierung",        cmd_calibrate},
    {"module",  "Modul-Status",         cmd_module},
    {"maintenance","Wartung",           cmd_maintenance},
    {"help",    "Diese Hilfe",          cmd_help},
    {NULL, NULL, NULL}
};
```

Jede Eingabe wird mit `help` beantwortet, wenn der Befehl nicht erkannt wird.
Fehlermeldungen enthalten immer den Befehlsnamen und das Problem, niemals nur
eine Fehlernummer.

### 1.2 BLE-Diagnose-Service

Zusaetzlich zu den in Dokument 07 definierten GATT-Services (Telemetry, Config,
Navigation, OTA) wird ein weiterer Service fuer Diagnosezwecke definiert.

**Diagnose-Service** (UUID: 0000A005-0000-1000-8000-00805F9B34FB)

| Characteristic | UUID-Suffix | Zugriff | Beschreibung |
|---|---|---|---|
| DTC-List | A005-01 | read / notify | Alle aktiven DTCs als compact binary |
| DTC-Clear | A005-02 | write | DTCs loeschen (0xAA = Bestaetigungstoken) |
| Self-Test-Control | A005-03 | write | 0x01 = Start, 0x02 = Abbrechen |
| Self-Test-Result | A005-04 | read / notify | Ergebnis als DTC-Liste |
| Sensor-Live | A005-05 | read / notify | Live-Sensorwerte (wie Telemetry, aber diagnose-spezifisch) |
| Module-Status | A005-06 | read | Liste aller Module mit Status (present/absent/fault) |
| Log-Export | A005-07 | read / notify | Log-Datei-Auszug als Stream |
| Diagnose-Report | A005-08 | read | Kompletter Report als JSON |
| Calibration-Data | A005-09 | read / write | Kalibrierungs-Daten (bin oder JSON) |
| Maintenance-Counters | A005-0A | read | Betriebsstunden, Ladezyklen, etc. |

Das Binary-Format fuer DTC-List ist kompakt (12 Bytes pro DTC):

```
Byte 0-1:  DTC-Code (uint16, z.B. 0x0104 fuer S0104)
Byte 2:    Severity (0=INFO, 1=WARN, 2=ERROR, 3=CRITICAL)
Byte 3:    Status (0=active, 1=history, 2=pending)
Byte 4-7:  First-Occurrence (unix timestamp, uint32)
Byte 8-11: Last-Occurrence (unix timestamp, uint32)
```

### 1.3 Companion-App: Diagnose-Tab

Die Companion-App (Flutter, siehe Dokument 07 Abschnitt 5) erhaelt einen
eigenen Diagnose-Tab fuer nicht-technische Nutzer. Die App verbirgt die
seriellen Befehle hinter einer GUI, kommuniziert aber ueber denselben
BLE-Diagnose-Service.

Der Diagnose-Tab hat fuenf Unterseiten:

1. **Fehlercodes**: Liste aller aktiven DTCs mit Beschreibung, Severity-Farbe
   (gruen/gelb/rot) und Timestamp. Ein "Zuruecksetzen"-Button loescht alle
   History-DTCs.
2. **Live-Werte**: Dashboard mit allen Sensorwerten (Speed, RPM, Temp, Volt,
   Fuel, Neigung). Aktualisierung alle 500ms ueber BLE-Notify.
3. **Selbsttest**: Ein "Test starten"-Button triggert den Self-Test. Ergebnisse
   werden als Liste mit OK/FAIL je Modul angezeigt.
4. **Wartung**: Betriebsstunden, Ladezyklen, naechste Wartung. Ein
   "Wartung durchgefuehrt"-Button resetet den Zaehler.
5. **Support**: Ein "Diagnose-Report exportieren"-Button generiert einen
   JSON-Report (alle DTCs, alle Modul-Status, alle Kalibrierungs-Daten,
   Firmware-Version, Betriebsstunden) und bietet ihn zum Teilen an
   (AirDrop, E-Mail, Messenger).

---

## 2. Fehler-Code-System (DTC)

### 2.1 Design-Entscheidung: Eigenes System

Die PD06 hat kein OBD-II, daher kann kein bestehendes OBD-II-Code-Format
verwendet werden. Ein eigenes System wird definiert, das an OBD-II angelehnt
ist, aber auf die spezifischen Komponenten dieses Bordcomputers zugeschnitten ist.

Das System nutzt 4-stellige Codes mit einem Buchstaben-Praefix, aehnlich wie
OBD-II (P/C/B/U), aber erweitert um die Kategorien Sensoren (S) und
Diagnose-System-intern (D).

### 2.2 Code-Struktur

```
[P|S|C|B|D] [0] [XX] [X]
  |          |   |    |
  |          |   |    +-- Fehler-Nummer (0-9)
  |          |   +------- Komponenten-Gruppe (00-99)
  |          +----------- Immer 0 (Reserve fuer zukuenftige 1xxx-Erweiterung)
  +---------------------- Kategorie
```

Kategorien:

| Praefix | Kategorie | Bereich | Beschreibung |
|---|---|---|---|
| P | Powertrain | P0100-P0999 | Drehzahl, Zuendung, Motor-Sensoren |
| S | Sensoren | S0100-S0999 | Temp, Druck, Kraftstoff, IMU, GPS |
| C | Communication | C0100-C0999 | BLE, SD-Karte, SPI, I2C |
| B | Body | B0100-B0999 | Display, Relais, Beleuchtung |
| D | Diagnose-System | D0100-D0999 | Diagnose-System intern |

### 2.3 DTC-Datenstruktur

Jede DTC wird als Struktur gespeichert:

```c
typedef enum {
    DTC_SEV_INFO     = 0,
    DTC_SEV_WARN     = 1,
    DTC_SEV_ERROR    = 2,
    DTC_SEV_CRITICAL = 3,
} dtc_severity_t;

typedef enum {
    DTC_STATUS_ACTIVE  = 0,  // Problem aktuell vorhanden
    DTC_STATUS_HISTORY  = 1,  // Problem behoben, aber gespeichert
    DTC_STATUS_PENDING  = 2,  // Einmal aufgetreten, noch nicht bestaetigt
} dtc_status_t;

typedef struct __attribute__((packed)) {
    uint16_t code;              // z.B. 0x0104 fuer S0104
    uint8_t  severity;          // dtc_severity_t
    uint8_t  status;            // dtc_status_t
    uint32_t first_occurrence;  // Unix-Timestamp (RTC)
    uint32_t last_occurrence;   // Unix-Timestamp (RTC)
    uint16_t count;             // Wie oft aufgetreten
    char     description[48];   // Klartext-Beschreibung
} dtc_entry_t;  // 60 Bytes
```

### 2.4 Persistente Speicherung

DTCs werden doppelt gespeichert:

1. **NVS (Non-Volatile Storage)**: Schneller Zugriff, begrenzt Schreibzyklen.
   Nur aktive DTCs (status ACTIVE oder PENDING). Maximal 32 DTCs in NVS.
   Schreibvorgang nur bei Aenderung (neuer DTC, Status-Wechsel, Count-Erhoehung).

2. **SD-Karte (/logs/dtc.log)**: Vollstaendige Historie, Append-Only.
   Jedes DTC-Ereignis wird als eine Zeile geschrieben:
   ```
   2026-10-13T14:23:01|S0104|WARN|ACTIVE|count=3|Kuehlmitteltemp-Sensor unplausibel
   ```

### 2.5 Auto-Clear

Wenn ein Problem behoben ist (z.B. Sensor wieder im plausiblen Bereich), wird
der DTC-Status von ACTIVE auf HISTORY gesetzt. Das passiert automatisch, ohne
Benutzereingriff. Auto-Clear ist konfigurierbar:

```c
// config.h
bool dtc_auto_clear_enabled = true;   // Standard: an
uint16_t dtc_auto_clear_cycles = 10;  // 10 aufeinanderfolgende valide Lesungen
```

Ein DTC wird erst nach `dtc_auto_clear_cycles` aufeinanderfolgenden validen
Zyklen als behoben markiert. Das verhindert Flackern bei transienten Fehlern.

History-DTCs bleiben in der NVS, bis sie manuell mit `dtc clear` geloescht werden.

### 2.6 DTC-Tabelle (definierte Codes)

Mindestens 30 definierte Codes. Die folgende Tabelle enthaelt 45 Codes, die das
komplette Bordcomputer-System abdecken.

#### Powertrain (P0xxx)

| Code | Severity | Beschreibung |
|---|---|---|
| P0101 | WARN | Drehzahlsignal unplausibel (RPM > Redline oder < 0) |
| P0102 | ERROR | Drehzahlsignal ausgefallen (keine Pulse fuer >2s bei laufendem Motor) |
| P0103 | WARN | Drehzahlsignal noisy (Jitter > 20%, Filter wird debounct) |
| P0104 | INFO | Drehzahlsignal kalibriert (neuer Kalibrierungsfaktor gespeichert) |
| P0201 | WARN | Zündimpuls-Signalpegel niedrig (Optokoppler-Ausgang < Schwellwert) |
| P0202 | ERROR | Zündimpuls-Signal komplett ausgefallen |
| P0301 | WARN | Gang-Erkennung unplausibel (berechneter Gang > 6 oder < 0) |
| P0302 | INFO | Gang-Erkennung kalibriert (Schaltpunkte neu gelernt) |
| P0401 | WARN | Bordspannung unter 11.5V bei laufendem Motor (Generator/Regler?) |
| P0402 | CRITICAL | Bordspannung unter 10.5V (Batterie kritisch, Datenverlust droht) |
| P0403 | WARN | Bordspannung ueber 15.0V (Regler ueberlaedt) |
| P0404 | INFO | Bordspannung normalisiert (warnt, jetzt wieder im Bereich) |

#### Sensoren (S0xxx)

| Code | Severity | Beschreibung |
|---|---|---|
| S0101 | ERROR | Kuehlmitteltemperatur-Sensor unplausibel (Wert < -40C oder > 150C) |
| S0102 | WARN | Kuehlmitteltemperatur > 110C (Ueberhitzungswarnung) |
| S0103 | CRITICAL | Kuehlmitteltemperatur > 120C (Motor stoppen!) |
| S0201 | ERROR | Kraftstoffstand-Sensor unplausibel (Wert ausserhalb 0-100%) |
| S0202 | WARN | Kraftstoffstand < 10% (Reserve) |
| S0203 | INFO | Kraftstoffstand kalibriert (neue Kurve gespeichert) |
| S0301 | ERROR | Oeldruck-Schalter unplausibel (ON bei hoher Drehzahl / OFF bei Leerlauf) |
| S0302 | WARN | Oeldruck-Schalter signalisiert niedrigen Druck bei laufendem Motor |
| S0401 | ERROR | IMU (MPU6050) nicht gefunden am I2C-Bus |
| S0402 | WARN | IMU-Daten unplausibel (Beschleunigung > 5g statisch, Gyro-Drift) |
| S0403 | INFO | IMU kalibriert (neue Offsets gespeichert) |
| S0501 | WARN | GPS-Modul antwortet nicht (keine NMEA-Daten fuer >5s) |
| S0502 | WARN | GPS-Fix verloren (sats < 4) |
| S0503 | INFO | GPS-Fix wiederhergestellt |
| S0601 | ERROR | Barometer (BMP280) nicht gefunden am I2C-Bus |
| S0602 | WARN | Barometer-Daten unplausibel (Druck < 300 oder > 1100 hPa) |

#### Communication (C0xxx)

| Code | Severity | Beschreibung |
|---|---|---|
| C0101 | ERROR | SD-Karte nicht mountbar (SPI-Kommunikation fehlgeschlagen) |
| C0102 | WARN | SD-Karte fast voll (> 90% belegt) |
| C0103 | ERROR | SD-Karte voll (Schreiben nicht moeglich) |
| C0104 | WARN | SD-Karten-Schreibfehler (Retry wird ausgefuehrt) |
| C0201 | WARN | BLE-Advertising konnte nicht gestartet werden |
| C0202 | WARN | BLE-Verbindung getrennt (Timeout oder Gegenstelle weg) |
| C0203 | ERROR | BLE-Stack-Fehler (NimBLE internal error) |
| C0301 | ERROR | I2C-Bus-Fehler (SDA/SCL stuck low, Bus-Hang) |
| C0302 | WARN | I2C-Geraet antwortet nicht (ACK-Missing bei Adresse) |
| C0401 | WARN | SPI-Bus-Fehler (Display-Controller nicht erreichbar) |
| C0402 | ERROR | UART-Fehler (GPS-UART: Framing-Error oder Overrun) |

#### Body (B0xxx)

| Code | Severity | Beschreibung |
|---|---|---|
| B0101 | ERROR | Display-Controller antwortet nicht (SPI-Init fehlgeschlagen) |
| B0102 | WARN | Display-Refresh fehlgeschlagen (Timeout bei Partial-Refresh) |
| B0201 | WARN | Relais 1 (Zusatzlicht) schaltet nicht (Feedback-Pin keine Aenderung) |
| B0202 | WARN | Relais 2 (Heizgriffe) schaltet nicht |
| B0203 | WARN | Relais 3 (USB-Ladung) schaltet nicht |
| B0301 | WARN | Beleuchtung-Hintergrundbeleuchtung nicht regelbar (PWM-Fehler) |
| B0401 | WARN | Taster 1 (Lenker) entprellt ungueltig (machinesignal) |
| B0402 | WARN | Taster 2 (Lenker) entprellt ungueltig |

#### Diagnose-System (D0xxx)

| Code | Severity | Beschreibung |
|---|---|---|
| D0101 | ERROR | NVS-Schreibfehler (DTC konnte nicht gespeichert werden) |
| D0102 | WARN | NVS fast voll (> 80% belegt) |
| D0103 | ERROR | RTC-Zeit nicht gesetzt (DTC-Timestamps unzuverlaessig) |
| D0201 | ERROR | PSRAM-Test fehlgeschlagen (Speicherfehler erkannt) |
| D0202 | WARN | Heap-Fragmentierung hoch (> 80%, GC wird kritisch) |
| D0301 | ERROR | OTA-Partition unguelten (Flash-Verifikation fehlgeschlagen) |
| D0302 | WARN | OTA-Rollback aktiviert (neue Firmware crashed, alte wiederhergestellt) |
| D0401 | ERROR | Self-Test fehlgeschlagen (ein oder mehrere Module nicht bestanden) |
| D0402 | INFO | Self-Test bestanden (alle Module OK) |
| D0501 | ERROR | Watchdog-Reset erkannt (Task hing, Neustart ausgefuehrt) |
| D0502 | WARN | Brownout-Detector ausgeloest (Spannungseinbruch erkannt) |
| D0503 | WARN | LiPo-Akku altert (Ladezyklen > 300, Austausch pruefen) |
| D0601 | ERROR | Kalibrierungs-Daten unguelten (Checksum-Fehler, Default wird geladen) |

---

## 3. Log-System

### 3.1 Log-Struktur auf der SD-Karte

```
/logs/
  system.log          -- Allgemeine System-Logs (Boot, Tasks, BLE, OTA)
  dtc.log             -- Fehlercode-Historie (Append-Only)
  sensors/
    rpm.csv            -- Drehzahl (timestamp_ms, rpm, source)
    speed.csv          -- Geschwindigkeit (timestamp_ms, speed_kmh, source)
    temp.csv           -- Kuehlmitteltemperatur (timestamp_ms, temp_c)
    fuel.csv           -- Kraftstoffstand (timestamp_ms, percent, liters)
    voltage.csv        -- Bordspannung (timestamp_ms, voltage, source)
    imu.csv            -- IMU (timestamp_ms, ax, ay, az, gx, gy, gz)
    baro.csv           -- Barometer (timestamp_ms, pressure_hpa, altitude_m)
    gps.csv            -- GPS (timestamp_ms, lat, lon, speed, sats, hdop)
  tours/
    20261013_1423.gpx  -- Tour als GPX-Track
    20261013_1423.json -- Tour-Telemetrie als JSON (pro Sekunde)
```

### 3.2 Log-Levels

| Level | Wert | Beschreibung | Wird geloggt nach |
|---|---|---|---|
| DEBUG | 0 | Sehr detailliert (I2C-Transfers, BLE-Pakete) | system.log (nur wenn DEBUG aktiv) |
| INFO | 1 | Normale Ereignisse (Boot, BLE-Connect, GPS-Fix) | system.log |
| WARN | 2 | Potenzielle Probleme (SD fast voll, Spannung niedrig) | system.log + DTC-Check |
| ERROR | 3 | Fehler (Sensor ausgefallen, BLE-Stack-Error) | system.log + dtc.log |
| CRITICAL | 4 | Systemkritisch (Brownout, Watchdog) | system.log + dtc.log + Display-Warnung |

### 3.3 Ring-Buffer im RAM

Das direkte Schreiben auf SD-Karte bei jedem Log-Eintrag ist zu langsam und
verschleisst die SD-Karte. Stattdessen wird ein Ring-Buffer im RAM verwendet:

```c
#define LOG_RING_BUFFER_SIZE 8192  // 8KB Ring-Buffer
#define LOG_FLUSH_INTERVAL_MS 5000 // Batch-Write alle 5 Sekunden

typedef struct {
    char buffer[LOG_RING_BUFFER_SIZE];
    uint16_t write_pos;
    uint16_t read_pos;
    SemaphoreHandle_t mutex;
} log_ring_buffer_t;

static log_ring_buffer_t log_ring;
static TaskHandle_t log_flush_task_handle;
```

Der Log-Task (`log_task`, Core 0, Prio 3) schreibt formatierte Eintraege in den
Ring-Buffer. Ein separater Flush-Task (`log_flush_task`, Core 0, Prio 1) leert
den Buffer alle 5 Sekunden auf die SD-Karte.

```
Log-Aufruf (beliebiger Task)
  |
  v
log_write(level, tag, fmt, ...)
  |
  v
[Ring-Buffer im RAM, 8KB]
  |
  +-- alle 5s oder wenn Buffer >75% voll
  |
  v
log_flush_task
  |
  v
f_write() -> SD-Karte
```

Wenn die SD-Karte nicht verfuegbar ist (DTC C0101), faellt der Logger auf einen
kleinen RAM-Only-Buffer zurueck (256 Eintraege). Bei naechstem erfolgreichem
SD-Mount wird dieser nachgeflushed. Die Display-Warnung "SD-Karte fehlt" wird
angezeigt.

### 3.4 Log-Format

Jeder Log-Eintrag hat folgendes Format:

```
[2026-10-13T14:23:01.234] [INFO] [gps_task] GPS fix acquired: sats=8, hdop=1.2
[2026-10-13T14:23:02.001] [WARN] [sensor_task] Temp sensor reading: -47.3 (unplausible)
[2026-10-13T14:23:02.002] [ERROR] [dtc_task] DTC S0101 set: Kuehlmitteltemp-Sensor unplausibel
```

CSV-Sensordaten haben einen Header in der ersten Zeile:

```csv
timestamp_ms,rpm,source
1697205781234,3450,pcnt
1697205781334,3470,pcnt
```

### 3.5 Log-Rotation

Log-Dateien werden nicht endlos gross. Die Rotation laeuft taeglich um Mitternacht
(RTC-Trigger):

- `system.log` wird zu `system_20261012.log` umbenannt, neue `system.log` begonnen
- Dateien aelter als 30 Tage werden geloescht
- Wenn der freie Speicher unter 10% faellt, werden die aeltesten Logs zuerst geloescht
- `dtc.log` wird nicht rotiert, sondern als fortlaufende Historie gefuehrt, kann aber
  manuell mit `log truncate dtc` gekuerzt werden

```c
void log_rotate_daily(void) {
    char old_name[64];
    char date_str[16];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(date_str, sizeof(date_str), "%Y%m%d", tm);
    snprintf(old_name, sizeof(old_name), "/logs/system_%s.log", date_str);
    f_rename("/logs/system.log", old_name);
    log_cleanup_old(30);  // Dateien > 30 Tage loeschen
}
```

### 3.6 Log-Export

Logs koennen ueber die serielle Konsole oder BLE exportiert werden:

- Seriell: `log tail` streamt die letzten 100 Zeilen live, dann weiter bis
  Ctrl-C. `log dump system` schreibt die komplette system.log an die Konsole.
- BLE: Die Log-Export-Characteristic (A005-07) streamt die Datei in Chunks zu
  je 240 Bytes (MTU 247 - 3 ATT - 4 Header). Die App setzt die Chunks wieder
  zusammen.

---

## 4. Self-Test-Routine

### 4.1 Boot-Self-Test (nicht-blockierend)

Beim Boot wird ein nicht-blockierender Self-Test im Hintergrund ausgefuehrt.
Der Bordcomputer ist waehrend des Tests bereits voll funktionsfaehig, der
Self-Test laeuft als FreeRTOS-Task (`selftest_task`, Core 0, Prio 1).

```
Boot
  |
  v
[app_main init: Sensoren, Display, BLE, Tasks starten]
  |
  +---> selftest_task ( Hintergrund, nicht-blockierend)
  |       |
  |       +-- 1. I2C-Scan
  |       +-- 2. ADC-Read
  |       +-- 3. UART-Test (GPS)
  |       +-- 4. SPI-Test (SD-Karte, Display)
  |       +-- 5. BLE-Test
  |       +-- 6. Akku-Voltage
  |       +-- 7. Speicher-Test (PSRAM, Flash)
  |       |
  |       v
  |     Ergebnisse als DTCs setzen
  |       |
  |       v
  |     selftest_result_t -> NVS + Notify (BLE)
  |
  v
[Normalbetrieb laeuft parallel]
```

### 4.2 Test-Module

#### 4.2.1 I2C-Scan

Testet, ob die erwarteten Geraete am I2C-Bus antworten.

```c
selftest_result_t selftest_i2c(void) {
    selftest_result_t res = {0};
    uint8_t expected[] = {0x68, 0x76};  // MPU6050, BMP280
    uint8_t found[128] = {0};

    esp_err_t ret = i2c_scan(I2C_NUM_0, found, sizeof(found));
    if (ret != ESP_OK) {
        dtc_set(S0401, DTC_SEV_ERROR);  // IMU nicht gefunden
        dtc_set(S0601, DTC_SEV_ERROR);  // BMP280 nicht gefunden
        res.pass = false;
        return res;
    }

    for (int i = 0; i < sizeof(expected); i++) {
        if (!found[expected[i]]) {
            if (expected[i] == 0x68)
                dtc_set(S0401, DTC_SEV_ERROR);
            if (expected[i] == 0x76)
                dtc_set(S0601, DTC_SEV_ERROR);
            res.pass = false;
        }
    }
    return res;
}
```

Erwartete I2C-Geraete:

| Adresse | Geraet | Beschreibung |
|---|---|---|
| 0x68 | MPU6050 | IMU (Beschleunigung, Gyro) |
| 0x76 | BMP280 | Barometer / Hoehenmesser |

### 4.2.2 ADC-Read

Testet alle ADC-Kanaele, ob sie im plausiblen Bereich liegen.

```c
selftest_result_t selftest_adc(void) {
    // ADC1: GPIO1-GPIO10 (sicher vor BLE-Arbitrierung)
    // Kanaele: Batterie-Voltage, Kraftstoffstand, Oeldruck (analog)
    uint32_t vbat_raw = adc_read(ADC1_CHANNEL_0);
    float vbat = (vbat_raw / 4095.0) * 3.3 * VOLTAGE_DIVIDER_RATIO;

    if (vbat < 9.0 || vbat > 16.0) {
        dtc_set(P0401, DTC_SEV_WARN);  // Bordspannung unplausibel
    }

    uint32_t fuel_raw = adc_read(ADC1_CHANNEL_1);
    float fuel_v = (fuel_raw / 4095.0) * 3.3;
    if (fuel_v < 0.1 || fuel_v > 3.2) {
        dtc_set(S0201, DTC_SEV_ERROR);  // Kraftstoffsensor unplausibel
    }
    return res;
}
```

| ADC-Kanal | GPIO | Sensor | Plausibler Bereich |
|---|---|---|---|
| ADC1_CH0 | GPIO1 | Bordspannung | 9.0V - 15.5V |
| ADC1_CH1 | GPIO2 | Kraftstoffstand | 0.2V - 3.0V (Spannungsteiler) |
| ADC1_CH2 | GPIO3 | Oeldruck (analog) | 0.1V - 4.5V |
| ADC1_CH3 | GPIO4 | Reserve | - |

### 4.2.3 UART-Test (GPS)

Prueft, ob das GPS-Modul NMEA-Daten sendet. Der Test wartet maximal 5 Sekunden
auf einen vollstaendigen NMEA-Satz.

```c
selftest_result_t selftest_uart_gps(void) {
    uint8_t buf[256];
    int len = uart_read_bytes(UART_NUM_1, buf, sizeof(buf),
                                pdMS_TO_TICKS(5000));
    if (len <= 0) {
        dtc_set(S0501, DTC_SEV_WARN);  // GPS antwortet nicht
        res.pass = false;
    } else {
        // Pruefen, ob gueltige NMEA-Daten
        if (strstr((char*)buf, "$GN") || strstr((char*)buf, "$GP")) {
            dtc_clear(S0501);  // GPS wieder da
        }
    }
    return res;
}
```

### 4.2.4 SPI-Test (SD-Karte, Display)

Testet zwei SPI-Busse:

1. **SPI2 (FSPI)**: Display-Controller (ILI9488/ST7789). Test: Init-Kommando
   senden und Status-Register lesen.
2. **SPI3 (HSPI)**: SD-Karte. Test: Mount versuchen, Root-Directory lesen.

```c
selftest_result_t selftest_spi(void) {
    // SD-Karte mounten
    esp_err_t ret = sd_mount_check();
    if (ret != ESP_OK) {
        dtc_set(C0101, DTC_SEV_ERROR);
        res.pass = false;
    }

    // Display-Controller
    ret = display_init_check();
    if (ret != ESP_OK) {
        dtc_set(B0101, DTC_SEV_ERROR);
        res.pass = false;
    }
    return res;
}
```

### 4.2.5 BLE-Test

Prueft, ob der BLE-Stack initialisiert ist und Advertising starten kann.

```c
selftest_result_t selftest_ble(void) {
    int rc = ble_gap_adv_start(ble_addr, &adv_params,
                                ble_gap_event_handler, NULL);
    if (rc != 0) {
        dtc_set(C0201, DTC_SEV_WARN);
        res.pass = false;
    } else {
        ble_gap_adv_stop();
        dtc_clear(C0201);
    }
    return res;
}
```

### 4.2.6 Akku-Voltage

Der Bordcomputer hat einen LiPo-Backup-Akku (fuer Power-Loss-Pufferung). Die
Akku-Voltage wird ueber einen separaten ADC-Kanal gemessen.

```c
selftest_result_t selftest_battery(void) {
    uint32_t raw = adc_read(ADC1_CHANNEL_4);  // LiPo-Voltage
    float vbat_lipo = (raw / 4095.0) * 3.3 * 2.0;  // Divider 1:2

    if (vbat_lipo < 3.3) {
        // LiPo leer (< 10%)
        dtc_set(D0502, DTC_SEV_WARN);  // Brownout-Risk
    } else if (vbat_lipo < 3.0) {
        dtc_set(D0502, DTC_SEV_CRITICAL);
    }
    return res;
}
```

| Zustand | Voltage | Aktion |
|---|---|---|
| Voll | 4.0-4.2V | Keine Aktion |
| Normal | 3.6-4.0V | Keine Aktion |
| Niedrig | 3.3-3.6V | DTC WARN, Display-Hinweis |
| Kritisch | < 3.3V | DTC CRITICAL, Shutdown vorbereiten |

### 4.2.7 Speicher-Test

```c
selftest_result_t selftest_memory(void) {
    // PSRAM-Test
    uint8_t *psram_ptr = heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);
    if (!psram_ptr) {
        dtc_set(D0201, DTC_SEV_ERROR);
        res.pass = false;
    } else {
        // Schreib/Lese-Test
        memset(psram_ptr, 0xA5, 1024);
        if (psram_ptr[0] != 0xA5 || psram_ptr[1023] != 0xA5) {
            dtc_set(D0201, DTC_SEV_ERROR);
            res.pass = false;
        }
        free(psram_ptr);
    }

    // Flash-Partitionen pruefen
    const esp_partition_t *ota0 = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    const esp_partition_t *ota1 = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);

    if (!ota0 || !ota1) {
        dtc_set(D0301, DTC_SEV_ERROR);
        res.pass = false;
    }
    return res;
}
```

### 4.3 Manueller Self-Test

Der Self-Test kann jederzeit manuell getriggert werden:

- Seriell: `selftest`
- BLE: Write 0x01 auf Self-Test-Control (A005-03)
- App: "Test starten"-Button im Diagnose-Tab

Der manuelle Test unterscheidet sich vom Boot-Test nur darin, dass er
blockierend ist (die Konsole/App wartet auf das Ergebnis) und das Ergebnis
unmittelbar zurueckgegeben wird.

### 4.4 Self-Test-Ergebnis

Das Ergebnis wird als Struktur zurueckgegeben:

```c
typedef struct {
    bool pass;
    uint8_t tests_run;
    uint8_t tests_passed;
    uint8_t tests_failed;
    uint32_t duration_ms;
    struct {
        bool i2c;
        bool adc;
        bool uart_gps;
        bool spi_sd;
        bool spi_display;
        bool ble;
        bool battery;
        bool memory;
    } results;
} selftest_result_t;
```

Beispiel-Ausgabe (seriell):

```
> selftest
Running self-test...
  [1/8] I2C-Scan.............. PASS (0x68, 0x76 found)
  [2/8] ADC-Read.............. PASS (VBAT=12.4V, FUEL=1.8V)
  [3/8] UART-GPS.............. PASS (NMEA data received)
  [4/8] SPI-SD................. PASS (mounted, 14.2GB free)
  [5/8] SPI-Display........... PASS (ILI9488 init OK)
  [6/8] BLE................... PASS (advertising started)
  [7/8] Battery.............. PASS (LiPo=3.92V)
  [8/8] Memory............... PASS (PSRAM OK, partitions OK)
---
Result: 8/8 passed in 234ms
```

---

## 5. Kalibrierungs-System

### 5.1 Uebersicht

Jeder analoge Sensor kann kalibriert werden. Kalibrierungs-Daten werden in NVS
gespeichert und sind ueber die serielle Konsole, BLE oder die Companion-App
exportierbar. Jede Kalibrierung hat eine Checksumme, um Datenintegritaet zu
gewaehrleisten.

```c
typedef struct {
    uint16_t sensor_id;       // z.B. 0x0201 fuer Kraftstoff
    uint16_t num_points;      // Anzahl Stuetzpunkte
    uint32_t cal_timestamp;   // Wann kalibriert
    uint32_t checksum;        // CRC32 ueber alle Daten
    struct {
        float raw;            // Rohwert (ADC-counts oder Volt)
        float physical;       // Physikalischer Wert (Liter, Grad, bar)
    } points[8];              // Max 8 Stuetzpunkte
} calibration_t;
```

### 5.2 Kraftstoffstand-Kalibrierung

Die Kraftstoffstand-Kalibrierung ist die wichtigste, weil der Sensor
(Widerstandsgeber im Tank, 5-90 Ohm, typisch fuer dieses Modell) nicht linear ist.

Prozedur (2-Punkt, erweiterbar auf Mehrpunkt):

```
1. Tank komplett leeren (Fahrt bis zum Aufbrauchen)
2. Konsole: "calibrate fuel"
   -> System liest ADC-Wert und speichert als EMPTY-Point
3. Tank volltanken (bis zum sichtbaren Niveau)
4. Konsole: "calibrate fuel"
   -> System liest ADC-Wert und speichert als FULL-Point
5. System interpoliert linear zwischen EMPTY und FULL
   ( fuer genauere Kurven: bis zu 8 Stuetzpunkte moeglich,
     z.B. bei 5L, 10L, 15L, 18L )
```

```c
calibration_t calibrate_fuel(calibration_t *cal) {
    uint32_t raw = adc_read(ADC1_CHANNEL_1);
    float voltage = (raw / 4095.0) * 3.3;

    if (cal->num_points == 0) {
        // Erster Punkt: Leertank
        cal->points[0].raw = voltage;
        cal->points[0].physical = 0.0;
        cal->num_points = 1;
        printf("EMPTY point saved: %.3f V = 0.0 L\n", voltage);
    } else if (cal->num_points == 1) {
        // Zweiter Punkt: Volltank
        cal->points[1].raw = voltage;
        cal->points[1].physical = 18.0;  // 18L Tank
        cal->num_points = 2;
        printf("FULL point saved: %.3f V = 18.0 L\n", voltage);
    } else {
        // Mehrpunkt-Kalibrierung: Punkt hinzufuegen
        // (interaktive Eingabe des Liter-Werts)
    }
    cal->cal_timestamp = time(NULL);
    cal->checksum = crc32(cal, offsetof(calibration_t, checksum));
    nvs_save_calibration(cal);
    return *cal;
}
```

### 5.3 Kuehlmitteltemperatur-Kalibrierung

2-Punkt-Kalibrierung mit Referenztemperaturen:

```
1. Sensor in Eiswasser (0C)
2. Konsole: "calibrate temp"
   -> System speichert ADC-Wert bei 0C
3. Sensor in kochendes Wasser (100C)
4. Konsole: "calibrate temp"
   -> System speichert ADC-Wert bei 100C
5. Lineare Interpolation zwischen 0C und 100C
```

### 5.4 Oeldruck-Kalibrierung

Nullpunkt-Kalibrierung bei ausgeschaltetem Motor (Oeldruck = 0 bar):

```
1. Motor aus
2. Konsole: "calibrate oil"
   -> System liest ADC-Wert und speichert als 0-bar-Point
3. (Optional) Referenzdruck angeschlossen, 2. Punkt
```

### 5.5 IMU-Kalibrierung

Lage-Kalibrierung: Motorrad steht aufrecht auf ebenem Untergrund.

```
1. Motorrad aufrecht, nicht auf Seitenstaender
2. Konsole: "calibrate imu"
   -> System liest 1000 Samples (ca. 2 Sekunden bei 500 Hz)
   -> Mittelwert der Gyro-Werte = Offset (sollte ~0 sein)
   -> Mittelwert der Accel-Werte = 1g in Z-Richtung
   -> Offsets in NVS gespeichert
```

```c
calibration_t calibrate_imu(void) {
    float ax_sum = 0, ay_sum = 0, az_sum = 0;
    float gx_sum = 0, gy_sum = 0, gz_sum = 0;
    const int N = 1000;

    for (int i = 0; i < N; i++) {
        mpu6050_data_t data;
        mpu6050_read(&data);
        ax_sum += data.ax;
        ay_sum += data.ay;
        az_sum += data.az;
        gx_sum += data.gx;
        gy_sum += data.gy;
        gz_sum += data.gz;
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    // Gyro-Offset
    cal_imu.gx_offset = gx_sum / N;
    cal_imu.gy_offset = gy_sum / N;
    cal_imu.gz_offset = gz_sum / N;

    // Accel-Offset (1g in Z-Richtung erwartet)
    cal_imu.ax_offset = 0.0;  // X sollte 0 sein
    cal_imu.ay_offset = 0.0;  // Y sollte 0 sein
    cal_imu.az_offset = (az_sum / N) - 1.0;  // Z sollte 1g sein

    nvs_save_calibration(&cal_imu);
    dtc_set(S0403, DTC_SEV_INFO);  // IMU kalibriert
    return cal_imu;
}
```

### 5.6 GPS-Position-Offset

GPS-Kalibrierung ist nur bei Bedarf. Sie speichert einen Offset zwischen
gemessener GPS-Position und einer bekannten Referenzposition (z.B. Vermessungspunkt).

```
1. Motorrad an bekannter Position platzieren
2. Konsole: "calibrate gps"
   -> System liest 60 Sekunden GPS-Daten
   -> Mittelwert bilden
   -> Offset zu Referenzposition speichern
```

### 5.7 Kalibrierungs-Export

Alle Kalibrierungs-Daten koennen als JSON exportiert werden:

```
> calibrate export
{
  "sensor_id": "S02",
  "sensor_name": "fuel_level",
  "num_points": 2,
  "cal_timestamp": "2026-10-13T14:23:01Z",
  "points": [
    {"raw": 0.234, "physical": 0.0},
    {"raw": 2.871, "physical": 18.0}
  ],
  "checksum": "0xA1B2C3D4"
}
```

---

## 6. Modularitaet und Reparierbarkeit

### 6.1 Hardware-Module

Jede Hardware-Komponente ist als eigenstaendiges, steckbares Modul ausgefuehrt.
Keine fest verlöteten Verbindungen zwischen Hauptplatine und Peripherie. Jedes
Modul hat einen eindeutigen Identifikator (I2C-Adresse, SPI-Chip-Select oder
ADC-Kanal), ueber den die Software es erkennt.

| Modul | Interface | ID | Erkennung |
|---|---|---|---|
| IMU (MPU6050) | I2C | 0x68 | I2C-Scan |
| Barometer (BMP280) | I2C | 0x76 | I2C-Scan |
| GPS-Modul | UART | UART1 | NMEA-Daten empfangen |
| SD-Karte | SPI3 | CS=GPIO10 | Mount erfolgreich |
| Display (TFT) | SPI2 | CS=GPIO9 | Controller-Init |
| Kraftstoff-Sensor | ADC1_CH1 | GPIO2 | ADC in Range |
| Temp-Sensor | ADC1_CH2 oder 1-Wire | GPIO3 | ADC/1-Wire |
| Oeldruck-Sensor | ADC1_CH3 | GPIO4 | ADC in Range |
| Bordspannung | ADC1_CH0 | GPIO1 | ADC in Range |
| Relais-Modul 1-3 | GPIO | GPIO36-38 | Feedback-Pin |
| LiPo-Backup | ADC1_CH4 | GPIO5 | ADC in Range |

### 6.2 Software-Modul-Erkennung

Der Sensormanager beim Boot registriert alle verfuegbaren Module. Module, die
nicht gefunden werden, werden als `ABSENT` markiert. Das System laeuft ohne sie
weiter, aber die betroffenen Features werden deaktiviert.

```c
typedef enum {
    MODULE_PRESENT,    // Modul gefunden und funktionsfaehig
    MODULE_ABSENT,     // Modul nicht gefunden (Kabel defekt, Modul entfernt)
    MODULE_FAULT,      // Modul gefunden, aber Kommunikation fehlerhaft
} module_status_t;

typedef struct {
    uint16_t id;            // Modul-ID (I2C-Adresse, ADC-Kanal, etc.)
    const char *name;       // Modul-Name
    module_status_t status;
    bool required;          // true = System startet nicht ohne dieses Modul
    void (*on_absent)(void); // Callback fuer graceful degradation
} module_t;
```

### 6.3 Graceful Degradation

Die folgende Tabelle zeigt, was passiert, wenn ein Modul fehlt:

| Fehlendes Modul | Auswirkung | Display-Anzeige | Rest funktioniert? |
|---|---|---|---|
| GPS | Keine Geschwindigkeit, kein GPX-Logging | "GPS Offline" Banner | Ja (Hall-Sensor Fallback, wenn vorhanden) |
| IMU | Keine Neigungsanzeige, kein Crash-Detection | "IMU Offline" Hinweis | Ja |
| BMP280 | Keine Hoehenanzeige | "Baro Offline" Hinweis | Ja |
| SD-Karte | Kein Logging, keine Tour-Aufzeichnung | "SD FEHLT" Warnung dauerhaft | Ja (Logs im RAM-Puffer) |
| Display | Keine Anzeige | (Display fehlt) | Ja (BLE-App als Display) |
| Kraftstoff-Sensor | Keine Kraftstoffanzeige | Kraftstoffanzeige ausgeblendet | Ja |
| Temp-Sensor | Keine Temperaturanzeige | Temperaturanzeige ausgeblendet | Ja |
| Oeldruck | Keine Oeldruckwarnung | Oeldruckanzeige ausgeblendet | Ja |
| Relais-Modul | Zubehoer nicht schaltbar | "Relais X defekt" | Ja |
| LiPo-Backup | Kein Power-Loss-Puffer | "Akku fehlt" Hinweis | Ja (mit Risiko) |

### 6.4 Modul-Diagnose-Anzeige

Jedes Modul hat einen Status, der im Diagnose-System angezeigt wird:

```
> module list
ID     Name              Status     Required
0x68   IMU (MPU6050)     PRESENT    No
0x76   Baro (BMP280)     PRESENT    No
UART1  GPS-Modul         PRESENT    No
SPI3   SD-Karte          PRESENT    Yes
SPI2   Display (TFT)     PRESENT    Yes
CH0    Bordspannung      PRESENT    Yes
CH1    Kraftstoff        PRESENT    No
CH2    Kuehltemp         PRESENT    No
CH3    Oeldruck          ABSENT     No
GPIO36 Relais 1          PRESENT    No
GPIO37 Relais 2          PRESENT    No
GPIO38 Relais 3          ABSENT     No
CH4    LiPo-Backup       PRESENT    No
---
12 modules registered, 2 absent, 0 fault
```

---

## 7. Remote-Support ueber Companion-App

### 7.1 Companion-App Diagnose-Funktionen

Die Companion-App (Flutter, siehe Dokument 07) kommuniziert ueber den
BLE-Diagnose-Service (Abschnitt 1.2) mit dem Bordcomputer. Die App kann
folgende Diagnose-Funktionen ausfuehren:

| Funktion | BLE-Characteristic | Beschreibung |
|---|---|---|
| DTCs auslesen | A005-01 (read/notify) | Liste aller aktiven und History-DTCs |
| DTCs loeschen | A005-02 (write) | Bestaetigungstoken 0xAA + 0x55 |
| Self-Test starten | A005-03 (write) | 0x01 = Start, 0x02 = Abbrechen |
| Self-Test-Ergebnis | A005-04 (read/notify) | Ergebnis als compact binary |
| Live-Sensorwerte | A005-05 (read/notify) | Wie Telemetry-Service, aber diagnose-spezifisch |
| Modul-Status | A005-06 (read) | Liste aller Module mit Status |
| Log-Export | A005-07 (read/notify) | Log-Datei als Stream (240B Chunks) |
| Diagnose-Report | A005-08 (read) | Kompletter Report als JSON |
| Kalibrierung | A005-09 (read/write) | Kalibrierungs-Daten lesen/schreiben |
| Wartungs-Zaehler | A005-0A (read) | Betriebsstunden, Ladezyklen |

### 7.2 Diagnose-Report (Support-Modus)

Der Diagnose-Report ist die wichtigste Funktion fuer den Remote-Support. Er
bündelt alle diagnose-relevanten Daten in einem JSON-Dokument, das der Nutzer
per E-Mail oder Messenger an den Hersteller/Entwickler senden kann.

```json
{
  "report_version": "1.0",
  "generated_at": "2026-10-13T14:23:01Z",
  "device": {
    "firmware_version": "1.2.3",
    "hardware_revision": "rev2",
    "esp_idf_version": "5.4.4",
    "chip_model": "ESP32-S3",
    "flash_size_mb": 8,
    "psram_size_mb": 2,
    "uptime_seconds": 86400
  },
  "modules": [
    {"id": "0x68", "name": "IMU", "status": "PRESENT"},
    {"id": "0x76", "name": "Baro", "status": "PRESENT"},
    {"id": "UART1", "name": "GPS", "status": "PRESENT"},
    {"id": "SPI3", "name": "SD", "status": "PRESENT"},
    {"id": "SPI2", "name": "Display", "status": "PRESENT"}
  ],
  "dtcs": [
    {
      "code": "S0102",
      "severity": "WARN",
      "status": "ACTIVE",
      "first_occurrence": "2026-10-13T14:20:00Z",
      "last_occurrence": "2026-10-13T14:23:00Z",
      "count": 3,
      "description": "Kuehlmitteltemp > 110C"
    }
  ],
  "calibrations": [
    {"sensor": "fuel_level", "points": 2, "last_calibrated": "2026-10-10T08:00:00Z"},
    {"sensor": "imu", "offsets_set": true, "last_calibrated": "2026-10-10T08:01:00Z"}
  ],
  "maintenance": {
    "operating_hours": 523.5,
    "charge_cycles": 42,
    "next_maintenance_hours": 1000,
    "sd_usage_percent": 34
  },
  "last_selftest": {
    "timestamp": "2026-10-13T06:00:00Z",
    "result": "PASS",
    "tests_run": 8,
    "tests_passed": 8
  }
}
```

### 7.3 Firmware-Update ueber App

Das Firmware-Update wird ueber den bereits definierten OTA-Service (Dokument 03,
Abschnitt 8) abgewickelt. Die App laedt die Firmware-Datei hoch und streamt sie
in Chunks von 240 Bytes an den Bordcomputer.

Die App zeigt den Fortschritt als Prozentbalken. Nach Abschluss startet der
Bordcomputer neu und verifiziert die neue Firmware. Bei einem Crash innerhalb
der ersten 60 Sekunden nach Update wird automatisch die alte Partition
zurueckgespielt (OTA-Rollback, DTC D0302).

```
App                         Bordcomputer
 |                              |
 |  OTA-Start (write 0x01)      |
 |----------------------------->|
 |  OTA-Status: READY (notify)  |
 |<-----------------------------|
 |  OTA-Data chunk 1 (240B)     |
 |----------------------------->|
 |  OTA-Status: 1% (notify)     |
 |<-----------------------------|
 |  OTA-Data chunk 2 (240B)     |
 |----------------------------->|
 |  ...                         |
 |  OTA-Data chunk N (240B)     |
 |----------------------------->|
 |  OTA-Finish (write 0x03)     |
 |----------------------------->|
 |  OTA-Status: VERIFYING       |
 |<-----------------------------|
 |  OTA-Status: COMPLETE        |
 |<-----------------------------|
 |  (Reboot)                    |
 |                              |
 |  New firmware running        |
 |  (BLE reconnect)             |
 |<-----------------------------|
```

### 7.4 Log-Export ueber BLE

Der Log-Export ueber BLE ist langsam (240 Bytes pro Chunk, bei einer
20ms-Connection-Interval sind das ca. 12 KB/s). Fuer große Log-Dateien wird
empfohlen, den Log-Export ueber USB-C (seriell) durchzufuehren.

Die App bietet zwei Modi:

1. **Quick-Export**: Letzte 1000 Zeilen system.log (ca. 50-100KB, ca. 5-10s
   ueber BLE)
2. **Full-Export**: Komplette Log-Datei (dauert lange, Fortschrittsbalken)

Der Log-Stream beginnt mit einem Header:

```
Byte 0-3:   Total-Laenge der Datei (uint32)
Byte 4-7:   Chunk-Anzahl (uint32)
Byte 8:     Log-Typ (0=system, 1=dtc, 2=sensor)
Byte 9+:    Chunk-Daten (240 Bytes)
```

---

## 8. Wartungs-Intervalle

### 8.1 Betriebsstunden-Zaehler

Der Bordcomputer zaehlt Betriebsstunden (wie ein Motorrad-Stundenzähler). Die
Zaehlung beginnt, sobald die Bordspannung > 11V liegt (Motor laeuft oder
Zuendung an). Der Zaehler wird in NVS gespeichert und ist reset-sicher.

```c
static float operating_hours = 0;
static uint32_t last_save_time = 0;

void maintenance_task(void *arg) {
    while (1) {
        float vbat = read_batt_voltage();
        if (vbat > 11.0) {
            operating_hours += 1.0 / 3600.0;  // 1 Sekunde = 1/3600 Stunde
        }

        // Alle 10 Minuten in NVS speichern
        uint32_t now = time(NULL);
        if (now - last_save_time > 600) {
            nvs_save_float("op_hours", operating_hours);
            last_save_time = now;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### 8.2 Wartungs-Erinnerungen

| Kriterium | Schwelle | Aktion |
|---|---|---|
| Betriebsstunden | alle 500h | "Wartung empfohlen" im Display, Self-Test wird angeregt |
| Betriebsstunden | alle 1000h | "Wartung faellig" im Display (blinkend) |
| SD-Karte belegt | > 90% | "SD-Karte bald voll, Logs exportieren" |
| SD-Karte belegt | > 98% | "SD-Karte voll, Logging gestoppt" |
| Ladezyklen LiPo | > 300 Zyklen | "Akku altert, Austausch pruefen" |
| Ladezyklen LiPo | > 500 Zyklen | "Akku austauschen" (DTC WARN) |
| Letzter Self-Test | > 30 Tage | "Self-Test empfohlen" |

### 8.3 Akku-Zyklus-Zaehler

Jeder vollstaendige Ladezyklus (Entladen < 3.4V -> Laden > 4.0V) wird gezaehlt.

```c
typedef enum {
    BATT_UNKNOWN,
    BATT_DISCHARGING,
    BATT_CHARGING,
    BATT_FULL,
} batt_state_t;

static batt_state_t batt_state = BATT_UNKNOWN;
static uint16_t charge_cycles = 0;

void battery_monitor(void) {
    float vbat = read_lipo_voltage();

    switch (batt_state) {
    case BATT_UNKNOWN:
        if (vbat < 3.4) batt_state = BATT_DISCHARGING;
        if (vbat > 4.0) batt_state = BATT_FULL;
        break;
    case BATT_DISCHARGING:
        if (vbat > 4.0) {
            charge_cycles++;
            batt_state = BATT_CHARGING;
            nvs_save_u16("chg_cycles", charge_cycles);
            if (charge_cycles > 300) {
                dtc_set(D0503, DTC_SEV_WARN);  // Akku altert
            }
            if (charge_cycles > 500) {
                dtc_set(D0503, DTC_SEV_ERROR); // Akku austauschen
            }
        }
        break;
    case BATT_CHARGING:
        if (vbat > 4.1) batt_state = BATT_FULL;
        break;
    case BATT_FULL:
        if (vbat < 3.4) batt_state = BATT_DISCHARGING;
        break;
    }
}
```

### 8.4 Wartungs-Reset

Nach durchgefuehrter Wartung kann der Wartungs-Zaehler zurueckgesetzt werden:

```
> maintenance reset
Wartungs-Intervall zurueckgesetzt.
Betriebsstunden-Zaehler: 523.5h -> 0h (Referenz fuer naechstes Intervall)
Letzter Self-Test: 2026-10-13T14:23:01Z
```

### 8.5 Serielle Wartungs-Befehle

| Befehl | Beschreibung |
|---|---|
| `maintenance status` | Zeigt Betriebsstunden, Ladezyklen, SD-Belegung |
| `maintenance reset` | Resetet Wartungs-Zaehler (nach Wartung) |
| `maintenance set hours <value>` | Setzt Betriebsstunden (manuelle Korrektur) |
| `maintenance set cycles <value>` | Setzt Ladezyklen (manuelle Korrektur) |

---

## 9. Kommando-Referenz: Serielle Diagnose-Konsole

### 9.1 Syntax

Alle Befehle werden als ASCII-Text eingegeben, terminiert durch Enter (\r oder
\n oder \r\n). Argumente sind durch Leerzeichen getrennt. Gross-/Kleinschreibung
wird nicht unterschieden. Befehle koennen abgekuerzt werden, solange eindeutig
(z.B. `dtc l` fuer `dtc list`).

### 9.2 Befehlsuebersicht

```
help                        Diese Hilfe anzeigen
dtc list                    Alle DTCs anzeigen (aktiv + History)
dtc clear                   Alle DTCs loeschen
dtc clear <code>            Einzelnen DTC loeschen (z.B. dtc clear S0102)
dtc export                  DTC-Liste als JSON exportieren
sensor list                 Alle verfuegbaren Sensoren auflisten
sensor read <name>          Live-Wert eines Sensors anzeigen
sensor read all             Alle Sensorwerte einmalig anzeigen
sensor watch <name>         Live-Wert eines Sensors kontinuierlich (alle 500ms)
selftest                    Kompletten Self-Test starten
selftest <module>           Einzelnes Modul testen (z.B. selftest i2c)
config get                  Komplette Konfiguration anzeigen
config get <key>            Einzelnen Config-Wert anzeigen
config set <key> <value>    Config-Wert setzen
config save                 Konfiguration in NVS speichern
config reset                Konfiguration auf Default zuruecksetzen
log tail [n]                Letzte n Zeilen system.log (default 100)
log tail live               Live-Log-Output (Ctrl-C zum Abbrechen)
log dump <file>             Komplette Log-Datei ausgeben
log list                    Alle Log-Dateien auflisten
log truncate <file>         Log-Datei leeren (nicht loeschen)
log rotate                  Log-Rotation sofort ausfuehren
ota flash                   Firmware-Update ueber Seriell starten
ota status                  OTA-Status anzeigen
ota rollback                Zurueck zur vorherigen Firmware
display test                Display-Test-Pattern anzeigen
display clear               Display loeschen
display brightness <0-100>  Display-Helligkeit setzen
calibrate <sensor>          Sensor-Kalibrierung starten
calibrate list              Alle Kalibrierungs-Daten anzeigen
calibrate export            Kalibrierung als JSON exportieren
calibrate reset <sensor>    Kalibrierung auf Default zuruecksetzen
module list                 Alle Module mit Status auflisten
module info <id>            Detail-Info fuer ein Modul
maintenance status          Wartungs-Status anzeigen
maintenance reset           Wartungs-Zaehler zuruecksetzen
reboot                      System neustarten
version                     Firmware-Version anzeigen
```

### 9.3 Detail: dtc

```
> dtc list
Active DTCs (3):
  S0102  WARN   ACTIVE   count=3  first=14:20:00  last=14:23:00  Kuehlmitteltemp > 110C
  C0102  WARN   ACTIVE   count=1  first=14:21:00  last=14:21:00  SD-Karte fast voll
  S0502  WARN   ACTIVE   count=2  first=14:22:00  last=14:22:30  GPS-Fix verloren

History DTCs (2):
  P0404  INFO   HISTORY  count=1  first=14:00:00  last=14:05:00  Bordspannung normalisiert
  S0503  INFO   HISTORY  count=1  first=14:01:00  last=14:10:00  GPS-Fix wiederhergestellt

> dtc clear
Cleared 5 DTCs (3 active, 2 history)
```

### 9.4 Detail: sensor

```
> sensor list
Available sensors:
  rpm          Drehzahl (RPM)            Source: PCNT
  speed        Geschwindigkeit (km/h)    Source: GPS
  temp         Kuehlmitteltemp (C)       Source: ADC1_CH2
  fuel         Kraftstoffstand (%)       Source: ADC1_CH1
  voltage      Bordspannung (V)          Source: ADC1_CH0
  oil          Oeldruck (bar)            Source: ADC1_CH3
  imu_roll     Neigung (Grad)           Source: MPU6050
  imu_pitch    Neigung (Grad)           Source: MPU6050
  altitude     Hoehe (m)                Source: BMP280
  pressure     Luftdruck (hPa)          Source: BMP280
  gps_lat      GPS Breitengrad           Source: GPS
  gps_lon      GPS Laengengrad           Source: GPS
  gps_sats     GPS Satelliten            Source: GPS

> sensor read all
rpm       = 0      (PCNT)
speed     = 0.0    (GPS)
temp      = 87.2   (ADC1_CH2)
fuel      = 72.3   (ADC1_CH1)
voltage   = 12.4   (ADC1_CH0)
oil       = 0.0    (ADC1_CH3)
imu_roll  = -1.2   (MPU6050)
imu_pitch = 0.5    (MPU6050)
altitude  = 245.3  (BMP280)
pressure  = 1013.2 (BMP280)
gps_lat   = 50.935
gps_lon   = 11.589
gps_sats  = 8
```

### 9.5 Detail: config

Konfigurationsschluessel werden als Namespace/key-Paare gespeichert. Die
folgenden Schluessel sind definiert:

| Schluessel | Typ | Default | Beschreibung |
|---|---|---|---|
| display.mode | uint8 | 0 | 0=Tour, 1=Track, 2=Config |
| display.brightness | uint8 | 80 | 0-100% |
| log.interval | uint8 | 1 | Log-Interval Sekunden |
| log.level | uint8 | 1 | 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR |
| log.rotation_days | uint8 | 30 | Max Tage |
| dtc.auto_clear | bool | true | Auto-Clear bei behobenen Fehlern |
| dtc.auto_clear_cycles | uint8 | 10 | Zyklen bis Auto-Clear |
| ble.device_name | string | BC-XXXX | BLE-Geraetename |
| ble.pairing | bool | true | Pairing erforderlich |
| maintenance.interval_hours | uint16 | 500 | Wartungs-Intervall |
| maintenance.sd_warn_percent | uint8 | 90 | SD-Warnschwelle |
| maintenance.battery_warn_cycles | uint16 | 300 | Akku-Warnschwelle |
| sensor.fuel.empty_mv | uint16 | 200 | Kalibrierung: Leertank |
| sensor.fuel.full_mv | uint16 | 2900 | Kalibrierung: Volltank |
| sensor.temp.offset_c | float | 0.0 | Temp-Kalibrierungs-Offset |
| sensor.imu.roll_offset | float | 0.0 | IMU Roll-Offset |
| sensor.imu.pitch_offset | float | 0.0 | IMU Pitch-Offset |
| sensor.gps.speed_offset | float | 0.0 | GPS Speed-Korrektur |
| ota.auto_rollback | bool | true | Rollback bei Crash |
| ota.verify_timeout_s | uint8 | 60 | Verifizierungs-Timeout |

```
> config get display
display.mode = 0 (Tour)
display.brightness = 80%

> config set display.brightness 100
display.brightness = 100
(Don't forget to 'config save' to persist)

> config save
Configuration saved to NVS (23 keys)
```

### 9.6 Detail: ota flash (seriell)

Das Firmware-Update ueber Seriell nutzt das YMODEM-Protokoll. Der Techniker
startet den Upload mit `ota flash`, der Bordcomputer erwartet dann die
Firmware-Datei als YMODEM-Stream.

```
> ota flash
Ready to receive firmware. Send via YMODEM.
Waiting for transfer...
```

Am PC (Beispiel mit lrzsz):

```bash
$ stty -F /dev/ttyACM0 115200 raw -echo
$ sz --ymodem firmware.bin < /dev/ttyACM0 > /dev/ttyACM0
```

Der Bordcomputer schreibt die Firmware in die inaktive Partition, verifiziert
die Checksum und startet neu. Bei Fehler wird DTC D0301 gesetzt und die
aktuelle Partition nicht verlassen.

```
Receiving firmware via YMODEM...
  Bytes received: 1245184
  Writing to partition ota_1...
  Verifying checksum... OK
  Setting boot partition to ota_1
  Rebooting in 3...2...1...
```

### 9.7 Detail: display test

```
> display test
Running display test pattern...
  [1/5] Solid white
  [2/5] Solid red
  [3/5] Solid green
  [4/5] Solid blue
  [5/5] Color bars + crosshair
Test complete. Display restored to normal mode.
```

---

## 10. ASCII-Flowcharts: Diagnose-Pipeline

### 10.1 Boot-Diagnose-Pipeline

```
+---------------------+
| Power On / Reset    |
+----------+----------+
           |
           v
+----------+----------+
| Bootloader (ROM)    |
| Checks flash,       |
| loads app partition |
+----------+----------+
           |
           v
+----------+----------+
| app_main()          |
| - NVS init          |
| - RTC sync (GPS)    |
| - I2C/SPI/ADC init  |
+----------+----------+
           |
           +--+--+--+--+--+--+
           |  |  |  |  |  |  |
           v  v  v  v  v  v  v
         [GPS][IMU][BARO][SD][DISP][BLE][ADC]
          |   |    |     |   |    |   |
          |   +----+--+--+---+----+---+---> Module Registry
          |                                            |
          |                                            v
          |                                   +--------+--------+
          |                                   | selftest_task    |
          |                                   | (background)     |
          |                                   | I2C/ADC/UART/    |
          |                                   | SPI/BLE/Batt/Mem |
          |                                   +--------+--------+
          |                                            |
          |                                            v
          |                                   +--------+--------+
          |                                   | DTC Engine      |
          |                                   | Set/Clear DTCs  |
          |                                   | NVS + SD persist|
          |                                   +--------+--------+
          |                                            |
          v                                            v
+----------+----------+                   +--------+--------+
| Normalbetrieb       |<------------------| Diagnose ready  |
| (Tasks laufen)      |                   | BLE + Serial    |
+---------------------+                   +-----------------+
```

### 10.2 DTC-Verarbeitungs-Pipeline

```
Sensor-Task liest Wert
  |
  v
Wert plausibel?
  |
  +---> JA: DTC fuer diesen Sensor aktiv?
  |       |
  |       +---> JA: Erhoehe auto-clear-Zaehler
  |       |     Zaehler >= Schwellwert?
  |       |       |
  |       |       +---> JA: DTC status -> HISTORY
  |       |       |     Auto-Clear-Benachrichtigung
  |       |       |     Log-Eintrag (INFO)
  |       |       |
  |       |       +---> NEIN: nichts tun
  |       |
  |       +---> NEIN: nichts tun
  |
  +---> NEIN: DTC fuer diesen Sensor bereits aktiv?
          |
          +---> JA: Count++
          |     last_occurrence = now()
          |     NVS update (nur wenn count % 10 == 0)
          |     SD-Karte: dtc.log append
          |
          +---> NEIN: DTC neu setzen
                code, severity, first_occurrence = now()
                last_occurrence = now()
                count = 1
                status = ACTIVE
                NVS save (sofort)
                SD-Karte: dtc.log append
                BLE notify (DTC-List Characteristic)
                Display-Warnung (wenn severity >= WARN)
```

### 10.3 Self-Test-Pipeline

```
Trigger (Boot / Serial / BLE / App)
  |
  v
+-------------------+
| selftest_task     |
| setzt running=1   |
+--------+----------+
         |
    +----+----+----+----+----+----+----+----+
    |    |    |    |    |    |    |    |    |
    v    v    v    v    v    v    v    v    v
  [I2C][ADC][UART][SPI][SPI][BLE][BAT][MEM]
   scan read test  test test test read test
    |    |    |    |    |    |    |    |
    |    |    |    |    |    |    |    |
    v    v    v    v    v    v    v    v
  PASS PASS PASS FAIL PASS PASS WARN PASS
    |    |    |    |    |    |    |    |
    +----+----+----+----+----+----+----+----+
         |
         v
+-------------------+
| Ergebnisse        |
| sammeln           |
+--------+----------+
         |
         v
+--------+----------+
| DTCs setzen/      |
| loeschen          |
| (je Test-Ergebnis)|
+--------+----------+
         |
         v
+--------+----------+
| selftest_result   |
| -> NVS speichern  |
| -> BLE notify     |
| -> Serial output  |
+-------------------+
```

### 10.4 OTA-Update-Pipeline

```
Techniker / App
  |
  | "ota flash" oder App-Button
  v
+-----------------------------+
| OTA-Session starten         |
| Partition: inaktiv (ota_X)  |
| Handle holen                |
+-------------+---------------+
              |
              v
+-------------+---------------+
| Firmware-Daten empfangen    |
| - Serial: YMODEM            |
| - BLE: 240B Chunks          |
| - Schreiben mit esp_ota_write|
+-------------+---------------+
              |
              v
+-------------+---------------+
| esp_ota_end()               |
| Checksum verifizieren       |
+-------------+---------------+
              |
      +-------+-------+
      |               |
      v               v
   CHECKSUM OK?    CHECKSUM FAIL
      |               |
      v               v
+-----+-----+   +-----+-----------+
| esp_ota   |   | DTC D0301       |
| set_boot  |   | set_boot bleibt |
| partition |   | alte Firmware  |
+-----+-----+   +-----------------+
      |
      v
+-----+-----+
| Reboot    |
+-----+-----+
      |
      v
+-----+-----------+
| Neue Firmware   |
| startet         |
| state =         |
| PENDING_VERIFY  |
+-----+-----------+
      |
      +---> 60s kein Crash?
      |       |
      |       +---> JA: state = VALID
      |              (Rollback-Fenster geschlossen)
      |              DTC D0302 loeschen
      |
      |       +---> NEIN: Crash/Watchdog
      |              |
      v              v
+-----+-----+   +-----+-----------+
| Normal-  |   | Rollback        |
| betrieb  |   | -> alte Part.   |
+----------+   | DTC D0302 WARN  |
               +-----------------+
```

### 10.5 Graceful-Degradation-Pipeline

```
sensor_task / selftest_task
  |
  v
Modul-Check (I2C-Scan, ADC-Range, UART-Data)
  |
  +---> Modul PRESENT + plausible data
  |       |
  |       v
  |     Normalbetrieb
  |     Features aktiv
  |     DTC loeschen (falls warnt)
  |
  +---> Modul ABSENT (nicht gefunden)
  |       |
  |       v
  |     module.status = ABSENT
  |     on_absent() Callback:
  |       - Feature deaktivieren
  |       - Display-Hinweis setzen
  |       - DTC setzen (WARN/ERROR)
  |       - Log-Eintrag
  |     Rest laeuft weiter
  |
  +---> Modul FAULT (gefunden, aber Fehler)
          |
          v
        module.status = FAULT
        DTC setzen (ERROR)
        Retry-Logic:
          - Alle 60s neuer Versuch
          - Nach 3 Fehlversuchen: ABSENT
        Display-Hinweis
        Rest laeuft weiter
```

---

## 11. NVS-Speicherplanung

### 11.1 NVS-Namespace-Struktur

```
Namespace "config"      -> Konfiguration (display, log, ble, sensor)
Namespace "dtc"        -> DTC-Eintraege (max 32, 60 Bytes each = 1920 Bytes)
Namespace "calib"      -> Kalibrierungs-Daten (pro Sensor ~100 Bytes)
Namespace "maint"      -> Wartungs-Zaehler (hours, cycles, last_selftest)
Namespace "ota"        -> OTA-Status (current partition, rollback flag)
Namespace "ble_bond"   -> BLE Bonding-Keys (von NimBLE verwaltet)
```

### 11.2 NVS-Belegung

Mit 24KB NVS-Partition (0x6000 = 24576 Bytes):

| Namespace | Geschuetzt | Beschreibung |
|---|---|---|
| config | ~1KB | 25 Schluessel, avg 40 Bytes |
| dtc | ~2KB | 32 DTCs x 60 Bytes |
| calib | ~1KB | 6 Sensoren x ~100 Bytes + CRC |
| maint | ~100B | Stunden, Zyklen, Timestamps |
| ota | ~100B | Partitions-Status |
| ble_bond | ~4KB | Bonding-Keys (NimBLE) |
| Reserve | ~16KB | Freier Platz |

NVS hat ~100.000 Schreibzyklen. Die DTC-Speicherung wird nur bei Aenderung
geschrieben (nicht bei jedem Lesezyklus), was die Lebensdauer auf Jahrzehnte
verlaengert.

---

## 12. Implementierungs-Plan

### 12.1 Phase 1: DTC-Engine und Serial Console (Kern)

- `dtc.c / dtc.h`: DTC-Struktur, set/clear/query, NVS-Persistenz, SD-Log
- `diag_console.c`: USB-CDC-Task, Befehlsparser, help-System
- Befehle: `dtc list/clear`, `sensor read`, `help`, `version`, `reboot`
- Integration in `app_main`: Task starten, DTC-Engine initialisieren

### 12.2 Phase 2: Self-Test und Module-Erkennung

- `selftest.c`: Alle Test-Module (I2C, ADC, UART, SPI, BLE, Battery, Memory)
- `module.c`: Modul-Registry, Status-Tracking, on_absent Callbacks
- Befehle: `selftest`, `module list/info`
- Integration: Boot-Self-Test im Hintergrund

### 12.3 Phase 3: Log-System und Kalibrierung

- `log.c`: Ring-Buffer, Flush-Task, Log-Rotation, Log-Export
- `calibrate.c`: Pro-Sensor Kalibrierungs-Routinen, NVS-Speicherung, Export
- Befehle: `log tail/dump/rotate`, `calibrate list/export/reset`
- SD-Karten-Struktur anlegen

### 12.4 Phase 4: BLE-Diagnose-Service

- `ble_diag.c`: GATT-Service A005 mit allen Characteristics
- Notify-Mechanismus fuer DTC-Updates und Self-Test-Ergebnisse
- Log-Export-Stream ueber BLE
- Diagnose-Report als JSON

### 12.5 Phase 5: Companion-App Diagnose-Tab

- Flutter-Diagnose-Tab mit fuenf Unterseiten
- BLE-Kommunikation mit Diagnose-Service
- JSON-Report-Generierung und Teilen-Funktion
- OTA-Update-UI mit Fortschrittsbalken

### 12.6 Phase 6: Wartungs-Intervalle

- `maintenance.c`: Betriebsstunden-Zaehler, Ladezyklen, Wartungs-Erinnerungen
- NVS-Speicherung
- Display-Integration: Wartungs-Hinweise
- Befehle: `maintenance status/reset`

---

## 13. Quellen

- ESP-IDF NVS API: https://docs.espressif.com/projects/esp-idf/en/v5.4.4/esp32s3/api-reference/storage/nvs_flash.html
- ESP-IDF OTA API: https://docs.espressif.com/projects/esp-idf/en/v5.4.4/esp32s3/api-reference/system/ota.html
- NimBLE GATT Services: https://mynewt.apache.org/latest/tutorials/ble/ble_svc_gatt/ble_svc_gatt.html
- OBD-II DTC-Format Referenz: https://en.wikipedia.org/wiki/OBD-II_PIDs
- YMODEM-Protokoll: https://en.wikipedia.org/wiki/YMODEM
- FreeRTOS Task-Design: https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/01-Tasks-and-co-routines
- CRC32-Implementierung: https://docs.espressif.com/projects/esp-idf/en/v5.4.4/esp32s3/api-reference/system/misc_system_api.html#crc
- ESP32-S3 ADC-Calibration: https://docs.espressif.com/projects/esp-idf/en/v5.4.4/esp32s3/api-reference/peripherals/adc_calibration.html
- I2C-Scan Implementierung: https://github.com/espressif/esp-idf/tree/master/examples/peripherals/i2c/i2c_self_test

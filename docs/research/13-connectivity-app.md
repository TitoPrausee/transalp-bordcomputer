# Connectivity & Companion-App: Nahtlose Smartphone-Integration

> Stand: 13.10.2026
> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986-1999, rein mechanisch, keine ECU
> Plattform: ESP32-S3 (siehe docs/research/03-software-stack.md)
> Vorherige Dokumente: 02-hardware-deep-dive, 03-software-stack, 04-pd06-technical-data, 05-enclosure-env, 06-feature-roadmap, 07-display-navigation-smartphone

---

## 0. Einleitung

Das Dokument 07-display-navigation-smartphone.md hat die grundlegenden Designentscheidungen fuer Displays und das Prinzip der Companion-App-Loesung beschrieben: Statt einer Bildschirm-Spiegelung wie bei geschlossenen Protokollen der Geraetehersteller wird eine eigene App entwickelt, die als Datenvermittler zwischen Smartphone-Diensten und Bordcomputer fungiert. Die Datenuebertragung erfolgt per BLE-GATT.

Dieses Dokument geht die volle Tiefe: Es definiert die BLE-Verbindungsarchitektur mit State-Machine und Auto-Reconnect, die vollstaendige GATT-Service-Definition mit allen UUIDs, Characteristics, Properties und Descriptors, das Datenuebertragungs-Protokoll fuer kleine bis grosse Datenmengen, die Companion-App-Architektur mit allen Schichten und Screens, und die Integration von Navigation, Notifications, Medien und Wetter. Zuletzt werden Robustheit und Graceful Degradation bei Verbindungsabbruch und schlechter Verbindung behandelt.

Die Verbindung soll so zuverlaessig und einfach sein wie bei Premium-Konsumenten-Elektronik: einmal gekoppelt, immer verbunden, nahtlose Uebergaenge, kein manueller Eingriff noetig. Das bedeutet, dass die ganze Komplexitaet der Verbindung in der Software liegen muss, nicht im Bedienkonzept.

---

## 1. BLE-Verbindungsarchitektur

### 1.1 Connection-State-Machine

Die BLE-Verbindung zwischen Bordcomputer (GATT-Server) und Smartphone (GATT-Client) durchlaeuft eine definierte State-Machine mit zwoelf Zustaenden. Die State-Machine ist auf beiden Seiten implementiert: im BLE-Layer des Bordcomputers (ESP32-S3, C/FreeRTOS) und im BLE-Layer der Companion-App.

```
                        +---------------+
                        |  UNBONDED     |
                        |  (kein Key)   |
                        +-------+-------+
                                |
                          Pairing-Request
                                |
                                v
                        +-------+-------+
                        |  BONDED       |
                        |  (Key stored) |
                        +-------+-------+
                                |
                    Zündung AN / App-Start
                                |
                                v
    +---------------+   +-------+-------+
    |  DISCONNECTED |<--+  SCANNING     |
    |  (idle)       |   |  (advertising |
    +-------+-------+   |   + scanning)  |
            ^           +-------+-------+
            |                   |
            | timeout 5s        | Device found
            |                   v
            |           +-------+-------+
            |           |  CONNECTING   |
            |           |  (LL-Connect) |
            |           +-------+-------+
            |                   |
            |                   | GATT-Discovery
            |                   v
            |           +-------+-------+
            |           |  DISCOVERING  |
            |           |  (Services)   |
            |           +-------+-------+
            |                   |
            |                   | MTU-Exchange
            |                   v
            |           +-------+-------+
            |           |  AUTHENTIC-   |
            |           |  ATING        |
            |           |  (Encrypt LL) |
            |           +-------+-------+
            |                   |
            |                   | Encrypted
            |                   v
            |           +-------+-------+
            +-----------|  CONNECTED    |
            |           |  (operational)|
            |           +-------+-------+
            |                   |
            |                   | RSSI < -90 dBm
            |                   v
            |           +-------+-------+
            +-----------|  DEGRADING    |
            |           |  (reduced)    |
            |           +-------+-------+
            |                   |
            |                   | Link Loss
            |                   v
            |           +-------+-------+
            +-----------|  RECONNECTING |
                        |  (backoff)    |
                        +-------+-------+
```

Die Zustaende im Detail:

**UNBONDED** — Der initiale Zustand. Der Bordcomputer hat keine gespeicherten Pairing-Keys. Advertising ist aktiv. Die App muss einen Pairing-Prozess ausloesen (siehe 1.4 Bonding).

**BONDED** — Pairing-Keys sind auf beiden Seiten persistent gespeichert. Der Bordcomputer weiss, welches Telefon erlaubt ist. Beim naechsten Start springt die State-Machine direkt von DISCONNECTED zu SCANNING, ohne erneut zu paaren.

**DISCONNECTED** — Keine aktive Verbindung. Der Bordcomputer ist im Advertising-Modus und wartet auf einen Connect-Versuch der App. Timeout von 5 Sekunden ohne Verbindung -> Ruecksprung in einen节能-Advertising-Modus mit laengerem Intervall.

**SCANNING** — Die App sucht aktiv nach dem Bordcomputer per BLE-Scan. Der Bordcomputer advertiset. Sobald die App das Advertising-Paket des Bordcomputers findet (gefiltert nach Service-UUID), wird zum CONNECTING-Zustand uebergegangen.

**CONNECTING** — Die LL-Connect-Request wurde gesendet. Der Link-Layer-Handshake findet statt. Dieser Zustand dauert typischerweise 50-200 ms. Bei Timeout (3 Sekunden) Ruecksprung zu SCANNING mit Backoff.

**DISCOVERING** — Die physische Verbindung steht. Die App fuehrt GATT-Discovery durch: alle Services, Characteristics und Descriptors werden enumeriert. Bei bekanntem GATT-Layout kann dieser Schritt beschleunigt werden, indem nur die bekannten UUIDs abgefragt werden (Service-Changed-Indication pruefen).

**AUTHENTICATING** — Die verschluesselte Verbindung wird aufgebaut. Bei vorhandenem Bonding-Key wird die Encryption mit dem gespeicherten LTK (Long Term Key) aktiviert. Ohne Bonding-Key wird ein neuer Pairing-Prozess ausgeloest (LE Secure Connections, Passkey oder Just-Works, je nach Konfiguration).

**CONNECTED** — Vollstaendig etablierte, verschluesselte, MTU-ausgehandelte Verbindung. Alle Services sind operativ. Die App abonniert alle relevanten Notify-Characteristics. Telemetrie- und Nav-Daten fliessen.

**DEGRADING** — Die Verbindung steht, aber die Signalqualitaet ist schlecht (RSSI < -90 dBm oder Packet-Loss > 10%). Die Uebertragungsrate wird reduziert: nur noch prioritaete Daten (Navigation, Anrufe), keine Telemetrie-Updates, keine Wetter-Daten. Siehe Abschnitt 9.

**RECONNECTING** — Die Verbindung wurde verloren. Die App versucht automatisch, die Verbindung wiederherzustellen, mit exponentiellem Backoff (1s, 2s, 4s, 8s, max 30s). Der Bordcomputer schaltet in den Advertising-Modus zurueck.

### 1.2 Auto-Reconnect

Der Auto-Reconnect ist das wichtigste Robustheits-Feature. Er muss in zwei Szenarien funktionieren:

**Szenario A: Kurzer Verbindungsabbruch (Tunnel, Bruecke, kurze Stoerung)**

Die App bemerkt den Verbindungsverlust innerhalb von 5 Sekunden (Supervision-Timeout). Sie startet sofort einen Reconnect-Versuch. Der Bordcomputer ist bereits im Advertising-Modus. Bei kurzer Stoerung (< 10s) ist die Verbindung typischerweise innerhalb von 3-5 Sekunden wieder hergestellt. Die App merkt sich den letzten bekannten Zustand (letzte Nav-Daten, letzte Media-Info) und zeigt diese weiter an, bis neue Daten eintreffen.

**Szenario B: Laengerer Verbindungsabbruch (Smartphone ausser Reichweite, Bluetooth deaktiviert)**

Die App versucht mit exponentiellem Backoff zu reconnecten. Nach 5 fehlgeschlagenen Versuchen (ca. 30 Sekunden) wird der Reconnect auf einen Intervall von 30 Sekunden reduziert, um Akku zu sparen. Wenn Bluetooth wieder verfuegbar ist (z.B. Telefon kommt zurueck in Reichweite), erkennt die App das Advertising des Bordcomputers beim naechsten Scan und stellt die Verbindung automatisch her.

Wichtig: Der Reconnect muss vollstaendig transparent fuer den Nutzer sein. Es gibt keinen Button "Verbinden". Die Verbindung kommt und geht, ohne dass der Fahrer eingreifen muss. Die einzige sichtbare Rueckmeldung ist ein kleines Verbindungssymbol auf dem Display (verbunden / nicht verbunden / schwach).

### 1.3 Connection-Parameters

BLE Connection-Parameters definieren, wie oft die beiden Geraete Daten austauschen koennen. Sie werden beim Verbindungsaufbau ausgehandelt. Der Bordcomputer (als Server) kann Preferred-Connection-Parameters advertisen, aber die App (als Client) muss sie aktiv setzen.

Fuer den Motorrad-Einsatz sind die Parameter auf schnelle Navigation-Updates optimiert:

| Parameter | Wert | Begruendung |
|---|---|---|
| Connection Interval | 15-30 ms | Schnelle Nav-Updates alle 1-2 Sekunden, aber auch Telemetrie-Notify alle 250 ms moeglich. 15 ms = maximale Reaktionszeit, 30 ms = Stromspar-Kompromiss |
| Slave Latency | 0 | Keine Latenz-Schoenung. Jedes Connection-Event muss genutzt werden, weil Telemetrie und Nav-Daten zeitkritisch sind. Latency > 0 wuerde Notifications verzoegern |
| Supervision Timeout | 5 s | Schneller Disconnect bei echtem Verbindungsabbruch. 5 s sind kurz genug, um nicht ewig auf eine tote Verbindung zu warten, aber lang genug, um kurzzeitige Stoerungen nicht als Abbruch zu werten |

Die App setzt diese Parameter nach erfolgreichem Verbindungsaufbau per `requestConnectionPriority` (oder aequivalenter API). Der Bordcomputer kann Connection-Parameter-Update-Requests senden, aber die App hat das letzte Wort.

Zusaetzlich kann der Bordcomputer eine Datenlaenge von 251 Bytes (LE Data Length Extension) anbieten, was den Throughput bei grossen Transfers erhoet. Die App sollte `requestMtu(247)` aufrufen (siehe auch Dokument 07, Abschnitt 2.3).

### 1.4 Bonding

Bonding ist der Prozess, bei dem Pairing-Keys persistent gespeichert werden, sodass nach dem ersten Pairing keine erneute Authentifizierung noetig ist.

**Pairing-Methode:** LE Secure Connections (Bluetooth 4.2+). Die ESP32-S3 unterstuetzt LE Secure Connections nativ. Als Pairing-Methode wird Numeric Comparison verwendet (beide Geraete zeigen eine 6-stellige Zahl, Nutzer bestaetigt). Das ist sicherer als Just-Works und benoetigt kein Display/Keyboard am Bordcomputer (die 6-stellige Zahl wird auf dem Bordcomputer-Display angezeigt, die Bestaetigung erfolgt ueber den Lenker-Taster).

**Key-Speicherung:**

Auf dem Bordcomputer werden die Bonding-Keys im NVS (Non-Volatile Storage) des ESP32-S3 gespeichert. Der ESP-IDF BLE-Stack verwaltet dies automatisch mit `esp_ble_gap_set_security_param`. Bei Zündung aus (Strom weg) bleiben die Keys erhalten, da NVS im Flash gespeichert ist.

Auf dem Smartphone werden die Keys vom Betriebssystem-BLE-Stack verwaltet. Die App selbst muss keine Keys speichern, sondern verlaesst sich auf die System-Bonding-Tabelle.

**Loeschen von Bonds:** Ueber den Config-Service (Characteristic 0x6005, Bond-Management) kann die App alle gespeicherten Bonds auf dem Bordcomputer loeschen. Das ist noetig, wenn der Fahrer das Telefon wechselt oder den Bordcomputer zuruecksetzt.

### 1.5 Multi-Phone-Support

Kann das System zwischen mehreren Handys wechseln? Ja, mit Einschraenkungen.

Der ESP32-S3 BLE-Stack unterstuetzt das Speichern mehrerer Bonding-Keys. Die maximale Anzahl haengt von der NVS-Partition ab, typischerweise 4-10 Geraete. In der Praxis sind 3 Geraete ausreichend: Haupttelefon (Fahrer), Zweitgeraet (Werkstatt/Diagnose), und ein Reserve.

**Wechsel-Logik:**

1. Wenn kein Telefon verbunden ist, advertiset der Bordcomputer.
2. Die App des verfuegbaren Telephons findet das Advertising, connectet, encrypted mit gespeichertem Key.
3. Wenn zwei Telefone gleichzeitig in Reichweite sind, gewinnt das, das zuerst connectet.
4. Ein "Aktives Telefon wechseln" ist nicht moeglich, waehrend eine Verbindung steht. Der Fahrer muss die aktuelle Verbindung auf dem aktiven Telefon trennen (Bluetooth am Bordcomputer aus- und einschalten via Config-Service, oder App beenden), damit das andere Telefon connecten kann.

**Einschraenkung:** Der Bordcomputer kann nicht zwei Telefone gleichzeitig verbinden. BLE-GATT-Server auf dem ESP32-S3 unterstuetzt zwar mehrere gleichzeitige Verbindungen, aber die Companion-App-Architektur ist auf eine 1:1-Verbindung ausgelegt (ein Bordcomputer, ein Telefon). Mehrere gleichzeitige Verbindungen wuerden die Bandbreite halbieren und die State-Machine deutlich komplexer machen. Das ist kein realer Use-Case fuer einen einzelnen Fahrer.

---

## 2. GATT-Service-Definition

### 2.1 UUID-Schema

Alle Custom-Services und Characteristics verwenden einen projekt-spezifischen 128-bit Base-UUID:

```
6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e
```

Die 16-bit-ID (`XXXX`) identifiziert den Service oder die Characteristic. Services haben IDs im Bereich 0x1000-0x8000 (jeweils 0x1000 Schritte). Characteristics innerhalb eines Services haben IDs im Bereich Service-ID + 0x01 bis Service-ID + 0x0F.

Standard-Services (Generic Access 0x1800, Device Information 0x180A) verwenden Bluetooth-SIG-Standard-UUIDs und werden nicht in der Custom-Tabelle gefuehrt.

### 2.2 GATT-Service-Tabelle

Die folgende Tabelle definiert alle acht Custom-Services mit ihren Characteristics, Properties und Descriptors.

#### Service: Navigation (0x1000)

| Handle | UUID (16-bit ID) | Characteristic | Properties | Descriptor | Payload |
|---|---|---|---|---|---|
| 0x0101 | 0x1001 | Turn-by-Turn Maneuver | Read, Notify | CCCD | Siehe Msg-Format NAV_MANEUVER |
| 0x0102 | 0x1002 | Route Info | Read, Notify | CCCD | Siehe Msg-Format NAV_ROUTE |
| 0x0103 | 0x1003 | Current Road | Read, Notify | CCCD | Siehe Msg-Format NAV_ROAD |
| 0x0104 | 0x1004 | Navigation Status | Read, Notify | CCCD | Siehe Msg-Format NAV_STATUS |
| 0x0105 | 0x1005 | Nav Command (write) | Write, Write Without Response | — | Siehe Msg-Format NAV_CMD |

CCCD = Client Characteristic Configuration Descriptor (Standard 0x2902, 2 Bytes, zum Aktivieren/Deaktivieren von Notifications).

**Turn-by-Turn Maneuver (0x1001)** — Enthaelt das naechste Maneuver: Maneuver-Typ (geradeaus, links, rechts, scharf links, etc.), Distanz zum Maneuver (in Metern), und Strassenname (UTF-8, max 64 Bytes). Wird alle 1-2 Sekunden von der App notifiziert.

**Route Info (0x1002)** — Gesamtstrecke (km), Reststrecke (km), ETA (Unix-Timestamp), Restzeit (Sekunden). Wird alle 10 Sekunden oder bei Aenderung notifiziert.

**Current Road (0x1003)** —Aktueller Strassenname und Tempolimit (km/h, 0 = unbekannt). Wird bei Strassenwechsel notifiziert.

**Navigation Status (0x1004)** — Status: inactive / active / rerouting / arriving / offline. Notify bei Statuswechsel.

**Nav Command (0x0105, Write)** — Bordcomputer -> App. Befehle wie "Route abbrechen" (0x01), "Naechstes Maneuer wiederholen" (0x02), "Alternative Route" (0x03).

#### Service: Media (0x2000)

| Handle | UUID (16-bit ID) | Characteristic | Properties | Descriptor | Payload |
|---|---|---|---|---|---|
| 0x0201 | 0x2001 | Now Playing | Read, Notify | CCCD | Siehe Msg-Format MEDIA_NOWPLAYING |
| 0x0202 | 0x2002 | Media Control | Write, Write Without Response | — | Siehe Msg-Format MEDIA_CMD |
| 0x0203 | 0x2003 | Volume | Read, Notify, Write | CCCD | uint8 (0-100) |

**Now Playing (0x2001)** — Artist (UTF-8, max 64 Bytes), Title (UTF-8, max 64 Bytes), Duration (uint32, Sekunden), Position (uint32, Sekunden), Playback-State (0=stopped, 1=playing, 2=paused). Notify alle 5 Sekunden oder bei Titelwechsel.

**Media Control (0x2002, Write)** — Bordcomputer -> App. Befehle: Play/Pause (0x01), Next (0x02), Previous (0x03), Seek Forward 10s (0x04), Seek Backward 10s (0x05). Die App setzt dies in die entsprechenden System-Media-APIs um.

**Volume (0x2003)** — System-Lautstaerke (0-100). Bidirektional: App notifiziert bei Aenderung, Bordcomputer kann schreiben (Lenkertaster -> Lauter/Leiser). Bei 0 = Stumm.

#### Service: Phone (0x3000)

| Handle | UUID (16-bit ID) | Characteristic | Properties | Descriptor | Payload |
|---|---|---|---|---|---|
| 0x0301 | 0x3001 | Incoming Call | Notify | CCCD | Siehe Msg-Format PHONE_CALL |
| 0x0302 | 0x3002 | Call Status | Read, Notify | CCCD | uint8 (0=idle, 1=ringing, 2=active, 3=ended) |
| 0x0303 | 0x3003 | Call Control | Write | — | Siehe Msg-Format PHONE_CMD |
| 0x0304 | 0x3004 | Notification | Notify | CCCD | Siehe Msg-Format PHONE_NOTIF |

**Incoming Call (0x3001)** — Caller-ID (UTF-8, max 64 Bytes, Telefonnummer oder Kontaktname), Call-Type (0=voice, 1=video). Notify bei eingehendem Anruf.

**Call Status (0x3002)** — Aktueller Anrufstatus. Notify bei jeder Statusaenderung.

**Call Control (0x0303, Write)** — Bordcomputer -> App. Befehle: Accept (0x01), Reject (0x02), Hang Up (0x03). Die App leitet dies an die Telefonie-API des Betriebssystems weiter. Annahme ueber Bordcomputer-Taster ist moeglich (Lenkertaster -> Accept/Reject), Ablehnung ebenfalls.

**Notification (0x0304)** — App -> Bordcomputer. App-Name (UTF-8, max 32 Bytes), Titel (UTF-8, max 64 Bytes), Body (UTF-8, max 100 Bytes, gekuerzt). Notify bei neuer Notification, die den Filter passiert (siehe Abschnitt 6).

#### Service: Weather (0x4000)

| Handle | UUID (16-bit ID) | Characteristic | Properties | Descriptor | Payload |
|---|---|---|---|---|---|
| 0x0401 | 0x4001 | Current Weather | Read, Notify | CCCD | Siehe Msg-Format WX_CURRENT |
| 0x0402 | 0x4002 | Forecast | Read, Notify | CCCD | Siehe Msg-Format WX_FORECAST |
| 0x0403 | 0x4003 | Weather Alert | Notify | CCCD | Siehe Msg-Format WX_ALERT |

**Current Weather (0x4001)** — Temperatur (int16, Zehntelgrad Celsius), Luftfeuchte (uint8, Prozent), Bedingung (uint8: 0=clear, 1=cloudy, 2=overcast, 3=rain, 4=heavy_rain, 5=thunderstorm, 6=snow, 7=fog), Windgeschwindigkeit (uint8, km/h), Fuehlt-wie (int16, Zehntelgrad). Notify alle 15 Minuten.

**Forecast (0x4002)** — 6-Stunden-Vorhersage als kompaktes Array: 6 Eintraege a 4 Bytes (Stunde + Bedingung + Temp_min + Temp_max). Insgesamt 24 Bytes. Notify alle 30 Minuten oder bei signifikanter Aenderung.

**Weather Alert (0x0403)** — Alert-Typ (uint8: 0=rain_approaching, 1=storm, 2=frost, 3=heat, 4=wind), Severity (uint8: 0=info, 1=warning, 2=severe), Zeit bis Eintritt (uint16, Minuten). Notify bei neuer Warnung.

#### Service: Telemetry (0x5000)

| Handle | UUID (16-bit ID) | Characteristic | Properties | Descriptor | Payload |
|---|---|---|---|---|---|
| 0x0501 | 0x5001 | Live Telemetry | Notify | CCCD | Siehe Msg-Format TLM_LIVE |
| 0x0502 | 0x5002 | Telemetry Snapshot | Read | — | Siehe Msg-Format TLM_SNAPSHOT |
| 0x0503 | 0x5003 | Track Export | Notify (Write-to-Trigger via 0x5004) | CCCD | Binary Transfer (siehe 3.3) |
| 0x0504 | 0x5004 | Track Export Control | Write | — | Siehe Msg-Format TLM_TRACK_CMD |

**Live Telemetry (0x5001)** — Bordcomputer -> App. Kompaktes Datenpaket mit allen Live-Werten: Speed (uint8, km/h), RPM (uint16, U/min), Gear (int8, -1=neutral), Coolant Temp (int8, Celsius), Ambient Temp (int8, Celsius), Fuel (uint8, Prozent), Voltage (uint8, Zehntelvolt), Latitude (int32, E-7), Longitude (int32, E-7), Altitude (int16, Meter), Heading (uint16, Zehntelgrad), Satellites (uint8), HDOP (uint8, Zehntel). Insgesamt 28 Bytes. Notify alle 250 ms (4 Hz).

**Telemetry Snapshot (0x5002)** — Vollstaendiger Snapshot aller konfigurierten Sensoren, inklusive tripmeter, ODO, GPS-Fix-Qualitaet, Sensor-Health. Read-on-Demand, ca. 60 Bytes. Die App liest diesen bei (Re-)Connect, um sofort einen vollstaendigen Zustand zu haben, ohne auf die naechsten Live-Notify-Pakete zu warten.

**Track Export (0x5003)** — Grosser Binary-Transfer (GPX-Datei). Wird ueber das Transfer-Protokoll (Abschnitt 3.3) abgewickelt. Die App schreibt einen Befehl an 0x5004, der Bordcomputer beginnt, die GPX-Datei chunked ueber Notify 0x5003 zu senden.

**Track Export Control (0x0504, Write)** — App -> Bordcomputer. Befehle: Start Export (0x01, gefolgt von Datum als uint32 Unix-Timestamp), Cancel Export (0x02), Get Export Status (0x03).

#### Service: Config (0x6000)

| Handle | UUID (16-bit ID) | Characteristic | Properties | Descriptor | Payload |
|---|---|---|---|---|---|
| 0x0601 | 0x6001 | Config Read | Read, Notify | CCCD | JSON-Chunk (siehe 3.2) |
| 0x0602 | 0x6002 | Config Write | Write | — | JSON-Chunk (siehe 3.2) |
| 0x0603 | 0x6003 | Config Export | Notify (Binary Transfer) | CCCD | Binary Transfer |
| 0x0604 | 0x6004 | Config Import | Write (Binary Transfer) | — | Binary Transfer |
| 0x0605 | 0x6005 | Bond Management | Write | — | uint8 (0x00=delete all bonds) |

**Config Read/Write (0x6001/0x6002)** — Die Konfiguration des Bordcomputers wird als JSON repraesentiert (siehe 06-feature-roadmap, Config-Modell). Da die JSON-Representation typischerweise 1-3 KB gross ist, wird sie ueber das Chunked-Transfer-Protokoll (Abschnitt 3.2) uebertragen. Write setzt einen `Transfer-ID` und sendet Chunks, der Bordcomputer validiert und uebernimmt atomar.

**Config Export/Import (0x6003/0x6004)** — Vollstaendige Konfiguration als Binary-Blob exportieren/importieren. Dies ist ein Binary-Transfer (Abschnitt 3.3), der die komplette NVS-Config-Partition umfasst. Nuetzlich fuer Backup/Restore oder Uebertragung auf einen anderen Bordcomputer.

**Bond Management (0x6005)** — Loescht alle gespeicherten Bonding-Keys auf dem Bordcomputer. Schreiben von 0x00 loescht alle Bonds. Erfordert verschluesselte Verbindung (nur ein gebondetes Telefon kann dies tun). Nach dem Loeschen startet der Bordcomputer neu.

#### Service: Diagnose (0x7000)

| Handle | UUID (16-bit ID) | Characteristic | Properties | Descriptor | Payload |
|---|---|---|---|---|---|
| 0x0701 | 0x7001 | DTC List | Read, Notify | CCCD | Siehe Msg-Format DIAG_DTC |
| 0x0702 | 0x7002 | Sensor Status | Read, Notify | CCCD | Siehe Msg-Format DIAG_SENSOR |
| 0x0703 | 0x7003 | Self-Test | Write, Notify | CCCD | Siehe Msg-Format DIAG_SELFTEST |
| 0x0704 | 0x7004 | Log Export | Notify (Binary Transfer) | CCCD | Binary Transfer |
| 0x0705 | 0x7005 | Log Export Control | Write | — | Siehe Msg-Format DIAG_LOG_CMD |

**DTC List (0x7001)** — Diagnostic Trouble Codes. Array von DTC-Eintraegen, jeweils 6 Bytes: Code (uint16), Severity (uint8: 0=info, 1=warning, 2=critical), Timestamp (uint32 Unix). Notify bei neuem DTC.

**Sensor Status (0x7002)** — Status aller Sensoren: GPS (0=ok, 1=no_fix, 2=error), RPM-Sensor (0=ok, 1=no_signal, 2=error), Temp-Sensor (0=ok, 1=disconnected, 2=error), Fuel-Sensor (0=ok, 1=error), SD-Card (0=ok, 1=not_present, 2=full, 3=error). 5 Bytes. Notify bei Statusaenderung.

**Self-Test (0x0703)** — App -> Bordcomputer: Write 0x01 startet Self-Test. Bordcomputer -> App: Notify mit Ergebnis (uint8: 0=pass, 1=warn, 2=fail), gefolgt von Detail-Bytes fuer jeden Sensor.

**Log Export (0x0704)** — System-Logs als Binary-Transfer. Die App triggert ueber 0x0705, der Bordcomputer sendet die Log-Datei chunked.

#### Service: OTA (0x8000)

| Handle | UUID (16-bit ID) | Characteristic | Properties | Descriptor | Payload |
|---|---|---|---|---|---|
| 0x0801 | 0x8001 | OTA Control | Write, Notify | CCCD | Siehe Msg-Format OTA_CTRL |
| 0x0802 | 0x8002 | OTA Data | Write Without Response | — | Binary Transfer (siehe 3.3) |
| 0x0803 | 0x8003 | OTA Verify | Notify | CCCD | Siehe Msg-Format OTA_VERIFY |

**OTA Control (0x8001)** — App -> Bordcomputer: Write mit Befehl (0x01=begin, 0x02=commit, 0x03=abort). Bordcomputer -> App: Notify mit Status (0x00=idle, 0x01=receiving, 0x02=verifying, 0x03=ready_to_commit, 0x04=committed, 0x80=error, gefolgt von Error-Code).

**OTA Data (0x8002)** — Firmware-Image als Binary-Transfer. Write Without Response fuer maximalen Throughput (kein ACK pro Paket, Retry auf hoeherer Ebene). Siehe Abschnitt 3.3 fuer das Binary-Transfer-Protokoll.

**OTA Verify (0x8003)** — Nach Abschluss des Transfers sendet der Bordcomputer das Verifikationsergebnis: SHA-256-Hash (32 Bytes) des empfangenen Images, groesse (uint32), und Status (0x00=ok, 0x01=hash_mismatch, 0x02=too_large, 0x03=invalid_magic). Die App vergleicht den Hash mit dem erwarteten und sendet Commit (0x02 an 0x8001), wenn sie uebereinstimmen.

---

## 3. Datenuebertragungs-Protokoll

### 3.1 Klein-Daten: Direct GATT-Write/Notify (< 20 Bytes)

Fuer Daten unter 20 Bytes (oder genauer: unter MTU-3 Bytes) wird der direkte GATT-Mechanismus verwendet. Die App schreibt per `writeCharacteristic` oder der Bordcomputer notifiziert per `esp_ble_gatts_send_indication`.

Beispiele:
- Volume-Aenderung: 1 Byte
- Media-Control-Befehl: 1 Byte
- Call-Control-Befehl: 1 Byte
- Nav-Status: 1 Byte

Diese Daten brauchen kein Framing, keine Sequence-Number, kein CRC. Der BLE-Link-Layer gewaehrleistet Integritaet und Reihenfolge bereits.

### 3.2 Mittel-Daten: Chunked Write (20-500 Bytes)

Fuer Daten zwischen 20 und 500 Bytes wird ein einfaches Chunked-Transfer-Protokoll verwendet. Dies betrifft hauptsaechlich die Config-Read/Write-Characteristics und kurze Telemetrie-Snapshots.

**Message-Format:**

```
Byte 0:     Transfer-ID (uint8, fortlaufend, identifiziert den Transfer)
Byte 1:     Flags (bit 0: more-flag, 1=more chunks follow, 0=last chunk)
Byte 2:     Sequence-Number (uint8, 0-basiert, wrap bei 255)
Byte 3-4:   Chunk-Length (uint16, Little-Endian, 0-244 Bytes)
Byte 5+:    Payload (Chunk-Length Bytes)
```

Der Sender (App oder Bordcomputer) teilt die Daten in Chunks von max 244 Bytes (bei MTU 247). Jeder Chunk bekommt eine Sequence-Number. Der Empfaenger sammelt alle Chunks und reassembliert die komplette Nachricht, wenn das More-Flag 0 ist.

**Fehlerbehandlung:** Bei fehlender Sequence-Number (Luecke) sendet der Empfaenger eine Negative-Acknowledgement-Nachricht an die jeweilige Control-Characteristic. Der Sender wiederholt den fehlenden Chunk. Nach 3 fehlgeschlagenen Versuchen wird der Transfer abgebrochen und vom Empfaenger verworfen.

**Timeout:** Wenn 5 Sekunden kein Chunk fuer eine Transfer-ID empfangen wird, wird der Transfer abgebrochen.

Beispiel: Config-JSON mit 1200 Bytes wird in 5 Chunks a 244 Bytes (plus ein letzter mit 20 Bytes) uebertragen. Bei MTU 247 und 30 ms Connection-Interval dauert das ca. 150 ms.

### 3.3 Gross-Daten: BLE-Transfer-Protokoll (> 500 Bytes)

Fuer Daten ueber 500 Bytes (Track-Export, Log-Export, Firmware-Image, Config-Binary-Export/Import) wird ein robustes Binary-Transfer-Protokoll verwendet. Dieses Protokoll ist auf der GATT-Ebene aufgesetzt und bietet CRC, Retry und Progress-Tracking.

**Protokoll-Header (7 Bytes, im ersten Chunk jedes Transfers):**

```
Byte 0-1:   Magic (0xBC, 0x01 — Protokoll-Version 1)
Byte 2-5:   Total-Length (uint32, Little-Endian, Gesamtgroesse in Bytes)
Byte 6:     Transfer-Type (0x01=Track, 0x02=Log, 0x03=Firmware, 0x04=Config)
```

**Chunk-Format (jeder Chunk inkl. Header-Chunk):**

```
Byte 0-1:   Transfer-ID (uint16, identifiziert den Transfer)
Byte 2-3:   Sequence-Number (uint16, 0-basiert, wrap bei 65535)
Byte 4:     Flags (bit 0: more, bit 1: header-follows, bit 2: retransmit)
Byte 5-6:   Chunk-Length (uint16, 0-244)
Byte 7-8:   CRC-16 (CCITT, ueber Byte 0-6 + Payload)
Byte 9+:    Payload (Chunk-Length Bytes)
```

Jeder Chunk ist 9 Bytes Header + max 244 Bytes Payload = max 253 Bytes. Bei MTU 247 passt der Chunk in eine Notification (9 + 238 Payload = 247).

**Ablauf:**

1. **Initiation:** Die App schreibt einen Start-Befehl an die jeweilige Control-Characteristic (z.B. Track Export Control 0x5004). Der Bordcomputer reserviert eine Transfer-ID und beginnt, Chunks ueber die Notify-Characteristic (z.B. 0x5003) zu senden.

2. **Uebertragung:** Der Bordcomputer sendet Chunks ohne auf ACK zu warten (Sliding-Window mit Window-Size 8). Die App bestaetigt empfangene Chunks, indem sie ACK-Pakete an die Control-Characteristic schreibt:

```
ACK-Format:
Byte 0:     0xFF (ACK-Marker)
Byte 1-2:   Transfer-ID
Byte 3-4:   Last-Sequence (hoechste zusammenhaengend empfangene Seq-Num)
```

3. **Retransmission:** Wenn der Bordcomputer ein ACK mit einer Sequence-Number erhaelt, die hinter seinem Sendestand liegt, sendet er alle Chunks ab Last-Sequence+1 erneut (Flag bit 2 = retransmit gesetzt).

4. **Completion:** Wenn der letzte Chunk (More-Flag=0) empfangen und per ACK bestaetigt wurde, sendet der Bordcomputer ein Final-Notify mit CRC-32 ueber die gesamten Daten:

```
Final-Notify:
Byte 0:     0xFE (Completion-Marker)
Byte 1-2:   Transfer-ID
Byte 3-6:   CRC-32 (ueber alle reassemblierten Daten)
Byte 7-10:  Total-Bytes (uint32)
```

Die App verifiziert CRC-32 und markiert den Transfer als abgeschlossen.

5. **Abbruch:** Beide Seiten koennen den Transfer abbrechen, indem sie 0x00 an die Control-Characteristic schreiben. Alle Ressourcen werden freigegeben.

### 3.4 Bandbreiten-Schaetzung

Die praktische BLE-Bandbreite haengt von MTU, Connection-Interval und Overhead ab.

Bei optimierten Parametern (MTU 247, Connection-Interval 15 ms, LE Data Length Extension 251 Bytes):

- Payload pro Notification: 244 Bytes
- Notifications pro Connection-Interval: 1-4 (begrenzt durch Link-Layer- Kapazitaet)
- Theoretisch: 244 * 4 / 0.015 = ca. 65 kB/s
- Praxis: ca. 20-40 kB/s (overhead, Inter-Frame-Space, Processing)

Reale Praxis-Werte (gemessen an aehnlichen ESP32-S3 Setups):

| Szenario | Bandbreite | Use-Case |
|---|---|---|
| MTU 247, 15 ms Interval, Write NoResp | 20-40 kB/s | Firmware-OTA, Track-Export |
| MTU 247, 30 ms Interval, Notify | 8-16 kB/s | Log-Export, Config |
| MTU 185, 30 ms Interval, Notify | 5-10 kB/s | Standard-Betrieb |
| MTU 23 (Default), 30 ms | 1-2 kB/s | Worst-Case ohne MTU-Aushandlung |

**Firmware-Update (300 kB Image):**
Bei 30 kB/s dauert ein Firmware-Update ca. 10 Sekunden. Bei 15 kB/s ca. 20 Sekunden. Das ist voll akzeptabel fuer ein OTA-Update.

**Track-Export (24h Tour, 1 Hz GPS, ca. 86.400 Punkte):**
Ein GPX-Track-Point ist ca. 80-120 Bytes. Bei 100 Bytes * 86.400 = ca. 8.6 MB. Bei 20 kB/s dauert der Export ca. 7 Minuten. Das ist lang, aber akzeptabel, da Track-Export nicht zeitkritisch ist (passiert nach der Tour). Alternativ kann der Track direkt auf der SD-Karte als GPX gespeichert und per SD-Karte gelesen werden, wenn der Bordcomputer ausgebaut ist.

**Telemetrie-Live-Stream (4 Hz, 28 Bytes/Paket):**
28 Bytes * 4 = 112 Bytes/s. Das ist ein Bruchteil der verfuegbaren Bandbreite und kein Problem.

Quellen:
- https://punchthrough.com/terminal-bb/ (BLE-Throughput benchmarks, 20-60 kB/s)
- https://interrupt.memfault.com/blog/bluetooth-throughput (BLE Throughput Optimization)
- https://novelbits.io/bluetooth-le-throughput-guide-part-1/ (BLE Bandbreitenanalyse)

---

## 4. Companion-App Architektur

### 4.1 Technologie-Stack

| Komponente | Wahl | Begruendung |
|---|---|---|
| Framework | React Native | Eine Codebasis fuer beide mobile Betriebssysteme. Grosses Oekosystem, reife BLE-Bibliotheken. |
| BLE-Bibliothek | react-native-ble-plx | Vollstaendige GATT-API, Connection-Parameter-Kontrolle, aktive Wartung, gute Dokumentation. Alternative: react-native-ble-manager (aelter, weniger Features). |
| State-Management | Zustand | Minimal, ohne Boilerplate, performant. Alternative: Redux (overkill fuer diese App). |
| Navigation | React Navigation | Standard fuer React Native, Stack- und Tab-Navigation. |
| Persistenz | SQLite (react-native-quick-sqlite) | Telemetrie-Cache, Tour-Historie, Config-Snapshot. Schnell, offline-faehig. |
| Maps | Native Karten-API (react-native-maps) | Nutzt die native Karten-Implementierung des jeweiligen Betriebssystems. Keine eigene Karten-Engine. |
| Wetter | OpenWeatherMap API | Kostenlos bis 1000 Calls/Tag, ausreichend fuer 15-Minuten-Updates. Alternative: System-Weather-API (falls verfuegbar). |
| Charts | react-native-chart-kit | Einfache Diagramme fuer Telemetrie-Historie und Tour-Statistiken. |
| Icons | react-native-vector-icons | Material/Feather-Icons, konsistent mit Bordcomputer-UI. |

### 4.2 Architektur-Diagramm

```
+=====================================================================+
|                        Companion-App                                 |
+=====================================================================+
|                                                                      |
|  +------------------+  +------------------+  +------------------+   |
|  |   UI-Layer       |  |   Data-Layer     |  |   BLE-Layer      |   |
|  |                  |  |                  |  |                  |   |
|  |  +------------+  |  |  +------------+  |  |  +------------+  |   |
|  |  | Dashboard  |  |  |  | Telemetry  |  |  |  | Connection  |  |   |
|  |  | Screen     |  |  |  | Cache      |  |  |  | Manager    |  |   |
|  |  +------------+  |  |  +------------+  |  |  +------------+  |   |
|  |  +------------+  |  |  +------------+  |  |  +------------+  |   |
|  |  | Navigation |  |  |  | Tour Store |  |  |  | GATT       |  |   |
|  |  | Screen     |  |  |  | (SQLite)   |  |  |  | Operations |  |   |
|  |  +------------+  |  |  +------------+  |  |  +------------+  |   |
|  |  +------------+  |  |  +------------+  |  |  +------------+  |   |
|  |  | Media      |  |  |  | Config     |  |  |  | Reconnect  |  |   |
|  |  | Screen     |  |  |  | Model      |  |  |  | Logic      |  |   |
|  |  +------------+  |  |  +------------+  |  |  +------------+  |   |
|  |  +------------+  |  |  +------------+  |  |  +------------+  |   |
|  |  | Diagnose   |  |  |  | Weather    |  |  |  | Transfer   |  |   |
|  |  | Screen     |  |  |  | Cache      |  |  |  | Protocol   |  |   |
|  |  +------------+  |  |  +------------+  |  |  +------------+  |   |
|  |  +------------+  |  |  +------------+  |  |  +------------+  |   |
|  |  | Settings   |  |  |  | Nav State  |  |  |  | MTU / Param |  |   |
|  |  | Screen     |  |  |  | Cache      |  |  |  | Manager    |  |   |
|  |  +------------+  |  |  +------------+  |  |  +------------+  |   |
|  |  +------------+  |  |  +------------+  |  |                  |   |
|  |  | Tours      |  |  |  | Media      |  |  |  Zustand Store  |   |
|  |  | Screen     |  |  |  | State      |  |  |  (BLE-State)    |   |
|  |  +------------+  |  |  +------------+  |  |                  |   |
|  |  +------------+  |  |                  |  |                  |   |
|  |  | OTA Screen |  |  |  Zustand Store  |  |                  |   |
|  |  +------------+  |  |  (App-State)    |  |                  |   |
|  |                  |  |                  |  |                  |   |
|  +--------+---------+  +--------+---------+  +--------+---------+   |
|           |                     |                     |             |
|           |    Zustand (global state bus)              |             |
|           +---------------------+---------------------+             |
|                                 |                                   |
|                                 v                                   |
|                     +-----------------------+                       |
|                     |   Native Bridge       |                       |
|                     |   (Platform APIs)     |                       |
|                     +-----------------------+                       |
|                                 |                                   |
|           +---------------------+---------------------+             |
|           |                     |                     |             |
|    +------+------+       +------+------+        +------+------+     |
|    | BLE (OS)    |       | Maps (OS)   |        | Media (OS) |     |
|    | BLE-Stack  |       | Karten-API  |        | Audio-Session |  |
|    +-------------+       +-------------+        +-------------+     |
|                                                                      |
+======================================================================+
                              |
                              | BLE GATT
                              v
                    +------------------+
                    |  ESP32-S3        |
                    |  Bordcomputer    |
                    +------------------+
```

### 4.3 BLE-Layer

Der BLE-Layer ist die unterste Schicht der App und kapselt die gesamte BLE-Kommunikation. Er besteht aus drei Modulen:

**Connection Manager:**

Verwaltet die Verbindungsmaschine (State-Machine aus Abschnitt 1.1). Responsibilities:
- BLE-Scan starten/stoppen, Advertising-Pakete filtern (nach Service-UUID 0x1000)
- Connect/Disconnect
- Auto-Reconnect mit exponentiellem Backoff
- MTU-Aushandlung (`requestMtu(247)`)
- Connection-Parameter-Update (`requestConnectionPriority(HIGH)`)
- Bonding-Status verfolgen
- RSSI-Monitoring (alle 5 Sekunden, fuer DEGRADING-Zustand)
- Zustand-Store (Zustand): `bleState` mit Feldern `connectionState`, `rssi`, `mtu`, `isEncrypted`, `bondedDevice`

**GATT Operations:**

Kapselt alle GATT-Reads, -Writes und -Notifications. Stellt typisierte Methoden zur Verfuegung:
- `readCharacteristic(serviceId, charId)` -> Promise<Uint8Array>
- `writeCharacteristic(serviceId, charId, data, withResponse)` -> Promise<void>
- `subscribeCharacteristic(serviceId, charId, callback)` -> Subscription
- Automatische UUID-Aufloesung (16-bit-ID -> 128-bit-UUID mit Base-Präfix)

Der GATT-Operations-Layer fuehrt automatisch Service-Discovery beim ersten Connect durch und cachet die Handle-Tabelle. Bei nachfolgenden Connects kann der Discovery-Schritt uebersprungen werden (es sei denn, der Service-Changed-Indicator wird gesendet).

**Transfer Protocol:**

Implementiert das Binary-Transfer-Protokoll aus Abschnitt 3.3. Responsibilities:
- Chunked-Send und Chunked-Receive
- CRC-16 und CRC-32 Berechnung/Verifikation
- ACK-Verwaltung und Sliding-Window
- Retransmission-Logik
- Progress-Tracking (fuer UI-Fortschrittsbalken)

### 4.4 Data-Layer

Der Data-Layer verwaltet alle Daten, die nicht direkt vom BLE kommen, aber von der App benoetigt werden:

**Telemetry Cache (SQLite):** Speichert Live-Telemetrie fuer offline-Anzeige und Historie. Die App empfaengt Live-Telemetry-Notifications (0x5001) und cached sie in einer SQLite-Tabelle. Bei Verbindungsabbruch kann die App weiterhin die letzten bekannten Werte anzeigen. Die Telemetrie-Historie wird auch fuer Diagramme im Dashboard verwendet (z.B. Speed-Verlauf der letzten 5 Minuten).

**Tour Store (SQLite):** Speichert komplette Touren mit Telemetrie-Track, Statistiken (max Speed, avg Speed, Distance, Duration, max RPM) und GPX-Export. Eine Tour wird beim Start der App (wenn der Bordcomputer connected ist) automatisch begonnen und beim Disconnect (nach 5 Minuten Inaktivitaet) beendet. Touren koennen nachtraeglich vom Bordcomputer exportiert werden (Track Export via 0x5003).

**Config Model:** Haelten die Konfiguration des Bordcomputers als typisiertes JavaScript-Objekt. Beim (Re-)Connect wird die Config via Config-Read (0x6001) gelesen und im Config-Model gespeichert. Aenderungen in der Settings-Screen schreiben zurueck via Config-Write (0x6002) und aktualisieren das Config-Model. Atomic-Write auf dem Bordcomputer stellt sicher, dass die Config konsistent bleibt.

**Nav State Cache:** Speichert die letzten bekannten Navigationsdaten (Maneuver, Route, Road). Bei Verbindungsabbruch kann die App diese weiter anzeigen und an den Bordcomputer re-senden, wenn die Verbindung wieder steht.

**Weather Cache:** Speichert die letzten Wetterdaten und Forecast. Wetterdaten werden von der App per HTTP-API abgerufen (OpenWeatherMap) und an den Bordcomputer gesendet. Bei Verbindungsabbruch behaelt die App die letzten Daten und aktualisiert sie, sobald die Verbindung wieder steht.

**Media State:** Speichert Now-Playing-Info und Volume. Die App liest die Now-Playing-Info aus dem System ab und cached sie fuer die Media-Screen.

### 4.5 UI-Layer und Screens

**Dashboard Screen:**

Zeigt Live-Telemetrie vom Bordcomputer. Layout:
- Grosses Speed-Display (Ziffern, zentral)
- RPM als Balken oder Tacho
- Gang-Anzeige
- Temperatur (Kuehlmittel + Umgebung)
- Kraftstoffstand mit Reichweite
- Spannung
- Verbindungssymbol (BLE-Status)
- Mini-Diagramm: Speed-Verlauf der letzten 5 Minuten (aus Telemetry Cache)
- Mini-Wetter-Widget (aktuelle Temp + Bedingung)

Die Dashboard-Screen abonniert Live-Telemetry-Notify (0x5001) und aktualisiert die UI bei jedem Paket (4 Hz). React Native re-rendert nur die geaenderten Komponenten (memoisierte Sub-Komponenten).

**Navigation Screen:**

Zeigt die native Karten-API mit aktueller Position und Route. Die App extrahiert Navigationsdaten aus der aktiven Navigation (siehe Abschnitt 5) und sendet sie per BLE an den Bordcomputer. Die Navigation-Screen zeigt:
- Vollbild-Karte mit Route
- Naechstes Maneuver als Overlay (Pfeil + Distanz + Strassenname)
- ETA und Reststrecke
- "An Bordcomputer senden" Status-Indikator

Wichtig: Die App fuehrt die Navigation nicht selbst aus. Sie nutzt die Navigations-Engine des Betriebssystems (die jeweilige native Karten-App) und liest die resultierenden Navigationsdaten aus. Die App ist ein Vermittler, kein eigenes Navi.

**Media Screen:**

Zeigt Now-Playing-Info und bietet Steuerung:
- Artist + Title (gross)
- Album-Cover (falls verfuegbar)
- Fortschrittsbalken (Position / Duration)
- Play/Pause, Next, Prev Buttons
- Lautstaerkeregler
- Media-Quelle (welche App spielt gerade ab?)

Die Media-Screen abonniert Now-Playing-Notify (0x2001) und Volume-Notify (0x2003). Steuerungs-Befehle werden per Write an Media-Control (0x2002) gesendet.

**Diagnose Screen:**

Zeigt Diagnose-Informationen:
- DTC-Liste (Diagnostic Trouble Codes) mit Code, Beschreibung, Severity, Timestamp
- Sensor-Status (GPS, RPM, Temp, Fuel, SD-Karte) als Status-Grid
- Self-Test-Button (startet Self-Test, zeigt Ergebnis)
- Log-Export-Button (exportiert System-Logs als Datei)
- Firmware-Version (aus Device-Information-Service)

**Settings Screen:**

Config-Editor fuer den Bordcomputer. Zeigt alle konfigurierbaren Parameter in einer strukturierten Liste:
- Display-Einstellungen (Helligkeit, Theme, Auto-Dim)
- Sensor-Kalibrierung (Radumfang, RPM-Pulses-per-Rev, Temp-Offset)
- BLE-Einstellungen (Device-Name, Bond-Management)
- Logging-Einstellungen (Log-Level, SD-Card-Format)
- Trip-Reset
- Config-Export/Import (Backup/Restore)

Aenderungen werden per Config-Write (0x6002) an den Bordcomputer gesendet. Atomic-Write stellt sicher, dass die Config konsistent bleibt.

**Tours Screen:**

Liste aller aufgezeichneten Touren:
- Datum, Strecke, Dauer, max Speed, avg Speed
- Tap auf Tour -> Detail-Ansicht mit Karte (GPX-Track auf Karte), Hoehenprofil, Speed-Profil
- Export als GPX-Datei (Teilen-Funktion)
- Loeschen von Touren

**OTA Screen:**

Firmware-Update:
- Aktuelle Firmware-Version anzeigen
- "Firmware-Datei auswaehlen" (Datei-Picker)
- Verifikation der Datei (Magic-Number, Groesse)
- Transfer mit Fortschrittsbalken
- Verifikation (SHA-256-Hash vergleichen)
- Commit-Button (nur nach erfolgreicher Verifikation)
- Rollback-Info (Bordcomputer behaelt alte Firmware im anderen OTA-Partition)

---

## 5. Navigation-Integration

### 5.1 Prinzip

Die App nutzt die native Karten-API des Smartphones. Das Betriebssystem stellt eine Navigations-Engine zur Verfuegung, die Route berechnet, Sprachausgabe macht und die Karte anzeigt. Die App greift auf die Navigationsdaten dieser Engine zu, ohne die UI zu uebernehmen.

Die App ist nicht das Navigationsgeraet. Die App ist die Bruoecke, die Navigationsdaten vom System-Navi extrahiert und an den Bordcomputer sendet. Der Fahrer navigiert mit der System-Karten-App (die Sprachausgabe kommt vom Telefon), und der Bordcomputer zeigt die wesentlichen Maneuver-Informationen auf dem Display an.

### 5.2 Datenextraktion

Die App extrahiert folgende Daten aus der aktiven Navigation:

**Naechstes Maneuver:**
- Maneuver-Typ: geradeaus (0), leicht links (1), links (2), scharf links (3), leicht rechts (4), rechts (5), scharf rechts (6), U-Turn (7), Ziel erreicht (8), Abbiegen auf Faelle (9)
- Distanz zum Maneuver: in Metern (uint32)
- Strassenname der naechsten Strasse nach dem Maneuver: UTF-8, max 64 Bytes

**Route-Info:**
- Gesamtstrecke: in Metern (uint32)
- Reststrecke: in Metern (uint32)
- ETA: Unix-Timestamp (uint32)
- Restzeit: in Sekunden (uint32)

**Aktuelle Strasse:**
- Strassenname: UTF-8, max 64 Bytes
- Tempolimit: km/h (uint8, 0 = unbekannt)

### 5.3 Message-Formate

**NAV_MANEUVER (0x1001):**

```
Byte 0:     Maneuver-Type (uint8)
Byte 1-4:   Distance (uint32, Meter, Little-Endian)
Byte 5:     Street-Name-Length (uint8, 0-64)
Byte 6+:    Street-Name (UTF-8, Street-Name-Length Bytes)
```

Maximalgroesse: 6 + 64 = 70 Bytes. Passt in eine MTU-247-Notification problemlos.

**NAV_ROUTE (0x1002):**

```
Byte 0-3:   Total-Distance (uint32, Meter)
Byte 4-7:   Remaining-Distance (uint32, Meter)
Byte 8-11:  ETA (uint32, Unix-Timestamp)
Byte 12-15: Remaining-Time (uint32, Sekunden)
```

16 Bytes. Passt in jede MTU.

**NAV_ROAD (0x1003):**

```
Byte 0:     Speed-Limit (uint8, km/h, 0=unbekannt)
Byte 1:     Road-Name-Length (uint8, 0-64)
Byte 2+:    Road-Name (UTF-8, Road-Name-Length Bytes)
```

Maximalgroesse: 2 + 64 = 66 Bytes.

**NAV_STATUS (0x1004):**

```
Byte 0:     Status (uint8: 0=inactive, 1=active, 2=rerouting, 3=arriving, 4=offline)
```

1 Byte.

**NAV_CMD (0x0105, Write):**

```
Byte 0:     Command (uint8: 0x01=cancel_route, 0x02=repeat_maneuver, 0x03=request_alternative)
```

1 Byte. Bordcomputer -> App.

### 5.4 Update-Frequenz und Maneuver-Uebergaenge

Die App sendet Navigationsdaten mit folgenden Frequenzen:

- NAV_MANEUVER: alle 1-2 Sekunden (oder sofort bei Maneuver-Aenderung)
- NAV_ROUTE: alle 10 Sekunden (oder bei signifikanter Aenderung > 500 m)
- NAV_ROAD: bei Strassenwechsel (event-basiert)
- NAV_STATUS: bei Statuswechsel (event-basiert)

**Maneuver-Uebergaenge:** Wenn sich das naechste Maneuver aendert (z.B. der Fahrer hat das vorherige Maneuver ausgefuehrt und das naechste wird aktuell), sendet die App sofort ein neues NAV_MANEUVER-Paket. Der Bordcomputer rendert einen smooth-Uebergang:

1. Altes Maneuver wird ausgeblendet (Fade-Out, 200 ms)
2. Neues Maneuver wird eingeblendet (Fade-In, 200 ms)
3. Pfeil-Rotation animiert (200 ms, Easing)

Die Animation wird in LVGL mit `lv_anim_t` implementiert. Die Dauer von 200 ms ist kurz genug, um nicht abzulenken, aber lang genug, um den Wechsel wahrnehmbar zu machen.

### 5.5 Offline-Verhalten

Bei Verbindungsabbruch zeigt der Bordcomputer "Navigation Offline" an. Die letzten bekannten Maneuver-Daten werden im RAM des Bordcomputers gecached (letztes Maneuver, letzte Route-Info). Wenn die Verbindung innerhalb von 30 Sekunden wieder steht, werden die gecachten Daten durch neue ersetzt. Bei laengerem Verbindungsabbruch wird das Navigations-Widget durch ein "Offline"-Symbol ersetzt, aber die Kerninstrumente (Speed, RPM, etc.) bleiben voll funktionsfaehig.

---

## 6. Notification-Relay

### 6.1 Prinzip

Die App registriert sich als Notification-Listener beim Betriebssystem und erfaehrt so von eingehenden Benachrichtigungen: Anrufe, SMS, Messenger-Nachrichten, und andere App-Notifications. Nicht jede Notification wird weitergeleitet. Ein Filter entscheidet, welche Notifications den Bordcomputer erreichen.

### 6.2 Filterung

**Immer weiterleiten:**
- Eingehende Anrufe (Priority: Critical)
- SMS / Text-Nachrichten (Priority: High)

**Konfigurierbar (Default: an):**
- Messenger-Apps (z.B. Chat-Apps) (Priority: Medium)

**Nie weiterleiten:**
- System-Notifications (Update-Verfuegbar, Speicher voll, etc.)
- App-Update-Notifications
- Werbung / Push-Marketing
- Notifications von Apps, die der Nutzer explizit blockiert hat

Der Filter wird in der Settings-Screen konfiguriert. Der Nutzer kann pro App entscheiden, ob Notifications weitergeleitet werden. Die Konfiguration wird im Config-Model gespeichert.

### 6.3 Message-Format

**PHONE_NOTIF (0x0304):**

```
Byte 0:     Priority (uint8: 0=critical, 1=high, 2=medium)
Byte 1:     App-Name-Length (uint8, 0-32)
Byte 2+:    App-Name (UTF-8, App-Name-Length Bytes)
Byte N:     Title-Length (uint8, 0-64)
Byte N+1+:  Title (UTF-8, Title-Length Bytes)
Byte M:     Body-Length (uint8, 0-100, Body wird auf 100 Zeichen gekuerzt)
Byte M+1+:  Body (UTF-8, Body-Length Bytes)
```

Maximalgroesse: 3 + 32 + 1 + 64 + 1 + 100 = 201 Bytes. Passt in MTU-247.

### 6.4 Anzeige auf dem Bordcomputer

**Toast-Notification:** Bei einer neuen Notification zeigt der Bordcomputer eine Toast-Notification auf dem Display. Diese erscheint als Overlay ueber der aktuellen UI, positioniert im oberen Drittel des Displays. Sie enthaelt App-Name (klein), Titel (mittel, fett), und Body (klein, auf 100 Zeichen gekuerzt). Der Toast ist 5 Sekunden sichtbar und blendet sich dann automatisch aus (Fade-Out, 300 ms).

**Anruf-Spezialbehandlung:** Bei eingehendem Anruf wird kein Toast, sondern ein Vollbild-Overlay angezeigt:
- Caller-Name (gross, zentral)
- "Annehmen" / "Ablehnen" Buttons (ueber Lenkertaster navigierbar)
- Der Overlay bleibt sichtbar, bis der Anruf beendet oder abgelehnt wird

**Anruf-Steuerung ueber Bordcomputer:** Der Bordcomputer kann Anrufe annehmen oder ablehnen ueber die Lenkertaster:
- Linker Taster (kurz): Annehmen
- Rechter Taster (kurz): Ablehnen
- Linker Taster (lang, bei aktivem Anruf): Auflegen

Die Taster-Events werden in der Bordcomputer-Firmware in PHONE_CMD-Nachrichten (0x0303) uebersetzt und per BLE an die App gesendet. Die App leitet sie an die Telefonie-API weiter.

---

## 7. Media-Steuerung

### 7.1 Datenfluss

Die App liet die Now-Playing-Info aus dem System ab (die jeweilige Media-API des Betriebssystems liefert Artist, Title, Album, Duration, Position und Playback-State). Diese Informationen werden per BLE an den Bordcomputer gesendet (MEDIA_NOWPLAYING, 0x2001). Der Bordcomputer zeigt die Media-Info als kleines Widget auf dem Display an.

Steuerungs-Befehle (Play/Pause/Next/Prev) gehen den umgekehrten Weg: Der Fahrer drueckt einen Lenkertaster, der Bordcomputer sendet eine MEDIA_CMD-Nachricht (0x2002) an die App, die App leitet den Befehl an die System-Media-API weiter.

### 7.2 Message-Formate

**MEDIA_NOWPLAYING (0x2001):**

```
Byte 0:     Playback-State (uint8: 0=stopped, 1=playing, 2=paused)
Byte 1-4:   Duration (uint32, Sekunden)
Byte 5-8:   Position (uint32, Sekunden)
Byte 9:     Artist-Length (uint8, 0-64)
Byte 10+:   Artist (UTF-8, Artist-Length Bytes)
Byte N:     Title-Length (uint8, 0-64)
Byte N+1+:  Title (UTF-8, Title-Length Bytes)
```

Maximalgroesse: 9 + 1 + 64 + 1 + 64 = 139 Bytes. Passt in MTU-247.

**MEDIA_CMD (0x2002, Write):**

```
Byte 0:     Command (uint8: 0x01=play_pause, 0x02=next, 0x03=prev, 0x04=seek_fwd_10s, 0x05=seek_back_10s)
```

1 Byte. Bordcomputer -> App.

### 7.3 Lautstaerkeregelung

Die System-Lautstaerke wird ueber die Volume-Characteristic (0x2003) synchronisiert. Die App notifiziert den Bordcomputer bei jeder Lautstaerkeaenderung. Der Bordcomputer kann die Lautstaerke per Write aendern (Lenkertaster -> lauter/leiser).

Lenkertaster-Belegung fuer Media:
- Oberer Taster (kurz): Next Track
- Unterer Taster (kurz): Previous Track
- Oberer Taster (lang): Lauter
- Unterer Taster (lang): Leiser
- Mittlerer Taster (kurz): Play/Pause

Wenn ein Anruf eingehender Anruf aktiv ist, werden die Taster fuer Anruf-Steuerung umbelegt (siehe Abschnitt 6.4). Die Taster-Belegung ist kontextabhaengig: Anruf > Media.

### 7.4 Media-Widget auf dem Display

Das Media-Widget ist ein kleines UI-Element, das permanent auf dem Display sichtbar ist (ausser bei aktivem Vollbild-Nav oder eingehendem Anruf). Es zeigt:
- Artist + Title (eine Zeile, scrollend wenn zu lang)
- Wiedergabe-Status (Play/Pause-Symbol)
- Position als dünner Fortschrittsbalken

Das Widget ist im unteren Drittel des Displays positioniert und ca. 30 Pixel hoch bei 480x320 Aufloesung. Es wird nur angezeigt, wenn Media aktiv ist (Playback-State != stopped).

---

## 8. Wetter-Integration

### 8.1 Datenquelle

Die App ruft Wetterdaten von der OpenWeatherMap API ab (Current Weather + 5-Day Forecast). Die API ist kostenlos bis 1000 Calls/Tag. Bei 15-Minuten-Update-Intervall sind das 96 Calls/Tag, gut innerhalb des Limits.

Alternative: Falls das Betriebssystem eine Weather-API anbietet (System-Weather-Service), kann diese als Datenquelle verwendet werden, ohne externe API. Dies ist Plattform-abhaengig und wird als Fallback verwendet, wenn keine Netzwerkverbindung fuer OpenWeatherMap verfuegbar ist.

Die App verwendet die aktuelle GPS-Position (vom Bordcomputer via Telemetry-Notify 0x5001) als Abfrage-Ort. Das bedeutet, dass Wetterdaten immer fuer die aktuelle Position des Motorrads abgerufen werden, nicht fuer eine statische Ortseinstellung.

### 8.2 Message-Formate

**WX_CURRENT (0x4001):**

```
Byte 0-1:   Temperature (int16, Zehntelgrad Celsius, Little-Endian)
Byte 2:     Humidity (uint8, Prozent)
Byte 3:     Condition (uint8: 0=clear, 1=cloudy, 2=overcast, 3=rain, 4=heavy_rain, 5=thunderstorm, 6=snow, 7=fog)
Byte 4:     Wind-Speed (uint8, km/h)
Byte 5-6:   Feels-Like (int16, Zehntelgrad Celsius, Little-Endian)
Byte 7:     Rain-Probability (uint8, Prozent, naechste 1h)
```

8 Bytes. Sehr kompakt.

**WX_FORECAST (0x4002):**

```
6 Eintraege a 4 Bytes (insgesamt 24 Bytes):
  Byte 0: Hour-Offset (uint8, 0=now, 1=+1h, ..., 5=+5h)
  Byte 1: Condition (uint8)
  Byte 2: Temp-Min (int8, Celsius)
  Byte 3: Temp-Max (int8, Celsius)
```

24 Bytes. Kompakte 6-Stunden-Vorhersage.

**WX_ALERT (0x4003):**

```
Byte 0:     Alert-Type (uint8: 0=rain_approaching, 1=storm, 2=frost, 3=heat, 4=wind)
Byte 1:     Severity (uint8: 0=info, 1=warning, 2=severe)
Byte 2-3:   Time-Until (uint16, Minuten, Little-Endian)
```

4 Bytes.

### 8.3 Anzeige auf dem Bordcomputer

Das Wetter-Widget auf dem Bordcomputer-Display zeigt:
- Aktuelle Temperatur (gross, Ziffern)
- Bedingung als Icon (Sonne, Wolke, Regen, etc.)
- Regenwahrscheinlichkeit (klein, Prozent)
- Wind (klein, km/h)

Das Widget ist im oberen rechten Bereich des Displays positioniert. Bei einer Wetter-Warnung (WX_ALERT) erscheint ein zusaetzliches Overlay:
- Bei `rain_approaching`: Regenwolken-Icon + "Regen in X min"
- Bei `storm`: Blitz-Icon + "Gewitter in X min" + rotes Overlay
- Bei `frost`: Schneeflocken-Icon + "Frost in X min"
- Bei `heat`: Thermometer-Icon + "Hitze in X min"
- Bei `wind`: Wind-Icon + "Sturm in X min"

Das Overlay bleibt sichtbar, bis die Bedingung eintritt oder die Warnung ablaeuft.

### 8.4 Regenwarnung

Die Regenwarnung ist das wichtigste Wetter-Feature fuer Motorradfahrer. Die App berechnet die Regenwahrscheinlichkeit aus dem Forecast und der aktuellen Position. Wenn die Regenwahrscheinlichkeit in den naechsten 30 Minuten ueber 60% steigt, wird eine WX_ALERT mit Type `rain_approaching` gesendet.

Zusaetzlich kann die App die aktuelle Niederschlags-Intensitaet aus der API abfragen (OpenWeatherMap liefert `rain.1h` in mm). Bei aktuellem Regen oder Regen innerhalb von 15 Minuten wird die Warnung auf Severity `warning` erhoeht.

---

## 9. Robustheit & Graceful Degradation

### 9.1 Bei BLE-Verbindungsabbruch

Die Robustheit bei Verbindungsabbruch ist kritisch fuer die Sicherheit. Der Bordcomputer muss in jedem Fall die Kernfunktionen (Speed, RPM, Temperatur, Kraftstoff, Trip) weiterhin anzeigen. Die Smartphone-abhaengigen Features degradieren kontrolliert.

| Feature | Bei Verbindungsabbruch | Wiederherstellung |
|---|---|---|
| Navigation | "Navigation Offline" anzeigen, letzte bekannte Route (Maneuver + Road) fuer 30s gecacht anzeigen, danach Offline-Symbol | Bei Reconnect: neue Nav-Daten empfangen, gecachte Daten ersetzen |
| Media | "Media Disconnected" anzeigen, Media-Widget ausblenden nach 10s | Bei Reconnect: Now-Playing-Notify abonnieren, Widget wieder einblenden |
| Phone | Notification-Relay deaktivieren, Toast-Notifications stoppen | Bei Reconnect: Notification-Filter reaktivieren |
| Wetter | Letzte bekannte Wetterdaten anzeigen (15 Min gueltig), danach "Weather Offline" | Bei Reconnect: Weather-Cache aktualisieren, neue Daten senden |
| Telemetry | Lokal auf SD-Karte weiter logging (wie immer, unabhaengig von BLE), Live-Stream zur App stoppt | Bei Reconnect: Telemetry-Cache der letzten 24h an App synchronisieren (Optional-Feature) |

### 9.2 Telemetry-Offline-Buffer

Der Bordcomputer schreibt Telemetrie immer auf die SD-Karte (GPX-Track-Logging, siehe 06-feature-roadmap Abschnitt 1.7). Bei Verbindungsabbruch wird das Logging fortgesetzt. Die App hat keinen Telemetrie-Verlust, weil die Daten auf der SD-Karte sind.

Zusaetzlich unterhaelt der Bordcomputer einen Ring-Buffer im RAM (letzte 60 Sekunden Telemetrie, 4 Hz, 28 Bytes/Paket = 6.720 Bytes). Bei Reconnect kann die App diesen Ring-Buffer abrufen (via Telemetry-Snapshot 0x5002), um die Luecke zu fuellen. Das ist ein Optional-Feature und kann deaktiviert werden, um Komplexitaet zu reduzieren.

### 9.3 Bei schlechter Verbindung (DEGRADING-Zustand)

Wenn die BLE-Signalqualitaet schlecht ist (RSSI < -90 dBm oder Packet-Loss > 10%), schaltet der Bordcomputer in den DEGRADING-Zustand. In diesem Zustand wird die Datenrate reduziert:

**Prioritaetsstufen:**

| Prioritaet | Daten | Frequenz im DEGRADING-Zustand |
|---|---|---|
| P0 (Critical) | Incoming Call, Call Status | Sofort (event-basiert) |
| P1 (High) | Navigation Maneuver | Alle 2-3 Sekunden (reduziert von 1-2s) |
| P2 (Medium) | Live Telemetry | Alle 1 Sekunde (reduziert von 250ms) |
| P3 (Low) | Now Playing, Route Info, Weather | Gestoppt (nur bei Reconnect) |
| P4 (Background) | Track Export, Log Export, OTA | Gestoppt |

Im DEGRADING-Zustand werden nur noch P0- und P1-Daten uebertragen. P2 wird auf 1 Hz reduziert. P3 und P4 werden gestoppt. Der Bordcomputer zeigt ein "schwaches Verbindung"-Symbol auf dem Display (halbes BLE-Icon oder gelbe Farbe).

Die App erkennt den DEGRADING-Zustand anhand des RSSI (den sie selbst misst) und der Notify-Frequenz (wenn Telemetry-Notifications seltener kommen). Sie passt ihre eigene UI entsprechend an: z.B. wird das Media-Widget ausgegraut, das Wetter-Widget zeigt "Offline".

### 9.4 Reconnect-Sequenz

Bei Reconnect nach Verbindungsabbruch durchlaeuft die App eine definierte Wiederherstellungs-Sequenz:

1. **MTU-Exchange** (`requestMtu(247)`)
2. **Connection-Parameter** (`requestConnectionPriority(HIGH)`)
3. **Service-Discovery** (nur wenn Service-Changed-Indication, sonst cached)
4. **Encryption aktivieren** (mit gespeichertem Bonding-Key)
5. **Subscribe** P0-Characteristics: Incoming Call (0x3001), Call Status (0x3002)
6. **Subscribe** P1-Characteristics: Nav Maneuver (0x1001), Nav Status (0x1004)
7. **Read** Telemetry Snapshot (0x5002) — sofortiger Vollzustand
8. **Subscribe** P2-Characteristics: Live Telemetry (0x5001)
9. **Read** Config (0x6001) — Config synchronisieren
10. **Subscribe** P3-Characteristics: Now Playing (0x2001), Weather (0x4001/0x4002), Volume (0x2003)
11. **Subscribe** P4-Characteristics (falls aktive Transfers): Track Export (0x5003), Log Export (0x0704)

Diese Sequenz stellt sicher, dass die wichtigsten Daten zuerst fliessen. Der gesamte Reconnect dauert typischerweise 500-1000 ms, nachdem die physische Verbindung steht.

### 9.5 Watchdog und Supervision

Der Bordcomputer hat einen Software-Watchdog, der die BLE-Verbindung ueberwacht. Wenn 5 Sekunden lang keine BLE-Events empfangen werden (kein Connection-Event, keine Notification, kein Write), wird ein Supervision-Timeout ausgeloest. Der Bordcomputer wechselt in den DISCONNECTED-Zustand und beginnt erneut zu advertisen.

Zusaetzlich ueberwacht die App die Verbindung mit einem Heartbeat: alle 10 Sekunden liest sie die NAV_STATUS-Characteristic (0x1004). Wenn der Read fehlschlaegt oder ein Timeout auftritt, wird die Verbindung als kaputt betrachtet und der Reconnect-Prozess gestartet.

---

## 10. Zusammenfassung und Implementierungs-Reihenfolge

### 10.1 Priorisierung

| Phase | Feature | Prioritaet | Aufwand |
|---|---|---|---|
| 1 | BLE-Verbindung (State-Machine, Bonding, Auto-Reconnect) | Must | Hoch |
| 1 | Telemetry Service (Live-Notify + Snapshot) | Must | Mittel |
| 1 | Companion-App Grundgeruest (BLE-Layer, Dashboard-Screen) | Must | Hoch |
| 2 | Navigation Service (GATT + App-Integration) | Must | Hoch |
| 2 | Media Service (GATT + App-Integration) | Should | Mittel |
| 3 | Phone Service (Notification-Relay, Call-Control) | Should | Mittel |
| 3 | Weather Service (GATT + API-Integration) | Should | Gering |
| 4 | Config Service (Read/Write/Export/Import) | Should | Mittel |
| 4 | Diagnose Service (DTC, Sensor-Status, Self-Test) | Should | Mittel |
| 5 | OTA Service (Firmware-Update) | Could | Hoch |
| 5 | Binary-Transfer-Protokoll (fuer Track/Log/OTA) | Could | Hoch |
| 6 | Multi-Phone-Support | Could | Gering |
| 6 | DEGRADING-Modus (priorisierte Datenrate) | Could | Mittel |

### 10.2 Abhaengigkeiten

- Die GATT-Service-Definition (Abschnitt 2) ist die Grundlage fuer alles. Sie muss zuerst auf dem Bordcomputer (ESP-IDF BLE-Stack) und in der App (react-native-ble-plx) implementiert werden.
- Das Binary-Transfer-Protokoll (Abschnitt 3.3) wird fuer Track-Export, Log-Export und OTA benoetigt. Es kann parallel zur GATT-Definition entwickelt werden, wird aber erst in Phase 5 benoetigt.
- Die Companion-App-Architektur (Abschnitt 4) ist unabhaengig von der Bordcomputer-Firmware und kann parallel entwickelt werden.
- Navigation, Media, Phone und Weather sind in der App unabhaengig voneinander und koennen parallel implementiert werden.

### 10.3 Schlussbemerkung

Die hier definierte Architektur verbindet den Bordcomputer mit dem Smartphone auf eine Weise, die fuer den Fahrer unsichtbar ist: einmal gekoppelt, verbindet sich das System automatisch, degradiert kontrolliert bei Stoerungen, und stellt alle Funktionen wieder her, sobald die Verbindung zurueck ist. Die BLE-GATT-Struktur mit acht Services deckt alle Use-Cases ab, von Live-Telemetrie bis OTA-Firmware-Update. Die Companion-App als React-Native-Codebasis fuer beide mobilen Betriebssysteme minimiert Entwicklungsaufwand, ohne Kompromisse bei der Funktionalitaet zu machen.

Die bewusste Entscheidung, keine geschlossenen Protokolle der Geraetehersteller zu verwenden, sondern ein eigenes GATT-Protokoll zu definieren, gibt uns volle Kontrolle ueber Datenformate, Update-Frequenzen und Fehlerbehandlung. Der Preis ist der Entwicklungsaufwand fuer die Companion-App. Der Gewinn ist ein System, das genau das tut, was ein Motorradfahrer braucht: die wesentlichen Informationen auf dem Display, ohne Ablenkung, ohne Abstriche.

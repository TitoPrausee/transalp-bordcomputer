# Sicherheitsanalyse & Threat Model (STRIDE)

> **Phase 4: Sicherheitsanalyse & Threat Model**
> Transalp Bordcomputer — BLE-Kommunikation und OTA-Update-System
> Stand: Oktober 2026
> Plattform: ESP32-S3-WROOM-1, ESP-IDF v5.4, NimBLE BLE Stack
> Companion-App: React Native mit react-native-ble-plx

---

## Inhaltsverzeichnis

1. [System-Uebersicht und Angriffsflaeche](#1-system-uebersicht-und-angriffsflaeche)
2. [Threat Model (STRIDE)](#2-threat-model-stride)
3. [Sicherheits-Architektur-Empfehlungen](#3-sicherheits-architektur-empfehlungen)
4. [Implementierungs-Plan](#4-implementierungs-plan)
5. [Secure-Code-Review-Checkliste](#5-secure-code-review-checkliste)

---

## 1. System-Uebersicht und Angriffsflaeche

### 1.1 System-Kontext

Der Transalp Bordcomputer ist ein eigenstaendiges Motorrad-Bordcomputer-System
fuer die Honda XL600V Transalp (PD06). Es besteht aus drei Hauptebenen:

- **Firmware** (ESP32-S3, Dual-Core 240 MHz): Sensordatenakquisition,
  Display-Rendering, SD-Logging, BLE-GATT-Server
- **Companion-App** (React Native): Smartphone-Seite, GATT-Client,
  Navigation/Media/Phone-Relay, OTA-Initiator
- **Hardware-Peripherie**: Sensoren (IMU, Barometer, GPS), Displays (TFT,
  E-Paper), MicroSD-Karte, Akku-System

Die Kommunikation zwischen Bordcomputer und Smartphone erfolgt ausschliesslich
ueber BLE (Bluetooth Low Energy). Es gibt keine WLAN- oder Ethernet-Schnittstelle
im Normalbetrieb. Die einzige Netzwerkverbindung entsteht temporär waehrend
eines OTA-Updates ueber HTTP, falls der Update-Transportweg HTTP gewaehlt wird.

### 1.2 Angriffsflaechen-Uebersicht

| Schnittstelle | Typ | Reichweite | Authentifizierung | Risiko |
|---|---|---|---|---|
| BLE GATT-Server | Funk (2.4 GHz) | ~10-30 m | Pairing (LE SC) | Hoch |
| OTA-Update (BLE) | Funk ueber BLE | ~10-30 m | Hash + Signatur | Kritisch |
| OTA-Update (HTTP) | Netzwerk | Internet | TLS (crt-bundle) | Mittel |
| OTA-Update (SD) | Physisch | Direkt | Keine | Niedrig* |
| Serial Console (UART) | Physisch | Direkt | Keine | Niedrig* |
| SD-Karte (FAT32) | Physisch | Direkt | Keine | Niedrig* |
| NVS (Flash) | Physisch | Direkt | Flash-Enc (opt.) | Niedrig* |

*\* Physische Angriffe erfordern direkten Zugang zum Geraet. Bei einem
Motorrad-Bordcomputer ist physischer Zugriff moeglich (Parken im oeffentlichen
Raum), jedoch ist die SD-Karte und Serial-Console nur nach Gehaeuseoeffnung
verfuegbar.*

### 1.3 BLE-Angriffsflaeche im Detail

Der BLE-GATT-Server exponiert 8 Custom-Services mit insgesamt 33
Characteristics. Jede Characteristic mit Write- oder Read-Property ist eine
potenzielle Angriffsflaeche.

**Write-faehige Characteristics (Befehlseingang):**

| Characteristic | Service | Properties | Funktion |
|---|---|---|---|
| NAV_CMD (0x1005) | Navigation | Write, WriteNR | Navigationsbefehl |
| MEDIA_CONTROL (0x2002) | Media | Write, WriteNR | Mediensteuerung |
| MEDIA_VOLUME (0x2003) | Media | Write | Lautstaerkeregelung |
| PHONE_CONTROL (0x3003) | Phone | Write | Anrufsteuerung |
| TRACK_EXPORT_CTRL (0x5004) | Telemetry | Write | Track-Export |
| CONFIG_WRITE (0x6002) | Config | Write | Konfiguration schreiben |
| CONFIG_IMPORT (0x6004) | Config | Write | Konfiguration importieren |
| BOND_MGMT (0x6005) | Config | Write | Bonding-Keys loeschen |
| SELF_TEST (0x7003) | Diagnose | Write | Selbsttest ausloesen |
| LOG_EXPORT_CTRL (0x7005) | Diagnose | Write | Log-Export |
| OTA_CONTROL (0x8001) | OTA | Write | OTA steuern |
| OTA_DATA (0x8002) | OTA | WriteNR | Firmware-Daten |

**Kritische Write-Characteristics (erfordern Authentifizierung):**

- CONFIG_WRITE / CONFIG_IMPORT: Aendert Geraetekonfiguration (Kalibrierung,
  Schwellwerte, Geraetename)
- BOND_MGMT: Loescht alle Pairing-Keys (erfordert physischen Reset)
- SELF_TEST: Loest Selbsttest aus (Ressourcenverbrauch)
- OTA_CONTROL / OTA_DATA: Firmware-Update (kritischster Vektor)

### 1.4 OTA-Update-System

Das OTA-System unterstuetzt drei Transportwege:

1. **BLE-Transfer**: Firmware-Image wird in 4 KB Chunks ueber BLE
   uebertragen. CRC16-CCITT pro Chunk. SHA-256-Verifikation nach Abschluss.
   RSA-Signaturpruefung (wenn Secure Boot aktiviert).
2. **HTTP-Download**: Firmware wird von einer URL heruntergeladen. TLS mit
   Certificate-Bundle. Range-Request-Resume nach Abbruch.
3. **SD-Karte**: Firmware-Datei wird von der SD-Karte gelesen.

**OTA-Sicherheitsmechanismen im Code:**

- Dual-Partition-Layout (ota_0 / ota_1) mit automatischem Rollback
- SHA-256-Hash-Verifikation der geflashten Partition
- RSA-3072-Signaturpruefung (ota_secure_boot.c, derzeit Platzhalter)
- Anti-Rollback-Counter ueber eFuses (Stub-Implementierung)
- Self-Test nach Boot vor Markierung als gueltig
- Boot-Loop-Erkennung (>3 Resets in 60s -> Rollback)

### 1.5 Aktuelle Sicherheitskonfiguration

Die `sdkconfig.defaults` zeigt den aktuellen Sicherheitsstatus:

```
# Secure Boot (disabled by default — enable for production)
# CONFIG_SECURE_BOOT_V2_ENABLED=y
# CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT=y
# Flash Encryption (disabled by default — enable for production)
# CONFIG_SECURE_FLASH_ENC_ENABLED=y
```

Secure Boot und Flash-Encryption sind **deaktiviert**. Das bedeutet:

- Firmware ist nicht signiert (jedes Image kann geflasht werden)
- Flash-Inhalt ist unverschluesselt (NVS, Konfiguration lesbar)
- RSA-Signaturpruefung in `ota_secure_boot.c` liefert im Dev-Modus
  `ESP_OK` ohne echte Pruefung (Fallback auf Hash-Only)

### 1.6 BLE-Security-Manager-Konfiguration

In `ble_service.c` ist der Security Manager konfiguriert:

```c
ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_DISP_YES_NO;  // Display + Yes/No
ble_hs_cfg.sm_bonding = 1;                          // Bonding aktiv
ble_hs_cfg.sm_mitm = 1;                             // MITM-Schutz
ble_hs_cfg.sm_sc = 1;                               // LE Secure Connections
```

LE Secure Connections mit ECDH P-256 ist aktiviert. Bonding-Keys werden
persistent in NVS gespeichert. Die IO-Capability ist auf
`BLE_SM_IO_CAP_DISP_YES_NO` gesetzt, was Numeric Comparison ermoeglicht.

---

## 2. Threat Model (STRIDE)

Die STRIDE-Methode analysiert sechs Bedrohungskategorien:
**S**poofing, **T**ampering, **R**epudiation,
**I**nformation **D**isclosure, **D**enial of **S**ervice,
**E**levation of **P**rivilege.

### 2.1 Spoofing (Identitaets-taeuschung)

#### S1: BLE-Geraete-Identitaet-Spoofing

**Komponente:** BLE-GATT-Server, Advertising
**Bedrohung:** Ein Angreifer startet einen eigenen BLE-GATT-Server mit der
gleichen Service-UUID (6e401000-b5a3-f393-e0a9-e50e24dcca9e) und dem
Geraetenamen "Transalp-BC". Die Companion-App verbindet sich mit dem
Rogue-Geraet statt mit dem echten Bordcomputer.

**Risiko:** Mittel. Die App filtert beim Scan nach der Advertising-Service-UUID
und dem Geraetenamen. Beide sind oeffentlich (in `gatt-uuids.ts` und
`gatt_uuids.h` hardcodiert). Ein Angreifer kann beide Werte kopieren.

**Auswirkung:** Der Angreifer erhaelt alle Telemetrie-Daten, die die App
sendet (Navigation, Media, Phone-Notifications). Der Angreifer kann gefaelschte
Navigationsdaten an die App senden. Der echte Bordcomputer ist nicht
beeintraechtigt, aber der Nutzer erhaelt falsche Daten.

**Erschwerend:** Die App verwendet BLE-Adresse als Geraete-ID. Bei aktiviertem
BLE Privacy (RPA) aendert sich die Adresse. Die App muss die Identity Resolving
Key (IRK) aus dem Bonding verwenden, um die Identitaet zu verifizieren.

**Mitigation:**
- Pairing mit LE Secure Connections ist konfiguriert (ECDH P-256)
- Nach erfolgreichem Bonding verwendet die App die bonded Identity
- Advertising sollte nicht die vollstaendige Service-UUID enthalten
  (nur 16-bit-kodierte UUID, schwerer zu klonen)
- In Produktion: Random Static Address statt Public Address verwenden

```c
// Empfehlung: Random Static Address fuer Produktion
// In ble_service.c, vor ble_hs_id_infer_auto():
#if CONFIG_PRODUCTION_BUILD
    uint8_t static_addr[6];
    // Generate or use factory-burned static random address
    ble_hs_id_gen_rnd(MBLE_ADDR_TYPE_STATIC, static_addr);
    ble_hs_id_set_rnd(static_addr);
#endif
```

#### S2: OTA-Quellen-Spoofing

**Komponente:** OTA-Update-System (HTTP-Transport)
**Bedrohung:** Ein Angreifer manipuliert den DNS oder den HTTP-Server, von dem
die Firmware heruntergeladen wird (Man-in-the-Middle).

**Risiko:** Mittel. Der HTTP-Client verwendet TLS mit dem ESP-IDF
Certificate-Bundle (`esp_crt_bundle_attach`). Das schuetzt vor passivem
MITM. Bei kompromittierter CA oder bei HTTP (ohne TLS) besteht keine
Schutz.

**Auswirkung:** Der Angreifer kann malicious Firmware auf das Geraet spielen,
was zu Arbitrary Code Execution fuehrt.

**Mitigation:**
- HTTPS ist in `sdkconfig.defaults` aktiviert (`CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS=y`)
- Certificate-Bundle ist konfiguriert (`esp_crt_bundle_attach`)
- Zusaetzlich: Firmware-Signaturpruefung (RSA-3072) nach Download
  vor dem Flashen. Die Signatur wird von der Companion-App mitgeliefert
  und in `ota_secure_boot.c` verifiziert.

```c
// In ota_finish_and_verify(), nach SHA-256-Check:
if (ota_secure_is_enabled()) {
    esp_err_t sig_err = ota_secure_verify_signature(
        s_ota.update_partition,
        s_ota.signature,        // 384 Bytes RSA-3072
        hash);                   // 32 Bytes SHA-256
    if (sig_err != ESP_OK) {
        ESP_LOGE(TAG, "Firmware signature verification FAILED");
        ota_set_error(OTA_ERR_VERIFY_SIG);
        return sig_err;
    }
}
```

#### S3: Companion-App-Spoofing

**Komponente:** BLE-GATT-Server (Firmware-Seite)
**Bedrohung:** Ein Angreifer verbindet sich mit dem Bordcomputer und gibt
sich als legitime Companion-App aus, um Diagnose-Befehle oder
Konfigurationsaenderungen durchzufuehren.

**Risiko:** Hoch (ohne Authentifizierung). Der GATT-Server akzeptiert
Verbindungen von jedem Geraet, das das Pairing durchlaeuft. Mit LE Secure
Connections und `sm_mitm = 1` ist das Pairing gegen MITM gesichert, aber
jedes Geraet kann den Pairing-Prozess initiieren.

**Auswirkung:** Ein Angreifer kann nach erfolgreichem Pairing alle
Write-Characteristics nutzen: Konfiguration aendern, DTCs loeschen,
OTA-Update starten.

**Mitigation:**
- Bonding auf max. 4 Geraete begrenzt (`CONFIG_BT_NIMBLE_MAX_BONDS=4`)
- Bond-Management-Service (0x6005) ermoeglicht Loeschen aller Bonds
- Diagnose- und Konfigurations-Service sollten zusaetzlich durch eine
  Admin-PIN geschuetzt werden (siehe Elevation of Privilege)

### 2.2 Tampering (Datenmanipulation)

#### T1: Firmware-Integritaet

**Komponente:** OTA-Update-System, Flash-Speicher
**Bedrohung:** Ein Angreifer manipuliert die Firmware direkt im Flash
(physischer Zugriff) oder durch ein ungesichertes OTA-Update.

**Risiko:** Hoch (ohne Secure Boot). Secure Boot V2 ist deaktiviert. Jedes
Firmware-Image kann geflasht werden, ohne dass der Bootloader die Signatur
prueft. Die RSA-Verifikation in `ota_secure_boot.c` ist ein
Anwendungs-Layer-Check, der im Dev-Modus auf Hash-Only faellt.

**Auswirkung:** Manipulierte Sensor-Werte, gefaelschte Logs, Backdoors,
Datenexfiltration ueber BLE.

**Mitigation:**
- Secure Boot V2 aktivieren (eFuse-basiert, irreversibel)
- Secure Boot V2 prueft die RSA-3072-Signatur beim Boot
- Readback-Protection durch Flash-Encryption
- Anti-Rollback-Counter verhindert Downgrade auf aeltere, bekannte
  Schwachstellen-Firmware

```c
// sdkconfig.defaults fuer Produktion:
// CONFIG_SECURE_BOOT_V2_ENABLED=y
// CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES=y
// Nach idf.py secure-boot all ist Secure Boot irreversibel aktiviert.
// Der private Schluessel muss offline aufbewahrt werden.
```

#### T2: BLE-Daten-Manipulation (OTA-Transfer)

**Komponente:** OTA-Data-Characteristic (0x8002), ota_ble_write_chunk()
**Bedrohung:** Ein Angreifer manipuliert Firmware-Chunks waehrend des
BLE-Transfers.

**Risiko:** Niedrig. Jeder Chunk wird mit CRC16-CCITT geprueft
(`ota_update.c` Zeile 757-763). Bei CRC-Fehler wird der Chunk abgelehnt,
die App kann ihn erneut senden. Nach dem kompletten Transfer wird ein
SHA-256-Hash ueber die gesamte Partition berechnet und mit dem erwarteten
Hash verglichen.

**Auswirkung:** Manipulierte Chunks werden erkannt und abgelehnt. Ein
 manipulierter Hash wuerde nur erkannt, wenn der Angreifer auch den
 erwarteten Hash in `s_ota.expected_hash` kontrollieren kann.

**Mitigation:**
- CRC16 pro Chunk (bereits implementiert)
- SHA-256-Verifikation ueber gesamte Partition (bereits implementiert)
- RSA-Signaturpruefung (implementiert, aber Platzhalter-Key)
- Dual-Partition mit Rollback bei Boot-Fehler (bereits implementiert)

```c
// ota_update.c — CRC-Pruefung pro Chunk (bereits vorhanden):
uint16_t computed_crc = crc16_ccitt(chunk->data, chunk->length);
if (computed_crc != chunk->crc16) {
    ESP_LOGE(TAG, "Chunk CRC mismatch at offset %lu", ...);
    return ESP_ERR_INVALID_CRC;
}
```

#### T3: SD-Karten-Daten-Manipulation

**Komponente:** Storage-Komponente, SD-Karte (FAT32)
**Bedrohung:** Bei physischem Zugriff kann die SD-Karte entnommen und
GPX-Tracks oder Log-Dateien manipuliert werden.

**Risiko:** Mittel (nur bei physischem Zugriff). Die SD-Karte verwendet FAT32
ohne Verschluesselung oder Integritaetspruefung.

**Auswirkung:** Gefaelschte Fahrt-Daten, manipulierte Telemetrie-Logs,
eingefuegte oder geloeschte GPX-Tracks.

**Mitigation:**
- Hash-Praefix in Log-Dateien einfuehren (Integritaets-Check)
- GPX-Dateien mit HMAC signieren (Schluessel im NVS mit Flash-Encryption)

```c
// Vorgeschlagene Log-Datei-Struktur mit Integritaets-Hash:
// Zeile 1: # HMAC-SHA256:<hex-hash>
// Zeile 2+: timestamp,lat,lon,speed_kmh,rpm,temp_c,alt_m,...
//
// Der Hash wird beim Schreiben berechnet und beim Lesen verifiziert.

static esp_err_t logger_write_with_hmac(
    const char *path, const uint8_t *data, size_t len)
{
    uint8_t hmac[32];
    // Verwende ESP-IDF HMAC-API mit eFuse-Key (Flash-Encryption)
    esp_err_t err = esp_hmac_calculate(HMAC_KEY_LOG, data, len, hmac);
    if (err != ESP_OK) return err;

    FILE *f = fopen(path, "a");
    if (!f) return ESP_FAIL;

    fprintf(f, "# HMAC-SHA256:");
    for (int i = 0; i < 32; i++)
        fprintf(f, "%02x", hmac[i]);
    fprintf(f, "\n");

    fwrite(data, 1, len, f);
    fclose(f);
    return ESP_OK;
}
```

#### T4: Konfigurations-Manipulation ueber BLE

**Komponente:** Config-Service (0x6000), CONFIG_WRITE (0x6002)
**Bedrohung:** Ein Angreifer aendert die Geraetekonfiguration ueber BLE:
Schwellwerte (rpm_redline, battery_low_v), Kalibrierung (fuel_cal,
gyro_offset), Geraetename.

**Risiko:** Hoch (ohne zusaetzliche Authentifizierung). Nach erfolgreichem
Pairing kann jeder verbundene Client die Konfiguration schreiben. Der
Config-Write-Handler in `ble_callbacks.c` hat keine Authentifizierung:

```c
// ble_callbacks.c Zeile 582-586 — keine Auth-Pruefung:
static void process_config_write(const uint8_t *data, uint16_t len)
{
    ESP_LOGI(TAG, "Config write: %d bytes", len);
    /* TODO: Parse JSON config chunk and apply via storage component */
}
```

**Auswirkung:** Manipulierte Schwellwerte koennen zu falschen Warnungen
fuehren. Geaenderter Geraetename koennte fuer Spoofing genutzt werden.
Manipulierte Kalibrierung liefert falsche Sensorwerte.

**Mitigation:**
- Admin-PIN fuer Config-Write und Config-Import (siehe Abschnitt 4.3)
- Config-Aenderungen in Audit-Log protokollieren (NVS)
- Kritische Werte (rpm_redline, battery_shutdown_v) gegen Plausibilitaet
  pruefen (Range-Check)

### 2.3 Repudiation (Nicht-Nachvollziehbarkeit)

#### R1: BLE-Verbindungs-Log fehlt

**Komponente:** BLE-GATT-Server, GAP-Event-Handler
**Bedrohung:** Es gibt keine Aufzeichnung, welche Geraete sich wann mit dem
Bordcomputer verbunden haben. Bei einer Sicherheitsuntersuchung kann nicht
nachvollzogen werden, ob ein unbefugtes Geraet Zugriff hatte.

**Risiko:** Mittel. Der GAP-Event-Handler in `ble_service.c` loggt
Connect/Disconnect nur als ESP_LOGI (serielle Konsole, fluechtig):

```c
// ble_service.c Zeile 414 — nur Log, keine persistente Aufzeichnung:
ESP_LOGI(TAG, "Connected: handle=%d", s_conn_handle);
```

**Auswirkung:** Nicht-Nachvollziehbarkeit von Verbindungen. Ein Angreifer
koennte sich verbinden, Daten abfragen und die Spuren verwischen.

**Mitigation:**
- Connection-Log in NVS persistieren (Timestamp + gekuerzte Device-Adresse)
- Ring-Buffer fuer letzte 32 Verbindungen
- Audit-Log fuer kritische Aktionen (Config-Write, DTC-Clear, OTA-Start)

```c
// Vorgeschlagene Connection-Logging-Funktion:
typedef struct __attribute__((packed)) {
    uint32_t timestamp;       // Unix-Zeit
    uint8_t  peer_addr[6];    // BLE-Adresse (gekuerzt auf 3 Bytes fuer Privacy)
    uint8_t  event_type;      // 0=connect, 1=disconnect
    uint8_t  reserved;
} ble_conn_log_entry_t;

#define CONN_LOG_MAX_ENTRIES  32
#define CONN_LOG_NVS_NS       "ble_audit"
#define CONN_LOG_NVS_KEY      "conn_log"

static esp_err_t ble_log_connection(uint8_t event_type,
                                     const uint8_t *peer_addr)
{
    static uint8_t s_log_idx = 0;
    ble_conn_log_entry_t entry;
    entry.timestamp = (uint32_t)time(NULL);
    entry.event_type = event_type;
    // Nur erste 3 Bytes der Adresse speichern (Privacy)
    memcpy(entry.peer_addr, peer_addr, 3);
    memset(entry.peer_addr + 3, 0, 3);

    nvs_handle_t h;
    esp_err_t err = nvs_open(CONN_LOG_NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    // Schreibe an Ring-Buffer-Position
    char key[16];
    snprintf(key, sizeof(key), "%s_%d", CONN_LOG_NVS_KEY, s_log_idx);
    nvs_set_blob(h, key, &entry, sizeof(entry));
    nvs_commit(h);
    nvs_close(h);

    s_log_idx = (s_log_idx + 1) % CONN_LOG_MAX_ENTRIES;
    return ESP_OK;
}
```

#### R2: Diagnose-Aktionen ohne Audit-Trail

**Komponente:** Diagnose-Service (0x7000), SELF_TEST (0x7003),
DTC-Clear (implizit ueber Config)
**Bedrohung:** Diagnose-Befehle (Self-Test, DTC-Loeschung) werden nicht
protokolliert. Ein Angreifer koennte DTCs loeschen, um Fehler zu vertuschen.

**Risiko:** Mittel. Die `process_self_test()` und `process_bond_delete()`
Funktionen haben keine Audit-Trail-Logik.

**Auswirkung:** DTC-Loeschung ist nicht nachvollziehbar. Bei Garantiefaellen
kann nicht geprueft werden, ob Fehler absichtlich geloescht wurden.

**Mitigation:**
- Audit-Log fuer alle Diagnose-Befehle in NVS
- DTC-Clear nur mit Admin-PIN ermoeglichen
- Audit-Log ueber LOG_EXPORT (0x7004) exportierbar

### 2.4 Information Disclosure (Informationspreisgabe)

#### I1: BLE-Telemetrie abhoerbar

**Komponente:** Telemetry-Service (0x5000), TLM_LIVE (0x5001)
**Bedrohung:** Ein Angreifer schnueffelt den BLE-Verkehr und faengt
Telemetrie-Daten ab (Position, Geschwindigkeit, Drehzahl, etc.).

**Risiko:** Mittel (nach Pairing). Vor Pairing sind keine Telemetrie-Daten
verfuegbar, da Notifications nur an verbundene+subscribed Clients gehen.
Nach Pairing ist die Verbindung mit AES-128-CCM verschluesselt.

**Aber:** BLE-Verschluesselung ist nur link-layer. Ein Angreifer mit
passivem Sniffer kann die BLE-Pakete auf dem Link-Layer sehen (verschluesselt),
aber nicht entschluesseln ohne den Pairing-Key. Bei aktiviertem
LE Secure Connections (ECDH P-256) ist der passive Sniff-Angriff
praktisch unmoeglich.

**Auswirkung:** Positionsdaten, Geschwindigkeit, Fahrzeugzustand
koennten an Dritte gelangen. Bei einem Motorrad-Fahrer sind
Positionsdaten besonders sensibel (Standort des Fahrzeugs).

**Mitigation:**
- Pairing erforderlich (bereits konfiguriert: `sm_mitm = 1`, `sm_sc = 1`)
- Telemetrie-Notifications nur nach CCCD-Subscribe (bereits implementiert)
- In Produktion: BLE Privacy (RPA) aktivieren, um Tracking zu erschweren

```c
// Privacy-Konfiguration in ble_configure_security():
ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ID;
// RPA aktivieren:
#if CONFIG_PRODUCTION_BUILD
    ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_SIGN;
#endif
```

#### I2: Diagnose-Daten sichtbar

**Komponente:** Diagnose-Service (0x7000), DTC_LIST (0x7001),
SENSOR_STATUS (0x7002)
**Bedrohung:** DTC-Codes und System-Status sind ueber BLE ohne zusaetzliche
Authentifizierung lesbar (nach Pairing).

**Risiko:** Mittel. Die DTC-Liste enthaelt Informationen ueber
Fahrzeugzustaende (Ueberhitzung, niedriger Akku, Sensor-Ausfaelle).
Diese Informationen sind fuer einen Angreifer interessant, um
Schwachstellen des Fahrzeugs zu identifizieren.

**Auswirkung:** Ein Angreifer koennte lernen, dass das Fahrzeug
Ueberhitzungsprobleme hat, und diese Information ausnutzen.

**Mitigation:**
- Diagnose-Service nur nach Admin-Authentifizierung (PIN) zugaenglich
- DTC-Liste erst nach PIN-Eingabe in der App lesbar

#### I3: NVS-Konfiguration lesbar

**Komponente:** NVS-Flash (Non-Volatile Storage)
**Bedrohung:** Bei deaktivierter Flash-Encryption kann der Flash-Inhalt
(physisch) ausgelesen werden. NVS enthaelt: Bonding-Keys, Konfiguration
(Geraetename, Kalibrierung), OTA-Resume-State.

**Risiko:** Hoch (ohne Flash-Encryption). Die `sdkconfig.defaults` zeigt:
`# CONFIG_SECURE_FLASH_ENC_ENABLED=y` (auskommentiert = deaktiviert).

**Auswirkung:** Ein Angreifer mit physischem Zugriff kann Bonding-Keys
auslesen und sich als bereits gepaartes Geraet ausgeben
(Identity-Spoofing ohne erneutes Pairing).

**Mitigung:**
- Flash-Encryption aktivieren (eFuse-basiert, irreversibel)
- NVS-Verschluesselung (NVS Encryption mit Flash-Encryption)
- Bonding-Keys werden durch Flash-Encryption automatisch geschuetzt

```c
// sdkconfig.defaults fuer Produktion:
// CONFIG_SECURE_FLASH_ENC_ENABLED=y
// CONFIG_SECURE_FLASH_ENCRYPTION_MODE_RELEASE=y
// CONFIG_NVS_ENCRYPTION=y
// Nach idf.py flash-encryption ist die Verschluesselung irreversibel.
```

#### I4: Serial Console in Produktion

**Komponente:** UART-Console (USB-Serial)
**Bedrohung:** Die serielle Konsole gibt detaillierte Log-Ausgaben aus
(ESP_LOGI/ESP_LOGW/ESP_LOGE), die interne Zustaende, Fehlercodes und
Konfiguration offenbaren.

**Risiko:** Niedrig (nur physisch). Die Konsole ist ueber USB-C erreichbar,
erfordert aber Gehaeuseoeffnung.

**Auswirkung:** Ein Angreifer kann Debug-Informationen auslesen, die bei
der Entwicklung von Angriffen helfen (Speicheradressen, Task-Zustaende,
Fehlermeldungen).

**Mitigation:**
- Log-Level in Produktion auf ERROR reduzieren
- Serial-Console ueber Config-Option deaktivierbar

```c
// board_config.h oder sdkconfig.defaults:
#if CONFIG_PRODUCTION_BUILD
    #define LOG_DEFAULT_LEVEL ESP_LOG_ERROR
    // ESP-IDF: CONFIG_LOG_DEFAULT_LEVEL_ERROR=y
    // Console-Output ueber USB deaktivieren:
    // CONFIG_ESP_CONSOLE_NONE=y
#else
    #define LOG_DEFAULT_LEVEL ESP_LOG_INFO
#endif
```

### 2.5 Denial of Service (Verfuegbarkeit)

#### D1: BLE-Verbindungs-Flut

**Komponente:** BLE-GAP-Event-Handler, Connection-Handling
**Bedrohung:** Ein Angreifer initiiert schnell hintereinander viele
BLE-Verbindungsversuche, um den Bordcomputer zu ueberlasten oder fuer
legitime Nutzer unerreichbar zu machen.

**Risiko:** Hoch. Der aktuelle Code hat kein Rate-Limiting fuer
Verbindungsversuche. Die `BLE_MAX_CONNECTIONS` ist auf 1 gesetzt, aber
nach jedem Disconnect wird Advertising sofort neu gestartet
(`ble_start_advertising()` im Disconnect-Handler).

**Auswirkung:** Der Bordcomputer ist fuer legitime Nutzer nicht erreichbar.
Die Companion-App kann keine Verbindung aufbauen. Telemetrie wird nicht
uebertragen (SD-Logging laeuft aber weiter).

**Mitigation:**
- Connection-Timeout: Begrenzung der Verbindungsversuche pro Zeiteinheit
- Rate-Limiting: Nach N fehlgeschlagenen Pairing-Versuchen Advertising
  fuer M Sekunden pausieren
- Verbindung nur von bekannten (bonded) Geraeten akzeptieren

```c
// Vorgeschlagenes Rate-Limiting in ble_service.c:

#define MAX_FAILED_PAIRINGS    5
#define PAIRING_COOLDOWN_SEC   30

static time_t s_last_pair_attempt = 0;
static uint8_t s_failed_pairings = 0;

static void ble_on_connect(uint16_t conn_handle)
{
    time_t now = time(NULL);

    // Cooldown-Pruefung
    if (s_failed_pairings >= MAX_FAILED_PAIRINGS) {
        if (now - s_last_pair_attempt < PAIRING_COOLDOWN_SEC) {
            ESP_LOGW(TAG, "Pairing cooldown active, rejecting");
            ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
            return;
        }
        s_failed_pairings = 0;  // Reset nach Cooldown
    }

    s_last_pair_attempt = now;
}

// Im BLE_GAP_EVENT_CONNECT Handler:
// Nach fehlgeschlagenem Pairing: s_failed_pairings++
// Nach erfolgreichem Pairing: s_failed_pairings = 0
```

#### D2: OTA-Abbruch (Brick-Risiko)

**Komponente:** OTA-Update-System
**Bedrohung:** Ein OTA-Update wird unterbrochen (Verbindungsabbruch,
Akku leer, manueller Abbruch). Das Geraet bootet nicht mehr.

**Risiko:** Niedrig. Das OTA-System hat mehrere Schutzmechanismen:

1. Dual-Partition-Layout: Das Update wird in die inaktive Partition
   geschrieben. Die aktive Partition bleibt unveraendert bis zum Commit.
2. `ESP_OTA_IMG_PENDING_VERIFY`: Nach dem Boot der neuen Firmware muss
   sie explizit als gueltig markiert werden (`esp_ota_mark_app_valid_cancel_rollback()`).
3. Self-Test nach Boot: `ota_firmware_self_test()` prueft NVS, Heap und OTA-State.
4. Automatischer Rollback bei Self-Test-Fehler.
5. Boot-Loop-Erkennung (>3 Resets in 60s).

**Aber:** Der Self-Test ist minimal (nur NVS + Heap + OTA-State). Er prueft
nicht, ob BLE, Display oder Sensoren funktionieren. Eine Firmware, die
bootet aber BLE nicht initialisiert, wuerde nicht zurueckgerollt.

**Auswirkung:** Bei einem unvollstaendigen OTA-Update bootet das Geraet
von der alten Partition. Bei einem vollstaendigen aber fehlerhaften Update
kann der Self-Test fehlschlagen und zum Rollback fuehren. Das Risiko eines
permanenten Bricks ist gering wegen des Dual-Partition-Layouts.

**Mitigung:**
- Erweiterten Self-Test implementieren (BLE, Display, SD-Karte pruefen)
- Boot-Loop-Erkennung in `ota_check_on_boot()` integrieren
- OTA-Resume nach Abbruch (bereits teilweise implementiert fuer HTTP)

```c
// Erweiterter Self-Test:
bool ota_firmware_self_test(void)
{
    // Bestehende Checks:
    // 1. NVS lesbar
    // 2. Heap > 50KB
    // 3. OTA-State = IDLE

    // Erweiterte Checks:
    // 4. BLE initialisierbar (ble_is_connected liefert false, kein Error)
    // 5. SD-Karte mountbar (event_bus EVT_SDCARD_MOUNTED)
    // 6. I2C-Sensoren erreichbar (MPU6050 + BMP280 WHO_AM_I)

    if (!event_bus_wait(EVT_SDCARD_MOUNTED, pdMS_TO_TICKS(5000))) {
        ESP_LOGE(TAG, "Self-test: SD card not mounted");
        return false;
    }

    ESP_LOGI(TAG, "Self-test passed (extended)");
    return true;
}
```

#### D3: CCCD-Subscription-Flut

**Komponente:** BLE-GATT-Server, BLE_GAP_EVENT_SUBSCRIBE
**Bedrohung:** Ein Angreifer subscribt und unsubscribt schnell
hintereinander auf Notifications, um den Bordcomputer zu ueberlasten.

**Risiko:** Niedrig. NimBLE behandelt CCCD-Aenderungen im Host-Task.
Der Bordcomputer setzt nur ein Boolean-Flag (`s_cccd_telemetry = true/false`).
Die Notify-Logik prueft das Flag vor dem Senden.

**Auswirkung:** Minimal. Keine spuerbare Beeintraechtigung.

**Mitigung:** Keine zusaetzliche erforderlich. NimBLE behandelt
CCCD-Aenderungen effizient.

### 2.6 Elevation of Privilege

#### E1: Diagnose-Befehle ohne Authentifizierung

**Komponente:** Diagnose-Service (0x7000), Self-Test (0x7003)
**Bedrohung:** Nach erfolgreichem Pairing kann jeder verbundene Client
Diagnose-Befehle ausfuehren: Self-Test starten, Log-Export anfordern.

**Risiko:** Hoch. Die `process_self_test()` Funktion hat keine
Authentifizierung. Jeder Geraet, das gepairt ist, kann den Self-Test
ausloesen, was CPU-Ressourcen bindet und Log-Ausgaben generiert.

**Auswirkung:** Ressourcenverbrauch durch wiederholte Self-Tests.
Informationspreisgabe durch Log-Export.

**Mitigation:**
- Admin-PIN fuer Diagnose-Service
- Admin-PIN fuer Config-Write, Config-Import, Bond-Management
- PIN ueber Secure Characteristic uebertragen (verschluesselt nach Pairing)

```c
// Vorgeschlagenes Admin-PIN-System:

#define ADMIN_PIN_LENGTH    4
#define ADMIN_PIN_NVS_KEY   "admin_pin"
#define ADMIN_PIN_DEFAULT   "0000"

static bool s_admin_authenticated = false;
static time_t s_admin_auth_time = 0;
#define ADMIN_AUTH_TIMEOUT_SEC  300  // 5 Minuten

// In ble_internal.h erweitern:
// typedef enum {
//     ...
//     BLE_CMD_ADMIN_AUTH,    // Admin-PIN senden
//     BLE_CMD_ADMIN_DEAUTH,  // Admin-Session beenden
// } ble_cmd_type_t;

// Neue Characteristic: ADMIN_PIN (0x6006)
// Properties: Write (nur nach Pairing verschluesselt)

static void process_admin_auth(const uint8_t *data, uint16_t len)
{
    if (len != ADMIN_PIN_LENGTH) {
        ESP_LOGW(TAG, "Admin auth: wrong PIN length");
        return;
    }

    // PIN aus NVS laden
    char stored_pin[ADMIN_PIN_LENGTH + 1];
    nvs_handle_t h;
    if (nvs_open("config", NVS_READONLY, &h) != ESP_OK) {
        return;
    }
    size_t pin_len = sizeof(stored_pin);
    if (nvs_get_str(h, ADMIN_PIN_NVS_KEY, stored_pin, &pin_len) != ESP_OK) {
        strncpy(stored_pin, ADMIN_PIN_DEFAULT, ADMIN_PIN_LENGTH);
    }
    nvs_close(h);

    // PIN vergleichen (constant-time)
    bool match = true;
    for (int i = 0; i < ADMIN_PIN_LENGTH; i++) {
        if (data[i] != (uint8_t)stored_pin[i]) {
            match = false;
        }
    }

    if (match) {
        s_admin_authenticated = true;
        s_admin_auth_time = time(NULL);
        ESP_LOGI(TAG, "Admin authenticated");
    } else {
        ESP_LOGW(TAG, "Admin auth: wrong PIN");
        // Rate-Limiting: failed_attempt_counter++
    }
}

// Helper: Prueft Admin-Status vor kritischen Operationen
static bool ble_require_admin(void)
{
    if (!s_admin_authenticated) return false;
    if (time(NULL) - s_admin_auth_time > ADMIN_AUTH_TIMEOUT_SEC) {
        s_admin_authenticated = false;
        return false;
    }
    return true;
}

// In process_config_write():
static void process_config_write(const uint8_t *data, uint16_t len)
{
    if (!ble_require_admin()) {
        ESP_LOGW(TAG, "Config write rejected: not admin");
        return;
    }
    // ... eigentliche Verarbeitung
}
```

#### E2: OTA ohne Verifikation (Arbitrary Code Execution)

**Komponente:** OTA-Service (0x8000), OTA-Data (0x8002)
**Bedrohung:** Ein Angreifer spielt eine eigene Firmware ueber BLE auf das
Geraet. Ohne Signaturpruefung wird diese ausgefuehrt.

**Risiko:** Behoben (TOT-55). Die RSA-Signaturpruefung in
`ota_secure_boot.c` ist fail-closed implementiert — es gibt keinen
Dev-Fallback mehr. Ein echter RSA-3072 Public Key ist in der Firmware
eingebettet (`ota_public_key_der.h`), und Secure Boot V2 ist in
`sdkconfig.defaults` aktiviert.

**Implementierung (aktuell):**

```c
// ota_secure_boot.c — ota_secure_verify_signature() (fail-closed, KEIN Dev-Fallback):
int ret = mbedtls_pk_parse_public_key(&pk_ctx,
                                       ota_public_key_der,
                                       OTA_PUBLIC_KEY_DER_LEN);
if (ret != 0) {
    ESP_LOGE(TAG, "RSA public key parse failed — rejecting image");
    mbedtls_pk_free(&pk_ctx);
    return ESP_ERR_INVALID_VERSION;   // Fail-closed!
}

ret = mbedtls_pk_verify(&pk_ctx, MBEDTLS_MD_SHA256,
                         hash_to_use, OTA_SHA256_LEN,
                         signature, OTA_SIGNATURE_LEN);
mbedtls_pk_free(&pk_ctx);

if (ret != 0) {
    ESP_LOGE(TAG, "RSA signature verification FAILED: -0x%04x", -ret);
    return ESP_ERR_INVALID_VERSION;   // Fail-closed!
}
```

Der RSA-3072 Public Key wird zur Build-Zeit aus dem Signing-Key generiert
(`scripts/gen_ota_key_header.py`) und als DER-Blob in
`ota_public_key_der.h` eingebettet. Der private Key ist gitignored und
wird als Paperclip-Secret verwaltet (siehe Abschnitt 4.6).

**Auswirkung (bleibend):** Ohne Secure Boot V2 waere Arbitrary Code
Execution moeglich. Mit Secure Boot V2 + fail-closed Verifikation wird
nur noch korrekt signierte Firmware akzeptiert.

**Mitigation (umgesetzt):**
- ✅ Secure Boot V2 aktiviert (`CONFIG_SECURE_BOOT_V2_ENABLED=y` in sdkconfig.defaults)
- ✅ Echter RSA-3072 Public-Key eingebettet (`ota_public_key_der.h`, 422 Bytes DER)
- ✅ Dev-Modus-Fallback entfernt (fail-closed, `ESP_ERR_INVALID_VERSION`)
- ✅ Anti-Rollback ueber eFuse `SECURE_VERSION` (popcount-basiert, irreversibel)
- ✅ Flash-Encryption aktiviert (`CONFIG_SECURE_FLASH_ENC_ENABLED=y`)
- OTA-Control (0x8001) nur nach Admin-Authentifizierung (offen: Admin-PIN-System)

#### E3: Bond-Management ohne Authentifizierung

**Komponente:** BOND_MGMT (0x6005)
**Bedrohung:** Jeder gepairte Client kann alle Bonding-Keys loeschen
(Schreibbefehl 0x00 an Characteristic 0x6005). Dies loest einen Reboot aus.

**Risiko:** Hoch. Die `process_bond_delete()` Funktion hat keine
Authentifizierung:

```c
// ble_callbacks.c Zeile 594-606 — keine Auth-Pruefung:
static void process_bond_delete(const uint8_t *data, uint16_t len)
{
    if (len < 1 || data[0] != 0x00) return;
    ESP_LOGI(TAG, "Deleting all bonding keys");
    ble_store_clear();
    esp_restart();
}
```

**Auswirkung:** Ein Angreifer kann alle Pairing-Keys loeschen, was
erfordert, dass alle legitimen Geraete neu gepairt werden muessen. Dies ist
ein Denial-of-Service und ermoeglicht ein Re-Pairing mit dem Angreifer.

**Mitigation:**
- Admin-PIN fuer Bond-Management
- Bestaetigung auf dem Bordcomputer-Display (Numeric Comparison)

---

## 3. Sicherheits-Architektur-Empfehlungen

### 3.1 Priorisierung (MoSCoW)

#### MUST (Zwingend fuer Produktion)

| ID | Massnahme | Begruendung |
|---|---|---|
| MUST-1 | Secure Boot V2 aktivieren | Verhindert_unsigned Firmware |
| MUST-2 | Flash-Encryption aktivieren | Schuetzt NVS, Bonding-Keys |
| MUST-3 | RSA-3072 Public-Key einbetten | OTA-Signaturpruefung |
| MUST-4 | Dev-Fallback in ota_secure_boot.c entfernen | Fail-closed |
| MUST-5 | Admin-PIN fuer kritische Services | EoP verhindern |
| MUST-6 | Log-Level in Produktion auf ERROR | Informationspreisgabe reduzieren |

#### SHOULD (Empfohlen fuer Produktion)

| ID | Massnahme | Begruendung |
|---|---|---|
| SHOULD-1 | NVS-Encryption aktivieren | Konfiguration schuetzen |
| SHOULD-2 | Connection-Log in NVS | Audit-Trail |
| SHOULD-3 | Rate-Limiting fuer BLE | DoS verhindern |
| MUST-7 | Serial-Console deaktivieren | Debug-Info verhindern |
| SHOULD-4 | Erweiterter OTA-Self-Test | Bessere Rollback-Entscheidung |
| SHOULD-5 | HMAC-Integritaets-Check fuer Logs | SD-Daten-Manipulation erkennen |
| SHOULD-6 | Anti-Rollback eFuse-Counter | Firmware-Downgrade verhindern |

#### COULD (Erweitert bei Bedarf)

| ID | Massnahme | Begruendung |
|---|---|---|
| COULD-1 | BLE Privacy (RPA) | Tracking erschweren |
| COULD-2 | Audit-Log fuer alle Write-Befehle | Volle Rueckverfolgbarkeit |
| COULD-3 | Config-Range-Checks | Plausibilitaet pruefen |
| COULD-4 | Secure Element fuer Key-Storage | Hardware-Isolierung |

#### WON'T (Nicht in dieser Phase)

| ID | Massnahme | Begruendung |
|---|---|---|
| WONT-1 | WLAN-Interface abschalten | Nicht vorhanden |
| WONT-2 | VPN-Tunnel fuer OTA | Uebertrieben fuer Motorrad |
| WONT-3 | Biometrische Authentifizierung | Nicht praktikabel |

### 3.2 BLE-Pairing-Architektur

**Aktuelle Konfiguration:**
- LE Secure Connections (ECDH P-256): Aktiviert
- MITM-Schutz: Aktiviert (`sm_mitm = 1`)
- Bonding: Aktiviert, max. 4 Bonds
- IO-Capability: `BLE_SM_IO_CAP_DISP_YES_NO` (Numeric Comparison)

**Empfehlung:**
Die Konfiguration ist sicher. LE Secure Connections mit Numeric
Comparison bietet MITM-Schutz. Die max. 4 Bonds sind ausreichend
(Haupttelefon, Zweitgeraet, Werkstatt, Reserve).

**Zusaetzlich empfohlen:**
- Bei erstmaligem Pairing: Bestaetigung auf dem Bordcomputer-Display
  (Numeric Comparison: 6-stellige Zahl anzeigen)
- Bond-Loeschung nur mit Admin-PIN
- Nach 3 fehlgeschlagenen Pairing-Versuchen: 30s Cooldown

### 3.3 OTA-Sicherheits-Architektur

**Schicht 1: Transport-Sicherheit**
- BLE: Verschluesselt durch Pairing (AES-128-CCM)
- HTTP: TLS mit Certificate-Bundle
- SD: Keine (physischer Zugriff erforderlich)

**Schicht 2: Integritaetspruefung**
- CRC16-CCITT pro Chunk (BLE-Transfer)
- SHA-256 ueber gesamte Partition (alle Transportwege)

**Schicht 3: Authentizitaetspruefung**
- RSA-3072-Signatur (Secure Boot V2)
- Anti-Rollback-Counter (eFuse)

**Schicht 4: Boot-Sicherheit**
- Secure Boot V2 (Bootloader prueft Signatur)
- Dual-Partition mit Rollback
- Self-Test vor Markierung als gueltig

```c
// OTA-Sicherheits-Pipeline (vollstaendig):
//
// 1. OTA begin (BLE/HTTP/SD)
//    -> Admin-PIN pruefen (BLE)
//    -> ota_begin_partition()
//
// 2. Transfer (BLE-Chunks / HTTP-Download / SD-Read)
//    -> CRC16 pro Chunk (BLE)
//    -> esp_ota_write()
//
// 3. Commit (BLE)
//    -> Admin-PIN pruefen
//    -> ota_finish_and_verify()
//       -> esp_ota_end()
//       -> SHA-256 berechnen
//       -> SHA-256 mit erwartetem Hash vergleichen
//       -> RSA-3072-Signatur verifizieren (KEIN Dev-Fallback)
//       -> Anti-Rollback-Counter pruefen
//       -> esp_ota_set_boot_partition()
//       -> esp_restart()
//
// 4. Boot (neue Firmware)
//    -> Secure Boot V2 prueft Signatur (Bootloader)
//    -> ota_check_on_boot(): PENDING_VERIFY?
//    -> ota_firmware_self_test() (erweitert)
//    -> OK: esp_ota_mark_app_valid_cancel_rollback()
//    -> FAIL: esp_ota_mark_app_invalid_rollback_and_reboot()
```

### 3.4 NVS-Sicherheit

**Aktuell:** NVS unverschluesselt, Flash-Encryption deaktiviert.

**Empfehlung:**
- Flash-Encryption aktivieren (`CONFIG_SECURE_FLASH_ENC_ENABLED=y`)
- NVS-Encryption aktivieren (`CONFIG_NVS_ENCRYPTION=y`)
- NVS-Namespace fuer sensible Daten: `config_secure`
- Admin-PIN verschluesselt in NVS gespeichert (durch Flash-Encryption)

### 3.5 Diagnose-Service-Sicherheit

**Aktuell:** Diagnose-Service ohne Authentifizierung nach Pairing.

**Empfehlung:**
- Admin-PIN fuer: SELF_TEST, LOG_EXPORT, DTC-Clear
- DTC-Liste (0x7001) und Sensor-Status (0x7002) bleiben ohne Admin-PIN
  lesbar (nur-informativ)
- Config-Service: Schreiboperationen (CONFIG_WRITE, CONFIG_IMPORT,
  BOND_MGMT) erfordern Admin-PIN
- OTA-Service: OTA_CONTROL (0x8001) erfordert Admin-PIN

---

## 4. Implementierungs-Plan

### 4.1 Phase 1: Kritische Sicherheitsfunktionen (MUST)

**Ziel:** Produktionsreife Basissicherheit

**Tasks:**

1. **Secure Boot V2 + Flash-Encryption aktivieren** ✅ (TOT-55)
   - `sdkconfig.defaults` angepasst (CONFIG_SECURE_BOOT_V2_ENABLED, CONFIG_SECURE_FLASH_ENC_ENABLED)
   - RSA-3072 Key-Pair generiert (`openssl genrsa -out ota_signing_key.pem 3072`)
   - Public-Key in `ota_public_key_der.h` eingebettet (422 Bytes DER, build-time codegen via `scripts/gen_ota_key_header.py`)
   - Build-Pipeline: `idf.py build` signiert automatisch (`CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES=y`)
   - Siehe Abschnitt 4.6 fuer Key-Management

2. **Dev-Fallback in ota_secure_boot.c entfernen** ✅ (TOT-55)
   - `ota_secure_verify_signature()`: fail-closed, `ESP_ERR_INVALID_VERSION` bei Parse-/Verifikationsfehler
   - Kein Hash-Only-Fallback mehr
   - Unit-Tests: `test/test_ota_secure_boot.c` (14 Tests, alle gruen)

3. **Admin-PIN-System implementieren**
   - Neue Characteristic: ADMIN_AUTH (0x6006, Write)
   - PIN in NVS gespeichert (Flash-Encryption geschuetzt)
   - Default-PIN: "0000" (aenderbar ueber Config-Service mit altem PIN)
   - Admin-Session: 5 Minuten gueltig, danach Re-Auth erforderlich
   - Rate-Limiting: 3 Fehlversuche -> 60s Lockout

4. **Admin-PIN in kritische Write-Handler integrieren**
   - `process_config_write()`: `ble_require_admin()` pruefen
   - `process_config_import()`: `ble_require_admin()` pruefen
   - `process_bond_delete()`: `ble_require_admin()` pruefen
   - `process_self_test()`: `ble_require_admin()` pruefen
   - `process_ota_control()`: `ble_require_admin()` pruefen

5. **Log-Level in Produktion reduzieren**
   - `sdkconfig.defaults`: `CONFIG_LOG_DEFAULT_LEVEL_ERROR=y` (Produktion)
   - Debug-Log nur in Development-Builds

### 4.2 Phase 2: Erweiterte Sicherheit (SHOULD)

**Ziel:** Audit-Trail und DoS-Schutz

**Tasks:**

1. **Connection-Log in NVS**
   - Ring-Buffer (32 Eintraege) in NVS
   - Timestamp + gekuerzte Peer-Adresse
   - Export ueber LOG_EXPORT (0x7004)

2. **Rate-Limiting fuer BLE-Verbindungen**
   - Max. 5 Pairing-Versuche in 60 Sekunden
   - 30s Cooldown nach Limit-Erreichung
   - Konfigurierbar ueber Config-Service

3. **Erweiterter OTA-Self-Test**
   - SD-Karte verfuegbar?
   - I2C-Sensoren erreichbar?
   - Display initialisiert?
   - BLE-Stack lauffaehig?

4. **HMAC-Integritaets-Check fuer Log-Dateien**
   - HMAC-SHA256 mit eFuse-Key
   - Praefix-Zeile in CSV-Log-Dateien
   - Verifikation beim GPX-Export

5. **Anti-Rollback-Counter (eFuse)**
   - `esp_efuse_write_field_bit(ESP_EFUSE_SECURE_VERSION)` nach Update
   - `ota_secure_check_anti_rollback()` in OTA-Commit einbinden

### 4.3 Phase 3: Erweitert (COULD)

**Tasks:**

1. BLE Privacy (RPA) aktivieren
2. Audit-Log fuer alle Write-Befehle
3. Config-Range-Checks (Plausibilitaet)
4. Config-Export/Import verschluesseln

### 4.4 Test-Strategie fuer Sicherheits-Features

#### Unit-Tests

```c
// test_admin_pin.c:
void test_admin_pin_correct(void) {
    // PIN setzen, korrekte PIN senden, auth=true erwarten
}

void test_admin_pin_wrong(void) {
    // Falsche PIN senden, auth=false erwarten
}

void test_admin_pin_timeout(void) {
    // Auth, 6 Min warten, auth abgelaufen erwarten
}

void test_admin_pin_lockout(void) {
    // 3x falsche PIN, Lockout erwarten
}
```

#### Integration-Tests

```c
// test_ota_security.c:
void test_ota_rejects_unsigned_firmware(void) {
    // OTA mit unsigned Image -> ota_secure_verify_signature muss fehlschlagen
}

void test_ota_accepts_signed_firmware(void) {
    // OTA mit korrekt signiertem Image -> Verifikation OK
}

void test_ota_rejects_tampered_chunk(void) {
    // Chunk mit falschem CRC -> ESP_ERR_INVALID_CRC
}

void test_ota_rollback_on_self_test_fail(void) {
    // Firmware mit defektem Self-Test -> Rollback zur alten Partition
}
```

#### Penetration-Tests

1. **BLE-Spoofing-Test:** Rogue-Geraet mit gleicher UUID/Name starten.
   App muss Rogue-Geraet ablehnen (Bonding-Identity-Check).
2. **OTA-MITM-Test:** BLE-Verkehr abfangen, Chunks manipulieren.
   CRC/SHA muss Abbruch erzeugen.
3. **Admin-PIN-Brute-Force-Test:** 100 PIN-Kombinationen senden.
   Lockout nach 3 Versuchen muss greifen.
4. **Connection-Flood-Test:** 100 Verbindungsversuche in 10 Sekunden.
   Cooldown muss greifen.
5. **NVS-Readback-Test:** Flash auslesen (mit Flash-Encryption).
   NVS-Inhalt muss unlesbar sein.

### 4.5 Code-Snippets fuer kritische Sicherheitsfunktionen

#### 4.5.1 Admin-PIN-Vergleich (constant-time)

```c
// WICHTIG: Constant-time comparison verhindert Timing-Angriffe
static bool admin_pin_compare(const uint8_t *input, uint8_t input_len,
                               const char *stored, uint8_t stored_len)
{
    if (input_len != stored_len) {
        return false;
    }

    volatile uint8_t diff = 0;
    for (uint8_t i = 0; i < stored_len; i++) {
        diff |= input[i] ^ (uint8_t)stored[i];
    }

    return diff == 0;
}
```

#### 4.5.2 OTA-Signatur-Verifikation (fail-closed)

```c
// ota_secure_boot.c — implementierte Version (TOT-55):
esp_err_t ota_secure_verify_signature(const esp_partition_t *partition,
                                       const uint8_t *signature,
                                       const uint8_t *hash)
{
    if (partition == NULL || signature == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 1. SHA-256 berechnen (falls nicht uebergeben)
    uint8_t computed_hash[OTA_SHA256_LEN];
    const uint8_t *hash_to_use = hash;
    if (hash_to_use == NULL) {
        esp_err_t err = ota_compute_partition_sha256(partition, computed_hash);
        if (err != ESP_OK) return err;
        hash_to_use = computed_hash;
    }

    // 2. Public-Key parsen (KEIN Dev-Fallback, fail-closed)
    mbedtls_pk_context pk_ctx;
    mbedtls_pk_init(&pk_ctx);

    int ret = mbedtls_pk_parse_public_key(&pk_ctx,
                                           ota_public_key_der,
                                           OTA_PUBLIC_KEY_DER_LEN);
    if (ret != 0) {
        ESP_LOGE(TAG, "RSA public key parse failed — rejecting image");
        mbedtls_pk_free(&pk_ctx);
        return ESP_ERR_INVALID_VERSION;  // Fail-closed!
    }

    // 3. RSA-3072 verifizieren
    ret = mbedtls_pk_verify(&pk_ctx, MBEDTLS_MD_SHA256,
                             hash_to_use, OTA_SHA256_LEN,
                             signature, OTA_SIGNATURE_LEN);
    mbedtls_pk_free(&pk_ctx);

    if (ret != 0) {
        ESP_LOGE(TAG, "RSA signature verification FAILED: -0x%04x", -ret);
        return ESP_ERR_INVALID_VERSION;  // Fail-closed!
    }

    ESP_LOGI(TAG, "RSA signature verified OK");
    return ESP_OK;
}
```

#### 4.5.3 Rate-Limiting fuer Pairing

```c
#define MAX_PAIR_ATTEMPTS     5
#define PAIR_COOLDOWN_MS      30000

static uint8_t s_pair_attempts = 0;
static TickType_t s_last_pair_reject = 0;

bool ble_check_pairing_allowed(void)
{
    if (s_pair_attempts >= MAX_PAIR_ATTEMPTS) {
        TickType_t elapsed = xTaskGetTickCount() - s_last_pair_reject;
        if (elapsed < pdMS_TO_TICKS(PAIR_COOLDOWN_MS)) {
            return false;  // Cooldown aktiv
        }
        s_pair_attempts = 0;  // Reset nach Cooldown
    }
    return true;
}

void ble_on_pair_failure(void)
{
    s_pair_attempts++;
    if (s_pair_attempts >= MAX_PAIR_ATTEMPTS) {
        s_last_pair_reject = xTaskGetTickCount();
        ESP_LOGW(TAG, "Pairing locked: %d attempts", s_pair_attempts);
    }
}

void ble_on_pair_success(void)
{
    s_pair_attempts = 0;
}
```

#### 4.5.4 Secure Config-Write mit Admin-Check

```c
static void process_config_write(const uint8_t *data, uint16_t len)
{
    // Admin-Authentifizierung pruefen
    if (!ble_require_admin()) {
        ESP_LOGW(TAG, "Config write rejected: admin auth required");
        // Notify Fehler-Code an App
        uint8_t err = 0xFF;  // ERR_NOT_AUTHORIZED
        ble_notify(ble_val_handles[CHR_H_CONFIG_READ], &err, 1);
        return;
    }

    // Laengen-Pruefung
    if (len == 0 || len > 244) {
        ESP_LOGW(TAG, "Config write: invalid length %d", len);
        return;
    }

    // TODO: Parse JSON config chunk and apply via storage component
    // WICHTIG: Range-Checks fuer kritische Werte:
    // - rpm_redline: 1000..15000
    // - battery_shutdown_v: 9.0..12.0
    // - coolant_temp_crit: 100..150

    ESP_LOGI(TAG, "Config write accepted: %d bytes", len);

    // Audit-Log
    ble_log_audit(AUDIT_CONFIG_WRITE, len);
}
```

#### 4.5.5 NVS-Encryption-Konfiguration

```python
# sdkconfig.defaults (Produktion):
CONFIG_SECURE_BOOT_V2_ENABLED=y
CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES=y
CONFIG_SECURE_FLASH_ENC_ENABLED=y
CONFIG_SECURE_FLASH_ENCRYPTION_MODE_RELEASE=y
CONFIG_NVS_ENCRYPTION=y
CONFIG_NVS_ASSERT_ERROR_CHECK=y

# Log-Level:
CONFIG_LOG_DEFAULT_LEVEL_ERROR=y
CONFIG_LOG_MAXIMUM_LEVEL_ERROR=y

# Console deaktivieren:
CONFIG_ESP_CONSOLE_NONE=y
```

### 4.6 Secure Boot Key Management (TOT-55)

Dieser Abschnitt dokumentiert den Lebenszyklus des RSA-3072 Secure Boot
Signing Keys: Generierung, Speicherung, Build-Integration, Rotation und
Produktions-Provisionierung.

#### 4.6.1 Schluessel-Hierarchie

| Komponente | Typ | Speicherort | Git-tracked |
|---|---|---|---|
| `ota_signing_key.pem` | RSA-3072 Private Key | Paperclip Secret `firmware/secure_boot/ota_signing_key` (env: `OTA_SIGNING_KEY`) | Nein (`.gitignore`) |
| `ota_public_key.der` | RSA-3072 Public Key (DER) | `firmware/secure_boot/ota_public_key.der` | Ja |
| `ota_public_key_der.h` | C-Header (DER als Byte-Array) | `firmware/components/ota/include/ota_public_key_der.h` | Ja (auto-generiert) |
| eFuse Digest | SHA-256 Digest des Public Keys | ESP32-S3 eFuse Block (irreversibel) | N/A (hardware) |

#### 4.6.2 Schluessel-Generierung

```bash
# RSA-3072 Private Key generieren (PEM, PKCS#1)
openssl genrsa -out firmware/secure_boot/ota_signing_key.pem 3072

# Public Key als DER extrahieren (wird von gen_ota_key_header.py verwendet)
openssl rsa -in firmware/secure_boot/ota_signing_key.pem \
    -pubout -outform DER \
    -out firmware/secure_boot/ota_public_key.der

# C-Header generieren (build-time codegen)
python3 firmware/scripts/gen_ota_key_header.py \
    firmware/secure_boot/ota_signing_key.pem \
    firmware/components/ota/include/ota_public_key_der.h
```

Das Skript `scripts/gen_ota_key_header.py` validiert die Key-Groesse
(~422 Bytes DER fuer RSA-3072) und gibt eine Warnung bei abweichender
Laenge aus.

#### 4.6.3 Build-Integration

Die Build-Zeit-Codegenerierung ist in `components/ota/CMakeLists.txt`
integriert:

```cmake
set(SIGNING_KEY "${CMAKE_SOURCE_DIR}/secure_boot/ota_signing_key.pem")

if(EXISTS "${SIGNING_KEY}")
    # Private Key vorhanden: Header regenerieren
    add_custom_command(
        OUTPUT "${KEY_HEADER}"
        COMMAND python3 "${KEYGEN_SCRIPT}" "${SIGNING_KEY}" "${KEY_HEADER}"
        DEPENDS "${SIGNING_KEY}" "${KEYGEN_SCRIPT}"
    )
    add_dependencies(${COMPONENT_LIB} ota_key_header)
endif()
```

- **Lokaler Build:** Wenn der Private Key vorhanden ist, wird der Header
  bei jedem Build regeneriert.
- **CI-Build:** Wenn der Private Key fehlt (z.B. CI ohne Secret), wird
  der committete Header aus `include/ota_public_key_der.h` verwendet.
- **Produktions-Build:** Der Private Key wird als Paperclip Secret
  (`env.OTA_SIGNING_KEY`) in die CI-Umgebung injiziert. Die
  `sdkconfig.defaults` Einstellung
  `CONFIG_SECURE_BOOT_SIGNING_KEY="secure_boot/ota_signing_key.pem"`
  sorgt dafuer, dass `idf.py build` die Firmware automatisch signiert.

#### 4.6.4 Private-Key-Speicherung

Der Private Key darf **niemals** in das Git-Repository committet werden
(`firmware/secure_boot/*.pem` ist in `.gitignore` eingetragen).

**Verwaltung ueber Paperclip Secrets:**
- Der Private Key wird als Paperclip Secret `firmware/secure_boot/ota_signing_key`
  gespeichert (Board-genehmigt).
- Die CI-Umgebung erhaelt den Key als Environment-Variable `OTA_SIGNING_KEY`
  (Binding: `env.OTA_SIGNING_KEY`).
- Der Secret-Wert wird nie in Logs, Kommentaren, Issues oder Dateien
  gespeichert. Paperclip protokolliert jeden Zugriff audit-sicher.

**Produktions-Empfehlung:** Fuer Hochsicherheits-Deployments sollte der
Private Key in einem HSM (Hardware Security Module) oder einem
Cloud-KMS (z.B. AWS KMS, Google Cloud KMS) gespeichert werden, und die
Signierung sollte remote erfolgen (offline signing).

#### 4.6.5 Key-Rotation-Policy

**Wann rotieren:**
- Bei Kompromittierungsverdacht (Private Key leaked)
- Periodisch (Empfehlung: alle 2 Jahre fuer Motorrad-Elektronik)
- Bei Hersteller- oder Teamwechsel

**Rotations-Prozess:**
1. Neuen RSA-3072 Key generieren (Abschnitt 4.6.2)
2. Neuen Public Key in Firmware einbetten (Header regenerieren)
3. Firmware mit NEUEM Key signieren und als OTA-Update ausliefern
4. Geraete aktualisieren sich per OTA (alte Firmware akzeptiert noch
   alte Signatur; neue Firmware enthaelt neuen Public Key)
5. Nach Uebergang: eFuse-Digest bleibt bestehen (irreversibel!)

**WICHTIG — eFuse-Irreversibilitaet:** Secure Boot V2 brennt den
Public-Key-Digest in den eFuse. Dieser kann **nicht** zurueckgesetzt
werden. Eine Key-Rotation aendert nur den Anwendungs-Layer-Public-Key
(`ota_public_key_der.h`), nicht den eFuse-Digest. Der eFuse-Digest
bleibt fuer die Bootloader-Verifikation gueltig. Fuer eine
bootloader-Level-Rotation muss die Hardware physisch ausgetauscht
werden (oder Secure Boot V2 mit Key-Revocation-Slots konfiguriert
werden, falls verfuegbar).

#### 4.6.6 Produktions-Provisionierung

Bei der ersten Inbetriebnahme (First Boot) mit aktiviertem Secure Boot V2:

1. Der Bootloader verifiziert die Firmware-Signatur gegen den
   eFuse-Digest (wird beim ersten Boot gebrannt).
2. Flash-Encryption wird aktiviert (NVS, Bonding-Keys verschluesselt).
3. Anti-Rollback: `SECURE_VERSION` eFuse wird auf die Firmware-Version
   gesetzt.

```bash
# Produktions-Build (mit Private Key in CI-Umgebung)
idf.py build  # signiert automatisch

# Flashen + Secure Boot aktivieren (einmalig, irreversibel)
idf.py -p /dev/cu.usbmodem* flash
# Nach dem ersten Boot: eFuse Secure Boot + Flash-Enc irreversibel aktiv
```

**Test-Verfahren (Akzeptanzkriterien TOT-55):**
1. Signiertes Firmware-Image bootet erfolgreich ✅ (Signatur-Pipeline
   mit OpenSSL verifiziert: Sign → Verify OK)
2. Uns signiertes Firmware-Image wird vom Bootloader abgelehnt ✅
   (fail-closed in `ota_secure_verify_signature()`, Unit-Test
   `test_ota_secure_boot.c` bestätigt `ESP_ERR_INVALID_VERSION`)
3. Tampered Image wird abgelehnt ✅ (OpenSSL-Verifikation: tampered →
   "Verification Failure")

**Hinweis:** Die Hardware-Level-Tests (physisches Booten mit/ohne
Signatur) erfordern ein ESP32-S3-Dev-Kit und muessen vor dem
Produktions-Release durchgefuehrt werden.

---

## 5. Secure-Code-Review-Checkliste

Diese Checkliste dient der systematischen Ueberpruefung des Codes auf
Sicherheitsrelevante Aspekte. Sie soll vor jedem Release durchlaufen werden.

### 5.1 BLE-Sicherheit

- [ ] LE Secure Connections aktiviert (`sm_sc = 1`)
- [ ] MITM-Schutz aktiviert (`sm_mitm = 1`)
- [ ] Bonding persistent (`sm_bonding = 1`, `BT_NIMBLE_NVS_PERSIST=y`)
- [ ] Max. Bonds begrenzt (`BT_NIMBLE_MAX_BONDS=4`)
- [ ] CCCD-Subscription wird vor jedem Notify geprueft
- [ ] Write-Handler validieren Input-Laenge (`len > 244` -> reject)
- [ ] Kritische Write-Handler erfordern Admin-PIN
- [ ] Keine sensiblen Daten in Advertising-Packets
- [ ] Disconnect loescht CCCD-Subscription-State

### 5.2 OTA-Sicherheit

- [ ] CRC16-CCITT pro Chunk wird geprueft
- [ ] SHA-256-Hash wird nach Flash verglichen
- [ ] RSA-3072-Signatur wird verifiziert (KEIN Dev-Fallback)
- [ ] Anti-Rollback-Counter wird geprueft
- [ ] Dual-Partition mit Rollback aktiv
- [ ] Self-Test nach Boot implementiert (erweitert)
- [ ] OTA-Control erfordert Admin-PIN
- [ ] OTA-Image-Groesse wird geprueft (`< OTA_MAX_IMAGE_SIZE`)
- [ ] OTA-Partition wird bei Fehler bereinigt (`ota_cleanup()`)
- [ ] HTTP-Download verwendet TLS (`esp_crt_bundle_attach`)

### 5.3 Daten-Sicherheit

- [ ] Flash-Encryption aktiviert
- [ ] NVS-Encryption aktiviert
- [ ] Keine Hartcodierten Passwoerter/Keys im Quellcode
- [ ] RSA-Private-Key nicht im Repository
- [ ] SD-Karten-Daten mit Integritaets-Hash (HMAC)
- [ ] Log-Dateien enthalten keine sensiblen Daten (Positionsdaten in Logs
      sind fuer den Use-Case erforderlich, aber nicht ueber BLE ohne Auth)
- [ ] NVS-BLOBs haben Schema-Version fuer Migration

### 5.4 Code-Qualitaet

- [ ] Keine `strcpy`/`sprintf` ohne Laengenbegrenzung
- [ ] `memcpy` mit Laengenpruefung vor Aufruf
- [ ] Mutex fuer alle geteilten Ressourcen (s_data_mutex, ota_mutex, spi_mutex)
- [ ] Keine `vTaskDelay(portMAX_DELAY)` in kritischen Pfaden
- [ ] Static_assert fuer Binary-Protokoll-Strukturen (telemetry_live_t,
      dtc_binary_t, sensor_status_t)
- [ ] Error-Handling: fail-closed (bei Fehler sicherer Zustand)
- [ ] Keine `TODO` in sicherheitsrelevantem Code

### 5.5 Build-Konfiguration

- [ ] Secure Boot V2 aktiviert (Produktion)
- [ ] Flash-Encryption aktiviert (Produktion)
- [ ] NVS-Encryption aktiviert (Produktion)
- [ ] Log-Level ERROR (Produktion)
- [ ] Serial-Console deaktiviert (Produktion)
- [ ] Compiler-Flags: `-fstack-protector-strong`, `-D_FORTIFY_SOURCE=2`
- [ ] Keine Debug-Symbole im Production-Build

### 5.6 Physische Sicherheit

- [ ] Gehaeuse verschraubt (Sicherheitsschrauben)
- [ ] SD-Karte nur nach Gehaeuseoeffnung zugaenglich
- [ ] UART-Pins nicht auf externem Header
- [ ] Boot-Pin (GPIO0) nicht leicht erreichbar
- [ ] eFuses gebrannt (Secure Boot, Flash-Enc, Anti-Rollback)

---

## Anhang: Risiko-Matrix

| Threat ID | Kategorie | Wahrscheinlichkeit | Auswirkung | Risiko | Status |
|---|---|---|---|---|---|
| S1 | Spoofing | Mittel | Mittel | Mittel | Mitigation: Pairing |
| S2 | Spoofing | Niedrig | Kritisch | Hoch | Mitigation: TLS + Signatur |
| S3 | Spoofing | Mittel | Hoch | Hoch | Mitigation: Admin-PIN |
| T1 | Tampering | Niedrig* | Kritisch | Hoch | Mitigation: Secure Boot |
| T2 | Tampering | Niedrig | Hoch | Mittel | Mitigation: CRC + SHA + RSA |
| T3 | Tampering | Niedrig* | Mittel | Niedrig | Mitigation: HMAC |
| T4 | Tampering | Hoch | Hoch | Hoch | Mitigation: Admin-PIN |
| R1 | Repudiation | Sicher | Mittel | Mittel | Mitigation: NVS-Log |
| R2 | Repudiation | Sicher | Mittel | Mittel | Mitigation: Audit-Log |
| I1 | Info Disclosure | Niedrig | Mittel | Niedrig | Mitigation: Pairing |
| I2 | Info Disclosure | Mittel | Niedrig | Niedrig | Mitigation: Admin-PIN |
| I3 | Info Disclosure | Niedrig* | Hoch | Mittel | Mitigation: Flash-Enc |
| I4 | Info Disclosure | Niedrig* | Niedrig | Niedrig | Mitigation: Log-Level |
| D1 | DoS | Hoch | Mittel | Hoch | Mitigation: Rate-Limit |
| D2 | DoS | Niedrig | Kritisch | Mittel | Mitigation: Rollback |
| D3 | DoS | Niedrig | Niedrig | Niedrig | Keine Massnahme noetig |
| E1 | EoP | Hoch | Mittel | Hoch | Mitigation: Admin-PIN |
| E2 | EoP | Mittel | Kritisch | Kritisch | Mitigation: Secure Boot + Signatur |
| E3 | EoP | Hoch | Mittel | Hoch | Mitigation: Admin-PIN |

*\* Erfordert physischen Zugriff*

---

## Anhang: Referenzen

- **STRIDE-Modell:** Spoofing, Tampering, Repudiation, Information Disclosure, Denial of Service, Elevation of Privilege
- **BLE Security:** Bluetooth Core Specification 4.2 (LE Secure Connections)
- **ESP32-S3 Secure Boot V2:** ESP-IDF Documentation, Security Features
- **RSA-3072:** NIST FIPS 186-4, Digital Signature Standard
- **SHA-256:** NIST FIPS 180-4, Secure Hash Standard
- **CRC16-CCITT:** ITU-T V.41, CRC-16-CCITT (Polynom 0x1021)

**Interne Referenzen:**
- `docs/ARCHITECTURE.md` — System-Architektur, BLE-Protokoll
- `docs/research/11-firmware-architecture.md` — Firmware-Architektur, OTA
- `docs/research/13-connectivity-app.md` — Connectivity-Konzept
- `firmware/components/ble/ble_service.c` — NimBLE GATT-Server
- `firmware/components/ble/ble_callbacks.c` — GATT-Handler
- `firmware/components/ota/ota_update.c` — OTA-Manager
- `firmware/components/ota/ota_secure_boot.c` — Secure-Boot-Support
- `firmware/sdkconfig.defaults` — Build-Konfiguration
- `companion-app/src/core/ble/gatt-uuids.ts` — GATT-UUID-Definitionen

---

*Dieses Dokument ist die zentrale Sicherheitsreferenz fuer den Transalp
Bordcomputer. Bei Aenderungen an der Sicherheitsarchitektur muss diese
Datei aktualisiert werden. Die Implementierung der Mitigationen muss
mit dem Secure-Code-Review (Abschnitt 5) verifiziert werden.*

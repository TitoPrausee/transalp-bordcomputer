#ifndef _OTA_UPDATE_H_
#define _OTA_UPDATE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "esp_app_desc.h"
#include "esp_ota_ops.h"

/*
 * ota_update.h — OTA Update Manager
 *
 * Verwaltet Firmware-Updates ueber drei Transportwege (HTTP, BLE, SD-Karte)
 * mit dualer OTA-Partition (ota_0 / ota_1), SHA-256-Verifikation,
 * RSA-Signatur-Pruefung und automatischem Rollback bei Boot-Fehlern.
 *
 * State-Machine:
 *
 *   IDLE -> DOWNLOADING -> VERIFYING -> FLASHING -> REBOOTING
 *     ^          |             |            |
 *     |          v             v            v
 *     +---- ERROR <----- (jeder Fehler) <---+
 *
 * Siehe docs/ARCHITECTURE.md Abschnitt "OTA" und
 *      docs/research/11-firmware-architecture.md Abschnitt 6.
 *
 * Thread-safety: Alle ota_* Funktionen sind thread-safe.
 * Der interne State wird durch einen Mutex geschuetzt.
 * Progress-Callbacks werden aus dem aufrufenden Task heraus aufgerufen.
 */

/* ============================================================
 * Configuration
 * ============================================================ */

/* OTA-Chunkgroesse fuer BLE-Transfer (muss ein Vielfaches von 4KB sein) */
#define OTA_CHUNK_SIZE           4096

/* OTA HTTP-Download: Buffergroesse */
#define OTA_HTTP_BUFFER_SIZE     1024

/* OTA HTTP-Timeout fuer Verbindungsversuche (ms) */
#define OTA_HTTP_CONNECT_TIMEOUT_MS  15000

/* OTA HTTP-Timeout fuer Datentransfer (ms) */
#define OTA_HTTP_DOWNLOAD_TIMEOUT_MS 30000

/* OTA BLE: maximale Chunk-Payload pro GATT-Write (MTU 247 - 3 ATT-Header) */
#define OTA_BLE_MTU_PAYLOAD     244

/* OTA-Task Stack-Groesse */
#define OTA_TASK_STACK_SIZE     8192

/* OTA-Task Prioritaet */
#define OTA_TASK_PRIORITY      4

/* Maximale Firmware-Image-Groesse (3 MB = Partitiongroesse) */
#define OTA_MAX_IMAGE_SIZE      (3 * 1024 * 1024)

/* RSA Public-Key-Laenge (3072-bit fuer Secure Boot V2) */
#define OTA_RSA_KEY_LEN         384

/* SHA-256 Hash-Laenge */
#define OTA_SHA256_LEN          32

/* Signatur-Laenge fuer RSA-3072 */
#define OTA_SIGNATURE_LEN      OTA_RSA_KEY_LEN

/* Maximale Laenge eines Changelog-Eintrags */
#define OTA_CHANGELOG_MAX_LEN  512

/* Maximale URL-Laenge fuer HTTP-Download */
#define OTA_URL_MAX_LEN        256

/* Maximale Pfadlaenge fuer SD-Karten-Dateien */
#define OTA_SD_PATH_MAX_LEN    128

/* NVS-Namespace fuer OTA-Metadaten */
#define OTA_NVS_NAMESPACE      "ota_meta"

/* NVS-Keys */
#define OTA_NVS_KEY_VERSION    "fw_ver"
#define OTA_NVS_KEY_RESUME     "resume"
#define OTA_NVS_KEY_OFFSET     "offset"

/* ============================================================
 * OTA State Enum
 * ============================================================ */

typedef enum {
    OTA_STATE_IDLE = 0,         /* Kein Update aktiv */
    OTA_STATE_DOWNLOADING,      /* Download/Transfer laeuft */
    OTA_STATE_VERIFYING,        /* Hash- und Signaturpruefung */
    OTA_STATE_FLASHING,         /* Schreiben ins Flash */
    OTA_STATE_REBOOTING,        /* Neustart wird vorbereitet */
    OTA_STATE_ERROR,            /* Fehlerzustand */
    OTA_STATE_ROLLBACK,         /* Rollback laeuft */
} ota_state_t;

/* ============================================================
 * OTA Transport-Wege
 * ============================================================ */

typedef enum {
    OTA_TRANSPORT_NONE = 0,
    OTA_TRANSPORT_HTTP,     /* Download via HTTP-URL */
    OTA_TRANSPORT_BLE,      /* Empfang ueber BLE-Chunks */
    OTA_TRANSPORT_SD,       /* Datei von SD-Karte */
} ota_transport_t;

/* ============================================================
 * OTA Error-Codes
 * ============================================================ */

typedef enum {
    OTA_ERR_NONE = 0,
    OTA_ERR_NO_PARTITION,       /* Keine inaktive OTA-Partition gefunden */
    OTA_ERR_ALREADY_RUNNING,    /* OTA laeuft bereits */
    OTA_ERR_INVALID_URL,        /* URL/Dateipfad ungueltig */
    OTA_ERR_DOWNLOAD_FAILED,    /* HTTP-Download fehlgeschlagen */
    OTA_ERR_WRITE_FAILED,       /* Flash-Schreibfehler */
    OTA_ERR_VERIFY_HASH,        /* SHA-256-Verifikation fehlgeschlagen */
    OTA_ERR_VERIFY_SIG,         /* RSA-Signatur ungueltig */
    OTA_ERR_IMAGE_TOO_LARGE,    /* Firmware-Image zu gross */
    OTA_ERR_CRC_MISMATCH,       /* BLE-Chunk CRC-Fehler */
    OTA_ERR_ABORTED,            /* Update abgebrochen */
    OTA_ERR_NO_SPACE,           /* Nicht genug Flash-Speicher */
    OTA_ERR_NOT_FOUND,          /* Datei auf SD nicht gefunden */
    OTA_ERR_INVALID_STATE,      /* State-Machine in falschem Zustand */
    OTA_ERR_SECURE_BOOT,        /* Secure-Boot-Verletzung */
    OTA_ERR_ANTI_ROLLBACK,      /* Firmware-Version zu alt */
    OTA_ERR_INTERNAL,           /* Interner Fehler */
} ota_error_t;

/* ============================================================
 * Progress-Callback
 *
 * Wird periodisch waehrend des Downloads/Flashens aufgerufen.
 *   percent   - Fortschritt 0..100
 *   state     - Aktueller OTA-State
 *   error     - OTA_ERR_NONE solange kein Fehler
 *   user_ctx  - Beliebiger Kontext (bei ota_init gesetzt)
 * ============================================================ */

typedef void (*ota_progress_cb_t)(uint8_t percent,
                                   ota_state_t state,
                                   ota_error_t error,
                                   void *user_ctx);

/* ============================================================
 * Version-Info
 * ============================================================ */

typedef struct {
    char current_version[32];          /* Aktuell laufende Version */
    char new_version[32];               /* Version des neuen Images */
    char changelog[OTA_CHANGELOG_MAX_LEN];
} ota_version_info_t;

/* ============================================================
 * BLE-Chunk-Struktur (siehe 11-firmware-architecture.md 6.2)
 * ============================================================ */

typedef struct __attribute__((packed)) {
    uint32_t offset;           /* Byte-Offset im Firmware-Image */
    uint16_t length;            /* Datenlaenge (<= OTA_CHUNK_SIZE) */
    uint16_t crc16;             /* CRC16-CCITT ueber data[] */
    uint8_t  data[OTA_CHUNK_SIZE];
} ota_chunk_t;

/* ============================================================
 * Public API
 * ============================================================ */

/**
 * Initialisiert das OTA-Subsystem.
 *
 * Muss vor allen anderen ota_* Funktionen aufgerufen werden.
 * Prueft Boot-State (Pending-Verify) und fuehrt ggf. Rollback aus.
 *
 * @param progress_cb  Callback fuer Fortschritts-Updates (darf NULL sein)
 * @param user_ctx     Anwender-Kontext fuer Callback (darf NULL sein)
 * @return ESP_OK bei Erfolg
 */
esp_err_t ota_init(ota_progress_cb_t progress_cb, void *user_ctx);

/**
 * Startet ein OTA-Update von einer HTTP-URL.
 *
 * Laedt die Firmware-Binaerdatei via HTTP herunter und flasht sie.
 * Unterstuetzt Range-Requests fuer Resume nach Abbruch.
 *
 * @param url  HTTP(S)-URL zur Firmware-Binärdatei
 * @return ESP_OK bei Erfolg
 */
esp_err_t ota_update_from_url(const char *url);

/**
 * Startet ein OTA-Update ueber BLE.
 *
 * Diese Funktion initialisiert den BLE-Transfer und kehrt zurueck.
 * Die eigentlichen Daten werden ueber ota_ble_write_chunk() empfangen.
 * Die Companion-App muss zuerst die erwartete Gesamtgroesse senden.
 *
 * @param total_size  Erwartete Groesse des Firmware-Images in Bytes
 * @return ESP_OK bei Erfolg
 */
esp_err_t ota_update_from_ble(uint32_t total_size);

/**
 * Startet ein OTA-Update von der SD-Karte.
 *
 * @param path  Pfad zur Firmware-Binaerdatei auf der SD-Karte (z.B. "/sd/firmware.bin")
 * @return ESP_OK bei Erfolg
 */
esp_err_t ota_update_from_sd(const char *path);

/**
 * Schreibt einen BLE-Firmware-Chunk ins Flash.
 *
 * Wird von der BLE-Callback-Schicht aufgerufen, wenn ein Chunk
 * ueber die OTA-Data-Characteristic (0x8002) empfangen wurde.
 * Prueft CRC16 und schreibt die Daten in die OTA-Partition.
 *
 * @param chunk  Zeiger auf den OTA-Chunk
 * @return ESP_OK bei Erfolg, ESP_ERR_INVALID_CRC bei CRC-Fehler
 */
esp_err_t ota_ble_write_chunk(const ota_chunk_t *chunk);

/**
 * Beendet den BLE-Transfer und startet die Verifikation.
 *
 * Wird aufgerufen, wenn die Companion-App "commit" sendet.
 * Fuehrt SHA-256-Verifikation durch und rebootet bei Erfolg.
 *
 * @param expected_hash  Erwarteter SHA-256-Hash (32 Bytes), darf NULL sein
 * @return ESP_OK bei Erfolg (reboot folgt)
 */
esp_err_t ota_ble_commit(const uint8_t *expected_hash);

/* ============================================================
 * OTA Verify Result (fuer CHR_OTA_VERIFY 0x8003 notify)
 * Siehe docs/BLE-INTERFACE-SPEC.md §9.3.
 * ============================================================ */

/* Verify-Result Status-Byte (0x8003 notify Offset 0). */
enum ota_verify_status {
    OTA_VERIFY_UNKNOWN       = 0,  /* Kein Verify durchgefuehrt */
    OTA_VERIFY_VERIFYING     = 1,  /* Verify laeuft */
    OTA_VERIFY_OK            = 2,  /* Hash verifiziert, Image akzeptiert */
    OTA_VERIFY_HASH_MISMATCH = 3,  /* Erwarteter Hash stimmt nicht ueberein */
    OTA_VERIFY_ERROR         = 4,  /* Sonstiger Verify-Fehler */
};

/* Verify-Result, das in ota_finish_and_verify() berechnet wird.
 * Die BLE-Schicht liest es nach ota_ble_commit() (Fehler-Pfad), um
 * den 0x8003-Notify aufzubauen. Payload-Groesse: OTA_VERIFY_NOTIFY_SIZE. */
typedef struct {
    uint8_t  status;                 /* ota_verify_status enum */
    uint8_t  sha256[OTA_SHA256_LEN]; /* Berechneter Image-Hash */
    uint32_t size;                   /* Geschriebene Bytes */
    uint32_t version;                /* Firmware-Version (best-effort, 0 = unbekannt) */
} ota_verify_result_t;

/**
 * Liefert das letzte OTA-Verify-Result.
 *
 * Wird nach ota_ble_commit() (Fehler-Pfad) aufgerufen, um den
 * CHR_OTA_VERIFY (0x8003) Notify mit dem berechneten Hash und
 * Fehlergrund aufzubauen. Auf dem Erfolgspfad rebootet das Geraet
 * innerhalb von ota_ble_commit(), bevor der Notify zugestellt
 * werden kann (geplanter Verify/Commit-Split: TOT-33).
 *
 * @param out  Ausgabe-Struktur (vom Aufrufer allokiert)
 * @return ESP_OK bei Erfolg
 */
esp_err_t ota_ble_get_verify_result(ota_verify_result_t *out);

/**
 * Bricht ein laufendes OTA-Update ab.
 *
 * Bereinigt Ressourcen und kehrt in den IDLE-State zurueck.
 * Eine bereits begonnene Flash-Partition wird verworfen.
 *
 * @return ESP_OK bei Erfolg
 */
esp_err_t ota_abort(void);

/**
 * Liefert den aktuellen OTA-State.
 *
 * @return Aktueller State
 */
ota_state_t ota_get_state(void);

/**
 * Liefert den letzten Fehlercode.
 *
 * @return Fehlercode (OTA_ERR_NONE wenn kein Fehler)
 */
ota_error_t ota_get_error(void);

/**
 * Liefert den aktuellen Fortschritt in Prozent.
 *
 * @return 0..100
 */
uint8_t ota_get_progress(void);

/**
 * Liefert Versions-Informationen.
 *
 * @param info  Ausgabe-Struktur (vom Aufrufer allokiert)
 * @return ESP_OK bei Erfolg
 */
esp_err_t ota_get_version_info(ota_version_info_t *info);

/**
 * Loest einen manuellen Rollback aus.
 *
 * Setzt die Boot-Partition auf die letzte als gueltig markierte
 * Partition zurueck und rebootet. Nur im IDLE-State aufrufbar.
 *
 * @return ESP_OK bei Erfolg
 */
esp_err_t ota_rollback(void);

/**
 * Prueft beim Boot, ob die Firmware validiert werden muss.
 *
 * Wird von ota_init() aufgerufen. Wenn die laufende Partition im
 * PENDING_VERIFY-State ist, wird ein Self-Test durchgefuehrt.
 * Bei Misserfolg -> automatischer Rollback.
 *
 * @return true wenn Firmware gueltig, false bei Rollback
 */
bool ota_check_on_boot(void);

/**
 * Firmware-Self-Test nach OTA-Update.
 *
 * Prueft grundlegende Systemfunktionen, um zu entscheiden ob
 * die neue Firmware als gueltig markiert wird.
 *
 * @return true wenn Self-Test bestanden
 */
bool ota_firmware_self_test(void);

/* ============================================================
 * Secure-Boot API (siehe ota_secure_boot.c)
 * ============================================================ */

/**
 * Verifiziert ein Firmware-Image mit RSA-Signatur.
 *
 * @param partition  OTA-Partition mit dem zu pruefenden Image
 * @param signature  RSA-3072 Signatur (384 Bytes)
 * @param hash       Erwarteter SHA-256-Hash (32 Bytes), oder NULL fuer Berechnung
 * @return ESP_OK wenn Signatur gueltig
 */
esp_err_t ota_secure_verify_signature(const esp_partition_t *partition,
                                       const uint8_t *signature,
                                       const uint8_t *hash);

/**
 * Prueft den Anti-Rollback-Counter.
 *
 * Vergleicht die Firmware-Version mit dem eFuse-Counter.
 * Bei zu alter Version wird das Update abgelehnt.
 *
 * @param new_version  Versionsnummer der neuen Firmware
 * @return ESP_OK wenn Version akzeptiert
 */
esp_err_t ota_secure_check_anti_rollback(uint32_t new_version);

/**
 * Aktualisiert den eFuse-Version-Counter nach erfolgreichem Update.
 *
 * @param version  Neue Firmware-Version
 * @return ESP_OK bei Erfolg
 */
esp_err_t ota_secure_update_version_counter(uint32_t version);

/**
 * Liefert den aktuellen Secure-Boot-Status.
 *
 * @return true wenn Secure Boot aktiviert
 */
bool ota_secure_is_enabled(void);

#endif /* _OTA_UPDATE_H_ */

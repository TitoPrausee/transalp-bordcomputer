# Cloud-Sync: Touren-Backup und Statistiken

> Stand: 16.10.2026
> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986-2000, rein mechanisch, keine ECU
> Plattform: ESP32-S3-WROOM-1 (N8R2: 8MB Flash, 2MB PSRAM), ESP-IDF v5.4, FreeRTOS SMP
> Vorherige Dokumente: 01-13, 14-obd-ii-integration

## 0. Einleitung

Der Bordcomputer protokolliert alle Touren auf einer SD-Karte im CSV- und
GPX-Format (siehe firmware/components/storage/logger.c). Die Daten sind
damit lokal verfuegbar, aber nicht gegen Verlust gesichert: SD-Karten
koennen defekt gehen, das Geraet kann gestohlen werden, und ein Wechsel
auf ein neues Geraet bedeutet manuellen Daten-Transfer per SD-Karten-
Lesegeraet.

Cloud-Sync loest diese Probleme: Touren werden automatisch oder auf
Wunsch mit einem Cloud-Dienst synchronisiert. Die Companion-App fungiert
als Vermittler: Sie liest die Daten vom Bordcomputer per BLE, wandelt sie
in ein transportables Format (JSON) um und uebertraegt sie an einen
Cloud-Dienst.

Dieses Dokument evaluiert drei Cloud-Sync-Strategien:

1. **GitHub Gist API** als kostenloses Free-Tier-Backend
2. **Self-hosted Backend** (Node.js + SQLite) als datenschutzfreundliche Alternative
3. **Hybrid-Modell**: Gist als primaerer Speicher, Self-hosted als optionaler Server

Alle drei werden nach Architektur, Datenfluss, API-Design, Datenschutz
und Aufwand bewertet.

---

## 1. Grundlagen

### 1.1 Bestehender Datenfluss (Firmware-Logger)

Der Logger (logger.c) sammelt Telemetriedaten in einem Ringbuffer und
schreibt sie batchweise auf die SD-Karte:

```
Sensor-Task (10Hz) -> sensor_data_queue
GPS-Task    (10Hz) -> gps_data_queue
                          |
                          v
Logger-Task (Core 0, Prio 3)
  - Nimmt GPS + Sensor-Daten
  - Berechnet haversine_m() fuer Mindestdistanz (>10m)
  - Push in Ringbuffer (LOGGER_RINGBUF_SIZE)
  - Batch-Flush bei 64 Eintraegen oder Timeout
  - CSV: /sdcard/logs/BOOTnnn_HHMMSS.csv
  - GPX: /sdcard/tracks/BOOTnnn_HHMMSS.gpx
```

CSV-Format (bestehend):
```
timestamp_ms,lat,lon,speed_kmh,course_deg,altitude_m,rpm,temp_c,roll_deg,pitch_deg,fuel_l
```

GPX-Export (bestehend): Standard GPX 1.1 XML mit trkpt-Elementen.

### 1.2 Bestehende BLE-Uebertragung

Der Telemetry-Service (0x5000) bietet bereits Track-Export-Characteristics:

| Characteristic | UUID | Properties | Beschreibung |
|---|---|---|---|
| CHR_TRACK_EXPORT | 0x5003 | notify | GPX-Binaer-Transfer |
| CHR_TRACK_EXPORT_CTRL | 0x5004 | write | Start/Cancel/Status |

Der Bordcomputer kann also bereits GPX-Tracks per BLE an die Companion-App
uebertragen. Die Cloud-Sync-Erweiterung setzt an diesem Punkt an: Die App
empfaengt den Track und schickt ihn in die Cloud.

### 1.3 GitHub Gist API

Gists sind vereinfachte Repositorys, die einzelne Dateien speichern.
Die Gist REST API bietet:

- `POST /gists` — Gist erstellen (public oder secret)
- `PATCH /gists/{id}` — Gist aktualisieren
- `GET /gists/{id}` — Gist lesen
- `DELETE /gists/{id}` — Gist loeschen
- `GET /gists` — Liste der eigenen Gists

Authentifizierung: Personal Access Token (PAT) mit `gist`-Scope.
Authorization-Header: `Authorization: Bearer <TOKEN>`

Rate-Limits:
- Authentifiziert: 5.000 Requests/Stunde
- Unauthentifiziert: 60 Requests/Stunde (nur Lesen von public Gists)
- Dateigroesse: max 1 MB pro Datei, bis 300 Dateien pro Gist

Fuer eine typische Tour (100 km, 10 Hz Logging, ~10.000 Punkte) ist die
CSV-Datei ca. 1-2 MB. Das ueberschreitet das 1 MB Gist-Limit. Daher muss
die Tour entweder komprimiert oder in mehrere Gist-Dateien aufgeteilt
werden.

### 1.4 DSGVO und Datenschutz

Tourendaten enthalten GPS-Tracks und damit Bewegungsprofile. Das ist
personenbezogen im Sinne der DSGVO. Die Bewertung haengt vom
Speicherort ab:

- **GitHub Gist (public):** Bewegungsprofil oeffentlich sichtbar. Nicht
  DSGVO-konform ohne ausdrueckliche Einwilligung. Nur fuer anonymisierte
  oder pseudonymisierte Daten akzeptabel.
- **GitHub Gist (secret):** Nur mit Token lesbar. Besser, aber GitHub
  ist ein US-Unternehmen. Datenuebermittlung in Drittland (USA).
  Datenschutzerklkung muss Transfer offenlegen.
- **Self-hosted Backend:** Daten bleiben auf eigenem Server. DSGVO-
  konform, wenn der Server in der EU steht. Keine Uebermittlung an
  Drittlaender.

Das Projekt steht unter PolyForm Noncommercial 1.0.0. Die Cloud-Sync-
Implementierung darf keine proprietaeren Cloud-Dienste erfordern, die
mit der Noncommercial-Lizenz kollidieren.

---

## 2. Architektur

### 2.1 Uebersicht

```
+----------------+     BLE      +------------------+    HTTP    +------------------+
|                |              |                  |           |                  |
|  Bordcomputer  |  GPX/CSV     |  Companion-App   |  JSON     |  Cloud-Backend   |
|  (ESP32-S3)    |  Transfer    |  (Smartphone)    |  Upload   |  (Gist / Self)   |
|                |              |                  |           |                  |
|  SD-Karte:     |  CHR_TRACK_  |  Sync-Manager    |  REST API |  Touren-Speicher |
|  /sdcard/logs/ |  EXPORT      |  Queue + Retry   |           |  Statistik       |
|  /sdcard/trks/ |  (0x5003/04) |  Offline-Cache   |           |  Sharing         |
|                |              |                  |           |                  |
+----------------+              +------------------+           +------------------+
                                        |
                                        | Local Cache
                                        v
                                 +--------------+
                                 | App Storage  |
                                 | (SQLite/FS)  |
                                 | Pending Sync |
                                 +--------------+
```

### 2.2 Sync-Strategie: Offline-First

Die Companion-App speichert empfangene Touren zunaechst lokal (SQLite
oder Dateisystem). Die Cloud-Synchronisierung laeuft asynchron:

1. **Empfang:** Bordcomputer uebertraegt GPX per BLE -> App speichert
   in lokale SQLite-Tabelle `tours`.
2. **Queueing:** Tour wird als `sync_status=pending` markiert.
3. **Sync-Trigger:** Manuel (Button "Sync") oder automatisch (WLAN
   verfuegbar + Akku > 20% + Zeit seit letzter Sync > 1h).
4. **Upload:** App sendet JSON- oder CSV-Payload an Cloud-Backend.
5. **Bestaetigung:** Backend bestaetigt Empfang -> `sync_status=synced`.
6. **Retry:** Bei Fehlers (kein Netz, Rate-Limit, 5xx) -> exponentieller
   Backoff (1m, 5m, 30m, max 24h). Nach 3 Fehlversuchen: `sync_status=failed`.
7. **Cleanup:** Synced-Touren bleiben lokal (fuer Offline-Zugriff).

### 2.3 Datenformat: Tour-JSON

Fuer den Cloud-Transfer wird ein kompaktes JSON-Format verwendet, das
sowohl Metadaten als auch Track-Punkte enthaelt:

```json
{
  "version": 1,
  "tour_id": "BOOT001_143022",
  "device": "BC-Transalp",
  "started_at": "2026-10-16T14:30:22Z",
  "ended_at": "2026-10-16T17:45:01Z",
  "stats": {
    "duration_s": 11779,
    "distance_km": 143.2,
    "max_speed_kmh": 128,
    "avg_speed_kmh": 43.7,
    "max_rpm": 7250,
    "avg_rpm": 3850,
    "fuel_used_l": 8.4,
    "max_temp_c": 98,
    "max_altitude_m": 2104,
    "total_climb_m": 1850
  },
  "track": [
    {"t": 0, "lat": 50.928412, "lon": 11.586914, "alt": 180,
     "spd": 0, "rpm": 0, "tmp": 22, "fuel": 17.0},
    {"t": 10000, "lat": 50.928450, "lon": 11.587010, "alt": 182,
     "spd": 12, "rpm": 2400, "tmp": 45, "fuel": 17.0}
  ]
}
```

Feldabkuerzungen im Track-Array minimieren die JSON-Groesse:
`t`=Timestamp ms (relativ), `spd`=km/h, `tmp`=C, `alt`=m.

### 2.4 Komprimierung

Eine 100-km-Tour mit 10.000 Punkten erzeugt ca. 1.5 MB unkomprimiertes
JSON. Strategien:

1. **JSON-Reduktion:** Nur Punkte mit Abstand >50m speichern (wie
   der Logger bereits mit LOGGER_MIN_DIST_M=10 macht, aber groeber).
   Reduziert auf ca. 2.000 Punkte -> 300 KB.
2. **Gzip-Komprimierung:** JSON mit gzip vor dem Upload. 300 KB ->
   ca. 60 KB. Gist API unterstuetzt keine gzip im Request, aber die
   App kann die Datei als Base64 in einem Gist-File speichern.
3. **Gist-Aufteilung:** Bei Touren >1 MB wird die Tour in mehrere
   Gist-Dateien aufgeteilt (tour_001_part1.json, tour_001_part2.json).

---

## 3. Datenfluss

### 3.1 Tour-Transfer: Bordcomputer -> App

```
Bordcomputer                    Companion-App
    |                                |
    |  User: "Tour beenden" in App   |
    |  -> Write CHR_TRACK_EXPORT_CTRL|
    |     (0x5004) = START           |
    |                                |
    |  Logger exportiert Ringbuffer  |
    |  als GPX in den BLE-Notify-    |
    |  Stream auf CHR_TRACK_EXPORT   |
    |  (0x5003)                      |
    |                                |
    |  Chunk 1 (244 bytes) --------> |
    |  Chunk 2 (244 bytes) --------> |
    |  ...                           |
    |  Chunk N (last, <244) -------> |
    |  End marker (0x00) -----------> |
    |                                |
    |  App: reassembles chunks,      |
    |  parses GPX, stores in SQLite  |
    |  with sync_status=pending       |
    |                                |
    |  App: Write CHR_TRACK_EXPORT_  |
    |  CTRL = ACK                    |
    |<-------------------------------|
    |                                |
    |  Bordcomputer: Tour als        |
    |  synced markieren (lokal)      |
```

### 3.2 Cloud-Sync: App -> Backend

```
Companion-App                   Cloud-Backend
    |                               |
    |  Sync-Trigger: WLAN + Akku    |
    |  + pending Tours vorhanden    |
    |                               |
    |  1. Tour aus SQLite laden     |
    |  2. JSON serialisieren        |
    |  3. Gzip komprimieren          |
    |  4. Base64 kodieren            |
    |                               |
    |  POST /gists                   |
    |  Authorization: Bearer <PAT>  |
    |  Body: {                       |
    |    "description": "Tour...",   |
    |    "public": false,             |
    |    "files": {                   |
    |      "tour.json": {             |
    |        "content": "<base64>"   |
    |      }                          |
    |    }                            |
    |  }                              |
    |------------------------------> |
    |                               |
    |  201 Created                   |
    |  { "id": "abc123", "url": ... }|
    |<------------------------------ |
    |                               |
    |  App: SQLite update            |
    |  sync_status=synced            |
    |  gist_id=abc123                |
```

### 3.3 Self-hosted Backend (Alternative)

```
Companion-App                   Self-hosted Server
    |                               |
    |  POST /api/tours               |
    |  Content-Type: application/json|
    |  Authorization: Bearer <JWT>   |
    |  Body: Tour-JSON (unkomprimiert)|
    |------------------------------> |
    |                               |
    |  Server:                       |
    |  - JWT validieren              |
    |  - Tour in SQLite speichern    |
    |  - Statistiken aktualisieren   |
    |  - 200 OK + tour_id            |
    |<------------------------------ |
```

### 3.4 Statistik-Aggregation

Der Cloud-Backend (egal ob Gist oder Self-hosted) aggregiert Statistiken:

```
Tour 1: 143 km, 3h 16m, max 128 km/h
Tour 2: 87 km, 2h 05m, max 115 km/h
Tour 3: 210 km, 4h 30m, max 135 km/h
------------------------------------
Gesamt: 440 km, 9h 51m, max 135 km/h
Avg: 147 km/Tour, 3h 17m/Tour
```

Bei Gist: Die App liest alle eigenen Gists, parst die Tour-JSON und
berechnet die Statistiken lokal. Keine Server-seitige Aggregation.

Bei Self-hosted: Der Server kann `/api/stats` Endpunkte anbieten, die
ueber alle Touren aggregieren.

---

## 4. API-Design

### 4.1 GitHub Gist API (Free-Tier)

Endpunkte (alle unter `https://api.github.com`):

| Methode | Pfad | Beschreibung |
|---|---|---|
| POST | /gists | Tour erstellen |
| PATCH | /gists/{id} | Tour aktualisieren |
| GET | /gists/{id} | Tour laden |
| GET | /gists | Alle eigenen Touren listen |
| DELETE | /gists/{id} | Tour loeschen |

Headers:
```
Authorization: Bearer <PAT>
Accept: application/vnd.github+json
X-GitHub-Api-Version: 2022-11-28
Content-Type: application/json
```

Create-Tour Request:
```json
{
  "description": "Tour 2026-10-16 143km Transalp",
  "public": false,
  "files": {
    "tour_BOOT001_143022.json": {
      "content": "<gzip+base64 encoded JSON>"
    }
  }
}
```

Response (201):
```json
{
  "id": "abc123def456",
  "url": "https://api.github.com/gists/abc123def456",
  "html_url": "https://gist.github.com/user/abc123def456",
  "created_at": "2026-10-16T18:00:00Z",
  "files": {
    "tour_BOOT001_143022.json": {
      "raw_url": "https://gist.github.com/.../raw/tour.json",
      "size": 61440,
      "truncated": false
    }
  }
}
```

### 4.2 Self-hosted Backend API

REST-Endpunkte:

| Methode | Pfad | Beschreibung |
|---|---|---|
| POST | /api/auth/login | JWT-Token holen |
| GET | /api/tours | Touren listen (paginated) |
| POST | /api/tours | Tour erstellen |
| GET | /api/tours/{id} | Einzelne Tour laden |
| PUT | /api/tours/{id} | Tour aktualisieren |
| DELETE | /api/tours/{id} | Tour loeschen |
| GET | /api/stats | Aggregierte Statistiken |

Authentifizierung: JWT-Bearer-Token. Login mit Benutzername/Passwort
oder Geraete-ID.

Tour erstellen:
```
POST /api/tours
Authorization: Bearer <JWT>
Content-Type: application/json

{
  "tour_id": "BOOT001_143022",
  "started_at": "2026-10-16T14:30:22Z",
  "ended_at": "2026-10-16T17:45:01Z",
  "stats": {...},
  "track": [...]
}

Response 201:
{
  "id": 42,
  "tour_id": "BOOT001_143022",
  "created_at": "2026-10-16T18:00:00Z"
}
```

### 4.3 Companion-App: Sync-Manager

Neue Datei: `companion-app/src/services/SyncManager.ts`

```typescript
interface TourRecord {
  tour_id: string;
  started_at: string;
  ended_at: string;
  stats: TourStats;
  track: TrackPoint[];
  sync_status: 'pending' | 'syncing' | 'synced' | 'failed';
  sync_error?: string;
  gist_id?: string;
  synced_at?: string;
}

interface SyncConfig {
  backend: 'gist' | 'self-hosted' | 'none';
  gist_token?: string;
  server_url?: string;
  server_token?: string;
  auto_sync: boolean;
  wifi_only: boolean;
  min_battery_pct: number;
}

class SyncManager {
  private queue: TourRecord[] = [];
  private isSyncing = false;

  async enqueueTour(tour: TourRecord): Promise<void>;
  async syncPending(): Promise<SyncResult>;
  private async uploadToGist(tour: TourRecord): Promise<string>;
  private async uploadToServer(tour: TourRecord): Promise<string>;
  async fetchAllTours(): Promise<TourRecord[]>;
  async fetchStats(): Promise<AggregatedStats>;
}
```

### 4.4 Companion-App: ToursScreen Erweiterung

Der bestehende `ToursScreen.tsx` wird um Sync-Status-Anzeige und
Sync-Button erweitert:

```
+-----------------------------------+
|  Touren                           |
|                                   |
|  [Sync: 3 pending, 12 synced]    |
|  [Jetzt synchronisieren]          |
|                                   |
|  Tour 143 km  16.10.  [Synced]   |
|  Tour 87 km   14.10.  [Synced]   |
|  Tour 210 km  12.10.  [Pending] |
|  Tour 52 km   10.10.  [Failed]  |
|                                   |
|  Statistik:                       |
|  Gesamt: 440 km, 9h 51m          |
|  Diese Saison: 1.240 km          |
+-----------------------------------+
```

### 4.5 Firmware-Aenderungen

Die Firmware benoetigt minimale Aenderungen:

1. **CHR_TRACK_EXPORT_CTRL (0x5004):** Neues Kommando `0x02 = EXPORT_JSON`
   neben dem bestehenden `0x01 = EXPORT_GPX`. Der Logger kann JSON direkt
   serialisieren, ohne GPX-Ueberweg.

2. **Tour-Metadaten:** `tour_context_t` in `storage.h` erweitern um
   `distance_km`, `max_speed_kmh`, `max_rpm`, `max_temp_c`, `max_altitude_m`.
   Diese Werte werden waehrend der Tour akkumuliert und beim Tour-Stop
   mitgesendet.

```c
/* storage.h: Erweiterung tour_context_t */
typedef struct {
    tour_state_t state;
    uint32_t start_time_ms;
    uint32_t point_count;
    uint16_t segment_count;
    bool has_last_point;
    float last_lat;
    float last_lon;
    char csv_path[SD_MAX_PATH_LEN];
    char gpx_path[SD_MAX_PATH_LEN];

    /* Neu: Statistik-Aggregation */
    float distance_km;
    float max_speed_kmh;
    uint16_t max_rpm;
    float max_temp_c;
    float max_altitude_m;
    float max_climb_m;
} tour_context_t;
```

---

## 5. Datenschutz: DSGVO und PolyForm Noncommercial

### 5.1 GitHub Gist (Free-Tier)

| Aspekt | Bewertung |
|---|---|
| Server-Standort | USA (Drittland) |
| Verschluesselung | TLS in Transit, ruhende Daten bei GitHub verschluesselt |
| Datenzugriff | GitHub (USA), moegliche Behoerdenanfragen nach US-Recht |
| Loeschung | DELETE /gists/{id} entfernt sofort, aber GitHub-Backups |
| Oeffentlichkeit | `public: false` (secret Gist) = nur mit Token lesbar |
| DSGVO-Konformitaet | Bedingt. Datenuebermittlung in Drittland muss in Datenschutzerklaerung offenlegt werden. Art. 44-49 DSGVO. |
| Noncommercial-Lizenz | GitHub kostenlos = kein kommerzieller Vertrag. Kompatibel. |

**Empfehlung:** Gist mit `public: false` (secret). Datenschutzerklaerung
muss auf Datenuebermittlung in USA hinweisen. Opt-in fuer Cloud-Sync
(nicht default).

### 5.2 Self-hosted Backend (Node.js + SQLite)

| Aspekt | Bewertung |
|---|---|
| Server-Standort | Frei waehlbar (EU-Empfehlung) |
| Verschluesselung | TLS konfigurierbar, SQLite unverschluesselt (oder SQLCipher) |
| Datenzugriff | Nur Server-Betreiber |
| Loeschung | DELETE sofort, keine Backups (sofern nicht konfiguriert) |
| Oeffentlichkeit | Nur mit Authentifizierung zugaenglich |
| DSGVO-Konformitaet | Hoch, wenn Server in EU steht. Keine Uebermittlung. |
| Noncommercial-Lizenz | Selbst betrieben = keine Kosten. Kompatibel. |

**Empfehlung:** Self-hosted als empfohlene Option fuer datenschutz-
bewusste Nutzer. Node.js + SQLite ist leichtgewichtig und kann auf
einem kleinen Single-Board-Computer oder einem kleinen VPS laufen.

### 5.3 Offline-First als Datenschutz-Feature

Der Offline-First-Ansatz ist selbst ein Datenschutz-Feature: Alle Daten
bleiben lokal, bis der Nutzer aktiv synchronisiert. Es gibt kein
background-tracking, keine automatische Datenuebermittlung. Der Nutzer
hat volle Kontrolle darueber, welche Touren in die Cloud wandern und
welche lokal bleiben.

Dies sollte als Feature kommuniziert werden, nicht als Einschraenkung.

---

## 6. Risiken

### 6.1 Technische Risiken

| Risiko | Wahrscheinlichkeit | Auswirkung | Mitigation |
|---|---|---|---|
| Gist 1 MB Limit bei grossen Touren | hoch | mittel | Aufteilung in mehrere Files, gzip+base64 |
| PAT abgelaufen oder revoked | mittel | hoch | App erkennt 401, zeigt Hinweis zur Token-Erneuerung |
| BLE-Transfer unterbrochen waehrend Tour-Export | mittel | mittel | Chunk-Reassembly mit Sequence-Nummern, Retry ab letztem ACK |
| SD-Karte voll, kein Platz fuer neue Tour | niedrig | hoch | App warnt bei <10% freiem Speicher, alte Touren archivierbar |
| Self-hosted Server offline | mittel | mittel | Offline-Queue, Retry bei naechster Gelegenheit |
| JSON-Parsing-Fehler in Tour-Daten | niedrig | hoch | Schema-Validierung vor Speicherung, Reject mit Fehlermeldung |

### 6.2 Datenschutz-Risiken

| Risiko | Auswirkung | Mitigation |
|---|---|---|
| Public Gist泄露 Bewegungsprofil | hoch | Default: secret Gist. Public nur mit ausdruecklicher Bestaetigung. |
| PAT im App-Speicher geleaked | hoch | Token im OS-Keychain/Keystore speichern, nicht in SharedPreferences |
| Tour-Daten an Dritte weitergegeben | mittel | Keine Third-Party-Analytics, keine Teilen-Funktion ohne Bestaetigung |
| Server-Database gehacked | hoch | SQLCipher Verschluesselung, starke Authentifizierung |

### 6.3 Architektur-Risiken

| Risiko | Auswirkung | Mitigation |
|---|---|---|
| Sync-Queue waechst unendlich bei dauerhaft fehlendem Netz | mittel | Max 100 pending Tours, aelteste werden verworfen mit Warnung |
| Gist API Aenderung oder Abschaffung | niedrig | Abstraktion im SyncManager, Backend austauschbar |
| BLE-Bandbreite reicht nicht fuer grosse Touren | mittel | Chunked Transfer, Progress-Anzeige, Resume |

---

## 7. Aufwandsschaetzung

### 7.1 Companion-App Cloud-Sync (Gist)

| Aufgabe | Aufwand |
|---|---|
| SyncManager Service (Queue, Retry, Backoff) | 8h |
| Gist API Client (Create, List, Delete) | 4h |
| JSON-Tour-Format Serializer | 3h |
| gzip + Base64 Encoder | 2h |
| SQLite-Touren-Speicher (Schema, CRUD) | 4h |
| ToursScreen: Sync-Status UI | 4h |
| Settings: Sync-Konfiguration (Token, Backend) | 3h |
| Statistik-Berechnung aus Gist-Touren | 3h |
| Fehlerbehandlung und Retry-Logik | 3h |
| Unit-Tests | 4h |
| **Zwischensumme Gist** | **38h** |

### 7.2 Self-hosted Backend

| Aufgabe | Aufwand |
|---|---|
| Node.js Server Setup (Express, JWT-Auth) | 4h |
| SQLite Schema (tours, users) | 2h |
| REST-API (CRUD + Stats) | 6h |
| JWT-Authentifizierung | 3h |
| Docker-Compose fuer Deployment | 2h |
| SyncManager: Self-hosted Client | 4h |
| TLS-Konfiguration (Let's Encrypt) | 2h |
| Unit-Tests | 3h |
| **Zwischensumme Self-hosted** | **26h** |

### 7.3 Firmware-Aenderungen

| Aufgabe | Aufwand |
|---|---|
| tour_context_t Statistik-Erweiterung | 2h |
| JSON-Export-Modus im Logger | 3h |
| CHR_TRACK_EXPORT_CTRL: JSON-Kommando | 1h |
| Tour-Statistik-Aggregation in logger_task | 2h |
| Unit-Tests | 2h |
| **Zwischensumme Firmware** | **10h** |

### 7.4 Gesamt

| Bereich | Aufwand |
|---|---|
| Companion-App Gist-Sync | 38h |
| Self-hosted Backend (optional) | 26h |
| Firmware-Aenderungen | 10h |
| Doku + Integrationstests | 6h |
| **Gesamt (mit Self-hosted)** | **80h** |
| **Gesamt (nur Gist)** | **54h** |

**Empfehlung:** Phase 1 nur Gist-Sync (54h). Self-hosted Backend als
optionale Phase 2 (zusaetzlich 26h). Der Gist-Ansatz liefert sofortigen
Mehrwert ohne Infrastruktur-Kosten und ist fuer einzelne Nutzer
ausreichend. Das Self-hosted Backend ist fuer Community-Installationen
oder Nutzer mit hoeheren Datenschutz-Anforderungen gedacht.

---

## 8. Implementierungs-Plan

### Phase 1: Gist-Sync (MVP)

1. `companion-app/src/services/SyncManager.ts` implementieren
2. `companion-app/src/services/GistClient.ts` implementieren
3. SQLite-Touren-Tabelle in App anlegen
4. JSON-Tour-Format definieren und Serializer bauen
5. ToursScreen um Sync-Status und Sync-Button erweitern
6. Settings: Token-Eingabe und Backend-Auswahl
7. Firmware: tour_context_t erweitern, JSON-Export-Modus
8. Unit-Tests fuer SyncManager und GistClient

### Phase 2: Self-hosted Backend (optional)

1. Node.js + Express + SQLite Server aufsetzen
2. JWT-Auth implementieren
3. REST-API implementieren
4. Docker-Compose fuer Deployment
5. SyncManager um Self-hosted-Client erweitern
6. TLS mit Let's Encrypt

---

## 9. Referenzen

- GitHub Gist REST API: https://docs.github.com/en/rest/gists/gists
- GitHub Rate Limits: 5.000/h authentifiziert, 60/h unauthentifiziert
- DSGVO Art. 44-49: Datenuebermittlung in Drittl=E4nder
- PolyForm Noncommercial 1.0.0: Lizenztext in /LICENSE
- Firmware: `firmware/components/storage/logger.c` (Logger, CSV/GPX)
- Firmware: `firmware/components/storage/include/storage.h` (tour_context_t)
- Firmware: `firmware/components/ble/gatt_uuids.h` (Track-Export-Characteristics)
- Companion-App: `companion-app/src/screens/ToursScreen.tsx`
- Companion-App: `companion-app/src/screens/SettingsScreen.tsx`
- Doku: `docs/research/13-connectivity-app.md` (BLE-Protokoll)
- Doku: `docs/API-REFERENCE.md` (GATT-Characteristics)

---

*Dieses Dokument evaluiert die Cloud-Sync-Integration fuer den Transalp Bordcomputer.
Empfehlung: Phase 1 Gist-Sync (54h), Phase 2 Self-hosted Backend (26h, optional).
Offline-First als Datenschutz-Feature: keine automatische Uebermittlung, volle Nutzer-Kontrolle.*

# BLE Interface Specification — Transalp Bordcomputer

> **Canonical spec for the companion-app team.**
> Owner: @klaus (CTO) · Prepared for: @felix (TOT-31 companion-app integration)
> Source of truth: this document. Firmware header `firmware/components/ble/gatt_uuids.h`
> and app `companion-app/src/core/ble/gatt-uuids.ts` **must** match this spec; any
> divergence is a bug (see §10 — known divergences).

This is the early BLE-Bridge interface spec requested by task **TOT-29** (item 6)
so Felix can proceed with app integration (TOT-31) before the full architecture
review is closed. It supersedes the older `docs/research/13-connectivity-app.md`
§2 design notes and consolidates `docs/API-REFERENCE.md`.

---

## 1. Connection & Advertising

| Property | Value |
|---|---|
| Device name (advertising) | `Transalp-BC` |
| Advertised service UUID | `6e401000-b5a3-f393-e0a9-e50e24dcca9e` (Navigation 0x1000) |
| Base UUID | `6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e` (16-bit `XXXX` slot) |
| Stack | NimBLE (BLE 5.0, ESP32-S3) |
| Max connections | 2 (companion app + CarPlay bridge) |
| MTU | 247 bytes (app should `requestMTU(247)` after connect) |
| Security | LE Secure Connections, bonding, MITM, IO cap = DisplayYesNo |
| Bond storage | NVS (`ble_store_util_status_rr` rollover) |

**Discovery flow:** scan filtering on `device.name === "Transalp-BC"` → connect →
`discoverAllServicesAndCharacteristics()` → subscribe to desired notify/indicate
characteristics by writing `0x0100` (notify) or `0x0200` (indicate) to each CCCD.

---

## 2. Base UUID Construction

```
6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e
        ^^── 16-bit service/characteristic ID (big-endian in the string)
```

Helper (TypeScript, from `gatt-uuids.ts`):
```ts
function uuid(id16: number): string {
  const hex = id16.toString(16).padStart(4, '0');
  return `6e40${hex}-b5a3-f393-e0a9-e50e24dcca9e`;
}
```

> ⚠️ **Wire order is little-endian.** NimBLE stores `ble_uuid128_t.value` reversed.
**Parsers must read all multi-byte fields little-endian** (`DataView` with
`littleEndian = true`).

---

## 3. Service Catalog

| Service | 16-bit ID | UUID | Side |
|---|---|---|---|
| Navigation | `0x1000` | `6e401000-…` | App-facing |
| Media | `0x2000` | `6e402000-…` | App-facing |
| Phone | `0x3000` | `6e403000-…` | App-facing |
| Weather | `0x4000` | `6e404000-…` | App-facing |
| Telemetry | `0x5000` | `6e405000-…` | App-facing |
| Config | `0x6000` | `6e406000-…` | App-facing |
| Diagnose | `0x7000` | `6e407000-…` | App-facing |
| OTA | `0x8000` | `6e408000-…` | App-facing |
| CarPlay Bridge | `0x9000` | `6e409000-…` | CarPlay-display-facing (see §11) |

Services `0x1000`–`0x8000` are exposed by the Bordcomputer GATT server for the
**companion app**. Service `0x9000` is the CarPlay-display bridge — the app
should **not** read it (see §11 for the collision warning).

---

## 4. Telemetry Service (0x5000) — primary live-data path

### 4.1 `CHR_TLM_LIVE` (0x5001) — notify, 22 bytes

This is the core Sensors → BLE → App data path, pushed at **5 Hz** when CCCD-subscribed.

**Firmware struct** (`firmware/components/ble/ble_callbacks.c:56`, `__attribute__((packed))`):

| Offset | Size | Type | Field | Unit | Notes |
|---|---|---|---|---|---|
| 0 | 1 | u8 | `speed_kmh` | km/h | 0–255 |
| 1 | 2 | u16 LE | `rpm` | U/min | 0–65535 |
| 3 | 1 | i8 | `gear` | gear # | `-1` = neutral, `0` unused, `1`–`6` engaged |
| 4 | 1 | i8 | `coolant_temp_c` | °C | coolant |
| 5 | 1 | i8 | `ambient_temp_c` | °C | ambient |
| 6 | 1 | u8 | `fuel_percent` | % | 0–100 |
| 7 | 1 | u8 | `voltage_tenth` | 0.1 V | battery, e.g. 124 = 12.4 V |
| 8 | 4 | i32 LE | `latitude_e7` | ° × 1e-7 | WGS84 |
| 12 | 4 | i32 LE | `longitude_e7` | ° × 1e-7 | WGS84 |
| 16 | 2 | i16 LE | `altitude_m` | m | |
| 18 | 2 | u16 LE | `heading_tenth` | 0.1° | 0–3599 |
| 20 | 1 | u8 | `satellites` | count | |
| 21 | 1 | u8 | `hdop_tenth` | 0.1 | HDOP |

`_Static_assert(sizeof(telemetry_live_t) == 22)` enforces the size at build time.

App parser: `companion-app/src/core/ble/binary-parsers.ts` → `parseTelemetryLive`.

### 4.2 `CHR_TLM_SNAPSHOT` (0x5002) — read, ~60 bytes

Full sensor snapshot (all fields at once). Read on demand.

### 4.3 `CHR_TRACK_EXPORT` (0x5003) — notify; `CHR_TRACK_EXPORT_CTRL` (0x5004) — write

GPX/binary track export stream. Write `0x01`=start, `0x02`=cancel to 0x5004;
chunks arrive on 0x5003.

---

## 5. Navigation Service (0x1000)

| Char | ID | Props | Payload |
|---|---|---|---|
| Maneuver | 0x1001 | R+N | `u8 type` (ManeuverType) + `u32 distance_m` + `UTF-8 streetName` (max 64 B) |
| Route | 0x1002 | R+N | `u32 totalDistance_m` + `u32 remainingDistance_m` + `u32 eta_unix` + `u32 remainingTime_s` |
| Road | 0x1003 | R+N | `u8 speedLimit_kmh` (0=unknown) + `UTF-8 roadName` (max 64 B) |
| Status | 0x1004 | R+N | `u8` NavStatus enum (0=Inactive…4=Offline) |
| Cmd | 0x1005 | W+WNR | Bordcomputer → App commands |

**ManeuverType** (byte 0 of 0x1001): 0=Straight, 1=SlightLeft, 2=Left, 3=SharpLeft,
4=SlightRight, 5=Right, 6=SharpRight, 7=UTurn, 8=Arrive, 9=Fork.

**NavStatus**: 0=Inactive, 1=Active, 2=Rerouting, 3=Arriving, 4=Offline.

---

## 6. Media / Phone / Weather Services

### Media (0x2000)
- 0x2001 NowPlaying (R+N): artist/title/duration/pos/state
- 0x2002 Control (W+WNR): play/pause/next/prev/seek
- 0x2003 Volume (R+N+W): `u8` 0–100

### Phone (0x3000)
- 0x3001 IncomingCall (N): caller-id, call-type
- 0x3002 CallStatus (R+N): `u8`
- 0x3003 Control (W): accept/reject/hangup
- 0x3004 Notification (N): app-name, title, body

### Weather (0x4000)
- 0x4001 Current (R+N, 8 B): `i16 temp_tenth` + `u8 humidity` + `u8 condition`(0=clear…7=fog) + `u8 windSpeed_kmh` + `i16 feelsLike_tenth` + `u8 rainProbability`
- 0x4002 Forecast (R+N): 6 h forecast array
- 0x4003 Alert (N): alert type, severity, time

---

## 7. Config Service (0x6000)

| Char | ID | Props | Payload |
|---|---|---|---|
| Read | 0x6001 | R+N | JSON chunk |
| Write | 0x6002 | W | JSON chunk |
| Export | 0x6003 | N | binary config export |
| Import | 0x6004 | W | binary config import |
| Bond Mgmt | 0x6005 | W | `0x00` = delete all bonds |

---

## 8. Diagnose Service (0x7000)

### 8.1 `CHR_DTC_LIST` (0x7001) — read+notify+indicate, 6 bytes/entry

| Offset | Size | Type | Field |
|---|---|---|---|
| 0 | 2 | u16 LE | `code` |
| 2 | 1 | u8 | `severity` (0=info, 1=warning, 2=critical) |
| 3 | 4 | u32 LE | `timestamp` (Unix seconds) |

`_Static_assert(sizeof(dtc_binary_t) == 6)` (firmware). Multiple entries concatenate.

### 8.2 `CHR_SENSOR_STATUS` (0x7002) — read+notify, 5 bytes

| Offset | Field | Values |
|---|---|---|
| 0 | gps | 0=ok, 1=no_fix, 2=error |
| 1 | rpm | 0=ok, 1=no_signal, 2=error |
| 2 | temp | 0=ok, 1=disconnected, 2=error |
| 3 | fuel | 0=ok, 1=error |
| 4 | sd_card | 0=ok, 1=not_present, 2=full, 3=error |

**5 distinct bytes, NOT a bitfield.** (`sensor_status_t`, `ble_callbacks.c:86`.)

### 8.3 `CHR_SELF_TEST` (0x7003) — write+notify

Write `0x00`=Quick / `0x01`=Full to trigger; result notified as
`u8 result`(0=pass,1=warn,2=fail) + one detail byte per sensor.

### 8.4 Log export — 0x7004 (N), 0x7005 (W: start/cancel)

> ⚠️ **App currently declares `0x7006 systemStatus` and `0x7007 taskInfo`** — these
are **not implemented in firmware** (see §10). Reads will GATT-fail. System-status
fields ride the Telemetry Snapshot (0x5002) instead.

---

## 9. OTA Service (0x8000)

### 9.1 `CHR_OTA_CONTROL` (0x8001) — write+notify

**Write** command byte + optional payload:
| Value | Command | Payload after cmd byte |
|---|---|---|
| `0x00` | Start | `u32 LE total_size` (expected firmware image size in bytes) |
| `0x01` | Cancel | — |
| `0x02` | Verify | — (status checkpoint; firmware notifies current OTA state) |
| `0x03` | Commit | `u8[32] expected_sha256` (optional; if omitted, hash is not pre-verified) |

The `Start` payload carries the expected image size so the OTA component can open
the update partition and compute progress (`ota_update_from_ble(total_size)`). The
`Commit` payload may carry the expected SHA-256 so the OTA component compares it
before setting the boot partition (`ota_ble_commit(expected_hash)`).

**Notify** status (3 bytes): `u8 state`(0=idle,1=uploading,2=verifying,3=done,
4=failed) + `u8 progress`(0–100) + `u8 errorCode`. (The trailing `version` field
from the original design is omitted for now; readers must tolerate a 3-byte notify.)

> ⚠️ **App `OtaCommand` enum is missing `Commit=3`** — see §10. Verify-then-commit
is a two-step flow; the app should send `0x02` (Verify) to request a status notify,
then `0x03` (Commit) to trigger the terminal verify + set-boot + reboot. The OTA
component currently fuses verify and commit inside `ota_ble_commit()` (it reboots
on success), so `0x02` is a non-terminal status checkpoint and `0x03` is the
terminal step. A future OTA-API split (`ota_ble_verify()` separate from
`ota_ble_commit()`) would enable a true two-step with the hash notified on
`0x8003` before reboot — see TOT-33 comment for the open interface question
raised with @wolfgang.

### 9.2 `CHR_OTA_DATA` (0x8002) — write_nr

Firmware image chunks. Per-chunk `ota_chunk_t` carries a CRC16-CCITT verified by
`ota_ble_write_chunk()`.

### 9.3 `CHR_OTA_VERIFY` (0x8003) — notify

`u8 status` + `u8[32] sha256` + `u32 size` + `u32 version` (41 bytes).

**Verify-result status byte** (offset 0): `0`=unknown, `1`=verifying,
`2`=ok (hash verified), `3`=hash_mismatch, `4`=error. Firmware now emits on
0x8003 on a failed Commit (TOT-35, `notify_ota_verify()` in `ble_callbacks.c`
via `ota_ble_get_verify_result()`). On a successful Commit the device reboots
inside `ota_ble_commit()` before the notify can be delivered; a true two-step
verify-notify-commit before reboot remains planned for TOT-33.

> ⚠️ **App does not subscribe to 0x8003** and instead reads a non-existent
`0x8004 otaStatus` (see §10). The app must subscribe to **0x8003** for verify
results; OTA status during upload rides 0x8001 notify.

---

## 10. Known Divergences (BUGS — must be reconciled for TOT-31)

These are integration gaps found during the TOT-29 architecture review. The app
(`companion-app/src/core/ble/gatt-uuids.ts`) declares characteristics the firmware
does **not** implement. Until resolved, the app will get GATT errors on these.

| 16-bit ID | App name | Firmware status | Action |
|---|---|---|---|
| `0x7006` | `systemStatus` | ❌ not in firmware | Remove from app, OR add to firmware. System status is in TLM_SNAPSHOT 0x5002. |
| `0x7007` | `taskInfo` | ❌ not in firmware | Remove from app, OR add to firmware (diagnostics-only). |
| `0x8004` | `otaStatus` | ❌ not in firmware | App must use `0x8001` (control notify) for status + `0x8003` (verify) for hash. |
| `0x8003` | `otaVerify` (declared, unused) | ✅ in firmware | App must **subscribe** to 0x8003 for verify results. Firmware now emits on 0x8003 on failed Commit (TOT-35); success-path notify before reboot is TOT-33. |

**OTA command enum:** app defines `Start=0, Cancel=1, Verify=2` — **missing `Commit=3`**.
Add it.

---

## 11. CarPlay Bridge Service (0x9000) — DO NOT USE FROM APP

Service `0x9000` is the **CarPlay-display bridge**, not a companion-app interface.
The Bordcomputer acts as GATT **server** here, pushing 12-byte telemetry
(`carplay_telemetry_t`) to an external EKIY/Jiuyin CarPlay display, and as GATT
**client** to the display's `0x9100` service. See `docs/CARPLAY-INTEGRATION.md`.

| Char | ID | Firmware side | Payload (firmware) |
|---|---|---|---|
| TLM Live | 0x9001 | N (12 B) | `carplay_telemetry_t`: speed, rpm, gear, coolant, fuel, voltage, ambient, fuel_low |
| TLM Snapshot | 0x9002 | R | full snapshot |
| Status | 0x9003 | R+N | connection status |
| Pairing | 0x9004 | W | 6-byte addr + 1 type |
| Bond Mgmt | 0x9005 | W | delete bonds |

> 🚨 **CRITICAL COLLISION:** the app currently declares `carplayStatus: 0x9001`
(194-byte media-metadata struct) and `carplayControl: 0x9002` (write). The firmware
emits a **12-byte telemetry** packet on 0x9001 and serves a **read snapshot** on
0x9002. The app's `parseCarPlayStatus` will misparse the firmware payload.
**Resolution:** the companion app must NOT touch service 0x9000. CarPlay media
metadata for the app (if ever needed) must use a **new, separate** service/UUID,
not 0x9000/0x9001. This is a hard incompatibility blocking TOT-31 until the app
removes its 0x9000 declarations.

---

## 12. Test Vectors — `CHR_TLM_LIVE` (0x5001)

Reference packing (little-endian, 22 bytes) for app-parser validation:

| Scenario | speed | rpm | gear | coolant | ambient | fuel | voltage | lat(E7) | lon(E7) | alt | heading | sats | hdop |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Typical ride | 95 | 4500 | 4 | 88 | 22 | 85 | 124 | 509353900 | 115872400 | 245 | 2700 | 9 | 12 |
| Idle | 0 | 1200 | -1 | 70 | 18 | 150 | 122 | … | … | … | 0 | 4 | 25 |
| Redline | 180 | 8000 | 6 | 95 | 30 | 50 | 130 | … | … | … | 900 | 9 | 10 |

> ⚠️ The Python simulator (`tools/simulator/mock_ble.py`) currently packs a
**different** telemetry format (`rpm, speed, temp, fuel, gear, roll, pitch, flags`).
It is out of sync with this spec. Do **not** use the simulator as a BLE contract
reference until TOT-29 follow-up fixes it (see `docs/INTEGRATION-TEST-REPORT.md`).

---

## 13. Change Control

Any change to a UUID, payload layout, or enum value must:
1. Update this spec (`docs/BLE-INTERFACE-SPEC.md`).
2. Update `firmware/components/ble/gatt_uuids.h` (+ `ble_callbacks.c` structs).
3. Update `companion-app/src/core/ble/gatt-uuids.ts` + `binary-parsers.ts`.
4. Update `tools/simulator/mock_ble.py` to match.
5. Add/adjust a test vector above.

The firmware `_Static_assert` on packed struct sizes is the build-time guard; the
app should add an equivalent runtime byte-length check in each parser.

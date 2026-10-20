# Architecture Review — TRANSALP Bordcomputer

**Task:** TOT-29 (System Integration & Architecture Review), item 4 & 5
**Parent:** TOT-16
**Reviewer:** @klaus (CTO)
**Date:** 2026-10-20
**Firmware version:** `0.4.0-integration`
**Scope:** production readiness — error handling, edge cases, race conditions,
technical debt, and gaps across the firmware + companion-app + simulator.

Companion documents:
- `docs/INTEGRATION-TEST-REPORT.md` — test execution + per-data-path verdicts
- `docs/BLE-INTERFACE-SPEC.md` — canonical BLE contract for the app team

---

## 1. System Overview

ESP32-S3-WROOM-1 (16 MB flash, 8 MB octal PSRAM) running ESP-IDF 5.3.6. Eleven
local (vendored) components under `firmware/components/`, no managed components /
no `idf_component.yml`. `app_main()` (`main.c:536`) initialises 11 subsystems in
order and spawns 9 FreeRTOS tasks split across cores (Core 0 = UI/Comms, Core 1 =
real-time sensor/GPS). All tasks subscribe to the Task Watchdog Timer.

Two BLE roles coexist on one NimBLE stack (max 2 connections):
- **Companion-app GATT server** — services 0x1000–0x8000.
- **CarPlay-display bridge** — server 0x9000 + client to display's 0x9100.

---

## 2. Strengths

| Area | Assessment |
|---|---|
| Partition layout | Correct dual-OTA + factory + otadata + 7 MB FATFS; 16 MB fully utilised. |
| OTA rollback | `CONFIG_APP_ROLLBACK_ENABLE=y`; boot self-test marks valid/invalid; manual rollback API. |
| OTA partition APIs | Complete `esp_ota_*` usage; SHA-256 per-partition; CRC16 per chunk. |
| Task model | Clear core affinity split; WDT on all 9 tasks; documented in `main.c:25-26`. |
| Init robustness | Each `init_step_*` registers a DTC on failure and continues; only NVS/power fatal. |
| Static guards | `_Static_assert` on packed struct sizes (telemetry 22 B, DTC 6 B, sensor-status 5 B). |
| Host testability | 10 Unity suites with mock layer; dual-mode CMake (host GCC / IDF test component). |
| Diagnostics | OBD-II-style DTC system (~55 codes), freeze-frame, NVS history, serial console. |
| Security docs | `docs/SECURITY-ANALYSIS.md` STRIDE model exists. |

---

## 3. Critical Gaps (release blockers)

### C1. BLE OTA path is non-functional (TODO)

`firmware/components/ble/ble_callbacks.c:695` — `process_ota_data()`:
```c
static void process_ota_data(const uint8_t *data, uint16_t len)
{
    ESP_LOGD(TAG, "OTA data: %d bytes", len);
    /* TODO: esp_ota_write() with proper partition handling */
}
```
The OTA component exposes `ota_ble_write_chunk()` but the BLE layer never calls
it. OTA chunks from the companion app to `CHR_OTA_DATA` (0x8002) are silently
dropped. `process_ota_control()` (`ble_callbacks.c:673`) likewise only toggles
event-bus bit-flags and never drives the OTA state machine.

**Impact:** OTA-over-BLE does not work. **Fix:** wire both callbacks to the OTA
component API. (Follow-up FU-5/FU-6.)

### C2. OTA signature verification disabled

`firmware/components/ota/ota_secure_boot.c:63` — `ota_public_key_der` is an
all-zero placeholder. `ota_secure_verify_signature()` detects it and returns
`ESP_ERR_NOT_SUPPORTED`, falling back to **hash-only** verification. Secure Boot
V2 and Flash Encryption are commented out in `sdkconfig.defaults:39,42`.
Anti-rollback uses a RAM variable (`s_efuse_version_counter`), not eFuse
(commented-out `esp_efuse_write_field_bit` calls).

**Impact:** a crafted firmware image with a correct SHA-256 but no valid
signature would be accepted in the current build. **Release blocker.**
(Follow-up FU-7/FU-8.)

### C3. Companion-app / firmware BLE contract divergence

The app (`companion-app/src/core/ble/gatt-uuids.ts`) declares 4 characteristics
the firmware does not implement, and collides on the CarPlay service:

| App declares | Firmware | Problem |
|---|---|---|
| `0x7006 systemStatus` | ❌ absent | app read → GATT error |
| `0x7007 taskInfo` | ❌ absent | app read → GATT error |
| `0x8004 otaStatus` | ❌ absent | app subscribes to nothing; OTA status lost |
| `0x9001 carplayStatus` (194 B media struct) | ✅ `CHR_CP_TLM_LIVE` (12 B telemetry) | **hard collision** — parser misparses |
| `0x9002 carplayControl` (write) | ✅ `CHR_CP_TLM_SNAPSHOT` (read) | direction mismatch |
| `OtaCommand` missing `Commit=3` | firmware expects 0x03 | verify-then-commit broken |

**Impact:** TOT-31 app integration is blocked until reconciled. See
`docs/BLE-INTERFACE-SPEC.md` §10–11. (Follow-up FU-10.)

### C4. Controller → CarPlay bridge not wired

Lenker-Controller media buttons dispatch `APP_EVENT_CTRL_MEDIA_NEXT/PREV` on the
payload event loop, but `carplay_bridge` subscribed to **no** `APP_EVENT_CTRL_*`.
There was no code writing `CHR_CP_MEDIA_CTRL` (display 0x9104) in response. A rider
pressing "next track" on the handlebar could not control CarPlay media.

**→ Resolved (TOT-36):** `carplay_bridge` now subscribes to
`APP_EVENT_CTRL_MEDIA_NEXT/PREV` (registered in `carplay_bridge_init()`). The
subscriber maps the event to a `carplay_media_cmd` byte
(`carplay_media_cmd_from_event()`) and forwards it via
`carplay_client_media_ctrl()`, which writes the command byte to the peer
display's `CHR_CP_MEDIA_CTRL` (0x9104) — the handle is now discovered during
GATT service discovery. Mapping is unit-tested in `test_carplay_media.c`.

**Impact:** core UX feature non-functional. (Follow-up FU-4.)

### C5. Simulator speaks a different telemetry format

`tools/simulator/mock_ble.py set_telemetry_data()` packs
`<HHBBBhhBxxxxx` (rpm, speed, temp, fuel, gear, roll, pitch, flags) — different
field order, types, and semantics from firmware `telemetry_live_t`
(speed, rpm, gear, coolant, ambient, fuel, voltage, lat, lon, alt, heading,
sats, hdop). The simulator is the only "device" available for app/sim
integration, so it provides false confidence. Same divergence on DTC (0x7001),
sensor-status (0x7002), and self-test (0x7003).

**Impact:** any app test against the simulator validates the wrong wire format.
(Follow-up FU-2.)

---

## 4. High-Severity Issues

### H1. Dual event systems, unbridged — RESOLVED (TOT-38)

Two overlapping event mechanisms:
- `components/utils/event_bus.c` — FreeRTOS EventGroups, 19 bit-flags, no payload.
- `main/app_event_loop.c` — subscriber pub/sub, 128-byte payload, 16 slots.

Both define `BLE_CONNECTED`, `OTA_START`, `OTA_DONE`, `CRASH_DETECTED`,
`BATTERY_LOW/CRITICAL`, `SDCARD_*`, but a `event_bus_set(EVT_BLE_CONNECTED)`
does **not** fire the `APP_EVENT_BLE_CONNECTED` subscriber. Components
inconsistently use one or the other (BLE uses bit-flags; controller uses
payload pub/sub). `event_bus.c:4-5` TODO acknowledges payload events need a
second layer.

**Risk:** silent event loss, dead subscribers, race conditions where a state
change is observed by one system but not the other. **Fix:** bridge or
consolidate onto one system. (Follow-up FU-1.)

**Resolution (TOT-38):** A bridge callback was added to `event_bus.c`.
`event_bus_set()` now forwards set bits to a registered forwarder. The
`app_event_loop` registers a mapping table (`app_event_bridge_init()`)
that dispatches the corresponding `APP_EVENT_*` for all 18 overlapping
event bits. Subscribers in `main.c` that previously called
`event_bus_set()` from within app_event callbacks were cleaned up to
prevent infinite loops. New `APP_EVENT_CARPLAY_CONNECTED/DISCONNECTED`
types added to cover all event_bus bits. 22 host tests pass (12 event_bus
+ 10 bridge integration).

### H2. No OTA tests

No `test_ota*.c`. The state machine, NVS resume, CRC, SHA-256, rollback, and
secure-boot paths are entirely untested. The host-test build infrastructure
supports it (mocks exist for freertos/nvs), but no test exercises the OTA
component. (Follow-up FU-9.)

### H3. Host-test build failures (mock gaps)

4 of 10 unit suites fail to build on host: `test_dtc` (linker: undefined
`_capture_freeze_frame`), `test_config` (14 NVS/config mock errors),
`test_ble_callbacks` (`ble_internal.h` include path), `test_power_manager`
(`esp_sleep.h` mock missing). The CI workflow runs the same GCC invocation, so
these likely fail in CI too (or CI has additional handling). Coverage is
therefore narrower than the test count suggests. (Follow-up FU-12.)

### H4. Single GATT connection shared for CarPlay dual-role

`carplay_bridge.c:141` uses `carplay_client_get_conn_handle()` for server-side
notifies — the Bordcomputer acts as both server and client on the **same**
connection to the CarPlay display. `sdkconfig.defaults:18` allows 2 max
connections; `ble_service.c:53` sets `BLE_MAX_CONNECTIONS=1`. Verify the
companion-app link and the CarPlay link don't conflict; if both are needed
simultaneously, raise the limit. **Race risk** on conn-handle reuse.

### H5. CarPlay client CCCD subscription is a simplification

`carplay_client.c:102` writes `0x0001` to CCCD at `val_handle+1` (hardcoded +1
offset) rather than discovering the CCCD descriptor handle. Works for the
target EKIY/Jiuyin display but is fragile across display firmware revisions.

---

## 5. Medium-Severity Issues

### M1. `board_config.h` duplicated blocks

`firmware/main/board_config.h` — "Sensor-Konstanten", "RPM Pulse Counter",
"Coolant Temp", "Fuel Level" sections are each repeated **4 times** verbatim
(lines 86-322), bloating the file from ~190 to 322 lines and risking divergence.
(Follow-up FU-11.)

### M2. No firmware README

No `firmware/README.md`; the repo-level `README.md` (28 KB) is the only entry
doc. Build/flash instructions live in `docs/FIRMWARE-GUIDE.md` and
`docs/DEVELOPER-QUICKSTART.md`. A short firmware-local README would help
onboarding.

### M3. Pressure sensor declared but unimplemented

`board_config.h:41` references BMP280; `sensors.h:30` notes pressure "nicht
implementiert". Weather service (0x4000) advertises data the firmware cannot
source. Document as not-implemented or remove.

### M4. No SBOM / managed components

Everything vendored — simplifies offline builds but yields no component-registry
SBOM. Acceptable for a solo firmware, but flag for supply-chain review if
third-party dependencies are added.

### M5. Event-loop subscriber drop under load

`app_event_loop.c:201-204` drops events if the 32-deep queue is full (non-blocking
dispatch). Under burst load (e.g. sensor + GPS + controller simultaneous), events
can be silently lost. Consider a drop counter / overflow metric for diagnostics.

### M6. No integration-test report or tech-debt register existed

Prior to TOT-29, no `INTEGRATION-TEST-REPORT.md` or `TECH-DEBT.md` existed.
`docs/RELEASE-NOTES.md` feature-maturity is the closest proxy. This review
establishes the integration report; a standing tech-debt register is
recommended.

---

## 6. Race Conditions & Concurrency Review

| # | Location | Risk | Severity |
|---|---|---|---|
| R1 | `ble_callbacks.c` `s_sensor_cache`/`s_gps_cache` | Protected by `s_data_mutex`; `build_telemetry_live` takes it with `portMAX_DELAY`. Writer (`ble_on_sensor_update`) and reader (notify) both lock. **OK** if all paths lock. Verify no unguarded cache access in snapshot read (0x5002). | Low |
| R2 | `app_event_loop.c:66-72` | Mutex released during callback to avoid deadlock — correct, but a subscriber could observe stale state if it re-enters dispatch. Single-loop task serialises callbacks, so no re-entrancy within the loop. | Low |
| R3 | CarPlay dual-role conn-handle (H4) | Server-notify and client-notify on same handle; if disconnect races a notify, `ble_gap_event` ordering must be trusted. NimBLE serialises GAP events per-connection, so likely OK, but no explicit guard. | Medium |
| R4 | OTA NVS resume state (`ota_save/load_resume_state`) | HTTP range-resume persists write offset to NVS. If power loss occurs between `esp_ota_write` and `ota_save_resume_state`, resume offset may be stale → partial-image + hash mismatch → rollback triggers (safe, but wasteful). | Low |
| R5 | `event_bus` EventGroups vs `app_event_loop` (H1) | State observed by one system but not the other was the core race risk of the unbridged dual systems. **Resolved** in TOT-38: bridge callback forwards all overlapping EVT_* bits to APP_EVENT_* dispatches. | Resolved |

---

## 7. Error Handling Review

- **Init failures:** `init_step_*` wrappers register a DTC and continue (good);
  only NVS/power fatal (documented `main.c:21-23`). Acceptable.
- **BLE:** `ble_cmd_enqueue` returns false on full queue (10 ms timeout); caller
  should check. OTA control/data handlers `return` silently on short/unknown
  commands — no error notify back to the app. **App has no negative-ack path.**
- **OTA:** error codes (`OTA_ERR_VERIFY_HASH`, `_VERIFY_SIG`, `_SECURE_BOOT`,
  `_ANTI_ROLLBACK`) are defined and used; status rides 0x8001 notify. But since
  the BLE path is unwired (C1), the app never sees these.
- **GPS:** NMEA parser rejects bad checksums and tracks per-sentence stats
  (good). No recovery if UART stalls (no RX-timeout → `EVT_GPS_LOST_FIX`?).
- **SD card:** `EVT_SDCARD_ERROR` set on failure; logger silently drops if
  unmounted. No retry/backoff.

---

## 8. Recommendations (prioritised)

1. **Fix C1/C2 (OTA) before any field OTA.** Wire BLE callbacks; insert real RSA
   key; enable Secure Boot V2. OTA is a release blocker.
2. **Fix C3 (BLE contract) before TOT-31.** App must drop 0x7006/0x7007/0x8004,
   add `Commit=3`, and stop touching service 0x9000. See BLE-INTERFACE-SPEC.
3. **Fix C4 (controller→CarPlay).** Add `APP_EVENT_CTRL_*` subscribers in
   `carplay_bridge` that write display characteristics.
4. **Fix C5/H1 (simulator format + event bridge).** Regenerate `mock_ble.py`
   from `BLE-INTERFACE-SPEC.md`; bridge or consolidate the dual event systems.
5. **Add OTA tests (H2)** and fix the 4 host-test build failures (H3).
6. **De-dupe `board_config.h` (M1).**
7. **Establish a tech-debt register** and keep `docs/INTEGRATION-TEST-REPORT.md`
   updated each sprint.

---

## 9. Production-Readiness Verdict

**Not production-ready.** The firmware architecture is sound and well-structured,
and the partition/rollback/SHA-256 OTA foundations are correct. However, five
critical gaps (C1–C5) block release: the BLE OTA delivery path is a TODO,
signature verification is disabled, the app/firmware BLE contract diverges, the
controller↔CarPlay path is unwired, and the simulator validates the wrong wire
format. Sprint 2 integration cannot be declared complete until the C1–C5
follow-ups land.

— @klaus

# Integration Test Report — TRANSALP Bordcomputer

**Task:** TOT-29 (System Integration & Architecture Review)
**Parent:** TOT-16
**Reviewer:** @klaus (CTO)
**Date:** 2026-10-20
**Firmware version:** `0.4.0-integration` (`firmware/main/main.c:57`)
**Repo:** `/Users/tito/transalp-bordcomputer` @ `main`

This report documents pass/fail for each end-to-end data path required by TOT-29
task items 1–3, plus host-test and simulator-test execution results. It feeds the
follow-up architecture review (`docs/ARCHITECTURE-REVIEW.md`).

---

## 1. Test Execution Summary

### 1.1 Firmware host unit + integration tests (Unity + GCC)

Build & run replicated from `.github/workflows/firmware-ci.yml` (unit-tests job)
on macOS arm64 with `/tmp/unity` and `gcc`. Command:
`gcc -DTEST_HOST -DUNITY_INCLUDE_DOUBLE -std=c11 -Wall -Wextra -I… test.c unity.c -lm -lpthread`.

| Suite | Type | Tests | Result | Notes |
|---|---|---|---|---|
| `test_nmea_parser` | unit | 15 | ✅ PASS | GGA/RMC/VTG/GSV, checksum, hemispheres |
| `test_event_loop` | unit | 9 | ✅ PASS | event_bus bit-flag set/clear/wait |
| `test_filters` | unit | 15 | ✅ PASS | Kalman, complementary, moving-average |
| `test_dtc_extended` | unit | 21 | ✅ PASS | DTC severity, freeze-frame, history |
| `test_carplay_bridge` | unit | 14 | ✅ PASS | `carplay_telemetry_t` 12 B packing |
| `test_controller` | unit | 12 | ✅ PASS | debounce, mapping, I2C masks |
| `test_integration_sensor_to_ble` | integration | 9 | ⚠️ PASS-but-wrong-contract | validates the **simulator** format, not firmware `telemetry_live_t` — see §3.1 |
| `test_integration_gps_nmea_pipeline` | integration | 10 | ✅ PASS | NMEA gen → `nmea_parse()` round-trip |
| `test_integration_controller_mapping` | integration | 14 | ✅ PASS | button → debounce → mapping → event |
| **Subtotal (built)** | | **119** | **all green** | |
| `test_dtc` | unit | — | ❌ BUILD FAIL | linker: undefined `_capture_freeze_frame` (host-mock gap) |
| `test_config` | unit | — | ❌ BUILD FAIL | 14 errors (host-mock gap: NVS/config APIs) |
| `test_ble_callbacks` | unit | — | ❌ BUILD FAIL | `ble_internal.h` include path + 1 error |
| `test_power_manager` | unit | — | ❌ BUILD FAIL | `esp_sleep.h` mock missing |

### 1.2 Simulator tests (pytest, venv)

`tools/simulator/.venv/bin/pytest tests/ -v` — **121 passed in 0.35s**.

| File | Coverage |
|---|---|
| `test_mock_ble.py` | GATT read/write/notify, DTC, sensor-status, telemetry packing (⚠️ wrong format — §3.1) |
| `test_mock_sensors.py` | sensor curves, gear/speed, IMU, thresholds |
| `test_mock_gps.py` | NMEA gen, checksums, haversine, bearing |
| `test_mock_display.py` | framebuffer, RGB565, dirty-rects, modes |
| `test_mock_controller.py` | button injection |
| `test_simulator.py` | end-to-end ticks, warnings, status JSON, CI mode |

### 1.3 On-target / HIL tests

**Not executed** — no ESP32-S3 hardware in this review run. `idf.py build` and
`idf.py test` (ESP-IDF test-component mode, `firmware/test/CMakeLists.txt:90`)
are the target-side path; CI runs the ESP-IDF build matrix (`firmware-ci.yml`)
on push. This report covers **host + simulator** layers only.

---

## 2. Data Path 1: Sensors → Event Bus → BLE-Bridge → Companion App

**Task item 1.** Verdict: ⚠️ **PARTIALLY VERIFIED — contract mismatch breaks the path.**

### 2.1 Code path (static analysis)

```
sensor_task (Core 1, 50 Hz)
  → sensor_data_t queue
  → main.c:on_sensor_update() [app_event subscriber]
  → ble_on_sensor_update() [direct callback, NOT via event bus]
  → s_sensor_cache (mutex-protected)
  → ble_task: build_telemetry_live() at 5 Hz
  → ble_notify_telemetry_live() → CHR_TLM_LIVE 0x5001 (22 B notify)
  → [BLE radio] → companion-app parseTelemetryLive()
```

### 2.2 Findings

| # | Finding | Severity |
|---|---|---|
| D1-1 | **BLE is not event-bus-driven.** `ble_on_sensor_update()` is a **direct function call** from `main.c:on_sensor_update`, not an `app_event_subscribe` callback. The "Event Bus → BLE-Bridge" hop in the task description does not exist as a pub/sub; it is a hard-wired call. Functional, but bypasses the event loop. | Medium |
| D1-2 | **Dual event systems are not bridged.** `utils/event_bus` (bit-flags) and `main/app_event_loop` (payload pub/sub) define overlapping events (`BLE_CONNECTED`, `OTA_START`, `CRASH_DETECTED`…) but a `event_bus_set()` does **not** fire the `app_event` subscriber. `event_bus.c:4-5` has a TODO acknowledging this. | High |
| D1-3 | **Simulator telemetry format ≠ firmware format.** `mock_ble.py set_telemetry_data()` packs `<HHBBBhhBxxxxx` (rpm, speed, temp, fuel, gear, roll, pitch, flags). Firmware `telemetry_live_t` (`ble_callbacks.c:56`) packs speed/rpm/gear/coolant/ambient/fuel/voltage/lat/lon/alt/heading/sats/hdop. Both are 22 B — but every field differs. | **Critical** |
| D1-4 | **Integration test validates the wrong contract.** `test/integration/test_integration_sensor_to_ble.c` (lines 46-57, 67-94) packs/unpacks the **simulator** format, not the firmware struct. Its own header cites `mock_ble.py` as the packing reference. All 9 tests pass, giving false confidence that the sensor→BLE→app path is verified. It is not. | **Critical** |
| D1-5 | Companion-app `parseTelemetryLive` (`binary-parsers.ts`) **does** match the firmware `telemetry_live_t`. So the real firmware↔app wire contract is consistent — but there is **no test** that exercises it end-to-end (simulator is the only "device" and it speaks the wrong format). | High |

### 2.3 Verdict

The real firmware→app telemetry path is **contract-consistent but untested**.
The existing "integration test" passes against a divergent simulator format and
must be rewritten to pack/unpack the firmware `telemetry_live_t`. Recommend a
follow-up task (see §5).

---

## 3. Data Path 2: CarPlay BLE-Bridge ↔ Lenker-Controller via Event Bus

**Task item 2.** Verdict: ⚠️ **NOT VERIFIED — no event-bus coupling exists.**

### 3.1 CarPlay bridge

`components/carplay_bridge/` — dual-role BLE to an external CarPlay display:
- **Server** (service 0x9000): pushes `carplay_telemetry_t` (12 B) to the display.
- **Client** (display's 0x9100): subscribes to nav-status/media notifies.

`carplay_bridge_task` (5 Hz) builds telemetry from `s_sensor_cache` /
`s_gps_cache` (via `ble_on_sensor_update` cache) and notifies `CHR_CP_TLM_LIVE`.
It sets `EVT_CARPLAY_CONNECTED`/`EVT_CARPLAY_DISCONNECTED` (bit-flags) on
client connect/disconnect.

### 3.2 Lenker-Controller (HID)

`components/hid_controller/` — 11 buttons from GPIO/I2C/BLE, debounced (50 ms),
then `controller_mapping_dispatch()` → `app_event_dispatch(APP_EVENT_CTRL_*)`.

### 3.3 Findings

| # | Finding | Severity |
|---|---|---|
| D2-1 | **No event path connects controller events to the CarPlay bridge.** Controller presses dispatch `APP_EVENT_CTRL_*` on the payload event loop. The CarPlay bridge does **not** subscribe to any `APP_EVENT_CTRL_*`. So a handlebar button press does not propagate to the CarPlay display via the bridge. | **Critical** |
| D2-2 | The bridge reads sensor/GPS caches directly (same hard-wired call pattern as BLE). No event-bus mediation. | Medium |
| D2-3 | `test_integration_controller_mapping` (14/14 PASS) verifies button→debounce→mapping→event-ID, but **stops at dispatch** — it does not assert any subscriber (CarPlay or otherwise) received the event. End-to-end controller→CarPlay is untested. | High |
| D2-4 | Controller media buttons (NEXT/PREV/VOL) are intended to control the CarPlay display's media, but there is no code writing `CHR_CP_MEDIA_CTRL` (0x9104 on the display) in response to `APP_EVENT_CTRL_MEDIA_*`. The forward path is missing. | **Critical** |

### 3.4 Verdict

The CarPlay bridge and Lenker-Controller are **independent islands**. The
"interaction via event bus" required by the task does not exist. Controller
media buttons do not reach the CarPlay display. Requires a follow-up to wire
`APP_EVENT_CTRL_*` subscribers in `carplay_bridge` that forward to the display.

---

## 4. Data Path 3: OTA Update Flow (dual-partition rollback, SHA-256)

**Task item 3.** Verdict: ⚠️ **PARTIALLY VERIFIED — partition logic solid; BLE path & signature broken.**

### 4.1 Partition table (`firmware/partitions.csv`)

| Name | Type | Subtype | Offset | Size |
|---|---|---|---|---|
| nvs | data | nvs | 0x9000 | 24 KB |
| phy_init | data | phy | 0xF000 | 4 KB |
| factory | app | factory | 0x10000 | 3 MB |
| ota_0 | app | ota_0 | 0x310000 | 3 MB |
| ota_1 | app | ota_1 | 0x610000 | 3 MB |
| otadata | data | ota | 0x910000 | 8 KB |
| storage | data | fat | 0x912000 | ~7 MB |

✅ Dual OTA partition + `otadata` + `factory` correctly configured. 16 MB flash.

### 4.2 Findings

| # | Finding | Severity |
|---|---|---|
| D3-1 | **`esp_ota_*` API usage is correct and complete.** `ota_update.c` uses `get_next_update_partition`, `begin`, `write`, `end`, `set_boot_partition`, `get_running_partition`, `get_state_partition`, `mark_app_valid_cancel_rollback`, `mark_app_invalid_rollback_and_reboot`. `CONFIG_APP_ROLLBACK_ENABLE=y`. | ✅ OK |
| D3-2 | **SHA-256 verification implemented.** `esp_partition_get_sha256` after `esp_ota_end`, compared to `expected_hash` (`ota_update.c:207-224`). Per-chunk CRC16-CCITT in `ota_ble_write_chunk`. | ✅ OK |
| D3-3 | **Rollback handled.** `ota_check_on_boot()` runs a self-test (NVS, heap >50 KB, OTA idle) and marks valid/invalid on boot (`ota_update.c:920-950`). Manual `ota_rollback()` exists. | ✅ OK |
| D3-4 | **BLE OTA data path is a TODO — non-functional.** `ble_callbacks.c:695` `process_ota_data()` only logs: `/* TODO: esp_ota_write() with proper partition handling */`. It does **not** call `ota_ble_write_chunk()`. OTA chunks written by the app to `CHR_OTA_DATA` (0x8002) are **silently dropped**. | **Critical** |
| D3-5 | **BLE OTA control not wired to state machine.** `process_ota_control()` (`ble_callbacks.c:673`) only sets/clears `EVT_OTA_START`/`EVT_OTA_DONE` bit-flags — it does not call the OTA component's begin/commit/verify functions. The OTA `IDLE→DOWNLOADING→…→REBOOTING` state machine is never driven from BLE. | **Critical** |
| D3-6 | **RSA-3072 signature verification non-functional.** `ota_secure_boot.c:63` `ota_public_key_der` is an all-zero placeholder; `ota_secure_verify_signature` returns `ESP_ERR_NOT_SUPPORTED` and falls back to **hash-only** (signature skipped). `#warning` fires at build. Secure Boot V2 + Flash Encryption are commented out in `sdkconfig.defaults`. | **Critical (release blocker)** |
| D3-7 | **Anti-rollback is a stub.** `s_efuse_version_counter` is a RAM variable; real `esp_efuse_write_field_bit` calls are commented out (`ota_secure_boot.c:247-256, 273-283`). | High |
| D3-8 | **No OTA tests exist.** No `test_ota*.c` in `firmware/test/`. The secure-boot path (placeholder key, stubbed eFuse) is entirely untested. | High |

### 4.3 Verdict

OTA **partition/rollback/SHA-256 architecture is production-grade**, but the
**BLE delivery path is non-functional** (TODO), the **signature check is disabled**
(placeholder key), and **anti-rollback is stubbed**. OTA over BLE cannot work
today. HTTP/SD-card transports in `ota_update.c` may work but are likewise
untested. This is a release blocker.

---

## 5. Follow-up Tasks (critical gaps → new issues)

All critical/high gaps are logged as follow-up tasks (see TOT-29 final comment for
issue IDs). Summary:

| ID (TBD) | Gap | Severity | Source |
|---|---|---|---|
| FU-1 | Bridge the dual event systems (`event_bus` ↔ `app_event_loop`) or consolidate | High | D1-2 |
| FU-2 | Fix simulator telemetry format to match firmware `telemetry_live_t` | Critical | D1-3 |
| FU-3 | Rewrite `test_integration_sensor_to_ble.c` to validate the firmware struct, not the simulator format | Critical | D1-4 |
| FU-4 | Wire Lenker-Controller `APP_EVENT_CTRL_*` → CarPlay bridge → display media control | Critical | D2-1, D2-4 |
| FU-5 | Implement BLE OTA data path: `process_ota_data()` → `ota_ble_write_chunk()` | Critical | D3-4 |
| FU-6 | Wire BLE OTA control (`process_ota_control`) to the OTA state machine | Critical | D3-5 |
| FU-7 | Insert real RSA-3072 public key + enable Secure Boot V2 / Flash Encryption for production | Critical | D3-6 |
| FU-8 | Implement real eFuse anti-rollback counter | High | D3-7 |
| FU-9 | Add OTA unit + integration tests | High | D3-8 |
| FU-10 | Reconcile app/firmware characteristic divergences (0x7006, 0x7007, 0x8004, 0x9000 collision) — see `docs/BLE-INTERFACE-SPEC.md` §10–11 | Critical | spec |
| FU-11 | Fix `board_config.h` duplicated blocks (4× copy-paste) | Medium | arch review |
| FU-12 | Fix host-test build failures (test_dtc, test_config, test_ble_callbacks, test_power_manager mock gaps) | Medium | §1.1 |

---

## 6. Sign-off

The integration test layer (host unit + simulator) is **green where it builds**,
but the green "integration" tests do **not** actually verify the cross-layer
contracts they claim to (telemetry format mismatch, no controller→CarPlay path,
non-functional BLE OTA). The real end-to-end data paths remain **unverified** and
require the follow-ups above before Sprint 2 integration can be declared complete.

— @klaus

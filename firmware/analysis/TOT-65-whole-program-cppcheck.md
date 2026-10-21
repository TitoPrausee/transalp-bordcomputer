# TOT-65 — Whole-program cppcheck `staticFunction` analysis via full `idf.py` build

Branch: `sprint-7/hardening`. Follow-up to TOT-61 (firmware hardening).

## Origin

TOT-61 suppressed the cppcheck `staticFunction` whole-program check with
documented rationale: the check requires cross-translation-unit / linker-level
analysis, which is unavailable when analyzing ESP-IDF component firmware with
host mocks (no ESP-IDF headers in the dev environment, no
`compile_commands.json` with cross-TU linkage). This task stands up the
whole-program analysis in CI and triages the 32 suppressed findings.

## What changed

### 1. CI: real `idf.py` build produces `compile_commands.json`

Added a new `whole-program-cppcheck` job to
`.github/workflows/firmware-ci.yml` (between `esp-idf-build` and `unit-tests`).
It:

1. Sets up ESP-IDF v5.4 (esp32s3 target) via `espressif/esp-idf-ci-action@v1`.
2. Runs `idf.py build` — this produces `build/compile_commands.json` with the
   real cross-compiler include paths, macro definitions, and cross-TU linkage
   that directory-mode / host-mock analysis lacks.
3. Filters the database to project sources only
   (`build/compile_commands_project.json`) via
   `firmware/scripts/filter_compile_commands.py` — the raw database contains
   hundreds of ESP-IDF managed-component entries we do not own.
4. Runs cppcheck `--enable=all --check-level=exhaustive` against the filtered
   database, with `--error-exitcode=1` (blocking).
5. Uploads the XML report as an artifact.

The existing `static-analysis` job (directory-mode, `--enable=warning,performance,
portability`) is retained for fast non-style feedback; the new job is the
authoritative whole-program analysis.

### 2. `staticFunction` suppression removed

`firmware/analysis/cppcheck-suppressions.txt` no longer suppresses
`staticFunction`. The `unusedFunction` suppression is **retained** — those
findings remain false positives even with whole-program analysis (ESP-IDF entry
points, FreeRTOS task entries via `xTaskCreate`, BLE callbacks registered by
function pointer — all indirect calls cppcheck cannot trace).

### 3. Triage of the 32 `staticFunction` findings

All 32 findings were re-examined. **Every one is a function declared in a
component header** (public `include/*.h` or internal `*_internal.h`) and called
cross-translation-unit. Making any of them `static` would conflict with its
non-static header declaration (a C constraint violation) and break the component
API contract.

| Component | Functions (all header-declared) | Header |
|-----------|---------------------------------|--------|
| ble | `ble_cmd_enqueue`, `ble_handle_read`, `ble_handle_write`, `ble_gap_event_handler` | `ble_internal.h` |
| carplay_bridge | `carplay_gatt_access_cb`, `carplay_client_get_state`, `carplay_client_gap_event` | `carplay_internal.h` |
| carplay_bridge | `carplay_bridge_get_status`, `carplay_bridge_is_connected`, `carplay_bridge_notify_telemetry`, `carplay_bridge_pair`, `carplay_bridge_unpair` | `include/carplay_bridge.h` |
| diagnostics | `diag_console_task`, `diag_console_init`, `diag_init`, `dtc_default_severity` | `include/diagnostics.h` |
| display | `tft_set_backlight` | `include/display.h` |
| ota | `ota_check_on_boot`, `ota_firmware_self_test` | `include/ota_update.h` |
| power | `power_set_backlight`, `power_set_gps_enabled`, `power_set_ble_enabled`, `power_reset_inactivity`, `power_get_wakeup_reason`, `power_init_with_config` | `include/power.h` |
| storage | `config_save`, `config_load_all`, `logger_write_csv_header`, `logger_write_csv_row`, `logger_export_gpx`, `logger_add_track_point`, `logger_flush` | `include/storage.h` |

**Result: 0 genuinely TU-local functions, 0 genuinely-dead functions.** No code
changes were needed — the 32 findings were all false positives caused by
cppcheck's per-TU analysis lacking cross-TU call visibility.

### 4. Verification

Confirmed locally (cppcheck 2.22.0) against the host-mock
`compile_commands.json` (project mode, which provides the same cross-TU
linkage as the real `idf.py` database for our sources):

- **Directory mode** (per-TU, audit-style): 32 `staticFunction` findings
  (all false positives — header-declared public API).
- **Project mode** (`--project=compile_commands.json`, whole-program):
  **0 `staticFunction` findings.**

### 5. CI verification (cppcheck 2.13.0, real idf.py compile_commands.json)

The `whole-program-cppcheck` CI job runs inside the `espressif/idf:v5.4`
Docker container: `idf.py build` produces `build/compile_commands.json`
(1078 entries), `filter_compile_commands.py` filters to 39 project-source
entries, and cppcheck 2.13.0 runs `--enable=all --check-level=exhaustive`
against the filtered database.

**CI run 39583990927: all 6 jobs pass** (ESP-IDF Build v5.2/v5.3/v5.4,
Unit Tests, Static Analysis, Whole-program cppcheck). ✅

The first CI runs surfaced 6 findings from the real ESP-IDF headers
(not reproducible with host mocks). Triaged and resolved:

- **2 `variableScope`** (power.c, power_manager.c): cppcheck 2.13.0 reports
  these on `static const` string arrays inside `if` blocks, even though the
  scope is already minimal. Not reported by cppcheck 2.22.0 (version-specific
  false positive). Arrays made `static const` + inline-suppressed.
- **1 `syntaxError`** (NimBLE `os_mbuf.h`): cppcheck can't parse the NimBLE
  `os_mbuf.h` macro-heavy header. The build succeeded, so this is a parser
  limitation. Suppressed via `--suppress=syntaxError:*os_mbuf.h`.
- **3 `constParameterPointer`** (ESP-IDF `mbedtls/ssl.h`): findings in
  ESP-IDF's mbedtls header, not our code. Our const-correctness was already
  verified in TOT-61. Suppressed via `--suppress=constParameterPointer:*ssl.h`.

### 6. Build fixes (pre-existing, found during CI setup)

TOT-64 (`ble_privacy` component) introduced two build issues that were masked
by `continue-on-error: true` on the `esp-idf-build` job. Fixed as part of
TOT-65 since the whole-program cppcheck job requires a successful build:

- `ble_privacy/CMakeLists.txt`: removed `esp_log` from `PRIV_REQUIRES` (not a
  resolvable standalone component in ESP-IDF v5.2-v5.4 Docker images).
- `ble_privacy.c`: added `#include "esp_random.h"` (ESP-IDF no longer includes
  it from `esp_system.h`; `esp_fill_random()` was implicitly declared).
- `test/mocks/esp_random.h`: new mock (mirrors real ESP-IDF structure;
  `test/mocks/esp_system.h` includes it for backward compat).

## Acceptance criteria status

| Criterion | Status |
|-----------|--------|
| ESP-IDF build produces `compile_commands.json` in CI | ✅ Done — `whole-program-cppcheck` job runs `idf.py build` inside Docker |
| Whole-program cppcheck runs green on the branch | ✅ CI run 39583990927: all 6 jobs pass, 0 unsuppressed findings |
| `staticFunction` suppression removed | ✅ Done — only `unusedFunction` retained with updated rationale |
| CI step added and passing | ✅ Blocking `whole-program-cppcheck` job (`--error-exitcode=1`) — green |
| Genuinely-dead functions removed; TU-local functions made static | ✅ N/A — triage found 0 (all 32 are header-declared public API) |

Do **not** merge to `main` — board may want v1.0.0 tagged first (per task scope).

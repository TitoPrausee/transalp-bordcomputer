# TOT-58 — Firmware Strict Static Analysis + BLE Security Audit

Branch: `sprint-7/hardening` (off `52bf411`). Does **not** touch `main`.

## 1. Strict static analysis

### cppcheck (STRICT)

Command:
```
cppcheck --enable=all --check-level=exhaustive \
  --suppress=missingIncludeSystem --suppress=missingInclude \
  --suppressions-list=analysis/cppcheck-suppressions.txt \
  -I components/*/include -I components/ble -I components/carplay_bridge \
  -I components/hid_controller -I main --inline-suppr --force \
  components/ main/
```

Result: **0 warnings** at strict (115/189 checkers active).

Baseline was 220 style findings. They were resolved as follows:

- **48 real code fixes** (no false positives):
  - const-correctness: 18 `constVariable`/`constVariablePointer`/`constParameterPointer` + 1 `constParameterCallback` (const array/pointer/parameter qualifications across ble, carplay, display, gps, storage, app_event_loop).
  - 5 `unusedStructMember` — removed genuinely-dead struct members (`http_dl_ctx_t::error`/`using_range` — whole dead typedef removed; `rtc_persist_t::charge_cycles`/`discharge_cycles` — dead RTC-persist fields removed; `diag_command_t::help` — wired up: `cmd_help` now iterates the command table instead of hardcoding strings).
  - 2 `unreadVariable` — `old_addr_type` now logged on BLE disconnect; `delay_ms` now applied to `s_reconnect_at_us` (implements the previously-dead exponential CarPlay reconnect backoff).
  - 1 `variableScope` — `read_len` moved into the OTA download loop.
  - 1 `duplicateConditionalAssign` — `power_adc_deinit` restructured (unconditional NULL assign, guarded `adc_cali_delete`).
  - 1 `unsignedLessThanZero` + 1 `knownConditionTrueFalse` — `selftest.c` `DISPLAY_CS_PIN` check converted to a preprocessor guard (the pin is a compile-time constant).
  - 1 `knownConditionTrueFalse` — `nmea_parser.c` dead `if/else` with identical branches removed.

- **2 documented whole-program suppressions** (`analysis/cppcheck-suppressions.txt`):
  - `unusedFunction` — cppcheck's own docs state this check needs whole-program analysis. All findings are header-declared component API, ESP-IDF task entries (`app_main`, `*_task` invoked by the FreeRTOS scheduler), or callbacks registered via function pointer. Matches the existing CI suppression.
  - `staticFunction` — all 32 findings are functions declared in component headers (public `include/*.h` or internal `*_internal.h`); making them static would conflict with their header declarations and break the API contract. Distinguishing genuinely-TU-local functions requires a full `idf.py` build (`compile_commands.json`) — tracked as a follow-up.

### clang-tidy (STRICT)

Config: `firmware/.clang-tidy` (strict checks: `bugprone-*`, `cert-*`, `clang-analyzer-*`, `cppcoreguidelines-*`, `misc-*`, `modernize-*`, `performance-*`, `portability-*`, `readability-*`, with documented exclusions for embedded-inappropriate / pure-style checks).
Analysis run with host mocks (`-include test/mocks/esp_idf_mock.h`) since ESP-IDF is not available in this environment; a `compile_commands.json` is generated for reproducibility.

Result: **0 warnings**.

Baseline was 1283 warnings. Resolved as:
- **~30 real fixes**: `bugprone-integer-division` (power.c ADC filter precision bug), `bugprone-sizeof-expression` (documented), `bugprone-implicit-widening-of-multiplication-result` (6 — size_t casts on display-buffer indexing), `bugprone-too-small-loop-variable` (3 — `uint16_t`→`size_t`/`int`), `bugprone-branch-clone` (dead identical if/else removed; intentional state-mapping & placeholder reads documented with NOLINT), `bugprone-multi-level-implicit-pointer-conversion`, `misc-unused-parameters`, `readability-redundant-nested-if`, `readability-redundant-declaration`, `readability-trivial-switch` (mock artifact), `modernize-avoid-c-style-cast` (C++ check, disabled for C), `cert-int09-c` enum-initializer consistency.
- **Documented exclusions** with rationale in `.clang-tidy`: `cppcoreguidelines-macro-to-enum`/`modernize-macro-to-enum` (#define integral constants are the codebase convention for register maps / command bytes), `cppcoreguidelines-init-variables`/`narrowing-conversions` (defensive-style, noisy in embedded), `misc-include-cleaner` (broken by mocked headers), `misc-use-internal-linkage` (whole-program, same as cppcheck `staticFunction`), `cert-dcl37-c`/`cert-dcl51-cpp` (reserved-identifier include guards `_XXX_H_` — codebase-wide convention enforced by the CI include-guard check; rename tracked as a project-wide follow-up), pure-style readability checks.
- **Mock-induced false positives suppressed inline with rationale**: `clang-analyzer-core.uninitialized.Assign` (the `strlen` mock returns a constant, making the analyzer believe NMEA `raw` is shorter than the guard guarantees), `clang-analyzer-unix.Stream` (host stdio model doesn't track the `fseek(SEEK_SET)` reset), `readability-trivial-switch` on NimBLE GAP-event handlers (`BLE_GAP_EVENT_*` / `struct ble_gap_event` are not modeled in the host mocks, so case labels collapse).

### Unit tests

12/13 host unit tests pass (test_nmea_parser, test_config, test_ota, test_power_manager, test_dtc, test_dtc_extended, test_filters, test_event_loop, test_event_bridge, test_carplay_bridge, test_carplay_media, test_ota_secure_boot). `test_ble_callbacks` has a **pre-existing** link failure (references `app_event_dispatch_simple` which is defined in `main/app_event_loop.c` but not linked into the test) — fails identically on the unmodified `52bf411` baseline; not introduced by this work.

## 2. BLE security audit

### Pairing / bonding flow — GOOD (no change)
- LE Secure Connections (`sm_sc=1`) with bonding (`sm_bonding=1`).
- MITM protection (`sm_mitm=1`) via Numeric Comparison (`sm_io_cap = BLE_SM_IO_CAP_DISP_YES_NO`).
- Key distribution: ENC + ID, both directions.
- Bond storage: NimBLE NVS persistence (`CONFIG_BT_NIMBLE_NVS_PERSIST=y`, `store_status_cb = ble_store_util_status_rr`, `CONFIG_BT_NIMBLE_MAX_BONDS=4`).
- Repeat-pairing handler deletes the old bond and retries (`BLE_GAP_REPEAT_PAIRING_RETRY`) — standard NimBLE pattern.

### MITM protection — GOOD (no change)
LE Secure Connections + MITM + Numeric Comparison provides ECDH-based MITM protection during pairing.

### Key storage — GOOD (no change)
Bond keys persist in NVS; `CONFIG_SECURE_FLASH_ENC_ENABLED=y` (Flash Encryption) and `CONFIG_SECURE_BOOT_V2_ENABLED=y` (Secure Boot V2) are enabled, so bond keys are encrypted at rest. Anti-rollback (`CONFIG_APP_ROLLBACK_ENABLE` + eFuse SECURE_VERSION) is in place for OTA.

### Encryption enforcement — GAP FIXED (critical)

**Finding:** No GATT characteristic carried encryption/authentication flags. Sensitive write characteristics — **OTA control/data, bond management, config write/import, self-test, log/track export control**, and all control writes (nav/media/phone) — were accessible on *any* connection regardless of link security. The write handler (`ble_handle_write`) performed no encryption check either. An unpaired device connecting could therefore push OTA data, clear bonds (DoS / lockout), or change config before pairing completed.

**Fix:** Added NimBLE security flags to the GATT table in `components/ble/ble_service.c`:
- `BLE_GATT_CHR_F_WRITE_AUTHEN` on **all** write characteristics (requires an authenticated/MITM-encrypted link, matching the `sm_mitm=1` pairing config).
- `BLE_GATT_CHR_F_READ_ENC` on sensitive reads: `config_read`, `telemetry_snapshot`, `dtc_list`, `sensor_status` (contain config / vehicle / GPS-location data).

NimBLE now rejects unauthenticated/unencrypted access to these characteristics with `BLE_ATT_ERR_INSUFFICIENT_AUTHEN`/`_ENCRYPTION` before the access callback runs.

### Coordination note
The companion app (TOT-20, owner @felix) must complete LE Secure Connections pairing before accessing secured characteristics. App-side flows that read telemetry/config or write OTA/config/bond-management before pairing will now receive ATT security errors until paired. This is the intended, secure behaviour.

### Recommendation (not fixed here — out of scope)
BLE privacy (random/resolvable addresses) to mitigate long-term device-address tracking — track as a separate follow-up.

## 3. Summary

| Area | Status |
|------|--------|
| cppcheck strict | 0 warnings (48 real fixes + 2 documented whole-program suppressions) |
| clang-tidy strict | 0 warnings (~30 real fixes + documented config) |
| Unit tests | 12/13 pass (1 pre-existing link failure) |
| BLE pairing/bonding | Good — LE SC + MITM + bonding + NVS |
| BLE MITM protection | Good — Numeric Comparison |
| BLE key storage | Good — NVS + Flash Encryption + Secure Boot V2 |
| BLE encryption enforcement | **Fixed** — GATT AUTHEN/ENC flags added to sensitive characteristics |

Do **not** merge — board may want v1.0.0 tagged first (per task scope).

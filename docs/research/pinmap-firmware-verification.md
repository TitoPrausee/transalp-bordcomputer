# Pinmap ↔ Firmware Verification Report

> **Issue:** TOT-30 PCB Manufacturing Prep & Hardware Assembly Guide
> **Task:** Verify pinmap and wiring against firmware pin assignments
> **References:** `docs/PINMAP.md` (sole reference), `firmware/main/board_config.h`,
> `docs/PCB-LAYOUT.md`
> Stand: Oktober 2026 · Author: Wolfgang (@wolfgang)

`docs/PINMAP.md` declares itself the **sole reference** for all pin assignments
("Diese Datei ist die alleinige Referenz fuer alle Pin-Zuweisungen.
`firmware/main/board_config.h` muss dieser Tabelle entsprechen.").

This report cross-checks `board_config.h` against that reference.

## Summary

`board_config.h` is **substantially out of sync** with `PINMAP.md`. Of the
firmware-defined signal pins, **10 pin assignments conflict** and **5 PINMAP
signals are entirely missing** from firmware. In addition, `board_config.h`
contains a **duplicate-definition defect** (three blocks of macros are repeated
4× — see §3). The MCU module variant and several sensor part numbers also
disagree between docs.

**This is a blocking finding for PCB manufacturing.** Routing the board against
the current `board_config.h` would conflict with `PINMAP.md` (the documented
wiring), and vice-versa. The conflict must be resolved with @klaus (CTO) before
the copper signal routing is laid out in KiCad.

## 1. Conflicting Pin Assignments

| Signal (PINMAP)        | PINMAP GPIO | board_config.h define   | board_config value | Conflict                                              |
|------------------------|-------------|-------------------------|--------------------|-------------------------------------------------------|
| TFT_RST (ILI9488 RST)  | 17          | `DISPLAY_RST_PIN`       | 21                 | 21 = STATUS_LED in PINMAP                             |
| STATUS_LED             | 21          | (reused as DISPLAY_RST) | 21                 | Drives display reset, not status LED                   |
| SD_CS (MicroSD CS)     | 47          | `SD_CS_PIN`             | 38                 | 38 = BUZZER in PINMAP; 47 unused in firmware            |
| DISPLAY_BUSY / EPD?     | — (47 used for SD) | `DISPLAY_BUSY_PIN` | 47                 | 47 = SD_CS in PINMAP; no BUSY pin in PINMAP at all     |
| COOLANT_TEMP (NTC ADC) | 2 (ADC1_CH1)| `TEMP_NTC_ADC_CHANNEL`  | 6 (ADC1_CH6 = GPIO7)| 7 = BTN_MENU in PINMAP; coolant should be GPIO2/CH1   |
| RPM_PULSE (PCNT0)      | 15          | `TEMP_ONEWIRE_PIN`      | 15                 | 15 = RPM optocoupler input; repurposed as 1-Wire       |
| FUEL_LEVEL (ADC)       | 4 (ADC1_CH3)| `FUEL_ADC_CHANNEL`      | 2 (ADC1_CH2 = GPIO3)| 3 = BTN_BACK (strapping); fuel should be GPIO4/CH3     |
| TACHO_PULSE (PCNT1)    | 16          | `RPM_PCNT_PIN`          | 16                 | Firmware uses 16 for RPM; PINMAP uses 16 for tacho     |
| OIL_PRESSURE (ADC)      | 5           | `GPS_POWER_EN_PIN`      | 5                  | 5 = oil-pressure ADC; repurposed as GPS power enable   |
| BTN_MENU               | 6           | `BOOST_EN_PIN`          | 6                  | 6 = menu button; repurposed as boost enable            |
| MPU6050_INT            | 48          | `MPU6050_INT_PIN`       | 2                  | 2 = COOLANT_TEMP ADC; IMU IRQ should be GPIO48          |
| GPS baud               | 115200 (AT6558)| `GPS_UART_BAUD`       | 38400 (NEO-M9N)    | GPS module identity + baud mismatch                    |

`DISPLAY_CS_PIN=10` ✓, `DISPLAY_DC_PIN=14` ✓, `SPI_*` (11/12/13) ✓,
`I2C_*` (8/9) ✓, `TFT_BL_PIN=18` ✓, `BTN_*` (6/7/3/0) ✓, `GPS_UART_*_PIN` (43/44) ✓,
`GPS_PPS_PIN=39` ✓ — these match.

## 2. PINMAP Signals Missing from Firmware

The following signals defined in `PINMAP.md` have **no corresponding definition**
in `board_config.h`:

| PINMAP signal     | GPIO | Notes                                             |
|-------------------|------|---------------------------------------------------|
| EPD_CS (E-Paper)  | 40   | E-Paper chip-select not wired in firmware         |
| EPD_DC (E-Paper)  | 41   | E-Paper data/command not wired                    |
| EPD_RST (E-Paper) | 42   | E-Paper reset not wired                           |
| BUZZER            | 38   | Piezo buzzer (LEDC PWM) not defined               |
| STATUS_LED        | 21   | Status/heartbeat LED not defined (shadowed by RST)|
| OIL_PRESSURE      | 5    | Oil-pressure ADC channel not defined              |
| TACHO_PULSE       | 16   | Speed (Reed) PCNT1 — firmware only has RPM on 16  |

`board_config.h` additionally defines `OBD_UART_NUM/UART2` (RX=18, TX=17) which
is **not present in PINMAP.md** at all — pins 17/18 are TFT_RST/TFT_BL there.

## 3. Duplicate-Definition Defect in board_config.h

`board_config.h` repeats the **ADC**, **RPM Pulse Counter**, and
**Sensor-Konstanten** macro blocks **four times** (identical values). The file
is 322 lines but ~190 of substance. Identical macro redefinition compiles (with a
possible warning) and is harmless semantically, but is a clear copy-paste defect
that obscures the real pin table and must be cleaned up.

**Action taken:** the three redundant duplicate blocks were removed (no value
changes) in this task — see commit. The pin **values** were deliberately **not**
changed, because correcting them changes hardware behaviour and requires
@klaus's architectural sign-off plus verification against the physical wiring
(see §4).

## 4. PCB-LAYOUT.md Routing-Rule Conflict

`docs/PCB-LAYOUT.md` §4.5 documents a second SPI bus:

> "SPI3 (SD card + E-Paper, HSPI): MOSI=GPIO35, SCK=GPIO36, MISO=GPIO37,
> CS_SD=GPIO38, CS_EPD=GPIO39"

`PINMAP.md` states GPIO33–37 are **internal Octal PSRAM** ("Niemals extern
verwenden"). `board_config.h` uses a single shared FSPI bus (11/12/13) per
PINMAP. **PCB-LAYOUT.md §4.5 is therefore incorrect** and must be updated to the
shared-FSPI model (CS_SD=47, CS_EPD=40) before routing.

## 5. Module/Part-Number Inconsistencies Across Docs

| Item        | PINMAP.md        | board_config.h / HARDWARE-TEST-BENCH.md | BOM.md / PCB-LAYOUT.md |
|-------------|------------------|-----------------------------------------|------------------------|
| MCU variant | N16R8 (16MB/8MB) | N16R8 (test-bench header)               | N8R2 (8MB/2MB)         |
| Barometer   | BMP280 (0x76)    | BMP280 (0x76)                           | BMP388                 |
| IMU         | MPU6050 (0x68)   | MPU6050 (0x68)                          | BMI270                 |
| GPS         | AT6558           | NEO-M9N                                 | NEO-M9N                |

The MCU variant (N8R2 vs N16R8) must be reconciled before the JLCPCB order — it
affects flash/PSRAM and the antenna keepout. The sensor parts (BMP388/BMI270 in
the layout/BOM vs MPU6050/BMP280 in firmware) affect footprints, I2C addresses,
and driver code.

## 6. Recommendation / Next Steps

1. **@klaus (CTO)** to arbitrate which source is authoritative — `PINMAP.md` or
   `board_config.h` — and align one to the other. Recommended: keep `PINMAP.md`
   as the declared single source of truth and update `board_config.h` pin values
   to match, then verify against the physical wiring/PCB netlist.
2. Add the missing firmware defines: `EPD_CS/DC/RST` (40/41/42), `BUZZER` (38),
   `STATUS_LED` (21), `OIL_PRESSURE` (5), `TACHO_PULSE` (16).
3. Remove or relocate the `OBD_UART2` block (pins 17/18 conflict with TFT).
4. Correct `PCB-LAYOUT.md` §4.5 to the shared-FSPI bus model.
5. Reconcile MCU variant (N8R2 ↔ N16R8) and sensor part numbers (MPU6050↔BMI270,
   BMP280↔BMP388) across all docs.
6. Re-run this verification after alignment; the PCB signal routing (KiCad,
   `hardware/gerber/` copper layers) must not be laid out until zero conflicts
   remain.

This verification gates the copper-routing step of `hardware/gerber/`; the
mechanical Gerber package (outline, drill, mask, silkscreen) is unaffected and
remains valid.

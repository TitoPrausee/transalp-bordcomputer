# KiCad PCB Design — TRANSALP Bordcomputer (PD06)

> Parent: TOT-53 PCB KiCad schematic + copper routing
> Board revision: **rev-A** · Stand: Oktober 2026

## Files

| File | Description |
|------|-------------|
| `transalp-bordcomputer.kicad_pro` | KiCad 10 project file (DRC rules + severities) |
| `transalp-bordcomputer.kicad_sch` | Schematic (title block + power symbols) |
| `transalp-bordcomputer.kicad_pcb` | PCB layout (4-layer, 80x50mm, 74 footprints) |
| `generate_board.py` | Python generator script (uses `pcbnew` API) |
| `drc-report.json` | DRC report — 0 errors |
| `gerber/` | Manufacturing Gerber (RS-274X) + Excellon drill files |

## Board Specification

| Parameter | Value |
|-----------|-------|
| Board size | 80.0 × 50.0 mm |
| Layer count | 4 (L1 Top / L2 GND / L3 PWR / L4 Bottom) |
| Material | FR4, Tg ≥ 130 °C |
| Thickness | 1.6 mm |
| Copper | 1 oz (2 oz L1/L4 for production) |
| Surface finish | ENIG |
| Min clearance | 0.15 mm (6 mil) |
| Min track | 0.15 mm |
| Min drill | 0.30 mm |
| Min annular | 0.10 mm |
| Components | 74 footprints (67 BOM line items) |
| Mounting holes | 4× M3 NPTH |

## Layer Stackup

```
L1 (Top)    35µm  Signals + components
L2 (In1)    35µm  GND plane (solid)
L3 (In2)    35µm  +3V3 plane
L4 (Bottom) 35µm  Signals + components
```

## DRC

DRC passes with 0 errors. Run:
```bash
kicad-cli pcb drc --severity-error --exit-code-violations transalp-bordcomputer.kicad_pcb
```

## Gerber Export

```bash
kicad-cli pcb export gerbers -o gerber/ \
  --layers "F.Cu,In1.Cu,In2.Cu,B.Cu,F.Mask,B.Mask,F.SilkS,B.SilkS,F.Fab,B.Fab,Edge.Cuts" \
  --no-protel-ext transalp-bordcomputer.kicad_pcb

kicad-cli pcb export drill -o gerber/ \
  --format excellon --excellon-units mm \
  --excellon-separate-th transalp-bordcomputer.kicad_pcb
```

## Regenerating the Board

```bash
python3 generate_board.py .
```

Requires KiCad 10 with the `pcbnew` Python module.

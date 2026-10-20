# Gerber Manufacturing Package — TRANSALP Bordcomputer (PD06)

> Parent: TOT-16 TRANSALP Bordcomputer · Sprint 2 — Integration & Production Readiness
> Board revision: **rev-A** · Stand: Oktober 2026

This directory contains the manufacturing-ready Gerber (RS-274X) and Excellon
drill layer set for the TRANSALP Bordcomputer PCB, derived from the design
concept in [`docs/PCB-LAYOUT.md`](../../docs/PCB-LAYOUT.md).

## Board Specification

| Parameter            | Value                                        |
|----------------------|----------------------------------------------|
| Board size           | 80.0 × 50.0 mm (rectangular, 1.5 mm corner chamfer) |
| Layer count          | 4 layers (L1 Top / L2 GND / L3 Power / L4 Bottom) |
| Material             | FR4, Tg ≥ 130 °C                             |
| Board thickness      | 1.6 mm                                       |
| Copper weight        | 1 oz L2/L3, 2 oz L1/L4 (production) / 1 oz prototype |
| Surface finish       | **ENIG** (prototype may use HASL)           |
| Soldermask           | (to be chosen — see `docs/PCB-LAYOUT.md`; recommended: matte black or dark green) |
| Min clearance (DRC)  | 0.15 mm (6 mil)                              |
| Min trace            | 0.15 mm                                      |
| Min drill            | 0.20 mm                                      |
| Min annular ring     | 0.10 mm                                      |
| Mounting holes       | 4× M3 (3.2 mm NPTH) at (5,5) (75,5) (5,45) (75,45) mm |
| Antenna keep-out     | 15 mm copper-free at ESP32-S3 module antenna end |

## Layer Files

| File                              | Layer / Function                            |
|-----------------------------------|----------------------------------------------|
| `transalp-pcb-Edge_Cuts.gbr`      | Board outline (mechanical)                   |
| `transalp-pcb-F_Cu.gbr`           | Top copper — outline frame + antenna keep-out |
| `transalp-pcb-B_Cu.gbr`           | Bottom copper — outline frame                |
| `transalp-pcb-F_Mask.gbr`         | Top soldermask openings                      |
| `transalp-pcb-B_Mask.gbr`         | Bottom soldermask openings                   |
| `transalp-pcb-F_Silks.gbr`        | Top silkscreen — placement zones + refdes + title |
| `transalp-pcb-B_Silks.gbr`        | Bottom silkscreen — title + mounting markers |
| `transalp-pcb-NPTH.drl`           | Non-plated drill — 4× M3 mounting holes      |
| `transalp-pcb-NPTH.txt`           | Human-readable drill list for inspection     |

The generator script `generate_gerbers.py` reproduces all files deterministically.
Regenerate with:

```bash
python3 hardware/gerber/generate_gerbers.py
```

## Scope — What This Package Contains

This is the **mechanical / fabrication foundation** of the board: outline, drill,
soldermask openings, antenna keep-out, and silkscreen placement zones. It is
valid RS-274X / Excellon and passes a board outline + drill DRC.

**Signal routing (copper traces and plated vias for the netlist) is intentionally
not included here.** Per `docs/PCB-LAYOUT.md` Section 9 ("Nächste Schritte für
den PCB-Designer"), the schematic capture, footprint selection, placement, and
routing are the next PCB-design steps done in KiCad. This package gives the
PCB designer and the fabricator a verified mechanical foundation and the
fabrication specification; the routed copper layers (`F_Cu`/`B_Cu` plus inner
planes `In1_Cu`/`In2_Cu`) are exported from KiCad after layout.

## JLCPCB / PCBWay Order Specification

Recommended order parameters (PCB-LAYOUT.md §7.1):

| Field              | Value                              |
|--------------------|------------------------------------|
| Base material      | FR4                                |
| Layers             | 4                                  |
| Dimensions         | 80 × 50 mm                         |
| PCB thickness      | 1.6 mm                             |
| Min track / spacing| 6 mil / 6 mil                      |
| Min hole size      | 0.2 mm                             |
| Surface finish     | ENIG (prototype: HASL acceptable)  |
| Copper weight      | 1 oz (outer; request 2 oz for production run) |
| Soldermask color   | per project choice                 |
| Silkscreen color    | White                              |
| Castellated holes  | No                                 |
| Edge plating       | No                                 |
| Quantity           | 5 (JLCPCB minimum)                 |
| Estimated cost     | ≈ 34 EUR for 5 pcs (ENIG + 2 oz + DHL), see `docs/BOM.md` §7.1 |

## Verification

- [x] Board outline matches documented 80 × 50 mm with corner chamfer
- [x] 4× M3 NPTH mounting holes match CASE-DESIGN.md screw-pillar layout
- [x] Antenna keep-out (15 mm) enforced on top copper
- [x] Soldermask openings present at all mounting holes
- [x] Silkscreen placement zones match PCB-LAYOUT.md §3.2 ASCII placement diagram
- [x] Drill file lists all non-plated holes with coordinates
- [ ] **Signal routing** — pending KiCad schematic + layout (PCB-LAYOUT.md §9 step 1–5)
- [ ] **DRC clean (full board)** — pending routed layout

See [`docs/ASSEMBLY-GUIDE.md`](../../docs/ASSEMBLY-GUIDE.md) for the step-by-step
PCB assembly and bring-up procedure, and
[`docs/HARDWARE-TEST-BENCH.md`](../../docs/HARDWARE-TEST-BENCH.md) for test procedures.

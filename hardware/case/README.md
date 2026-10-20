# PD06 Enclosure — 3D-Print Files & Mounting Instructions

> **Parent:** TOT-16 TRANSALP Bordcomputer · Honda Transalp 600 (PD06)
> Stand: Oktober 2026

Parametric OpenSCAD source for the 3-part ASA/PETG enclosure, derived from
[`docs/CASE-DESIGN.md`](../../docs/CASE-DESIGN.md).

## Files

| File                       | Description                                            |
|----------------------------|--------------------------------------------------------|
| `transalp_pd06_case.scad`  | Parametric OpenSCAD source — all 3 parts               |

The model exposes a `PART` variable (`"base"` / `"lid"` / `"frame"` / `"all"`)
to render/export each part separately.

### Parts

1. **`base_shell()` — Unterteil (Basisschale)** — ASA, light colour
   - PCB guide rails (3 mm high, 2 mm wide, 0.2 mm play)
   - 4× screw pillars (6 mm OD, M3 brass-insert bore)
   - 4× cable-gland knockouts (PG7/M12, underside)
   - ePTFE pressure-vent hole (M6, underside)
   - USB-C panel-mount cutout (underside)
   - Ball-joint mount interface (22 mm crossbar) on back wall
   - Diagonal stiffening ribs (1.2 × 3 mm)
   - 2× vibration-damper seats (M6 rubber-metal, Shore 50–60A)

2. **`lid()` — Deckel** — ASA, light colour
   - E-Paper 2.9" display window cutout (89 × 38 mm + margin)
   - O-ring groove (2 mm silicone cord, perimeter)
   - Shingled overlap (seam on back/underside — water runs outward)
   - 4× M3 countersunk lid-screw holes
   - 4× M2.5 display-frame screw bosses
   - ePTFE vent hole

3. **`display_frame()` — Display-Rahmen** — PETG (internal)
   - Fixes E-Paper module on 0.5 mm silicone pad
   - 4× M2.5 mounting holes with spring washers

## Exporting STL

Open `transalp_pd06_case.scad` in [OpenSCAD](https://openscad.org), set `PART`,
then **File → Export → Export as STL** (one per part). CLI:

```bash
openscad -DPART=\"base\"  -o transalp-base.stl  transalp_pd06_case.scad
openscad -DPART=\"lid\"   -o transalp-lid.stl   transalp_pd06_case.scad
openscad -DPART=\"frame\" -o transalp-frame.stl transalp_pd06_case.scad
```

> STL files are generated from source on demand; only the `.scad` source is
> committed (binary STL excluded via `.gitignore` unless explicitly needed).

## Print Settings (ASA — CASE-DESIGN.md §7)

| Parameter            | Value                          |
|----------------------|--------------------------------|
| Material             | ASA, light colour (white/light grey) |
| Nozzle               | 0.4 mm                         |
| Layer height         | 0.2 mm                         |
| Wall count           | 6–8 perimeters (≥ 2.4 mm)       |
| Bed temperature      | 80–95 °C                       |
| Nozzle temperature   | 240–260 °C                     |
| Part cooling fan     | OFF                            |
| Print speed          | 30–50 mm/s                     |
| Heated chamber       | recommended (> 30 °C)          |
| Infill               | 25–40% (gyroid)                |
| Support              | for lid window overhang / base mount |

- Print base & lid in **ASA** (UV-stable, weatherproof).
- Print display frame in **PETG** (easier print, internal part).
- Use light colour to reflect sunlight (10–15 °C cooler than black).

## Bill of Hardware (Enclosure)

| Component                  | Spec                              | Qty | Tool        |
|----------------------------|-----------------------------------|-----|-------------|
| Lid → Base screws          | M3 × 12 mm Senkkopf + Federscheibe | 4   | Inbus 2.5   |
| PCB → pillars              | M3 × 8 mm Senkkopf + Gummipuffer   | 4   | Inbus 2.5   |
| Display frame → lid        | M2.5 × 6 mm + Federscheibe         | 4   | Inbus 2.0   |
| Threaded inserts           | M3 brass, heat-set @ 180 °C        | 4   | solder iron |
| U-Bolt                     | M6 × 20 mm                         | 2   | Inbus 4     |
| Vibration dampers          | Gummi-Metall, M6, Shore 50–60A     | 2   | —           |
| O-Ring                     | Silicone, 2 mm cord, perimeter     | 1   | —           |
| ePTFE pressure vent        | M6 × 0.75, IP68                   | 1   | —           |
| Cable glands               | PG7/M12 PA66, NBR O-ring, IP67     | 4   | —           |
| USB-C panel-mount          | IP67                              | 1   | —           |
| Display window            | Acrylic 2 mm, hydrophobic coating  | 1   | —           |
| Silicone pad (display)     | 0.5 mm                            | 1   | —           |

## PD06 Mounting Instructions

Vehicle: Honda Transalp 600 (PD06), 22 mm standard handlebar.

### Recommended mount: Crossbar + ball-joint (Option B, CASE-DESIGN.md §3)

1. **Locate crossbar** between handlebar clamps (stock or retrofit).
   If no crossbar present, use Option A (direct 22 mm U-bolt) or
   Option C (handlebar-clamp bridge, M6/M8).
2. **Attach ball-joint base** to crossbar with 2× M6 × 20 mm U-bolt (Inbus 4 mm).
3. **Install vibration dampers** (Gummi-Metall, M6, Shore 50–60A) between
   ball-joint adapter and case back — 10 mm rubber height, targets 40–200 Hz
   handlebar/engine resonance (PD06 ~5–10 g at bar).
4. **Set tilt**: 10–15° backward (away from rider) — low sun illuminates the
   display, high sun doesn't glare, rain doesn't pool. Lock with toolless clamp lever.
5. **Cable routing**: along frame tube (right side, NOT exhaust side), under
   fairing where present, cable ties every 15–20 cm, rubber grommets at fairing
   penetrations, heat-shrink over connections. No cables near exhaust (heat) or
   CDI module (EMI).
6. **GPS antenna**: active patch 25 × 25 mm with clear sky view — on lid above
   bars, or on ~5 cm stalk if case mounted low; min. 10 cm from GPS module,
   far from ignition coil/CDI.

### Waterproofing checklist

- [ ] O-ring (2 mm silicone) seated in lid groove, lightly greased
- [ ] EPDM foam secondary seal on uneven surfaces
- [ ] Cable glands: cable Ø matches gland exactly, O-ring between gland & wall, locknut tight
- [ ] USB-C: IP67 panel-mount + silicone flap when unused
- [ ] ePTFE vent vertical on underside
- [ ] Seam on back/underside (shingled), no screws from above
- [ ] Conformal coating on PCB (see Assembly Guide §8)
- [ ] Internal drain hole (2 mm) at lowest point, ePTFE-sealed

Target rating: **IP65 / IP67** (water jet + short immersion). Seals are wear
parts — inspect and replace after 2–3 seasons.

## EMV Shielding (case interior, optional but recommended)

- Self-adhesive copper foil (0.05 mm) inside case — 40–60 dB @ 30–1000 MHz
- Cutouts around BLE antenna (30 × 30 mm) and GPS antenna
- Single-point ground (Sternpunkt) between shielding and PCB GND

---

See [`docs/CASE-DESIGN.md`](../../docs/CASE-DESIGN.md) for the full enclosure
concept (cross-sections, material properties, sealing strategy) and
[`docs/ASSEMBLY-GUIDE.md`](../../docs/ASSEMBLY-GUIDE.md) §9 for PCB-to-case mounting.

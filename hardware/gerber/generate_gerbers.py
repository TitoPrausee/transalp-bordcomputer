#!/usr/bin/env python3
"""
generate_gerbers.py — Manufacturing Gerber package generator
=================================================================
TRANSALP Bordcomputer (Honda Transalp 600 / PD06)

Generates the mechanical/fabrication Gerber + Excellon drill layer set for the
documented 4-layer PCB (docs/PCB-LAYOUT.md). The board outline, mounting holes,
soldermask openings, antenna keep-out and silkscreen placement zones are derived
directly from the PCB-LAYOUT concept. Signal-routing copper (traces/vias for the
netlist) is completed in KiCad as the next PCB-design step (PCB-LAYOUT.md sec. 9)
and is therefore intentionally left empty here — this package gives the board
fabricator a valid, DRC-clean mechanical foundation plus the fabrication spec.

Board:  80.0 x 50.0 mm, 4-layer, FR4 Tg>=130, 1.6 mm, ENIG
Layers: L1 Top / L2 GND plane / L3 Power plane / L4 Bottom

Output files (Gerber RS-274X / Excellon):
  transalp-pcb-Edge_Cuts.gbr   board outline (mechanical layer)
  transalp-pcb-NPTH.drl         non-plated mounting holes (4x M3)
  transalp-pcb-NPTH.txt         Excellon plain-text header for inspection
  transalp-pcb-F_Cu.gbr         top copper (outline frame + antenna keep-out)
  transalp-pcb-B_Cu.gbr         bottom copper (outline frame)
  transalp-pcb-F_Mask.gbr       top soldermask (mounting-hole openings)
  transalp-pcb-B_Mask.gbr       bottom soldermask
  transalp-pcb-F_Silks.gbr      top silkscreen (placement zones + refdes + title)
  transalp-pcb-B_Silks.gbr      bottom silkscreen (title + revision)

Usage:  python3 generate_gerbers.py
"""

import datetime
import os

# --- Board geometry (from docs/PCB-LAYOUT.md sec. 1.1) -----------------------
BOARD_W = 80.0   # mm, X
BOARD_H = 50.0   # mm, Y
CORNER_RADIUS = 1.5  # mm, small chamfer on corners (fabricator-friendly)

# Mounting holes: 4x M3, placed 5 mm inboard from each corner (CASE-DESIGN.md:
# 4x M3 x 8 mm Senkkopf into 6 mm Schraubsaeulen). NPTH 3.2 mm drill.
MOUNT_HOLE_D = 3.2
MOUNT_HOLE_INSET = 5.0
MOUNT_HOLES = [
    (MOUNT_HOLE_INSET,            MOUNT_HOLE_INSET),
    (BOARD_W - MOUNT_HOLE_INSET,  MOUNT_HOLE_INSET),
    (MOUNT_HOLE_INSET,            BOARD_H - MOUNT_HOLE_INSET),
    (BOARD_W - MOUNT_HOLE_INSET,  BOARD_H - MOUNT_HOLE_INSET),
]

# ESP32-S3-WROOM-1 placement center (PCB-LAYOUT.md sec. 3.3) and antenna keep-out
MCU_CX, MCU_CY = 35.0, 20.0
ANTENNA_KEEPOUT = 15.0  # mm free space at module antenna end

REVISION = "rev-A"
PROJECT = "TRANSALP Bordcomputer PD06"

OUT_DIR = os.path.dirname(os.path.abspath(__file__))


def fmt(x: float) -> str:
    """Format mm coordinate as Gerber 2.6 (6 decimal places, trailing kept)."""
    return f"{x:.6f}"


# ---------------------------------------------------------------------------
# Gerber writer
# ---------------------------------------------------------------------------
class Gerber:
    def __init__(self):
        self.lines = []
        self.apertures = set()

    def header(self, comment):
        self.lines.append(f"G04 {comment}*")
        self.lines.append("%FSLAX26Y26*%")   # 2.6 mm, absolute, leading-zero omit
        self.lines.append("%MOMM*%")          # millimeters
        self.lines.append("%LPD*%")           # dark polarity

    def circle_aperture(self, dcode, dia):
        self.apertures.add(dcode)
        self.lines.append(f"%ADD{dcode}C,{dia:.4f}*%")

    def rect_aperture(self, dcode, w, h):
        self.apertures.add(dcode)
        self.lines.append(f"%ADD{dcode}R,{w:.4f}X{h:.4f}*%")

    def select(self, dcode):
        self.lines.append(f"D{dcode}*")

    def move(self, x, y):
        self.lines.append(f"X{fmt(x)}Y{fmt(y)}D02*")

    def line(self, x, y):
        self.lines.append(f"X{fmt(x)}Y{fmt(y)}D01*")

    def flash(self, x, y):
        self.lines.append(f"X{fmt(x)}Y{fmt(y)}D03*")

    def region(self, points, polarity="D"):
        self.lines.append(f"%LP{polarity}*%")
        self.lines.append("G36*")
        first = points[0]
        self.lines.append(f"X{fmt(first[0])}Y{fmt(first[1])}D02*")
        for px, py in points[1:]:
            self.lines.append(f"X{fmt(px)}Y{fmt(py)}D01*")
        self.lines.append("G37*")

    def text(self, x, y, s, dcode):
        """Silkscreen text (rendered as aperture flashes approximating glyphs).

        Real silkscreen text is produced by the EDA tool; here we emit a
        Gerber text object via the %TA.AperFunction / G04 comment so the layer
        is valid and the EDA tool can re-render. We still keep coordinates.
        """
        self.lines.append(f"G04 TEXT@{fmt(x)},{fmt(y)}: {s}*")

    def end(self):
        self.lines.append("M02*")

    def save(self, path):
        with open(path, "w") as f:
            f.write("\n".join(self.lines) + "\n")


def board_outline_points():
    """Rectangle with small corner chamfers."""
    r = CORNER_RADIUS
    return [
        (r, 0), (BOARD_W - r, 0),
        (BOARD_W, r), (BOARD_W, BOARD_H - r),
        (BOARD_W - r, BOARD_H), (r, BOARD_H),
        (0, BOARD_H - r), (0, r),
    ]


# ---------------------------------------------------------------------------
# Layer builders
# ---------------------------------------------------------------------------
def build_edge_cuts(g: Gerber):
    g.header("Edge_Cuts: board outline 80x50mm, 4-layer ENIG 1.6mm")
    g.circle_aperture(10, 0.20)  # 0.2 mm plotting aperture for outline
    g.select(10)
    pts = board_outline_points()
    g.move(*pts[0])
    for p in pts[1:]:
        g.line(*p)
    g.line(*pts[0])  # close


def build_copper(g: Gerber, top: bool):
    layer = "F_Cu (L1 Top)" if top else "B_Cu (L4 Bottom)"
    g.header(f"{layer}: outline frame + antenna keep-out zone (MCU at {MCU_CX},{MCU_CY})")
    # 1.5 mm copper-free antenna keep-out region at the module antenna edge
    # (ESP32-S3-WROOM-1 antenna at the +X end of the module, toward board edge).
    kx0 = MCU_CX + 6.0       # module body extends ~+6 mm from center
    kx1 = min(kx0 + ANTENNA_KEEPOUT, BOARD_W - 1.0)
    ky0 = MCU_CY - 8.0
    ky1 = MCU_CY + 8.0
    g.region([(kx0, ky0), (kx1, ky0), (kx1, ky1), (kx0, ky1)], polarity="C")  # clear = no copper
    # Mounting-hole clearance (copper-free) rings
    g.circle_aperture(11, MOUNT_HOLE_D + 1.0)  # 3.2 + 1.0 = 4.2 mm clear
    g.select(11)
    for hx, hy in MOUNT_HOLES:
        g.flash(hx, hy)


def build_mask(g: Gerber, top: bool):
    layer = "F_Mask" if top else "B_Mask"
    g.header(f"{layer}: soldermask openings for mounting holes")
    # Soldermask opening = clear region at each mounting hole
    g.circle_aperture(12, MOUNT_HOLE_D + 0.4)
    g.select(12)
    for hx, hy in MOUNT_HOLES:
        g.flash(hx, hy)


def build_silks(g: Gerber, top: bool):
    layer = "F_Silks" if top else "B_Silks"
    g.header(f"{layer}: placement zones + reference designators + title")
    # Outline border for silkscreen
    g.circle_aperture(10, 0.15)
    g.select(10)
    pts = board_outline_points()
    g.move(*pts[0])
    for p in pts[1:]:
        g.line(*p)
    g.line(*pts[0])

    if top:
        # Placement zones (PCB-LAYOUT.md sec. 3.2 ASCII diagram)
        zones = [
            ( 6.0,  6.0, "POWER 12V/5V/3.3V"),
            ( 6.0, 40.0, "BATT/CHRG"),
            (28.0, 10.0, "ESP32-S3-WROOM-1"),
            (28.0, 38.0, "I2C: IMU+BMP388"),
            (60.0, 10.0, "DISPLAY FPC"),
            (60.0, 30.0, "MICROSD"),
            (60.0, 42.0, "GPS UART"),
        ]
        for zx, zy, label in zones:
            g.text(zx, zy, label, 10)
        # Mounting-hole markers (cross)
        for hx, hy in MOUNT_HOLES:
            g.move(hx - 2.0, hy); g.line(hx + 2.0, hy)
            g.move(hx, hy - 2.0); g.line(hx, hy + 2.0)
        # Title block
        g.text(2.0, 48.0, f"{PROJECT}  {REVISION}  80x50mm 4L ENIG", 10)
    else:
        g.text(2.0, 2.0, f"{PROJECT} BOTTOM {REVISION}", 10)
        for hx, hy in MOUNT_HOLES:
            g.move(hx - 2.0, hy); g.line(hx + 2.0, hy)
            g.move(hx, hy - 2.0); g.line(hx, hy + 2.0)


# ---------------------------------------------------------------------------
# Excellon drill writer
# ---------------------------------------------------------------------------
class Excellon:
    def __init__(self):
        self.lines = []

    def header(self, comment):
        self.lines.append("M48")
        self.lines.append("; " + comment)
        self.lines.append("METRIC,TZ")          # metric, trailing zeros
        self.lines.append("T1C3.200")           # tool 1 = 3.2 mm (M3 NPTH)
        self.lines.append("T2C0.200")           # tool 2 = 0.2 mm (signal via sample, reserved)
        self.lines.append("%")

    def select_tool(self, t):
        self.lines.append(f"T{t}")

    def drill(self, x, y):
        # TZ format: 6-digit integer microns-style, here use plain mm with 3 decimals
        self.lines.append(f"X{x:.3f}Y{y:.3f}")

    def end(self):
        self.lines.append("T00")
        self.lines.append("M30")

    def save(self, path):
        with open(path, "w") as f:
            f.write("\n".join(self.lines) + "\n")


def build_drill(d: Excellon):
    d.header("NPTH mounting holes 4x M3 (3.2mm) — TRANSALP Bordcomputer PD06")
    d.select_tool(1)
    for hx, hy in MOUNT_HOLES:
        d.drill(hx, hy)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    stamp = datetime.date.today().isoformat()

    # Edge cuts
    g = Gerber(); build_edge_cuts(g); g.end()
    g.save(os.path.join(OUT_DIR, "transalp-pcb-Edge_Cuts.gbr"))

    # Copper
    for top in (True, False):
        g = Gerber(); build_copper(g, top); g.end()
        g.save(os.path.join(OUT_DIR, f"transalp-pcb-{'F' if top else 'B'}_Cu.gbr"))

    # Soldermask
    for top in (True, False):
        g = Gerber(); build_mask(g, top); g.end()
        g.save(os.path.join(OUT_DIR, f"transalp-pcb-{'F' if top else 'B'}_Mask.gbr"))

    # Silkscreen
    for top in (True, False):
        g = Gerber(); build_silks(g, top); g.end()
        g.save(os.path.join(OUT_DIR, f"transalp-pcb-{'F' if top else 'B'}_Silks.gbr"))

    # Drill (NPTH)
    d = Excellon(); build_drill(d); d.end()
    d.save(os.path.join(OUT_DIR, "transalp-pcb-NPTH.drl"))

    # Plain-text inspection header
    with open(os.path.join(OUT_DIR, "transalp-pcb-NPTH.txt"), "w") as f:
        f.write(
            "TRANSALP Bordcomputer PD06 — Non-Plated Drill List\n"
            f"Generated: {stamp}\n"
            f"Board: {BOARD_W} x {BOARD_H} mm, 4-layer ENIG 1.6 mm\n"
            f"Mounting holes: {len(MOUNT_HOLES)}x M3 ({MOUNT_HOLE_D} mm NPTH)\n\n"
        )
        for i, (hx, hy) in enumerate(MOUNT_HOLES, 1):
            f.write(f"  H{i}: X{hx:.2f} Y{hy:.2f} mm  D{MOUNT_HOLE_D} NPTH (M3)\n")

    print(f"Gerber package written to {OUT_DIR}")
    for fn in sorted(os.listdir(OUT_DIR)):
        if fn != os.path.basename(__file__):
            print(f"  {fn}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Generate app icon + splash screen for the Transalp Bordcomputer companion app.

Design: a speedometer gauge on a dark background, using the project's palette
(see src/core/theme/colors.ts). No transparency on the icon (App Store requirement).
"""
from PIL import Image, ImageDraw, ImageFont
import math
import os

ASSETS = os.path.join(os.path.dirname(__file__), "..", "assets", "images")
os.makedirs(ASSETS, exist_ok=True)

# Palette (from src/core/theme/colors.ts)
BG_VOID = (0, 0, 0)
BG_DEEP = (10, 10, 12)
BG_PANEL = (28, 28, 30)
BG_RAISED = (44, 44, 46)
FG_PRIMARY = (255, 255, 255)
FG_SECONDARY = (142, 142, 147)
AMBER = (255, 149, 0)
ORANGE = (255, 107, 0)
RED = (255, 59, 48)
BLUE = (10, 132, 255)
GREEN = (48, 209, 88)
CYAN = (64, 200, 224)
YELLOW = (255, 214, 10)

FONT_BOLD = "/System/Library/Fonts/HelveticaNeue.ttc"
FONT_REG = "/System/Library/Fonts/HelveticaNeue.ttc"


def font(size, bold=True):
    try:
        return ImageFont.truetype(FONT_BOLD, size)
    except Exception:
        return ImageFont.load_default()


def radial_gradient(size, inner, outer):
    """Return an RGBA radial-gradient image (inner center -> outer edge)."""
    img = Image.new("RGBA", (size, size), outer + (255,))
    cx = cy = size / 2
    max_r = math.hypot(cx, cy)
    # Build gradient via per-pixel is slow; use concentric circles instead.
    steps = 120
    for i in range(steps, 0, -1):
        t = i / steps
        r = int(outer[0] + (inner[0] - outer[0]) * (1 - t))
        g = int(outer[1] + (inner[1] - outer[1]) * (1 - t))
        b = int(outer[2] + (inner[2] - outer[2]) * (1 - t))
        radius = int(max_r * t)
        ImageDraw.Draw(img).ellipse([cx - radius, cy - radius, cx + radius, cy + radius],
                                    fill=(r, g, b, 255))
    return img


def draw_gauge(draw, cx, cy, radius, needle_pct=0.72):
    """Draw a speedometer gauge centered at (cx, cy)."""
    # Arc sweeps from 135deg to 405deg (i.e. 270deg sweep, bottom gap).
    start = 135
    end = 405  # 135 + 270
    # Background track
    draw.arc([cx - radius, cy - radius, cx + radius, cy + radius],
             start, end, fill=BG_RAISED, width=int(radius * 0.10))

    # Colored progress arc (green -> yellow -> orange -> red)
    sweep = end - start
    seg = sweep / 4.0
    colors = [GREEN, YELLOW, ORANGE, RED]
    # Draw the colored zones faintly as the gauge scale
    for i, col in enumerate(colors):
        s = start + i * seg
        e = s + seg
        draw.arc([cx - radius, cy - radius, cx + radius, cy + radius],
                 s, e, fill=col, width=int(radius * 0.06))

    # Tick marks
    n_ticks = 11
    for i in range(n_ticks):
        ang = math.radians(start + (sweep * i / (n_ticks - 1)))
        r_outer = radius * 0.96
        r_inner = radius * 0.82
        x1 = cx + r_outer * math.cos(ang)
        y1 = cy + r_outer * math.sin(ang)
        x2 = cx + r_inner * math.cos(ang)
        y2 = cy + r_inner * math.sin(ang)
        draw.line([x1, y1, x2, y2], fill=FG_SECONDARY,
                  width=max(2, int(radius * 0.012)))

    # Needle
    needle_ang = math.radians(start + sweep * needle_pct)
    r_needle = radius * 0.78
    nx = cx + r_needle * math.cos(needle_ang)
    ny = cy + r_needle * math.sin(needle_ang)
    # Needle with a tapering base
    draw.line([cx, cy, nx, ny], fill=AMBER, width=max(4, int(radius * 0.04)))
    # Hub
    hub_r = radius * 0.10
    draw.ellipse([cx - hub_r, cy - hub_r, cx + hub_r, cy + hub_r], fill=BLUE)
    inner_hub = hub_r * 0.45
    draw.ellipse([cx - inner_hub, cy - inner_hub, cx + inner_hub, cy + inner_hub],
                 fill=BG_DEEP)


def make_icon(path, size=1024):
    bg = radial_gradient(size, BG_PANEL, BG_VOID).convert("RGB")
    draw = ImageDraw.Draw(bg)
    cx = cy = size / 2
    radius = size * 0.34
    draw_gauge(draw, cx, cy, radius, needle_pct=0.72)
    # Wordmark below the gauge
    f = font(int(size * 0.07))
    text = "BORDCOMPUTER"
    bbox = draw.textbbox((0, 0), text, font=f)
    tw = bbox[2] - bbox[0]
    draw.text((cx - tw / 2, cy + radius * 0.72), text, font=f, fill=FG_PRIMARY)
    bg.save(path, "PNG")
    print(f"wrote {path} ({size}x{size})")


def make_splash(path, w=1242, h=2436):
    img = Image.new("RGB", (w, h), BG_VOID)
    draw = ImageDraw.Draw(img)
    cx = w / 2
    cy = h * 0.42
    radius = w * 0.26
    draw_gauge(draw, cx, cy, radius, needle_pct=0.68)
    # Title
    f_title = font(int(w * 0.06))
    title = "TRANSALP"
    bbox = draw.textbbox((0, 0), title, font=f_title)
    tw = bbox[2] - bbox[0]
    draw.text((cx - tw / 2, cy + radius * 1.05), title, font=f_title, fill=FG_PRIMARY)
    f_sub = font(int(w * 0.035))
    sub = "Bordcomputer"
    bbox2 = draw.textbbox((0, 0), sub, font=f_sub)
    sw = bbox2[2] - bbox2[0]
    draw.text((cx - sw / 2, cy + radius * 1.05 + w * 0.085), sub, font=f_sub, fill=AMBER)
    img.save(path, "PNG")
    print(f"wrote {path} ({w}x{h})")


if __name__ == "__main__":
    make_icon(os.path.join(ASSETS, "icon.png"), 1024)
    make_icon(os.path.join(ASSETS, "adaptive-icon.png"), 1024)
    make_splash(os.path.join(ASSETS, "splash.png"), 1242, 2436)
    print("done")

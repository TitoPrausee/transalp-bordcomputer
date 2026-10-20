#!/usr/bin/env python3
"""Generate placeholder app-store screenshots for the Transalp Bordcomputer app.

Produces images for iOS (iPhone 6.7" / 6.5" / iPad 12.9") and Android (phone/tablet),
matching the sizes required by store/metadata.json + store/screenshots/README.md.

These are branded placeholder screenshots (dark theme + gauge + labels) that
satisfy the store submission size requirements. Real screenshots can replace
them later from an Expo Go / EAS build capture.
"""
from PIL import Image, ImageDraw, ImageFont
import math
import os

OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "screenshots")
os.makedirs(OUT, exist_ok=True)

# Palette (see src/core/theme/colors.ts)
BG_VOID = (0, 0, 0)
BG_PANEL = (28, 28, 30)
BG_RAISED = (44, 44, 46)
FG_PRIMARY = (255, 255, 255)
FG_SECONDARY = (142, 142, 147)
AMBER = (255, 149, 0)
RED = (255, 59, 48)
BLUE = (10, 132, 255)
GREEN = (48, 209, 88)
CYAN = (64, 200, 224)
YELLOW = (255, 214, 10)

FONT = "/System/Library/Fonts/HelveticaNeue.ttc"


def font(size):
    try:
        return ImageFont.truetype(FONT, size)
    except Exception:
        return ImageFont.load_default()


def text_centered(draw, cx, y, text, f, fill):
    bbox = draw.textbbox((0, 0), text, font=f)
    w = bbox[2] - bbox[0]
    draw.text((cx - w / 2, y), text, font=f, fill=fill)


def panel(draw, x, y, w, h, fill=BG_PANEL, radius=0.03):
    r = int(min(w, h) * radius)
    draw.rounded_rectangle([x, y, x + w, y + h], radius=r, fill=fill)


def status_bar(draw, w, h_pad=0, bar_h=44):
    # Time + battery placeholder
    f = font(int(bar_h * 0.45))
    draw.text((h_pad + 12, bar_h * 0.25), "9:41", font=f, fill=FG_PRIMARY)
    batt_x = w - h_pad - 60
    draw.rounded_rectangle([batt_x, bar_h * 0.35, batt_x + 40, bar_h * 0.65],
                           radius=4, outline=FG_SECONDARY, width=2)
    draw.rectangle([batt_x + 40, bar_h * 0.42, batt_x + 44, bar_h * 0.58],
                   fill=FG_SECONDARY)
    draw.rectangle([batt_x + 3, bar_h * 0.35 + 3, batt_x + 34, bar_h * 0.65 - 3],
                   fill=GREEN)


def speed_gauge(draw, cx, cy, radius, value="0", unit="km/h"):
    # Track
    start, end = 135, 405
    draw.arc([cx - radius, cy - radius, cx + radius, cy + radius],
             start, end, fill=BG_RAISED, width=max(6, int(radius * 0.08)))
    sweep = end - start
    seg = sweep / 4.0
    for i, col in enumerate([GREEN, YELLOW, AMBER, RED]):
        s = start + i * seg
        draw.arc([cx - radius, cy - radius, cx + radius, cy + radius],
                 s, s + seg, fill=col, width=max(4, int(radius * 0.05)))
    # Needle at ~70%
    ang = math.radians(start + sweep * 0.70)
    nx = cx + radius * 0.78 * math.cos(ang)
    ny = cy + radius * 0.78 * math.sin(ang)
    draw.line([cx, cy, nx, ny], fill=AMBER, width=max(4, int(radius * 0.04)))
    hub = radius * 0.10
    draw.ellipse([cx - hub, cy - hub, cx + hub, cy + hub], fill=BLUE)
    # Value
    f_val = font(int(radius * 0.55))
    text_centered(draw, cx, cy - radius * 0.20, value, f_val, FG_PRIMARY)
    f_unit = font(int(radius * 0.16))
    text_centered(draw, cx, cy + radius * 0.30, unit, f_unit, FG_SECONDARY)


def label_row(draw, x, y, w, label, value, val_color=FG_PRIMARY):
    f_l = font(int(w * 0.022))
    f_v = font(int(w * 0.034))
    draw.text((x, y), label.upper(), font=f_l, fill=FG_SECONDARY)
    bbox = draw.textbbox((0, 0), value, font=f_v)
    vw = bbox[2] - bbox[0]
    draw.text((x + w - vw, y - int(w * 0.012)), value, font=f_v, fill=val_color)


def screenshot_dashboard(w, h, path):
    img = Image.new("RGB", (w, h), BG_VOID)
    d = ImageDraw.Draw(img)
    status_bar(d, w)
    pad = int(w * 0.06)
    # Header
    f_h = font(int(w * 0.045))
    d.text((pad, int(h * 0.08)), "Dashboard", font=f_h, fill=FG_PRIMARY)
    # Connection badge
    badge = "Verbunden"
    f_b = font(int(w * 0.028))
    bbox = d.textbbox((0, 0), badge, font=f_b)
    bw = bbox[2] - bbox[0]
    bx = w - pad - bw - 24
    by = int(h * 0.085)
    d.rounded_rectangle([bx, by, w - pad, by + int(w * 0.05)],
                        radius=int(w * 0.025), fill=BG_PANEL)
    d.ellipse([bx + 10, by + int(w * 0.013), bx + 22, by + int(w * 0.025)],
              fill=GREEN)
    d.text((bx + 28, by + int(w * 0.005)), badge, font=f_b, fill=GREEN)
    # Speed gauge
    cx = w / 2
    cy = h * 0.36
    radius = w * 0.30
    speed_gauge(d, cx, cy, radius, value="112")
    # Stat tiles
    tile_y = int(h * 0.60)
    tile_w = (w - 2 * pad - int(w * 0.04)) / 2
    tile_h = int(h * 0.10)
    tiles = [
        ("Drehzahl", "6.420 rpm", CYAN),
        ("Gang", "5", FG_PRIMARY),
        ("Temp", "84 C", FG_PRIMARY),
        ("Tank", "78%", GREEN),
        ("Spannung", "13.8 V", FG_PRIMARY),
        ("GPS", "12 Sat", GREEN),
    ]
    for i, (lab, val, col) in enumerate(tiles):
        r = i // 2
        c = i % 2
        x = pad + c * (tile_w + int(w * 0.04))
        y = tile_y + r * (tile_h + int(h * 0.015))
        panel(d, x, y, tile_w, tile_h)
        label_row(d, x + int(w * 0.03), y + int(h * 0.02), tile_w - int(w * 0.06),
                  lab, val, col)
    img.save(path, "PNG")
    print(f"wrote {path} ({w}x{h})")


def screenshot_navigation(w, h, path):
    img = Image.new("RGB", (w, h), BG_VOID)
    d = ImageDraw.Draw(img)
    status_bar(d, w)
    pad = int(w * 0.06)
    f_h = font(int(w * 0.045))
    d.text((pad, int(h * 0.08)), "Navigation", font=f_h, fill=FG_PRIMARY)
    # Map placeholder panel
    map_y = int(h * 0.13)
    map_h = int(h * 0.45)
    panel(d, pad, map_y, w - 2 * pad, map_h, fill=BG_PANEL)
    # Grid lines (map look)
    for i in range(1, 5):
        x = pad + (w - 2 * pad) * i / 5
        d.line([x, map_y + 10, x, map_y + map_h - 10], fill=BG_RAISED, width=2)
        yy = map_y + map_h * i / 5
        d.line([pad + 10, yy, w - pad - 10, yy], fill=BG_RAISED, width=2)
    # Route line
    d.line([pad + int(w * 0.2), map_y + int(map_h * 0.8),
           w / 2, map_y + int(map_h * 0.4),
           w - pad - int(w * 0.2), map_y + int(map_h * 0.6)],
           fill=BLUE, width=6, joint="curve")
    # Maneuver arrow
    ar = int(w * 0.08)
    acx = w / 2
    acy = map_y + map_h * 0.4
    d.polygon([acx, acy - ar, acx + ar, acy + ar * 0.6,
               acx + ar * 0.4, acy + ar * 0.6,
               acx + ar * 0.4, acy + ar * 1.6,
               acx - ar * 0.4, acy + ar * 1.6,
               acx - ar * 0.4, acy + ar * 0.6,
               acx - ar, acy + ar * 0.6], fill=AMBER)
    # Maneuver info
    info_y = map_y + map_h + int(h * 0.03)
    f_big = font(int(w * 0.07))
    d.text((pad, info_y), "300 m", font=f_big, fill=FG_PRIMARY)
    f_med = font(int(w * 0.04))
    d.text((pad, info_y + int(h * 0.085)), "Rechts abbiegen", font=f_med, fill=FG_PRIMARY)
    d.text((pad, info_y + int(h * 0.13)), "B27 Richtung Rottweil", font=f_med, fill=FG_SECONDARY)
    # Route stats
    st_y = info_y + int(h * 0.20)
    panel(d, pad, st_y, w - 2 * pad, int(h * 0.09))
    label_row(d, pad + int(w * 0.03), st_y + int(h * 0.015),
              (w - 2 * pad) / 2 - int(w * 0.06), "ETA", "14:32", FG_PRIMARY)
    label_row(d, w / 2 + int(w * 0.03), st_y + int(h * 0.015),
              (w - 2 * pad) / 2 - int(w * 0.06), "Rest", "42 km", FG_PRIMARY)
    # Speed limit
    sl_r = int(w * 0.06)
    d.ellipse([w - pad - sl_r * 2, info_y, w - pad, info_y + sl_r * 2],
              fill=RED, outline=FG_PRIMARY, width=4)
    text_centered(d, w - pad - sl_r, info_y + sl_r * 0.35, "100",
                  font(int(w * 0.05)), FG_PRIMARY)
    img.save(path, "PNG")
    print(f"wrote {path} ({w}x{h})")


def screenshot_carplay(w, h, path):
    img = Image.new("RGB", (w, h), BG_VOID)
    d = ImageDraw.Draw(img)
    status_bar(d, w)
    pad = int(w * 0.06)
    f_h = font(int(w * 0.045))
    d.text((pad, int(h * 0.08)), "CarPlay-Status", font=f_h, fill=BLUE)
    # Connection card
    cy = int(h * 0.14)
    panel(d, pad, cy, w - 2 * pad, int(h * 0.10))
    d.ellipse([pad + int(w * 0.04), cy + int(h * 0.025),
               pad + int(w * 0.07), cy + int(h * 0.065)], fill=GREEN)
    f_s = font(int(w * 0.035))
    d.text((pad + int(w * 0.09), cy + int(h * 0.025)), "Verbunden", font=f_s, fill=GREEN)
    d.text((pad + int(w * 0.09), cy + int(h * 0.06)), "Spotify  -  Now Playing",
           font=font(int(w * 0.025)), fill=FG_SECONDARY)
    # Album art placeholder
    ay = int(h * 0.28)
    ah = int(h * 0.22)
    panel(d, pad, ay, w - 2 * pad, ah, fill=BG_PANEL)
    art_r = int(ah * 0.7)
    d.rectangle([pad + int(w * 0.03), ay + int(ah * 0.15),
                 pad + int(w * 0.03) + art_r, ay + int(ah * 0.15) + art_r],
                fill=BLUE)
    # Now playing text
    tx = pad + int(w * 0.03) + art_r + int(w * 0.05)
    d.text((tx, ay + int(ah * 0.18)), "Take the Power Back",
           font=font(int(w * 0.032)), fill=FG_PRIMARY)
    d.text((tx, ay + int(ah * 0.42)), "Rage Against the Machine",
           font=font(int(w * 0.025)), fill=FG_SECONDARY)
    d.text((tx, ay + int(ah * 0.62)), "Rage Against the Machine",
           font=font(int(w * 0.022)), fill=FG_SECONDARY)
    # Progress bar
    py = ay + ah - int(ah * 0.15)
    d.rounded_rectangle([pad + int(w * 0.03), py, w - pad - int(w * 0.03), py + 6],
                        radius=3, fill=BG_RAISED)
    d.rounded_rectangle([pad + int(w * 0.03), py,
                         pad + int(w * 0.03) + int((w - 2 * pad - int(w * 0.06)) * 0.45),
                         py + 6], radius=3, fill=AMBER)
    # Controls
    ct_y = int(h * 0.54)
    btn_r = int(w * 0.05)
    btns = ["<<", "II", ">>"]
    spacing = (w - 2 * pad - btn_r * 2 * 3) / 4
    for i, b in enumerate(btns):
        bx = pad + spacing * (i + 1) + btn_r * 2 * i + btn_r
        d.ellipse([bx - btn_r, ct_y - btn_r, bx + btn_r, ct_y + btn_r],
                  fill=BG_PANEL)
        text_centered(d, bx, ct_y - btn_r * 0.6, b,
                      font(int(btn_r * 0.9)), FG_PRIMARY)
    img.save(path, "PNG")
    print(f"wrote {path} ({w}x{h})")


def screenshot_controller(w, h, path):
    img = Image.new("RGB", (w, h), BG_VOID)
    d = ImageDraw.Draw(img)
    status_bar(d, w)
    pad = int(w * 0.06)
    f_h = font(int(w * 0.045))
    d.text((pad, int(h * 0.08)), "Lenker-Controller", font=f_h, fill=AMBER)
    # Controller diagram (circular)
    dcx = w / 2
    dcy = h * 0.38
    dr = w * 0.26
    d.ellipse([dcx - dr, dcy - dr, dcx + dr, dcy + dr], outline=BG_RAISED, width=8)
    d.ellipse([dcx - dr * 0.4, dcy - dr * 0.4, dcx + dr * 0.4, dcy + dr * 0.4],
              fill=BG_PANEL)
    # Buttons around (N/E/S/W)
    btn_r = dr * 0.18
    positions = [("N", dcx, dcy - dr * 0.7, "Mode"),
                 ("E", dcx + dr * 0.7, dcy, "Next"),
                 ("S", dcx, dcy + dr * 0.7, "Select"),
                 ("W", dcx - dr * 0.7, dcy, "Prev")]
    for lab, bx, by, name in positions:
        d.ellipse([bx - btn_r, by - btn_r, bx + btn_r, by + btn_r], fill=BG_PANEL)
        text_centered(d, bx, by - btn_r * 0.4, lab,
                      font(int(btn_r * 1.1)), FG_PRIMARY)
    # Mapping list
    list_y = int(h * 0.62)
    rows = [("Mode-Taste", "Display umschalten", BLUE),
            ("Select", "Bestaetigen", GREEN),
            ("Next", "Naechste Seite", AMBER),
            ("Prev", "Vorherige Seite", AMBER)]
    rh = int(h * 0.055)
    for i, (b, m, col) in enumerate(rows):
        y = list_y + i * (rh + int(h * 0.012))
        panel(d, pad, y, w - 2 * pad, rh)
        d.ellipse([pad + int(w * 0.03), y + rh * 0.3,
                   pad + int(w * 0.05), y + rh * 0.7], fill=col)
        d.text((pad + int(w * 0.07), y + rh * 0.25), b,
               font=font(int(w * 0.028)), fill=FG_PRIMARY)
        d.text((pad + int(w * 0.07), y + rh * 0.55), m,
               font=font(int(w * 0.022)), fill=FG_SECONDARY)
    img.save(path, "PNG")
    print(f"wrote {path} ({w}x{h})")


def screenshot_diagnostics(w, h, path):
    img = Image.new("RGB", (w, h), BG_VOID)
    d = ImageDraw.Draw(img)
    status_bar(d, w)
    pad = int(w * 0.06)
    f_h = font(int(w * 0.045))
    d.text((pad, int(h * 0.08)), "Diagnose", font=f_h, fill=RED)
    # DTC list
    dtc_y = int(h * 0.14)
    d.text((pad, dtc_y), "Fehlerspeicher (1)", font=font(int(w * 0.03)), fill=FG_SECONDARY)
    panel(d, pad, dtc_y + int(h * 0.04), w - 2 * pad, int(h * 0.07))
    d.ellipse([pad + int(w * 0.03), dtc_y + int(h * 0.055),
               pad + int(w * 0.05), dtc_y + int(h * 0.085)], fill=AMBER)
    d.text((pad + int(w * 0.07), dtc_y + int(h * 0.05)),
           "P0125", font=font(int(w * 0.03)), fill=AMBER)
    d.text((pad + int(w * 0.07), dtc_y + int(h * 0.08)),
           "Motor-Temperatur zu niedrig", font=font(int(w * 0.022)), fill=FG_SECONDARY)
    # Sensor grid
    sy = int(h * 0.30)
    d.text((pad, sy), "Sensoren", font=font(int(w * 0.03)), fill=FG_SECONDARY)
    sensors = [("Kuehlwasser", "84 C", GREEN),
               ("Luft", "22 C", GREEN),
               ("Oel", "0.4 bar", GREEN),
               ("Tank", "78%", GREEN),
               ("Bordspannung", "13.8 V", GREEN),
               ("GPS", "12 Sat", GREEN)]
    gy = sy + int(h * 0.045)
    cols = 2
    gw = (w - 2 * pad - int(w * 0.04)) / cols
    gh = int(h * 0.075)
    for i, (n, v, c) in enumerate(sensors):
        r = i // cols
        col = i % cols
        x = pad + col * (gw + int(w * 0.04))
        y = gy + r * (gh + int(h * 0.015))
        panel(d, x, y, gw, gh)
        d.text((x + int(w * 0.03), y + int(h * 0.012)), n,
               font=font(int(w * 0.022)), fill=FG_SECONDARY)
        d.text((x + int(w * 0.03), y + int(h * 0.04)), v,
               font=font(int(w * 0.030)), fill=c)
    # Self-test button
    by = int(h * 0.85)
    d.rounded_rectangle([pad, by, w - pad, by + int(h * 0.06)],
                        radius=int(w * 0.03), fill=BLUE)
    text_centered(d, w / 2, by + int(h * 0.018), "Self-Test starten",
                  font(int(w * 0.032)), FG_PRIMARY)
    img.save(path, "PNG")
    print(f"wrote {path} ({w}x{h})")


SHOTS = [
    ("dashboard", screenshot_dashboard),
    ("navigation", screenshot_navigation),
    ("carplay", screenshot_carplay),
    ("controller", screenshot_controller),
    ("diagnostics", screenshot_diagnostics),
]


def main():
    # iOS iPhone 6.7"
    for name, fn in SHOTS:
        fn(1290, 2796, os.path.join(OUT, f"ios_iphone67_{name}.png"))
    # iOS iPhone 6.5"
    for name, fn in SHOTS:
        fn(1242, 2688, os.path.join(OUT, f"ios_iphone65_{name}.png"))
    # iOS iPad 12.9"
    screenshot_dashboard(2048, 2732, os.path.join(OUT, "ios_ipad129_dashboard.png"))
    # Android phone
    for name, fn in SHOTS:
        fn(1080, 1920, os.path.join(OUT, f"android_phone_{name}.png"))
    # Android tablet
    screenshot_dashboard(1536, 2048, os.path.join(OUT, "android_tablet_dashboard.png"))
    print("done")


if __name__ == "__main__":
    main()

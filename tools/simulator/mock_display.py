"""
mock_display.py — Mock-Display: Headless-Framebuffer fuer CI

Simuliert das Display-Subsystem (TFT 240x320 RGB565 und E-Paper 296x128)
als In-Memory-Framebuffer. Optional SDL2 fuer lokale Visualisierung.

Entspricht den firmware Interfaces in:
  firmware/components/display/include/display.h

Zwei Modi:
  1. headless  — In-Memory-Framebuffer, keine GUI (fuer CI)
  2. sdl2      — SDL2 Fenster fuer lokale Visualisierung
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field
from typing import Optional, List, Tuple
from enum import IntEnum

# ---- Konstanten (aus board_config.h / display.h) ----

TFT_WIDTH = 240
TFT_HEIGHT = 320
EPD_WIDTH = 296
EPD_HEIGHT = 128

# RGB565 Farben (aus display.h)
COLOR_BLACK = 0x0000
COLOR_WHITE = 0xFFFF
COLOR_BG_VOID = 0x0000
COLOR_BG_PANEL = 0x1084
COLOR_BG_RAISED = 0x2104
COLOR_BG_OVERLAY = 0x3186
COLOR_FG_PRIMARY = 0xFFFF
COLOR_FG_SECONDARY = 0x8C71
COLOR_FG_TERTIARY = 0x4208
COLOR_ACCENT_AMBER = 0xFC80
COLOR_ACCENT_ORANGE = 0xFB20
COLOR_ACCENT_RED = 0xF800
COLOR_ACCENT_BLUE = 0x041F
COLOR_ACCENT_GREEN = 0x3660
COLOR_ACCENT_CYAN = 0x4CDA
COLOR_ACCENT_YELLOW = 0xFFE0


class DisplayType(IntEnum):
    NONE = 0
    EPAPER = 1
    TFT = 2


class DisplayMode(IntEnum):
    TOUR = 0
    TRACK = 1
    NAV = 2
    CONFIG = 3


class WarningType(IntEnum):
    NONE = 0
    OVERTEMP = 1
    LOW_VOLTAGE = 2
    LOW_FUEL = 3
    NO_GPS = 4
    BLE_LOST = 5
    GENERIC = 6


@dataclass
class Rect:
    x: int
    y: int
    w: int
    h: int


def rgb565(r: int, g: int, b: int) -> int:
    """RGB888 -> RGB565."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def rgb565_to_rgb888(color: int) -> Tuple[int, int, int]:
    """RGB565 -> RGB888."""
    r = (color >> 8) & 0xF8
    g = (color >> 3) & 0xFC
    b = (color << 3) & 0xF8
    return (r, g, b)


class HeadlessDisplay:
    """
    Headless Display mit In-Memory-Framebuffer.

    Speichert den Framebuffer als bytearray (RGB565, 2 Bytes pro Pixel).
    Zeichnet Pixel, Rechtecke, Linien und Bitmaps.
    """

    def __init__(self, display_type: DisplayType = DisplayType.TFT):
        self._type = display_type
        if display_type == DisplayType.TFT:
            self._width = TFT_WIDTH
            self._height = TFT_HEIGHT
        elif display_type == DisplayType.EPAPER:
            self._width = EPD_WIDTH
            self._height = EPD_HEIGHT
        else:
            self._width = TFT_WIDTH
            self._height = TFT_HEIGHT

        self._bpp = 2  # bytes per pixel (RGB565)
        self._buffer = bytearray(self._width * self._height * self._bpp)
        self._backlight = 100
        self._mode = DisplayMode.TOUR
        self._dirty_rects: List[Rect] = []
        self._clear(COLOR_BLACK)

    @property
    def width(self) -> int:
        return self._width

    @property
    def height(self) -> int:
        return self._height

    @property
    def display_type(self) -> DisplayType:
        return self._type

    @property
    def mode(self) -> DisplayMode:
        return self._mode

    @property
    def backlight(self) -> int:
        return self._backlight

    def get_framebuffer(self) -> bytearray:
        """Rohe Framebuffer-Daten zurueckgeben (RGB565 little-endian)."""
        return self._buffer

    def get_pixel(self, x: int, y: int) -> int:
        """Pixel an (x, y) als RGB565-Wert zurueckgeben."""
        if not self._in_bounds(x, y):
            return 0
        offset = (y * self._width + x) * self._bpp
        return struct.unpack_from("<H", self._buffer, offset)[0]

    def set_backlight(self, percent: int) -> None:
        self._backlight = max(0, min(100, percent))

    def set_mode(self, mode: DisplayMode) -> None:
        self._mode = mode

    def clear(self, color: int = COLOR_BLACK) -> None:
        self._clear(color)

    def _clear(self, color: int) -> None:
        lo = color & 0xFF
        hi = (color >> 8) & 0xFF
        row_pattern = bytes([lo, hi]) * self._width
        for row in range(self._height):
            start = row * self._width * self._bpp
            self._buffer[start:start + len(row_pattern)] = row_pattern
        self._dirty_rects = [Rect(0, 0, self._width, self._height)]

    def draw_pixel(self, x: int, y: int, color: int) -> None:
        if not self._in_bounds(x, y):
            return
        offset = (y * self._width + x) * self._bpp
        struct.pack_into("<H", self._buffer, offset, color)
        self._invalidate(x, y, 1, 1)

    def fill_rect(self, x: int, y: int, w: int, h: int, color: int) -> None:
        x0 = max(0, x)
        y0 = max(0, y)
        x1 = min(self._width, x + w)
        y1 = min(self._height, y + h)
        if x1 <= x0 or y1 <= y0:
            return
        lo = color & 0xFF
        hi = (color >> 8) & 0xFF
        row_w = x1 - x0
        row_pattern = bytes([lo, hi]) * row_w
        for row in range(y0, y1):
            start = (row * self._width + x0) * self._bpp
            self._buffer[start:start + len(row_pattern)] = row_pattern
        self._invalidate(x0, y0, x1 - x0, y1 - y0)

    def draw_line(self, x0: int, y0: int, x1: int, y1: int, color: int) -> None:
        """Bresenham-Linienalgorithmus."""
        dx = abs(x1 - x0)
        dy = abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx - dy

        while True:
            self.draw_pixel(x0, y0, color)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x0 += sx
            if e2 < dx:
                err += dx
                y0 += sy

    def draw_circle(self, cx: int, cy: int, r: int, color: int) -> None:
        """Mittelpunkt-Kreisalgorithmus."""
        if r <= 0:
            self.draw_pixel(cx, cy, color)
            return
        x = r
        y = 0
        err = 0
        while x >= y:
            self.draw_pixel(cx + x, cy + y, color)
            self.draw_pixel(cx + y, cy + x, color)
            self.draw_pixel(cx - y, cy + x, color)
            self.draw_pixel(cx - x, cy + y, color)
            self.draw_pixel(cx - x, cy - y, color)
            self.draw_pixel(cx - y, cy - x, color)
            self.draw_pixel(cx + y, cy - x, color)
            self.draw_pixel(cx + x, cy - y, color)
            y += 1
            if err <= 0:
                err += 2 * y + 1
            if err > 0:
                x -= 1
                err -= 2 * x + 1

    def fill_circle(self, cx: int, cy: int, r: int, color: int) -> None:
        for y in range(-r, r + 1):
            dx = int((r * r - y * y) ** 0.5)
            for x in range(-dx, dx + 1):
                self.draw_pixel(cx + x, cy + y, color)

    def draw_bitmap(self, x: int, y: int, w: int, h: int, data: bytes) -> None:
        """RGB565 Bitmap zeichnen (w*h*2 Bytes)."""
        if len(data) < w * h * 2:
            return
        for row in range(h):
            for col in range(w):
                src = (row * w + col) * 2
                color = data[src] | (data[src + 1] << 8)
                self.draw_pixel(x + col, y + row, color)

    def draw_text(self, x: int, y: int, text: str, color: int = COLOR_WHITE) -> None:
        """
        Vereinfachte Textausgabe (1 Pixel pro Zeichen als Platzhalter).

        Die echte Font-Rendering passiert im Firmware-Renderer.
        Fuer den Simulator markieren wir nur die Position.
        """
        for i, ch in enumerate(text):
            self.draw_pixel(x + i, y, color)

    def get_dirty_rects(self) -> List[Rect]:
        """Dirty-Rectangles zurueckgeben (fuer partial refresh)."""
        return list(self._dirty_rects)

    def clear_dirty_rects(self) -> None:
        self._dirty_rects.clear()

    def flush(self) -> None:
        """Framebuffer push simulieren (fuer CI: no-op)."""
        pass

    def _in_bounds(self, x: int, y: int) -> bool:
        return 0 <= x < self._width and 0 <= y < self._height

    def _invalidate(self, x: int, y: int, w: int, h: int) -> None:
        if len(self._dirty_rects) < 16:
            self._dirty_rects.append(Rect(x, y, w, h))

    def snapshot(self) -> bytes:
        """Framebuffer-Kopie als bytes zurueckgeben (fuer Screenshots/Tests)."""
        return bytes(self._buffer)

    def fill_test_pattern(self) -> None:
        """Testmuster zeichnen (Farbbalken)."""
        bar_w = self._width // 8
        colors = [
            COLOR_ACCENT_RED, COLOR_ACCENT_ORANGE, COLOR_ACCENT_AMBER,
            COLOR_ACCENT_YELLOW, COLOR_ACCENT_GREEN, COLOR_ACCENT_CYAN,
            COLOR_ACCENT_BLUE, COLOR_WHITE,
        ]
        for i, c in enumerate(colors):
            self.fill_rect(i * bar_w, 0, bar_w, self._height, c)


class SdlDisplay(HeadlessDisplay):
    """
    SDL2-basierte Display fuer lokale Visualisierung.

    Erfordert pygame (pip install pygame).
    Wirft ImportError wenn pygame nicht verfuegbar.
    """

    def __init__(self, display_type: DisplayType = DisplayType.TFT,
                 scale: int = 2, title: str = "TRANSALP Bordcomputer Simulator"):
        try:
            import pygame
        except ImportError as e:
            raise ImportError(
                "SDL2 display requires pygame. Install with: pip install pygame"
            ) from e

        super().__init__(display_type)
        self._pygame = pygame
        self._scale = scale
        self._pygame.init()
        self._screen = self._pygame.display.set_mode(
            (self._width * scale, self._height * scale)
        )
        self._pygame.display.set_caption(title)
        self._surface = self._pygame.Surface((self._width, self._height))

    def flush(self) -> bool:
        """Framebuffer auf SDL-Surface pushen. True solange Fenster offen."""
        for event in self._pygame.event.get():
            if event.type == self._pygame.QUIT:
                return False

        # RGB565 buffer -> SDL surface
        pixels = self._pygame.surfarray.pixels3d(self._surface)
        for y in range(self._height):
            for x in range(self._width):
                color = self.get_pixel(x, y)
                r, g, b = rgb565_to_rgb888(color)
                pixels[x][y] = [r, g, b]
        del pixels

        scaled = self._pygame.transform.scale(
            self._surface, (self._width * self._scale, self._height * self._scale)
        )
        self._screen.blit(scaled, (0, 0))
        self._pygame.display.flip()
        return True

    def close(self) -> None:
        self._pygame.quit()


def create_display(display_type: DisplayType = DisplayType.TFT,
                   use_sdl: bool = False, **kwargs) -> HeadlessDisplay:
    """Factory: Headless oder SDL2 Display erzeugen."""
    if use_sdl:
        return SdlDisplay(display_type, **kwargs)
    return HeadlessDisplay(display_type)

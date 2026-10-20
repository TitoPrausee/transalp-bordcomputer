"""
test_mock_display.py — Tests fuer das Mock-Display (Headless Framebuffer)

Testet:
  1. Framebuffer-Groesse (TFT 240x320, E-Paper 296x128)
  2. Pixel zeichnen und lesen
  3. Rechteck fuellen
  4. Linie zeichnen (Bresenham)
  5. Kreis zeichnen
  6. RGB565-Konvertierung
  7. Dirty-Rect-System
  8. Backlight-Steuerung
"""

from mock_display import (
    HeadlessDisplay, DisplayType, DisplayMode,
    TFT_WIDTH, TFT_HEIGHT, EPD_WIDTH, EPD_HEIGHT,
    COLOR_BLACK, COLOR_WHITE, COLOR_ACCENT_RED, COLOR_ACCENT_GREEN,
    rgb565, rgb565_to_rgb888,
)


class TestDisplayGeometry:
    """Test der Display-Geometrie."""

    def test_tft_dimensions(self):
        """TFT hat 240x320 Pixel."""
        d = HeadlessDisplay(DisplayType.TFT)
        assert d.width == TFT_WIDTH
        assert d.height == TFT_HEIGHT

    def test_epaper_dimensions(self):
        """E-Paper hat 296x128 Pixel."""
        d = HeadlessDisplay(DisplayType.EPAPER)
        assert d.width == EPD_WIDTH
        assert d.height == EPD_HEIGHT

    def test_framebuffer_size(self):
        """Framebuffer hat korrekte Groesse (2 Bytes pro Pixel)."""
        d = HeadlessDisplay(DisplayType.TFT)
        fb = d.get_framebuffer()
        assert len(fb) == TFT_WIDTH * TFT_HEIGHT * 2

    def test_display_type(self):
        """Display-Typ wird korrekt gesetzt."""
        d_tft = HeadlessDisplay(DisplayType.TFT)
        assert d_tft.display_type == DisplayType.TFT
        d_epd = HeadlessDisplay(DisplayType.EPAPER)
        assert d_epd.display_type == DisplayType.EPAPER


class TestPixelOperations:
    """Test der Pixel-Operationen."""

    def test_draw_and_read_pixel(self):
        """Pixel zeichnen und zuruecklesen."""
        d = HeadlessDisplay()
        d.draw_pixel(10, 20, COLOR_ACCENT_RED)
        assert d.get_pixel(10, 20) == COLOR_ACCENT_RED

    def test_pixel_out_of_bounds_ignored(self):
        """Pixel ausserhalb des Bildschirms werden ignoriert."""
        d = HeadlessDisplay()
        d.draw_pixel(-1, 0, COLOR_WHITE)
        d.draw_pixel(TFT_WIDTH, 0, COLOR_WHITE)
        d.draw_pixel(0, TFT_HEIGHT, COLOR_WHITE)
        # Keine Exception, kein Pixel gesetzt
        assert d.get_pixel(0, 0) == COLOR_BLACK

    def test_clear_sets_all_pixels(self):
        """clear() setzt alle Pixel auf die Farbe."""
        d = HeadlessDisplay()
        d.clear(COLOR_WHITE)
        for y in range(0, TFT_HEIGHT, 50):
            for x in range(0, TFT_WIDTH, 50):
                assert d.get_pixel(x, y) == COLOR_WHITE


class TestDrawingPrimitives:
    """Test der Zeichen-Primitiven."""

    def test_fill_rect(self):
        """Rechteck fuellen."""
        d = HeadlessDisplay()
        d.clear(COLOR_BLACK)
        d.fill_rect(10, 10, 20, 20, COLOR_ACCENT_GREEN)
        assert d.get_pixel(10, 10) == COLOR_ACCENT_GREEN
        assert d.get_pixel(29, 29) == COLOR_ACCENT_GREEN
        assert d.get_pixel(9, 9) == COLOR_BLACK
        assert d.get_pixel(30, 30) == COLOR_BLACK

    def test_draw_line(self):
        """Linie zeichnen."""
        d = HeadlessDisplay()
        d.clear(COLOR_BLACK)
        d.draw_line(0, 0, 10, 10, COLOR_WHITE)
        assert d.get_pixel(0, 0) == COLOR_WHITE
        assert d.get_pixel(5, 5) == COLOR_WHITE
        assert d.get_pixel(10, 10) == COLOR_WHITE

    def test_draw_circle(self):
        """Kreis zeichnen."""
        d = HeadlessDisplay()
        d.clear(COLOR_BLACK)
        cx, cy, r = 50, 50, 10
        d.draw_circle(cx, cy, r, COLOR_WHITE)
        # Punkte auf dem Kreis sollten gesetzt sein
        assert d.get_pixel(cx + r, cy) == COLOR_WHITE
        assert d.get_pixel(cx - r, cy) == COLOR_WHITE
        assert d.get_pixel(cx, cy + r) == COLOR_WHITE
        assert d.get_pixel(cx, cy - r) == COLOR_WHITE

    def test_fill_circle(self):
        """Gefuellter Kreis."""
        d = HeadlessDisplay()
        d.clear(COLOR_BLACK)
        d.fill_circle(50, 50, 5, COLOR_WHITE)
        assert d.get_pixel(50, 50) == COLOR_WHITE
        assert d.get_pixel(53, 50) == COLOR_WHITE
        assert d.get_pixel(47, 50) == COLOR_WHITE


class TestRgb565:
    """Test der RGB565-Konvertierung."""

    def test_rgb565_black(self):
        assert rgb565(0, 0, 0) == 0x0000

    def test_rgb565_white(self):
        assert rgb565(255, 255, 255) == 0xFFFF

    def test_rgb565_red(self):
        assert rgb565(255, 0, 0) == 0xF800

    def test_rgb565_green(self):
        assert rgb565(0, 255, 0) == 0x07E0

    def test_rgb565_blue(self):
        assert rgb565(0, 0, 255) == 0x001F

    def test_roundtrip(self):
        """RGB565 -> RGB888 -> RGB565 ist verlustbehaftet aber plausibel."""
        for r, g, b in [(255, 0, 0), (0, 255, 0), (0, 0, 255), (128, 128, 128)]:
            color = rgb565(r, g, b)
            r2, g2, b2 = rgb565_to_rgb888(color)
            assert abs(r - r2) <= 8
            assert abs(g - g2) <= 4
            assert abs(b - b2) <= 8


class TestDisplayFeatures:
    """Test weiterer Display-Features."""

    def test_dirty_rects(self):
        """Dirty-Rects werden nach Zeichnen registriert."""
        d = HeadlessDisplay()
        d.clear_dirty_rects()
        d.draw_pixel(10, 10, COLOR_WHITE)
        assert len(d.get_dirty_rects()) > 0

    def test_backlight(self):
        """Backlight-Prozent kann gesetzt und gelesen werden."""
        d = HeadlessDisplay()
        d.set_backlight(75)
        assert d.backlight == 75
        d.set_backlight(150)
        assert d.backlight == 100  # clamped
        d.set_backlight(-10)
        assert d.backlight == 0  # clamped

    def test_mode_switch(self):
        """Display-Modus kann gewechselt werden."""
        d = HeadlessDisplay()
        assert d.mode == DisplayMode.TOUR
        d.set_mode(DisplayMode.NAV)
        assert d.mode == DisplayMode.NAV

    def test_snapshot(self):
        """Snapshot liefert eine Kopie des Framebuffers."""
        d = HeadlessDisplay()
        d.clear(COLOR_BLACK)
        d.draw_pixel(0, 0, COLOR_WHITE)
        snap = d.snapshot()
        assert isinstance(snap, bytes)
        assert len(snap) == TFT_WIDTH * TFT_HEIGHT * 2

    def test_test_pattern(self):
        """Testmattern zeichnet Farbbalken."""
        d = HeadlessDisplay()
        d.fill_test_pattern()
        # Erste und letzte Spalte sollten unterschiedlich sein
        assert d.get_pixel(0, 0) != d.get_pixel(TFT_WIDTH - 1, 0)

"""
test_mock_gps.py — Tests fuer den Mock-GPS / NMEA-Generator

Testet:
  1. GPX-Parsing (sample_alps.gpx)
  2. NMEA-Checksummen-Korrektheit
  3. NMEA-Satz-Format (GGA, RMC, VTG)
  4. Koordinaten-Format (ddmm.mmmm / dddmm.mmmm)
  5. Geschwindigkeitsberechnung
  6. Kursberechnung (Bearing)
  7. Haversine-Entfernung
  8. Track-Fortschritt
"""

import pytest
import os
from mock_gps import (
    NmeaGenerator, GpsData, TrackPoint, parse_gpx,
    haversine_km, bearing_deg,
)


SAMPLE_TRACK = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "tracks", "sample_alps.gpx"
)


class TestGpxParsing:
    """Test des GPX-Parsers."""

    def test_parse_sample_track(self):
        """Sample-Track wird korrekt geparst."""
        points = parse_gpx(SAMPLE_TRACK)
        assert len(points) > 10
        assert all(isinstance(p, TrackPoint) for p in points)

    def test_track_has_coordinates(self):
        """Alle Punkte haben gueltige Koordinaten."""
        points = parse_gpx(SAMPLE_TRACK)
        for p in points:
            assert -90 <= p.lat <= 90
            assert -180 <= p.lon <= 180

    def test_track_has_elevation(self):
        """Punkte haben Hoehenangaben."""
        points = parse_gpx(SAMPLE_TRACK)
        assert any(p.ele > 0 for p in points)

    def test_track_has_timestamps(self):
        """Punkte haben Zeitstempel."""
        points = parse_gpx(SAMPLE_TRACK)
        assert any(p.time is not None for p in points)


class TestNmeaChecksum:
    """Test der NMEA-Checksummen-Berechnung."""

    def test_checksum_is_xor(self):
        """Checksumme ist XOR ueber alle Zeichen zwischen $ und *."""
        body = "GPGGA,120000.000,4729.9400,N,01105.9400,E,1,09,1.2,0700.0,M"
        cksum = NmeaGenerator.nmea_checksum(body)
        expected = 0
        for ch in body:
            expected ^= ord(ch)
        assert cksum == expected

    def test_generated_sentences_have_valid_checksum(self):
        """Generierte NMEA-Saetze haben korrekte Checksummen."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        for _ in range(20):
            try:
                sentence = next(gen)
            except StopIteration:
                break
            assert sentence.startswith("$")
            assert "*" in sentence
            # Checksumme verifizieren
            body = sentence[1:sentence.index("*")]
            given_hex = sentence[sentence.index("*") + 1:].strip()
            given_cksum = int(given_hex, 16)
            calc_cksum = NmeaGenerator.nmea_checksum(body)
            assert given_cksum == calc_cksum, \
                f"Checksum mismatch: {sentence.strip()}"

    def test_sentence_format(self):
        """NMEA-Saetze haben das korrekte Format ($...*XX)."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        for _ in range(10):
            try:
                s = next(gen)
            except StopIteration:
                break
            assert s.startswith("$GP")
            assert s.endswith("\r\n")
            assert "*" in s


class TestNmeaContent:
    """Test der NMEA-Satz-Inhalte."""

    def test_rmc_has_active_status(self):
        """RMC-Saetze haben Status A (active) bei Fix."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        rmc_found = False
        for _ in range(30):
            try:
                s = next(gen)
            except StopIteration:
                break
            if "GPRMC" in s:
                rmc_found = True
                # Status-Feld sollte A sein
                parts = s.split(",")
                assert parts[2] == "A", f"RMC status not active: {s.strip()}"
        assert rmc_found, "No RMC sentence generated"

    def test_gga_has_fix_quality(self):
        """GGA-Saetze haben Fix-Qualitaet >= 1."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        gga_found = False
        for _ in range(30):
            try:
                s = next(gen)
            except StopIteration:
                break
            if "GPGGA" in s:
                gga_found = True
                parts = s.split(",")
                fix_q = int(parts[6])
                assert fix_q >= 1, f"Fix quality should be >= 1: {s.strip()}"
        assert gga_found, "No GGA sentence generated"

    def test_coordinate_format(self):
        """Koordinaten haben NMEA-Format (ddmm.mmmm / dddmm.mmmm)."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        for _ in range(30):
            try:
                s = next(gen)
            except StopIteration:
                break
            if "GPRMC" in s:
                parts = s.split(",")
                lat_str = parts[3]  # ddmm.mmmm
                lon_str = parts[5]  # dddmm.mmmm
                assert len(lat_str) >= 8, f"Lat too short: {lat_str}"
                assert len(lon_str) >= 9, f"Lon too short: {lon_str}"
                assert parts[4] in ("N", "S")
                assert parts[6] in ("E", "W")


class TestGeoCalculations:
    """Test der Geo-Berechnungen."""

    def test_haversine_same_point(self):
        """Entfernung zum selben Punkt ist 0."""
        assert haversine_km(47.5, 11.1, 47.5, 11.1) == pytest.approx(0.0)

    def test_haversine_known_distance(self):
        """Bekannte Entfernung zwischen zwei Punkten."""
        # Muenchen (48.137, 11.575) -> Salzburg (47.809, 13.055) ~116 km
        d = haversine_km(48.137, 11.575, 47.809, 13.055)
        assert 110 < d < 125, f"Expected ~116km, got {d:.1f}km"

    def test_bearing_north(self):
        """Bearing nach Norden ist ~0 Grad."""
        b = bearing_deg(47.0, 11.0, 48.0, 11.0)
        assert -1 < b < 1 or b > 359

    def test_bearing_east(self):
        """Bearing nach Osten ist ~90 Grad."""
        b = bearing_deg(47.0, 11.0, 47.0, 12.0)
        assert 85 < b < 95


class TestNmeaGeneratorProgress:
    """Test des Fortschritts des NMEA-Generators."""

    def test_progress_increases(self):
        """Fortschritt steigt mit iterierten Saetzen."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        p0 = gen.progress
        for _ in range(5):
            try:
                next(gen)
            except StopIteration:
                break
        assert gen.progress > p0

    def test_reset(self):
        """Reset setzt den Generator zurueck."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        for _ in range(5):
            try:
                next(gen)
            except StopIteration:
                break
        assert gen.progress > 0
        gen.reset()
        assert gen.progress == 0.0

    def test_gps_data_has_fix(self):
        """Nach Iteration hat GPS Daten einen Fix."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        try:
            next(gen)
        except StopIteration:
            pass
        data = gen.get_data()
        assert data.has_fix is True
        assert data.lat != 0.0
        assert data.lon != 0.0

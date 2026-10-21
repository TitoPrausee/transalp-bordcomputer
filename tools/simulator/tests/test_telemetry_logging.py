"""
test_telemetry_logging.py — End-to-End Telemetry-Logging Validation

Validiert (TOT-59 Scope 2): CSV-Telemetry-Ausgabe und GPX-Track-Logging
sind korrekt und vollstaendig, wenn der Simulator mit Telemetry-Logging
laeuft.

Getestet wird:
  1. CSV-Datei wird erzeugt und hat den korrekten Header
  2. CSV-Zeilen haben 11 Felder (timestamp_ms,lat,lon,...,fuel_l)
  3. CSV-Timestamps sind monoton steigend
  4. CSV-Werte sind plausible Sensordaten (RPM, Temp, Fuel im Range)
  5. GPX-Datei ist wohlgeformtes XML (GPX 1.1)
  6. GPX hat <trkpt> Elemente mit lat/lon Attributen
  7. GPX <trkpt> haben <ele> und <time> Kindelemente
  8. Distanzfilter: alle aufeinanderfolgenden Punkte sind >= 10m apart
  9. Telemetry-Stats: total_points > 0, points_skipped >= 0
"""

import csv
import os
import xml.etree.ElementTree as ET

import pytest

from simulator import Simulator, SimulatorConfig
from telemetry_logger import TelemetryLogger, haversine_m, LOGGER_MIN_DIST_M


SAMPLE_TRACK = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "tracks", "sample_alps.gpx"
)

CSV_COLUMNS = [
    "timestamp_ms", "lat", "lon", "speed_kmh", "course_deg",
    "altitude_m", "rpm", "temp_c", "roll_deg", "pitch_deg", "fuel_l",
]


@pytest.fixture
def telemetry_sim(tmp_path):
    """Simulator mit Telemetry-Logging und Sample-Track."""
    config = SimulatorConfig(
        track_file=SAMPLE_TRACK,
        duration_s=30,
        tick_hz=50,
        speed_multiplier=10.0,
        fast=True,
        skip_display=True,
        log_telemetry=True,
        telemetry_dir=str(tmp_path),
        telemetry_name="test_tour",
        seed=42,
    )
    sim = Simulator(config)
    sim.run_blocking()
    return sim, tmp_path


class TestTelemetryFiles:
    """CSV- und GPX-Dateien werden erzeugt."""

    def test_csv_file_exists(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        csv_path = os.path.join(str(tmp_path), "test_tour.csv")
        assert os.path.exists(csv_path)
        assert os.path.getsize(csv_path) > 0

    def test_gpx_file_exists(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        gpx_path = os.path.join(str(tmp_path), "test_tour.gpx")
        assert os.path.exists(gpx_path)
        assert os.path.getsize(gpx_path) > 0


class TestCsvFormat:
    """CSV-Format-Validierung (entspricht firmware logger_write_csv_row)."""

    def test_csv_header_correct(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        csv_path = os.path.join(str(tmp_path), "test_tour.csv")
        with open(csv_path, "r", encoding="utf-8") as f:
            reader = csv.reader(f)
            header = next(reader)
        assert header == CSV_COLUMNS

    def test_csv_rows_have_11_fields(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        csv_path = os.path.join(str(tmp_path), "test_tour.csv")
        with open(csv_path, "r", encoding="utf-8") as f:
            reader = csv.reader(f)
            next(reader)  # skip header
            rows = list(reader)
        assert len(rows) > 0
        for row in rows:
            assert len(row) == 11, f"Row has {len(row)} fields, expected 11"

    def test_csv_timestamps_monotonic(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        csv_path = os.path.join(str(tmp_path), "test_tour.csv")
        with open(csv_path, "r", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            timestamps = [int(row["timestamp_ms"]) for row in reader]
        assert len(timestamps) > 1
        for i in range(1, len(timestamps)):
            assert timestamps[i] >= timestamps[i - 1], \
                "Timestamps must be monotonically non-decreasing"

    def test_csv_rpm_in_range(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        csv_path = os.path.join(str(tmp_path), "test_tour.csv")
        with open(csv_path, "r", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            for row in reader:
                rpm = int(row["rpm"])
                assert 0 <= rpm <= 8000, f"RPM {rpm} out of range [0, 8000]"

    def test_csv_temp_in_range(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        csv_path = os.path.join(str(tmp_path), "test_tour.csv")
        with open(csv_path, "r", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            for row in reader:
                temp = float(row["temp_c"])
                assert 0 <= temp <= 130, f"Temp {temp} out of plausible range"

    def test_csv_fuel_in_range(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        csv_path = os.path.join(str(tmp_path), "test_tour.csv")
        with open(csv_path, "r", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            for row in reader:
                fuel = float(row["fuel_l"])
                assert 0 <= fuel <= 17, f"Fuel {fuel} out of range [0, 17]"

    def test_csv_lat_lon_valid(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        csv_path = os.path.join(str(tmp_path), "test_tour.csv")
        with open(csv_path, "r", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            for row in reader:
                lat = float(row["lat"])
                lon = float(row["lon"])
                assert -90 <= lat <= 90, f"Lat {lat} out of range"
                assert -180 <= lon <= 180, f"Lon {lon} out of range"


class TestGpxFormat:
    """GPX-Format-Validierung (entspricht firmware logger_export_gpx)."""

    def test_gpx_well_formed_xml(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        gpx_path = os.path.join(str(tmp_path), "test_tour.gpx")
        tree = ET.parse(gpx_path)  # raises ParseError if malformed
        root = tree.getroot()
        assert root.tag.endswith("gpx")

    def test_gpx_has_trkpts(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        gpx_path = os.path.join(str(tmp_path), "test_tour.gpx")
        tree = ET.parse(gpx_path)
        # Handle GPX namespace
        ns = ""
        if tree.getroot().tag.startswith("{"):
            ns = tree.getroot().tag.split("}")[0] + "}"
        trkpts = tree.findall(f".//{ns}trkpt")
        assert len(trkpts) > 0, "GPX must contain at least one trkpt"

    def test_gpx_trkpt_has_lat_lon(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        gpx_path = os.path.join(str(tmp_path), "test_tour.gpx")
        tree = ET.parse(gpx_path)
        ns = ""
        if tree.getroot().tag.startswith("{"):
            ns = tree.getroot().tag.split("}")[0] + "}"
        for trkpt in tree.findall(f".//{ns}trkpt"):
            assert "lat" in trkpt.attrib, "trkpt must have lat attribute"
            assert "lon" in trkpt.attrib, "trkpt must have lon attribute"
            lat = float(trkpt.attrib["lat"])
            lon = float(trkpt.attrib["lon"])
            assert -90 <= lat <= 90
            assert -180 <= lon <= 180

    def test_gpx_trkpt_has_ele_and_time(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        gpx_path = os.path.join(str(tmp_path), "test_tour.gpx")
        tree = ET.parse(gpx_path)
        ns = ""
        if tree.getroot().tag.startswith("{"):
            ns = tree.getroot().tag.split("}")[0] + "}"
        for trkpt in tree.findall(f".//{ns}trkpt"):
            ele = trkpt.find(f"{ns}ele")
            time = trkpt.find(f"{ns}time")
            assert ele is not None, "trkpt must have <ele> child"
            assert time is not None, "trkpt must have <time> child"
            float(ele.text)  # must be parseable as float
            int(time.text)  # must be parseable as int (timestamp_ms)

    def test_gpx_structure(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        gpx_path = os.path.join(str(tmp_path), "test_tour.gpx")
        tree = ET.parse(gpx_path)
        ns = ""
        if tree.getroot().tag.startswith("{"):
            ns = tree.getroot().tag.split("}")[0] + "}"
        root = tree.getroot()
        assert "1.1" in root.attrib.get("version", "")
        trk = root.find(f"{ns}trk")
        assert trk is not None
        trkseg = trk.find(f"{ns}trkseg")
        assert trkseg is not None


class TestDistanceFilter:
    """Der 10m-Distanzfilter (LOGGER_MIN_DIST_M) wird angewendet."""

    def test_csv_points_above_min_distance(self, telemetry_sim):
        """Alle aufeinanderfolgenden CSV-Punkte sind >= 10m apart."""
        sim, tmp_path = telemetry_sim
        csv_path = os.path.join(str(tmp_path), "test_tour.csv")
        with open(csv_path, "r", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            points = [(float(r["lat"]), float(r["lon"])) for r in reader]
        assert len(points) >= 2
        for i in range(1, len(points)):
            dist = haversine_m(
                points[i - 1][0], points[i - 1][1],
                points[i][0], points[i][1]
            )
            assert dist >= LOGGER_MIN_DIST_M, \
                f"Points {i-1}-{i} only {dist:.1f}m apart (< {LOGGER_MIN_DIST_M}m)"

    def test_gpx_points_above_min_distance(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        gpx_path = os.path.join(str(tmp_path), "test_tour.gpx")
        tree = ET.parse(gpx_path)
        ns = ""
        if tree.getroot().tag.startswith("{"):
            ns = tree.getroot().tag.split("}")[0] + "}"
        pts = []
        for trkpt in tree.findall(f".//{ns}trkpt"):
            pts.append((float(trkpt.attrib["lat"]), float(trkpt.attrib["lon"])))
        assert len(pts) >= 2
        for i in range(1, len(pts)):
            dist = haversine_m(
                pts[i - 1][0], pts[i - 1][1],
                pts[i][0], pts[i][1]
            )
            assert dist >= LOGGER_MIN_DIST_M


class TestTelemetryStats:
    """Telemetry-Statistik-Validierung."""

    def test_stats_total_points_positive(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        assert sim.telemetry is not None
        assert sim.telemetry.stats.total_points > 0

    def test_stats_points_written_matches_csv(self, telemetry_sim):
        sim, tmp_path = telemetry_sim
        csv_path = os.path.join(str(tmp_path), "test_tour.csv")
        with open(csv_path, "r", encoding="utf-8") as f:
            reader = csv.reader(f)
            next(reader)  # header
            csv_rows = sum(1 for _ in reader)
        assert sim.telemetry.stats.points_written == csv_rows


class TestHaversine:
    """Unit-Tests fuer die Haversine-Funktion."""

    def test_zero_distance(self):
        assert haversine_m(50.0, 11.0, 50.0, 11.0) == pytest.approx(0.0, abs=0.001)

    def test_one_degree_latitude(self):
        d = haversine_m(50.0, 11.0, 51.0, 11.0)
        assert d == pytest.approx(111195.0, abs=500.0)

    def test_below_threshold(self):
        d = haversine_m(50.0, 11.0, 50.0, 11.00001)
        assert d < LOGGER_MIN_DIST_M

    def test_above_threshold(self):
        d = haversine_m(50.0, 11.0, 50.0, 11.001)
        assert d > LOGGER_MIN_DIST_M


class TestTelemetryLoggerUnit:
    """Isolierte Unit-Tests fuer TelemetryLogger."""

    def test_inactive_add_returns_false(self, tmp_path):
        logger = TelemetryLogger(output_dir=str(tmp_path))
        assert not logger.add_track_point(0, 50.0, 11.0)

    def test_start_makes_active(self, tmp_path):
        logger = TelemetryLogger(output_dir=str(tmp_path))
        logger.start_tour()
        assert logger.is_active

    def test_first_point_always_accepted(self, tmp_path):
        logger = TelemetryLogger(output_dir=str(tmp_path))
        logger.start_tour()
        assert logger.add_track_point(0, 50.0, 11.0, rpm=3000)
        assert logger.stats.total_points == 1

    def test_close_point_skipped(self, tmp_path):
        logger = TelemetryLogger(output_dir=str(tmp_path))
        logger.start_tour()
        logger.add_track_point(0, 50.0, 11.0)
        # Point < 10m away -> skipped
        assert not logger.add_track_point(100, 50.0, 11.00001)
        assert logger.stats.points_skipped == 1
        assert logger.stats.total_points == 1

    def test_far_point_accepted(self, tmp_path):
        logger = TelemetryLogger(output_dir=str(tmp_path))
        logger.start_tour()
        logger.add_track_point(0, 50.0, 11.0)
        assert logger.add_track_point(200, 50.0, 11.001)
        assert logger.stats.total_points == 2

    def test_stop_writes_csv_and_gpx(self, tmp_path):
        logger = TelemetryLogger(output_dir=str(tmp_path))
        logger.start_tour(name="unit_tour")
        logger.add_track_point(0, 50.0, 11.0, rpm=3000, temp_c=85.0, fuel_l=10.0)
        logger.add_track_point(200, 50.0, 11.001, rpm=4000, temp_c=90.0, fuel_l=9.5)
        logger.stop_tour()
        assert os.path.exists(os.path.join(str(tmp_path), "unit_tour.csv"))
        assert os.path.exists(os.path.join(str(tmp_path), "unit_tour.gpx"))


class TestSimulatorWithoutTrack:
    """Telemetry-Logging ohne GPS-Track (nur Sensordaten)."""

    def test_telemetry_without_track(self, tmp_path):
        config = SimulatorConfig(
            track_file=None,
            duration_s=5,
            tick_hz=50,
            fast=True,
            skip_display=True,
            log_telemetry=True,
            telemetry_dir=str(tmp_path),
            telemetry_name="no_track",
            seed=42,
        )
        sim = Simulator(config)
        sim.run_blocking()
        csv_path = os.path.join(str(tmp_path), "no_track.csv")
        # Without a track, GPS stays at (0,0); points are logged
        # but the distance filter skips all but the first (same position).
        assert os.path.exists(csv_path)

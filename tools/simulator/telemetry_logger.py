"""
telemetry_logger.py — CSV/GPX Telemetry-Logger fuer den Simulator

Spiegelt die firmwareseitige logger.c-Implementierung
(firmware/components/storage/logger.c) nach:
  - CSV: timestamp_ms,lat,lon,speed_kmh,course_deg,altitude_m,
         rpm,temp_c,roll_deg,pitch_deg,fuel_l
  - GPX: GPX 1.1 mit <trkpt lat="" lon=""><ele/><time/></trkpt>
  - Distanzfilter: Punkte < 10m auseinander werden verworfen
    (LOGGER_MIN_DIST_M, Haversine-Formel)
  - Maximale Log-Rate: 10 Hz (LOGGER_MAX_RATE_HZ)

Wird vom Simulator verwendet, um End-to-End Telemetry-Logging
zu validieren (TOT-59 Scope 2).
"""

from __future__ import annotations

import math
import os
from dataclasses import dataclass
from typing import Optional, List

# ---- Konstanten (aus firmware/components/storage/include/storage.h) ----

LOGGER_MIN_DIST_M = 10.0
LOGGER_MAX_RATE_HZ = 10
LOGGER_RINGBUF_SIZE = 256

CSV_HEADER = (
    "timestamp_ms,lat,lon,speed_kmh,course_deg,altitude_m,"
    "rpm,temp_c,roll_deg,pitch_deg,fuel_l\n"
)


@dataclass
class LogEntry:
    """Aequivalent zu firmware log_entry_t."""

    timestamp_ms: int = 0
    lat: float = 0.0
    lon: float = 0.0
    speed_kmh: float = 0.0
    course_deg: float = 0.0
    altitude_m: float = 0.0
    rpm: int = 0
    temp_c: float = 0.0
    roll_deg: float = 0.0
    pitch_deg: float = 0.0
    fuel_l: float = 0.0


@dataclass
class TelemetryStats:
    """Aequivalent zu firmware logger_stats_t."""

    total_points: int = 0
    points_skipped: int = 0
    points_written: int = 0
    flush_count: int = 0
    write_errors: int = 0


def haversine_m(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    """
    Haversine-Formel: Grosskreis-Distanz zwischen zwei Koordinaten in Metern.

    Entspricht der static haversine_m() in logger.c.
    """
    r = 6371000.0
    phi1 = math.radians(lat1)
    phi2 = math.radians(lat2)
    dphi = math.radians(lat2 - lat1)
    dlam = math.radians(lon2 - lon1)
    a = (math.sin(dphi / 2) ** 2
         + math.cos(phi1) * math.cos(phi2) * math.sin(dlam / 2) ** 2)
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    return r * c


class TelemetryLogger:
    """
    Sammelt Sensor- + GPS-Daten und schreibt CSV/GPX-Dateien.

    Usage:
        logger = TelemetryLogger(output_dir="/tmp/telemetry")
        logger.start_tour()
        for tick in simulation:
            logger.add_track_point(sensor_data, gps_data)
        logger.stop_tour()  # schreibt CSV + GPX
    """

    def __init__(self, output_dir: str = "/tmp/telemetry",
                 rate_hz: int = LOGGER_MAX_RATE_HZ):
        self._output_dir = output_dir
        self._rate_hz = min(max(rate_hz, 1), LOGGER_MAX_RATE_HZ)
        self._interval_ms = 1000 // self._rate_hz
        self._ring: List[LogEntry] = []
        self._active = False
        self._last_lat: Optional[float] = None
        self._last_lon: Optional[float] = None
        self._last_log_ms: int = 0
        self._stats = TelemetryStats()
        self._csv_path: Optional[str] = None
        self._gpx_path: Optional[str] = None

    @property
    def stats(self) -> TelemetryStats:
        return self._stats

    @property
    def csv_path(self) -> Optional[str]:
        return self._csv_path

    @property
    def gpx_path(self) -> Optional[str]:
        return self._gpx_path

    @property
    def is_active(self) -> bool:
        return self._active

    def start_tour(self, name: str = "sim_tour") -> None:
        """Tour starten (entspricht logger_start_tour)."""
        os.makedirs(self._output_dir, exist_ok=True)
        self._csv_path = os.path.join(self._output_dir, f"{name}.csv")
        self._gpx_path = os.path.join(self._output_dir, f"{name}.gpx")
        self._active = True
        self._last_lat = None
        self._last_lon = None
        self._last_log_ms = -self._interval_ms  # allow first point at t=0
        self._ring.clear()

    def add_track_point(self, timestamp_ms: int,
                        lat: float, lon: float,
                        speed_kmh: float = 0.0, course_deg: float = 0.0,
                        altitude_m: float = 0.0, rpm: int = 0,
                        temp_c: float = 0.0, roll_deg: float = 0.0,
                        pitch_deg: float = 0.0,
                        fuel_l: float = 0.0) -> bool:
        """
        Track-Point hinzufuegen (entspricht logger_add_track_point).

        Wendet den Distanzfilter an (Punkte < 10m werden verworfen)
        und respektiert die maximale Log-Rate.
        Rueckgabe: True wenn der Punkt akzeptiert wurde.
        """
        if not self._active:
            return False

        # Rate-Limiting
        if timestamp_ms - self._last_log_ms < self._interval_ms:
            return False

        # Distanzfilter
        if self._last_lat is not None and self._last_lon is not None:
            dist = haversine_m(self._last_lat, self._last_lon, lat, lon)
            if dist < LOGGER_MIN_DIST_M:
                self._stats.points_skipped += 1
                return False

        entry = LogEntry(
            timestamp_ms=timestamp_ms,
            lat=lat, lon=lon,
            speed_kmh=speed_kmh, course_deg=course_deg,
            altitude_m=altitude_m, rpm=rpm,
            temp_c=temp_c, roll_deg=roll_deg,
            pitch_deg=pitch_deg, fuel_l=fuel_l,
        )

        # Ringbuffer (Overflow aeltstes Element verdraengen)
        if len(self._ring) >= LOGGER_RINGBUF_SIZE:
            self._ring.pop(0)
            self._stats.points_skipped += 1

        self._ring.append(entry)
        self._last_lat = lat
        self._last_lon = lon
        self._last_log_ms = timestamp_ms
        self._stats.total_points += 1
        return True

    def flush_csv(self) -> int:
        """
        Ringbuffer in CSV-Datei schreiben (entspricht flush_batch_to_csv).

        Rueckgabe: Anzahl geschriebener Zeilen.
        """
        if not self._csv_path or not self._ring:
            return 0

        write_header = not os.path.exists(self._csv_path) or \
            os.path.getsize(self._csv_path) == 0

        try:
            with open(self._csv_path, "a", encoding="utf-8") as f:
                if write_header:
                    f.write(CSV_HEADER)
                for entry in self._ring:
                    f.write(self._format_csv_row(entry))
                    self._stats.points_written += 1
            self._stats.flush_count += 1
            self._ring.clear()
            return self._stats.points_written
        except (IOError, OSError) as e:
            self._stats.write_errors += 1
            print(f"TelemetryLogger: CSV write error: {e}")
            return 0

    def export_gpx(self) -> bool:
        """
        Ringbuffer als GPX-Datei exportieren (entspricht logger_export_gpx).

        Rueckgabe: True bei Erfolg.
        """
        if not self._gpx_path:
            return False

        try:
            with open(self._gpx_path, "w", encoding="utf-8") as f:
                f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
                f.write('<gpx version="1.1" creator="transalp-bordcomputer" '
                        'xmlns="http://www.topografix.com/GPX/1/1">\n')
                f.write('  <metadata>\n')
                f.write('    <name>Tour Track</name>\n')
                f.write('  </metadata>\n')
                f.write('  <trk>\n')
                f.write('    <name>Tour</name>\n')
                f.write('    <trkseg>\n')
                for entry in self._ring:
                    f.write(self._format_gpx_point(entry))
                f.write('    </trkseg>\n')
                f.write('  </trk>\n')
                f.write('</gpx>\n')
            return True
        except (IOError, OSError) as e:
            self._stats.write_errors += 1
            print(f"TelemetryLogger: GPX write error: {e}")
            return False

    def stop_tour(self) -> None:
        """Tour beenden: GPX export + CSV flush (entspricht logger_stop_tour).

        GPX wird vor dem CSV-Flush exportiert, da flush_csv den Ringbuffer
        leert — so erhalten beide Formate die vollstaendigen Daten.
        """
        if not self._active:
            return
        self.export_gpx()
        self.flush_csv()
        self._active = False

    @staticmethod
    def _format_csv_row(entry: LogEntry) -> str:
        """CSV-Zeile formatieren (entspricht logger_write_csv_row)."""
        return (
            f"{entry.timestamp_ms},"
            f"{entry.lat:.6f},"
            f"{entry.lon:.6f},"
            f"{entry.speed_kmh:.1f},"
            f"{entry.course_deg:.1f},"
            f"{entry.altitude_m:.1f},"
            f"{entry.rpm},"
            f"{entry.temp_c:.1f},"
            f"{entry.roll_deg:.1f},"
            f"{entry.pitch_deg:.1f},"
            f"{entry.fuel_l:.1f}\n"
        )

    @staticmethod
    def _format_gpx_point(entry: LogEntry) -> str:
        """GPX-Trackpoint formatieren (entspricht write_gpx_point)."""
        return (
            f'      <trkpt lat="{entry.lat:.6f}" lon="{entry.lon:.6f}">\n'
            f'        <ele>{entry.altitude_m:.1f}</ele>\n'
            f'        <time>{entry.timestamp_ms}</time>\n'
            f'      </trkpt>\n'
        )

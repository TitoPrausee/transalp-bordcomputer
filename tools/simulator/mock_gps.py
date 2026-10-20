"""
mock_gps.py — Mock-GPS: NMEA-String-Generator von GPX-Tracks

Liest GPX-Track-Dateien und generiert NMEA-0183 Saetze (GGA, RMC, VTG),
die vom firmwareseitigen NMEA-Parser (firmware/components/gps/nmea_parser.c)
korrekt geparst werden.

Unterstützt:
  - GPX 1.1 Parsing (trkpt mit lat/lon/ele/time)
  - Geschwindigkeitsberechnung zwischen Wegpunkten
  - Kursberechnung (Bearing)
  - Beschleunigte Wiedergabe (speed_multiplier)
  - Korrekte XOR-Checksummen
  - HDOP-Simulation
"""

from __future__ import annotations

import math
import random
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from typing import List, Optional, Tuple, Iterator
from datetime import datetime, timezone, timedelta


# ---- GPS-Datenstruktur (entspricht firmware gps_data_t) ----

@dataclass
class GpsData:
    """Python-Aequivalent zu firmware gps_data_t."""

    has_fix: bool = False
    lat: float = 0.0
    lon: float = 0.0
    altitude_m: float = 0.0
    speed_kmh: float = 0.0
    course_deg: float = 0.0
    satellites: int = 0
    hdop: float = 99.0
    fix_quality: int = 0  # 0=none, 1=3D, 2=DGPS
    valid: bool = False

    year: int = 2026
    month: int = 1
    day: int = 1
    hour: int = 0
    minute: int = 0
    second: int = 0

    timestamp_ms: int = 0


# ---- GPX-Parsing ----

@dataclass
class TrackPoint:
    """Ein Wegpunkt aus einer GPX-Datei."""
    lat: float
    lon: float
    ele: float = 0.0
    time: Optional[datetime] = None


def parse_gpx(filepath: str) -> List[TrackPoint]:
    """
    GPX 1.1 Datei parsen und Wegpunkte zurueckgeben.

    Erwartet <trkpt lat="..." lon="..."> mit optionalem <ele> und <time>.
    """
    tree = ET.parse(filepath)
    root = tree.getroot()

    # NamespaceHandling: GPX 1.1 hat xmlns
    ns = ""
    if root.tag.startswith("{"):
        ns = root.tag.split("}")[0] + "}"

    points: List[TrackPoint] = []

    for trkpt in root.iter(f"{ns}trkpt"):
        lat_str = trkpt.get("lat")
        lon_str = trkpt.get("lon")
        if lat_str is None or lon_str is None:
            continue

        try:
            lat = float(lat_str)
            lon = float(lon_str)
        except (ValueError, TypeError):
            continue

        ele = 0.0
        ele_elem = trkpt.find(f"{ns}ele")
        if ele_elem is not None and ele_elem.text:
            try:
                ele = float(ele_elem.text)
            except (ValueError, TypeError):
                pass

        time_val: Optional[datetime] = None
        time_elem = trkpt.find(f"{ns}time")
        if time_elem is not None and time_elem.text:
            try:
                time_val = datetime.fromisoformat(
                    time_elem.text.replace("Z", "+00:00")
                )
            except (ValueError, TypeError):
                pass

        points.append(TrackPoint(lat=lat, lon=lon, ele=ele, time=time_val))

    return points


# ---- Geo-Berechnungen ----

def haversine_km(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    """Entfernung zwischen zwei Koordinaten in km (Haversine-Formel)."""
    R = 6371.0  # Erdradius km
    phi1 = math.radians(lat1)
    phi2 = math.radians(lat2)
    dphi = math.radians(lat2 - lat1)
    dlam = math.radians(lon2 - lon1)

    a = (
        math.sin(dphi / 2) ** 2
        + math.cos(phi1) * math.cos(phi2) * math.sin(dlam / 2) ** 2
    )
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    return R * c


def bearing_deg(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    """Kurs (Bearing) von Punkt 1 zu Punkt 2 in Grad true."""
    phi1 = math.radians(lat1)
    phi2 = math.radians(lat2)
    dlam = math.radians(lon2 - lon1)

    x = math.sin(dlam) * math.cos(phi2)
    y = math.cos(phi1) * math.sin(phi2) - math.sin(phi1) * math.cos(phi2) * math.cos(dlam)
    theta = math.atan2(x, y)
    return (math.degrees(theta) + 360.0) % 360.0


# ---- NMEA-Generator ----

class NmeaGenerator:
    """
    Generiert NMEA-0183 Saetze aus GPX-Wegpunkten.

    Usage:
        gen = NmeaGenerator("tracks/sample_alps.gpx", speed_multiplier=10)
        for sentence in gen:
            process(sentence)
    """

    def __init__(
        self,
        gpx_filepath: str,
        speed_multiplier: float = 1.0,
        start_time: Optional[datetime] = None,
        hdop: float = 1.2,
        satellites: int = 9,
        seed: Optional[int] = None,
    ):
        if seed is not None:
            self._rng = random.Random(seed)
        else:
            self._rng = random.Random()

        self._points = parse_gpx(gpx_filepath)
        if not self._points:
            raise ValueError(f"No track points found in {gpx_filepath}")

        self._speed_multiplier = speed_multiplier
        self._start_time = start_time or datetime.now(timezone.utc)
        self._base_hdop = hdop
        self._base_sats = satellites
        self._index = 0
        self._elapsed_s = 0.0
        self._current = GpsData()

    def __iter__(self) -> Iterator[str]:
        return self

    def __next__(self) -> str:
        """Naechsten NMEA-Satz generieren."""
        if self._index >= len(self._points) - 1:
            raise StopIteration

        p1 = self._points[self._index]
        p2 = self._points[self._index + 1]

        # Entfernung und Zeit
        dist_km = haversine_km(p1.lat, p1.lon, p2.lat, p2.lon)
        course = bearing_deg(p1.lat, p1.lon, p2.lat, p2.lon)

        # Zeitberechnung: Original-Zeitdifferenz oder 1s pro Punkt
        if p1.time and p2.time:
            orig_dt = (p2.time - p1.time).total_seconds()
        else:
            orig_dt = 1.0

        sim_dt = orig_dt / self._speed_multiplier if orig_dt > 0 else 0.1
        speed_kmh = (dist_km / sim_dt) * 3600 if sim_dt > 0 else 0.0

        self._elapsed_s += sim_dt
        sim_time = self._start_time + timedelta(seconds=self._elapsed_s)

        # HDOP mit etwas Variation
        hdop = self._base_hdop + self._rng.uniform(-0.2, 0.3)
        sats = max(4, self._base_sats + self._rng.randint(-2, 2))

        self._current = GpsData(
            has_fix=True,
            lat=p2.lat,
            lon=p2.lon,
            altitude_m=p2.ele,
            speed_kmh=speed_kmh,
            course_deg=course,
            satellites=sats,
            hdop=hdop,
            fix_quality=1,
            valid=hdop < 5.0,
            year=sim_time.year,
            month=sim_time.month,
            day=sim_time.day,
            hour=sim_time.hour,
            minute=sim_time.minute,
            second=sim_time.second,
            timestamp_ms=int(self._elapsed_s * 1000),
        )

        # Saetze generieren: GGA, RMC, VTG
        gga = self._gen_gga(sim_time)
        rmc = self._gen_rmc(sim_time)
        vtg = self._gen_vtg()

        self._index += 1

        # Drei Saetze pro Tick zurueckgeben (RMC als Haupt-, dann GGA, VTG)
        # Wir geben immer RMC zurueck und rotieren GGA/VTG
        sentences = [rmc, gga, vtg]
        return sentences[self._index % 3]

    def get_data(self) -> GpsData:
        """Aktuelle GPS-Daten zurueckgeben."""
        return self._current

    @property
    def progress(self) -> float:
        """Fortschritt 0.0..1.0."""
        if len(self._points) <= 1:
            return 1.0
        return self._index / (len(self._points) - 1)

    @property
    def remaining_points(self) -> int:
        return max(0, len(self._points) - 1 - self._index)

    def _gen_gga(self, dt: datetime) -> str:
        """$GPGGA — Global Positioning System Fix Data generieren."""
        time_str = dt.strftime("%H%M%S.000")
        lat, lat_dir = self._fmt_lat(self._current.lat)
        lon, lon_dir = self._fmt_lon(self._current.lon)
        fix_q = self._current.fix_quality
        sats = self._current.satellites
        hdop = f"{self._current.hdop:.1f}"
        alt = f"{self._current.altitude_m:.1f}"

        body = (
            f"GPGGA,{time_str},{lat},{lat_dir},{lon},{lon_dir},"
            f"{fix_q},{sats:02d},{hdop},{alt},M,0.0,M,,"
        )
        return self._wrap(body)

    def _gen_rmc(self, dt: datetime) -> str:
        """$GPRMC — Recommended Minimum Navigation Information generieren."""
        time_str = dt.strftime("%H%M%S.000")
        date_str = dt.strftime("%d%m%y")
        status = "A" if self._current.has_fix else "V"
        lat, lat_dir = self._fmt_lat(self._current.lat)
        lon, lon_dir = self._fmt_lon(self._current.lon)
        speed_kn = self._current.speed_kmh / 1.852  # km/h -> knots
        course = f"{self._current.course_deg:.1f}"

        body = (
            f"GPRMC,{time_str},{status},{lat},{lat_dir},{lon},{lon_dir},"
            f"{speed_kn:.1f},{course},{date_str},,"
        )
        return self._wrap(body)

    def _gen_vtg(self) -> str:
        """$GPVTG — Track Made Good and Ground Speed generieren."""
        course = f"{self._current.course_deg:.1f}"
        speed_kn = self._current.speed_kmh / 1.852
        speed_kmh = f"{self._current.speed_kmh:.1f}"

        body = f"GPVTG,{course},T,,M,{speed_kn:.1f},N,{speed_kmh},K,"
        return self._wrap(body)

    def _fmt_lat(self, lat: float) -> Tuple[str, str]:
        """Breitengrad in NMEA-Format ddmm.mmmm umwandeln."""
        dir_char = "N" if lat >= 0 else "S"
        lat_abs = abs(lat)
        deg = int(lat_abs)
        minutes = (lat_abs - deg) * 60.0
        return f"{deg:02d}{minutes:07.4f}", dir_char

    def _fmt_lon(self, lon: float) -> Tuple[str, str]:
        """Laengengrad in NMEA-Format dddmm.mmmm umwandeln."""
        dir_char = "E" if lon >= 0 else "W"
        lon_abs = abs(lon)
        deg = int(lon_abs)
        minutes = (lon_abs - deg) * 60.0
        return f"{deg:03d}{minutes:07.4f}", dir_char

    @staticmethod
    def _wrap(body: str) -> str:
        """NMEA-Satz mit $ und *CC (XOR-Checksumme) versehen."""
        cksum = 0
        for ch in body:
            cksum ^= ord(ch)
        return f"${body}*{cksum:02X}\r\n"

    @staticmethod
    def nmea_checksum(body: str) -> int:
        """XOR-Checksumme berechnen (zwischen $ und *)."""
        cksum = 0
        for ch in body:
            cksum ^= ord(ch)
        return cksum

    def reset(self) -> None:
        """Generator zuruecksetzen."""
        self._index = 0
        self._elapsed_s = 0.0
        self._current = GpsData()

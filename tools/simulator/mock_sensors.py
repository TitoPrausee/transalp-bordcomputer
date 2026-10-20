"""
mock_sensors.py — Mock-Sensor-Schicht fuer den Firmware-Simulator

Generiert plausible Sensordaten, die der firmwareseitigen sensor_data_t
(firmware/components/sensors/include/sensors.h) entsprechen.

Unterstuetzt konfigurierbare Verlaeufe (Curves) fuer:
  - RPM        (0-8000 U/min, Redline ~7500)
  - Coolant    (20-120 C, Uebertemp bei 120 C)
  - Fuel       (0-17 L, Reserve bei 3 L)
  - Gear       (0=Neutral, 1-6, berechnet aus Speed+RPM)
  - IMU        (Roll/Pitch in Grad, Accel in g, Gyro in dps)
  - Speed      (km/h, abgeleitet aus RPM und Gang)

Konfiguration erfolgt ueber Keyframe-Listen (Zeitpunkt, Wert).
Zwischen Keyframes wird linear interpoliert.
"""

from __future__ import annotations

import math
import random
from dataclasses import dataclass
from typing import List, Tuple, Optional, Any


# ---- Konstanten (aus firmware/main/board_config.h) ----

RPM_MAX_VALUE = 8000
RPM_REDLINE = 7500
RPM_IDLE = 1200

TEMP_OVERTEMP_C = 120.0
TEMP_AMBIENT_C = 25.0

TANK_CAPACITY_L = 17.0
FUEL_RESERVE_L = 3.0

GEAR_NEUTRAL = 0
GEAR_MIN = 1
GEAR_MAX = 6

# Gear ratios (typical Honda Transalp PD06)
GEAR_RATIOS = [0.0, 2.846, 1.947, 1.434, 1.158, 0.963, 0.851]
FINAL_DRIVE = 2.937
TIRE_CIRCUMFERENCE_M = 1.985  # 90/90-21 front + 150/70-18 rear avg

# IMU noise
IMU_ACCEL_NOISE = 0.02   # g
IMU_GYRO_NOISE = 0.5     # dps


@dataclass
class SensorData:
    """Python-Aequivalent zu firmware sensor_data_t."""

    # IMU (6-Achsen)
    accel_x_g: float = 0.0
    accel_y_g: float = 0.0
    accel_z_g: float = 1.0  # 1g at rest (gravity)
    gyro_x_dps: float = 0.0
    gyro_y_dps: float = 0.0
    gyro_z_dps: float = 0.0
    roll_deg: float = 0.0
    pitch_deg: float = 0.0

    # Temperatur
    temp_c: float = TEMP_AMBIENT_C
    pressure_hpa: float = 1013.25
    altitude_m: float = 0.0

    # Fahrzeug-Sensoren
    rpm: int = 0
    speed_kmh: float = 0.0
    gear: int = GEAR_NEUTRAL
    coolant_temp_c: float = TEMP_AMBIENT_C
    fuel_level_l: float = TANK_CAPACITY_L

    # Diagnose
    crash_detected: bool = False
    timestamp_ms: int = 0


# ---- Curve-Engine ----

class SensorCurve:
    """
    Konfigurierbarer Sensor-Verlauf ueber Keyframes.

    Keyframes sind (zeit_ms, wert) Paare. Zwischen Keyframes
    wird linear interpoliert. Nach dem letzten Keyframe bleibt
    der Wert konstant.
    """

    def __init__(self, keyframes: Optional[List[Tuple[float, float]]] = None,
                 noise: float = 0.0, clamp_min: Optional[float] = None,
                 clamp_max: Optional[float] = None,
                 rng: Optional[Any] = None):
        self._keyframes: List[Tuple[float, float]] = sorted(
            keyframes or [], key=lambda kf: kf[0]
        )
        self._noise = noise
        self._clamp_min = clamp_min
        self._clamp_max = clamp_max
        self._rng = rng if rng is not None else random

    def sample(self, elapsed_ms: float) -> float:
        """Wert zum Zeitpunkt elapsed_ms zurueckgeben."""
        if not self._keyframes:
            return 0.0

        # Vor erstem Keyframe -> erster Wert
        if elapsed_ms <= self._keyframes[0][0]:
            val = self._keyframes[0][1]
        # Nach letztem Keyframe -> letzter Wert
        elif elapsed_ms >= self._keyframes[-1][0]:
            val = self._keyframes[-1][1]
        else:
            # Interpolation zwischen zwei Keyframes
            for i in range(len(self._keyframes) - 1):
                t0, v0 = self._keyframes[i]
                t1, v1 = self._keyframes[i + 1]
                if t0 <= elapsed_ms <= t1:
                    if t1 == t0:
                        val = v0
                    else:
                        frac = (elapsed_ms - t0) / (t1 - t0)
                        val = v0 + frac * (v1 - v0)
                    break
            else:
                val = self._keyframes[-1][1]

        # Noise
        if self._noise > 0:
            val += self._rng.uniform(-self._noise, self._noise)

        # Clamp
        if self._clamp_min is not None:
            val = max(val, self._clamp_min)
        if self._clamp_max is not None:
            val = min(val, self._clamp_max)

        return val


# ---- Profile ----

def default_rpm_curve() -> SensorCurve:
    """Typischer Stadt- und Landstrassen-Fahrverlauf (10 Min)."""
    return SensorCurve(
        keyframes=[
            (0,       RPM_IDLE),    # Stand, Leerlauf
            (5_000,   RPM_IDLE),    # 5s Leerlauf
            (10_000,  3000),        # Anfahren, 1. Gang
            (20_000,  4500),        # Beschleunigung
            (30_000,  5500),        # 3. Gang, Landstrasse
            (60_000,  5000),        # Cruise
            (90_000,  6000),        # Ueberholen
            (120_000, 4000),        # Abbremsen
            (150_000, RPM_IDLE),    # Stop
            (600_000, RPM_IDLE),    # Stand bis 10 Min
        ],
        noise=50,
        clamp_min=0,
        clamp_max=RPM_MAX_VALUE,
    )


def default_temp_curve() -> SensorCurve:
    """Kuehlmittel-Temperatur: Aufwaermen von ambient auf ~90C."""
    return SensorCurve(
        keyframes=[
            (0,       TEMP_AMBIENT_C),
            (60_000,  70.0),    # 1 Min: aufwaermen
            (120_000, 85.0),    # 2 Min: Betriebstemp
            (300_000, 92.0),    # 5 Min: normal
            (500_000, 95.0),    # 8 Min: heisse Fahrt
            (600_000, 90.0),    # 10 Min: Abkuehlung im Stand
        ],
        noise=0.5,
        clamp_min=TEMP_AMBIENT_C,
        clamp_max=TEMP_OVERTEMP_C,
    )


def default_fuel_curve() -> SensorCurve:
    """Kraftstoffstand: 17L -> 5L ueber 10 Min."""
    return SensorCurve(
        keyframes=[
            (0,       TANK_CAPACITY_L),
            (300_000, 12.0),
            (600_000, 5.0),
        ],
        noise=0.1,
        clamp_min=0,
        clamp_max=TANK_CAPACITY_L,
    )


def default_imu_curves() -> Tuple[SensorCurve, SensorCurve]:
    """Roll/Pitch-Verlauf fuer Kurvenfahrt."""
    roll = SensorCurve(
        keyframes=[
            (0,       0.0),
            (20_000,  15.0),   # Rechtskurve
            (40_000,  -12.0),  # Linkskurve
            (60_000,  8.0),
            (80_000,  -20.0),  # Starke Linkskurve
            (100_000, 0.0),
            (600_000, 0.0),
        ],
        noise=1.0,
        clamp_min=-45.0,
        clamp_max=45.0,
    )
    pitch = SensorCurve(
        keyframes=[
            (0,       0.0),
            (10_000,  3.0),   # Beschleunigung
            (30_000,  -2.0),  # Bremsen
            (50_000,  5.0),
            (70_000,  -3.0),
            (100_000, 0.0),
            (600_000, 0.0),
        ],
        noise=0.5,
        clamp_min=-20.0,
        clamp_max=20.0,
    )
    return roll, pitch


# ---- Mock-Sensor-Layer ----

class MockSensorLayer:
    """
    Mock-Sensor-Schicht: Generiert plausible Sensordaten.

    Usage:
        sensors = MockSensorLayer()
        sensors.start()
        data = sensors.update()  # bei jedem Tick
    """

    def __init__(
        self,
        rpm_curve: Optional[SensorCurve] = None,
        temp_curve: Optional[SensorCurve] = None,
        fuel_curve: Optional[SensorCurve] = None,
        roll_curve: Optional[SensorCurve] = None,
        pitch_curve: Optional[SensorCurve] = None,
        seed: Optional[int] = None,
    ):
        self._rng = random.Random(seed)

        self._rpm_curve = rpm_curve or default_rpm_curve()
        self._rpm_curve._rng = self._rng
        self._temp_curve = temp_curve or default_temp_curve()
        self._temp_curve._rng = self._rng
        self._fuel_curve = fuel_curve or default_fuel_curve()
        self._fuel_curve._rng = self._rng
        self._roll_curve, self._pitch_curve = (
            (roll_curve, pitch_curve)
            if roll_curve and pitch_curve
            else default_imu_curves()
        )
        self._roll_curve._rng = self._rng
        self._pitch_curve._rng = self._rng

        self._elapsed_ms = 0.0
        self._data = SensorData()
        self._started = False

    def start(self) -> None:
        """Simulationszeit zuruecksetzen und erste Probe nehmen."""
        self._elapsed_ms = 0.0
        self._started = True
        self.update(0.0)

    @property
    def elapsed_ms(self) -> float:
        return self._elapsed_ms

    def update(self, delta_ms: Optional[float] = None) -> SensorData:
        """
        Sensordaten fuer den naechsten Tick aktualisieren.

        delta_ms: Millisekunden seit letztem update().
                  Wenn None, wird 20ms (50Hz) angenommen.
        """
        if delta_ms is None:
            delta_ms = 20.0

        self._elapsed_ms += delta_ms
        t = self._elapsed_ms

        # RPM
        rpm = int(round(self._rpm_curve.sample(t)))
        rpm = max(0, min(rpm, RPM_MAX_VALUE))

        # Gear (berechnet aus RPM)
        gear = self._calc_gear(rpm, t)

        # Speed (aus RPM, Gang und Uebersetzung)
        speed = self._calc_speed(rpm, gear)

        # Temperatur
        coolant = self._temp_curve.sample(t)

        # Kraftstoff
        fuel = self._fuel_curve.sample(t)

        # IMU
        roll = self._roll_curve.sample(t)
        pitch = self._pitch_curve.sample(t)
        accel_x, accel_y, accel_z = self._imu_accel(roll, pitch)
        gyro_x, gyro_y, gyro_z = self._imu_gyro(roll, pitch, delta_ms)

        # Crash-Erkennung (extreme Beschleunigung)
        crash = abs(accel_x) > 3.0 or abs(accel_y) > 3.0

        self._data = SensorData(
            accel_x_g=accel_x,
            accel_y_g=accel_y,
            accel_z_g=accel_z,
            gyro_x_dps=gyro_x,
            gyro_y_dps=gyro_y,
            gyro_z_dps=gyro_z,
            roll_deg=roll,
            pitch_deg=pitch,
            temp_c=coolant,
            pressure_hpa=1013.25 + self._rng.uniform(-2, 2),
            altitude_m=self._rng.uniform(0, 500),
            rpm=rpm,
            speed_kmh=speed,
            gear=gear,
            coolant_temp_c=coolant,
            fuel_level_l=fuel,
            crash_detected=crash,
            timestamp_ms=int(t),
        )

        return self._data

    def get_data(self) -> SensorData:
        """Aktuelle Sensordaten zurueckgeben (ohne Update)."""
        return self._data

    def _calc_gear(self, rpm: int, t: float) -> int:
        """Gang aus RPM ableiten (vereinfacht)."""
        if rpm < RPM_IDLE + 200:
            return GEAR_NEUTRAL
        if rpm < 2500:
            return 1
        if rpm < 3500:
            return 2
        if rpm < 4500:
            return 3
        if rpm < 5500:
            return 4
        if rpm < 6500:
            return 5
        return 6

    def _calc_speed(self, rpm: int, gear: int) -> float:
        """Geschwindigkeit aus RPM und Gang berechnen."""
        if gear == GEAR_NEUTRAL:
            return 0.0
        ratio = GEAR_RATIOS[gear] * FINAL_DRIVE
        wheel_rps = (rpm / 60.0) / ratio
        speed_ms = wheel_rps * TIRE_CIRCUMFERENCE_M
        return speed_ms * 3.6  # m/s -> km/h

    def _imu_accel(self, roll: float, pitch: float) -> Tuple[float, float, float]:
        """Beschleunigung aus Roll/Pitch ableiten (inkl. Gravitation)."""
        roll_rad = math.radians(roll)
        pitch_rad = math.radians(pitch)
        ax = -math.sin(pitch_rad) * 9.81 / 9.81 + self._rng.uniform(-IMU_ACCEL_NOISE, IMU_ACCEL_NOISE)
        ay = math.sin(roll_rad) + self._rng.uniform(-IMU_ACCEL_NOISE, IMU_ACCEL_NOISE)
        az = math.cos(roll_rad) * math.cos(pitch_rad) + self._rng.uniform(-IMU_ACCEL_NOISE, IMU_ACCEL_NOISE)
        return (ax, ay, az)

    def _imu_gyro(self, roll: float, pitch: float, delta_ms: float) -> Tuple[float, float, float]:
        """Gyroskop-Raten aus Roll/Pitch-Aenderung ableiten."""
        gx = self._rng.uniform(-IMU_GYRO_NOISE, IMU_GYRO_NOISE)
        gy = self._rng.uniform(-IMU_GYRO_NOISE, IMU_GYRO_NOISE)
        gz = self._rng.uniform(-IMU_GYRO_NOISE, IMU_GYRO_NOISE)
        return (gx, gy, gz)

    def is_overtemp(self) -> bool:
        return self._data.coolant_temp_c >= TEMP_OVERTEMP_C

    def is_fuel_low(self) -> bool:
        return self._data.fuel_level_l <= FUEL_RESERVE_L

    def is_redline(self) -> bool:
        return self._data.rpm >= RPM_REDLINE

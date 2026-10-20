"""
test_mock_sensors.py — Tests fuer die Mock-Sensor-Schicht

Testet:
  1. RPM-Werteebereich (0-8000)
  2. Temperatur-Wertebeereich (20-120C)
  3. Kraftstoff-Wertebeereich (0-17L)
  4. Gang-Berechnung (0-6)
  5. IMU-Werte (Roll/Pitch)
  6. SensorCurve-Interpolation
  7. Speed-Berechnung aus RPM und Gang
  8. Warnungs-Flags (Overtemp, Low Fuel, Redline)
"""

import pytest
from mock_sensors import (
    MockSensorLayer, SensorData, SensorCurve,
    RPM_MAX_VALUE, RPM_IDLE,
    TEMP_OVERTEMP_C, TEMP_AMBIENT_C,
    TANK_CAPACITY_L,
    GEAR_NEUTRAL, GEAR_MAX,
)


class TestSensorData:
    """Test der SensorData-Datenstruktur."""

    def test_default_values(self):
        """SensorData hat plausible Default-Werte."""
        d = SensorData()
        assert d.rpm == 0
        assert d.speed_kmh == 0.0
        assert d.gear == GEAR_NEUTRAL
        assert d.coolant_temp_c == TEMP_AMBIENT_C
        assert d.fuel_level_l == TANK_CAPACITY_L
        assert d.accel_z_g == 1.0  # 1g at rest
        assert d.crash_detected is False


class TestSensorCurve:
    """Test der Curve-Engine."""

    def test_linear_interpolation(self):
        """Zwischen Keyframes wird linear interpoliert."""
        curve = SensorCurve(
            keyframes=[(0, 0.0), (1000, 100.0)],
            noise=0.0,
        )
        assert curve.sample(0) == 0.0
        assert curve.sample(500) == pytest.approx(50.0)
        assert curve.sample(1000) == pytest.approx(100.0)

    def test_clamp(self):
        """Werte werden auf clamp-Bereich begrenzt."""
        curve = SensorCurve(
            keyframes=[(0, -10.0), (1000, 200.0)],
            noise=0.0,
            clamp_min=0.0,
            clamp_max=100.0,
        )
        assert curve.sample(0) == 0.0  # clamped from -10
        assert curve.sample(1000) == 100.0  # clamped from 200

    def test_hold_after_last_keyframe(self):
        """Nach letztem Keyframe bleibt der Wert konstant."""
        curve = SensorCurve(keyframes=[(0, 42.0), (100, 99.0)], noise=0.0)
        assert curve.sample(10000) == pytest.approx(99.0)

    def test_hold_before_first_keyframe(self):
        """Vor erstem Keyframe gilt erster Wert."""
        curve = SensorCurve(keyframes=[(100, 42.0), (200, 99.0)], noise=0.0)
        assert curve.sample(0) == pytest.approx(42.0)

    def test_empty_curve_returns_zero(self):
        """Leere Curve gibt 0 zurueck."""
        curve = SensorCurve()
        assert curve.sample(1000) == 0.0


class TestMockSensorLayer:
    """Test der Mock-Sensor-Schicht."""

    def test_rpm_in_range(self):
        """RPM liegt immer im Bereich 0-8000."""
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        for _ in range(500):
            data = sensors.update(20)
            assert 0 <= data.rpm <= RPM_MAX_VALUE, f"RPM {data.rpm} out of range"

    def test_temperature_in_range(self):
        """Kuehlmittel-Temperatur liegt im Bereich 20-120C."""
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        for _ in range(500):
            data = sensors.update(20)
            assert TEMP_AMBIENT_C <= data.coolant_temp_c <= TEMP_OVERTEMP_C, \
                f"Temp {data.coolant_temp_c} out of range"

    def test_fuel_in_range(self):
        """Kraftstoffstand liegt im Bereich 0-17L."""
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        for _ in range(500):
            data = sensors.update(20)
            assert 0 <= data.fuel_level_l <= TANK_CAPACITY_L, \
                f"Fuel {data.fuel_level_l} out of range"

    def test_gear_in_range(self):
        """Gang liegt im Bereich 0-6."""
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        for _ in range(500):
            data = sensors.update(20)
            assert GEAR_NEUTRAL <= data.gear <= GEAR_MAX, \
                f"Gear {data.gear} out of range"

    def test_imu_roll_pitch_bounded(self):
        """Roll/Pitch liegen im plausiblen Bereich."""
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        for _ in range(500):
            data = sensors.update(20)
            assert -45 <= data.roll_deg <= 45, f"Roll {data.roll_deg} out of range"
            assert -20 <= data.pitch_deg <= 20, f"Pitch {data.pitch_deg} out of range"

    def test_speed_consistent_with_rpm_and_gear(self):
        """Geschwindigkeit ist 0 bei Neutral, >0 bei eingelegtem Gang."""
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        for _ in range(500):
            data = sensors.update(20)
            if data.gear == GEAR_NEUTRAL:
                assert data.speed_kmh == 0.0
            elif data.rpm > RPM_IDLE + 200:
                assert data.speed_kmh > 0.0

    def test_timestamp_increases(self):
        """Zeitstempel steigt mit jedem Update."""
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        t0 = sensors.update(20).timestamp_ms
        t1 = sensors.update(20).timestamp_ms
        t2 = sensors.update(50).timestamp_ms
        assert t1 > t0
        assert t2 > t1

    def test_reproducible_with_seed(self):
        """Mit gleichem Seed sind die Werte reproduzierbar."""
        s1 = MockSensorLayer(seed=123)
        s2 = MockSensorLayer(seed=123)
        s1.start()
        s2.start()
        for _ in range(100):
            d1 = s1.update(20)
            d2 = s2.update(20)
            assert d1.rpm == d2.rpm
            assert d1.coolant_temp_c == pytest.approx(d2.coolant_temp_c)

    def test_overtemp_detection(self):
        """Overtemp wird erkannt wenn Temperatur >= 120C."""
        sensors = MockSensorLayer(
            temp_curve=SensorCurve(
                keyframes=[(0, 125.0)],
                clamp_max=TEMP_OVERTEMP_C,
            ),
            seed=42,
        )
        sensors.start()
        sensors.update(20)
        assert sensors.is_overtemp()

    def test_fuel_low_detection(self):
        """Low Fuel wird erkannt wenn Kraftstoff <= 3L."""
        sensors = MockSensorLayer(
            fuel_curve=SensorCurve(keyframes=[(0, 2.0)]),
            seed=42,
        )
        sensors.start()
        sensors.update(20)
        assert sensors.is_fuel_low()

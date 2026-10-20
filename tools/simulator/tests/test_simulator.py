"""
test_simulator.py — Integration-Tests fuer den Simulator

Testet:
  1. Simulator-Initialisierung
  2. Tick-Ausfuehrung (Sensoren, GPS, Display, BLE)
  3. Duration-Limit
  4. Warnungs-Erkennung
  5. Status-JSON-Ausgabe
  6. CI-Mode-Logging
  7. End-to-End Simulation (kurzer Lauf)
"""

import json
import os
from simulator import Simulator, SimulatorConfig
from mock_display import DisplayType


SAMPLE_TRACK = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "tracks", "sample_alps.gpx"
)


class TestSimulatorInit:
    """Test der Simulator-Initialisierung."""

    def test_default_config(self):
        """Simulator mit Default-Config initialisierbar."""
        sim = Simulator()
        assert sim.sensors is not None
        assert sim.display is not None
        assert sim.ble is not None
        assert sim.controller is not None
        assert sim.ble.is_connected

    def test_with_track(self):
        """Simulator mit GPX-Track initialisiert GPS-Generator."""
        config = SimulatorConfig(track_file=SAMPLE_TRACK, seed=42)
        sim = Simulator(config)
        assert sim.gps is not None

    def test_without_track(self):
        """Simulator ohne Track hat keinen GPS-Generator."""
        config = SimulatorConfig(track_file=None, seed=42)
        sim = Simulator(config)
        assert sim.gps is None

    def test_epaper_display(self):
        """E-Paper Display kann konfiguriert werden."""
        config = SimulatorConfig(
            display_type=DisplayType.EPAPER, seed=42
        )
        sim = Simulator(config)
        assert sim.display.display_type == DisplayType.EPAPER


class TestSimulatorTick:
    """Test der Tick-Ausfuehrung."""

    def test_start_and_tick(self):
        """Nach start() und tick() hat der Simulator Sensordaten."""
        sim = Simulator(SimulatorConfig(seed=42))
        sim.start()
        state = sim.tick(20)
        assert state.tick_count == 1
        assert state.sensor_data is not None
        assert state.elapsed_ms == 20

    def test_multiple_ticks(self):
        """Mehrere Ticks akkumulieren Zeit und Ticks."""
        sim = Simulator(SimulatorConfig(seed=42))
        sim.start()
        for _ in range(10):
            sim.tick(20)
        assert sim.state.tick_count == 10
        assert sim.state.elapsed_ms == 200

    def test_tick_without_start_noop(self):
        """Tick ohne start() aendert nichts."""
        sim = Simulator(SimulatorConfig(seed=42))
        state = sim.tick(20)
        assert state.tick_count == 0
        assert state.elapsed_ms == 0

    def test_gps_update(self):
        """GPS wird bei ausreichend verstrichener Zeit aktualisiert."""
        config = SimulatorConfig(
            track_file=SAMPLE_TRACK,
            gps_hz=1.0,
            speed_multiplier=100,
            seed=42,
        )
        sim = Simulator(config)
        sim.start()
        # 1 Sekunde simulierte Zeit -> GPS sollte updaten
        for _ in range(50):
            sim.tick(20)
        assert sim.state.gps_data.has_fix or sim.gps is None

    def test_display_updated(self):
        """Display-Framebuffer wird nach Tick aktualisiert."""
        sim = Simulator(SimulatorConfig(seed=42))
        sim.start()
        sim.tick(20)
        fb = sim.display.get_framebuffer()
        # Nicht alles sollte schwarz sein (Widgets gezeichnet)
        assert any(b != 0 for b in fb)

    def test_ble_telemetry_updated(self):
        """BLE Telemetry wird nach Tick aktualisiert."""
        from mock_ble import CHR_TLM_LIVE
        sim = Simulator(SimulatorConfig(seed=42))
        sim.start()
        sim.tick(20)
        tlm = sim.ble._chars[CHR_TLM_LIVE].value
        assert len(tlm) > 0


class TestSimulatorDuration:
    """Test des Duration-Limits."""

    def test_duration_limit(self):
        """Simulator stoppt nach Ablauf der Duration."""
        config = SimulatorConfig(duration_s=0.1, seed=42)  # 100ms
        sim = Simulator(config)
        sim.start()
        for _ in range(20):
            sim.tick(20)
            if not sim.running:
                break
        assert not sim.running

    def test_stops_at_duration(self):
        """Simulator laeuft bis Duration und stoppt."""
        config = SimulatorConfig(duration_s=1.0, seed=42)
        sim = Simulator(config)
        sim.start()
        tick_count = 0
        while sim.running and tick_count < 100:
            sim.tick(20)
            tick_count += 1
        assert not sim.running


class TestSimulatorWarnings:
    """Test der Warnungs-Erkennung."""

    def test_warning_cleared_on_safe(self):
        """Warnungen werden geloescht wenn Werte sicher sind."""
        sim = Simulator(SimulatorConfig(seed=42))
        sim.start()
        sim.tick(20)
        # Bei Start-Sensordaten sollten keine kritischen Warnungen sein
        # (RPM im Leerlauf, Tank voll, Temp normal)
        assert "CRASH DETECTED" not in sim.state.warnings

    def test_overtemp_warning(self):
        """Overtemp-Warnung bei zu hoher Temperatur."""
        from mock_sensors import SensorCurve, TEMP_OVERTEMP_C
        config = SimulatorConfig(seed=42)
        sim = Simulator(config)
        sim.sensors = type(sim.sensors)(
            temp_curve=SensorCurve(
                keyframes=[(0, TEMP_OVERTEMP_C)],
                clamp_max=TEMP_OVERTEMP_C,
            ),
            seed=42,
        )
        sim.start()
        sim.tick(20)
        assert "OVERTEMP" in sim.state.warnings


class TestSimulatorStatus:
    """Test der Status-Ausgabe."""

    def test_status_json(self):
        """Status-JSON ist valides JSON mit erwarteten Feldern."""
        sim = Simulator(SimulatorConfig(seed=42))
        sim.start()
        sim.tick(20)
        status = json.loads(sim.get_status_json())
        assert "elapsed_ms" in status
        assert "tick_count" in status
        assert "sensors" in status
        assert "gps" in status
        assert "warnings" in status
        assert "ble_connected" in status

    def test_status_json_sensor_values(self):
        """Status-JSON enthaelt plausible Sensorwerte."""
        sim = Simulator(SimulatorConfig(seed=42))
        sim.start()
        sim.tick(20)
        status = json.loads(sim.get_status_json())
        s = status["sensors"]
        assert "rpm" in s
        assert "speed_kmh" in s
        assert "gear" in s
        assert "coolant_temp_c" in s
        assert "fuel_level_l" in s

    def test_controller_event_handling(self):
        """Controller-Events werden verarbeitet."""
        from mock_controller import ControllerButton
        sim = Simulator(SimulatorConfig(seed=42))
        sim.start()
        sim.tick(20)
        initial_mode = sim.display.mode
        sim.controller.inject_button(ControllerButton.HOME, pressed=True)
        sim.tick(20)
        # Mode sollte gewechselt haben
        assert sim.display.mode != initial_mode or sim.state.events > 0


class TestEndToEnd:
    """End-to-End Integration-Tests."""

    def test_short_simulation_run(self):
        """Kurze Simulationslauf (5 Sekunden) ohne Fehler."""
        config = SimulatorConfig(
            track_file=SAMPLE_TRACK,
            duration_s=5.0,
            tick_hz=50.0,
            speed_multiplier=100,
            seed=42,
        )
        sim = Simulator(config)
        sim.start()
        while sim.running:
            sim.tick(20)
        sim.stop()
        assert sim.state.tick_count > 0
        assert sim.state.elapsed_ms >= 5000
        assert sim.ble.is_connected is False  # disconnected on stop

    def test_reproducible_runs(self):
        """Mit gleichem Seed sind Laeufe reproduzierbar."""
        config1 = SimulatorConfig(seed=42, duration_s=1.0)
        config2 = SimulatorConfig(seed=42, duration_s=1.0)
        sim1 = Simulator(config1)
        sim2 = Simulator(config2)
        sim1.start()
        sim2.start()
        for _ in range(50):
            sim1.tick(20)
            sim2.tick(20)
        assert sim1.state.sensor_data.rpm == sim2.state.sensor_data.rpm

    def test_ci_mode_no_crash(self):
        """CI-Modus laeuft ohne Exception."""
        config = SimulatorConfig(
            track_file=SAMPLE_TRACK,
            duration_s=2.0,
            ci_mode=True,
            seed=42,
        )
        sim = Simulator(config)
        sim.start()
        while sim.running:
            sim.tick(20)
        assert sim.state.tick_count > 0

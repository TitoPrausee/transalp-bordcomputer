#!/usr/bin/env python3
"""
simulator.py — Haupt-Orchestrator fuer den TRANSALP Bordcomputer Simulator

Verbindet alle Mock-Schichten zu einem lauffaehigen Simulator:
  - MockSensorLayer  (RPM, Temp, Fuel, Gear, IMU)
  - NmeaGenerator    (GPS aus GPX-Tracks)
  - HeadlessDisplay  (Framebuffer fuer CI / SDL2 fuer lokal)
  - MockBle          (GATT-Server Simulation)
  - MockController   (Tastatur -> Controller-Events)

Kann als CLI-Tool oder als Python-Modul verwendet werden.

Usage:
    python3 simulator.py --track tracks/sample_alps.gpx --duration 600
    python3 simulator.py --headless --ci-mode
    python3 simulator.py --sdl  # lokale Visualisierung
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time
from dataclasses import dataclass, field
from typing import Optional, List

from mock_sensors import MockSensorLayer, SensorData, RPM_MAX_VALUE, TEMP_OVERTEMP_C, FUEL_RESERVE_L
from mock_gps import NmeaGenerator, GpsData
from mock_display import HeadlessDisplay, DisplayType, DisplayMode, COLOR_BLACK, COLOR_WHITE, COLOR_ACCENT_GREEN, COLOR_ACCENT_RED, COLOR_ACCENT_AMBER, COLOR_ACCENT_BLUE, rgb565
from mock_ble import MockBle, CHR_TLM_LIVE, CHR_SENSOR_STATUS, CHR_DTC_LIST
from mock_controller import MockController, ControllerButton, ControllerEvent


# ---- Simulations-State ----

@dataclass
class SimulatorConfig:
    """Konfiguration fuer den Simulator."""
    track_file: Optional[str] = None
    duration_s: float = 600.0       # 10 Min default
    tick_hz: float = 50.0           # 50 Hz Sensor-Update
    gps_hz: float = 1.0             # 1 Hz GPS-Update
    speed_multiplier: float = 10.0  # GPX-Wiedergabe beschleunigen
    headless: bool = True
    use_sdl: bool = False
    seed: Optional[int] = 42
    display_type: DisplayType = DisplayType.TFT
    ci_mode: bool = False
    log_nmea: bool = False
    log_interval_s: float = 10.0
    fast: bool = False  # No real-time sleep (for CI long runs)
    skip_display: bool = False  # Skip framebuffer updates (for CI long runs)


@dataclass
class SimulatorState:
    """Laufzeit-State des Simulators."""
    elapsed_ms: float = 0.0
    tick_count: int = 0
    sensor_data: SensorData = field(default_factory=SensorData)
    gps_data: GpsData = field(default_factory=GpsData)
    warnings: List[str] = field(default_factory=list)
    nmea_sentences: List[str] = field(default_factory=list)
    events: int = 0


class Simulator:
    """
    Haupt-Simulator: Orchestriert alle Mock-Schichten.

    Usage:
        sim = Simulator(config)
        sim.start()
        while sim.running:
            sim.tick()
        sim.stop()
    """

    def __init__(self, config: Optional[SimulatorConfig] = None):
        self.config = config or SimulatorConfig()
        self._running = False
        self._state = SimulatorState()

        # Mock-Schichten initialisieren
        self.sensors = MockSensorLayer(seed=self.config.seed)
        self.display = HeadlessDisplay(self.config.display_type)
        self.ble = MockBle()
        self.controller = MockController()

        # GPS (optional, wenn Track vorhanden)
        self.gps: Optional[NmeaGenerator] = None
        if self.config.track_file and os.path.exists(self.config.track_file):
            self.gps = NmeaGenerator(
                self.config.track_file,
                speed_multiplier=self.config.speed_multiplier,
                seed=self.config.seed,
            )

        # BLE verbinden
        self.ble.connect()
        self.ble.subscribe(CHR_TLM_LIVE)
        self.ble.subscribe(CHR_SENSOR_STATUS)
        self.ble.subscribe(CHR_DTC_LIST)

        # Timing
        self._tick_interval_ms = 1000.0 / self.config.tick_hz
        self._gps_interval_ms = 1000.0 / self.config.gps_hz
        self._last_gps_ms = 0.0
        self._last_log_ms = 0.0

    @property
    def running(self) -> bool:
        return self._running

    @property
    def state(self) -> SimulatorState:
        return self._state

    def start(self) -> None:
        """Simulator starten."""
        self._running = True
        self.sensors.start()
        self._state.elapsed_ms = 0.0
        self._state.tick_count = 0

    def stop(self) -> None:
        """Simulator anhalten."""
        self._running = False
        self.ble.disconnect()

    def tick(self, delta_ms: Optional[float] = None) -> SimulatorState:
        """
        Einen Simulations-Tick ausfuehren.

        delta_ms: Millisekunden seit letztem Tick (default: tick_interval).
        """
        if not self._running:
            return self._state

        if delta_ms is None:
            delta_ms = self._tick_interval_ms

        self._state.elapsed_ms += delta_ms
        self._state.tick_count += 1

        # 1. Sensoren updaten
        self._state.sensor_data = self.sensors.update(delta_ms)

        # 2. GPS updaten (mit eigener Rate)
        if self.gps and self._state.elapsed_ms - self._last_gps_ms >= self._gps_interval_ms:
            self._last_gps_ms = self._state.elapsed_ms
            try:
                nmea = next(self.gps)
                self._state.gps_data = self.gps.get_data()
                if self.config.log_nmea:
                    self._state.nmea_sentences.append(nmea)
            except StopIteration:
                self.gps = None  # Track zu Ende

        # 3. Display updaten (Widgets zeichnen) — skip in CI fast mode
        if not self.config.skip_display:
            self._update_display()

        # 4. BLE Telemetry pushen
        self.ble.set_telemetry_data(self._state.sensor_data)

        # 5. Controller-Events verarbeiten
        events = self.controller.poll_events()
        self._state.events += len(events)
        for evt in events:
            self._handle_controller_event(evt)

        # 6. Warnungen pruefen
        self._check_warnings()

        # 7. Dauer pruefen
        if self._state.elapsed_ms >= self.config.duration_s * 1000:
            self._running = False

        # 8. Logging (periodisch)
        if self.config.ci_mode and self._state.elapsed_ms - self._last_log_ms >= self.config.log_interval_s * 1000:
            self._last_log_ms = self._state.elapsed_ms
            self._log_status()

        return self._state

    def _update_display(self) -> None:
        """Display-Framebuffer mit Widget-Daten aktualisieren."""
        d = self.display
        d.clear(0x1084)  # Background panel color (skip separate clear+fill)

        # Speed (gross)
        speed = int(self._state.sensor_data.speed_kmh)
        speed_str = f"{speed:3d}"
        d.draw_text(10, 10, f"Speed: {speed_str} km/h", COLOR_WHITE)

        # RPM
        rpm = self._state.sensor_data.rpm
        rpm_str = f"{rpm:5d}"
        d.draw_text(10, 30, f"RPM: {rpm_str}", COLOR_WHITE)

        # Gear
        gear = self._state.sensor_data.gear
        gear_char = "N" if gear == 0 else str(gear)
        d.draw_text(10, 50, f"Gear: {gear_char}", COLOR_WHITE)

        # Temperature
        temp = self._state.sensor_data.coolant_temp_c
        temp_color = COLOR_ACCENT_RED if temp >= TEMP_OVERTEMP_C else COLOR_WHITE
        d.draw_text(10, 70, f"Temp: {temp:.0f}C", temp_color)

        # Fuel
        fuel = self._state.sensor_data.fuel_level_l
        fuel_color = COLOR_ACCENT_AMBER if fuel <= FUEL_RESERVE_L else COLOR_WHITE
        d.draw_text(10, 90, f"Fuel: {fuel:.1f}L", fuel_color)

        # GPS
        if self._state.gps_data.has_fix:
            d.draw_text(10, 110, f"GPS: {self._state.gps_data.lat:.4f} {self._state.gps_data.lon:.4f}", COLOR_ACCENT_GREEN)
        else:
            d.draw_text(10, 110, "GPS: No Fix", COLOR_ACCENT_RED)

        # Roll/Pitch
        roll = self._state.sensor_data.roll_deg
        pitch = self._state.sensor_data.pitch_deg
        d.draw_text(10, 130, f"Roll: {roll:.1f}  Pitch: {pitch:.1f}", COLOR_ACCENT_BLUE)

        # Warnings
        y = 150
        for w in self._state.warnings:
            d.draw_text(10, y, f"! {w}", COLOR_ACCENT_RED)
            y += 20

    def _handle_controller_event(self, evt) -> None:
        """Controller-Event verarbeiten (z.B. Display-Modus wechseln)."""
        if evt.event == ControllerEvent.BTN_HOME:
            modes = list(DisplayMode)
            current_idx = modes.index(self.display.mode)
            self.display.set_mode(modes[(current_idx + 1) % len(modes)])

    def _check_warnings(self) -> None:
        """Warnungen aus Sensordaten ableiten."""
        self._state.warnings.clear()
        s = self._state.sensor_data

        if s.coolant_temp_c >= TEMP_OVERTEMP_C:
            self._state.warnings.append("OVERTEMP")
        if s.fuel_level_l <= FUEL_RESERVE_L:
            self._state.warnings.append("LOW FUEL")
        if s.rpm >= RPM_MAX_VALUE:
            self._state.warnings.append("REDLINE")
        if s.crash_detected:
            self._state.warnings.append("CRASH DETECTED")
        if self.gps is None and self.config.track_file:
            self._state.warnings.append("GPS TRACK ENDED")

        # BLE Sensor-Status updaten
        self.ble.set_sensor_status(
            rpm_ok=True,
            temp_ok=s.coolant_temp_c < TEMP_OVERTEMP_C,
            fuel_ok=s.fuel_level_l > 0,
            gps_ok=self._state.gps_data.has_fix,
            imu_ok=True,
        )

    def _log_status(self) -> None:
        """Status-Zeile fuer CI-Logging ausgeben."""
        s = self._state.sensor_data
        g = self._state.gps_data
        elapsed = self._state.elapsed_ms / 1000.0
        warnings_str = ", ".join(self._state.warnings) if self._state.warnings else "none"
        print(
            f"[{elapsed:8.1f}s] "
            f"RPM={s.rpm:5d} "
            f"Speed={s.speed_kmh:6.1f}km/h "
            f"Gear={s.gear} "
            f"Temp={s.coolant_temp_c:5.1f}C "
            f"Fuel={s.fuel_level_l:4.1f}L "
            f"Roll={s.roll_deg:+6.1f} "
            f"GPS=({g.lat:.4f},{g.lon:.4f}) "
            f"Warnings: {warnings_str}"
        )

    def run_blocking(self) -> None:
        """Simulator blockierend laufen lassen (bis Duration erreicht)."""
        self.start()
        try:
            while self._running:
                self.tick()
                if not self.config.fast:
                    time.sleep(self._tick_interval_ms / 1000.0)
        except KeyboardInterrupt:
            pass
        finally:
            self.stop()

    def get_status_json(self) -> str:
        """Aktuellen Status als JSON zurueckgeben."""
        s = self._state.sensor_data
        g = self._state.gps_data
        return json.dumps({
            "elapsed_ms": self._state.elapsed_ms,
            "tick_count": self._state.tick_count,
            "sensors": {
                "rpm": s.rpm,
                "speed_kmh": round(s.speed_kmh, 1),
                "gear": s.gear,
                "coolant_temp_c": round(s.coolant_temp_c, 1),
                "fuel_level_l": round(s.fuel_level_l, 1),
                "roll_deg": round(s.roll_deg, 1),
                "pitch_deg": round(s.pitch_deg, 1),
                "crash_detected": s.crash_detected,
            },
            "gps": {
                "has_fix": g.has_fix,
                "lat": round(g.lat, 6),
                "lon": round(g.lon, 6),
                "speed_kmh": round(g.speed_kmh, 1),
                "altitude_m": round(g.altitude_m, 1),
                "satellites": g.satellites,
                "hdop": round(g.hdop, 1),
            },
            "warnings": self._state.warnings,
            "events": self._state.events,
            "ble_connected": self.ble.is_connected,
        }, indent=2)


def main():
    """CLI-Entry-Point."""
    parser = argparse.ArgumentParser(
        description="TRANSALP Bordcomputer Firmware Simulator"
    )
    parser.add_argument(
        "--track", "-t",
        default=None,
        help="GPX-Track-Datei fuer GPS-Simulation",
    )
    parser.add_argument(
        "--duration", "-d",
        type=float,
        default=600.0,
        help="Simulationsdauer in Sekunden (default: 600 = 10 Min)",
    )
    parser.add_argument(
        "--speed-multiplier", "-s",
        type=float,
        default=10.0,
        help="GPX-Wiedergabe-Beschleunigung (default: 10x)",
    )
    parser.add_argument(
        "--tick-hz",
        type=float,
        default=50.0,
        help="Sensor-Update-Rate in Hz (default: 50)",
    )
    parser.add_argument(
        "--sdl",
        action="store_true",
        help="SDL2-Fenster fuer lokale Visualisierung",
    )
    parser.add_argument(
        "--headless",
        action="store_true",
        default=True,
        help="Headless-Modus (keine GUI, fuer CI)",
    )
    parser.add_argument(
        "--ci-mode",
        action="store_true",
        help="CI-Modus: periodische Status-Ausgabe",
    )
    parser.add_argument(
        "--log-nmea",
        action="store_true",
        help="NMEA-Saetze protokollieren",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Random-Seed fuer reproduzierbare Laeufe",
    )
    parser.add_argument(
        "--epaper",
        action="store_true",
        help="E-Paper Display statt TFT simulieren",
    )
    parser.add_argument(
        "--status-json",
        action="store_true",
        help="Status als JSON ausgeben und beenden (ein Tick)",
    )
    parser.add_argument(
        "--fast",
        action="store_true",
        help="Kein Real-Time-Sleep (fuer CI Langzeitläufe)",
    )
    parser.add_argument(
        "--skip-display",
        action="store_true",
        help="Display-Updates ueberspringen (fuer CI Langzeitläufe)",
    )

    args = parser.parse_args()

    # Track-Datei suchen
    track_file = args.track
    if track_file is None:
        default_track = os.path.join(
            os.path.dirname(__file__), "tracks", "sample_alps.gpx"
        )
        if os.path.exists(default_track):
            track_file = default_track

    config = SimulatorConfig(
        track_file=track_file,
        duration_s=args.duration,
        tick_hz=args.tick_hz,
        speed_multiplier=args.speed_multiplier,
        headless=args.headless and not args.sdl,
        use_sdl=args.sdl,
        seed=args.seed,
        display_type=DisplayType.EPAPER if args.epaper else DisplayType.TFT,
        ci_mode=args.ci_mode,
        log_nmea=args.log_nmea,
        fast=args.fast,
        skip_display=args.skip_display,
    )

    sim = Simulator(config)

    if args.status_json:
        sim.start()
        sim.tick()
        print(sim.get_status_json())
        sim.stop()
        return

    if args.ci_mode:
        print(f"=== TRANSALP Bordcomputer Simulator (CI Mode) ===")
        print(f"Track: {track_file or 'none'}")
        print(f"Duration: {args.duration}s")
        print(f"Speed: {args.speed_multiplier}x")
        print(f"Tick: {args.tick_hz}Hz")
        print(f"Display: {config.display_type.name}")
        print()

    sim.run_blocking()

    if args.ci_mode:
        print()
        print("=== Simulation Complete ===")
        print(f"Ticks: {sim.state.tick_count}")
        print(f"Events: {sim.state.events}")
        print(f"Duration: {sim.state.elapsed_ms / 1000.0:.1f}s")
        print(f"Warnings: {sim.state.warnings or 'none'}")


if __name__ == "__main__":
    main()

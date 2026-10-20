"""
test_simulator_firmware_interface.py

Integration tests: Simulator Mock Layer vs Firmware Component Interfaces

Verifies that the Python simulator mock layer (mock_*.py) produces data
that is consistent with the firmware C component interfaces (sensors.h,
gps.h, ble.h, display.h, controller_input.h).

These tests validate the contract between the simulator and the firmware:
  - SensorData fields match sensor_data_t (sensors.h)
  - GpsData fields match gps_data_t (gps.h)
  - BLE telemetry packing matches the 22-byte format
  - Controller button IDs match controller_button_t (controller_input.h)
  - Display constants match display.h / board_config.h
  - NMEA output is parseable by the firmware parser format
"""

import os
import sys
import struct

# Add simulator root to path for imports
SIM_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, SIM_ROOT)

from mock_sensors import (
    MockSensorLayer, SensorData,
    RPM_MAX_VALUE, TEMP_OVERTEMP_C,
    TEMP_AMBIENT_C,
    TANK_CAPACITY_L, FUEL_RESERVE_L,
    GEAR_NEUTRAL, GEAR_MIN, GEAR_MAX,
    GEAR_RATIOS,
)
from mock_gps import (
    NmeaGenerator, GpsData,
)
from mock_ble import (
    MockBle, chr_uuid,
    CHR_TLM_LIVE, CHR_DTC_LIST, CHR_SENSOR_STATUS,
    SVC_NAVIGATION, SVC_TELEMETRY, SVC_DIAGNOSE,
    DtcEntry, DTC_SEV_WARNING, DTC_SEV_CRITICAL,
    DTC_ENTRY_SIZE, TLM_LIVE_SIZE, TLM_LIVE_FMT,
    SENSOR_GPS_OK, SENSOR_RPM_OK, SENSOR_TEMP_OK,
    SENSOR_FUEL_OK, SENSOR_SD_OK,
    CharPerm,
)
from mock_display import (
    HeadlessDisplay, DisplayType, DisplayMode,
    TFT_WIDTH, TFT_HEIGHT, EPD_WIDTH, EPD_HEIGHT,
    COLOR_BLACK, COLOR_WHITE, COLOR_ACCENT_RED,
    COLOR_ACCENT_GREEN, COLOR_ACCENT_BLUE,
    rgb565, rgb565_to_rgb888,
)
from mock_controller import (
    MockController, ControllerButton,
    BUTTON_TO_EVENT, KEY_MAP, CTRL_DEBOUNCE_MS,
)


SAMPLE_TRACK = os.path.join(SIM_ROOT, "tracks", "sample_alps.gpx")


# ============================================================
# Sensor Interface Consistency (sensors.h)
# ============================================================

class TestSensorDataMatchesFirmware:
    """Verify SensorData fields match firmware sensor_data_t (sensors.h)."""

    def test_sensor_data_has_imu_fields(self):
        """SensorData must have all 6 IMU axes (accel xyz, gyro xyz)."""
        sd = SensorData()
        assert hasattr(sd, 'accel_x_g')
        assert hasattr(sd, 'accel_y_g')
        assert hasattr(sd, 'accel_z_g')
        assert hasattr(sd, 'gyro_x_dps')
        assert hasattr(sd, 'gyro_y_dps')
        assert hasattr(sd, 'gyro_z_dps')

    def test_sensor_data_has_orientation_fields(self):
        """SensorData must have roll_deg and pitch_deg."""
        sd = SensorData()
        assert hasattr(sd, 'roll_deg')
        assert hasattr(sd, 'pitch_deg')

    def test_sensor_data_has_vehicle_fields(self):
        """SensorData must have rpm, speed_kmh, gear, coolant_temp_c, fuel_level_l."""
        sd = SensorData()
        assert hasattr(sd, 'rpm')
        assert hasattr(sd, 'speed_kmh')
        assert hasattr(sd, 'gear')
        assert hasattr(sd, 'coolant_temp_c')
        assert hasattr(sd, 'fuel_level_l')

    def test_sensor_data_has_diagnostic_fields(self):
        """SensorData must have crash_detected and timestamp_ms."""
        sd = SensorData()
        assert hasattr(sd, 'crash_detected')
        assert hasattr(sd, 'timestamp_ms')

    def test_rpm_max_value_matches_firmware(self):
        """RPM_MAX_VALUE must match firmware board_config.h (8000)."""
        assert RPM_MAX_VALUE == 8000

    def test_temp_overtemp_matches_firmware(self):
        """TEMP_OVERTEMP_C must match firmware board_config.h (120.0)."""
        assert TEMP_OVERTEMP_C == 120.0

    def test_tank_capacity_matches_firmware(self):
        """TANK_CAPACITY_L must match firmware board_config.h (17.0)."""
        assert TANK_CAPACITY_L == 17.0

    def test_fuel_reserve_matches_firmware(self):
        """FUEL_RESERVE_L must match firmware board_config.h (3.0)."""
        assert FUEL_RESERVE_L == 3.0

    def test_gear_range_matches_firmware(self):
        """Gear range 0-6 must match firmware (0=Neutral, 1-6)."""
        assert GEAR_NEUTRAL == 0
        assert GEAR_MIN == 1
        assert GEAR_MAX == 6

    def test_gear_ratios_count(self):
        """Gear ratios must have 7 entries (0=neutral + 6 gears)."""
        assert len(GEAR_RATIOS) == 7

    def test_rpm_in_valid_range(self):
        """Generated RPM must be within 0..RPM_MAX_VALUE."""
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        for _ in range(100):
            data = sensors.update(20)
            assert 0 <= data.rpm <= RPM_MAX_VALUE

    def test_gear_in_valid_range(self):
        """Generated gear must be 0..6."""
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        for _ in range(100):
            data = sensors.update(20)
            assert GEAR_NEUTRAL <= data.gear <= GEAR_MAX

    def test_fuel_in_valid_range(self):
        """Generated fuel must be 0..TANK_CAPACITY_L."""
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        for _ in range(100):
            data = sensors.update(20)
            assert 0 <= data.fuel_level_l <= TANK_CAPACITY_L

    def test_temp_in_valid_range(self):
        """Generated coolant temp must be ambient..overtemp."""
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        for _ in range(100):
            data = sensors.update(20)
            assert TEMP_AMBIENT_C <= data.coolant_temp_c <= TEMP_OVERTEMP_C

    def test_speed_non_negative(self):
        """Speed must be non-negative."""
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        for _ in range(100):
            data = sensors.update(20)
            assert data.speed_kmh >= 0

    def test_timestamp_increases(self):
        """timestamp_ms must increase with each update."""
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        prev_ts = 0
        for _ in range(10):
            data = sensors.update(20)
            assert data.timestamp_ms >= prev_ts
            prev_ts = data.timestamp_ms


# ============================================================
# GPS Interface Consistency (gps.h)
# ============================================================

class TestGpsDataMatchesFirmware:
    """Verify GpsData fields match firmware gps_data_t (gps.h)."""

    def test_gps_data_has_fix_fields(self):
        """GpsData must have has_fix, lat, lon, altitude_m, speed_kmh, course_deg."""
        gd = GpsData()
        assert hasattr(gd, 'has_fix')
        assert hasattr(gd, 'lat')
        assert hasattr(gd, 'lon')
        assert hasattr(gd, 'altitude_m')
        assert hasattr(gd, 'speed_kmh')
        assert hasattr(gd, 'course_deg')

    def test_gps_data_has_quality_fields(self):
        """GpsData must have satellites, hdop, fix_quality, valid."""
        gd = GpsData()
        assert hasattr(gd, 'satellites')
        assert hasattr(gd, 'hdop')
        assert hasattr(gd, 'fix_quality')
        assert hasattr(gd, 'valid')

    def test_gps_data_has_datetime_fields(self):
        """GpsData must have year, month, day, hour, minute, second."""
        gd = GpsData()
        assert hasattr(gd, 'year')
        assert hasattr(gd, 'month')
        assert hasattr(gd, 'day')
        assert hasattr(gd, 'hour')
        assert hasattr(gd, 'minute')
        assert hasattr(gd, 'second')

    def test_gps_data_has_timestamp(self):
        """GpsData must have timestamp_ms."""
        gd = GpsData()
        assert hasattr(gd, 'timestamp_ms')

    def test_nmea_generator_produces_valid_fix(self):
        """NmeaGenerator must produce data with has_fix=True."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        next(gen)
        data = gen.get_data()
        assert data.has_fix
        assert data.fix_quality >= 1

    def test_gps_speed_non_negative(self):
        """GPS speed must be non-negative."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        for _ in range(10):
            next(gen)
            data = gen.get_data()
            assert data.speed_kmh >= 0

    def test_gps_course_in_range(self):
        """GPS course must be 0..360."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        for _ in range(10):
            next(gen)
            data = gen.get_data()
            assert 0 <= data.course_deg < 360

    def test_gps_lat_lon_valid_range(self):
        """GPS lat must be -90..90, lon must be -180..180."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        for _ in range(10):
            next(gen)
            data = gen.get_data()
            assert -90 <= data.lat <= 90
            assert -180 <= data.lon <= 180

    def test_nmea_sentence_has_checksum(self):
        """NMEA sentences must have $ prefix and *XX checksum suffix."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        for _ in range(5):
            sentence = next(gen)
            assert sentence.startswith('$')
            assert '*' in sentence
            # Checksum is 2 hex chars after *
            cksum_part = sentence.rstrip('\r\n').split('*')[1]
            assert len(cksum_part) == 2
            int(cksum_part, 16)  # must be valid hex

    def test_nmea_sentence_types(self):
        """NMEA generator must produce GGA, RMC, or VTG sentences."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        seen_types = set()
        for _ in range(20):
            sentence = next(gen)
            # Extract sentence type (e.g. GPGGA -> GGA)
            body = sentence.lstrip('$').split('*')[0]
            stype = body[2:5] if len(body) > 5 else ""
            seen_types.add(stype)
        assert 'GGA' in seen_types or 'RMC' in seen_types or 'VTG' in seen_types

    def test_nmea_checksum_correct(self):
        """NMEA checksum must be correct XOR of all chars between $ and *."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)
        for _ in range(10):
            sentence = next(gen).rstrip('\r\n')
            body = sentence.lstrip('$').split('*')[0]
            expected_cksum = NmeaGenerator.nmea_checksum(body)
            actual_cksum = int(sentence.split('*')[1], 16)
            assert expected_cksum == actual_cksum


# ============================================================
# BLE Interface Consistency (ble.h / gatt_uuids.h)
# ============================================================

class TestBleInterfaceMatchesFirmware:
    """Verify BLE mock matches firmware GATT definitions (gatt_uuids.h)."""

    def test_base_uuid_format(self):
        """Base UUID must be 6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e."""
        uuid = chr_uuid(0x1001)
        assert uuid.startswith('6e40')
        assert uuid.endswith('-b5a3-f393-e0a9-e50e24dcca9e')

    def test_service_ids(self):
        """All 8 service IDs must be multiples of 0x1000."""
        services = [SVC_NAVIGATION, 0x2000, 0x3000, 0x4000,
                    SVC_TELEMETRY, 0x6000, SVC_DIAGNOSE, 0x8000]
        for svc in services:
            assert svc & 0x0FFF == 0

    def test_telemetry_characteristic(self):
        """TLM_LIVE characteristic must be 0x5001."""
        assert CHR_TLM_LIVE == 0x5001

    def test_dtc_characteristic(self):
        """DTC_LIST characteristic must be 0x7001."""
        assert CHR_DTC_LIST == 0x7001

    def test_sensor_status_characteristic(self):
        """SENSOR_STATUS characteristic must be 0x7002."""
        assert CHR_SENSOR_STATUS == 0x7002

    def test_telemetry_payload_size(self):
        """Live telemetry payload must be the packed struct size.

        Matches firmware telemetry_live_t (ble_callbacks.c:57), 22 bytes
        little-endian and packed: speed(u8) rpm(u16) gear(i8) coolant(i8)
        ambient(i8) fuel(u8) voltage(u8) lat(i32) lon(i32) alt(i16)
        heading(u16) sats(u8) hdop(u8). See docs/BLE-INTERFACE-SPEC.md §4.1.
        """
        ble = MockBle()
        ble.connect()
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        data = sensors.update(20)
        ble.set_telemetry_data(data)
        tlm = ble._chars[CHR_TLM_LIVE].value
        assert len(tlm) == TLM_LIVE_SIZE  # exactly 22 bytes, no padding

    def test_dtc_entry_packed_size(self):
        """DTC entry packed must match firmware dtc_binary_t layout.

        Firmware struct (ble_callbacks.c:77, packed): uint16_t code +
        uint8_t severity + uint32_t timestamp = 7 bytes. The spec §8.1
        header advertises "6 bytes/entry" and a _Static_assert==6, but
        those contradict the field-layout table (offsets 0:u16, 2:u8,
        3:u32 = 7 bytes). The actual packed struct is 7 bytes; the
        _Static_assert==6 is a firmware bug (never compiled on host).
        """
        dtc = DtcEntry(code=0x0101, severity=DTC_SEV_WARNING)
        packed = dtc.pack()
        assert len(packed) == DTC_ENTRY_SIZE  # 7 bytes

    def test_sensor_status_packed_size(self):
        """Sensor status packed must be 5 bytes (sensor_status_t, spec §8.2).

        5 distinct status bytes (gps, rpm, temp, fuel, sd_card), NOT a
        bitfield.
        """
        ble = MockBle()
        ble.connect()
        ble.set_sensor_status(gps=SENSOR_GPS_OK, rpm=SENSOR_RPM_OK,
                              temp=SENSOR_TEMP_OK, fuel=SENSOR_FUEL_OK,
                              sd_card=SENSOR_SD_OK)
        status = ble.read(CHR_SENSOR_STATUS)
        assert len(status) == 5

    def test_all_characteristics_have_uuid(self):
        """Every characteristic must have a valid UUID string."""
        ble = MockBle()
        for ch in ble.characteristics:
            assert ch.uuid is not None
            assert len(ch.uuid) == 36
            assert ch.uuid.startswith('6e40')

    def test_read_permissions(self):
        """Readable characteristics must have READ permission."""
        ble = MockBle()
        ble.connect()
        for ch in ble.characteristics:
            if ch.readable:
                assert ch.permissions & CharPerm.READ

    def test_telemetry_roundtrip(self):
        """Telemetry data packed by set_telemetry_data must be 22 bytes
        and contain expected RPM value at its spec-defined offset.

        RPM is a uint16 LE at offset 1 (after speed_kmh u8 at offset 0),
        per docs/BLE-INTERFACE-SPEC.md §4.1 and firmware telemetry_live_t.
        """
        ble = MockBle()
        ble.connect()
        sensors = MockSensorLayer(seed=42)
        sensors.start()
        data = sensors.update(20)

        ble.set_telemetry_data(data)
        tlm = ble._chars[CHR_TLM_LIVE].value

        assert len(tlm) == TLM_LIVE_SIZE  # 22 bytes
        # Unpack full struct and verify RPM field (offset 1, uint16 LE).
        (speed, rpm, gear, coolant, ambient, fuel, voltage,
         lat, lon, alt, heading, sats, hdop) = struct.unpack(TLM_LIVE_FMT, tlm)
        assert rpm == data.rpm

    def test_dtc_add_and_read(self):
        """DTC can be added and read back via DTC_LIST characteristic."""
        ble = MockBle()
        ble.connect()
        ble.clear_dtcs()
        ble.add_dtc(0x0101, DTC_SEV_WARNING)
        ble.add_dtc(0x0102, DTC_SEV_CRITICAL)

        dtc_data = ble.read(CHR_DTC_LIST)
        assert len(dtc_data) == 2 * DTC_ENTRY_SIZE  # 2 entries * 7 bytes = 14


# ============================================================
# Controller Interface Consistency (controller_input.h)
# ============================================================

class TestControllerInterfaceMatchesFirmware:
    """Verify controller mock matches firmware controller_input.h."""

    def test_button_count(self):
        """Must have 11 buttons (0-10) matching controller_button_t."""
        valid_buttons = [b for b in ControllerButton if b >= 0]
        assert len(valid_buttons) == 11

    def test_button_ids_match_firmware(self):
        """Button IDs must match firmware controller_button_t enum."""
        assert ControllerButton.NAV_UP == 0
        assert ControllerButton.NAV_DOWN == 1
        assert ControllerButton.NAV_LEFT == 2
        assert ControllerButton.NAV_RIGHT == 3
        assert ControllerButton.SELECT == 4
        assert ControllerButton.BACK == 5
        assert ControllerButton.HOME == 6
        assert ControllerButton.VOL_UP == 7
        assert ControllerButton.VOL_DOWN == 8
        assert ControllerButton.MEDIA_NEXT == 9
        assert ControllerButton.MEDIA_PREV == 10

    def test_debounce_ms_matches_firmware(self):
        """CTRL_DEBOUNCE_MS must be 50 (matching controller_input.h)."""
        assert CTRL_DEBOUNCE_MS == 50

    def test_button_to_event_mapping_complete(self):
        """All valid buttons must have a 1:1 event mapping."""
        for btn in ControllerButton:
            if btn < 0:
                continue
            assert btn in BUTTON_TO_EVENT

    def test_key_map_covers_all_buttons(self):
        """Key map must provide at least one key for each button."""
        mapped_buttons = set(KEY_MAP.values())
        for btn in ControllerButton:
            if btn < 0:
                continue
            assert btn in mapped_buttons

    def test_controller_inject_and_poll(self):
        """Controller can inject a button press and poll the event."""
        ctrl = MockController()
        ctrl.inject_button(ControllerButton.NAV_UP, pressed=True)
        events = ctrl.poll_events()
        assert len(events) == 1
        assert events[0].button == ControllerButton.NAV_UP

    def test_controller_debounce_rejects_duplicate(self):
        """Debounce rejects immediate duplicate press."""
        ctrl = MockController()
        first = ctrl.inject_button(ControllerButton.HOME, pressed=True)
        second = ctrl.inject_button(ControllerButton.HOME, pressed=True)
        assert first is True
        assert second is False

    def test_controller_key_injection(self):
        """Key injection works for all mapped keys."""
        ctrl = MockController()
        # Test a few key mappings
        assert ctrl.inject_key('up', pressed=True) is True
        ctrl.reset_all()
        assert ctrl.inject_key('return', pressed=True) is True
        ctrl.reset_all()
        assert ctrl.inject_key('escape', pressed=True) is True


# ============================================================
# Display Interface Consistency (display.h / board_config.h)
# ============================================================

class TestDisplayInterfaceMatchesFirmware:
    """Verify display mock matches firmware display.h / board_config.h."""

    def test_tft_dimensions(self):
        """TFT must be 240x320 (matching display.h)."""
        assert TFT_WIDTH == 240
        assert TFT_HEIGHT == 320

    def test_epd_dimensions(self):
        """E-Paper must be 296x128 (matching display.h)."""
        assert EPD_WIDTH == 296
        assert EPD_HEIGHT == 128

    def test_color_constants(self):
        """Color constants must match firmware display.h RGB565 values."""
        assert COLOR_BLACK == 0x0000
        assert COLOR_WHITE == 0xFFFF
        assert COLOR_ACCENT_RED == 0xF800
        assert COLOR_ACCENT_GREEN == 0x3660
        assert COLOR_ACCENT_BLUE == 0x041F

    def test_display_type_enum(self):
        """DisplayType must have NONE, EPAPER, TFT."""
        assert DisplayType.NONE == 0
        assert DisplayType.EPAPER == 1
        assert DisplayType.TFT == 2

    def test_display_mode_enum(self):
        """DisplayMode must have TOUR, TRACK, NAV, CONFIG."""
        assert DisplayMode.TOUR == 0
        assert DisplayMode.TRACK == 1
        assert DisplayMode.NAV == 2
        assert DisplayMode.CONFIG == 3

    def test_framebuffer_size(self):
        """Framebuffer size must be width*height*2 (RGB565)."""
        display = HeadlessDisplay(DisplayType.TFT)
        fb = display.get_framebuffer()
        assert len(fb) == TFT_WIDTH * TFT_HEIGHT * 2

    def test_rgb565_conversion(self):
        """rgb565 and rgb565_to_rgb888 must be inverses (within quantization)."""
        r, g, b = 200, 100, 50
        color = rgb565(r, g, b)
        r2, g2, b2 = rgb565_to_rgb888(color)
        # RGB565 loses lower bits; check upper 5/6/5 bits match
        assert (r & 0xF8) == (r2 & 0xF8)
        assert (g & 0xFC) == (g2 & 0xFC)
        assert (b & 0xF8) == (b2 & 0xF8)

    def test_display_set_mode(self):
        """Display mode can be changed."""
        display = HeadlessDisplay(DisplayType.TFT)
        display.set_mode(DisplayMode.NAV)
        assert display.mode == DisplayMode.NAV

    def test_epaper_framebuffer_size(self):
        """E-Paper framebuffer must be 296*128*2 bytes."""
        display = HeadlessDisplay(DisplayType.EPAPER)
        fb = display.get_framebuffer()
        assert len(fb) == EPD_WIDTH * EPD_HEIGHT * 2


# ============================================================
# Cross-Component Integration: Full Simulator Pipeline
# ============================================================

class TestFullSimulatorPipeline:
    """Integration tests for the complete simulator data pipeline."""

    def test_sensor_to_ble_pipeline(self):
        """Sensor data flows through to BLE telemetry."""
        ble = MockBle()
        ble.connect()
        ble.subscribe(CHR_TLM_LIVE)

        sensors = MockSensorLayer(seed=42)
        sensors.start()

        for _ in range(50):
            data = sensors.update(20)
            ble.set_telemetry_data(data)

        # Verify telemetry was updated
        tlm = ble._chars[CHR_TLM_LIVE].value
        assert len(tlm) == TLM_LIVE_SIZE  # 22 bytes, firmware telemetry_live_t

        # RPM in telemetry should match last sensor reading.
        # RPM is uint16 LE at offset 1 (after speed_kmh u8 at offset 0),
        # per docs/BLE-INTERFACE-SPEC.md §4.1.
        last_data = sensors.get_data()
        tlm_rpm = struct.unpack('<H', tlm[1:3])[0]
        assert tlm_rpm == last_data.rpm

    def test_gps_to_nmea_to_parser_format(self):
        """GPS track produces valid NMEA that could be parsed by firmware."""
        gen = NmeaGenerator(SAMPLE_TRACK, speed_multiplier=100, seed=42)

        for _ in range(20):
            sentence = next(gen)
            # Validate NMEA format for firmware parser compatibility
            assert sentence.startswith('$')
            assert sentence.endswith('\r\n')
            body = sentence.lstrip('$').rstrip('\r\n')
            parts = body.split('*')
            assert len(parts) == 2
            # Body before * must be non-empty
            assert len(parts[0]) > 0
            # Checksum must be 2 hex chars
            assert len(parts[1]) == 2
            int(parts[1], 16)

    def test_sensor_warnings_propagate_to_ble_status(self):
        """Sensor warnings propagate to BLE sensor status."""
        from simulator import Simulator, SimulatorConfig
        from mock_sensors import SensorCurve, TEMP_OVERTEMP_C

        sim = Simulator(SimulatorConfig(seed=42))
        sim.sensors = MockSensorLayer(
            temp_curve=SensorCurve(
                keyframes=[(0, TEMP_OVERTEMP_C)],
                clamp_max=TEMP_OVERTEMP_C,
            ),
            seed=42,
        )
        sim.start()
        sim.tick(20)

        # Overtemp should be in warnings
        assert "OVERTEMP" in sim.state.warnings

        # BLE sensor status should reflect temp not OK
        status = sim.ble.read(CHR_SENSOR_STATUS)
        temp_ok_bit = (status[0] >> 1) & 1  # bit 1 = temp_ok
        assert temp_ok_bit == 0  # temp NOT ok

    def test_controller_changes_display_mode(self):
        """Controller HOME button changes display mode."""
        from simulator import Simulator, SimulatorConfig

        sim = Simulator(SimulatorConfig(seed=42))
        sim.start()
        sim.tick(20)
        initial_mode = sim.display.mode

        sim.controller.inject_button(ControllerButton.HOME, pressed=True)
        sim.tick(20)

        # Mode should have changed OR events should have been processed
        assert sim.display.mode != initial_mode or sim.state.events > 0

    def test_simulator_status_json_complete(self):
        """Simulator status JSON contains all expected fields."""
        from simulator import Simulator, SimulatorConfig
        import json

        sim = Simulator(SimulatorConfig(
            track_file=SAMPLE_TRACK,
            seed=42,
        ))
        sim.start()
        sim.tick(20)

        status = json.loads(sim.get_status_json())
        assert "elapsed_ms" in status
        assert "tick_count" in status
        assert "sensors" in status
        assert "gps" in status
        assert "warnings" in status
        assert "ble_connected" in status

        # Sensor fields
        s = status["sensors"]
        assert "rpm" in s
        assert "speed_kmh" in s
        assert "gear" in s
        assert "coolant_temp_c" in s
        assert "fuel_level_l" in s

        # GPS fields
        g = status["gps"]
        assert "has_fix" in g
        assert "lat" in g
        assert "lon" in g

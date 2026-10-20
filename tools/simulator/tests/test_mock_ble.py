"""
test_mock_ble.py — Tests fuer den Mock-BLE GATT-Server

Testet:
  1. Verbindung (connect/disconnect)
  2. GATT-Characteristic-Read
  3. GATT-Characteristic-Write
  4. Permissions (read-only, write-only)
  5. Subscribe/Notify
  6. DTC-Verwaltung
  7. Sensor-Status
  8. Telemetry-Packing
  9. Service-Listing
"""

import struct
import pytest
from mock_ble import (
    MockBle, GattCharacteristic, CharPerm, DtcEntry,
    chr_uuid, svc_uuid,
    CHR_TLM_LIVE, CHR_TLM_SNAPSHOT, CHR_MEDIA_CONTROL, CHR_MEDIA_VOLUME,
    CHR_DTC_LIST, CHR_SENSOR_STATUS, CHR_SELF_TEST, CHR_BOND_MGMT,
    CHR_NAV_MANEUVER, CHR_CONFIG_WRITE,
    SVC_NAVIGATION, SVC_TELEMETRY, SVC_DIAGNOSE,
    DTC_SEV_WARNING, DTC_SEV_CRITICAL,
)


class TestBleConnection:
    """Test der BLE-Verbindung."""

    def test_connect(self):
        ble = MockBle()
        assert not ble.is_connected
        ble.connect()
        assert ble.is_connected

    def test_disconnect(self):
        ble = MockBle()
        ble.connect()
        ble.disconnect()
        assert not ble.is_connected

    def test_read_requires_connection(self):
        ble = MockBle()
        with pytest.raises(ConnectionError):
            ble.read(CHR_TLM_SNAPSHOT)

    def test_write_requires_connection(self):
        ble = MockBle()
        with pytest.raises(ConnectionError):
            ble.write(CHR_MEDIA_CONTROL, b"\x01")


class TestGattRead:
    """Test des GATT-Read."""

    def test_read_snapshot(self):
        """TLM Snapshot ist lesbar."""
        ble = MockBle()
        ble.connect()
        data = ble.read(CHR_TLM_SNAPSHOT)
        assert isinstance(data, bytes)

    def test_read_unknown_characteristic(self):
        """Unbekannte Characteristic wirft KeyError."""
        ble = MockBle()
        ble.connect()
        with pytest.raises(KeyError):
            ble.read(0xFFFF)

    def test_read_write_only_fails(self):
        """Write-only Characteristic kann nicht gelesen werden."""
        ble = MockBle()
        ble.connect()
        with pytest.raises(PermissionError):
            ble.read(CHR_CONFIG_WRITE)


class TestGattWrite:
    """Test des GATT-Write."""

    def test_write_media_control(self):
        """Media Control ist schreibbar."""
        ble = MockBle()
        ble.connect()
        ble.write(CHR_MEDIA_CONTROL, b"\x01")
        assert ble._chars[CHR_MEDIA_CONTROL].value == b"\x01"

    def test_write_volume(self):
        """Media Volume ist les- und schreibbar."""
        ble = MockBle()
        ble.connect()
        ble.write(CHR_MEDIA_VOLUME, b"\x50")
        assert ble.read(CHR_MEDIA_VOLUME) == b"\x50"

    def test_write_unknown_fails(self):
        """Unbekannte Characteristic wirft KeyError beim Write."""
        ble = MockBle()
        ble.connect()
        with pytest.raises(KeyError):
            ble.write(0xFFFF, b"\x00")

    def test_write_read_only_fails(self):
        """Read-only Characteristic kann nicht beschrieben werden."""
        ble = MockBle()
        ble.connect()
        with pytest.raises(PermissionError):
            ble.write(CHR_TLM_SNAPSHOT, b"\x00")

    def test_self_test_trigger(self):
        """Self-Test Write setzt Ergebnis auf pass."""
        ble = MockBle()
        ble.connect()
        ble.write(CHR_SELF_TEST, b"\x01")
        result = ble._chars[CHR_SELF_TEST].value
        assert struct.unpack("<I", result)[0] == 0


class TestSubscribeNotify:
    """Test Subscribe/Notify."""

    def test_subscribe_notify(self):
        """Subscribe + Notify funktioniert."""
        ble = MockBle()
        ble.connect()
        ble.subscribe(CHR_TLM_LIVE)
        result = ble.notify(CHR_TLM_LIVE, b"\x01\x02\x03")
        assert result is True

    def test_notify_without_subscribe_fails(self):
        """Notify ohne Subscribe wird ignoriert."""
        ble = MockBle()
        ble.connect()
        result = ble.notify(CHR_TLM_LIVE, b"\x01\x02\x03")
        assert result is False

    def test_unsubscribe(self):
        """Nach Unsubscribe wird Notify ignoriert."""
        ble = MockBle()
        ble.connect()
        ble.subscribe(CHR_TLM_LIVE)
        ble.unsubscribe(CHR_TLM_LIVE)
        result = ble.notify(CHR_TLM_LIVE, b"\x01")
        assert result is False

    def test_subscribe_non_notifiable_fails(self):
        """Subscribe auf nicht-notifiable Characteristic wirft Fehler."""
        ble = MockBle()
        ble.connect()
        with pytest.raises(PermissionError):
            ble.subscribe(CHR_CONFIG_WRITE)


class TestDtcManagement:
    """Test der DTC-Verwaltung."""

    def test_add_dtc(self):
        """DTC kann hinzugefuegt werden."""
        ble = MockBle()
        ble.connect()
        ble.subscribe(CHR_DTC_LIST)
        ble.add_dtc(0x0101, DTC_SEV_WARNING)
        data = ble.read(CHR_DTC_LIST)
        assert len(data) == 6  # 6 bytes per DTC

    def test_clear_dtcs(self):
        """DTCs koennen geloescht werden."""
        ble = MockBle()
        ble.connect()
        ble.add_dtc(0x0101)
        ble.add_dtc(0x0202, DTC_SEV_CRITICAL)
        ble.clear_dtcs()
        data = ble.read(CHR_DTC_LIST)
        assert len(data) == 0

    def test_dtc_packing(self):
        """DTC wird als 6 Bytes gepackt."""
        dtc = DtcEntry(code=0x0102, severity=DTC_SEV_WARNING, active=True, occurrences=3)
        packed = dtc.pack()
        assert len(packed) == 6
        code, sev, active, occ = struct.unpack("<HBHB", packed)
        assert code == 0x0102
        assert sev == DTC_SEV_WARNING
        assert active == 1
        assert occ == 3


class TestSensorStatus:
    """Test des Sensor-Status."""

    def test_set_sensor_status(self):
        """Sensor-Status kann gesetzt werden."""
        ble = MockBle()
        ble.connect()
        ble.set_sensor_status(rpm_ok=True, temp_ok=True, fuel_ok=True,
                              gps_ok=True, imu_ok=True)
        data = ble.read(CHR_SENSOR_STATUS)
        assert len(data) == 5
        status_byte = data[0]
        assert status_byte == 0x1F  # alle 5 Bits gesetzt

    def test_sensor_status_partial(self):
        """Sensor-Status mit einigen Ausfällen."""
        ble = MockBle()
        ble.connect()
        ble.set_sensor_status(rpm_ok=True, temp_ok=False, fuel_ok=True,
                              gps_ok=False, imu_ok=True)
        data = ble.read(CHR_SENSOR_STATUS)
        status_byte = data[0]
        assert status_byte & 0x01  # rpm ok
        assert not (status_byte & 0x02)  # temp fail
        assert status_byte & 0x04  # fuel ok
        assert not (status_byte & 0x08)  # gps fail
        assert status_byte & 0x10  # imu ok


class TestTelemetryPacking:
    """Test der Telemetry-Packing."""

    def test_set_telemetry_data(self):
        """Telemetry-Daten werden als 22 Bytes gepackt."""
        from mock_sensors import SensorData
        ble = MockBle()
        ble.connect()
        sd = SensorData(
            rpm=4000, speed_kmh=80.0, coolant_temp_c=90.0,
            fuel_level_l=12.0, gear=3,
            roll_deg=5.0, pitch_deg=2.0,
        )
        ble.set_telemetry_data(sd)
        data = ble._chars[CHR_TLM_LIVE].value
        assert len(data) <= 22


class TestUuids:
    """Test der UUID-Konstruktion."""

    def test_char_uuid_format(self):
        """Characteristic-UUID hat korrektes Format."""
        uuid = chr_uuid(CHR_TLM_LIVE)
        assert "6e40" in uuid
        assert "5001" in uuid

    def test_service_uuid_format(self):
        """Service-UUID hat korrektes Format."""
        uuid = svc_uuid(SVC_TELEMETRY)
        assert "6e40" in uuid
        assert "5000" in uuid

    def test_uuid_uniqueness(self):
        """Verschiedene Characteristics haben verschiedene UUIDs."""
        u1 = chr_uuid(CHR_TLM_LIVE)
        u2 = chr_uuid(CHR_TLM_SNAPSHOT)
        assert u1 != u2


class TestServiceListing:
    """Test der Service-Listing-Ausgabe."""

    def test_list_services(self):
        """list_services gibt einen lesbaren String zurueck."""
        ble = MockBle()
        listing = ble.list_services()
        assert "Navigation" in listing
        assert "Telemetry" in listing
        assert "Diagnose" in listing
        assert "OTA" in listing

    def test_all_services_present(self):
        """Alle 8 Services sind definiert."""
        ble = MockBle()
        services = set()
        for ch in ble.characteristics:
            services.add(ch.char_id & 0xF000)
        assert len(services) == 8

"""
mock_ble.py — Mock-BLE: Kommandozeilen-Interface fuer GATT-Read/Write-Simulation

Simuliert den BLE GATT-Server (NimBLE) mit allen Services und
Characteristics aus der Firmware:
  firmware/components/ble/include/ble.h
  firmware/components/ble/gatt_uuids.h

Wire formats are regenerated from docs/BLE-INTERFACE-SPEC.md and match
the firmware structs in firmware/components/ble/ble_callbacks.c:
  - telemetry_live_t (22 bytes, CHR_TLM_LIVE 0x5001)
  - dtc_binary_t     (7 bytes/entry, CHR_DTC_LIST 0x7001)
  - sensor_status_t  (5 bytes, CHR_SENSOR_STATUS 0x7002)
  - self-test result  (u8 result + detail bytes, CHR_SELF_TEST 0x7003)

Erlaubt das Lesen und Schreiben von GATT-Characteristics ueber
ein Kommandozeilen-Interface. Notifications werden simuliert.

Services:
  0x1000 Navigation   0x5000 Telemetry
  0x2000 Media        0x6000 Config
  0x3000 Phone        0x7000 Diagnose
  0x4000 Weather       0x8000 OTA
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from typing import Dict, List, Optional, Callable, Any
from enum import IntEnum


# ---- Service/Characteristic UUIDs (aus gatt_uuids.h) ----

SVC_NAVIGATION = 0x1000
SVC_MEDIA = 0x2000
SVC_PHONE = 0x3000
SVC_WEATHER = 0x4000
SVC_TELEMETRY = 0x5000
SVC_CONFIG = 0x6000
SVC_DIAGNOSE = 0x7000
SVC_OTA = 0x8000

CHR_NAV_MANEUVER = 0x1001
CHR_NAV_ROUTE = 0x1002
CHR_NAV_ROAD = 0x1003
CHR_NAV_STATUS = 0x1004
CHR_NAV_CMD = 0x1005

CHR_MEDIA_NOWPLAYING = 0x2001
CHR_MEDIA_CONTROL = 0x2002
CHR_MEDIA_VOLUME = 0x2003

CHR_PHONE_INCOMING = 0x3001
CHR_PHONE_STATUS = 0x3002
CHR_PHONE_CONTROL = 0x3003
CHR_PHONE_NOTIF = 0x3004

CHR_WX_CURRENT = 0x4001
CHR_WX_FORECAST = 0x4002
CHR_WX_ALERT = 0x4003

CHR_TLM_LIVE = 0x5001
CHR_TLM_SNAPSHOT = 0x5002
CHR_TRACK_EXPORT = 0x5003
CHR_TRACK_EXPORT_CTRL = 0x5004

CHR_CONFIG_READ = 0x6001
CHR_CONFIG_WRITE = 0x6002
CHR_CONFIG_EXPORT = 0x6003
CHR_CONFIG_IMPORT = 0x6004
CHR_BOND_MGMT = 0x6005

CHR_DTC_LIST = 0x7001
CHR_SENSOR_STATUS = 0x7002
CHR_SELF_TEST = 0x7003
CHR_LOG_EXPORT = 0x7004
CHR_LOG_EXPORT_CTRL = 0x7005

CHR_OTA_CONTROL = 0x8001
CHR_OTA_DATA = 0x8002
CHR_OTA_VERIFY = 0x8003

# Base UUID format
BASE_UUID = "6e40{:04x}-b5a3-f393-e0a9-e50e24dcca9e"


def chr_uuid(char_id: int) -> str:
    """Vollstaendige 128-bit UUID fuer eine 16-bit Characteristic-ID."""
    return BASE_UUID.format(char_id)


def svc_uuid(svc_id: int) -> str:
    """Vollstaendige 128-bit UUID fuer eine 16-bit Service-ID."""
    return BASE_UUID.format(svc_id)


# ---- Permissions ----

class CharPerm(IntEnum):
    READ = 1
    WRITE = 2
    NOTIFY = 4
    INDICATE = 8
    WRITE_NR = 16


# ---- DTC / Diagnose Enums (aus gatt_uuids.h) ----

MANEUVER_STRAIGHT = 0
MANEUVER_SLIGHT_LEFT = 1
MANEUVER_LEFT = 2
MANEUVER_SHARP_LEFT = 3
MANEUVER_SLIGHT_RIGHT = 4
MANEUVER_RIGHT = 5
MANEUVER_SHARP_RIGHT = 6
MANEUVER_UTURN = 7
MANEUVER_ARRIVE = 8
MANEUVER_FORK = 9

NAV_STATUS_INACTIVE = 0
NAV_STATUS_ACTIVE = 1
NAV_STATUS_REROUTING = 2
NAV_STATUS_ARRIVING = 3
NAV_STATUS_OFFLINE = 4

DTC_SEV_INFO = 0
DTC_SEV_WARNING = 1
DTC_SEV_CRITICAL = 2

# ---- Packed payload sizes (match firmware _Static_assert in ble_callbacks.c) ----
# telemetry_live_t: speed(u8) rpm(u16) gear(i8) coolant(i8) ambient(i8)
#   fuel(u8) voltage(u8) lat(i32) lon(i32) alt(i16) heading(u16) sats(u8) hdop(u8)
TLM_LIVE_SIZE = 22
TLM_LIVE_FMT = "<BHbbbBBiihHBB"

# dtc_binary_t: code(u16) severity(u8) timestamp(u32)
# NOTE: the spec §8.1 header says "6 bytes/entry" and the firmware
# _Static_assert says == 6, but uint16_t+uint8_t+uint32_t packed = 7 bytes.
# The field-layout table in the spec (offset 0:u16, 2:u8, 3:u32) confirms 7.
# The _Static_assert(sizeof(dtc_binary_t)==6) is a firmware bug (never compiled).
DTC_ENTRY_SIZE = 7
DTC_ENTRY_FMT = "<HBI"

# sensor_status_t: 5 distinct bytes (gps, rpm, temp, fuel, sd_card)
SENSOR_STATUS_SIZE = 5

# Self-test result: u8 result (0=pass, 1=warn, 2=fail) + detail bytes
SELF_TEST_RESULT_PASS = 0
SELF_TEST_RESULT_WARN = 1
SELF_TEST_RESULT_FAIL = 2


# ---- Sensor status values (from spec §8.2, sensor_status_t in ble_callbacks.c) ----

SENSOR_GPS_OK = 0
SENSOR_GPS_NO_FIX = 1
SENSOR_GPS_ERROR = 2

SENSOR_RPM_OK = 0
SENSOR_RPM_NO_SIGNAL = 1
SENSOR_RPM_ERROR = 2

SENSOR_TEMP_OK = 0
SENSOR_TEMP_DISCONNECTED = 1
SENSOR_TEMP_ERROR = 2

SENSOR_FUEL_OK = 0
SENSOR_FUEL_ERROR = 1

SENSOR_SD_OK = 0
SENSOR_SD_NOT_PRESENT = 1
SENSOR_SD_FULL = 2
SENSOR_SD_ERROR = 3


@dataclass
class GattCharacteristic:
    """Eine GATT-Characteristic mit Wert und Berechtigungen."""
    uuid: str
    char_id: int
    name: str
    permissions: int
    value: bytes = b""
    subscribed: bool = False
    on_write: Optional[Callable[[bytes], None]] = None

    @property
    def readable(self) -> bool:
        return bool(self.permissions & CharPerm.READ)

    @property
    def writable(self) -> bool:
        return bool(self.permissions & (CharPerm.WRITE | CharPerm.WRITE_NR))

    @property
    def notifiable(self) -> bool:
        return bool(self.permissions & (CharPerm.NOTIFY | CharPerm.INDICATE))


@dataclass
class DtcEntry:
    """Ein Diagnostic Trouble Code (7 bytes, matches firmware dtc_binary_t).

    Wire format (little-endian, packed):
      offset 0: u16 code
      offset 2: u8  severity (0=info, 1=warning, 2=critical)
      offset 3: u32 timestamp (Unix seconds)
    """
    code: int
    severity: int = DTC_SEV_WARNING
    timestamp: int = 0

    def pack(self) -> bytes:
        return struct.pack(DTC_ENTRY_FMT, self.code, self.severity, self.timestamp)


class MockBle:
    """
    Mock BLE GATT-Server.

    Verwaltet alle Services und Characteristics, erlaubt
    Read/Write/Subscribe-Operationen ueber CLI.

    Usage:
        ble = MockBle()
        ble.connect()
        value = ble.read(CHR_TLM_SNAPSHOT)
        ble.write(CHR_MEDIA_CONTROL, b'\\x01')
    """

    def __init__(self):
        self._connected = False
        self._chars: Dict[int, GattCharacteristic] = {}
        self._dtcs: List[DtcEntry] = []
        self._sensor_status = bytes([0, 0, 0, 0, 0])  # all-ok sensor_status_t
        self._gps_data: Any = None  # set via set_gps_data for telemetry packing
        self._battery_voltage: float = 12.6  # for voltage_tenth field
        self._telemetry_callback: Optional[Callable] = None
        self._init_characteristics()

    def _init_characteristics(self) -> None:
        """Alle Characteristics aus der Firmware definieren."""
        defs = [
            # Navigation
            (CHR_NAV_MANEUVER,    "Nav Maneuver",     CharPerm.READ | CharPerm.NOTIFY),
            (CHR_NAV_ROUTE,        "Nav Route",        CharPerm.READ | CharPerm.NOTIFY),
            (CHR_NAV_ROAD,         "Nav Road",         CharPerm.READ | CharPerm.NOTIFY),
            (CHR_NAV_STATUS,       "Nav Status",       CharPerm.READ | CharPerm.NOTIFY),
            (CHR_NAV_CMD,          "Nav Cmd",          CharPerm.WRITE | CharPerm.WRITE_NR),
            # Media
            (CHR_MEDIA_NOWPLAYING, "Media NowPlaying", CharPerm.READ | CharPerm.NOTIFY),
            (CHR_MEDIA_CONTROL,    "Media Control",    CharPerm.WRITE | CharPerm.WRITE_NR),
            (CHR_MEDIA_VOLUME,     "Media Volume",     CharPerm.READ | CharPerm.NOTIFY | CharPerm.WRITE),
            # Phone
            (CHR_PHONE_INCOMING,   "Phone Incoming",   CharPerm.NOTIFY),
            (CHR_PHONE_STATUS,     "Phone Status",     CharPerm.READ | CharPerm.NOTIFY),
            (CHR_PHONE_CONTROL,     "Phone Control",    CharPerm.WRITE),
            (CHR_PHONE_NOTIF,      "Phone Notif",      CharPerm.NOTIFY),
            # Weather
            (CHR_WX_CURRENT,       "WX Current",       CharPerm.READ | CharPerm.NOTIFY),
            (CHR_WX_FORECAST,       "WX Forecast",      CharPerm.READ | CharPerm.NOTIFY),
            (CHR_WX_ALERT,          "WX Alert",         CharPerm.NOTIFY),
            # Telemetry
            (CHR_TLM_LIVE,         "TLM Live",         CharPerm.NOTIFY),
            (CHR_TLM_SNAPSHOT,     "TLM Snapshot",     CharPerm.READ),
            (CHR_TRACK_EXPORT,     "Track Export",     CharPerm.NOTIFY),
            (CHR_TRACK_EXPORT_CTRL, "Track Export Ctrl", CharPerm.WRITE),
            # Config
            (CHR_CONFIG_READ,      "Config Read",      CharPerm.READ | CharPerm.NOTIFY),
            (CHR_CONFIG_WRITE,      "Config Write",    CharPerm.WRITE),
            (CHR_CONFIG_EXPORT,    "Config Export",    CharPerm.NOTIFY),
            (CHR_CONFIG_IMPORT,    "Config Import",    CharPerm.WRITE),
            (CHR_BOND_MGMT,        "Bond Mgmt",        CharPerm.WRITE),
            # Diagnose
            (CHR_DTC_LIST,         "DTC List",         CharPerm.READ | CharPerm.NOTIFY),
            (CHR_SENSOR_STATUS,    "Sensor Status",    CharPerm.READ | CharPerm.NOTIFY),
            (CHR_SELF_TEST,        "Self Test",        CharPerm.WRITE | CharPerm.NOTIFY),
            (CHR_LOG_EXPORT,       "Log Export",       CharPerm.NOTIFY),
            (CHR_LOG_EXPORT_CTRL,  "Log Export Ctrl",  CharPerm.WRITE),
            # OTA
            (CHR_OTA_CONTROL,      "OTA Control",     CharPerm.WRITE | CharPerm.NOTIFY),
            (CHR_OTA_DATA,          "OTA Data",         CharPerm.WRITE_NR),
            (CHR_OTA_VERIFY,       "OTA Verify",       CharPerm.NOTIFY),
        ]
        for char_id, name, perms in defs:
            self._chars[char_id] = GattCharacteristic(
                uuid=chr_uuid(char_id),
                char_id=char_id,
                name=name,
                permissions=perms,
            )

    def connect(self) -> None:
        self._connected = True

    def disconnect(self) -> None:
        self._connected = False
        for ch in self._chars.values():
            ch.subscribed = False

    @property
    def is_connected(self) -> bool:
        return self._connected

    def read(self, char_id: int) -> bytes:
        """Characteristic-Wert lesen."""
        if not self._connected:
            raise ConnectionError("BLE not connected")
        ch = self._chars.get(char_id)
        if ch is None:
            raise KeyError(f"Unknown characteristic 0x{char_id:04X}")
        if not ch.readable:
            raise PermissionError(f"{ch.name} is not readable")
        if char_id == CHR_DTC_LIST:
            return b"".join(dtc.pack() for dtc in self._dtcs)
        if char_id == CHR_SENSOR_STATUS:
            return self._sensor_status
        return ch.value

    def write(self, char_id: int, data: bytes, response: bool = True) -> None:
        """Characteristic-Wert schreiben."""
        if not self._connected:
            raise ConnectionError("BLE not connected")
        ch = self._chars.get(char_id)
        if ch is None:
            raise KeyError(f"Unknown characteristic 0x{char_id:04X}")
        if not ch.writable:
            raise PermissionError(f"{ch.name} is not writable")
        ch.value = data
        if ch.on_write:
            ch.on_write(data)
        # Special handlers
        if char_id == CHR_BOND_MGMT and data == b"\x00":
            pass  # Delete all bonds
        if char_id == CHR_SELF_TEST:
            # Spec §8.3: write 0x00=Quick / 0x01=Full to trigger; result
            # notified as u8 result (0=pass,1=warn,2=fail) + one detail byte
            # per sensor. We simulate a pass with no detail issues.
            ch.value = struct.pack("<BB", SELF_TEST_RESULT_PASS, 0)

    def subscribe(self, char_id: int) -> None:
        """Notifications fuer eine Characteristic abonnieren."""
        ch = self._chars.get(char_id)
        if ch is None:
            raise KeyError(f"Unknown characteristic 0x{char_id:04X}")
        if not ch.notifiable:
            raise PermissionError(f"{ch.name} does not support notifications")
        ch.subscribed = True

    def unsubscribe(self, char_id: int) -> None:
        ch = self._chars.get(char_id)
        if ch and ch.subscribed:
            ch.subscribed = False

    def notify(self, char_id: int, data: bytes) -> bool:
        """Notification senden (wenn subscribed)."""
        ch = self._chars.get(char_id)
        if ch is None or not ch.subscribed:
            return False
        ch.value = data
        return True

    def set_telemetry_data(self, sensor_data: Any,
                           gps_data: Any = None) -> None:
        """Live-Telemetry (22 Bytes packed) aus SensorData [+ GpsData] setzen.

        Matches firmware telemetry_live_t (ble_callbacks.c:57):
          speed_kmh(u8) rpm(u16) gear(i8) coolant_temp_c(i8)
          ambient_temp_c(i8) fuel_percent(u8) voltage_tenth(u8)
          latitude_e7(i32) longitude_e7(i32) altitude_m(i16)
          heading_tenth(u16) satellites(u8) hdop_tenth(u8)

        gps_data overrides the internal GPS cache (set via set_gps_data).
        If neither is available, GPS fields are zeroed (matching firmware
        behavior when g->valid is false).
        """
        try:
            speed = int(sensor_data.speed_kmh + 0.5)
            rpm = sensor_data.rpm
            gear = -1 if sensor_data.gear == 0 else sensor_data.gear
            coolant = int(sensor_data.coolant_temp_c)
            ambient = int(sensor_data.temp_c)
            fuel = int(sensor_data.fuel_level_l + 0.5)
            voltage = int(self._battery_voltage * 10.0 + 0.5)

            g = gps_data if gps_data is not None else self._gps_data
            if g is not None and getattr(g, "valid", False):
                lat_e7 = int(g.lat * 1e7)
                lon_e7 = int(g.lon * 1e7)
                altitude = int(g.altitude_m)
                heading = int(g.course_deg * 10.0)
                sats = g.satellites
                hdop = int(g.hdop * 10.0 + 0.5)
            else:
                lat_e7 = 0
                lon_e7 = 0
                altitude = 0
                heading = 0
                sats = 0
                hdop = 0

            packed = struct.pack(
                TLM_LIVE_FMT,
                speed, rpm, gear, coolant, ambient,
                fuel, voltage,
                lat_e7, lon_e7, altitude, heading,
                sats, hdop,
            )
            self._chars[CHR_TLM_LIVE].value = packed
        except (AttributeError, struct.error):
            pass

    def set_gps_data(self, gps_data: Any) -> None:
        """GPS-Daten fuer Telemetry-Packing setzen (entspricht ble_on_gps_update)."""
        self._gps_data = gps_data

    def set_battery_voltage(self, voltage: float) -> None:
        """Batteriespannung fuer voltage_tenth-Feld setzen."""
        self._battery_voltage = voltage

    def set_sensor_status(self, gps: int = SENSOR_GPS_OK,
                          rpm: int = SENSOR_RPM_OK,
                          temp: int = SENSOR_TEMP_OK,
                          fuel: int = SENSOR_FUEL_OK,
                          sd_card: int = SENSOR_SD_OK) -> None:
        """5-Byte Sensor-Health-Status setzen (sensor_status_t, spec §8.2).

        Each byte is a distinct status value, NOT a bitfield:
          byte 0: gps      (0=ok, 1=no_fix, 2=error)
          byte 1: rpm      (0=ok, 1=no_signal, 2=error)
          byte 2: temp      (0=ok, 1=disconnected, 2=error)
          byte 3: fuel      (0=ok, 1=error)
          byte 4: sd_card   (0=ok, 1=not_present, 2=full, 3=error)
        """
        self._sensor_status = struct.pack("<BBBBB", gps, rpm, temp, fuel, sd_card)

    def add_dtc(self, code: int, severity: int = DTC_SEV_WARNING,
                timestamp: int = 0) -> None:
        """DTC hinzufuegen (code, severity, timestamp)."""
        self._dtcs.append(DtcEntry(code=code, severity=severity,
                                   timestamp=timestamp))
        self.notify(CHR_DTC_LIST, b"".join(d.pack() for d in self._dtcs))

    def clear_dtcs(self) -> None:
        self._dtcs.clear()
        self._chars[CHR_DTC_LIST].value = b""

    @property
    def characteristics(self) -> List[GattCharacteristic]:
        return list(self._chars.values())

    def list_services(self) -> str:
        """Human-readable Service/Characteristic-Liste."""
        lines = []
        current_svc = 0
        for ch in self._chars.values():
            svc = ch.char_id & 0xF000
            if svc != current_svc:
                current_svc = svc
                svc_names = {
                    SVC_NAVIGATION: "Navigation", SVC_MEDIA: "Media",
                    SVC_PHONE: "Phone", SVC_WEATHER: "Weather",
                    SVC_TELEMETRY: "Telemetry", SVC_CONFIG: "Config",
                    SVC_DIAGNOSE: "Diagnose", SVC_OTA: "OTA",
                }
                lines.append(f"\n[{svc_uuid(svc)}] {svc_names.get(svc, 'Unknown')}")
            perms = []
            if ch.readable:
                perms.append("R")
            if ch.writable:
                perms.append("W")
            if ch.notifiable:
                perms.append("N")
            lines.append(
                f"  0x{ch.char_id:04X} {ch.name:20s} [{''.join(perms)}] {ch.uuid}"
            )
        return "\n".join(lines)

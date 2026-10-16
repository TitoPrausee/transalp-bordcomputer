/**
 * binary-parsers.test.ts — Tests fuer BLE Binary-Parser
 *
 * Testet encode/decode Roundtrips fuer alle binaren Datenformate:
 *   - parseTelemetryLive / encodeTelemetryLive (22 bytes)
 *   - parseSensorStatus / encodeSensorStatus (5 bytes)
 *   - parseDtcList / encodeDtcList (6 bytes per entry)
 *   - parseSystemStatus / encodeSystemStatus (17 bytes)
 *   - base64 conversion helpers
 *   - Edge-Cases (leer, saturation, negative Werte)
 */

import {
  parseTelemetryLive,
  encodeTelemetryLive,
  parseSensorStatus,
  encodeSensorStatus,
  parseDtcList,
  encodeDtcList,
  parseSystemStatus,
  encodeSystemStatus,
  encodeSelfTestCommand,
  parseSelfTestResult,
  encodeDtcClearCommand,
  base64ToDataView,
  dataViewToBase64,
  uint8ArrayToBase64,
} from './binary-parsers';
import { SelfTestMode } from './gatt-uuids';

// ============================================================
// Telemetry Live (22 bytes)
// ============================================================

describe('parseTelemetryLive', () => {
  it('should parse a valid 22-byte telemetry packet', () => {
    const buf = new ArrayBuffer(22);
    const dv = new DataView(buf);

    dv.setUint8(0, 120);           // speed
    dv.setUint16(1, 6500, true);   // rpm (LE)
    dv.setInt8(3, 3);              // gear
    dv.setInt8(4, 85);             // coolantTemp
    dv.setInt8(5, 22);             // ambientTemp
    dv.setUint8(6, 80);           // fuel
    dv.setUint8(7, 126);          // voltage
    dv.setInt32(8, 509353900, true);  // latitude
    dv.setInt32(12, 115872400, true); // longitude
    dv.setInt16(16, 350, true);   // altitude
    dv.setUint16(18, 2700, true); // heading
    dv.setUint8(20, 9);           // satellites
    dv.setUint8(21, 12);          // hdop

    const result = parseTelemetryLive(dv);

    expect(result.speed).toBe(120);
    expect(result.rpm).toBe(6500);
    expect(result.gear).toBe(3);
    expect(result.coolantTemp).toBe(85);
    expect(result.ambientTemp).toBe(22);
    expect(result.fuel).toBe(80);
    expect(result.voltage).toBe(126);
    expect(result.latitude).toBe(509353900);
    expect(result.longitude).toBe(115872400);
    expect(result.altitude).toBe(350);
    expect(result.heading).toBe(2700);
    expect(result.satellites).toBe(9);
    expect(result.hdop).toBe(12);
  });

  it('should parse negative gear (-1 = neutral)', () => {
    const buf = new ArrayBuffer(22);
    const dv = new DataView(buf);
    dv.setInt8(3, -1);

    const result = parseTelemetryLive(dv);
    expect(result.gear).toBe(-1);
  });

  it('should parse negative temperature', () => {
    const buf = new ArrayBuffer(22);
    const dv = new DataView(buf);
    dv.setInt8(4, -15);

    const result = parseTelemetryLive(dv);
    expect(result.coolantTemp).toBe(-15);
  });
});

describe('encodeTelemetryLive', () => {
  it('should encode telemetry data to 22 bytes', () => {
    const data = {
      speed: 100,
      rpm: 5000,
      gear: 4,
      coolantTemp: 90,
      ambientTemp: 25,
      fuel: 75,
      voltage: 124,
      latitude: 509300000,
      longitude: 115800000,
      altitude: 400,
      heading: 1800,
      satellites: 8,
      hdop: 15,
    };

    const result = encodeTelemetryLive(data);
    expect(result.length).toBe(22);

    // Roundtrip: parse the encoded data
    const dv = new DataView(result.buffer);
    const parsed = parseTelemetryLive(dv);
    expect(parsed.speed).toBe(100);
    expect(parsed.rpm).toBe(5000);
    expect(parsed.gear).toBe(4);
    expect(parsed.coolantTemp).toBe(90);
    expect(parsed.fuel).toBe(75);
    expect(parsed.satellites).toBe(8);
  });

  it('should handle negative latitude (Southern hemisphere)', () => {
    const data = {
      speed: 0, rpm: 0, gear: 0, coolantTemp: 0, ambientTemp: 0,
      fuel: 0, voltage: 0,
      latitude: -339353900,
      longitude: -1515872400,
      altitude: 50, heading: 0, satellites: 0, hdop: 0,
    };

    const encoded = encodeTelemetryLive(data);
    const dv = new DataView(encoded.buffer);
    const parsed = parseTelemetryLive(dv);
    expect(parsed.latitude).toBe(-339353900);
    expect(parsed.longitude).toBe(-1515872400);
  });
});

// ============================================================
// Sensor Status (5 bytes)
// ============================================================

describe('parseSensorStatus / encodeSensorStatus', () => {
  it('should parse a 5-byte sensor status packet', () => {
    const buf = new ArrayBuffer(5);
    const dv = new DataView(buf);
    dv.setUint8(0, 0);  // gps: ok
    dv.setUint8(1, 1);  // rpm: no_signal
    dv.setUint8(2, 2);  // temp: error
    dv.setUint8(3, 0);  // fuel: ok
    dv.setUint8(4, 3);  // sdCard: error

    const result = parseSensorStatus(dv);
    expect(result.gps).toBe(0);
    expect(result.rpm).toBe(1);
    expect(result.temp).toBe(2);
    expect(result.fuel).toBe(0);
    expect(result.sdCard).toBe(3);
  });

  it('should encode and roundtrip sensor status', () => {
    const data = { gps: 1, rpm: 2, temp: 1, fuel: 1, sdCard: 2 };
    const encoded = encodeSensorStatus(data);
    expect(encoded.length).toBe(5);

    const dv = new DataView(encoded.buffer);
    const parsed = parseSensorStatus(dv);
    expect(parsed).toEqual(data);
  });
});

// ============================================================
// DTC List (6 bytes per entry)
// ============================================================

describe('parseDtcList / encodeDtcList', () => {
  it('should parse multiple DTC entries', () => {
    const buf = new ArrayBuffer(12);  // 2 entries * 6 bytes
    const dv = new DataView(buf);

    // Entry 1: code=0x0101, severity=1, timestamp=1000
    dv.setUint16(0, 0x0101, true);
    dv.setUint8(2, 1);
    dv.setUint8(3, 0xE8);  // 1000 & 0xFF
    dv.setUint8(4, 0x03);  // (1000 >> 8) & 0xFF
    dv.setUint8(5, 0x00);  // (1000 >> 16) & 0xFF

    // Entry 2: code=0x0401, severity=2, timestamp=2000
    dv.setUint16(6, 0x0401, true);
    dv.setUint8(8, 2);
    dv.setUint8(9, 0xD0);  // 2000 & 0xFF
    dv.setUint8(10, 0x07); // (2000 >> 8) & 0xFF
    dv.setUint8(11, 0x00); // (2000 >> 16) & 0xFF

    const result = parseDtcList(dv);
    expect(result.length).toBe(2);
    expect(result[0].code).toBe(0x0101);
    expect(result[0].severity).toBe(1);
    expect(result[0].timestamp).toBe(1000);
    expect(result[1].code).toBe(0x0401);
    expect(result[1].severity).toBe(2);
    expect(result[1].timestamp).toBe(2000);
  });

  it('should handle empty DTC list', () => {
    const buf = new ArrayBuffer(0);
    const dv = new DataView(buf);
    const result = parseDtcList(dv);
    expect(result.length).toBe(0);
  });

  it('should encode and roundtrip DTC list', () => {
    const entries = [
      { code: 0x0101, severity: 1, timestamp: 500 },
      { code: 0x0402, severity: 2, timestamp: 9999 },
    ];

    const encoded = encodeDtcList(entries);
    expect(encoded.length).toBe(12);

    const dv = new DataView(encoded.buffer);
    const parsed = parseDtcList(dv);
    expect(parsed.length).toBe(2);
    expect(parsed[0].code).toBe(0x0101);
    expect(parsed[0].timestamp).toBe(500);
    expect(parsed[1].code).toBe(0x0402);
    expect(parsed[1].timestamp).toBe(9999);
  });

  it('should encode DTC clear command (clear all)', () => {
    const cmd = encodeDtcClearCommand();
    expect(cmd.length).toBe(2);
    const dv = new DataView(cmd.buffer);
    expect(dv.getUint16(0, true)).toBe(0xFFFF);
  });

  it('should encode DTC clear command (specific code)', () => {
    const cmd = encodeDtcClearCommand(0x0101);
    expect(cmd.length).toBe(2);
    const dv = new DataView(cmd.buffer);
    expect(dv.getUint16(0, true)).toBe(0x0101);
  });
});

// ============================================================
// System Status (17 bytes)
// ============================================================

describe('parseSystemStatus / encodeSystemStatus', () => {
  it('should parse and roundtrip system status', () => {
    const data = {
      uptime: 3600,
      heapFree: 145000,
      heapTotal: 320000,
      batteryPct: 85,
      cpuTemp: 45,
      taskCount: 12,
      bleConnections: 1,
      wifiRssi: -60,
    };

    const encoded = encodeSystemStatus(data);
    expect(encoded.length).toBe(17);

    const dv = new DataView(encoded.buffer);
    const parsed = parseSystemStatus(dv);
    expect(parsed.uptime).toBe(3600);
    expect(parsed.heapFree).toBe(145000);
    expect(parsed.heapTotal).toBe(320000);
    expect(parsed.batteryPct).toBe(85);
    expect(parsed.cpuTemp).toBe(45);
    expect(parsed.taskCount).toBe(12);
    expect(parsed.bleConnections).toBe(1);
    expect(parsed.wifiRssi).toBe(-60);
  });
});

// ============================================================
// Self-Test
// ============================================================

describe('Self-Test encoding', () => {
  it('should encode self-test command (quick mode)', () => {
    const cmd = encodeSelfTestCommand(SelfTestMode.Quick);
    expect(cmd.length).toBe(1);
    expect(cmd[0]).toBe(0);
  });

  it('should encode self-test command (full mode)', () => {
    const cmd = encodeSelfTestCommand(SelfTestMode.Full);
    expect(cmd.length).toBe(1);
    expect(cmd[0]).toBe(1);
  });

  it('should parse self-test result', () => {
    const buf = new ArrayBuffer(6);
    const dv = new DataView(buf);
    dv.setUint8(0, 0);  // pass
    dv.setUint8(1, 0);  // GPS ok
    dv.setUint8(2, 0);  // RPM ok
    dv.setUint8(3, 2);  // Temp fail
    dv.setUint8(4, 0);  // Fuel ok
    dv.setUint8(5, 0);  // SD ok

    const result = parseSelfTestResult(dv);
    expect(result.result).toBe(0);
    expect(result.details.length).toBe(5);
    expect(result.details[3]).toBe(2);
  });
});

// ============================================================
// Base64 Helpers
// ============================================================

describe('base64 conversion helpers', () => {
  it('should roundtrip base64 -> DataView -> base64', () => {
    const bytes = new Uint8Array([0x01, 0x02, 0x03, 0xFF, 0x00, 0x80]);
    const b64 = uint8ArrayToBase64(bytes);
    expect(b64.length).toBeGreaterThan(0);

    const dv = base64ToDataView(b64);
    expect(dv.byteLength).toBe(6);
    expect(dv.getUint8(0)).toBe(0x01);
    expect(dv.getUint8(3)).toBe(0xFF);
    expect(dv.getUint8(4)).toBe(0x00);
    expect(dv.getUint8(5)).toBe(0x80);
  });

  it('should roundtrip DataView -> base64 -> DataView', () => {
    const buf = new ArrayBuffer(4);
    const dv = new DataView(buf);
    dv.setUint32(0, 0xDEADBEEF, true);

    const b64 = dataViewToBase64(dv);
    const restored = base64ToDataView(b64);
    expect(restored.getUint32(0, true)).toBe(0xDEADBEEF);
  });

  it('should handle empty array', () => {
    const empty = new Uint8Array(0);
    const b64 = uint8ArrayToBase64(empty);
    expect(b64).toBe('');
  });
});

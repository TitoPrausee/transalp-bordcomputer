/**
 * binary-parsers.test.ts — Tests fuer die BLE Binary-Parser
 *
 * Testet:
 *   - parseTelemetryLive: korrekte Feld-Extraktion aus Binary-Daten
 *   - parseSensorStatus: Sensor-Status-Werte
 *   - parseDtcList: DTC-Codes, Severity
 *   - encode/decode Roundtrips
 */

import { base64ToDataView, dataViewToBase64 } from '../binary-parsers';
import { parseTelemetryLive, encodeTelemetryLive } from '../binary-parsers';
import { parseSensorStatus, encodeSensorStatus } from '../binary-parsers';
import { parseDtcList, encodeDtcList } from '../binary-parsers';
import type { TelemetryLive, SensorStatus, DtcEntry } from '../gatt-uuids';

describe('base64 <-> DataView conversion', () => {
  test('roundtrip preserves data', () => {
    const original = 'SGVsbG8gV29ybGQ=';
    const dv = base64ToDataView(original);
    const result = dataViewToBase64(dv);
    expect(result).toBe(original);
  });

  test('empty string produces empty DataView', () => {
    const dv = base64ToDataView('');
    expect(dv.byteLength).toBe(0);
  });
});

describe('TelemetryLive parser', () => {
  test('roundtrip preserves speed and rpm', () => {
    const original: TelemetryLive = {
      speed: 120,
      rpm: 4500,
      gear: 4,
      coolantTemp: 85,
      ambientTemp: 22,
      fuel: 75,
      voltage: 125,
      latitude: 505612340,
      longitude: 113523450,
      altitude: 300,
      heading: 1800,
      satellites: 8,
      hdop: 12,
    };
    const encoded = encodeTelemetryLive(original);
    const dv = new DataView(encoded.buffer);
    const decoded = parseTelemetryLive(dv);
    expect(decoded.speed).toBe(original.speed);
    expect(decoded.rpm).toBe(original.rpm);
    expect(decoded.gear).toBe(original.gear);
    expect(decoded.fuel).toBe(original.fuel);
  });

  test('handles zero values', () => {
    const original: TelemetryLive = {
      speed: 0,
      rpm: 0,
      gear: -1,
      coolantTemp: 0,
      ambientTemp: 0,
      fuel: 0,
      voltage: 0,
      latitude: 0,
      longitude: 0,
      altitude: 0,
      heading: 0,
      satellites: 0,
      hdop: 0,
    };
    const encoded = encodeTelemetryLive(original);
    const dv = new DataView(encoded.buffer);
    const decoded = parseTelemetryLive(dv);
    expect(decoded.speed).toBe(0);
    expect(decoded.rpm).toBe(0);
  });
});

describe('SensorStatus parser', () => {
  test('roundtrip preserves status values', () => {
    const original: SensorStatus = {
      gps: 0,
      rpm: 0,
      temp: 1,
      fuel: 0,
      sdCard: 2,
    };
    const encoded = encodeSensorStatus(original);
    const dv = new DataView(encoded.buffer);
    const decoded = parseSensorStatus(dv);
    expect(decoded.gps).toBe(original.gps);
    expect(decoded.temp).toBe(original.temp);
    expect(decoded.sdCard).toBe(original.sdCard);
  });

  test('all-zero status', () => {
    const original: SensorStatus = {
      gps: 0,
      rpm: 0,
      temp: 0,
      fuel: 0,
      sdCard: 0,
    };
    const encoded = encodeSensorStatus(original);
    const dv = new DataView(encoded.buffer);
    const decoded = parseSensorStatus(dv);
    expect(decoded.gps).toBe(0);
    expect(decoded.fuel).toBe(0);
  });
});

describe('DTC List parser', () => {
  test('roundtrip with multiple DTCs', () => {
    const original: DtcEntry[] = [
      { code: 0x0101, severity: 1, timestamp: 1697000000 },
      { code: 0x0301, severity: 2, timestamp: 1697000100 },
    ];
    const encoded = encodeDtcList(original);
    const dv = new DataView(encoded.buffer);
    const decoded = parseDtcList(dv);
    expect(decoded).toHaveLength(2);
    expect(decoded[0].severity).toBe(1);
    expect(decoded[1].severity).toBe(2);
  });

  test('empty list roundtrip', () => {
    const encoded = encodeDtcList([]);
    const dv = new DataView(encoded.buffer);
    const decoded = parseDtcList(dv);
    expect(decoded).toHaveLength(0);
  });
});

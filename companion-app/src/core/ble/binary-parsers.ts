/**
 * Binary-Parser fuer BLE-Datenformate.
 *
 * Der Bordcomputer sendet alle Daten als packed binary (Little-Endian).
 * Diese Modul wandelt DataView -> TypeScript-Objekte und zurueck.
 *
 * Format-Spezifikation: siehe firmware/components/ble/gatt_uuids.h
 * und docs/research/13-connectivity-app.md Abschnitt 2.
 */

import type {
  TelemetryLive,
  SensorStatus,
  DtcEntry,
  SelfTestResult,
  SystemStatus,
  OtaStatus,
  OtaVerifyResult,
  WeatherCurrent,
  NavManeuver,
  NavRoute,
  NavRoad,
} from './gatt-uuids';
import { ManeuverType, SelfTestMode, OtaCommand, OtaVerifyStatus } from './gatt-uuids';

// ============================================================
// Hilfsfunktionen: DataView Little-Endian Reader
// ============================================================

export function base64ToDataView(base64: string): DataView {
  const binary = atob(base64);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) {
    bytes[i] = binary.charCodeAt(i);
  }
  return new DataView(bytes.buffer);
}

export function dataViewToBase64(dv: DataView): string {
  let binary = '';
  const bytes = new Uint8Array(dv.buffer);
  for (let i = 0; i < bytes.length; i++) {
    binary += String.fromCharCode(bytes[i]);
  }
  return btoa(binary);
}

export function uint8ArrayToBase64(arr: Uint8Array): string {
  let binary = '';
  for (let i = 0; i < arr.length; i++) {
    binary += String.fromCharCode(arr[i]);
  }
  return btoa(binary);
}

function readUint8(dv: DataView, offset: number): number {
  return dv.getUint8(offset);
}

function readInt8(dv: DataView, offset: number): number {
  return dv.getInt8(offset);
}

function readUint16(dv: DataView, offset: number): number {
  return dv.getUint16(offset, true); // Little-Endian
}

function readInt16(dv: DataView, offset: number): number {
  return dv.getInt16(offset, true);
}

function readUint32(dv: DataView, offset: number): number {
  return dv.getUint32(offset, true);
}

function readInt32(dv: DataView, offset: number): number {
  return dv.getInt32(offset, true);
}

function readUtf8(dv: DataView, offset: number, maxLen: number): string {
  const bytes: number[] = [];
  for (let i = 0; i < maxLen; i++) {
    const b = dv.getUint8(offset + i);
    if (b === 0) break;
    bytes.push(b);
  }
  return new TextDecoder().decode(new Uint8Array(bytes));
}

function writeUint8(dv: DataView, offset: number, value: number): void {
  dv.setUint8(offset, value & 0xff);
}

function writeInt8(dv: DataView, offset: number, value: number): void {
  dv.setInt8(offset, value);
}

function writeUint16(dv: DataView, offset: number, value: number): void {
  dv.setUint16(offset, value & 0xffff, true);
}

function writeUint32(dv: DataView, offset: number, value: number): void {
  dv.setUint32(offset, value >>> 0, true);
}

function writeInt16(dv: DataView, offset: number, value: number): void {
  dv.setInt16(offset, value, true);
}

function writeInt32(dv: DataView, offset: number, value: number): void {
  dv.setInt32(offset, value, true);
}

// ============================================================
// Telemetry Live (0x5001, 22 Bytes packed)
// ============================================================
// Byte  Offset  Field
// 0    0       speed (uint8, km/h)
// 1-2  1       rpm (uint16, U/min)
// 3    3       gear (int8, -1=neutral, 0=N, 1-6=Gang)
// 4    4       coolantTemp (int8, °C)
// 5    5       ambientTemp (int8, °C)
// 6    6       fuel (uint8, %)
// 7    7       voltage (uint8, Zehntelvolt)
// 8-11 8       latitude (int32, E-7)
// 12-15 12     longitude (int32, E-7)
// 16-17 16     altitude (int16, Meter)
// 18-19 18     heading (uint16, Zehntelgrad)
// 20   20      satellites (uint8)
// 21   21      hdop (uint8, Zehntel)
// ============================================================

export function parseTelemetryLive(dv: DataView): TelemetryLive {
  return {
    speed: readUint8(dv, 0),
    rpm: readUint16(dv, 1),
    gear: readInt8(dv, 3),
    coolantTemp: readInt8(dv, 4),
    ambientTemp: readInt8(dv, 5),
    fuel: readUint8(dv, 6),
    voltage: readUint8(dv, 7),
    latitude: readInt32(dv, 8),
    longitude: readInt32(dv, 12),
    altitude: readInt16(dv, 16),
    heading: readUint16(dv, 18),
    satellites: readUint8(dv, 20),
    hdop: readUint8(dv, 21),
  };
}

export function encodeTelemetryLive(data: TelemetryLive): Uint8Array {
  const buf = new ArrayBuffer(22);
  const dv = new DataView(buf);
  writeUint8(dv, 0, data.speed);
  writeUint16(dv, 1, data.rpm);
  writeInt8(dv, 3, data.gear);
  writeInt8(dv, 4, data.coolantTemp);
  writeInt8(dv, 5, data.ambientTemp);
  writeUint8(dv, 6, data.fuel);
  writeUint8(dv, 7, data.voltage);
  writeInt32(dv, 8, data.latitude);
  writeInt32(dv, 12, data.longitude);
  writeInt16(dv, 16, data.altitude);
  writeUint16(dv, 18, data.heading);
  writeUint8(dv, 20, data.satellites);
  writeUint8(dv, 21, data.hdop);
  return new Uint8Array(buf);
}

// ============================================================
// Sensor-Status (0x7002, 5 Bytes)
// ============================================================
// 0: gps (0=ok, 1=no_fix, 2=error)
// 1: rpm (0=ok, 1=no_signal, 2=error)
// 2: temp (0=ok, 1=disconnected, 2=error)
// 3: fuel (0=ok, 1=error)
// 4: sdCard (0=ok, 1=not_present, 2=full, 3=error)
// ============================================================

export function parseSensorStatus(dv: DataView): SensorStatus {
  return {
    gps: readUint8(dv, 0),
    rpm: readUint8(dv, 1),
    temp: readUint8(dv, 2),
    fuel: readUint8(dv, 3),
    sdCard: readUint8(dv, 4),
  };
}

export function encodeSensorStatus(data: SensorStatus): Uint8Array {
  const buf = new ArrayBuffer(5);
  const dv = new DataView(buf);
  writeUint8(dv, 0, data.gps);
  writeUint8(dv, 1, data.rpm);
  writeUint8(dv, 2, data.temp);
  writeUint8(dv, 3, data.fuel);
  writeUint8(dv, 4, data.sdCard);
  return new Uint8Array(buf);
}

// ============================================================
// DTC-Liste (0x7001, 6 Bytes pro Eintrag)
// ============================================================
// 0-1: code (uint16)
// 2:   severity (0=info, 1=warning, 2=critical)
// 3-5: reserved/padding -> timestamp komprimiert (uint24)
// ============================================================

export function parseDtcList(dv: DataView): DtcEntry[] {
  const entrySize = 6;
  const count = Math.floor(dv.byteLength / entrySize);
  const entries: DtcEntry[] = [];

  for (let i = 0; i < count; i++) {
    const offset = i * entrySize;
    const code = readUint16(dv, offset);
    const severity = readUint8(dv, offset + 2);
    // 3-Byte Timestamp (Little-Endian): reicht fuer ~194 Tage ab Epoch
    const tsLo = readUint8(dv, offset + 3);
    const tsMid = readUint8(dv, offset + 4);
    const tsHi = readUint8(dv, offset + 5);
    const timestamp = tsLo | (tsMid << 8) | (tsHi << 16);

    entries.push({ code, severity, timestamp });
  }

  return entries;
}

export function encodeDtcList(entries: DtcEntry[]): Uint8Array {
  const buf = new ArrayBuffer(entries.length * 6);
  const dv = new DataView(buf);

  for (let i = 0; i < entries.length; i++) {
    const offset = i * 6;
    const e = entries[i];
    writeUint16(dv, offset, e.code);
    writeUint8(dv, offset + 2, e.severity);
    writeUint8(dv, offset + 3, e.timestamp & 0xff);
    writeUint8(dv, offset + 4, (e.timestamp >> 8) & 0xff);
    writeUint8(dv, offset + 5, (e.timestamp >> 16) & 0xff);
  }

  return new Uint8Array(buf);
}

// ============================================================
// Self-Test (0x7003, Write 1 Byte, Notify Result)
// ============================================================
// Write: 1 Byte Mode (0=quick, 1=full)
// Notify: 1 Byte result (0=pass,1=warn,2=fail) + N Bytes details
// ============================================================

export function encodeSelfTestCommand(mode: SelfTestMode): Uint8Array {
  const buf = new ArrayBuffer(1);
  const dv = new DataView(buf);
  writeUint8(dv, 0, mode);
  return new Uint8Array(buf);
}

export function parseSelfTestResult(dv: DataView): SelfTestResult {
  const result = readUint8(dv, 0);
  const details: number[] = [];
  for (let i = 1; i < dv.byteLength; i++) {
    details.push(readUint8(dv, i));
  }
  return { result, details };
}

// ============================================================
// DTC Clear Command (Write an 0x7001)
// ============================================================
// 0xFFFF = alle loeschen, sonst = spezifischen Code loeschen
// ============================================================

export function encodeDtcClearCommand(code?: number): Uint8Array {
  const buf = new ArrayBuffer(2);
  const dv = new DataView(buf);
  writeUint16(dv, 0, code ?? 0xffff);
  return new Uint8Array(buf);
}

// ============================================================
// System-Status (via Snapshot, ~24 Bytes)
// ============================================================
// 0-3:   uptime (uint32, Sekunden)
// 4-7:   heapFree (uint32, Bytes)
// 8-11:  heapTotal (uint32, Bytes)
// 12:    batteryPct (uint8, %)
// 13:    cpuTemp (int8, °C)
// 14:    taskCount (uint8)
// 15:    bleConnections (uint8)
// 16:    wifiRssi (int8, dBm)
// ============================================================

export function parseSystemStatus(dv: DataView): SystemStatus {
  return {
    uptime: readUint32(dv, 0),
    heapFree: readUint32(dv, 4),
    heapTotal: readUint32(dv, 8),
    batteryPct: readUint8(dv, 12),
    cpuTemp: readInt8(dv, 13),
    taskCount: readUint8(dv, 14),
    bleConnections: readUint8(dv, 15),
    wifiRssi: readInt8(dv, 16),
  };
}

export function encodeSystemStatus(data: SystemStatus): Uint8Array {
  const buf = new ArrayBuffer(17);
  const dv = new DataView(buf);
  writeUint32(dv, 0, data.uptime);
  writeUint32(dv, 4, data.heapFree);
  writeUint32(dv, 8, data.heapTotal);
  writeUint8(dv, 12, data.batteryPct);
  writeInt8(dv, 13, data.cpuTemp);
  writeUint8(dv, 14, data.taskCount);
  writeUint8(dv, 15, data.bleConnections);
  writeInt8(dv, 16, data.wifiRssi);
  return new Uint8Array(buf);
}

// ============================================================
// Task-Info — REMOVED
// 0x7007 taskInfo is not implemented in firmware (spec §8.4, §10).
// ============================================================

// ============================================================
// OTA-Status (0x8001 control notify, 3 bytes)
// ============================================================
// 0: state (uint8: 0=idle,1=uploading,2=verifying,3=done,4=failed)
// 1: progress (uint8: 0-100)
// 2: errorCode (uint8)
// Readers must tolerate a 3-byte notify (spec §9.1).
// ============================================================

export function parseOtaStatus(dv: DataView): OtaStatus {
  return {
    state: readUint8(dv, 0),
    progress: readUint8(dv, 1),
    errorCode: readUint8(dv, 2),
  };
}

export function encodeOtaCommand(cmd: OtaCommand): Uint8Array {
  const buf = new ArrayBuffer(1);
  const dv = new DataView(buf);
  writeUint8(dv, 0, cmd);
  return new Uint8Array(buf);
}

export function encodeOtaCommitCommand(expectedSha256?: Uint8Array): Uint8Array {
  if (!expectedSha256 || expectedSha256.length === 0) {
    const buf = new ArrayBuffer(1);
    const dv = new DataView(buf);
    writeUint8(dv, 0, OtaCommand.Commit);
    return new Uint8Array(buf);
  }
  const buf = new ArrayBuffer(1 + 32);
  const dv = new DataView(buf);
  writeUint8(dv, 0, OtaCommand.Commit);
  const hashLen = Math.min(expectedSha256.length, 32);
  for (let i = 0; i < 32; i++) {
    writeUint8(dv, 1 + i, i < hashLen ? expectedSha256[i] : 0);
  }
  return new Uint8Array(buf);
}

export function encodeOtaDataChunk(data: Uint8Array): Uint8Array {
  // Chunks werden als raw binary gesendet (no framing)
  return data;
}

// ============================================================
// OTA Verify Result (0x8003 notify, 41 bytes)
// ============================================================
// 0:     status (uint8: 0=unknown, 1=verifying, 2=ok, 3=hash_mismatch, 4=error)
// 1-32:  sha256 (32 bytes)
// 33-36: size (uint32 LE)
// 37-40: version (uint32 LE)
// ============================================================

export function parseOtaVerifyResult(dv: DataView): OtaVerifyResult {
  const status = readUint8(dv, 0) as OtaVerifyStatus;

  let sha256 = '';
  for (let i = 1; i < Math.min(dv.byteLength, 1 + 32); i++) {
    sha256 += dv.getUint8(i).toString(16).padStart(2, '0');
  }

  const size = dv.byteLength >= 37 ? readUint32(dv, 33) : 0;
  const version = dv.byteLength >= 41 ? readUint32(dv, 37) : 0;

  return { status, sha256, size, version };
}

// ============================================================
// Weather Current (0x4001, 8 Bytes)
// ============================================================
// 0-1: temperature (int16, Zehntelgrad)
// 2:  humidity (uint8, %)
// 3:  condition (uint8, 0=clear..7=fog)
// 4:  windSpeed (uint8, km/h)
// 5-6: feelsLike (int16, Zehntelgrad)
// 7:  rainProbability (uint8, %)
// ============================================================

export function parseWeatherCurrent(dv: DataView): WeatherCurrent {
  return {
    temperature: readInt16(dv, 0),
    humidity: readUint8(dv, 2),
    condition: readUint8(dv, 3),
    windSpeed: readUint8(dv, 4),
    feelsLike: readInt16(dv, 5),
    rainProbability: readUint8(dv, 7),
  };
}

// ============================================================
// Navigation Maneuver (0x1001, 4 + 64 Bytes)
// ============================================================
// 0:    maneuverType (uint8)
// 1-4:  distance (uint32, Meter)
// 5-68: streetName (64 Bytes UTF-8, null-terminiert)
// ============================================================

export function parseNavManeuver(dv: DataView): NavManeuver {
  const type = readUint8(dv, 0) as ManeuverType;
  const distance = readUint32(dv, 1);
  const streetName = readUtf8(dv, 5, 64);
  return { type, distance, streetName };
}

export function encodeNavManeuver(data: NavManeuver): Uint8Array {
  const buf = new ArrayBuffer(69);
  const dv = new DataView(buf);
  writeUint8(dv, 0, data.type);
  writeUint32(dv, 1, data.distance);
  const nameBytes = new TextEncoder().encode(data.streetName);
  for (let i = 0; i < 64; i++) {
    writeUint8(dv, 5 + i, i < nameBytes.length ? nameBytes[i] : 0);
  }
  return new Uint8Array(buf);
}

// ============================================================
// Navigation Route (0x1002, 16 Bytes)
// ============================================================
// 0-3:   totalDistance (uint32, Meter)
// 4-7:   remainingDistance (uint32, Meter)
// 8-11:  eta (uint32, Unix-Timestamp)
// 12-15: remainingTime (uint32, Sekunden)
// ============================================================

export function parseNavRoute(dv: DataView): NavRoute {
  return {
    totalDistance: readUint32(dv, 0),
    remainingDistance: readUint32(dv, 4),
    eta: readUint32(dv, 8),
    remainingTime: readUint32(dv, 12),
  };
}

export function encodeNavRoute(data: NavRoute): Uint8Array {
  const buf = new ArrayBuffer(16);
  const dv = new DataView(buf);
  writeUint32(dv, 0, data.totalDistance);
  writeUint32(dv, 4, data.remainingDistance);
  writeUint32(dv, 8, data.eta);
  writeUint32(dv, 12, data.remainingTime);
  return new Uint8Array(buf);
}

// ============================================================
// Navigation Road (0x1003, 1 + 64 Bytes)
// ============================================================
// 0:    speedLimit (uint8, km/h, 0=unbekannt)
// 1-64: roadName (64 Bytes UTF-8, null-terminiert)
// ============================================================

export function parseNavRoad(dv: DataView): NavRoad {
  const speedLimit = readUint8(dv, 0);
  const roadName = readUtf8(dv, 1, 64);
  return { speedLimit, roadName };
}

export function encodeNavRoad(data: NavRoad): Uint8Array {
  const buf = new ArrayBuffer(65);
  const dv = new DataView(buf);
  writeUint8(dv, 0, data.speedLimit);
  const nameBytes = new TextEncoder().encode(data.roadName);
  for (let i = 0; i < 64; i++) {
    writeUint8(dv, 1 + i, i < nameBytes.length ? nameBytes[i] : 0);
  }
  return new Uint8Array(buf);
}

// ============================================================
// Config JSON Chunk Encoding/Decoding
// ============================================================

export function encodeConfigJson(json: string): Uint8Array {
  return new TextEncoder().encode(json);
}

export function decodeConfigJson(data: Uint8Array): string {
  return new TextDecoder().decode(data);
}

// ============================================================
// CarPlay — REMOVED
// Service 0x9000 is the CarPlay-display bridge, NOT app-facing (spec §11).
// The app must NOT touch service 0x9000.
// ============================================================

/**
 * BleManager.test.ts — Tests fuer den BleManager
 *
 * Testet:
 *   - Auto-reconnect logic
 *   - Error handling (BT disabled, scan failed, connection failed)
 *   - OTA chunk sending
 *   - Notification dispatching
 *   - Encode helpers
 *   - Connection state tracking
 */

import { BleManager, DEFAULT_BLE_MANAGER_CONFIG } from './BleManager';
import { MockBleProvider } from '../core/ble/MockBleProvider';
import { BleErrorCode } from '../core/ble/BleProvider';
import { ConnectionState, isConnected } from '../core/ble/connection-state';
import { SelfTestMode, OtaCommand } from '../core/ble/gatt-uuids';
import { CHAR } from '../core/ble/gatt-uuids';
import type { BleEventCallbacks } from '../core/ble/BleProvider';

// Helper
function waitFor(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

describe('BleManager construction', () => {
  it('should create with default config', () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    expect(mgr.getConnectionState()).toBe(ConnectionState.Disconnected);
    expect(mgr.is_connected()).toBe(false);
    mgr.destroy();
  });

  it('should accept custom config', () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider, { autoReconnect: false, scanTimeoutMs: 5000, mtu: 185 });
    expect(mgr.is_connected()).toBe(false);
    mgr.destroy();
  });

  it('should use default config values', () => {
    expect(DEFAULT_BLE_MANAGER_CONFIG.autoReconnect).toBe(true);
    expect(DEFAULT_BLE_MANAGER_CONFIG.scanTimeoutMs).toBe(10000);
    expect(DEFAULT_BLE_MANAGER_CONFIG.mtu).toBe(247);
  });
});

describe('BleManager init and lifecycle', () => {
  it('should init without error', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();
    // No error thrown = success
    mgr.destroy();
  });

  it('should destroy cleanly', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();
    mgr.destroy();
    // Second destroy should not crash
    expect(() => mgr.destroy()).not.toThrow();
  });
});

describe('BleManager scan', () => {
  it('should start scan and reach connected state', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();

    await mgr.startScan();
    await waitFor(4200);

    expect(mgr.is_connected()).toBe(true);
    mgr.destroy();
  }, 10000);

  it('should report connection state changes to listeners', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();

    const states: ConnectionState[] = [];
    mgr.addListener({ onConnectionStateChanged: (s) => states.push(s) });

    await mgr.startScan();
    await waitFor(4500);

    expect(states.length).toBeGreaterThan(0);
    expect(states).toContain(ConnectionState.Connected);
    mgr.destroy();
  }, 10000);
});

describe('BleManager error handling', () => {
  it('should emit error when bluetooth is disabled', async () => {
    // Create a provider that reports BT as disabled
    const provider = new MockBleProvider();
    // Override isBluetoothEnabled to return false
    jest.spyOn(provider, 'isBluetoothEnabled').mockResolvedValue(false);

    const mgr = new BleManager(provider);
    await mgr.init();

    const errors: unknown[] = [];
    mgr.addListener({ onError: (e) => errors.push(e) });

    await mgr.startScan();

    expect(errors.length).toBe(1);
    const err = errors[0] as { code: BleErrorCode; message: string };
    expect(err.code).toBe(BleErrorCode.BluetoothDisabled);
    mgr.destroy();
  });

  it('should handle disconnect', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();

    await mgr.startScan();
    await waitFor(4200);
    expect(mgr.is_connected()).toBe(true);

    await mgr.disconnect();
    expect(mgr.is_connected()).toBe(false);
    mgr.destroy();
  }, 10000);
});

describe('BleManager GATT reads', () => {
  let mgr: BleManager;

  beforeEach(async () => {
    const provider = new MockBleProvider();
    mgr = new BleManager(provider);
    await mgr.init();
    await mgr.startScan();
    await waitFor(4200);
  });

  afterEach(() => {
    mgr.destroy();
  });

  it('should read telemetry snapshot', async () => {
    const snapshot = await mgr.readTelemetrySnapshot();
    expect(snapshot).toHaveProperty('speed');
  }, 10000);

  it('should read config', async () => {
    const config = await mgr.readConfig();
    expect(typeof config).toBe('string');
  }, 10000);

  it('should read DTC list', async () => {
    const dtcs = await mgr.readDtcList();
    expect(Array.isArray(dtcs)).toBe(true);
  }, 10000);
});

describe('BleManager write operations', () => {
  let mgr: BleManager;

  beforeEach(async () => {
    const provider = new MockBleProvider();
    mgr = new BleManager(provider);
    await mgr.init();
    await mgr.startScan();
    await waitFor(4200);
  });

  afterEach(() => {
    mgr.destroy();
  });

  it('should write config', async () => {
    await mgr.writeConfig('{"brightness": 50}');
  }, 10000);

  it('should trigger self-test', async () => {
    await mgr.triggerSelfTest(SelfTestMode.Quick);
  }, 10000);

  it('should clear DTC', async () => {
    await mgr.clearDtc(0x0101);
    await mgr.clearDtc(); // clear all
  }, 10000);

  it('should delete all bonds', async () => {
    await mgr.deleteAllBonds();
  }, 10000);

  it('should export and import config', async () => {
    const exported = await mgr.exportConfig();
    expect(typeof exported).toBe('string');
    await mgr.importConfig(exported);
  }, 10000);
});

describe('BleManager OTA', () => {
  let mgr: BleManager;

  beforeEach(async () => {
    const provider = new MockBleProvider();
    mgr = new BleManager(provider);
    await mgr.init();
    await mgr.startScan();
    await waitFor(4200);
  });

  afterEach(() => {
    mgr.destroy();
  });

  it('should start OTA', async () => {
    await mgr.startOta(1024);
  }, 10000);

  it('should upload firmware in chunks', async () => {
    await mgr.startOta(400);

    const progressUpdates: { sent: number; total: number }[] = [];
    const firmware = new Uint8Array(400);

    await mgr.uploadOtaFirmware(firmware, (sent, total) => {
      progressUpdates.push({ sent, total });
    });

    expect(progressUpdates.length).toBeGreaterThan(0);
    const last = progressUpdates[progressUpdates.length - 1];
    expect(last.sent).toBe(400);
    expect(last.total).toBe(400);
  }, 15000);

  it('should verify OTA', async () => {
    await mgr.startOta(100);
    await mgr.verifyOta();
  }, 10000);

  it('should cancel OTA', async () => {
    await mgr.startOta(1024);
    await mgr.cancelOta();
  }, 10000);
});

describe('BleManager notification handling', () => {
  it('should dispatch telemetry on tlmLive notification', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();

    const telemetry: unknown[] = [];
    mgr.addListener({ onTelemetry: (data) => telemetry.push(data) });

    // Simulate a notification with base64-encoded 22-byte telemetry
    const buf = new ArrayBuffer(22);
    const dv = new DataView(buf);
    dv.setUint8(0, 80);  // speed
    dv.setUint16(1, 4000, true);  // rpm

    const bytes = new Uint8Array(buf);
    let b64 = '';
    for (let i = 0; i < bytes.length; i++) {
      b64 += String.fromCharCode(bytes[i]);
    }
    mgr.handleNotification(CHAR.tlmLive, btoa(b64));

    expect(telemetry.length).toBe(1);
    const data = telemetry[0] as { speed: number; rpm: number };
    expect(data.speed).toBe(80);
    expect(data.rpm).toBe(4000);

    mgr.destroy();
  });

  it('should dispatch sensor status on notification', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();

    const statuses: unknown[] = [];
    mgr.addListener({ onSensorStatus: (s) => statuses.push(s) });

    const buf = new ArrayBuffer(5);
    const dv = new DataView(buf);
    dv.setUint8(0, 0);
    dv.setUint8(1, 1);
    dv.setUint8(2, 2);
    dv.setUint8(3, 0);
    dv.setUint8(4, 3);

    const bytes = new Uint8Array(buf);
    let b64 = '';
    for (let i = 0; i < bytes.length; i++) b64 += String.fromCharCode(bytes[i]);
    mgr.handleNotification(CHAR.sensorStatus, btoa(b64));

    expect(statuses.length).toBe(1);
    const status = statuses[0] as { gps: number; sdCard: number };
    expect(status.gps).toBe(0);
    expect(status.sdCard).toBe(3);

    mgr.destroy();
  });

  it('should ignore unknown characteristics', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();

    const errors: unknown[] = [];
    mgr.addListener({ onError: (e) => errors.push(e) });

    // Unknown characteristic should not produce errors
    mgr.handleNotification('unknown-uuid', '');
    expect(errors.length).toBe(0);

    mgr.destroy();
  });
});

describe('BleManager encode helpers', () => {
  const provider = new MockBleProvider();
  const mgr = new BleManager(provider);

  it('should encode self-test command', () => {
    const encoded = mgr.encodeSelfTest(SelfTestMode.Full);
    expect(typeof encoded).toBe('string');
    expect(encoded.length).toBeGreaterThan(0);
  });

  it('should encode DTC clear command', () => {
    const encoded = mgr.encodeDtcClear(0x0101);
    expect(typeof encoded).toBe('string');
    expect(encoded.length).toBeGreaterThan(0);
  });

  it('should encode DTC clear-all command', () => {
    const encoded = mgr.encodeDtcClear();
    expect(typeof encoded).toBe('string');
  });

  it('should encode OTA command', () => {
    const encoded = mgr.encodeOtaCmd(OtaCommand.Start);
    expect(typeof encoded).toBe('string');
  });

  it('should encode OTA chunk', () => {
    const data = new Uint8Array([1, 2, 3, 4, 5]);
    const encoded = mgr.encodeOtaChunk(data);
    expect(typeof encoded).toBe('string');
    expect(encoded.length).toBeGreaterThan(0);
  });
});

describe('BleManager connection state', () => {
  it('should track connection state', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();

    expect(mgr.is_connected()).toBe(false);
    expect(isConnected(mgr.getConnectionState())).toBe(false);

    await mgr.startScan();
    await waitFor(4200);

    expect(mgr.is_connected()).toBe(true);
    expect(isConnected(mgr.getConnectionState())).toBe(true);

    mgr.destroy();
  }, 10000);
});

// ============================================================
// BleManager Error Handling & Reconnection
// ============================================================

describe('BleManager error recovery', () => {
  it('should emit error on scan failure and allow retry', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();

    const errors: unknown[] = [];
    mgr.addListener({ onError: (e) => errors.push(e) });

    // Simulate BT disabled
    jest.spyOn(provider, 'isBluetoothEnabled').mockResolvedValue(false);
    await mgr.startScan();

    expect(errors.length).toBe(1);
    const err = errors[0] as { code: BleErrorCode; recoverable: boolean };
    expect(err.code).toBe(BleErrorCode.BluetoothDisabled);
    expect(err.recoverable).toBe(false);

    // Restore BT and retry
    jest.spyOn(provider, 'isBluetoothEnabled').mockResolvedValue(true);
    await mgr.startScan();
    await waitFor(4200);
    expect(mgr.is_connected()).toBe(true);

    mgr.destroy();
  }, 15000);

  it('should dispatch errors to all listeners', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();

    const errors1: unknown[] = [];
    const errors2: unknown[] = [];
    mgr.addListener({ onError: (e) => errors1.push(e) });
    mgr.addListener({ onError: (e) => errors2.push(e) });

    jest.spyOn(provider, 'isBluetoothEnabled').mockResolvedValue(false);
    await mgr.startScan();

    expect(errors1.length).toBe(1);
    expect(errors2.length).toBe(1);

    mgr.destroy();
  });
});

// ============================================================
// BleManager OTA Control & Verify Notification Dispatch
// ============================================================

describe('BleManager OTA notification dispatch', () => {
  it('should dispatch OTA status on otaControl notification', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();

    const statuses: unknown[] = [];
    mgr.addListener({ onOtaStatus: (s) => statuses.push(s) });

    // Encode a 3-byte OTA status: state=1 (uploading), progress=50, errorCode=0
    const buf = new ArrayBuffer(3);
    const dv = new DataView(buf);
    dv.setUint8(0, 1);
    dv.setUint8(1, 50);
    dv.setUint8(2, 0);

    const bytes = new Uint8Array(buf);
    let b64 = '';
    for (let i = 0; i < bytes.length; i++) b64 += String.fromCharCode(bytes[i]);
    mgr.handleNotification(CHAR.otaControl, btoa(b64));

    expect(statuses.length).toBe(1);
    const status = statuses[0] as { state: number; progress: number; errorCode: number };
    expect(status.state).toBe(1);
    expect(status.progress).toBe(50);
    expect(status.errorCode).toBe(0);

    mgr.destroy();
  });

  it('should dispatch OTA verify result on otaVerify notification', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();

    const results: unknown[] = [];
    mgr.addListener({ onOtaVerifyResult: (r) => results.push(r) });

    // Encode a 41-byte verify result: status=3 (hash_mismatch), sha256=0..31, size=1024, version=1
    const buf = new ArrayBuffer(41);
    const dv = new DataView(buf);
    dv.setUint8(0, 3); // HashMismatch
    for (let i = 0; i < 32; i++) dv.setUint8(1 + i, i);
    dv.setUint32(33, 1024, true);
    dv.setUint32(37, 1, true);

    const bytes = new Uint8Array(buf);
    let b64 = '';
    for (let i = 0; i < bytes.length; i++) b64 += String.fromCharCode(bytes[i]);
    mgr.handleNotification(CHAR.otaVerify, btoa(b64));

    expect(results.length).toBe(1);
    const result = results[0] as { status: number; sha256: string; size: number; version: number };
    expect(result.status).toBe(3);
    expect(result.sha256.length).toBe(64);
    expect(result.size).toBe(1024);
    expect(result.version).toBe(1);

    mgr.destroy();
  });

  it('should handle parse errors in notifications gracefully', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();

    const errors: unknown[] = [];
    mgr.addListener({ onError: (e) => errors.push(e) });

    // Send a truncated telemetry packet
    const buf = new ArrayBuffer(3);
    const dv = new DataView(buf);
    dv.setUint8(0, 80);
    dv.setUint8(1, 0);
    dv.setUint8(2, 0);

    const bytes = new Uint8Array(buf);
    let b64 = '';
    for (let i = 0; i < bytes.length; i++) b64 += String.fromCharCode(bytes[i]);

    expect(() => mgr.handleNotification(CHAR.tlmLive, btoa(b64))).not.toThrow();

    mgr.destroy();
  });
});

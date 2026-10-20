/**
 * MockBleProvider.test.ts — Tests fuer den Mock-BLE-Provider
 *
 * Testet:
 *   - Connection state machine (Disconnected -> Scanning -> Connected)
 *   - Telemetry callback delivery
 *   - DTC list operations (read, clear)
 *   - Config read/write
 *   - Self-test trigger
 *   - System status / task info
 *   - OTA lifecycle (start, upload, verify)
 *   - Destroy cleanup
 */

import { MockBleProvider } from './MockBleProvider';
import { ConnectionState, canTransition, reconnectDelay, RECONNECT_MAX_ATTEMPTS } from './connection-state';
import { SelfTestMode, ManeuverType } from './gatt-uuids';
import type { BleEventCallbacks, Subscription } from './BleProvider';

// Helper: wait for a condition or timeout
function waitFor(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

describe('MockBleProvider connection state machine', () => {
  let provider: MockBleProvider;

  beforeEach(() => {
    provider = new MockBleProvider();
  });

  afterEach(() => {
    provider.destroy();
  });

  it('should start in Disconnected state', () => {
    expect(provider.getConnectionState()).toBe(ConnectionState.Disconnected);
  });

  it('should transition to Scanning on startScan', async () => {
    await provider.startScan();
    expect(provider.getConnectionState()).toBe(ConnectionState.Scanning);
  });

  it('should reach Connected after scan + connect sequence', async () => {
    const stateChanges: ConnectionState[] = [];
    provider.subscribe({
      onConnectionStateChanged: (state) => stateChanges.push(state),
    });

    await provider.startScan();

    // Scan (2s) + Connect (1s) + Discover (500ms) + Auth (500ms) = 4s
    await waitFor(4200);

    expect(provider.getConnectionState()).toBe(ConnectionState.Connected);
    expect(stateChanges).toContain(ConnectionState.Scanning);
    expect(stateChanges).toContain(ConnectionState.Connecting);
    expect(stateChanges).toContain(ConnectionState.Discovering);
    expect(stateChanges).toContain(ConnectionState.Authenticating);
    expect(stateChanges).toContain(ConnectionState.Connected);
  }, 10000);

  it('should return connected device after connection', async () => {
    await provider.startScan();
    await waitFor(4200);

    const device = provider.getConnectedDevice();
    expect(device).not.toBeNull();
    expect(device!.name).toBe('Transalp-BC');
    expect(device!.rssi).toBeLessThan(0);
  }, 10000);

  it('should disconnect cleanly', async () => {
    await provider.startScan();
    await waitFor(4200);
    expect(provider.getConnectionState()).toBe(ConnectionState.Connected);

    await provider.disconnect();
    expect(provider.getConnectionState()).toBe(ConnectionState.Disconnected);
    expect(provider.getConnectedDevice()).toBeNull();
  }, 10000);
});

describe('MockBleProvider telemetry', () => {
  let provider: MockBleProvider;

  beforeEach(() => {
    provider = new MockBleProvider();
  });

  afterEach(() => {
    provider.destroy();
  });

  it('should deliver telemetry updates after connection', async () => {
    const telemetryUpdates: unknown[] = [];
    provider.subscribe({
      onTelemetry: (data) => telemetryUpdates.push(data),
    });

    await provider.startScan();
    await waitFor(4200); // Connect

    // Wait for at least one telemetry update (250ms interval)
    await waitFor(500);

    expect(telemetryUpdates.length).toBeGreaterThan(0);
    const first = telemetryUpdates[0] as { speed: number; rpm: number };
    expect(first).toHaveProperty('speed');
    expect(first).toHaveProperty('rpm');
  }, 10000);

  it('should read telemetry snapshot', async () => {
    await provider.startScan();
    await waitFor(4200);

    const snapshot = await provider.readTelemetrySnapshot();
    expect(snapshot).toHaveProperty('speed');
    expect(snapshot).toHaveProperty('rpm');
  }, 10000);
});

describe('MockBleProvider DTC operations', () => {
  let provider: MockBleProvider;

  beforeEach(() => {
    provider = new MockBleProvider();
  });

  afterEach(() => {
    provider.destroy();
  });

  it('should return empty DTC list initially', async () => {
    const dtcs = await provider.readDtcList();
    expect(dtcs).toEqual([]);
  });

  it('should clear all DTCs', async () => {
    // clearDtc with no argument clears all
    await provider.clearDtc();
    const dtcs = await provider.readDtcList();
    expect(dtcs).toEqual([]);
  });

  it('should emit DTC list update after clear', async () => {
    const dtcUpdates: unknown[][] = [];
    provider.subscribe({
      onDtcList: (entries) => dtcUpdates.push(entries),
    });

    await provider.clearDtc();
    await waitFor(100);

    expect(dtcUpdates.length).toBeGreaterThan(0);
  });
});

describe('MockBleProvider config', () => {
  let provider: MockBleProvider;

  beforeEach(() => {
    provider = new MockBleProvider();
  });

  afterEach(() => {
    provider.destroy();
  });

  it('should read config as JSON string', async () => {
    const config = await provider.readConfig();
    expect(typeof config).toBe('string');

    const parsed = JSON.parse(config);
    expect(parsed).toHaveProperty('brightness');
    expect(parsed).toHaveProperty('deviceName');
    expect(parsed).toHaveProperty('rpmRedline');
  });

  it('should write config without error', async () => {
    await provider.writeConfig('{"brightness": 50}');
    // No error thrown = success
  });

  it('should export and import config', async () => {
    const exported = await provider.exportConfig();
    expect(typeof exported).toBe('string');

    await provider.importConfig(exported);
    // No error = success
  });
});

describe('MockBleProvider self-test', () => {
  let provider: MockBleProvider;

  beforeEach(() => {
    provider = new MockBleProvider();
  });

  afterEach(() => {
    provider.destroy();
  });

  it('should deliver self-test result after trigger', async () => {
    const results: unknown[] = [];
    provider.subscribe({
      onSelfTestResult: (result) => results.push(result),
    });

    await provider.triggerSelfTest(SelfTestMode.Full);

    // Result is delivered after 1s
    await waitFor(1200);

    expect(results.length).toBe(1);
    const result = results[0] as { result: number; details: number[] };
    expect(result.result).toBe(0); // pass
    expect(result.details.length).toBeGreaterThan(0);
  }, 5000);
});

describe('MockBleProvider system status', () => {
  let provider: MockBleProvider;

  beforeEach(() => {
    provider = new MockBleProvider();
  });

  afterEach(() => {
    provider.destroy();
  });

  it('should read system status', async () => {
    const status = await provider.readSystemStatus();
    expect(status).not.toBeNull();
    expect(status!.uptime).toBeGreaterThanOrEqual(0);
    expect(status!.heapFree).toBeGreaterThan(0);
    expect(status!.batteryPct).toBeGreaterThan(0);
    expect(status!.batteryPct).toBeLessThanOrEqual(100);
  });

  it('should read task info', async () => {
    const tasks = await provider.readTaskInfo();
    expect(tasks.length).toBeGreaterThan(0);
    expect(tasks[0]).toHaveProperty('name');
    expect(tasks[0]).toHaveProperty('priority');
    expect(tasks[0]).toHaveProperty('stackFree');
  });

  it('should read OTA status', async () => {
    const status = await provider.readOtaStatus();
    expect(status).not.toBeNull();
    expect(status!.state).toBe(0); // idle
  });
});

describe('MockBleProvider OTA', () => {
  let provider: MockBleProvider;

  beforeEach(() => {
    provider = new MockBleProvider();
  });

  afterEach(() => {
    provider.destroy();
  });

  it('should start OTA and report progress', async () => {
    const otaUpdates: unknown[] = [];
    provider.subscribe({
      onOtaStatus: (status) => otaUpdates.push(status),
    });

    await provider.startOta(1024);
    expect(otaUpdates.length).toBeGreaterThan(0);

    const status = otaUpdates[0] as { state: number; progress: number };
    expect(status.state).toBe(1); // uploading
  });

  it('should update progress on data write', async () => {
    const otaUpdates: unknown[] = [];
    provider.subscribe({
      onOtaStatus: (status) => otaUpdates.push(status),
    });

    await provider.startOta(1024);
    otaUpdates.length = 0;

    await provider.writeOtaData(new Uint8Array(180));
    expect(otaUpdates.length).toBeGreaterThan(0);

    const status = otaUpdates[otaUpdates.length - 1] as { progress: number };
    expect(status.progress).toBeGreaterThan(0);
  });

  it('should verify OTA and report done', async () => {
    const otaUpdates: unknown[] = [];
    provider.subscribe({
      onOtaStatus: (status) => otaUpdates.push(status),
    });

    await provider.startOta(100);
    await provider.verifyOta();

    const last = otaUpdates[otaUpdates.length - 1] as { state: number; progress: number };
    expect(last.state).toBe(3); // done
    expect(last.progress).toBe(100);
  });

  it('should cancel OTA', async () => {
    await provider.startOta(1024);
    await provider.cancelOta();
    // No error = success
  });
});

describe('MockBleProvider subscription', () => {
  let provider: MockBleProvider;

  beforeEach(() => {
    provider = new MockBleProvider();
  });

  afterEach(() => {
    provider.destroy();
  });

  it('should allow multiple subscriptions', () => {
    let count1 = 0;
    let count2 = 0;

    const sub1 = provider.subscribe({ onConnectionStateChanged: () => count1++ });
    const sub2 = provider.subscribe({ onConnectionStateChanged: () => count2++ });

    // Trigger a state change
    provider.startScan();

    expect(count1).toBeGreaterThan(0);
    expect(count2).toBeGreaterThan(0);

    sub1();
    sub2();
  });

  it('should unsubscribe correctly', () => {
    let count = 0;
    const sub = provider.subscribe({ onConnectionStateChanged: () => count++ });

    provider.startScan();
    const countAfterFirst = count;

    sub();

    // After unsubscribe, no more events should be delivered
    provider.disconnect();
    expect(count).toBe(countAfterFirst);
  });
});

describe('MockBleProvider destroy', () => {
  it('should stop all timers and intervals on destroy', async () => {
    const provider = new MockBleProvider();
    await provider.startScan();
    await waitFor(4200);

    provider.destroy();

    expect(provider.getConnectionState()).toBe(ConnectionState.Disconnected);
  }, 10000);
});

describe('connection-state utility functions', () => {
  it('canTransition should validate allowed transitions', () => {
    expect(canTransition(ConnectionState.Disconnected, ConnectionState.Scanning)).toBe(true);
    expect(canTransition(ConnectionState.Scanning, ConnectionState.Connecting)).toBe(true);
    expect(canTransition(ConnectionState.Connected, ConnectionState.Degrading)).toBe(true);
  });

  it('canTransition should reject invalid transitions', () => {
    expect(canTransition(ConnectionState.Disconnected, ConnectionState.Connected)).toBe(false);
    expect(canTransition(ConnectionState.Connected, ConnectionState.Scanning)).toBe(false);
  });
});

// ============================================================
// Reconnection Backoff
// ============================================================

describe('Reconnection backoff', () => {
  it('should use exponential delays for first 5 attempts', () => {
    expect(reconnectDelay(0)).toBe(1000);
    expect(reconnectDelay(1)).toBe(2000);
    expect(reconnectDelay(2)).toBe(4000);
    expect(reconnectDelay(3)).toBe(8000);
    expect(reconnectDelay(4)).toBe(16000);
  });

  it('should cap at max delay after 5 attempts', () => {
    expect(reconnectDelay(5)).toBe(30000);
    expect(reconnectDelay(10)).toBe(30000);
    expect(reconnectDelay(100)).toBe(30000);
  });

  it('should have max attempts defined', () => {
    expect(RECONNECT_MAX_ATTEMPTS).toBe(5);
  });
});

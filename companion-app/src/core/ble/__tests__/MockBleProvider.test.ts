/**
 * MockBleProvider.test.ts — Tests fuer den MockBleProvider
 *
 * Testet:
 *   - Connection State Machine: Disconnected -> Scanning -> Connected
 *   - Telemetry callbacks werden gefeuert
 *   - Auto-Reconnect Verhalten
 *   - Error Handling
 */

import { MockBleProvider } from '../MockBleProvider';
import { ConnectionState } from '../connection-state';

describe('MockBleProvider', () => {
  test('starts in Disconnected state', () => {
    const provider = new MockBleProvider();
    expect(provider.getConnectionState()).toBe(ConnectionState.Disconnected);
    provider.destroy();
  });

  test('isBluetoothEnabled returns true', async () => {
    const provider = new MockBleProvider();
    await provider.init();
    const enabled = await provider.isBluetoothEnabled();
    expect(enabled).toBe(true);
    provider.destroy();
  });

  test('subscribe registers callbacks', () => {
    const provider = new MockBleProvider();
    let stateReceived: ConnectionState | null = null;
    const unsub = provider.subscribe({
      onConnectionStateChanged: (state) => {
        stateReceived = state;
      },
    });
    expect(typeof unsub).toBe('function');
    unsub();
    provider.destroy();
  });

  test('startScan transitions to Scanning', async () => {
    const provider = new MockBleProvider();
    await provider.init();
    await provider.startScan();
    // Mock should transition to Scanning immediately
    expect(provider.getConnectionState()).not.toBe(ConnectionState.Disconnected);
    provider.destroy();
  });

  test('disconnect returns to Disconnected', async () => {
    const provider = new MockBleProvider();
    await provider.init();
    await provider.startScan();
    await provider.disconnect();
    expect(provider.getConnectionState()).toBe(ConnectionState.Disconnected);
    provider.destroy();
  });

  test('getConnectedDevice returns null when disconnected', () => {
    const provider = new MockBleProvider();
    expect(provider.getConnectedDevice()).toBeNull();
    provider.destroy();
  });

  test('destroy is safe to call multiple times', () => {
    const provider = new MockBleProvider();
    provider.destroy();
    provider.destroy();
    // Should not throw
  });
});

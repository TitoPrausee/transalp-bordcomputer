/**
 * store.test.ts — Tests fuer den Zustand-Store
 *
 * Testet:
 *   - Initiale State-Werte
 *   - State-Transitions (setTelemetry, setBleState, etc.)
 *   - Mock-Daten werden korrekt gesetzt
 */

import { useAppStore, EMPTY_TELEMETRY, MOCK_DTCS } from '../store';
import { ConnectionState } from '../../ble/connection-state';

describe('AppStore', () => {
  beforeEach(() => {
    useAppStore.getState().reset();
  });

  test('initial state has empty telemetry', () => {
    const state = useAppStore.getState();
    expect(state.telemetry).toEqual(EMPTY_TELEMETRY);
  });

  test('initial BLE state is Disconnected', () => {
    const state = useAppStore.getState();
    expect(state.bleState).toBe(ConnectionState.Disconnected);
  });

  test('setBleState updates connection state', () => {
    useAppStore.getState().setBleState(ConnectionState.Connected);
    expect(useAppStore.getState().bleState).toBe(ConnectionState.Connected);
  });

  test('setTelemetry updates telemetry data', () => {
    const telemetry = {
      ...EMPTY_TELEMETRY,
      speed: 120,
      rpm: 4500,
    };
    useAppStore.getState().setTelemetry(telemetry);
    const state = useAppStore.getState();
    expect(state.telemetry.speed).toBe(120);
    expect(state.telemetry.rpm).toBe(4500);
  });

  test('setBleDevice updates device info', () => {
    useAppStore.getState().setBleDevice('Test-BC', 'AA:BB:CC');
    const state = useAppStore.getState();
    expect(state.bleDeviceName).toBe('Test-BC');
    expect(state.bleDeviceId).toBe('AA:BB:CC');
  });

  test('setDtcList updates DTCs', () => {
    useAppStore.getState().setDtcList(MOCK_DTCS);
    expect(useAppStore.getState().dtcList).toEqual(MOCK_DTCS);
  });

  test('setBleRssi updates RSSI', () => {
    useAppStore.getState().setBleRssi(-65);
    expect(useAppStore.getState().bleRssi).toBe(-65);
  });

  test('setBleMtu updates MTU', () => {
    useAppStore.getState().setBleMtu(247);
    expect(useAppStore.getState().bleMtu).toBe(247);
  });
});

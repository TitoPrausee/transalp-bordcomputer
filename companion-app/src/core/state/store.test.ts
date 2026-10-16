/**
 * store.test.ts — Tests fuer den Zustand-Store
 *
 * Testet:
 *   - Initial state (defaults)
 *   - BLE connection state transitions
 *   - Telemetry updates
 *   - Config updates (set + patch)
 *   - DTC list updates
 *   - Mock mode toggle
 *   - Navigation data updates
 */

import { useAppStore, EMPTY_TELEMETRY, MOCK_TOURS, MOCK_SENSOR_STATUS } from './store';
import { ConnectionState } from '../ble/connection-state';
import { DEFAULT_CONFIG } from './config';

// Reset store before each test
beforeEach(() => {
  useAppStore.setState({
    ble: {
      state: ConnectionState.Disconnected,
      deviceName: null,
      deviceId: null,
      rssi: 0,
      mtu: 0,
      isEncrypted: false,
    },
    telemetry: { ...EMPTY_TELEMETRY },
    config: { ...DEFAULT_CONFIG },
    navManeuver: null,
    navRoute: null,
    navRoad: null,
    navStatus: 0,
    dtcList: [],
    sensorStatus: { ...MOCK_SENSOR_STATUS },
    selfTestResult: null,
    weather: {
      temperature: 182,
      humidity: 65,
      condition: 1,
      windSpeed: 12,
      feelsLike: 180,
      rainProbability: 20,
    },
    systemStatus: null,
    taskInfo: [],
    otaStatus: null,
    tours: [...MOCK_TOURS],
    isMockMode: true,
  });
});

describe('Store initial state', () => {
  it('should start with disconnected BLE state', () => {
    const state = useAppStore.getState();
    expect(state.ble.state).toBe(ConnectionState.Disconnected);
    expect(state.ble.deviceName).toBeNull();
    expect(state.ble.rssi).toBe(0);
  });

  it('should start with empty telemetry', () => {
    const state = useAppStore.getState();
    expect(state.telemetry).toEqual(EMPTY_TELEMETRY);
    expect(state.telemetry.speed).toBe(0);
    expect(state.telemetry.rpm).toBe(0);
  });

  it('should start with default config', () => {
    const state = useAppStore.getState();
    expect(state.config.brightness).toBe(70);
    expect(state.config.theme).toBe('dark');
    expect(state.config.rpmRedline).toBe(8000);
  });

  it('should start in mock mode', () => {
    const state = useAppStore.getState();
    expect(state.isMockMode).toBe(true);
  });

  it('should start with mock tours', () => {
    const state = useAppStore.getState();
    expect(state.tours.length).toBeGreaterThan(0);
    expect(state.tours[0].distanceKm).toBeGreaterThan(0);
  });
});

describe('BLE state transitions', () => {
  it('should update BLE connection state', () => {
    useAppStore.getState().setBleState(ConnectionState.Scanning);
    expect(useAppStore.getState().ble.state).toBe(ConnectionState.Scanning);

    useAppStore.getState().setBleState(ConnectionState.Connected);
    expect(useAppStore.getState().ble.state).toBe(ConnectionState.Connected);
    expect(useAppStore.getState().ble.isEncrypted).toBe(true);
  });

  it('should update BLE RSSI', () => {
    useAppStore.getState().setBleRssi(-72);
    expect(useAppStore.getState().ble.rssi).toBe(-72);
  });

  it('should update BLE MTU', () => {
    useAppStore.getState().setBleMtu(247);
    expect(useAppStore.getState().ble.mtu).toBe(247);
  });

  it('should set BLE device info', () => {
    useAppStore.getState().setBleDevice('Transalp-BC', 'device-123');
    const ble = useAppStore.getState().ble;
    expect(ble.deviceName).toBe('Transalp-BC');
    expect(ble.deviceId).toBe('device-123');
  });

  it('should mark encrypted when connected or degrading', () => {
    useAppStore.getState().setBleState(ConnectionState.Connected);
    expect(useAppStore.getState().ble.isEncrypted).toBe(true);

    useAppStore.getState().setBleState(ConnectionState.Degrading);
    expect(useAppStore.getState().ble.isEncrypted).toBe(true);
  });

  it('should not mark encrypted when disconnected', () => {
    useAppStore.getState().setBleState(ConnectionState.Connected);
    useAppStore.getState().setBleState(ConnectionState.Disconnected);
    expect(useAppStore.getState().ble.isEncrypted).toBe(false);
  });
});

describe('Telemetry updates', () => {
  it('should update telemetry data', () => {
    const newData = {
      speed: 120,
      rpm: 6500,
      gear: 4,
      coolantTemp: 95,
      ambientTemp: 22,
      fuel: 80,
      voltage: 126,
      latitude: 509300000,
      longitude: 115800000,
      altitude: 350,
      heading: 2700,
      satellites: 9,
      hdop: 12,
    };

    useAppStore.getState().setTelemetry(newData);
    const state = useAppStore.getState();
    expect(state.telemetry.speed).toBe(120);
    expect(state.telemetry.rpm).toBe(6500);
    expect(state.telemetry.satellites).toBe(9);
  });

  it('should replace telemetry, not merge', () => {
    useAppStore.getState().setTelemetry({
      ...EMPTY_TELEMETRY,
      speed: 100,
    });

    // Other fields should be from EMPTY_TELEMETRY, not previous state
    const state = useAppStore.getState();
    expect(state.telemetry.speed).toBe(100);
    expect(state.telemetry.rpm).toBe(0);  // from EMPTY_TELEMETRY
  });
});

describe('Config updates', () => {
  it('should replace entire config', () => {
    const newConfig = { ...DEFAULT_CONFIG, brightness: 90, theme: 'light' as const };
    useAppStore.getState().setConfig(newConfig);
    expect(useAppStore.getState().config.brightness).toBe(90);
    expect(useAppStore.getState().config.theme).toBe('light');
  });

  it('should patch config partially', () => {
    useAppStore.getState().updateConfig({ brightness: 50 });
    const state = useAppStore.getState();
    expect(state.config.brightness).toBe(50);
    // Other fields should remain at defaults
    expect(state.config.theme).toBe('dark');
    expect(state.config.rpmRedline).toBe(8000);
  });

  it('should patch multiple config fields', () => {
    useAppStore.getState().updateConfig({
      brightness: 30,
      rpmRedline: 9500,
      shiftLight: 8500,
    });
    const state = useAppStore.getState();
    expect(state.config.brightness).toBe(30);
    expect(state.config.rpmRedline).toBe(9500);
    expect(state.config.shiftLight).toBe(8500);
  });
});

describe('DTC list updates', () => {
  it('should update DTC list', () => {
    const dtcs = [
      { code: 0x0101, severity: 1, timestamp: 100 },
      { code: 0x0402, severity: 2, timestamp: 200 },
    ];
    useAppStore.getState().setDtcList(dtcs);
    expect(useAppStore.getState().dtcList.length).toBe(2);
    expect(useAppStore.getState().dtcList[0].code).toBe(0x0101);
  });

  it('should clear DTC list', () => {
    useAppStore.getState().setDtcList([
      { code: 0x0101, severity: 1, timestamp: 100 },
    ]);
    useAppStore.getState().setDtcList([]);
    expect(useAppStore.getState().dtcList.length).toBe(0);
  });
});

describe('Sensor status updates', () => {
  it('should update sensor status', () => {
    const status = { gps: 1, rpm: 2, temp: 0, fuel: 1, sdCard: 0 };
    useAppStore.getState().setSensorStatus(status);
    expect(useAppStore.getState().sensorStatus).toEqual(status);
  });
});

describe('Mock mode toggle', () => {
  it('should toggle mock mode', () => {
    expect(useAppStore.getState().isMockMode).toBe(true);
    useAppStore.getState().setMockMode(false);
    expect(useAppStore.getState().isMockMode).toBe(false);
    useAppStore.getState().setMockMode(true);
    expect(useAppStore.getState().isMockMode).toBe(true);
  });
});

describe('Navigation updates', () => {
  it('should update nav maneuver', () => {
    const maneuver = { type: 2, distance: 500, streetName: 'Main St' };
    useAppStore.getState().setNavManeuver(maneuver);
    expect(useAppStore.getState().navManeuver).toEqual(maneuver);
  });

  it('should update nav route', () => {
    const route = { totalDistance: 10000, remainingDistance: 8000, eta: 1234567890, remainingTime: 600 };
    useAppStore.getState().setNavRoute(route);
    expect(useAppStore.getState().navRoute).toEqual(route);
  });

  it('should update nav status', () => {
    useAppStore.getState().setNavStatus(1);
    expect(useAppStore.getState().navStatus).toBe(1);
  });
});

describe('System status and OTA', () => {
  it('should update system status', () => {
    const status = {
      uptime: 3600,
      heapFree: 145000,
      heapTotal: 320000,
      batteryPct: 85,
      cpuTemp: 45,
      taskCount: 12,
      bleConnections: 1,
      wifiRssi: -60,
    };
    useAppStore.getState().setSystemStatus(status);
    expect(useAppStore.getState().systemStatus).toEqual(status);
  });

  it('should update OTA status', () => {
    const ota = {
      state: 1,
      progress: 50,
      firmwareVersion: '1.3.0',
      errorCode: 0,
      imageHash: 'abc123',
    };
    useAppStore.getState().setOtaStatus(ota);
    expect(useAppStore.getState().otaStatus).toEqual(ota);
  });
});

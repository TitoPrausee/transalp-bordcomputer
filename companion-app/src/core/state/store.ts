/**
 * Globaler App-State mit Zustand.
 *
 * Verwaltet:
 * - BLE-Connection-State
 * - Telemetrie-Daten (Live)
 * - Config-Modell
 * - Navigation-Daten
 * - Diagnose-Daten (DTC, Sensor-Status)
 * - Wetter-Daten
 */

import { create } from 'zustand';
import { ConnectionState } from '../ble/connection-state';
import type {
  TelemetryLive,
  NavManeuver,
  NavRoute,
  NavRoad,
  SensorStatus,
  DtcEntry,
  SelfTestResult,
  WeatherCurrent,
} from '../ble/gatt-uuids';
import type { BordcomputerConfig } from './config';
import { DEFAULT_CONFIG } from './config';

export interface BleConnectionInfo {
  state: ConnectionState;
  deviceName: string | null;
  deviceId: string | null;
  rssi: number;
  mtu: number;
  isEncrypted: boolean;
}

export interface TourSummary {
  id: string;
  date: string; // ISO-Date
  distanceKm: number;
  durationMin: number;
  maxSpeed: number;
  avgSpeed: number;
  maxRpm: number;
}

export const EMPTY_TELEMETRY: TelemetryLive = {
  speed: 0,
  rpm: 0,
  gear: 0,
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

// --- Mock-Tourdaten ---

export const MOCK_TOURS: TourSummary[] = [
  {
    id: 'tour-001',
    date: '2026-10-13',
    distanceKm: 142,
    durationMin: 204,
    maxSpeed: 128,
    avgSpeed: 68,
    maxRpm: 7800,
  },
  {
    id: 'tour-002',
    date: '2026-10-10',
    distanceKm: 86,
    durationMin: 112,
    maxSpeed: 112,
    avgSpeed: 54,
    maxRpm: 7200,
  },
  {
    id: 'tour-003',
    date: '2026-10-06',
    distanceKm: 213,
    durationMin: 285,
    maxSpeed: 134,
    avgSpeed: 71,
    maxRpm: 8100,
  },
  {
    id: 'tour-004',
    date: '2026-09-29',
    distanceKm: 58,
    durationMin: 72,
    maxSpeed: 98,
    avgSpeed: 48,
    maxRpm: 6500,
  },
];

// --- Mock-DTC-Liste ---

export const MOCK_DTCS: DtcEntry[] = [
  // Keine aktiven Fehler im Normalzustand
];

// --- Mock-Sensor-Status ---

export const MOCK_SENSOR_STATUS: SensorStatus = {
  gps: 0,
  rpm: 0,
  temp: 0,
  fuel: 0,
  sdCard: 0,
};

export const MOCK_WEATHER: WeatherCurrent = {
  temperature: 182,
  humidity: 65,
  condition: 1,
  windSpeed: 12,
  feelsLike: 180,
  rainProbability: 20,
};

// --- Store ---

interface AppState {
  // BLE-Connection
  ble: BleConnectionInfo;
  setBleState: (state: ConnectionState) => void;
  setBleRssi: (rssi: number) => void;
  setBleMtu: (mtu: number) => void;
  setBleDevice: (name: string, id: string) => void;

  // Telemetrie
  telemetry: TelemetryLive;
  setTelemetry: (data: TelemetryLive) => void;

  // Config
  config: BordcomputerConfig;
  setConfig: (config: BordcomputerConfig) => void;
  updateConfig: (patch: Partial<BordcomputerConfig>) => void;

  // Navigation
  navManeuver: NavManeuver | null;
  navRoute: NavRoute | null;
  navRoad: NavRoad | null;
  navStatus: number;
  setNavManeuver: (data: NavManeuver) => void;
  setNavRoute: (data: NavRoute) => void;
  setNavRoad: (data: NavRoad) => void;
  setNavStatus: (status: number) => void;

  // Diagnose
  dtcList: DtcEntry[];
  sensorStatus: SensorStatus;
  selfTestResult: SelfTestResult | null;
  setDtcList: (entries: DtcEntry[]) => void;
  setSensorStatus: (data: SensorStatus) => void;
  setSelfTestResult: (result: SelfTestResult) => void;

  // Wetter
  weather: WeatherCurrent;
  setWeather: (data: WeatherCurrent) => void;

  // Tours (Mock)
  tours: TourSummary[];

  // Verbindungs-Symbol-Status
  isMockMode: boolean;
  setMockMode: (on: boolean) => void;
}

export const useAppStore = create<AppState>((set) => ({
  // BLE
  ble: {
    state: ConnectionState.Disconnected,
    deviceName: null,
    deviceId: null,
    rssi: 0,
    mtu: 0,
    isEncrypted: false,
  },
  setBleState: (state) =>
    set((s) => ({
      ble: {
        ...s.ble,
        state,
        isEncrypted:
          state === ConnectionState.Connected ||
          state === ConnectionState.Degrading,
      },
    })),
  setBleRssi: (rssi) => set((s) => ({ ble: { ...s.ble, rssi } })),
  setBleMtu: (mtu) => set((s) => ({ ble: { ...s.ble, mtu } })),
  setBleDevice: (name, id) => set((s) => ({ ble: { ...s.ble, deviceName: name, deviceId: id } })),

  // Telemetrie
  telemetry: EMPTY_TELEMETRY,
  setTelemetry: (data) => set({ telemetry: data }),

  // Config
  config: { ...DEFAULT_CONFIG },
  setConfig: (config) => set({ config }),
  updateConfig: (patch) => set((s) => ({ config: { ...s.config, ...patch } })),

  // Navigation
  navManeuver: null,
  navRoute: null,
  navRoad: null,
  navStatus: 0,
  setNavManeuver: (data) => set({ navManeuver: data }),
  setNavRoute: (data) => set({ navRoute: data }),
  setNavRoad: (data) => set({ navRoad: data }),
  setNavStatus: (status) => set({ navStatus: status }),

  // Diagnose
  dtcList: MOCK_DTCS,
  sensorStatus: MOCK_SENSOR_STATUS,
  selfTestResult: null,
  setDtcList: (entries) => set({ dtcList: entries }),
  setSensorStatus: (data) => set({ sensorStatus: data }),
  setSelfTestResult: (result) => set({ selfTestResult: result }),

  // Wetter
  weather: MOCK_WEATHER,
  setWeather: (data) => set({ weather: data }),

  // Tours
  tours: MOCK_TOURS,

  // Mock-Mode
  isMockMode: true,
  setMockMode: (on) => set({ isMockMode: on }),
}));

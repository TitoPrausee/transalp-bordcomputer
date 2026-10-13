/**
 * Mock-BLE-Provider.
 *
 * Da kein echtes Geraet verfuegbar ist, simuliert dieser Provider die
 * gesamte BLE-Kommunikation. Er erzeugt realistische Telemetrie-Daten
 * (Speed schwankt, RPM etc.) und erlaubt das Testen der UI ohne Hardware.
 *
 * Die simulierte State-Machine durchlaeuft:
 *   Disconnected -> Scanning (2s) -> Connecting (1s) -> Discovering (500ms) ->
 *   Authenticating (500ms) -> Connected
 */

import type { BleProvider, BleDevice, BleEventCallbacks, Subscription } from './BleProvider';
import type {
  TelemetryLive,
  NavManeuver,
  NavRoute,
  NavRoad,
  SensorStatus,
  DtcEntry,
  SelfTestResult,
  WeatherCurrent,
  SystemStatus,
  TaskInfo,
  OtaStatus,
} from './gatt-uuids';
import { ManeuverType, NavStatus, SelfTestMode } from './gatt-uuids';
import {
  ConnectionState,
  canTransition,
  reconnectDelay,
  RECONNECT_MAX_ATTEMPTS,
} from './connection-state';

const MOCK_DEVICE: BleDevice = {
  id: 'mock-bordcomputer-001',
  name: 'Transalp-BC',
  rssi: -65,
};

const SCAN_DURATION_MS = 2000;
const CONNECT_DURATION_MS = 1000;
const DISCOVER_DURATION_MS = 500;
const AUTH_DURATION_MS = 500;
const TELEMETRY_INTERVAL_MS = 250; // 4 Hz
const RSSI_INTERVAL_MS = 5000;
const HEARTBEAT_INTERVAL_MS = 10000;
const NAV_UPDATE_INTERVAL_MS = 2000;

export class MockBleProvider implements BleProvider {
  private callbacks: BleEventCallbacks[] = [];
  private state: ConnectionState = ConnectionState.Disconnected;
  private device: BleDevice | null = null;
  private timers: ReturnType<typeof setTimeout>[] = [];
  private intervals: ReturnType<typeof setInterval>[] = [];
  private reconnectAttempts = 0;
  private destroyed = false;
  private bootTime = Date.now();
  private otaProgress = 0;
  private otaActive = false;

  private telemetry: TelemetryLive = {
    speed: 0,
    rpm: 1200,
    gear: 0,
    coolantTemp: 82,
    ambientTemp: 18,
    fuel: 75,
    voltage: 124,
    latitude: 509300000,
    longitude: 115800000,
    altitude: 412,
    heading: 0,
    satellites: 9,
    hdop: 12,
  };

  private navManeuver: NavManeuver = {
    type: ManeuverType.Straight,
    distance: 340,
    streetName: 'Bundesstrasse 7',
  };

  private navRoute: NavRoute = {
    totalDistance: 42000,
    remainingDistance: 38000,
    eta: Math.floor(Date.now() / 1000) + 1680,
    remainingTime: 1680,
  };

  private navRoad: NavRoad = {
    speedLimit: 100,
    roadName: 'Bundesstrasse 7',
  };

  private sensorStatus: SensorStatus = {
    gps: 0,
    rpm: 0,
    temp: 0,
    fuel: 0,
    sdCard: 0,
  };

  private dtcList: DtcEntry[] = [];

  private weather: WeatherCurrent = {
    temperature: 182,
    humidity: 65,
    condition: 1,
    windSpeed: 12,
    feelsLike: 180,
    rainProbability: 20,
  };

  // --- Lifecycle ---

  async init(): Promise<void> {
    // Simuliert BLE-Subsystem-Init
    await this.delay(100);
  }

  async isBluetoothEnabled(): Promise<boolean> {
    return true;
  }

  // --- Scan & Connect ---

  async startScan(): Promise<void> {
    if (this.destroyed) return;
    this.setState(ConnectionState.Scanning);

    // Nach 2 Sekunden "Geraet gefunden"
    this.timers.push(
      setTimeout(() => {
        if (this.state !== ConnectionState.Scanning) return;
        this.device = { ...MOCK_DEVICE };
        this.connect(this.device.id);
      }, SCAN_DURATION_MS),
    );
  }

  async stopScan(): Promise<void> {
    if (this.state === ConnectionState.Scanning) {
      this.setState(ConnectionState.Disconnected);
    }
    this.clearTimers();
  }

  async connect(_deviceId: string): Promise<void> {
    if (!this.transition(ConnectionState.Connecting)) return;

    // LL-Connect-Handshake
    this.timers.push(
      setTimeout(() => {
        if (!this.transition(ConnectionState.Discovering)) return;

        // GATT-Discovery
        this.timers.push(
          setTimeout(() => {
            if (!this.transition(ConnectionState.Authenticating)) return;

            // Encryption
            this.timers.push(
              setTimeout(() => {
                if (!this.transition(ConnectionState.Connected)) return;
                this.reconnectAttempts = 0;
                this.startTelemetryStream();
                this.startRssiMonitoring();
                this.startHeartbeat();
                this.startNavUpdates();

                // Initial snapshot senden
                this.emitTelemetry();
              }, AUTH_DURATION_MS),
            );
          }, DISCOVER_DURATION_MS),
        );
      }, CONNECT_DURATION_MS),
    );
  }

  async disconnect(): Promise<void> {
    this.stopStreams();
    this.clearTimers();
    this.device = null;
    this.setState(ConnectionState.Disconnected);
  }

  // --- GATT-Reads ---

  async readTelemetrySnapshot(): Promise<Partial<TelemetryLive>> {
    return { ...this.telemetry };
  }

  async readConfig(): Promise<string> {
    return JSON.stringify({
      brightness: 70,
      theme: 'dark',
      autoDim: true,
      gpsRate: 10,
      rpmRedline: 8000,
      shiftLight: 7000,
      bmpQnh: 1013,
      deviceName: 'Transalp-BC',
      logFormat: 'csv_binary',
      logInterval: 5,
    });
  }

  async writeConfig(_configJson: string): Promise<void> {
    // Simuliert atomic write
    await this.delay(50);
  }

  async triggerSelfTest(_mode?: SelfTestMode): Promise<void> {
    const result: SelfTestResult = {
      result: 0, // pass
      details: [0, 0, 0, 0, 0], // GPS, RPM, Temp, Fuel, SD
    };
    // Nach 1s Ergebnis senden
    this.timers.push(
      setTimeout(() => {
        this.emit('onSelfTestResult', result);
      }, 1000),
    );
  }

  async deleteAllBonds(): Promise<void> {
    await this.delay(100);
  }

  async readDtcList(): Promise<DtcEntry[]> {
    return [...this.dtcList];
  }

  async clearDtc(_code?: number): Promise<void> {
    if (_code === undefined || _code === 0xffff) {
      this.dtcList = [];
    } else {
      this.dtcList = this.dtcList.filter((d) => d.code !== _code);
    }
    this.emit('onDtcList', [...this.dtcList]);
    await this.delay(50);
  }

  async readSystemStatus(): Promise<SystemStatus | null> {
    return {
      uptime: Math.floor((Date.now() - this.bootTime) / 1000),
      heapFree: 145000 + Math.floor(Math.random() * 20000),
      heapTotal: 320000,
      batteryPct: 75 + Math.floor(Math.random() * 20),
      cpuTemp: 42 + Math.round(Math.random() * 8),
      taskCount: 12,
      bleConnections: 1,
      wifiRssi: -55 - Math.floor(Math.random() * 15),
    };
  }

  async readTaskInfo(): Promise<TaskInfo[]> {
    return [
      { name: 'main', priority: 5, stackFree: 8192, state: 0 },
      { name: 'ble_srv', priority: 7, stackFree: 4096, state: 0 },
      { name: 'sensors', priority: 8, stackFree: 3072, state: 0 },
      { name: 'display', priority: 6, stackFree: 6144, state: 0 },
      { name: 'logger', priority: 4, stackFree: 5120, state: 0 },
      { name: 'gps', priority: 7, stackFree: 3584, state: 0 },
      { name: 'ota', priority: 3, stackFree: 2048, state: 2 },
      { name: 'watchdog', priority: 9, stackFree: 1024, state: 0 },
    ];
  }

  async readOtaStatus(): Promise<OtaStatus | null> {
    return {
      state: 0,
      progress: 0,
      firmwareVersion: '1.2.0',
      errorCode: 0,
      imageHash: '',
    };
  }

  async startOta(_firmwareSize: number): Promise<void> {
    this.otaProgress = 0;
    this.otaActive = true;
    this.emit('onOtaStatus', {
      state: 1,
      progress: 0,
      firmwareVersion: '1.2.0',
      errorCode: 0,
      imageHash: '',
    } as OtaStatus);
  }

  async writeOtaData(_data: Uint8Array): Promise<void> {
    this.otaProgress = Math.min(100, this.otaProgress + 2);
    this.emit('onOtaStatus', {
      state: 1,
      progress: this.otaProgress,
      firmwareVersion: '1.2.0',
      errorCode: 0,
      imageHash: '',
    } as OtaStatus);
  }

  async cancelOta(): Promise<void> {
    this.otaActive = false;
    this.otaProgress = 0;
  }

  async verifyOta(): Promise<void> {
    this.emit('onOtaStatus', {
      state: 3,
      progress: 100,
      firmwareVersion: '1.3.0',
      errorCode: 0,
      imageHash: 'a1b2c3d4',
    } as OtaStatus);
  }

  async exportConfig(): Promise<string> {
    return this.readConfig();
  }

  async importConfig(_configJson: string): Promise<void> {
    await this.delay(50);
  }

  // --- Event-Subscription ---

  subscribe(callbacks: BleEventCallbacks): Subscription {
    this.callbacks.push(callbacks);
    return () => {
      this.callbacks = this.callbacks.filter((cb) => cb !== callbacks);
    };
  }

  getConnectionState(): ConnectionState {
    return this.state;
  }

  getConnectedDevice(): BleDevice | null {
    return this.device;
  }

  // --- Simulierte Datenstroeme ---

  private startTelemetryStream(): void {
    this.intervals.push(
      setInterval(() => {
        if (this.state !== ConnectionState.Connected && this.state !== ConnectionState.Degrading) {
          return;
        }
        this.updateMockTelemetry();
        this.emitTelemetry();
      }, TELEMETRY_INTERVAL_MS),
    );
  }

  private startRssiMonitoring(): void {
    this.intervals.push(
      setInterval(() => {
        if (!this.device) return;
        // RSSI schwankt leicht
        const rssi = -65 + Math.round((Math.random() - 0.5) * 10);
        this.device = { ...this.device, rssi };

        // Bei schlechtem RSSI -> DEGRADING, bei gutem -> Connected
        if (rssi < -90 && this.state === ConnectionState.Connected) {
          this.transition(ConnectionState.Degrading);
        } else if (rssi > -85 && this.state === ConnectionState.Degrading) {
          this.transition(ConnectionState.Connected);
        }

        this.emit('onRssi', rssi);
      }, RSSI_INTERVAL_MS),
    );
  }

  private startHeartbeat(): void {
    this.intervals.push(
      setInterval(() => {
        if (this.state === ConnectionState.Disconnected) {
          this.startReconnect();
        }
      }, HEARTBEAT_INTERVAL_MS),
    );
  }

  private startNavUpdates(): void {
    this.intervals.push(
      setInterval(() => {
        if (this.state !== ConnectionState.Connected && this.state !== ConnectionState.Degrading) {
          return;
        }
        // Maneuver-Distanz reduziert sich
        this.navManeuver = {
          ...this.navManeuver,
          distance: Math.max(0, this.navManeuver.distance - 30),
        };
        if (this.navManeuver.distance === 0) {
          // Naechstes Maneuver
          const types = [ManeuverType.Left, ManeuverType.Right, ManeuverType.Straight];
          this.navManeuver = {
            type: types[Math.floor(Math.random() * types.length)],
            distance: 200 + Math.floor(Math.random() * 400),
            streetName: 'Landstrasse',
          };
        }
        this.emit('onNavManeuver', this.navManeuver);
        this.emit('onNavRoute', this.navRoute);
        this.emit('onNavRoad', this.navRoad);
      }, NAV_UPDATE_INTERVAL_MS),
    );
  }

  private updateMockTelemetry(): void {
    // Speed: beschleunigt und bremst
    const speedDelta = (Math.random() - 0.4) * 10;
    this.telemetry.speed = Math.max(0, Math.min(180, Math.round(this.telemetry.speed + speedDelta)));

    // RPM korreliert mit Speed
    const targetRpm = 2000 + this.telemetry.speed * 40;
    const rpmDelta = (targetRpm - this.telemetry.rpm) * 0.15 + (Math.random() - 0.5) * 200;
    this.telemetry.rpm = Math.max(800, Math.round(this.telemetry.rpm + rpmDelta));

    // Gear: schaltet bei bestimmten RPM
    if (this.telemetry.rpm > 7000) {
      this.telemetry.gear = Math.min(6, this.telemetry.gear + 1);
      this.telemetry.rpm = 3000;
    } else if (this.telemetry.rpm < 1500 && this.telemetry.gear > 1) {
      this.telemetry.gear = Math.max(0, this.telemetry.gear - 1);
      this.telemetry.rpm = 5000;
    }

    // Temp: schwankt leicht um 82-88
    this.telemetry.coolantTemp = 82 + Math.round((Math.random() - 0.3) * 8);

    // Fuel: sinkt langsam
    if (Math.random() < 0.01) {
      this.telemetry.fuel = Math.max(0, this.telemetry.fuel - 1);
    }

    // Voltage: 12.0V - 14.2V
    this.telemetry.voltage = 120 + Math.round(Math.random() * 22);

    // Heading: dreht leicht
    this.telemetry.heading = (this.telemetry.heading + Math.round((Math.random() - 0.5) * 20)) % 3600;
  }

  // --- Auto-Reconnect ---

  private startReconnect(): void {
    if (this.state === ConnectionState.Reconnecting) return;
    this.transition(ConnectionState.Reconnecting);

    const attempt = this.reconnectAttempts;
    this.reconnectAttempts++;

    if (attempt >= RECONNECT_MAX_ATTEMPTS) {
      // Reduziert auf 30s-Intervall
      this.timers.push(
        setTimeout(() => {
          if (this.state !== ConnectionState.Reconnecting) return;
          this.startScan();
        }, 30000),
      );
    } else {
      const delay = reconnectDelay(attempt);
      this.timers.push(
        setTimeout(() => {
          if (this.state !== ConnectionState.Reconnecting) return;
          this.startScan();
        }, delay),
      );
    }
  }

  // --- Hilfsmethoden ---

  private setState(newState: ConnectionState): void {
    if (this.state === newState) return;
    if (newState !== ConnectionState.Disconnected && !canTransition(this.state, newState)) {
      console.warn(`Invalid transition ${this.state} -> ${newState}, forcing.`);
    }
    this.state = newState;
    this.emit('onConnectionStateChanged', newState);
  }

  private transition(newState: ConnectionState): boolean {
    if (!canTransition(this.state, newState)) {
      console.warn(`Cannot transition ${this.state} -> ${newState}`);
      return false;
    }
    this.state = newState;
    this.emit('onConnectionStateChanged', newState);
    return true;
  }

  private emit<K extends keyof BleEventCallbacks>(event: K, data: unknown): void {
    for (const cb of this.callbacks) {
      const handler = cb[event];
      if (handler) {
        (handler as (d: unknown) => void)(data);
      }
    }
  }

  private emitTelemetry(): void {
    this.emit('onTelemetry', { ...this.telemetry });
  }

  private delay(ms: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, ms));
  }

  private clearTimers(): void {
    this.timers.forEach((t) => clearTimeout(t));
    this.timers = [];
  }

  private stopStreams(): void {
    this.intervals.forEach((i) => clearInterval(i));
    this.intervals = [];
  }

  destroy(): void {
    this.destroyed = true;
    this.stopStreams();
    this.clearTimers();
    this.callbacks = [];
  }
}

/**
 * RealBleProvider — react-native-ble-plx Implementierung des BleProvider-Interface.
 *
 * Verwendet die react-native-ble-plx Bibliothek fuer echte BLE-Kommunikation
 * mit dem Transalp Bordcomputer.
 *
 * Production-Features:
 *  - Auto-Reconnect mit exponentiellem Backoff (1s, 2s, 4s, 8s, max 30s)
 *  - Connection-Loss-Detection (device.onDisconnected)
 *  - Operation-Timeouts (Scan, Connect, Read, Write)
 *  - Graceful Degradation bei schlechtem RSSI (< -90 dBm)
 *  - CarPlay-Status-Notifications
 *  - RSSI-Monitoring
 *
 * Diese Klasse wird in der Produktion verwendet. Fuer Tests und Development
 * wird MockBleProvider verwendet.
 */

import { BleManager as PlxBleManager, State as BluetoothState } from 'react-native-ble-plx';
import type {
  BleProvider,
  BleDevice,
  BleEventCallbacks,
  Subscription,
} from './BleProvider';
import { BleErrorCode } from './BleProvider';
import { ConnectionState, reconnectDelay, RECONNECT_MAX_ATTEMPTS } from './connection-state';
import { SERVICE, CHAR, DEVICE_NAME } from './gatt-uuids';
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
  CarPlayStatus,
} from './gatt-uuids';
import { SelfTestMode, OtaCommand, CarPlayControl } from './gatt-uuids';
import {
  base64ToDataView,
  dataViewToBase64,
  parseTelemetryLive,
  parseSensorStatus,
  parseDtcList,
  parseNavManeuver,
  parseNavRoute,
  parseNavRoad,
  parseWeatherCurrent,
  parseSystemStatus,
  parseTaskInfo,
  parseOtaStatus,
  parseSelfTestResult,
  parseCarPlayStatus,
  encodeSelfTestCommand,
  encodeDtcClearCommand,
  encodeOtaCommand,
  encodeOtaDataChunk,
  encodeCarPlayControlCommand,
} from './binary-parsers';

// --- Timing-Konstanten ---

const SCAN_TIMEOUT_MS = 10_000;
const CONNECT_TIMEOUT_MS = 15_000;
const READ_TIMEOUT_MS = 10_000;
const WRITE_TIMEOUT_MS = 10_000;
const RSSI_POLL_INTERVAL_MS = 5_000;
const RSSI_DEGRADING_THRESHOLD = -90;
const RSSI_RECOVER_THRESHOLD = -85;

/** Map von Characteristic-UUIDs auf Handler-Funktionen. */
interface CharHandler {
  uuid: string;
  service: string;
  onValue: (dv: DataView) => void;
}

export class RealBleProvider implements BleProvider {
  private plx: PlxBleManager;
  private callbacks: BleEventCallbacks = {};
  private connectionState: ConnectionState = ConnectionState.Disconnected;
  private connectedDevice: BleDevice | null = null;
  private connectedDeviceId: string | null = null;
  private subscriptions: (() => void)[] = [];
  private scanTimeout: ReturnType<typeof setTimeout> | null = null;
  private rssiInterval: ReturnType<typeof setInterval> | null = null;
  private reconnectTimer: ReturnType<typeof setTimeout> | null = null;
  private reconnectAttempts = 0;
  private isReconnecting = false;
  private destroyed = false;

  constructor() {
    this.plx = new PlxBleManager();
  }

  async init(): Promise<void> {
    this.plx.onStateChange((state: BluetoothState) => {
      if (state === BluetoothState.PoweredOff) {
        this.handleConnectionLost('Bluetooth powered off');
        this.callbacks.onError?.({
          code: BleErrorCode.BluetoothDisabled,
          message: 'Bluetooth is disabled',
          recoverable: true,
        });
      } else if (state === BluetoothState.PoweredOn) {
        // Bluetooth re-enabled — attempt reconnect if we were connected
        if (
          this.connectionState === ConnectionState.Disconnected &&
          this.connectedDeviceId &&
          !this.isReconnecting
        ) {
          this.startReconnect();
        }
      }
    });
  }

  async isBluetoothEnabled(): Promise<boolean> {
    const state = await this.plx.state();
    return state === BluetoothState.PoweredOn;
  }

  async startScan(): Promise<void> {
    if (this.destroyed) return;
    const btEnabled = await this.isBluetoothEnabled();
    if (!btEnabled) {
      this.callbacks.onError?.({
        code: BleErrorCode.BluetoothDisabled,
        message: 'Bluetooth is not enabled',
        recoverable: true,
      });
      return;
    }

    this.setConnectionState(ConnectionState.Scanning);

    if (this.scanTimeout) clearTimeout(this.scanTimeout);
    this.scanTimeout = setTimeout(() => {
      this.plx.stopDeviceScan();
      if (this.connectionState === ConnectionState.Scanning) {
        this.callbacks.onError?.({
          code: BleErrorCode.ScanFailed,
          message: 'Scan timeout — no Bordcomputer found',
          recoverable: true,
        });
        this.setConnectionState(ConnectionState.Disconnected);
      }
    }, SCAN_TIMEOUT_MS);

    this.plx.startDeviceScan(null, null, (error, device) => {
      if (error) {
        this.callbacks.onError?.({
          code: BleErrorCode.ScanFailed,
          message: error.message,
          recoverable: true,
        });
        return;
      }
      if (device && device.name === DEVICE_NAME) {
        this.plx.stopDeviceScan();
        if (this.scanTimeout) clearTimeout(this.scanTimeout);
        this.connect(device.id);
      }
    });
  }

  async stopScan(): Promise<void> {
    this.plx.stopDeviceScan();
    if (this.scanTimeout) clearTimeout(this.scanTimeout);
  }

  async connect(deviceId: string): Promise<void> {
    if (this.destroyed) return;
    this.setConnectionState(ConnectionState.Connecting);
    this.connectedDeviceId = deviceId;

    try {
      const device = await this.withTimeout(
        this.plx.connectToDevice(deviceId, {
          autoConnect: true,
          requestMTU: 247,
        }),
        CONNECT_TIMEOUT_MS,
        'Connection timeout',
      );

      await this.withTimeout(
        device.discoverAllServicesAndCharacteristics(),
        CONNECT_TIMEOUT_MS,
        'GATT discovery timeout',
      );

      this.connectedDevice = {
        id: device.id,
        name: device.name || DEVICE_NAME,
        rssi: device.rssi || 0,
      };

      // Listen for unexpected disconnections
      device.onDisconnected((_, disconnectedDevice) => {
        if (disconnectedDevice.id === deviceId) {
          this.handleConnectionLost('Device disconnected');
        }
      });

      // Reset reconnect attempts on successful connection
      this.reconnectAttempts = 0;
      this.isReconnecting = false;

      this.setConnectionState(ConnectionState.Connected);
      this.callbacks.onRssi?.(this.connectedDevice.rssi);
      this.callbacks.onMtu?.(247);

      this.setupNotifications(device);
      this.startRssiMonitoring(device);
    } catch (e) {
      const message = e instanceof Error ? e.message : 'Connection failed';
      this.setConnectionState(ConnectionState.Disconnected);
      this.callbacks.onError?.({
        code: BleErrorCode.ConnectionFailed,
        message,
        recoverable: true,
      });
    }
  }

  private setupNotifications(device: import('react-native-ble-plx').Device): void {
    const handlers: CharHandler[] = [
      { uuid: CHAR.tlmLive, service: SERVICE.telemetry, onValue: (dv) => this.callbacks.onTelemetry?.(parseTelemetryLive(dv)) },
      { uuid: CHAR.navManeuver, service: SERVICE.navigation, onValue: (dv) => this.callbacks.onNavManeuver?.(parseNavManeuver(dv)) },
      { uuid: CHAR.navRoute, service: SERVICE.navigation, onValue: (dv) => this.callbacks.onNavRoute?.(parseNavRoute(dv)) },
      { uuid: CHAR.navRoad, service: SERVICE.navigation, onValue: (dv) => this.callbacks.onNavRoad?.(parseNavRoad(dv)) },
      { uuid: CHAR.sensorStatus, service: SERVICE.diagnose, onValue: (dv) => this.callbacks.onSensorStatus?.(parseSensorStatus(dv)) },
      { uuid: CHAR.dtcList, service: SERVICE.diagnose, onValue: (dv) => this.callbacks.onDtcList?.(parseDtcList(dv)) },
      { uuid: CHAR.selfTest, service: SERVICE.diagnose, onValue: (dv) => this.callbacks.onSelfTestResult?.(parseSelfTestResult(dv)) },
      { uuid: CHAR.wxCurrent, service: SERVICE.weather, onValue: (dv) => this.callbacks.onWeatherCurrent?.(parseWeatherCurrent(dv)) },
      { uuid: CHAR.systemStatus, service: SERVICE.diagnose, onValue: (dv) => this.callbacks.onSystemStatus?.(parseSystemStatus(dv)) },
      { uuid: CHAR.taskInfo, service: SERVICE.diagnose, onValue: (dv) => this.callbacks.onTaskInfo?.(parseTaskInfo(dv)) },
      { uuid: CHAR.otaStatus, service: SERVICE.ota, onValue: (dv) => this.callbacks.onOtaStatus?.(parseOtaStatus(dv)) },
      { uuid: CHAR.carplayStatus, service: SERVICE.carplay, onValue: (dv) => this.callbacks.onCarPlayStatus?.(parseCarPlayStatus(dv)) },
    ];

    for (const h of handlers) {
      const sub = device.monitorCharacteristicForService(
        h.service, h.uuid,
        (error, characteristic) => {
          if (error || !characteristic?.value) return;
          try {
            h.onValue(base64ToDataView(characteristic.value));
          } catch (parseErr) {
            this.callbacks.onError?.({
              code: BleErrorCode.Unknown,
              message: `Parse error for ${h.uuid}: ${parseErr instanceof Error ? parseErr.message : String(parseErr)}`,
              recoverable: false,
            });
          }
        }
      );
      this.subscriptions.push(() => { try { sub.remove(); } catch { /* ignore */ } });
    }
  }

  /** Startet periodisches RSSI-Monitoring fuer Graceful Degradation. */
  private startRssiMonitoring(device: import('react-native-ble-plx').Device): void {
    if (this.rssiInterval) clearInterval(this.rssiInterval);

    this.rssiInterval = setInterval(async () => {
      if (this.connectionState !== ConnectionState.Connected &&
          this.connectionState !== ConnectionState.Degrading) {
        return;
      }
      try {
        const refreshed = await device.readRSSI();
        const rssi = refreshed.rssi ?? 0;
        if (this.connectedDevice) {
          this.connectedDevice = { ...this.connectedDevice, rssi };
        }
        this.callbacks.onRssi?.(rssi);

        // Graceful degradation: Connected -> Degrading bei schlechtem RSSI
        if (rssi < RSSI_DEGRADING_THRESHOLD && this.connectionState === ConnectionState.Connected) {
          this.setConnectionState(ConnectionState.Degrading);
        } else if (rssi > RSSI_RECOVER_THRESHOLD && this.connectionState === ConnectionState.Degrading) {
          this.setConnectionState(ConnectionState.Connected);
        }
      } catch {
        // RSSI read failed — non-critical, ignore
      }
    }, RSSI_POLL_INTERVAL_MS);
  }

  /** Behandelt unerwarteten Verbindungsabbruch und startet Auto-Reconnect. */
  private handleConnectionLost(reason: string): void {
    // Subscriptions und Timer bereinigen
    for (const unsub of this.subscriptions) {
      try { unsub(); } catch { /* ignore */ }
    }
    this.subscriptions = [];
    if (this.rssiInterval) {
      clearInterval(this.rssiInterval);
      this.rssiInterval = null;
    }

    this.connectedDevice = null;
    this.callbacks.onError?.({
      code: BleErrorCode.ConnectionFailed,
      message: reason,
      recoverable: true,
    });

    this.startReconnect();
  }

  /** Startet Auto-Reconnect mit exponentiellem Backoff. */
  private startReconnect(): void {
    if (this.destroyed || this.isReconnecting) return;
    if (!this.connectedDeviceId) return;

    this.isReconnecting = true;
    this.setConnectionState(ConnectionState.Reconnecting);

    const attempt = this.reconnectAttempts;
    this.reconnectAttempts++;

    const delay = reconnectDelay(attempt);

    this.reconnectTimer = setTimeout(() => {
      if (this.destroyed) return;
      if (this.reconnectAttempts > RECONNECT_MAX_ATTEMPTS) {
        // Max attempts reached — back off to slow interval
        this.reconnectAttempts = 0;
      }
      this.isReconnecting = false;
      // Re-scan to find the device again
      this.startScan();
    }, delay);
  }

  async disconnect(): Promise<void> {
    // Reconnect-Timer stoppen
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
    this.isReconnecting = false;

    for (const unsub of this.subscriptions) {
      try { unsub(); } catch { /* ignore */ }
    }
    this.subscriptions = [];

    if (this.rssiInterval) {
      clearInterval(this.rssiInterval);
      this.rssiInterval = null;
    }

    if (this.connectedDevice) {
      try {
        await this.plx.cancelDeviceConnection(this.connectedDevice.id);
      } catch { /* ignore — device may already be disconnected */ }
      this.connectedDevice = null;
    }
    this.setConnectionState(ConnectionState.Disconnected);
  }

  async readTelemetrySnapshot(): Promise<Partial<TelemetryLive>> {
    if (!this.connectedDevice) return {};
    try {
      const char = await this.withTimeout(
        this.plx.readCharacteristicForDevice(
          this.connectedDevice.id, SERVICE.telemetry, CHAR.tlmSnapshot
        ),
        READ_TIMEOUT_MS,
        'Telemetry read timeout',
      );
      if (!char?.value) return {};
      return parseTelemetryLive(base64ToDataView(char.value));
    } catch (e) {
      this.emitReadError('Telemetry', e);
      return {};
    }
  }

  async readConfig(): Promise<string> {
    if (!this.connectedDevice) return '';
    try {
      const char = await this.withTimeout(
        this.plx.readCharacteristicForDevice(
          this.connectedDevice.id, SERVICE.config, CHAR.configRead
        ),
        READ_TIMEOUT_MS,
        'Config read timeout',
      );
      return char?.value || '';
    } catch (e) {
      this.emitReadError('Config', e);
      return '';
    }
  }

  async writeConfig(configJson: string): Promise<void> {
    if (!this.connectedDevice) return;
    try {
      await this.withTimeout(
        this.plx.writeCharacteristicWithResponseForDevice(
          this.connectedDevice.id, SERVICE.config, CHAR.configWrite,
          dataViewToBase64(new DataView(new TextEncoder().encode(configJson).buffer))
        ),
        WRITE_TIMEOUT_MS,
        'Config write timeout',
      );
    } catch (e) {
      this.emitWriteError('Config', e);
    }
  }

  async triggerSelfTest(mode?: SelfTestMode): Promise<void> {
    if (!this.connectedDevice) return;
    try {
      const cmd = encodeSelfTestCommand(mode || SelfTestMode.Quick);
      await this.withTimeout(
        this.plx.writeCharacteristicWithResponseForDevice(
          this.connectedDevice.id, SERVICE.diagnose, CHAR.selfTest,
          dataViewToBase64(new DataView(cmd.buffer))
        ),
        WRITE_TIMEOUT_MS,
        'Self-test write timeout',
      );
    } catch (e) {
      this.emitWriteError('Self-test', e);
    }
  }

  async deleteAllBonds(): Promise<void> {
    if (!this.connectedDevice) return;
    try {
      await this.withTimeout(
        this.plx.writeCharacteristicWithResponseForDevice(
          this.connectedDevice.id, SERVICE.config, CHAR.bondMgmt,
          dataViewToBase64(new DataView(new Uint8Array([0x00]).buffer))
        ),
        WRITE_TIMEOUT_MS,
        'Bond deletion timeout',
      );
    } catch (e) {
      this.emitWriteError('Bond deletion', e);
    }
  }

  async readDtcList(): Promise<DtcEntry[]> {
    if (!this.connectedDevice) return [];
    try {
      const char = await this.withTimeout(
        this.plx.readCharacteristicForDevice(
          this.connectedDevice.id, SERVICE.diagnose, CHAR.dtcList
        ),
        READ_TIMEOUT_MS,
        'DTC read timeout',
      );
      if (!char?.value) return [];
      return parseDtcList(base64ToDataView(char.value));
    } catch (e) {
      this.emitReadError('DTC', e);
      return [];
    }
  }

  async clearDtc(code?: number): Promise<void> {
    if (!this.connectedDevice) return;
    try {
      const cmd = encodeDtcClearCommand(code);
      await this.withTimeout(
        this.plx.writeCharacteristicWithResponseForDevice(
          this.connectedDevice.id, SERVICE.diagnose, CHAR.dtcList,
          dataViewToBase64(new DataView(cmd.buffer))
        ),
        WRITE_TIMEOUT_MS,
        'DTC clear timeout',
      );
    } catch (e) {
      this.emitWriteError('DTC clear', e);
    }
  }

  async readSystemStatus(): Promise<SystemStatus | null> {
    if (!this.connectedDevice) return null;
    try {
      const char = await this.withTimeout(
        this.plx.readCharacteristicForDevice(
          this.connectedDevice.id, SERVICE.diagnose, CHAR.systemStatus
        ),
        READ_TIMEOUT_MS,
        'System status read timeout',
      );
      if (!char?.value) return null;
      return parseSystemStatus(base64ToDataView(char.value));
    } catch (e) {
      this.emitReadError('System status', e);
      return null;
    }
  }

  async readTaskInfo(): Promise<TaskInfo[]> {
    if (!this.connectedDevice) return [];
    try {
      const char = await this.withTimeout(
        this.plx.readCharacteristicForDevice(
          this.connectedDevice.id, SERVICE.diagnose, CHAR.taskInfo
        ),
        READ_TIMEOUT_MS,
        'Task info read timeout',
      );
      if (!char?.value) return [];
      return parseTaskInfo(base64ToDataView(char.value));
    } catch (e) {
      this.emitReadError('Task info', e);
      return [];
    }
  }

  async readOtaStatus(): Promise<OtaStatus | null> {
    if (!this.connectedDevice) return null;
    try {
      const char = await this.withTimeout(
        this.plx.readCharacteristicForDevice(
          this.connectedDevice.id, SERVICE.ota, CHAR.otaStatus
        ),
        READ_TIMEOUT_MS,
        'OTA status read timeout',
      );
      if (!char?.value) return null;
      return parseOtaStatus(base64ToDataView(char.value));
    } catch (e) {
      this.emitReadError('OTA status', e);
      return null;
    }
  }

  async startOta(_firmwareSize: number): Promise<void> {
    if (!this.connectedDevice) return;
    try {
      const cmd = encodeOtaCommand(OtaCommand.Start);
      await this.withTimeout(
        this.plx.writeCharacteristicWithResponseForDevice(
          this.connectedDevice.id, SERVICE.ota, CHAR.otaControl,
          dataViewToBase64(new DataView(cmd.buffer))
        ),
        WRITE_TIMEOUT_MS,
        'OTA start timeout',
      );
    } catch (e) {
      this.emitWriteError('OTA start', e);
    }
  }

  async writeOtaData(data: Uint8Array): Promise<void> {
    if (!this.connectedDevice) return;
    try {
      const chunk = encodeOtaDataChunk(data);
      await this.withTimeout(
        this.plx.writeCharacteristicWithoutResponseForDevice(
          this.connectedDevice.id, SERVICE.ota, CHAR.otaData,
          dataViewToBase64(new DataView(chunk.buffer))
        ),
        WRITE_TIMEOUT_MS,
        'OTA data write timeout',
      );
    } catch (e) {
      this.emitWriteError('OTA data', e);
    }
  }

  async cancelOta(): Promise<void> {
    if (!this.connectedDevice) return;
    try {
      const cmd = encodeOtaCommand(OtaCommand.Cancel);
      await this.withTimeout(
        this.plx.writeCharacteristicWithResponseForDevice(
          this.connectedDevice.id, SERVICE.ota, CHAR.otaControl,
          dataViewToBase64(new DataView(cmd.buffer))
        ),
        WRITE_TIMEOUT_MS,
        'OTA cancel timeout',
      );
    } catch (e) {
      this.emitWriteError('OTA cancel', e);
    }
  }

  async verifyOta(): Promise<void> {
    if (!this.connectedDevice) return;
    try {
      const cmd = encodeOtaCommand(OtaCommand.Verify);
      await this.withTimeout(
        this.plx.writeCharacteristicWithResponseForDevice(
          this.connectedDevice.id, SERVICE.ota, CHAR.otaControl,
          dataViewToBase64(new DataView(cmd.buffer))
        ),
        WRITE_TIMEOUT_MS,
        'OTA verify timeout',
      );
    } catch (e) {
      this.emitWriteError('OTA verify', e);
    }
  }

  async exportConfig(): Promise<string> {
    return this.readConfig();
  }

  async importConfig(configJson: string): Promise<void> {
    return this.writeConfig(configJson);
  }

  // --- CarPlay ---

  async readCarPlayStatus(): Promise<CarPlayStatus | null> {
    if (!this.connectedDevice) return null;
    try {
      const char = await this.withTimeout(
        this.plx.readCharacteristicForDevice(
          this.connectedDevice.id, SERVICE.carplay, CHAR.carplayStatus
        ),
        READ_TIMEOUT_MS,
        'CarPlay status read timeout',
      );
      if (!char?.value) return null;
      return parseCarPlayStatus(base64ToDataView(char.value));
    } catch (e) {
      this.emitReadError('CarPlay status', e);
      return null;
    }
  }

  async sendCarPlayControl(cmd: CarPlayControl): Promise<void> {
    if (!this.connectedDevice) return;
    try {
      const data = encodeCarPlayControlCommand(cmd);
      await this.withTimeout(
        this.plx.writeCharacteristicWithResponseForDevice(
          this.connectedDevice.id, SERVICE.carplay, CHAR.carplayControl,
          dataViewToBase64(new DataView(data.buffer))
        ),
        WRITE_TIMEOUT_MS,
        'CarPlay control timeout',
      );
    } catch (e) {
      this.emitWriteError('CarPlay control', e);
    }
  }

  subscribe(callbacks: BleEventCallbacks): Subscription {
    this.callbacks = callbacks;
    return () => { this.callbacks = {}; };
  }

  getConnectionState(): ConnectionState {
    return this.connectionState;
  }

  getConnectedDevice(): BleDevice | null {
    return this.connectedDevice;
  }

  destroy(): void {
    this.destroyed = true;
    this.isReconnecting = false;
    for (const unsub of this.subscriptions) {
      try { unsub(); } catch { /* ignore */ }
    }
    this.subscriptions = [];
    if (this.scanTimeout) clearTimeout(this.scanTimeout);
    if (this.reconnectTimer) clearTimeout(this.reconnectTimer);
    if (this.rssiInterval) clearInterval(this.rssiInterval);
    this.rssiInterval = null;
    try {
      this.plx.destroy();
    } catch { /* ignore */ }
  }

  private setConnectionState(state: ConnectionState): void {
    this.connectionState = state;
    this.callbacks.onConnectionStateChanged?.(state);
  }

  /** Wrap a promise with a timeout. Rejects with an Error if the timeout fires. */
  private withTimeout<T>(promise: Promise<T>, ms: number, timeoutMessage: string): Promise<T> {
    return new Promise<T>((resolve, reject) => {
      const timer = setTimeout(() => {
        reject(new Error(timeoutMessage));
      }, ms);
      promise.then(
        (result) => { clearTimeout(timer); resolve(result); },
        (err) => { clearTimeout(timer); reject(err); },
      );
    });
  }

  private emitReadError(operation: string, e: unknown): void {
    const message = e instanceof Error ? e.message : String(e);
    this.callbacks.onError?.({
      code: BleErrorCode.ReadFailed,
      message: `${operation} read failed: ${message}`,
      recoverable: true,
    });
  }

  private emitWriteError(operation: string, e: unknown): void {
    const message = e instanceof Error ? e.message : String(e);
    this.callbacks.onError?.({
      code: BleErrorCode.WriteFailed,
      message: `${operation} write failed: ${message}`,
      recoverable: true,
    });
  }
}

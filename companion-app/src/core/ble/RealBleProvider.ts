/**
 * RealBleProvider — react-native-ble-plx Implementierung des BleProvider-Interface.
 *
 * Verwendet die react-native-ble-plx Bibliothek fuer echte BLE-Kommunikation
 * mit dem Transalp Bordcomputer.
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
import { ConnectionState } from './connection-state';
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
} from './gatt-uuids';
import { SelfTestMode, OtaCommand } from './gatt-uuids';
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
  encodeSelfTestCommand,
  encodeDtcClearCommand,
  encodeOtaCommand,
  encodeOtaDataChunk,
} from './binary-parsers';

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
  private subscriptions: (() => void)[] = [];
  private scanTimeout: ReturnType<typeof setTimeout> | null = null;

  constructor() {
    this.plx = new PlxBleManager();
  }

  async init(): Promise<void> {
    this.plx.onStateChange((state: BluetoothState) => {
      if (state === BluetoothState.PoweredOff) {
        this.setConnectionState(ConnectionState.Disconnected);
        this.callbacks.onError?.({
          code: BleErrorCode.BluetoothDisabled,
          message: 'Bluetooth is disabled',
          recoverable: true,
        });
      }
    });
  }

  async isBluetoothEnabled(): Promise<boolean> {
    const state = await this.plx.state();
    return state === BluetoothState.PoweredOn;
  }

  async startScan(): Promise<void> {
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
      this.setConnectionState(ConnectionState.Disconnected);
    }, 10000);

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
    this.setConnectionState(ConnectionState.Connecting);

    try {
      const device = await this.plx.connectToDevice(deviceId, {
        autoConnect: true,
        requestMTU: 247,
      });

      await device.discoverAllServicesAndCharacteristics();

      this.connectedDevice = {
        id: device.id,
        name: device.name || DEVICE_NAME,
        rssi: device.rssi || 0,
      };

      this.setConnectionState(ConnectionState.Connected);
      this.callbacks.onRssi?.(this.connectedDevice.rssi);
      this.callbacks.onMtu?.(247);

      this.setupNotifications(device);
    } catch (e) {
      this.setConnectionState(ConnectionState.Disconnected);
      this.callbacks.onError?.({
        code: BleErrorCode.ConnectionFailed,
        message: e instanceof Error ? e.message : 'Connection failed',
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
    ];

    for (const h of handlers) {
      const sub = device.monitorCharacteristicForService(
        h.service, h.uuid,
        (error, characteristic) => {
          if (error || !characteristic?.value) return;
          h.onValue(base64ToDataView(characteristic.value));
        }
      );
      this.subscriptions.push(sub);
    }
  }

  async disconnect(): Promise<void> {
    for (const unsub of this.subscriptions) {
      try { unsub(); } catch { /* ignore */ }
    }
    this.subscriptions = [];

    if (this.connectedDevice) {
      await this.plx.cancelDeviceConnection(this.connectedDevice.id);
      this.connectedDevice = null;
    }
    this.setConnectionState(ConnectionState.Disconnected);
  }

  async readTelemetrySnapshot(): Promise<Partial<TelemetryLive>> {
    if (!this.connectedDevice) return {};
    const char = await this.plx.readCharacteristicForDevice(
      this.connectedDevice.id, SERVICE.telemetry, CHAR.tlmSnapshot
    );
    if (!char?.value) return {};
    return parseTelemetryLive(base64ToDataView(char.value));
  }

  async readConfig(): Promise<string> {
    if (!this.connectedDevice) return '';
    const char = await this.plx.readCharacteristicForDevice(
      this.connectedDevice.id, SERVICE.config, CHAR.configRead
    );
    return char?.value || '';
  }

  async writeConfig(configJson: string): Promise<void> {
    if (!this.connectedDevice) return;
    await this.plx.writeCharacteristicWithResponseForDevice(
      this.connectedDevice.id, SERVICE.config, CHAR.configWrite,
      dataViewToBase64(new DataView(new TextEncoder().encode(configJson).buffer))
    );
  }

  async triggerSelfTest(mode?: SelfTestMode): Promise<void> {
    if (!this.connectedDevice) return;
    const cmd = encodeSelfTestCommand(mode || SelfTestMode.Quick);
    await this.plx.writeCharacteristicWithResponseForDevice(
      this.connectedDevice.id, SERVICE.diagnose, CHAR.selfTest,
      dataViewToBase64(new DataView(cmd.buffer))
    );
  }

  async deleteAllBonds(): Promise<void> {
    if (!this.connectedDevice) return;
    await this.plx.writeCharacteristicWithResponseForDevice(
      this.connectedDevice.id, SERVICE.config, CHAR.bondMgmt,
      dataViewToBase64(new DataView(new Uint8Array([0x00]).buffer))
    );
  }

  async readDtcList(): Promise<DtcEntry[]> {
    if (!this.connectedDevice) return [];
    const char = await this.plx.readCharacteristicForDevice(
      this.connectedDevice.id, SERVICE.diagnose, CHAR.dtcList
    );
    if (!char?.value) return [];
    return parseDtcList(base64ToDataView(char.value));
  }

  async clearDtc(code?: number): Promise<void> {
    if (!this.connectedDevice) return;
    const cmd = encodeDtcClearCommand(code);
    await this.plx.writeCharacteristicWithResponseForDevice(
      this.connectedDevice.id, SERVICE.diagnose, CHAR.dtcList,
      dataViewToBase64(new DataView(cmd.buffer))
    );
  }

  async readSystemStatus(): Promise<SystemStatus | null> {
    if (!this.connectedDevice) return null;
    const char = await this.plx.readCharacteristicForDevice(
      this.connectedDevice.id, SERVICE.diagnose, CHAR.systemStatus
    );
    if (!char?.value) return null;
    return parseSystemStatus(base64ToDataView(char.value));
  }

  async readTaskInfo(): Promise<TaskInfo[]> {
    if (!this.connectedDevice) return [];
    const char = await this.plx.readCharacteristicForDevice(
      this.connectedDevice.id, SERVICE.diagnose, CHAR.taskInfo
    );
    if (!char?.value) return [];
    return parseTaskInfo(base64ToDataView(char.value));
  }

  async readOtaStatus(): Promise<OtaStatus | null> {
    if (!this.connectedDevice) return null;
    const char = await this.plx.readCharacteristicForDevice(
      this.connectedDevice.id, SERVICE.ota, CHAR.otaStatus
    );
    if (!char?.value) return null;
    return parseOtaStatus(base64ToDataView(char.value));
  }

  async startOta(_firmwareSize: number): Promise<void> {
    if (!this.connectedDevice) return;
    const cmd = encodeOtaCommand(OtaCommand.Start);
    await this.plx.writeCharacteristicWithResponseForDevice(
      this.connectedDevice.id, SERVICE.ota, CHAR.otaControl,
      dataViewToBase64(new DataView(cmd.buffer))
    );
  }

  async writeOtaData(data: Uint8Array): Promise<void> {
    if (!this.connectedDevice) return;
    const chunk = encodeOtaDataChunk(data);
    await this.plx.writeCharacteristicWithoutResponseForDevice(
      this.connectedDevice.id, SERVICE.ota, CHAR.otaData,
      dataViewToBase64(new DataView(chunk.buffer))
    );
  }

  async cancelOta(): Promise<void> {
    if (!this.connectedDevice) return;
    const cmd = encodeOtaCommand(OtaCommand.Cancel);
    await this.plx.writeCharacteristicWithResponseForDevice(
      this.connectedDevice.id, SERVICE.ota, CHAR.otaControl,
      dataViewToBase64(new DataView(cmd.buffer))
    );
  }

  async verifyOta(): Promise<void> {
    if (!this.connectedDevice) return;
    const cmd = encodeOtaCommand(OtaCommand.Verify);
    await this.plx.writeCharacteristicWithResponseForDevice(
      this.connectedDevice.id, SERVICE.ota, CHAR.otaControl,
      dataViewToBase64(new DataView(cmd.buffer))
    );
  }

  async exportConfig(): Promise<string> {
    return this.readConfig();
  }

  async importConfig(configJson: string): Promise<void> {
    return this.writeConfig(configJson);
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
    for (const unsub of this.subscriptions) {
      try { unsub(); } catch { /* ignore */ }
    }
    this.subscriptions = [];
    if (this.scanTimeout) clearTimeout(this.scanTimeout);
    this.plx.destroy();
  }

  private setConnectionState(state: ConnectionState): void {
    this.connectionState = state;
    this.callbacks.onConnectionStateChanged?.(state);
  }
}

/**
 * BleManager — High-Level BLE-Verbindungsmanager.
 *
 * Diese Klasse kapselt die gesamte BLE-Kommunikation mit dem Bordcomputer:
 * - Scan nach "Transalp-BC" Geraeten
 * - Verbindungsaufbau mit Auto-Reconnect und Backoff
 * - GATT-Service-Discovery
 * - Subscription auf alle notifizierenden Characteristics
 * - Binary-Parser fuer alle Datenformate
 * - Schreibende Operationen (DTC loeschen, Self-Test, OTA, Config)
 *
 * Sie abstrahiert den BleProvider und bietet eine ereignisbasierte API,
 * die vom useBleManager-Hook mit dem Zustand-Store verdrahtet wird.
 *
 * Im Prototyp wird der MockBleProvider verwendet. Bei echtem Geraet
 * wuerde hier der RealBleProvider (react-native-ble-plx) instanziiert.
 */

import type { BleProvider, BleDevice, BleEventCallbacks, Subscription, BleError } from '../core/ble/BleProvider';
import { BleErrorCode } from '../core/ble/BleProvider';
import { MockBleProvider } from '../core/ble/MockBleProvider';
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
} from '../core/ble/gatt-uuids';
import { SelfTestMode, OtaCommand, CarPlayControl } from '../core/ble/gatt-uuids';
import { ConnectionState, isConnected } from '../core/ble/connection-state';
import {
  parseTelemetryLive,
  parseSensorStatus,
  parseDtcList,
  parseSelfTestResult,
  parseSystemStatus,
  parseTaskInfo,
  parseOtaStatus,
  parseWeatherCurrent,
  parseNavManeuver,
  parseNavRoute,
  parseNavRoad,
  parseCarPlayStatus,
  base64ToDataView,
  encodeSelfTestCommand,
  encodeDtcClearCommand,
  encodeOtaCommand,
  encodeOtaDataChunk,
  encodeCarPlayControlCommand,
  uint8ArrayToBase64,
} from '../core/ble/binary-parsers';
import { SERVICE, CHAR, DEVICE_NAME } from '../core/ble/gatt-uuids';

// --- OTA-Konstanten ---

const OTA_CHUNK_SIZE = 180; // Bytes pro BLE-Write (MTU 247 - Header)
const OTA_CHUNK_DELAY_MS = 20; // Kleine Pause zwischen Chunks

// --- Typen ---

export interface BleManagerConfig {
  autoReconnect: boolean;
  scanTimeoutMs: number;
  mtu: number;
}

export const DEFAULT_BLE_MANAGER_CONFIG: BleManagerConfig = {
  autoReconnect: true,
  scanTimeoutMs: 10000,
  mtu: 247,
};

// --- BleManager ---

export class BleManager {
  private provider: BleProvider;
  private config: BleManagerConfig;
  private subscription: Subscription | null = null;
  private listeners: BleEventCallbacks[] = [];
  private connected = false;

  constructor(provider: BleProvider, config?: Partial<BleManagerConfig>) {
    this.provider = provider;
    this.config = { ...DEFAULT_BLE_MANAGER_CONFIG, ...config };
  }

  // --- Lifecycle ---

  async init(): Promise<void> {
    await this.provider.init();

    // Event-Callbacks mit dem Provider verdrahten
    this.subscription = this.provider.subscribe({
      onConnectionStateChanged: (state) => this.handleConnectionStateChange(state),
      onTelemetry: (data) => this.dispatch('onTelemetry', data),
      onNavManeuver: (data) => this.dispatch('onNavManeuver', data),
      onNavRoute: (data) => this.dispatch('onNavRoute', data),
      onNavRoad: (data) => this.dispatch('onNavRoad', data),
      onNavStatus: (status) => this.dispatch('onNavStatus', status),
      onSensorStatus: (data) => this.dispatch('onSensorStatus', data),
      onDtcList: (entries) => this.dispatch('onDtcList', entries),
      onSelfTestResult: (result) => this.dispatch('onSelfTestResult', result),
      onWeatherCurrent: (data) => this.dispatch('onWeatherCurrent', data),
      onSystemStatus: (data) => this.dispatch('onSystemStatus', data),
      onTaskInfo: (tasks) => this.dispatch('onTaskInfo', tasks),
      onOtaStatus: (status) => this.dispatch('onOtaStatus', status),
      onCarPlayStatus: (status) => this.dispatch('onCarPlayStatus', status),
      onRssi: (rssi) => this.dispatch('onRssi', rssi),
      onMtu: (mtu) => this.dispatch('onMtu', mtu),
      onError: (error) => this.dispatch('onError', error),
    });
  }

  // --- Scan & Connect ---

  async startScan(): Promise<void> {
    const btEnabled = await this.provider.isBluetoothEnabled();
    if (!btEnabled) {
      this.emitError(BleErrorCode.BluetoothDisabled, 'Bluetooth ist deaktiviert', false);
      return;
    }
    try {
      await this.provider.startScan();
    } catch (e) {
      this.emitError(BleErrorCode.ScanFailed, `Scan fehlgeschlagen: ${this.errMsg(e)}`, true);
    }
  }

  async stopScan(): Promise<void> {
    await this.provider.stopScan();
  }

  async connect(deviceId: string): Promise<void> {
    try {
      await this.provider.connect(deviceId);
    } catch (e) {
      this.emitError(BleErrorCode.ConnectionFailed, `Verbindung fehlgeschlagen: ${this.errMsg(e)}`, true);
    }
  }

  async disconnect(): Promise<void> {
    await this.provider.disconnect();
    this.connected = false;
  }

  // --- GATT-Reads ---

  async readTelemetrySnapshot(): Promise<Partial<TelemetryLive>> {
    try {
      return await this.provider.readTelemetrySnapshot();
    } catch (e) {
      this.emitError(BleErrorCode.ReadFailed, `Telemetrie-Lesen fehlgeschlagen: ${this.errMsg(e)}`, true);
      return {};
    }
  }

  async readConfig(): Promise<string> {
    try {
      return await this.provider.readConfig();
    } catch (e) {
      this.emitError(BleErrorCode.ReadFailed, `Config-Lesen fehlgeschlagen: ${this.errMsg(e)}`, true);
      return '';
    }
  }

  async readDtcList(): Promise<DtcEntry[]> {
    try {
      return await this.provider.readDtcList();
    } catch (e) {
      this.emitError(BleErrorCode.ReadFailed, `DTC-Lesen fehlgeschlagen: ${this.errMsg(e)}`, true);
      return [];
    }
  }

  async readSystemStatus(): Promise<SystemStatus | null> {
    try {
      return await this.provider.readSystemStatus();
    } catch (e) {
      this.emitError(BleErrorCode.ReadFailed, `System-Status-Lesen fehlgeschlagen: ${this.errMsg(e)}`, true);
      return null;
    }
  }

  async readTaskInfo(): Promise<TaskInfo[]> {
    try {
      return await this.provider.readTaskInfo();
    } catch (e) {
      this.emitError(BleErrorCode.ReadFailed, `Task-Info-Lesen fehlgeschlagen: ${this.errMsg(e)}`, true);
      return [];
    }
  }

  async readOtaStatus(): Promise<OtaStatus | null> {
    try {
      return await this.provider.readOtaStatus();
    } catch (e) {
      this.emitError(BleErrorCode.ReadFailed, `OTA-Status-Lesen fehlgeschlagen: ${this.errMsg(e)}`, true);
      return null;
    }
  }

  // --- Schreibende Operationen ---

  async writeConfig(configJson: string): Promise<void> {
    try {
      await this.provider.writeConfig(configJson);
    } catch (e) {
      this.emitError(BleErrorCode.WriteFailed, `Config-Schreiben fehlgeschlagen: ${this.errMsg(e)}`, true);
    }
  }

  async triggerSelfTest(mode: SelfTestMode = SelfTestMode.Full): Promise<void> {
    try {
      await this.provider.triggerSelfTest(mode);
    } catch (e) {
      this.emitError(BleErrorCode.WriteFailed, `Self-Test-Ausloesung fehlgeschlagen: ${this.errMsg(e)}`, true);
    }
  }

  async deleteAllBonds(): Promise<void> {
    try {
      await this.provider.deleteAllBonds();
    } catch (e) {
      this.emitError(BleErrorCode.WriteFailed, `Bond-Loeschung fehlgeschlagen: ${this.errMsg(e)}`, true);
    }
  }

  async clearDtc(code?: number): Promise<void> {
    try {
      await this.provider.clearDtc(code);
    } catch (e) {
      this.emitError(BleErrorCode.WriteFailed, `DTC-Loeschung fehlgeschlagen: ${this.errMsg(e)}`, true);
    }
  }

  // --- OTA ---

  async startOta(firmwareSize: number): Promise<void> {
    try {
      await this.provider.startOta(firmwareSize);
    } catch (e) {
      this.emitError(BleErrorCode.OtaFailed, `OTA-Start fehlgeschlagen: ${this.errMsg(e)}`, true);
    }
  }

  async uploadOtaFirmware(data: Uint8Array, onProgress?: (sent: number, total: number) => void): Promise<void> {
    const total = data.length;
    let offset = 0;

    try {
      while (offset < total) {
        const end = Math.min(offset + OTA_CHUNK_SIZE, total);
        const chunk = data.slice(offset, end);
        await this.provider.writeOtaData(chunk);
        offset = end;
        onProgress?.(offset, total);

        // Kurze Pause um den BLE-Stack nicht zu ueberlasten
        if (offset < total) {
          await this.delay(OTA_CHUNK_DELAY_MS);
        }
      }
    } catch (e) {
      this.emitError(BleErrorCode.OtaFailed, `OTA-Upload fehlgeschlagen: ${this.errMsg(e)}`, true);
    }
  }

  async cancelOta(): Promise<void> {
    try {
      await this.provider.cancelOta();
    } catch (e) {
      this.emitError(BleErrorCode.OtaFailed, `OTA-Abbruch fehlgeschlagen: ${this.errMsg(e)}`, true);
    }
  }

  async verifyOta(): Promise<void> {
    try {
      await this.provider.verifyOta();
    } catch (e) {
      this.emitError(BleErrorCode.OtaFailed, `OTA-Verifizierung fehlgeschlagen: ${this.errMsg(e)}`, true);
    }
  }

  // --- Config Export/Import ---

  async exportConfig(): Promise<string> {
    try {
      return await this.provider.exportConfig();
    } catch (e) {
      this.emitError(BleErrorCode.ReadFailed, `Config-Export fehlgeschlagen: ${this.errMsg(e)}`, true);
      return '';
    }
  }

  async importConfig(configJson: string): Promise<void> {
    try {
      await this.provider.importConfig(configJson);
    } catch (e) {
      this.emitError(BleErrorCode.WriteFailed, `Config-Import fehlgeschlagen: ${this.errMsg(e)}`, true);
    }
  }

  // --- CarPlay ---

  async readCarPlayStatus(): Promise<CarPlayStatus | null> {
    try {
      return await this.provider.readCarPlayStatus();
    } catch (e) {
      this.emitError(BleErrorCode.ReadFailed, `CarPlay-Status-Lesen fehlgeschlagen: ${this.errMsg(e)}`, true);
      return null;
    }
  }

  async sendCarPlayControl(cmd: CarPlayControl): Promise<void> {
    try {
      await this.provider.sendCarPlayControl(cmd);
    } catch (e) {
      this.emitError(BleErrorCode.WriteFailed, `CarPlay-Control fehlgeschlagen: ${this.errMsg(e)}`, true);
    }
  }

  encodeCarPlayCmd(cmd: CarPlayControl): string {
    return uint8ArrayToBase64(encodeCarPlayControlCommand(cmd));
  }

  // --- Status ---

  getConnectionState(): ConnectionState {
    return this.provider.getConnectionState();
  }

  getConnectedDevice(): BleDevice | null {
    return this.provider.getConnectedDevice();
  }

  is_connected(): boolean {
    return isConnected(this.provider.getConnectionState());
  }

  // --- Event-Listener ---

  addListener(callbacks: BleEventCallbacks): Subscription {
    this.listeners.push(callbacks);
    return () => {
      this.listeners = this.listeners.filter((l) => l !== callbacks);
    };
  }

  // --- Notification-Handler (fuer echte BLE-Provider) ---

  /**
   * Verarbeitet eine BLE-Notification mit Base64-codierten Daten.
   * Wird vom RealBleProvider fuer jede Characteristic aufgerufen.
   */
  handleNotification(characteristicUuid: string, base64Data: string): void {
    const data = base64ToDataView(base64Data);

    try {
      switch (characteristicUuid) {
        case CHAR.tlmLive:
          this.dispatch('onTelemetry', parseTelemetryLive(data));
          break;
        case CHAR.sensorStatus:
          this.dispatch('onSensorStatus', parseSensorStatus(data));
          break;
        case CHAR.dtcList:
          this.dispatch('onDtcList', parseDtcList(data));
          break;
        case CHAR.selfTest:
          this.dispatch('onSelfTestResult', parseSelfTestResult(data));
          break;
        case CHAR.systemStatus:
          this.dispatch('onSystemStatus', parseSystemStatus(data));
          break;
        case CHAR.taskInfo:
          this.dispatch('onTaskInfo', parseTaskInfo(data));
          break;
        case CHAR.otaStatus:
          this.dispatch('onOtaStatus', parseOtaStatus(data));
          break;
        case CHAR.wxCurrent:
          this.dispatch('onWeatherCurrent', parseWeatherCurrent(data));
          break;
        case CHAR.navManeuver:
          this.dispatch('onNavManeuver', parseNavManeuver(data));
          break;
        case CHAR.navRoute:
          this.dispatch('onNavRoute', parseNavRoute(data));
          break;
        case CHAR.navRoad:
          this.dispatch('onNavRoad', parseNavRoad(data));
          break;
        case CHAR.carplayStatus:
          this.dispatch('onCarPlayStatus', parseCarPlayStatus(data));
          break;
        default:
          // Unbekannte Characteristic ignorieren
          break;
      }
    } catch (e) {
      this.emitError(BleErrorCode.Unknown, `Parse-Fehler fuer ${characteristicUuid}: ${this.errMsg(e)}`, false);
    }
  }

  // --- Encode-Helfer fuer Write-Operationen ---

  encodeSelfTest(mode: SelfTestMode): string {
    return uint8ArrayToBase64(encodeSelfTestCommand(mode));
  }

  encodeDtcClear(code?: number): string {
    return uint8ArrayToBase64(encodeDtcClearCommand(code));
  }

  encodeOtaCmd(cmd: OtaCommand): string {
    return uint8ArrayToBase64(encodeOtaCommand(cmd));
  }

  encodeOtaChunk(data: Uint8Array): string {
    return uint8ArrayToBase64(encodeOtaDataChunk(data));
  }

  // --- Cleanup ---

  destroy(): void {
    if (this.subscription) {
      this.subscription();
      this.subscription = null;
    }
    this.listeners = [];
    this.provider.destroy();
    this.connected = false;
  }

  // --- Private ---

  private handleConnectionStateChange(state: ConnectionState): void {
    this.connected = isConnected(state);
    this.dispatch('onConnectionStateChanged', state);
  }

  private dispatch<K extends keyof BleEventCallbacks>(event: K, data: unknown): void {
    for (const listener of this.listeners) {
      const handler = listener[event];
      if (handler) {
        (handler as (d: unknown) => void)(data);
      }
    }
  }

  private emitError(code: BleErrorCode, message: string, recoverable: boolean): void {
    const error: BleError = { code, message, recoverable };
    this.dispatch('onError', error);
  }

  private errMsg(e: unknown): string {
    if (e instanceof Error) return e.message;
    return String(e);
  }

  private delay(ms: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, ms));
  }
}

// --- Singleton-Factory ---

let bleManagerInstance: BleManager | null = null;

/**
 * Erzeugt (oder liefert) den singleton BleManager.
 * Im Prototyp wird der MockBleProvider verwendet.
 */
export function getBleManager(provider?: BleProvider): BleManager {
  if (bleManagerInstance && !provider) {
    return bleManagerInstance;
  }
  // Wenn ein Provider uebergeben wird, neuen Manager erzeugen
  // (nur fuer Tests oder explizite Neuinitialisierung)
  if (bleManagerInstance) {
    bleManagerInstance.destroy();
  }

  const p = provider ?? new MockBleProvider();
  bleManagerInstance = new BleManager(p);
  return bleManagerInstance;
}

export { SERVICE, CHAR, DEVICE_NAME };

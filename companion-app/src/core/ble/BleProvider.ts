/**
 * BLE-Provider-Interface.
 *
 * Definiert die Abstraktionsschicht zwischen der App und der konkreten
 * BLE-Implementierung (echtes Geraet per react-native-ble-plx oder Mock).
 */

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
import { ConnectionState } from './connection-state';
import { SelfTestMode } from './gatt-uuids';

export type Subscription = () => void;

export interface BleDevice {
  id: string;
  name: string;
  rssi: number;
}

export enum BleErrorCode {
  BluetoothDisabled = 'BLUETOOTH_DISABLED',
  ScanFailed = 'SCAN_FAILED',
  ConnectionFailed = 'CONNECTION_FAILED',
  ReadFailed = 'READ_FAILED',
  WriteFailed = 'WRITE_FAILED',
  OtaFailed = 'OTA_FAILED',
  Unknown = 'UNKNOWN',
}

export interface BleError {
  code: BleErrorCode;
  message: string;
  recoverable: boolean;
}

export interface BleEventCallbacks {
  onConnectionStateChanged?: (state: ConnectionState) => void;
  onTelemetry?: (data: TelemetryLive) => void;
  onNavManeuver?: (data: NavManeuver) => void;
  onNavRoute?: (data: NavRoute) => void;
  onNavRoad?: (data: NavRoad) => void;
  onNavStatus?: (status: number) => void;
  onSensorStatus?: (data: SensorStatus) => void;
  onDtcList?: (entries: DtcEntry[]) => void;
  onSelfTestResult?: (result: SelfTestResult) => void;
  onWeatherCurrent?: (data: WeatherCurrent) => void;
  onSystemStatus?: (data: SystemStatus) => void;
  onTaskInfo?: (tasks: TaskInfo[]) => void;
  onOtaStatus?: (status: OtaStatus) => void;
  onRssi?: (rssi: number) => void;
  onMtu?: (mtu: number) => void;
  onError?: (error: BleError) => void;
}

export interface BleProvider {
  /** Initialisiert das BLE-Subsystem. */
  init(): Promise<void>;

  /** Prueft, ob Bluetooth aktiviert ist. */
  isBluetoothEnabled(): Promise<boolean>;

  /** Startet den Scan nach dem Bordcomputer. Ruft callbacks.onConnectionStateChanged. */
  startScan(): Promise<void>;

  /** Stoppt einen laufenden Scan. */
  stopScan(): Promise<void>;

  /** Verbindet sich mit dem Geraet. Loest den State-Machine-Durchlauf aus. */
  connect(deviceId: string): Promise<void>;

  /** Trennt die Verbindung. */
  disconnect(): Promise<void>;

  /** Liest die Telemetrie-Snapshot-Characteristic (0x5002). */
  readTelemetrySnapshot(): Promise<Partial<TelemetryLive>>;

  /** Liest die Config-Read-Characteristic (0x6001). */
  readConfig(): Promise<string>;

  /** Schreibt die Config-Write-Characteristic (0x6002). */
  writeConfig(configJson: string): Promise<void>;

  /** Loest einen Self-Test aus (Write 0x01 an 0x7003). */
  triggerSelfTest(mode?: SelfTestMode): Promise<void>;

  /** Loescht alle Bonding-Keys (Write 0x00 an 0x6005). */
  deleteAllBonds(): Promise<void>;

  /** Liest die DTC-Liste (0x7001). */
  readDtcList(): Promise<DtcEntry[]>;
  clearDtc(code?: number): Promise<void>;
  readSystemStatus(): Promise<SystemStatus | null>;
  readTaskInfo(): Promise<TaskInfo[]>;
  readOtaStatus(): Promise<OtaStatus | null>;
  startOta(firmwareSize: number): Promise<void>;
  writeOtaData(data: Uint8Array): Promise<void>;
  cancelOta(): Promise<void>;
  verifyOta(): Promise<void>;
  exportConfig(): Promise<string>;
  importConfig(configJson: string): Promise<void>;

  /** Registriert Event-Callbacks. Gibt eine unsubscribe-Funktion zurueck. */
  subscribe(callbacks: BleEventCallbacks): Subscription;

  /** Gibt den aktuellen Verbindungsstatus zurueck. */
  getConnectionState(): ConnectionState;

  /** Gibt die verbundene Geraete-ID zurueck (oder null). */
  getConnectedDevice(): BleDevice | null;

  /** Bereinigt alle Ressourcen. */
  destroy(): void;
}

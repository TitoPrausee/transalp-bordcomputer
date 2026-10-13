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
} from './gatt-uuids';
import { ConnectionState } from './connection-state';

export type Subscription = () => void;

export interface BleDevice {
  id: string;
  name: string;
  rssi: number;
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
  onRssi?: (rssi: number) => void;
  onMtu?: (mtu: number) => void;
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
  triggerSelfTest(): Promise<void>;

  /** Loescht alle Bonding-Keys (Write 0x00 an 0x6005). */
  deleteAllBonds(): Promise<void>;

  /** Liest die DTC-Liste (0x7001). */
  readDtcList(): Promise<DtcEntry[]>;

  /** Registriert Event-Callbacks. Gibt eine unsubscribe-Funktion zurueck. */
  subscribe(callbacks: BleEventCallbacks): Subscription;

  /** Gibt den aktuellen Verbindungsstatus zurueck. */
  getConnectionState(): ConnectionState;

  /** Gibt die verbundene Geraete-ID zurueck (oder null). */
  getConnectedDevice(): BleDevice | null;

  /** Bereinigt alle Ressourcen. */
  destroy(): void;
}

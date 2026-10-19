/**
 * GATT-Service- und Characteristic-Definitionen.
 *
 * Uebernommen aus docs/research/13-connectivity-app.md Abschnitt 2.
 * Base-UUID: 6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e
 */

const BASE_UUID_PREFIX = '6e40';
const BASE_UUID_SUFFIX = '-b5a3-f393-e0a9-e50e24dcca9e';

/**
 * Wandelt eine 16-bit-ID in eine 128-bit-UUID um.
 * Beispiel: 0x1001 -> '6e401001-b5a3-f393-e0a9-e50e24dcca9e'
 */
function uuid(id16: number): string {
  const hex = id16.toString(16).padStart(4, '0');
  return `${BASE_UUID_PREFIX}${hex}${BASE_UUID_SUFFIX}`;
}

// --- Service-IDs ---

export const SERVICE = {
  navigation: uuid(0x1000),
  media: uuid(0x2000),
  phone: uuid(0x3000),
  weather: uuid(0x4000),
  telemetry: uuid(0x5000),
  config: uuid(0x6000),
  diagnose: uuid(0x7000),
  ota: uuid(0x8000),
  carplay: uuid(0x9000),
} as const;

// --- Characteristic-IDs ---

export const CHAR = {
  // Navigation Service (0x1000)
  navManeuver: uuid(0x1001),
  navRoute: uuid(0x1002),
  navRoad: uuid(0x1003),
  navStatus: uuid(0x1004),
  navCmd: uuid(0x1005),

  // Media Service (0x2000)
  mediaNowPlaying: uuid(0x2001),
  mediaControl: uuid(0x2002),
  volume: uuid(0x2003),

  // Phone Service (0x3000)
  phoneIncomingCall: uuid(0x3001),
  phoneCallStatus: uuid(0x3002),
  phoneCallControl: uuid(0x3003),
  phoneNotification: uuid(0x3004),

  // Weather Service (0x4000)
  wxCurrent: uuid(0x4001),
  wxForecast: uuid(0x4002),
  wxAlert: uuid(0x4003),

  // Telemetry Service (0x5000)
  tlmLive: uuid(0x5001),
  tlmSnapshot: uuid(0x5002),
  trackExport: uuid(0x5003),
  trackExportCtrl: uuid(0x5004),

  // Config Service (0x6000)
  configRead: uuid(0x6001),
  configWrite: uuid(0x6002),
  configExport: uuid(0x6003),
  configImport: uuid(0x6004),
  bondMgmt: uuid(0x6005),

  // Diagnose Service (0x7000)
  dtcList: uuid(0x7001),
  sensorStatus: uuid(0x7002),
  selfTest: uuid(0x7003),
  systemStatus: uuid(0x7006),
  taskInfo: uuid(0x7007),
  logExport: uuid(0x7004),
  logExportCtrl: uuid(0x7005),

  // OTA Service (0x8000)
  otaControl: uuid(0x8001),
  otaData: uuid(0x8002),
  otaVerify: uuid(0x8003),
  otaStatus: uuid(0x8004),

  // CarPlay Service (0x9000)
  carplayStatus: uuid(0x9001),
  carplayControl: uuid(0x9002),
} as const;

// --- Standard-Services (Bluetooth SIG) ---

export const STD_SERVICE = {
  genericAccess: '00001800-0000-1000-8000-00805f9b34fb',
  deviceInformation: '0000180a-0000-1000-8000-00805f9b34fb',
} as const;

// --- Bordcomputer-Advertising-Identifier ---

/** Der Bordcomputer advertiset die Navigation-Service-UUID. */
export const ADVERTISED_SERVICE_UUID = SERVICE.navigation;

/** Name, nach dem beim BLE-Scan gefiltert wird. */
export const DEVICE_NAME = 'Transalp-BC';

// --- Maneuver-Typen (NAV_MANEUVER Byte 0) ---

export enum ManeuverType {
  Straight = 0,
  SlightLeft = 1,
  Left = 2,
  SharpLeft = 3,
  SlightRight = 4,
  Right = 5,
  SharpRight = 6,
  UTurn = 7,
  Arrive = 8,
  Fork = 9,
}

// --- Telemetrie-Payload (TLM_LIVE, 0x5001, 28 Bytes) ---

export interface TelemetryLive {
  speed: number; // km/h (uint8)
  rpm: number; // U/min (uint16)
  gear: number; // int8, -1 = neutral
  coolantTemp: number; // Celsius (int8)
  ambientTemp: number; // Celsius (int8)
  fuel: number; // Prozent (uint8)
  voltage: number; // Zehntelvolt (uint8)
  latitude: number; // E-7 (int32)
  longitude: number; // E-7 (int32)
  altitude: number; // Meter (int16)
  heading: number; // Zehntelgrad (uint16)
  satellites: number; // (uint8)
  hdop: number; // Zehntel (uint8)
}

// --- Sensor-Status (DIAG_SENSOR, 0x7002, 5 Bytes) ---

export interface SensorStatus {
  gps: number; // 0=ok, 1=no_fix, 2=error
  rpm: number; // 0=ok, 1=no_signal, 2=error
  temp: number; // 0=ok, 1=disconnected, 2=error
  fuel: number; // 0=ok, 1=error
  sdCard: number; // 0=ok, 1=not_present, 2=full, 3=error
}

// --- DTC-Eintrag (DIAG_DTC, 6 Bytes pro Eintrag) ---

export interface DtcEntry {
  code: number; // uint16
  severity: number; // 0=info, 1=warning, 2=critical
  timestamp: number; // uint32 Unix
}

// --- Self-Test-Ergebnis (DIAG_SELFTEST) ---

export interface SelfTestResult {
  result: number; // 0=pass, 1=warn, 2=fail
  details: number[]; // Ein Byte pro Sensor
}

// --- Wetter (WX_CURRENT, 0x4001, 8 Bytes) ---

export interface WeatherCurrent {
  temperature: number; // Zehntelgrad Celsius (int16)
  humidity: number; // Prozent (uint8)
  condition: number; // 0=clear..7=fog (uint8)
  windSpeed: number; // km/h (uint8)
  feelsLike: number; // Zehntelgrad Celsius (int16)
  rainProbability: number; // Prozent naechste 1h (uint8)
}

// --- Navigation-Maneuver (NAV_MANEUVER, 0x1001) ---

export interface NavManeuver {
  type: ManeuverType;
  distance: number; // Meter (uint32)
  streetName: string; // UTF-8, max 64 Bytes
}

// --- Route-Info (NAV_ROUTE, 0x1002) ---

export interface NavRoute {
  totalDistance: number; // Meter (uint32)
  remainingDistance: number; // Meter (uint32)
  eta: number; // Unix-Timestamp (uint32)
  remainingTime: number; // Sekunden (uint32)
}

// --- Current Road (NAV_ROAD, 0x1003) ---

export interface NavRoad {
  speedLimit: number; // km/h, 0=unbekannt (uint8)
  roadName: string; // UTF-8, max 64 Bytes
}

// --- Nav-Status (NAV_STATUS, 0x1004) ---

export enum NavStatus {
  Inactive = 0,
  Active = 1,
  Rerouting = 2,
  Arriving = 3,
  Offline = 4,
}

// --- System-Status (Teil des Telemetry-Snapshot) ---

export interface SystemStatus {
  uptime: number; // Sekunden (uint32)
  heapFree: number; // Bytes (uint32)
  heapTotal: number; // Bytes (uint32)
  batteryPct: number; // Prozent (uint8)
  cpuTemp: number; // Celsius (int8)
  taskCount: number; // (uint8)
  bleConnections: number; // (uint8)
  wifiRssi: number; // dBm (int8)
}

// --- Task-Info (fuer Diagnose) ---

export interface TaskInfo {
  name: string; // 16 Bytes UTF-8
  priority: number; // (uint8)
  stackFree: number; // Bytes (uint16)
  state: number; // (uint8: 0=running,1=blocked,2=suspended,3=deleted)
}

// --- OTA-Status ---

export interface OtaStatus {
  state: number; // 0=idle,1=uploading,2=verifying,3=done,4=failed
  progress: number; // 0-100
  firmwareVersion: string; // major.minor.patch
  errorCode: number; // 0=ok
  imageHash: string; // SHA256 hex
}

// --- Self-Test-Modus ---

export enum SelfTestMode {
  Quick = 0,
  Full = 1,
}

// --- OTA-Kommando ---

export enum OtaCommand {
  Start = 0,
  Cancel = 1,
  Verify = 2,
}

// ============================================================
// CarPlay-Status (0x9001)
// ============================================================
// Byte  Offset  Field
// 0    0       connected (uint8: 0=disconnected, 1=connected)
// 1    1       activeApp (uint8: 0=none, 1=navigation, 2=music, 3=phone, 4=podcasts)
// 2    3       artistLen (uint8) -> artist (UTF-8, max 64 Bytes)
// 3+64  3       artist (64 Bytes, null-terminiert)
// 67+64 67      title (64 Bytes, null-terminiert)
// 131+64 131    album (64 Bytes, null-terminiert)
// ============================================================

/** Aktive CarPlay-App-Kategorie. */
export enum CarPlayApp {
  None = 0,
  Navigation = 1,
  Music = 2,
  Phone = 3,
  Podcasts = 4,
}

export interface CarPlayStatus {
  connected: boolean;
  activeApp: CarPlayApp;
  artist: string;
  title: string;
  album: string;
}

// ============================================================
// CarPlay-Control (0x9002, Write 1 Byte)
// ============================================================
// 0=pair, 1=unpair, 2=next_track, 3=prev_track, 4=play_pause
// ============================================================

export enum CarPlayControl {
  Pair = 0,
  Unpair = 1,
  NextTrack = 2,
  PrevTrack = 3,
  PlayPause = 4,
}

// ============================================================
// Controller-Support
// ============================================================

/** Verfuegbare Controller-Typen fuer die Lenker-Fernbedienung. */
export enum ControllerType {
  None = 0,
  HondaOem = 1,
  GenericBle = 2,
  Keyboard = 3,
}

/** Mapping einer Controller-Taste zu einer App-Aktion. */
export enum ControllerAction {
  None = 0,
  NextTrack = 1,
  PrevTrack = 2,
  VolumeUp = 3,
  VolumeDown = 4,
  PlayPause = 5,
  Navigate = 6,
  AnswerCall = 7,
  EndCall = 8,
  VoiceControl = 9,
  Dashboard = 10,
  CarPlay = 11,
}

/** Eine Taste am Lenker-Controller. */
export enum ControllerButton {
  Up = 0,
  Down = 1,
  Left = 2,
  Right = 3,
  Select = 4,
  Back = 5,
  Mode = 6,
  VolumeUp = 7,
  VolumeDown = 8,
}

/** Tastatur-Keys, die im Mock-Modus auf Controller-Buttons gemappt werden. */
export const CONTROLLER_KEY_MAP: Record<string, ControllerButton> = {
  ArrowUp: ControllerButton.Up,
  ArrowDown: ControllerButton.Down,
  ArrowLeft: ControllerButton.Left,
  ArrowRight: ControllerButton.Right,
  Enter: ControllerButton.Select,
  Escape: ControllerButton.Back,
  'm': ControllerButton.Mode,
  '+': ControllerButton.VolumeUp,
  '-': ControllerButton.VolumeDown,
};

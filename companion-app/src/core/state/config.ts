/**
 * Config-Modell fuer den Bordcomputer.
 * Wird via Config-Read (0x6001) gelesen und Config-Write (0x6002) geschrieben.
 */

export interface BordcomputerConfig {
  // Display
  brightness: number; // 0-100 %
  theme: 'dark' | 'light';
  autoDim: boolean;

  // Sensoren
  gpsRate: number; // Hz
  rpmRedline: number; // U/min
  shiftLight: number; // U/min
  bmpQnh: number; // hPa

  // BLE
  deviceName: string;
  pairedDevices: number;

  // Logging
  logFormat: 'csv' | 'binary' | 'csv_binary';
  logInterval: number; // Sekunden
}

export const DEFAULT_CONFIG: BordcomputerConfig = {
  brightness: 70,
  theme: 'dark',
  autoDim: true,
  gpsRate: 10,
  rpmRedline: 8000,
  shiftLight: 7000,
  bmpQnh: 1013,
  deviceName: 'Transalp-BC',
  pairedDevices: 2,
  logFormat: 'csv_binary',
  logInterval: 5,
};

export function parseConfig(json: string): BordcomputerConfig {
  try {
    const parsed = JSON.parse(json);
    return { ...DEFAULT_CONFIG, ...parsed };
  } catch {
    return { ...DEFAULT_CONFIG };
  }
}

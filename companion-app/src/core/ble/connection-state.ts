/**
 * BLE Connection-State-Machine.
 *
 * Uebernommen aus docs/research/13-connectivity-app.md Abschnitt 1.1.
 *
 * Zustaende:
 *   UNBONDED -> BONDED -> DISCONNECTED -> SCANNING -> CONNECTING ->
 *   DISCOVERING -> AUTHENTICATING -> CONNECTED -> DEGRADING -> RECONNECTING
 *
 * Der Prototyp nutzt eine vereinfachte Maschine mit den fuer die App
 * relevanten Zustaenden. UNBONDED/BONDED sind implizit (das Betriebssystem
 * uebernimmt das Bonding).
 */

export enum ConnectionState {
  /** Keine aktive Verbindung, Advertising laeuft. */
  Disconnected = 'DISCONNECTED',
  /** App sucht aktiv nach dem Bordcomputer per BLE-Scan. */
  Scanning = 'SCANNING',
  /** LL-Connect-Request gesendet, Handshake laeuft. */
  Connecting = 'CONNECTING',
  /** Physische Verbindung steht, GATT-Discovery laeuft. */
  Discovering = 'DISCOVERING',
  /** Verschluesselte Verbindung wird aufgebaut (Bonding-Key). */
  Authenticating = 'AUTHENTICATING',
  /** Vollstaendig etablierte, verschluesselte, MTU-ausgehandelte Verbindung. */
  Connected = 'CONNECTED',
  /** Verbindung steht, aber Signalqualitaet schlecht (RSSI < -90 dBm). */
  Degrading = 'DEGRADING',
  /** Verbindung verloren, Auto-Reconnect mit Backoff laeuft. */
  Reconnecting = 'RECONNECTING',
}

/** Zulaessige Zustaendsuebergaenge. */
const TRANSITIONS: Record<ConnectionState, ConnectionState[]> = {
  [ConnectionState.Disconnected]: [
    ConnectionState.Scanning,
    ConnectionState.Reconnecting,
  ],
  [ConnectionState.Scanning]: [
    ConnectionState.Connecting,
    ConnectionState.Disconnected,
  ],
  [ConnectionState.Connecting]: [
    ConnectionState.Discovering,
    ConnectionState.Scanning,
    ConnectionState.Disconnected,
  ],
  [ConnectionState.Discovering]: [
    ConnectionState.Authenticating,
    ConnectionState.Disconnected,
  ],
  [ConnectionState.Authenticating]: [
    ConnectionState.Connected,
    ConnectionState.Disconnected,
  ],
  [ConnectionState.Connected]: [
    ConnectionState.Degrading,
    ConnectionState.Reconnecting,
    ConnectionState.Disconnected,
  ],
  [ConnectionState.Degrading]: [
    ConnectionState.Connected,
    ConnectionState.Reconnecting,
    ConnectionState.Disconnected,
  ],
  [ConnectionState.Reconnecting]: [
    ConnectionState.Scanning,
    ConnectionState.Disconnected,
  ],
};

export function canTransition(from: ConnectionState, to: ConnectionState): boolean {
  return TRANSITIONS[from]?.includes(to) ?? false;
}

/** Ist der Zustand "verbunden" (Daten fliessen)? */
export function isConnected(state: ConnectionState): boolean {
  return state === ConnectionState.Connected || state === ConnectionState.Degrading;
}

/** Ist der Zustand aktiv (Scan/Connect/Reconnect laeuft)? */
export function isBusy(state: ConnectionState): boolean {
  return (
    state === ConnectionState.Scanning ||
    state === ConnectionState.Connecting ||
    state === ConnectionState.Discovering ||
    state === ConnectionState.Authenticating ||
    state === ConnectionState.Reconnecting
  );
}

// --- Auto-Reconnect Backoff ---

export const RECONNECT_DELAYS_MS = [1000, 2000, 4000, 8000, 16000];
export const RECONNECT_MAX_DELAY_MS = 30000;
export const RECONNECT_MAX_ATTEMPTS = 5;

/** Liefert die Backoff-Verzoegerung fuer den n-ten Versuch (0-basiert). */
export function reconnectDelay(attempt: number): number {
  if (attempt < RECONNECT_DELAYS_MS.length) {
    return RECONNECT_DELAYS_MS[attempt];
  }
  return RECONNECT_MAX_DELAY_MS;
}

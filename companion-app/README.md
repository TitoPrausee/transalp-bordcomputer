# Transalp Companion-App

React-Native-Prototyp der Companion-App fuer den Transalp-Bordcomputer
(Honda Transalp 600 / PD06). Die App verbindet sich per BLE (Bluetooth Low
Energy) mit dem Bordcomputer und zeigt Live-Telemetrie, Navigation,
Diagnose-Daten und Tour-Historie an.

## Architektur

```
companion-app/
  App.tsx                  # Root-Komponente (registriert die App)
  src/
    App.tsx                # NavigationContainer + Theme + BLE-Init
    core/
      ble/
        BleProvider.ts     # Provider-Interface (Abstraktionsschicht)
        MockBleProvider.ts  # Mock-Implementierung (simuliert Geraet)
        gatt-uuids.ts      # Alle GATT-Service/Characteristic-UUIDs + Typen
        connection-state.ts # BLE State-Machine (12 Zustaende)
      state/
        store.ts           # Zustand-Store (globaler App-State)
        config.ts          # Config-Modell (Bordcomputer-Konfiguration)
      theme/
        colors.ts          # Farbpalette, Typografie, Animations-Timings
    components/
      UI.tsx               # Wiederverwendbare UI-Komponenten (Card, StatTile, ...)
      RpmBar.tsx           # Drehzahl-Balken mit Farbverlauf
      SpeedDisplay.tsx     # Grosses Speed-Display mit Rolling-Ziffern
      NavArrow.tsx         # Navigationspfeil mit Fade-Animation
    screens/
      DashboardScreen.tsx  # Live-Telemetrie (Speed, RPM, Temp, Fuel, Gear)
      NavigationScreen.tsx # Karten-Platzhalter + Maneuver-Anzeige
      DiagnoseScreen.tsx   # DTC-Liste, Sensor-Status, Self-Test
      SettingsScreen.tsx   # Config-Editor (Helligkeit, BLE, Logging)
      ToursScreen.tsx      # Tour-Historie mit Statistiken
    navigation/
      TabNavigator.tsx     # Bottom-Tab-Navigator (5 Tabs)
    hooks/
      useBleManager.ts     # BLE-Lifecycle-Manager Hook
```

## BLE-Architektur

### GATT-Services

Alle Services und Characteristics folgen dem projektspezifischen UUID-Schema:
```
Base: 6e40XXXX-b5a3-f393-e0a9-e50e24dcca9e
```

| Service       | ID     | Characteristics                        |
|---------------|--------|----------------------------------------|
| Navigation    | 0x1000 | Maneuver, Route, Road, Status, Cmd     |
| Media         | 0x2000 | NowPlaying, Control, Volume            |
| Phone         | 0x3000 | IncomingCall, CallStatus, Control, Notif|
| Weather       | 0x4000 | Current, Forecast, Alert                |
| Telemetry     | 0x5000 | Live, Snapshot, TrackExport, TrackCtrl |
| Config        | 0x6000 | Read, Write, Export, Import, BondMgmt  |
| Diagnose      | 0x7000 | DTC, SensorStatus, SelfTest, LogExport |
| OTA           | 0x8000 | Control, Data, Verify                   |

### Connection-State-Machine

```
DISCONNECTED -> SCANNING -> CONNECTING -> DISCOVERING ->
  AUTHENTICATING -> CONNECTED -> DEGRADING -> RECONNECTING
```

Mit Auto-Reconnect (exponentieller Backoff: 1s, 2s, 4s, 8s, max 30s).

### Mock-BLE-Provider

Da kein echtes Geraet verfuegbar ist, simuliert der `MockBleProvider`:
- Komplette State-Machine (Scan -> Connect -> Discover -> Auth -> Connected)
- Realistische Telemetrie-Daten (Speed schwankt, RPM korreliert, Gear schaltet)
- Navigation-Updates (Maneuver-Distanz reduziert sich)
- RSSI-Monitoring (schwach -> DEGRADING)
- Auto-Reconnect bei Verbindungsabbruch

## Setup

### Voraussetzungen

- Node.js >= 18
- npm oder yarn
- Expo CLI (wird automatisch installiert)

### Installation

```bash
cd companion-app
npm install
```

### Start

```bash
# Expo Dev-Server starten
npx expo start

# oder explizit:
npx expo start --go    # Expo Go (kein nativer Build noetig)
npx expo start --dev-client  # Mit nativem Dev-Client
```

Die App laesst sich in Expo Go oeffnen (QR-Code scannen). Der
Mock-BLE-Provider erfordert keine echte Hardware — die App zeigt sofort
simulierte Telemetrie-Daten an.

### TypeScript-Check

```bash
npm run lint
```

## Design

Die App uebernimmt das Design des Bordcomputers:

- **Dark-Mode**: Hintergrund #000000, Panel #1C1C1E
- **Akzentfarben**: Blue (#0A84FF) fuer BT/Nav, Green (#30D158) fuer OK,
  Red (#FF3B30) fuer Warnungen, Amber/Orange fuer RPM
- **Typografie**: Inter / system font, 72px Speed, 24px Headings, 16px Body
- **Animationen**: 80-400ms, nicht-linear (ease_out, ease_in_out, ease_out_back)
- **RPM-Farbverlauf**: Gruen (0-40%) -> Gelb (40-70%) -> Orange (70-85%) -> Rot (85-100%)

## State Management

Zustand (minimal, ohne Boilerplate):
- `ble`: Connection-State, RSSI, MTU, Geraete-Info
- `telemetry`: Live-Telemetrie (Speed, RPM, Gear, Temp, Fuel, Volt, GPS)
- `config`: Bordcomputer-Konfiguration
- `navManeuver/navRoute/navRoad/navStatus`: Navigationsdaten
- `dtcList/sensorStatus/selfTestResult`: Diagnose-Daten
- `weather`: Wetterdaten
- `tours`: Tour-Historie (Mock)

## Screens

1. **Dashboard** — Live-Telemetrie mit grossem Speed-Display, RPM-Balken,
   Sekundaerwerten (Gang, Temp, Fuel, Volt) und Verbindungsstatus
2. **Navigation** — Karten-Platzhalter, naechstes Maneuver (Pfeil + Distanz),
   Route-Info (ETA, Reststrecke), Strassenname, Tempolimit
3. **Touren** — Tour-Liste mit Statistiken, Tap auf Tour fuer Detail-Ansicht
   (Distanz, Dauer, Max-Speed, Hoehenprofil als Balkendiagramm)
4. **Diagnose** — DTC-Liste, Sensor-Status-Grid, Self-Test-Button mit
   animiertem Ergebnis
5. **Settings** — Config-Editor mit Helligkeit, Dark-Mode, GPS-Rate,
   RPM-Redline, Shift-Light, BLE-Pairing, Logging-Intervall.
   "Auf Bordcomputer anwenden" sendet Config per BLE.

## Naechste Schritte (Prototyp -> Produktion)

- [x] RealBleProvider mit react-native-ble-plx implementieren
- [x] Auto-Reconnect mit exponentiellem Backoff (1s, 2s, 4s, 8s, max 30s)
- [x] Connection-Loss-Detection (device.onDisconnected)
- [x] Operation-Timeouts (Scan, Connect, Read, Write)
- [x] Graceful Degradation bei schlechtem RSSI (< -90 dBm)
- [x] CarPlay-Status-Notifications in RealBleProvider
- [x] Error Handling: Loading States, Error States, ConnectionBanner
- [x] App Store Metadata in `store/` (Beschreibung, Privacy Policy, Screenshots)
- [ ] Binary-Transfer-Protokoll (fuer Track/Log/OTA) — teilweise implementiert
- [ ] Native Karten-Integration
- [ ] Media- und Phone-Service-Screens
- [ ] OTA-Firmware-Update-Screen
- [ ] SQLite-Persistenz (Telemetrie-Cache, Tour-Store)
- [ ] Wetter-API-Integration

## Production Hardening (TOT-31)

### Error Handling & Reconnection

- **RealBleProvider**: Auto-Reconnect mit `device.onDisconnected`-Listener,
  exponentiellem Backoff (reuses `reconnectDelay` aus `connection-state.ts`)
- **Timeouts**: Alle BLE-Operationen (Scan, Connect, Read, Write) haben
  Timeouts mit `withTimeout()`-Wrapper
- **Graceful Degradation**: RSSI-Monitoring alle 5s, Uebergang
  Connected -> Degrading bei RSSI < -90 dBm
- **Store**: `lastError`, `reconnectAttempts`, `isRetrying` State
- **UI**: `ConnectionBanner` zeigt Reconnect-Status und Fehler mit
  Retry-Button, `LoadingState` und `ErrorState` Komponenten

### App Store Metadata

`store/` enthaelt:
- `metadata.json` — App-Beschreibung (DE/EN), Keywords, Kategorien
- `privacy-policy.md` — Datenschutzerklaerung
- `screenshots/README.md` — Screenshot-Spezifikation (iOS + Android)

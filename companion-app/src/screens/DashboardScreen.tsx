/**
 * Dashboard-Screen.
 *
 * Zeigt Live-Telemetrie vom Bordcomputer (oder Mock-Daten).
 *
 * Features:
 *  - Grosse Tacho-Anzeige (km/h)
 *  - Drehzahl-Bargraph (RPM mit Schaltlicht)
 *  - Temperatur (Motor) mit Farbcodierung
 *  - Tankanzeige (Bargraph mit Reserve-Warnung)
 *  - Gang-Anzeige (gross, zentriert)
 *  - Zusatzdaten: Oeldruck, Spannung, GPS-Speed
 *  - Landscape- und Portrait-Modus
 *  - Hintergrundfarbe: Dunkel (nachtschonend)
 *  - Verbindungsstatus-Indikator oben
 *  - Bluetooth-Signalstaerke-Anzeige
 */

import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  useWindowDimensions,
  Animated,
  Easing,
} from 'react-native';
import { ScreenContainer, ScreenHeader, ConnectionBadge, StatTile, Card, ConnectionBanner } from '../components/UI';
import { SpeedDisplay } from '../components/SpeedDisplay';
import { RpmBar } from '../components/RpmBar';
import { useAppStore } from '../core/state/store';
import { useBleManager } from '../hooks/useBleManager';
import {
  bg,
  fg,
  accent,
  fontSize,
  tempColor,
  fuelColor,
  voltageColor,
} from '../core/theme/colors';
import { isConnected } from '../core/ble/connection-state';

// --- Fuel Bargraph ---

function FuelBar({ fuel }: { fuel: number }) {
  const segments = 10;
  const filled = Math.ceil(fuel / 10);
  const isReserve = fuel < 15;
  const isLow = fuel < 25 && fuel >= 15;

  return (
    <View style={fuelStyles.container}>
      <Text style={fuelStyles.label}>TANK</Text>
      <View style={fuelStyles.barContainer}>
        {Array.from({ length: segments }, (_, i) => {
          const isActive = i < filled;
          let color: string = accent.green;
          if (isReserve) color = accent.red;
          else if (isLow) color = accent.yellow;
          return (
            <View
              key={i}
              style={[
                fuelStyles.segment,
                { backgroundColor: isActive ? color : bg.raised },
              ]}
            />
          );
        })}
      </View>
      <View style={fuelStyles.row}>
        <Text style={[fuelStyles.value, { color: fuelColor(fuel) }]}>{fuel}%</Text>
        {isReserve && (
          <View style={fuelStyles.reserveBadge}>
            <Text style={fuelStyles.reserveText}>RESERVE</Text>
          </View>
        )}
      </View>
    </View>
  );
}

const fuelStyles = StyleSheet.create({
  container: { marginVertical: 4 },
  label: { fontSize: fontSize.label, color: fg.secondary, textTransform: 'uppercase', letterSpacing: 1, marginBottom: 4 },
  barContainer: { flexDirection: 'row', gap: 3, height: 20 },
  segment: { flex: 1, borderRadius: 3 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginTop: 4 },
  value: { fontSize: fontSize.body, fontWeight: 'bold' },
  reserveBadge: { backgroundColor: accent.red, paddingHorizontal: 6, paddingVertical: 2, borderRadius: 4 },
  reserveText: { fontSize: fontSize.tiny, color: '#FFFFFF', fontWeight: 'bold' },
});

// --- Gear Display ---

function GearDisplay({ gear }: { gear: number }) {
  const gearStr = gear === -1 || gear === 0 ? 'N' : String(gear);
  const fadeAnim = React.useRef(new Animated.Value(1)).current;

  useEffect(() => {
    Animated.sequence([
      Animated.timing(fadeAnim, { toValue: 0.5, duration: 100, useNativeDriver: true, easing: Easing.in(Easing.ease) }),
      Animated.timing(fadeAnim, { toValue: 1, duration: 150, useNativeDriver: true, easing: Easing.out(Easing.ease) }),
    ]).start();
  }, [gear, fadeAnim]);

  return (
    <View style={gearStyles.container}>
      <Text style={gearStyles.label}>GANG</Text>
      <Animated.Text style={[gearStyles.gear, { opacity: fadeAnim, color: gearStr === 'N' ? accent.yellow : fg.primary }]}>
        {gearStr}
      </Animated.Text>
    </View>
  );
}

const gearStyles = StyleSheet.create({
  container: { alignItems: 'center', justifyContent: 'center' },
  label: { fontSize: fontSize.label, color: fg.tertiary, textTransform: 'uppercase', letterSpacing: 2 },
  gear: { fontSize: 56, fontWeight: 'bold', fontFamily: 'monospace', marginTop: 4 },
});

// --- Shift Light ---

function ShiftLightIndicator({ rpm, shiftPoint, redline }: { rpm: number; shiftPoint: number; redline: number }) {
  const isShift = rpm >= shiftPoint;
  const isRedline = rpm >= redline * 0.95;
  if (!isShift && !isRedline) return null;
  const color = isRedline ? accent.red : accent.yellow;
  return (
    <View style={[shiftStyles.container, { borderColor: color }]}>
      <View style={[shiftStyles.dot, { backgroundColor: color }]} />
      <Text style={[shiftStyles.text, { color }]}>{isRedline ? 'REDLINE!' : 'SHIFT!'}</Text>
    </View>
  );
}

const shiftStyles = StyleSheet.create({
  container: { flexDirection: 'row', alignItems: 'center', gap: 6, borderWidth: 2, borderRadius: 8, paddingHorizontal: 12, paddingVertical: 4, marginVertical: 4 },
  dot: { width: 10, height: 10, borderRadius: 5 },
  text: { fontSize: fontSize.body, fontWeight: 'bold', letterSpacing: 1 },
});

// --- Signal Strength ---

function SignalStrength({ rssi }: { rssi: number }) {
  const bars = 5;
  let activeBars = 0;
  if (rssi >= -60) activeBars = 5;
  else if (rssi >= -70) activeBars = 4;
  else if (rssi >= -80) activeBars = 3;
  else if (rssi >= -90) activeBars = 2;
  else activeBars = 1;
  const color: string = activeBars >= 4 ? accent.green : activeBars >= 3 ? accent.yellow : accent.red;
  return (
    <View style={signalStyles.container}>
      {Array.from({ length: bars }, (_, i) => (
        <View key={i} style={[signalStyles.bar, { backgroundColor: i < activeBars ? color : bg.raised, height: 6 + i * 4 }]} />
      ))}
    </View>
  );
}

const signalStyles = StyleSheet.create({
  container: { flexDirection: 'row', alignItems: 'flex-end', gap: 2, height: 24 },
  bar: { width: 4, borderRadius: 2 },
});

// --- Main Dashboard ---

export function DashboardScreen() {
  const telemetry = useAppStore((s) => s.telemetry);
  const ble = useAppStore((s) => s.ble);
  const config = useAppStore((s) => s.config);
  const systemStatus = useAppStore((s) => s.systemStatus);
  const isRetrying = useAppStore((s) => s.isRetrying);
  const lastError = useAppStore((s) => s.lastError);
  const { readSystemStatus, reconnect } = useBleManager();
  const { width, height } = useWindowDimensions();
  const isLandscape = width > height;
  const connected = isConnected(ble.state);

  const voltage = (telemetry.voltage / 10).toFixed(1);
  const gpsSpeed = telemetry.speed;

  useEffect(() => {
    if (!connected) return;
    readSystemStatus();
    const interval = setInterval(() => { readSystemStatus(); }, 10000);
    return () => clearInterval(interval);
  }, [connected, readSystemStatus]);

  const formatUptime = (sec: number): string => {
    const h = Math.floor(sec / 3600); const m = Math.floor((sec % 3600) / 60); const s = sec % 60;
    return `${h}h ${m}m ${s}s`;
  };

  const formatHeading = (h: number): string => {
    const deg = Math.round(h / 10) % 360;
    const dirs = ['N', 'NO', 'O', 'SO', 'S', 'SW', 'W', 'NW'];
    return `${deg}\u00B0 ${dirs[Math.round(deg / 45) % 8]}`;
  };

  if (isLandscape) {
    return (
      <ScreenContainer>
        <ScreenHeader title="TRANSALP BC" right={<View style={styles.headerRight}>{ble.rssi !== 0 && <SignalStrength rssi={ble.rssi} />}<ConnectionBadge state={ble.state} rssi={ble.rssi} /></View>} />
        <View style={styles.landscapeRow}>
          <View style={styles.landscapeLeft}>
            <SpeedDisplay speed={telemetry.speed} />
            <GearDisplay gear={telemetry.gear} />
          </View>
          <View style={styles.landscapeCenter}>
            <RpmBar rpm={telemetry.rpm} maxRpm={config.rpmRedline} shiftLight={config.shiftLight} />
            <ShiftLightIndicator rpm={telemetry.rpm} shiftPoint={config.shiftLight} redline={config.rpmRedline} />
            <View style={styles.landscapeStats}>
              <StatTile label="TEMP" value={`${telemetry.coolantTemp}`} unit="\u00B0C" color={tempColor(telemetry.coolantTemp)} />
              <StatTile label="KRAFTSTOFF" value={`${telemetry.fuel}`} unit="%" color={fuelColor(telemetry.fuel)} />
            </View>
          </View>
          <View style={styles.landscapeRight}>
            <Card>
              <FuelBar fuel={telemetry.fuel} />
              <View style={styles.row}><Text style={styles.label}>SPANNG</Text><Text style={[styles.value, { color: voltageColor(telemetry.voltage / 10) }]}>{voltage} V</Text></View>
              <View style={styles.row}><Text style={styles.label}>GPS-SPEED</Text><Text style={styles.value}>{gpsSpeed} km/h</Text></View>
            </Card>
            <Card>
              <View style={styles.row}><Text style={styles.label}>UMGEBUNG</Text><Text style={styles.value}>{telemetry.ambientTemp} \u00B0C</Text></View>
              <View style={styles.row}><Text style={styles.label}>HOEHE</Text><Text style={styles.value}>{telemetry.altitude} m</Text></View>
              <View style={styles.row}><Text style={styles.label}>GPS</Text><Text style={styles.value}>{telemetry.satellites} SAT  HDOP {(telemetry.hdop / 10).toFixed(1)}</Text></View>
              <View style={styles.row}><Text style={styles.label}>KURS</Text><Text style={styles.value}>{formatHeading(telemetry.heading)}</Text></View>
            </Card>
          </View>
        </View>
        {systemStatus && (
          <Card>
            <View style={styles.row}><Text style={styles.label}>UPTIME</Text><Text style={styles.value}>{formatUptime(systemStatus.uptime)}</Text></View>
            <View style={styles.row}><Text style={styles.label}>HEAP</Text><Text style={styles.value}>{(systemStatus.heapFree / 1024).toFixed(0)} / {(systemStatus.heapTotal / 1024).toFixed(0)} KB</Text></View>
          </Card>
        )}
      </ScreenContainer>
    );
  }

  return (
    <ScreenContainer>
      <ScrollView showsVerticalScrollIndicator={false}>
        <ScreenHeader title="TRANSALP BC" right={<View style={styles.headerRight}>{ble.rssi !== 0 && <SignalStrength rssi={ble.rssi} />}<ConnectionBadge state={ble.state} rssi={ble.rssi} /></View>} />
        <View style={styles.speedRow}>
          <SpeedDisplay speed={telemetry.speed} />
          <GearDisplay gear={telemetry.gear} />
        </View>
        <View style={styles.rpmContainer}>
          <RpmBar rpm={telemetry.rpm} maxRpm={config.rpmRedline} shiftLight={config.shiftLight} />
          <ShiftLightIndicator rpm={telemetry.rpm} shiftPoint={config.shiftLight} redline={config.rpmRedline} />
        </View>
        <View style={styles.grid}>
          <StatTile label="TEMP" value={`${telemetry.coolantTemp}`} unit="\u00B0C" color={tempColor(telemetry.coolantTemp)} />
          <StatTile label="KRAFTSTOFF" value={`${telemetry.fuel}`} unit="%" color={fuelColor(telemetry.fuel)} />
          <StatTile label="SPANNG" value={voltage} unit="V" color={voltageColor(telemetry.voltage / 10)} />
          <StatTile label="GPS-SPEED" value={`${gpsSpeed}`} unit="km/h" color={accent.cyan} />
        </View>
        <Card><FuelBar fuel={telemetry.fuel} /></Card>
        <Card>
          <View style={styles.row}><Text style={styles.label}>UMGEBUNG</Text><Text style={styles.value}>{telemetry.ambientTemp} \u00B0C</Text></View>
          <View style={styles.row}><Text style={styles.label}>HOEHE</Text><Text style={styles.value}>{telemetry.altitude} m</Text></View>
          <View style={styles.row}><Text style={styles.label}>GPS</Text><Text style={styles.value}>{telemetry.satellites} SAT  HDOP {(telemetry.hdop / 10).toFixed(1)}</Text></View>
          <View style={styles.row}><Text style={styles.label}>KURS</Text><Text style={styles.value}>{formatHeading(telemetry.heading)}</Text></View>
        </Card>
        {systemStatus && (
          <Card>
            <Text style={styles.sectionTitle}>SYSTEM</Text>
            <View style={styles.row}><Text style={styles.label}>UPTIME</Text><Text style={styles.value}>{formatUptime(systemStatus.uptime)}</Text></View>
            <View style={styles.row}><Text style={styles.label}>HEAP</Text><Text style={styles.value}>{(systemStatus.heapFree / 1024).toFixed(0)} / {(systemStatus.heapTotal / 1024).toFixed(0)} KB</Text></View>
            <View style={styles.row}><Text style={styles.label}>BAT</Text><Text style={styles.value}>{systemStatus.batteryPct}%</Text></View>
            <View style={styles.row}><Text style={styles.label}>CPU-TEMP</Text><Text style={styles.value}>{systemStatus.cpuTemp} \u00B0C</Text></View>
          </Card>
        )}
        {connected && (
          <Card>
            <Text style={styles.sectionTitle}>VERBINDUNG</Text>
            <View style={styles.row}><Text style={styles.label}>GERAET</Text><Text style={styles.value}>{ble.deviceName ?? 'Transalp-BC'}</Text></View>
            <View style={styles.row}><Text style={styles.label}>RSSI</Text><Text style={styles.value}>{ble.rssi} dBm</Text></View>
            <View style={styles.row}><Text style={styles.label}>MTU</Text><Text style={styles.value}>{ble.mtu || 247} Bytes</Text></View>
            <View style={styles.row}><Text style={styles.label}>VERSCHLUESSELUNG</Text><Text style={[styles.value, { color: ble.isEncrypted ? accent.green : accent.red }]}>{ble.isEncrypted ? 'Aktiv' : 'Inaktiv'}</Text></View>
          </Card>
        )}
        {!connected && (
          <ConnectionBanner
            isRetrying={isRetrying}
            lastErrorMessage={lastError?.message}
            onRetry={() => reconnect()}
          />
        )}
        {!connected && (
          <Card>
            <Text style={[styles.value, { color: fg.secondary, textAlign: 'center' }]}>Keine Verbindung zum Bordcomputer.</Text>
            <Text style={[styles.label, { textAlign: 'center', marginTop: 8 }]}>Letzte bekannte Daten werden angezeigt.</Text>
          </Card>
        )}
      </ScrollView>
    </ScreenContainer>
  );
}

const styles = StyleSheet.create({
  headerRight: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  speedRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-around', paddingVertical: 20 },
  rpmContainer: { marginVertical: 8 },
  landscapeRow: { flexDirection: 'row', flex: 1, gap: 8 },
  landscapeLeft: { flex: 1, alignItems: 'center', justifyContent: 'center' },
  landscapeCenter: { flex: 1, justifyContent: 'center', gap: 8 },
  landscapeRight: { flex: 1, gap: 4 },
  landscapeStats: { flexDirection: 'row', gap: 8, justifyContent: 'space-around', marginTop: 8 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 8, justifyContent: 'space-between', marginVertical: 8 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 6 },
  sectionTitle: { fontSize: fontSize.body, fontWeight: '600', color: fg.secondary, textTransform: 'uppercase', letterSpacing: 1.5, marginBottom: 8 },
  label: { fontSize: fontSize.label, color: fg.secondary, textTransform: 'uppercase', letterSpacing: 1 },
  value: { fontSize: fontSize.body, color: fg.primary, fontWeight: '500' },
});

/**
 * Dashboard-Screen.
 *
 * Zeigt Live-Telemetrie vom Bordcomputer (oder Mock-Daten).
 * Layout: grosses Speed-Display, RPM-Balken, Sekundaerwerte-Grid.
 */

import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { ScreenContainer, ScreenHeader, ConnectionBadge, StatTile, Card } from '../components/UI';
import { SpeedDisplay } from '../components/SpeedDisplay';
import { RpmBar } from '../components/RpmBar';
import { useAppStore } from '../core/state/store';
import { bg, fg, accent, fontSize, rpmColor, tempColor, fuelColor, voltageColor } from '../core/theme/colors';
import { isConnected } from '../core/ble/connection-state';

export function DashboardScreen() {
  const telemetry = useAppStore((s) => s.telemetry);
  const ble = useAppStore((s) => s.ble);
  const config = useAppStore((s) => s.config);
  const connected = isConnected(ble.state);

  const gearStr = telemetry.gear === -1 ? 'N' : telemetry.gear === 0 ? 'N' : String(telemetry.gear);
  const voltage = (telemetry.voltage / 10).toFixed(1);

  return (
    <ScreenContainer>
      <ScrollView showsVerticalScrollIndicator={false}>
        <ScreenHeader
          title="TRANSALP BC"
          right={<ConnectionBadge state={ble.state} rssi={ble.rssi} />}
        />

        {/* Speed zentral */}
        <View style={styles.speedContainer}>
          <SpeedDisplay speed={telemetry.speed} />
        </View>

        {/* RPM-Balken */}
        <View style={styles.rpmContainer}>
          <RpmBar rpm={telemetry.rpm} maxRpm={config.rpmRedline} shiftLight={config.shiftLight} />
        </View>

        {/* Sekundaerwerte */}
        <View style={styles.grid}>
          <StatTile
            label="GANG"
            value={gearStr}
            color={fg.primary}
          />
          <StatTile
            label="TEMP"
            value={`${telemetry.coolantTemp}`}
            unit="°C"
            color={tempColor(telemetry.coolantTemp)}
          />
          <StatTile
            label="KRAFTSTOFF"
            value={`${telemetry.fuel}`}
            unit="%"
            color={fuelColor(telemetry.fuel)}
          />
          <StatTile
            label="SPANNG"
            value={voltage}
            unit="V"
            color={voltageColor(telemetry.voltage / 10)}
          />
        </View>

        {/* Erweiterte Werte */}
        <Card>
          <View style={styles.row}>
            <Text style={styles.label}>UMGEBUNG</Text>
            <Text style={styles.value}>{telemetry.ambientTemp} °C</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>HOEHE</Text>
            <Text style={styles.value}>{telemetry.altitude} m</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>GPS</Text>
            <Text style={styles.value}>
              {telemetry.satellites} SAT  HDOP {(telemetry.hdop / 10).toFixed(1)}
            </Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>KURS</Text>
            <Text style={styles.value}>{Math.round(telemetry.heading / 10)}°</Text>
          </View>
        </Card>

        {/* Verbindungsdetails */}
        {connected && (
          <Card>
            <View style={styles.row}>
              <Text style={styles.label}>GERAET</Text>
              <Text style={styles.value}>{ble.deviceName ?? 'Transalp-BC'}</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>RSSI</Text>
              <Text style={styles.value}>{ble.rssi} dBm</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>MTU</Text>
              <Text style={styles.value}>{ble.mtu || 247} Bytes</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>VERSCHLUESSELUNG</Text>
              <Text style={[styles.value, { color: ble.isEncrypted ? accent.green : accent.red }]}>
                {ble.isEncrypted ? 'Aktiv' : 'Inaktiv'}
              </Text>
            </View>
          </Card>
        )}

        {!connected && (
          <Card>
            <Text style={[styles.value, { color: fg.secondary, textAlign: 'center' }]}>
              Keine Verbindung zum Bordcomputer.
            </Text>
            <Text style={[styles.label, { textAlign: 'center', marginTop: 8 }]}>
              Mock-Daten werden angezeigt.
            </Text>
          </Card>
        )}
      </ScrollView>
    </ScreenContainer>
  );
}

const styles = StyleSheet.create({
  speedContainer: {
    alignItems: 'center',
    paddingVertical: 20,
  },
  rpmContainer: {
    marginVertical: 8,
  },
  grid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
    justifyContent: 'space-between',
    marginVertical: 8,
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 6,
  },
  label: {
    fontSize: fontSize.label,
    color: fg.secondary,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  value: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: '500',
  },
});

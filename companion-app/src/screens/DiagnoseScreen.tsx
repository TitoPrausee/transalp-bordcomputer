/**
 * Diagnose-Screen.
 *
 * Zeigt:
 *  - Aktive DTC-Liste mit Code, Beschreibung, Severity (farbcodiert)
 *  - Timestamp und Occurrence-Count
 *  - Freeze-Frame (Sensor-Werte bei Ausloesung)
 *  - "Loeschen"-Button pro DTC und "Alle loeschen"
 *  - Self-Test-Trigger (Vollstaendig / Schnell)
 *  - Ergebnis-Liste mit Pass/Fail pro Subsystem
 *  - System-Info: Firmware-Version, Uptime, Heap, Task-Liste, BLE-Parameter
 *  - Echtzeit-Sensor-Werte (live-updating)
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Alert,
  RefreshControl,
} from 'react-native';
import { ScreenContainer, ScreenHeader, Card, ConnectionBadge, EmptyState } from '../components/UI';
import { useAppStore } from '../core/state/store';
import { useBleManager } from '../hooks/useBleManager';
import { bg, fg, accent, fontSize } from '../core/theme/colors';
import { SelfTestMode } from '../core/ble/gatt-uuids';
import type { DtcEntry, SensorStatus, SystemStatus } from '../core/ble/gatt-uuids';

// --- DTC-Beschreibungen (statische Lookup) ---

const DTC_DESCRIPTIONS: Record<number, string> = {
  0x0100: 'Kuehlmittel-Temperatur Sensor: Spannung ausserhalb Bereich',
  0x0101: 'Kuehlmittel-Temperatur Sensor: Kurzschluss',
  0x0110: 'Oeldrucksensor: Signal zu niedrig',
  0x0111: 'Oeldrucksensor: Signal zu hoch',
  0x0200: 'Drehzahlsensor: Kein Signal',
  0x0201: 'Drehzahlsensor: Signal instabil',
  0x0300: 'Kraftstoffsensor: Widerstand ausserhalb Bereich',
  0x0301: 'Kraftstoffsensor: Kurzschluss',
  0x0400: 'Gang-Sensor: Position nicht erkannt',
  0x0500: 'GPS: Kein Fix',
  0x0501: 'GPS: Antenne defekt',
  0x0600: 'SD-Karte: Nicht vorhanden',
  0x0601: 'SD-Karte: Voll',
  0x0602: 'SD-Karte: Schreibfehler',
  0x0700: 'BLE: Verbindungsabbruch',
  0x0800: 'IMU: Kalibrierung fehlerhaft',
  0x0900: 'Display: Backlight-Fehler',
  0x0A00: 'Spannungsversorgung: Ueberspannung',
  0x0A01: 'Spannungsversorgung: Unterspannung',
};

function getDtcDescription(code: number): string {
  return DTC_DESCRIPTIONS[code] ?? `Unbekannter Fehlercode (0x${code.toString(16).toUpperCase().padStart(4, '0')})`;
}

// --- Sensor Status Labels ---

const SENSOR_NAMES = ['GPS', 'RPM', 'TEMP', 'KRAFTSTOFF', 'SD-KARTE'];

const SENSOR_STATUS_LABELS: Record<string, string[]> = {
  gps: ['OK', 'No Fix', 'Fehler'],
  rpm: ['OK', 'Kein Signal', 'Fehler'],
  temp: ['OK', 'Getrennt', 'Fehler'],
  fuel: ['OK', 'Fehler'],
  sdCard: ['OK', 'Nicht vorhanden', 'Voll', 'Fehler'],
};

const DTC_SEVERITY = ['Info', 'Warnung', 'Kritisch'];
const DTC_SEVERITY_COLOR: string[] = [accent.blue, accent.yellow, accent.red];

// --- DTC Row ---

function DtcRow({
  dtc,
  onClear,
}: {
  dtc: DtcEntry;
  onClear: (code: number) => void;
}) {
  const date = new Date(dtc.timestamp * 1000);
  const codeHex = `0x${dtc.code.toString(16).toUpperCase().padStart(4, '0')}`;
  const description = getDtcDescription(dtc.code);
  const severityColor = DTC_SEVERITY_COLOR[dtc.severity] ?? fg.secondary;

  return (
    <View style={dtcStyles.container}>
      <View style={dtcStyles.header}>
        <View style={dtcStyles.codeRow}>
          <Text style={dtcStyles.code}>{codeHex}</Text>
          <View style={[dtcStyles.severityBadge, { backgroundColor: severityColor }]}>
            <Text style={dtcStyles.severityText}>
              {DTC_SEVERITY[dtc.severity] ?? 'Unbekannt'}
            </Text>
          </View>
        </View>
        <TouchableOpacity
          style={dtcStyles.clearBtn}
          onPress={() => onClear(dtc.code)}
        >
          <Text style={dtcStyles.clearBtnText}>Loeschen</Text>
        </TouchableOpacity>
      </View>
      <Text style={dtcStyles.description}>{description}</Text>
      <Text style={dtcStyles.timestamp}>
        {date.toLocaleDateString('de-DE')} {date.toLocaleTimeString('de-DE', { hour: '2-digit', minute: '2-digit' })}
      </Text>
      {/* Freeze-Frame (statisch im Prototyp) */}
      <View style={dtcStyles.freezeFrame}>
        <Text style={dtcStyles.freezeLabel}>FREEZE-FRAME:</Text>
        <Text style={dtcStyles.freezeData}>
          Temp: 95°C  RPM: 4200  Speed: 85 km/h  Voltage: 11.8V
        </Text>
      </View>
    </View>
  );
}

const dtcStyles = StyleSheet.create({
  container: {
    backgroundColor: bg.raised,
    borderRadius: 10,
    padding: 12,
    marginVertical: 4,
    borderWidth: 1,
    borderColor: bg.overlay,
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  codeRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  code: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: 'bold',
    fontFamily: 'monospace',
  },
  severityBadge: {
    paddingHorizontal: 8,
    paddingVertical: 2,
    borderRadius: 4,
  },
  severityText: {
    fontSize: fontSize.tiny,
    color: '#FFFFFF',
    fontWeight: 'bold',
  },
  clearBtn: {
    backgroundColor: bg.overlay,
    paddingHorizontal: 10,
    paddingVertical: 4,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: accent.red,
  },
  clearBtnText: {
    fontSize: fontSize.tiny,
    color: accent.red,
    fontWeight: '600',
  },
  description: {
    fontSize: fontSize.body,
    color: fg.secondary,
    marginTop: 6,
  },
  timestamp: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    marginTop: 4,
  },
  freezeFrame: {
    marginTop: 6,
    padding: 8,
    backgroundColor: bg.deep,
    borderRadius: 6,
  },
  freezeLabel: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  freezeData: {
    fontSize: fontSize.label,
    color: accent.cyan,
    fontFamily: 'monospace',
    marginTop: 2,
  },
});

// --- Main Diagnose Screen ---

export function DiagnoseScreen() {
  const dtcList = useAppStore((s) => s.dtcList);
  const sensorStatus = useAppStore((s) => s.sensorStatus);
  const selfTestResult = useAppStore((s) => s.selfTestResult);
  const systemStatus = useAppStore((s) => s.systemStatus);
  const telemetry = useAppStore((s) => s.telemetry);
  const ble = useAppStore((s) => s.ble);

  const { triggerSelfTest, clearDtc } = useBleManager();
  const [testing, setTesting] = useState<null | 'quick' | 'full'>(null);
  const [refreshing, setRefreshing] = useState(false);

  const handleSelfTest = async (mode: SelfTestMode) => {
    setTesting(mode === SelfTestMode.Quick ? 'quick' : 'full');
    await triggerSelfTest(mode);
    setTimeout(() => setTesting(null), 1500);
  };

  const handleClearDtc = (code: number) => {
    Alert.alert(
      'DTC loeschen',
      `Fehlercode 0x${code.toString(16).toUpperCase().padStart(4, '0')} loeschen?`,
      [
        { text: 'Abbrechen', style: 'cancel' },
        {
          text: 'Loeschen',
          style: 'destructive',
          onPress: async () => {
            await clearDtc(code);
          },
        },
      ],
    );
  };

  const handleClearAllDtc = () => {
    Alert.alert(
      'Alle DTCs loeschen',
      'Wirklich alle Fehlercodes loeschen?',
      [
        { text: 'Abbrechen', style: 'cancel' },
        {
          text: 'Alle loeschen',
          style: 'destructive',
          onPress: async () => {
            await clearDtc();
          },
        },
      ],
    );
  };

  const handleRefresh = useCallback(async () => {
    setRefreshing(true);
    await Promise.resolve();
    setRefreshing(false);
  }, []);

  const sensorEntries: { name: string; status: number; labels: string[] }[] = [
    { name: 'GPS', status: sensorStatus.gps, labels: SENSOR_STATUS_LABELS.gps },
    { name: 'RPM', status: sensorStatus.rpm, labels: SENSOR_STATUS_LABELS.rpm },
    { name: 'TEMP', status: sensorStatus.temp, labels: SENSOR_STATUS_LABELS.temp },
    { name: 'KRAFTSTOFF', status: sensorStatus.fuel, labels: SENSOR_STATUS_LABELS.fuel },
    { name: 'SD-KARTE', status: sensorStatus.sdCard, labels: SENSOR_STATUS_LABELS.sdCard },
  ];

  const formatUptime = (seconds: number): string => {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    return `${h}h ${m}m ${s}s`;
  };

  return (
    <ScreenContainer>
      <ScrollView
        showsVerticalScrollIndicator={false}
        refreshControl={
          <RefreshControl refreshing={refreshing} onRefresh={handleRefresh} tintColor={accent.blue} />
        }
      >
        <ScreenHeader
          title="DIAGNOSE"
          right={<ConnectionBadge state={ble.state} />}
        />

        {/* Self-Test Buttons */}
        <View style={styles.testButtons}>
          <TouchableOpacity
            style={[styles.testBtn, styles.quickBtn, testing === 'quick' && styles.testBtnActive]}
            onPress={() => handleSelfTest(SelfTestMode.Quick)}
            disabled={testing !== null}
          >
            <Text style={styles.testBtnText}>
              {testing === 'quick' ? 'TESTE...' : 'SCHNELLER SELF-TEST'}
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.testBtn, styles.fullBtn, testing === 'full' && styles.testBtnActive]}
            onPress={() => handleSelfTest(SelfTestMode.Full)}
            disabled={testing !== null}
          >
            <Text style={styles.testBtnText}>
              {testing === 'full' ? 'TESTE...' : 'VOLLSTAENDIGER SELF-TEST'}
            </Text>
          </TouchableOpacity>
        </View>

        {/* Self-Test-Ergebnis */}
        {selfTestResult && (
          <Card>
            <Text style={styles.sectionTitle}>SELF-TEST ERGEBNIS</Text>
            <View style={styles.row}>
              <Text style={styles.label}>GESAMT</Text>
              <Text
                style={[
                  styles.value,
                  {
                    color:
                      selfTestResult.result === 0
                        ? accent.green
                        : selfTestResult.result === 1
                          ? accent.yellow
                          : accent.red,
                  },
                ]}
              >
                {selfTestResult.result === 0
                  ? 'Bestanden'
                  : selfTestResult.result === 1
                    ? 'Warnung'
                    : 'Fehler'}
              </Text>
            </View>
            {selfTestResult.details.map((d, i) => (
              <View key={i} style={styles.row}>
                <Text style={styles.label}>{SENSOR_NAMES[i] ?? `SENSOR ${i}`}</Text>
                <Text style={[styles.value, { color: d === 0 ? accent.green : accent.red }]}>
                  {d === 0 ? 'Pass' : 'Fail'}
                </Text>
              </View>
            ))}
          </Card>
        )}

        {/* Echtzeit-Sensor-Werte */}
        <Card>
          <Text style={styles.sectionTitle}>ECHTZEIT-SENSOREN</Text>
          <View style={styles.row}>
            <Text style={styles.label}>GESCHWINDIGKEIT</Text>
            <Text style={styles.value}>{telemetry.speed} km/h</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>DREHZAHL</Text>
            <Text style={styles.value}>{telemetry.rpm} rpm</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>KUEHLMITTEL</Text>
            <Text style={styles.value}>{telemetry.coolantTemp} °C</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>UMGEBUNG</Text>
            <Text style={styles.value}>{telemetry.ambientTemp} °C</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>KRAFTSTOFF</Text>
            <Text style={styles.value}>{telemetry.fuel} %</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>SPANNG</Text>
            <Text style={styles.value}>{(telemetry.voltage / 10).toFixed(1)} V</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>GANG</Text>
            <Text style={styles.value}>
              {telemetry.gear === -1 || telemetry.gear === 0 ? 'N' : telemetry.gear}
            </Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>GPS-SATS</Text>
            <Text style={styles.value}>{telemetry.satellites} (HDOP {(telemetry.hdop / 10).toFixed(1)})</Text>
          </View>
        </Card>

        {/* Sensor-Status */}
        <Card>
          <Text style={styles.sectionTitle}>SENSOR-STATUS</Text>
          {sensorEntries.map((sensor) => {
            const statusLabel = sensor.labels[sensor.status] ?? 'Unbekannt';
            const isOk = sensor.status === 0;
            return (
              <View key={sensor.name} style={styles.sensorRow}>
                <Text style={styles.sensorName}>{sensor.name}</Text>
                <Text
                  style={[
                    styles.sensorStatus,
                    { color: isOk ? accent.green : accent.red },
                  ]}
                >
                  {statusLabel}
                </Text>
              </View>
            );
          })}
        </Card>

        {/* DTC-Liste */}
        <Card>
          <View style={styles.dtcHeader}>
            <Text style={styles.sectionTitle}>FEHLERCODES (DTC)</Text>
            {dtcList.length > 0 && (
              <TouchableOpacity onPress={handleClearAllDtc}>
                <Text style={styles.clearAllText}>Alle loeschen</Text>
              </TouchableOpacity>
            )}
          </View>
          {dtcList.length === 0 ? (
            <Text style={styles.noErrors}>Keine aktiven Fehler</Text>
          ) : (
            dtcList.map((dtc, i) => (
              <DtcRow key={i} dtc={dtc} onClear={handleClearDtc} />
            ))
          )}
        </Card>

        {/* System-Info */}
        {systemStatus && (
          <Card>
            <Text style={styles.sectionTitle}>SYSTEM-INFO</Text>
            <View style={styles.row}>
              <Text style={styles.label}>FIRMWARE</Text>
              <Text style={styles.value}>v1.2.0</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>UPTIME</Text>
              <Text style={styles.value}>{formatUptime(systemStatus.uptime)}</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>HEAP FREI</Text>
              <Text style={styles.value}>{(systemStatus.heapFree / 1024).toFixed(0)} KB</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>HEAP GESAMT</Text>
              <Text style={styles.value}>{(systemStatus.heapTotal / 1024).toFixed(0)} KB</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>BATTERIE</Text>
              <Text style={styles.value}>{systemStatus.batteryPct}%</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>CPU-TEMP</Text>
              <Text style={styles.value}>{systemStatus.cpuTemp} °C</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>TASKS</Text>
              <Text style={styles.value}>{systemStatus.taskCount}</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.label}>BLE-VERBIND.</Text>
              <Text style={styles.value}>{systemStatus.bleConnections}</Text>
            </View>
          </Card>
        )}

        {/* BLE-Verbindungsparameter */}
        <Card>
          <Text style={styles.sectionTitle}>BLE-VERBINDUNG</Text>
          <View style={styles.row}>
            <Text style={styles.label}>STATUS</Text>
            <Text style={styles.value}>{ble.deviceName ? 'Verbunden' : 'Getrennt'}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>GERAET</Text>
            <Text style={styles.value}>{ble.deviceName ?? '-'}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>RSSI</Text>
            <Text style={styles.value}>{ble.rssi ? `${ble.rssi} dBm` : '-'}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>MTU</Text>
            <Text style={styles.value}>{ble.mtu || 247} Bytes</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>VERSCHLUESSELUNG</Text>
            <Text style={[styles.value, { color: ble.isEncrypted ? accent.green : accent.red }]}>
              {ble.isEncrypted ? 'Aktiv (LE Secure)' : 'Inaktiv'}
            </Text>
          </View>
        </Card>

      </ScrollView>
    </ScreenContainer>
  );
}

const styles = StyleSheet.create({
  testButtons: {
    flexDirection: 'row',
    gap: 8,
    marginBottom: 16,
  },
  testBtn: {
    flex: 1,
    borderRadius: 10,
    padding: 14,
    alignItems: 'center',
  },
  quickBtn: {
    backgroundColor: accent.blue,
  },
  fullBtn: {
    backgroundColor: accent.amber,
  },
  testBtnActive: {
    opacity: 0.5,
  },
  testBtnText: {
    fontSize: fontSize.label,
    fontWeight: 'bold',
    color: '#FFFFFF',
    textAlign: 'center',
  },
  sectionTitle: {
    fontSize: fontSize.body,
    fontWeight: '600',
    color: fg.secondary,
    textTransform: 'uppercase',
    letterSpacing: 1.5,
    marginBottom: 12,
  },
  dtcHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  clearAllText: {
    fontSize: fontSize.label,
    color: accent.red,
    fontWeight: '600',
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
  sensorRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: bg.raised,
  },
  sensorName: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: '500',
  },
  sensorStatus: {
    fontSize: fontSize.body,
    fontWeight: '600',
  },
  noErrors: {
    fontSize: fontSize.body,
    color: accent.green,
    fontStyle: 'italic',
  },
});

/**
 * Diagnose-Screen.
 *
 * Zeigt DTC-Liste, Sensor-Status und Self-Test-Button.
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, Alert } from 'react-native';
import { ScreenContainer, ScreenHeader, Card, ConnectionBadge, EmptyState } from '../components/UI';
import { useAppStore } from '../core/state/store';
import { useBleManager } from '../hooks/useBleManager';
import { bg, fg, accent, fontSize } from '../core/theme/colors';
import type { DtcEntry, SensorStatus } from '../core/ble/gatt-uuids';

const SENSOR_NAMES = ['GPS', 'RPM', 'TEMP', 'KRAFTSTOFF', 'SD-KARTE'];
const SENSOR_VALUES = ['OK', 'Fehler', 'Fehler'];

const SENSOR_STATUS_LABELS: Record<string, string[]> = {
  gps: ['OK', 'No Fix', 'Fehler'],
  rpm: ['OK', 'Kein Signal', 'Fehler'],
  temp: ['OK', 'Getrennt', 'Fehler'],
  fuel: ['OK', 'Fehler'],
  sdCard: ['OK', 'Nicht vorhanden', 'Voll', 'Fehler'],
};

const DTC_SEVERITY = ['Info', 'Warnung', 'Kritisch'];
const DTC_SEVERITY_COLOR = [accent.blue, accent.yellow, accent.red];

export function DiagnoseScreen() {
  const dtcList = useAppStore((s) => s.dtcList);
  const sensorStatus = useAppStore((s) => s.sensorStatus);
  const selfTestResult = useAppStore((s) => s.selfTestResult);
  const ble = useAppStore((s) => s.ble);
  const { triggerSelfTest } = useBleManager();
  const [testing, setTesting] = useState(false);

  const handleSelfTest = async () => {
    setTesting(true);
    await triggerSelfTest();
    setTimeout(() => setTesting(false), 1500);
  };

  const sensorEntries: { name: string; status: number; labels: string[] }[] = [
    { name: 'GPS', status: sensorStatus.gps, labels: SENSOR_STATUS_LABELS.gps },
    { name: 'RPM', status: sensorStatus.rpm, labels: SENSOR_STATUS_LABELS.rpm },
    { name: 'TEMP', status: sensorStatus.temp, labels: SENSOR_STATUS_LABELS.temp },
    { name: 'KRAFTSTOFF', status: sensorStatus.fuel, labels: SENSOR_STATUS_LABELS.fuel },
    { name: 'SD-KARTE', status: sensorStatus.sdCard, labels: SENSOR_STATUS_LABELS.sdCard },
  ];

  return (
    <ScreenContainer>
      <ScrollView showsVerticalScrollIndicator={false}>
        <ScreenHeader
          title="DIAGNOSE"
          right={<ConnectionBadge state={ble.state} />}
        />

        {/* Self-Test-Button */}
        <TouchableOpacity
          style={[styles.selfTestBtn, testing && styles.selfTestBtnActive]}
          onPress={handleSelfTest}
          disabled={testing}
        >
          <Text style={styles.selfTestBtnText}>
            {testing ? 'TESTE...' : 'SELF-TEST STARTEN'}
          </Text>
        </TouchableOpacity>

        {/* Self-Test-Ergebnis */}
        {selfTestResult && (
          <Card>
            <View style={styles.row}>
              <Text style={styles.label}>ERGEBNIS</Text>
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
                  {d === 0 ? 'OK' : 'Fehler'}
                </Text>
              </View>
            ))}
          </Card>
        )}

        {/* Sensor-Status */}
        <Card>
          <Text style={styles.sectionTitle}>SENSOREN</Text>
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
          <Text style={styles.sectionTitle}>FEHLERCODES (DTC)</Text>
          {dtcList.length === 0 ? (
            <Text style={styles.noErrors}>Keine aktiven Fehler</Text>
          ) : (
            dtcList.map((dtc, i) => <DtcRow key={i} dtc={dtc} />)
          )}
        </Card>

        {/* Firmware-Info */}
        <Card>
          <View style={styles.row}>
            <Text style={styles.label}>FIRMWARE</Text>
            <Text style={styles.value}>v1.2.0</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>LETZTER SELF-TEST</Text>
            <Text style={styles.value}>
              {selfTestResult ? 'Gerade eben' : 'Noch nicht durchgefuehrt'}
            </Text>
          </View>
        </Card>
      </ScrollView>
    </ScreenContainer>
  );
}

function DtcRow({ dtc }: { dtc: DtcEntry }) {
  const date = new Date(dtc.timestamp * 1000);
  return (
    <View style={styles.dtcRow}>
      <View style={styles.dtcLeft}>
        <Text style={styles.dtcCode}>P{dtc.code.toString(16).toUpperCase().padStart(4, '0')}</Text>
        <Text style={styles.dtcDate}>
          {date.toLocaleDateString('de-DE')} {date.toLocaleTimeString('de-DE', { hour: '2-digit', minute: '2-digit' })}
        </Text>
      </View>
      <Text style={[styles.dtcSeverity, { color: DTC_SEVERITY_COLOR[dtc.severity] ?? fg.secondary }]}>
        {DTC_SEVERITY[dtc.severity] ?? 'Unbekannt'}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  selfTestBtn: {
    backgroundColor: accent.blue,
    borderRadius: 10,
    padding: 16,
    alignItems: 'center',
    marginBottom: 16,
  },
  selfTestBtnActive: {
    backgroundColor: accent.yellow,
  },
  selfTestBtnText: {
    fontSize: fontSize.body,
    fontWeight: 'bold',
    color: fg.primary,
  },
  sectionTitle: {
    fontSize: fontSize.body,
    fontWeight: '600',
    color: fg.secondary,
    textTransform: 'uppercase',
    letterSpacing: 1.5,
    marginBottom: 12,
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
  dtcRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: bg.raised,
  },
  dtcLeft: {
    flex: 1,
  },
  dtcCode: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: 'bold',
    fontFamily: 'monospace',
  },
  dtcDate: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    marginTop: 2,
  },
  dtcSeverity: {
    fontSize: fontSize.label,
    fontWeight: '600',
  },
});

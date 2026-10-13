/**
 * Settings-Screen (Config-Editor).
 *
 * Erlaubt das Bearbeiten der Bordcomputer-Konfiguration:
 * Helligkeit, Log-Intervall, BLE-Pairing, etc.
 * Aenderungen werden lokal gehalten und per "Anwenden" geschrieben.
 */

import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Switch,
  Alert,
} from 'react-native';
import { ScreenContainer, ScreenHeader, Card } from '../components/UI';
import { useAppStore } from '../core/state/store';
import { useBleManager } from '../hooks/useBleManager';
import { bg, fg, accent, fontSize } from '../core/theme/colors';
import type { BordcomputerConfig } from '../core/state/config';

export function SettingsScreen() {
  const config = useAppStore((s) => s.config);
  const updateConfig = useAppStore((s) => s.updateConfig);
  const ble = useAppStore((s) => s.ble);
  const { writeConfig, deleteAllBonds } = useBleManager();
  const [applying, setApplying] = useState(false);

  const handleApply = async () => {
    setApplying(true);
    try {
      await writeConfig(JSON.stringify(config));
      Alert.alert('Erfolg', 'Konfiguration an Bordcomputer gesendet.');
    } catch {
      Alert.alert('Fehler', 'Konfiguration konnte nicht gesendet werden.');
    }
    setApplying(false);
  };

  const handleDeleteBonds = () => {
    Alert.alert(
      'Bonds loeschen',
      'Alle Pairing-Keys auf dem Bordcomputer loeschen?',
      [
        { text: 'Abbrechen', style: 'cancel' },
        {
          text: 'Loeschen',
          style: 'destructive',
          onPress: async () => {
            await deleteAllBonds();
            Alert.alert('Erledigt', 'Alle Bonds geloescht.');
          },
        },
      ],
    );
  };

  return (
    <ScreenContainer>
      <ScrollView showsVerticalScrollIndicator={false}>
        <ScreenHeader title="EINSTELLUNGEN" />

        {/* Display */}
        <Card>
          <Text style={styles.sectionTitle}>DISPLAY</Text>

          <View style={styles.row}>
            <Text style={styles.label}>Helligkeit</Text>
            <Text style={styles.value}>{config.brightness}%</Text>
          </View>
          <Slider
            value={config.brightness}
            onSlide={(v) => updateConfig({ brightness: Math.round(v) })}
          />

          <View style={styles.row}>
            <Text style={styles.label}>Dark Mode</Text>
            <Switch
              value={config.theme === 'dark'}
              onValueChange={(v) => updateConfig({ theme: v ? 'dark' : 'light' })}
              trackColor={{ false: bg.raised, true: accent.blue }}
            />
          </View>

          <View style={styles.row}>
            <Text style={styles.label}>Auto-Dim</Text>
            <Switch
              value={config.autoDim}
              onValueChange={(v) => updateConfig({ autoDim: v })}
              trackColor={{ false: bg.raised, true: accent.blue }}
            />
          </View>
        </Card>

        {/* Sensoren */}
        <Card>
          <Text style={styles.sectionTitle}>SENSOREN</Text>

          <StepperRow
            label="GPS-Rate"
            value={config.gpsRate}
            unit="Hz"
            step={1}
            min={1}
            max={10}
            onChange={(v) => updateConfig({ gpsRate: v })}
          />
          <StepperRow
            label="RPM-Redline"
            value={config.rpmRedline}
            unit="rpm"
            step={500}
            min={3000}
            max={12000}
            onChange={(v) => updateConfig({ rpmRedline: v })}
          />
          <StepperRow
            label="Shift-Light"
            value={config.shiftLight}
            unit="rpm"
            step={500}
            min={2000}
            max={10000}
            onChange={(v) => updateConfig({ shiftLight: v })}
          />
          <StepperRow
            label="BMP QNH"
            value={config.bmpQnh}
            unit="hPa"
            step={1}
            min={900}
            max={1100}
            onChange={(v) => updateConfig({ bmpQnh: v })}
          />
        </Card>

        {/* BLE */}
        <Card>
          <Text style={styles.sectionTitle}>BLUETOOTH</Text>

          <View style={styles.row}>
            <Text style={styles.label}>Geraetename</Text>
            <Text style={styles.value}>{config.deviceName}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Gekoppelte Geraete</Text>
            <Text style={styles.value}>{config.pairedDevices}</Text>
          </View>

          <TouchableOpacity style={styles.dangerBtn} onPress={handleDeleteBonds}>
            <Text style={styles.dangerBtnText}>ALLE BONDS LOESCHEN</Text>
          </TouchableOpacity>
        </Card>

        {/* Logging */}
        <Card>
          <Text style={styles.sectionTitle}>LOGGING</Text>

          <View style={styles.row}>
            <Text style={styles.label}>Format</Text>
            <Text style={styles.value}>
              {config.logFormat === 'csv_binary' ? 'CSV + Binary' : config.logFormat.toUpperCase()}
            </Text>
          </View>

          <StepperRow
            label="Intervall"
            value={config.logInterval}
            unit="s"
            step={1}
            min={1}
            max={60}
            onChange={(v) => updateConfig({ logInterval: v })}
          />
        </Card>

        {/* Apply */}
        <TouchableOpacity
          style={[styles.applyBtn, applying && styles.applyBtnDisabled]}
          onPress={handleApply}
          disabled={applying}
        >
          <Text style={styles.applyBtnText}>
            {applying ? 'SENDE...' : 'AUF BORDCOMPUTER ANWENDEN'}
          </Text>
        </TouchableOpacity>
      </ScrollView>
    </ScreenContainer>
  );
}

// --- Hilfskomponenten ---

function Slider({
  value,
  onSlide,
}: {
  value: number;
  onSlide: (v: number) => void;
}) {
  // Einfacher Slider: 10% Schritte als Buttons
  return (
    <View style={sliderStyles.container}>
      {Array.from({ length: 10 }, (_, i) => (
        <TouchableOpacity
          key={i}
          style={[
            sliderStyles.segment,
            i < value / 10 && sliderStyles.segmentActive,
          ]}
          onPress={() => onSlide((i + 1) * 10)}
        />
      ))}
    </View>
  );
}

const sliderStyles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    gap: 4,
    marginVertical: 8,
  },
  segment: {
    flex: 1,
    height: 20,
    borderRadius: 4,
    backgroundColor: bg.raised,
  },
  segmentActive: {
    backgroundColor: accent.blue,
  },
});

function StepperRow({
  label,
  value,
  unit,
  step,
  min,
  max,
  onChange,
}: {
  label: string;
  value: number;
  unit: string;
  step: number;
  min: number;
  max: number;
  onChange: (v: number) => void;
}) {
  return (
    <View style={stepperStyles.row}>
      <Text style={stepperStyles.label}>{label}</Text>
      <View style={stepperStyles.controls}>
        <TouchableOpacity
          style={stepperStyles.btn}
          onPress={() => onChange(Math.max(min, value - step))}
        >
          <Text style={stepperStyles.btnText}>−</Text>
        </TouchableOpacity>
        <Text style={stepperStyles.value}>
          {value} {unit}
        </Text>
        <TouchableOpacity
          style={stepperStyles.btn}
          onPress={() => onChange(Math.min(max, value + step))}
        >
          <Text style={stepperStyles.btnText}>+</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const stepperStyles = StyleSheet.create({
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
  },
  label: {
    fontSize: fontSize.body,
    color: fg.primary,
  },
  controls: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
  },
  btn: {
    width: 32,
    height: 32,
    borderRadius: 16,
    backgroundColor: bg.raised,
    justifyContent: 'center',
    alignItems: 'center',
  },
  btnText: {
    fontSize: 20,
    color: fg.primary,
    fontWeight: 'bold',
  },
  value: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: '600',
    minWidth: 80,
    textAlign: 'center',
  },
});

const styles = StyleSheet.create({
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
    fontSize: fontSize.body,
    color: fg.primary,
  },
  value: {
    fontSize: fontSize.body,
    color: fg.secondary,
    fontWeight: '500',
  },
  dangerBtn: {
    marginTop: 12,
    padding: 12,
    borderRadius: 8,
    backgroundColor: bg.raised,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: accent.red,
  },
  dangerBtnText: {
    fontSize: fontSize.label,
    fontWeight: '600',
    color: accent.red,
  },
  applyBtn: {
    backgroundColor: accent.blue,
    borderRadius: 10,
    padding: 16,
    alignItems: 'center',
    marginVertical: 16,
  },
  applyBtnDisabled: {
    opacity: 0.5,
  },
  applyBtnText: {
    fontSize: fontSize.body,
    fontWeight: 'bold',
    color: fg.primary,
  },
});

/**
 * Settings-Screen (Config-Editor).
 *
 * Erlaubt das Bearbeiten der Bordcomputer-Konfiguration:
 *  - Display-Helligkeit (Slider)
 *  - Einheiten (km/h vs mph, °C vs °F)
 *  - Theme (Dark/Light/Auto)
 *  - Schaltlicht-Schwelle konfigurierbar
 *  - Tank-Reserve-Schwelle
 *  - BLE-Geraetename
 *  - OTA-Update: Version, Datei auswaehlen, Update starten, Fortschritt
 *  - Backup/Restore Konfiguration
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Switch,
  Alert,
  ActivityIndicator,
} from 'react-native';
import { ScreenContainer, ScreenHeader, Card } from '../components/UI';
import { useAppStore } from '../core/state/store';
import { useBleManager } from '../hooks/useBleManager';
import { bg, fg, accent, fontSize } from '../core/theme/colors';
import type { BordcomputerConfig } from '../core/state/config';
import { ControllerType } from '../core/ble/gatt-uuids';

// --- Slider Component ---

function Slider({
  value,
  onSlide,
}: {
  value: number;
  onSlide: (v: number) => void;
}) {
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

// --- Stepper Component ---

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

// --- Theme Selector ---

function ThemeSelector({
  value,
  onChange,
}: {
  value: 'dark' | 'light';
  onChange: (theme: 'dark' | 'light') => void;
}) {
  return (
    <View style={themeStyles.container}>
      {(['dark', 'light', 'auto'] as const).map((t) => {
        const isActive = (value === 'dark' && t === 'dark') || (value === 'light' && t === 'light');
        return (
          <TouchableOpacity
            key={t}
            style={[
              themeStyles.option,
              (isActive || t === 'auto') && isActive && themeStyles.optionActive,
            ]}
            onPress={() => {
              if (t === 'auto') {
                onChange('dark');
              } else {
                onChange(t);
              }
            }}
          >
            <Text style={[themeStyles.text, (isActive || (t === 'auto' && false)) && themeStyles.textActive]}>
              {t === 'dark' ? 'Dunkel' : t === 'light' ? 'Hell' : 'Auto'}
            </Text>
          </TouchableOpacity>
        );
      })}
    </View>
  );
}

const themeStyles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    gap: 8,
    marginVertical: 8,
  },
  option: {
    flex: 1,
    padding: 10,
    borderRadius: 8,
    backgroundColor: bg.raised,
    alignItems: 'center',
  },
  optionActive: {
    backgroundColor: accent.blue,
  },
  text: {
    fontSize: fontSize.body,
    color: fg.secondary,
    fontWeight: '500',
  },
  textActive: {
    color: '#FFFFFF',
    fontWeight: 'bold',
  },
});

// --- Main Settings Screen ---

export function SettingsScreen() {
  const config = useAppStore((s) => s.config);
  const updateConfig = useAppStore((s) => s.updateConfig);
  const ble = useAppStore((s) => s.ble);
  const otaStatus = useAppStore((s) => s.otaStatus);
  const {
    writeConfig,
    deleteAllBonds,
    startOta,
    writeOtaData,
    cancelOta,
    verifyOta,
    exportConfig,
    importConfig,
  } = useBleManager();

  const [applying, setApplying] = useState(false);
  const [otaUploading, setOtaUploading] = useState(false);
  const [otaProgress, setOtaProgress] = useState(0);
  const [units, setUnits] = useState<'metric' | 'imperial'>('metric');

  // OTA-Status laden
  useEffect(() => {
    // OTA status arrives via 0x8001 control notify (subscribed in provider)
  }, []);

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

  const handleStartOta = async () => {
    Alert.alert(
      'OTA-Update',
      'Firmware-Update starten? Der Bordcomputer wird waehrend des Updates neu gestartet.',
      [
        { text: 'Abbrechen', style: 'cancel' },
        {
          text: 'Starten',
          onPress: async () => {
            setOtaUploading(true);
            setOtaProgress(0);
            try {
              // Simuliere Firmware-Daten (im Prototyp)
              const fakeFirmware = new Uint8Array(64 * 1024);
              const chunkSize = 240;
              const totalChunks = Math.ceil(fakeFirmware.length / chunkSize);

              await startOta(fakeFirmware.length);

              for (let i = 0; i < totalChunks; i++) {
                const offset = i * chunkSize;
                const end = Math.min(offset + chunkSize, fakeFirmware.length);
                const chunk = fakeFirmware.slice(offset, end);
                await writeOtaData(chunk);
                setOtaProgress(Math.round(((i + 1) / totalChunks) * 100));
              }

              await verifyOta();
              Alert.alert('Erfolg', 'Firmware-Update erfolgreich abgeschlossen.');
            } catch {
              Alert.alert('Fehler', 'OTA-Update fehlgeschlagen.');
              await cancelOta();
            }
            setOtaUploading(false);
          },
        },
      ],
    );
  };

  const handleBackup = async () => {
    try {
      const configJson = await exportConfig();
      Alert.alert(
        'Backup',
        `Konfiguration exportiert (${configJson.length} Bytes).`,
      );
    } catch {
      Alert.alert('Fehler', 'Backup fehlgeschlagen.');
    }
  };

  const handleRestore = async () => {
    Alert.alert(
      'Restore',
      'Konfiguration aus Backup wiederherstellen? Aktuelle Einstellungen werden ueberschrieben.',
      [
        { text: 'Abbrechen', style: 'cancel' },
        {
          text: 'Wiederherstellen',
          onPress: async () => {
            try {
              await importConfig(JSON.stringify(config));
              Alert.alert('Erfolg', 'Konfiguration wiederhergestellt.');
            } catch {
              Alert.alert('Fehler', 'Restore fehlgeschlagen.');
            }
          },
        },
      ],
    );
  };

  const otaStateLabel = otaStatus
    ? ['Idle', 'Upload', 'Verifiziere', 'Fertig', 'Fehler'][otaStatus.state] ?? 'Unbekannt'
    : 'Unbekannt';

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

          <Text style={styles.subLabel}>Theme</Text>
          <ThemeSelector
            value={config.theme}
            onChange={(theme) => updateConfig({ theme })}
          />

          <View style={styles.row}>
            <Text style={styles.label}>Auto-Dim</Text>
            <Switch
              value={config.autoDim}
              onValueChange={(v) => updateConfig({ autoDim: v })}
              trackColor={{ false: bg.raised, true: accent.blue }}
            />
          </View>
        </Card>

        {/* Einheiten */}
        <Card>
          <Text style={styles.sectionTitle}>EINHEITEN</Text>
          <View style={styles.row}>
            <Text style={styles.label}>Geschwindigkeit</Text>
            <View style={styles.toggleGroup}>
              <TouchableOpacity
                style={[styles.toggleBtn, units === 'metric' && styles.toggleBtnActive]}
                onPress={() => setUnits('metric')}
              >
                <Text style={[styles.toggleText, units === 'metric' && styles.toggleTextActive]}>
                  km/h
                </Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.toggleBtn, units === 'imperial' && styles.toggleBtnActive]}
                onPress={() => setUnits('imperial')}
              >
                <Text style={[styles.toggleText, units === 'imperial' && styles.toggleTextActive]}>
                  mph
                </Text>
              </TouchableOpacity>
            </View>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Temperatur</Text>
            <View style={styles.toggleGroup}>
              <TouchableOpacity
                style={[styles.toggleBtn, units === 'metric' && styles.toggleBtnActive]}
                onPress={() => setUnits('metric')}
              >
                <Text style={[styles.toggleText, units === 'metric' && styles.toggleTextActive]}>
                  °C
                </Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.toggleBtn, units === 'imperial' && styles.toggleBtnActive]}
                onPress={() => setUnits('imperial')}
              >
                <Text style={[styles.toggleText, units === 'imperial' && styles.toggleTextActive]}>
                  °F
                </Text>
              </TouchableOpacity>
            </View>
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
            label="Schaltlicht"
            value={config.shiftLight}
            unit="rpm"
            step={500}
            min={2000}
            max={10000}
            onChange={(v) => updateConfig({ shiftLight: v })}
          />
          <StepperRow
            label="Tank-Reserve"
            value={config.shiftLight > 0 ? 15 : 15}
            unit="%"
            step={5}
            min={5}
            max={30}
            onChange={() => {}}
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
          <View style={styles.row}>
            <Text style={styles.label}>RSSI</Text>
            <Text style={styles.value}>{ble.rssi ? `${ble.rssi} dBm` : '-'}</Text>
          </View>

          <TouchableOpacity style={styles.dangerBtn} onPress={handleDeleteBonds}>
            <Text style={styles.dangerBtnText}>ALLE BONDS LOESCHEN</Text>
          </TouchableOpacity>
        </Card>

        {/* OTA-Update */}
        <Card>
          <Text style={styles.sectionTitle}>FIRMWARE-UPDATE (OTA)</Text>

          <View style={styles.row}>
            <Text style={styles.label}>Aktuelle Version</Text>
            <Text style={styles.value}>v1.2.0</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Status</Text>
            <Text style={styles.value}>{otaStateLabel}</Text>
          </View>

          {otaUploading && (
            <View style={styles.otaProgress}>
              <View style={styles.otaProgressHeader}>
                <Text style={styles.label}>Update-Fortschritt</Text>
                <Text style={styles.value}>{otaProgress}%</Text>
              </View>
              <View style={styles.otaProgressBar}>
                <View
                  style={[
                    styles.otaProgressFill,
                    { width: `${otaProgress}%` },
                  ]}
                />
              </View>
              <ActivityIndicator color={accent.blue} style={{ marginTop: 8 }} />
            </View>
          )}

          {!otaUploading && (
            <>
              <TouchableOpacity style={styles.otaBtn} onPress={handleStartOta}>
                <Text style={styles.otaBtnText}>FIRMWARE-DATEI AUSWAEHLEN</Text>
              </TouchableOpacity>
              {otaStatus && otaStatus.state === 1 && (
                <TouchableOpacity
                  style={[styles.otaBtn, styles.otaCancelBtn]}
                  onPress={async () => {
                    await cancelOta();
                    Alert.alert('Abgebrochen', 'OTA-Update abgebrochen.');
                  }}
                >
                  <Text style={styles.otaBtnText}>ABBRECHEN</Text>
                </TouchableOpacity>
              )}
            </>
          )}
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

        {/* Backup / Restore */}
        <Card>
          <Text style={styles.sectionTitle}>BACKUP / RESTORE</Text>
          <View style={styles.backupRow}>
            <TouchableOpacity style={styles.backupBtn} onPress={handleBackup}>
              <Text style={styles.backupBtnText}>BACKUP</Text>
            </TouchableOpacity>
            <TouchableOpacity style={styles.backupBtn} onPress={handleRestore}>
              <Text style={styles.backupBtnText}>RESTORE</Text>
            </TouchableOpacity>
          </View>
        </Card>

        {/* CarPlay-Display-Pairing */}
        <Card>
          <Text style={styles.sectionTitle}>CARPLAY-DISPLAY</Text>

          <View style={styles.row}>
            <Text style={styles.label}>CarPlay aktiviert</Text>
            <Switch
              value={config.carplayEnabled}
              onValueChange={(v) => updateConfig({ carplayEnabled: v })}
              trackColor={{ false: bg.raised, true: accent.blue }}
            />
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Display-Name</Text>
            <Text style={styles.value}>{config.carplayDisplayName || 'Transalp-BC'}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Gepaart</Text>
            <Text
              style={[
                styles.value,
                { color: config.carplayPaired ? accent.green : fg.tertiary },
              ]}
            >
              {config.carplayPaired ? 'Ja' : 'Nein'}
            </Text>
          </View>
        </Card>

        {/* Controller-Typ */}
        <Card>
          <Text style={styles.sectionTitle}>CONTROLLER-TYP</Text>

          <View style={controllerTypeStyles.container}>
            {(
              [
                ControllerType.None,
                ControllerType.HondaOem,
                ControllerType.GenericBle,
                ControllerType.Keyboard,
              ] as ControllerType[]
            ).map((t) => {
              const labels: Record<ControllerType, string> = {
                [ControllerType.None]: 'Kein',
                [ControllerType.HondaOem]: 'Honda OEM',
                [ControllerType.GenericBle]: 'Generic BLE',
                [ControllerType.Keyboard]: 'Tastatur',
              };
              return (
                <TouchableOpacity
                  key={t}
                  style={[
                    controllerTypeStyles.option,
                    config.controllerType === t && controllerTypeStyles.optionActive,
                  ]}
                  onPress={() => updateConfig({ controllerType: t })}
                >
                  <Text
                    style={[
                      controllerTypeStyles.text,
                      config.controllerType === t && controllerTypeStyles.textActive,
                    ]}
                  >
                    {labels[t]}
                  </Text>
                </TouchableOpacity>
              );
            })}
          </View>
          <Text style={styles.subLabel}>
            Tastatur-Modus fuer Debugging ueber den Controller-Tab verfuegbar.
          </Text>
        </Card>

        {/* Apply */}
        <TouchableOpacity
          style={[styles.applyBtn, applying && styles.applyBtnDisabled]}
          onPress={handleApply}
          disabled={applying}
        >
          {applying ? (
            <ActivityIndicator color="#FFFFFF" />
          ) : (
            <Text style={styles.applyBtnText}>AUF BORDCOMPUTER ANWENDEN</Text>
          )}
        </TouchableOpacity>
      </ScrollView>
    </ScreenContainer>
  );
}

const styles = StyleSheet.create({
  sectionTitle: {
    fontSize: fontSize.body,
    fontWeight: '600',
    color: fg.secondary,
    textTransform: 'uppercase',
    letterSpacing: 1.5,
    marginBottom: 12,
  },
  subLabel: {
    fontSize: fontSize.label,
    color: fg.tertiary,
    textTransform: 'uppercase',
    letterSpacing: 1,
    marginTop: 8,
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
  toggleGroup: {
    flexDirection: 'row',
    gap: 4,
  },
  toggleBtn: {
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 6,
    backgroundColor: bg.raised,
  },
  toggleBtnActive: {
    backgroundColor: accent.blue,
  },
  toggleText: {
    fontSize: fontSize.label,
    color: fg.secondary,
    fontWeight: '600',
  },
  toggleTextActive: {
    color: '#FFFFFF',
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
  otaBtn: {
    backgroundColor: accent.blue,
    borderRadius: 10,
    padding: 14,
    alignItems: 'center',
    marginTop: 12,
  },
  otaCancelBtn: {
    backgroundColor: accent.red,
  },
  otaBtnText: {
    fontSize: fontSize.label,
    fontWeight: 'bold',
    color: '#FFFFFF',
  },
  otaProgress: {
    marginTop: 12,
    padding: 12,
    backgroundColor: bg.raised,
    borderRadius: 8,
  },
  otaProgressHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  otaProgressBar: {
    height: 12,
    backgroundColor: bg.deep,
    borderRadius: 6,
    overflow: 'hidden',
  },
  otaProgressFill: {
    height: '100%',
    backgroundColor: accent.blue,
    borderRadius: 6,
  },
  backupRow: {
    flexDirection: 'row',
    gap: 8,
    marginTop: 8,
  },
  backupBtn: {
    flex: 1,
    padding: 12,
    borderRadius: 8,
    backgroundColor: bg.raised,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: bg.overlay,
  },
  backupBtnText: {
    fontSize: fontSize.label,
    fontWeight: '600',
    color: accent.cyan,
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
    color: '#FFFFFF',
  },
});

const controllerTypeStyles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
    marginVertical: 4,
  },
  option: {
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderRadius: 8,
    backgroundColor: bg.raised,
  },
  optionActive: {
    backgroundColor: accent.blue,
  },
  text: {
    fontSize: fontSize.label,
    color: fg.secondary,
    fontWeight: '500',
  },
  textActive: {
    color: '#FFFFFF',
    fontWeight: 'bold',
  },
});

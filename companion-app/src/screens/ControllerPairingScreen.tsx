/**
 * Controller-Pairing-Screen.
 *
 * Verwaltet den Lenker-Controller und dessen Tastenbelegung.
 *
 * Features:
 *  - Anzeige des konfigurierten Controller-Typs
 *  - Mock-Controller-Input ueber Tastatur (fuer Debugging)
 *  - Button-Mapping-Konfiguration (welche Taste macht was)
 *  - Test-Ausgabe der zuletzt gedrueckten Taste + Aktion
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Platform,
} from 'react-native';
import { ScreenContainer, ScreenHeader, Card, EmptyState } from '../components/UI';
import { useAppStore } from '../core/state/store';
import { bg, fg, accent, fontSize } from '../core/theme/colors';
import {
  ControllerType,
  ControllerButton,
  ControllerAction,
  CONTROLLER_KEY_MAP,
} from '../core/ble/gatt-uuids';

// --- Controller-Typ-Labels ---

const CONTROLLER_TYPE_LABELS: Record<ControllerType, string> = {
  [ControllerType.None]: 'Kein Controller',
  [ControllerType.HondaOem]: 'Honda OEM',
  [ControllerType.GenericBle]: 'Generic BLE',
  [ControllerType.Keyboard]: 'Tastatur (Mock)',
};

// --- Button-Labels ---

const BUTTON_LABELS: Record<ControllerButton, string> = {
  [ControllerButton.Up]: 'Oben',
  [ControllerButton.Down]: 'Unten',
  [ControllerButton.Left]: 'Links',
  [ControllerButton.Right]: 'Rechts',
  [ControllerButton.Select]: 'Select',
  [ControllerButton.Back]: 'Zurueck',
  [ControllerButton.Mode]: 'Mode',
  [ControllerButton.VolumeUp]: 'Lauter',
  [ControllerButton.VolumeDown]: 'Leiser',
};

// --- Action-Labels ---

const ACTION_LABELS: Record<ControllerAction, string> = {
  [ControllerAction.None]: 'Keine',
  [ControllerAction.NextTrack]: 'Naechster Track',
  [ControllerAction.PrevTrack]: 'Vorheriger Track',
  [ControllerAction.VolumeUp]: 'Lauter',
  [ControllerAction.VolumeDown]: 'Leiser',
  [ControllerAction.PlayPause]: 'Play/Pause',
  [ControllerAction.Navigate]: 'Navigation',
  [ControllerAction.AnswerCall]: 'Anruf annehmen',
  [ControllerAction.EndCall]: 'Anruf beenden',
  [ControllerAction.VoiceControl]: 'Sprachsteuerung',
  [ControllerAction.Dashboard]: 'Dashboard',
  [ControllerAction.CarPlay]: 'CarPlay',
};

// --- Alle Buttons in Reihenfolge ---

const ALL_BUTTONS: ControllerButton[] = [
  ControllerButton.Up,
  ControllerButton.Down,
  ControllerButton.Left,
  ControllerButton.Right,
  ControllerButton.Select,
  ControllerButton.Back,
  ControllerButton.Mode,
  ControllerButton.VolumeUp,
  ControllerButton.VolumeDown,
];

// --- Alle verfuegbaren Aktionen ---

const ALL_ACTIONS: ControllerAction[] = [
  ControllerAction.None,
  ControllerAction.NextTrack,
  ControllerAction.PrevTrack,
  ControllerAction.VolumeUp,
  ControllerAction.VolumeDown,
  ControllerAction.PlayPause,
  ControllerAction.Navigate,
  ControllerAction.AnswerCall,
  ControllerAction.EndCall,
  ControllerAction.VoiceControl,
  ControllerAction.Dashboard,
  ControllerAction.CarPlay,
];

// --- Action-Selector (fuer das Mapping) ---

function ActionSelector({
  button,
  currentAction,
  onSelect,
}: {
  button: ControllerButton;
  currentAction: ControllerAction;
  onSelect: (action: ControllerAction) => void;
}) {
  const [expanded, setExpanded] = useState(false);
  const buttonLabel = BUTTON_LABELS[button] ?? 'Unbekannt';

  return (
    <View style={selectorStyles.container}>
      <View style={selectorStyles.row}>
        <Text style={selectorStyles.buttonLabel}>{buttonLabel}</Text>
        <TouchableOpacity
          style={selectorStyles.currentBtn}
          onPress={() => setExpanded((e) => !e)}
        >
          <Text style={selectorStyles.currentText}>
            {ACTION_LABELS[currentAction] ?? 'Keine'}
          </Text>
          <Text style={selectorStyles.chevron}>{expanded ? '\u25B2' : '\u25BC'}</Text>
        </TouchableOpacity>
      </View>
      {expanded && (
        <View style={selectorStyles.dropdown}>
          {ALL_ACTIONS.map((action) => (
            <TouchableOpacity
              key={action}
              style={[
                selectorStyles.option,
                action === currentAction && selectorStyles.optionActive,
              ]}
              onPress={() => {
                onSelect(action);
                setExpanded(false);
              }}
            >
              <Text
                style={[
                  selectorStyles.optionText,
                  action === currentAction && selectorStyles.optionTextActive,
                ]}
              >
                {ACTION_LABELS[action] ?? 'Keine'}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      )}
    </View>
  );
}

const selectorStyles = StyleSheet.create({
  container: {
    paddingVertical: 4,
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 4,
  },
  buttonLabel: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: '500',
  },
  currentBtn: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 6,
    backgroundColor: bg.raised,
  },
  currentText: {
    fontSize: fontSize.label,
    color: fg.secondary,
    fontWeight: '600',
  },
  chevron: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
  },
  dropdown: {
    marginTop: 4,
    borderRadius: 8,
    backgroundColor: bg.deep,
    overflow: 'hidden',
  },
  option: {
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: bg.raised,
  },
  optionActive: {
    backgroundColor: accent.blue,
  },
  optionText: {
    fontSize: fontSize.label,
    color: fg.secondary,
  },
  optionTextActive: {
    color: '#FFFFFF',
    fontWeight: 'bold',
  },
});

// --- Mock-Tastatur-Listener ---

function useKeyboardController(
  enabled: boolean,
  onButton: (button: ControllerButton) => void,
) {
  useEffect(() => {
    if (!enabled) return;
    if (Platform.OS !== 'web') return;

    const handler = (e: KeyboardEvent) => {
      const button = CONTROLLER_KEY_MAP[e.key];
      if (button !== undefined) {
        e.preventDefault();
        onButton(button);
      }
    };

    window.addEventListener('keydown', handler);
    return () => window.removeEventListener('keydown', handler);
  }, [enabled, onButton]);
}

// --- Main Screen ---

export function ControllerPairingScreen() {
  const config = useAppStore((s) => s.config);
  const updateConfig = useAppStore((s) => s.updateConfig);
  const controllerMap = useAppStore((s) => s.controllerMap);
  const setControllerButton = useAppStore((s) => s.setControllerButton);

  const [lastButton, setLastButton] = useState<ControllerButton | null>(null);
  const [lastAction, setLastAction] = useState<ControllerAction | null>(null);
  const [log, setLog] = useState<string[]>([]);

  const isKeyboardMock = config.controllerType === ControllerType.Keyboard;

  const handleButton = useCallback(
    (button: ControllerButton) => {
      const action = controllerMap[button] ?? ControllerAction.None;
      setLastButton(button);
      setLastAction(action);

      const entry = `${BUTTON_LABELS[button]} -> ${ACTION_LABELS[action] ?? 'Keine'}`;
      setLog((prev) => [entry, ...prev].slice(0, 8));
    },
    [controllerMap],
  );

  useKeyboardController(isKeyboardMock, handleButton);

  // Simuliert einen Button-Press (fuer manuelles Testing auf Geraet)
  const simulateButton = (button: ControllerButton) => {
    handleButton(button);
  };

  return (
    <ScreenContainer>
      <ScrollView showsVerticalScrollIndicator={false}>
        <ScreenHeader title="CONTROLLER" />

        {/* Controller-Typ */}
        <Card>
          <Text style={styles.sectionTitle}>CONTROLLER-TYP</Text>
          <View style={styles.row}>
            <Text style={styles.label}>Aktiv</Text>
            <Text style={styles.value}>
              {CONTROLLER_TYPE_LABELS[config.controllerType] ?? 'Unbekannt'}
            </Text>
          </View>

          <Text style={styles.subLabel}>Typ aendern</Text>
          <View style={typeSelectorStyles.container}>
            {(
              [
                ControllerType.None,
                ControllerType.HondaOem,
                ControllerType.GenericBle,
                ControllerType.Keyboard,
              ] as ControllerType[]
            ).map((t) => (
              <TouchableOpacity
                key={t}
                style={[
                  typeSelectorStyles.option,
                  config.controllerType === t && typeSelectorStyles.optionActive,
                ]}
                onPress={() => updateConfig({ controllerType: t })}
              >
                <Text
                  style={[
                    typeSelectorStyles.text,
                    config.controllerType === t && typeSelectorStyles.textActive,
                  ]}
                >
                  {CONTROLLER_TYPE_LABELS[t] ?? 'Unbekannt'}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </Card>

        {/* Mock-Tastatur-Hinweis */}
        {isKeyboardMock && (
          <Card>
            <Text style={styles.sectionTitle}>MOCK-EINGABE</Text>
            <Text style={mockInfoStyles.text}>
              Tastatur-Modus aktiv. Im Web-Preview koennen folgende Tasten
              verwendet werden:
            </Text>
            <View style={mockInfoStyles.keyGrid}>
              {Object.entries(CONTROLLER_KEY_MAP).map(([key, button]) => (
                <View key={key} style={mockInfoStyles.keyRow}>
                  <Text style={mockInfoStyles.key}>{key}</Text>
                  <Text style={mockInfoStyles.arrow}>{'->'}</Text>
                  <Text style={mockInfoStyles.action}>
                    {BUTTON_LABELS[button] ?? 'Unbekannt'}
                  </Text>
                </View>
              ))}
            </View>
          </Card>
        )}

        {/* Test-Buttons (manuelle Eingabe) */}
        <Card>
          <Text style={styles.sectionTitle}>TEST-EINGABE</Text>
          <View style={testBtnStyles.grid}>
            {ALL_BUTTONS.map((button) => (
              <TouchableOpacity
                key={button}
                style={[
                  testBtnStyles.btn,
                  lastButton === button && testBtnStyles.btnActive,
                ]}
                onPress={() => simulateButton(button)}
              >
                <Text style={testBtnStyles.btnText}>
                  {BUTTON_LABELS[button] ?? 'Unbekannt'}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </Card>

        {/* Letzte Eingabe */}
        <Card>
          <Text style={styles.sectionTitle}>LETZTE EINGABE</Text>
          {lastButton !== null ? (
            <View>
              <View style={styles.row}>
                <Text style={styles.label}>Taste</Text>
                <Text style={styles.value}>
                  {BUTTON_LABELS[lastButton] ?? 'Unbekannt'}
                </Text>
              </View>
              <View style={styles.row}>
                <Text style={styles.label}>Aktion</Text>
                <Text style={[styles.value, { color: accent.cyan }]}>
                  {lastAction !== null ? ACTION_LABELS[lastAction] ?? 'Keine' : '-'}
                </Text>
              </View>
            </View>
          ) : (
            <EmptyState message="Noch keine Eingabe. Druecken Sie eine Taste oder verwenden Sie die Test-Buttons." />
          )}
        </Card>

        {/* Eingabe-Protokoll */}
        {log.length > 0 && (
          <Card>
            <Text style={styles.sectionTitle}>PROTOKOLL</Text>
            {log.map((entry, i) => (
              <Text key={i} style={logStyles.entry}>
                {entry}
              </Text>
            ))}
          </Card>
        )}

        {/* Button-Mapping */}
        <Card>
          <Text style={styles.sectionTitle}>TASTENBELEGUNG</Text>
          {ALL_BUTTONS.map((button) => (
            <ActionSelector
              key={button}
              button={button}
              currentAction={controllerMap[button] ?? ControllerAction.None}
              onSelect={(action) => setControllerButton(button, action)}
            />
          ))}
        </Card>
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
    marginBottom: 4,
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 6,
  },
  label: {
    fontSize: fontSize.body,
    color: fg.secondary,
  },
  value: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: '500',
  },
});

const typeSelectorStyles = StyleSheet.create({
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

const mockInfoStyles = StyleSheet.create({
  text: {
    fontSize: fontSize.body,
    color: fg.secondary,
    marginBottom: 8,
  },
  keyGrid: {
    gap: 4,
  },
  keyRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    paddingVertical: 2,
  },
  key: {
    fontSize: fontSize.body,
    color: accent.cyan,
    fontFamily: 'monospace',
    fontWeight: 'bold',
    minWidth: 80,
  },
  arrow: {
    fontSize: fontSize.body,
    color: fg.tertiary,
  },
  action: {
    fontSize: fontSize.body,
    color: fg.primary,
  },
});

const testBtnStyles = StyleSheet.create({
  grid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
  },
  btn: {
    paddingHorizontal: 14,
    paddingVertical: 10,
    borderRadius: 8,
    backgroundColor: bg.raised,
    alignItems: 'center',
  },
  btnActive: {
    backgroundColor: accent.blue,
  },
  btnText: {
    fontSize: fontSize.label,
    color: fg.primary,
    fontWeight: '600',
  },
});

const logStyles = StyleSheet.create({
  entry: {
    fontSize: fontSize.label,
    color: fg.secondary,
    fontFamily: 'monospace',
    paddingVertical: 2,
  },
});

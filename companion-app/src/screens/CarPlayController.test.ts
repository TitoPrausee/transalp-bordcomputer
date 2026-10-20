/**
 * ControllerMapping.test.ts — Tests fuer Controller-Support.
 *
 * Testet:
 *   - Controller-Button-Mapping im Store (setControllerButton, DEFAULT_CONTROLLER_MAP)
 *   - CONTROLLER_KEY_MAP Tastatur-Mapping
 *   - ControllerType enum
 *
 * CarPlay 0x9000 service tests removed (spec §11: app must not touch service 0x9000).
 */

import { useAppStore, DEFAULT_CONTROLLER_MAP } from '../core/state/store';
import {
  ControllerType,
  ControllerButton,
  ControllerAction,
  CONTROLLER_KEY_MAP,
} from '../core/ble/gatt-uuids';

// Reset store before each test
beforeEach(() => {
  useAppStore.setState({
    controllerMap: { ...DEFAULT_CONTROLLER_MAP },
  });
});

// ============================================================
// Controller-Mapping Store
// ============================================================

describe('Controller button mapping store', () => {
  it('should update a single controller button mapping', () => {
    useAppStore.getState().setControllerButton(
      ControllerButton.Select,
      ControllerAction.Navigate,
    );
    const state = useAppStore.getState();
    expect(state.controllerMap[ControllerButton.Select]).toBe(ControllerAction.Navigate);
  });

  it('should have all controller buttons mapped in default config', () => {
    const map = useAppStore.getState().controllerMap;
    const allButtons = [
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
    for (const btn of allButtons) {
      expect(map[btn]).toBeDefined();
    }
  });

  it('should default Select button to PlayPause action', () => {
    useAppStore.setState({ controllerMap: { ...DEFAULT_CONTROLLER_MAP } });
    const map = useAppStore.getState().controllerMap;
    expect(map[ControllerButton.Select]).toBe(ControllerAction.PlayPause);
  });
});

// ============================================================
// CONTROLLER_KEY_MAP (Tastatur-Mock)
// ============================================================

describe('CONTROLLER_KEY_MAP keyboard mapping', () => {
  it('should map ArrowUp to ControllerButton.Up', () => {
    expect(CONTROLLER_KEY_MAP['ArrowUp']).toBe(ControllerButton.Up);
  });

  it('should map Enter to ControllerButton.Select', () => {
    expect(CONTROLLER_KEY_MAP['Enter']).toBe(ControllerButton.Select);
  });

  it('should map Escape to ControllerButton.Back', () => {
    expect(CONTROLLER_KEY_MAP['Escape']).toBe(ControllerButton.Back);
  });

  it('should map + and - to volume buttons', () => {
    expect(CONTROLLER_KEY_MAP['+']).toBe(ControllerButton.VolumeUp);
    expect(CONTROLLER_KEY_MAP['-']).toBe(ControllerButton.VolumeDown);
  });

  it('should return undefined for unmapped keys', () => {
    expect(CONTROLLER_KEY_MAP['x']).toBeUndefined();
    expect(CONTROLLER_KEY_MAP['Space']).toBeUndefined();
  });
});

// ============================================================
// ControllerType Config
// ============================================================

describe('ControllerType config', () => {
  it('should have all controller type enum values', () => {
    expect(ControllerType.None).toBe(0);
    expect(ControllerType.HondaOem).toBe(1);
    expect(ControllerType.GenericBle).toBe(2);
    expect(ControllerType.Keyboard).toBe(3);
  });

  it('should allow updating controllerType in config', () => {
    useAppStore.getState().updateConfig({ controllerType: ControllerType.Keyboard });
    expect(useAppStore.getState().config.controllerType).toBe(ControllerType.Keyboard);

    useAppStore.getState().updateConfig({ controllerType: ControllerType.HondaOem });
    expect(useAppStore.getState().config.controllerType).toBe(ControllerType.HondaOem);
  });
});

/**
 * CarPlayController.test.ts — Tests fuer CarPlay-Status und Controller-Support.
 *
 * Testet:
 *   - CarPlay-Status im Store (setCarPlayStatus)
 *   - Controller-Button-Mapping im Store (setControllerButton, DEFAULT_CONTROLLER_MAP)
 *   - CONTROLLER_KEY_MAP Tastatur-Mapping
 *   - BleManager.encodeCarPlayCmd (CarPlay-Control-Encoding)
 *   - BleManager handleNotification fuer CarPlay-Status
 *   - MockBleProvider CarPlay-Control (NextTrack, PrevTrack, Unpair, Pair)
 *   - MockBleProvider readCarPlayStatus
 */

import { useAppStore, DEFAULT_CONTROLLER_MAP, MOCK_CARPLAY } from '../core/state/store';
import { BleManager } from '../services/BleManager';
import { MockBleProvider } from '../core/ble/MockBleProvider';
import {
  CarPlayApp,
  CarPlayControl,
  ControllerType,
  ControllerButton,
  ControllerAction,
  CONTROLLER_KEY_MAP,
} from '../core/ble/gatt-uuids';
import { ConnectionState } from '../core/ble/connection-state';

// Helper
function waitFor(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

// Reset store before each test
beforeEach(() => {
  useAppStore.setState({
    carplay: { ...MOCK_CARPLAY },
    controllerMap: { ...DEFAULT_CONTROLLER_MAP },
  });
});

// ============================================================
// CarPlay-Status Store
// ============================================================

describe('CarPlay store state', () => {
  it('should update CarPlay status via setCarPlayStatus', () => {
    const status = {
      connected: true,
      activeApp: CarPlayApp.Music,
      artist: 'Iron Maiden',
      title: 'The Trooper',
      album: 'Piece of Mind',
    };
    useAppStore.getState().setCarPlayStatus(status);
    const state = useAppStore.getState();
    expect(state.carplay.connected).toBe(true);
    expect(state.carplay.activeApp).toBe(CarPlayApp.Music);
    expect(state.carplay.artist).toBe('Iron Maiden');
    expect(state.carplay.title).toBe('The Trooper');
    expect(state.carplay.album).toBe('Piece of Mind');
  });

  it('should start with disconnected mock CarPlay state', () => {
    useAppStore.setState({ carplay: { ...MOCK_CARPLAY } });
    const state = useAppStore.getState();
    expect(state.carplay.connected).toBe(false);
    expect(state.carplay.activeApp).toBe(CarPlayApp.None);
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
// BleManager CarPlay Encoding
// ============================================================

describe('BleManager CarPlay encode', () => {
  const provider = new MockBleProvider();
  const mgr = new BleManager(provider);

  it('should encode CarPlay control command as base64 string', () => {
    const encoded = mgr.encodeCarPlayCmd(CarPlayControl.NextTrack);
    expect(typeof encoded).toBe('string');
    expect(encoded.length).toBeGreaterThan(0);
  });

  it('should produce different encodings for different commands', () => {
    const next = mgr.encodeCarPlayCmd(CarPlayControl.NextTrack);
    const prev = mgr.encodeCarPlayCmd(CarPlayControl.PrevTrack);
    const pair = mgr.encodeCarPlayCmd(CarPlayControl.Pair);
    expect(next).not.toBe(prev);
    expect(next).not.toBe(pair);
    expect(prev).not.toBe(pair);
  });

  mgr.destroy();
});

// ============================================================
// BleManager CarPlay Notification
// ============================================================

describe('BleManager CarPlay notification', () => {
  it('should dispatch CarPlay status on carplayStatus notification', async () => {
    const provider = new MockBleProvider();
    const mgr = new BleManager(provider);
    await mgr.init();

    const statuses: unknown[] = [];
    mgr.addListener({ onCarPlayStatus: (s) => statuses.push(s) });

    // Connect to get CarPlay stream running
    await mgr.startScan();
    await waitFor(4200);

    // The mock provider sends an initial CarPlay status on connect
    expect(statuses.length).toBeGreaterThan(0);
    const status = statuses[0] as { connected: boolean; activeApp: number };
    expect(status.connected).toBe(true);

    mgr.destroy();
  }, 15000);
});

// ============================================================
// MockBleProvider CarPlay Control
// ============================================================

describe('MockBleProvider CarPlay control', () => {
  let provider: MockBleProvider;

  beforeEach(async () => {
    provider = new MockBleProvider();
    await provider.init();
  });

  afterEach(() => {
    provider.destroy();
  });

  it('should read CarPlay status with music metadata', async () => {
    const status = await provider.readCarPlayStatus();
    expect(status).not.toBeNull();
    expect(status!.connected).toBe(true);
    expect(status!.activeApp).toBe(CarPlayApp.Music);
    expect(status!.artist.length).toBeGreaterThan(0);
    expect(status!.title.length).toBeGreaterThan(0);
  });

  it('should advance to next track on NextTrack command', async () => {
    const before = await provider.readCarPlayStatus();
    await provider.sendCarPlayControl(CarPlayControl.NextTrack);
    const after = await provider.readCarPlayStatus();
    expect(after!.connected).toBe(true);
    expect(after!.title).not.toBe(before!.title);
  });

  it('should go to previous track on PrevTrack command', async () => {
    const initial = await provider.readCarPlayStatus();
    await provider.sendCarPlayControl(CarPlayControl.NextTrack);
    await provider.sendCarPlayControl(CarPlayControl.PrevTrack);
    const after = await provider.readCarPlayStatus();
    expect(after!.title).toBe(initial!.title);
    expect(after!.artist).toBe(initial!.artist);
  });

  it('should disconnect CarPlay on Unpair command', async () => {
    await provider.sendCarPlayControl(CarPlayControl.Unpair);
    const status = await provider.readCarPlayStatus();
    expect(status!.connected).toBe(false);
    expect(status!.activeApp).toBe(CarPlayApp.None);
    expect(status!.artist).toBe('');
  });

  it('should reconnect CarPlay on Pair command after unpair', async () => {
    await provider.sendCarPlayControl(CarPlayControl.Unpair);
    expect((await provider.readCarPlayStatus())!.connected).toBe(false);

    await provider.sendCarPlayControl(CarPlayControl.Pair);
    const status = await provider.readCarPlayStatus();
    expect(status!.connected).toBe(true);
    expect(status!.activeApp).toBe(CarPlayApp.Music);
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

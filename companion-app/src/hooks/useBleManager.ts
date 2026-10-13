/**
 * BLE-Manager Hook.
 *
 * Verwaltet den Lebenszyklus des BLE-Providers, verdrahtet die
 * Event-Callbacks mit dem Zustand-Store und stellt die Verbindungsfunktionen
 * fuer die UI zur Verfuegung.
 *
 * Im Prototyp wird immer der MockBleProvider verwendet. Bei echtem Geraet
 * wuerde hier der RealBleProvider (react-native-ble-plx) instanziiert.
 */

import { useEffect, useRef, useCallback } from 'react';
import { BleProvider } from '../core/ble/BleProvider';
import { MockBleProvider } from '../core/ble/MockBleProvider';
import { ConnectionState } from '../core/ble/connection-state';
import { useAppStore } from '../core/state/store';

export function useBleManager() {
  const providerRef = useRef<BleProvider | null>(null);

  // Store-Aktionen holen (stabil durch Zustand)
  const setBleState = useAppStore((s) => s.setBleState);
  const setBleRssi = useAppStore((s) => s.setBleRssi);
  const setBleMtu = useAppStore((s) => s.setBleMtu);
  const setBleDevice = useAppStore((s) => s.setBleDevice);
  const setTelemetry = useAppStore((s) => s.setTelemetry);
  const setNavManeuver = useAppStore((s) => s.setNavManeuver);
  const setNavRoute = useAppStore((s) => s.setNavRoute);
  const setNavRoad = useAppStore((s) => s.setNavRoad);
  const setNavStatus = useAppStore((s) => s.setNavStatus);
  const setSensorStatus = useAppStore((s) => s.setSensorStatus);
  const setDtcList = useAppStore((s) => s.setDtcList);
  const setSelfTestResult = useAppStore((s) => s.setSelfTestResult);
  const setWeather = useAppStore((s) => s.setWeather);

  // Provider initialisieren
  useEffect(() => {
    const provider = new MockBleProvider();
    providerRef.current = provider;

    provider.init().then(() => {
      provider.subscribe({
        onConnectionStateChanged: (state) => {
          setBleState(state);
          if (state === ConnectionState.Connected) {
            const dev = provider.getConnectedDevice();
            if (dev) setBleDevice(dev.name, dev.id);
          }
        },
        onTelemetry: (data) => setTelemetry(data),
        onNavManeuver: (data) => setNavManeuver(data),
        onNavRoute: (data) => setNavRoute(data),
        onNavRoad: (data) => setNavRoad(data),
        onNavStatus: (status) => setNavStatus(status),
        onSensorStatus: (data) => setSensorStatus(data),
        onDtcList: (entries) => setDtcList(entries),
        onSelfTestResult: (result) => setSelfTestResult(result),
        onWeatherCurrent: (data) => setWeather(data),
        onRssi: (rssi) => setBleRssi(rssi),
        onMtu: (mtu) => setBleMtu(mtu),
      });

      // Auto-Connect im Mock-Modus
      provider.startScan();
    });

    return () => {
      provider.destroy();
      providerRef.current = null;
    };
  }, [
    setBleState,
    setBleRssi,
    setBleMtu,
    setBleDevice,
    setTelemetry,
    setNavManeuver,
    setNavRoute,
    setNavRoad,
    setNavStatus,
    setSensorStatus,
    setDtcList,
    setSelfTestResult,
    setWeather,
  ]);

  const reconnect = useCallback(async () => {
    if (providerRef.current) {
      await providerRef.current.startScan();
    }
  }, []);

  const disconnect = useCallback(async () => {
    if (providerRef.current) {
      await providerRef.current.disconnect();
    }
  }, []);

  const triggerSelfTest = useCallback(async () => {
    if (providerRef.current) {
      await providerRef.current.triggerSelfTest();
    }
  }, []);

  const deleteAllBonds = useCallback(async () => {
    if (providerRef.current) {
      await providerRef.current.deleteAllBonds();
    }
  }, []);

  const writeConfig = useCallback(async (configJson: string) => {
    if (providerRef.current) {
      await providerRef.current.writeConfig(configJson);
    }
  }, []);

  return {
    reconnect,
    disconnect,
    triggerSelfTest,
    deleteAllBonds,
    writeConfig,
  };
}

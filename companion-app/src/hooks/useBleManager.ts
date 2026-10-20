/**
 * BLE-Manager Hook.
 *
 * Verwaltet den Lebenszyklus des BLE-Providers, verdrahtet die
 * Event-Callbacks mit dem Zustand-Store und stellt die Verbindungsfunktionen
 * fuer die UI zur Verfuegung.
 *
 * In Development wird der MockBleProvider verwendet. In Produktion
 * (auf einem echten Geraet) wird der RealBleProvider (react-native-ble-plx)
 * instanziiert. Die Auswahl erfolgt ueber __DEV__.
 */

import { useEffect, useRef, useCallback } from 'react';
import type { BleProvider } from '../core/ble/BleProvider';
import { MockBleProvider } from '../core/ble/MockBleProvider';
import { RealBleProvider } from '../core/ble/RealBleProvider';
import { ConnectionState } from '../core/ble/connection-state';
import { SelfTestMode } from '../core/ble/gatt-uuids';
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
  const setSystemStatus = useAppStore((s) => s.setSystemStatus);
  const setOtaStatus = useAppStore((s) => s.setOtaStatus);
  const setOtaVerifyResult = useAppStore((s) => s.setOtaVerifyResult);

  // Error / Reconnection State
  const setLastError = useAppStore((s) => s.setLastError);
  const clearError = useAppStore((s) => s.clearError);
  const setReconnectAttempts = useAppStore((s) => s.setReconnectAttempts);
  const setRetrying = useAppStore((s) => s.setRetrying);

  // Provider initialisieren — Mock in Development, Real in Produktion
  useEffect(() => {
    const provider = __DEV__ ? new MockBleProvider() : new RealBleProvider();
    providerRef.current = provider;

    provider.init().then(() => {
      provider.subscribe({
        onConnectionStateChanged: (state) => {
          setBleState(state);
          if (state === ConnectionState.Connected) {
            const dev = provider.getConnectedDevice();
            if (dev) setBleDevice(dev.name, dev.id);
            // Clear errors and retry state on successful connection
            clearError();
            setReconnectAttempts(0);
            setRetrying(false);
          } else if (state === ConnectionState.Reconnecting) {
            setRetrying(true);
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
        onOtaStatus: (status) => setOtaStatus(status),
        onOtaVerifyResult: (result) => setOtaVerifyResult(result),
        onRssi: (rssi) => setBleRssi(rssi),
        onMtu: (mtu) => setBleMtu(mtu),
        onError: (error) => setLastError(error),
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
    setSystemStatus,
    setOtaStatus,
    setOtaVerifyResult,
    setLastError,
    clearError,
    setReconnectAttempts,
    setRetrying,
  ]);

  const reconnect = useCallback(async () => {
    if (providerRef.current) {
      clearError();
      setRetrying(true);
      await providerRef.current.startScan();
    }
  }, [clearError, setRetrying]);

  const disconnect = useCallback(async () => {
    if (providerRef.current) {
      await providerRef.current.disconnect();
    }
  }, []);

  const triggerSelfTest = useCallback(async (mode?: SelfTestMode) => {
    if (providerRef.current) {
      await providerRef.current.triggerSelfTest(mode);
    }
  }, []);

  const clearDtc = useCallback(async (code?: number) => {
    if (providerRef.current) {
      await providerRef.current.clearDtc(code);
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

  const readSystemStatus = useCallback(async () => {
    return useAppStore.getState().systemStatus;
  }, []);

  const readOtaStatus = useCallback(async () => {
    return useAppStore.getState().otaStatus;
  }, []);

  const startOta = useCallback(async (firmwareSize: number) => {
    if (providerRef.current) {
      await providerRef.current.startOta(firmwareSize);
    }
  }, []);

  const writeOtaData = useCallback(async (data: Uint8Array) => {
    if (providerRef.current) {
      await providerRef.current.writeOtaData(data);
    }
  }, []);

  const cancelOta = useCallback(async () => {
    if (providerRef.current) {
      await providerRef.current.cancelOta();
    }
  }, []);

  const verifyOta = useCallback(async () => {
    if (providerRef.current) {
      await providerRef.current.verifyOta();
    }
  }, []);

  const commitOta = useCallback(async (expectedSha256?: Uint8Array) => {
    if (providerRef.current) {
      await providerRef.current.commitOta(expectedSha256);
    }
  }, []);

  const exportConfig = useCallback(async () => {
    if (providerRef.current) {
      return await providerRef.current.exportConfig();
    }
    return '';
  }, []);

  const importConfig = useCallback(async (configJson: string) => {
    if (providerRef.current) {
      await providerRef.current.importConfig(configJson);
    }
  }, []);

  return {
    reconnect,
    disconnect,
    triggerSelfTest,
    clearDtc,
    deleteAllBonds,
    writeConfig,
    readSystemStatus,
    readOtaStatus,
    startOta,
    writeOtaData,
    cancelOta,
    verifyOta,
    commitOta,
    exportConfig,
    importConfig,
  };
}

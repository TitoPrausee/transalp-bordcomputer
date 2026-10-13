/**
 * Root-App-Komponente.
 *
 * Initialisiert:
 * - BLE-Manager (via useBleManager Hook)
 * - NavigationContainer + TabNavigator
 * - Dark-Mode-Statusbar
 *
 * Die App startet im Dark-Mode (sync zum Bordcomputer).
 * Der Mock-BLE-Provider verbindet sich automatisch beim Start.
 */

import React from 'react';
import { NavigationContainer, DefaultTheme, DarkTheme } from '@react-navigation/native';
import { StatusBar } from 'expo-status-bar';
import { useBleManager } from './hooks/useBleManager';
import { TabNavigator } from './navigation/TabNavigator';
import { bg, accent } from './core/theme/colors';

const AppTheme = {
  ...DarkTheme,
  colors: {
    ...DarkTheme.colors,
    primary: accent.blue,
    background: bg.void,
    card: bg.panel,
    text: '#FFFFFF',
    border: bg.raised,
    notification: '#FF3B30',
  },
};

export default function App() {
  // BLE-Manager initialisieren (startet Auto-Connect)
  useBleManager();

  return (
    <NavigationContainer theme={AppTheme}>
      <StatusBar style="light" />
      <TabNavigator />
    </NavigationContainer>
  );
}

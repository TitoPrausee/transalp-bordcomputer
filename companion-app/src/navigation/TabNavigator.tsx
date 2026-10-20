/**
 * Tab-Navigator-Konfiguration.
 *
 * Bottom-Tab-Navigation mit 7 Hauptbereichen:
 * Dashboard, Navigation, Tours, Diagnose, CarPlay, Controller, Settings.
 */

import React from 'react';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { bg, fg, accent } from '../core/theme/colors';
import { DashboardScreen } from '../screens/DashboardScreen';
import { NavigationScreen } from '../screens/NavigationScreen';
import { DiagnoseScreen } from '../screens/DiagnoseScreen';
import { SettingsScreen } from '../screens/SettingsScreen';
import { ToursScreen } from '../screens/ToursScreen';
import { CarPlayStatusScreen } from '../screens/CarPlayStatusScreen';
import { ControllerPairingScreen } from '../screens/ControllerPairingScreen';

export type TabParamList = {
  Dashboard: undefined;
  Navigation: undefined;
  Tours: undefined;
  Diagnose: undefined;
  CarPlay: undefined;
  Controller: undefined;
  Settings: undefined;
};

const Tab = createBottomTabNavigator<TabParamList>();

export function TabNavigator() {
  return (
    <Tab.Navigator
      screenOptions={{
        headerShown: false,
        tabBarActiveTintColor: accent.blue,
        tabBarInactiveTintColor: fg.tertiary,
        tabBarStyle: {
          backgroundColor: bg.panel,
          borderTopColor: bg.raised,
          borderTopWidth: 1,
        },
        tabBarLabelStyle: {
          fontSize: 10,
          fontWeight: '600',
          textTransform: 'uppercase',
          letterSpacing: 0.5,
        },
      }}
    >
      <Tab.Screen
        name="Dashboard"
        component={DashboardScreen}
        options={{ tabBarLabel: 'Dashboard' }}
      />
      <Tab.Screen
        name="Navigation"
        component={NavigationScreen}
        options={{ tabBarLabel: 'Nav' }}
      />
      <Tab.Screen
        name="Tours"
        component={ToursScreen}
        options={{ tabBarLabel: 'Touren' }}
      />
      <Tab.Screen
        name="Diagnose"
        component={DiagnoseScreen}
        options={{ tabBarLabel: 'Diag' }}
      />
      <Tab.Screen
        name="CarPlay"
        component={CarPlayStatusScreen}
        options={{ tabBarLabel: 'CarPlay' }}
      />
      <Tab.Screen
        name="Controller"
        component={ControllerPairingScreen}
        options={{ tabBarLabel: 'Ctrl' }}
      />
      <Tab.Screen
        name="Settings"
        component={SettingsScreen}
        options={{ tabBarLabel: 'Config' }}
      />
    </Tab.Navigator>
  );
}

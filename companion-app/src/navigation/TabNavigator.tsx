/**
 * Tab-Navigator-Konfiguration.
 *
 * Bottom-Tab-Navigation mit 5 Hauptbereichen:
 * Dashboard, Navigation, Diagnose, Settings, Tours.
 */

import React from 'react';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { bg, fg, accent } from '../core/theme/colors';
import { DashboardScreen } from '../screens/DashboardScreen';
import { NavigationScreen } from '../screens/NavigationScreen';
import { DiagnoseScreen } from '../screens/DiagnoseScreen';
import { SettingsScreen } from '../screens/SettingsScreen';
import { ToursScreen } from '../screens/ToursScreen';

export type TabParamList = {
  Dashboard: undefined;
  Navigation: undefined;
  Diagnose: undefined;
  Settings: undefined;
  Tours: undefined;
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
        name="Settings"
        component={SettingsScreen}
        options={{ tabBarLabel: 'Config' }}
      />
    </Tab.Navigator>
  );
}

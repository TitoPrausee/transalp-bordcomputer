/**
 * Wiederverwendbare UI-Komponenten im Bordcomputer-Design.
 */

import React from 'react';
import { View, Text, StyleSheet, ViewStyle, TextStyle } from 'react-native';
import { bg, fg, accent, fontSize } from '../core/theme/colors';

// --- Card ---

interface CardProps {
  children: React.ReactNode;
  style?: ViewStyle;
}

export function Card({ children, style }: CardProps) {
  return <View style={[styles.card, style]}>{children}</View>;
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: bg.panel,
    borderRadius: 12,
    padding: 16,
    marginVertical: 4,
  },
});

// --- StatTile ---

interface StatTileProps {
  label: string;
  value: string;
  unit?: string;
  color?: string;
  style?: ViewStyle;
}

export function StatTile({ label, value, unit, color = fg.primary, style }: StatTileProps) {
  return (
    <View style={[statTileStyles.tile, style]}>
      <Text style={[statTileStyles.value, { color }]}>{value}</Text>
      {unit ? <Text style={statTileStyles.unit}>{unit}</Text> : null}
      <Text style={statTileStyles.label}>{label}</Text>
    </View>
  );
}

const statTileStyles = StyleSheet.create({
  tile: {
    backgroundColor: bg.raised,
    borderRadius: 10,
    padding: 12,
    alignItems: 'center',
    minWidth: 80,
  },
  value: {
    fontSize: fontSize.heading,
    fontWeight: 'bold',
    color: fg.primary,
  },
  unit: {
    fontSize: fontSize.label,
    color: fg.secondary,
    marginTop: 2,
  },
  label: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    marginTop: 4,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
});

// --- SectionHeader ---

interface SectionHeaderProps {
  title: string;
  color?: string;
}

export function SectionHeader({ title, color = fg.secondary }: SectionHeaderProps) {
  return <Text style={[headerStyles.title, { color }]}>{title}</Text>;
}

const headerStyles = StyleSheet.create({
  title: {
    fontSize: fontSize.body,
    fontWeight: '600',
    marginBottom: 8,
    marginTop: 12,
    textTransform: 'uppercase',
    letterSpacing: 1.5,
  },
});

// --- ConnectionBadge ---

interface ConnectionBadgeProps {
  state: string;
  rssi?: number;
}

export function ConnectionBadge({ state, rssi }: ConnectionBadgeProps) {
  const color =
    state === 'CONNECTED'
      ? accent.green
      : state === 'DEGRADING'
        ? accent.yellow
        : state === 'DISCONNECTED'
          ? fg.tertiary
          : accent.blue;

  const label =
    state === 'CONNECTED'
      ? 'Verbunden'
      : state === 'DEGRADING'
        ? 'Schwach'
        : state === 'DISCONNECTED'
          ? 'Getrennt'
          : state === 'SCANNING'
            ? 'Suche...'
            : state === 'RECONNECTING'
              ? 'Reconnect'
              : 'Verbinde';

  return (
    <View style={badgeStyles.container}>
      <View style={[badgeStyles.dot, { backgroundColor: color }]} />
      <Text style={[badgeStyles.text, { color }]}>{label}</Text>
      {rssi !== undefined && rssi !== 0 && (
        <Text style={badgeStyles.rssi}>{rssi} dBm</Text>
      )}
    </View>
  );
}

const badgeStyles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
  },
  dot: {
    width: 8,
    height: 8,
    borderRadius: 4,
  },
  text: {
    fontSize: fontSize.label,
    fontWeight: '600',
  },
  rssi: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
  },
});

// --- EmptyState ---

interface EmptyStateProps {
  message: string;
  style?: ViewStyle;
}

export function EmptyState({ message, style }: EmptyStateProps) {
  return (
    <View style={[emptyStyles.container, style]}>
      <Text style={emptyStyles.text}>{message}</Text>
    </View>
  );
}

const emptyStyles = StyleSheet.create({
  container: {
    padding: 24,
    alignItems: 'center',
  },
  text: {
    fontSize: fontSize.body,
    color: fg.tertiary,
    textAlign: 'center',
  },
});

// --- ScreenContainer ---

interface ScreenContainerProps {
  children: React.ReactNode;
  style?: ViewStyle;
}

export function ScreenContainer({ children, style }: ScreenContainerProps) {
  return <View style={[screenStyles.container, style]}>{children}</View>;
}

const screenStyles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: bg.void,
    paddingHorizontal: 16,
    paddingTop: 16,
  },
});

// --- ScreenHeader ---

interface ScreenHeaderProps {
  title: string;
  right?: React.ReactNode;
}

export function ScreenHeader({ title, right }: ScreenHeaderProps) {
  return (
    <View style={headerBarStyles.container}>
      <Text style={headerBarStyles.title}>{title}</Text>
      {right}
    </View>
  );
}

const headerBarStyles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 16,
  },
  title: {
    fontSize: fontSize.heading,
    fontWeight: 'bold',
    color: fg.primary,
  },
});

// --- Text-Styles ---

export const textStyles = StyleSheet.create({
  heading: {
    fontSize: fontSize.heading,
    fontWeight: 'bold',
    color: fg.primary,
  } as TextStyle,
  body: {
    fontSize: fontSize.body,
    color: fg.primary,
  } as TextStyle,
  bodySecondary: {
    fontSize: fontSize.body,
    color: fg.secondary,
  } as TextStyle,
  label: {
    fontSize: fontSize.label,
    color: fg.secondary,
    textTransform: 'uppercase',
    letterSpacing: 1,
  } as TextStyle,
  tiny: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
  } as TextStyle,
  mono: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontFamily: 'monospace',
  } as TextStyle,
});

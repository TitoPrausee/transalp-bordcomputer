/**
 * Navigationspfeil-Komponente.
 *
 * Zeigt den naechsten Maneuver-Typ als Pfeil-Icon.
 * Wechselt mit Fade-Animation zwischen Typen.
 */

import React, { memo, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, Animated, Easing } from 'react-native';
import { accent, fg, fontSize } from '../core/theme/colors';
import { ManeuverType } from '../core/ble/gatt-uuids';

interface NavArrowProps {
  type: ManeuverType;
  distance: number;
  streetName: string;
}

const ARROW_SYMBOLS: Record<ManeuverType, string> = {
  [ManeuverType.Straight]: '↑',
  [ManeuverType.SlightLeft]: '↖',
  [ManeuverType.Left]: '←',
  [ManeuverType.SharpLeft]: '↖',
  [ManeuverType.SlightRight]: '↗',
  [ManeuverType.Right]: '→',
  [ManeuverType.SharpRight]: '↗',
  [ManeuverType.UTurn]: '↺',
  [ManeuverType.Arrive]: '🏁',
  [ManeuverType.Fork]: 'Y',
};

function NavArrowInner({ type, distance, streetName }: NavArrowProps) {
  const fade = useRef(new Animated.Value(0)).current;

  useEffect(() => {
    Animated.sequence([
      Animated.timing(fade, {
        toValue: 0,
        duration: 150,
        easing: Easing.in(Easing.ease),
        useNativeDriver: true,
      }),
      Animated.timing(fade, {
        toValue: 1,
        duration: 200,
        easing: Easing.out(Easing.ease),
        useNativeDriver: true,
      }),
    ]).start();
  }, [type, fade]);

  const distanceStr =
    distance >= 1000 ? `${(distance / 1000).toFixed(1)} km` : `${distance} m`;

  return (
    <View style={styles.container}>
      <Animated.View style={{ opacity: fade, alignItems: 'center' }}>
        <Text style={styles.arrow}>{ARROW_SYMBOLS[type] || '↑'}</Text>
      </Animated.View>
      <Text style={styles.distance}>{distanceStr}</Text>
      <Text style={styles.street} numberOfLines={2}>
        {streetName}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    justifyContent: 'center',
    padding: 20,
  },
  arrow: {
    fontSize: 80,
    color: accent.blue,
    fontWeight: 'bold',
  },
  distance: {
    fontSize: fontSize.navArrow,
    fontWeight: 'bold',
    color: fg.primary,
    marginTop: 8,
  },
  street: {
    fontSize: fontSize.body,
    color: fg.secondary,
    marginTop: 4,
    textAlign: 'center',
  },
});

export const NavArrow = memo(NavArrowInner);

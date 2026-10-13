/**
 * Grosses Speed-Display mit Rolling-Ziffern.
 *
 * Die Geschwindigkeit wird als grosse Zahl (72px) zentral angezeigt.
 * Bei Aenderung rollen die Ziffern nach oben (Beschleunigung) oder
 * unten (Verzoegerung).
 */

import React, { memo, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, Animated, Easing } from 'react-native';
import { bg, fg, fontSize } from '../core/theme/colors';

interface SpeedDisplayProps {
  speed: number;
  unit?: string;
}

function SpeedDisplayInner({ speed, unit = 'km/h' }: SpeedDisplayProps) {
  const displayValue = useRef(new Animated.Value(speed)).current;
  const [shown, setShown] = React.useState(speed);

  useEffect(() => {
    Animated.timing(displayValue, {
      toValue: speed,
      duration: 100,
      easing: Easing.out(Easing.ease),
      useNativeDriver: false,
    }).start();
    const id = displayValue.addListener((v) => setShown(Math.round(v.value)));
    return () => displayValue.removeListener(id);
  }, [speed, displayValue]);

  return (
    <View style={styles.container}>
      <Text style={styles.speed}>{shown}</Text>
      <Text style={styles.unit}>{unit}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    justifyContent: 'center',
  },
  speed: {
    fontSize: fontSize.speed,
    fontWeight: 'bold',
    color: fg.primary,
    fontVariant: ['tabular-nums'],
  },
  unit: {
    fontSize: fontSize.label,
    color: fg.secondary,
    marginTop: 4,
    textTransform: 'uppercase',
    letterSpacing: 2,
  },
});

export const SpeedDisplay = memo(SpeedDisplayInner);

/**
 * RPM-Balken-Komponente.
 *
 * Zeigt die Drehzahl als horizontalen Balken mit Farbverlauf
 * (gruen -> gelb -> orange -> rot). Bei >95% pulsiert rot.
 */

import React, { memo, useEffect, useRef } from 'react';
import { View, StyleSheet, Animated, Easing } from 'react-native';
import { bg, accent, fg, fontSize } from '../core/theme/colors';
import { Text } from 'react-native';

interface RpmBarProps {
  rpm: number;
  maxRpm: number;
  shiftLight?: number;
}

function RpmBarInner({ rpm, maxRpm, shiftLight }: RpmBarProps) {
  const pct = maxRpm > 0 ? Math.min(1, rpm / maxRpm) : 0;
  const isRedline = pct > 0.95;
  const isShift = shiftLight ? pct > shiftLight / maxRpm : false;

  const pulse = useRef(new Animated.Value(1)).current;

  useEffect(() => {
    if (isRedline) {
      const anim = Animated.loop(
        Animated.sequence([
          Animated.timing(pulse, {
            toValue: 0.3,
            duration: 500,
            useNativeDriver: true,
            easing: Easing.inOut(Easing.ease),
          }),
          Animated.timing(pulse, {
            toValue: 1,
            duration: 500,
            useNativeDriver: true,
            easing: Easing.inOut(Easing.ease),
          }),
        ]),
      );
      anim.start();
      return () => anim.stop();
    }
    pulse.setValue(1);
  }, [isRedline, pulse]);

  const barColor = isRedline ? accent.red : isShift ? accent.yellow : rpmColor(pct);

  function rpmColor(p: number): string {
    if (p < 0.4) return accent.green;
    if (p < 0.7) return accent.yellow;
    if (p < 0.85) return accent.orange;
    return accent.red;
  }

  return (
    <View style={styles.container}>
      <View style={styles.barBg}>
        <Animated.View
          style={[
            styles.barFill,
            {
              width: `${pct * 100}%`,
              backgroundColor: barColor,
              opacity: isRedline ? pulse : 1,
            },
          ]}
        />
        {shiftLight && (
          <View
            style={[
              styles.marker,
              { left: `${(shiftLight / maxRpm) * 100}%`, backgroundColor: accent.yellow },
            ]}
          />
        )}
        <View
          style={[
            styles.marker,
            { left: '95%', backgroundColor: accent.red },
          ]}
        />
      </View>
      <Text style={styles.rpmText}>{Math.round(rpm)} rpm</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  barBg: {
    flex: 1,
    height: 24,
    backgroundColor: bg.deep,
    borderRadius: 4,
    overflow: 'hidden',
    position: 'relative',
  },
  barFill: {
    height: '100%',
    borderRadius: 4,
  },
  marker: {
    position: 'absolute',
    top: 0,
    bottom: 0,
    width: 2,
  },
  rpmText: {
    fontSize: fontSize.label,
    color: fg.secondary,
    minWidth: 70,
    textAlign: 'right',
  },
});

export const RpmBar = memo(RpmBarInner);

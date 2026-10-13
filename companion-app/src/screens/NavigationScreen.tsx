/**
 * Navigation-Screen.
 *
 * Zeigt Karten-Platzhalter + Maneuver-Anzeige.
 * Bei aktiver Navigation werden naechstes Maneuver, Route-Info
 * und Strassenname angezeigt.
 */

import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { ScreenContainer, ScreenHeader, Card, ConnectionBadge, EmptyState } from '../components/UI';
import { NavArrow } from '../components/NavArrow';
import { useAppStore } from '../core/state/store';
import { bg, fg, accent, fontSize } from '../core/theme/colors';
import { NavStatus } from '../core/ble/gatt-uuids';

const STATUS_LABELS: Record<number, string> = {
  0: 'Inaktiv',
  1: 'Aktiv',
  2: 'Neuberechnung',
  3: 'Ankunft',
  4: 'Offline',
};

export function NavigationScreen() {
  const navManeuver = useAppStore((s) => s.navManeuver);
  const navRoute = useAppStore((s) => s.navRoute);
  const navRoad = useAppStore((s) => s.navRoad);
  const navStatus = useAppStore((s) => s.navStatus);
  const ble = useAppStore((s) => s.ble);

  const hasNav = navManeuver !== null;

  return (
    <ScreenContainer>
      <ScrollView showsVerticalScrollIndicator={false}>
        <ScreenHeader
          title="NAVIGATION"
          right={<ConnectionBadge state={ble.state} />}
        />

        {/* Karten-Platzhalter */}
        <View style={styles.mapPlaceholder}>
          <Text style={styles.mapText}>Karten-Platzhalter</Text>
          <Text style={styles.mapSubtext}>
            Native Karten-API wird zur Laufzeit geladen.
          </Text>
        </View>

        {/* Maneuver-Anzeige */}
        {hasNav ? (
          <Card>
            <NavArrow
              type={navManeuver!.type}
              distance={navManeuver!.distance}
              streetName={navManeuver!.streetName}
            />

            {/* Route-Info */}
            {navRoute && (
              <View style={styles.routeInfo}>
                <View style={styles.routeCell}>
                  <Text style={styles.routeLabel}>REST</Text>
                  <Text style={styles.routeValue}>
                    {(navRoute.remainingDistance / 1000).toFixed(1)} km
                  </Text>
                </View>
                <View style={styles.routeCell}>
                  <Text style={styles.routeLabel}>ETA</Text>
                  <Text style={styles.routeValue}>
                    {new Date(navRoute.eta * 1000).toLocaleTimeString('de-DE', {
                      hour: '2-digit',
                      minute: '2-digit',
                    })}
                  </Text>
                </View>
                <View style={styles.routeCell}>
                  <Text style={styles.routeLabel}>NOCH</Text>
                  <Text style={styles.routeValue}>
                    {Math.floor(navRoute.remainingTime / 60)} min
                  </Text>
                </View>
              </View>
            )}

            {/* Current Road */}
            {navRoad && (
              <View style={styles.roadInfo}>
                <Text style={styles.roadLabel}>AKTUELLE STRASSE</Text>
                <Text style={styles.roadName}>{navRoad.roadName}</Text>
                {navRoad.speedLimit > 0 && (
                  <View style={styles.speedLimit}>
                    <Text style={styles.speedLimitText}>{navRoad.speedLimit}</Text>
                  </View>
                )}
              </View>
            )}

            {/* Nav-Status */}
            <View style={styles.statusRow}>
              <Text style={styles.statusLabel}>STATUS</Text>
              <Text style={[styles.statusValue, { color: navStatus === 1 ? accent.green : accent.yellow }]}>
                {STATUS_LABELS[navStatus] ?? 'Unbekannt'}
              </Text>
            </View>
          </Card>
        ) : (
          <EmptyState message="Keine aktive Navigation. Starte eine Route auf dem Geraet." />
        )}
      </ScrollView>
    </ScreenContainer>
  );
}

const styles = StyleSheet.create({
  mapPlaceholder: {
    height: 200,
    backgroundColor: bg.deep,
    borderRadius: 12,
    justifyContent: 'center',
    alignItems: 'center',
    marginBottom: 16,
    borderWidth: 1,
    borderColor: bg.raised,
  },
  mapText: {
    fontSize: fontSize.body,
    color: fg.tertiary,
    fontWeight: '600',
  },
  mapSubtext: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    marginTop: 4,
  },
  routeInfo: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginTop: 16,
    paddingTop: 16,
    borderTopWidth: 1,
    borderTopColor: bg.raised,
  },
  routeCell: {
    alignItems: 'center',
  },
  routeLabel: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  routeValue: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: 'bold',
    marginTop: 2,
  },
  roadInfo: {
    marginTop: 16,
    paddingTop: 16,
    borderTopWidth: 1,
    borderTopColor: bg.raised,
    alignItems: 'center',
  },
  roadLabel: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  roadName: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: '600',
    marginTop: 4,
  },
  speedLimit: {
    marginTop: 8,
    width: 40,
    height: 40,
    borderRadius: 20,
    borderWidth: 3,
    borderColor: accent.red,
    justifyContent: 'center',
    alignItems: 'center',
  },
  speedLimitText: {
    fontSize: fontSize.body,
    fontWeight: 'bold',
    color: fg.primary,
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 16,
    paddingTop: 16,
    borderTopWidth: 1,
    borderTopColor: bg.raised,
  },
  statusLabel: {
    fontSize: fontSize.label,
    color: fg.secondary,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  statusValue: {
    fontSize: fontSize.body,
    fontWeight: '600',
  },
});

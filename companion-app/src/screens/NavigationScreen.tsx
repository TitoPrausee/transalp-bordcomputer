/**
 * Navigation-Screen.
 *
 * Zeigt Turn-by-Turn Navigation mit:
 *  - Naechste Anweisung (grosses Symbol + Text)
 *  - Entfernung zur naechsten Anweisung
 *  - Ankunftszeit und Reststrecke
 *  - Aktuelle Strasse
 *  - Geschwindigkeitslimit
 *  - GPS-Status (Satellitenanzahl, Fix-Qualitaet)
 *  - Route-Preview (vereinfachte Darstellung)
 *  - Kartenansicht (POC mit statischer Karte)
 *  - Sprachausgabe-Trigger (via TTS-API des Smartphones)
 */

import React, { useState, useEffect, useCallback, useRef } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Platform,
  Linking,
} from 'react-native';
import { ScreenContainer, ScreenHeader, Card, ConnectionBadge, EmptyState } from '../components/UI';
import { NavArrow } from '../components/NavArrow';
import { useAppStore } from '../core/state/store';
import { useBleManager } from '../hooks/useBleManager';
import { bg, fg, accent, fontSize } from '../core/theme/colors';
import { ManeuverType, NavStatus } from '../core/ble/gatt-uuids';

// --- Status Labels ---
const STATUS_LABELS: Record<number, string> = {
  0: 'Inaktiv',
  1: 'Aktiv',
  2: 'Neuberechnung',
  3: 'Ankunft',
  4: 'Offline',
};

// --- Maneuver Texte ---
const MANEUVER_TEXTS: Record<ManeuverType, string> = {
  [ManeuverType.Straight]: 'Geradeaus weiterfahren',
  [ManeuverType.SlightLeft]: 'Leicht links halten',
  [ManeuverType.Left]: 'Links abbiegen',
  [ManeuverType.SharpLeft]: 'Scharf links abbiegen',
  [ManeuverType.SlightRight]: 'Leicht rechts halten',
  [ManeuverType.Right]: 'Rechts abbiegen',
  [ManeuverType.SharpRight]: 'Scharf rechts abbiegen',
  [ManeuverType.UTurn]: 'Wenden',
  [ManeuverType.Arrive]: 'Ziel erreicht',
  [ManeuverType.Fork]: 'An Abzweigung halten',
};

// --- GPS Fix Quality ---

function getGpsFixQuality(sats: number, hdop: number): { label: string; color: string } {
  if (sats === 0) return { label: 'Kein Fix', color: accent.red };
  if (sats < 4) return { label: '2D Fix', color: accent.yellow };
  if (sats < 6 || hdop > 20) return { label: '3D Fix (schwach)', color: accent.yellow };
  if (sats >= 8 && hdop < 10) return { label: '3D Fix (sehr gut)', color: accent.green };
  return { label: '3D Fix', color: accent.green };
}

// --- Route Preview (vereinfacht) ---

interface RouteStep {
  id: number;
  maneuver: ManeuverType;
  street: string;
  distance: string;
  isNext: boolean;
}

function RoutePreview({ steps }: { steps: RouteStep[] }) {
  return (
    <View style={routeStyles.container}>
      <Text style={routeStyles.title}>ROUTENVORSCHAU</Text>
      {steps.length === 0 ? (
        <Text style={routeStyles.empty}>Keine Routendaten verfuegbar</Text>
      ) : (
        steps.map((step) => {
          const arrow = getManeuverSymbol(step.maneuver);
          return (
            <View
              key={step.id}
              style={[
                routeStyles.step,
                step.isNext && routeStyles.stepActive,
              ]}
            >
              <Text style={[routeStyles.arrow, { color: step.isNext ? accent.blue : fg.secondary }]}>
                {arrow}
              </Text>
              <View style={routeStyles.stepInfo}>
                <Text style={routeStyles.stepStreet} numberOfLines={1}>
                  {step.street}
                </Text>
                <Text style={routeStyles.stepDistance}>{step.distance}</Text>
              </View>
            </View>
          );
        })
      )}
    </View>
  );
}

const routeStyles = StyleSheet.create({
  container: {
    marginVertical: 4,
  },
  title: {
    fontSize: fontSize.label,
    color: fg.secondary,
    textTransform: 'uppercase',
    letterSpacing: 1.5,
    marginBottom: 8,
  },
  empty: {
    fontSize: fontSize.body,
    color: fg.tertiary,
    fontStyle: 'italic',
  },
  step: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
    paddingVertical: 10,
    borderBottomWidth: 1,
    borderBottomColor: bg.raised,
  },
  stepActive: {
    backgroundColor: bg.raised,
    borderRadius: 8,
    paddingHorizontal: 8,
    marginHorizontal: -8,
  },
  arrow: {
    fontSize: 28,
    fontWeight: 'bold',
    width: 36,
    textAlign: 'center',
  },
  stepInfo: {
    flex: 1,
  },
  stepStreet: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: '500',
  },
  stepDistance: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    marginTop: 2,
  },
});

function getManeuverSymbol(type: ManeuverType): string {
  const symbols: Record<ManeuverType, string> = {
    [ManeuverType.Straight]: '↑',
    [ManeuverType.SlightLeft]: '↖',
    [ManeuverType.Left]: '←',
    [ManeuverType.SharpLeft]: '↖',
    [ManeuverType.SlightRight]: '↗',
    [ManeuverType.Right]: '→',
    [ManeuverType.SharpRight]: '↗',
    [ManeuverType.UTurn]: '↺',
    [ManeuverType.Arrive]: '◎',
    [ManeuverType.Fork]: 'Y',
  };
  return symbols[type] ?? '↑';
}

// --- Speed Limit Sign ---

function SpeedLimitSign({ limit }: { limit: number }) {
  if (limit <= 0) return null;
  return (
    <View style={speedLimitStyles.container}>
      <View style={speedLimitStyles.circle}>
        <Text style={speedLimitStyles.text}>{limit}</Text>
      </View>
      <Text style={speedLimitStyles.label}>km/h</Text>
    </View>
  );
}

const speedLimitStyles = StyleSheet.create({
  container: {
    alignItems: 'center',
    marginVertical: 4,
  },
  circle: {
    width: 56,
    height: 56,
    borderRadius: 28,
    borderWidth: 4,
    borderColor: accent.red,
    backgroundColor: '#FFFFFF',
    justifyContent: 'center',
    alignItems: 'center',
  },
  text: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#000000',
  },
  label: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    marginTop: 2,
  },
});

// --- TTS (Text-to-Speech) ---

async function speakInstruction(text: string): Promise<void> {
  try {
    // TTS via expo-speech (wird zur Laufzeit geladen falls verfuegbar)
    if (Platform.OS === 'ios' || Platform.OS === 'android') {
      // eslint-disable-next-line @typescript-eslint/no-explicit-any
      const Speech = (globalThis as any)?.expoSpeech ?? null;
      if (Speech && typeof Speech.speak === 'function') {
        Speech.speak(text, {
          language: 'de-DE',
          pitch: 1.0,
          rate: 0.95,
        });
      }
    }
  } catch {
    // TTS nicht verfuegbar - silently ignore
  }
}

// --- Main Navigation Screen ---

export function NavigationScreen() {
  const navManeuver = useAppStore((s) => s.navManeuver);
  const navRoute = useAppStore((s) => s.navRoute);
  const navRoad = useAppStore((s) => s.navRoad);
  const navStatus = useAppStore((s) => s.navStatus);
  const telemetry = useAppStore((s) => s.telemetry);
  const ble = useAppStore((s) => s.ble);

  const hasNav = navManeuver !== null;
  const lastManeuverType = useRef<ManeuverType | null>(null);
  const [ttsEnabled, setTtsEnabled] = useState(true);

  // TTS bei Maneuver-Wechsel
  useEffect(() => {
    if (!hasNav || !ttsEnabled) return;
    const maneuver = navManeuver!;
    if (lastManeuverType.current !== maneuver.type) {
      lastManeuverType.current = maneuver.type;
      const text = MANEUVER_TEXTS[maneuver.type] ?? 'Weiterfahren';
      const distStr =
        maneuver.distance >= 1000
          ? `${(maneuver.distance / 1000).toFixed(1)} Kilometer`
          : `${maneuver.distance} Meter`;
      speakInstruction(`${distStr} ${text}`);
    }
  }, [navManeuver, hasNav, ttsEnabled]);

  // GPS-Status
  const gpsFix = getGpsFixQuality(telemetry.satellites, telemetry.hdop);

  // Route-Preview aus Mock-Daten generieren
  const routeSteps: RouteStep[] = hasNav
    ? [
        {
          id: 0,
          maneuver: navManeuver!.type,
          street: navManeuver!.streetName,
          distance:
            navManeuver!.distance >= 1000
              ? `${(navManeuver!.distance / 1000).toFixed(1)} km`
              : `${navManeuver!.distance} m`,
          isNext: true,
        },
        {
          id: 1,
          maneuver: ManeuverType.Straight,
          street: navRoad?.roadName ?? 'Weiterfahren',
          distance: '1.2 km',
          isNext: false,
        },
        {
          id: 2,
          maneuver: ManeuverType.Right,
          street: 'Abzweigung',
          distance: '2.5 km',
          isNext: false,
        },
      ]
    : [];

  const formatEta = (eta: number): string => {
    const date = new Date(eta * 1000);
    return date.toLocaleTimeString('de-DE', { hour: '2-digit', minute: '2-digit' });
  };

  const formatDuration = (seconds: number): string => {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    if (h > 0) return `${h}h ${m}min`;
    return `${m} min`;
  };

  return (
    <ScreenContainer>
      <ScrollView showsVerticalScrollIndicator={false}>
        <ScreenHeader
          title="NAVIGATION"
          right={
            <View style={styles.headerRight}>
              <TouchableOpacity onPress={() => setTtsEnabled(!ttsEnabled)}>
                <Text style={[styles.ttsToggle, { color: ttsEnabled ? accent.green : fg.tertiary }]}>
                  {ttsEnabled ? '🔊' : '🔇'}
                </Text>
              </TouchableOpacity>
              <ConnectionBadge state={ble.state} />
            </View>
          }
        />

        {/* GPS-Status */}
        <Card>
          <View style={styles.gpsRow}>
            <View>
              <Text style={styles.gpsLabel}>GPS-STATUS</Text>
              <Text style={[styles.gpsFix, { color: gpsFix.color }]}>{gpsFix.label}</Text>
            </View>
            <View style={styles.gpsRight}>
              <View style={styles.gpsStat}>
                <Text style={styles.gpsStatValue}>{telemetry.satellites}</Text>
                <Text style={styles.gpsStatLabel}>SAT</Text>
              </View>
              <View style={styles.gpsStat}>
                <Text style={styles.gpsStatValue}>{(telemetry.hdop / 10).toFixed(1)}</Text>
                <Text style={styles.gpsStatLabel}>HDOP</Text>
              </View>
              <View style={styles.gpsStat}>
                <Text style={styles.gpsStatValue}>{telemetry.altitude}</Text>
                <Text style={styles.gpsStatLabel}>m</Text>
              </View>
            </View>
          </View>
        </Card>

        {/* Karten-Platzhalter */}
        <View style={styles.mapPlaceholder}>
          <Text style={styles.mapText}>KARTEN-ANSICHT</Text>
          <Text style={styles.mapSubtext}>
            Native Karten-API wird zur Laufzeit geladen.
          </Text>
          {hasNav && telemetry.latitude !== 0 && (
            <Text style={styles.mapCoords}>
              {`${(telemetry.latitude / 1e7).toFixed(5)}, ${(telemetry.longitude / 1e7).toFixed(5)}`}
            </Text>
          )}
        </View>

        {/* Maneuver-Anzeige */}
        {hasNav ? (
          <>
            <Card>
              <NavArrow
                type={navManeuver!.type}
                distance={navManeuver!.distance}
                streetName={navManeuver!.streetName}
              />

              {/* Maneuver Text */}
              <Text style={styles.maneuverText}>
                {MANEUVER_TEXTS[navManeuver!.type] ?? 'Weiterfahren'}
              </Text>

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
                      {formatEta(navRoute.eta)}
                    </Text>
                  </View>
                  <View style={styles.routeCell}>
                    <Text style={styles.routeLabel}>NOCH</Text>
                    <Text style={styles.routeValue}>
                      {formatDuration(navRoute.remainingTime)}
                    </Text>
                  </View>
                  <View style={styles.routeCell}>
                    <Text style={styles.routeLabel}>GESAMT</Text>
                    <Text style={styles.routeValue}>
                      {(navRoute.totalDistance / 1000).toFixed(0)} km
                    </Text>
                  </View>
                </View>
              )}

              {/* Current Road + Speed Limit */}
              {navRoad && (
                <View style={styles.roadInfo}>
                  <View style={styles.roadLeft}>
                    <Text style={styles.roadLabel}>AKTUELLE STRASSE</Text>
                    <Text style={styles.roadName}>{navRoad.roadName}</Text>
                  </View>
                  <SpeedLimitSign limit={navRoad.speedLimit} />
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

            {/* Route Preview */}
            <Card>
              <RoutePreview steps={routeSteps} />
            </Card>

            {/* Current Speed Display */}
            <Card>
              <View style={styles.speedRow}>
                <Text style={styles.roadLabel}>AKTUELLE GESCHWINDIGKEIT</Text>
                <Text style={styles.currentSpeed}>{telemetry.speed}</Text>
                <Text style={styles.speedUnit}>km/h</Text>
              </View>
            </Card>
          </>
        ) : (
          <EmptyState message="Keine aktive Navigation. Starte eine Route auf dem Geraet." />
        )}
      </ScrollView>
    </ScreenContainer>
  );
}

const styles = StyleSheet.create({
  headerRight: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
  },
  ttsToggle: {
    fontSize: 24,
  },
  gpsRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  gpsLabel: {
    fontSize: fontSize.label,
    color: fg.secondary,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  gpsFix: {
    fontSize: fontSize.body,
    fontWeight: '600',
    marginTop: 2,
  },
  gpsRight: {
    flexDirection: 'row',
    gap: 16,
  },
  gpsStat: {
    alignItems: 'center',
  },
  gpsStatValue: {
    fontSize: fontSize.heading,
    fontWeight: 'bold',
    color: fg.primary,
  },
  gpsStatLabel: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    textTransform: 'uppercase',
  },
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
  mapCoords: {
    fontSize: fontSize.tiny,
    color: accent.blue,
    marginTop: 8,
    fontFamily: 'monospace',
  },
  maneuverText: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: '600',
    textAlign: 'center',
    marginTop: 8,
  },
  routeInfo: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginTop: 16,
    paddingTop: 16,
    borderTopWidth: 1,
    borderTopColor: bg.raised,
    flexWrap: 'wrap',
    gap: 8,
  },
  routeCell: {
    alignItems: 'center',
    minWidth: 70,
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
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginTop: 16,
    paddingTop: 16,
    borderTopWidth: 1,
    borderTopColor: bg.raised,
  },
  roadLeft: {
    flex: 1,
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
  speedRow: {
    flexDirection: 'row',
    alignItems: 'baseline',
    justifyContent: 'center',
    gap: 4,
  },
  currentSpeed: {
    fontSize: fontSize.speed,
    fontWeight: 'bold',
    color: fg.primary,
    fontVariant: ['tabular-nums'],
  },
  speedUnit: {
    fontSize: fontSize.body,
    color: fg.secondary,
  },
});

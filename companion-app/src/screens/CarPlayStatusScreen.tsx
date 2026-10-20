/**
 * CarPlay-Status-Screen.
 *
 * Zeigt den Verbindungsstatus des CarPlay-Displays und die aktuell
 * aktive CarPlay-App (Navigation/Musik/Telefon/Podcasts).
 *
 * Features:
 *  - Verbindungsstatus CarPlay-Display (connected/disconnected)
 *  - Aktive CarPlay-App (Navigation/Musik/Telefon/Podcasts)
 *  - Musik-Metadaten anzeigen (Artist, Titel, Album)
 *  - Steuerung: Next/Prev Track, Play/Pause, Pair/Unpair
 */

import React, { useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
} from 'react-native';
import { ScreenContainer, ScreenHeader, Card, EmptyState } from '../components/UI';
import { useAppStore } from '../core/state/store';
import { useBleManager } from '../hooks/useBleManager';
import { bg, fg, accent, fontSize } from '../core/theme/colors';
import { CarPlayApp, CarPlayControl } from '../core/ble/gatt-uuids';

// --- App-Labels ---

const APP_LABELS: Record<CarPlayApp, string> = {
  [CarPlayApp.None]: 'Keine',
  [CarPlayApp.Navigation]: 'Navigation',
  [CarPlayApp.Music]: 'Musik',
  [CarPlayApp.Phone]: 'Telefon',
  [CarPlayApp.Podcasts]: 'Podcasts',
};

const APP_COLORS: Record<CarPlayApp, string> = {
  [CarPlayApp.None]: fg.tertiary,
  [CarPlayApp.Navigation]: accent.blue,
  [CarPlayApp.Music]: accent.cyan,
  [CarPlayApp.Phone]: accent.green,
  [CarPlayApp.Podcasts]: accent.amber,
};

// --- Connection-Status-Badge ---

function CarPlayConnectionBadge({ connected }: { connected: boolean }) {
  const color = connected ? accent.green : fg.tertiary;
  const label = connected ? 'Verbunden' : 'Getrennt';
  return (
    <View style={badgeStyles.container}>
      <View style={[badgeStyles.dot, { backgroundColor: color }]} />
      <Text style={[badgeStyles.text, { color }]}>{label}</Text>
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
});

// --- Music-Metadata-Card ---

function MusicMetadataCard({
  artist,
  title,
  album,
}: {
  artist: string;
  title: string;
  album: string;
}) {
  const hasData = artist || title || album;
  if (!hasData) {
    return (
      <Card>
        <Text style={styles.sectionTitle}>MUSIK</Text>
        <EmptyState message="Keine Musik-Metadaten verfuegbar." />
      </Card>
    );
  }

  return (
    <Card>
      <Text style={styles.sectionTitle}>MUSIK</Text>
      <View style={musicStyles.trackContainer}>
        <View style={musicStyles.iconContainer}>
          <Text style={musicStyles.icon}>{'\u266B'}</Text>
        </View>
        <View style={musicStyles.infoContainer}>
          <Text style={musicStyles.title} numberOfLines={1}>{title || 'Unbekannt'}</Text>
          <Text style={musicStyles.artist} numberOfLines={1}>{artist || 'Unbekannt'}</Text>
          <Text style={musicStyles.album} numberOfLines={1}>{album || 'Unbekannt'}</Text>
        </View>
      </View>
    </Card>
  );
}

const musicStyles = StyleSheet.create({
  trackContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 16,
  },
  iconContainer: {
    width: 56,
    height: 56,
    borderRadius: 12,
    backgroundColor: bg.raised,
    justifyContent: 'center',
    alignItems: 'center',
  },
  icon: {
    fontSize: 28,
    color: accent.cyan,
  },
  infoContainer: {
    flex: 1,
  },
  title: {
    fontSize: fontSize.heading,
    fontWeight: 'bold',
    color: fg.primary,
  },
  artist: {
    fontSize: fontSize.body,
    color: fg.secondary,
    marginTop: 2,
  },
  album: {
    fontSize: fontSize.label,
    color: fg.tertiary,
    marginTop: 2,
  },
});

// --- Control-Button ---

function ControlButton({
  label,
  onPress,
  color = accent.blue,
  disabled = false,
}: {
  label: string;
  onPress: () => void;
  color?: string;
  disabled?: boolean;
}) {
  return (
    <TouchableOpacity
      style={[
        controlStyles.btn,
        { backgroundColor: color, opacity: disabled ? 0.4 : 1 },
      ]}
      onPress={onPress}
      disabled={disabled}
    >
      <Text style={controlStyles.btnText}>{label}</Text>
    </TouchableOpacity>
  );
}

const controlStyles = StyleSheet.create({
  btn: {
    flex: 1,
    padding: 14,
    borderRadius: 10,
    alignItems: 'center',
    justifyContent: 'center',
  },
  btnText: {
    fontSize: fontSize.label,
    fontWeight: 'bold',
    color: '#FFFFFF',
  },
});

// --- Main Screen ---

export function CarPlayStatusScreen() {
  const carplay = useAppStore((s) => s.carplay);
  const config = useAppStore((s) => s.config);
  const ble = useAppStore((s) => s.ble);
  const { readCarPlayStatus, sendCarPlayControl } = useBleManager();

  useEffect(() => {
    readCarPlayStatus();
  }, [readCarPlayStatus]);

  const handleNext = () => sendCarPlayControl(CarPlayControl.NextTrack);
  const handlePrev = () => sendCarPlayControl(CarPlayControl.PrevTrack);
  const handlePlayPause = () => sendCarPlayControl(CarPlayControl.PlayPause);
  const handlePair = () => sendCarPlayControl(CarPlayControl.Pair);
  const handleUnpair = () => sendCarPlayControl(CarPlayControl.Unpair);

  const bleConnected = ble.state === 'CONNECTED' || ble.state === 'DEGRADING';

  const appLabel = APP_LABELS[carplay.activeApp] ?? 'Unbekannt';
  const appColor = APP_COLORS[carplay.activeApp] ?? fg.primary;
  const isMusicActive = carplay.activeApp === CarPlayApp.Music;

  return (
    <ScreenContainer>
      <ScrollView showsVerticalScrollIndicator={false}>
        <ScreenHeader
          title="CARPLAY"
          right={<CarPlayConnectionBadge connected={carplay.connected} />}
        />

        {/* Verbindungsstatus */}
        <Card>
          <Text style={styles.sectionTitle}>VERBINDUNGSSTATUS</Text>
          <View style={styles.row}>
            <Text style={styles.label}>CarPlay-Display</Text>
            <Text
              style={[
                styles.value,
                { color: carplay.connected ? accent.green : fg.tertiary },
              ]}
            >
              {carplay.connected ? 'Verbunden' : 'Getrennt'}
            </Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Display-Name</Text>
            <Text style={styles.value}>{config.carplayDisplayName || 'Transalp-BC'}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>CarPlay aktiviert</Text>
            <Text
              style={[
                styles.value,
                { color: config.carplayEnabled ? accent.green : accent.red },
              ]}
            >
              {config.carplayEnabled ? 'Ja' : 'Nein'}
            </Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.label}>Gepaart</Text>
            <Text
              style={[
                styles.value,
                { color: config.carplayPaired ? accent.green : fg.tertiary },
              ]}
            >
              {config.carplayPaired ? 'Ja' : 'Nein'}
            </Text>
          </View>
        </Card>

        {/* Aktive App */}
        <Card>
          <Text style={styles.sectionTitle}>AKTIVE APP</Text>
          <View style={activeAppStyles.container}>
            <View
              style={[
                activeAppStyles.badge,
                { backgroundColor: appColor },
              ]}
            >
              <Text style={activeAppStyles.badgeText}>{appLabel}</Text>
            </View>
          </View>
        </Card>

        {/* Musik-Metadaten (nur wenn verbunden und Musik aktiv) */}
        {carplay.connected && isMusicActive && (
          <MusicMetadataCard
            artist={carplay.artist}
            title={carplay.title}
            album={carplay.album}
          />
        )}

        {/* Musik-Metadaten-Leerkarte wenn verbunden aber keine Musik */}
        {carplay.connected && !isMusicActive && (
          <Card>
            <Text style={styles.sectionTitle}>MUSIK</Text>
            <EmptyState message="Keine Musik-App aktiv. Wechseln Sie zu Musik auf dem CarPlay-Display." />
          </Card>
        )}

        {/* Steuerung */}
        <Card>
          <Text style={styles.sectionTitle}>STEUERUNG</Text>
          <View style={controlRowStyles.row}>
            <ControlButton
              label={'\u23EE'}
              onPress={handlePrev}
              disabled={!carplay.connected}
              color={bg.raised}
            />
            <ControlButton
              label={'\u23EF'}
              onPress={handlePlayPause}
              disabled={!carplay.connected}
              color={accent.blue}
            />
            <ControlButton
              label={'\u23ED'}
              onPress={handleNext}
              disabled={!carplay.connected}
              color={bg.raised}
            />
          </View>
          <View style={controlRowStyles.row}>
            {carplay.connected ? (
              <ControlButton
                label="TRENNEN"
                onPress={handleUnpair}
                color={accent.red}
              />
            ) : (
              <ControlButton
                label="KOPPELN"
                onPress={handlePair}
                color={accent.green}
                disabled={!bleConnected}
              />
            )}
          </View>
        </Card>

        {/* Hinweis wenn Bordcomputer nicht verbunden */}
        {!bleConnected && (
          <Card>
            <EmptyState message="Keine Verbindung zum Bordcomputer. CarPlay-Status nicht verfuegbar." />
          </Card>
        )}
      </ScrollView>
    </ScreenContainer>
  );
}

const styles = StyleSheet.create({
  sectionTitle: {
    fontSize: fontSize.body,
    fontWeight: '600',
    color: fg.secondary,
    textTransform: 'uppercase',
    letterSpacing: 1.5,
    marginBottom: 12,
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 6,
  },
  label: {
    fontSize: fontSize.body,
    color: fg.secondary,
  },
  value: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: '500',
  },
});

const controlRowStyles = StyleSheet.create({
  row: {
    flexDirection: 'row',
    gap: 8,
    marginVertical: 4,
  },
});

const activeAppStyles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  badge: {
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 8,
  },
  badgeText: {
    fontSize: fontSize.body,
    fontWeight: 'bold',
    color: '#FFFFFF',
  },
});

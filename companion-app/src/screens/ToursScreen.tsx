/**
 * Tours-Screen.
 *
 * Zeigt die Tour-Historie (Mock-Daten) mit Statistiken.
 * Tap auf Tour -> Detail-Ansicht.
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity } from 'react-native';
import { ScreenContainer, ScreenHeader, Card, EmptyState } from '../components/UI';
import { useAppStore, type TourSummary } from '../core/state/store';
import { bg, fg, accent, fontSize } from '../core/theme/colors';

export function ToursScreen() {
  const tours = useAppStore((s) => s.tours);
  const [selected, setSelected] = useState<TourSummary | null>(null);

  if (selected) {
    return (
      <TourDetail
        tour={selected}
        onBack={() => setSelected(null)}
      />
    );
  }

  return (
    <ScreenContainer>
      <ScrollView showsVerticalScrollIndicator={false}>
        <ScreenHeader title="TOUREN" />

        {tours.length === 0 ? (
          <EmptyState message="Noch keine Touren aufgezeichnet." />
        ) : (
          <>
            {/* Gesamt-Uebersicht */}
            <Card>
              <View style={styles.summaryRow}>
                <View style={styles.summaryCell}>
                  <Text style={styles.summaryValue}>
                    {tours.reduce((sum, t) => sum + t.distanceKm, 0)}
                  </Text>
                  <Text style={styles.summaryLabel}>KM GESAMT</Text>
                </View>
                <View style={styles.summaryCell}>
                  <Text style={styles.summaryValue}>{tours.length}</Text>
                  <Text style={styles.summaryLabel}>TOUREN</Text>
                </View>
              </View>
            </Card>

            {/* Tour-Liste */}
            {tours.map((tour) => (
              <TourListItem key={tour.id} tour={tour} onPress={() => setSelected(tour)} />
            ))}
          </>
        )}
      </ScrollView>
    </ScreenContainer>
  );
}

function TourListItem({ tour, onPress }: { tour: TourSummary; onPress: () => void }) {
  const date = new Date(tour.date);
  const dateStr = date.toLocaleDateString('de-DE', {
    day: '2-digit',
    month: 'short',
    year: 'numeric',
  });
  const durH = Math.floor(tour.durationMin / 60);
  const durM = tour.durationMin % 60;

  return (
    <TouchableOpacity onPress={onPress}>
      <Card>
        <View style={styles.tourRow}>
          <View style={styles.tourLeft}>
            <Text style={styles.tourDate}>{dateStr}</Text>
            <Text style={styles.tourDist}>{tour.distanceKm} km</Text>
          </View>
          <View style={styles.tourRight}>
            <Text style={styles.tourStat}>{durH}h {durM}min</Text>
            <Text style={styles.tourSub}>max {tour.maxSpeed} km/h</Text>
          </View>
        </View>
      </Card>
    </TouchableOpacity>
  );
}

function TourDetail({ tour, onBack }: { tour: TourSummary; onBack: () => void }) {
  const date = new Date(tour.date);
  const dateStr = date.toLocaleDateString('de-DE', {
    day: '2-digit',
    month: 'long',
    year: 'numeric',
  });
  const durH = Math.floor(tour.durationMin / 60);
  const durM = tour.durationMin % 60;

  return (
    <ScreenContainer>
      <ScrollView showsVerticalScrollIndicator={false}>
        <View style={styles.detailHeader}>
          <TouchableOpacity onPress={onBack}>
            <Text style={styles.backBtn}>‹ Zurueck</Text>
          </TouchableOpacity>
          <Text style={styles.detailTitle}>TOUR-STATISTIK</Text>
        </View>

        {/* Hero */}
        <Card>
          <View style={styles.heroRow}>
            <View style={styles.heroCell}>
              <Text style={styles.heroValue}>{tour.distanceKm}</Text>
              <Text style={styles.heroLabel}>KM</Text>
            </View>
            <View style={styles.heroCell}>
              <Text style={styles.heroValue}>{durH}h {durM}min</Text>
              <Text style={styles.heroLabel}>DAUER</Text>
            </View>
          </View>
          <Text style={styles.detailDate}>{dateStr}</Text>
        </Card>

        {/* Statistiken */}
        <Card>
          <View style={styles.statRow}>
            <Text style={styles.statLabel}>Durchschnitt</Text>
            <Text style={styles.statValue}>{tour.avgSpeed} km/h</Text>
          </View>
          <View style={styles.statRow}>
            <Text style={styles.statLabel}>Max-Speed</Text>
            <Text style={styles.statValue}>{tour.maxSpeed} km/h</Text>
          </View>
          <View style={styles.statRow}>
            <Text style={styles.statLabel}>Max-RPM</Text>
            <Text style={styles.statValue}>{tour.maxRpm} rpm</Text>
          </View>
        </Card>

        {/* Hoehe (Mock) */}
        <Card>
          <Text style={styles.sectionTitle}>HOEHENPROFIL</Text>
          <View style={styles.altBars}>
            {[412, 680, 950, 1247, 1100, 820, 600, 450].map((h, i) => {
              const maxH = 1247;
              const pct = (h / maxH) * 100;
              return (
                <View key={i} style={styles.altBarContainer}>
                  <View style={[styles.altBar, { height: `${pct}%`, backgroundColor: accent.cyan }]} />
                  <Text style={styles.altBarLabel}>{h}</Text>
                </View>
              );
            })}
          </View>
        </Card>

        {/* Export */}
        <TouchableOpacity style={styles.exportBtn}>
          <Text style={styles.exportBtnText}>GPX EXPORTIEREN</Text>
        </TouchableOpacity>
      </ScrollView>
    </ScreenContainer>
  );
}

const styles = StyleSheet.create({
  summaryRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
  },
  summaryCell: {
    alignItems: 'center',
  },
  summaryValue: {
    fontSize: fontSize.navArrow,
    fontWeight: 'bold',
    color: fg.primary,
  },
  summaryLabel: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  tourRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  tourLeft: {
    flex: 1,
  },
  tourDate: {
    fontSize: fontSize.label,
    color: fg.secondary,
    textTransform: 'uppercase',
  },
  tourDist: {
    fontSize: fontSize.heading,
    fontWeight: 'bold',
    color: fg.primary,
    marginTop: 2,
  },
  tourRight: {
    alignItems: 'flex-end',
  },
  tourStat: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: '500',
  },
  tourSub: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    marginTop: 2,
  },
  detailHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 16,
    gap: 12,
  },
  backBtn: {
    fontSize: fontSize.body,
    color: accent.blue,
  },
  detailTitle: {
    fontSize: fontSize.heading,
    fontWeight: 'bold',
    color: fg.primary,
  },
  heroRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 8,
  },
  heroCell: {
    alignItems: 'center',
  },
  heroValue: {
    fontSize: fontSize.navArrow,
    fontWeight: 'bold',
    color: fg.primary,
  },
  heroLabel: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    textTransform: 'uppercase',
    letterSpacing: 1,
    marginTop: 4,
  },
  detailDate: {
    fontSize: fontSize.label,
    color: fg.secondary,
    textAlign: 'center',
    marginTop: 8,
  },
  statRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
  },
  statLabel: {
    fontSize: fontSize.body,
    color: fg.secondary,
  },
  statValue: {
    fontSize: fontSize.body,
    color: fg.primary,
    fontWeight: '600',
  },
  sectionTitle: {
    fontSize: fontSize.body,
    fontWeight: '600',
    color: fg.secondary,
    textTransform: 'uppercase',
    letterSpacing: 1.5,
    marginBottom: 12,
  },
  altBars: {
    flexDirection: 'row',
    height: 120,
    alignItems: 'flex-end',
    gap: 8,
  },
  altBarContainer: {
    flex: 1,
    alignItems: 'center',
    height: '100%',
    justifyContent: 'flex-end',
  },
  altBar: {
    width: '80%',
    borderRadius: 4,
  },
  altBarLabel: {
    fontSize: fontSize.tiny,
    color: fg.tertiary,
    marginTop: 4,
  },
  exportBtn: {
    backgroundColor: bg.raised,
    borderRadius: 10,
    padding: 16,
    alignItems: 'center',
    marginVertical: 16,
    borderWidth: 1,
    borderColor: accent.cyan,
  },
  exportBtnText: {
    fontSize: fontSize.body,
    fontWeight: '600',
    color: accent.cyan,
  },
});

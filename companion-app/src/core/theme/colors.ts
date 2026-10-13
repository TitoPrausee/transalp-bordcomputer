/**
 * Farbpalette und Design-Tokens fuer die Companion-App.
 *
 * Uebernommen aus docs/research/10-ui-ux-design.md.
 * Alle Tokens sind synchron zum Bordcomputer (Dark-Mode).
 */

// --- Hintergrund-Layer (Hierarchie von hinten nach vorne) ---

export const bg = {
  void: '#000000', // Vollschwarz, Haupt-Hintergrund
  deep: '#0A0A0C', // Tiefster Layer
  panel: '#1C1C1E', // Panel-Hintergrund, Cards
  raised: '#2C2C2E', // Erhoehte Elemente, aktive Cards
  overlay: '#3A3A3C', // Overlays, Modals
} as const;

// --- Vordergrund-Text ---

export const fg = {
  primary: '#FFFFFF', // Primärer Text, Hauptwerte
  secondary: '#8E8E93', // Sekundärer Text, Labels
  tertiary: '#48484A', // Tertiärer Text, deaktiviert
} as const;

// --- Akzentfarben ---

export const accent = {
  amber: '#FF9500', // Primärer Akzent, RPM-Verlauf Start
  orange: '#FF6B00', // RPM-Mittelbereich
  red: '#FF3B30', // Warnungen, Drehzahlbegrenzer
  blue: '#0A84FF', // Info, Navigation, BT-Status
  green: '#30D158', // Erfolg, GPS-Fix, OK-Status
  cyan: '#40C8E0', // Telemetrie, Live-Daten
  yellow: '#FFD60A', // Schaltlicht, Hinweis
} as const;

// --- RPM-Farbverlauf (Drehzahl-Balken) ---

export function rpmColor(rpm: number, maxRpm: number): string {
  const pct = maxRpm > 0 ? rpm / maxRpm : 0;
  if (pct < 0.4) return accent.green;
  if (pct < 0.7) return accent.yellow;
  if (pct < 0.85) return accent.orange;
  return accent.red;
}

// --- Temperatur-Farbe ---

export function tempColor(tempC: number): string {
  if (tempC >= 110) return accent.red;
  if (tempC >= 100) return accent.yellow;
  return fg.primary;
}

// --- Kraftstoff-Farbe ---

export function fuelColor(fuelPct: number): string {
  if (fuelPct < 10) return accent.red;
  if (fuelPct < 25) return accent.yellow;
  return fg.primary;
}

// --- Bordspannung-Farbe ---

export function voltageColor(v: number): string {
  if (v < 11.5 || v > 14.8) return accent.red;
  return fg.primary;
}

// --- Typografie-Indizes (Pixel-Groessen) ---

export const fontSize = {
  speed: 72,
  navArrow: 48,
  rpmDigit: 36,
  heading: 24,
  body: 16,
  label: 12,
  tiny: 10,
} as const;

// --- Timing-Kurven (Bezier, kompatibel mit React Native Animated) ---

export const easing = {
  easeOut: [0.25, 0.46, 0.45, 0.94] as const,
  easeInOut: [0.42, 0.0, 0.58, 1.0] as const,
  easeOutBack: [0.34, 1.56, 0.64, 1.0] as const,
  easeIn: [0.42, 0.0, 1.0, 1.0] as const,
  linear: [0.0, 0.0, 1.0, 1.0] as const,
} as const;

// --- Animations-Dauern (ms) ---

export const duration = {
  rpmUp: 120,
  rpmDown: 250,
  rpmBounce: 150,
  speedDigit: 100,
  gearChange: 200,
  fuelUpdate: 400,
  navMorph: 300,
  navGrow: 800,
  screenSlide: 250,
  screenFade: 200,
  toastIn: 200,
  toastOut: 150,
  bootSeq: 2000,
  countUp: 800,
  chartEntry: 300,
} as const;

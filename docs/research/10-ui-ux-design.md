# Premium UI/UX Design-Konzept: Animationen, Layouts, Visual Language

> Stand: 13.10.2026
> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986–1999, rein mechanisch, keine ECU
> Plattform: ESP32-S3 (N8R8), 16MB Flash, 8MB PSRAM, Dual-Core 240MHz
> Displays: 3.5" TFT 480x320 RGB565 (Phase 1), 2.13" E-Paper 250x122 (Phase 2)
> UI-Bibliothek: LVGL 9 mit esp_lvgl_port (TFT), GxEPD2 (E-Paper)
> Vorherige Dokumente: 01–07 in docs/research/

## 0. Einleitung und Design-Philosophie

Dieses Dokument definiert die visuelle Sprache, die Animations-Systematik und die
Screen-Layouts fuer den Transalp-Bordcomputer. Die Zielsetzung ist eine UI, die auf
einem 480x320-TFT und einem 250x122-E-Paper-Display extrem hochwertig, fluessig
und schoen wirkt, trotz der stark limitierten Rechenleistung eines ESP32-S3.

Die Design-Philosophie orientiert sich an kalifornischer Premium-Technologie-
Aesthetik: radikale Reduktion auf das Wesentliche, subtile Tiefenwirkung durch
Layering und Transparenz, nahtlose Uebergaenge zwischen Zustaenden, und
Animationen, die nie dekorativ sondern immer funktional sind. Jede Bewegung auf
dem Display kommuniziert Information: eine Zahl, die sich bewegt, zeigt
Beschleunigung; ein Balken, der fuellt, zeigt steigende Drehzahl; ein Pfeil, der
morphed, zeigt eine neue Richtung an.

Drei Grundprinzipien leiten jede Designentscheidung:

**Reduktion**: Kein Element auf dem Display, das nicht dem Fahrer bei der
Fahrt hilft. Kein dekorativer Schnickschnack, keine animierten Hintergruende,
keine Eye-Candy-Elemente, die Aufmerksamkeit binden. Jedes Pixel muss einen
Informationswert haben oder eine Bewegung ermoeglichen, die Information
vermittelt.

**Tiefe**: Die UI nutzt visuelle Hierarchie, um Wichtiges von Unwichtigem zu
trennen. Die Geschwindigkeit als wichtigster Wert steht im Zentrum, gross und
dominant. Sekundaerwerte wie Temperatur oder Kraftstoffstand sind kleiner und
am Rand. Tertiaere Werte wie HDOP oder Satellitenanzahl sind minimal oder
versteckt. Diese Hierarchie wird durch Groesse, Helligkeit, Position und
Farbsaettigung erzeugt, nicht durch dekorative Effekte.

**Fluss**: Alle Zustaendswechsel sind animiert, aber die Animationen sind
kurz (80–400ms) und dienen der raeumlichen Orientierung. Ein Screen, der von
rechts einslidet, kommt von einer logisch "naechsten" Funktion. Ein Overlay,
das fade-t, ist tempororaer. Die Bewegungsgeschwindigkeit korrespondiert mit
der Wichtigkeit der Aenderung: ein Gangwechsel ist schnell (120ms), eine
Screen-Transition ist mittelschnell (250ms), ein Boot-Sequenz ist bewusst
langsamer (800–1500ms), um den Systemstart als bewussten Moment zu inszenieren.

---

## 1. Design-Sprache

### 1.1 Farbpalette

Die Farbpalette basiert auf einem Dark-Mode-Primary-Konzept. Der Hintergrund
ist tief schwarz, nicht dunkelgrau — das reduziert Blendeffekte bei Nachtfahrt
und maximiert den Kontrast fuer die hellen Werte. Auf Schwarz baut eine
dreischichtige Hierarchie von Anthrazit-Tonen auf, die als Container und
Panel-Hintergruende dienen.

**Basis-Palette (RGB565-Hex):**

| Token | Hex (RGB888) | RGB565 | Verwendung |
|---|---|---|---|
| `COLOR_BG_VOID` | #000000 | 0x0000 | Vollschwarz, Haupt-Hintergrund TFT |
| `COLOR_BG_DEEP` | #0A0A0C | 0x0000 | Tiefster Layer, fast unsichtbar |
| `COLOR_BG_PANEL` | #1C1C1E | 0x1084 | Panel-Hintergrund, Cards |
| `COLOR_BG_RAISED` | #2C2C2E | 0x2104 | Erhoehte Elemente, aktive Cards |
| `COLOR_BG_OVERLAY` | #3A3A3C | 0x3186 | Overlays, Modals |
| `COLOR_FG_PRIMARY` | #FFFFFF | 0xFFFF | Primärer Text, Hauptwerte |
| `COLOR_FG_SECONDARY` | #8E8E93 | 0x8C71 | Sekundärer Text, Labels |
| `COLOR_FG_TERTIARY` | #48484A | 0x4208 | Tertiärer Text, deaktiviert |

**Akzentfarben:**

| Token | Hex (RGB888) | RGB565 | Verwendung |
|---|---|---|---|
| `COLOR_ACCENT_AMBER` | #FF9500 | 0xFC80 | Primärer Akzent, Drehzahl-Verlauf Start |
| `COLOR_ACCENT_ORANGE` | #FF6B00 | 0xFB20 | RPM-Mittelbereich |
| `COLOR_ACCENT_RED` | #FF3B30 | 0xF800 | Warnungen, Drehzahlbegrenzer |
| `COLOR_ACCENT_BLUE` | #0A84FF | 0x041F | Info, Navigation, Bluetooth-Status |
| `COLOR_ACCENT_GREEN` | #30D158 | 0x3660 | Erfolg, GPS-Fix, OK-Status |
| `COLOR_ACCENT_CYAN` | #40C8E0 | 0x4CDA | Telemetrie, Live-Daten |
| `COLOR_ACCENT_YELLOW` | #FFD60A | 0xFFE0 | Schaltlicht, Hinweis |

**RPM-Farbverlauf (Drehzahl-Balken):**

Der Drehzahl-Balken durchlaeuft einen Farbverlauf von gruen ueber gelb und
orange zu rot, der die Drehzahlzone kommuniziert:

| Drehzahl-Bereich | Farbe | Hex | Bedeutung |
|---|---|---|---|
| 0–40% | Gruen | #30D158 | Normaler Fahrbetrieb |
| 40–70% | Gelb | #FFD60A | Mittlere Last |
| 70–85% | Orange | #FF6B00 | Sportlicher Bereich |
| 85–95% | Rot | #FF3B30 | Schaltlicht aktiv |
| 95–100% | Pulsierend Rot | #FF3B30 (blink) | Drehzahlbegrenzer |

Auf E-Paper (1-Bit, schwarz/weiss) entfaellt der Farbverlauf. Stattdessen wird
die Drehzahlzone durch unterschiedliche Muster (durchgezogen fuer normal,
schraffiert fuer Warnbereich) und durch eine symbolische rote Linie am
Drehzahlbegrenzer kommuniziert — die "rote Linie" ist auf E-Paper eine
schwarze Linie mit einem kleinen Pfeil-Symbol darueber, die visuell klar als
Begrenzer erkennbar ist.

**Farbcodierung-Regeln:**

- Geschwindigkeit: immer `COLOR_FG_PRIMARY` (weiss) — die wichtigste Zahl bleibt
  neutral und dominant
- Drehzahl: Farbverlauf wie oben, die Farbe selbst kommuniziert die Laststufe
- Warnungen: `COLOR_ACCENT_RED` fuer kritische Werte (Uebertemp, Oeldruck,
  Bordspannung kritisch) — gleichzeitig blinkend
- Info: `COLOR_ACCENT_BLUE` fuer Bluetooth-Status, Navigations-Hinweise,
  System-Meldungen
- Erfolg: `COLOR_ACCENT_GREEN` fuer GPS-Fix, Verbindung hergestellt,
  Self-Test bestanden
- Akku/Ladezustand: `COLOR_ACCENT_GREEN` > 20%, `COLOR_ACCENT_YELLOW` 10–20%,
  `COLOR_ACCENT_RED` < 10% (pulsierend)

### 1.2 Typografie

Auf dem ESP32-S3 mit LVGL 9 stehen zwei Font-Rendering-Pfade zur Verfuegung:
eingebaute Bitmap-Fonts (ueber LVGLs `lv_font`-System) und custom FreeType-
basierte Rasterisierung beim Build. Fuer dieses Projekt werden die Fonts
bereits beim Build als Bitmaps gerendert und als C-Arrays kompiliert — kein
runtime FreeType, das Speicher und CPU kosten wuerde.

**Font-Hierarchie:**

| Token | Font-Familie | Groesse | Gewicht | Verwendung |
|---|---|---|---|---|
| `FONT_SPEED` | Inter / Roboto | 72px | Bold | Geschwindigkeit (Hauptzahl) |
| `FONT_RPM_DIGIT` | Inter / Roboto | 36px | SemiBold | Drehzahl digital |
| `FONT_HEADING` | Inter / Roboto | 24px | Bold | Screen-Titel, Hauptwerte |
| `FONT_BODY` | Inter / Roboto | 16px | Regular | Sekundaerwerte, Labels |
| `FONT_LABEL` | Inter / Roboto | 12px | Medium | Mini-Labels, Einheiten |
| `FONT_NAV_ARROW` | Inter / Roboto | 48px | Bold | Navigations-Distanz |
| `FONT_TINY` | Inter / Roboto | 10px | Regular | Status-Leiste, Icons |

**Font-Empfehlung: Inter (oder als Fallback Roboto)**

Inter ist eine Open-Source-Schriftart, die speziell fuer Bildschirme
optimiert wurde. Sie hat eine hohe x-Hoehe (verbessert Lesbarkeit bei kleinen
Groessen), klare Letterformen und eine ausgepraegte Gewichts-Hierarchie.
Roboto ist als Fallback geeignet, da es aehnliche Eigenschaften hat und in der
LVGL-Standarddistribution verfuegbar ist.

Beide Fonts sind als TTF/OTF frei verfuegbar und koennen mit dem LVGL Font
Converter (`lv_font_conv`) in C-Arrays umgewandelt werden. Der Font-Converter
erzeugt Bitmaps in benoetigten Groessen mit nur den benoetigten Glyphen
(reduziert Flash-Verbrauch). Fuer deutschsprachige UI werden zusaetzlich
Umlaute (ae, oe, ue, sz) und Sonderzeichen benoetigt — der Zeichensatz wird
auf ASCII + Germanisch-Latein beschraenkt, um Flash zu sparen.

**Praktische Umsetzung:**

```c
// LVGL Font-Definitionen (beim Build generiert)
LV_FONT_DECLARE(inter_72_bold);    // Geschwindigkeit
LV_FONT_DECLARE(inter_36_semibold); // Drehzahl
LV_FONT_DECLARE(inter_24_bold);     // Headings
LV_FONT_DECLARE(inter_16_regular);  // Body
LV_FONT_DECLARE(inter_12_medium);    // Labels
LV_FONT_DECLARE(inter_48_bold);     // Navigation
```

**Flash-Budget fuer Fonts:**

Ein Font mit vollem Latin-1-Zeichensatz (~190 Glyphen) braucht bei 72px Bold
ca. 35–45 KB Flash. Bei 6 Font-Groessen mit insgesamt ca. 150–200 KB Flash
ist das bei 16 MB Flash verschwindend gering.

Auf E-Paper (1-Bit) werden die gleichen Fonts verwendet, aber nur in einer
Groesse pro Rolle: 24px Bold fuer Werte, 12px Regular fuer Labels. Der E-Paper-
Screen ist 250x122 (oder 296x128 bei 2.9"), da ist Platz fuer maximal eine
72px-Zahl plus Labels. Die E-Paper-Typografie ist daher kompromissbehaftet:
Speed in 48px Bold, RPM in 24px Bold, Labels in 10px. Das ist lesbar, aber
nicht "gross" — das TFT ist das primaere Anzeigedisplay.

### 1.3 Schatten, Tiefe und Layering

Auf einem 480x320-TFT mit RGB565 ist echtes Schatten-Rendering (Gaussian Blur
ueber Alpha-Kanal) zu teuer: ein 10px-Schatten um ein 200x100-Panel erfordert
10x10 = 100 Multiplikationen pro Pixel des Panels, das sind bei einem
30-FPS-Refresh exponentiell viele Operationen, die der ESP32-S3 nicht
leisten kann.

Stattdessen wird Tiefe durch eine Kombination aus drei Techniken erzeugt:

**1. Elevation-Striche (Hard Shadow):**

Jedes Panel, das "erhoeht" sein soll, bekommt einen 1–2px breiten Rand in
einem dunkleren Grauton als der Hintergrund. Das ist kein weicher Schatten,
aber ein harter Kontrast, der das Panel visuell vom Hintergrund loest. Die
Regel: ein Panel auf `COLOR_BG_VOID` (#000000) bekommt einen Rand in
`COLOR_BG_DEEP` (#0A0A0C) — fast unsichtbar, aber strukturgebend. Ein Panel
auf `COLOR_BG_PANEL` (#1C1C1E) bekommt einen Rand in `COLOR_BG_RAISED`
(#2C2C2E).

**2. Helligkeits-Hierarchie:**

Die Hintergrundfarbe kommuniziert Hierarchie. Das tiefste Element ist
`COLOR_BG_VOID` (schwarz). Darueber liegen Panels in `COLOR_BG_PANEL`
(hellgrau). Darueber liegen aktive/erhoehte Elemente in `COLOR_BG_RAISED`
(helleres Grau). Diese drei Helligkeitsstufen erzeugen eine visuelle Tiefe
von hinten nach vorne, ohne Schatten.

**3. Transparenz-Layering fuer Overlays:**

Modale Overlays (z.B. eine Warnung, die ueber dem Instrument-Screen liegt)
verwenden einen halbtransparenten Hintergrund: `COLOR_BG_OVERLAY` mit 80%
Deckkraft ueber dem darunterliegenden Screen. Das erzeugt den Eindruck einer
"Glasscheibe" ueber dem eigentlichen UI. Auf RGB565 ohne echtes Alpha-Blending
wird dies durch Pre-Rendering simuliert: der Overlay-Hintergrund ist eine
Mischfarbe aus Overlay-Farbe und dem durchschnittlichen Hintergrund.

Echtes Alpha-Blending ist in LVGL 9 verfuegbar, aber pro Pixel teuer. Die
Strategie: Overlays werden mit `lv_obj_set_style_bg_opa(obj, LV_OPA_80, 0)`
konfiguriert. LVGL rendert Alpha-Blending nur in den "invalidierten" Bereichen
(wo sich etwas aendert), nicht pro Frame fuer den ganzen Screen. Das ist
performant genug fuer seltene Overlays (Warnungen, Modals), aber nicht fuer
jeden Frame.

### 1.4 Ikonographie

Die Icons folgen einem einheitlichen Stil: **Linien-Icons (Outline Style)**
mit 1.5–2px Strichstaerke, abgerundeten Enden (Round Caps) und einer
einheitlichen Groesse von 16x16, 24x24 und 32x32 Pixeln.

Warum Linien-Icons statt gefuellte Icons? Drei Gruende:

1. **Konsistenz mit der Design-Sprache**: Die gesamte UI ist reduziert und
   linear. Linien-Icons passen zu dieser Aesthetik, gefuellte Icons waeren
   visuell zu schwer und wuerden die Leichtigkeit der UI brechen.
2. **Skalierbarkeit**: Linien-Icons koennen in verschiedenen Groessen
   verwendet werden, ohne dass sie pixelig wirken — die Strichstaerke wird
   proportional mit skaliert.
3. **Flash-Effizienz**: Linien-Icons haben weniger gefuellte Pixel als
   gefuellte Icons, was die Bitmap-Groesse reduziert (relevant bei
   vielen Icons).

Die Icons werden als Vektor-Pfade in SVG definiert und mit dem LVGL-Converter
in Bitmaps (in den benoetigten Groessen) umgewandelt. Fuer E-Paper werden
zusaetzlich 1-Bit-Bitmaps generiert (nur Schwarz/Weiss, keine Antialiasing).

**Icon-Set (Auswahl):**

| Name | Verwendung | Beschreibung |
|---|---|---|
| `icon_speed` | Geschwindigkeit | Tacho-Kreis mit Nadel |
| `icon_rpm` | Drehzahl | Rechteck mit gefuelltem Balken |
| `icon_gear` | Gang | Zahnrad |
| `icon_temp` | Temperatur | Thermometer |
| `icon_fuel` | Kraftstoff | Tankpumpe / Benzin-Tropfen |
| `icon_battery` | Akku | Rechteck mit Blitz |
| `icon_gps` | GPS-Status | Satellit / Pin |
| `icon_bluetooth` | BT-Status | B-Form |
| `icon_nav_arrow` | Navigation | Pfeil |
| `icon_nav_left` | Nav-Links | Nach links gebogener Pfeil |
| `icon_nav_right` | Nav-Rechts | Nach rechts gebogener Pfeil |
| `icon_nav_uturn` | Wende | U-foermiger Pfeil |
| `icon_nav_roundabout` | Kreisverkehr | Kreispfeil |
| `icon_nav_arrive` | Ziel | Flagge |
| `icon_warning` | Warnung | Dreieck mit Ausrufezeichen |
| `icon_error` | Fehler | Dreieck mit X |
| `icon_settings` | Einstellungen | Zahnrad-Kombi |
| `icon_stats` | Statistik | Balkendiagramm |
| `icon_media_play` | Wiedergabe | Dreieck |
| `icon_media_pause` | Pause | Zwei Striche |
| `icon_phone` | Anruf | Telefonhoerer |

Alle Icons werden als SVG-Quellen in `assets/icons/` gepflegt und beim Build
in die benoetigten Bitmap-Formate konvertiert.

### 1.5 Farbcodierung im Ueberblick

| Datentyp | Farbe | Regel |
|---|---|---|
| Geschwindigkeit | #FFFFFF (weiss) | Immer neutral-weiss, hoechste Prioritaet |
| Drehzahl | Farbverlauf gruen→gelb→orange→rot | Farbe kommuniziert Laststufe |
| Gang | #FFFFFF (weiss) | Neutral, klein unter RPM |
| Temperatur | #FFFFFF normal, #FF3B30 kritisch | Rot ab 110°C Kuehlmittel |
| Kraftstoff | #FFFFFF normal, #FFD60A niedrig, #FF3B30 kritisch | Gelb < 25%, Rot < 10% |
| Bordspannung | #FFFFFF normal, #FF3B30 kritisch | Rot < 11.5V oder > 14.8V |
| Warnungen | #FF3B30 (rot) | Blinkend fuer kritische |
| Info / BT / Nav | #0A84FF (blau) | Statische Info-Anzeige |
| Erfolg / GPS-Fix | #30D158 (gruen) | Status-Indikator |
| Akku intern | #30D158 / #FFD60A / #FF3B30 | Wie Kraftstoff |

---

## 2. Animationen

Animationen sind das Herzstueck der UX. Auf einem 480x320-TFT mit 15–30 FPS
koennen wir keine 60-FPS-Animationen wie auf einem Smartphone bieten, aber
wir koennen mit klaren Timing-Kurven und gezielten Bewegungen eine
"hochwertige" Wahrnehmung erzeugen. Der Schluessel ist: weniger, aber
praesisere Animationen. Eine Bewegung von 200ms mit der richtigen Ease-Kurve
wirkt hochwertiger als eine 500ms-Animation, die linear verlaeuft.

### 2.1 Timing-Funktionen und Bezier-Kurven

Alle Animationen verwenden eine von fuenf definierten Timing-Funktionen.
Diese werden als kubische Bezier-Kurven spezifiziert (CSS-kompatibel) und in
LVGL als `lv_anim_path_t`-Callbacks umgesetzt.

| Name | Bezier (x1, y1, x2, y2) | Verwendung |
|---|---|---|
| `ease_out` | (0.25, 0.46, 0.45, 0.94) | Standard fuer Einblenden, Fuellen |
| `ease_in_out` | (0.42, 0.0, 0.58, 1.0) | Screen-Transitions, Positionswechsel |
| `ease_out_back` | (0.34, 1.56, 0.64, 1.0) | Overshoot bei Gang, subtile Bounce |
| `ease_in` | (0.42, 0.0, 1.0, 1.0) | Ausblenden, Beschleunigung |
| `linear` | (0.0, 0.0, 1.0, 1.0) | Nur fuer kontinuierliche Werte (Speed-Roll) |

```c
// LVGL Path-Callbacks
static const lv_anim_path_t path_ease_out = {
    .cb = lv_anim_path_overshoot  // LVGL eingebaute Naeherung
};
static const lv_anim_path_t path_ease_in_out = {
    .cb = lv_anim_path_ease_in_out
};
static const lv_anim_path_t path_linear = {
    .cb = lv_anim_path_linear
};
```

**Generelle Animations-Regeln:**

- Dauer: 80–400ms. Nie laenger als 400ms fuer eine UI-Transition.
- Interpolation: immer nicht-linear, ausser bei kontinuierlichen Werten.
- Overshoot: maximal 5–10% ueber Zielwert, nur bei `ease_out_back`.
- Reduktion: Wenn der Wert sich sehr schnell aendert (> 5x pro Sekunde),
  Animation ueberspringen und sofort setzen.
- Frame-Budget: 30 FPS Ziel. Bei 15 FPS werden Animationen halb so viele
  Schritte haben, aber die Timing-Kurve bleibt erhalten.

### 2.2 Drehzahl-Anzeige (RPM)

Die Drehzahl-Anzeige ist die animationstechnisch anspruchsvollste Komponente.
Sie muss schnell reagieren (RPM aendert sich bei Vollgas in <1s von 2000 auf
8000), aber sie darf nicht "nervoes" wirken. Die Loesung ist ein
Zweiphasen-Animationsmodell: eine schnelle Phase beim Gasgeben (Aufwaerts)
und eine langsamere, traegheitsbehaftete Phase beim Gaswegnehmen (Abwaerts).

**Visuelle Form:**

Ein horizontaler Balken am oberen Bildschirmrand, der sich von links nach
rechts fuellt. Der Balken ist 440px breit und 24px hoch (auf 480x320). Die
Farbe des Balkenfuellstands wechselt entlang des Farbverlaufs (gruen → gelb →
orange → rot). Am rechten Ende, bei ca. 85% der Breite, ist eine markante
"Shift-Light"-Linie (2px breit, gelb). Bei 95% beginnt die rote Zone: der
Balken pulsierend rot, 2px ueber dem Balken eine blinkende Warnlinie.

**Animationsverhalten:**

*Beim Gasgeben (RPM steigt):*
- Der Balken fuellt sich nach rechts mit `ease_out` (0.25, 0.46, 0.45, 0.94).
- Dauer: 120ms pro Update-Schritt.
- Die Traegheit ist gering: der Balken folgt dem tatsaechlichen RPM-Wert
  mit max. 50ms Verzoegerung (ein Frame bei 20 FPS).
- Die Farbuebergaenge sind hart (kein sanfter Farbuebergang zwischen Zonen) —
  das ist absichtlich: die Zone (gruen/gelb/orange/rot) ist eine klare
  Information, kein dekorativer Verlauf.

*Beim Gaswegnehmen (RPM faellt):*
- Der Balken faellt mit `ease_in_out` (0.42, 0.0, 0.58, 1.0).
- Dauer: 250ms pro Update-Schritt — bewusst langsamer als Aufwaerts.
- Bei schnellem Zurueckfallen (Schaltvorgang, Kupplung gezogen) gibt es einen
  leichten Overshoot: der Balken faellt unter den Zielwert und federt zurueck.
  Overshoot: 8% des Deltas, Dauer des Rueckfederns: 150ms mit `ease_out_back`
  (0.34, 1.56, 0.64, 1.0).
- Dieser Overshoot simuliert die mechanische Traegheit eines analogen Tachos
  und erzeugt das Gefuehl von "Substanz" und "Gewicht".

*Rote Zone (Drehzahlbegrenzer):*
- Bei RPM > 95% des Rotationsmaximums: Balken pulsiert rot.
- Pulsfrequenz: 2 Hz (500ms an, 500ms aus), `COLOR_ACCENT_RED` ↔ dimmed red.
- Zusaetzlich blinkt eine 2px-Linie ueber dem Balkenende.
- Das Pulsieren wird nicht per Frame berechnet, sondern per Timer: alle 500ms
  wird der Style gewechselt (sichtbar/unsichtbar). Das ist extrem billig.

**Code-Struktur:**

```c
typedef struct {
    float current_rpm;      // Animierter RPM-Wert (fuer Balken)
    float target_rpm;       // Tatsaechlicher RPM vom Sensor
    float last_update_ms;
    bool is_redline;        // RPM im roten Bereich
} rpm_animation_t;

void rpm_animation_update(rpm_animation_t *anim, float target, uint32_t now_ms) {
    float delta = target - anim->current_rpm;
    float dt = (now_ms - anim->last_update_ms) / 1000.0f;

    if (delta > 0) {
        // Aufwaerts: schnell, ease_out
        float speed = delta * 8.0f; // Hohe Folgerate
        anim->current_rpm += speed * dt;
        if (anim->current_rpm > target) anim->current_rpm = target;
    } else {
        // Abwaerts: langsam, mit Traegheit
        float speed = delta * 4.0f; // Niedrigere Folgerate
        anim->current_rpm += speed * dt;
        // Overshoot-Bounce bei starkem Abfall
        if (delta < -500.0f) {
            // Trigger bounce (im naechsten Frame)
            anim->bounce_target = target + (delta * 0.08f);
        }
    }

    anim->target_rpm = target;
    anim->last_update_ms = now_ms;
    anim->is_redline = (target > RPM_REDLINE_THRESHOLD);
}
```

### 2.3 Geschwindigkeit (Speed)

Die Geschwindigkeit wird als grosse digitale Zahl in der Bildschirmmitte
angezeigt (72px Bold). Die Herausforderung: GPS liefert nur alle 100ms (10Hz)
einen neuen Wert. In der Zwischenzeit wuerde die Zahl stillstehen. Die
Loesung ist eine **Smooth-Interpolation**: die angezeigte Zahl wird zwischen
den GPS-Updates kontinuierlich extrapoliert.

**Extrapolation:**

```c
// Alle 33ms (30 FPS) wird die Anzeige aktualisiert
float speed_displayed;
float speed_target;     // Letzter GPS-Wert
float speed_rate;       // Geschwindigkeitsaenderung pro Sekunde

void speed_tick(uint32_t now_ms) {
    float dt = 0.033f; // 30 FPS
    // Linear extrapolation zwischen GPS-Updates
    float predicted = speed_target + speed_rate * dt;
    // Sanftes Annaeherung an den vorhergesagten Wert
    speed_displayed += (predicted - speed_displayed) * 0.3f;
}
```

**Zahlenwechsel-Animation ("Rolling"):**

Wenn sich die Geschwindigkeit aendert (z.B. von 98 auf 99 km/h), wird nicht
einfach die Zahl ausgetauscht. Stattdessen "rollt" die alte Zahl nach oben
weg und die neue Zahl rollt von unten ein. Das ist die klassische
"Slot-Machine"-Animation.

- Dauer: 100ms pro Ziffer, `ease_out` (0.25, 0.46, 0.45, 0.94).
- Nur die sich aendernden Ziffern werden animiert. Wenn 98 → 99, animiert
  nur die letzte Ziffer. Wenn 99 → 100, animieren alle drei Ziffern.
- Richtung: bei Beschleunigung rollen Ziffern nach oben (alte geht raus
  oben, neue kommt rein unten). Bei Verzoegerung rollen sie nach unten.

**Implementierung in LVGL:**

LVGL hat eingebaute Roller/Spinner-Widgets, aber fuer die Geschwindigkeit
wird ein Custom-Widget gebaut, das einen `lv_obj_t` mit einem Clip-Bereich
verwendet und die Ziffern als `lv_label` mit Y-Position-Verschiebung animiert.

```c
// Vereinfachte Struktur
typedef struct {
    lv_obj_t *container;
    lv_obj_t *digit_labels[3]; // 3 Ziffern fuer 0-999
    int current_digits[3];
    int target_digits[3];
} speed_display_t;

void speed_set_value(speed_display_t *spd, int kmh) {
    int new_digits[3] = { kmh / 100, (kmh / 10) % 10, kmh % 10 };
    for (int i = 0; i < 3; i++) {
        if (new_digits[i] != spd->target_digits[i]) {
            // Animation: alte Ziffer rollt hoch, neue kommt von unten
            animate_digit_roll(spd->digit_labels[i],
                               spd->current_digits[i],
                               new_digits[i], 100);
            spd->target_digits[i] = new_digits[i];
        }
    }
}
```

### 2.4 Gang-Anzeige

Die Gang-Anzeige ist eine kleine Zahl (24px Bold) neben der Drehzahl. Beim
Schalten (Gang wechselt von 4 auf 5) gibt es eine Slide-In-Animation.

**Animation:**

- Die alte Gang-Zahl slidet nach links heraus (80px, 150ms, `ease_in`).
- Gleichzeitig slidet die neue Gang-Zahl von rechts herein (80px, 150ms,
  `ease_out_back` mit leichtem Overshoot).
- Der neue Gang "landet" mit einem leichten Bounce (8% Overshoot).
- Gesamtdauer: 200ms (ueberlappend).

Die `ease_out_back`-Kurve (0.34, 1.56, 0.64, 1.0) sorgt fuer den charakteristischen
"Bounce"-Effekt, der dem Gangwechsel ein gefuehl von "Einrasten" gibt — als
ob der Gang mechanisch einrastet. Das ist eine subtile, aber wirkungsvolle
Animation, die die Haptik des Interface erhoht.

### 2.5 Kraftstoffstand

Der Kraftstoffstand wird als vertikaler Balken (oder als Ikone mit Fuellung)
dargestellt. Die Animation ist eine sanfte Fuellung/Entleerung.

**Normalbetrieb:**
- Der Fuellstand aendert sich langsam (ueber Minuten).
- Animation: `ease_in_out`, 400ms pro Update.
- Die Fuellung bewegt sich sanft zum neuen Wert, kein Overshoot.

**Niedriger Kraftstoffstand (< 25%):**
- Die Farbe wechselt auf `COLOR_ACCENT_YELLOW` (#FFD60A).
- Die Fuellung pulsiert leicht (Breite variierend um 2px, 1 Hz).
- Zusatz: kleines "RES" Label blinkt.

**Kritischer Kraftstoffstand (< 10%):**
- Farbe wechselt auf `COLOR_ACCENT_RED` (#FF3B30).
- Pulsieren intensiviert: 2 Hz, Breitenvariation 4px.
- Gleichzeitig erscheint eine Warn-Toast-Animation (siehe 2.8).

### 2.6 Navigation: Pfeil-Morphing

Der Navigationspfeil ist das visuelle Herzstueck des Navigations-Screens.
Er morphed zwischen verschiedenen Maneuver-Typen (geradeaus, links, rechts,
wende, kreisverkehr). Das Morphing ist keine 3D-Animation, sondern eine
2D-Form-Interpolation: ein Pfeil-Polygon transformiert sich in ein anderes.

**Maneuver-Typen und Polygone:**

Jeder Maneuver-Typ hat ein definiertes Polygon (Array von 2D-Punkten). Das
Morphing interpoliert zwischen dem aktuellen und dem Ziel-Polygon.

```c
// Polygon fuer "geradeaus" (gerader Pfeil nach oben)
static const lv_point_t poly_straight[] = {
    {50, 5}, {95, 60}, {70, 60}, {70, 95}, {30, 95}, {30, 60}, {5, 60}
};
// Polygon fuer "links abbiegen"
static const lv_point_t poly_left[] = {
    {50, 5}, {95, 60}, {70, 60}, {70, 80}, {30, 80}, {30, 95}, {5, 95}, {5, 60}
};
// Polygon fuer "rechts abbiegen"
static const lv_point_t poly_right[] = {
    {50, 5}, {95, 60}, {95, 95}, {70, 95}, {70, 80}, {30, 80}, {30, 60}, {5, 60}
};
// Polygon fuer "wende" (U-turn)
static const lv_point_t poly_uturn[] = {
    {50, 5}, {95, 50}, {80, 50}, {80, 95}, {20, 95}, {20, 50}, {5, 50}
};
```

**Morphing-Animation:**

- Beim Empfang eines neuen Maneuvers vom Typ A → Typ B:
- Interpolation der Polygon-Punkte ueber 300ms mit `ease_in_out`
  (0.42, 0.0, 0.58, 1.0).
- Waehrend des Morphing ist der Pfeil halbtransparent (LVGL OPA_50), um die
  "Verformung" zu betonen. Am Ende der Animation wird er voll deckend.
- Wenn beide Polygone die gleiche Punktanzahl haben, ist die Interpolation
  trivial (Punkt-fuer-Punkt). Bei unterschiedlichen Punktanzahlen wird das
  kleinere Polygon virtuell erweitert (Punkte dupliziert), um die
  Punktzahl anzugleichen.

**Naechster Maneuver (Countdown):**

Wenn der naechste Maneuver naehert (Distanz < 500m), vergroessert sich der
Pfeil: von 100x100px auf 140x140px ueber 800ms (`ease_out_back`).
Zusaetzlich erscheint ein Countdown: bei < 100m zur Maneuver wird eine
zusaetzliche Distanz-Zahl in grosser Schrift (36px) eingeblendet, die pro
10m-Abstand aktualisiert wird. Bei < 30m pulsiert der Pfeil (1 Hz, 10%
Groessenschwankung), um die Dringlichkeit zu kommunizieren.

### 2.7 UI-Uebergaenge (Screen-Transitions)

Zwischen den Screens (Instrument, Navigation, Split, etc.) gibt es Slide-
und Fade-Transitions. Die Bewegungsrichtung folgt einer logischen
"Hierarchie": vorwaerts (naechster Screen) slidet von rechts, rueckwaerts
(voriger Screen) slidet von links.

**Slide-Transition (vollstaendiger Screen-Wechsel):**

- Dauer: 250ms, `ease_in_out` (0.42, 0.0, 0.58, 1.0).
- Der alte Screen slidet nach links heraus (480px), der neue kommt von rechts
  (480px). Beide bewegen sich gleichzeitig.
- Alternativ: Crossfade (alter Screen fade-t aus, neuer fade-t ein) bei 200ms.
  Slide ist praesenter, Fade ist dezenter. Default: Slide fuer Haupt-Screens,
  Fade fuer Sub-Screens (z.B. Settings-Ebene 2).

**Fade-Transition (Overlays):**

- Fuer modale Overlays (Warnungen, Quick-Settings): Fade-In 150ms (`ease_out`),
  Fade-Out 120ms (`ease_in`).
- Der Overlay erscheint zentriert, der Hintergrund wird auf 50% Helligkeit
  gedimmt.

**Push-Transition (Navigation → Instrument):**

- Wenn Navigation aktiv ist und der Nutzer zurueck zum Instrument-Screen will:
  der Nav-Screen slidet nach oben heraus, der Instrument-Screen kommt von
  unten herein. Dauer: 300ms, `ease_in_out`. Das erzeugt ein "Schichten"-
  Gefuehl: Navigation ist eine "Ebene" ueber den Instrumenten.

### 2.8 Warnungen und Toast-Notifications

Warnungen erscheinen als Toast-Notifications: ein Panel, das von oben
hereinslidet, 3 Sekunden sichtbar bleibt, und dann nach oben herausslidet.

**Animation:**

- Slide-In: von Y=-80px nach Y=0px, 200ms, `ease_out` (0.25, 0.46, 0.45, 0.94).
- Haltezeit: 3000ms (kritische Warnungen halten bis bestaetigt).
- Slide-Out: von Y=0px nach Y=-80px, 150ms, `ease_in` (0.42, 0.0, 1.0, 1.0).
- Mehrere Warnungen stapeln sich vertikal (jede neue rutscht die alten nach
  unten).

**Kritische Warnungen (persistierend):**

Kritische Warnungen (Uebertemperatur, Oeldruck, Bordspannung kritisch) werden
nicht automatisch ausgeblendet. Sie bleiben sichtbar, bis der Zustand sich
normalisiert. Der Toast-Panel pulsiert (1 Hz, Hintergrund zwischen rot und
dunkelrot), bis die Warnung quittiert oder behoben ist.

### 2.9 Boot-Animation

Die Boot-Animation ist der erste Eindruck. Sie muss in der Zeit ablaufen, die
das System zum Initialisieren braucht (typisch 1.5–3 Sekunden: NVS laden,
BLE initialisieren, GPS starten, Display initialisieren). Sie ist kein
"Loading-Spinner", sondern eine choreografierte Sequenz.

**Sequenz:**

1. **0–200ms**: Bildschirm schwarz. Ein einzelner Punkt erscheint in der
   Mitte (1px, weiss).
2. **200–600ms**: Der Punkt waechst zu einem Kreis (Radius 0→30px,
   `ease_out`). Gleichzeitig faellt die Helligkeit des Punkts leicht ab,
   so dass der Kreis wie ein "Halo" wirkt.
3. **600–1000ms**: Aus dem Kreis "waechst" ein senkrechter Strich nach unten
   (Motorrad-Silhouette-Andeutung, 2px breit, 40px lang, `ease_out`).
4. **1000–1400ms**: Text "TRANSALP BORDCOMPUTER" fade-t ein unter der
   Silhouette (12px, `COLOR_FG_SECONDARY`).
5. **1400–1800ms**: Eine duenne horizontale Linie faehrt von links nach
   rechts unter dem Text durch (480px, 200ms, `ease_in_out`). Das ist der
   "Progress"-Indikator.
6. **1800–2000ms**: Alles fade-t aus, der Haupt-Screen fade-t ein.

Wenn das System laenger als 2s zum Booten braucht, bleibt die Szene bei
Schritt 5 (Linie+Text) stehen, bis das System bereit ist. Die Linie bewegt
sich weiter subtil (1px Auf-und-Ab, 2 Hz), um zu signalisieren, dass das
System "arbeitet".

**Performance-Anforderung:**

Die Boot-Animation muss in den vorhandenen Frame-Buffer passen und darf den
Boot-Prozess nicht verzoegern. Sie laeuft im Display-Task mit niedriger
Prioritaet und rendert nur die wenigen benoetigten Elemente pro Frame. Da
waehrend des Bootens keine Sensor-Daten anliegen, ist die CPU frei fuer
Rendering.

### 2.10 Lade-Animation (Akku)

Wenn der Bordcomputer geladen wird (per USB oder Bordnetz-Lader), erscheint
eine Lade-Animation im Akku-Icon der Status-Leiste.

**Animation:**

- Das Akku-Icon hat drei "Segmente" (Fuellung in 33%-Schritten).
- Waehrend des Ladens fuellen sich die Segmente nacheinander:
  Segment 1 fuellt (300ms), Pause 100ms, Segment 2 fuellt (300ms), Pause
  100ms, Segment 3 fuellt (300ms), Pause 500ms, dann alle entleeren und von
  vorne.
- Die Fuellung jedes Segments ist animiert mit `ease_out` (150ms).
- Bei vollstaendig geladenem Akku (100%): alle Segmente leuchten dauerhaft
  gruen, keine Animation.

### 2.11 Bluetooth-Verbindungs-Animation

Beim Verbinden mit der Companion-App pulsiert das BT-Icon.

**Animation:**

- Icon "atmet": Skalierung zwischen 90% und 110%, 1.5 Hz, `ease_in_out`.
- Beim erfolgreichen Verbinden: ein "Ping"-Effekt — ein Ring expandiert vom
  Icon aus (Radius 0→40px, 400ms, `ease_out`), wird dabei transparent.
- Danach: Icon statisch in `COLOR_ACCENT_BLUE`.
- Beim Trennen: Icon fade-t kurz auf 50% Transparenz und zurueck, dann
  statisch in `COLOR_FG_TERTIARY` (grau).

---

## 3. Screen-Layouts

### 3.1 Instrument-Screen

Der Standard-Screen beim Fahren ohne aktive Navigation. Alle Kerninstrumente
sichtbar.

```
+--------------------------------------------------+
|                                                  |
|  [RPM-Balken: ======== 4500 =====    ]           |
|  [Shift Light |    Redline ]                      |
|                                                  |
|                                                  |
|                    98                            |
|                   km/h                            |
|                                                  |
|                                                  |
|  [G:5]    [22 C]   [1/2]    [GPS 9]   [3.92V]    |
|                                                  |
+--------------------------------------------------+
  | 24px RPM-Balken (Farbverlauf, animiert)
  | 72px Speed-Zahl (zentriert, weiss, rollend)
  | Status-Leiste: Gang, Temp, Fuel, GPS-Sats, Volt
```

**Layout-Details (480x320):**

- Y 0–28: RPM-Balken, voll breit (mit 10px Padding → 460px breit, 24px hoch)
  - Farbverlauf gruen→gelb→orange→rot, animierte Fuellung
  - Shift-Light-Marker bei 85%, Redline bei 95%
- Y 28–34: Sub-RPM-Text "4500 rpm" rechtsbuendig, 12px, `COLOR_FG_SECONDARY`
- Y 34–220: Hauptbereich
  - Speed-Zahl zentriert: 72px Bold, Y-Position ~130
  - "km/h" Label darunter, 12px, `COLOR_FG_SECONDARY`
- Y 220–262: Sekundaerwerte in 5 Spalten
  - Gang (24px Bold): "5", mit `icon_gear` (16px) davor
  - Temperatur (16px): "22 C", mit `icon_temp`
  - Kraftstoff (16px): "1/2", mit `icon_fuel` und vertikalem Mini-Balken
  - GPS (12px): "9 SAT", mit `icon_gps`
  - Bordspannung (12px): "12.4V", mit `icon_battery`
- Y 262–280: Uhrzeit zentriert (24px Bold): "14:23"
- Y 280–320: Status-Leiste (12px)
  - Links: BT-Status (Icon + "Verbunden" / "Getrennt")
  - Mitte: Tour-Modus ("TOUR")
  - Rechts: Datum oder Tageskilometer

**E-Paper-Variante (250x122):**

Das E-Paper zeigt eine reduzierte Version der Instrumente, statisch:

```
+------------------------------------------+
|  98 km/h   |   4500 rpm   G:5            |
|            |   [####  ----]              |
|------------|-----------------------------|
|  22 C   1/2   340m   9 SAT   12.4V      |
|  14:23                    TOUR           |
+------------------------------------------+
```

E-Paper zeigt: Speed (48px), RPM (24px) + Mini-Balken, Gang, Temp, Fuel,
Hoehe, GPS, Volt, Uhrzeit. Keine Animationen, aber saubere Typografie und
klare Hierarchie. Refresh alle 2–5 Sekunden (Partial-Refresh), abhaengig von
Aenderungsrate.

### 3.2 Navigation-Screen

Vollbild-Navigation, wenn eine Route aktiv ist. Speed wird klein in der Ecke
angezeigt, damit sie stets sichtbar bleibt.

```
+--------------------------------------------------+
|                                                  |
|                    +---+                         |
|                    | ^ |                         |
|                    | | |     340 m               |
|                    +---+                         |
|                                                  |
|              B  1  - 7   K  l  r                 |
|              Bundesstrasse 17                    |
|                                                  |
|  [G:5]  [98]  ETA 15:42   42 km                 |
|                                                  |
+--------------------------------------------------+
```

**Layout-Details (480x320):**

- Y 0–200: Maneuver-Bereich
  - Y 10–150: Grosser Navigationspfeil (100x100px, zentriert), morphed
    zwischen Maneuver-Typen
  - Y 160–200: Distanz zum naechsten Maneuver (36px Bold): "340 m"
    - Bei < 100m: vergroesserter Pfeil (140x140px), pulsierend
    - Bei < 30m: zusaetzlich "JETZT" Label blinkend
- Y 200–240: Strassenname (24px Bold, zentriert): "Bundesstrasse 17"
  - Wenn zu lang fuer eine Zeile: Marquee-Scroll (10px/s, `linear`)
- Y 240–280: Kombinierte Info-Leiste
  - Links: Gang + Speed (kompakt: "5  98")
  - Mitte: ETA (16px Bold): "ETA 15:42"
  - Rechts: Rest-Distanz (16px Bold): "42 km"
- Y 280–320: Status-Leiste (wie Instrument-Screen)

**Naechster Maneuver-Preview:**

Wenn ein Maneuver nach dem naechsten nahe ist (< 2km), wird ein kleiner
"Next"-Pfeil rechts unten angezeigt (24x24px, halbtransparent) mit Distanz.
Das erlaubt dem Fahrer, sich mental auf den uebernaechsten Maneuver
vorzubereiten.

### 3.3 Split-Screen

Instrumente und Navigation gleichzeitig, wenn beide wichtig sind (z.B. bei
Stadtverkehr mit Navigation).

```
+-----------------------+--------------------------+
|                       |                          |
|        98 km/h        |        +---+             |
|                       |        | ^ |   340 m      |
|                       |        | | |             |
|                       |        +---+             |
|  [G:5] [4500] [22 C] |   Bundesstrasse 17       |
|  [1/2] [9 SAT] [12.4V]|   ETA 15:42   42 km     |
|                       |                          |
+-----------------------+--------------------------+
  | 240px Instrumente      |  240px Navigation      |
```

**Layout-Details (480x320, Split bei X=240):**

- Linke Haelfte (0–240):
  - Speed (48px Bold, zentriert): "98" + "km/h" (10px)
  - RPM-Balken (200px breit, 16px hoch) mit Shift-Light
  - Status-Grid (3x2): Gang, RPM, Temp, Fuel, GPS, Volt
- Rechte Haelfte (240–480):
  - Nav-Pfeil (60x60px, zentriert)
  - Distanz (24px Bold)
  - Strassenname (12px, ggf. scrollend)
  - ETA + Rest-Distanz (12px)

Der Split-Screen ist ein Kompromiss: beide Haelften haben weniger Platz, aber
alle kritischen Informationen bleiben sichtbar. Die Animationen sind reduziert:
Speed rollt noch, aber Pfeil-Morphing ist schneller (200ms statt 300ms), um
die kleinere Flaeche zu respektieren.

### 3.4 Diagnose-Screen

Fehlercodes, Sensor-Status und Self-Test. Dieser Screen ist nicht fuer die
Fahrt gedacht, sondern fuer den Stillstand (Werkstatt, Diagnose).

```
+--------------------------------------------------+
|  DIAGNOSE                            [SELF-TEST] |
|--------------------------------------------------|
|                                                  |
|  SENSOREN:                                       |
|    GPS        NEO-M9N        OK    9 SAT  HDOP 1.2 |
|    IMU        MPU6050       OK    Roll: +5.2     |
|    Baro       BMP280       OK    842 m   1013 hPa|
|    Temp       KTY81        OK    22 C            |
|    Fuel       Geber        OK    1/2   45%       |
|    RPM        PCNT         OK    4500            |
|    Volt       ADC          OK    12.4V           |
|                                                  |
|  FEHLERCODES:                                    |
|    [Keine aktiven Fehler]                        |
|                                                  |
|  LETZTE SELBST-TEST:  13.10.2026 14:20          |
|    Alle Systeme: OK                              |
|                                                  |
|  > Sensor-Details   > Fehlerverlauf   > Reset   |
+--------------------------------------------------+
```

**Layout-Details (480x320):**

- Y 0–30: Header "DIAGNOSE" (24px Bold) + Self-Test-Button rechts
- Y 30–250: Sensor-Liste (12px, monospaced-look)
  - Jede Zeile: Sensor-Name, Modul, Status-Icon, Live-Wert
  - Status: gruenes Haekchen fuer OK, rotes X fuer Fehler, gelbes "?" fuer
    unbekannt/nicht verbunden
- Y 250–290: Fehlercode-Bereich
  - Liste aktiver Fehler oder "Keine aktiven Fehler"
  - Jeder Fehler: Code + Kurzbeschreibung + Zeitstempel
- Y 290–320: Navigation-Footer (3 Menuepunkte, einer aktiv markiert)

**Self-Test-Animation:**

Wenn der Self-Test ausgeloest wird:
1. Button wird zu "TESTE..." (gelb)
2. Jede Sensor-Zeile wird nacheinander hervorgehoben (Hintergrund auf
   `COLOR_BG_RAISED`), der Test laeuft fuer diesen Sensor (200ms pro Sensor).
3. Bei Erfolg: Status wechselt auf gruen mit Haekchen.
4. Bei Fehler: Status wechselt auf rot mit X, Fehler-Toast erscheint.
5. Nach allen Sensoren: "Self-Test abgeschlossen" Toast, Button zurueck auf
   "SELF-TEST".

### 3.5 Settings-Screen

Konfiguration des Bordcomputers. Liste von Einstellungs-Kategorien.

```
+--------------------------------------------------+
|  EINSTELLUNGEN                      [< Zurueck]  |
|--------------------------------------------------|
|                                                  |
|  > Display                                       |
|    Helligkeit        [=======|---] 70%           |
|    Dark/Light Mode   [Dark]                      |
|    Auto-Dim          [AN]                        |
|                                                  |
|  > Sensoren                                      |
|    GPS-Rate          [10 Hz]                     |
|    RPM-Redline       [8000]                      |
|    Shift-Light        [7000]                      |
|    BMP-QNH           [1013 hPa]                  |
|                                                  |
|  > Bluetooth                                     |
|    Geraetename       [Transalp-BC]               |
|    Pairing-Liste     [2 Geraete]                |
|    Verbindung        [Verbunden: Pixel 7]       |
|                                                  |
|  > Logging                                       |
|    Format            [CSV + Binary]             |
|    Intervall         [5 s]                       |
|    SD-Karte          [8 GB, 2.1 GB frei]        |
|                                                  |
|  > System                                        |
|    Firmware          [v1.2.0]                   |
|    OTA-Update        [Nach Updates suchen]      |
|    Werkseinstellung  [Reset]                    |
|                                                  |
+--------------------------------------------------+
```

**Layout-Details (480x320):**

- Y 0–30: Header + Zurueck-Button
- Y 30–320: Scrollbare Liste von Einstellungen
  - Jede Kategorie hat einen Header (16px Bold, `COLOR_FG_SECONDARY`)
  - Jede Einstellung ist eine Zeile (16px Regular): Label + Wert/Control
  - Steuerung ueber Lenker-Taster: Up/Down navigiert, Select aendert Wert,
    Long-Select oeffnet Sub-Menu

**Value-Change-Animation:**

Wenn ein Wert geaendert wird (z.B. Helligkeit von 60% auf 70%):
- Der alte Wert rollt nach links heraus, der neue rollt von rechts ein
  (wie Speed, 100ms, `ease_out`).
- Bei Slider-Werten (Helligkeit): der Slider-Thumb slidet zur neuen Position
  (150ms, `ease_out`), der Fuellbalken folgt.

### 3.6 Tour-Statistik-Screen

Zusammenfassung nach Tour-Ende. Wird angezeigt, wenn die Tour beendet wird
(Zuendung aus oder manueller Tour-Stop).

```
+--------------------------------------------------+
|  TOUR-STATISTIK                      13.10.2026  |
|--------------------------------------------------|
|                                                  |
|         142 km         3h 24min                  |
|        Distanz         Dauer                     |
|                                                  |
|  Durchschnitt       68 km/h                      |
|  Max-Speed         128 km/h                       |
|  Max-RPM          7800                           |
|                                                  |
|  Hoehe                                             |
|    Start      412 m                              |
|    Max        1.247 m                             |
|    Aufstieg     +950 m                           |
|    Abstieg      -820 m                           |
|                                                  |
|  Verbrauch        ca. 5.2 L  (34 km/L)          |
|  Kraftstoff       -3/4 Tank                      |
|                                                  |
|  [GPX exportieren]   [Statistik teilen]         |
|                                                  |
+--------------------------------------------------+
```

**Layout-Details (480x320):**

- Y 0–30: Header mit Datum
- Y 30–100: Hero-Bereich mit zwei grossen Werten nebeneinander
  - Distanz (48px Bold) + Label "Distanz" (12px)
  - Dauer (48px Bold) + Label "Dauer" (12px)
- Y 100–180: Sekundaer-Statistiken (16px, 2 Spalten)
  - Durchschnitt, Max-Speed, Max-RPM
- Y 180–260: Hoehen-Profil
  - Mini-Balkendiagramm der Hoehenverteilung (8 Balken, 30px hoch)
  - Zahlen: Start, Max, Aufstieg, Abstieg
- Y 260–320: Export/Teilen-Buttons

**Entry-Animation (Screen erscheint):**

Die Statistik-Werte "zaehlen hoch" (Count-Up-Animation):
- Distanz: von 0 auf 142 km in 800ms, `ease_out` (0.25, 0.46, 0.45, 0.94).
- Dauer: von 0 auf 3h24min in 800ms (parallel).
- Die Hoehen-Balken wachsen von unten nach oben, gestaffelt (jeder Balken
  100ms versetzt, 300ms pro Balken, `ease_out`).
- Sekundaer-Werte erscheinen mit Fade-In, gestaffelt nach unten.

Diese Entry-Animation macht die Statistik zu einem "Moment" — das
Tour-Ende wird als bewusster Abschluss inszeniert, nicht als blosser
Daten-Dump.

---

## 4. E-Paper vs. TFT: Animation-Strategie

### 4.1 Grundsaetzliche Trennung

Das E-Paper-Display und das TFT-Display haben grundverschiedene
Eigenschaften und erfordern unterschiedliche Strategien:

| Eigenschaft | TFT (480x320) | E-Paper (250x122) |
|---|---|---|
| Refresh-Rate | 15–30 FPS | 0.3–2 s (Partial), 3 s (Full) |
| Farben | RGB565 (65536) | 1-Bit (schwarz/weiss) |
| Stromverbrauch | Hoch (Backlight) | Sehr gering (nur bei Refresh) |
| Sonnenlesbarkeit | Bei 1000 nits gut | Exzellent (reflektiv) |
| Ghosting | Nein | Ja (bei haeufigem Partial-Refresh) |
| Animationen | Ja, alle | Nein, nur statische Updates |

**Strategie: TFT fuer Dynamik, E-Paper fuer Persistenz.**

Das TFT ist das primaere Anzeigedisplay fuer alle dynamischen Werte:
Geschwindigkeit, Drehzahl, Gang, Navigation, Animationen. Alle in Abschnitt 2
beschriebenen Animationen laufen ausschliesslich auf dem TFT.

Das E-Paper ist das sekundaere Display fuer persistente, sich langsam
aendernde Werte. Es ist immer an (auch wenn das TFT ausgeschaltet ist, z.B.
bei Power-Save) und zeigt die Kerninstrumente in statischer Form.

### 4.2 Inhaltsverteilung

**TFT (480x320) — dynamischer Inhalt:**
- Geschwindigkeit (rollende Zahl)
- Drehzahl (animierter Balken)
- Gang (Slide-In)
- Navigation (Pfeil-Morphing, Distanz, Strassenname)
- Animationen, Transitionen, Overlays
- Boot-Animation
- Diagnose (mit Self-Test-Animation)
- Statistik (mit Count-Up)

**E-Paper (250x122) — statischer Inhalt:**
- Geschwindigkeit (statisch, aktualisiert alle 2s)
- Drehzahl (statischer Balken, aktualisiert alle 1s)
- Gang (statisch)
- Kraftstoffstand (statisch)
- Uhrzeit (statisch, aktualisiert jede Minute)
- Hoehe (statisch, aktualisiert alle 5s)
- Bordspannung (statisch, aktualisiert alle 5s)
- Tour-Modus / Status-Text

### 4.3 E-Paper Refresh-Strategie

**Partial-Refresh (0.3s):**
- Verwendet fuer Werte, die sich haeufig aendern (Speed, RPM).
- Nur der geaenderte Bereich wird aktualisiert.
- Ghosting-Schutz: alle 50 Partial-Refreshs ein Full-Refresh erzwingen.

**Full-Refresh (3s):**
- Verwendet beim Screen-Wechsel (E-Paper zeigt anderen Screen).
- Alle 50 Updates automatisch (Ghosting-Reset).
- Bei Temperatur < 10°C: Full-Refresh haeufiger (alle 20 Updates), da
  Partial-Refresh bei Kaelte unzuverlaessig wird.

**Refresh-Trigger:**
- Nicht zeitbasiert (alle X Sekunden), sondern wertbasiert: Refresh nur, wenn
  sich der Wert signifikant geaendert hat.
- Speed: Refresh bei Aenderung >= 1 km/h.
- RPM: Refresh bei Aenderung >= 100 rpm.
- Kraftstoff: Refresh bei Aenderung >= 5%.
- Uhrzeit: Refresh jede volle Minute.

Das reduziert die Anzahl der Refreshs drastisch. Bei konstanter Geschwindigkeit
auf der Autobahn aktualisiert sich das E-Paper kaum, was Strom spart und
Ghosting minimiert.

### 4.4 Dual-Display-Rendering-Pipeline

```
    +-------------------+
    |   sensor_task     |
    |   (Core 1)        |
    +--------+----------+
             |
    +--------v----------+
    |   data_queue      |
    +--------+----------+
             |
    +--------v----------+        +-------------------+
    |   display_task    |        |   epaper_task     |
    |   (Core 0)        |        |   (Core 0)       |
    |                   |        |                   |
    |  LVGL render      |        |  GxEPD2 render   |
    |  TFT framebuffer  |        |  E-Paper buffer  |
    |  30 FPS target    |        |  Wertbasiert     |
    +--------+----------+        +---------+---------+
             |                             |
    +--------v----------+        +---------v---------+
    |   SPI2 (FSPI)    |        |   SPI3 (HSPI)     |
    |   TFT ILI9488    |        |   E-Paper SSD1680 |
    +-------------------+        +-------------------+
```

Beide Displays haben eigene Tasks und eigene SPI-Busse. Der `display_task`
rendert mit LVGL auf das TFT (30 FPS Ziel, Partial-Refresh). Der `epaper_task`
rendert mit GxEPD2 auf das E-Paper (wertbasiert, 0.3–3s pro Update).

Wichtig: Beide Tasks lesen aus der gleichen Daten-Queue, aber mit
unterschiedlichen Raten. Der `display_task` liest bei jedem Frame (30 Hz),
der `epaper_task` liest alle 500ms und prueft, ob sich Werte signifikant
geaendert haben.

---

## 5. Performance-Budget

### 5.1 FPS-Ziel und Realitaet

**Theoretische maximale Framerate auf ESP32-S3 mit 480x320 TFT:**

Bei 80 MHz SPI-Takt und RGB565 (2 Bytes pro Pixel):
- Full-Screen-Refresh: 80.000.000 / (480 * 320 * 2) = ca. 26 FPS theoretisch
- Praxis: 15–20 FPS wegen SPI-Overhead, DMA-Latenz, Task-Scheduling
- Mit LVGL Partial-Refresh (nur geaenderte Bereiche): effektiv 30+ FPS
  bei kleinen Updates, 15–20 FPS bei grossflaechigen Aenderungen

**Ziel: 30 FPS fuer UI-Interaktionen, 15 FPS fuer kontinuierliche Werte.**

Die Differenzierung ist wichtig: eine Screen-Transition oder ein Pfeil-Morph
soll fluessig sein (30 FPS), aber die rollende Geschwindigkeitszahl kann
mit 15 FPS laufen, da das menschliche Auge bei sich schnell aendernden Zahlen
keine 30 FPS wahrnimmt.

### 5.2 Framebuffer-Groessen

| Display | Aufloesung | Farbtiefe | Framebuffer-Groesse | Speicherort |
|---|---|---|---|---|
| TFT 480x320 | 480x320 | RGB565 (16-bit) | 307.200 Bytes (300 KB) | PSRAM |
| E-Paper 250x122 | 250x122 | 1-Bit | 3.813 Bytes (3.7 KB) | Internes SRAM |
| E-Paper 296x128 | 296x128 | 1-Bit | 4.736 Bytes (4.6 KB) | Internes SRAM |
| TFT Draw-Buffer | 480x320 | RGB565 | 76.800 Bytes (75 KB, 1/4) | Internes SRAM (DMA) |

**Strategie:**
- Der TFT-Framebuffer (300 KB) liegt im PSRAM. ESP-IDF `esp_lcd` mit
  Bounce-Buffer uebertraegt in Bloecken vom PSRAM in interne SRAM und per DMA
  an das Display.
- Der LVGL Draw-Buffer (1/4 Screen = 75 KB) liegt im internen SRAM fuer
  DMA-faehigen Zugriff. Das ist gross genug fuer effizientes Partial-Rendering.
- Der E-Paper-Buffer (3.7–4.6 KB) liegt im internen SRAM. GxEPD2 braucht
  zusaetzlich einen Shadow-Buffer gleicher Groesse (Differentielles Update),
  also insgesamt 7.4–9.2 KB — immer noch problemlos im internen SRAM.

### 5.3 Double-Buffering

**TFT: Single-Framebuffer + Draw-Buffer (kein echtes Double-Buffering)**

LVGL rendert in den Draw-Buffer (internes SRAM), kopiert das Ergebnis in den
PSRAM-Framebuffer, und der `esp_lcd`-Treiber uebertraegt denFramebuffer per
DMA mit Bounce-Buffer an das Display. Das ist kein klassisches Double-
Buffering, aber durch die Trennung von Render-Buffer und Transfer-Buffer
entsteht kein Tearing (der Transfer liest aus dem PSRAM-Framebuffer, waehrend
LVGL in den Draw-Buffer schreibt).

**E-Paper: Shadow-Buffer (Differential)**

GxEPD2 haelt zwei Buffer: den aktuellen Framebuffer und einen Shadow-Buffer
mit dem vorherigen Zustand. Bei Partial-Refresh vergleicht GxEPD2 beide und
sendet nur die Differenz an das Display. Das ist kein Double-Buffering im
Sinne von Render-vs-Display, sondern ein Differenz-Buffer.

### 5.4 Render-Pipeline: Statisch vs. Dynamisch

**Statische Elemente (werden einmal gerendert, nicht pro Frame):**
- Screen-Hintergrund (`COLOR_BG_VOID`)
- Panel-Hintergruende (`COLOR_BG_PANEL`)
- Header und Footer-Bereiche
- Icons (sofern sie sich nicht aendern)
- Labels, die sich nicht aendern ("km/h", "rpm", etc.)
- Statische Linien und Rahmen

**Dynamische Elemente (werden pro Frame neu gerendert):**
- Speed-Zahl (rollt)
- RPM-Balken (Fuellstand aendert sich)
- Gang-Zahl (wechselt)
- Navigations-Pfeil (morphed)
- Distanz-Zahl (aendert sich)
- Status-Icons (BT, GPS, Akku — aendern sich gelegentlich)
- Animations-Overlays (Warnungen, Toasts)

**LVGL Partial-Refresh-Strategie:**

LVGL markiert nur die Bereiche als "dirty" (invalidiert), in denen sich etwas
geaendert hat. Der Render-Task rendert nur diese Bereiche neu in den
Draw-Buffer und uebertraegt nur diese Bereiche an das Display. Das reduziert
die pro Frame zu uebertragende Datenmenge drastisch.

Beispiel: Wenn sich nur die Speed-Zahl (72px Bold, ca. 100x80px Bereich)
aendert, werden nur 100x80x2 = 16.000 Bytes uebertragen (statt 307.200 Bytes
fuer den ganzen Screen). Bei 80 MHz SPI sind das 0.2ms Uebertragungszeit
(statt 3.8ms fuer Full-Screen). Das ermoeglicht effektiv 30+ FPS fuer
kleine Updates.

**Dirty-Region-Management:**

```c
// LVGL invalidiert nur geaenderte Objekte
// Beispiel: Speed-Zahl aendert sich
lv_obj_t *speed_label = ...;
lv_label_set_text(speed_label, "99");
// lv_label_set_text invalidiert automatisch den Bereich des Labels
// LVGL rendert nur diesen Bereich im naechsten Frame neu
```

### 5.5 CPU-Last-Schaetzung

| Task | Core | CPU-Last (Schaetzung) | Beschreibung |
|---|---|---|---|
| `display_task` | 0 | 30–50% | LVGL render + SPI-Transfer |
| `epaper_task` | 0 | 2–5% | GxEPD2, wertbasiert |
| `ble_task` | 0 | 5–10% | NimBLE Stack |
| `logger_task` | 0 | 2–5% | Batch-Write SD |
| `gps_task` | 1 | 3–5% | NMEA parsen |
| `sensor_task` | 1 | 10–15% | I2C lesen, Fusion |
| `can_task` | 1 | 2–5% | TWAI RX |
| `power_task` | 0 | 1–2% | ADC, Power-Mgmt |

**Bottleneck: `display_task` auf Core 0.**

Bei 30 FPS und einem durchschnittlichen Partial-Refresh von 20% der
Bildschirmflaeche (ca. 60.000 Bytes zu rendern und zu uebertragen) liegt die
CPU-Last fuer den `display_task` bei 30–50%. Das laesst genug Spielraum fuer
BLE und Logger auf Core 0. Core 1 ist mit den Real-Time-Tasks (GPS, Sensoren)
bei ca. 20–25% Auslastung.

**Optimierungspotenziale:**

1. **Sprite-Caching**: Statisch gerenderte Elemente (Icons, Panels) werden
   einmal gerendert und als Bitmap gecacht. Bei einem Partial-Refresh wird
   das Caching genutzt, statt das Element neu zu zeichnen.
2. **Font-Caching**: Die Glyphen-Bitmaps werden einmal beim Laden
   dekomprimiert und gecacht. Wiederholte Renders der gleichen Glyphe
   sind dann nur ein Memcpy.
3. **Dirty-Region-Aggregation**: Wenn mehrere kleine Bereiche invalidiert
   sind, werden sie zu einer groesseren rechteckigen Region zusammengefasst,
   um die Anzahl der SPI-Transfers zu reduzieren (ein grosser Transfer ist
   effizienter als viele kleine).
4. **DMA-Prioritaet**: Der `display_task` hat Prioritaet 2 (niedrig). Bei
   hohem Sensor-Traffic kann die Framerate auf 15 FPS fallen. Das ist
   akzeptabel — Sensor-Daten haben Vorrang vor UI-Fluessigkeit.

---

## 6. Companion-App UI-Konzept

### 6.1 Grundprinzipien

Die Companion-App (Flutter, beide Smartphone-Plattformen) ist die Schnittstelle zwischen
Smartphone und Bordcomputer. Sie muss sich nativ anfuehlen — nicht wie eine
Web-App. Die Design-Sprache des Bordcomputers wird in die App uebernommen:
Dark-Mode synchron, gleiche Farbpalette, gleiche Typografie-Hierarchie,
gleiche Icons.

**Native Look-and-Feel:**
- Auf der einen Plattform: Verwendung von `Cupertino`-Widgets fuer Navigation und Controls.
- Auf der anderen Plattform: Verwendung von `Material 3`-Widgets.
- Beide Plattformen teilen die gleiche Farbpalette und Typografie.
- Keine generische "One-UI-fits-all"-Loesung. Plattformspezifische
  Anpassungen in der Navigations-Struktur, aber gemeinsames Visuelles.

### 6.2 App-Struktur

Die App hat fuenf Hauptbereiche, ueber eine Bottom-Navigation-Bar erreichbar:

1. **Dashboard**: Live-Telemetrie vom Bordcomputer
2. **Navigation**: Routen-Planung und Senden an Bordcomputer
3. **Statistik**: Tour-Historie mit Charts
4. **Konfiguration**: Bordcomputer-Einstellungen
5. **Geraet**: Verbindung, OTA, Diagnose

### 6.3 Dashboard-Screen

```
+------------------------------------------+
|  TRANSALP BC              [BT: Verbunden] |
|------------------------------------------|
|                                          |
|              98                          |
|             km/h                         |
|                                          |
|  +-------+ +-------+ +-------+ +------+ |
|  | 4500  | | 22 C  | | 1/2   | | 12.4V|| |
|  | RPM   | | TEMP  | | FUEL  | | VOLT || |
|  +-------+ +-------+ +-------+ +------+ |
|                                          |
|  ROLL: +5.2    PITCH: -1.8    842 m     |
|                                          |
|  GPS: 9 SAT   HDOP: 1.2   50.93N 11.58E |
|                                          |
|------------------------------------------|
|  [Dashboard] [Nav] [Stats] [Config] [Ger]|
+------------------------------------------+
```

Das Dashboard spiegelt den Instrument-Screen des Bordcomputers in der App
wider. Die Werte werden per BLE live empfangen (Telemetry-Service). Die
Farbcodierung ist identisch: RPM in Farbverlauf, Temperatur rot ab 110°C,
etc.

**Live-Telemetrie-Updates:**

- BLE-Notify alle 200ms (5 Hz) fuer Speed/RPM.
- BLE-Notify alle 1s fuer Sensoren (Temp, Fuel, Volt, Roll, Pitch).
- GPS-Daten alle 500ms.
- Die App interpoliert zwischen Updates (wie der Bordcomputer selbst), um
  fluessige Anzeige zu gewaehrleisten.

### 6.4 Navigation-Screen

```
+------------------------------------------+
|  NAVIGATION               [< Zurueck]    |
|------------------------------------------|
|                                          |
|  Von: [Aktuelle Position]                |
|  Nach: [Ziel eingeben...]                |
|                                          |
|  ----------------------------------------|
|                                          |
|  Route: Jena -> Weimar                   |
|  Distanz: 24 km   Dauer: 28 min          |
|                                          |
|  +------------------------------------+  |
|  | 1. In 340 m rechts abbiegen       |  |
|  |    auf Bundesstrasse 7            |  |
|  | 2. In 1.2 km geradeaus            |  |
|  |    auf Bundesstrasse 7 bleiben    |  |
|  | 3. In 3.5 km links abbiegen       |  |
|  |    auf Ernst-Abbe-Strasse         |  |
|  +------------------------------------+  |
|                                          |
|  [Route an Bordcomputer senden]          |
|                                          |
|------------------------------------------|
|  [Dashboard] [Nav] [Stats] [Config] [Ger]|
+------------------------------------------+
```

Die Routenplanung nutzt die native Maps-API des jeweiligen Smartphones.
Die App berechnet die Route und extrahiert die
Maneuver-Liste. Beim "Senden an Bordcomputer" wird die Maneuver-Liste per
BLE an den Bordcomputer geschickt (Navigation-Service, Next-Turn
Characteristic).

### 6.5 Statistik-Screen

```
+------------------------------------------+
|  STATISTIK                               |
|------------------------------------------|
|                                          |
|  Letzte Tour: 13.10.2026                 |
|  142 km   3h 24min   68 km/h avg         |
|                                          |
|  +------------------------------------+  |
|  |     Hoehenprofil                   |  |
|  |  1247 +------^-----                |  |
|  |       |    |    \                  |  |
|  |   842 +-+--+     \--               |  |
|  |       |               \            |  |
|  |   412 +---+--+---------+--          |  |
|  |       0   35  70  105  142 km      |  |
|  +------------------------------------+  |
|                                          |
|  Geschwindigkeitsverteilung:             |
|  0-50    [====]           18%            |
|  50-80   [========]      32%            |
|  80-100  [======]        24%            |
|  100-120 [====]          16%            |
|  120+    [==]             10%            |
|                                          |
|  Alle Touren: 1.247 km in 8 Touren      |
|                                          |
|------------------------------------------|
|  [Dashboard] [Nav] [Stats] [Config] [Ger]|
+------------------------------------------+
```

Die Statistik zeigt Diagramme (Hoehenprofil als Line-Chart,
Geschwindigkeitsverteilung als Balkendiagramm). Die Charts werden mit
`fl_chart` (Flutter Chart Library) gerendert. Die Daten werden entweder live
vom Bordcomputer per BLE heruntergeladen oder von der SD-Karte exportiert.

### 6.6 Konfigurations-Editor

```
+------------------------------------------+
|  KONFIGURATION                            |
|------------------------------------------|
|                                          |
|  Display                                 |
|    Helligkeit        [=======|---] 70%   |
|    Dark Mode         [AN]                |
|    Auto-Dim          [AN]                |
|                                          |
|  Sensoren                                |
|    GPS-Rate          [10 Hz]             |
|    RPM-Redline       [8000]              |
|    Shift-Light       [7000]              |
|    BMP QNH           [1013 hPa]          |
|                                          |
|  BLE                                     |
|    Geraetename       [Transalp-BC]       |
|    Pairing-Liste     [2 Geraete]        |
|                                          |
|  Logging                                 |
|    Format            [CSV + Binary]     |
|    Intervall         [5 s]              |
|                                          |
|  [Auf Bordcomputer anwenden]              |
|                                          |
|------------------------------------------|
|  [Dashboard] [Nav] [Stats] [Config] [Ger]|
+------------------------------------------+
```

Der Konfigurations-Editor spiegelt die Settings des Bordcomputers. Aenderungen
werden lokal in der App gespeichert und erst beim "Auf Bordcomputer anwenden"
per BLE (Config-Service) geschrieben. Das erlaubt Offline-Bearbeitung und
verhindert haeufige BLE-Transfers.

### 6.7 Geraet-Screen

```
+------------------------------------------+
|  GERAET                                  |
|------------------------------------------|
|                                          |
|  Verbindung: Verbunden                   |
|  Geraet: Transalp-BC                     |
|  Firmware: v1.2.0                        |
|  Uptime: 4h 23min                        |
|  Akku: 78%                               |
|                                          |
|  SD-Karte: 8 GB, 2.1 GB frei            |
|  Letzter Track: 13.10.2026 14:20        |
|                                          |
|  [Firmware-Update pruefen]               |
|  [Tracks herunterladen]                  |
|  [Diagnose ausfuehren]                   |
|  [Verbindung trennen]                    |
|                                          |
|------------------------------------------|
|  [Dashboard] [Nav] [Stats] [Config] [Ger]|
+------------------------------------------+
```

Der Geraet-Screen zeigt Verbindungsstatus, Firmware-Version, Uptime und
bietet Funktionen: OTA-Update, Track-Download, Diagnose-Trigger.

**OTA-Update-UI:**

Beim Firmware-Update:
- Progress-Bar mit Prozentanzeige.
- Status-Text: "Uebertrage... 45%" → "Installiere..." → "Neustart...".
- Bei Fehler: klare Fehlermeldung und "Erneut versuchen"-Button.
- Waehrend des Updates: Warnung, dass das Geraet nicht vom Strom getrennt
  werden darf.

### 6.8 App-Animationen

Die App uebernimmt die Animations-Philosophie des Bordcomputers:
- Screen-Transitions: Slide von rechts (vorwaerts), von links (rueckwaerts),
  250ms, `ease_in_out`.
- Werte-Aenderungen: sanftes Ueberblenden, kein harter Sprung.
- Verbindungsaufbau: BT-Icon pulsiert (wie Bordcomputer).
- Chart-Entry: Balken und Linien wachsen von unten/links, gestaffelt, mit
  `ease_out` (300ms pro Element, 50ms Versatz).

Die App kann 60 FPS (Smartphone-Standard), aber die Animationsdauern sind
bewusst aehnlich zum Bordcomputer (80–400ms), um ein konsistentes Gefuehl
zwischen Geraet und App zu erzeugen.

---

## 7. Zusammenfassung der Design-Entscheidungen

### 7.1 Farbpalette (Komplett)

**Basis:**
- Hintergrund: #000000 (Void), #0A0A0C (Deep), #1C1C1E (Panel), #2C2C2E
  (Raised), #3A3A3C (Overlay)
- Vordergrund: #FFFFFF (Primary), #8E8E93 (Secondary), #48484A (Tertiary)

**Akzente:**
- Amber/Orange: #FF9500, #FF6B00 (RPM)
- Rot: #FF3B30 (Warnungen, Redline)
- Blau: #0A84FF (Info, Navigation, BT)
- Gruen: #30D158 (Erfolg, GPS)
- Cyan: #40C8E0 (Telemetrie)
- Gelb: #FFD60A (Shift-Light, Hinweis)

### 7.2 Font-Hierarchie (Komplett)

- 72px Bold: Geschwindigkeit
- 48px Bold: Navigation-Distanz, Tour-Hero-Werte
- 36px SemiBold: Drehzahl digital
- 24px Bold: Headings, Screen-Titel, Strassenname
- 16px Regular: Body, Sekundaerwerte
- 12px Medium: Labels, Einheiten
- 10px Regular: Status-Leiste, Mini-Labels

Font-Familie: Inter (Open Source), Fallback: Roboto. Beide als Bitmap-Fonts
mit dem LVGL Font Converter beim Build generiert.

### 7.3 Animations-Timings (Komplett)

| Animation | Dauer | Timing-Kurve | Beschreibung |
|---|---|---|---|
| RPM-Balken Aufwaerts | 120ms | ease_out | Schnelles Fuellen |
| RPM-Balken Abwaerts | 250ms | ease_in_out | Traegheitsabfall |
| RPM-Balken Bounce | 150ms | ease_out_back | Overshoot beim Abfall |
| Speed-Ziffernwechsel | 100ms | ease_out | Roll nach oben/unten |
| Gang-Wechsel | 200ms | ease_out_back | Slide-In mit Bounce |
| Kraftstoff-Update | 400ms | ease_in_out | Sanfte Fuellung |
| Nav-Pfeil Morph | 300ms | ease_in_out | Polygon-Interpolation |
| Nav-Pfeil Vergroesserung | 800ms | ease_out_back | Bei nahem Maneuver |
| Screen-Transition (Slide) | 250ms | ease_in_out | Links/rechts |
| Screen-Transition (Fade) | 200ms | ease_out | Overlays |
| Toast Slide-In | 200ms | ease_out | Von oben |
| Toast Slide-Out | 150ms | ease_in | Nach oben |
| Boot-Sequenz | 2000ms | Various | Choreografiert |
| Count-Up (Statistik) | 800ms | ease_out | Werte zaehlen hoch |
| Chart-Entry (App) | 300ms | ease_out | Balken wachsen |

### 7.4 Screen-Uebersicht

| Screen | Display | Primaerer Inhalt |
|---|---|---|
| Instrument | TFT + E-Paper | Speed, RPM, Gang, Temp, Fuel, Volt |
| Navigation | TFT | Pfeil, Distanz, Strassenname, ETA |
| Split | TFT | Instrumente + Navigation |
| Diagnose | TFT | Sensor-Status, Fehlercodes, Self-Test |
| Settings | TFT | Konfigurations-Liste |
| Tour-Statistik | TFT | Distanz, Dauer, Geschwindigkeit, Hoehe |

### 7.5 E-Paper vs. TFT Zusammenfassung

- TFT: Alle Animationen, 30 FPS (15 bei hohem Sensor-Traffic), RGB565,
  PSRAM-Framebuffer, primaer fuer dynamische Werte
- E-Paper: Statische Updates, wertbasiert, 1-Bit, internes SRAM, primaer
  fuer persistente Werte und Always-On-Anzeige
- Beide haben eigene Tasks und SPI-Busse, lesen aus gemeinsamer Daten-Queue

---

## Quellen

- LVGL 9 Dokumentation: https://docs.lvgl.io/9/
- LVGL Font Converter: https://github.com/lvgl/lv_font_conv
- ESP-IDF esp_lcd (Framebuffer, Bounce Buffer):
  https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s3/api-reference/peripherals/lcd.html
- GxEPD2 (E-Paper): https://github.com/ZinggJM/GxEPD2
- esp_lvgl_port: https://github.com/espressif/esp-bsp/tree/master/components/lvgl_port
- Inter Font (Open Source): https://github.com/rsms/inter
- Roboto Font: https://github.com/googlefonts/roboto
- Flutter fl_chart: https://github.com/imaNNeo/fl_chart
- CSS Timing Functions (Bezier-Kurven Referenz):
  https://developer.mozilla.org/en-US/docs/Web/CSS/easing-function
- E-Paper Partial-Refresh Limits (Ghosting, Temperatur):
  https://www.waveshare.com/wiki/2.13inch_e-Paper_HAT

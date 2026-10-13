# Gehäuse-Design: 3D-Druck-Konzept, Montage, Wasserfestigkeit

> Stand: Oktober 2026 — Konzept für das Bordcomputer-Gehäuse der Honda Transalp 600 (PD06).
> Grundlage: docs/research/05-gehaeuse-einbau-emv.md und docs/research/02-hardware-deep-dive.md.
> Alle Marken- und Produktnamen wurden vermieden; es wird nur generisch auf Komponenten-Klassen verwiesen.

---

## Inhaltsverzeichnis

1. [Gehäuse-Konzept](#1-gehäuse-konzept)
2. [Montage am Motorrad](#2-montage-am-motorrad)
3. [Display-Anordnung](#3-display-anordnung)
4. [Anschluss-Bereich](#4-anschluss-bereich)
5. [Wasserfestigkeit](#5-wasserfestigkeit)
6. [Innenleben](#6-innenleben)
7. [EMV-Schirmung](#7-emv-schirmung)
8. [3D-Druck-Empfehlungen](#8-3d-druck-empfehlungen)
9. [ASCII-Schnittzeichnungen](#9-ascii-schnittzeichnungen)

---

## 1. Gehäuse-Konzept

### 1.1 Formfaktor und Fahrzeuganpassung

Das Gehäuse wird spezifisch für die Honda Transalp 600 (PD06, Baujahr 1986–2000)
entwickelt. Die PD06 hat einen Standard-Lenker mit 22 mm Durchmesser. Häufig
ist eine Querstange (Crossbar) zwischen den Lenkerhaltern vorhanden oder kann
nachgerüstet werden. Das Gehäuse ist so konzipiert, dass es entweder direkt am
22-mm-Lenker oder an einer Crossbar montiert werden kann.

Der Formfaktor orientiert sich an den Abmessungen der internen Komponenten:

| Komponente | Abmessung (mm) | Hinweis |
|---|---|---|
| ESP32-S3-DevKitC-1 | 70 × 52 | Prototyp-Phase |
| Custom PCB (Phase 2) | ~50 × 40 | WROOM-1 Modul |
| E-Paper-Display 2,9" | 89 × 38 (Modul) | 296 × 128 Pixel |
| GPS-Modul (Breakout) | ~30 × 25 | Externe Antenne |
| IMU + Baro (combined) | ~25 × 15 | I2C-Bus |
| SD-Karten-Halter | ~15 × 15 | SPI-Bus |
| DC/DC-Wandler-Modul | 22 × 15 | Buck-Converter |
| LiPo-Akku (1000–2000 mAh) | ~50 × 30 × 8 | Pufferakku |
| Relais-Modul (4-Kanal) | 75 × 25 | Optional, extern |

Daraus ergibt sich ein erforderliches Innenvolumen von ca. 85 × 60 × 35 mm
für die Hauptplatine und alle internen Module. Zuzüglich Wandstärke, Display-
Aussparung und Kabeldurchführungen resultiert das in folgenden Außenabmessungen:

**Außenabmessungen (Prototyp):**

```
Breite:  100 mm
Höhe:     80 mm
Tiefe:    45 mm (inkl. Display-Vertiefung und Kabelanschlüsse)
```

Das Gesamtgewicht muss unter 400 g bleiben, um die Lenkergeometrie der
Transalp nicht zu beeinträchtigen und Resonanzschwingungen am Lenker
nicht zu verstärken. Mit ASA-Gehäuse (~80 g), Elektronik (~60 g),
Akku (~40 g), Display (~20 g) und Kabeldurchführungen (~15 g) liegt
das geschätzte Gesamtgewicht bei ca. 215 g — komfortabel innerhalb
des Limits.

### 1.2 Materialwahl

Das Gehäuse wird im FDM-Druckverfahren hergestellt. Die Materialwahl
entscheidet über Temperaturbeständigkeit, UV-Stabilität und mechanische
Robustheit im Motorrad-Einsatz.

| Material | Glasübergang (Tg) | UV-beständig | Kältebeständig | Druckbarkeit | Fazit |
|---|---|---|---|---|---|
| PLA | ~60°C | Nein | Schlecht | Einfach | Unbrauchbar (verformt in der Sonne) |
| PETG | ~80°C | Bedingt | Gut | Einfach | Minimum für Prototyp |
| ABS | ~105°C | Nein | Schlecht | Mittel | Bricht bei Kälte, UV-fragil |
| ASA | ~100°C | Ja | Sehr gut | Mittel | Empfohlen für finalen Einsatz |
| TPU | — | Ja | Sehr gut | Mittel | Nur für Dichtungen/Puffer |

**Primärmaterial: ASA** (Acrylnitril-Styrol-Acrylat) für das Außengehäuse.

ASA ist die Weiterentwicklung von ABS mit UV-Resistenz und besserer
Wetterbeständigkeit, speziell für Automobil-Außenanwendungen entwickelt.
Die Glasübergangstemperatur liegt bei ca. 98°C, die Heat Deflection
Temperature bei ca. 100°C. Ein hellfarbiges ASA (weiß oder hellgrau)
reflektiert Sonnenstrahlung und hält die Innentemperatur 10–15°C niedriger
als ein schwarzes Gehäuse.

Wichtige Eigenschaften von ASA:
- UV-resistent, keine Vergilbung über Jahre
- Dichte: ca. 1,13 g/cm³
- Charpy-Schlagzähigkeit: ca. 10 kJ/m² (in Z-Richtung: ca. 6,7 kJ/m²)
- Layer Adhesion ist die Schwachstelle: nur ca. 29% der Druckbett-Festigkeit
  in Z-Richtung. Wandstärken müssen dies kompensieren.

**Sekundärmaterial: PETG** für die innere Platte (Display-Rahmen,
PCB-Einschub-Schienen). PETG ist einfacher zu drucken, benötigt keine
beheizte Kammer, und hat mit 80°C eine ausreichende Temperaturgrenze
für interne Bauteile, die keiner direkten Sonneneinstrahlung ausgesetzt sind.

**Dichtungsmaterial: Silikon** für O-Ringe. Silikon-O-Ringe sind
UV-stabil und überstehen den Temperaturbereich von -40 bis +200°C.
EPDM-Schaumstoff wird als Sekundärdichtung für unebene Flächen verwendet.

### 1.3 Wandstärken und Versteifung

| Bauteil | Wandstärke | Perimeter (0,4 mm Düse) | Begründung |
|---|---|---|---|
| Außenwände | 2,4–3,2 mm | 6–8 | Strukturelle Integrität, Vibration |
| Deckel | 3,2 mm | 8 Layer (0,4 mm) | Steifigkeit, Wasserbarriere |
| Boden | 3,2 mm | 8 Layer | Steifigkeit, EMV-Träger |
| Schraubsäulen | 6 mm Außendurchmesser | — | M3-Gewinde eingeschmolzen |
| Kabeldurchführungen | 4 mm | — | Verstärkt im Bereich Cable-Glands |
| Display-Aussparung | 2,0 mm | 5 | Rahmen für Display-Abdeckung |

Versteifungsrippen werden an folgenden Stellen eingebracht:
- Zwischen den Schraubsäulen (Diagonalrippen, 1,2 mm dick, 3 mm hoch)
- Am Display-Rahmen (Umfassungsrippe, 1,5 mm dick)
- An der Halterungs-Schnittstelle (Verdickung auf 4 mm im Bereich
  der U-Bolt-Aufnahme)

### 1.4 Gehäuse-Topologie

Das Gehäuse besteht aus drei druckbaren Teilen:

1. **Unterteil (Basisschale):** Enthält PCB-Führungsschienen,
   Schraubsäulen, Kabeldurchführungen an der Unterseite, und die
   Halterungs-Schnittstelle an der Rückseite.

2. **Deckel:** Schließt das Gehäuse von oben ab, trägt den Display-
   Ausschnitt, die Druckausgleichs-Membran und die Taste. Der Deckel
   greift über das Unterteil (Shingling-Prinzip), so dass Wasser, das
   an der Naht entlangläuft, nach außen abfließt.

3. **Display-Rahmen (innen):** Ein separater Einsatz aus PETG, der
   das E-Paper-Modul fixiert und auf einem Silikon-Pad ruht. Wird
   mit dem Deckel verschraubt.

Die Naht zwischen Deckel und Unterteil liegt an der Rückseite/Unterseite,
nicht oben. Wasser, das von oben kommt, trifft auf eine geschlossene
Fläche. Schrauben greifen von unten oder von der Seite in das Unterteil,
niemals von oben (Wasser würde in die Schraubenlöcher laufen).

---

## 2. Montage am Motorrad

### 2.1 Halterungs-Optionen

Drei Montage-Optionen wurden evaluiert:

**Option A: Direkte Lenkerhalterung (22 mm)**
- U-Bolt-Schelle aus Metall um den 22-mm-Lenker
- Halterungs-Adapter aus ASA gedruckt
- Vorteil: einfach, schnell abnehmbar
- Nachteil: lenkerdrehend, weniger stabil für fest verbaute Elektronik

**Option B: Crossbar-Montage (empfohlen)**
- Befestigung an einer Querstange zwischen den Lenkerhaltern
- Die PD06 hat oft schon eine Crossbar serienmäßig oder nachgerüstet
- Gerät wird an der Crossbar befestigt
- Vorteil: stabilere Plattform, weniger Lenker-Drehmoment, bessere
  Positionierung

**Option C: Lenkerhalter-Klemmbrücke**
- Montage an den Lenkerhalter-Klemmen (M6 oder M8 Schrauben)
- Vorteil: sehr stabil, ohne Lenker zu belasten
- Nachteil: modellspezifisch, braucht angepasste Halterung

**Empfehlung: Option B (Crossbar)** mit einer kugelgelenkbasierten
Halterung (kompatibel mit dem Industriestandard für Motorrad-Halterungen).
Das erlaubt Neigung und Rotation für optimale Lesbarkeit und ist
mit den meisten am Markt verfügbaren Motorrad-Halterungen kompatibel.

### 2.2 Schwingungsdämpfung

Zwischen Gehäuse und Halterung werden Gummidämpfer eingefügt:

- Gummi-Metall-Verbunddämpfer (Shore 50–60A Härte)
- Reduzieren hochfrequente Vibrationen vom Motor (V2-Motor, 50–200 Hz)
- Entkoppeln das Gehäuse vom Lenker-Resonanzbereich (40–60 Hz)
- Standard: M6-Gewinde, 10 mm Gummi-Höhe
- Montage: Dämpfer zwischen Halterungs-Adapter und Gehäuserückseite

Die Transalp 600 produziert am Lenker ca. 5–10 g Vibration (V2-Motor,
verhältnismäßig moderat im Vergleich zu Einzylindern mit bis zu 30 g).
Trotzdem sind Gummidämpfer essenziell, um:
1. Mikrorisse im E-Paper-Glas-Substrat zu vermeiden
2. PCB-Befestigungen nicht zu lockern
3. Steckverbindungen nicht loszuvibrieren
4. Resonanzüberhöhung am Lenker zu vermeiden

### 2.3 Neigungsverstellung

Der Bordcomputer muss neigungsverstellbar sein, um verschiedene
Fahrergrößen und Sitzpositionen zu unterstützen:

- Neigungsbereich: 0° bis 15° nach hinten (vom Fahrer weg)
- Kugelgelenk mit Spannhebel (Werkzeuglos verstellbar)
- Nach der Einstellung arretierbar (Klemmhebel, kein Ratsche nötig)

Empfohlene Neigung: 10–15° nach hinten, damit:
- Bei tiefem Sonnenstand das Licht nicht direkt vom Display reflektiert wird
- Bei hohem Sonnenstand das Display von oben beleuchtet wird
- Regenwasser nicht auf dem Display stehen bleibt (Tropfenform)

### 2.4 Montage-Werkzeug

- Hauptschrauben: M6 Inbus (4 mm) für U-Bolt und Klemmbrücke
- Gehäuseschrauben: M3 Inbus (2,5 mm) für Deckel und PCB
- Klemmhebel für Kugelgelenk: Werkzeuglos von Hand
- Kabelverschraubungen: Handfest oder mit Maulschlüssel (14 mm für PG7)

### 2.5 Kabelführung am Motorrad

Die Kabel vom Bordcomputer zur Sensorik verlaufen:
- Entlang dem Rahmenrohr (rechte Seite, nicht an der Auspuff-Seite)
- Unter Verkleidungsteilen, wo vorhanden
- An Kabelbindern am Rahmen fixiert (alle 15–20 cm)
- Schrumpfschlauch über Kabelverbindungen als mechanischer Schutz
- Gummitüllen wo Kabel durch Verkleidungsöffnungen gehen

Wichtig: keine Kabel entlang der Auspuffanlage (Hitze) oder in der
Nähe des CDI-Moduls (EMV-Einstreuung).

---

## 3. Display-Anordnung

### 3.1 Primärdisplay: E-Paper 2,9"

Das Hauptdisplay ist ein E-Paper-Modul mit 2,9" Diagonale (296 × 128 Pixel,
monochrom). E-Paper wurde gewählt, weil:
- Sonnenlicht-Lesbarkeit exzellent ist (reflektiv, kein Backlight nötig)
- Stromverbrauch minimal ist (~0 mW zwischen Updates, ~26 mW beim Refresh)
- Leseinkel >170° beträgt
- Die Anzeige statischer Werte (Geschwindigkeit, Drehzahl, GPS-Info)
  keine schnellen Animationen erfordert

Das E-Paper-Modul wird frontal (in Fahrtrichtung) hinter einer
Klarsicht-Abdeckung montiert. Die Abdeckung besteht aus 2 mm Acrylglas
oder Polycarbonat und schützt das Display vor mechanischer Beschädigung
und direktem Wasserkontakt.

### 3.2 Sekundärdisplay (Optional)

Ein optionales TFT-Display (z.B. 1,14" bis 2,0" SPI TFT) kann ergänzend
für zeitkritische Anzeigen (Drehzahlanzeige mit Balkengraph, Echtzeit-
Neigungsanzeige) verwendet werden. Das TFT-Display wird unterhalb oder
neben dem E-Paper positioniert, ebenfalls hinter einer Klarsicht-Abdeckung.

Da TFT-Displays bei direkter Sonneneinstrahlung schlecht lesbar sind
(Backlight konkurriert mit Sonnenlicht), wird das TFT im Schatten des
Gehäusedeckels positioniert oder abgedunkelt.

### 3.3 Abdeckung und Wasserabweisung

Beide Displays sind hinter einer Klarsicht-Abdeckung montiert:
- Material: 2 mm Acrylglas (PMMA) oder Polycarbonat (PC)
- Befestigung: Eingeclipst in den Deckel, mit Silikon-Dichtmasse
  umlaufend versiegelt
- Wasserabweisende Beschichtung (hydrophob) auf der Außenseite
  der Abdeckung, damit Tropfen abperlen und die Sicht nicht beeinträchtigen

Die Abdeckung hat eine leichte Wölbung nach außen (konvex), damit:
- Wasser von der Mitte nach außen abläuft
- Die Display-Fläche keine waagerechte Fläche bildet, auf der Wasser
  stehen bleiben kann

### 3.4 Display-Befestigung innen

Das E-Paper-Modul wird wie folgt fixiert:
- Modul ruht auf einer 0,5 mm Silikon-Unterlage (Vibrations-Entkopplung)
- Display-Rahmen aus PETG umschließt das Modul seitlich
- Verschraubt mit M2,5 Schrauben und Federscheiben
- FPC-Kabel (Flexible Printed Circuit) mit etwas Spiel, nicht unter Spannung
- Display-Rahmen mit Rippen versteift, um Biegeschwingungen zu minimieren

Die Silikon-Unterlage ist wichtig, weil E-Paper-Displays ein Glas-Substrat
(0,3–0,5 mm dick) haben, das biegeschwingungs-empfindlich ist. Mikrorisse
im Glas oder an der FPC-Verbindung sind die typischen Ausfallursachen
bei Vibrationsbelastung.

---

## 4. Anschluss-Bereich

### 4.1 Kabeldurchführungen (Cable-Glands)

Alle Kabeldurchführungen befinden sich an der Unterseite des Gehäuses,
damit Wasser, das an den Kabeln entlangläuft, nach unten abtropft und
nicht ins Gehäuseinnere gelangt.

Verwendet werden Kabelverschraubungen (Cable-Glands) in PG- oder
M-Gewinde-Ausführung:

| Größe | Kabel-Ø | Gewinde-Ø | Einsatz |
|---|---|---|---|
| PG7 / M12 | 3–6,5 mm | 12,5 mm | Sensorkabel (einzelne Adern) |
| PG9 / M16 | 4–8 mm | 15,2 mm | Sensorkabel (mehrere Adern) |
| PG11 / M18 | 5–10 mm | 18,0 mm | Stromversorgung |
| PG13.5 / M20 | 6–11 mm | 20,1 mm | Display-Flachbandkabel (extern) |

Material: Polyamid (PA66) mit NBR-O-Ring und Kontermutter.
IP-Rating: IP67 oder IP68.

Die Cable-Glands werden von außen durch das Gehäuse gesteckt,
die Kontermutter innen angezogen. Ein O-Ring zwischen Gland und
Gehäusewand dichtet die Durchführung ab (nicht nur zwischen Gland und Kabel).

Der Kabel-Ø muss genau passend zum Gland gewählt werden:
- Zu dünn: Wasserweg zwischen Kabel und Gland-Dichtung
- Zu dick: Dichtung klemmt nicht richtig, Kabel gequetscht

### 4.2 USB-C-Buchse

Die USB-C-Buchse dient dem Service-Zugang (Konfiguration, Firmware-Update,
Log-Download). Sie ist als wasserdichte Panel-Mount-Buchse (IP67) ausgeführt,
mit integrierter Dichtungsmembran, die die Kontakte abdeckt, wenn kein
Stecker eingesteckt ist.

Position: An der Unterseite des Gehäuses, neben den Cable-Glands.
Die Buchse ist nach unten gerichtet, damit Wasser nicht stehen bleibt.

Alternative: USB-C-Buchse im Inneren des Gehäuses, Zugang nur mit
geöffnetem Deckel. Das eliminiert die Schwachstelle komplett, ist aber
im Betrieb unpraktisch.

Empfehlung: Wasserdichte Panel-Mount-Buchse (IP67) an der Unterseite,
zusätzlich mit einer Silikon-Klappe abgedeckt, wenn nicht in Gebrauch.

### 4.3 BLE-Antenne

Das Bluetooth-Low-Energy-Modul des ESP32-S3 kommuniziert mit dem
Smartphone. Die Antenne funktioniert durch das ASA-Kunststoffgehäuse
hindurch problemlos — Kunststoff ist für Hochfrequenz weitgehend
transparent. Es ist keine externe Antenne nötig.

Wichtig: Im Bereich der Antenne darf keine metallische Abschirmung
(Kupferfolie, Alu-Platte) auf der Innenseite des Gehäuses angebracht
werden. Ein Bereich von mindestens 30 × 30 mm um die Antenne muss
freie Sicht nach außen haben.

### 4.4 GPS-Antenne

Die GPS-Antenne (Active Patch, 25 × 25 mm) braucht freie Sicht nach
oben zum Himmel. Sie wird entweder:
- Oberhalb des Lenkers auf dem Gehäusedeckel montiert, oder
- Auf einem kurzen Stiel (~5 cm) nach oben, falls das Gehäuse tief montiert wird

Die GPS-Antenne ist durch das Kunststoffgehäuse hindurch funktionsfähig.
Wichtig ist ein Mindestabstand von 10 cm zum GPS-Modul (bei aktiven
Antennen) und möglichst großer Abstand zur Zündspule und zum CDI-Modul
(EMV-Störung der Satellitensignale).

### 4.5 Druckausgleichs-Membran

Ein komplett wasserdichtes Gehäuse erzeugt ein Kondenswasser-Problem:
Temperaturwechsel führen dazu, dass Luft im Gehäuse Feuchtigkeit aufnimmt
und bei Abkühlung als Kondensat an den Innenwänden kondensiert.

Lösung: Eine ePTFE-Membran (expandiertes PTFE) an der Unterseite des
Gehäuses. Die Membran lässt Luft und Wasserdampf in beide Richtungen
passieren, blockt aber flüssiges Wasser und Staub ab.

Eigenschaften:
- IP67/IP68-rated (1 m Wassersäule für 30 min ohne Wassereintritt)
- Bi-direktionaler Luftaustausch, kein Überdruck-Ventil nötig
- Hydrophob und oleophob
- UV- und temperaturstabil (-40 bis +125°C)
- Snap-in, Screw-in oder Adhesive-Ausführung

Montage: An der Unterseite des Gehäuses, vertikal orientiert, damit
Wasser nicht auf der Membran stehen bleibt. Gewindeausführung M6 × 0,75.

### 4.6 Tasten

Bedientasten (z.B. für Display-Umschaltung, Menü-Navigation) werden als
Silikonmembran-Taster ausgeführt: Ein Silikon-Pilz durchdringt die
Gehäusewand und drückt auf den Mikroschalter auf dem PCB. Die Silikon-
membran dichtet die Öffnung ab, ohne dass Wasser von außen eindringen kann.

Die Tasten sind an der Oberseite oder Vorderseite des Gehäuses positioniert,
geschützt unter einer leichten Überdachung (Wasser läuft nicht direkt
auf die Taste).

---

## 5. Wasserfestigkeit

### 5.1 IP-Schutzklasse: Ziel

Ziel ist ein Dual-Rating von IP65/IP67:
- IP65: Schutz gegen Wasserstrahl (6,3 mm Düse, 12,5 l/min, 3 m Abstand)
- IP67: Schutz gegen kurzzeitiges Eintauchen (1 m Wassertiefe, 30 min)

Wichtig: IP67 bedeutet NICHT automatisch, dass das Gerät auch Wasserstrahl
(IP65/66) standhält. Es sind verschiedene Tests mit verschiedenen
Dichtungsbelastungen. Ein IP67-Gehäuse kann bei Hochdruckreinigung versagen,
weil der Strahl eine andere Belastung erzeugt als statisches Eintauchen.

Die Realität am Motorrad: ein Bordcomputer am Lenker wird selten komplett
eingetaucht, aber Furt-Durchfahrten und Waschstraßen sind real.
IP67 ist das realistische Minimum, IP65 für den Hochdruckreiniger-Zugang.

### 5.2 Dichtungskonzept

Die Wasserdichtigkeit beruht auf mehreren Ebenen:

**Primärdichtung: Silikon-O-Ring (Umfangs-Dichtung am Deckel)**
- O-Ring aus Silikon (2 mm Cordstärke, Shore 60A)
- Umlaufende Nut im Deckel, O-Ring wird gegen die Auflagefläche
  des Unterteils gepresst
- Temperaturbereich: -40 bis +200°C
- UV-stabil (Silikon vergilbt nicht unter UV)
- Die Nut muss toleranzkritisch gefertigt werden — bei 3D-Druck
  ggf. nachbearbeiten (nachschleifen oder nachdrucken mit kalibriertem
  Extrusions-Multiplikator)

**Sekundärdichtung: EPDM-Schaumstoff**
- Für unebene Flächen, an denen ein O-Ring nicht zuverlässig dichtet
- Selbstklebend, auf die Auflagefläche des Unterteils aufgeklebt
- Shore 10–30A, sehr weich, füllt Spalte

**Kabeldurchführungen:**
- Cable-Glands mit O-Ring-Dichtung (siehe Abschnitt 4.1)
- O-Ring zwischen Gland und Gehäusewand
- Kabel-Ø passend zum Gland

**USB-C-Buchse:**
- Wasserdichte Panel-Mount-Buchse (IP67) mit integrierter Dichtungsmembran
- Zusätzliche Silikon-Klappe bei Nichtgebrauch

**Tasten:**
- Silikonmembran-Taster (durch Gehäusewand)
- Keine direkte Öffnung in der Gehäusewand

**Druckausgleichs-Membran:**
- ePTFE-Membran (IP68) für Druckausgleich und Kondensat-Entweichen
- An der Unterseite, vertikal montiert

### 5.3 Gehäuse-Design-Prinzipien für Wasserfestigkeit

**Tropfenform:** Das Gehäuse ist so geformt, dass Wasser von oben abläuft.
Keine horizontalen Flächen, auf denen Wasser stehen kann. Runde Oberseite,
schräge Wände, nach unten offene Überlappungen.

**Überlappung (Shingling):** Der Deckel greift über das Unterteil, so dass
Wasser, das an der Naht entlangläuft, an der Außenseite herunterläuft und
nicht ins Innere gelangt. Wie Dachschindeln.

**Naht-Position:** Die Deckelnaht liegt an der Unterseite/Rückseite, nicht
oben. Wasser, das von oben kommt, trifft auf eine geschlossene Fläche.

**Schrauben-Position:** Schrauben greifen von unten oder seitlich in das
Unterteil, niemals von oben. Wenn von oben unvermeidbar: O-Ring unter
dem Schraubenkopf.

**Kabelanschlüsse an der Unterseite:** Alle Cable-Glands und die USB-C-Buchse
sind an der Unterseite positioniert, nach unten gerichtet. Wasser, das an
den Kabeln entlangläuft, tropft nach unten ab.

### 5.4 Conformal Coating (PCB-Schutzlack)

Zusätzlich zur Gehäusedichtung wird das PCB mit einem Schutzlack
(Conformal Coating) überzogen. Das schützt die Platine auch bei
Wassereintritt kurzzeitig vor Korrosion und Kurzschluss.

Empfohlener Lacktyp: Polyurethan (UR)
- Dicke: 25–75 µm (2–3 dünne Schichten)
- Temperaturbereich: -65 bis +130°C
- Chemikalien-Resistenz: Sehr gut (Kraftstoff, Öl, Reinigungsmittel)
- Rework: Schwer (aber möglich mit speziellen Lösungsmitteln)

Vor dem Coating müssen maskiert werden:
- Stecker und Lötpads (für Service-Zugang)
- HF-Antennen (sonst Dämpfung)
- Sensoren, die Luftkontakt brauchen (z.B. Barometer-Öffnung)

Auftrag: Pinsel oder Sprühdose (für Prototyp ausreichend).
Aushärtung: 24 h Raumtemperatur oder 60 min bei 80°C.

### 5.5 Wassereintritt: Notfall-Strategie

Wenn Wasser ins Gehäuse gelangt (und das passiert irgendwann — Dichtungen
altern, Cable-Glands lockern sich):

1. **Kondensat-Entlüftung:** Die ePTFE-Membran lässt Wasserdampf entweichen
2. **Abfluss:** Ein kleines Loch an der tiefsten Stelle des Gehäuses,
   verschlossen mit einer ePTFE-Membran, lässt flüssiges Wasser ab
3. **Conformal Coating:** Schützt das PCB auch bei Wassereintritt kurzzeitig
4. **Wartung:** Dichtungen und Gaskets als Verschleißteile behandeln.
   Nach 2–3 Saisons Motorrad-Einsatz ist die Dichtung verschlechtert.
   IP67 bedeutet "30 min 1 m Wassertiefe neu" — nicht nach Jahren.

---

## 6. Innenleben

### 6.1 PCB-Einschub und Führungsschienen

Das PCB wird auf Führungsschienen in das Unterteil eingeschoben.
Die Schienen sind Teil des Unterteils (mitgedruckt) und haben
eine Nut, in die das PCB-Kanten eingreifen.

- Schienen-Höhe: 3 mm über dem Boden
- Schienen-Breite: 2 mm (Passung zum PCB-Rand)
- Schienen-Länge: vollständige Gehäusetiefe
- Toleranz: 0,2 mm Spiel (PCB muss gleiten, aber nicht wackeln)

Das PCB wird nach dem Einschub mit 4 × M3 Senkkopfschrauben in den
Schraubsäulen arretiert. Die Schraubsäulen haben eingeschmolzene
M3-Gewinde (Messing-Einschraubhülsen oder selbstschneidend in ASA).

### 6.2 Verschraubung

| Bauteil | Schraube | Anzahl | Werkzeug | Hinweis |
|---|---|---|---|---|
| Deckel → Unterteil | M3 × 12 mm Senkkopf | 4 | Inbus 2,5 mm | Mit Federscheibe |
| PCB → Schraubsäulen | M3 × 8 mm Senkkopf | 4 | Inbus 2,5 mm | Mit Gummipuffer |
| Display-Rahmen → Deckel | M2,5 × 6 mm | 4 | Inbus 2,0 mm | Mit Federscheibe |
| Halterung → Gehäuse | M6 × 20 mm | 2 | Inbus 4 mm | U-Bolt |

Alle Schrauben am Motorrad brauchen Federscheiben (DIN 127B oder
vergleichbar) gegen Vibration. Schraubensicherungslack kann zusätzlich
aufgetragen werden.

### 6.3 Abstandhalter

Zwischen PCB und Gehäuseboden werden Abstandsbolzen verwendet:
- M3 Abstandsbolzen (Sechskant, 8 mm oder 10 mm Länge)
- Material: Messing oder Edelstahl
- Funktion: definierter Abstand, Masseverbindung PCB → Gehäuse
  (wichtig für EMV, siehe Abschnitt 7)

Zwischen PCB-Schraubenkopf und PCB werden Gummipuffer gelegt:
- 1 mm Silikon-Unterlegscheibe
- Funktion: Vibrations-Entkopplung des PCB

### 6.4 Kabelführung innen

Im Inneren des Gehäuses werden Kabelbinder-Punkte vorgesehen:
- Kleine Löcher (3 mm) an den Innenwänden, durch die Kabelbinder
  gefädelt werden
- Alle 20–30 mm ein Kabelbinder-Punkt
- Kabelführung: geordnet, keine freihängenden Kabel (Vibration!)
- Kabel nicht über das PCB geführt (Service-Zugang)

### 6.5 LiPo-Akku-Platzierung

Der LiPo-Akku (1000–2000 mAh, Puffer für Spannungsausfälle) wird
thermisch vom ESP32 getrennt:
- Akku in einer separaten Kammer oder am anderen Ende des Gehäuses
- Wärmebrücke vom ESP32 nach außen (nicht zum Akku)
- Akku fixiert mit Klettband oder Kabelbinder (nicht kleben —
  Akku kann aufquellen und muss entnehmbar sein)

Im Winter: Ladung bei unter 0°C per Software blockieren
(Temperatursensor am Akku). Lithium-Plating bei Ladung unter 0°C
reduziert die Kapazität dauerhaft und kann zum internen Kurzschluss
führen.

### 6.6 Innen-Entwässerung (Optional)

Ein optionales kleines Loch (2 mm) an der tiefsten Stelle des Gehäuses,
verschlossen mit einer ePTFE-Membran, dient als Kondenswasser-Ablauf.
Flüssiges Wasser, das durch Kondensation oder Leckage ins Innere gelangt,
kann durch diese Öffnung als Dampf oder Flüssigkeit entweichen.

---

## 7. EMV-Schirmung

### 7.1 Problemstellung

Die Honda Transalp 600 (PD06) verwendet eine CDI (Capacitive Discharge
Ignition). Bei der CDI wird ein Kondensator auf 200–400 V aufgeladen und
entlädt sich über die Zündspule, die die Spannung auf 20–40 kV
hochtransformiert. Dieser Entladevorgang erzeugt ein breitbandiges
elektromagnetisches Spektrum von wenigen kHz bis über 100 MHz.

EMV-Probleme durch die Zündung äußern sich als:
- Spannungsspitzen auf der 12V-Versorgung (Ripple bis 5 Vss)
- Elektromagnetische Einstreuung in Sensorleitungen
- Reset des Mikrocontrollers durch EMV-Pulse auf GPIO- oder
  Versorgungsleitungen
- GPS-Empfangsverlust durch Breitbandstörung der Antenne

Ein 3D-gedrucktes ASA-Gehäuse bietet von sich aus keine elektromagnetische
Abschirmung — Kunststoff ist für EMV transparent.

### 7.2 Abschirmungsmethoden

**Methode 1: Kupferfolie innen (empfohlen für Prototyp)**
- Selbstklebende Kupferfolie (0,05 mm) auf die Innenseite des Gehäuses
- Schirmwirkung: 40–60 dB im Bereich 30–1000 MHz
- Masse-Anschluss: Folie mit einer Lasche an die PCB-Masse anschließen
- Kosten: ~5–10 € pro Gehäuse
- Einfach nachträglich aufklebbar

**Methode 2: Alu-Platte als innere Abschirmung**
- Dünne Alu-Platte (0,5–1 mm) im Gehäuseboden, PCB montiert darauf
- Schirmwirkung: 60–80 dB
- Zugleich Wärmeleitung (Alu leitet Wärme vom ESP32 nach außen)
- Gewicht: ~20–30 g zusätzlich

**Methode 3: Leitfähiger Innenlack**
- Kupfer-Graphit-Lack oder Nickel-Acrylat-Lack auf die Innenseite gesprüht
- Schirmwirkung: 30–50 dB (schwächer als Folie, aber gleichmäßiger)
- Vorteil: erreicht auch Hohlräume und Rippen
- Nachteil: schwer gleichmäßig aufzutragen, kann abblättern

**Empfehlung:** Methode 1 (Kupferfolie innen) als Standard-Maßnahme.
Wenn EMV-Probleme auftreten (GPS-Ausfälle, Mikrocontroller-Resets),
auf Methode 2 (Alu-Platte) wechseln. Der Bereich um die BLE-Antenne
und GPS-Antenne muss von der Abschirmung ausgenommen werden (Fenster).

### 7.3 Ferrit-Kerne

An jedem externen Kabel, das ins Gehäuse mündet, wird ein Klappferrit
angebracht:
- Typ: Material 43 oder 73, Impedanz >100 Ohm bei 100 MHz
- Platz: möglichst nah am Gehäuse, um die Antennenwirkung des
  externen Kabels zu minimieren
- Funktion: wirkt als Drossel für Hochfrequenz (Impedanz steigt mit
  Frequenz), reduziert eingekoppelte Störungen

Ferritkerne sind einfach nachrüstbar und eine der effektivsten
EMV-Maßnahmen mit dem geringsten Aufwand.

### 7.4 Masseverbindung Gehäuse → PCB

Die Masseverbindung zwischen Gehäuse-Abschirmung und PCB-Masse ist
essenziell für die Funktion der Abschirmung. Ohne Masseanschluss wirkt
die Kupferfolie als unkontrollierte Antenne und kann Störungen
verstärken statt abschirmen.

Umsetzung:
- Eine Lasche aus Kupferfolie oder ein kurzes Kabel verbindet die
  Innen-Abschirmung mit dem PCB-Masse-Polygon
- Anschlusspunkt: Sternpunkt (Single-Point Ground) auf dem PCB
- Nur EINE Masseverbindung zwischen Gehäuse und PCB (keine
  Masseschleifen)

### 7.5 Kabel-Abschirmung

Sensorleitungen, die aus dem Gehäuse herausgehen, können als Antennen
wirken und EMV-Störungen einkoppeln.

- Geschirmte Kabel (SFTP oder S/UTP) für Sensorkabel
- Schirm an EINEM Ende auf Masse legen (Gehäuseseite), um Masseschleifen
  zu vermeiden
- Schirmwirkung: 80–90% Abdeckung bei Hochfrequenz
- Verdrillte Adern (Twisted Pair) für analoge Sensorsignale: reduziert
  induktive Einkopplung um 20–40 dB

### 7.6 RC-Glieder und Entkopplung

An jedem GPIO-Eingang, der aus dem Motorrad-Netz kommt, wird ein
RC-Tiefpassglied eingesetzt:
- Werte: R = 1–10 kOhm, C = 10–100 nF
- Grenzfrequenz: z.B. bei R=10k, C=100nF → ~160 Hz
- Filtert hochfrequente Störungen (Zündfunken) heraus, lässt aber
  niederfrequente Signale (Geschwindigkeit <200 Hz) passieren

Entkopplungskondensatoren:
- An jedem IC-Versorgungspin: 100 nF Keramikkondensator, nah am Pin
- Bulk-Kondensator (10–100 µF Elektrolyt) am Board-Eingang
- Für den Mikrocontroller: 10 µF + 100 nF Kombination an der 3,3V-Versorgung

---

## 8. 3D-Druck-Empfehlungen

### 8.1 Druckrichtung und Schichtorientierung

Die Druckrichtung muss so gewählt werden, dass die Schichten quer zur
Belastungsrichtung liegen. ASA erreicht in Z-Richtung (senkrecht zu den
Druckschichten) nur ca. 29% der Festigkeit der XY-Ebene (17 kg vs. 57 kg
flach). Für ein Gehäuse, das Vibrationen ausgesetzt ist, muss die
Wandstärke diese Schwäche kompensieren.

Empfohlene Druckorientierung:
- Gehäuse-Unterteil: Deckfläche nach unten (größte ebene Fläche auf dem
  Druckbett), Öffnung nach oben
- Deckel: Display-Aussparung nach oben, Naht-Fläche nach unten
- Display-Rahmen: Flach auf dem Druckbett

Die Belastung durch Vibration erfolgt hauptsächlich in Z-Richtung des
Motorrads (vertikal). Die Druckschichten sollten senkrecht dazu liegen,
also horizontal orientiert sein. Das wird durch die oben beschriebene
Druckorientierung erreicht.

### 8.2 Stützstruktur-Strategie

Nicht gut druckbar ohne Stützstruktur:
- Überhänge >50° (ASA-Spezifikation)
- Horizontale Löcher für Cable-Glands — besser vertikal drucken oder
  Stützmaterial verwenden
- Dichtungsnuten für O-Ringe — toleranzkritisch, ggf. nachbearbeiten

Stützstrategie:
- Überhänge an Kabeldurchführungen: Stützmaterial mit 0,2 mm Abstand
  (bruchbar, nach dem Druck entfernbar)
- Deckel-Innenseite: Wenn der Deckel gewölbt ist, Stützmaterial für
  die Wölbung
- Display-Aussparung: Brücke (Bridge) über den Ausschnitt, kein Stütz
  nötig bei <5 mm Überbrückung

Alternative: Gehäuseteile so konstruieren, dass Stützstruktur vermieden
wird. Kabeldurchführungen vertikal ausrichten. Dichtungsnuten in zwei
Hälften drucken und zusammenfügen.

### 8.3 Druckparameter für ASA

| Parameter | Wert | Begründung |
|---|---|---|
| Druckbettemperatur | 80–95°C | Verhindert Warping, verbessert Schichthaftung |
| Düsentemperatur | 240–260°C | ASA-Schmelzbereich |
| Bauteil-Lüfter | AUS | ASA nicht aktiv kühlen (Verzug, Delamination) |
| Druckgeschwindigkeit | 30–50 mm/s | Langsam für bessere Schichthaftung |
| Schichthöhe | 0,2 mm | Standard für 0,4 mm Düse |
| Wandstärke | min. 2,4 mm (6 Perimeter) | Strukturfestigkeit |
| Beheizte Kammer | Empfohlen (>30°C) | Verbessert Schichthaftung um ~20% |

ASA ist anspruchsvoller zu drucken als PETG oder PLA. Warping (Verzug
an den Ecken) und Delamination (Schichten trennen sich) sind die
häufigsten Probleme. Eine beheizte Druckkammer oder zumindest ein
geschlossener Bauraum sind empfehlenswert.

### 8.4 Druckmaterial-Vergleich

| Kriterium | PETG | ASA | ABS |
|---|---|---|---|
| Druckbarkeit | Einfach (keine Kammer) | Mittel (Kammer empfohlen) | Mittel (Kammer Pflicht) |
| UV-Beständigkeit | Bedingt | Ja | Nein |
| Temperaturgrenze | 80°C | 100°C | 105°C |
| Kältebeständigkeit | Gut | Sehr gut | Schlecht (spröde) |
| Schichthaftung | Gut | Mittel (Z-Richtung schwach) | Mittel |
| Wasserdichtigkeit | Ja | Ja | Ja |
| Oberflächenqualität | Glatt | Matt | Matt |
| Aceton-Dampfnachbearbeitung | Nicht möglich | Nicht möglich | Möglich |

**Für das Außengehäuse:** ASA (UV-stabil, temperaturbeständig, für
Automobil-Anwendungen entwickelt).

**Für Prototyp-Phase:** PETG (einfacher zu drucken, ausreichend für
Erprobung ohne UV- und Temperatur-Extreme).

**Nicht verwenden:** PLA (verformt in der Sonne), ABS ohne
UV-Schutz (vergilbt und wird spröde).

### 8.5 Nachbearbeitung

**Aceton-Dampfnachbearbeitung (nur bei ABS):**
- ABS lässt sich mit Aceton-Dampf glätten: die Schichten verschmelzen
  zu einer glatten Oberfläche
- Vorteil: bessere Oberflächenqualität, weniger Rauheit, leichter zu
  reinigen, bessere Wasserdichtigkeit (keine Rillen zwischen Schichten)
- Nachteil: nicht möglich mit ASA oder PETG
- Bei ASA: mechanisches Nachschleifen oder Auftrag einer Decklackierung

**Für ASA:**
- Oberfläche nach dem Druck leicht rauh (matt). Das ist akzeptabel.
- Für glattere Oberfläche: feines Schleifpapier (Körnung 400–800)
  oder Auftrag eines Klarlacks (UV-stabil, kompatibel mit ASA)
- Die Dichtungsnut für den O-Ring muss exakt nachgearbeitet werden:
  Kalibrierung mit dem O-Ring, ggf. nachschleifen bis die Passung stimmt

**Gewinde einschmelzen:**
- Für M3-Schraubsäulen: Messing-Einschraubhülse mit Lötkolben
  einschmelzen (auf 180°C erhitzen, in die Säule drücken, abkühlen lassen)
- Alternative: selbstschneidendes M3-Gewinde direkt ins ASA
  (funktioniert, aber weniger haltbar bei häufigem Lösen)

### 8.6 Druckqualitäts-Kontrolle

Nach dem Druck sollten folgende Punkte kontrolliert werden:
1. Wandstärke messen (Mindestmaße aus Abschnitt 1.3)
2. Schichthaftung prüfen (keine sichtbaren Spalte zwischen Schichten)
3. Dichtungsnut mit O-Ring kalibrieren (Passung muss dicht sein)
4. Gewinde in Schraubsäulen prüfen (Schraube lässt sich eindrehen,
   ohne die Säule zu sprengen)
5. Kabeldurchführungen auf Passung prüfen (Cable-Gland lässt sich
  eindrehen und dichtet ab)
6. Display-Aussparung auf Maßhaltigkeit prüfen (Abdeckung passt)
7. Wasserdichtigkeits-Test (Trocken-test: Gehäuse mit trockenem
   Reis oder Silica-Gel füllen, verschließen, 30 min in Wasser tauchen,
   Reis kontrollieren)

---

## 9. ASCII-Schnittzeichnungen

### 9.1 Seitenansicht (Schnitt A-A)

Schnitt durch das Gehäuse von der Seite (Fahrtrichtung nach rechts),
durch die Mitte des Displays und der PCB-Ebene.

```
       Fahrtrichtung ===>
       
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  (Acrylglas-Abdeckung, 2mm)
  :                               :
  :  +---+               +---+    :   <-- Display-Rahmen (PETG)
  :  | E | E-Paper 2.9"  |   |    :       auf Silikon-Pad
  :  | P | (296x128)     |   |    :
  :  +---+               +---+    :
  :  :::::::::::::::::::::::::::: :
  :  : PCB (ESP32-S3 + Sensoren) : :   <-- PCB auf Führungsschienen
  :  :::::::::::::::::::::::::::: :
  :                               :
  :  [Akku]         [DC/DC]       :   <-- LiPo-Akku + Buck-Converter
  :                               :
  :  oooo  oooo  oooo  oooo      :   <-- Kabeldurchführungen (PG)
  :                               :
  +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
  |                               |   <-- Gehäuse-Unterteil (ASA, 3mm)
  |        Schraubsäule           |       Schraubsäulen M3
  |          |||                  |
  +----------+||+-----------------+
             |||
      [Gummidämpfer]                  <-- Vibrations-Entkopplung
             |||
  +----------+||+-----------------+
  |          |||       Halterung |       <-- Halterungs-Adapter (ASA)
  |     U-Bolt 22mm  (Lenker)    |
  +-----+============+-----+          <-- 22mm Lenker / Crossbar
        |  Lenker  |
        +----------+
        
  <---- 100 mm Breite ---->
  <-- 45 mm Tiefe (Höhe im Bild) -->
```

### 9.2 Frontansicht

Ansicht von vorne (Fahrer sieht auf das Display).

```
       Fahrtrichtung (Fahrer blickt hierher) ===>
       
  +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
  :                                   :   <-- Deckel (ASA, 3mm)
  :   +---------------------------+   :
  :   |                           |   :   <-- Acrylglas-Abdeckung
  :   |                           |   :       (klarsicht, 2mm)
  :   |                           |   :
  :   |    E-Paper 2.9" Display   |   :   <-- 296 x 128 Pixel
  :   |    (296 x 128 Pixel)      |   :
  :   |                           |   :
  :   |                           |   :
  :   |                           |   :
  :   +---------------------------+   :
  :                                   :
  :        [ TASTE ]                  :   <-- Silikonmembran-Taster
  :                                   :
  +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
  |  o   o   o   o   o   o   o   o   |   <-- Kabeldurchführungen (PG7/PG9)
  |  |   |   |   |   |   |   |   |   |       + USB-C (IP67) an der Unterseite
  +-----------------------------------+
  |                                   |   <-- Gehäuse-Unterteil (ASA)
  |        Schraubsäulen (M3)         |       4x Senkkopfschrauben
  |     x              x              |       (von unten zugänglich)
  |                                   |
  |     x              x              |
  +-----------------------------------+
  
  <-------- 100 mm Breite -------->
  <---------- 80 mm Höhe ---------->
```

### 9.3 Rückansicht

Ansicht von hinten (auf die Gehäuserückseite mit Halterung).

```
       (Rückseite, Fahrer blickt weg)  <===
       
  +-----------------------------------+
  |                                   |   <-- Gehäuse-Rückseite (ASA)
  |                                   |
  |    +-------------------+          |
  |    |                   |          |   <-- Halterungs-Adapter
  |    |   Kugelgelenk     |          |       (Kugelgelenk für Neigung)
  |    |   (Neigungsverst.)|          |
  |    +-------------------+          |
  |                                   |
  |    [Gummidämpfer]                |   <-- Schwingungsdämpfer
  |         x                         |       (M6, Shore 50-60A)
  |    [Gummidämpfer]                |
  |         x                         |
  |                                   |
  |                                   |
  |    o  ePTFE-Membran               |   <-- Druckausgleich (M6)
  |    o  (vertikal montiert)         |       an der Unterseite
  +-----------------------------------+
  
  <-- 100 mm Breite -->
  <-- 80 mm Höhe -->
```

### 9.4 Innenansicht (Explosionszeichnung)

Explosionsdarstellung der Gehäusekomponenten, von oben nach unten.

```
                    Fahrtrichtung ===>
                    
  Schicht 1: Deckel (ASA, 3mm)
  +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
  :  +--- Acrylglas (2mm) ---+        :
  :  |                       |        :   <-- Display-Abdeckung
  :  +-----------------------+        :       (eingeclipst, silikonversiegelt)
  :                                   :
  :  o ePTFE-Membran (Druckausgleich) :   <-- an der Unterseite des Deckels
  :  o [TASTE] Silikonmembran        :
  +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
                    |
                    v (Schrauben M3 x 12mm, 4x, mit Federscheibe)
                    |
  Schicht 2: Display-Rahmen (PETG)
  +-----------------------------------+
  |  +---+               +---+        |
  |  |   | E-Paper-Modul |   |        |   <-- E-Paper 2.9" auf Silikon-Pad
  |  +---+               +---+        |       (0,5mm Silikon-Unterlage)
  |                                   |
  |  x              x                 |   <-- M2,5 Schrauben (4x)
  |                                   |       mit Federscheibe
  |  x              x                 |
  +-----------------------------------+
                    |
                    v
  Schicht 3: O-Ring-Dichtung (Silikon, 2mm Cord)
  +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
  :  ================================ :   <-- Umfangs-Dichtung
  :  ================================ :       (in Nut im Deckel)
  +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
                    |
                    v
  Schicht 4: PCB (ESP32-S3 + Sensoren)
  +-----------------------------------+
  |  [ESP32-S3-WROOM-1]               |
  |  [GPS-Modul]   [IMU]   [Baro]     |   <-- Haupt-PCB
  |  [SD-Karte]    [DC/DC]            |
  |  [TP4056]      [LiPo-Anschluss]   |
  |                                   |
  |  o   o   o   o                    |   <-- M3 Abstandsbolzen (4x)
  |  (Abstandhalter + Gummipuffer)   |
  +-----------------------------------+
                    |
                    v
  Schicht 5: LiPo-Akku (separat, thermisch getrennt)
  +-----------------------------------+
  |  [LiPo 1000-2000 mAh]             |   <-- Puffer-Akku
  |  (fixiert mit Klettband)          |       thermisch vom ESP32 getrennt
  +-----------------------------------+
                    |
                    v
  Schicht 6: Gehäuse-Unterteil (ASA, 3mm)
  +-----------------------------------+
  |  :::::::::::::::::::::::::::      |   <-- Führungsschienen für PCB
  |  :::::::::::::::::::::::::::      |       (mitgedruckt, 3mm hoch)
  |                                   |
  |  o   o   o   o   o   o            |   <-- Kabeldurchführungen (PG7/PG9)
  |  (Cable-Glands, von unten)        |       + USB-C IP67
  |                                   |
  |  x   x   x   x                    |   <-- Schraubsäulen M3 (4x)
  |  (Gewinde eingeschmolzen)         |
  |                                   |
  |  +-------------------+            |   <-- Halterungs-Schnittstelle
  |  | Kugelgelenk-Aufn. |            |       an der Rückseite
  |  +-------------------+            |
  +-----------------------------------+
                    |
                    v
  Schicht 7: Kupferfolie-Abschirmung (innen, 0,05mm)
  +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
  :  (selbstklebend, auf Innenseite) :
  :  [Fenster um BLE-Antenne]        :   <-- Aussparung für HF
  :  [Fenster um GPS-Antenne]       :
  +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
                    |
                    v
  Schicht 8: Halterung + Gummidämpfer
  +-----------------------------------+
  |  [Gummidämpfer]  [Gummidämpfer]   |   <-- Vibrations-Entkopplung
  |         x              x          |       (M6, Shore 50-60A)
  |  [Halterungs-Adapter aus ASA]     |
  |  [Kugelgelenk] [Klemmhebel]       |   <-- Neigungsverstellung 0-15°
  +-----------------------------------+
                    |
                    v
  Schicht 9: Lenker / Crossbar (22mm)
  +-----+============+-----+
  |     |  22mm Rohr |     |           <-- Lenker oder Crossbar
  +-----+============+-----+
        |  U-Bolt    |
        +------------+
```

### 9.5 Schnitt B-B: Detail Kabeldurchführung

Detaillierter Schnitt durch eine Cable-Gland an der Gehäuseunterseite.

```
  Innen (Gehäuse)                    Außen (Motorrad)
  <---                                --->
  
  +-------------------------------------------+
  |              Gehäusewand (ASA, 4mm)       |
  |                                           |
  |     +--+        +----+        +--+        |
  |     |  |        |    |        |  |        |
  |     |  |        | O  |        |  |        |
  |  ===|  |========| R  |========|  |========|  <-- Kabel
  |     |  |        | i  |        |  |        |       (Sensorkabel)
  |     |  |        | n  |        |  |        |
  |     |  |        | g  |        |  |        |
  |     +--+        +----+        +--+        |
  |                                           |
  |  Kontermutter   Cable-     Dichtungs-     |
  |  (innen)        Gland     mutter (außen)  |
  |                 (PA66)    (mit O-Ring)    |
  +-------------------------------------------+
  
  Wasserdichtigkeit durch 3 Dichtpunkte:
  1. O-Ring zwischen Gland und Gehäusewand
  2. O-Ring zwischen Gland und Kabel (innen im Gland)
  3. Kontermutter presst Gland gegen Gehäusewand
  
  Kabel-Ø muss genau zum Gland passen (3-6,5mm für PG7).
```

### 9.6 Schnitt C-C: Detail Deckelnaht (Shingling)

Detaillierter Schnitt durch die Deckelnaht, die das Shingling-Prinzip
verdeutlicht.

```
  Außen (Regen)                     Innen (Gehäuse)
  <---                                --->
  
  Wasser läuft von oben:
  |
  v
  |
  v
  +~~~~~~ Deckel (überlappend) ~~~~~~+
  :                                   :
  :  +-------------------------------+:
  :  |                               :   <-- Deckel greift über Unterteil
  :  |    O-Ring (Silikon, 2mm)      :       (Shingling wie Dachschindeln)
  :  |    ====[]=================    :
  :  |                               :
  :  +-------------------------------+:
  :                                   :
  +-----------------------------------+
  |          Unterteil (ASA, 3mm)     |
  |                                   |
  
  Wasser, das an der Naht entlangläuft, fließt an der Außenseite
  des Unterteils nach unten ab — gelangt nicht ins Innere.
  
  Die Naht liegt an der Rückseite/Unterseite, NICHT oben.
  Wasser von oben trifft auf eine geschlossene Fläche.
```

---

## Anhang: Material- und Bauteilliste

| Bauteil | Wahl | Begründung |
|---|---|---|
| Gehäusematerial | ASA (hell, weiß/hellgrau) | UV-stabil, bis 100°C, Automobil-Standard |
| Innen-Platte / Display-Rahmen | PETG | Einfacher zu drucken, ausreichend für interne Teile |
| Dichtung Deckel | Silikon-O-Ring (2 mm Cord) | -40 bis +200°C, UV-stabil |
| Sekundärdichtung | EPDM-Schaumstoff | Für unebene Flächen, selbstklebend |
| Kabeldurchführung | PG7/PG9 Cable-Glands (PA66) | IP67, günstig, etabliert |
| Druckausgleich | ePTFE-Membran (M6) | IP68, bi-direktional, Kondensat-Entweichen |
| USB-C-Buchse | IP67 Panel-Mount | Wasserdicht, Service-Zugang |
| Tasten | Silikonmembran-Taster | Wasserdicht durch Gehäusewand |
| Display-Abdeckung | Acrylglas 2 mm | Klarsicht, wasserabweisend beschichtet |
| PCB-Schutz | Conformal Coating (Polyurethan) | Feuchtigkeit, Chemikalien, Temperatur |
| EMV-Abschirmung | Kupferfolie selbstklebend (innen) | 40–60 dB Schirmdämpfung |
| EMV-Filterung | Ferritkerne an allen externen Kabeln | HF-Entkopplung, einfach nachrüstbar |
| Vibrations-Dämpfer | Gummi-Metall (M6, Shore 50–60A) | Entkoppelt Gehäuse vom Lenker |
| Halterung | Kugelgelenk (Crossbar, 22 mm) | Industriestandard, neigungsverstellbar |
| Schrauben Gehäuse | M3 × 12 mm Senkkopf + Federscheibe | Vibrationssicher |
| Schrauben PCB | M3 × 8 mm + Gummipuffer | Vibration-entkoppelt, reparierbar |
| Schrauben Display | M2,5 × 6 mm + Federscheibe | Vibrationssicher |
| Abstandhalter | M3 Abstandsbolzen (Messing) | Definierter Abstand, Masseverbindung |
| Gewichtslimit | <400 g gesamt | Lenker-Geometrie nicht beeinträchtigen |
| IP-Schutzklasse | IP65/IP67 (Dual-Rating) | Spritzwasser + kurzes Eintauchen |
| Druckrichtung | Schichten quer zur Belastung | Kompensiert Z-Richtung-Schwäche von ASA |
| Druckbett-Temp | 80–95°C (ASA) | Verhindert Warping |
| Lüfter | AUS (bei ASA) | Verzug und Delamination vermeiden |
| Druckgeschwindigkeit | 30–50 mm/s | Bessere Schichthaftung |
| Wandstärke Außen | 2,4–3,2 mm (6–8 Perimeter) | Strukturfestigkeit |

---

## Quellen

Die in diesem Dokument enthaltenen technischen Daten und Empfehlungen basieren
auf der Recherche in folgenden Quellen (siehe docs/research/05-gehaeuse-einbau-emv.md
für vollständige Quellenangaben mit Links):

- CNC Kitchen: PLA vs PETG vs ASA Materialvergleich
- Hersteller-Datenblätter für ASA-Filament (Tg, HDT, Schlagzähigkeit)
- Kataloge für Kabelverschraubungen und Industriegehäuse (Takachi-Serie)
- ePTFE-Schutzmembranen (GORE Protective Vents FAQ und Produktdatenblätter)
- IEC 60529: IP-Schutzklassen Definitionen
- PCB Conformal Coating Auswahl-Leitfäden (Acryl, Silikon, Polyurethan, Epoxid)
- ESP32-S3 Modul-Spezifikationen (Temperaturbereich -40 bis +85°C)
- LiPo-Temperaturbereich: Ladung 0–45°C, Entladung -20 bis +60°C
- E-Paper Wide-Temperature-Varianten (-20 bis +60°C)
- SAE J1211: Motorrad-Vibrationsbelastung (15–25 g RMS am Lenker)
- CDI EMV-Grundlagen (Stromsteigung bis 100 A/µs, Spektrum kHz–100 MHz)
- EMV-Abschirmung mit Kupferfolie (40–60 dB Schirmdämpfung)
- Ferritkern-Auswahl-Leitfäden (Material 43/73, Impedanz >100 Ohm bei 100 MHz)
- Optokoppler-Datenblätter (galvanische Trennung für Zünd-/Lichtsignale)
- Lastabwurf-Schutz (TVS-Dioden, MOV, ISO 7637-2 Pulse 5)
- Verpolungsschutz mit MOSFET (P-Kanal, RDS(on) 0,02 Ohm)
- DC/DC-Wandler mit weiter Input-Range (Buck, 7–36V, >90% Effizienz)
- Kondensator als Lastabwurf-Puffer (4700–10000 µF, 25V)
- Kommerzielle Motorrad-Navi-Produkte (IPX7, ruggedized, RAM-Mount-kompatibel)
- Motorrad-PCB-Assembly-Leitfäden (Vibrationsoptimiertes PCB-Design)
- Motorrad-Elektronik-Projekte (Open Source, DIY-Gehäuse)

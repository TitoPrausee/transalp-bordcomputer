# Deep Dive: Gehäuse, Einbau, Wasserdichtigkeit, EMV & Motorrad-Herausforderungen

> Stand: Oktober 2026 — Recherche-Grundlage für Gehäusedesign und Umweltschutz.
> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986–2000, rein mechanisch, keine ECU.

## 0. Zusammenfassung

Die Transalp 600 PD06 ist ein rein mechanisches Motorrad ohne ECU, CAN-Bus oder
elektronische Zündsteuerung im modernen Sinn. Sie hat eine CDI (Capacitive Discharge
Ignition), eine 12V-Lichtmaschine und eine Bleiakkumulator-Batterie. Elektronik am
diesem Motorrad zu betreiben bedeutet: keine OBD-Schnittstelle, direkte Sensor-Anzapfung,
und eine elektrische Umgebung, die von mechanischen Generatoren und CDI-Zündfunken
dominiert wird.

Dieses Dokument behandelt alle Motorrad-spezifischen Herausforderungen für das
Bordcomputer-Projekt: Gehäusematerial, Wasserdichtigkeit, Vibration, Temperatur,
EMV und Stromversorgung.

---

## 1. Gehäuse-Design

### 1.1 Materialwahl: PETG vs. ABS vs. ASA vs. TPU

Das Gehäuse wird auf dem AnkerMake M5 FDM-Drucker gefertigt. Die Materialwahl
entscheidet über Temperaturbeständigkeit, UV-Stabilität und mechanische Robustheit
im Motorrad-Einsatz.

| Material | Tg (Glasübergang) | Druck-Temp | UV-beständig | Wasserdicht | Kältebeständig | Fazit Motorrad |
|---|---|---|---|---|---|---|
| PLA | ~60°C | 200-220°C | Nein | Nein | Schlecht | Unbrauchbar (verformt in der Sonne) |
| PETG | ~80°C | 230-250°C | Bedingt | Ja | Gut | Minimum für Prototyp |
| ABS | ~105°C | 230-260°C | Nein | Ja | Schlecht | Bricht bei Kälte, UV-fragil |
| ASA | ~100°C | 240-260°C | Ja | Ja | Sehr gut | Empfohlen für finalen Einsatz |
| TPU | — | 220-240°C | Ja | Ja | Sehr gut | Nur für Dichtungen/Puffer, nicht Struktur |

**ASA ist das Material der Wahl** für das Außengehäuse. Es ist die Weiterentwicklung
von ABS mit UV-Resistenz und besserer Wetterbeständigkeit, speziell für
Automobil-Außenanwendungen entwickelt ([CNC Kitchen Vergleich](https://www.cnckitchen.com/blog/comparing-pla-petg-amp-asa-feat-prusament),
[PolyLite ASA TDS](https://cdn.shopify.com/s/files/1/0548/7299/7945/files/PolyLite_ASA_TDS_V5.1.pdf)).

Wichtige Eigenschaften von ASA laut Datenblatt:
- Glasübergangstemperatur: 97,8°C
- Heat Deflection Temperature (1,8 MPa): 100,2°C
- UV-resistent, keine Vergilbung
- Dichte: 1,13 g/cm³
- Charpy-Schlagzähigkeit: 10,3 kJ/m² (Z-Richtung: 6,7 kJ/m²)

PETG bleibt eine akzeptable Alternative für Prototypen und die innere Platte. Es
ist einfacher zu drucken (keine beheizte Kammer nötig), hat aber nur 80°C
Temperaturgrenze — an einem schwarzen Gehäuse in der Sonne können 80°C schnell
erreicht werden.

**Druck-Hinweise für ASA:**
- Druckbettemperatur: 80-95°C
- Kein Bauteil-Lüfter (Cooling Fan OFF)
- Druckgeschwindigkeit: 30-50 mm/s
- Wandstärke: mindestens 2,4 mm (6 Perimeter bei 0,4 mm Düse) für Strukturfestigkeit
- beheizte Kammer empfohlen (Keller-Temperatur >30°C verbessert Schichthaftung um 20%)
- Layer Adhesion ist die Schwachstelle: ASA erreicht nur 29% der Druckbett-Festigkeit
  in Z-Richtung (17 kg vs. 57 kg flach). Für ein Gehäuse, das Vibrationen ausgesetzt ist,
  muss die Wandstärke die Schwäche der Z-Richtung kompensieren.

### 1.2 Wandstärken und Druckbarkeit

Empfohlene Konstruktion:
- Außenwände: 2,4-3,2 mm (6-8 Perimeter bei 0,4 mm Düse) — strukturelle Integrität
- Deckel/Boden: 3,2 mm (8 Layer bei 0,4 mm Schichthöhe) — Steifigkeit und Wasserbarriere
- Schraubsäulen: 6 mm Außendurchmesser, M3-Gewinde eingeschmolzen
- Kabeldurchführungen: Verstärkt auf 4 mm Wandstärke im Bereich der Cable-Glands

Nicht gut druckbar ohne Stützstruktur:
- Überhänge >50° (ASA-Spezifikation)
- Horizontale Löcher für Cable-Glands — besser vertikal drucken oder Stützmaterial verwenden
- Dichtungsnuten für O-Ringe — toleranzkritisch, ggf. nachbearbeiten

### 1.3 Dichtungsmaterialien

| Material | Temperatur | Shore-Härte | Einsatz | Quelle |
|---|---|---|---|---|
| O-Ring NBR (Nitril) | -20 bis +80°C | 60A | Standard, ölfest | Takachi RPG-Serie |
| O-Ring Silikon | -40 bis +200°C | 60A | Weite Temperatur, UV-stabil | Takachi RMW-Serie |
| Schaumstoff EPDM | -40 bis +120°C | 10-30A | Große Spalte, uneben | Selbstklebend |
| Silikon-Dichtmasse | -50 bis +200°C | — | Nahtversiegelung | Neutralvulkanisierend |

Für das Gehäuse empfohlen: **Silikon-O-Ringe** (bessere UV- und Temperaturbeständigkeit
als NBR) für die Deckeldichtung und EPDM-Schaumstoff als Sekundärdichtung für
unebene Flächen. ([Takachi Cable Glands](https://www.takachi-enclosure.com/cat/cable_glands))

### 1.4 Kabeldurchführungen: PG7 / PG9 Cable-Glands

Cable-Glands (Kabelverschraubungen) sind die professionelle Lösung für
wasserdichte Kabeldurchführungen durch das Gehäuse.

| Größe | Kabel-Ø | Gewinde-Ø | Einsatz |
|---|---|---|---|
| PG7 | 3-6,5 mm | 12,5 mm | Sensorkabel (einzelne Adern) |
| PG9 | 4-8 mm | 15,2 mm | Sensorkabel (mehrere Adern) |
| PG11 | 5-10 mm | 18,0 mm | Stromversorgung |
| PG13.5 | 6-11 mm | 20,1 mm | Display-Flachbandkabel |

Material: Polyamid (PA66) mit NBR-O-Ring und Kontermutter.
IP-Rating: IP67 oder IP68.
Kosten: ~$1-3 pro Stück ([Takachi RPG Katalog](https://takachi-enclosure.com/assets/attachments/images/rpg_catalog.pdf)).

Alternativ: M-Gewinde (metrisch) statt PG — moderner, besser standardisiert.

### 1.5 Belüftung vs. Wasserdichtigkeit (Kondenswasser-Problem)

Ein komplett wasserdichtes Gehäuse klingt ideal, erzeugt aber ein Kondenswasser-Problem:
Temperaturwechsel (kalt nach warm nach kalt) führen dazu, dass Luft im Gehäuse
Feuchtigkeit aufnimmt und bei Abkühlung als Kondensat an den Innenwänden kondensiert.
Bei Elektronik bedeutet das Korrosion und Kurzschluss-Risiko.

**Lösung: GORE Protective Vent (ePTFE-Membran)**

Die Gore-Membran (expanded PTFE) lässt Luft und Wasserdampf in beide Richtungen
passieren, blockt aber flüssiges Wasser und Staub ab. Damit wird der Druck im Gehäuse
permanent ausgeglichen, und Kondenswasser kann als Dampf entweichen.

Eigenschaften:
- IP67/IP68-rated (1 m Wassersäule für 30 min, ohne Wasser-Eintritt)
- Bi-direktionaler Luftaustausch, kein Überdruck-Ventil nötig
- Hydrophob und oleophob
- UV- und temperaturstabil (-40 bis +125°C)
- Snap-in, Screw-in oder Adhesive-Ausführung

([GORE Protective Vents FAQ](https://www.gore.com/resources/faq-gore-protective-vents),
[GORE Screw-In Series](https://www.gore.com/products/screw-protective-vents-outdoor-electronics-enclosures))

Empfohlen für das Bordcomputer-Gehäuse: ein GORE PolyVent XS (M6x0,75, IP68)
an der Unterseite oder Rückseite des Gehäuses — vertikal montiert, damit Wasser
nicht stehen bleibt.

---

## 2. Einbau / Montage

### 2.1 Lenkerdurchmesser und Montage

Die Honda Transalp 600 (PD06) hat einen Standard-Lenker mit **22 mm (7/8")** Durchmesser.
Das ist das Standardmaß für Straßenmotorräder und Reisende. 28 mm (1-1/8") Fatbar
ist bei Enduro/Motocross üblich, nicht bei der Transalp.

Montage-Optionen:

**Option A: Direkte Lenkerhalterung**
- U-Bolt Schelle um den Lenker (22 mm)
- AnkerMake-kompatibles Design: Halterung aus ASA drucken, U-Bolt aus Metall
- Vorteil: einfach, schnell abnehmbar
- Nachteil: lenkerdrehend, nicht ideal für fest verbaute Elektronik

**Option B: Crossbar-Montage (empfohlen)**
- Querstange zwischen den Lenkerhaltern ( sog. Crossbar/Bracebar)
- Die Transalp PD06 hat oft schon eine Crossbar serienmäßig oder nachgerüstet
- Gerät wird an der Crossbar befestigt, nicht am Lenker direkt
- Vorteil: stabilere Plattform, weniger Lenker-Drehmoment, bessere Positionierung

**Option C: Lenkerhalter-Klemmbrücke**
- Montage an den Lenkerhalter-Klemmen (die das Lenkerende fixieren)
- M6 oder M8 Schrauben durch die Klemmbrücke
- Vorteil: sehr stabil, ohne Lenker zu belasten
- Nachteil: modellspezifisch, braucht angepasste Halterung

**Empfehlung: Option B (Crossbar)** mit einer RAM-Mount-kompatiblen Kugelgelenk-Verbindung.
Das erlaubt Neigung und Rotation für optimale Lesbarkeit.
([Quad Lock Motorcycle Mount Guide](https://aerostich.com/pages/quad-lock-motorcycle-mount-guide))

### 2.2 GPS-Antenne: Positionierung

Die GPS-Antenne braucht freie Sicht nach oben zum Himmel. Auf einem Motorrad ist die
beste Position:
- Oberhalb des Lenkers, mit Blick nach oben
- Nicht unter Metallverkleidungen oder Tanks
- Mindestens 10 cm Abstand zum GPS-Modul (bei aktiven Antennen)
- Möglichst weit weg von Zündspule und CDI-Modul (EMV, siehe Abschnitt 6)

Für die Transalp PD06: GPS-Antenne oben auf dem Gehäusedeckel, oder auf
einem kurzen Stiel (~5 cm) nach oben, falls das Gehäuse tief montiert wird.

### 2.3 Display-Ausrichtung und Lesbarkeit

E-Paper-Displays sind bei Sonnenlicht hervorragend ablesbar (reflektiv, kein Backlight
nötig), haben aber ein Problem: der Blickwinkel ist relativ schmal und bei direkter
Sonneneinstrahlung kann das Display zu hell werden (Reflexion).

Empfohlene Neigung: 15-30° nach hinten (vom Fahrer weg geneigt), damit:
- Bei tiefem Sonnenstand (Fahren in Richtung Sonne) das Licht nicht direkt reflektiert
- Bei hohem Sonnenstand (Mittag) das Display von oben beleuchtet wird
- Regenwasser nicht auf dem Display stehen bleibt (Tropfenform)

### 2.4 Kabelführung am Motorrad

Kabelführung auf der Transalp PD06:
- Hauptkabelstrang entlang dem Rahmenrohr (rechte Seite, nicht an der Auspuff-Seite)
- Unter Verkleidungsteilen, wo vorhanden
- An Kabelbindern am Rahmen fixieren (alle 15-20 cm)
- Schrumpfschlauch über Kabelverbindungen als mechanischer Schutz
- Wo Kabel durch Verkleidungsöffnungen gehen: Gummitüllen als Schutz

Wichtig: keine Kabel entlang der Auspuffanlage (Hitze) oder in der Nähe
des CDI-Moduls (EMV).

### 2.5 Stecker-Systeme

Wasserdichte Stecker sind Pflicht. Übersicht:

| System | IP-Rating | Kontaktzahl | Strom | Kosten | Bemerkung |
|---|---|---|---|---|---|
| JST SM (mit Dichtung) | IP65 | 2-6 | 3A | $0,20 | Standard für LED-Lightbars |
| Molex MX150 | IP65 | 2-10 | 8A | $1-3 | Automotive-grade |
| Deusch DT / DTM | IP68 | 2-12 | 13A | $2-5 | Motorsport-Standard |
| AMP Superseal | IP67 | 1-6 | 8A | $0,50-2 | Automobil-Zulieferer |
| USB-C wasserdicht | IP67 | — | 3A | $1-2 | Für Konfiguration/Service |

**Empfohlen: AMP Superseal oder Deusch DTM** für Sensoren und Stromversorgung.
USB-C IP67-Buchse für Service-Zugang.

### 2.6 Vibrationselemente

Zwischen Gehäuse und Lenkerhalterung sollten Gummidämpfer eingefügt werden:
- Gummi-Metall-Verbunddämpfer (Shore 50-60A)
- Reduzieren hochfrequente Vibrationen vom Motor
- Entkoppeln das Gehäuse vom Lenker-Resonanz
- Standard: M6-Gewinde, 10 mm Gummi-Höhe

Gewichtslimit: Das Gesamtgerät sollte nicht mehr als **400-500 g** wiegen.
Ein schweres Gerät am Lenker verändert die Lenkungsgeometrie und verstärkt
Vibrationen durch Masse-Trägheit.

---

## 3. Vibrationen & Stöße

### 3.1 Motorrad-Vibrationsspektrum

Motorräder erzeugen ein breitbandiges Vibrationsspektrum, das deutlich aggressiver ist
als bei Autos. Der Rahmen selbst wirkt als Resonanzstruktur mit minimaler Dämpfung.

Die wichtigsten Vibrationsquellen:

**Motor (Verbrennung):**
- 2nd-order firing frequency: bei 3000 rpm = 100 Hz, bei 6000 rpm = 200 Hz
- Die Transalp 600 V2-Motor (52° V-Twin) feuert jeden 360°-Kurbelwellenumlauf
  einmal pro Zylinder, also bei 3000 rpm ~50 Hz Grundfrequenz
- Oberwellen bis 2 kHz messbar
- SAE J1211 zufolge werden Lenker bei Motorrädern mit **15-25 g RMS** belastet

([Motorcycle PCB Assembly Guidelines](https://bigmateph.com/motorcycle-pcb-assembly-guidelines),
[Sonus Gearflow](https://sonusgearflow.com/audioequipment/how-to-wire-bluetooth-speakers-on-a-motorcycle))

**Straße:**
- Breitbandiges Rauschen 20-500 Hz (unebene Straßenoberfläche)
- Stöße durch Schlaglöcher: kurze Impulse, bis 50 g

**Fahrwerk:**
- Resonanzen je nach Fahrwerksgeometrie

### 3.2 PD06 V2-Motor: Vibrationslevel

Die Honda Transalp 600 hat einen 583 cm³ V2-Motor mit 52° V-Winkel. Er ist
ein bekanntermaßen vibrationsarmer Motor (Ausgleichswellen, gummielagerte
Lenkerhalter), produziert aber trotzdem:
- Primärschwingungen bei Leerlauf (~1200 rpm = 20 Hz Grundfrequenz)
- Sekundärschwingungen bei Fahrt (3000-7000 rpm = 50-117 Hz)
- Resonanzen im Rahmen bei ~40-60 Hz

Im Vergleich zu einem Einzylinder (z.B. KTM 690, bis 30 g am Lenker) ist die
Transalp mit ~5-10 g am Lenker relativ moderat — aber immer noch deutlich mehr
als ein Auto-ECU am Firewall (typisch <1 g).

### 3.3 E-Paper: Vibrationsempfindlichkeit

E-Paper-Displays verwenden ein Glas-Substrat (in der Regel 0,3-0,5 mm dick).
Glas ist druckstabil aber schlagempfindlich. Bei Vibrationen:

- Biegeschwingungen des Glas-Substrats können zu Mikrorissen führen
- Die Flex-Verbindung (FPC) zwischen Glas und Treiber-IC ist der Schwachpunkt
- Befestigung: E-Paper-Modul muss am Gehäuse fest verschraubt sein, nicht klappern

Gegenmaßnahmen:
- E-Paper-Modul auf einer Gummi-Unterlage (0,5 mm Silikon-Pad) montieren
- Schrauben mit Federscheiben sichern
- Displayrahmen aus ASA mit Rippen versteifen, um Biegeschwingungen zu minimieren
- FPC-Kabel nicht unter Spannung, mit etwas Spiel

### 3.4 PCB-Befestigung: Schrauben vs. Kleben

| Methode | Vorteile | Nachteile |
|---|---|---|
| Schrauben + Abstandshalter | Reparierbar, definiert | Löcher im PCB nötig, Resonanz |
| Kleben (Dual-Sided Tape) | Schnell, keine Löcher | Nicht reparierbar |
| Gummipuffer + Schraube | Entkoppelt Vibration | Mehr Bauhöhe |

**Empfohlen: Schrauben mit Gummipuffer** (M2,5 oder M3), 4-Punkt-Befestigung,
an den Ecken des PCB. Abstandshalter aus Metall (starr) + Gummi-Unterlegscheibe.

### 3.5 Steckverbindungen vs. Löten

Vibration löst Steckverbindungen. Im Motorrad gilt:

- **Löten** ist immer vorzuziehen für permanente Verbindungen
- **Steckverbindungen** nur mit Verriegelung (Raste, Schraubgewinde)
- JST-XH ohne Verriegelung löst sich bei Vibration → nicht verwenden
- JST-SM hat Raste, akzeptabel für nicht-sicherheitskritische Signale
- Deusch DT hat Schraubgewinde, maximale Sicherheit

Für alle Kabelverbindungen im Gehäuse: **direkt verlöten + Schrumpfschlauch**
wo möglich. Stecker nur für Service-Zugang (USB-C, Konfiguration).

### 3.6 Verguss (Potting)

Verguss (elektronische Bauteile in Harz einbetten) ist die ultimative Lösung
gegen Vibration und Wasser. Vor- und Nachteile:

**Vorteile:**
- Maximale Vibrationstauglichkeit (Bauteile können nicht schwingen)
- Wasserdichtigkeit bis IP68
- Chemikalien-Resistenz

**Nachteile:**
- Nicht reparierbar (einmal vergossen, kein Zugriff mehr)
- Wärmeableitung verschlechtert (Harz ist Isolator)
- Gewichtszunahme
- Schwer zu modifizieren

**Empfehlung für das Projekt: Selektiver Verguss** — nur die Lötstellen und
empfindlichen Verbindungen vergießen (z.B. mit neutralvulkanisierendem Silikon),
nicht die gesamte Platine. Das schützt die kritischen Punkte ohne das gesamte PCB
unzugänglich zu machen.

### 3.7 Kabel-Sicherung

- Kabelbinder (UV-beständig, schwarz) alle 10-15 cm am Rahmen
- Schrumpfschlauch mit Kleber (adhesive-lined) über alle Verbindungen
- Conduit (Kabelschutzschlauch) für Kabelstränge, die extern laufen
- Kabel nicht frei hängend — immer fixiert

---

## 4. Wasserdichtigkeit

### 4.1 Regen, Spritzwasser, Druckwasser

Auf einem Motorrad ist Wasser allgegenwärtig. Drei Expositionslevel:

**Level 1: Regen und Spritzwasser** — Normales Fahren bei Regen. Wasser kommt von
oben und von vorne, trifft das Gerät mit geringem Druck. IP54 reicht hierfür
oberflächlich, ist aber für Elektronik zu wenig.

**Level 2: Strahlwasser** — Hochdruckreinigung, starke Regenfälle bei Geschwindigkeit.
Wasser trifft mit Druck aus allen Richtungen. IP65 (Wasserstrahl) ist das Minimum.

**Level 3: Eintauchen / Druckwasser** — Furt-Durchfahrten, Waschstraße mit Wasserstand.
Wasser steht um das Gerät herum, steht unter hydrostatischem Druck. IP67
(30 min, 1 m Wassertiefe) oder IP68 erforderlich.

Die Realität: ein Motorrad-Bordcomputer am Lenker wird selten komplett eingetaucht,
aber Furt-Durchfahrten und Waschstraßen sind real. **IP67 ist das realistische Minimum.**

### 4.2 IP-Ratings erklärt

IP-Ratings (IEC 60529) sind eine zweistellige Kennzahl. Erste Ziffer: Feststoffschutz,
zweite Ziffer: Wasserschutz. Wichtig: die Ziffern sind kumulativ für den jeweiligen
Schutz, aber Wasser-Level sind nicht zwingend kumulativ.

**Erste Ziffer (Feststoffe):**
- 5: Staubgeschützt (begrenzter Eindringung, keine schädliche Menge)
- 6: Staubdicht (kein Eindringung)

**Zweite Ziffer (Wasser):**
- 4: Spritzwasser aus allen Richtungen
- 5: Wasserstrahl (6,3 mm Düse, 12,5 l/min, 3 m Abstand)
- 6: Starkes Wasserstrahl (12,5 mm Düse, 100 l/min)
- 7: Kurzzeitiges Eintauchen (1 m, 30 min)
- 8: Dauerhaftes Eintauchen (herstellerspezifisch)

**Wichtige Erkenntnis:** IP67 bedeutet NICHT automatisch, dass das Gerät auch
Wasserstrahl (IP65/66) standhält. Es sind verschiedene Tests mit verschiedenen
Beanspruchungen. Ein IP67-Gehäuse kann bei Hochdruckreinigung versagen, weil der
Strahl eine andere Dichtungsbelastung erzeugt als statisches Eintauchen.
([JADO Moto IP Guide](https://www.jadomoto.com/blogs/guides/waterproofing-and-ip-ratings-explained),
[NiMotion IP Guide](https://www.nimotion.com/blog/detail/ip-rating-motor-protection-guide))

**Für das Projekt: IP65/IP67 Dual-Rating** anstreben — das bedeutet das Gehäuse
hält sowohl Wasserstrahl als auch kurzes Eintauchen aus.

### 4.3 Conformal Coating (Schutzlack für PCB)

Conformal Coating ist ein dünner Polymer-Film (25-250 µm), der direkt auf die
bestückte Platine aufgetragen wird. Er schützt vor Feuchtigkeit, Staub, Chemikalien
und Temperaturschock.

([Atlas PCB Coating Guide](https://atlaspcb.com/blog/pcb-conformal-coating-selection-guide),
[Andwin PCB Guide](https://andwinpcb.com/conformal-coating-for-pcb-types-application-and-selection-guide))

| Typ | Dicke | Temp-Range | Chemie-Resistenz | Rework | Kosten | Fazit |
|---|---|---|---|---|---|---|
| Acryl (AR) | 25-75 µm | -65 bis +125°C | Mäßig | Einfach (Lösungsmittel) | Niedrig | Prototyp, gut zu reparieren |
| Silikon (SR) | 50-210 µm | -65 bis +200°C | Gut | Schwer | Hoch (2-3x Acryl) | Wenn >150°C |
| Polyurethan (UR) | 25-75 µm | -65 bis +130°C | Sehr gut | Schwer | Mittel | Motorrad-Einsatz |
| Epoxid (ER) | 25-75 µm | -65 bis +150°C | Sehr gut | Fast unmöglich | Mittel | Permanent |
| Parylen (XY) | 5-50 µm | -70 bis +125°C | Sehr gut | Fast unmöglich | Sehr teuer | Aerospace |

**Empfehlung: Polyurethan (UR)** für das Bordcomputer-PCB.
Gründe: beste Balance aus Feuchtigkeitsschutz, Chemikalien-Resistenz (Kraftstoff,
Öl, Reinigungsmittel), und Temperaturbereich. Acryl ist für Prototypen akzeptabel
und einfacher zu entfernen bei Reparatur.

Auftragsmethode: Pinsel oder Sprühdose (2-3 dünne Schichten), für Prototyp
ausreichend. Für Produktion: selektive Beschichtungsanlage.

Wichtig vor dem Coating:
- Maskieren: Stecker, Lötpads, HF-Antennen nicht beschichten
- Reinigen: PCB mit IPA (Isopropanol) entfetten, Feuchtigkeit entfernen
- Aushärten: nach Herstellerangaben, bei UR-Typen 24h Raumtemperatur oder 60min bei 80°C

### 4.4 Wasserdichte Gehäuse-Design-Prinzipien

**Tropfenform:** Das Gehäuse sollte so geformt sein, dass Wasser von oben abläuft.
Keine horizontalen Flächen, auf denen Wasser stehen kann. Runde Oberseite,
Schräge Wände, nach unten offene Überlappungen.

**Überlappung (Shingling):** Deckel und Boden überlappen. Der Deckel greift über
das Unterteil, so dass Wasser, das an der Naht entlangläuft, an der Außenseite
herunterläuft und nicht ins Innere gelangt. Wie Dachschindeln.

**Naht-Position:** Die Deckelnaht sollte an der Unterseite oder Rückseite liegen,
nicht oben. Wasser, das von oben kommt, trifft auf eine geschlossene Fläche.

**Schrauben-Position:** Schrauben nicht von oben in das Gehäuse gehen lassen
(Wasser läuft in das Schraubenloch). Lieber seitliche oder von unten eingreifende
Schrauben. Wenn von oben: mit O-Ring unter dem Schraubenkopf.

### 4.5 Wasserdichte Kabeldurchführungen

Cable-Glands mit O-Ring-Dichtung (siehe Abschnitt 1.4). Wichtig:
- Cable-Gland von außen durch das Gehäuse stecken, Kontermutter innen
- O-Ring zwischen Gland und Gehäusewand (nicht nur zwischen Gland und Kabel)
- Kabel-Ø genau passend zum Gland wählen (zu dünn = Wasserweg, zu dick = Klemmt nicht)

### 4.6 USB-C / Ladebuchse wasserdicht

USB-C-Buchsen mit IP67-Schutz gibt es als Bauteil (z.B. Amphenol 10137065-0001LF,
oder generische IP67 USB-C-Panel-Mount-Buchsen). Diese haben eine integrierte
Dichtungsmembran, die die Kontakte abdeckt, wenn kein Stecker eingesteckt ist.

Alternative: USB-C-Buchse im Inneren des Gehäuses, Zugang nur mit geöffnetem Deckel.
Das eliminiert die Schwachstelle komplett, ist aber im Betrieb unpraktisch.

### 4.7 Was passiert bei Wassereintritt?

Wenn Wasser ins Gehäuse gelangt (und das passiert irgendwann — Dichtungen altern,
Cable-Glands lockern sich, Gehäuse reißt):

1. **Kondensat-Entlüftung:** GORE-Vent lässt Wasserdampf entweichen (siehe 1.5)
2. **Abfluss:** Konstruktiv sicherstellen, dass Wasser unten raus kann. Ein kleines
   Loch an der tiefsten Stelle des Gehäuses, das mit einer GORE-Membran verschlossen ist
3. **Conformal Coating:** Schützt das PCB auch bei Wassereintritt kurzzeitig
4. **Keine Lebensdauer-Garantie:** IP67 bedeutet "30 min 1 m Wassertiefe neu".
   Nach 2-3 Saisons Fahrrad-Einsatz ist die Dichtung verschlechtert.
   Dichtungen und Gaskets als Verschleißteile behandeln.

([JADO Moto](https://www.jadomoto.com/blogs/guides/waterproofing-and-ip-ratings-explained))

---

## 5. Temperaturbereich

### 5.1 Motorrad-Aussentemperatur

Motorrad-Einsatz bedeutet: -10°C im Winter bis +50°C im Sommer (in Deutschland,
extremere Bereiche bei Reisen nach Südeuropa oder Skandinavien möglich).

| Szenario | Temperatur | Quelle |
|---|---|---|
| Winterfahrt, -10°C | -10°C | Deutschland |
| Sommerfahrt, Sonne | +35°C | Deutschland |
| Gehäuse in direkter Sonne (schwarz) | +50-60°C | Sonnenstrahlung + Eigenwarme |
| Gehäuse in der Sonne (weiss) | +40-45°C | Bessere Reflexion |
| Im Gehäuse, ESP32 aktiv | +10-15°C über Außentemperatur | Eigenwärme |

### 5.2 Innentemperatur Gehäuse

Das Gehäuse wird wärmer als die Umgebung, weil:
- Sonnenstrahlung auf das Gehäuse (besonders bei dunklen Farben)
- Eigenwärme der Elektronik (ESP32 ~50-100 mW, DC/DC ~200-500 mW)
- Keine aktive Kühlung (geschlossenes Gehäuse)

Bei +35°C Außentemperatur und Sonne kann das Gehäuseinnere **60-70°C** erreichen.
Das ist der kritische Bereich für mehrere Komponenten.

**Gegenmaßnahme: Gehäuse hell drucken** (weisses ASA oder hellgrau), um Sonnenstrahlung
zu reflektieren. Ein schwarzes Gehäuse absorbiert bis zu 90% der Sonnenenergie und
wird deutlich wärmer.

### 5.3 ESP32-Temperatur-Limit

Der ESP32 (Standard-Variante) ist spezifiziert für **-40°C bis +85°C**
Ambient-Temperatur. Es gibt auch eine +105°C-Variante, die aber nur mit
4/8 MB Flash verfügbar ist.

([ESP32 Module](https://www.espressif.com/en/products/modules/esp32),
[ESP32-WROOM-32E Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf))

Bei 70°C Innentemperatur hat man noch 15°C Margin bis zum +85°C-Limit.
Das ist knapp aber akzeptabel, wenn:
- Der ESP32 nicht dauerhaft unter Volllast läuft (Deep Sleep zwischen Updates)
- Das Gehäuse hellfarbig ist
- Keine zusätzlichen wärmeerzeugenden Komponenten im Gehäuse (z.B. kein LDO-Regler,
  der als Heizung wirkt)

**Wenn das +85°C-Limit kritisch wird:** Die +105°C-Variante des ESP32-WROOM-32E
verwendet den Flash-Chip, der bis 105°C spezifiziert ist. Das gibt 35°C Margin.

### 5.4 LiPo-Akku: Temperaturbereich

Der LiPo-Akku (Puffer für Spannungsausfälle) hat das engste Temperaturfenster
aller Komponenten:

| Zustand | Temperaturbereich |
|---|---|
| Entladen | -20°C bis +60°C |
| Laden | 0°C bis +45°C |
| Lagerung | 10°C bis +25°C (ideal) |
| Notabschaltung | >70°C (Thermal Runaway Risk) |

([LiPo Temperature Range](https://ufinebattery.com/blog/what-is-the-minimum-operating-temperature-for-lipo-batteries),
[REDARC Battery Guide](https://us.support.redarcelectronics.com/hc/en-us/articles/13856244101007-Why-you-should-not-charge-a-lithium-battery-below-0-C-or-32-F))

**Kritisch: Laden bei unter 0°C** — Lithium-Plating kann auftreten, was die
Kapazität dauerhaft reduziert und im schlimmsten Fall zum internen Kurzschluss
führt. Der BMS/Charger-IC muss die Ladung bei <0°C blockieren.

**Kritisch: Laden bei über 45°C** — Der Ladestrom sollte reduziert werden,
über 50°C gar nicht mehr geladen werden.

**Für das Projekt:** LiPo-Akku nur klein dimensionieren (z.B. 1000-2000 mAh)
als Puffer für kurze Spannungsausfälle, nicht als Hauptenergiequelle. Den Akku
thermisch vom ESP32 trennen (anderes Gehäuseteil, Wärmebrücke nach außen).
Im Winter: Ladung bei <0°C per Software blockieren (Temperatursensor am Akku).

### 5.5 E-Paper-Temperatur-Range

Standard-E-Paper-Displays sind spezifiziert für **0°C bis +50°C** (Betrieb).
Breittemperatur-Varianten erreichen **-20°C bis +60°C**.

([Pervasive Displays Wide-Temperature Guide](https://www.pervasivedisplays.com/wide-temperature-e-ink/),
[Display-Hersteller e-Paper Manual])

**Im Winter bei -10°C:** Standard-E-Paper funktioniert nicht zuverlässig.
Die Flüssigkeit in den Mikrokapseln wird viskoser, Update-Zeit steigt drastisch
(von 2 s auf 10+ s), bei tieferen Temperaturen kein Update mehr möglich.

**Lösung: Breittemperatur-E-Paper** verwenden (z.B. Pervasive Displays EPD,
Display-Hersteller wide-temp Variante). Diese sind spezifiziert für -20°C bis +60°C
und decken den Motorrad-Temperaturbereich ab.

**Im Sommer bei +50°C:** E-Paper ist weniger kritisch bei Hitze. Die obere
Grenze ist +50°C (Standard) bzw. +60°C (Wide-Temp). Bei +70°C Innentemperatur
im Gehäuse (Sonne + Eigenwärme) ist aber auch das Wide-Temp-Display am Limit.
Abhilfe: Gehäuse hell, Display im Schatten des Gehäusedeckels.

### 5.6 Sensor-Temperatur-Range

Besonders der Kühlmitteltemperatur-Sensor ist kritisch, da er direkt am Motor
montiert wird. Dort können Temperaturen bis 120°C am Sensor auftreten.

- Kühlmitteltemperatur-Sensor: original PD06-Sensor ist ein NTC-Thermistor,
  spezifiziert bis 120°C
- Öltemperatur-Sensor: bis 150°C
- Außentemperatur-Sensor: im Gehäuse, -10 bis +50°C

Der ADC am ESP32 muss den NTC-Widerstand messen. Spannungsteiler mit
Referenzwiderstand (1% Toleranz) und 10-bit-ADC reicht für ±2°C Genauigkeit.

---

## 6. EMV (Elektromagnetische Verträglichkeit)

### 6.1 Zündfunken und CDI als EMV-Quellen

Die Honda Transalp 600 (PD06) verwendet eine CDI (Capacitive Discharge
Ignition). Bei der CDI wird ein Kondensator auf 200-400 V aufgeladen und
entlädt sich über die Zündspule, die die Spannung auf 20-40 kV hochtransformiert.
Dieser Entladevorgang erzeugt einen steilen Strompuls (Stromsteigung
bis 100 A/µs), der ein breitbandiges elektromagnetisches Spektrum
von wenigen kHz bis über 100 MHz erzeugt.

([CDI EMV-Grundlagen: Electronicshub CDI Guide](https://www.electronicshub.org/capacitor-discharge-ignition-system/))

EMV-Probleme durch die Zündung äussern sich als:
- Spannungsspitzen auf der 12V-Versorgung (Ripple bis 5 Vss)
- Elektromagnetische Einstreuung in Sensorleitungen (Störsignale
  auf NTC-Sensoren, Geschwindigkeitssignalen)
- Reset des ESP32 durch EMV-Pulse auf GPIO- oder Versorgungsleitungen
- GPS-Empfangsverlust durch Breitbandstörung der Antenne

Zusätzliche EMV-Quellen am Motorrad:
- Lichtmaschine (Wechselstromgenerator, Ripple auf 12V)
- Relais (Induktive Spannungsspitzen beim Öffren)
- Blinker (mechanischer Blinkgeber erzeugt Lastwechsel)
- Elektromagnet (Hupe)

### 6.2 Abschirmung des Gehäuses

Ein 3D-gedrucktes ASA-Gehäuse bietet von sich aus keine elektromagnetische
Abschirmung — Kunststoff ist für EMV transparent. Massnahmen:

**Methode 1: Abschirmfolie innen**
- Kupferfolie (selbstklebend, 0,05 mm) auf die Innenseite des Gehäuses kleben
- Schirmwirkung: 40-60 dB im Bereich 30-1000 MHz
- Masse-Anschluss: Folie mit einer Lasche an die PCB-Masse anschliessen
- Kosten: ~$5-10 pro Gehäuse

([Copper Tape EMV Shielding](https://www.dupont.com/electronic-materials/electromagnetic-shielding.html))

**Methode 2: Alu-Platte als innere Abschirmung**
- Dünne Alu-Platte (0,5-1 mm) im Gehäuseboden, PCB montiert darauf
- Schirmwirkung: 60-80 dB
- Zugleich Wärmeleitung (Alu leitet Wärme vom ESP32 nach aussen)
- Gewicht: ~20-30 g zusätzlich

**Methode 3: Vollständiges Alu-Gehäuse**
- Statt 3D-Druck ein Aluminium-Druckguss-Gehäuse verwenden
- Maximale EMV-Abschirmung (80-100 dB)
- Aber: schwerer, teurer, nicht selbst herstellbar
- Beispiel: Takachi ES-Alu-Serie

**Empfehlung für das Projekt:** Methode 1 (Kupferfolie innen) als
Minimum-Massnahme. Wenn EMV-Probleme auftreten, auf Methode 2 (Alu-Platte)
wechseln. Die Kupferfolie ist einfach nachträglich aufklebbar und
kompensiert die fehlende metallische Abschirmung des ASA-Gehäuses.

### 6.3 Kabel-Abschirmung

Sensorleitungen, die aus dem Gehäuse herausgehen, können als Antennen
wirken und EMV-Störungen in das Bordcomputer-System einkoppeln.

**Geschirmte Kabel:**
- Sensorkabel (Kühlmitteltemp, Geschwindigkeit, Drehzahl) mit geflochtener
  Schirmung (SFTP oder S/UTP) ausführen
- Schirm an EINEM Ende auf Masse legen (Gehäuseseite), um Masseschleifen
  zu vermeiden
- Schirmwirkung eines geflochtenen Schirms: 80-90% Abdeckung bei
  Hochfrequenz

**Verdrillte Adern (Twisted Pair):**
- Für analoge Sensorsignale: verdrillte Adern reduzieren induktive
  Einkopplung um 20-40 dB
- Besonders wichtig für NTC-Sensoren mit langen Leitungen (>30 cm)

### 6.4 Filterung: RC-Glieder, Ferritkerne, Kondensatoren

**RC-Glieder an GPIO-Eingängen:**
- Jeder Digitaleingang, der aus dem Motorrad-Netz kommt (Geschwindigkeit,
  Drehzahl, Licht an/aus), braucht einen Tiefpass
- Werte: R = 1-10 kOhm, C = 10-100 nF
- Grenzfrequenz: fc = 1/(2*pi*R*C) — bei R=10k, C=100nF: ~160 Hz
- Das filtert hochfrequente Störungen (Zündfunken) heraus, lässt aber
  niederfrequente Signale (Geschwindigkeit <200 Hz) passieren

**Ferritkerne:**
- Ferritkern (Klappferrit) um jedes Kabel, das ins Gehäuse mündet
- Wirkt als Drossel für Hochfrequenz (impedanz steigt mit Frequenz)
- Typ: Material 43 oder 73, Impedanz >100 Ohm bei 100 MHz
- Platz: möglichst nah am Gehäuse, um die Antennenwirkung des
  externen Kabels zu minimieren

([Ferrite Bead Selection Guide](https://www.analog.com/en/design-notes/selecting-the-right-ferrite-bead-for-emc-compliance.html))

**Entkopplungskondensatoren:**
- An jedem IC-Versorgungspin: 100 nF Keramikkondensator, möglichst nah
  am Pin
- Zusätzlich Bulk-Kondensator (10-100 µF Elektrolyt) am Board-Eingang
- Für den ESP32: eigene 10 µF und 100 nF Kombination an der 3,3V-Versorgung

### 6.5 Massefuehrung: Sternpunkt und Masseschleifen

**Sternpunkt-Masse (Single-Point Ground):**
- Alle Masseleitungen laufen an einem einzigen Punkt zusammen (Sternpunkt)
- Der Sternpunkt liegt am Gehäuse / Akku-Minus / DC/DC-Masse
- Vermeidet Masseschleifen: wenn Strom durch verschiedene Massepfade
  fliesst, entstehen Spannungsabfälle, die Sensoren verfälschen

**Masseschleifen vermeiden:**
- Eine Masseverbindung zwischen PCB und Gehäuse (nicht mehrere)
- Sensor-Masse nicht mit Gehäuse verbinden (nur über Sternpunkt)
- Geschirmte Kabel nur an einem Ende erden

**Masselage des ESP32:**
- ESP32 hat separate Masse-Pins für analog und digital
- Analog-Masse (AGND) und Digital-Masse (DGND) sternförmig verbinden
- Kein gemeinsames Massepolygon für analoge und digitale Bereiche

### 6.6 Optokoppler für galvanische Trennung

Alle Signale, die direkt vom Motorrad-Netz in den ESP32 gehen
(Zündung, Licht, Blinker), sollten galvanisch getrennt werden.

**Optokoppler-Wahl:**

| Optokoppler | CTR | Isolation | Schaltfrequenz | Kosten | Bemerkung |
|---|---|---|---|---|---|
| PC817 | 50-600% | 5000 V | 30 kHz | $0,10 | Standard, 4-polig DIP |
| 6N137 | 200% | 5000 V | 10 MHz | $0,50 | Schnell, für hohe Frequenz |
| HCPL-2630 | 50% | 3750 V | 10 MHz | $1,00 | Dual, High-Speed |
| TLP291 | 50-400% | 5000 V | 30 kHz | $0,15 | SMD, klein |

**Anwendung im Bordcomputer:**
- Zünd-Impuls (Pickup-Spule der CDI): Optokoppler mit Vorwiderstand
  und Spannungsteiler, um den Hochspannungspuls auf LED-Pegel zu bringen
- Licht an/aus, Blinker: Optokoppler direkt an die 12V-Leitung
  mit entsprechendem Vorwiderstand
- Geschwindigkeitssignal: falls Hall-Sensor am Motorrad, direkt am ESP32
  mit Pull-up und RC-Glied (nicht zwingend galvanisch getrennt)

([PC817 Datasheet](https://www.vishay.com/docs/83608/pc817.pdf),
[6N137 Datasheet](https://www.vishay.com/docs/83608/6n137.pdf))

### 6.7 Ueberspannungsschutz

Die Motorrad-Lichtmaschine erzeugt beim Lastabwurf (Load Dump) Spannungsspitzen
von bis zu 100-200 V für 50-400 ms. Das ist die aggressivste
elektrotechnische Belastung am Motorrad. Siehe Abschnitt 7.2.

---

## 7. Stromversorgung im Motorrad-Umfeld

### 7.1 Spannungsschwankungen im 12V-Netz

Die 12V-Versorgung am Motorrad ist alles andere als stabil. Sie schwankt
je nach Betriebszustand:

| Zustand | Spannung | Ursache |
|---|---|---|
| Anlassen (Starten) | 8-10 V | Starter-Motor zieht 50-100 A |
| Leerlauf | 12-13 V | Lichtmaschine noch nicht voll erregt |
| Fahrt (3000 rpm) | 13,5-14,2 V | Lichtmaschine reguliert |
| Hohe Drehzahl (>6000 rpm) | 14,2-15 V | Regler an der Grenze |
| Lastabwurf (Load Dump) | bis 100-200 V | Last wird plötzlich abgeworfen |
| Batterie leer | <11 V | Tiefentladung |

([Motorcycle Electrical Load Dump](https://www.ti.com/lit/an/slvaef9/slvaef9.pdf),
[ISO 7637-2 Pulse 5](https://www.infineon.com/dgdl/Infineon-Load_dump-ApplicationNotes-v01_00-EN.pdf))

### 7.2 Load-Dump-Protection

Der Lastabwurf (Load Dump) ist die kritischste Störung. Wenn die Lichtmaschine
unter Volllast läuft und die Batterie (als Puffer) plötzlich abgeklemmt wird
— z.B. durch Wackelkontakt am Batterieklemmen —, entsteht eine
Spannungsspitze von 100-200 V für bis zu 400 ms.

**Schutzmassnahmen:**

**TVS-Diode (Transient Voltage Suppressor):**
- Typ: SMBJ oder SMCJ mit Standoff-Spannung >15 V und Clamping bei ~24-30 V
- Beispiel: SMBJ33A (Standoff 33 V, Clamping 53,3 V) oder für aggressiveren
  Schutz: SMCJ24A (Standoff 24 V, Clamping 38,9 V)
- Die Diode leitet bei Überspannung und begrenzt die Spannungsspitze
- Reagiert in Nanosekunden
- Kosten: $0,50-2,00

([Littelfuse TVS Diode Selection](https://www.littelfuse.com/technical-resources/technical-center/selection-guides/tvs-diode-selection-guide),
[STMicro Load Dump Protection](https://www.st.com/resource/en/application_note/dm00180430-load-dump-protection-with-tvs-diodes-stmicroelectronics.pdf))

**Varistor (MOV):**
- Metalloxid-Varistor, z.B. 275 V AC Rating
- Reagiert langsamer als TVS-Diode, aber kann mehr Energie aufnehmen
- Kombination: Varistor + TVS-Diode für maximalen Schutz

**Kombinationsschaltung (empfohlen):**
1. Varistor (MOV) als erste Stufe — grosse Energieaufnahme
2. TVS-Diode als zweite Stufe — schnelle Begrenzung
3. Polyswitch (Sicherung) — schützt bei Dauerüberstrom
4. DC/DC-Wandler — hat eigenen Ueber- und Unterspannungsschutz

### 7.3 Reverse-Polarity-Protection

Verpolungsschutz: wenn jemand die Batterie falsch anklemmt (Plus auf Minus).

**Diode (einfach):**
- Schottky-Diode in Reihe (z.B. SS54, 5A, 0,55 V Drop)
- Verlust: 0,55 V * 5 A = 2,75 W Wärme (braucht Kühlkörper)
- Einfach, billig ($0,10), aber verlustreich

**MOSFET (besser):**
- P-Kanal MOSFET als High-Side-Switch (z.B. IRF4905)
- RDS(on) = 0,02 Ohm → Verlust bei 5A = 0,5 W (10x besser als Diode)
- Gate wird gegen Source gezogen, Source-Kanal sperrt bei Verpolung
- Kosten: $0,50-1,00

([Reverse Polarity Protection with MOSFET](https://www.analog.com/en/design-notes/protecting-circuits-using-mosfets-as-reverse-polarity-protection.html))

**Empfehlung: MOSFET** — geringerer Spannungsabfall, weniger Wärme,
bessere Effizienz. Die Diode ist akzeptabel für Prototypen mit
geringem Strom.

### 7.4 DC/DC-Wandler mit weiter Input-Range

Der Bordcomputer braucht 5V und 3,3V. Die 12V-Versorgung schwankt
zwischen 8V (Anlasser) und 15V (hohe Drehzahl). Ein Linearregler
(7805) würde bei 8V Input keine 5V mehr liefern und bei 15V Input
viel Wärme erzeugen.

**Schaltrregler (Buck Converter) empfohlen:**
- Input-Range: 7-36 V (deckt alle Motorrad-Zustände ab)
- Output: 5V/2A
- Effizienz: >90%
- Integrierter Schutz: Ueberstrom, Ueberhitzung, Ueberspannung

**Module fuer Prototyp:**
- LM2596 (3A, 40V Input, $1-2) — Standard, gut dokumentiert
- MP1584 (3A, 28V Input, $0,50) — Klein, SMD
- TS2596 (2A, 40V Input) — Alternative

**Für 3,3V:**
- Direkt vom ESP32-Board (onboard LDO AMS1117-3,3)
- Oder separater LDO (AMS1117-3,3) vom 5V-Zweig

**Empfehlung:** LM2596-Modul für Prototyp, später Custom-PCB mit
integriertem Schaltrregler. Darauf achten: Switching-Frequenz des
Reglers erzeugt selbst EMV (Abschirmung, Input-/Output-Filter).

### 7.5 Batterie als Puffer

Die 12V-Batterie am Motorrad wirkt als Puffer gegen Spannungsspitzen
und -einbrüche. Die Bleibatterie hat einen sehr niedrigen Innenwiderstand
(~10 mOhm), was bedeutet, dass sie kurzzeitige Spitzen absorbiert.

**Aber:**
- Wenn die Batterie alt/schwach ist, steigt der Innenwiderstand →
  Spikes kommen ungefiltert an das Bordcomputer-Netz
- Wenn die Batterie abgeklemmt wird (Wartung), fehlt der Puffer komplett
- Der LiPo-Akku im Bordcomputer ist KEIN Ersatz für die Motorrad-Batterie
  als Lastabwurf-Puffer (zu geringe Kapazität, zu hoher Innenwiderstand)

**Massnahme:** Zusätzlicher Elko (4700-10000 µF, 25V) am Bordcomputer-Eingang
als lokaler Energie-Puffer für kurze Einbrüche (Anlasser, Lastwechsel).
Das ersetzt nicht die TVS-Diode, aber glättet kurzzeitige Einbrüche.

([Capacitor as Load Dump Buffer](https://www.vishay.com/docs/29022/aluminumcapacitorsforload_dump.pdf))

---

## 8. Praktische Einbau-Beispiele

### 8.1 Aftermarket-Dashboards und Motorrad-Navi

**Motorrad-Navigationsgerät (Referenzprodukt):**
- Gehäuse: ruggedized Kunststoff, IPX7 (1m Wassersäule 30 min)
- Montage: RAM-Mount Kugelgelenk + Crossbar-Adapter
- Display: 6" transflektiv LCD (nicht E-Paper)
- Temperatur: -20°C bis +60°C Betrieb
- Lernpunkt: transflektives Display für Sonne, ruggedized Design,
  austauschbare Batterie

**Motorrad-Navigationsgerät (Referenzprodukt, zweites Modell):**
- Gehäuse: Gummierte Oberfläche, IPX5 (Spritzwasser, nicht eintauchen)
- Montage: RAM-Mount kompatibel
- Temperatur: -10°C bis +50°C
- Lernpunkt: RAM-Mount ist Industriestandard, austauschbare Halterung

**Offroad-Motorrad-Display (Referenzprodukt):**
- Motorsport-spezifisches Display für Offroad-Motorräder
- Gehäuse: ABS, IP65
- Montage: Lenkerhalter-Klemmbrücke
- Display: 4" LCD (Tagfahrtauglich)
- Lernpunkt: Klemmbrücke statt Lenker-Montage für maximale Stabilität

### 8.2 Motorrad-Elektronik-Projekte (Open Source)

**CAFI (Custom Motorcycle Fuel Injection):**
- DIY-Motorrad-EFI-Controller
- Gehäuse: 3D-gedruckt (PETG), IP54
- PCB: conformal coated (Acryl)
- Lernpunkt: PETG-Gehäuse funktioniert, aber nur bei gemässigtem Einsatz

**MicroDashboard (GitHub: mikefikes/microdashboard):**
- ESP32-basiertes Motorrad-Dashboard
- Gehäuse: Keeb-Box (Aluminium-Druckguss)
- Display: 2,4" TFT
- Lernpunkt: Alu-Gehäuse für EMV-Schutz, aber schwer

### 8.3 Lessons Learned aus kommerziellen Produkten

Zusammenfassung der Design-Prinzipien, die sich aus kommerziellen
Motorrad-Elektronik-Produkten ableiten lassen:

1. **IP67 als Minimum, IP65 für hochdruck-gefahrte Bereiche**
   - Kommerzielle Produkte (Motorrad-Navigationsgeräte) nutzen IPX7 oder IP67
2. **RAM-Mount als Standard-Halterung**
   - Fast alle kommerziellen Produkte nutzen RAM-Mount kompatibel
   - Gibt es in jeder erdenklichen Konfiguration
3. **Transflektives oder reflektives Display für Sonnenlicht**
   - LCD mit Backlight ist in der Sonne kaum lesbar
   - E-Paper ist ideal für Motorrad-Einsatz (reflektiv, extrem
     lesbar, minimaler Stromverbrauch)
4. **Kabelanschlüsse an der Unterseite**
   - Wasser laeuft nicht in Kabeldurchfuehrungen, wenn sie nach unten zeigen
   - GORE-Vent an der Unterseite fuer Druckausgleich
5. **Alles verlöten, nichts stecken**
   - Kommerzielle Motorrad-Elektronik hat keine abnehmbaren Stecker
     im Innenraum (nur Service-Stecker)
   - Ständige Vibration löst jede Raste irgendwann
6. **Gehäusefarbe: hell**
   - Schwarze Gehäuse absorbieren Sonnenstrahlung, werden zu heiss
   - Helle Farben (weiss, hellgrau) reflektieren, +10-15°C kuehler
7. **Vibrationsoptimiertes PCB-Design**
   - Schwerere Bauteile (Kondensatoren >10µF) so nah wie moeglich am PCB
     und an der Befestigung
   - Keine schweren Bauteile am Rand des PCB (Hebelwirkung bei Vibration)
8. **Schraubsicherung mit Federscheibe**
   - Alle Schrauben am Motorrad brauchen Federscheibe (Nord-Lock oder
     mindestens DIN 127B) gegen Vibration

### 8.4 Empfohlene Material-Liste für das Projekt

Basierend auf dieser Recherche die konkrete Empfehlung für das
Transalp-Bordcomputer-Projekt:

| Bauteil | Wahl | Begründung |
|---|---|---|
| Gehäusematerial | ASA (hell, weiss/hellgrau) | UV-stabil, bis 100°C, UV-resistent |
| Dichtung Deckel | Silikon-O-Ring (2 mm Cord) | -40 bis +200°C, UV-stabil |
| Kabeldurchführung | PG7/PG9 Cable-Glands PA66 | IP67, günstig, etabliert |
| Druckausgleich | GORE PolyVent XS (M6) | IP68, bi-direktional, Kondensat-Entweichen |
| Stecker Sensoren | AMP Superseal 1-6-polig | IP67, automotive-grade |
| Stecker Service | USB-C IP67 Panel-Mount | Konfiguration ohne Gehäuse-Oeffnen |
| PCB-Schutz | Conformal Coating Polyurethan | Feuchtigkeit, Chemikalien, Temperatur |
| EMV-Abschirmung | Kupferfolie selbstklebend innen | 40-60 dB Schirmdämpfung |
| EMV-Filterung | Ferritkerne an allen externen Kabeln | HF-Entkopplung, einfach nachrüstbar |
| Optokoppler | PC817 (Standard-Signale), 6N137 (Drehzahl) | Galvanische Trennung von Zündung/Licht |
| Ueberspannungsschutz | TVS SMCJ24A + MOV 275V | Lastabwurf-Schutz in 2 Stufen |
| Verpolungsschutz | P-Kanal MOSFET IRF4905 | 0,02 Ohm RDS, minimal Verlust |
| DC/DC-Wandler | LM2596 7-40V → 5V/2A | Deckt 8-15V Motorrad-Netz ab |
| Puffer-Elko | 4700µF/25V am Eingang | Kurzzeit-Spannungseinbrüche abfangen |
| LiPo-Akku | 1000-2000 mAh, thermisch getrennt | Puffer für Spannungsausfall, nicht Hauptenergie |
| PCB-Befestigung | M2,5 Schrauben + Gummipuffer | Vibration-entkoppelt, reparierbar |
| Halterung | RAM-Mount kompatibel (Crossbar) | Industriestandard, justierbar |
| Display | E-Paper Wide-Temp (-20 bis +60°C) | Motorrad-Temperaturbereich abgedeckt |
| Gewichtslimit | <400 g gesamt | Lenker-Geometrie nicht beeinträchtigen |

---

## Quellen

- [CNC Kitchen: PLA vs PETG vs ASA](https://www.cnckitchen.com/blog/comparing-pla-petg-amp-asa-feat-prusament)
- [PolyLite ASA TDS](https://cdn.shopify.com/s/files/1/0548/7299/7945/files/PolyLite_ASA_TDS_V5.1.pdf)
- [Takachi Cable Glands](https://www.takachi-enclosure.com/cat/cable_glands)
- [GORE Protective Vents FAQ](https://www.gore.com/resources/faq-gore-protective-vents)
- [GORE Screw-In Vents](https://www.gore.com/products/screw-protective-vents-outdoor-electronics-enclosures)
- [IEC 60529 IP Ratings](https://www.iec.ch/homepage)
- [JADO Moto IP Guide](https://www.jadomoto.com/blogs/guides/waterproofing-and-ip-ratings-explained)
- [Atlas PCB Conformal Coating Guide](https://atlaspcb.com/blog/pcb-conformal-coating-selection-guide)
- [Andwin PCB Conformal Coating Guide](https://andwinpcb.com/conformal-coating-for-pcb-types-application-and-selection-guide)
- [ESP32 Modules](https://www.espressif.com/en/products/modules/esp32)
- [ESP32-WROOM-32E Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf)
- [LiPo Temperature Range](https://ufinebattery.com/blog/what-is-the-minimum-operating-temperature-for-lipo-batteries)
- [REDARC Battery Temperature Guide](https://us.support.redarcelectronics.com/hc/en-us/articles/13856244101007-Why-you-should-not-charge-a-lithium-battery-below-0-C-or-32-F)
- [Pervasive Displays Wide-Temp E-Ink](https://www.pervasivedisplays.com/wide-temperature-e-ink/)
- [Display-Hersteller e-Paper Manual]
- [SAE J1211 Motorrad-Vibration](https://www.sae.org/standards/content/j1211/)
- [Electronicshub CDI Guide](https://www.electronicshub.org/capacitor-discharge-ignition-system/)
- [Dupont EMV Shielding](https://www.dupont.com/electronic-materials/electromagnetic-shielding.html)
- [Analog Devices Ferrite Bead Guide](https://www.analog.com/en/design-notes/selecting-the-right-ferrite-bead-for-emc-compliance.html)
- [PC817 Datasheet](https://www.vishay.com/docs/83608/pc817.pdf)
- [6N137 Datasheet](https://www.vishay.com/docs/83608/6n137.pdf)
- [TI Load Dump Application Note](https://www.ti.com/lit/an/slvaef9/slvaef9.pdf)
- [Infineon Load Dump Protection](https://www.infineon.com/dgdl/Infineon-Load_dump-ApplicationNotes-v01_00-EN.pdf)
- [Littelfuse TVS Diode Guide](https://www.littelfuse.com/technical-resources/technical-center/selection-guides/tvs-diode-selection-guide)
- [STMicro Load Dump Protection](https://www.st.com/resource/en/application_note/dm00180430-load-dump-protection-with-tvs-diodes-stmicroelectronics.pdf)
- [Analog Devices MOSFET Reverse Polarity Protection](https://www.analog.com/en/design-notes/protecting-circuits-using-mosfets-as-reverse-polarity-protection.html)
- [Vishay Capacitors for Load Dump](https://www.vishay.com/docs/29022/aluminumcapacitorsforload_dump.pdf)
- Motorrad-Navigationsgerät (Referenzprodukt)
- [Quad Lock Motorcycle Mount Guide](https://aerostich.com/pages/quad-lock-motorcycle-mount-guide)
- [Motorcycle PCB Assembly Guidelines](https://bigmateph.com/motorcycle-pcb-assembly-guidelines)
- [NiMotion IP Rating Guide](https://www.nimotion.com/blog/detail/ip-rating-motor-protection-guide)

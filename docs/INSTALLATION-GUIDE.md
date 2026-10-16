# Montage-Anleitung & Einbau-Konzept für die Honda Transalp 600 (PD06)

> Bordcomputer Transalp 600 (PD06)
> MCU: ESP32-S3-WROOM-1 (N16R8: 16MB Flash, 8MB Octal PSRAM)
> Stand: 16. Oktober 2026
> Quelle: docs/HARDWARE.md, docs/PINMAP.md, docs/CASE-DESIGN.md,
>   docs/PCB-LAYOUT.md, docs/BOM.md, docs/ARCHITECTURE.md,
>   docs/research/04-pd06-technische-daten.md, docs/research/02-hardware-deep-dive.md

Dieses Dokument ist die detaillierte Einbau- und Montageanleitung für den
Transalp Bordcomputer in die Honda Transalp 600 (PD06). Es richtet sich an
den Bastler und Heimwerker, der das System selbst einbaut. Es geht tiefer als
das Benutzerhandbuch (docs/USER-MANUAL.md) und setzt grundlegende Elektrik-,
Löt- und Mechanikkenntnisse voraus.

---

## Inhaltsverzeichnis

1. Vorbereitung
2. PCB-Zusammenbau
3. Verkabelung am Motorrad
4. Display-Montage
   - 4.5 CarPlay-Display: Montage am Lenker
   - 4.6 Lenker-Controller: Montage und Anschluss
   - 4.7 CarPlay und Controller: Pairing und Konfiguration
5. Inbetriebnahme
6. Fehlersuche

---

## 1. Vorbereitung

### 1.1 Werkzeug-Liste

Die folgende Tabelle listet das benötigte Werkzeug für den kompletten Einbau
auf, unterteilt nach Arbeitsschritt. Werkzeug, das nur für optionale
Schritte benötigt wird, ist als "optional" markiert.

**Allgemein:**

| Werkzeug | Verwendung | Bemerkung |
|---------|-----------|-----------|
| Inbus-Schlüssel 2,5 mm | Displayschrauben M2,5 | Senkkopf |
| Inbus-Schlüssel 4 mm | U-Bolt Muttern M6 | Lenkerhalterung |
| Inbus-Schlüssel 5 mm | PCB-Schrauben M3 | Senkkopf |
| Inbus-Schlüssel 3 mm | Controller-Halterung M5 | SilverFox C2 / Chigee CGRC |
| Maulschlüssel 8 mm | Kabelbaum-Steckverbinder | PD06 |
| Maulschlüssel 10 mm | Batteriepole | YB12A-B |
| Maulschlüssel 14 mm | PG7/PG9 Kabelverschraubungen | Handfest |
| Maulschlüssel 17 mm | M12-Kabelverschraubung (SilverFox C2) | Nur bei kabelgebundenem Controller |
| Kreuzschlitzschraubendreher | Verkleidungsteile | PD06 |
| Seitenschneider | Kabel | — |
| Abisolierzange | Kabelverarbeitung | AWG24 |
| Crimpzange | Wasserdichte Steckverbinder | Für Rastverbinder |
| Lötkolben (40-60 W) | Kabelverbindungen im Gehäuse | Feinspitze |
| Heissluftstation (optional) | SMD-Bestückung | Nur bei Custom-PCB |
| Multimeter (digital) | Spannungs- und Durchgangspruefung | — |
| Drehmomentschlüssel | Ölablassschraube, Sensoren | 10-50 Nm Bereich |
| Schrumpfschlauch-Pistole | Schrumpfschlauch | Heissluft |
| Pinzette | SMD-Bauteile | Antistatisch |
| Smartphone (iPhone/Android) | CarPlay-Pairing, Controller-Pairing | Fuer Inbetriebnahme 4.7 |

**Verbrauchsmaterial:**

| Material | Menge | Verwendung |
|---------|-------|-----------|
| Schrumpfschlauch 3 mm | 2 m | Kabelverbindungen |
| Schrumpfschlauch 6 mm | 1 m | Steckverbinder |
| Kabelbinder 100 mm (UV-beständig, schwarz) | 20 Stück | Kabelfixierung |
| Kabelbinder 200 mm | 10 Stück | Kabelstränge |
| Kabelschutzschlauch (Conduit) | 1 m | Externe Kabelstränge |
| Lötzinn (0,7 mm, bleifrei oder bleihaltig) | 1 Rolle | Lötarbeiten |
| Isopropanol (IPA) | 100 ml | Reinigung |
| Klettband (self-adhesive) | 10 cm | Akku-Fixierung |

### 1.2 Einkaufsliste (zusätzlich zur BOM)

Die vollständige Stückliste (BOM) ist in docs/BOM.md dokumentiert. Die
folgenden Positionen kommen zusätzlich zur BOM und decken die
Installationsmaterialien ab, die im Bordcomputer nicht enthalten sind,
aber für den Einbau am Motorrad benötigt werden.

| Material | Spezifikation | Menge | Verwendung |
|---------|--------------|-------|-----------|
| Geschirmtes Kabel 4-adrig | AWG24, S/UTP, verdrillt | 3 m | Sensoren |
| Kabel 2-adrig | AWG20, ungeschirmt | 1 m | 12V Versorgung |
| Kabel Einzelader | AWG24, diverse Farben | 2 m | Innenverkabelung |
| Wasserdichte Steckverbinder 2-polig | IP67, Rastverriegelung | 1 Paar | 12V Versorgung |
| Wasserdichte Steckverbinder 3-polig | IP67, Rastverriegelung | 3 Paare | ADC-Sensoren |
| Wasserdichte Steckverbinder 2-polig | IP68, Rastverriegelung | 1 Paar | RPM-Signal |
| Wasserdichte Steckverbinder 4-polig | IP67, Rastverriegelung | 1 Paar | Relais-Steuerung |
| Flachsicherung 5A | ATO/ATC | 3 Stück | Überstromschutz |
| Inline-Sicherungshalter | Für Flachsicherung ATO | 1 Stück | 12V-Zuleitung |
| Kabelschuhe | M6 Ringkabelschuh, AWG20 | 4 Stück | Masse, 12V |
| Kabelschuhe | M4 Ringkabelschuh, AWG24 | 6 Stück | Sensoranschlüsse |
| Ferritkern (Klappferrit) | 100 Ohm @ 100 MHz | 4 Stück | EMV-Filterung |
| T-Abzweigverbinder | 22-18 AWG, wasserdicht | 2 Stück | Pickup, Bordspannung |
| Gummitüllen | 6 mm | 4 Stück | Kabeldurchführungen |
| Doppelseitiges Klebeband (3M VHB oder äquivalent) | 50 mm x 50 mm | 1 Stück | Akku-Fixierung |
| Schraubensicherungslack | Mittelstark | 1 Flasche | Vibrationssicherung |

### 1.3 Sicherheits-Hinweise

> WICHTUNG: Diese Anleitung enthält Arbeitsschritte, die an der elektrischen
> Anlage eines Motorrads durchgeführt werden. Fehlerhafte Verkabelung kann
> zu Kurzschluss, Brand, Batteriebeschädigung oder Ausfall der Zündung
> führen. Lies die folgenden Sicherheits-Hinweise vollständig durch, bevor
> du beginnst.

**1. Batterie abklemmen:**
Klemme die Batterie ab, bevor du Kabel anschliesst oder modifizierst.
Die PD06 hat eine 12V Blei-Säure-Batterie (12 Ah). Ein Kurzschluss an der
Batterie kann Funken, Hitze und im schlimmsten Fall ein Feuer verursachen.
Trenne zuerst den Minuspol (grünes Kabel), dann den Pluspol
(schwarz/braunes Kabel). Beim Anklemmen: zuerst Plus, dann Minus.

**2. Keine originalen Kabel durchtrennen:**
Durchtrenne keine Kabel im originalen Kabelbaum der PD06. Verwende
T-Abzweigverbinder für den Signalaabgriff (Pickup, Bordspannung) oder löte
auf und isoliere mit Schrumpfschlauch. Der originale Kabelbaum ist
30+ Jahre alt und Ersatzteile werden knapp.

**3. Sicherung einbauen:**
Jeder neue Stromkreis braucht eine eigene Sicherung, so nah wie möglich
am Abzweigpunkt (innerhalb von 10-15 cm). Der Bordcomputer bekommt eine
5A-Sicherung. Keine bestehende Sicherung des Motorrads verwenden.

**4. Zündung ausschalten:**
Stelle sicher, dass der Zündschalter auf OFF steht und der Kill-Switch
auf RUN ist, bevor du die Batterie abklemmst. Entferne den Schlüssel.

**5. Keine Hitze an der Batterie:**
Löte niemals direkt an den Batteriepolen. Verwende Kabelschuhe und
Schraubverbindungen. Die Batterie kann explosive Gase absondern.

**6. Kühlmittel auffangen:**
Wenn du den Kühlmitteltemperatur-Sensor installierst (Inlinesensor im
oberen Kühlschlauch), muss Kühlmittel abgelassen werden. Kühlmittel ist
giftig und umweltschädlich. Fange es in einem sauberen Behälter auf und
entsorge es ordnungsgemäß. Die PD06 hat ca. 2,0 Liter Kühlmittel.

**7. Motoröl auffangen:**
Wenn du den Oeldrucksensor oder eine Oeltemperatur-Sonde installierst,
muss Motoröl abgelassen werden. Oel ist umweltschädlich. Fange es auf und
entsorge es ordnungsgemäß. Die PD06 hat 2,10 Liter (Ölwechsel) bzw.
2,30 Liter (mit Filter).

**8. EMV-Hinweis:**
Die CDI-Zündung der PD06 erzeugt starke elektromagnetische Störungen.
Arbeete niemals an der Zündanlage bei laufendem Motor. Halte
Sensorkabel von der CDI-Box und den Zündspulen fern. Verwende
geschirmte Kabel und Ferritkerne.

**9. Vibration:**
Alle Schrauben am Motorrad brauchen Federscheiben gegen Vibration.
Der V2-Motor der PD06 erzeugt am Lenker 5-10 g Vibration. Nicht
gesicherte Schrauben lockern sich innerhalb weniger Kilometer.

**10. Wasserdichtigkeit:**
Nach jedem Öffnen des Gehäuses die O-Ring-Dichtung kontrollieren und
ggf. reinigen. Verwende keinen Hochdruckreiniger auf das Gehäuse oder
die Kabeldurchführungen.

### 1.4 Zeitaufwand-Schätzung pro Schritt

| Schritt | Beschreibung | Geschätzter Aufwand |
|--------|-------------|-------------------|
| 1.1 | Werkzeug und Material sammeln | 1 h |
| 1.2 | Batterie abklemmen, Motorrad vorbereiten | 0,5 h |
| 2.1 | PCB bestücken und löten (Prototyp: Perfboard) | 4-8 h |
| 2.2 | PCB bestücken und löten (Custom-PCB, Heissluft) | 2-4 h |
| 2.3 | Kühlkörper und Prüfpunkte | 1 h |
| 2.4 | PCB-Smoke-Test (Spannungen messen) | 0,5 h |
| 2.5 | Gehäuse-Einbau des PCBs | 1 h |
| 3.1 | 12V-Zuleitung und Sicherung verlegen | 1 h |
| 3.2 | Pickup-Signal (RPM) anklemmen | 1,5 h |
| 3.3 | Kühlmitteltemperatur-Sensor installieren | 2 h |
| 3.4 | Tankgeber anklemmen (falls nachgerüstet) | 1,5 h |
| 3.5 | Oeldrucksensor installieren (optional) | 2 h |
| 3.6 | GPS-Antenne montieren | 0,5 h |
| 3.7 | Masseanschluss herstellen | 0,5 h |
| 3.8 | Kabellängen anpassen, Ferritkerne | 1 h |
| 4.1 | Display-Halterung am Lenker | 1 h |
| 4.2 | Display-Kabel verlegen | 0,5 h |
| 4.3 | Wasserschutz und Windschutz | 0,5 h |
| 4.5 | CarPlay-Display am Lenker montieren | 1 h |
| 4.6 | Lenker-Controller montieren und anschliessen | 1 h |
| 4.7 | CarPlay und Controller Pairing + Konfiguration | 1 h |
| 5.1 | Erstes Einschalten und Smoke-Test | 0,5 h |
| 5.2 | Self-Test über Serial Console | 1 h |
| 5.3 | Kalibrierung (RPM, Tank, Temperatur) | 1 h |
| 5.4 | GPS-Fix abwarten | 0,5 h |
| 5.5 | BLE-Pairing mit Smartphone | 0,5 h |
| 5.6 | Testfahrt und Plausibilitaetspruefung | 1 h |
| — | **Gesamtschaetzung (Prototyp)** | **27-33 h** |
| — | **Gesamtschaetzung (Custom-PCB)** | **23-29 h** |
| — | **Gesamtschaetzung (Complete mit CarPlay/Controller)** | **+3 h** |

Die Schätzung geht von einem einzelnen Bastler mit grundlegenden
Fertigkeiten aus. Erfahrene Bastler können die Zeiten um 30-50%
unterschreiten. Ungeübte Lötarbeiten am SMD-Bereich können die
PCB-Zusammenbauzeit verdoppeln.

---

## 2. PCB-Zusammenbau

Dieser Abschnitt behandelt die Bestückung der Leiterplatte (PCB). Es gibt
zwei Szenarien:

- **Prototyp (Phase 1):** ESP32-S3-DevKitC-1 auf Perfboard mit Breakout-
  Modulen. Kein Custom-PCB. Einfacher zu löten, aber grösser und weniger
  kompakt.
- **Production (Phase 2):** Custom-PCB (4-Layer, 80x50 mm) mit direkt
  aufgelötetem ESP32-S3-WROOM-1 Modul und SMD-Bauteilen.

Beide Szenarien werden im Folgenden beschrieben.

### 2.1 Bestückung der Leiterplatte (SMD und Through-Hole)

#### 2.1.1 Loet-Reihenfolge

Die Loet-Reihenfolge ist kritisch: niedrige Bauteile zuerst, hohe Bauteile
zuletzt. Wenn du zuerst grosse Bauteile lötest, blockieren sie den Zugang
zu benachbarten Pads.

**Custom-PCB (SMD):**

1. **Loetpaste auftragen:** Mit der Edelstahl-Schablone (Stainless-Steel-
   Stencil) Loetpaste auf alle SMD-Pads ziehen. Schablone flach auf das
   PCB legen, Paste mit Rakel ziehen. Pastendicke: 0,12-0,15 mm.

2. **Bauteile platzieren (von flach nach hoch):**

   a) 0805-Widerstaende und Kondensatoren (Passiva, 0,5-0,8 mm hoch)
   b) Ferrite-Beads (0805, gleiche Hoehe wie Widerstaende)
   c) SMD-LEDs und Zener-Dioden (SOT-23, 1,0-1,2 mm)
   d) SMD-ICs: HT7333 (SOT-89), TC4427 (SOIC-8), PC817 (DIP-4)
   e) ESP32-S3-WROOM-1 Modul (2,4 mm hoch, groesstes SMD-Bauteil)
   f) MicroSD-Slot (Push-Push, SMD)
   g) FPC-Connector 12-polig (0,5 mm Pitch)
   h) USB-C-Buchse IP67 (Panel-Mount, Through-Hole)
   i) Steckverbinder (AMP Superseal, Through-Hole)
   j) Elkos 100µF (Through-Hole, stehend)
   k) IRLZ44N MOSFET (TO-220, Through-Hole)
   l) 18650-Akkuhalter (Through-Hole, auf L4/Bottom)

3. **Reflow:** Heissluftstation oder Reflow-Ofen. Temperaturprofil:
   - Vorwärmen: 150°C für 60 s (Preheat)
   - Ramp: 150-217°C in 30 s (Reflow-Ramp)
   - Peak: 240-250°C für 20-30 s
   - Abkühlen: < 6°C/s (kontrolliert)

   Alternativ: SMD-Bauteile mit dem Loetkolben einzeln löten (fuer
   Prototyp-Phase ausreichend, aber zeitaufwendig).

4. **Through-Hole-Bauteile:** Nach dem SMD-Reflow von der Bottom-Seite
   löten. Loetkolben 350°C, 1-2 s pro Lötstelle.

**Prototyp (Perfboard):**

1. DevKitC-1 in die Mitte des Perfboards platzieren und mit Stiftleisten
   verlöten.
2. Breakout-Module (MPU6050, BMP280, GPS, SD-Karte) um das DevKitC-1
   herum platzieren.
3. Module mit Jumper-Wire oder verlöteten Einzeladern verbinden.
4. Buck-Converter-Modul (MP1584) an einer Ecke platzieren.
5. LDO (AMS1117-3.3) als Breakout-Modul neben dem Buck platzieren.
6. Optokoppler (PC817) auf einer kleinen Breakout-Platine oder direkt
   auf dem Perfboard verdrahten.

#### 2.1.2 Loet-Temperatur und Werkzeug

| Bauteil-Typ | Loetkolben-Temp | Loetzeit | Spitze |
|-------------|----------------|----------|-------|
| 0805 SMD (R, C) | 330°C | 1-2 s | 0,4 mm Feinspitze |
| SOT-23/SOT-89 | 340°C | 2-3 s | 0,4 mm Feinspitze |
| SOIC-8 (TC4427) | 340°C | 3-5 s pro Pin | 0,4 mm Feinspitze |
| WROOM-1 Modul | 350°C | 2-3 s pro Pad | 0,6 mm Chisel |
| Through-Hole | 350°C | 2-3 s pro Pin | 0,8 mm Chisel |
| USB-C IP67 | 350°C | 3-4 s pro Pin | 0,8 mm Chisel |

Verwende bleifreies Loetzinn (SAC305, 0,7 mm) fuer Custom-PCB und
bleihaltiges Sn63/Pb37 (0,7 mm) fuer Prototyp-Perfboard. Bleihaltiges
Zinn ist leichter zu verarbeiten und verzeiht Fehler besser, ist aber
nicht RoHS-konform.

### 2.2 Kuehlkoerper-Montage fuer Spannungsregler

Der Buck-Converter (MP1584) und der ESP32-S3-WROOM-1 erzeugen Waerme.
Die Waermeableitung erfolgt ueber die GND-Plane des PCBs (4-Layer, L2).

**Buck-Converter (MP1584):**
Das MP1584-Modul hat bereits einen integrierten Kuehlkoerper auf dem IC.
Bei der Modul-Variante (22x15 mm Breakout) ist keine zusaetzliche
Kuehlung noetig. Bei der IC-Variante (SMD auf Custom-PCB) wird ein
Kupfer-Polygon (min. 100 mm²) unter dem IC auf L1 als Waermespreader
angelegt, mit 4-8 Thermal-Vias (0,3 mm Drill) zur GND-Plane (L2).

**ESP32-S3-WROOM-1:**
Das Modul hat einen metallischen Boden-Pad (GND) auf der Unterseite. Auf
dem Custom-PCB wird unter dem Modul eine Kupferflaeche (min. 100 mm²)
freigehalten (GND-Polygon auf L1), die ueber Thermal-Vias mit der
GND-Plane (L2) verbunden ist. Dies leitet die Waerme vom Modul in die
innere Kupferflaeche, die eine groessere thermische Masse hat.

**LDO (HT7333):**
Der HT7333 in SOT-89 verbraucht max. (5V - 3,3V) × 250mA = 425 mW.
Das ist im Bereich der Grenze fuer SOT-89 ohne Kuehlung (ca. 500 mW
bei 25°C Umgebungstemperatur). Bei hoeherer Umgebungstemperatur
(70°C im Gehaeuse) wird es knapp. Abhilfe: breite Kupferflaeche
(min. 50 mm²) auf L1 um den HT7333 als Waermespreader.

### 2.3 Pruef-Punkte: Spannung testen vor Inbetriebnahme

Vor dem ersten Einschalten muessen die Spannungsversorgung und die
Spannungspegel an kritischen Punkten gemessen werden. Das verhindert,
dass ein Loetfehler oder ein Kurzschluss den ESP32-S3 oder die Sensoren
zerstoert.

**Pruef-Punkte auf dem PCB:**

| Pruef-Punkt | Soll-Wert | Messung | Kommentar |
|-------------|-----------|---------|-----------|
| 12V Eingang | 12-15 V | DC | Nach Sicherung, vor Buck |
| 5V Rail (Buck-Output) | 5,0 ± 0,1 V | DC | Am Ausgang des MP1584 |
| 3,3V Rail (LDO-Output) | 3,3 ± 0,05 V | DC | Am Ausgang des HT7333 |
| GND | 0 V (Referenz) | DC | Massebezug |
| ADC GPIO1 (Batt) | 1,2-1,5 V | DC | Spannungsteiler, Zündung an |
| ADC GPIO2 (Coolant) | 0,8-1,6 V | DC | NTC bei Raumtemperatur |
| ADC GPIO4 (Fuel) | 0,2-1,5 V | DC | Tankgeber |
| ADC GPIO5 (Oil) | 0,3-0,5 V | DC | Oeldruck bei 0 bar |
| GPIO15 (RPM) | 3,3 V (HIGH) | DC | Pull-Up, kein Signal |
| GPIO16 (Tacho) | 3,3 V (HIGH) | DC | Pull-Up, kein Signal |
| I2C SDA (GPIO8) | 3,3 V (HIGH) | DC | Pull-Up 4,7k |
| I2C SCL (GPIO9) | 3,3 V (HIGH) | DC | Pull-Up 4,7k |

**Pruef-Reihenfolge:**

1. **Durchgangspruefung (Ohm) vor Power-On:**
   - Kein Kurzschluss zwischen 12V und GND (Multimeter: Piepton, Ohm > 10k)
   - Kein Kurzschluss zwischen 5V und GND (Multimeter: Ohm > 1k)
   - Kein Kurzschluss zwischen 3,3V und GND (Multimeter: Ohm > 100 Ohm)
   - Kein Kurzschluss zwischen 5V und 3,3V (Multimeter: Ohm > 100 Ohm)
   - Kein Kurzschluss zwischen 12V und 5V (Multimeter: Ohm > 10k)

2. **Spannungsmessung (DC) nach Power-On:**
   - 12V an das PCB anlegen (geschaltetes +12V oder Labornetzgeraet)
   - Zuerst 12V-Eingang messen (sollte 12-15V sein)
   - Dann 5V-Rail messen (sollte 5,0 ± 0,1V sein)
   - Dann 3,3V-Rail messen (sollte 3,3 ± 0,05V sein)
   - Wenn alle Spannungen korrekt: Smoke-Test bestanden

3. **Stromaufnahme (mA) kontrollieren:**
   - Multimeter in Reihe zur 12V-Zuleitung (Strommessbereich)
   - Erwartet: 50-150 mA ohne Display-Backlight, ohne GPS-Fix
   - Mit Display-Backlight: 200-300 mA
   - Mit GPS-Fix und BLE: 300-500 mA
   - Wenn > 500 mA: Kurzschluss oder defektes Bauteil. Sofort abschalten.

### 2.4 Gehaeuse-Einbau des PCBs

Nach erfolgreichem Smoke-Test wird das PCB in das Gehaeuse eingebaut.
Das Gehaeuse ist in docs/CASE-DESIGN.md detailliert beschrieben.

**Einbau-Schritte:**

1. **Unterteil vorbereiten:** PCB-Fuehrungsschienen im Gehaeuse-
   Unterteil pruefen. Falls Gussgrate vom 3D-Druck vorhanden, mit
   einem Messer entfernen.

2. **Kabelverschraubungen (PG7/PG9) montieren:** Kabelverschraubungen
   von aussen in die Oeffnungen an der Unterseite schrauben. O-Ring
   zwischen Gland und Gehaeusewand einsetzen, Kontermutter innen
   festziehen. Handfest, nicht ueberdrehen.

3. **USB-C-Buchse (IP67) montieren:** An der Unterseite, neben den
   Kabelverschraubungen. Mit beiliegender Mutter und Dichtung
   festziehen.

4. **ePTFE-Druckausgleichsmembran montieren:** M6 x 0,75 Gewinde in
   die entsprechende Oeffnung an der Unterseite schrauben. Vertikal
   orientiert (Membran zeigt nach unten, Wasser kann abtropfen).

5. **PCB einschieben:** PCB in die Fuehrungsschienen des Unterteils
   schieben. Auf richtige Orientierung achten (Steckverbinder zeigen
   zur Gehaeuseunterseite, Antenne zum freien Bereich).

6. **PCB verschrauben:** 4 × M3 x 8 mm Senkkopfschrauben in die
   Schraubsaeulen. Gummipuffer zwischen PCB und Schraubsaeule als
   Vibrations-Entkopplung. Federscheibe unter jede Schraube.

7. **Kabel durch PG7/PG9 fuehren:** Sensor-Kabel durch die
   Kabelverschraubungen ins Gehaeuseinnere ziehen. Schirm an einem
   Ende (Gehaeuseseite) auf Masse legen. Kabel an die
   Steckverbinder auf dem PCB anklemmen.

8. **Kupferfolie aufkleben:** Auf die Innenseite des Gehaeuses
   (Rueckseite). Aussparung fuer BLE-Antenne (min. 30 × 30 mm freie
   Flaeche um das WROOM-1-Modulende) lassen. Aussparung fuer GPS-
   Antenne, falls die Antenne im Gehaeuse montiert ist.

9. **Display-Rahmen einsetzen:** E-Paper-Modul in den PETG-Display-
   Rahmen einsetzen, auf 0,5 mm Silikon-Unterlage. Mit 4 × M2,5 x 6 mm
   Schrauben und Federscheiben fixieren.

10. **Deckel aufsetzen:** Silikon-O-Ring in die Nut im Deckel einlegen.
    Deckel auf das Unterteil setzen (Shingling-Prinzip: Deckel greift
    ueber das Unterteil). Mit 4 × M3 x 12 mm Schrauben und Federscheiben
    verschrauben.

**ASCII-Diagramm: PCB-Einbau im Gehaeuse (Seitenansicht)**

```
     Fahrtrichtung ===>

  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  Acrylglas-Abdeckung (2mm)
  :                              :
  :  +---+               +---+   :   Display-Rahmen (PETG)
  :  | E | E-Paper 2.9"  |   |   :   auf Silikon-Pad
  :  | P | (296x128)     |   |   :
  :  +---+               +---+   :
  :  ::::::::::::::::::::::::::  :
  :  : PCB (ESP32-S3 + Sensoren) :   PCB auf Fuehrungsschienen
  :  ::::::::::::::::::::::::::  :
  :                              :
  :  [Akku]         [DC/DC]     :   LiPo-Akku + Buck-Converter
  :                              :
  :  oooo  oooo  oooo  oooo      :   Kabelverschraubungen (PG)
  :                              :
  +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
  |                              |   Gehaeuse-Unterteil (ASA, 3mm)
  |        Schraubsaeule         |     Schraubsaeulen M3
  |          |||                 |
  +----------+||+----------------+
             |||
      [Gummidmpfer]                   Vibrations-Entkopplung
             |||
  +----------+||+----------------+
  |          |||       Halterung |     Halterungs-Adapter (ASA)
  |     U-Bolt 22mm  (Lenker)    |
  +-----+============+-----+          22mm Lenker / Crossbar
        |  Lenker  |
        +----------+

  <---- 100 mm Breite ---->
  <-- 45 mm Tiefe (Hoehe) -->
```

---

## 3. Verkabelung am Motorrad

Dieser Abschnitt beschreibt die Verkabelung des Bordcomputers an der
Honda Transalp 600 (PD06). Die PD06 ist ein rein mechanisches Motorrad
ohne ECU, ohne CAN-Bus und ohne elektronische Diagnoseschnittstelle.
Alle Sensoren werden eigenstaendig nachgeruestet.

Die Kabelverlegung folgt einem festen Pfad vom Motorrad zum Bordcomputer
am Lenker. Die PD06 hat folgende relevante Anschluesse:

**ASCII-Diagramm: Kabelverlegung am PD06 (Seitenansicht)**

```
       Lenker / Crossbar (22 mm)
       +-----+============+-----+
       |     |  Bordcomputer |   |     <-- Gehaeuse am Lenker
       +-----+============+-----+
              |||
        [Gummidmpfer]
              |
       +------+----+
       |  Kabel-   |                    <-- Kabelstrang am rechten
       |  strang   |                        Rahmenrohr nach unten
       +------+----+
              |
    +---------+---------+
    |                   |
    |   Sitzbank        |               <-- CDI-Module unter Sitzbank
    |   (CDI-Module)    |                   (Abstand halten!)
    +---------+---------+
              |
    +---------+---------+
    |                   |
    |   Tank (18 L)    |               <-- Tankgeber (falls nachgerstet)
    |                   |
    +---------+---------+
              |
    +---------+---------+
    |                   |
    |   Batterie        |               <-- 12V Abgriff (geschaltet)
    |   (YB12A-B)       |                   Schwarz = +12V geschaltet
    +---------+---------+                   Gruen = Masse
              |
    +---------+---------+
    |                   |
    |   Kühler          |               <-- Kuehlmitteltemp-Sensor
    |                   |                   im oberen Kuehlerschlauch
    +---------+---------+
              |
    +---------+---------+
    |                   |
    |   Motorblock      |               <-- Pickup (RPM)
    |   (V2, 52 Grad)   |                   Oeldruckschalter (M10x1,0)
    |                   |                   Oelablassschraube (M14x1,5)
    +---------+---------+
```

### 3.1 Kabel-Verlegung: Lenkerbereich, Rahmen, Tank

Die Kabel vom Bordcomputer (Lenker) zu den Sensoren (Motor, Kuehler,
Batterie, Tank) verlaufen an der rechten Seite des Motorrads. Die rechte
Seite ist zu bevorzugen, weil die linke Seite naeher an der Auspuffanlage
liegt (Hitze).

**Kabelweg:**

1. **Motorblock (Pickup, Oeldruck, Oeltemperatur):** Kabel an der
   rechten Seite des Kurbelgehaeuses zusammenfuehren.
2. **Rahmenrohr (rechte Seite):** Kabelstrang entlang dem rechten
   Rahmenrohr nach oben fuehren. Alle 15-20 cm mit Kabelbindern am
   Rahmen fixieren.
3. **Unter Sitzbank:** Kabel unter der Sitzbank entlangfuehren. Hier
   sitzen auch die CDI-Module -- Abstand halten (min. 10 cm), um
   elektromagnetische Einstreuung zu vermeiden.
4. **Lenkerhalter:** Kabelstrang an den Lenkerhaltern nach oben fuehren.
5. **Lenker/Crossbar:** Kabel an der Crossbar zum Bordcomputer fuehren.

**Wichtige Regeln:**

- Keine Kabel entlang der Auspuffanlage (Hitze kann die Isolierung
  beschaedigen, ab 80°C dauerhaft).
- Keine Kabel in der Naehe des CDI-Moduls (elektromagnetische
  Einstreuung, besonders auf analoge ADC-Leitungen).
- Geschirmte Kabel (S/UTP, verdrillt) fuer alle Sensorkabel verwenden.
- Schirm an EINEM Ende auf Masse legen (Gehaeuseseite / PCB-GND am
  Steckverbinder), um Masseschleifen zu vermeiden.
- Kabel nicht frei haengend -- immer fixiert (Vibration kann lose
  Kabel beschädigen).
- Kabelschutzschlauch (Conduit) fuer externe Kabelstraenge verwenden.
- Ferritkerne (Klappferrite) an jedem Kabel, das ins Gehaeuse muendet,
  moeglichst nah am Gehaeuse platzieren.

### 3.2 Anschluesse an das PD06-System

#### 3.2.1 Zuendspule / Pickup (RPM-Signal)

Die Drehzahlmessung erfolgt ueber das Pickup-Signal der CDI-Zündung.
Der Pulsgenerator liefert 1 Puls pro Kurbelwellenumdrehung (f = n/60).

**Signalquelle:**
- Bei Modellen bis 1995 (V-H bis V-R): Zwei separate Pickup-Spulen
  (eine pro Zylinder), Kabel gelb oder blau.
- Bei Modellen ab 1996 (V-T bis V-X): Eine einzelne Pickup-Spule.
- Das Signal kann am CDI-Kabel (Pin 5 der CDI-Box, gelb/weiss) oder
  an der Minusseite der Zuendspule (Coil-) abgegriffen werden.

**Kabelbaum-Farbcodes (Honda Standard) fuer den Pickup:**
- Gelb/Weiss (Yellow/White): Zuendspule / Pickup-Signal

**WICHTIG:** Das Pickup-Signal darf NIEMALS direkt an den ESP32
angeschlossen werden. Die CDI erzeugt Hochspannungspulse (200-400V am
Kondensator, 20-40 kV an der Zuendspule), die den Microcontroller
zerstoeren wuerden. Verwende zwingend einen Optokoppler (PC817 fuer
Standard-Signale, 6N137 fuer hohe Frequenzen) zur galvanischen Trennung.

**Pegel-Anpassung:**

```
  CDI Pickup (gelb/weiss)
      |
      +--[Vorwiderstand 10k]--[Zener 5,1V]--+
                                            |
                                     [PC817 LED-Seite]
                                            |
                                     [PC817 Transistor-Seite]
                                     Collector --- 3,3V
                                     Emitter --+--- GPIO15 (PCNT0)
                                                |
                                           [10k Pull-Up]
                                                |
                                              GND
```

Funktionsweise:
1. Die CDI-Zuendung erzeugt einen Hochspannungsimpuls pro
   Zuendvorgang (2 Zylinder, 1 Impuls pro 360° Kurbelwellenumlauf).
2. Der Vorwiderstand und die Zener-Diode begrenzen die Spannung.
3. Der Optokoppler (PC817) trennt das Motorrad-Netz galvanisch.
4. Ein Schmitt-Trigger (74HC14) entprellt und erzeugt saubere
   Rechteckimpulse.
5. Der PCNT zaehlt die Impulse: RPM = Pulse × 60 (bei 1 Puls/Umdrehung).

**Verkabelung:**

1. Batterie abklemmen (Minuspol zuerst).
2. Pickup-Kabel (gelb/weiss) in der Naehe der CDI-Box identifizieren.
   Die CDI-Einheiten sitzen unter der Sitzbank in einer Gummihalterung.
3. T-Abzweigverbinder auf das gelb/weisse Kabel crimpenn. Alternativ:
   Kabel isolieren, anloeten, mit Schrumpfschlauch ueberziehen.
4. Abzweigung mit geschirmtem Kabel (AWG24, S/UTP) verbinden.
5. Kabel durch Kabelschutzschlauch fuehren und am rechten Rahmenrohr
   entlang zum Bordcomputer verlegen.
6. Kabel durch PG7-Kabelverschraubung in das Gehaeuse fuehren.
7. An den Optokoppler-Eingang des PCB anschliessen (wasserdichter
   2-poliger Steckverbinder, IP68).
8. Ferritkern (Klappferrit) am Kabel, nah am Gehaeuse, umklammern.

**Frequenzbereich:**
- Leerlauf (1.300 U/min): 21,7 Hz
- Nenndrehzahl (8.000 U/min): 133,3 Hz
- Maximalbereich (9.000 U/min): 150 Hz
- Der PC817 ist bis ca. 30 kHz spezifiziert -- ausreichend.
- Fuer hohe Drehzahlen oder feinere Aufloesung: 6N137 (10 MHz).

#### 3.2.2 Kuehlmitteltemperatur: Sensor-Adapter im Kuehlerschlauch

Die PD06 hat serienmaessig keinen elektronischen
Kuehlmitteltemperatursensor. Fuer die Temperaturanzeige muss ein
Inlinesensor in den oberen Kuehlerschlauch (vom Motor zum Kuehler)
eingesetzt werden.

**Sensor-Spezifikation:**
- NTC-Thermistor 10k bei 25°C, B3950
- Oder: DS18B20 wasserdicht (digital, 1-Wire, ±0,5°C)
- Innendurchmesser des Kuehlerschlauchs: 32-36 mm (messen!)

**Kennlinie des NTC (10k, B3950):**

| Temperatur | NTC-Widerstand | ADC-Spannung (10k Ref) |
|-----------|---------------|----------------------|
| 0°C | ~33k | 2,55 V |
| 25°C | 10k | 1,65 V |
| 50°C | ~3,6k | 0,88 V |
| 80°C | ~1,2k | 0,36 V |
| 100°C | ~680 Ohm | 0,21 V |
| 120°C | ~420 Ohm | 0,13 V |

Auswertung: Steinhart-Hart-Gleichung:
1/T = 1/T0 + (1/B) * ln(R/R0)
mit T0 = 298,15 K (25°C), R0 = 10k, B = 3950.

**Montage:**

1. Kuehlmittel ablassen (ca. 2,0 Liter). Auffangen und entsorgen.
2. Oberen Kuehlerschlauch am Motor loesen (Schlauchschelle loesen).
3. Innendurchmesser des Schlauchs messen (meist 32-36 mm).
4. Inlinesensor-Adapter (O-Ring-Schelle) passend zum Schlauch-
   Innendurchmesser in den Schlauch einsetzen.
5. Schlauch wieder montieren, Klemmschellen festziehen.
6. Kuehlmittel auffuellen und entlueften (Luft im System vermeiden).
7. Sensor-Kabel zum Bordcomputer verlegen, durch PG7 fuehren.
8. An den ADC-Eingang des PCB anschliessen (wasserdichter 3-poliger
   Steckverbinder: Signal, Masse, Shield).

**Wanngrenzen:**
- 110°C: Warnung (gelb)
- 120°C: Kritisch (rot, akustische Warnung)

Thermostat der PD06: oeffnet bei 80-84°C, voll offen bei 95°C.
Kuehlerluefter schaltet bei 98-102°C ein, bei 93-97°C wieder aus.

#### 3.2.3 Tank-Sensor: Anschluss an den Tankgeber

Die Transalp 600 hat serienmaessig keinen elektronischen Tankgeber.
Die Kraftstoffanzeige funktioniert ueber eine Vakuum-Petcock-Membran
mit Reserve-Stellung und eine Warnleuchte.

Fuer eine elektronische Tankstandsanzeige muss ein Aftermarket-Sender
nachgeruestet werden.

**Optionen:**

a) **Schwimmer-Sensor im Tank:** Aftermarket-Sender (0-190 Ohm oder
   240-33 Ohm) durch die Tankoeffnung oder eine separate Bohrung im
   Tankdeckel installieren.

b) **Externer Durchfluss-Sensor:** Im Kraftstoffschlauch montieren.
   Misst den Kraftstoffdurchfluss, nicht den Stand.

**Schaltung (Spannungsteiler mit variablen Widerstand):**

```
  3,3V --[100 Ohm Ref]--+--- GPIO4 (ADC1_CH3)
                         |
                    [Tankgeber 5-90 Ohm]
                         |
  GND -------------------+
```

Bei leerem Tank (90 Ohm): V = 3,3 * 90/190 = 1,56 V
Bei vollem Tank (5 Ohm): V = 3,3 * 5/105 = 0,16 V

Kalibrierung: Lookup-Table im Flash, da Tankgeometrie nicht linear.
Drei-Punkt-Kalibrierung: voll / halbvoll / leer.

**Montage (Schwimmer-Sensor):**

1. Tank leeren (oder auf Reserve stellen und restlichen Kraftstoff
   auffangen).
2. Tankdeckel oder Tankoeffnung zugaenglich machen.
3. Schwimmer-Sensor durch die Tankoeffnung oder eine separate Bohrung
   im Tankdeckel installieren.
4. Dichtung am Tankdeckel pruefen (Kraftstoffdicht).
5. Sensor-Kabel zum Bordcomputer verlegen, durch PG7 fuehren.
6. An den ADC-Eingang des PCB anschliessen (wasserdichter 3-poliger
   Steckverbinder).

**Tankdaten PD06:**
- Tankvolumen: 18,0 Liter
- Reserve: 2,9-3,5 Liter
- Verbrauch: ca. 4,5-5,5 L/100 km
- Reichweite (praktisch): 300-350 km

#### 3.2.4 Bordspannung: Abgriff an der Batterie (mit Sicherung)

Die Bordspannung wird ueber einen Spannungsteiler am ADC gemessen,
um den Ladezustand der Motorrad-Batterie zu ueberwachen.

**Anschlusspunkt:**
- Schwarz (geschaltetes +12V ueber Zuendschalter): Nur +12V, wenn
  Zuendung AN.
- Schwarz/Braun (Batterie-Plus, direkt): Immer +12V.
- Gruen: Masse (Rahmen).

Empfohlen: Geschaltetes +12V (schwarzes Kabel), damit der Bordcomputer
nur bei eingeschalteter Zuendung Strom zieht und die Batterie nicht
entlaedt, wenn das Motorrad abgestellt ist.

**Schaltung (Spannungsteiler 10:1):**

```
  12V Bordnetz --[100k 1%]--+--- GPIO1 (ADC1_CH0)
                            |
                        [10k 1%]
                            |
  GND ----------------------+
```

Bei 15V Bordnetz: 1,5 V am ADC.
Bei 12V Bordnetz: 1,2 V am ADC.
1%-Widerstaende fuer genaue Messung (±0,1V bei 15V).
Zusaetzlich 100nF Kondensator am ADC-Pin gegen Rauschen.
Auswertung: vbat = adc_voltage × 10.0 (in Volt).
Generator-Status: vbat > 13,8V = Generator laeuft.

**Verkabelung:**

1. Batterie abklemmen (Minuspol zuerst).
2. Geschaltetes +12V vom schwarzen Kabel abzapfen (T-Abzweigverbinder).
3. Masse vom gruenen Kabel abzapfen.
4. Inline-Sicherungshalter mit 5A-Flachsicherung in die +12V-Leitung
   einbauen, so nah wie moeglich am Abzweigpunkt (10-15 cm).
5. Kabel (AWG20, ungeschirmt, 2-adrig) durch Kabelschutzschlauch zum
   Bordcomputer verlegen.
6. Kabel durch PG9-Kabelverschraubung in das Gehaeuse fuehren.
7. An den 12V-Eingang des PCB anschliessen (wasserdichter 2-poliger
   Steckverbinder, IP67).

**Batteriedaten PD06:**
- Typ: 12V Blei-Saeure, YB12A-B
- Kapazitaet: 12 Ah
- Voll geladen: 13,0-13,2 V
- Entladen: unter 12,3 V
- Ladespannung: 13,5-15,5 V bei 5.000 U/min

**Sicherung:**
- Bordcomputer-Sicherung: 5A (eigene Sicherung, nicht in bestehende
  Sicherung eingreifen).
- Transalp 600 Sicherungen: 30A Haupt, 3-4 x 10A, 1-2 x 15A.
- Die 5A-Sicherung reicht fuer den Bordcomputer (max. 30W = 2,5A bei
  12V) mit Reserve fuer Inrush-Stroeme.

#### 3.2.5 GPS-Antenne: Montage unter der Sitzbank oder im Lenkerbereich

Die GPS-Antenne (Active Patch, 25x25 mm) braucht freie Sicht nach oben
zum Himmel. Sie wird durch das Kunststoffgehaeuse hindurch
funktionsfaehig sein, aber Metall verdeckt das Signal.

**Montage-Optionen:**

a) **Auf dem Gehaeusedeckel (empfohlen):** Die GPS-Antenne wird
   oberhalb des Lenkers auf dem Gehaeusedeckel montiert. Die
   Antenne hat direkte Sicht nach oben. Kabel fuehrt durch das
   Gehaeuse zum GPS-Modul auf dem PCB.

b) **Unter der Sitzbank:** Die GPS-Antenne wird unter der Sitzbank
   montiert. Die Sitzbank ist meist aus Schaumstoff und Kunststoff
   -- ausreichend durchlaessig fuer GPS-Signale. Vorteil: kurze
   Kabellaeufe. Nachteil: die Antenne ist weiter vom Lenker entfernt,
   das GPS-Modul muesste auf einem separaten Daughterboard untergebracht
   werden.

c) **Im Lenkerbereich:** Die GPS-Antenne wird am Lenkerhalter oder an
   der Crossbar montiert, in der Naehe des Bordcomputers. Aelternative,
   wenn die Antenne nicht auf dem Gehaeusedeckel platziert werden kann.

**Wichtige Regeln:**
- Mindestabstand 10 cm zwischen GPS-Antenne und GPS-Modul (bei aktiven
  Antennen, wegen der internen Verstaerkung).
- Moeglichst grosser Abstand zur Zuendspule und zum CDI-Modul (EMV-
  Stoerung der Satellitensignale).
- Die Antenne darf nicht durch Metall verdeckt sein (Tank, Motorblock,
  Verkleidung aus Metall).
- Bei Montage im Gehaeuse: Aussparung in der Kupferfolie fuer die
  GPS-Antenne (min. 30x30 mm freie Flaeche).

**Verkabelung:**
- GPS-Antenne -> GPS-Modul auf PCB: Koaxialkabel (vorhanden an der
  aktiven Antenne, meist 1,5-3 m).
- GPS-Modul -> ESP32-S3: UART (TX=GPIO43, RX=GPIO44, PPS=GPIO39).
- GPS-Modul bekommt eigene Stromzufuhr 3,3V ueber Ferrite-Bead
  (600 Ohm @ 100 MHz), um Stoerungen zu isolieren.

**GPS-Fix:**
- Kaltstart (erstes Einschalten oder nach langer Standzeit): bis zu
  2 Minuten fuer den ersten Fix.
- Warmstart: 10-30 Sekunden.
- Das GPS-Modul braucht mindestens 4 Satelliten fuer einen 3D-Fix.
- Freie Sicht nach oben ist die wichtigste Voraussetzung.

#### 3.2.6 Oeldrucksensor (optional)

Der Oeldruckschalter der PD06 ist am Motorblock, rechte Seite, unterhalb
des Oelfilters. Gewinde: M10 x 1,0 (metrisches Feingewinde). Der
Serien-Schalter ist ein Warnschalter, der bei ca. 0,2-0,4 bar oeffnet.

Fuer eine analoge Oeldruck-Anzeige muss der Serien-Schalter durch ein
T-Stueck-Adapter ersetzt werden, um sowohl den Warnschalter als auch
einen Drucksensor zu behalten.

**Adapter:**
- M10 x 1,0 (Motorblock) -> 1/8 NPT (Sensor)
- T-Stueck: M10 x 1,0 Eingang, 1/8 NPT Seitengewinde,
  M10 x 1,0 Ausgang (fuer den originalen Warnschalter).

**Sensor:**
- Analog 0-5 bar, 10-180 Ohm (branchenueblich) oder 0,5-4,5V Ausgang.
- Anschluss: 1/8 NPT Gewinde, mit Dichtung.

**Schaltung (fuer 0,5-4,5V Sensor):**

```
  Sensor-Ausgang (0,5-4,5V) --[10k]--+--- GPIO5 (ADC1_CH4)
                                      |
                                  [10k]
                                      |
  GND --------------------------------+
```

Spannungsteiler 1:2: 4,5V -> 2,25V am ADC (im 12dB-Bereich).
Auswertung: oil_bar = (adc_voltage * 2 - 0,5) / 4,0 * 10,0

**Oeldruck-Spezifikationen PD06:**
- Oeldruck (warm, 80°C): 4,5 bar @ 6.000 U/min
- Oeldruckwarnleuchte Schaltpunkt: < 0,2 bar
- Oeldruck (warm, Leerlauf): ca. 0,5-1,0 bar
- Oeldruck (warm, Fahrt): 3-5 bar
- Oeldruck (Kaltstart): bis 6 bar

**Warn-Schwellen:**
- < 0,5 bar bei laufendem Motor: Kritisch (Oelmangel)
- > 6 bar: Warnung (Oeldruck-Regler defekt)

**Montage:**

1. Motoroel ablassen (warm, 2,10 Liter). Auffangen und entsorgen.
2. Originalen Oeldruckschalter herausschrauben (Maulschluessel 14 mm,
   gegen den Uhrzeigersinn).
3. T-Stueck-Adapter einschrauben (mit Dichtung, Anzugsdrehmoment 12 Nm).
4. Oeldrucksensor in das Seitengewinde des T-Stuecks schrauben
   (mit Dichtung, Teflonband nicht verwenden, da es die Masse-
   verbindung stoeren kann; stattdessen Dichtmittel fuer Gewinde).
5. Originalen Warnschalter in das Ausgangsgewinde des T-Stuecks
   schrauben (mit neuer Dichtung, 12 Nm).
6. Motoroel auffuellen (2,10 Liter bei Oelwechsel).
7. Sensor-Kabel zum Bordcomputer verlegen, durch PG7 fuehren.
8. An den ADC-Eingang des PCB anschliessen (wasserdichter 3-poliger
   Steckverbinder: Signal, Masse, Shield).

### 3.3 Stecker-System: Wasserdichte Stecker

Alle externen Anschluesse muessen vibrations- und wasserfest sein
(min. IP67). Die folgenden Steckverbinder werden empfohlen:

| Anschluss | Stecker-Typ | IP-Rating | Kontakte | Strom |
|----------|------------|----------|----------|-------|
| 12V Bordnetz | Rastverbinder 2-polig | IP67 | 2 | 5A |
| Sensoren (ADC) | Rastverbinder 3-polig | IP67 | 3 | 3A |
| RPM (Optokoppler) | Rastverbinder 2-polig | IP68 | 2 | 3A |
| Relais-Steuerung | Rastverbinder 4-polig | IP67 | 4 | 3A |
| USB-C Service | IP67 USB-C Panel-Mount | IP67 | - | 3A |
| Display (FPC) | FFC/FPC intern | - | 12 | - |
| GPS (UART) | Rastverbinder 4-polig | IP65 | 4 | 3A |

Die internen Verbindungen (Display-FPC, GPS-Kabel) koennen mit
JST-SM oder FFC gemacht werden, da sie innerhalb des wasserdichten
Gehaeuses liegen. Alle externen Anschluesse muessen IP67 haben.

**Vermeiden:** Steckverbinder ohne Verriegelung (z.B. JST-XH). Sie
loesen sich bei Vibration. Der V2-Motor der PD06 erzeugt am Lenker
5-10 g Vibration -- nicht gesicherte Stecker lockern sich innerhalb
weniger Kilometer.

**Crimpen:**
- Crimpzange fuer die jeweiligen Steckverbinder verwenden.
- Crimpung auf Festigkeit pruefen (Zugtest: Kabel darf sich nicht aus
  der Crimphuelse ziehen lassen).
- Schrumpfschlauch mit Kleber (adhesive-lined) ueber jede Crimpver-
  bindung, als mechanischer und Feuchtigkeitsschutz.

### 3.4 Kabelbinder und Schrumpfschlauch-Verarbeitung

Alle externen Kabel muessen gesichert und geschuetzt werden:

**Kabelbinder (UV-beständig, schwarz):**
- Alle 10-15 cm am Rahmen fixieren.
- Ueberlappend an Kabelbuendeln (zwei Kabelbinder pro Fixpunkt, um
  180° versetzt, fuer mehr Flaechendruck).
- Kabelbinder-Enden abschneiden (keine scharfen Kanten).

**Schrumpfschlauch mit Kleber (adhesive-lined):**
- Ueber alle Kabelverbindungen (Crimpverbindungen, Loetverbindungen).
- Schrumpftemperatur: 90-120°C (Heissluftpistole).
- Schrumpfrate 2:1 (3 mm Durchmesser schrumpft auf 1,5 mm).
- Der Kleber fuellt Hohlräume und dichtet gegen Feuchtigkeit ab.

**Kabelschutzschlauch (Conduit):**
- Fuer externe Kabelstränge, die dem Wetter ausgesetzt sind.
- Schlauch aus Polyamid (PA6), geteilt (nachtraeglich aufklappbar).
- Schutz vor mechanischer Beschädigung und UV-Strahlung.

**Ferritkerne (Klappferrite):**
- An jedem Kabel, das ins Gehaeuse muendet.
- Moeglichst nah am Gehaeuse (innerhalb von 5-10 cm).
- Klappferrit um das Kabel schliessen, mit Kabelbinder sichern.
- Wirkung: Drossel fuer Hochfrequenz, reduziert eingekoppelte
  Stoerungen von der CDI-Zuendung.

### 3.5 Kabellaengen-Schaetzung fuer PD06

Die Kabellängen werden geschaetzt anhand der Abmessungen der PD06
(Radstand 1.505 mm, Gesamtlänge 2.265-2.280 mm). Die Werte sind
Richtwerte; die tatsächliche Länge muss am jeweiligen Motorrad
nachgemessen werden. 20-30% Reserve einplanen.

| Signalquelle | Ziel | Kabellänge (cm) | Kabeltyp |
|-------------|------|----------------|----------|
| Pickup am Kurbelgehaeuse | Bordcomputer (Lenker) | 100-120 | AWG24, geschirmt |
| Kuehlmitteltemp am Kuehler | Bordcomputer (Lenker) | 80-100 | AWG24, geschirmt |
| Kraftstoffgeber am Tank | Bordcomputer (Lenker) | 60-80 | AWG24, geschirmt |
| Batterie (12V) | Bordcomputer (Lenker) | 80-100 | AWG20, ungeschirmt |
| Oeldrucksensor am Motor | Bordcomputer (Lenker) | 100-120 | AWG24, geschirmt |
| Oeltemperatur (Oelablass) | Bordcomputer (Lenker) | 100-120 | AWG24, geschirmt |
| Tacho (Hall-Sensor am Vorderrad) | Bordcomputer (Lenker) | 40-60 | AWG24, geschirmt |
| GPS-Antenne | GPS-Modul auf PCB | 15-30 | Koaxial |

Die maximale Leitungslaenge fuer ADC-Sensoren betraegt 50 cm (mit
RC-Filter und Abtastung). Fuer laengere Strecken geschirmte Kabel
verwenden und Ferritkerne am Kabelende nah am Gehaeuse platzieren.
Bei Kabelaengen ueber 50 cm kann die ADC-Genauigkeit durch
Spannungsabfaelle und EMV-Einstreuungen abnehmen. Der RC-Filter
(1 kOhm + 100 nF = 1,6 kHz Grenzfrequenz) am ADC-Eingang filtert
hochfrequente Stoerungen (Zuendfunken > 100 kHz) heraus.

### 3.6 Masseanschluss

Der Masseanschluss (Rahmen/Masse) ist kritisch fuer die Funktion aller
Sensoren und des Bordcomputers.

**Anschlusspunkt:**
- Das gruene Kabel im Honda-Kabelbaum ist Masse (Rahmen).
- Der Bordcomputer hat einen eigenen Masseanschluss an der
  Gehaeuserueckseite (Klemme mit Sternpunkt-Verbindung zum PCB).

**Sternpunkt-Masse:**
Alle Masseleitungen laufen an einem einzigen Punkt zusammen (Sternpunkt
/ Single-Point Ground), um Masseschleifen zu vermeiden. Wenn Strom durch
verschiedene Massepfade fliesst, entstehen Spannungsabfaelle, die
Sensoren verfaelschen. Der Sternpunkt liegt am Gehaeuse / Akku-Minus /
DC/DC-Masse.

**Verbindung:**

1. Massekabel (gruen) vom Motorrad-Kabelbaum an den Masseanschluss
   des Bordcomputers anschliessen.
2. Kabelquerschnitt: mindestens 0,75 mm² (besser 1,5 mm²).
3. Verbindung mit Ringkabelschuh und Schraube sichern (mit Federscheibe
   gegen Vibration).
4. Massepunkt mit Schrumpfschlauch isolieren (Kurzschlussschutz).

**ASCII-Diagramm: Sternpunkt-Masse**

```
                    Bordcomputer-Gehaeuse
                    |
                    |  (Masseklemme)
                    v
  +----------------------------------+
  |         STERNPUNKT               |
  |    (Single-Point Ground)         |
  +--+------+------+------+----+-----+
     |      |      |      |    |
     |      |      |      |    +-- GPS-Modul GND
     |      |      |      +------- SD-Karte GND
     |      |      +-------------- Display GND
     |      +--------------------- PCB-GND (Sensoren)
     |
     +-- 12V-Buck-Converter GND
     |
     +-- Motorrad-Masse (gruen)
     |
     +-- Akku-Minus (18650)
```

---

## 4. Display-Montage

Der Bordcomputer hat zwei Displays: das primäre TFT-Display (3.5"
ILI9488, 480x320 Pixel, SPI) und das sekundäre E-Paper-Display
(2.13", 250x122 Pixel, SPI). Beide werden am shared SPI2-Bus (FSPI)
betrieben.

### 4.1 TFT-Display-Halterung am Lenker (Klemmung an Lenkerdurchmesser PD06)

Die Honda Transalp 600 (PD06) hat einen Standard-Lenker mit 22 mm
Durchmesser (7/8 Zoll). Das ist das Standardmass fuer Strassen-
motorraeder.

**Montage-Optionen:**

**Option A: Direkte Lenkerhalterung (einfachste Variante)**
- U-Bolt-Schelle aus Metall um den 22-mm-Lenker
- Halterungs-Adapter aus ASA gedruckt
- Vorteil: einfach, schnell abnehmbar
- Nachteil: lenkerdrehend, weniger stabil fuer fest verbaute Elektronik

**Option B: Crossbar-Montage (empfohlen)**
- Befestigung an einer Querstange (Crossbar) zwischen den
  Lenkerhaltern
- Die PD06 hat oft schon eine Crossbar serienmaessig oder
  nachgeruestet
- Vorteil: stabilere Plattform, weniger Lenker-Drehmoment,
  bessere Positionierung

**Option C: Lenkerhalter-Klemmbruecke**
- Montage an den Lenkerhalter-Klemmen (M6 oder M8 Schrauben)
- Vorteil: sehr stabil, ohne Lenker zu belasten
- Nachteil: modellspezifisch, braucht angepasste Halterung

**Empfohlene Montage (Option B: Crossbar) mit Kugelgelenk:**

Die Halterung wird an der Crossbar befestigt und ueber ein Kugelgelenk
mit dem Gehaeuse verbunden. Das erlaubt Neigung und Rotation fuer
optimale Lesbarkeit.

**Montage-Schritte:**

1. Crossbar zwischen den Lenkerhaltern montieren (falls nicht
   vorhanden). Die Crossbar ist ein Rohr mit 22 mm Durchmesser, das
   mit Klemmschellen an den Lenkerhaltern fixiert wird.
2. U-Bolt um die Crossbar legen.
3. Halterungs-Adapter (ASA, 3D-gedruckt) zwischen U-Bolt und
   Crossbar positionieren.
4. U-Bolt mit M6-Muttern festziehen (mit Federscheibe).
   Anzugsdrehmoment: 5-8 Nm (handfest + 1/2 Umdrehung).
5. Gummidmpfer zwischen Halterungs-Adapter und Gehaeuserueckseite
   einsetzen. Gummi-Metall-Verbunddaempfer, Shore 50-60A, M6-Gewinde.
6. Kugelgelenk mit M6-Schraube am Halterungs-Adapter befestigen.
7. Gehaeuse auf das Kugelgelenk setzen und mit Klemmhebel arretieren.
8. Neigung einstellen (10-15° nach hinten vom Fahrer weg) und
   Klemmhebel festziehen.

**ASCII-Diagramm: Lenkerhalterung (Seitenansicht)**

```
       Fahrer <===  Fahrtrichtung

      /---\                    /---\
     |     |                  |     |
     | Len |                  | Len |
     | ker  |                | ker  |
     \--+--/                  \--+--/
        |                        |
        +--------+==+------------+
                 ||              <-- Crossbar (22 mm)
              +--++--+
              | U-Bolt|               <-- U-Bolt-Schelle
              +--++--+
                 ||
          +------++------+
          | Halterungs-  |           <-- Halterungs-Adapter (ASA)
          | Adapter      |
          +------+-------+
                 |
          [Gummidmpfer]               <-- Vibrations-Entkopplung
                 |
          +------+-------+
          | Kugelgelenk  |           <-- Neigungsverstellung 0-15°
          +------+-------+
                 |
          +------+-------+
          |  Gehaeuse    |           <-- Bordcomputer-Gehaeuse
          |  (Display)   |
          +--------------+
```

### 4.2 E-Paper-Display-Position (falls Dual-Display)

Das E-Paper-Display (2.13", 250x122 Pixel) wird als sekundaeres
Display fuer statische Informationen verwendet (Hoehe, Uhrzeit,
Kompass, Tour-Daten). Es wird im Gehaeuse hinter der Acrylglas-
Abdeckung montiert.

**Montage im Gehaeuse:**

1. E-Paper-Modul in den PETG-Display-Rahmen einsetzen.
2. 0,5 mm Silikon-Unterlage unter das Modul legen (Vibrations-
   entkopplung, wichtig wegen des Glas-Substrats).
3. Mit 4 x M2,5 x 6 mm Schrauben und Federscheiben im Display-Rahmen
   fixieren.
4. FPC-Kabel (Flexible Printed Circuit) vom E-Paper zum PCB
   anschliessen. Kabel mit etwas Spiel, nicht unter Spannung.
5. Display-Rahmen mit dem Deckel verschrauben.

Das TFT-Display (3.5") wird ebenfalls ueber ein FPC-Kabel an den
Display-Connector auf dem PCB angeschlossen. Das TFT ist write-only
(kein MISO benoetigt).

### 4.3 Kabel-Verlegung vom Display zur MCU

Die Display-Kabel verlaufen innerhalb des Gehaeuses (geschuetzt vor
Wasser und Vibration). Sie sind kurz (< 10 cm) und werden als
FPC-Flachbandkabel ausgefuehrt.

**Wichtig:**
- FPC-Kabel nicht unter Spannung (knickgefaehrdet).
- FPC-Kabel nicht in der Naehe von Hochfrequenzquellen (GPS-Antenne,
  BLE-Antenne).
- FPC-Connector auf dem PCB: 12-polig, 0,5 mm Pitch. Vorsichtig
  einstecken, die Contacts sind fein und empfindlich gegen
  Fehlausrichtung.

### 4.4 Windschutz und Wasserfestigkeit der Display-Einheit

Die Display-Abdeckung besteht aus 2 mm Acrylglas (PMMA) oder
Polycarbonat. Sie ist eingeclipst in den Deckel, mit Silikon-
Dichtmasse umlaufend versiegelt.

**Wasserabweisende Beschichtung:**
Eine hydrophobe Beschichtung auf der Aussenseite der Abdeckung
sorgt dafuer, dass Tropfen abperlen und die Sicht nicht
beeintraechtigen.

**Wölbung:**
Die Abdeckung hat eine leichte Wölbung nach aussen (konvex), damit:
- Wasser von der Mitte nach aussen ablaeuft.
- Die Display-Flaeche keine waagerechte Flaeche bildet, auf der
  Wasser stehen bleiben kann.

**Windschutz:**
Das Gehaeuse selbst dient als Windschutz fuer das Display. Die
Neigung (10-15° nach hinten) stellt sicher, dass der Fahrtwind
ueber das Display hinwegstroemt und nicht direkt auf die
Abdeckung trifft. Das reduziert Verschmutzung und
Regentropfen-Ansammlung.

**Neigungsempfehlung:**
- 10-15° nach hinten (vom Fahrer weg geneigt).
- Bei tiefem Sonnenstand: Licht wird nicht direkt vom Display
  reflektiert.
- Bei hohem Sonnenstand: Display wird von oben beleuchtet.
- Regenwasser perlt ab (Tropfenform durch konvexe Wölbung).

---

## 4.5 CarPlay-Display: Montage am Lenker

Das CarPlay-Display (5" IPS, Wireless CarPlay) ist ein externes
Zusatz-Display, das neb dem Bordcomputer-Display montiert wird.
Es koppelt sich direkt ueber Bluetooth + WiFi ans iPhone oder
Android-Smartphone und benoetigt keine Kabelverbindung zum Bordcomputer.
Die Kommunikation Bordcomputer zu CarPlay-Display laeuft ueber BLE GATT
(Telemetrie-Spiegel: RPM, Geschwindigkeit, Kuehlmitteltemp etc.).

### 4.5.1 Montage-Position

Das CarPlay-Display wird auf der linken Lenkerseite montiert
(neben dem Bordcomputer-Gehaeuse), so dass beide Displays im Sichtfeld
des Fahrers liegen. Alternative: ueber dem Bordcomputer-Gehaeuse
gestapelt, wenn der Lenkerplatz begrenzt ist.

**Montage-Optionen:**

**Option A: Lenkerhalterung 22mm (empfohlen)**
- Aluminium-Halterung mit U-Bolt am 22mm-Lenker
- Schwingungsgeedaempft (Gummi-Puffer zwischen Halterung und Display)
- Neigung einstellbar (Kugelgelenk)
- Lieferung im CarPlay-Upgrade-Kit enthalten

**Option B: Crossbar-Montage**
- Befestigung an einer Querstange (Crossbar) zwischen den Lenkerhaltern
- Gleiche Montage wie Bordcomputer-Gehaeuse (Abschnitt 4.1)
- Vorteil: stabilere Plattform, laesst sich zusammen mit dem Bordcomputer
  an der Crossbar positionieren

**Option C: Lenkerhalter-Klemmbruecke**
- Direkt an den Lenkerhalter-Klemmen (M6 oder M8)
- Sehr stabil, aber modellspezifisch

### 4.5.2 Montage-Schritte (Option A: Lenkerhalterung 22mm)

1. U-Bolt der Lenker-Halterung um den 22mm-Lenker legen (linke Seite,
   neben dem Bordcomputer-Gehaeuse). Position so waehlen, dass das
   Display den Lenker-Einschlag nicht blockiert.
2. Halterung mit M6-Muttern und Federscheiben festziehen
   (Anzugsdrehmoment 5-8 Nm).
3. Gummidmpfer zwischen Halterung und Display-Rueckseite einsetzen.
4. Kugelgelenk mit M6-Schraube an der Halterung befestigen.
5. CarPlay-Display auf das Kugelgelenk setzen und mit Klemmhebel
   arretieren.
6. Neigung einstellen (10-15° nach hinten vom Fahrer weg) und
   Klemmhebel festziehen.
7. Kabelfuehrung: USB-C Kabel (wasserdicht, IP67) vom Display
   entlang dem Lenker zum Bordcomputer verlegen. Kabel alle 10 cm
   mit Kabelbindern fixieren.

**ASCII-Diagramm: CarPlay-Display am Lenker (Draufsicht)**

```
                    Fahrer ===>

         +------+           +------+
         |      |           |      |
         | TFT  |  Bordcomp.|CarPlay|   CarPlay-Display (5")
         |3,5"  |  Gehaeuse | 5"   |
         |      |           |      |
         +------+           +------+
              |              |
         +----+----+   +----+----+
         | Lenker |   | Lenker |
         | 22mm   |   | 22mm   |
         +--------+   +--------+
              |              |
              +------+-------+
                     |
              [Lenkerhalterung]
```

### 4.5.3 Stromversorgung CarPlay-Display

Das CarPlay-Display wird ueber USB-C mit 5V versorgt. Die
Stromversorgung erfolgt ueber den 12V-auf-5V Buck-Converter, der
bereits im Bordcomputer-Gehaeuse sitzt.

**Anschluss:**
1. USB-C Kabel (wasserdicht, IP67) vom CarPlay-Display durch eine
   PG7-Kabelverschraubung ins Bordcomputer-Gehaeuse fuehren.
2. USB-C Kabel an den 5V-Ausgang des Buck-Converters (MP1584)
   anschliessen (parallel zum Bordcomputer, gleiche 5V-Schiene).
3. Sicherung pruefen: Der Buck-Converter ist mit 5A abgesichert.
   Das CarPlay-Display zieht typisch 500-800 mA (2,5-4W). Die 5A-
   Sicherung reicht fuer Bordcomputer + CarPlay-Display.
4. Ferritkern (Klappferrit) am USB-C Kabel nah am Gehaeuse.

**Stromaufnahme:**
- CarPlay-Display (ohne Bluetooth-Pairing): 300-400 mA
- CarPlay-Display (mit CarPlay aktiv, GPS, Musik): 600-800 mA
- Maximal (Display auf voller Helligkeit, ladendes Smartphone): 1.200 mA

### 4.5.4 Wasserfestigkeit CarPlay-Display

Das CarPlay-Display hat IPX7 (EKIY, Jiuyin) bzw. IP67 (KEMIMOTO).
Trotzdem sollten folgende Punkte beachtet werden:

- USB-C-Anschluss am Display ist mit einer Gummi-Klappe abgedeckt.
  Nach dem Anschließen des Kabels die Klappe ueber das Kabel
  schliessen und mit Kabelbinder sichern.
- Das Display-Gehaeuse ist nicht fuer Dauereintauchen ausgelegt.
  Bei starkem Regen die Halterung so ausrichten, dass Wasser von
  oben ablaufen kann (leichte Neigung nach vorne).
- Nach jeder Fahrt im Regen: Display und Kabelanschluesse
  kontrollieren, bei Korrosion Kontaktspray verwenden.

---

## 4.6 Lenker-Controller: Montage und Anschluss

Der Lenker-Controller ermoeglicht die Bedienung von Bordcomputer-Display
und CarPlay ohne den Handschuh auszuziehen. Es gibt drei Varianten:
SilverFox C2 (Premium, kabelgebunden + BLE), Chigee CGRC (wireless BLE)
und Custom PCB (I2C).

### 4.6.1 Montage-Position

Der Controller wird am rechten Lenker montiert, in Reichweite des
Daumens der rechten Hand. Die Position muss so gewaehlt werden, dass:

- Der Daumen alle Tasten/den Joystick bequem erreicht.
- Der Gasgriff nicht blockiert wird.
- Der Controller nicht mit der Hand brake oder dem Kill-Switch kollidiert.

**Montage:**

1. Controller-Halterung um den 22mm-Lenker legen (rechte Seite).
   Bei SilverFox C2 ist die Halterung im Lieferumfang enthalten.
   Bei Chigee CGRC ist eine 30-35mm-Halterung enthalten -- hier
   kann ein Adapter (22mm auf 30mm) noetig sein.
2. Halterung mit Inbus-Schrauben (M5 oder M6) und Federscheiben
   festziehen. Anzugsdrehmoment 3-5 Nm.
3. Controller in die Halterung einsetzen und arretieren
   (Riegel oder Schraube).
4. Kabelfuehrung: Controller-Kabel entlang dem Lenker nach links
   zum Bordcomputer-Gehaeuse verlegen. Kabel nicht unter Spannung,
   mit Kabelbindern am Lenker fixieren (alle 10 cm).

**ASCII-Diagramm: Controller am Lenker (Draufsicht)**

```
                    Fahrer ===>

  +------+        +------+        +------+
  | Ctrl |        | TFT  |        |CarPlay|
  | 22mm |  Lenker|3,5"  |  Lenker| 5"   |
  +------+        +------+        +------+
       |               |               |
  +----+----+    +----+----+    +----+----+
  | Lenker  |    | Lenker |    | Lenker |
  | 22mm    |    | 22mm   |    | 22mm   |
  +---------+    +--------+    +--------+
       |               |               |
       +-------+-------+-------+-------+
               |               |
        [Lenkerhalter]  [Lenkerhalter]
```

### 4.6.2 Anschluss: SilverFox C2 (kabelgebunden + BLE)

Der SilverFox C2 unterstuetzt gleichzeitig Kabelverbindung ueber M12-
Stecker und BLE. Fuer die kabelgebundene Variante:

**M12-Anschluss:**
1. M12-Kabel (4-polig, IP67, 1,5m) am Controller einstecken und
   arretieren (Bajonett-Verschluss, 1/4 Drehung).
2. Anderes Ende des M12-Kabels durch eine PG9-Kabelverschraubung ins
   Bordcomputer-Gehaeuse fuehren.
3. M12-Pin-Belegung:
   - Pin 1: +5V (vom Buck-Converter)
   - Pin 2: GND
   -  Pin 3: GPIO (Controller-Data, mit Pull-Up 10k)
   -  Pin 4: GPIO (Controller-Clock, mit Pull-Up 10k)
4. Pin 1 und 2 an die 5V-Schiene bzw. GND anschliessen.
5. Pin 3 und 4 an die Controller-GPIO-Pins des ESP32-S3 anschliessen
   (GPIO17 DATA, GPIO18 CLOCK -- siehe Pinmap).

**BLE-Alternative:**
- SilverFox C2 kann auch rein ueber BLE gekoppelt werden
  (ohne M12-Kabel). In diesem Fall nur die 5V-Versorgung anschliessen.
- Pairing ueber die Companion-App oder Serial Console.
- Vorteil: nur ein Stromkabel noetig, kein Datenkabel.
- Nachteil: Batterie-Abhaengigkeit des Controllers (CR2032, ca. 6 Monate).

### 4.6.3 Anschluss: Chigee CGRC Multi (wireless BLE)

Der Chigee CGRC ist ein rein wireless Controller (BLE 5.0, CR2032).

**Montage:**
1. Controller-Halterung am 22mm-Lenker montieren (rechte Seite).
   Achtung: CGRC-Halterung ist fuer 30-35mm ausgelegt. Fuer 22mm
   Lenker wird ein Adapter-Ring (22mm auf 30mm) benoetigt, im
   CarPlay-Upgrade-Kit enthalten.
2. Controller in die Halterung clippen.

**Stromversorgung:**
- CR2032-Batterie (im Lieferumfang). Lebensdauer ca. 6 Monate bei
  taeglicher Nutzung.
- Keine Kabelverbindung zum Bordcomputer noetig.

**BLE-Pairing:**
- Pairing ueber die Companion-App: "Controller koppeln" -> CGRC
  aus der Liste auswaehlen -> bestaetigen.
- Alternative: Serial Console -> `controller pair` -> CGRC auswaehlen.
- Nach dem Pairing verbindet sich der Controller automatisch beim
  Bordcomputer-Start.

### 4.6.4 Anschluss: Custom Controller PCB (I2C)

Der Custom Controller (ATmega328P oder RP2040) wird ueber I2C
angeschlossen. Das PCB wird am Lenker montiert und ueber ein 4-adriges
Kabel (SDA, SCL, VCC, GND) mit dem Bordcomputer verbunden.

**Anschluss:**
1. 4-adriges Kabel (AWG24, abgeschirmt) vom Controller-PCB zum
   Bordcomputer-Gehaeuse verlegen (durch PG7).
2. Pin-Belegung:
   - VCC: 3,3V (vom ESP32-S3, ueber Ferrite-Bead)
   - GND: Masse
   - SDA: GPIO8 (I2C Data, mit Pull-Up 4,7k)
   - SCL: GPIO9 (I2C Clock, mit Pull-Up 4,7k)
3. Schirm an einem Ende auf Masse (Gehaeuseseite).
4. Controller-PCB am 22mm-Lenker mit zwei Kabelbindern oder einer
   3D-gedruckten Halterung fixieren.

**Firmware:**
- Der Custom Controller laeuft mit eigener Firmware (ATmega328P: Arduino,
  RP2040: MicroPython oder C). Die Firmware liest die Tasten und den
  Joystick ein und sendet Events ueber I2C an den ESP32-S3.
- Die Firmware-Komponente `firmware/components/hid_controller/` im
  Bordcomputer verarbeitet die I2C-Events (siehe TOT-18).

---

## 4.7 CarPlay und Controller: Pairing und Konfiguration

Nach der Montage von CarPlay-Display und Lenker-Controller muessen
beide mit dem Bordcomputer und dem Smartphone gekoppelt werden.

### 4.7.1 CarPlay-Display mit Smartphone koppeln

1. Bordcomputer einschalten (Zündung an). CarPlay-Display geht an
   (5V vom Buck-Converter).
2. iPhone: Einstellungen -> Bluetooth -> Suchen. CarPlay-Display
   (z.B. "EKIY-CP-XXXX") aus der Liste auswaehlen und koppeln.
3. iPhone fragt: "CarPlay verwenden?" -> Zulassen.
4. CarPlay-Oberflaeche erscheint auf dem 5"-Display. Navigation,
   Musik, Telefon sind jetzt verfuegbar.
5. Bei Android: Android Auto oeffnen -> Pairing -> Display auswaehlen.

**Bei Verbindungsproblemen:**
- Bluetooth am Smartphone aus- und einschalten.
- CarPlay-Display neu starten (Zündung aus, 10 Sekunden, an).
- iPhone: Einstellungen -> Allgemein -> CarPlay -> Neues Fahrzeug.
- Abstand < 1 Meter beim ersten Pairing.

### 4.7.2 CarPlay-Display mit Bordcomputer koppeln (Telemetrie-Spiegel)

Das CarPlay-Display kann Telemetrie-Daten vom Bordcomputer empfangen
(RPM, Geschwindigkeit, Kuehlmitteltemp etc.). Dies erfordert eine BLE-
Verbindung zwischen Bordcomputer und CarPlay-Display.

1. Serial Console oeffnen (USB-C, 115200 Baud).
2. Befehl: `carplay pair`
3. Bordcomputer sucht nach CarPlay-Displays in der Naehe.
4. CarPlay-Display aus der Liste auswaehlen und bestaetigen.
5. Bordcomputer sendet nun Telemetrie-Daten ueber BLE GATT an das
   CarPlay-Display. Die Daten werden in der CarPlay-App des
   Bordcomputers angezeigt (falls installiert) oder als Widget
   auf dem CarPlay-Home-Screen.

**Gesendete Daten (BLE GATT TX):**
- RPM (U/min)
- Geschwindigkeit (km/h)
- Kuehlmitteltemperatur (°C)
- Kraftstoffstand (%)
- Gang (N, 1-5)
- Oeldruck (bar, falls installiert)

### 4.7.3 Lenker-Controller mit Bordcomputer koppeln

Je nach Controller-Variante:

**SilverFox C2 (BLE):**
1. Serial Console oder Companion-App: `controller pair`
2. SilverFox C2 in den Pairing-Modus versetzen (Taste auf der Rueckseite
   des Controllers 3 Sekunden gedrueckt halten, bis LED blinkt).
3. Controller aus der Liste auswaehlen und bestaetigen.
4. LED leuchtet durchgehend blau = verbunden.

**Chigee CGRC (BLE):**
1. Companion-App: "Controller koppeln" -> CGRC auswaehlen.
2. CR2032-Batterie einlegen (falls noch nicht geschehen). Controller
   geht automatisch in den Advertising-Modus.
3. Pairing bestaetigen.

**Custom PCB (I2C):**
1. Kein Pairing noetig. Controller wird beim Booten automatisch
   erkannt (I2C-Scan in der Self-Test-Phase).
2. Falls der Controller nicht erkannt wird: I2C-Adresse pruefen
   (Standard: 0x08). Serial Console: `i2c scan` -> 0x08 muss
   in der Liste erscheinen.

### 4.7.4 Controller-Mapping konfigurieren

Die Standard-Belegung des Controllers:

| Taste/Joystick | Aktion Bordcomputer-Display | Aktion CarPlay |
|---------------|----------------------------|----------------|
| Joystick Up/Down | Menue hoch/runter | Liste hoch/runter |
| Joystick Left/Right | Menue links/rechts | Vor/Zurueck |
| Joystick Select (Druck) | Menue bestaetigen | Play/Pause |
| Taste Back | Zurueck | Home |
| Taste Home | Hauptmenue | Siri / Google Assistant |
| Vol + / Vol - | Bordcomputer Lauter/Leiser | Musik Lauter/Leiser |
| Media Next / Prev | --- | Naechster/Voriger Track |

Die Belegung kann ueber die Companion-App oder Serial Console
angepasst werden:

```
> controller mapping
Current mapping:
  NAV_UP    -> MENU_UP
  NAV_DOWN  -> MENU_DOWN
  NAV_LEFT  -> MENU_LEFT
  NAV_RIGHT -> MENU_RIGHT
  BTN_SELECT -> MENU_SELECT
  BTN_BACK   -> MENU_BACK
  BTN_HOME   -> MENU_HOME
  VOL_UP     -> VOLUME_UP
  VOL_DOWN   -> VOLUME_DOWN
  MEDIA_NEXT -> MEDIA_NEXT
  MEDIA_PREV -> MEDIA_PREV

> controller mapping set NAV_UP MENU_SCROLL_UP
Mapping updated: NAV_UP -> MENU_SCROLL_UP
```

### 4.7.5 Test: Controller und CarPlay

Nach Pairing und Konfiguration testen:

1. Bordcomputer-Display: Mit dem Controller durch die Menues
   navigieren. Alle Tasten reagieren?
2. CarPlay-Display: Musik abspielen, mit dem Controller Skip/Play/
   Pause steuern. Navigation oeffnen, mit dem Controller zoomen.
3. Telemetrie-Spiegel: CarPlay-Display zeigt RPM, Geschwindigkeit
   etc. vom Bordcomputer? (Falls BLE-Pairing mit Bordcomputer
   durchgefuehrt, siehe 4.7.2).
4. Bei Verbindungsabbruch: Serial Console -> `controller status`
   und `carplay status` pruefen.

---

## 5. Inbetriebnahme

### 5.1 Erster Power-On: Spannungen messen, Smoke-Test

Nach der kompletten Installation (mechanisch und elektrisch) kann der
Bordcomputer zum ersten Mal eingeschaltet werden.

**Vorbereitung:**

1. Alle Kabelverbindungen pruefen (Sichtkontrolle: keine losen Kabel,
   alle Stecker eingerastet, alle Schrauben mit Federscheiben gesichert).
2. Sicherung kontrollieren (5A, eingesetzt).
3. Batterie anklemmen (zuerst Pluspol schwarz/braun, dann Minuspol
   gruen). Anzugsdrehmoment Batteriepole: 3-5 Nm.
4. Multimeter an die 12V-Eingangsklemmen des Bordcomputers halten und
   Spannung pruefen. Sollte ca. 12-13V sein, wenn Zuendung aus.

**Smoke-Test am PCB (vor dem Gehaeuse-Einbau durchgefuehrt, siehe
Abschnitt 2.3):**
Wenn der Smoke-Test bereits erfolgreich war, kann dieser Schritt
uebersprungen werden. Wenn nicht, jetzt durchfuehren.

**Erstes Einschalten:**

1. Zündung einschalten (Motor nicht starten).
2. Der Bordcomputer sollte innerhalb von 2-3 Sekunden hochfahren.
3. Das Display zeigt den Boot-Screen (Logo, Firmware-Version).
4. Der Self-Test laeuft ab (siehe 5.2).
5. Spannungen am Bordcomputer pruefen:
   - 12V-Eingang: 12-15V bei eingeschalteter Zuendung
   - 5V-Rail: 5,0 ± 0,1V
   - 3,3V-Rail: 3,3 ± 0,05V
6. Wenn alles korrekt: Smoke-Test bestanden. Motor kann gestartet
   werden.
7. Nach Motorstart: Bordspannung sollte auf 13,5-15,5V steigen
   (Generator laeuft). Wenn die Spannung unter 12V bleibt: Generator
   oder Regler/Gleichrichter pruefen.

**Was tun, wenn der Smoke-Test fehlschlaegt:**
- Wenn die 5A-Sicherung durchbrennt: Kurzschluss. Alle Kabelver-
  bindungen auf Kurzschluss pruefen (Plus nach Masse). Sicherung erst
  ersetzen, nachdem der Kurzschluss behoben ist.
- Wenn 5V-Rail fehlt: Buck-Converter defekt oder ueberlastet.
- Wenn 3,3V-Rail fehlt: LDO defekt. ESP32-S3 ueberpruefen.
- Wenn Rauch oder Geruch: Sofort abschalten. Bauteil ueberpruefen.

### 5.2 Self-Test ueber Serial Console

Der Bordcomputer fuehrt beim Booten einen Self-Test durch, der alle
Hardware-Komponenten prueft. Die Ergebnisse koennen ueber die
Serial Console (USB-C, 115200 Baud) ausgelesen werden.

**Self-Test-Abfolge:**

1. **Display-Test:** Alle Pixel werden kurz eingeschaltet (Schwarzbild),
   dann geloescht (Weissbild). Dauer: ca. 2 Sekunden.
2. **GPS-Modul initialisieren:** Konfiguration an das GPS-Modul senden
   und auf Antwort warten. Wenn keine Antwort: DTC TBC-0201.
3. **I2C-Sensoren ansprechen:** IMU (MPU6050/BMI270) und Barometer
   (BMP280/BMP390) werden ueber I2C angesprochen. Wenn ein Sensor
   nicht antwortet: DTC TBC-01xx.
4. **SD-Karte mounten:** MicroSD-Karte wird initialisiert und
   gemountet. Wenn die Karte fehlt oder defekt ist: DTC TBC-0501.
5. **ADC-Kanaele lesen:** Batteriespannung, Kraftstoffstand und
   andere analoge Sensoren werden gelesen. Wenn ein Wert unplausibel
   ist (z.B. Spannung <5V oder >20V): DTC TBC-03xx.
6. **Optokoppler-Eingaenge pruefen:** Die RPM-Signal-Eingaenge werden
   auf Plausibilitaet geprueft (Pull-Up-Widerstand pruefen, kein
   permanenter Low-Pegel).
7. **BLE-Modul initialisieren:** Bluetooth-Modul wird initialisiert und
   in den Advertising-Modus geschaltet. Wenn keine Antwort:
   DTC TBC-04xx.
8. **Konfiguration laden:** Konfiguration wird von der SD-Karte (oder
   Flash) geladen. Wenn die Konfiguration korrupt ist:
   Werkseinstellungen laden.

**Ergebnis:**
- Bei Erfolg: Dreifacher Piepton, Status-LED leuchtet gruen.
  Bordcomputer wechselt in die Hauptansicht.
- Bei Fehler: Absteigende Tonfolge, Status-LED leuchtet rot.
  DTC wird auf dem Display angezeigt. Der Bordcomputer funktioniert
  mit eingeschraenkter Funktionalitaet (fehlende Sensoren werden als
  "---" angezeigt).

**Serial Console verbinden:**

1. USB-C-Kabel vom Computer an die USB-C-Buchse (IP67) des
   Bordcomputers anschliessen.
2. Serial-Terminal oeffnen (z.B. screen, minicom, PuTTY):
   `screen /dev/ttyUSB0 115200` (Linux/macOS)
   `putty -serial COM3 -seri 115200` (Windows)
3. Bordcomputer neu starten (Zündung aus, 5 Sekunden warten,
   Zündung an).
4. Boot-Meldungen und Self-Test-Ergebnisse im Terminal beobachten.

### 5.3 Kalibrierung: RPM-Multiplikator, Tank-Kalibrierungstabelle

Nach erfolgreichem Self-Test muessen die Sensoren kalibriert werden.
Die Kalibrierung erfolgt ueber die Companion-App oder die Serial
Console.

#### 5.3.1 RPM-Multiplikator

Die Drehzahl wird aus der Frequenz des Pickup-Signals berechnet:
RPM = 60 × f (bei 1 Puls pro Kurbelwellenumdrehung).

Die PD06 hat 1 Puls pro Kurbelwellenumdrehung (Pickup-Pulsgenerator).
Der Multiplikator ist 60.

**Ueberpruefung:**
- Motor im Leerlauf (1.300 ± 100 U/min): Anzeige sollte ca. 1.300
  U/min zeigen.
- Bei 3.000 U/min (Drehzahlmesser am Motorrad zum Vergleich): Anzeige
  sollte ca. 3.000 U/min zeigen.
- Wenn die Anzeige konstant zu hoch oder zu niedrig ist: Multiplikator
  in den Einstellungen anpassen.

**Abweichungsursachen:**
- Bei Modellen bis 1995 mit zwei Pickup-Spulen: Stelle sicher, dass
  das richtige Kabel abgegriffen wurde (gelb/weiss fuer Zuendspule/
  Pickup). Bei Abgriff an beiden Pickup-Spulen wuerde die Frequenz
  verdoppelt (RPM = 30 × f).
- Bei Abgriff an der Zuendspule (Coil-): Die Zuendung hat
  Doppelzuendung (twin spark, 2 Zuendkerzen pro Zylinder). Das Signal
  an der Coil- kann 2 Pulse pro Umdrehung liefern (RPM = 30 × f).

#### 5.3.2 Tank-Kalibrierungstabelle

Die Kennlinie des Kraftstoffsensors ist nicht linear (oben flach,
unten steil). Die Kalibrierung erfolgt ueber die Companion-App mit
drei Stuetzstellen:

1. Tank volltanken (18 Liter).
2. App oeffnen, Sensor-Kalibrierung -> Kraftstoffstand ->
   "Voll" kalibrieren. ADC-Wert wird gespeichert.
3. Tank auf 50% entleeren (ca. 9 Liter verbrauchen).
4. "Halbvoll" kalibrieren. ADC-Wert wird gespeichert.
5. Tank auf Reserve (ca. 3 Liter Rest) entleeren.
6. "Leer" kalibrieren. ADC-Wert wird gespeichert.
7. Die App interpoliert die Kennlinie aus den drei Stuetzstellen.

**Tankdaten PD06:**
- Tankvolumen: 18,0 Liter
- Reserve: 2,9-3,5 Liter
- Verbrauch: ca. 4,5-5,5 L/100 km

#### 5.3.3 Temperatur-Kalibrierung

Der Kuehlmitteltemperatur-Sensor (NTC 10k, B3950) kann ueber die
Companion-App mit einem Offset-Wert kalibriert werden:

1. Motor kalt (Raumtemperatur). Referenzthermometer in den
   Kuehlmittel-Einfuellstutzen halten.
2. Angezeigte Temperatur mit Referenzthermometer vergleichen.
3. Offset in der App einstellen (z.B. -2°C, wenn die Anzeige 2°C
   zu hoch ist).
4. Motor warmfahren. Bei Betriebstemperatur (ca. 80-90°C) erneut
   pruefen.

#### 5.3.4 Tacho-Kalibrierung (falls Hall-Sensor)

Wenn ein Hall-Sensor am Vorderrad installiert ist, muss der
Reifenumfang kalibriert werden:

- Vorderrad (90/90-21): Umfang 2.185 mm
- Hinterrad (130/80-17): Umfang 2.010 mm

Der Reifenumfang kann sich mit Reifenmarke und -profil leicht
aendern. Fuer genaue Messung: Reifenumfang mit Massband am
aufgepumpten Reifen messen und in der App einstellen.

Bei 100 km/h dreht das Vorderrad mit ca. 763 U/min (12,7 Hz).

### 5.4 GPS-Fix abwarten

Nach dem Self-Test beginnt das GPS-Modul mit der Satellitensuche.
Bei einem Kaltstart (erstes Einschalten oder nach langer Standzeit)
kann der erste GPS-Fix bis zu 2 Minuten dauern.

**Voraussetzungen fuer einen schnellen GPS-Fix:**
- Freie Sicht nach oben (im Freien, nicht in einer Garage oder unter
  einem Dach).
- Die GPS-Antenne darf nicht durch Metall verdeckt sein (Tank,
  Motorblock, Verkleidung).
- Mindestens 10 cm Abstand zwischen GPS-Antenne und GPS-Modul (bei
  aktiven Antennen).
- Mindestens 4 Satelliten fuer einen 3D-Fix.

Sobald ein GPS-Fix erreicht ist, leuchtet die GPS-LED durchgehend
blau und das Display zeigt die aktuelle Geschwindigkeit und Position.

**Wenn kein GPS-Fix nach 5 Minuten:**
1. Drinnen? GPS funktioniert nur im Freien.
2. Antenne verdeckt? Kontrollieren.
3. Antennenkabel defekt? Kontrollieren.
4. GPS-Modul defekt? Ueber Diagnose-Konsole pruefen.

### 5.5 BLE-Pairing mit Smartphone

Nach dem GPS-Fix kann die Companion-App gekoppelt werden:

1. Companion-App auf dem Smartphone oeffnen.
2. "Geraet koppeln" tippen.
3. App sucht nach dem Bordcomputer (BLE-Scan, 5-10 Sekunden).
4. Bordcomputer aus der Liste auswaehlen
   ("Transalp-BC-XXXX").
5. Pairing bestaetigen (Just-Works oder Passkey-Eingabe).
6. BLE-LED am Bordcomputer leuchtet durchgehend weiss.
7. Die App laedt die aktuelle Konfiguration und Telemetrie-Daten.

Nach dem ersten Pairing werden die Pairing-Keys gespeichert. Beim
naechsten Start verbinden sich Bordcomputer und Smartphone automatisch,
sobald die Zuendung eingeschaltet wird und die App im Hintergrund
laeuft.

**Praktische Reichweite:**
- Smartphone in der Brust- oder Jackentasche: 5-15 Meter, stabil.
- Smartphone im Tankrucksack: 2-5 Meter, meist stabil.
- Smartphone im Rucksack am Ruecken: 1-3 Meter, oft instabil.
- Smartphone im Seitenkoffer: unzuverlaessig (Metall blockiert).

### 5.6 Testfahrt: alle Sensoren plausibel?

Nach der Inbetriebnahme sollte eine Testfahrt durchgefuehrt werden,
um alle Sensoren auf Plausibilitaet zu pruefen.

**Testfahrt-Checkliste:**

| Sensor | erwartetes Verhalten | Pruelfpunkt |
|---------|---------------------|-------------|
| Drehzahl (RPM) | Folgt Gasgriff, Leerlauf ca. 1.300 | Vergleich mit Tacho/Drehzahlmesser |
| Geschwindigkeit | Folgt Tacho, plausibel | Vergleich mit GPS-Geschwindigkeit |
| Kuehlmitteltemp | Steigt langsam auf 80-95°C | Bei 98°C Luefter einschalten |
| Bordspannung | 13,5-15,5V bei laufendem Motor | Bei Leerlauf: evtl. unter 13V |
| Tankstand | Sinkt langsam, plausibel | Nachvollziehbar bei bekannter Fuellung |
| GPS-Position | Folgt der Strecke | Auf Karte in der App verfolgen |
| GPS-Geschwindigkeit | Entspricht Tacho ±5% | Bei konstanter Geschwindigkeit |
| IMU (Neigung) | Reagiert auf Kurvenlage | In Kurven sichtbar |
| Barometer/Hoehe | Entspricht Topographie | Auf bekannter Strecke pruefen |
| Oeldruck (falls installiert) | 3-5 bar bei Fahrt, >0,5 bar Leerlauf | Bei warmem Motor |

**Bei Unplaussibilitaeten:**
- Drehzahl zu hoch/niedrig: Multiplikator pruefen (5.3.1).
- Geschwindigkeit falsch: Reifenumfang kalibrieren (5.3.4).
- Temperatur unplausibel: Sensor-Kabel und Kalibrierung pruefen.
- Bordspannung zu niedrig: Generator/Regler des Motorrads pruefen.
- GPS ungenau: Antennenposition pruefen.
- BLE-Verbindung abgebrochen: Smartphone in Brusttasche tragen.

---

## 6. Fehlersuche

### 6.1 Haeufige Probleme und Loesungen

#### 6.1.1 Display bleibt dunkel

**Ursachen und Loesungen:**

1. **Keine Spannung:** Sicherung (5A) kontrollieren. 12V-Eingangs-
   spannung am Bordcomputer mit Multimeter messen (sollte 12-15V bei
   laufendem Motor sein, 12-13V bei eingeschalteter Zuendung und
   stehendem Motor).

2. **Sicherung defekt:** Wenn die Sicherung durchgebrannt ist, liegt
   ein Kurzschluss vor. Kabelverbindungen auf Kurzschluesse pruefen
   (Plus nach Masse). Sicherung erst ersetzen, nachdem der Kurzschluss
   behoben ist.

3. **Display-Kabel lose:** Gehaeuse oeffnen (4 x M3 Schrauben am
   Deckel) und FPC-Kabel zwischen Display und PCB kontrollieren. Das
   Kabel muss fest im Connector sitzen, nicht unter Spannung.

4. **E-Paper-Refresh fehlgeschlagen:** Manchmal startet das E-Paper-
   Display nach einem Spannungsausfall nicht korrekt. Spannung
   trennen (Zuendung aus, 30 Sekunden warten), dann wieder
   einschalten.

5. **Firmware-Problem:** Wenn das Display nach dem Booten dunkel
   bleibt, aber die Status-LED leuchtet: USB-C-Kabel anschliessen
   und Serial Console (115200 Baud) pruefen.

#### 6.1.2 Kein GPS-Fix

**Ursachen und Loesungen:**

1. **Drinnen:** GPS funktioniert nur im Freien mit freier Sicht zum
   Himmel. Nach draussen gehen.

2. **Kalter Start:** Beim ersten Einschalten oder nach laengerer
   Standzeit kann der erste GPS-Fix bis zu 2 Minuten dauern.

3. **Antenne verdeckt:** Kontrollieren, ob die GPS-Antenne freie Sicht
   nach oben hat. Bei Montage im Gehaeuse: Kupferfolie-Abschirmung
   hat ein Fenster fuer die Antenne?

4. **Antennenkabel defekt:** Kabel zwischen GPS-Antenne und GPS-Modul
   kontrollieren. Kabel darf nicht geknickt oder beschaedigt sein.

5. **GPS-Modul defekt:** Nach 5 Minuten im Freien kein Fix: GPS-Modul
   ueber Diagnose-Konsole (USB-C) pruefen. Wenn das Modul nicht auf
   Konfigurationsbefehle antwortet: Modul ersetzen.

#### 6.1.3 BLE-Verbindung bricht ab

**Ursachen und Loesungen:**

1. **Smartphone zu weit weg:** Smartphone in der Brust- oder
   Jackentasche tragen, nicht im Rucksack. Praktische Reichweite:
   5-15 Meter.

2. **App im Hintergrund beendet:** Betriebssystem-Einstellungen fuer
   Hintergrundaktualisierung der Companion-App pruefen.

3. **Pairing verloren:** Pairing in der App loeschen und neu koppeln.

4. **Stoerung:** Andere Bluetooth-Geraete in der Naehe (Headset,
   Intercom) koennen stoeren. Andere Geraete ausschalten und testen.

5. **Firmware-Kompatibilitaet:** App und Firmware auf neueste Version
   aktualisieren.

#### 6.1.4 Drehzahl zeigt falsche Werte

**Ursachen und Loesungen:**

1. **Falsche Kalibrierung:** RPM = 60 × f bei 1 Puls pro Umdrehung.
   Wenn die Anzeige konstant zu hoch oder zu niedrig ist:
   Multiplikator in den Einstellungen pruefen (1 fuer die PD06).
   Bei Modellen bis 1995 mit zwei Pickup-Spulen: richtiges Kabel
   abgegriffen?

2. **Optokoppler defekt:** Wenn die Anzeige springt oder 0 anzeigt:
   Diagnose-Konsole pruefen, ob Pulse am GPIO-Eingang ankommen.
   Wenn nicht: Optokoppler-Modul ersetzen.

3. **EMV-Stoerung:** CDI-Zuendung erzeugt starke Stoerungen. Bei
   unruhiger Anzeige: Sensorkabel ausreichend geschirmt? Ferritkerne
   am Kabel? Kabel nicht in der Naehe der CDI-Box oder Zuendspule?

4. **Verbindung lose:** Alle Steckverbindungen zwischen Pickup und
   Bordcomputer kontrollieren. Nur verriegelte Steckverbinder
   verwenden.

5. **Falsches Kabel abgegriffen:** Bei Modellen bis 1995 mit zwei
   Pickup-Spulen: gelb/weiss fuer Zuendspule/Pickup. Bei
   Unsicherheit: Signal an der Minusseite der Zuendspule (Coil-)
   abgreifen.

#### 6.1.5 Temperatur unplausibel

**Ursachen und Loesungen:**

1. **Sensor-Kabel lose:** Bei offenem Kabel zeigt der ADC den
   Maximalwert (sehr niedrige Temperatur). Bei kurzgeschlossenem
   Kabel zeigt der ADC 0 (sehr hohe Temperatur).

2. **Sensor defekt:** Widerstand des Sensors bei bekannter Temperatur
   mit Multimeter messen. Mit Kennlinie vergleichen (50°C -> 154 Ohm,
   80°C -> 52 Ohm, 120°C -> 16 Ohm bei Original-Sensor;
   25°C -> 10k bei NTC 10k B3950).

3. **Wasser im Stecker:** Steckverbinder auf Korrosion oder
   Wassereintritt pruefen. Reinigen (Isopropanol), trocknen lassen,
   Kontaktspray behandeln. Mit Schrumpfschlauch ueberziehen.

4. **Kalibrierung falsch:** Offset-Wert in der Companion-App
   einstellen.

5. **EMV-Stoerung:** Analoge Sensorleitungen sind empfindlich.
   Kabel ausreichend geschirmt? Schirm an einem Ende auf Masse?
   RC-Filter (1 kOhm + 100 nF) am ADC-Eingang installiert?

### 6.2 DTC-Codes und Bedeutung

Der Bordcomputer verwendet Diagnostic Trouble Codes (DTCs) im Format
TBC-XYYY, wobei X die Kategorie und YYY die Fehlernummer ist.

| DTC-Code | Kategorie | Beschreibung | Massnahme |
|----------|-----------|-------------|-----------|
| TBC-0101 | I2C | MPU6050/BMI270 nicht gefunden | I2C-Verkabelung pruefen, Adresse 0x68 |
| TBC-0102 | I2C | BMP280/BMP390 nicht gefunden | I2C-Verkabelung pruefen, Adresse 0x76 |
| TBC-0201 | GPS | GPS-Modul antwortet nicht | UART-Verkabelung pruefen (TX=43, RX=44) |
| TBC-0202 | GPS | Kein GPS-Fix nach 5 min | Antennenposition pruefen |
| TBC-0301 | ADC | Bordspannung unplausibel (<5V oder >20V) | Spannungsteiler pruefen, Zuendschalter |
| TBC-0302 | ADC | Kuehlmitteltemp unplausibel | Sensor-Kabel, NTC-Widerstand messen |
| TBC-0303 | ADC | Kraftstoffstand unplausibel | Tankgeber-Kabel, Widerstand messen |
| TBC-0304 | ADC | Oeldruck unplausibel | Sensor-Kabel, Sensor-Widerstand messen |
| TBC-0401 | BLE | BLE-Modul nicht initialisierbar | Firmware-Update, ESP32-S3 pruefen |
| TBC-0501 | SD | SD-Karte nicht mountbar | SD-Karte einsetzen, FAT32 formatieren |
| TBC-0502 | SD | SD-Karte voll | SD-Karte austauschen oder Daten loeschen |
| TBC-0601 | RPM | Kein RPM-Signal | Optokoppler, Pickup-Kabel pruefen |
| TBC-0602 | Tacho | Kein Tacho-Signal | Reed-Kontakt/Hall-Sensor pruefen |
| TBC-0701 | Display | TFT nicht initialisierbar | FPC-Kabel pruefen |
| TBC-0702 | Display | E-Paper nicht initialisierbar | FPC-Kabel pruefen |
| TBC-0801 | Controller | Kein Controller gefunden (I2C/BLE) | I2C-Adresse/Verkabelung pruefen, BLE-Pairing wiederholen |
| TBC-0802 | Controller | Controller nicht verbunden | Batterie (CR2032) pruefen, Pairing wiederholen |
| TBC-0901 | CarPlay | CarPlay-Display nicht erreichbar | 5V-Versorgung, USB-C Kabel pruefen |
| TBC-0902 | CarPlay | Telemetrie-Spiegel nicht verbunden | BLE-Pairing Bordcomputer-Display wiederholen |

### 6.3 Serial-Console-Befehle fuer Diagnose

Die Serial Console (USB-C, 115200 Baud) bietet Diagnose-Befehle zur
Fehlersuche. Die folgenden Befehle koennen im Terminal eingegeben
werden:

| Befehl | Beschreibung |
|--------|-------------|
| `help` | Zeigt alle verfuegbaren Befehle |
| `status` | System-Status (Uptime, CPU-Temp, Speicher) |
| `sensors` | Aktuelle Sensorwerte (alle Sensoren) |
| `dtc` | Zeigt aktive DTC-Codes |
| `dtc clear` | Loescht alle DTC-Codes |
| `gps status` | GPS-Status (Fix, Satelliten, Position) |
| `gps nmea` | Roh-NMEA-Daten anzeigen |
| `ble status` | BLE-Status (verbunden, RSSI, Geraet) |
| `ble scan` | BLE-Scan starten |
| `sd info` | SD-Karten-Info (Kapazitaet, frei) |
| `sd ls` | Dateien auf der SD-Karte auflisten |
| `sd log` | Letzte Log-Eintraege anzeigen |
| `adc` | ADC-Rohwerte aller Kanaele |
| `i2c scan` | I2C-Bus-Scan (Adressen) |
| `calibration` | Aktuelle Kalibrierung anzeigen |
| `calibration set rpm <mult>` | RPM-Multiplikator setzen |
| `calibration set fuel <adc> <liters>` | Tank-Stuetzstelle setzen |
| `calibration set temp <offset>` | Temperatur-Offset setzen |
| `config save` | Konfiguration auf SD-Karte speichern |
| `config load` | Konfiguration von SD-Karte laden |
| `config reset` | Werkseinstellungen wiederherstellen |
| `reboot` | Bordcomputer neu starten |
| `factory` | Werksreset (alle Daten loeschen) |
| `controller status` | Controller-Status (verbunden, Variante, Batterie) |
| `controller pair` | BLE-Controller-Pairing starten |
| `controller mapping` | Aktuelle Button-Mapping-Tabelle anzeigen |
| `controller mapping set <btn> <action>` | Button-Mapping aendern |
| `carplay status` | CarPlay-Display-Status (verbunden, Telemetrie) |
| `carplay pair` | CarPlay-Display-Pairing (Telemetrie-Spiegel) starten |

**Diagnose-Beispiel: DTC TBC-0601 (Kein RPM-Signal)**

```
> dtc
Active DTCs:
  TBC-0601: No RPM signal on GPIO15 (PCNT0)
  TBC-0502: SD card nearly full (95% used)

> sensors
--- Sensor Values ---
  RPM:           0 U/min     [WARN: no signal]
  Speed:         0 km/h
  Coolant:       24.3 °C
  Batt:          12.4 V
  Fuel:          45 %
  Oil:           0.0 bar     [no sensor]
  GPS Fix:       No fix
  GPS Sats:      0
  IMU Accel:     X=0.01 Y=-0.02 Z=9.81
  Baro:          1013.2 hPa
  Altitude:      0 m

> adc
ADC1 Channel Values (raw, 12-bit):
  CH0 (GPIO1, Batt):     1482  -> 12.4V
  CH1 (GPIO2, Coolant):  1364  -> 24.3°C
  CH3 (GPIO4, Fuel):     1023  -> 45%
  CH4 (GPIO5, Oil):      0     -> 0.0 bar [no sensor]

> i2c scan
I2C Bus 0 Scan:
  0x68: MPU6050/BMI270 [OK]
  0x76: BMP280/BMP390 [OK]

> gps status
GPS Status: NO FIX
  Satellites in view: 0
  HDOP: 99.9
  Last fix: never
```

In diesem Beispiel ist das RPM-Signal nicht verbunden (Optokoppler-
Kabel pruefen) und die SD-Karte ist fast voll (Daten loeschen oder
SD-Karte austauschen).

---

## Quellen

Dieses Dokument basiert auf den folgenden Projektdokumenten:

- `docs/HARDWARE.md` -- Hardware-Schaltplan und Pin-Layout
- `docs/PINMAP.md` -- ESP32-S3 GPIO Pin-Belegung
- `docs/CASE-DESIGN.md` -- Gehaeuse-Design (3D-Druck, Montage)
- `docs/PCB-LAYOUT.md` -- PCB-Design-Konzept
- `docs/BOM.md` -- Bill of Materials (inkl. CarPlay-Display und Lenker-Controller, Abschnitt 1.11)
- `docs/ARCHITECTURE.md` -- System-Architektur
- `docs/PRODUCT-CATALOG.md` -- Produktkatalog mit 5 Produktvarianten
- `docs/SHOPPING-LIST.md` -- Einkaufsliste mit AliExpress-Links fuer CarPlay-Displays
- `docs/USER-MANUAL.md` -- Benutzerhandbuch und Installationsanleitung
- `docs/research/04-pd06-technische-daten.md` -- PD06 technische Daten
- `docs/research/02-hardware-deep-dive.md` -- Hardware-Details

Technische Daten des Fahrzeugs: Honda Service Manual (P/N 61MM900),
Honda Owner's Manual XL600V Transalp.

# Benutzerhandbuch & Installationsanleitung
## Transalp Bordcomputer für die Honda XL600V Transalp 600 (PD06)

> Stand: Oktober 2026
> Fahrzeug: Honda XL600V Transalp 600, Modellcode PD06, Baujahre 1986–2000
> Plattform: ESP32-S3-WROOM-1, ESP-IDF, FreeRTOS
> Dieses Dokument richtet sich sowohl an nicht-technische Motorradfahrer als auch an Bastler, die das System selbst installieren und konfigurieren möchten.

---

## Inhaltsverzeichnis

**Teil 1: Benutzerhandbuch**

1. Einführung
2. Display-Erklärung
3. Bedienung
4. Features im Detail
5. Companion-App
6. Wartung & Pflege

**Teil 2: Installationsanleitung**

7. Voraussetzungen
8. Mechanische Montage
9. Elektrische Installation
10. PD06-spezifische Hinweise
11. Inbetriebnahme
12. Fehlersuche

---

# TEIL 1: BENUTZERHANDBUCH

---

## 1. Einführung

### 1.1 Was ist der Bordcomputer?

Der Transalp Bordcomputer ist ein elektronisches Informations- und Diagnosesystem, das am Lenker deiner Honda Transalp 600 (PD06) montiert wird. Die PD06 ist ein rein mechanisches Motorrad aus den Baujahren 1986 bis 2000. Sie hat keine elektronische Motorsteuerung (ECU), keinen CAN-Bus und keine standardisierte Diagnoseschnittstelle. Alles, was der Bordcomputer an Daten anzeigt, wird über eigenständige Sensoren erfasst, die am Motorrad nachgerüstet werden.

Der Bordcomputer besteht aus einem wetterfesten Gehäuse (3D-gedruckt, ASA-Kunststoff), das eine Elektronikplatine mit einem Microcontroller (ESP32-S3), einem GPS-Modul, Sensoren für Beschleunigung und Luftdruck, einem E-Paper-Display, einer SD-Karte zur Tour-Aufzeichnung und einer Bluetooth-Schnittstelle für die Verbindung zu einem Smartphone enthält.

### 1.2 Was kann der Bordcomputer? (Feature-Übersicht)

| Feature | Beschreibung |
|---|---|
| Geschwindigkeitsanzeige | GPS-basiert, ±1–2 km/h Genauigkeit, alternativ Hall-Sensor am Vorderrad |
| Drehzahlmesser (RPM) | Über Pickup-Signal der Zündung, 1 Puls pro Kurbelwellenumdrehung |
| Shift-Light | Visueller Drehzahl-Warnton mit einstellbaren Schwellwerten |
| Ganganzeige | Berechnet aus Verhältnis Geschwindigkeit zu Drehzahl (ratio-basiert) |
| Kühlmitteltemperatur | Inlinesensor im oberen Kühlschlauch, Anzeige in °C |
| Motoröltemperatur | Optional, über Sensor an der Ölablassschraube |
| Umgebungstemperatur | Über internen Sensor (I2C) im Gehäuse |
| Kraftstoffstand | Nachgerüsteter Tankgeber (Widerstandssensor), Reichweitenberechnung |
| Batteriespannung | Direkte Messung, Über-/Unterspannungswarnung |
| GPS-Position | u-blox NEO-M9N, 25 Hz Update-Rate, 1PPS-Zeitsynchronisation |
| Navigation (Turn-by-Turn) | Navigationshinweise vom Smartphone über die Companion-App per BLE |
| Tour-Aufzeichnung | GPX-Datei auf MicroSD-Karte, kompatibel mit gängigen Touren-Plattformen |
| Diagnose (DTC) | Eigenes Fehlercode-System, Self-Test beim Boot, aktive Fehlercodes |
| Notfall-Funktion | Positionsübertragung an Notkontakt per Smartphone-App |
| OTA-Update | Firmware-Updates über die Companion-App per Bluetooth |
| Neigungserkennung | IMU-Sensor (BMI270) erkennt Sturz und Aufstellwinkel |

### 1.3 Für wen ist der Bordcomputer gedacht?

Der Bordcomputer richtet sich an zwei Gruppen:

**Alltagsfahrer und Reisende:** Du fährst regelmäßig mit deiner Transalp 600 und möchtest mehr Informationen als der serienmäßige Tacho bietet. Du schätzt eine verlässliche Geschwindigkeitsanzeige, eine Drehzahlanzeige mit Shift-Light, eine Temperaturanzeige und die Möglichkeit, Touren aufzuzeichnen und Navigationshinweise zu erhalten, ohne ein separates Navigationsgerät am Lenker zu haben.

**Bastler und Selbsteinbauer:** Du hast Grundkenntnisse in Elektrik und Mechanik und möchtest das System selbst installieren. Dieses Handbuch enthält alle Informationen, die du für den Einbau brauchst: Sensor-Anzapfpunkte am PD06-Motor, Kabelbaum-Farbcodes, Werkzeug-Listen und Schritt-für-Schritt-Anleitungen. Wenn du löten und Kabelbinder handhaben kannst, ist die Installation machbar.

---

## 2. Display-Erklärung

### 2.1 Hauptdisplay: E-Paper 2,9"

Das Hauptdisplay ist ein E-Paper-Display (elektronisches Papier) mit 2,9 Zoll Diagonale und einer Auflösung von 296 × 128 Pixeln (monochrom). E-Paper wurde gewählt, weil:

- Die Lesbarkeit bei direkter Sonneneinstrahlung exzellent ist (das Display reflektiert Umgebungslicht, es braucht keine Hintergrundbeleuchtung).
- Der Stromverbrauch minimal ist (nahezu 0 mW zwischen Aktualisierungen, ca. 26 mW beim Refresh).
- Der Leseinkel über 170 Grad beträgt.

**Was wird auf dem Hauptdisplay angezeigt?**

Das Display ist in verschiedene Bereiche aufgeteilt:

**Oberer Bereich — Drehzahlbalken:**
Ein horizontaler Balken am oberen Bildschirmrand, der sich von links nach rechts füllt. Die Farbe des Balkens wechselt entlang eines Farbverlaufs (grün → gelb → orange → rot). Bei ca. 85% der Breite liegt die Shift-Light-Marke (gelb). Bei 95% beginnt die rote Zone: der Balken pulsiert rot, und eine blinkende Warnlinie erschent über dem Balken. Die genauen Drehzahl-Schwellwerte sind konfigurierbar (Standard: Shift-Light bei 7.000 U/min, rote Zone bei 8.500 U/min).

**Mittelbereich — Geschwindigkeit:**
Groß dargestellte Geschwindigkeit in km/h, zentriert. Darunter das Einheiten-Label.

**Unterer Bereich — Sekundärwerte:**
In mehreren Spalten werden folgende Werte angezeigt:
- Gang (1–5 oder N für Leerlauf)
- Kühlmitteltemperatur (°C)
- Kraftstoffstand (Balken oder Prozent)
- Batteriespannung (V)
- GPS-Status (Fix/kein Fix, Satellitenanzahl)

### 2.2 Sekundärdisplay: TFT (optional)

Ein optionales TFT-Display (1,14 bis 2,0 Zoll, SPI) kann ergänzend für zeitkritische Anzeigen verwendet werden, die schnellere Aktualisierungen benötigen als E-Paper liefert (z.B. ein Drehzahl-Balkendiagramm mit schnellen Animationen oder eine Echtzeit-Neigungsanzeige). Das TFT-Display wird unterhalb oder neben dem E-Paper positioniert, im Schatten des Gehäusedeckels, da TFT-Displays bei direkter Sonneneinstrahlung schlecht lesbar sind.

### 2.3 Status-LEDs

Der Bordcomputer kann über Status-LEDs verfügen, die über dem Display oder an der Gehäuseoberseite angeordnet sind. Die folgenden LEDs sind vorgesehen:

| LED | Farbe | Bedeutung |
|---|---|---|
| Power | Grün | Leuchtet, wenn der Bordcomputer mit Strom versorgt wird |
| GPS | Blau | Blinkt beim Suchen von Satelliten, leuchtet durchgehend bei GPS-Fix |
| BLE | Weiß | Blinkt beim Pairing-Modus, leuchtet bei aktiver Smartphone-Verbindung |
| Warnung | Rot | Leuchtet bei aktivem Fehlercode (DTC) oder kritischer Warnung (Öldruck, Überhitzung) |
| Shift-Light | Gelb/Rot | Pulsierend bei Erreichen der eingestellten Shift-Light-Drehzahl |

### 2.4 Lautsprecher / Sounder

Der Bordcomputer hat einen kleinen Piezo-Sounder (Buzzer) für akustische Signale:

| Signal | Bedeutung |
|---|---|
| Kurzer Piep (einmal) | Bestätigung einer Tasteneingabe |
| Doppelter Piep | Shift-Light erreicht (begleitet die visuelle Warnung) |
| Langer, durchgehender Ton | Kritische Warnung (Öldruck, Überhitzung, niedriger Kraftstoffstand) |
| Dreifacher Piep beim Booten | Self-Test abgeschlossen, alle Systeme OK |
| Aufsteigende Tonfolge | Firmware-Update erfolgreich abgeschlossen |
| Absteigende Tonfolge | Firmware-Update fehlgeschlagen oder Fehler im Self-Test |

---

## 3. Bedienung

### 3.1 Tasten und ihre Funktionen

Der Bordcomputer hat eine Taste an der Gehäuseoberseite, ausgeführt als Silikonmembran-Taster (wasserdicht durch die Gehäusewand). Die Taste hat folgende Funktionen, je nach Kontext:

| Aktion | Funktion (Hauptansicht) | Funktion (Menü) |
|---|---|---|
| Kurz drücken (<0,5s) | Display aktualisieren / Display-Modus umschalten | Nächster Menüpunkt |
| Lang drücken (>2s) | Menü öffnen / Menü verlassen | Bestätigen / Speichern |
| Doppeldruck | Tour-Aufzeichnung starten/stoppen | Zurück |

Zusätzliche Tasten können über freie GPIO-Pins angeschlossen werden (Touch-Buttons oder physische Taster), je nach Konfiguration.

### 3.2 Menü-Navigation

Das Menü ist auf dem E-Paper-Display als einfache Listenstruktur aufgebaut. Du navigierst mit der Taste durch die Menüpunkte und bestätigst mit Langdruck. Die Menüstruktur sieht wie folgt aus:

```
MENÜ
├── Anzeige
│   ├── Helligkeit (TFT-Backlight, wenn vorhanden)
│   ├── Display-Modus (Vollständig / Kompakt / Nav-Fokus)
│   └── Auto-Off (Display nach X Sekunden ausschalten)
├── Einheiten
│   ├── Geschwindigkeit (km/h / mph)
│   ├── Temperatur (°C / °F)
│   └── Druck (bar / psi)
├── Alarme
│   ├── Shift-Light Drehzahl (U/min)
│   ├── Rote Zone Drehzahl (U/min)
│   ├── Temperatur-Warnung (°C)
│   ├── Niedrige Spannung (V)
│   ├── Hohe Spannung (V)
│   └── Kraftstoff-Warnung (%)
├── Tour
│   ├── Aufzeichnung starten/stoppen
│   ├── Letzte Tour anzeigen
│   └── GPX exportieren (über App oder SD-Karte)
├── Diagnose
│   ├── Self-Test ausführen
│   ├── Aktive Fehlercodes (DTC)
│   ├── Sensor-Übersicht (Live-Werte)
│   └── System-Logs
├── Konfiguration
│   ├── Radumfang (mm, für Hall-Sensor)
│   ├── Reifengröße
│   ├── Übersetzung (für Ganganzeige)
│   └── Werkseinstellungen
└── Info
    ├── Firmware-Version
    ├── Hardware-Version
    └── Betriebsstunden
```

### 3.3 Einstellungen ändern

**Helligkeit einstellen:**
Falls ein TFT-Display mit Backlight vorhanden ist, kann die Helligkeit im Menü unter Anzeige → Helligkeit eingestellt werden. Das E-Paper-Display hat keine Hintergrundbeleuchtung und ist immer bei guter Beleuchtung lesbar. Bei schlechten Lichtverhältnissen (Nachtfahrt) ist das E-Paper ohne Backlight nur schwer zu lesen. In diesem Fall kann die Auto-Off-Funktion aktiviert werden, damit das Display nur bei Tastendruck aktualisiert wird.

**Einheiten ändern:**
Unter Einheiten kannst du zwischen km/h und mph, °C und °F sowie bar und psi umschalten. Die Standardeinstellung ist km/h, °C und bar.

**Alarme einstellen:**
Alle Warnschwellen sind konfigurierbar:
- Shift-Light: Standard 7.000 U/min, einstellbar 3.000–9.000 U/min
- Rote Zone: Standard 8.500 U/min
- Temperatur-Warnung: Standard 110°C (Kühlmittel)
- Niedrige Spannung: Standard 11,5V
- Hohe Spannung: Standard 15,5V
- Kraftstoff-Warnung: Standard 20% (Restreichweite)

### 3.4 Quick-Access Funktionen

| Aktion | Funktion |
|---|---|
| Doppeldruck der Taste | Tour-Aufzeichnung starten oder stoppen |
| Taste 5s gedrückt halten | Notfall-Funktion auslösen (Positionsübermittlung an Notkontakt) |
| Taste beim Booten gedrückt halten | Service-Modus (erweiterte Diagnose, nur für Werkstatt) |

---

## 4. Features im Detail

### 4.1 Geschwindigkeitsanzeige (GPS vs. Rad)

Der Bordcomputer verwendet primär GPS für die Geschwindigkeitsanzeige. Das GPS-Modul (NEO-M9N) liefert bis zu 25 Updates pro Sekunde, was für Motorrad-Geschwindigkeiten mehr als ausreichend ist. Die Genauigkeit liegt bei ±1–2 km/h.

**Vorteile der GPS-geschwindigkeit:**
- Keine mechanische Verbindung zum Rad nötig
- Unabhängig von Reifengröße und Reifenverschleiß
- Keine Kalibrierung nötig

**Nachteile:**
- In Tunneln und Tiefgaragen kein GPS-Empfang
- Bei sehr schlechtem Empfang (dichte Bewaldung, enge Schluchten) kann die Anzeige kurzzeitig ausfallen

**Fallback: Hall-Sensor am Vorderrad:**
Wenn GPS nicht verfügbar ist, kann ein Hall-Sensor am Vorderrad als Fallback dienen. Ein Magnet wird auf der Felge montiert, der Sensor an der Gabelbrücke. Bei jeder Radumdrehung erzeugt der Sensor einen Puls. Die Geschwindigkeit wird aus der Pulsfrequenz und dem konfigurierten Radumfang berechnet. Für die Transalp 600 mit der Bereifung 130/80-17 (hinten) bzw. 90/90-21 (vorne) beträgt der Vorderrad-Umfang 2.185 mm. Bei 100 km/h dreht das Vorderrad mit ca. 763 U/min (12,7 Hz).

Wenn die Transalp noch die serienmäßige mechanische Tachowelle hat, kann alternatively ein elektronischer Tacho-Adapter auf der Tachowelle montiert werden.

### 4.2 Drehzahl-Monitor mit Shift-Light

Die Drehzahl wird über das Pickup-Signal der Zündung erfasst. Die PD06 hat eine CDI-Zündung (Capacitive Discharge Ignition). Der Pulsgenerator liefert 1 Puls pro Kurbelwellenumdrehung. Die Frequenz entspricht der Motordrehzahl geteilt durch 60 (1 Puls/Umdrehung → f = n/60). Bei 8.000 U/min ergibt das ca. 133 Pulse pro Sekunde, was mit dem ESP32-Interrupt problemlos auswertbar ist.

Das Signal wird über einen Optokoppler galvanisch vom Bordcomputer-Netz getrennt, um die empfindliche Elektronik vor den Hochspannungspulsen der Zündung zu schützen.

**Shift-Light:**
Der Drehzahl-Balken auf dem Display wechselt seine Farbe und Füllstand entsprechend der Drehzahl. Bei Erreichen der eingestellten Shift-Light-Drehzahl (Standard 7.000 U/min) pulsiert der Balken gelb und ein akustisches Signal ertönt. Bei der roten Zone (Standard 8.500 U/min) pulsiert der Balken rot mit einer blinkenden Warnlinie darüber.

Die Transalp 600 hat keinen elektronischen Drehzahlbegrenzer im modernen Sinne. Der rote Bereich beginnt bei ca. 9.000 U/min, die Nennleistung wird bei 8.000 U/min erreicht. Der Bordcomputer hilft dir, im optimalen Drehzahlbereich zu fahren und rechtzeitig zu schalten.

### 4.3 Ganganzeige

Die Ganganzeige wird berechnet aus dem Verhältnis von Geschwindigkeit zu Drehzahl (ratio-basiert). Der Bordcomputer vergleicht die aktuelle Ratio (v/n) mit der bekannten Übersetzungstabelle der Transalp 600:

| Gang | Übersetzung (frühe Modelle, 1987–1995) | Übersetzung (späte Modelle, 1996–1999) |
|---|---|---|
| 1 | 2,571 | 2,500 |
| 2 | 1,777 | 1,722 |
| 3 | 1,380 | 1,333 |
| 4 | 1,125 | 1,111 |
| 5 | 0,961 | 0,961 |

Diese Methode funktioniert ohne zusätzliche Hardware, hat aber eine Einschränkung: Bei gezogener Kupplung (Drehzahl steigt, Geschwindigkeit bleibt gleich) wird der Gang falsch berechnet. Der Bordcomputer zeigt in diesem Fall kurzzeitig den falschen Gang an, bis die Kupplung wieder losgelassen wird.

Im Leerlauf zeigt der Bordcomputer "N" an (erkannt über den Leerlaufschalter am Schaltmechanismus, Kabel hellgrün).

### 4.4 Temperaturen (Motoröl, Umgebung, Kühlmittel)

**Kühlmitteltemperatur:**
Die PD06 hat serienmäßig keine elektronische Kühlmitteltemperaturanzeige, nur einen Thermostat und bei einigen Modellen eine Warnlampe. Für den Bordcomputer wird ein Inlinesensor in den oberen Kühlschlauch (vom Motor zum Kühler) eingesetzt. Der Thermostat öffnet bei 80–84°C und ist bei 95°C vollständig geöffnet. Der Kühlerlüfter schaltet bei 98–102°C ein und bei 93–97°C wieder aus.

Die Temperaturwarnung wird bei 110°C (Standard, konfigurierbar) ausgelöst. Bei Kühlmitteltemperaturen über 120°C besteht Überhitzungsgefahr — sofort anhalten und Motor abstellen.

**Motoröltemperatur (optional):**
Ein optionaler Temperatursensor kann an der Ölablassschraube montiert werden (Gewinde M14 × 1,5 beim PD06, Adapter nötig). Die Öltemperatur ist normalerweise 10–20°C höher als die Kühlmitteltemperatur. Bei Öltemperaturen über 130°C besteht Gefahr für den Motor.

**Umgebungstemperatur:**
Ein interner Sensor (I2C, BMP388) im Gehäuse misst die Umgebungstemperatur. Da der Sensor im Gehäuse sitzt, kann der Wert durch Eigenerwärmung der Elektronik um einige Grad höher liegen als die tatsächliche Außentemperatur. Die Anzeige ist daher als Richtwert zu verstehen, nicht als präzise Außen Temperatur.

### 4.5 Kraftstoffstand

Die Transalp 600 hat einen 18-Liter-Tank mit 2,9–3,5 Litern Reserve. Die serienmäßige Kraftstoffanzeige funktioniert über eine Vakuum-Petcock-Membran mit Reserve-Stellung und eine Warnleuchte — es gibt keinen elektronischen Tankgeber.

Für den Bordcomputer muss ein Aftermarket-Kraftstoffstandssensor nachgerüstet werden. Ein Schwimmer-Sensor (Widerstandsbasiert, z.B. 0–190 Ohm oder 240–33 Ohm) wird durch die Tanköffnung oder eine separate Bohrung im Tankdeckel installiert. Die Kennlinie ist nicht linear (oben flach, unten steil) und muss empirisch kalibriert werden.

**Reichweitenberechnung:**
Der Bordcomputer berechnet die Reichweite aus dem aktuellen Füllstand und dem durchschnittlichen Verbrauch. Der Verbrauch wird aus der Kraftstoffstand-Änderung und der zurückgelegten Strecke ermittelt (gleitender Durchschnitt über die letzten 50 km). Bei der Transalp 600 liegt der durchschnittliche Verbrauch bei ca. 4,5–5,5 L/100 km, die praktische Reichweite bei ca. 300–350 km.

Die Kraftstoff-Warnung wird bei 20% Restfüllung (Standard, konfigurierbar) ausgelöst.

### 4.6 Navigation (Turn-by-Turn vom Smartphone)

Der Bordcomputer selbst hat keine eingebaute Karten-Engine. Die Navigation funktioniert über die Companion-App auf deinem Smartphone: Die App berechnet die Route und sendet Navigationshinweise (Richtungspfeile, Distanz, Straßenname) per Bluetooth an den Bordcomputer. Das E-Paper-Display zeigt dann:

- Einen großen Richtungspfeil (60 × 60 Pixel)
- Die Distanz zum nächsten Manöver (z.B. "300 m")
- Den Straßennamen (z.B. "B85 rechts abbiegen")

Die Navigationshinweise werden alle 1–2 Sekunden aktualisiert, sobald ein neues Manöver ansteht. Die Richtungspfeile werden auch auf dem Split-Screen-Modus (Geschwindigkeit links, Navigation rechts) angezeigt.

Wenn die Bluetooth-Verbindung abbricht (Tunnel, Reichweite), speichert der Bordcomputer die letzte bekannte Anweisung und zeigt sie weiterhin an. Bei erneuter Verbindung wird die Navigation automatisch fortgesetzt.

### 4.7 Tour-Aufzeichnung (GPX)

Der Bordcomputer zeichnet deine gefahrenen Touren als GPX-Datei auf einer MicroSD-Karte auf. GPX ist ein XML-basiertes Standardformat für GPS-Tracks und ist kompatibel mit gängigen Touren-Plattformen.

**Wie funktioniert es:**
- Der Bordcomputer schreibt alle 1–5 Sekunden einen Trackpunkt auf die SD-Karte (Datum, Uhrzeit, GPS-Position, Höhe).
- Die Datei wird unter `/tracks/YYYYMMDD_HHMMSS.gpx` gespeichert (z.B. `20261013_143022.gpx`).
- Die Aufzeichnung beginnt automatisch beim GPS-Fix und endet beim Ausschalten.
- Alternativ kann die Aufzeichnung manuell gestartet und gestoppt werden (Doppeldruck der Taste).

**GPX-Struktur (Beispiel):**

```xml
<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="Transalp-Bordcomputer"
     xmlns="http://www.topografix.com/GPX/1/1">
  <trk>
    <name>Tour 2026-10-13</name>
    <trkseg>
      <trkpt lat="50.9795" lon="11.3265">
        <ele>325.0</ele>
        <time>2026-10-13T14:30:22Z</time>
      </trkpt>
      <trkpt lat="50.9796" lon="11.3270">
        <ele>326.0</ele>
        <time>2026-10-13T14:30:23Z</time>
      </trkpt>
    </trkseg>
  </trk>
</gpx>
```

**Tour-Export:**
Du kannst die GPX-Dateien auf zwei Wegen exportieren:
1. Per Companion-App über Bluetooth (Track-Export-Service, dauert bei einer 24-Stunden-Tour ca. 7 Minuten bei 20 kB/s).
2. Direkt von der SD-Karte: Bordcomputer ausschalten, SD-Karte entnehmen und am Computer auslesen.

Die GPX-Dateien sind kompatibel mit gängigen Touren-Plattformen und GPX-Editoren.

### 4.8 Diagnose (DTC-Anzeige)

Der Bordcomputer hat ein eigenes Diagnose-System mit Fehlercodes (DTC, Diagnostic Trouble Codes), das an das Projekt angepasst ist. Da die PD06 keine OBD-II-Schnittstelle hat, wurde ein eigenes Fehlercode-Format entwickelt, das von den Standard-OBD-II-Codes inspiriert ist.

**Wie funktionieren die Fehlercodes:**
Jeder Fehlercode hat das Format `TBC-XXYY`, wobei XX die Kategorie und YY der spezifische Fehler ist:

| Kategorie | Bereich | Beispiele |
|---|---|---|
| Sensor | TBC-01xx | TBC-0101: Kühlmitteltemperatur-Sensor unplausibel |
| GPS | TBC-02xx | TBC-0201: Kein GPS-Fix nach 120 Sekunden |
| Versorgung | TBC-03xx | TBC-0301: Batteriespannung unter 11,5V |
| BLE | TBC-04xx | TBC-0401: BLE-Verbindung verloren |
| SD-Karte | TBC-05xx | TBC-0501: SD-Karte nicht erkannt |
| Display | TBC-06xx | TBC-0601: E-Paper-Refresh fehlgeschlagen |

**Self-Test:**
Beim Booten führt der Bordcomputer automatisch einen Self-Test durch. Die Abfolge:

1. Display-Test (alle Pixel an, dann aus)
2. GPS-Modul initialisieren (Antwort prüfen)
3. I2C-Sensoren ansprechen (IMU, Barometer)
3. SD-Karte mounten
4. ADC-Kanäle lesen (Batteriespannung, Kraftstoffstand)
5. Optokoppler-Eingänge prüfen (RPM-Signal)
6. BLE-Modul initialisieren
7. Konfiguration laden

Bei erfolgreichem Self-Test ertönt ein dreifacher Piepton und die Status-LED leuchtet grün. Bei einem Fehler ertönt eine absteigende Tonfolge, die Status-LED leuchtet rot, und der entsprechende DTC wird angezeigt.

Der Self-Test kann auch manuell im Menü unter Diagnose → Self-Test ausführen gestartet werden.

### 4.9 Notfall-Funktion

Der Bordcomputer hat eine Notfall-Funktion, die bei einem Unfall oder in Notsituationen automatisch oder manuell ausgelöst werden kann.

**Automatische Auslösung (Crash-Erkennung):**
Der IMU-Sensor (BMI270) im Bordcomputer überwacht kontinuierlich die Beschleunigung. Bei einem plötzlichen, starken Stoß (Crash-Schwelle: konfigurierbar, Standard ca. 4g über 50 ms) wird ein Crash-Ereignis ausgelöst. Der Bordcomputer sendet dann über die Companion-App eine Notfall-Nachricht an den hinterlegten Notfallkontakt, die folgende Informationen enthält:
- GPS-Position (Breiten- und Längengrad)
- Zeitstempel
- Letzte bekannte Geschwindigkeit und Fahrtrichtung

**Manuelle Auslösung:**
Durch 5 Sekunden gedrückt halten der Taste wird die Notfall-Funktion manuell ausgelöst. Es ertönt ein Bestätigungston und die Nachricht wird gesendet.

**Voraussetzung:**
Die Notfall-Funktion funktioniert nur, wenn:
- Die Companion-App auf dem Smartphone aktiv ist
- Eine Bluetooth-Verbindung zum Bordcomputer besteht
- Das Smartphone Mobilfunkempfang hat
- Ein Notfallkontakt in der App hinterlegt ist

---

## 5. Companion-App

### 5.1 App installieren

Die Companion-App ist für beide gängigen mobilen Betriebssysteme verfügbar. Sie wird über den jeweiligen App-Store auf deinem Smartphone installiert. Suche nach "Transalp Bordcomputer" im App-Store deines Betriebssystems.

**Systemanforderungen:**
- Betriebssystem in einer aktuellen Version
- Bluetooth Low Energy (BLE) Unterstützung
- GPS (für die Navigationsfunktion)
- Internetverbindung (für Karten, Wetter, Notfall-Funktion)

### 5.2 Verbinden mit Bordcomputer (BLE)

Die Verbindung zwischen Bordcomputer und Smartphone erfolgt über Bluetooth Low Energy (BLE). Der Bordcomputer fungiert als GATT-Server, die App als GATT-Client.

**Erstes Pairing:**
1. Schalte den Bordcomputer ein (Zündung an).
2. Öffne die Companion-App auf deinem Smartphone.
3. Tippe auf "Gerät koppeln" in der App.
4. Die App sucht nach dem Bordcomputer (BLE-Scan).
5. Wähle den Bordcomputer aus der Liste (Name: "Transalp-BC-XXXX").
6. Bestätige das Pairing auf beiden Geräten (Just-Works oder Passkey, je nach Konfiguration).
7. Die Pairing-Keys werden auf beiden Seiten gespeichert. Beim nächsten Start verbinden sich die Geräte automatisch.

**Auto-Reconnect:**
Die App versucht automatisch, die Verbindung wiederherzustellen, wenn sie abbricht (Tunnel, Reichweite). Der Reconnect erfolgt mit exponentiellem Backoff (1s, 2s, 4s, 8s, max. 30s). Du musst nichts manuell tun.

**BLE-Reichweite auf dem Motorrad:**
Die BLE-Reichweite auf dem Motorrad beträgt typischerweise 5–15 Meter. Das Smartphone sollte sich in der Brust- oder Jackentasche befinden, nicht im Rucksack weit vom Bordcomputer entfernt. Die BLE-Antenne des ESP32-S3 funktioniert durch das ASA-Kunststoffgehäuse hindurch problemlos. Wichtig: Im Bereich der Antenne darf keine metallische Abschirmung (Kupferfolie) auf der Innenseite des Gehäuses angebracht sein (mindestens 30 × 30 mm freie Fläche).

**Einschränkung:**
Der Bordcomputer kann nur ein Smartphone gleichzeitig verbinden. Wenn du das Telefon wechseln möchtest, musst du die aktuelle Verbindung trennen (Bluetooth am Bordcomputer aus- und einschalten über die App, oder App beenden).

### 5.3 Welche Daten werden angezeigt?

Die App hat folgende Hauptbildschirme:

**Dashboard:**
Zeigt die gleichen Live-Daten wie das Bordcomputer-Display, plus erweiterte Informationen:
- Geschwindigkeit, Drehzahl, Gang (Live)
- Kühlmitteltemperatur, Öltemperatur, Umgebungstemperatur
- Kraftstoffstand und berechnete Reichweite
- Batteriespannung
- GPS-Position, Höhe, Satellitenanzahl, Fix-Qualität
- Neigungswinkel (Roll/Pitch) aus der IMU
- Fahrzeit, Tageskilometer

**Navigation:**
Kartendarstellung mit aktueller Position und geplanter Route. Navigationshinweise werden an den Bordcomputer gesendet. Die App nutzt die native Karten-Implementierung des jeweiligen Betriebssystems.

**Touren:**
Liste aller aufgezeichneten Touren mit Statistiken (Strecke, Dauer, Durchschnittsgeschwindigkeit, Höhenprofil). Touren können als GPX exportiert und geteilt werden.

**Diagnose:**
- Sensor-Übersicht mit Live-Werten
- Aktive Fehlercodes (DTC)
- System-Logs (herunterladbar)
- Self-Test aus der App starten
- Konfigurations-Snapshot laden

**Einstellungen:**
- Alle Bordcomputer-Einstellungen (Einheiten, Alarme, Shift-Light)
- Sensor-Kalibrierung
- Display-Einstellungen
- BLE-Konfiguration

**Firmware-Update (OTA):**
- Firmware-Image hochladen
- Update-Fortschritt verfolgen
- Verifikation und Commit

### 5.4 Tour-Export und -Import

**Export:**
In der App unter "Touren" kannst du eine aufgezeichnete Tour auswählen und exportieren. Der Bordcomputer sendet die GPX-Datei per BLE (Track-Export-Service). Bei einer 24-Stunden-Tour mit 1 Hz GPS-Updates (ca. 86.400 Punkte, ca. 8,6 MB) dauert der Export ca. 7 Minuten bei 20 kB/s. Alternativ kann die SD-Karte entnommen und direkt am Computer ausgelesen werden.

**Import:**
Du kannst GPX-Routen in die App importieren und als Navigationsroute an den Bordcomputer senden. Die App berechnet dann die Turn-by-Turn-Hinweise und überträgt sie.

### 5.5 Konfiguration über die App

Alle Einstellungen, die am Bordcomputer-Display über die Taste vorgenommen werden können, lassen sich auch bequem über die App konfigurieren. Die App bietet zusätzlich:

- Sensor-Kalibrierung (Radumfang, Kraftstoffstand-Kennlinie, Temperatur-Offset)
- Firmware-Update per OTA
- Log-Download für Fehleranalyse
- Konfiguration exportieren/importieren (als JSON)
- Notfallkontakt hinterlegen
- Wetter-Integration (alle 15 Minuten aktualisiert)

---

## 6. Wartung & Pflege

### 6.1 Reinigung des Displays

Das E-Paper-Display ist hinter einer 2 mm dicken Acrylglas- oder Polycarbonat-Abdeckung montiert. Die Abdeckung hat eine hydrophobe (wasserabweisende) Beschichtung, damit Tropfen abperlen.

**Reinigung:**
- Mit weichem Mikrofasertuch und klarem Wasser abwischen.
- Bei stärkerer Verschmutzung milde Seifenlauge verwenden.
- Keine scheuernden Reinigungsmittel, keine Lösungsmittel (Aceton, Verdünner).
- Keinen Hochdruckreiniger direkt auf das Display richten (die hydrophobe Beschichtung kann beschädigt werden).

### 6.2 Software-Updates (OTA)

Firmware-Updates werden über die Companion-App per Bluetooth eingespielt (Over-The-Air, OTA).

**Update-Prozess:**
1. App öffnen und mit Bordcomputer verbinden.
2. Unter "Firmware-Update" nach neuer Firmware suchen.
3. Wenn ein Update verfügbar ist, "Jetzt aktualisieren" tippen.
4. Die App überträgt das Firmware-Image per BLE an den Bordcomputer.
5. Der Bordcomputer verifiziert das Image (SHA-256-Hash-Prüfung).
6. Bei erfolgreicher Verifikation: Commit bestätigen, Bordcomputer startet neu.
7. Der Bordcomputer führt den Self-Test durch. Bei Erfolg ertönt die aufsteigende Tonfolge.

**Dauer:**
Bei einem 300 kB großen Firmware-Image und 20 kB/s Übertragungsrate dauert das Update ca. 15 Sekunden für die Übertragung, plus Verifikation und Neustart.

**Wichtig:**
- Den Motor während des Updates nicht starten (Spannungsspitzen durch den Anlasser können den Update-Prozess stören).
- Die Bluetooth-Verbindung während des Updates nicht trennen.
- Bei Fehlschlagen des Updates (Hash-Mismatch, Verbindungsabbruch): Der Bordcomputer behält die alte Firmware und startet normal neu. Es entsteht kein "Brick" (unkompatibler Zustand).

### 6.3 Akku / Spannungsversorgung prüfen

Der Bordcomputer hat keinen eigenen Akku im herkömmlichen Sinn, sondern wird über das 12V-Bordnetz des Motorrads versorgt. Ein kleiner LiPo-Akku (1000–2000 mAh) im Gehäuse dient als Puffer für kurze Spannungsausfälle (z.B. beim Anlassen, wenn die Bordspannung auf 8–10 V einbricht).

**Batteriespannung kontrollieren:**
Die Bordspannung wird permanent überwacht und auf dem Display angezeigt. Normwerte:
- Leerlauf: 12–13 V
- Fahrt (3.000 U/min): 13,5–14,2 V
- Hohe Drehzahl (>6.000 U/min): 14,2–15 V
- Bei Werten unter 11,5 V: Warnung (Batterie schwach)
- Bei Werten über 15,5 V: Warnung (Regler/Gleichrichter defekt)

**Pufferakku (LiPo):**
Der LiPo-Akku im Bordcomputer ist thermisch vom Hauptprozessor getrennt und dient nur als Puffer. Er wird bei laufendem Motor geladen. Im Winter (unter 0°C) blockiert die Software die Ladung automatisch, um Lithium-Plating zu vermeiden (was die Kapazität dauerhaft reduzieren und zu internen Kurzschlüssen führen kann). Bei Temperaturen über 45°C wird der Ladestrom reduziert.

Der Pufferakku ist nicht für den dauerhaften Betrieb gedacht. Wenn du den Bordcomputer längere Zeit (Wochen) nicht benutzt, entlädt sich der Akku langsam. Das ist normal und schadet nicht — er wird beim nächsten Start wieder geladen.

### 6.4 Fehlerbehebung (Problem → Lösung)

| Problem | Mögliche Ursache | Lösung |
|---|---|---|
| Display bleibt dunkel | Keine Spannung, Sicherung defekt, Display-Kabel lose | Spannung prüfen, Sicherung kontrollieren, Kabelverbindung Display-PCB prüfen |
| Display zeigt nichts, aber LED leuchtet | E-Paper-Refresh fehlgeschlagen | Neustart (Spannung trennen und wiederherstellen). Wenn das Problem bleibt: USB-C verbinden und Diagnose-Konsole prüfen |
| GPS findet keinen Fix | Antenne verdeckt, im Gebäude, kalter Start | Ins Freie gehen, freie Sicht nach oben, bis zu 2 Minuten warten (Kaltstart). Wenn das Problem bleibt: GPS-Antennenkabel prüfen |
| BLE-Verbindung bricht ab | Smartphone zu weit weg, BLE gestört | Smartphone näher am Bordcomputer, App neu starten. Wenn das Problem bleibt: Pairing wiederholen |
| Drehzahl zeigt falsche Werte | Falsche Kalibrierung, Optokoppler defekt | Übersetzung/Pickup-Einstellung prüfen. Optokoppler-Signal mit Diagnose-Konsole prüfen |
| Temperatur unplausibel | Sensor-Kabel lose, Sensor defekt, Wasser im Stecker | Sensor-Verbindung prüfen, Stecker auf Korrosion kontrollieren, Sensor-Widerstand messen |
| Gang wird falsch angezeigt | Übersetzung nicht kalibriert, Kupplung gezogen | Übersetzung in den Einstellungen anpassen. Bei gezogener Kupplung ist falsche Anzeige normal |
| Kraftstoffstand springt | Sensor nicht kalibriert, lose Verbindung | Kraftstoffstand-Kennlinie kalibrieren (in App unter Sensor-Kalibrierung) |
| SD-Karte nicht erkannt | Karte nicht eingesetzt, falsch formatiert, defekt | SD-Karte entnehmen und wieder einsetzen. FAT32 formatieren. Karte ersetzen |
| Bordcomputer startet nicht | Spannung zu niedrig (<8V), Firmware korrupt | Batterie prüfen. Firmware per USB-C und Companion-App neu aufspielen |
| App findet Bordcomputer nicht | BLE nicht aktiv, Bordcomputer im falschen Modus | Bluetooth am Smartphone aktivieren. Bordcomputer neu starten. Pairing-Liste in App leeren und neu koppeln |
| Self-Test schlägt fehl | Hardware-Defekt, Sensor nicht angeschlossen | DTC im Diagnose-Menü ablesen, betroffenen Sensor/Stecker prüfen |

---

# TEIL 2: INSTALLATIONSANLEITUNG

---

## 7. Voraussetzungen

### 7.1 Werkzeug-Liste

Für die Installation des Bordcomputers an der Honda Transalp 600 (PD06) brauchst du folgendes Werkzeug:

**Allgemein:**
- Satz Inbus-Schlüssel (2,5 mm, 4 mm, 5 mm)
- Maulschlüssel (8 mm, 10 mm, 14 mm)
- Kreuzschlitzschraubendreher
- Seitenschneider / Abisolierzange
- Crimpzange für Kabelschuhe (für AMP Superseal oder Deusch DTM, je nach Stecksystem)
- Lötkolben (für Kabelverbindungen im Gehäuse)
- Schrumpfschlauch (verschiedene Durchmesser)
- Kabelbinder (UV-beständig, schwarz, 100 mm und 200 mm)
- Multimeter (für Spannungs- und Widerstandsmessung)

**Fahrzeugspezifisch:**
- 10 mm Steckschlüssel (für Batteriepole)
- 8 mm Steckschlüssel (für Kabelbaum-Steckverbinder)
- Drehmomentschlüssel (für Ölablassschraube, falls Temperatursensor an der Ölschraube montiert wird)

**Optional (für erweiterte Installation):**
- Schrumpfschlauch-Sortiment (mit Kleber)
- Conduit (Kabelschutzschlauch) für externe Kabelstränge
- Ferritkerne (Klappferrite) für EMV-Filterung an externen Kabeln
- Isopropanol (IPA) zum Reinigen vor dem Löten

### 7.2 Benötigte Teile (BOM-Referenz)

Die vollständige Stückliste (Bill of Materials) ist in den Projektdokumenten unter `docs/research/09-master-plan.md` Abschnitt 10 dokumentiert. Hier die Zusammenfassung der wichtigsten Komponenten:

**Bordcomputer-Gehäuse und Elektronik:**
- 3D-gedrucktes ASA-Gehäuse (Unterteil, Deckel, Display-Rahmen)
- ESP32-S3-WROOM-1 Platine (N8R2: 8 MB Flash, 2 MB PSRAM)
- E-Paper-Display 2,9" (296 × 128 Pixel, Breittemperatur -20 bis +60°C)
- GPS-Modul (NEO-M9N) mit aktiver Patch-Antenne
- IMU-Sensor (BMI270) und Barometer (BMP388)
- MicroSD-Karte (8–32 GB, Class 10)
- DC/DC-Wandler (Buck-Converter, 7–40 V Eingang, 5 V/2 A Ausgang)
- LiPo-Akku (1000–2000 mAh, Puffer)
- Kabelverschraubungen (PG7/PG9, PA66, IP67)
- USB-C-Buchse (IP67, Panel-Mount)
- Silikon-O-Ring (2 mm Cord, für Deckeldichtung)
- ePTFE-Druckausgleichsmembran (M6)
- Kupferfolie (selbstklebend, für EMV-Abschirmung innen)
- Conformal Coating (Polyurethan, für PCB-Schutz)

**Sensoren (extern am Motorrad):**
- Hall-Sensor + Magnet (für Geschwindigkeit am Vorderrad, optional als Fallback)
- Kühlmitteltemperatur-Inlinesensor (für oberen Kühlschlauch, 32–36 mm Innendurchmesser)
- Kraftstoffstandssensor (Aftermarket-Schwimmer, 0–190 Ohm oder 240–33 Ohm)
- Optokoppler-Modul (PC817 oder 6N137, für RPM-Signal von der Zündung)

**Montage:**
- U-Bolt-Schelle (22 mm, für Lenker oder Crossbar)
- Gummi-Metall-Dämpfer (M6, Shore 50–60A)
- Kugelgelenk-Halterung (neigungsverstellbar, 0–15°)
- Schrauben: M6 × 20 mm (Halterung), M3 × 12 mm (Gehäuse), M3 × 8 mm (PCB), M2,5 × 6 mm (Display)

**Kabel und Steckverbinder:**
- Geschirmte Kabel (SFTP oder S/UTP) für Sensoren
- AMP Superseal Steckverbinder (IP67, 2–6-polig) für Sensor-Anschlüsse
- Kabelbinder (UV-beständig)
- Schrumpfschlauch mit Kleber (adhesive-lined)

### 7.3 Fähigkeiten / Vorkenntnisse

Für die Installation solltest du folgende Fähigkeiten mitbringen:

- **Grundlegende Elektrikkenntnisse:** Du weißt, wie man einen Spannungsteiler aufbaut, Widerstände misst und Kabel verbindet (löten oder crimpen).
- **Mechanische Grundkenntnisse:** Du kannst Kabelbinder setzen, Schrauben anziehen und Kabel durch das Motorrad führen.
- **Keine Angst vor Fehlern:** Der Bordcomputer ist ein Zusatzsystem. Wenn etwas nicht funktioniert, startet das Motorrad trotzdem normal. Du kannst nichts am Motorrad kaputt machen, solange du keine bestehenden Kabel durchtrennst.

---

## 8. Mechanische Montage

### 8.1 Gehäuse zusammenbauen

Das Gehäuse besteht aus drei 3D-gedruckten Teilen:

1. **Unterteil (Basisschale):** Enthält PCB-Führungsschienen, Schraubsäulen, Kabeldurchführungen an der Unterseite und die Halterungs-Schnittstelle an der Rückseite.
2. **Deckel:** Schließt das Gehäuse von oben ab, trägt den Display-Ausschnitt, die Druckausgleichs-Membran und die Taste. Der Deckel greift über das Unterteil (Shingling-Prinzip), so dass Wasser an der Naht nach außen abfließt.
3. **Display-Rahmen (innen):** Ein separater Einsatz aus PETG, der das E-Paper-Modul fixiert und auf einem Silikon-Pad ruht. Wird mit dem Deckel verschraubt.

**Zusammenbau-Schritte:**

1. PCB in das Unterteil einschieben (auf die Führungsschienen).
2. PCB mit 4 × M3 × 8 mm Schrauben in den Schraubsäulen arretieren (mit Gummipuffer als Vibrations-Entkopplung).
3. Display-Rahmen mit E-Paper-Modul in den Deckel einsetzen.
4. Display-Rahmen mit 4 × M2,5 × 6 mm Schrauben am Deckel verschrauben (mit Federscheibe).
5. Silikon-O-Ring in die Nut im Deckel einlegen.
6. Deckel auf das Unterteil setzen und mit 4 × M3 × 12 mm Schrauben verschrauben (mit Federscheibe).
7. Kabelverschraubungen (PG7/PG9) in die Öffnungen an der Unterseite einschrauben (mit O-Ring zwischen Gland und Gehäusewand, Kontermutter innen).
8. USB-C-Buchse (IP67) an der Unterseite befestigen.
9. ePTFE-Druckausgleichsmembran (M6) an der Unterseite einschrauben (vertikal orientiert).
10. Kupferfolie auf die Innenseite des Gehäuses kleben (mit Fenster für BLE- und GPS-Antenne, mindestens 30 × 30 mm freie Fläche).

### 8.2 Halterung am Lenker (22 mm PD06)

Die Honda Transalp 600 (PD06) hat einen Standard-Lenker mit 22 mm (7/8 Zoll) Durchmesser. Das ist das Standardmaß für Straßenmotorräder.

**Montage-Optionen:**

**Option A: Direkte Lenkerhalterung (einfachste Variante)**
- U-Bolt-Schelle aus Metall um den 22-mm-Lenker
- Halterungs-Adapter aus ASA gedruckt
- Vorteil: einfach, schnell abnehmbar
- Nachteil: lenkerdrehend, weniger stabil für fest verbaute Elektronik

**Option B: Crossbar-Montage (empfohlen)**
- Befestigung an einer Querstange (Crossbar) zwischen den Lenkerhaltern
- Die PD06 hat oft schon eine Crossbar serienmäßig oder nachgerüstet
- Vorteil: stabilere Plattform, weniger Lenker-Drehmoment, bessere Positionierung

**Option C: Lenkerhalter-Klemmbrücke**
- Montage an den Lenkerhalter-Klemmen (M6 oder M8 Schrauben)
- Vorteil: sehr stabil, ohne Lenker zu belasten
- Nachteil: modellspezifisch, braucht angepasste Halterung

**Empfohlene Montage (Option B: Crossbar) mit Kugelgelenk:**
Die Halterung wird an der Crossbar befestigt und über ein Kugelgelenk mit dem Gehäuse verbunden. Das erlaubt Neigung und Rotation für optimale Lesbarkeit.

**Montage-Schritte:**
1. Crossbar zwischen den Lenkerhaltern montieren (falls nicht vorhanden).
2. U-Bolt um die Crossbar legen.
3. Halterungs-Adapter (ASA) zwischen U-Bolt und Crossbar positionieren.
4. U-Bolt mit M6-Muttern festziehen (mit Federscheibe).
5. Gummidämpfer zwischen Halterungs-Adapter und Gehäuserückseite einsetzen.
6. Kugelgelenk mit M6-Schraube am Halterungs-Adapter befestigen.
7. Gehäuse auf das Kugelgelenk setzen und mit Klemmhebel arretieren.
8. Neigung einstellen (10–15° nach hinten vom Fahrer weg) und Klemmhebel festziehen.

### 8.3 Display-Ausrichtung einstellen

Der Bordcomputer muss neigungsverstellbar sein, um verschiedene Fahrergrößen und Sitzpositionen zu unterstützen. Empfohlene Neigung: 10–15° nach hinten (vom Fahrer weg geneigt), damit:
- Bei tiefem Sonnenstand das Licht nicht direkt vom Display reflektiert wird
- Bei hohem Sonnenstand das Display von oben beleuchtet wird
- Regenwasser nicht auf dem Display stehen bleibt (Tropfenform)

Die Neigung wird am Klemmhebel des Kugelgelenks werkzeuglos eingestellt. Nach der Einstellung den Klemmhebel festziehen.

### 8.4 Kabelführung am Motorrad

Die Kabel vom Bordcomputer zur Sensorik verlaufen:
- Entlang dem Rahmenrohr (rechte Seite, nicht an der Auspuff-Seite)
- Unter Verkleidungsteilen, wo vorhanden
- An Kabelbindern am Rahmen fixiert (alle 15–20 cm)
- Schrumpfschlauch über Kabelverbindungen als mechanischer Schutz
- Gummitüllen wo Kabel durch Verkleidungsöffnungen gehen

**Wichtig:**
- Keine Kabel entlang der Auspuffanlage (Hitze kann die Isolierung beschädigen).
- Keine Kabel in der Nähe des CDI-Moduls (elektromagnetische Einstreuung).
- Geschirmte Kabel (SFTP oder S/UTP) für alle Sensorkabel verwenden.
- Schirm an einem Ende auf Masse legen (Gehäuseseite), um Masseschleifen zu vermeiden.
- Kabel nicht frei hängend — immer fixiert (Vibration kann lose Kabel beschädigen).

---

## 9. Elektrische Installation

> **SICHERHEITSWARNUNG:** Klemme die Batterie ab, bevor du Kabel anschließt oder modifizierst. Die Transalp 600 hat eine 12V-Blei-Säure-Batterie (12 Ah, YB12A-B). Ein Kurzschluss an der Batterie kann Funken, Hitze und im schlimmsten Fall ein Feuer verursachen. Trenne zuerst den Minuspol (grünes Kabel), dann den Pluspol (schwarz/braunes Kabel).

### 9.1 Anschluss an 12V Versorgung (Zündungsplus)

Der Bordcomputer wird an das geschaltete 12V-Netz des Motorrads angeschlossen. "Geschaltet" bedeutet: Der Bordcomputer erhält nur dann Strom, wenn die Zündung eingeschaltet ist. Das verhindert, dass der Bordcomputer die Batterie entlädt, wenn das Motorrad abgestellt ist.

**Anschlusspunkt:**
- Das schwarze Kabel im Honda-Kabelbaum ist das geschaltete +12V (über Zündschalter). Es führt nur dann +12V, wenn die Zündung AN ist.
- Das schwarz/braune Kabel ist Batterie-Plus (direkt, immer +12V).
- Das grüne Kabel ist Masse (Rahmen).

**Verbindung:**
1. Batterie abklemmen (Minuspol zuerst, grünes Kabel).
2. Geschaltetes +12V vom schwarzen Kabel abzapfen (Kabelschuh oder T-Abzweigung).
3. Masse vom grünen Kabel abzapfen.
4. Kabel durch eine Kabelverschraubung (PG11) an der Gehäuseunterseite in das Gehäuse führen.
5. Kabel an den 12V-Eingang des PCB anschließen (AMP Superseal 2-polig).

### 9.2 Sicherung einbauen

Zwischen dem 12V-Abzweigpunkt und dem Bordcomputer muss eine Sicherung installiert werden, um den Bordcomputer und das Motorrad-Kabelsystem zu schützen.

**Sicherungswahl:**
- Der Bordcomputer verbraucht weniger als 30W (bei 12V entspricht das ca. 2,5A).
- Empfohlene Sicherung: 5A (flink, glas oder Automotive-Stecksicherung).
- Die Sicherung so nah wie möglich am Abzweigpunkt platzieren (innerhalb von 10–15 cm).

Die Transalp 600 hat serienmäßig folgende Sicherungen:
- Hauptsicherung: 30A
- Teilsicherungen (je nach Markt): 3–4 × 10A, 1–2 × 15A

Der Bordcomputer sollte eine eigene Sicherung bekommen, nicht in eine bestehende Sicherung eingreifen.

### 9.3 Sensor-Kabel verlegen

#### Pickup-Sensor (Drehzahl)

Das Drehzahlsignal wird vom Pulsgenerator (Pickup) der CDI-Zündung abgegriffen. Der Pickup liefert 1 Puls pro Kurbelwellenumdrehung.

**Anschluss am PD06:**
- Bei Modellen bis 1995 (V-H bis V-R): Zwei separate Pickup-Spulen (eine pro Zylinder), Kabel gelb oder blau.
- Bei Modellen ab 1996 (V-T bis V-X): Eine einzelne Pickup-Spule.
- Das Signal kann am CDI-Kabel (Pin 5 der CDI-Box, gelb/weiß) oder an der Minusseite der Zündspule (Coil-) abgegriffen werden.

**WICHTIG:** Das Pickup-Signal darf nicht direkt an den ESP32 angeschlossen werden. Die CDI erzeugt Hochspannungspulse (200–400V am Kondensator, 20–40kV an der Zündspule), die den Microcontroller zerstören würden. Verwende zwingend einen Optokoppler (PC817 für Standard-Signale, 6N137 für hohe Frequenzen) zur galvanischen Trennung.

**Verkabelung:**
1. Pickup-Kabel (gelb/weiß) in der Nähe der CDI-Box identifizieren.
2. Abzweigung mit T-Verbinder oder durch Anlöten herstellen.
3. Kabel durch Kabelschutzschlauch führen und am Rahmen entlang zum Bordcomputer verlegen.
4. Kabel durch PG7-Kabelverschraubung in das Gehäuse führen.
5. An den Optokoppler-Eingang des PCB anschließen (AMP Superseal oder Deusch DTM).

#### Temperaturfühler (Kühlmittel)

Die PD06 hat serienmäßig keinen elektronischen Kühlmitteltemperatursensor. Für die Temperaturanzeige muss ein Inlinesensor in den oberen Kühlschlauch (vom Motor zum Kühler) eingesetzt werden.

**Montage:**
1. Kühlmittel ablassen (siehe Service-Manual, Kühlmittelmenge ca. 2,0 Liter).
2. Oberen Kühlschlauch am Motor lösen.
3. Inlinesensor (Adapter mit O-Ring-Schelle) in den Schlauch einsetzen. Den Innendurchmesser des Schlauchs messen (meist 32–36 mm) und passenden Adapter verwenden.
4. Schlauch wieder montieren, Klemmschellen festziehen.
5. Kühlmittel auffüllen und entlüften.
6. Sensor-Kabel zum Bordcomputer verlegen und durch PG7 führen.
7. An den ADC-Eingang des PCB anschließen (AMP Superseal 3-polig: Signal, Masse, Shield).

Der Sensor ist ein NTC-Thermistor mit folgender Kennlinie:
- 50°C → 154 Ohm
- 80°C → 52 Ohm
- 120°C → 16 Ohm

Am ADC des ESP32 (0–3,3V) wird ein Spannungsteiler mit Referenzwiderstand verwendet. Die Genauigkeit liegt bei ±2°C.

#### Kraftstoffgeber (falls analog nachgerüstet)

Die Transalp 600 hat serienmäßig keinen elektronischen Tankgeber. Die Kraftstoffanzeige funktioniert über eine Vakuum-Petcock-Membran mit Reserve-Stellung und eine Warnleuchte. Für eine elektronische Tankstandsanzeige muss ein Aftermarket-Sender nachgerüstet werden.

**Montage:**
1. Tank leeren (oder auf Reserve stellen und restlichen Kraftstoff auffangen).
2. Tankdeckel oder Tanköffnung zugänglich machen.
3. Schwimmer-Sensor (0–190 Ohm oder 240–33 Ohm) durch die Tanköffnung oder eine separate Bohrung im Tankdeckel installieren.
4. Sensor-Kabel zum Bordcomputer verlegen und durch PG7 führen.
5. An den ADC-Eingang des PCB anschließen (AMP Superseal 3-polig).

**Kalibrierung:**
Die Kennlinie des Kraftstoffsensors ist nicht linear (oben flach, unten steil). Die Kalibrierung erfolgt über die Companion-App:
1. Tank volltanken.
2. App öffnen, Sensor-Kalibrierung → Kraftstoffstand → "Voll" kalibrieren.
3. Tank auf 50% entleeren (ca. 9 Liter verbrauchen).
4. "Halbvoll" kalibrieren.
5. Tank auf Reserve (ca. 3 Liter Rest) entleeren.
6. "Leer" kalibrieren.
7. Die App interpoliert die Kennlinie aus den drei Stützstellen.

### 9.4 Masseanschluss

Der Masseanschluss (Rahmen/Masse) ist kritisch für die Funktion aller Sensoren und des Bordcomputers.

**Anschlusspunkt:**
- Das grüne Kabel im Honda-Kabelbaum ist Masse (Rahmen).
- Der Bordcomputer hat einen eigenen Masseanschluss an der Gehäuserückseite (Klemme mit Sternpunkt-Verbindung zum PCB).

**Sternpunkt-Masse:**
Alle Masseleitungen laufen an einem einzigen Punkt zusammen (Sternpunkt / Single-Point Ground), um Masseschleifen zu vermeiden. Wenn Strom durch verschiedene Massepfade fließt, entstehen Spannungsabfälle, die Sensoren verfälschen. Der Sternpunkt liegt am Gehäuse / Akku-Minus / DC/DC-Masse.

**Verbindung:**
1. Massekabel (grün) vom Motorrad-Kabelbaum an den Masseanschluss des Bordcomputers anschließen.
2. Kabelquerschnitt: mindestens 0,75 mm² (besser 1,5 mm²).
3. Verbindung mit Kabelschuh und Schraube sichern (mit Federscheibe gegen Vibration).

### 9.5 Kabelbinder und Schutzschläuche

Alle externen Kabel müssen gesichert und geschützt werden:

- **Kabelbinder (UV-beständig, schwarz):** Alle 10–15 cm am Rahmen fixieren.
- **Schrumpfschlauch mit Kleber (adhesive-lined):** Über alle Kabelverbindungen als mechanischer und Feuchtigkeitsschutz.
- **Conduit (Kabelschutzschlauch):** Für Kabelstränge, die extern laufen (z.B. vom Motor zum Lenker). Schützt vor mechanischer Beschädigung und UV-Strahlung.
- **Ferritkerne (Klappferrite):** An jedem Kabel, das ins Gehäuse mündet, möglichst nah am Gehäuse. Wirkt als Drossel für Hochfrequenz und reduziert eingekoppelte Störungen.

---

## 10. PD06-spezifische Hinweise

### 10.1 Wo befindet sich die Pickup-Spule am PD06?

Die Pickup-Spule (Pulsgenerator) der Transalp 600 befindet sich am Motorblock, an der Seite des Kurbelgehäuses. Die genaue Position variiert leicht je nach Modelljahr:

- **Modelle bis 1995 (V-H bis V-R):** Zwei separate Pickup-Spulen (eine pro Zylinder), im Bereich des Kurbelgehäuses auf der rechten Seite. Die Kabel sind gelb bzw. blau und führen zu den jeweiligen CDI-Einheiten.
- **Modelle ab 1996 (V-T bis V-X):** Eine einzelne Pickup-Spule, ebenfalls am Kurbelgehäuse. Das Kabel führt zu der gemeinsamen Digital-CDI-Box.

Die CDI-Einheiten sitzen unter der Sitzbank in einer Gummihalterung. Das CDI-Kabel hat folgende Pinbelegung:
- Pin 1: +12V (über Sicherung, Zündschalter, Kill-Switch)
- Pin 5: Pulsgenerator (Pickup) — das ist der Abgriffpunkt für das Drehzahlsignal

**Kabelbaum-Farbcodes (Honda Standard) für den Pickup:**
- Gelb/Weiß (Yellow/White): Zündspule / Pickup-Signal

Das Signal kann am Pin 5 der CDI-Box abgegriffen werden (mit Optokoppler oder kapazitiver Kopplung, um die CDI nicht zu belasten). Alternativ das Signal an der Minusseite der Zündspule (Coil-) mit einem induktiven Klammer-Sensor.

### 10.2 Motoröl-Temperatur: Ölablassschraube als Messpunkt?

Die Ölablassschraube der Transalp 600 hat ein Gewinde von M14 × 1,5 (metrisches Feingewinde) und ein Anzugsdrehmoment von 34 Nm. Sie befindet sich an der Unterseite des Kurbelgehäuses.

Für die Motoröltemperatur-Messung kann ein Temperatursensor mit M14 × 1,5 Gewinde anstelle der originalen Ölablassschraube montiert werden. Es gibt Aftermarket-Sensoren (z.B. von Koso oder VDO), die dieses Gewinde haben und einen Temperatursensor integrieren.

**Vorgehensweise:**
1. Motoröl ablassen (warm, damit es flüssiger ist — Vorsicht: heiß!).
2. Originale Ölablassschraube herausdrehen.
3. Dichtung prüfen (neue Dichtung verwenden, falls die alte beschädigt ist).
4. Temperatursensor mit M14 × 1,5 Gewinde einschrauben (mit neuer Dichtung).
5. Anzugsdrehmoment: 34 Nm (wie originale Ölablassschraube).
6. Motoröl auffüllen (Ölfüllmenge: 2,10 Liter bei Ölwechsel, 2,30 Liter mit Filterwechsel).
7. Sensor-Kabel zum Bordcomputer verlegen.

**Alternative Messpunkte:**
- Öldruckschalter-Abgang (M10 × 1,0) am Kurbelgehäuse: Hier kann ein T-Stück-Adapter montiert werden, um sowohl den originalen Öldruckwarnschalter als auch einen Temperatursensor/Drucksensor zu behalten.
- Kühlmitteltemperatur: Im oberen Kühlschlauch (siehe Abschnitt 9.3).

### 10.3 Kabelweg vom Motor zum Lenker

Der Kabelweg vom Motor (Sensoren) zum Lenker (Bordcomputer) verläuft an der Transalp 600 wie folgt:

1. **Motorblock (Pickup, Öldruck, Öltemperatur):** Kabel an der rechten Seite des Kurbelgehäuses zusammenführen.
2. **Rahmenrohr (rechte Seite):** Kabelstrang entlang dem rechten Rahmenrohr nach oben führen. Die rechte Seite ist zu bevorzugen, weil die linke Seite näher an der Auspuffanlage ist.
3. **Unter Sitzbank:** Kabel unter der Sitzbank entlangführen (hier sitzen auch die CDI-Module — Abstand halten, EMV!).
4. **Lenkerhalter:** Kabelstrang an den Lenkerhaltern nach oben führen.
5. **Lenker/Crossbar:** Kabel an der Crossbar zum Bordcomputer führen.

**Kabellänge:**
- Pickup am Kurbelgehäuse → Lenker: ca. 100–120 cm
- Kühlmitteltemperatur am oberen Kühler → Lenker: ca. 80–100 cm
- Kraftstoffgeber am Tank → Lenker: ca. 60–80 cm
- Batterie (12V) → Lenker: ca. 80–100 cm

Die maximale Leitungslänge für ADC-Sensoren beträgt 50 cm (mit RC-Filter und Abtastung). Für längere Strecken geschirmte Kabel verwenden und Ferritkerne am Kabelende nah am Gehäuse platzieren.

### 10.4 Reichweite der BLE-Verbindung auf dem Motorrad

Die BLE-Reichweite (Bluetooth Low Energy) auf dem Motorrad ist durch mehrere Faktoren eingeschränkt:

- Der Körper des Fahrers absorbiert und blockiert HF-Signale.
- Metalle (Tank, Rahmen, Motorblock) reflektieren und dämpfen das Signal.
- Die ESP32-S3-Antenne hat eine begrenzte Sendeleistung (typisch +4 bis +8 dBm).

**Praktische Reichweite:**
- Smartphone in der Brust- oder Jackentasche: 5–15 Meter, in der Regel stabil.
- Smartphone im Tankrucksack: 2–5 Meter, meist stabil.
- Smartphone im Rucksack am Rücken: 1–3 Meter, oft instabil (Körper blockiert).
- Smartphone im Seitenkoffer: unzuverlässig (Metall blockiert).

**Empfehlung:** Smartphone in der inneren Brusttasche der Motorradjacke oder am Lenker mit einer Smartphone-Halterung tragen. Die Companion-App muss aktiv im Hintergrund laufen (Betriebssystem-Einstellungen: Hintergrundaktualisierung für die App aktivieren).

Bei schlechter Verbindung (RSSI < -90 dBm oder Packet-Loss > 10%) schaltet der Bordcomputer in den DEGRADING-Zustand: Es werden nur noch priorisierte Daten übertragen (Navigation, Anrufe), keine Telemetrie-Updates und keine Wetter-Daten. Die Verbindung wird automatisch wiederhergestellt, sobald die Signalqualität besser wird.

---

## 11. Inbetriebnahme

### 11.1 Erst-Einschalten

Nach der kompletten Installation (mechanisch und elektrisch) kann der Bordcomputer zum ersten Mal eingeschaltet werden.

**Vorbereitung:**
1. Alle Kabelverbindungen prüfen (Sichtkontrolle: keine losen Kabel, alle Stecker eingerastet).
2. Sicherung kontrollieren (5A, eingesetzt).
3. Batterie anklemmen (zuerst Pluspol schwarz/braun, dann Minuspol grün).
4. Multimeter an die 12V-Eingangsklemmen des Bordcomputers halten und Spannung prüfen (sollte ca. 12–13V sein, wenn Zündung aus).

**Erstes Einschalten:**
1. Zündung einschalten (Motor nicht starten).
2. Der Bordcomputer sollte innerhalb von 2–3 Sekunden hochfahren.
3. Das E-Paper-Display zeigt den Boot-Screen (Logo, Firmware-Version).
4. Der Self-Test läuft ab.

### 11.2 Self-Test-Abfolge

Der Self-Test prüft alle Hardware-Komponenten in folgender Reihenfolge:

1. **Display-Test:** Alle Pixel werden kurz eingeschaltet (Schwarzbild), dann gelöscht (Weißbild). Das dauert ca. 2 Sekunden.
2. **GPS-Modul initialisieren:** Der Bordcomputer sendet eine Konfiguration an das GPS-Modul und wartet auf die Antwort. Wenn das Modul nicht antwortet: DTC TBC-0201.
3. **I2C-Sensoren ansprechen:** IMU (BMI270) und Barometer (BMP388) werden über I2C angesprochen. Wenn ein Sensor nicht antwortet: DTC TBC-01xx.
4. **SD-Karte mounten:** Die MicroSD-Karte wird initialisiert und gemountet. Wenn die Karte fehlt oder defekt ist: DTC TBC-0501.
5. **ADC-Kanäle lesen:** Batteriespannung, Kraftstoffstand und andere analoge Sensoren werden gelesen. Wenn ein Wert unplausibel ist (z.B. Spannung <5V oder >20V): DTC TBC-03xx.
6. **Optokoppler-Eingänge prüfen:** Die RPM-Signal-Eingänge werden auf Plausibilität geprüft (Pull-Up-Widerstand prüfen, kein permanenter Low-Pegel).
7. **BLE-Modul initialisieren:** Das Bluetooth-Modul wird initialisiert und in den Advertising-Modus geschaltet. Wenn das Modul nicht antwortet: DTC TBC-04xx.
8. **Konfiguration laden:** Die Konfiguration wird von der SD-Karte (oder Flash) geladen. Wenn die Konfiguration korrupt ist: Werkseinstellungen laden.

**Ergebnis:**
- Bei Erfolg: Dreifacher Piepton, Status-LED leuchtet grün. Der Bordcomputer wechselt in die Hauptansicht.
- Bei Fehler: Absteigende Tonfolge, Status-LED leuchtet rot. Der DTC wird auf dem Display angezeigt. Der Bordcomputer funktioniert mit eingeschränkter Funktionalität (fehlende Sensoren werden als "---" angezeigt).

### 11.3 GPS-Fix abwarten

Nach dem Self-Test beginnt das GPS-Modul mit der Satellitensuche. Bei einem Kaltstart (erstes Einschalten oder nach langer Standzeit) kann der erste GPS-Fix bis zu 2 Minuten dauern. Die GPS-LED blinkt blau während der Suche.

**Voraussetzungen für einen schnellen GPS-Fix:**
- Freie Sicht nach oben (im Freien, nicht in einer Garage oder unter einem Dach).
- Die GPS-Antenne darf nicht durch Metall verdeckt sein (Tank, Motorblock, Verkleidung).
- Mindestens 10 cm Abstand zwischen GPS-Antenne und GPS-Modul (bei aktiven Antennen).

Sobald ein GPS-Fix erreicht ist (mindestens 4 Satelliten), leuchtet die GPS-LED durchgehend blau und das Display zeigt die aktuelle Geschwindigkeit und Position an.

### 11.4 Companion-App koppeln

Nach dem GPS-Fix kann die Companion-App gekoppelt werden:

1. Companion-App auf dem Smartphone öffnen.
2. "Gerät koppeln" tippen.
3. App sucht nach dem Bordcomputer (BLE-Scan, dauert ca. 5–10 Sekunden).
4. Bordcomputer aus der Liste auswählen ("Transalp-BC-XXXX").
5. Pairing bestätigen (Just-Works oder Passkey-Eingabe).
6. BLE-LED am Bordcomputer leuchtet durchgehend weiß.
7. Die App lädt die aktuelle Konfiguration und Telemetrie-Daten.

Nach dem ersten Pairing werden die Pairing-Keys gespeichert. Beim nächsten Start verbinden sich Bordcomputer und Smartphone automatisch, sobald die Zündung eingeschaltet wird und die App im Hintergrund läuft.

### 11.5 Erste Konfiguration (Einheiten, Alarme)

Nach dem Koppeln solltest du die wichtigsten Einstellungen vornehmen:

**In der App unter Einstellungen:**

1. **Einheiten:**
   - Geschwindigkeit: km/h (Standard für Europa)
   - Temperatur: °C
   - Druck: bar

2. **Shift-Light:**
   - Shift-Light-Drehzahl: 7.000 U/min (Standard, anpassen an deinen Fahrstil)
   - Rote Zone: 8.500 U/min (Standard)

3. **Temperatur-Warnung:**
   - Kühlmittel: 110°C (Standard)
   - Motoröl (falls installiert): 130°C

4. **Kraftstoff-Warnung:**
   - Restfüllstand: 20% (Standard)

5. **Spannungs-Warnung:**
   - Niedrig: 11,5V
   - Hoch: 15,5V

6. **Radumfang (falls Hall-Sensor installiert):**
   - Vorderrad (90/90-21): 2.185 mm
   - Hinterrad (130/80-17): 2.010 mm

7. **Notfallkontakt:**
   - Name und Telefonnummer hinterlegen

8. **Tour-Aufzeichnung:**
   - Automatische Aufzeichnung: AN (Standard)

---

## 12. Fehlersuche

### 12.1 Display bleibt dunkel

**Mögliche Ursachen und Lösungen:**

1. **Keine Spannung:** Kontrolliere die Sicherung (5A). Kontrolliere die 12V-Eingangsspannung am Bordcomputer mit einem Multimeter (sollte 12–15V bei laufendem Motor sein, 12–13V bei eingeschalteter Zündung und stehendem Motor).

2. **Sicherung defekt:** Wenn die Sicherung durchgebrannt ist, liegt ein Kurzschluss vor. Kontrolliere die Kabelverbindungen auf Kurzschlüsse (Plus nach Masse). Sicherung erst ersetzen, nachdem der Kurzschluss behoben ist.

3. **Display-Kabel lose:** Öffne das Gehäuse (4 × M3 Schrauben am Deckel) und kontrolliere das FPC-Kabel zwischen Display und PCB. Das Kabel muss fest im Connector sitzen, nicht unter Spannung.

4. **E-Paper-Refresh fehlgeschlagen:** Manchmal startet das E-Paper-Display nach einem Spannungsausfall nicht korrekt. Trenne die Spannung (Zündung aus, 30 Sekunden warten), dann wieder einschalten.

5. **Firmware-Problem:** Wenn das Display nach dem Booten dunkel bleibt, aber die Status-LED leuchtet: Schließe ein USB-C-Kabel an und verbinde mit einem Computer. Öffne eine serielle Konsole (115200 Baud) und prüfe die Boot-Meldungen.

### 12.2 Kein GPS-Fix

**Mögliche Ursachen und Lösungen:**

1. **Drinnen:** GPS funktioniert nur im Freien mit freier Sicht zum Himmel. Gehe nach draußen.

2. **Kalter Start:** Beim ersten Einschalten oder nach längerer Standzeit kann der erste GPS-Fix bis zu 2 Minuten dauern. Warte geduldig.

3. **Antenne verdeckt:** Kontrolliere, ob die GPS-Antenne freie Sicht nach oben hat. Sie darf nicht durch Metall verdeckt sein. Wenn die Antenne im Gehäuse montiert ist: Kontrolliere, ob die Kupferfolie-EMV-Abschirmung ein Fenster für die Antenne hat.

4. **Antennenkabel defekt:** Kontrolliere das Kabel zwischen GPS-Antenne und GPS-Modul. Das Kabel darf nicht geknickt oder beschädigt sein.

5. **GPS-Modul defekt:** Wenn nach 5 Minuten im Freien kein Fix erreicht wird, kann das GPS-Modul defekt sein. Prüfe über die Diagnose-Konsole (USB-C), ob das Modul auf Konfigurationsbefehle antwortet. Wenn nicht: Modul ersetzen.

### 12.3 BLE-Verbindung bricht ab

**Mögliche Ursachen und Lösungen:**

1. **Smartphone zu weit weg:** Halte das Smartphone in der Brust- oder Jackentasche, nicht im Rucksack. Praktische Reichweite: 5–15 Meter.

2. **App im Hintergrund beendet:** Manche Betriebssysteme beenden Hintergrund-Apps aggressiv, um Akku zu sparen. Stelle sicher, dass die Companion-App in den Hintergrund-Aktualisierungs-Einstellungen deines Betriebssystems erlaubt ist.

3. **Pairing verloren:** Wenn das Pairing verloren ging (z.B. nach App-Neuinstallation oder Betriebssystem-Update): Pairing in der App löschen und neu koppeln.

4. **Störung:** Andere Bluetooth-Geräte in der Nähe (Headset, Intercom) können stören. Schalte andere Bluetooth-Geräte aus und teste, ob die Verbindung stabil wird.

5. **Firmware-Kompatibilität:** Stelle sicher, dass die App-Version und die Bordcomputer-Firmware kompatibel sind. Aktualisiere beide auf die neueste Version.

### 12.4 Drehzahl zeigt falsche Werte

**Mögliche Ursachen und Lösungen:**

1. **Falsche Kalibrierung:** Die Drehzahl wird aus der Frequenz des Pickup-Signals berechnet: RPM = 60 × f (bei 1 Puls pro Umdrehung). Wenn die Anzeige konstant zu hoch oder zu niedrig ist, kann die Kalibrierung falsch sein. Prüfe in den Einstellungen, ob die Pulses-pro-Umdrehung korrekt eingestellt ist (1 für die PD06).

2. **Optokoppler defekt:** Wenn die Anzeige springt oder 0 anzeigt, kann der Optokoppler defekt sein. Prüfe mit der Diagnose-Konsole, ob Pulse am GPIO-Eingang ankommen. Wenn nicht: Optokoppler-Modul ersetzen.

3. **EMV-Störung:** Die CDI-Zündung erzeugt starke elektromagnetische Störungen. Wenn die Drehzahlanzeige bei laufendem Motor unruhig ist: Prüfe, ob das Sensorkabel ausreichend geschirmt ist, ob Ferritkerne am Kabel angebracht sind, und ob das Kabel nicht in der Nähe der CDI-Box oder Zündspule verläuft.

4. **Verbindung lose:** Kontrolliere alle Steckverbindungen zwischen Pickup und Bordcomputer. Vibration kann Stecker lockern. Verwende nur verriegelte Steckverbinder (AMP Superseal oder Deusch DTM).

5. **Falsches Kabel abgegriffen:** Bei Modellen bis 1995 mit zwei Pickup-Spulen: Stelle sicher, dass das richtige Kabel abgegriffen wurde (gelb/weiß für Zündspule/Pickup). Bei Unsicherheit: Das Signal an der Minusseite der Zündspule (Coil-) abgreifen.

### 12.5 Temperatur unplausibel

**Mögliche Ursachen und Lösungen:**

1. **Sensor-Kabel lose:** Kontrolliere die Kabelverbindung zwischen Sensor und Bordcomputer. Der Sensor ist ein NTC-Thermistor — bei offenem Kabel zeigt der ADC den Maximalwert (entspricht einer sehr niedrigen Temperatur). Bei kurzgeschlossenem Kabel zeigt der ADC 0 (entspricht einer sehr hohen Temperatur).

2. **Sensor defekt:** Miss den Widerstand des Sensors bei bekannter Temperatur mit einem Multimeter. Vergleiche mit der Kennlinie (50°C → 154 Ohm, 80°C → 52 Ohm, 120°C → 16 Ohm). Wenn der Widerstand stark abweicht: Sensor ersetzen.

3. **Wasser im Stecker:** Kontrolliere den Steckverbinder auf Korrosion oder Wassereintritt. Wenn Wasser im Stecker ist: Stecker reinigen (Isopropanol), trocknen lassen, mit Kontaktspray behandeln. Stecker mit Schrumpfschlauch überziehen.

4. **Kalibrierung falsch:** In der Companion-App unter Sensor-Kalibrierung kann ein Offset-Wert eingestellt werden, um kleine Abweichungen zu korrigieren. Vergleiche die angezeigte Temperatur mit einem Referenzthermometer.

5. **EMV-Störung:** Analoge Sensorleitungen sind empfindlich für elektromagnetische Störungen. Prüfe, ob das Kabel ausreichend geschirmt ist (SFTP), ob der Schirm an einem Ende auf Masse liegt, und ob ein RC-Filter (1 kOhm + 100 nF) am ADC-Eingang installiert ist.

---

## Anhang

### A. PD06 Technische Daten (Zusammenfassung)

| Parameter | Wert |
|---|---|
| Modellcode | PD06 (1987–1996), PD10 (1997–1999) |
| Motor | 583 cm³ V2, 52°, flüssigkeitsgekühlt, 4-Takt, SOHC, 3 Ventile/Zyl. |
| Leistung | 37 kW (50 PS) @ 8.000 U/min (spätere Modelle) |
| Drehmoment | 52–53 Nm @ 6.000 U/min |
| Verdichtung | 9,2 : 1 |
| Zündung | CDI (bis 1995) / transistorisiert (ab 1996) |
| Pickup | 1 Puls pro Kurbelwellenumdrehung |
| Getriebe | 5-Gang, Links, Rücktritt, 1-N-2-3-4-5 |
| Übersetzung Sekundär | 3,133 (47/15) |
| Tank | 18 Liter, 2,9–3,5 Liter Reserve |
| Verbrauch | ca. 4,5–5,5 L/100 km |
| Batterie | 12V, 12 Ah, YB12A-B |
| Lichtmaschine | 310 W (350 W bei 1989–1993) |
| Lenker | 22 mm (7/8") Standard |
| Bereifung vorne | 90/90-21 54S |
| Bereifung hinten | 130/80-17 65S |
| Reifenumfang vorne | 2.185 mm |
| Reifenumfang hinten | 2.010 mm |
| Vorderrad bei 100 km/h | ca. 763 U/min (12,7 Hz) |
| Ölfüllmenge (Wechsel) | 2,10 Liter (2,30 mit Filter) |
| Ölablassschraube | M14 × 1,5, 34 Nm |
| Öldruckschalter | M10 × 1,0, 12 Nm |
| Sicherungen | 30A Haupt, 3–4 × 10A, 1–2 × 15A |
| Kabelbaum Masse | Grün |
| Kabelbaum +12V geschaltet | Schwarz |
| Kabelbaum +12V direkt | Schwarz/Braun |
| Kabelbaum Pickup/Zündspule | Gelb/Weiß |
| Kabelbaum Leerlaufschalter | Hellgrün |
| Kabelbaum Ölwarnschalter | Hellgrün/Rot |

### B. Übersetzungstabelle: km/h pro 1000 U/min pro Gang

**Frühe Modelle (1987–1995, V-H bis V-R):**

| Gang | km/h pro 1000 rpm | km/h @ 8000 rpm |
|---|---|---|
| 1 | 7,93 | 63,4 |
| 2 | 11,47 | 91,8 |
| 3 | 14,77 | 118,2 |
| 4 | 18,12 | 145,0 |
| 5 | 21,22 | 169,7 |

**Späte Modelle (1996–1999, V-T bis V-X):**

| Gang | km/h pro 1000 rpm | km/h @ 8000 rpm |
|---|---|---|
| 1 | 8,16 | 65,2 |
| 2 | 11,84 | 94,7 |
| 3 | 15,30 | 122,4 |
| 4 | 18,35 | 146,8 |
| 5 | 21,22 | 169,7 |

### C. Kabelbaum-Farbcodes (Honda Standard)

| Farbe | Funktion |
|---|---|
| Schwarz (Black) | Geschaltet +12V (Zündschalter) |
| Schwarz/Weiß (Black/White) | Kill-Switch, Zündsystem |
| Schwarz/Braun (Black/Brown) | Batterie + (direkt) |
| Grün (Green) | Masse (Rahmen) |
| Gelb (Yellow) | Lichtmaschine → Regler (3× gelb) |
| Gelb/Weiß (Yellow/White) | Zündspule / Pickup |
| Braun (Brown) | Rücklicht, Instrumentenbeleuchtung |
| Blau (Blue) | Fernlicht |
| Weiß (White) | Abblendlicht |
| Hellgrün (Light Green) | Leerlaufschalter |
| Hellgrün/Rot (Light Green/Red) | Ölwarnschalter |
| Orange (Orange) | Blinker rechts |
| Hellblau (Light Blue) | Blinker links |

### D. Wartungsintervalle der PD06 (Referenz)

| Wartung | Intervall |
|---|---|
| Ölwechsel | alle 6.000 km oder jährlich |
| Ölfilterwechsel | alle 12.000 km |
| Ventilspiel prüfen | alle 12.000 km (Einlass 0,15 mm, Auslass 0,20 mm kalt) |
| Zündkerzen | alle 12.000 km (NGK DPR8EA-9, 0,8–0,9 mm Abstand) |
| Kühlmittel | alle 2 Jahre |
| Bremsflüssigkeit | alle 2 Jahre (DOT 4) |
| Luftfilter | alle 12.000 km |
| Kettenspiel | 35–45 mm, regelmäßig nachspannen |
| Gabelöl | alle 20.000 km |

### E. Sicherheits-Hinweise

1. **Batterie abklemmen:** Klemme immer die Batterie ab (Minuspol zuerst), bevor du Kabel anschließt oder modifizierst.

2. **Keine Kabel durchtrennen:** Durchtrenne keine originalen Kabel im Kabelbaum. Verwende T-Abzweiger oder löte auf und isoliere mit Schrumpfschlauch.

3. **Sicherung verwenden:** Jeder neue Stromkreis braucht eine eigene Sicherung, nah am Abzweigpunkt.

4. **Motor nicht starten bei laufendem Firmware-Update:** Spannungsspitzen durch den Anlasser können den Update-Prozess stören.

5. **Wasserdichtigkeit:** Nach dem Öffnen des Gehäuses die O-Ring-Dichtung kontrollieren und ggf. reinigen. Dichtungen sind Verschleißteile — nach 2–3 Saisons Motorrad-Einsatz sollten sie überprüft werden.

6. **Kein Hochdruckreiniger:** Nicht den Hochdruckreiniger direkt auf das Gehäuse, die Display-Abdeckung oder die Kabeldurchführungen richten. Wasser mit hohem Druck kann in Dichtungen eindringen, auch wenn das Gehäuse IP67-zertifiziert ist.

7. **Temperatur:** Der Bordcomputer ist für den Temperaturbereich -10°C bis +50°C ausgelegt. Im Winter bei unter -10°C kann das E-Paper-Display träge reagieren oder ausfallen (Breittemperatur-Variante: -20°C bis +60°C). Im Sommer bei direkter Sonneneinstrahlung kann das Gehäuseinnere 60–70°C erreichen — der ESP32-S3 ist bis +85°C spezifiziert.

8. **Vibration:** Alle Schrauben am Motorrad brauchen Federscheiben (DIN 127B oder vergleichbar) gegen Vibration. Schraubensicherungslack kann zusätzlich aufgetragen werden.

9. **EMV:** Halte Sensor-Kabel von der CDI-Box und den Zündspulen fern. Verwende geschirmte Kabel und Ferritkerne.

10. **Notfall:** Trage beim Fahren immer das Smartphone bei dir (Brusttasche), damit die Notfall-Funktion funktioniert. Hinterlege einen Notfallkontakt in der Companion-App.

---

## Quellen

Dieses Handbuch basiert auf den folgenden Projektdokumenten:

- `docs/research/04-pd06-technische-daten.md` — PD06 Technische Daten, Service-Manual, Community-Wissen
- `docs/research/05-gehaeuse-einbau-emv.md` — Gehäuse, Einbau, Wasserdichtigkeit, EMV
- `docs/research/06-feature-roadmap.md` — Feature-Roadmap und Bewertung
- `docs/research/09-master-plan.md` — Master-Plan mit BOM und Kosten
- `docs/research/10-ui-ux-design.md` — UI/UX-Design
- `docs/research/12-diagnose-support.md` — Diagnose, Support und Wartbarkeit
- `docs/research/13-connectivity-app.md` — Connectivity und Companion-App
- `docs/CASE-DESIGN.md` — Gehäuse-Design: 3D-Druck, Montage, Wasserfestigkeit
- `docs/PINMAP.md` — GPIO Pin Mapping
- `docs/PCB-LAYOUT.md` — PCB-Layout-Konzept

Technische Daten des Fahrzeugs: Honda Service Manual (P/N 61MM900), Haynes Repair Manual, Honda Owner's Manual XL600V Transalp.

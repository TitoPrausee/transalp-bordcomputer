# Feature-Roadmap: Maximale Funktionalitaet fuer den Bordcomputer

> Stand: 13.10.2026
> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986-2000, rein mechanisch, keine ECU
> Plattform: ESP32-S3 (siehe docs/research/03-hardware-software-stack.md)

## 0. Einleitung

Die Honda Transalp 600 (PD06) ist ein rein mechanisches Motorrad ohne elektronische Motorsteuerung. Es gibt keinen CAN-Bus, kein OBD-II, keine ECU. Alle Daten, die der Bordcomputer anzeigen soll, muessen ueber eigenstaendige Sensoren erfasst werden. Das ist einerseits eine Herausforderung (keine fertigen Datenfeeds), andererseits eine Chance: Wir haben volle Kontrolle ueber jede Messgroesse und jedes Sensor-Interface.

Dieses Dokument recherchiert und bewertet saemtliche Features, die in einen solchen Motorrad-Bordcomputer passen koennten. Jedes Feature wird bewertet nach:

- **Machbarkeit**: einfach / mittel / aufwendig
- **Hardware**: benoetigte Sensoren und Komponenten
- **Software**: benoetige Firmware-Module, Bibliotheken, Protokolle
- **Prioritaet**: Must / Should / Could / Won't

Die Priorisierung folgt MoSCoW. Must-Features sind fuer ein lauffaehiges Basis-System zwingend. Should-Features erhoehen den Nutzen erheblich. Could-Features sind erwuenscht aber nicht kritisch. Won't-Features werden bewusst zurueckgestellt.

---

## 1. Core-Features (Pflicht)

### 1.1 Geschwindigkeitsanzeige

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Must |

**Hardware:**
- GPS-Modul (u-blox NEO-M9N empfohlen, 25 Hz Update-Rate, ~15-20 EUR) als Primaerquelle
- Alternativ: Hall-Sensor am Vorderrad mit Magnet auf der Bremsscheibe (Fallback wenn GPS-Signal schwach)

**Software:**
- NMEA-Parsing (TinyGPS++ oder u-blox UBX-Protokoll fuer hoehere Raten)
- Geschwindigkeit aus GPS-VTG-Sentence oder PVT-Daten
- Bei Hall-Sensor: Interrupt-basierte Pulsmessung, RPM = 60.000.000 / (delta_micros * pulses_per_rev), Geschwindigkeit = wheel_rpm * radumfang * 60 / 1000 (km/h)
- Kalibrierung des Radumfangs konfigurierbar

Die GPS-basierte Geschwindigkeit ist einfacher zu implementieren und ausreichend genau (±1-2 km/h). Fuer Tracker-Szenarien oder Tunnel ist ein Hall-Sensor am Rad wertvoll, aber nicht zwingend fuer die erste Version.

Quellen:
- https://github.com/ChetanXpro/diy-gps (ESP32 GPS Navigator)
- https://github.com/renatobo/bonogps (ESP32 GPS Track Logging)
- https://electricalflux.com/mcu-general/arduino-rpm-gauge-sensor-wiring-faq (Hall-Sensor RPM)

### 1.2 Drehzahlmesser (RPM)

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Must |

**Hardware:**
- Induktiver Pick-up am Zündkabel (Kapazitiv oder Induktiv)
- Alternative: Optokoppler am Zündspuleneingang (12V-Seite)
- Signal-Konditionierung: Spannungsteiler + Zener-Diode + RC-Filter (Schutz vor HV-Spikes)
- Galvanische Trennung empfohlen (Optokoppler, z.B. PC817)

**Software:**
- Hardware-Interrupt (FALLING-Edge) auf GPIO-Pin
- Zeitmessung zwischen Pulsen: RPM = 60.000.000 / (delta_micros * pulses_per_rev)
- Fuer 4-Takt V2 (Transalp 600): 2 Zylinder, 1 Zündung pro Umdrehung pro Zylinder (Wasted Spark). Bei Abgriff an einem Zündkabel: 1 Puls pro Umdrehung
- Moving-Average-Filter zur Glaettung (5-10 Samples)
- Entprellung: Mindestabstand zwischen Pulsen (z.B. 2000 µs) ignorieren

Die PD06 Transalp hat eine kontaktlose digitale Zündung (TCI). An einem Zündkabel liegen pro Kurbelwellenumdrehung ein Zündfunke an (Wasted Spark System). Das heisst 1 Puls = 1 Umdrehung. Bei 8000 RPM entspricht das 133 Pulsen pro Sekunde, was mit ESP32-Interrupts problemlos moeglich ist.

Die Zündspulen erzeugen jedoch signifikante elektromagnetische Stoerungen (EMI). Abgeschirmtes Kabel, niedriger Pull-up-Widerstand (220 Ohm) und RC-Filter sind zwingend erforderlich.

Quellen:
- https://forum.arduino.cc/t/motorcycle-tachometer-help/103871 (EMI-Problematik)
- https://forum.arduino.cc/t/motorcycle-data-monitoring-project-need-guidance/85954 (kapazitiver Pick-up)
- https://www.reddit.com/r/arduino/comments/14r5dh/ (Zündspulensignal-Analyse)

### 1.3 Kuehlmitteltemperatur

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Must |

**Hardware:**
- DS18B20 (digital, wasserdicht, 1-Wire, -55 bis +125°C, ±0.5°C, ~3 EUR)
- Einbau in Kuehlmittelschlauch mittels T-Stueck oder Adapter (M14 oder M16 Gewinde, je nach PD06-Kuehler)
- 4.7k Pull-up-Widerstand zwischen Data und VCC

**Alternative:** NTC 10k Thermistor (analog, billiger aber ungenauer, braucht Spannungsteiler + Kalibrierung)

**Software:**
- DallasTemperature Bibliothek (1-Wire)
- Lesen alle 1-2 Sekunden (Thermische Traegheit macht hoehere Raten sinnlos)
- Alarm-Schwelle: >110°C Warnung, >115°C Kritisch

Die PD06 Transalp hat einen Luftgekuehlten V2-Motor. Die Kuehlmitteltemperatur ist relevant fuer den Oelkuehler-Kreislauf. Eine Ueberhitzungswarnung ist besonders im Stop-and-Go-Verkehr oder bei Bergauffahrten wichtig.

Quelle:
- https://forum.arduino.cc/t/motorcycle-data-monitoring-project-need-guidance/85954 (DS18B20 fuer Motortemperatur)

### 1.4 Kraftstoffstand + Reichweitenberechnung

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel bis aufwendig |
| Prioritaet | Must |

**Hardware:**
- Kraftstoffstandsensor: Die PD06 hat einen kapazitiven oder widerstandsbasierten Kraftstoffstandgeber im Tank
- Abgriff des bestehenden Geber-Signals (meist 0-100 Ohm oder 0-190 Ohm Widerstand)
- Spannungsteiler + Operationsverstaerker zur Anpassung an ESP32-ADC (0-3.3V)
- Alternativ: Ultraschallsensor im Tank (komplex, nicht empfohlen)

**Software:**
- ADC-Lesung mit Mittelwertbildung (10-20 Samples)
- Kennlinie: Widerstand -> Fuellstand (nicht linear, Kalibrierung noetig)
- Reichweite = (aktueller Fuellstand * Verbrauch pro 100 km) / 100
- Verbrauch aus Kraftstoffstand-Aenderung + zurueckgelegter Strecke (gleitender Durchschnitt ueer letzte 50 km)

Die Transalp 600 hat einen 18-Liter-Tank. Der originale Standgeber ist ein Widerstandssensor. Die Kennlinie ist nicht linear (oben flach, unten steil) und muss empirisch kalibriert werden.

### 1.5 Uhrzeit (GPS-synced)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Must |

**Hardware:**
- GPS-Modul (bereits fuer Geschwindigkeit vorhanden)
- Optional: DS3231 RTC (Real Time Clock, I2C) als Fallback wenn GPS-Signal fehlt

**Software:**
- NMEA $GPRMC oder $GPZDA Sentence parsen -> UTC-Zeit
- Zeitzonen-Konvertierung (CET/CEST, DST-Regeln)
- RTC-Synchronisation alle paar Minuten bei GPS-Fix
- Anzeige im 24h-Format

Quelle:
- https://github.com/m-gracia/esp32-bike-computer-main (GPS-Zeit mit DST-Umstellung)

### 1.6 Tripcmeter (Tageskilometer, Tourkilometer)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Must |

**Hardware:**
- GPS-Modul (bereits vorhanden)
- Optional: Hall-Sensor am Vorderrad fuer hoehere Genauigkeit

**Software:**
- Trip A (Tageskilometer): Reset manuell oder bei naechster Fahrt (Stillstand > 8h)
- Trip B (Tourkilometer): Reset manuell
- Gesamtkilometer (ODO): Persistent in NVS (Non-Volatile Storage) gespeichert
- Berechnung aus GPS-Positionen (Haversine-Formel) oder Radumdrehungen (Hall-Sensor)

Die Haversine-Formel berechnet die Grosskreis-Distanz zwischen zwei GPS-Koordinaten:

```
d = 2 * R * arcsin(sqrt(sin²((lat2-lat1)/2) + cos(lat1) * cos(lat2) * sin²((lon2-lon1)/2)))
```

wobei R = 6371 km (Erdradius). Bei 1 Hz GPS-Update und typischen Motorradgeschwindigkeiten ist die Genauigkeit ausreichend (±2-5% Abweichung).

### 1.7 GPS-Track-Logging (GPX-Export)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach bis mittel |
| Prioritaet | Must |

**Hardware:**
- GPS-Modul (bereits vorhanden)
- MicroSD-Karten-Modul (SPI, CS auf GPIO5, ~2 EUR)
- MicroSD-Karte (8-32 GB, Class 10)

**Software:**
- GPX-Datei-Format (XML-basiert, Standard fuer GPS-Tracks)
- Schreiben alle 1-5 Sekunden (bei 1 Hz GPS-Update)
- Datei: `/tracks/YYYYMMDD_HHMMSS.gpx`
- GPX-Struktur:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="Transalp-Bordcomputer"
     xmlns="http://www.topografix.com/GPX/1/1">
  <trk>
    <name>Tour 2026-10-13</name>
    <trkseg>
      <trkpt lat="50.928" lon="11.583">
        <ele>215.0</ele>
        <time>2026-10-13T12:00:00Z</time>
      </trkpt>
      ...
    </trkseg>
  </trk>
</gpx>
```

- Kompatibel mit Komoot, Strava, OSMAnd, GPX Studio
- Puffer im RAM (Ring-Buffer 50-100 Punkte) um SD-Schreibzugriffe zu minimieren

Quellen:
- https://github.com/justcallmekoko/ESP32Marauder/wiki/GPS-Tracker (GPX-Logging auf SD)
- https://github.com/RaemondBW/OpenTrailPaper (E-Paper Bike Computer mit GPX/FIT)

---

## 2. Erweiterte Features (sollte man haben)

### 2.1 Ganganzeige

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Should |

**Hardware:** Keine zusaetzliche Hardware — verwendet RPM-Sensor (Abschnitt 1.2)
und Geschwindigkeit (GPS oder Hall-Sensor am Rad).

**Software:**
- Berechnung des Verhaeltnisses: `ratio = RPM / Radgeschwindigkeit`
- Jeder Gang hat ein charakteristisches Verhaeltnis (Verhaeltnis engine RPM zu
  Rad-Drehzahl), das als "Fingerabdruck" des Gangs dient
- Lern-Modus: Beim ersten Start faehrt der Rider jeden Gang (1-5) bei
  konstanter Geschwindigkeit und stabilen RPM fuer einige Sekunden. Die Firmware
  speichert die Verhaeltnisse in NVS.
- Im Betrieb: Vergleich des aktuellen Verhaeltnisses mit gespeicherten Werten,
  naechster Match = aktueller Gang
- Plausibilitaetspruefung: Kupplung gezogen oder Leerlauf -> Gang-Anzeige
  einfrieren oder "N" (Neutral) anzeigen
- Bei der PD06: 5-Gang-Getriebe, Kettenantrieb. Uebersetzungsverhaeltnisse sind
  fest definiert, aber die Kalibrierung ist genauer als feste Werte.

Diese Methode ist von "decogabry" als Nixie-Gangindicator fuer Motorraeder
dokumentiert (ESP32 + OBD fuer RPM + Radsensor). Fuer die PD06 muessen wir RPM
ueber Zuendimpulse und Geschwindigkeit ueber GPS/Hall erfassen, aber das
Prinzip ist identisch.

Quellen:
- https://hackster.io/news/a-self-learning-microcontroller-delivers-motorcycle-gear-indication-on-a-vintage-nixie-tube-9228f7171b25
- https://hackaday.com/2026/01/20/nixie-gear-indicator-shines-bright

### 2.2 Neigungsanzeige (Schaerglage, Roll/Pitch)

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Should |

**Hardware:** MPU6050 IMU (bereits in Hardware-Planung enthalten, ~$2)

**Software:**
- Roll-Winkel (Schaerglage) aus Beschleunigungssensor + Gyroskop
- Kalman-Filter fusioniert Beschleuniger (absoluter Referenzwinkel bei
  Stillstand, aber verrauscht bei Beschleunigung) mit Gyroskop (glaettet, aber
  driften ueber Zeit)
- Formel: `kalmanAngle += dt * (gyroX - kalmanBias); y = accRoll - kalmanAngle;`
- Anzeige als Balken oder Zahlenwert (-45 bis +45 Grad)
- Max-Lean-Recorder: speichert maximale Schaerglage links/rechts pro Tour
- Auto-Kalibrierung der Neutralposition beim Start (Motorrad steht gerade)

Die Herausforderung: Bei Kurvenfahrt wirkt Fliehkraft auf den Beschleuniger,
was den reinen Beschleunigungswert verfaelscht. Der Kalman-Filter loest dies,
indem er dem Gyroskop in dynamischen Phasen mehr Gewicht gibt und dem
Beschleuniger bei Niedrigbeschleunigung.

Quellen:
- https://github.com/Trexii/Arduino-Bike-Lean-Angle-meter-Kalman-filter- (MPU6050 + OLED)
- https://pmc.ncbi.nlm.nih.gov/articles/PMC8512544 (Roll Angle Estimation, Kalman Filter)
- https://www.computer.org/csdl/proceedings-article/cscs/2017/07968631/12OmNzdoMpP (Crash Detection mit IMU)

### 2.3 Hoehenmesser (barometrisch)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

**Hardware:** BMP280 barometrischer Drucksensor (bereits geplant, I2C, ~$2)

**Software:**
- BMP280 liefert Luftdruck in hPa und Temperatur
- Hoehenberechnung ueber barometrische Formel:
  `hoehe = 44330 * (1 - (druck / sealevel_druck)^(1/5.255))`
- Referenzdruck (sea level pressure) aus GPS-Hoehe beim Start kalibrieren oder
  aus Wetterdaten (falls ueber Smartphone verfuegbar)
- Hoehenmeter-Berechnung: Integration von dh/dt fuer Aufstieg/Abstieg
- Anzeige: aktuelle Hoehe (m ue. NN), Hohenmeter hoch/runter pro Tour

Genauigkeit: ca. +-1m bei kalibriertem Referenzdruck. Wetterbedingte
Luftdruckschwankungen koennen zu Fehlern von 10-20m fuehren, wenn der
Referenzdruck nicht regelmaessig aktualisiert wird.

Quellen:
- https://www.electronicwings.com/esp32/bmp280-barometer-sensor-interfacing-with-esp32
- https://github.com/kimsniper/bmp280 (ESP-IDF Implementation)

### 2.4 Umwelttemperatur

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

**Hardware:** BMP280 (bereits vorhanden) liefert Temperatur. Alternativ: DS18B20
als zusaetzlicher Sensor an sichtgeschuetzter Stelle (unter Verkleidung, nicht
in direkter Sonne).

**Software:**
- BMP280-Temperatur ist durch Eigenwaerme des Gehaeuses beeinflusst
- Korrektur: Offset-Kalibrierung (Vergleich mit Referenzthermometer)
- DS18B20 ist genauer fuer Aussentemperatur (separat montiert)
- Anzeige in Celsius, Warnung bei Frostgefahr (<3 Grad Celsius)

### 2.5 Oeldruck

| Attribut | Wert |
|---|---|
| Machbarkeit | aufwendig |
| Prioritaet | Should |

**Hardware:**
- Die PD06 hat eine Oeldruck-Warnleuchte (Schalter, der bei zu niedrigem Druck
  aufleuchtet). Dieser Schalter ist digital (ON/OFF), nicht analog.
- Fuer analogen Oeldruck: Oeldrucksensor mit analogem Ausgang (z.B. VDO
  0-5 bar, 10-180 Ohm), der anstelle des Originalschalters in den Oelkreislauf
  eingebaut wird. Benoetigt Adaptergewinde (M10x1, M14x1.5 etc., je nach PD06).
- Spannungsteiler + ADC zur Erfassung

**Software:**
- ADC-Lesung, Umrechnung Widerstand -> Druck (Kennlinie des Sensors)
- Trend-Logging: Oeldruck ueber Zeit, Erkennung von langsamem Abfall
- Warnung bei Druck < 0,5 bar bei Leerlauf oder < 1,5 bar bei hoeherer Drehzahl
- Bei nur digitalem Schalter: Warnung bei Oeldruck-Abfall (Warnleuchte an)

Die PD06 hat eine Oeldruckkontrolleuchte. Der Originalsensor ist ein
Einfach-Schalter (Schliesst bei Druckabfall). Fuer eine analoge Anzeige muss
ein zusaetzlicher Sensor eingebaut werden, was mechanische Modifikation am
Oelkreislauf erfordert. Mit Vorsicht zu geniessen.

### 2.6 Bordspannung (Generator-Status)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

**Hardware:** Spannungsteiler an der 12V-Bordnetzleitung (z.B. 100kOhm + 33kOhm
ergibt max 3,3V bei 13V Bordnetz), ADC-Pin.

**Software:**
- ADC-Lesung, Umrechnung in Volt (unter Beruecksichtigung der Teilerverhaeltnisse)
- Mittelung ueber 10-50 Samples (Bordnetz ist bei laufendem Motor stark
  verrauscht durch Generator und Zuendung)
- Diagnose: Spannung < 12V bei laufendem Motor -> Generator/Regler defekt.
  Spannung > 14,5V -> Regler ueberlaedt (Gefahr fuer Akku)
- Trend-Logging fuer Langzeitdiagnose
- Anzeige: Bordspannung in Volt, Warnung bei < 11,5V oder > 15V

### 2.7 Verbrauchs-Logging

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Should |

**Hardware:** Kraftstoffstandsensor (Abschnitt 1.4) + GPS (Strecke)

**Software:**
- Verbrauch = Delta_Kraftstoffstand / Delta_Strecke * 100 (l/100km)
- Gleitender Durchschnitt ueer letzte 50-100 km (Kraftstoffstand ist zu
  verrauscht fuer Echtzeit-Verbrauch)
- Langzeit-Trend: Verbrauch pro Tour, pro Woche, pro Monat
- Vergleich mit historischen Daten (Wird der Motor durstiger?)

Die Genauigkeit ist begrenzt durch die Aufloesung des Kraftstoffstand-Sensors
(ca. 0,5-1 Liter). Bei 17,5L Tank und 5 l/100km Verbrauch entspricht das ca.
5-10% Fehlertoleranz pro Messung. Mit Mittelung ueber mehrere Tankfuellungen
wird es brauchbar.

### 2.8 Fahrtzeit, Pause-Erkennung

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

**Software:**
- Fahrtzeit: Zeitspanne zwischen erstem und letztem GPS-Bewegungsereignis
- Pause-Erkennung: GPS-Geschwindigkeit < 2 km/h fuer > 60 Sekunden -> Pause
- Pausezeit: Summe aller Pausen
- Reine Fahrzeit = Tourzeit - Pausezeit
- Anzeige: Fahrtzeit, Pausezeit, Gesamtzeit
- Persistenz: Letzte Tour in NVS gespeichert

### 2.9 Max-Speed / Max-RPM-Recorder

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

**Software:**
- Speichert Maximum von Geschwindigkeit und RPM pro Tour
- Reset bei Tour-Start (oder manuell)
- Anzeige als "Peak" in der Tour-Zusammenfassung
- Optional: Timestamp des Peaks (fuer spaetere Korrelation mit Track)

---

## 3. Smart-Features (Bluetooth + Smartphone)

### 3.1 Turn-by-Turn Navigation (BLE -> Smartphone -> Bordcomputer)

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Should |

**Hardware:**
- ESP32-S3 BLE (eingebaut)
- Smartphone als Routing-Backend

**Software:**
- BLE GATT-Service fuer Navigation (Custom UUID)
- Smartphone-App sendet: Maneuver-Typ (links/rechts/geradeaus/Rundkehr), Distanz zum Maneuver, Strassenname, naechstes Maneuver
- Darstellung auf E-Paper: grosser Pfeil, Distanz, Strassenname
- Kein Karten-Rendering auf dem Geraet (zu ressourcenintensiv)
- Protokoll: JSON ueber BLE (chunked, ~200 Bytes pro Update)

Das Konzept orientiert sich an Geraeten wie Beeline Moto oder dem SteedPilot-Projekt. Das Smartphone berechnet die Route (eine gängige Navigations-App, Komoot, OsmAnd) und sendet nur die naechste Anweisung an den Bordcomputer. Das Display zeigt Pfeil + Distanz + Strassennamen.

Quellen:
- https://github.com/deanthecoder/SteedPilot (ESP32 + Smartphone, kreisrundes HUD)
- https://github.com/rabbihossain/BikeNavESP32 (ESP32 Navigation mit BLE JSON-Protokoll)

### 3.2 Wettervorhersage (Regenwarnung, Temperatur)

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Could |

**Software:**
- Smartphone-App ruft Wetter-API auf (OpenWeatherMap, DWD)
- BLE-Uebertragung: Wetterzustand, Regenwahrscheinlichkeit, Temperatur in 1h/3h
- Bordcomputer zeigt: Wetter-Icon + Regenwarnung
- Alternative: Bordcomputer mit WiFi verbindet sich direkt mit API (nur bei Stillstand/WLAN)

### 3.3 Incoming-Call-Anzeige (Caller-ID)

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Could |

**Hardware:**
- ESP32-S3 BLE

**Software:**
- mobiles BS (kalifornischer Technologiehersteller): ANCS (Notifizierungs-Service eines kalifornischen Technologieherstellers) - native BLE-Integration, keine App noetig
- mobiles BS (anderes Ökosystem): Custom BLE-App, die Telefonie-Events abfaengt und ueber BLE sendet
- Anzeige: Anrufername/Nummer auf Display, optional Accept/Reject-Taste

Der iKawa-Projekt zeigt, dass ANCS auf dem ESP32 funktioniert: Nach einmaligem Pairing mit dem Smartphone (kalifornischer Technologiehersteller) werden alle Benachrichtigungen automatisch uebertragen. Fuer das andere mobilele BS ist eine Companion-App noetig.

Quellen:
- https://github.com/gabrielerandazzo/iKawa (ANCS + AMS Integration)
- https://www.electroniclinic.com/ble-call-system-for-motorcycles-using-esp32-s3-touch-display/ (BLE Call System)

### 3.4 Music-Track-Info (Artist/Title)

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Could |

**Software:**
- mobiles BS (kalifornischer Technologiehersteller): AMS (Media-Service eines kalifornischen Technologieherstellers) - Track-Info, Artist, Titel, Playback-Steuerung
- mobiles BS (anderes Ökosystem): Custom BLE-App mit MediaSessionManager
- Anzeige: Artist + Titel auf Display (scrollend bei langen Namen)
- Optional: Previous/Next/Play-Pause ueber Tasten am Bordcomputer

Quelle:
- https://github.com/gabrielerandazzo/iKawa (AMS-Integration fuer Media-Controls)

### 3.5 SMS/Notification-Preview

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Could |

**Software:**
- mobiles BS (kalifornischer Technologiehersteller): ANCS (wie bei Call-Anzeige)
- mobiles BS (anderes Ökosystem): NotificationListenerService -> BLE-Weiterleitung
- Anzeige: Absender + erste Zeile der Nachricht
- Optional: Vorlesen der Nachricht via TTS an BT-Kopfhoerer

### 3.6 POI-Logging (Wegpunkte mit Taste markieren)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

**Hardware:**
- Taster an GPIO (INPUT_PULLUP, entprellt in Software)

**Software:**
- Tastendruck -> aktuelle GPS-Position als Waypoint speichern
- Datei: `/poi/YYYYMMDD.gpx` mit Waypoints
- Waypoint-Format: Name (Timestamp), Lat, Lon, Alt, Beschreibung (optional)
- Spaetere Abfrage der Waypoints ueber Smartphone-App oder SD-Karte

---

## 4. Safety-Features

### 4.1 Crash-Erkennung (IMU: ploetzliche Verzoegerung)

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Should |

**Hardware:**
- MPU6050 (bereits fuer Neigungsanzeige vorhanden) oder BNO055
- Smartphone fuer Notfall-Kommunikation (BLE)

**Software:**
- Kontinuierliche Ueberwachung der Beschleunigung (100 Hz)
- Crash-Kriterium: Beschleunigung > 7g fuer > 100ms (Kollision) UND Neigung > 75° fuer > 500ms (Umkippen bestaetigt)
- Multi-Stage-Verifikation zur Reduzierung von Fehlalarmen:
  1. Beschleunigungsschwelle ueberschritten -> Verdacht
  2. Neigung fuer 3-5 Sekunden bestaetigt -> Crash bestaetigt
  3. Countdown 10 Sekunden -> Abbruch durch Tastendruck (Fehlalarm)
  4. Wenn nicht abgebrochen: SOS senden
- Algorithmen aus der Forschung: Euclidean norm der Beschleunigung, zeitbasierte Schwellwertfilterung

Die Forschung zeigt, dass ein Mehrstufiger Algorithmus noetig ist: Bloße Beschleunigungsschwellen loesen Fehlalarme bei Schlagloechern oder hartem Bremsen aus. Die Kombination aus Beschleunigungsspitze + anhaltender Fehlneigung ist zuverlaessig.

Quellen:
- https://github.com/TrieuHzang/iot-motorcycle-accident-detection-system (ESP32 + MPU6050 + GPS, Blynk Cloud)
- https://ijraset.com/best-journal/realtime-two-wheeler-accident-detection-with-automated-emergency-notification-and-post-impact-voice-logging-using-iot (7g/100ms Schwelle, 75° Neigung, SIM800L)

### 4.2 Automatic SOS (SMS/Anruf bei Crash + GPS-Position)

| Attribut | Wert |
|---|---|
| Machbarkeit | aufwendig |
| Prioritaet | Should |

**Hardware:**
- Smartphone (BLE) als Kommunikationskanal
- Alternative: SIM800L GSM-Modul (unabhaengig vom Smartphone, aber zusaetzliche Hardware + SIM-Karte noetig)

**Software (Smartphone-Weg):**
- Bei bestaetigtem Crash: BLE-Befehl an Smartphone-App
- App sendet SMS mit Navigations-App-Link an Notfallkontakt
- Optional: Automatischer Anruf

**Software (SIM800L-Weg):**
- AT-Befehle: AT+CMGS (SMS senden), ATD (Anruf)
- GPS-Koordinaten in SMS-Text einbetten
- Vorteil: Funktioniert auch ohne Smartphone
- Nachteil: Zusaetzliche Hardware, Stromverbrauch, SIM-Kosten

Der Smartphone-Weg ist einfacher und ausreichend, solange das Handy gepaart ist. Der SIM800L-Weg ist das redundante Fallback fuer Alleinfaahrer ohne Smartphone-Verbindung.

Quellen:
- https://github.com/aamani-c/Accident-detection-and-notification-system. (Telegram Bot Notifikation)
- https://rectronx.com/blog/smart-helmet-safety-monitoring-esp32 (Smart Helmet mit SIM800L + Countdown)

### 4.3 Sturz-Aufzeichnung (letzten 30s IMU-Daten sichern)

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Could |

**Software:**
- Ring-Buffer im RAM: 30 Sekunden IMU-Daten bei 50 Hz = 1500 Samples
- Bei Crash-Erkennung: Ring-Buffer auf SD-Karte sichern
- Format: CSV mit Timestamp, Ax, Ay, Az, Gx, Gy, Gz
- Nutzen: Unfallanalyse, Versicherungsbelege

### 4.4 Geofencing (Warnung bei Verlassen eines Bereichs)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Could |

**Software:**
- Konfigurierbarer Bereich: Mittelpunkt (Lat/Lon) + Radius (km)
- Ueberpruefung alle 10 Sekunden
- Warnung wenn GPS-Position ausserhalb des Radius
- Anwendungsfaelle: Diebstahlschutz, Reisebegrenzung, Gruppenfahrt

### 4.5 Diebstahl-Warnung (IMU-Bewegung bei ausgeschaltetem Bordcomputer)

| Attribut | Wert |
|---|---|
| Machbarkeit | aufwendig |
| Prioritaet | Could |

**Hardware:**
- IMU (MPU6050) im Low-Power-Modus (Wake-on-Motion)
- ESP32 Deep-Sleep mit ULP-Coprozessor oder RTC-GPIO-Wake

**Software:**
- Bei ausgeschalteter Zuendung: ESP32 in Deep-Sleep, IMU aktiv
- Bei Bewegungserkennung: ESP32 aufwachen, BLE-Verbindung zu Smartphone aufbauen
- Push-Benachrichtigung an Smartphone-App
- Alternative: SIM800L fuer SMS-Warnung unabhaengig vom Smartphone

Der ESP32-S3 unterstuetzt Deep-Sleep mit sehr niedrigem Stromverbrauch (~10 uA). Die IMU kann im Low-Power-Modus laufen und den ESP32 bei Bewegung wecken. Die Reichweite von BLE ist jedoch begrenzt (~30m), sodass die Warnung nur funktioniert, wenn das Smartphone in der Naehe ist.

---

## 5. Tour-Features

### 5.1 Track-Recording mit Telemetrie

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Should |

**Software:**
- Erweitertes GPX-Logging mit zusaetzlichen Datenpunkten
- Telemetrie-Daten: RPM, Geschwindigkeit, Neigung, Temperatur, Bordspannung
- Sampling-Rate: 1 Hz (GPS) bis 10 Hz (IMU/RPM)
- Format: GPX mit Extensions oder zusaetzliche CSV-Datei
- GPX-Extensions Beispiel:

```xml
<trkpt lat="50.928" lon="11.583">
  <ele>215.0</ele>
  <time>2026-10-13T12:00:00Z</time>
  <extensions>
    <bordcomputer:rpm>3500</bordcomputer:rpm>
    <bordcomputer:roll>5.2</bordcomputer:roll>
    <bordcomputer:temp>88</bordcomputer:temp>
    <bordcomputer:volt>13.8</bordcomputer:volt>
  </extensions>
</trkpt>
```

- Alternative: Separate CSV-Datei parallel zum GPX-Track
- Datei: `/telemetry/YYYYMMDD_HHMMSS.csv` mit Spalten: timestamp, lat, lon, alt, speed, rpm, roll, pitch, temp, volt

### 5.2 Export als GPX/KML/CSV nach Tour

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

**Software:**
- GPX: Standard-Export (bereits implementiert, siehe 1.7)
- KML: Geodaten-Format eines Suchmaschinenbetreibers, einfache Konvertierung aus GPX
- CSV: Rohdaten-Export fuer eigene Analysen (Excel, Python, R)
- Konvertierung beim Export oder in Echtzeit auf SD-Karte

### 5.3 Tour-Statistiken

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Should |

**Software:**
- Hoehenmeter: Summe aller positiven Hoehenunterschiede (barometrisch, siehe 2.3)
- Durchschnittsgeschwindigkeit: Gesamtdistanz / Fahrtzeit (ohne Pausen)
- Fahrtzeit / Pausenzeit / Gesamtzeit (siehe 2.8)
- Maximale Geschwindigkeit / RPM (siehe 2.9)
- Kraftstoffverbrauch der Tour (siehe 2.7)
- Anzeige auf Display bei Tour-Ende (Zusammenfassung)
- Persistente Speicherung pro Tour in JSON auf SD-Karte

### 5.4 Segment-Timing (Pass-Zeit, Sektor-Zeiten)

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Could |

**Software:**
- Definition von Segmenten (Start-/Endpunkt als GPS-Koordinaten)
- Timer startet beim Ueberfahren des Startpunkts
- Timer stoppt beim Ueberfahren des Endpunkts
- Vergleich mit Bestzeit (Delta-Anzeige)
- Speicherung der Bestzeiten pro Segment in NVS
- Anwendungsfaelle: Passfahrten, Lieblingsstrecken, Trackdays

### 5.5 Rueckweg-Navigation ("Back-Track")

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Could |

**Software:**
- Aufzeichnung der gefahrenen Route als Liste von Wegpunkten
- Ruecknavigation: Wegpunkte in umgekehrter Reihenfolge abfahren
- Anzeige: Pfeil + Distanz zum naechsten Waypoint (wie bei Turn-by-Turn)
- Berechnung: Peilung zum naechsten Waypoint vs. aktuelle Heading (GPS-Course)
- Pfeilrichtung = relative Peilung

Dies ist das einfachste Navigationskonzept ohne Smartphone. Der Bordcomputer merkt sich die Route und fuehrt zurueck. Funktioniert auch abseits von Strassen (Enduro-Fahrten).

### 5.6 POI-Marking

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

Siehe 3.6 (POI-Logging). Identische Implementierung, hier als Tour-Feature eingeordnet.

---

## 6. Diagnose-Features (PD06-spezifisch)

### 6.1 Oeldruck-Trend

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Should |

**Software:**
- Langzeit-Logging des Oeldrucks (analoger Sensor vorausgesetzt, siehe 2.5)
- Aufzeichnung alle 10 Sekunden auf SD-Karte
- Trend-Analyse: Druckverlauf ueber letzten 1000 km
- Warnung bei trendmaessigem Abfall (gleitender Durchschnitt sinkt ueber 500 km um > 0.3 bar)
- Oelverduennung oder Verschleiss an der Oelpumpe fruehzeitig erkennbar

### 6.2 Kuehlmitteltemp-Trend (Ueberhitzungswarnung)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

**Software:**
- Langzeit-Logging der Kuehlmitteltemperatur
- Aufzeichnung alle 10 Sekunden
- Warnung bei: Temperatur steigt > 2°C pro Minute (Ueberhitzung droht)
- Warnung bei: Durchschnittstemperatur steigt ueber mehrere Fahrten (Kuehlerschaetzung, Thermostat defekt)
- PD06-spezifisch: Luftgekuehlter V2, Kuehlmittel fuer Oelkuehler. Bei Bergfahrten kritisch.

### 6.3 Bordspannung-Trend (Regler/Generator-Probleme)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

**Software:**
- Langzeit-Logging der Bordspannung
- Aufzeichnung alle 30 Sekunden
- Warnung bei: Spannung faellt bei laufendem Motor unter 13.5V (Generator/Regler)
- Warnung bei: Spannung steigt ueber 15V (Regler defekt, Ueberladung)
- Trend-Analyse: Durchschnittsspannung bei Leerlauf vs. Fahrt ueber Zeit
- Bekanntes PD06-Problem: Lichtmaschinenregler-Ausfall

### 6.4 Kraftstoffverbrauch-Historie

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Could |

**Software:**
- Verbrauch pro Tour speichern (siehe 2.7)
- Langzeit-Trend: Verbrauch ueber 1000 km (gleitender Durchschnitt)
- Korrelation mit: Aussentemperatur, Geschwindigkeit, Streckentyp
- Erkennung von Veraenderungen (z.B. hoeherer Verbrauch = Luftfilter zu? Reifendruck niedrig?)

### 6.5 Wartungs-Reminder (Oelwechsel-Intervall nach km/Zeit)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

**Software:**
- Konfigurierbare Intervalle: Oelwechsel (alle 6000 km oder 12 Monate)
- Zaehler: Kilometer seit letztem Oelwechsel (aus GPS/Trip)
- Zaehler: Tage seit letztem Oelwechsel
- Warnung bei Erreichen von 90% des Intervalls
- Reset manuell nach durchgefuehrter Wartung
- Weitere Intervalle: Reifen, Bremsfluessigkeit, Kettensatz
- Speicherung in NVS (persistent)

---

## 7. Nice-to-Have / Experimentell

### 7.1 Reifendruck-Ueberwachung (TPMS)

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Could |

**Hardware:**
- BLE-TPMS-Sensoren (z.B. ZEEPIN TP630, ~20-30 EUR pro Paar)
- Ersatz der Ventilkappen durch TPMS-Sensoren
- ESP32-S3 BLE empfängt Advertising-Daten (passiv, keine Pairing noetig)

**Software:**
- BLE-Scanner: Advertising-Daten der TPMS-Sensoren abfangen
- Datenstruktur (ZEEPIN-Format): Manufacturer Data mit Druck (kPa), Temperatur (°C), Batterie (%)
- Parsing: Little-Endian Konvertierung, Bytes 8-11 = Druck, Bytes 12-15 = Temperatur
- Anzeige: Druck (bar/PSI) und Temperatur pro Reifen
- Warnung bei: Druck < 2.0 bar oder > 3.0 bar, Temperatur > 80°C

Die TPMS-Sensoren senden passiv als BLE-Beacons. Der ESP32 muss nur zuhoeren, keine Verbindung noetig. Reichweite ausreichend fuer Motorrad.

Quellen:
- https://github.com/ra6070/BLE-TPMS (ESP32 BLE TPMS ZEEPIN-Sensoren)
- https://github.com/andi38/TPMS (TPMS 433MHz + BLE Dekodierung)
- https://github.com/m-gracia/esp32-bike-computer-main (TPMS BLE im Bike Computer)

### 7.2 Radgeschwindigkeit separat (Hall-Sensor am Vorderrad)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Could |

**Hardware:**
- Hall-Sensor (A3144) an der Gabel, Magnet auf Bremsscheibe
- 1-4 Magnete fuer hoehere Aufloesung

**Software:**
- Interrupt-basierte Pulsmessung
- Geschwindigkeit = (Pulse / PPR) * Radumfang * (3600 / Zeitfenster)
- Genauer als GPS bei niedrigen Geschwindigkeiten (< 10 km/h)
- Fallback bei GPS-Ausfall (Tunnel, Garage)
- Odometer aus Zaehlerstand ableitbar

Quelle:
- https://forum.arduino.cc/t/motorcycle-data-monitoring-project-need-guidance/85954

### 7.3 GPS-basierte Beschleunigungsmessung (0-100 km/h Timer)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Could |

**Software:**
- Timer startet bei Geschwindigkeit > 0 km/h
- Stoppt bei Erreichen von 100 km/h (oder konfigurierbar: 0-60, 0-80)
- Anzeige: Zeit in Sekunden
- Bestzeitspeicher
- Hinweis: GPS-basierte Beschleunigung ist ungenauer als IMU-basierte (GPS-Update-Rate limitiert)

### 7.4 Lean-Angle-Max-Recorder

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Could |

**Software:**
- Maximale Schraeglage (Roll) pro Tour speichern (siehe 2.2)
- Links und Rechts getrennt
- Anzeige in Tour-Zusammenfassung
- Persistente Bestzeitspeicherung in NVS

### 7.5 Audio-Warnungen (Piezo-Buzzer)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Could |

**Hardware:**
- Aktiver Piezo-Buzzer an GPIO (z.B. GPIO15)
- Alternativ: passiver Buzzer mit PWM fuer Melodien

**Software:**
- Unterschiedliche Warntoene fuer verschiedene Ereignisse:
  - Kuehlmitteltemperatur kritisch: kontinuierlicher Ton
  - Oeldruck kritisch: intermittierender Ton
  - Kraftstoffreserve: doppelter Piep
  - Ueberdrehzahl: einzelner Piep bei Schaltdrehzahl
  - Bordspannung kritisch: dreifacher Piep

### 7.6 LED-Status-Strip (RGB-LED fuer Warnungen)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Could |

**Hardware:**
- WS2812B / NeoPixel LED-Strip (adressierbar, 1-Pin, 5V)
- Pegelwandler fuer Data-Pin

**Software:**
- Farb-Codierung: Gruen (alles OK), Gelb (Warnung), Rot (Kritisch)
- Verschiedene LEDs fuer verschiedene Sensoren
- Blinken bei kritischen Warnungen

### 7.7 Zweites Display am Lenkerende

| Attribut | Wert |
|---|---|
| Machbarkeit | aufwendig |
| Prioritaet | Won't |

Zusaetzliches kleines E-Paper-Display nur fuer Warnungen. Won't wegen Aufwand/Nutzen-Verhaeltnis. Das Hauptdisplay sollte Warnungen prominent genug anzeigen.

### 7.8 Sprachausgabe via BT-Kopfhoerer (in Helm)

| Attribut | Wert |
|---|---|
| Machbarkeit | aufwendig |
| Prioritaet | Won't |

ESP32-S3 unterstuetzt kein Bluetooth Classic (A2DP). TTS auf ESP32 ist ressourcenintensiv. Die Smartphone-Route (App macht TTS, sendet an BT-Kopfhoerer) ist vorzuziehen, aber dann kein Bordcomputer-Feature mehr.

---

## 8. Daten-Export und Analyse

### 8.1 GPX-Track (Standard)

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Must |

**Software:**
- GPX 1.1 Format (siehe 1.7)
- Kompatibel mit Komoot, Strava, OSMAnd, GPX Studio, Fitness-Tracking-Plattform (Referenz)
- Ein Datei pro Tour
- Metadata: Datum, Tour-Name, Gesamt-Distanz, Fahrtzeit

### 8.2 CSV-Telemetrie

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

**Software:**
- Rohdaten-Export als CSV mit hoher Aufloesung (10 Hz fuer IMU/RPM, 1 Hz fuer GPS)
- Spalten: timestamp, lat, lon, alt, speed, heading, rpm, roll, pitch, temp_coolant, temp_ambient, volt, fuel_level
- Nutzen: Eigene Analyse in Python/Pandas, Excel, R
- Datei: `/telemetry/YYYYMMDD_HHMMSS.csv`

### 8.3 JSON-API ueber BLE (Live-Daten an Smartphone-App)

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Should |

**Software:**
- BLE GATT-Characteristic mit Notify-Funktion
- JSON-Payload mit allen Live-Daten (~200 Bytes, Update alle 1 Sekunde)
- Protokoll:

```json
{
  "speed": 85.2,
  "rpm": 4200,
  "gear": 5,
  "temp_coolant": 88,
  "temp_ambient": 18,
  "volt": 13.8,
  "fuel": 12.5,
  "roll": 5.2,
  "pitch": -1.1,
  "alt": 215,
  "lat": 50.928,
  "lon": 11.583,
  "trip_a": 142.3,
  "odo": 45821
}
```

- Smartphone-App kann Live-Daten anzeigen, loggen, weiterleiten
- Alternative: Web Bluetooth API (PWA) direkt im Browser, keine App noetig

Quellen:
- https://github.com/HamzaYslmn/Esp-BLE-Web (Web Bluetooth PWA)
- https://github.com/dheera/ESP32-Web-Bluetooth-Terminal (BLE Terminal PWA)

### 8.4 Web-Dashboard (nach Tour)

| Attribut | Wert |
|---|---|
| Machbarkeit | aufwendig |
| Prioritaet | Could |

**Software:**
- Statische HTML/JS-Datei auf SD-Karte
- Import der Telemetrie-CSV/JSON
- Visualisierung: Geschwindigkeitsverlauf, Hohenprofil, RPM-Verteilung, Verbrauchskurve
- Karten-Overlay mit GPX-Track (OpenStreetMap Tiles, offline)
- Kein Server noetig: Pure Client-Side JavaScript
- Alternative: Export zu Cloud-Dienst (Strava, Komoot) via Smartphone-App

### 8.5 Statistik-Uebersicht auf dem Display

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

**Software:**
- Tour-Ende-Zusammenfassung auf dem E-Paper-Display
- Anzeige: Distanz, Fahrtzeit, Durchschnittsgeschwindigkeit, Hoehenmeter, Max-Speed, Max-RPM, Verbrauch, Max-Schraeglage
- Speicherung als Zusammenfassung in JSON auf SD-Karte
- Auto-Reset bei naechster Tour (oder manuell)

---

## 9. Smartphone-App-Konzept

### 9.1 Braucht man eine eigene App?

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Should |

**Antwort:** Ja, fuer die meisten Smart-Features. Ausnahmen:

- Benachrichtigungen des kalifornischen Technologieherstellers (Calls, SMS, Music): ANCS/AMS sind Standard-BLE-Protokolle, keine App noetig nach einmaligem Pairing
- Navigation: Erfordert eine App, die Routing-Daten an den Bordcomputer sendet (ausser der Bordcomputer macht Back-Track-Navigation)
- Konfiguration: Kann ueber Web-BLE (PWA) geloest werden, keine native App noetig
- Live-Daten-Streaming: PWA mit Web Bluetooth API moeglich

**Empfehlung:** PWA (Progressive Web App) als primaere Loesung. Native App nur fuer Features, die PWA nicht abdecken kann (z.B. Notification-Listener des anderen mobilen BS, Hintergrund-BLE-Kommunikation).

### 9.2 React Native / Flutter / native

| Attribut | Wert |
|---|---|
| Machbarkeit | aufwendig |
| Prioritaet | Could |

**Vergleich:**

| Option | Vorteile | Nachteile |
|---|---|---|
| PWA (Web Bluetooth) | Keine Installation, plattformunabhaengig, schnell entwickelt | Safari des kalifornischen Technologieherstellers unterstuetzt kein Web Bluetooth (Workaround: Bluefy Browser), keine Hintergrund-BLE |
| React Native | Cross-platform, grosse Community, BLE-Bibliotheken verfuegbar | Overhead, native Module fuer BLE noetig |
| Flutter | Cross-platform, gute Performance, BLE-Bibliotheken | Weniger BLE-Bibliotheken als RN, groessere App |
| Native (Swift/Kotlin) | Beste Performance, volle BLE-Kontrolle, ANCS/AMS nativ | Doppelter Entwicklungsaufwand (mobile BS A + mobile BS B) |

**Empfehlung:** PWA fuer Konfiguration und Live-Daten. Bei Bedarf spaeter native App fuer Navigation-Relay und Benachrichtigungen. Die PWA kann Web Bluetooth direkt ansprechen.

Quelle:
- https://github.com/HamzaYslmn/Esp-BLE-Web (PWA mit Widget-Katalog, Auto-Discovery)
- https://github.com/dheera/ESP32-Web-Bluetooth-Terminal (PWA BLE Terminal)

### 9.3 BLE-Protokoll-Design (GATT-Services)

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Must |

**GATT-Service-Struktur:**

| Service | UUID | Characteristics |
|---|---|---|
| Device Info | 0x180A (Standard) | Manufacturer Name, Model Number, Firmware Rev, Battery Level |
| Bordcomputer Data | Custom 128-bit | Live Data (Notify, ~200 Bytes, 1 Hz), Config (Read/Write) |
| Navigation | Custom 128-bit | Nav State (Write), Nav Command (Write) |
| POI | Custom 128-bit | POI List (Read), POI Add (Write) |
| File Transfer | Custom 128-bit | File List (Read), File Download (Read, chunked) |

**Protokoll-Design:**
- Alle Daten als JSON (menschenlesbar, einfach zu debuggen)
- Chunking fuer Payloads > MTU (Default 23 Bytes, negotiate bis 247 Bytes)
- Notify fuer Live-Daten (Bordcomputer pusht an App)
- Write fuer Befehle (App sendet an Bordcomputer)
- Read fuer statische Daten (Konfiguration, Dateiliste)

### 9.4 Was macht die App?

| Funktion | Beschreibung | Prioritaet |
|---|---|---|
| Konfiguration | Einstellungen des Bordcomputers (Schwellwerte, Intervalle, Display-Layout) | Must |
| Navigation-Relay | Route vom Smartphone an Bordcomputer senden | Should |
| Live-Daten | Live-Anzeige aller Sensordaten auf dem Smartphone | Should |
| Import/Export | GPX/CSV-Dateien vom Bordcomputer herunterladen | Should |
| Wartungs-Log | Wartungs-Intervalle zuruecksetzen, Historie ansehen | Could |
| Firmware-Update | OTA-Update des Bordcomputers ueber BLE | Could |
| Crash-Notification | Empfang von SOS-Nachrichten bei Crash | Should |

### 9.5 Alternative: Web-App (PWA) mit Web-BLE

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Should |

**Vorteile:**
- Keine App-Store-Freigabe noetig
- Plattformunabhaengig (Chrome, Edge, Opera auf diversen mobilen BS / macOS/Windows/Linux)
- Installierbar als PWA (Home-Screen-Icon, Offline-Modus)
- Schnelle Entwicklung (React/Vue/vanilla JS)

**Nachteile:**
- Safari des kalifornischen Technologieherstellers unterstuetzt kein Web Bluetooth (Workaround: Bluefy Browser)
- Keine Hintergrund-BLE-Kommunikation (App muss im Vordergrund sein)
- Kein Zugriff auf Notification-Listener des anderen mobilen BS (fuer SMS/Call-Weiterleitung)

**Empfehlung:** PWA als primaere Loesung fuer Konfiguration, Live-Daten und Import/Export. Fuer Navigation und Benachrichtigungen eine leichtgewichtige native App (oder Integration in bestehende Navigations-Apps wie OsmAnd ueber Intent/URL-Scheme).

Quellen:
- https://github.com/HamzaYslmn/Esp-BLE-Web (PWA mit Auto-Discovery Widget-Katalog)
- https://github.com/dheera/ESP32-Web-Bluetooth-Terminal (PWA BLE Terminal)

---

## 10. Erweiterbarkeit

### 10.1 Offene Architektur: eigene Sensoren per I2C/SPI

| Attribut | Wert |
|---|---|
| Machbarkeit | mittel |
| Prioritaet | Should |

**Software:**
- Modulares Sensor-Interface: Jeder Sensor ist ein eigenes Modul mit init(), read(), config()
- Sensoren registrieren sich beim Sensormanager
- I2C-Bus: BMP280, MPU6050, BME280 bereits belegt (Adressen 0x76, 0x68, 0x77)
- Zusaetzliche I2C-Sensoren moeglich (z.B. SCD41 CO2, SHT31 Temperatur/Feuchte)
- SPI-Bus: SD-Karte belegt, zusaetzliche SPI-Sensoren moeglich mit eigenem CS-Pin
- Analog: ADC-Pins fuer analoge Sensoren (0-3.3V)

**Hardware-Erweiterungen:**
- I2C-Hub (z.B. TCA9548A) bei Adresskonflikten
- ADC-Expander (z.B. ADS1115, 16-Bit, 4 Kanal) fuer mehr analoge Sensoren

### 10.2 Plugin-System fuer UI (Custom-Screens)

| Attribut | Wert |
|---|---|
| Machbarkeit | aufwendig |
| Prioritaet | Could |

**Software:**
- UI-Screens als Module (Aehnlich wie LVGL Screen-Objekte)
- Standard-Screens: Dashboard, Navigation, Tour-Statistik, Diagnose
- Custom-Screens: Benutzerdefinierte Layouts (z.B. nur RPM + Gang, oder nur Tour-Statistik)
- Screen-Wechsel ueber Tasten oder Touch
- Konfiguration ueber JSON-Datei auf SD-Karte

### 10.3 Konfiguration ueber YAML/JSON auf SD-Karte

| Attribut | Wert |
|---|---|
| Machbarkeit | einfach |
| Prioritaet | Should |

**Software:**
- Konfigurationsdatei: `/config/bordcomputer.json` auf SD-Karte
- Parameter: Warn-Schwellen, Display-Layout, Sensorkalibrierung, WLAN-Credentials
- Bei Aenderung: Neustart oder Hot-Reload
- Alternative: Konfiguration ueber BLE (Smartphone-App/PWA)

Beispiel-Konfiguration:

```json
{
  "display": {
    "brightness": 80,
    "screens": ["dashboard", "nav", "tour", "diag"],
    "default_screen": "dashboard"
  },
  "sensors": {
    "coolant_warn": 110,
    "coolant_crit": 115,
    "oil_warn": 0.5,
    "volt_low": 12.0,
    "volt_high": 14.8,
    "rpm_shift": 6500
  },
  "gps": {
    "update_rate": 1,
    "track_interval": 5
  },
  "trip": {
    "auto_pause_speed": 5,
    "auto_pause_time": 180
  }
}
```

### 10.4 API fuer externe Anbindung

| Attribut | Wert |
|---|---|
| Machbarkeit | aufwendig |
| Prioritaet | Could |

**Software:**
- REST-API ueber WiFi (wenn Bordcomputer im WLAN ist)
- Endpunkte: `/api/data` (Live-Daten), `/api/tracks` (Liste), `/api/tracks/{id}` (Download)
- MQTT-Publisher: Live-Daten an MQTT-Broker (z.B. Home Assistant)
- Webhook bei Crash-Erkennung
- Alternative: BLE-GATT als API (wie in 8.3 beschrieben)

Moegliche Integrationen:
- Home Assistant: Sensor-Entities fuer Temperatur, Spannung, etc.
- Eigene Dashboards: Grafana mit InfluxDB als Datenbank
- Strava/Komoot: Automatischer Upload der GPX-Tracks
- Wenn Bordcomputer WiFi aktiviert (bei Stillstand): automatische Synchronisation

---

## 11. Priorisierungsmatrix

| Feature | Machbarkeit | Prioritaet |
|---|---|---|
| Geschwindigkeit (GPS) | einfach | Must |
| RPM (Zündspule) | mittel | Must |
| Kuehlmitteltemperatur | einfach | Must |
| Kraftstoffstand | mittel | Must |
| Uhrzeit (GPS) | einfach | Must |
| Tripcmeter | einfach | Must |
| GPX-Logging | einfach | Must |
| Bordspannung | einfach | Should |
| Ganganzeige (berechnet) | mittel | Should |
| Neigungsanzeige (IMU) | mittel | Should |
| Hoehenmesser (BMP280) | einfach | Should |
| Umwelttemperatur | einfach | Should |
| Oeldruck (analog) | aufwendig | Should |
| Verbrauchs-Logging | mittel | Should |
| Fahrtzeit/Pause | einfach | Should |
| Max-Recorder | einfach | Should |
| Turn-by-Turn Navigation | mittel | Should |
| POI-Logging | einfach | Should |
| Crash-Erkennung | mittel | Should |
| Automatic SOS | aufwendig | Should |
| Tour-Statistiken | mittel | Should |
| Telemetrie-Recording | mittel | Should |
| Wartungs-Reminder | einfach | Should |
| BLE JSON-API | mittel | Should |
| PWA (Web-BLE) | mittel | Should |
| JSON-Config | einfach | Should |
| Diagnose-Trends | einfach | Should |
| Wettervorhersage | mittel | Could |
| Incoming-Call (ANCS) | mittel | Could |
| Music-Info (AMS) | mittel | Could |
| SMS/Notification | mittel | Could |
| Sturz-Aufzeichnung | mittel | Could |
| Geofencing | einfach | Could |
| Diebstahl-Warnung | aufwendig | Could |
| Segment-Timing | mittel | Could |
| Back-Track Navigation | mittel | Could |
| TPMS | mittel | Could |
| Hall-Sensor Vorderrad | einfach | Could |
| 0-100 Timer | einfach | Could |
| Lean-Angle-Max | einfach | Could |
| Piezo-Buzzer | einfach | Could |
| LED-Strip | einfach | Could |
| Verbrauch-Historie | mittel | Could |
| Web-Dashboard | aufwendig | Could |
| Native App | aufwendig | Could |
| Plugin-System UI | aufwendig | Could |
| REST-API / MQTT | aufwendig | Could |
| 2. Display | aufwendig | Won't |
| Sprachausgabe BT | aufwendig | Won't |

---

## 12. Hardware-Zusammenfassung

| Komponente | Typ | Preis | Interface |
|---|---|---|---|
| ESP32-S3 | MCU | 5-10 EUR | - |
| u-blox NEO-M9N | GPS | 15-20 EUR | UART |
| MPU6050 | IMU (6-Achsen) | 3 EUR | I2C |
| BMP280 | Barometer | 1-2 EUR | I2C |
| DS18B20 | Kuehlmitteltemp | 3 EUR | 1-Wire |
| MicroSD-Modul | Speicher | 2 EUR | SPI |
| E-Paper 4.2" | Display | 10-15 EUR | SPI |
| Spannungsteiler | Bordspannung | 0.50 EUR | ADC |
| Zündspulen-Pickup | RPM | 2-5 EUR | GPIO/IRQ |
| Piezo-Buzzer | Audio | 1 EUR | GPIO |
| WS2812B-Strip | LED | 2-3 EUR | GPIO |
| VDO Oeldrucksensor | Oeldruck | 20-30 EUR | ADC |
| ZEEPIN TP630 | TPMS | 20-30 EUR | BLE |
| **Gesamt (Basis)** | | **~50-70 EUR** | |
| **Gesamt (Vollausbau)** | | **~120-150 EUR** | |

---

## 13. Quellenverzeichnis

### ESP32 / Bordcomputer-Projekte
- ESP32 OBD Live Web Dashboard: https://devinn.org/obd2/guides/esp32-obd-live-web-dashboard.html
- Blackbox (GPS+IMU Telemetry): https://github.com/jctoledo/blackbox
- DIY GPS Navigator: https://github.com/ChetanXpro/diy-gps
- ESP32 Bike Computer (Yamaha Tracer 9): https://github.com/m-gracia/esp32-bike-computer-main
- Hackaday: Motorcycle Dashboard from ECU: https://hackaday.com/2020/09/21/a-motorcycle-dashboard-straight-from-the-ecu
- OpenTrailPaper (E-Paper Bike Computer): https://github.com/RaemondBW/OpenTrailPaper
- SteedPilot (ESP32 + Smartphone Navigation): https://github.com/deanthecoder/SteedPilot
- BikeNavESP32 (BLE Navigation Dashboard): https://github.com/rabbihossain/BikeNavESP32
- iKawa (ANCS/AMS Kawasaki Dashboard): https://github.com/gabrielerandazzo/iKawa
- BonoGPS (ESP32 GPS Track Logging): https://github.com/renatobo/bonogps

### Crash-Erkennung / Safety
- IoT Motorcycle Accident Detection: https://github.com/TrieuHzang/iot-motorcycle-accident-detection-system
- Two-Wheeler Accident Detection (7g/100ms): https://ijraset.com/best-journal/realtime-two-wheeler-accident-detection-with-automated-emergency-notification-and-post-impact-voice-logging-using-iot
- Accident Detection + Telegram: https://github.com/aamani-c/Accident-detection-and-notification-system.
- Smart Helmet (ESP32+SIM800L): https://rectronx.com/blog/smart-helmet-safety-monitoring-esp32

### Sensorik
- BMP280 Weather Station: https://github.com/Milad-Mohseni-Eng/ESP32-BMP280-OLED-Weather-Station
- Adafruit BMP280 Guide: https://learn.adafruit.com/adafruit-bmp280-barometric-pressure-plus-temperature-sensor-breakout
- ESP32 BMP280 Guide: https://www.espboards.dev/sensors/bmp280/
- Arduino RPM Gauge FAQ: https://electricalflux.com/mcu-general/arduino-rpm-gauge-sensor-wiring-faq
- Motorcycle Tachometer (Forum): https://forum.arduino.cc/t/motorcycle-tachometer-help/103871
- Motorcycle Data Monitoring: https://forum.arduino.cc/t/motorcycle-data-monitoring-project-need-guidance/85954
- Gear Position Indicator: https://forum.arduino.cc/t/how-to-use-analogread-to-get-gear-position-on-motorcycle/468182
- GearDisplay (Hall-Sensor): https://github.com/Ehryk/GearDisplay

### TPMS
- BLE-TPMS (ZEEPIN): https://github.com/ra6070/BLE-TPMS
- TPMS 433MHz + BLE: https://github.com/andi38/TPMS

### PD06 Transalp
- Oeldruckschalter: https://www.ebay.com/itm/384471229975
- ADVrider Forum (PD06): https://advrider.com/f/threads/africa-twin-tank-on-transalp-mod-fuel-pumps.1206792

### BLE / Web Bluetooth
- ESP-IDF BLE GATT Server: https://docs.espressif.com/projects/esp-idf/en/v5.3.4/esp32/api-reference/bluetooth/bt_le.html
- ESP-BLE-Web (PWA Widget Discovery): https://github.com/HamzaYslmn/Esp-BLE-Web
- ESP32 Web Bluetooth Terminal (PWA): https://github.com/dheera/ESP32-Web-Bluetooth-Terminal
- BLE Call System Motorcycle: https://www.electroniclinic.com/ble-call-system-for-motorcycles-using-esp32-s3-touch-display/

### E-Paper
- ESP E-Paper Component: https://components.espressif.com/components/tuanpmt/esp_epaper/versions/1.0.4/examples/esp32_wroom_generic

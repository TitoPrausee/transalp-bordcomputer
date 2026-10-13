# Feldtest-Protokoll & Evaluations-Kriterien

> Stand: Oktober 2026
> Plattform: ESP32-S3-WROOM-1 (N16R8: 16 MB Flash, 8 MB Octal PSRAM), ESP-IDF v5.4, FreeRTOS SMP
> Fahrzeug: XL600V Transalp (PD06), Baujahr 1986-1999, rein mechanisch, keine ECU
> Einsatzprofil: Gemischte Strasse und Gelaende, Reisetouren bis 500 km/Tag
> Vorgaenger-Dokumente: TEST-STRATEGY, USER-MANUAL, ARCHITECTURE, CASE-DESIGN, HARDWARE, research/04, research/01

---

## Inhaltsverzeichnis

1. Uebersicht und Zielsetzung
2. Test-Phasen
3. Test-Metriken pro Phase
4. Bewertungskriterien
5. Test-Datenerfassung
6. Risiken und Mitigation
7. Test-Schedule
8. Test-Ablauf-Diagramm

---

## 1. Uebersicht und Zielsetzung

### 1.1 Zweck dieses Dokuments

Dieses Dokument definiert das umfassende Feldtest-Protokoll fuer den Bordcomputer
der XL600V Transalp (PD06). Es beschreibt, wie das System unter realen Bedingungen
auf dem Motorrad getestet wird, welche Metriken dabei erfasst werden, und welche
Abnahmekriterien fuer eine Freigabe gelten. Es ergaenzt die bestehende
Test-Strategie (TEST-STRATEGY.md), die Unit-, Integration- und HIL-Tests auf
Bench-Ebene definiert. Waehrend die Test-Strategie die Test-Pyramide von unten
nach oben aufbaut, beginnt dieses Protokoll an der Spitze: dem vollstaendigen
System im realen Einsatz auf dem Motorrad.

Das Einsatzprofil der Transalp 600 ist entscheidend fuer die Testgestaltung. Die
PD06 ist ein Reisemotorrad mit gelaendegaengigen Faehigkeiten: gemischte
Strassen- und Forstweg-Fahrten, lange Reisetouren mit Dauerbetrieb von 8 Stunden
und mehr, und ganzjaehriger Einsatz bei Temperaturen von -10 bis +50 Grad Celsius.
Der Bordcomputer muss in all diesen Szenarien zuverlaessig funktionieren. Das
Test-Protokoll bildet diese Bedingungen systematisch ab.

### 1.2 System-Under-Test

Der Bordcomputer besteht aus drei Ebenen, die gemeinsam getestet werden:

- **Firmware** (ESP32-S3, ESP-IDF v5.4, FreeRTOS SMP): Akquiriert Sensordaten
  (RPM, Geschwindigkeit, Kuehlmitteltemperatur, Kraftstoffstand, Bordspannung,
  Oeldruck, IMU, Barometer), rendert die UI auf TFT- und E-Paper-Displays,
  loggt Telemetrie auf MicroSD, und kommuniziert per BLE mit der
  Companion-App.

- **Hardware** (PCB, Gehaeuse, Sensoren, Verkabelung): Das 3D-gedruckte
  ASA-Gehaeuse mit IP65/IP67-Schutzklasse, die Platine mit ESP32-S3-Modul,
  DC/DC-Wandler (Buck 12V zu 5V, LDO 5V zu 3.3V), LiPo-Pufferakku, und alle
  externen Sensoren (Optokoppler fuer RPM, NTC fuer Kuehlmittel, Schwimmer
  fuer Kraftstoffstand, Hall-Sensor oder Reed-Kontakt fuer Geschwindigkeit).

- **Companion-App** (React Native, beide mobilen Betriebssysteme): BLE-Client
  fuer Live-Telemetrie, Navigation, Diagnose, Tour-Export und OTA-Updates.

### 1.3 Abnahmekriterien auf hoher Ebene

Das System gilt als freigegeben fuer den Feldeinsatz, wenn:

1. Alle kritischen Test-Phasen (Labortest, Stand-Test, Feldtest Stadt,
   Feldtest Landstrasse) bestanden sind.
2. Kein kritischer DTC waehrend der Fahrt auftritt (keine Abstuerze, keine
   Datenverluste, keine Sensor-Ausfaelle).
3. Die GPX-Track-Aufzeichnung nach jeder Testfahrt vollstaendig und korrekt
   ist (keine Luecken, korrekte Zeitstempel, plausible Koordinaten).
4. Die BLE-Verbindung zur Companion-App waehrend der Fahrt stabil bleibt
   (maximal 3 Abbrueche pro Stunde, Reconnect innerhalb 5 Sekunden).
5. Das Display bei allen Lichtverhaeltnissen lesbar bleibt (Sonnenschein,
   Nacht, Regen).
6. Das Power-Management den Bordcomputer bei Spannungsausfall (Anlassen)
   mindestens 2 Sekunden ueberbrueckt, ohne Neustart.
7. Die Gehaeuse-Innenraeume nach jedem Witterungs-Test trocken sind
   (kein Kondensat, kein Wassereintritt).

### 1.4 Referenz-Fahrzeug und Test-Umgebung

Als Referenz-Fahrzeug dient eine XL600V Transalp (PD06) mit 22-mm-Lenker,
Crossbar-Montage und Standard-Bereifung (vorne 90/90-21, hinten 130/80-17).
Das Motorrad muss sich in einem mechanisch einwandfreien Zustand befinden:
Kuehlsystem dicht, Batterie geladen (mindestens 12,6 V Ruhespannung),
Lichtmaschine und Regler/Gleichrichter funktionstuechtig (13,5-15,5 V bei
5000 U/min). Der Bordcomputer wird an der Crossbar montiert, alle Sensorkabel
werden entlang des rechten Rahmenrohrs verlegt und mit Kabelbindern fixiert.
Die Companion-App laeuft auf einem aktuellen Smartphone in der Brusttasche der
Motorradjacke.

---

## 2. Test-Phasen

### 2.1 Phasen-Uebersicht

Die Test-Phasen bauen aufeinander auf: Jede Phase setzt voraus, dass die
vorherige erfolgreich abgeschlossen wurde. Dies stellt sicher, dass Fehler
frueh erkannt und nicht in teurere Phasen weitergereicht werden.

| Phase | Name | Dauer | Umgebung | Motorrad | Motor | Hauptziel |
|---|---|---|---|---|---|---|
| P1 | Labortest | 2-4 h | Werkstatt, Test-Bench | Nein | Nein | Funktionspruefung aller Subsysteme |
| P2 | Stand-Test | 30 min | Werkstatt, draussen | Ja | Laeuft | EMV, Sensor-Verifikation am Motor |
| P3 | Feldtest Stadt | 30 min | Stadtverkehr | Ja | Laeuft | Stop-and-Go, thermische Belastung |
| P4 | Feldtest Landstrasse | 60 min | Landstrasse, 50-100 km/h | Ja | Laeuft | Dauerspeed, Vibration, GPS-Track |
| P5 | Feldtest Autobahn | 30 min | Autobahn, 100-130 km/h | Ja | Laeuft | Hohe Geschwindigkeit, Winddruck |
| P6 | Feldtest Gelaende | 30 min | Forstweg / Schotter | Ja | Laeuft | Vibrationsspitzen, Stossbelastung |
| P7 | Dauertest | 500 km / 8 h+ | Gemischt | Ja | Laeuft | Dauerbelastung, thermische Stabilitaet |
| P8 | Witterungs-Test | Variabel | Regen, Sonne, Temperatur | Ja | Variabel | Wetterfestigkeit, Temperaturverhalten |

### 2.2 Phase P1: Labortest (Test-Bench, ohne Motorrad)

**Ziel:** Vollstaendige Funktionspruefung aller Subsysteme unter kontrollierten
Bedingungen, bevor das System am Motorrad installiert wird.

**Voraussetzungen:**
- Bordcomputer vollstaendig aufgebaut (PCB bestueckt, Gehaeuse montiert,
  Displays angeschlossen).
- USB-C-Verbindung zum Computer fuer Serial-Console und Stromversorgung.
- Sensoren simuliert: RPM per Funktionsgenerator am PCNT0-Eingang,
  Geschwindigkeit per Funktionsgenerator am PCNT1-Eingang (oder manuelle
  Magnet-Passagen am Hall-Sensor), Kuehlmitteltemperatur per Potentiometer
  am ADC, Kraftstoffstand per Potentiometer, Bordspannung per Labornetzteil.
- MicroSD-Karte eingesetzt (8-32 GB, Class 10, FAT32).
- Companion-App auf Smartphone installiert, BLE-Verbindung hergestellt.

**Test-Schritte:**

| Schritt | Aktion | Erwartetes Ergebnis | Messmethode |
|---|---|---|---|
| P1.1 | Bordcomputer per USB-C einschalten | Boot-Screen erscheint in < 3 s | Sichtkontrolle, Serial-Log |
| P1.2 | Self-Test abwarten | 7 Hardware-Checks passieren, 3facher Piep | Serial-Log, DTC-Liste leer |
| P1.3 | GPS-Fix abwarten (draussen) | Fix in < 120 s (Kaltstart) | GPS-LED blau, Display zeigt Position |
| P1.4 | RPM-Signal simulieren (1000-8000 U/min) | Anzeige folgt Signal, Abweichung < 1 % | Funktionsgenerator vs. Display |
| P1.5 | Geschwindigkeit simulieren (0-130 km/h) | Anzeige folgt, Abweichung < 1 % | Funktionsgenerator vs. Display |
| P1.6 | Kuehlmittel-Temperatur simulieren | Anzeige in 2-Grad-Schritten, Abw. < 2 C | Potentiometer vs. Display |
| P1.7 | Kraftstoffstand simulieren | Anzeige 0-100 %, lineare Interpolation | Potentiometer vs. Display |
| P1.8 | Bordspannung variieren (8-15 V) | Anzeige folgt, Warnung bei < 11,5 / > 15,5 V | Labornetzteil vs. Display |
| P1.9 | Tour-Aufzeichnung starten (Doppeldruck) | GPX-Datei wird erstellt, Trackpunkte alle 1-5 s | SD-Karte auslesen |
| P1.10 | Companion-App Dashboard pruefen | Alle Live-Werte sichtbar, Update-Rate 4 Hz | App-Dashboard |
| P1.11 | OTA-Update durchfuehren (Test-Image) | SHA-256-Verifikation OK, Neustart erfolgreich | Serial-Log, App-OTA-Status |
| P1.12 | 30 min Dauerbetrieb (alle Systeme aktiv) | Kein Absturz, kein DTC, kein Watchdog-Reset | Serial-Log, DTC-Liste |

**Pass-Kriterium:** Alle 12 Schritte erfolgreich. Kein aktiver DTC.
**Abbruch-Kriterium:** Systemabsturz, Display-Ausfall, oder 3 oder mehr
fehlgeschlagene Schritte.

### 2.3 Phase P2: Stand-Test (Motorrad im Stand, Motor laeuft)

**Ziel:** Verifikation der Sensorik am realen Motorrad, EMV-Verhalten unter
Zuedeinfluss, und Power-Management unter Motorlast.

**Voraussetzungen:**
- P1 bestanden.
- Bordcomputer am Motorrad installiert (Lenker/Crossbar, alle Sensoren
  angeschlossen, Kabel verlegt).
- Motorrad im Stand, auf ebenem Untergrund, im Freien (fuer GPS).
- Serial-Console per USB-C protokolliert (falls USB-C erreichbar) oder
  alternativ Companion-App Log-Download nach Test.

**Test-Schritte:**

| Schritt | Aktion | Erwartetes Ergebnis | Messmethode |
|---|---|---|---|
| P2.1 | Zuendung einschalten (Motor aus) | Bordcomputer bootet, Self-Test OK | DTC-Liste, Serial-Log |
| P2.2 | Motor starten (E-Starter) | Bordcomputer ueberbrueckt Spannungseinbruch | Serial-Log (Boot-Loop?) |
| P2.3 | Leerlauf (1300 U/min) beobachten | RPM-Anzeige stabil bei 1300 +/- 100 | Display vs. Gehoer |
| P2.4 | Drehzahl auf 3000 U/min halten | RPM-Anzeige folgt, stabil | Display vs. Gehoer |
| P2.5 | Drehzahl auf 6000 U/min halten | RPM folgt, Shift-Light bei 7000 nicht ausgeloest | Display |
| P2.6 | Bordspannung bei Leerlauf messen | 12-13 V (Batterie entlaedt im Leerlauf) | Display vs. Multimeter |
| P2.7 | Bordspannung bei 5000 U/min messen | 13,5-14,5 V (Generator laedt) | Display vs. Multimeter |
| P2.8 | Kuehlmitteltemperatur beobachten | Steigt langsam, plausibel (Raumtemp + 20-40 C) | Display vs. IR-Thermometer |
| P2.9 | BLE-Verbindung pruefen (5 m Abstand) | Stabil, RSSI > -80 dBm | App-Diagnose-Screen |
| P2.10 | 10 min Standbetrieb (Motor laeuft) | Kein Absturz, kein DTC, keine EMV-Stoerung | Serial-Log, DTC-Liste |
| P2.11 | Motor abstellen | Bordcomputer bleibt aktiv, geordneter Shutdown | Serial-Log |
| P2.12 | Gehaeuse-Innenraum pruefen | Trocken, kein Kondensat | Sichtkontrolle (Deckel oeffnen) |

**Pass-Kriterium:** Alle 12 Schritte erfolgreich. Insbesondere P2.2
(Spannungseinbruch beim Anlassen) und P2.10 (EMV-Stabilitaet) muessen
fehlerfrei sein.
**Abbruch-Kriterium:** Bordcomputer startet beim Anlassen neu (Boot-Loop),
RPM-Anzeige springt stark (> 500 U/min), oder EMV-bedingter Display-Ausfall.

### 2.4 Phase P3: Feldtest Stadt (30 min, Stop-and-Go)

**Ziel:** Pruefung des Systems unter typischem Stadtverkehr mit haeufigem
Stop-and-Go, wechselnden Geschwindigkeiten und Ampelphasen.

**Voraussetzungen:**
- P1 und P2 bestanden.
- Bordcomputer am Motorrad installiert, alle Sensoren funktionsfaehig.
- Smartphone mit Companion-App in Brusttasche, App aktiv.
- Serial-Console nicht verfuegbar waehrend der Fahrt: Logging erfolgt auf
  SD-Karte und in der App.

**Test-Strecke:** 30 min Stadtverkehr mit ca. 10 Ampelstopps, Geschwindigkeiten
0-50 km/h, durchschnittlich 25 km/h. Strecke sollte im Voraus festgelegt und
mit GPS-Referenz aufgezeichnet werden.

**Test-Schritte:**

| Schritt | Aktion | Erwartetes Ergebnis | Messmethode |
|---|---|---|---|
| P3.1 | Tour-Aufzeichnung starten | GPX-Datei beginnt, erster Trackpunkt | App / SD-Karte |
| P3.2 | 30 min Stadtfahrt | Alle Werte plausibel, keine Ausfaelle | Display, App |
| P3.3 | Bei jedem Stop: RPM auf 0 (Leerlauf) | Gang-Anzeige zeigt N im Stand | Display |
| P3.4 | GPS-Track mit Referenz vergleichen | Abweichung < 5 m pro Punkt, keine Luecken > 10 s | GPX vs. Referenz-GPX |
| P3.5 | BLE-Verbindungsabbrueche zaehlen | Max. 3 pro Stunde, Reconnect < 5 s | App-Log |
| P3.6 | Kuehlmitteltemperatur pruefen | Steigt im Stadtverkehr (Kuehlung gering) | Display, max. 110 C |
| P3.7 | Kraftstoffstand pruefen | Sinkt plausibel, keine Spruenge | Display |
| P3.8 | Display-Lesbarkeit bei Sonne | Geschwindigkeit ablesbar | Foto-Dokumentation |
| P3.9 | DTC-Liste nach Fahrt pruefen | Keine neuen DTCs | App-Diagnose |
| P3.10 | GPX-Datei auf SD-Karte pruefen | Vollstaendig, korrekte Zeitstempel | SD-Karte auslesen |

**Pass-Kriterium:** GPX-Track vollstaendig, max. 3 BLE-Abbrueche, kein DTC.
**Abbruch-Kriterium:** Systemabsturz waehrend der Fahrt, GPS-Track mit Luecken
> 30 s, oder BLE-Verbindung verloren fuer > 30 s.

### 2.5 Phase P4: Feldtest Landstrasse (60 min, 50-100 km/h)

**Ziel:** Pruefung unter Dauerfahrt mit konstanter Geschwindigkeit, Vibration
und thermischer Belastung ueber einen laengeren Zeitraum.

**Voraussetzungen:**
- P1, P2 und P3 bestanden.

**Test-Strecke:** 60 min Landstrasse, durchschnittlich 80 km/h, wenig
Stop-and-Go, kurvige und gerade Abschnitte. Ca. 80 km Strecke.

**Test-Schritte:**

| Schritt | Aktion | Erwartetes Ergebnis | Messmethode |
|---|---|---|---|
| P4.1 | Tour-Aufzeichnung starten | GPX beginnt | App / SD-Karte |
| P4.2 | 60 min Landstrassenfahrt | Stabile Werte, keine Ausfaelle | Display, App |
| P4.3 | GPS-Genauigkeit bei 80 km/h | Positionsgenauigkeit < 5 m, Speed-Abw. < 2 km/h | GPX vs. Tacho-Referenz |
| P4.4 | Ganganzeige bei konstantem Speed | Gang korrekt (3-5 bei 50-100 km/h) | Display |
| P4.5 | RPM bei konstantem Speed | 3000-6000 U/min, stabil | Display |
| P4.6 | Bordspannung bei Dauerfahrt | 13,5-14,5 V (Generator laedt) | Display |
| P4.7 | Kuehlmitteltemp bei Dauerfahrt | Stabil bei 80-95 C (Thermostat offen) | Display |
| P4.8 | Vibration: Display-Refresh | E-Paper-Refresh funktioniert unter Vibration | Display-Beobachtung |
| P4.9 | Vibration: Steckverbindungen | Keine lose Stecker nach Fahrt | Sichtkontrolle |
| P4.10 | BLE bei 80 km/h (Brusttasche) | Stabil, max. 2 Abbrueche/stunde | App-Log |
| P4.11 | DTC-Liste nach Fahrt | Keine neuen DTCs | App-Diagnose |
| P4.12 | GPX-Vollstaendigkeit | Alle 1-5 s ein Punkt, keine Luecken | SD-Karte auslesen |

**Pass-Kriterium:** GPX vollstaendig, kein DTC, Bordspannung im Normbereich.
**Abbruch-Kriterium:** Systemabsturz, Display-Ausfall durch Vibration, oder
Bordspannung ausserhalb 11,5-15,5 V.

### 2.6 Phase P5: Feldtest Autobahn (30 min, 100-130 km/h)

**Ziel:** Pruefung bei hohen Geschwindigkeiten, erhoehter Vibration durch
Motordrehzahl und Fahrtwind, sowie Stoerung des BLE-Signals durch
Geschwindigkeit und Koerperabschattung.

**Voraussetzungen:**
- P1 bis P4 bestanden.

**Test-Strecke:** 30 min Autobahn, durchschnittlich 120 km/h, wenig Verkehr
fuer konstantes Tempo. Ca. 60 km Strecke. Es gelten die gueltigen
Geschwindigkeitsbegrenzungen; die Testfahrt wird innerhalb der legalen
Geschwindigkeitsbereiche durchgefuehrt.

**Test-Schritte:**

| Schritt | Aktion | Erwartetes Ergebnis | Messmethode |
|---|---|---|---|
| P5.1 | Tour-Aufzeichnung starten | GPX beginnt | App / SD-Karte |
| P5.2 | 30 min Autobahnfahrt (100-130 km/h) | Stabile Werte, keine Ausfaelle | Display, App |
| P5.3 | GPS-Genauigkeit bei 120 km/h | Speed-Abw. < 2 km/h vs. Referenz | GPX vs. Referenz |
| P5.4 | RPM bei 120 km/h (5. Gang) | ca. 5600 U/min bei 5. Gang (spaete Modelle) | Display |
| P5.5 | Bordspannung bei 120 km/h | 13,5-14,5 V, stabil | Display |
| P5.6 | Kuehlmitteltemp bei 120 km/h | Stabil bei 80-95 C (Fahrtwind-Kuehlung) | Display |
| P5.7 | Winddruck: Gehaeuse-Stabilitaet | Keine Bewegung, Kugelgelenk haelt Position | Sichtkontrolle |
| P5.8 | BLE bei 120 km/h (Brusttasche) | Stabil, max. 3 Abbrueche/stunde | App-Log |
| P5.9 | Display-Lesbarkeit bei Sonne | Geschwindigkeit ablesbar | Foto-Dokumentation |
| P5.10 | Vibration bei 120 km/h | Display-Refresh ok, keine Pixel-Ausfaelle | Display-Beobachtung |
| P5.11 | DTC-Liste nach Fahrt | Keine neuen DTCs | App-Diagnose |
| P5.12 | GPX-Vollstaendigkeit bei 120 km/h | Alle 1-5 s ein Punkt, keine Luecken | SD-Karte auslesen |

**Pass-Kriterium:** GPX vollstaendig, kein DTC, Display-Refresh unter
Vibration einwandfrei.
**Abbruch-Kriterium:** Systemabsturz, Gehaeuse bewegt sich am Kugelgelenk
durch Winddruck, oder BLE-Verbindung instabil fuer > 30 s.

### 2.7 Phase P6: Feldtest Gelaende (30 min, Forstweg / Schotter)

**Ziel:** Pruefung unter Transalp-typischen Gelaendebedingungen: starke
Vibrationen durch Schotter und Waschbrettpisten, Stoesse durch Schlagloecher,
erhoehte Staubbelastung und staerkere Motorlast bei niedrigen Geschwindigkeiten.

Diese Phase ist fuer die PD06 besonders wichtig, da die Transalp 600
urspruenglich als Gelaendemotorrad konzipiert war und viele Fahrer sie auf
Forstwegen und Schotterpisten einsetzen.

**Voraussetzungen:**
- P1 bis P5 bestanden.
- Teststrecke: Forstweg mit Schotter, Waschbrettpisten und vereinzelten
  Schlagloechern. Geschwindigkeit 20-60 km/h. Keine extremen Offroad-Passagen
  (keine Felskletterei, keine Wasserdurchfahrten > 30 cm Tiefe).

**Test-Schritte:**

| Schritt | Aktion | Erwartetes Ergebnis | Messmethode |
|---|---|---|---|
| P6.1 | Tour-Aufzeichnung starten | GPX beginnt | App / SD-Karte |
| P6.2 | 30 min Gelaendefahrt | Stabile Werte, keine Ausfaelle | Display, App |
| P6.3 | Vibrationsspitzen: IMU-Aufzeichnung | Beschleunigungsspizen erkannt, keine Crash-Ausloesung | Serial-Log, DTC |
| P6.4 | Ganganzeige im Gelaende | Korrekt bei niedrigem Speed, auch im 1./2. Gang | Display |
| P6.5 | RPM bei niedrigem Speed | Stabil, keine Spruenge durch Rutschen | Display |
| P6.6 | GPS-Track im Gelaende | Track folgt Weg, Abw. < 10 m | GPX vs. Referenz |
| P6.7 | Steckverbindungen nach Gelaende | Keine losen Stecker, keine Korrosion | Sichtkontrolle |
| P6.8 | Display-Refresh bei Vibration | E-Paper-Refresh ok, keine Glastorsion | Display-Beobachtung |
| P6.9 | Gehaeuse auf Risse pruefen | Keine Risse, keine Verformung | Sichtkontrolle |
| P6.10 | BLE bei Gelaendefahrt | Stabil (niedrige Geschwindigkeit, gute Reichweite) | App-Log |
| P6.11 | DTC-Liste nach Gelaende | Keine neuen DTCs (insb. kein Crash-False-Positive) | App-Diagnose |
| P6.12 | GPX-Vollstaendigkeit | Alle 1-5 s ein Punkt, keine Luecken durch Vibration | SD-Karte auslesen |

**Pass-Kriterium:** GPX vollstaendig, kein DTC (insb. kein
Crash-False-Positive), keine Gehaeuseschaeden.
**Abbruch-Kriterium:** Systemabsturz durch Stoss, Crash-Erkennung loest
false-positive aus, Gehaeuse rissig oder Stecker gelockert.

### 2.8 Phase P7: Dauertest (500 km / 8 h+)

**Ziel:** Pruefung der Langzeitstabilitaet unter realen Reisebedingungen.
Dies ist der entscheidende Test fuer die Einsatztauglichkeit als
Reisebordcomputer: Das System muss ueber einen ganzen Reisetag
zusammenhaengend funktionieren, ohne Neustart, ohne Datenverlust und ohne
thermischen Ueberlast.

**Voraussetzungen:**
- P1 bis P6 bestanden.
- Eine geplante Route von ca. 500 km, gemischt (Autobahn, Landstrasse,
  Gelaendeanteil). Dauer mindestens 8 Stunden inkl. Pausen.
- Kraftstoff-Reserve: Tank wird zu Beginn vollgetankt, Reserve-Zapfsaeule
  nach ca. 300 km eingeplant (Transalp-Reichweite ca. 300-350 km).

**Test-Schritte:**

| Schritt | Aktion | Erwartetes Ergebnis | Messmethode |
|---|---|---|---|
| P7.1 | Tour-Aufzeichnung starten | GPX beginnt, GPS-Fix OK | App / SD-Karte |
| P7.2 | 500 km Dauertestfahrt | System durchgehend aktiv, kein Neustart | Serial-Log (nach Fahrt) |
| P7.3 | GPS-Track: 8+ Stunden durchgehend | Keine Luecken > 30 s, korrekte Zeitstempel | SD-Karte auslesen |
| P7.4 | MCU-Temperatur ueber 8 h | Innentemperatur < 70 C (kein Thermal-Throttling) | Serial-Log (BMP280-Temp) |
| P7.5 | Display-Temperatur ueber 8 h | Display-Temp < 60 C (kein Ausfall) | IR-Thermometer nach Fahrt |
| P7.6 | SD-Karte: Datei-Integritaet | GPX-Datei valid (XML-Struktur), keine Korruption | GPX-Parser |
| P7.7 | NVS-Ueberleben nach Reboot | Konfiguration erhalten, DTC-Tabelle erhalten | App-Diagnose nach Reboot |
| P7.8 | BLE: 8 h durchgehend | Max. 10 Abbrueche ueber 8 h, alle Auto-Reconnect | App-Log |
| P7.9 | Bordspannung: durchgehend > 13,5 V | Generator laedt ueber ganze Fahrt | Display / Serial-Log |
| P7.10 | Kraftstoffstand: plausibel ueber 500 km | Sinkt kontinuierlich, kein Sprung | Display |
| P7.11 | DTC-Liste nach 500 km | Max. 2 nicht-kritische DTCs (z.B. GPS-Loss in Tunnel) | App-Diagnose |
| P7.12 | Systemzustand nach 8 h | Kein Memory-Leak (Heap stabil), kein Stack-Overflow | Serial-Log (Heap-Stats) |

**Pass-Kriterium:** System durchgehend aktiv (kein ungewollter Neustart),
GPX-Track vollstaendig (max. 1 % Luecken), kein kritischer DTC, Heap stabil
(Schwankung < 20 % vom Startwert).
**Abbruch-Kriterium:** Systemabsturz, Watchdog-Reset, SD-Karten-Fehler,
Display-Ausfall, oder thermische Ueberlast (MCU > 85 C).

### 2.9 Phase P8: Witterungs-Test (Regen, Sonne, Temperatur)

**Ziel:** Pruefung der Wetterfestigkeit und des Temperaturverhaltens unter
realistischen Umweltbedingungen. Der Bordcomputer soll ganzjaehrig einsetzbar
sein (0-35 C Umgebungstemperatur, direkte Sonneneinstrahlung, Regen).

**Voraussetzungen:**
- P1 bis P7 bestanden (oder parallel zu P7 durchfuehrbar).

**Test-Szenarien:**

| Szenario | Bedingungen | Dauer | Hauptpruefung |
|---|---|---|---|
| P8.1 Regen | Landstrasse, maessiger Regen, 15-20 C | 30 min | Wasserdichtigkeit (IP65/IP67), Display-Lesbarkeit bei Regen |
| P8.2 Sonne | Landstrasse, direkte Sonne, > 25 C | 60 min | Display-Lesbarkeit, thermische Belastung, UV-Belastung |
| P8.3 Kalt | Landstrasse, 0-5 C, bedeckt | 30 min | E-Paper-Trageheit, Akku-Verhalten, Start bei Kaelte |
| P8.4 Hitze | Stadt/Gelaende, > 30 C, Sonne | 30 min | Thermische Stabilitaet, Thermal-Throttling |
| P8.5 Nacht | Landstrasse, nachts, 10-15 C | 30 min | Display-Lesbarkeit bei Nacht (Backlight) |
| P8.6 Kondensation | Morgenfahrt, kalter Tau auf Gehaeuse | 20 min | Kondensat im Gehaeuseinnern, Display-Beschlag |

**Test-Schritte pro Szenario:**

| Schritt | Aktion | Erwartetes Ergebnis | Messmethode |
|---|---|---|---|
| P8.x.1 | Fahrt unter Bedingungen durchfuehren | System funktioniert, keine Ausfaelle | Display, App |
| P8.x.2 | Display-Lesbarkeit bewerten | Lesbar (Note 1-5) | Foto-Dokumentation |
| P8.x.3 | Gehaeuse-Innenraum nach Fahrt | Trocken, kein Kondensat, kein Wassereintritt | Sichtkontrolle |
| P8.x.4 | DTC-Liste nach Fahrt | Keine neuen DTCs | App-Diagnose |
| P8.x.5 | System-Temperatur nach Fahrt | MCU < 70 C, Display < 60 C | Serial-Log / IR-Thermometer |

**Pass-Kriterium:** In allen 6 Szenarien funktionstuechtig, Gehaeuse-Innenraum
immer trocken, Display bei allen Bedingungen lesbar (mindestens Note 3 von 5).
**Abbruch-Kriterium:** Wassereintritt ins Gehaeuse, Display-Ausfall durch
Temperatur, Systemabsturz durch thermische Ueberlast.

---

## 3. Test-Metriken pro Phase

### 3.1 Metrik-Definitionen

Jede Test-Phase erfasst einen Satz von Metriken. Die Metriken werden
waehrend der Fahrt kontinuierlich auf SD-Karte geloggt und nach der Fahrt
ausgewertet. Die folgende Tabelle definiert alle Metriken mit Einheit,
Zielwert und Messmethode.

| Metrik | Einheit | Zielwert | Messmethode | Phase |
|---|---|---|---|---|
| RPM-Abweichung | % | < 1 % vs. Referenz | Funktionsgenerator (P1), Gehoer/Referenz (P2+) | P1-P7 |
| Speed-Abweichung (GPS) | km/h | < 2 km/h vs. Referenz | GPS-Referenzgeraet | P1-P7 |
| Speed-Abweichung (Rad) | km/h | < 1 km/h vs. Referenz | Tacho-Referenz | P1-P6 |
| Kuehlmittel-Temp-Drift | C | < 2 C vs. IR-Thermometer | IR-Thermometer am Kuehlschlauch | P2-P7 |
| Kraftstoff-Abweichung | L | < 1 L vs. Nachfuellung | Nachfuellung nach Fahrt | P3-P7 |
| Bordspannung-Abweichung | V | < 0,2 V vs. Multimeter | Multimeter an Batterie | P2-P7 |
| GPS-Positionsgenauigkeit | m | < 5 m (CEP50) | GPX vs. Referenz-Track | P1-P7 |
| GPS-Speed-Glaettung | s | Verzoegerung < 2 s | Sprungantwort (0 auf 50 km/h) | P3-P5 |
| GPX-Log-Vollstaendigkeit | % | > 99 % der erwarteten Punkte | Erwartete vs. tatsaechliche Punkte | P3-P7 |
| GPX-Zeitstempel-Genauigkeit | s | < 1 s Abweichung | NTP-Referenz nach Sync | P3-P7 |
| Display-Aktualisierungsrate | FPS | > 25 FPS (TFT), 1 Hz (E-Paper) | Serial-Log (Frame-Zeit) | P1-P7 |
| BLE-Verbindungsabbrueche | /h | < 3/h (Stadt), < 2/h (Land) | App-Log | P3-P7 |
| BLE-Reconnect-Zeit | s | < 5 s (Median) | App-Log | P3-P7 |
| BLE-RSSI | dBm | > -80 dBm (5 m, Brusttasche) | App-Diagnose | P2-P7 |
| BLE-Throughput (Track-Export) | kB/s | > 8 kB/s | App-Transfer-Zeit | P3-P7 |
| MCU-Temperatur | C | < 70 C | BMP280 / Serial-Log | P2-P8 |
| Display-Temperatur | C | < 60 C | IR-Thermometer | P2-P8 |
| Heap-Speicher (freier Heap) | KB | Stabil, Schwankung < 20 % | Serial-Log (esp_get_free_heap_size) | P1, P7 |
| Stack-Wasserstand (high-water) | Byte | > 512 Byte Reserve je Task | Serial-Log (uxTaskGetStackHighWaterMark) | P1, P7 |
| SD-Schreibrate | kB/s | > 2 kB/s (5 Hz, 64-Slot-Batch) | Serial-Log (Logger-Stats) | P1-P7 |
| DTC-Haeufigkeit | /h | 0 kritische/h | DTC-Liste | P2-P7 |
| Watchdog-Resets | /h | 0 | Serial-Log (Boot-Cause) | P1-P7 |
| Boot-Zeit | s | < 3 s (Kaltstart) | Serial-Log (Timestamps) | P1 |
| GPS-Fix-Zeit (Kaltstart) | s | < 120 s | Serial-Log (Fix-Event) | P1 |
| GPS-Fix-Zeit (Warmstart) | s | < 10 s | Serial-Log (Fix-Event) | P3-P7 |
| Power-Overbrueckung (Anlassen) | s | > 2 s ohne Neustart | Serial-Log (Boot-Cause) | P2 |
| Akku-Entladung (bei Dauerbetrieb) | %/h | < 5 %/h (mit Generator) | Serial-Log (Voltage-Trend) | P7 |

### 3.2 Metrik-Erfassung pro Phase

Die folgende Matrix zeigt, welche Metriken in welcher Phase erfasst werden.
Ein "X" bedeutet, dass die Metrik in dieser Phase gemessen und ausgewertet
werden muss.

| Metrik | P1 | P2 | P3 | P4 | P5 | P6 | P7 | P8 |
|---|---|---|---|---|---|---|---|---|
| RPM-Abweichung | X | X | X | X | X | X | X | - |
| Speed-Abweichung (GPS) | X | X | X | X | X | X | X | - |
| Speed-Abweichung (Rad) | X | X | X | X | X | X | X | - |
| Kuehlmittel-Temp-Drift | - | X | X | X | X | X | X | - |
| Kraftstoff-Abweichung | - | - | X | X | X | - | X | - |
| Bordspannung-Abweichung | X | X | X | X | X | X | X | - |
| GPS-Positionsgenauigkeit | X | X | X | X | X | X | X | - |
| GPX-Log-Vollstaendigkeit | X | - | X | X | X | X | X | X |
| Display-Aktualisierungsrate | X | X | X | X | X | X | X | X |
| BLE-Abbrueche/h | - | X | X | X | X | X | X | X |
| BLE-Reconnect-Zeit | - | X | X | X | X | X | X | X |
| BLE-RSSI | X | X | X | X | X | X | X | X |
| MCU-Temperatur | - | X | - | X | - | - | X | X |
| Display-Temperatur | - | - | - | - | X | - | X | X |
| Heap-Speicher | X | - | - | - | - | - | X | - |
| Stack-Wasserstand | X | - | - | - | - | - | X | - |
| SD-Schreibrate | X | X | X | X | X | X | X | - |
| DTC-Haeufigkeit | X | X | X | X | X | X | X | X |
| Watchdog-Resets | X | X | X | X | X | X | X | X |
| Boot-Zeit | X | - | - | - | - | - | - | - |
| GPS-Fix-Zeit (Kaltstart) | X | - | - | - | - | - | - | - |
| GPS-Fix-Zeit (Warmstart) | - | X | X | X | X | X | X | - |
| Power-Overbrueckung | - | X | - | - | - | - | - | - |
| Akku-Entladung | - | - | - | - | - | - | X | - |

### 3.3 Sensor-Genauigkeit im Detail

#### 3.3.1 RPM-Abweichung

Die Drehzahl wird ueber den Pickup-Sensor der CDI-Zuendung erfasst (1 Puls pro
Kurbelwellenumdrehung). Der PCNT (Pulse-Counter) zaehlt Impulse hardwaremaessig
in einem 20-ms-Intervall (50 Hz). Die RPM berechnet sich aus: RPM = PulseCount
x 60 / (Pulses_pro_Umdrehung x Intervall_s). Bei 1 Puls/Umdrehung und 20-ms-
Intervall: RPM = PulseCount x 3000.

Referenzmessung im Labortest: Funktionsgenerator mit bekannter Frequenz
am PCNT0-Eingang. Bei 133 Hz (8000 U/min) muessen 133 x 0,02 = 2,66 Pulse
pro Zyklus gezaehlt werden, was auf 50 Hz-Sampling zu RPM = 2,66 x 3000 = 7980
ergibt. Die Abweichung betraegt < 1 % durch die Rundung auf ganze Pulse.

Am Motorrad: Der Pickup liefert bei Leerlauf (1300 U/min) ca. 21,7 Hz, bei
8000 U/min ca. 133 Hz. Die Anzeige muss mit dem Gehoer (Leerlaufklang) und
mit einem Referenz-Drehzahlmesser (falls verfuegbar) uebereinstimmen.

#### 3.3.2 Temperatur-Drift

Die Kuehlmitteltemperatur wird ueber einen NTC-Thermistor (10k, B3950) im
oberen Kuehlschlauch gemessen. Die Auswertung erfolgt ueber einen
Spannungsteiler am ADC1 (12-bit, 0-3,3 V) und die Steinhart-Hart-Gleichung.
Die Genauigkeit betraegt typisch +/- 2 C im Bereich 20-120 C.

Referenzmessung: IR-Thermometer am Kuehlschlauch in der Naehe des Sensors.
Der thermostatische Regelbereich der PD06 (Oeffnen bei 80-84 C, voll offen
bei 95 C, Luefter bei 98-102 C) dient als Plausibilitaetspruefung.

Drift-Test: Waehrend der 8-stuendigen Dauertestfahrt (P7) wird die
Temperaturanzeige bei konstanter Fahrt (80 km/h, 5. Gang) ueberprueft. Die
Anzeige muss innerhalb von +/- 2 C stabil bleiben, sobald der Thermostat
voll geoeffnet ist (ca. 10 min nach Fahrtbeginn).

#### 3.3.3 GPS-Tracking-Genauigkeit

Die GPS-Positionsgenauigkeit wird als CEP50 (Circular Error Probable, 50 %
der Punkte innerhalb dieses Radius) gegen einen Referenz-Track gemessen.
Der Referenz-Track wird mit einem separaten GPS-Geraet (oder Smartphone mit
höherer Update-Rate) aufgenommen.

Bei 1 Hz GPS-Update und 80 km/h (22 m/s) liegt der Abstand zwischen zwei
Punkten bei ca. 22 m. Die GPX-Trackpunkte muessen kontinuierlich sein
(keine Luecken > 5 s bei 1 Hz, bzw. > 30 s bei 0,2 Hz). Bei Bewertung der
Genauigkeit werden Punkte in Tunneln und unter Bruecken ausgeschlossen.

---

## 4. Bewertungskriterien

### 4.1 Pass/Fail-Schwellen

Jede Metrik hat einen festen Schwellwert, bei dessen Ueberschreitung der Test
als "Fail" bewertet wird. Die Schwellen sind in zwei Kategorien unterteilt:

**Kritisch (K):** Ueberschreitung fuehrt zum sofortigen Abbruch der Test-Phase.
Das System ist fuer den Feldeinsatz nicht freigegeben.

**Nicht-kritisch (N):** Ueberschreitung wird notiert, der Test wird
fortgesetzt. Das System kann mit Einschraenkungen freigegeben werden.

| Metrik | Schwellwert (Pass) | Schwellwert (Fail) | Kategorie |
|---|---|---|---|
| RPM-Abweichung | < 1 % | > 3 % | K |
| Speed-Abweichung (GPS) | < 2 km/h | > 5 km/h | K |
| Kuehlmittel-Temp-Drift | < 2 C | > 5 C | N |
| Kraftstoff-Abweichung | < 1 L | > 3 L | N |
| Bordspannung-Abweichung | < 0,2 V | > 0,5 V | K |
| GPS-Positionsgenauigkeit | < 5 m CEP50 | > 15 m CEP50 | N |
| GPX-Log-Vollstaendigkeit | > 99 % | < 95 % | K |
| GPX-Zeitstempel-Abweichung | < 1 s | > 5 s | N |
| Display-FPS (TFT) | > 25 FPS | < 15 FPS | N |
| BLE-Abbrueche/h | < 3/h | > 10/h | K |
| BLE-Reconnect-Zeit (Median) | < 5 s | > 15 s | K |
| BLE-RSSI (5 m) | > -80 dBm | < -95 dBm | N |
| MCU-Temperatur | < 70 C | > 85 C | K |
| Display-Temperatur | < 60 C | > 70 C | N |
| Heap-Speicher-Schwankung | < 20 % | > 50 % | K |
| Stack-High-Water | > 512 Byte | < 128 Byte | K |
| DTC (kritisch) | 0/h | > 0/h | K |
| DTC (nicht-kritisch) | < 2/h | > 5/h | N |
| Watchdog-Resets | 0/h | > 0/h | K |
| Boot-Zeit | < 3 s | > 5 s | N |
| GPS-Fix-Zeit (Kaltstart) | < 120 s | > 300 s | N |
| Power-Overbrueckung | > 2 s | < 1 s | K |
| Gehaeuse-Wasserdichtigkeit | Trocken | Feucht/Eintritt | K |

### 4.2 Gewichtung der Kriterien

Nicht alle Metriken sind gleich wichtig. Die Gewichtung spiegelt die
Sicherheitsrelevanz und die Einsatzkritikalitaet wider.

**Kritische Funktionen (Gewicht 3x):**
- DTC-Haeufigkeit (keine kritischen DTCs waehrend Fahrt)
- GPX-Log-Vollstaendigkeit (Datenverlust ist nicht akzeptabel)
- Watchdog-Resets (Systemstabilitaet)
- Bordspannung-Abweichung (falsche Spannungsanzeige kann zu Fehldiagnosen fuehren)
- BLE-Abbrueche/h (Verbindung zur App muss fuer Notfall-Funktion stehen)
- Power-Overbrueckung (System muss Anlassvorgang ueberstehen)
- Heap-Speicher (Memory-Leak fuehrt unweigerlich zum Absturz)
- Stack-High-Water (Stack-Overflow fuehrt zum Absturz)
- Gehaeuse-Wasserdichtigkeit (Wassereintritt zerstoert Elektronik)

**Wichtige Funktionen (Gewicht 2x):**
- RPM-Abweichung (Drehzahlanzeige fuer Shift-Light wichtig)
- Speed-Abweichung (Geschwindigkeitsanzeige)
- BLE-Reconnect-Zeit (Verbindung muss schnell wiederhergestellt werden)
- MCU-Temperatur (thermische Stabilitaet fuer Dauerbetrieb)

**Ergaenzende Funktionen (Gewicht 1x):**
- Kuehlmittel-Temp-Drift
- Kraftstoff-Abweichung
- GPS-Positionsgenauigkeit
- Display-FPS
- BLE-RSSI
- Display-Temperatur
- DTC (nicht-kritisch)
- Boot-Zeit
- GPS-Fix-Zeit

### 4.3 Gesamtbewertung

Die Gesamtbewertung eines Test-Durchlaufs ergibt sich aus der gewichteten
Summe der bestandenen und durchgefallenen Metriken.

| Bewertung | Bedingung |
|---|---|
| Sehr Gut | Alle kritischen Metriken Pass, max. 1 nicht-kritische Metrik marginal |
| Gut | Alle kritischen Metriken Pass, max. 2 nicht-kritische Metriken Fail |
| Minimal akzeptabel | Alle kritischen Metriken Pass, max. 3 nicht-kritische Metriken Fail |
| Nicht bestanden | Mindestens eine kritische Metrik Fail |

### 4.4 Abnahmekriterien fuer Release-Readiness

Das System wird fuer den Feldeinsatz freigegeben, wenn:

1. Die Phasen P1 bis P6 mindestens mit "Minimal akzeptabel" bestanden wurden.
2. Phase P7 (Dauertest) mindestens mit "Gut" bestanden wurde.
3. Phase P8 (Witterungs-Test) mindestens mit "Minimal akzeptabel" bestanden
   wurde, wobei das Szenario P8.1 (Regen) zwingend mit "Gut" bestanden
   werden muss (Wasserdichtigkeit ist kritisch).
4. Kein einziger Watchdog-Reset in P3 bis P7 aufgetreten ist.
5. Kein einziger kritischer DTC in P3 bis P7 aufgetreten ist.
6. Die GPX-Track-Vollstaendigkeit in P7 bei > 99 % liegt.
7. Der freie Heap-Speicher am Ende von P7 nicht mehr als 20 % vom
   Startwert abweicht.
8. Das Gehaeuse nach allen Witterungs-Tests (P8.1 bis P8.6) trocken ist.

---

## 5. Test-Datenerfassung

### 5.1 Log-Formate

Waehrend der Feldtests werden drei Arten von Daten aufgezeichnet:

**Serial-Console-Log (USB-C):**
- Nur in P1 (Labortest) und teilweise P2 (Stand-Test) verfuegbar.
- Format: Freitext mit Timestamps (millis() seit Boot).
- Inhalt: Boot-Meldungen, Self-Test-Ergebnisse, DTC-Meldungen,
  Heap-Statistiken (alle 30 s), Task-Statistiken, GPS-Fix-Events,
  BLE-Events (Connect/Disconnect/Reconnect), SD-Schreibfehler.
- Erfassung: USB-C an Computer, Terminal-Programm mit Logging
  (115200 Baud, 8N1, kein Flow-Control).

**SD-Karten-Log (CSV-Telemetrie):**
- Kontinuierlich in allen Phasen.
- Format: CSV, ein Datensatz pro 200 ms (5 Hz).
- Dateiname: /sdcard/logs/YYYYMMDD_HHMMSS.csv
- Felder: timestamp_ms,lat,lon,speed_kmh,rpm,temp_c,alt_m,roll_deg,pitch_deg,
  coolant_temp_c,fuel_percent,battery_v,gear,satellites,fix
- Zusaetzlich: /sdcard/logs/dtc.log fuer DTC-Ereignisse.

**GPX-Track-Datei:**
- Kontinuierlich in P3 bis P8.
- Format: GPX 1.1, XML-basiert.
- Dateiname: /sdcard/tracks/YYYYMMDD_HHMMSS.gpx
- Trackpunkte: Alle 1-5 s (konfigurierbar), mit ele und time.
- Erweiterte Daten als GPX-Extensions (RPM, Temp, Fuel als Custom-Extension):

```xml
<trkpt lat="50.9795" lon="11.3265">
  <ele>325.0</ele>
  <time>2026-10-13T14:30:22Z</time>
  <extensions>
    <bordcomputer:rpm>4500</bordcomputer:rpm>
    <bordcomputer:temp>85</bordcomputer:temp>
    <bordcomputer:fuel>75</bordcomputer:fuel>
  </extensions>
</trkpt>
```

### 5.2 Companion-App-Log

Die Companion-App protokolliert BLE-Ereignisse auf dem Smartphone:
- Connect/Disconnect-Zeitstempel
- RSSI-Werte (alle 5 s)
- MTU-Exchange-Ergebnis
- Reconnect-Dauer
- Notify-Empfangs-Statistik (Paketverlust)
- OTA-Transfer-Fortschritt und -Ergebnis

Das App-Log kann nach der Fahrt als CSV exportiert werden und dient als
ergaenzende Datenquelle zur SD-Karten- und Serial-Log-Auswertung.

### 5.3 Foto-/Video-Dokumentation

Fuer die Bewertung der Display-Lesbarkeit wird eine standardisierte
Foto-Dokumentation durchgefuehrt:

| Bedingung | Wann | Foto-Inhalt |
|---|---|---|
| Direkte Sonne | P3, P4, P5, P8.2 | Display bei Sonnenlicht, Blende und Winkel dokumentiert |
| Nacht | P8.5 | Display bei Nacht (Backlight an) |
| Regen | P8.1 | Display bei Regen (Tropfen auf Abdeckung) |
| Schatten | P3, P4 | Display im Schatten (Referenz) |

Bewertungsskala (1-5):
- 5: Alle Werte klar lesbar, kein Kontrastverlust
- 4: Alle Werte lesbar, leichter Kontrastverlust
- 3: Hauptwerte (Speed, RPM) lesbar, Nebenwerte schwer
- 2: Nur Hauptwerte (Speed) lesbar
- 1: Display unlesbar

Zusaetzlich wird bei jeder Testfahrt ein kurzes Video (30 s) vom Display
aufgenommen, um die Aktualisierungsrate und etwaiges Flackern zu dokumentieren.

### 5.4 Nachtraegliche Analyse

Nach jeder Testfahrt werden die gesammelten Daten ausgewertet:

**GPX-Track-Analyse:**
- Tool: GPX-Editor oder Python-Skript (gpxpy-Library)
- Pruefung: XML-Validitaet, Punktanzahl vs. erwartete Punktanzahl,
  Zeitstempel-Monotonie, Plausibilitaet der Koordinaten (keine Spruenge > 100 m
  zwischen aufeinanderfolgenden Punkten bei < 50 km/h).
- Statistik: Gesamtstrecke, Dauer, Durchschnittsgeschwindigkeit, max Speed,
  Hoehenprofil.

**CSV-Telemetrie-Analyse:**
- Tool: Python-Skript (pandas), Tabellenkalkulation
- Pruefung: Vollstaendigkeit (keine Luecken im timestamp), Plausibilitaet
  (RPM 0-12000, Temp -20 bis 130, Speed 0-200, Voltage 8-16).
- Statistik: Durchschnittswerte, Min/Max, Standardabweichung pro Metrik.

**Serial-Log-Analyse:**
- Tool: Text-Editor, grep, Python-Skript
- Pruefung: Boot-Cause (Power-On vs. Watchdog vs. Panic), DTC-Meldungen,
  Heap-Verlauf (Memory-Leak?), Task-Watchdog-Events, BLE-Event-Statistik.
- Heap-Trend: Freier Heap alle 30 s geloggt. Trend muss stabil sein
  (Abweichung < 20 % vom Startwert nach 8 h).

**App-Log-Analyse:**
- Tool: CSV-Export aus App, Python-Skript
- Pruefung: BLE-Verbindungsabbrueche (Anzahl, Dauer, Reconnect-Zeit),
  RSSI-Verlauf, Paketverlust-Rate.
- Statistik: Median Reconnect-Zeit, max Abbruch-Dauer, durchschnittlicher RSSI.

### 5.5 Checkliste fuer Datenerfassung

Vor jeder Testfahrt ist folgende Checkliste abzuarbeiten:

- [ ] MicroSD-Karte eingesetzt (8-32 GB, Class 10, FAT32)
- [ ] USB-C-Verbindung nur bei P1/P2 (Serial-Log)
- [ ] Companion-App gestartet, BLE verbunden
- [ ] GPS-Fix abwarten (LED blau durchgehend)
- [ ] Tour-Aufzeichnung starten (Doppeldruck oder App)
- [ ] Smartphone in Brusttasche (fuer BLE)
- [ ] Foto-Ausruestung bereit (Kamera/Smartphone fuer Display-Fotos)
- [ ] Referenz-GPS-Geraet eingeschaltet (fuer GPX-Vergleich)
- [ ] Multimeter griffbereit (fuer Spannungskontrolle bei P2)
- [ ] IR-Thermometer griffbereit (fuer Temperatur-Referenz bei P2)

Nach jeder Testfahrt:

- [ ] Tour-Aufzeichnung stoppen
- [ ] SD-Karte entnehmen und am Computer auslesen
- [ ] GPX-Datei validieren (XML-Struktur, Punktanzahl)
- [ ] CSV-Telemetrie auslesen
- [ ] Serial-Log sichern (falls P1/P2)
- [ ] App-Log exportieren
- [ ] Fotos/Videos sortieren und benennen
- [ ] Gehaeuse oeffnen und Innenraum auf Feuchtigkeit pruefen
- [ ] DTC-Liste ueber App auslesen
- [ ] Alle Ergebnisse in Test-Report eintragen

---

## 6. Risiken und Mitigation

### 6.1 Motorrad-Sturz

**Risiko:** Ein Sturz waehrend des Feldtests kann den Bordcomputer mechanisch
beschaedigen (Gehaeusebruch, Displaybruch, PCB-Schaden) und zu Datenverlust
fuehren (SD-Karte wird nicht mehr flush-geschrieben).

**Mitigation:**
- Gehaeuse aus ASA (schlagzaeh, UV-beständig), Wandstaerke 2,4-3,2 mm,
  mit Versteifungsrippen an den Schraubsaeulen.
- Display hinter 2 mm Acrylglas/Polycarbonat-Abdeckung mit hydrophober
  Beschichtung.
- PCB mit 4 x M3 x 8 mm Schrauben auf Gummipuffern fixiert (Vibrationsentkopplung).
- SD-Karte: logger_task schreibt in 64-Eintrag-Batches (Ringbuffer), flush
  alle 200 ms. Bei Sturz gehen maximal die letzten 200 ms verloren.
- DTC B0301 (Watchdog-Timeout) und S0401 (Crash > 3g) werden in NVS
  gespeichert und ueberleben einen Reboot.
- Sturz-Schwelle fuer Crash-Erkennung: > 3g (konfigurierbar). Bei Ausloesung
  wird EVT_CRASH_DETECTED im Event-Bus gesetzt und die letzte GPS-Position
  an die Companion-App gesendet (Notfall-Funktion).
- Testfahrer tragt Schutzkleidung (Helm, Handschuhe, Stiefel, Rueckenprotektor).
- Gelaende-Test (P6) nur auf vertrauten Strecken durchfuehren.

### 6.2 Extreme Temperaturen

**Risiko:** Bei Temperaturen ueber 35 C Umgebung und direkter Sonneneinstrahlung
kann das Gehaeuseinnere 60-70 C erreichen. Der ESP32-S3 ist bis 85 C spezifiziert,
aber Thermal-Throttling kann ab ca. 75 C einsetzen. Bei Temperaturen unter 0 C
kann das E-Paper-Display trage werden (Breittemperatur-Variante -20 bis +60 C
erforderlich). Der LiPo-Pufferakku darf unter 0 C nicht geladen werden
(Lithium-Plating-Gefahr).

**Mitigation:**
- Hellfarbiges ASA-Gehaeuse (weiss oder hellgrau) reflektiert Sonnenstrahlung,
  haelt Innentemperatur 10-15 C niedriger als schwarz.
- MCU-Temperatur wird vom BMP280 (Umgebungstemperatur im Gehaeuse) ueberwacht.
  Bei > 70 C wird ein DTC (DTC_OVERTEMPERATURE) gesetzt und das
  Thermal-Throttling aktiviert (Display-Helligkeit reduzieren, Animations-Rate
  senken).
- Bei > 80 C wird eine Warnung auf dem Display ausgegeben und die
  Display-Backlight-Helligkeit auf 50 % reduziert.
- LiPo-Ladung wird bei < 0 C blockiert (Software-Logik im power_task:
  BMP280-Temp < 0 C -> Ladung aus).
- Bei > 45 C wird der Ladestrom reduziert (Software-Logik).
- E-Paper-Breittemperatur-Variante fuer den Einsatz unter 0 C.
- Dauertest (P7) im Sommer: Innentemperatur alle 30 s im Serial-Log protokollieren.
- Witterungs-Test P8.3 (Kalt) und P8.4 (Hitze) pruefen die Grenzbereiche.

### 6.3 Regen und Wasser

**Risiko:** Wassereintritt ins Gehaeuse kann Kurzschluesse auf dem PCB
verursachen, das Display beschaedigen und die Elektronik zerstoeren.
Kondensation kann bei Temperaturwechseln (kalte Nacht, warme Fahrt) im
Gehaeuseinnern auftreten.

**Mitigation:**
- Gehaeuse mit IP65/IP67-Schutzklasse: Silikon-O-Ring (2 mm Cord) als
  Hauptdichtung zwischen Deckel und Unterteil. Deckel greift ueber Unterteil
  (Shingling-Prinzip), Wasser laeuft an der Naht nach aussen ab.
- Kabelverschraubungen (PG7/PG9, PA66, IP67) mit O-Ring zwischen Verschraubung
  und Gehaeusewand.
- ePTFE-Druckausgleichsmembran (M6) verhindert Kondensation durch
  Feuchtigkeitsaustausch, ohne Wasser eintreten zu lassen.
- PCB mit Conformal Coating (Polyurethan) geschuetzt.
- Display-Abdeckung mit hydrophober Beschichtung (Tropfen perlen ab).
- Schrauben von unten/Seite (niemals von oben), Wasser laeuft nicht in
  Schraubenloecher.
- Kein Hochdruckreiniger auf Gehaeuse (trotz IP67 kann Druck Wasser durch
  Dichtungen pressen).
- Nach jedem Regen-Test (P8.1): Gehaeuse oeffnen, Innenraum auf Feuchtigkeit
  pruefen. Bei Kondensation: Gehaeuse offen trocknen lassen, Dichtungen
  pruefen.
- Kabeldurchfuehrungen mit Schrumpfschlauch mit Kleber (adhesive-lined)
  ueberziehen.

### 6.4 Vibration

**Risiko:** Die PD06 produziert am Lenker ca. 5-10 g Vibration (V2-Motor,
50-200 Hz). Dauerhafte Vibration kann Loetkontakte brechen, Steckverbinder
lockern, das E-Paper-Glas-Substrat reissen und Schrauben losen.

**Mitigation:**
- Gummi-Metall-Verbunddaempfer (Shore 50-60A, M6, 10 mm Gummi-Hoehe) zwischen
  Halterung und Gehaeuse reduzieren hochfrequente Vibration.
- Alle Schrauben mit Federscheiben (DIN 127B) gesichert.
- Loetkontakte: PCB mit Conformal Coating zusaetzlich stabilisiert.
  Durchkontaktierungen fuer mechanisch belastete Anschluesse.
- Steckverbinder: Nur verriegelte Typen (AMP Superseal oder aehnlich, IP67).
  Kein friction-lock oder einfache Pin-Header fuer externe Anschluesse.
- E-Paper-Display auf Silikon-Pad gelagert (Stossdaempfung), im PETG-Rahmen
  fixiert.
- Kabel nicht frei haengend: alle 10-15 cm mit Kabelbindern am Rahmen fixiert.
- Nach jedem Gelaende-Test (P6) und Dauertest (P7): Sichtkontrolle aller
  Steckverbinder und Schrauben. Bei gelockerten Verbindungen: Ursache
  analysieren (Daempfer defekt? Schwingungsresonanz?).

### 6.5 EMV (Elektromagnetische Vertraeglichkeit)

**Risiko:** Die CDI-Zuendung der PD06 erzeugt starke elektromagnetische
Stoerungen (Hochspannungspulse 200-400 V am Ladekondensator, 20-40 kV an der
Zuendspule). Diese koennen Sensorkabel, den ADC und die MCU stoeren. Umgekehrt
kann der Bordcomputer Stoerstrahlung abgeben, die andere Elektronik
beeinflusst (obwohl die PD06 keine sensible Elektronik hat, sollten
Funkgeraete nicht gestoert werden).

**Mitigation:**
- Optokoppler (PC817 oder 6N137) fuer galvanische Trennung des RPM-Signals
  von der CDI. Der Optokoppler-Eingang ist mit 22-47 kOhm Vorwiderstand
  hochohmig an das Pickup-Signal gekoppelt, um die CDI nicht zu belasten.
- Geschirmte Kabel (SFTP oder S/UTP) fuer alle Sensorkabel.
- Schirm an einem Ende auf Masse (Gehaeuseseite), um Masseschleifen zu
  vermeiden.
- Ferritkerne (Klappferrite) an jedem Kabel, das ins Gehaeuse muendet,
  nah am Gehaeuse.
- Kupferfolie auf Gehaeuse-Innenseite als EMV-Abschirmung (mit Fenster fuer
  BLE- und GPS-Antenne, mindestens 30 x 30 mm frei).
- Sensorkabel fern von CDI-Box und Zuendspulen verlegen (rechte Seite des
  Rahmens, nicht links neben der Auspuffanlage).
- Sternpunkt-Masse: Alle Masseleitungen an einem Punkt zusammengefuehrt, um
  Masseschleifen zu vermeiden.
- Stand-Test (P2) prueft das EMV-Verhalten mit laufendem Motor. Die
  RPM-Anzeige muss bei laufendem Motor stabil sein (keine Spruenge durch
  Zündstoerungen). Das Display darf nicht flackern oder ausfallen.
- ADC-Kanaele: 16x Oversampling fuer Rauschunterdrueckung, RC-Filter
  (1 kOhm + 100 nF) am ADC-Eingang.

### 6.6 Datenverlust

**Risiko:** SD-Karten koennen ausfallen (Kontaktverlust durch Vibration,
Dateisystem-Korruption durch unerwartetes Abschalten, Verschleiss der
Flash-Zellen). Der LiPo-Pufferakku kann entladen werden, wenn der Generator
ausfaellt und das System nicht rechtzeitig abschaltet.

**Mitigation:**
- SD-Karte: Class 10, 8-32 GB, FAT32 (nicht exFAT, da exFAT weniger robust
  bei unerwartetem Abschalten).
- logger_task: Ringbuffer mit 64 Eintraegen, Batch-Write alle 200 ms.
  Bei SD-Schreibfehler wird EVT_SDCARD_ERROR im Event-Bus gesetzt und
  DTC C0202 (SD-Schreibfehler) gemeldet.
- NVS (Non-Volatile Storage): Konfiguration und DTC-Tabelle werden im NVS
  gespeichert (separat von SD-Karte). NVS ueberlebt Stromausfall.
- Power-Management: power_task ueberwacht Bordspannung. Bei < 11,5 V wird
  DTC B0401 (Batterie niedrig) gesetzt. Bei < 10,5 V (konfigurierbar) wird
  EVT_SHUTDOWN ausgeloest: SD-Karte wird geflusht (logger_task flush),
  NVS wird geschrieben, System geht in SHUTDOWN-Zustand.
- Boot-Loop Detection: Bei mehr als 3 Resets innerhalb von 60 Sekunden wird
  DTC B0401 gesetzt und ggf. ein OTA-Rollback ausgeloest (falls die
  Firmware kuerzlich aktualisiert wurde).
- Nach jeder Testfahrt: SD-Karte entnehmen, GPX-Datei auf XML-Validitaet
  pruefen, CSV auf Vollstaendigkeit pruefen.

---

## 7. Test-Schedule

### 7.1 Test-Reihenfolge und Abhaengigkeiten

Die Test-Phasen muessen in einer bestimmten Reihenfolge durchgefuehrt werden,
da jede Phase auf den Ergebnissen der vorherigen aufbaut. Die Abhaengigkeiten
sind im folgenden ASCII-Diagramm dargestellt (Abschnitt 8).

```
P1 (Labortest)
  |
  v
P2 (Stand-Test)
  |
  v
P3 (Feldtest Stadt) -----> P4 (Feldtest Landstrasse)
  |                              |
  |                              v
  |                    P5 (Feldtest Autobahn)
  |                              |
  |                              v
  |                    P6 (Feldtest Gelaende)
  |                              |
  |                              v
  +----------------------------> P7 (Dauertest 500 km)
  |                              |
  |                              v
  +----------------------------> P8 (Witterungs-Test)
```

P1 muss als erstes bestanden werden. P2 setzt P1 voraus. P3 setzt P2 voraus.
P4 setzt P3 voraus. P5 und P6 koennen parallel oder sequenziell nach P4
durchgefuehrt werden. P7 (Dauertest) setzt P4, P5 und P6 voraus. P8
(Witterungs-Test) kann parallel zu P7 durchgefuehrt werden, setzt aber P6
voraus (System muss mechanisch fuer Gelaende verifiziert sein).

### 7.2 Minimal-Test-Suite fuer schnelle Regression

Fuer schnelle Regressionstests nach Aenderungen an der Firmware oder Hardware
wird eine reduzierte Test-Suite verwendet, die die kritischsten Pruefungen
enthaelt. Diese Suite dauert ca. 2 Stunden und deckt die wichtigsten
Fehlerquellen ab.

| Schritt | Phase (gekuerzt) | Dauer | Kritische Pruefung |
|---|---|---|---|
| R1 | P1 (Labortest, gekuerzt) | 30 min | Boot, Self-Test, Sensor-Simulation, 10 min Dauerbetrieb |
| R2 | P2 (Stand-Test, gekuerzt) | 20 min | Anlassen, Leerlauf, Bordspannung, EMV |
| R3 | P3 (Stadt, gekuerzt) | 30 min | 15 min Stadtfahrt, GPX-Vollstaendigkeit, BLE-Stabilitaet |
| R4 | P8.1 (Regen, gekuerzt) | 15 min | 10 min Regenfahrt, Wasserdichtigkeit |
| R5 | Auswertung | 25 min | SD auslesen, GPX validieren, DTC-Liste, Heap-Check |

**Pass-Kriterium fuer Regression:** Alle kritischen Metriken (DTC, Watchdog,
GPX-Vollstaendigkeit, Heap, Wasserdichtigkeit) Pass. Kein Absturz.

### 7.3 Vollstaendiger Test-Zyklus fuer Release

Fuer ein Release (Firmware-Version oder Hardware-Aenderung) wird der
vollstaendige Test-Zyklus durchgefuehrt:

| Tag | Phase | Dauer | Wetter |
|---|---|---|---|
| 1 | P1: Labortest | 2-4 h | Innen |
| 1 | P2: Stand-Test | 30 min | Draussen, trocken |
| 2 | P3: Feldtest Stadt | 30 min + Auswertung | Trocken, > 10 C |
| 2 | P4: Feldtest Landstrasse | 60 min + Auswertung | Trocken, > 10 C |
| 3 | P5: Feldtest Autobahn | 30 min + Auswertung | Trocken, > 10 C |
| 3 | P6: Feldtest Gelaende | 30 min + Auswertung | Trocken, > 10 C |
| 4 | P7: Dauertest | 8 h+ (500 km) | Variabel |
| 5 | P8.1-P8.3: Witterung (Regen, Sonne, Kalt) | je 30 min | Entsprechend |
| 6 | P8.4-P8.6: Witterung (Hitze, Nacht, Kondensation) | je 30 min | Entsprechend |
| 6 | Gesamtauswertung | 2 h | - |

Gesamtdauer: ca. 6 Tage (mit Auswertung). Die Witterungs-Tests (P8) koennen
sich ueber mehrere Tage verteilen, da sie von den Wetterbedingungen abhaengen.

### 7.4 Test-Dokumentation

Jede Testfahrt wird in einem Test-Report dokumentiert. Der Report enthaelt:

- Datum, Uhrzeit, Wetterbedingungen
- Getestete Phase(n)
- Fahrername
- Motorrad-Kilometerstand
- Firmware-Version
- Hardware-Version
- Companion-App-Version
- Ergebnisse pro Schritt (Pass/Fail/N/A)
- Metrik-Werte (aus SD-Karte und App-Log)
- DTC-Liste nach Fahrt
- GPX-Datei-Statistik (Punkte, Strecke, Dauer)
- Heap-Statistik (Start, Ende, Min, Max)
- Fotos der Display-Lesbarkeit
- Notizen und Beobachtungen
- Gesamtbewertung (Sehr Gut / Gut / Minimal akzeptabel / Nicht bestanden)

Der Test-Report wird digital gespeichert (z.B. als Markdown-Datei im
Projektverzeichnis unter docs/test-reports/) und ist Grundlage fuer die
Release-Freigabe.

---

## 8. Test-Ablauf-Diagramm

Das folgende ASCII-Diagramm zeigt den vollstaendigen Test-Ablauf von P1 bis
zur Release-Freigabe:

```
                        +------------------+
                        |   START          |
                        |   Test-Zyklus    |
                        +--------+---------+
                                 |
                                 v
                    +------------+------------+
                    |  P1: Labortest (2-4 h) |
                    |  Test-Bench, kein Moto |
                    |  12 Schritte, Self-Test|
                    |  Sensor-Simulation     |
                    +------------+----------+
                                 |
                          Pass?  |
                    +------+----+----+------+
                    |      |         |      |
                   Ja     Nein      Nein   Nein
                    |   (Abbruch)         |
                    v      |              |
          +---------+--+   |              |
          |  P2: Stand-Test (30 min)      |
          |  Motorrad, Motor laeuft       |
          |  EMV, Spannung, Anlassen      |
          +-------+-------+               |
                  |                       |
           Pass?  |                       |
          +-------+-------+              |
          |               |              |
         Ja             Nein (Abbruch)   |
          |               |              |
          v               v              |
    +-----+-----+                        |
    | P3: Stadt  |                        |
    | 30 min     |                        |
    | Stop-and-Go|                        |
    +-----+------+                        |
          |                               |
   Pass?  |                               |
    +-----+-----+                         |
    |           |                         |
   Ja         Nein (Abbruch)             |
    |           |                         |
    v           v                         |
  +--+--------+                          |
  | P4: Land-  |                         |
  | strasse    |                         |
  | 60 min     |                         |
  | 50-100 km/h|                         |
  +--+--------+                          |
     |                                   |
     v                                   |
  +---+---------+                        |
  | P5: Autobahn |                        |
  | 30 min       |                        |
  | 100-130 km/h |                        |
  +---+---------+                        |
     |                                   |
     v                                   |
  +---+---------+                        |
  | P6: Gelaende |                        |
  | 30 min       |                       |
  | Forstweg     |                       |
  +---+---------+                        |
     |                                   |
     v                                   |
  +---+---------+                        |
  | P7: Dauer-   |<----------------------+
  | test 500 km  |
  | 8 h+         |
  +---+---------+
     |
     v
  +---+----------+
  | P8: Witterung|
  | 6 Szenarien  |
  | Regen/Sonne/ |
  | Temp/Nacht   |
  +---+----------+
     |
     v
  +---+-----------------+
  | Auswertung          |
  | GPX/CSV/Serial/App  |
  | Bewertungskriterien |
  +---+-----------------+
     |
     v
  +---+------------------+      +-------------------+
  | Release-Readiness   |----->| FREIGEGEBEN       |
  | Pruefung            |      | fuer Feldeinsatz  |
  | (Abschnitt 4.4)     |      +-------------------+
  +---+------------------+
     |
     | Nicht bestanden
     v
  +---+-----------------+
  | Nachbesserung      |
  | Fehleranalyse      |
  | Neuer Test-Zyklus  |
  +---------------------+
```

### 8.1 Entscheidungsbaum fuer Abbrueche

Wenn eine Phase abgebrochen wird (Abbruch-Kriterium erfuellt), ist die
weitere Vorgehensweise:

1. **Ursachenanalyse:** Serial-Log, DTC-Liste und App-Log auswerten.
   Fehlerursache identifizieren (Hardware, Firmware, Konfiguration).
2. **Behebung:** Fehler beheben (Patch, Konfigurationsaenderung,
   Hardware-Reparatur).
3. **Regression:** Minimal-Test-Suite (Abschnitt 7.2) durchfuehren, um
   die Behebung zu verifizieren.
4. **Wiederholung:** Die abgebrochene Phase vollstaendig wiederholen.
5. **Dokumentation:** Abbruch, Ursache, Behebung und Wiederholung im
   Test-Report dokumentieren.

Bei wiederholtem Abbruch in derselben Phase (3 oder mehr Versuche) wird
die Release-Freigabe zurueckgezogen und eine grundlegende Ueberarbeitung
der betroffenen Komponente eingeleitet.

---

## 9. Anhang

### 9.1 PD06-spezifische Test-Parameter

Die folgenden Parameter sind spezifisch fuer die XL600V Transalp (PD06)
und muessen bei der Testdurchfuehrung beruecksichtigt werden:

| Parameter | Wert | Relevanz fuer Test |
|---|---|---|
| Leerlaufdrehzahl | 1300 +/- 100 U/min | P2.3: RPM-Anzeige im Leerlauf |
| Nenndrehzahl | 8000 U/min | P2.5: Shift-Light-Schwelle bei 7000 |
| Roter Bereich | ab 9000 U/min | Shift-Light rote Zone bei 8500 |
| Pickup-Pulse | 1 pro Kurbelwellenumdrehung | RPM-Berechnung |
| Thermostat oeffnet | 80-84 C | P4.7: Kuehlmitteltemp stabil > 84 C |
| Luefter schaltet ein | 98-102 C | P3.6: max. 110 C Warnschwelle |
| Generator | 310-350 W bei 5000 U/min | P2.7: Bordspannung > 13,5 V bei 5000 |
| Bordspannung ( Leerlauf) | 12-13 V | P2.6: untere Grenze |
| Bordspannung (Fahrt) | 13,5-15,5 V | P4.6: Normalbereich |
| Tankinhalt | 18 L (2,9-3,5 L Reserve) | P7: Kraftstoff-Planung |
| Verbrauch | 4,5-5,5 L/100 km | P7: Reichweite ca. 300-350 km |
| Bereifung vorne | 90/90-21, Umfang 2185 mm | Speed-Berechnung (Hall-Sensor) |
| Bereifung hinten | 130/80-17, Umfang 2010 mm | Tacho-Referenz |
| Lenker | 22 mm (7/8") | Montage |
| Vibration am Lenker | 5-10 g (V2-Motor) | P6: Vibrations-Festigkeit |
| Gewichte Uebersetzung | 1.=2,500, 5.=0,961 (spaete Modelle) | P4.4: Ganganzeige |

### 9.2 Referenz-Messgeraete

Fuer die Metrik-Erfassung werden folgende Referenz-Messgeraete benoetigt:

- Multimeter (Spannung, Widerstand): fuer Bordspannung und NTC-Pruefung
- IR-Thermometer: fuer Kuehlmitteltemperatur-Referenz
- Funktionsgenerator: fuer RPM- und Speed-Simulation im Labortest
- Zweites GPS-Geraet (oder Smartphone): fuer GPS-Track-Referenz
- Oszilloskop: fuer Pickup-Signal-Verifikation (nur P1, optional P2)
- Waage (optional): fuer Kraftstoff-Waegung als Referenz

### 9.3 Glossar

- **ADC**: Analog-to-Digital Converter (Analog-Digital-Umsetzer)
- **BLE**: Bluetooth Low Energy
- **CDI**: Capacitive Discharge Ignition (Kondensatorzuendung)
- **CEP50**: Circular Error Probable (50 % der Messpunkte innerhalb dieses Radius)
- **DTC**: Diagnostic Trouble Code (Fehlercode)
- **EMV**: Elektromagnetische Vertraeglichkeit
- **FPS**: Frames Per Second (Bildwiederholrate)
- **GATT**: Generic Attribute Profile (BLE-Protokoll)
- **GPX**: GPS Exchange Format (XML-basiertes Track-Format)
- **GPIO**: General Purpose Input/Output
- **Heap**: Dynamischer Arbeitsspeicher
- **HIL**: Hardware-in-the-Loop
- **I2C**: Inter-Integrated Circuit (serieller Bus)
- **IMU**: Inertial Measurement Unit (Beschleunigungssensor)
- **IP65/IP67**: Schutzklasse gegen Fremdkoerper und Wasser
- **LDO**: Low Dropout Regulator (Spannungsregler)
- **MCU**: Microcontroller Unit
- **MTU**: Maximum Transmission Unit (BLE-Paketgroesse)
- **NMEA**: National Marine Electronics Association (GPS-Datenformat)
- **NVS**: Non-Volatile Storage (nichtfluechtiger Speicher)
- **OTA**: Over-The-Air (Funk-Update)
- **PCB**: Printed Circuit Board (Leiterplatte)
- **PCNT**: Pulse Counter (ESP32-Hardware-Zaehler)
- **PSRAM**: Pseudo-Static RAM (externer RAM)
- **RPM**: Revolutions Per Minute (Drehzahl)
- **RSSI**: Received Signal Strength Indicator (BLE-Signalstaerke)
- **RTC**: Real-Time Clock
- **SD**: Secure Digital (Speicherkarte)
- **SPI**: Serial Peripheral Interface (serieller Bus)
- **Stack**: Stackspeicher je Task
- **TFT**: Thin Film Transistor (Display-Typ)
- **UART**: Universal Asynchronous Receiver/Transmitter (serielle Schnittstelle)
- **Watchdog**: Hardware-Timer, der System bei Blockade neu startet

---

*Dieses Dokument ist die alleinige Referenz fuer Feldtests des Bordcomputers.
Aenderungen an der Test-Strategie (TEST-STRATEGY.md) oder der Architektur
(ARCHITECTURE.md) muessen hier nachgezogen werden, wenn sie Test-Phasen oder
Metriken beeinflussen.*

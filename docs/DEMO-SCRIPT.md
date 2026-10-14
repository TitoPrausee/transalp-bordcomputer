# Demo-Video-Skript & Projekt-Präsentation

> Phase 6 — Demo-Materialien fuer den Transalp Bordcomputer
> Stand: Oktober 2026
> Fahrzeug: Honda XL600V Transalp 600 (PD06), Baujahr 1986–2000
> Repository: github.com/TitoPrausee/transalp-bordcomputer
> Lizenz: PolyForm Noncommercial 1.0.0

---

## Inhaltsverzeichnis

1. [Demo-Video-Skript](#1-demo-video-skript)
2. [Projekt-Präsentation (Slide-Konzept)](#2-projekt-praesentation-slide-konzept)
3. [Social-Media-Kurzform](#3-social-media-kurzform)

---

## 1. Demo-Video-Skript

**Gesamtlaufzeit:** ca. 6 Minuten (360 Sekunden)
**Zielgruppe:** Motorrad-Enthusiasten, Maker, Open-Source-Entwickler, PD06-Fahrer
**Sprache:** Deutsch
**Format:** 16:9, 1080p oder hoeher

### Produktionstabelle

| Szene | Titel | Dauer | Kumuliert |
|---|---|---|---|
| 1 | Intro | 30 s | 0:30 |
| 2 | Hardware-Ueberblick | 60 s | 1:30 |
| 3 | Firmware-Demo | 90 s | 3:00 |
| 4 | Companion-App | 90 s | 4:30 |
| 5 | Einbau am Motorrad | 60 s | 5:30 |
| 6 | Fazit & Call-to-Action | 30 s | 6:00 |

---

### Szene 1: Intro (30 s)

**Bild/Video-Beschreibung:**

Die Kamera faehrt langsam ueber eine Honda Transalp 600 (PD06) im Gegenlicht
der Morgensonne. Das Motorrad steht auf einem Feldweg, im Hintergrund
Berglandschaft. weicher Fokus geht in einen harten Schnitt auf das
TFT-Display des Bordcomputers, der gerade hochfaehrt — der Boot-Screen mit
dem Text „TRANSALP BORDCOMPUTER" leuchtet auf. Titel-Overlay blendet sich ein.

**Mockup/Screenshot-Referenz:**
- `mockups/screens/boot.html` — Boot-Splash-Animation mit Wachstums-Sequenz

**Voice-Over (Deutsch):**

> Das ist die Honda Transalp 600, Baujahr 1986 bis 2000 — ein rein mechanisches
> Motorrad. Keine ECU, kein CAN-Bus, keine Diagnoseschnittstelle. Was, wenn man
> trotzdem modernes Cockpit, GPS-Tracking und Smartphone-Anbindung haben will?
>
> Genau das zeigt dieses Projekt: Ein kompletter Open-Source Bordcomputer,
> basierend auf einem ESP32-S3, mit Dual-Display, Sensoren, BLE-Anbindung und
> einer Companion-App. Alles selbst entwickelt, alles dokumentiert, alles
> Open-Source.

**Uebergang:**

Harder Schnitt vom Boot-Screen zu einer Nahaufnahme des aufgebauten
Bordcomputers auf dem Tisch. Die Kamera zoomt auf die Hardware-Komponenten.

---

### Szene 2: Hardware-Ueberblick (60 s)

**Bild/Video-Beschreibung:**

Top-Down-Aufnahme des entfalteten Bordcomputers auf einem dunklen Hintergrund.
Die Kamera bewegt sich langsam ueber die einzelnen Komponenten, jeweils
markiert mit Text-Einblendungen: ESP32-S3-WROOM-1, GPS-Modul, TFT-Display,
E-Paper-Display, IMU, Barometer, Relais-Modul, Backup-Akku. Ein
Schaltplan-Ausschnitt wird kurz eingeblendet.

**Mockup/Screenshot-Referenz:**
- `mockups/screens/instrument.html` — Instrumenten-Screen zeigt die Hardware-
  abhaengigen Widgets in Aktion
- `mockups/assets/icons/icon-gps.svg`, `icon-battery.svg`, `icon-temp.svg`,
  `icon-rpm.svg` — Icon-Referenzen fuer die Sensor-Kategorien

**Voice-Over (Deutsch):**

> Das Herzstueck ist ein ESP32-S3-WROOM-1: Dual-Core mit 240 Megahertz,
> 16 Megabyte Flash, 8 Megabyte PSRAM und integriertem Bluetooth Low Energy.
>
> Daran angeschlossen: ein 3,5-Zoll-TFT-Display fuer Echtzeit-Telemetrie mit
> 30 Bildern pro Sekunde, und ein 2,13-Zoll-E-Paper-Display fuer sonnenlesbare
> statische Werte. Ein GNSS-Modul mit 10-Hertz-Update-Rate liefert Position,
> Geschwindigkeit und Hoehe. Eine 6-Achsen-IMU erfasst Neigung und
> Beschleunigung, ein Barometer misst Hoehe und Luftdruck.
>
> Die Fahrzeugdaten werden direkt abgegriffen: Drehzahl ueber Optokoppler an
> der Zuendspule, Geschwindigkeit ueber Hall-Sensor am Vorderrad,
> Kuehlmitteltemperatur ueber NTC im Kuehlschlauch, Kraftstoffstand ueber
> den Tankgeber. Alles galvanisch getrennt, alles gegen Vibration und
> Spannungsspitzen gesichert.
>
> Ein 18650-LiPo-Akku mit 3400 Milliamperestunden sorgt fuer Stromausfall-
> Sicherheit und ermoeglicht Deep-Sleep mit nur 0,4 Milliampere.

**Uebergang:**

Ueberblende vom Hardware-Aufbau zum eingeschalteten TFT-Display, das gerade
den Haupt-Screen rendert. Die Kamera fokussiert sich auf die Widgets.

---

### Szene 3: Firmware-Demo (90 s)

**Bild/Video-Beschreibung:**

Vollbild-Aufnahme des TFT-Displays. Zunaechst sieht man den Boot-Screen mit
der Animations-Sequenz (Punkt waechst zum Kreis, Text blendet ein). Danach
wechselt das Display in den Standard-Fahr-Layout: grosse Geschwindigkeits-
anzeige in der Mitte, Drehzahl-Balken oben, Gang-Anzeige, Temperatur, Tank,
GPS-Status und Uhrzeit in der Status-Leiste. Die Werte aendern sich live
(simuliert durch Test-Task). Anschliessend wird ein Navigations-Pfeil
eingeblendet, der den naechsten Manoeuver anzeigt. Zum Schluss wird der
Diagnose-Screen gezeigt mit Sensor-Status und DTC-Liste.

**Mockup/Screenshot-Referenz:**
- `mockups/screens/boot.html` — Boot-Animation mit Wachstums-Sequenz
- `mockups/screens/instrument.html` — Standard-Fahr-Layout mit allen Widgets
- `mockups/screens/navigation.html` — Navigations-Modus mit Pfeil und Strasse
- `mockups/screens/diagnose.html` — Diagnose-Screen mit Sensor-Status
- `mockups/screens/warning.html` — Warning-Overlay bei kritischen Zustaenden
- `mockups/screens/tour-statistik.html` — Tour-Statistik nach Fahrtende
- `mockups/assets/icons/icon-nav-arrow.svg`, `icon-gear.svg`, `icon-fuel.svg` —
  Widget-Icon-Referenzen

**Voice-Over (Deutsch):**

> Beim Einschalten laeuft eine animierte Boot-Sequenz: Ein Punkt waechst zu
> einem Kreis, der Projektname blendet ein, dann eine Linie faehrt von links
> nach rechts. Danach startet der Haupt-Screen.
>
> Die Firmware basiert auf ESP-IDF in Version 5.4 mit FreeRTOS im SMP-Modus.
> Sieben Tasks sind auf zwei Kerne aufgeteilt: Sensor- und GPS-Erfassung auf
> Core 1, Display-Rendering, Logging und BLE auf Core 0. Die
> Power-Management-Task hat die hoechste Prioritaet, damit ein kritischer
> Akku-Zustand sofort zum geordneten Shutdown fuehrt.
>
> Das TFT-Display zeigt die Echtzeit-Telemetrie: Geschwindigkeit gross in der
> Mitte, Drehzahl als Balken mit Farbcodierung — gruen im Normalbereich,
> rot ab 85 Prozent als Shift-Light. Daneben: Gang, Kuehlmitteltemperatur,
> Tankstand, GPS-Status, Bordspannung und Uhrzeit. Die Rendering-Pipeline
> verwendet Dirty-Rect-Tracking: nur geaenderte Bildbereiche werden ueber
> SPI mit DMA uebertragen.
>
> Bei aktiver Navigation erscheint ein grosser Pfeil mit Distanz und
> Strassenname. Die Daten kommen ueber BLE von der Smartphone-App. Der
> Pfeil morphed beim Manoeuver-Wechsel weich von einer Richtung zur
> naechsten.
>
> Im Diagnose-Modus zeigt das Display alle Sensor-Status, aktive
> Fehlercodes und das Ergebnis des letzten Self-Tests. 16 DTC-Codes in
> vier Kategorien — Powertrain, Sensors, Comms und Body — werden beim
> Boot geprueft. Bei kritischen Zustaenden erscheint ein rotes
> Warning-Banner oben auf dem Display.
>
> Am Tour-Ende erscheint die Statistik: Distanz, Dauer, Durchschnitts-
> geschwindigkeit, Hoehenmeter. Alles wird automatisch auf SD-Karte
> geloggt — CSV mit 5 Hertz, GPX-Track mit 1 Hertz.

**Uebergang:**

Die Kamera zieht sich vom Bordcomputer zurueck, zeigt nun ein Smartphone
neben dem Geraet. Die Companion-App wird geoeffnet und verbindet sich per
BLE automatisch.

---

### Szene 4: Companion-App (90 s)

**Bild/Video-Beschreibung:**

Aufnahme eines Smartphone-Bildschirms (Screen-Recording). Die Companion-App
oeffnet sich und zeigt den Verbindungs-Status: „Verbinde..." wechselt zu
„Verbunden". Danach werden die fuenf Haupt-Screens durchgegangen:
Dashboard mit Live-Telemetrie, Navigation mit Kartenansicht und
Turn-by-Turn-Pfeil, Diagnose mit DTC-Liste und Sensor-Status, Settings
mit Konfigurationsmoeglichkeiten, und Tours mit vergangener Tour und
GPX-Export. Die Werte auf dem Smartphone spiegeln die Display-Werte
in Echtzeit wider.

**Mockup/Screenshot-Referenz:**
- `mockups/screens/instrument.html` — Dashboard entspricht dem Instrument-Screen
- `mockups/screens/navigation.html` — Navigation-Screen der App
- `mockups/screens/diagnose.html` — Diagnose-Screen der App
- `mockups/screens/settings.html` — Settings-Screen der App
- `mockups/screens/tour-statistik.html` — Tours-Screen der App
- `companion-app/src/screens/` — tatsaechliche React-Native-Implementierung:
  DashboardScreen.tsx, NavigationScreen.tsx, DiagnoseScreen.tsx,
  SettingsScreen.tsx, ToursScreen.tsx

**Voice-Over (Deutsch):**

> Die Companion-App ist eine React-Native-Applikation mit einer einzigen
> Codebasis fuer beide mobilen Betriebssysteme. Sie verbindet sich
> automatisch per Bluetooth Low Energy mit dem Bordcomputer.
>
> Der BLE-GATT-Server stellt acht Custom-Services mit 33 Characteristics
> zur Verfuegung: Telemetrie, Navigation, Media, Phone, Weather, Config,
> Diagnose und OTA-Update. Die Live-Telemetrie wird mit 4 Hertz uebertragen
> — das reicht fuer ein fluessiges Dashboard auf dem Smartphone.
>
> Das Dashboard zeigt Geschwindigkeit, Drehzahl, Temperatur, Tankstand und
> Bordspannung in Echtzeit. Die Navigation uebertraegt Turn-by-Turn-Anweisungen
> vom Smartphone ans Display: Pfeil-Richtung, Distanz und Strassenname
> werden als Binary-Daten gepackt und in unter 20 Byte pro Manoeuver
> uebertragen.
>
> Im Diagnose-Screen lassen sich Fehlercodes auslesen, der Self-Test
> triggern und Sensor-Status abfragen — alles per Bluetooth, ohne
> Werkstattbesuch. In den Settings werden Radumfang, Kraftstoff-Kennlinie,
> Display-Helligkeit und BLE-Bonding verwaltet.
>
> Der Tours-Screen zeigt vergangene Touren mit Statistik und
> GPX-Export-Funktion. Die App ist mit einem Mock-Provider ausgestattet,
> der Testdaten erzeugt — so kann die UI ohne physische Hardware
> entwickelt werden.
>
> Firmware-Updates gehen ueber drei Wege: direkt per Bluetooth, ueber
> eine HTTP-Verbindung oder von der SD-Karte. Bei jedem Update wird
> der SHA-256-Hash verifiziert und ein automatischer Rollback
> durchgefuehrt, falls der Self-Test nach dem Boot fehlschlaegt.

**Uebergang:**

Harder Schnitt von der App-Ansicht zu einer Garage oder Einfahrt. Die
Transalp 600 steht bereit, der Bordcomputer wird montiert.

---

### Szene 5: Einbau am Motorrad (60 s)

**Bild/Video-Beschreibung:**

Aufnahme in einer Garage oder im Freien. Die PD06 steht auf dem
Seitenstaender. Zunaechst sieht man das fertige Gehaeuse — 3D-gedruckt,
wasserdicht, mit Kabeldurchfuehrungen. Der Bordcomputer wird am
Lenker oder Cockpit montiert. Die Verkabelung wird gezeigt: Anschluss
an 12V-Bordnetz, Optokoppler an der Zuendspule, Hall-Sensor am
Vorderrad, NTC im Kuehlschlauch. Der USB-C-Anschluss fuer Laden und
Konfiguration wird gezeigt. Abschliessend: Zuendung an, Bordcomputer
bootet, Display zeigt Live-Daten.

**Mockup/Screenshot-Referenz:**
- `mockups/screens/instrument.html` — Finaler Screen nach Inbetriebnahme
- `mockups/screens/boot.html` — Boot-Sequenz beim Zuendung an
- `docs/INSTALLATION-GUIDE.md` — Detaillierte Montage-Anleitung fuer PD06

**Voice-Over (Deutsch):**

> Der Einbau ist fuer die PD06 detailliert dokumentiert. Das Gehaeuse ist
> 3D-gedruckt, wasserdicht nach IP67, mit Conformal-Coating auf der Platine
> gegen Feuchtigkeit und Vibration.
>
> Die Verkabelung ist straightforward: 12V ans Bordnetz, Optokoppler an die
> Zuendspule fuer die Drehzahl, Hall-Sensor mit Magnet am Vorderrad fuer
> die Geschwindigkeit, NTC im Kuehlschlauch fuer die Kuehlmitteltemperatur.
> Der Spannungsteiler am Tankgeber liefert den Kraftstoffstand. Ueber
> USB-C wird geladen und konfiguriert.
>
> Beim Oeldruck wird ein analoger Sensor an der Oelpumpe verbaut —
> M10x1-Gewinde, wasserdicht. Alle Anschluesse sind mit wasserdichten
> Steckern ausgefuehrt. Die Kalibrierung erfolgt nach dem Einbau ueber
> die Companion-App: Radumfang, Pulse-pro-Umdrehung und Kraftstoff-Kennlinie
> werden in der Konfiguration gespeichert.
>
> Einmal konfiguriert, startet der Bordcomputer bei jeder Zuendung automatisch,
> verbindet sich mit dem Smartphone und beginnt mit dem Logging. Fertig.

**Uebergang:**

Fade auf eine Schluss-Szene: Das Motorrad faehrt los, die Kamera bleibt
zurueck. Text-Overlays erscheinen mit den Projekt-Links.

---

### Szene 6: Fazit & Call-to-Action (30 s)

**Bild/Video-Beschreibung:**

Die Transalp faehrt auf einer Landschaftsstrasse davon. Langsamer Fade zu
einem Abschluss-Bildschirm mit Projekt-Logo, GitHub-Repository-Link und
Lizenz-Hinweis. Text-Overlays: „Open-Source", „ESP32-S3", „PolyForm
Noncommercial", „github.com/TitoPrausee/transalp-bordcomputer".

**Mockup/Screenshot-Referenz:**
- `mockups/screens/boot.html` — Projekt-Logo und -Text vom Boot-Screen

**Voice-Over (Deutsch):**

> Der Transalp Bordcomputer ist komplett Open-Source: 15.000 Zeilen
> Firmware-Code, 6.000 Zeilen Companion-App, 260.000 Woerter Dokumentation,
> 35 UI-Mockups und 4 CI-CD-Workflows. Alles unter der PolyForm-Noncommercial-
> Lizenz.
>
> Egal, ob du einen eigenen Bordcomputer bauen, die Firmware erweitern oder
> einen neuen Sensor-Treiber beisteuern willst — das Repository ist offen.
> Fork es, baue es, fahre damit.
>
> Finde das Projekt auf GitHub unter TitoPrausee/transalp-bordcomputer.

**Ende des Videos.**

---

## 2. Projekt-Präsentation (Slide-Konzept)

**Format:** Markdown-Konzept, umsetzbar in Praesentations-Software
**Anzahl Slides:** 14
**Zielgruppe:** Interessierte, potenzielle Mitwirkende, Community

---

### Slide 1: Titel und Hook

**Titel:** Transalp Bordcomputer

**Untertitel:** Open-Source Bordcomputer fuer die Honda Transalp 600 (PD06)

**Bullet-Points:**
- ESP32-S3 · GPS · Dual-Display · BLE · Open-Source
- 260.000+ Woerter Dokumentation · 15.500+ Zeilen Firmware-Code
- 49 Tasks abgeschlossen · 7 Projektphasen done

**Screenshot-Platzhalter:**
- `[SCREENSHOT: mockups/screens/boot.html — Boot-Screen mit Projekt-Titel]`

**Sprechnotiz:** Ein Motorrad aus den Achtzigern, aufgeruestet mit moderner
Elektronik — komplett Open-Source und dokumentiert.

---

### Slide 2: Problem & Loesung

**Titel:** Das Problem

**Bullet-Points:**
- Die Honda Transalp 600 (PD06, 1986–2000) ist rein mechanisch
- Keine ECU, kein CAN-Bus, keine OBD-II-Schnittstelle
- Keine Moeglichkeit, moderne Telemetrie, Navigation oder Diagnose zu nutzen
- Kommerzielle Bordcomputer sind teuer, geschlossen und nicht anpassbar

**Titel (rechte Haelfte):** Die Loesung

**Bullet-Points:**
- Eigenstaendiger Bordcomputer mit ESP32-S3
- Sensoren werden direkt am Fahrzeug abgegriffen (Optokoppler, Hall, NTC)
- Dual-Display: TFT fuer Echtzeit, E-Paper fuer Sonnenlicht
- BLE-Verbindung zum Smartphone fuer Navigation, Diagnose und Updates
- Vollstaendig dokumentiert, Open-Source, nachbaubar

**Screenshot-Platzhalter:**
- `[SCREENSHOT: mockups/screens/instrument.html — Standard-Fahr-Layout]`

**Sprechnotiz:** Statt ein geschlossenes System zu kaufen, bauen wir eines
von Grund auf — mit vollstaendiger Dokumentation.

---

### Slide 3: Hardware-Architektur (1/2)

**Titel:** Hardware-Architektur — MCU und Peripherie

**Bullet-Points:**
- MCU: ESP32-S3-WROOM-1, Dual-Core 240 MHz, 16 MB Flash, 8 MB PSRAM
- Framework: ESP-IDF v5.4 mit FreeRTOS SMP
- GPS: 4-Konstellation GNSS (GPS + GLONASS + Galileo + BeiDou), 10 Hz, 2,0 m CEP
- IMU: 6-Achsen (Accel + Gyro), I2C, 50 Hz Complementary-Filter
- Barometer: BMP280, Hoehe und Luftdruck
- Speicher: MicroSD Class 10, FAT32, ueber SPI-Bus
- BLE 5.0 mit NimBLE-Stack, 8 GATT-Services, 33 Characteristics

**Screenshot-Platzhalter:**
- `[SCREENSHOT: Architektur-Blockdiagramm aus docs/ARCHITECTURE.md]`

**Sprechnotiz:** Der ESP32-S3 ist das Herzstueck — ausreichend Rechenleistung
fuer Rendering und Sensordaten, integriertes BLE und reichlich Speicher.

---

### Slide 4: Hardware-Architektur (2/2)

**Titel:** Hardware-Architektur — Sensoren und Power

**Bullet-Points:**
- Drehzahl: Optokoppler an Zuendspule, Pulse-Counter (PCNT0)
- Geschwindigkeit: Hall-Sensor am Vorderrad, Pulse-Counter (PCNT1)
- Kuehlmitteltemperatur: NTC 10k, Steinhart-Hart-Gleichung, ADC
- Kraftstoffstand: Spannungsteiler am Tankgeber, 6-Punkt-Kennlinie
- Oeldruck: Analog 0–5 bar, M10x1, wasserdicht
- Bordspannung: Spannungsteiler, Generator-Status-Erkennung
- Power: LiPo 18650 (3400 mAh), TP4056-Lader, Deep-Sleep 0,4 mA
- Steuerung: 4-Kanal-Relais (Zusatzlicht, Heizgriffe, USB, Zubehoer)

**Screenshot-Platzhalter:**
- `[SCREENSHOT: Pinmap aus docs/PINMAP.md oder Schaltplan aus docs/HARDWARE.md]`

**Sprechnotiz:** Alle Sensorsignale werden galvanisch getrennt erfasst —
der Bordcomputer misst, ohne in die Fahrzeugelektronik einzugreifen.

---

### Slide 5: Firmware & Software-Stack (1/2)

**Titel:** Firmware-Architektur — Task-Modell

**Bullet-Points:**
- 7 FreeRTOS-Tasks, auf 2 Cores aufgeteilt
- Core 1 (Echtzeit): sensor_task (50 Hz), gps_task (10 Hz)
- Core 0 (Best-Effort): display_task (30 FPS), anim_task (60 Hz),
  ble_task (Event), logger_task (5 Hz), power_task (1 Hz)
- Strikte Trennung: Datenakquisition auf Core 1, Praesentation auf Core 0
- Inter-Task-Kommunikation ueber FreeRTOS-Queues und Event-Bus
- power_task hat hoechste Prioritaet (6) fuer sofortigen Shutdown

**Tabelle:**

| Task | Core | Prio | Rate | Beschreibung |
|---|---|---|---|---|
| sensor_task | 1 | 5 | 50 Hz | IMU, BMP280, ADC, PCNT |
| gps_task | 1 | 4 | 10 Hz | NMEA-Parser, UART |
| display_task | 0 | 2 | 30 FPS | Rendering, Dirty-Rect |
| ble_task | 0 | 3 | Event | GATT-Server |
| logger_task | 0 | 3 | 5 Hz | SD-Logging |
| power_task | 0 | 6 | 1 Hz | Akku, Sleep |

**Screenshot-Platzhalter:**
- `[SCREENSHOT: Architektur-Diagramm aus docs/ARCHITECTURE.md — Task-Modell]`

**Sprechnotiz:** Die Architektur trennt harte Echtzeit (Sensoren, GPS) von
weicher Echtzeit (Display, BLE) — auf zwei Kernen, ohne sich zu blockieren.

---

### Slide 6: Firmware & Software-Stack (2/2)

**Titel:** Rendering-Pipeline und Widget-System

**Bullet-Points:**
- Rendering-Pipeline: Widget-Tree -> Layout -> Paint -> Compositor -> Dirty-Rect -> SPI/DMA
- Dirty-Rect-Tracking: max. 16 Rechtecke, ueberlappende werden vereinigt
- 18 Widget-Typen: Speed, RPM-Bar, Gear, Temp, Fuel, Nav-Arrow, Altitude, Warning, BLE-Status, Wetter, Tour-Stats u.a.
- TFT: RGB565-Framebuffer im PSRAM, Double-Buffering, DMA-Bounce-Buffer
- E-Paper: 1-Bit-Framebuffer, Partial-Refresh 0,3 s, Full-Refresh 3 s
- Animations-Engine: Tween/Spring-Physics, 60 Hz Update
- Ghosting-Schutz: automatischer Full-Refresh nach 50 Partial-Refreshs

**Screenshot-Platzhalter:**
- `[SCREENSHOT: Widget-Katalog aus docs/WIDGET-CATALOG.md — Screen-Layouts]`

**Sprechnotiz:** Die Rendering-Pipeline ist optimiert: Nur geaenderte
Bildbereiche werden uebertragen — das spart CPU-Zeit und SPI-Bandbreite.

---

### Slide 7: UI/UX & Screens (1/2)

**Titel:** Display-Screens — Instrument und Navigation

**Bullet-Points:**
- Standard-Fahr-Layout: Speed zentral, RPM-Balken, Gang, Temp, Tank, GPS, Uhr
- Navigations-Modus: grosser Pfeil, Distanz, Strassenname, kurskorrigiert
- 4 Display-Modi: Tour, Track, Nav, Config — manuell oder automatisch
- Automatischer Wechsel: bei aktivem NAV-Service, bei Tour-Ende, bei Idle
- Uebergangs-Animation: Slide-Transition 250 ms (TFT), Full-Refresh (E-Paper)
- Status-Leiste: BLE-Status, GPS-Sats, Bordspannung, Generator, Trip-Zaehler

**Screenshot-Platzhalter:**
- `[SCREENSHOT: mockups/screens/instrument.html — Instrument-Screen]`
- `[SCREENSHOT: mockups/screens/navigation.html — Navigations-Screen]`

**Sprechnotiz:** Die UI ist auf Ablesbarkeit bei Sonne und Vibration
optimiert — grosse Zahlen, klare Farben, minimaler Schnickschnack.

---

### Slide 8: UI/UX & Screens (2/2)

**Titel:** Diagnose, Warning und Tour-Statistik

**Bullet-Points:**
- Diagnose-Screen: Sensor-Status, DTC-Liste, Self-Test-Trigger
- 16 DTC-Codes in 4 Kategorien (Powertrain, Sensors, Comms, Body)
- Warning-Overlay: rotes Banner bei kritischen Zustaenden (Uebertemp, Oeldruck)
- Tour-Statistik: Distanz, Dauer, Durchschnitt, Max-Speed, Hoehenmeter
- Count-Up-Animation bei Tour-Ende (800 ms, ease-out)
- Boot-Animation: 2-Sekunden-Sequenz mit Wachstum und Text-Einblendung

**Screenshot-Platzhalter:**
- `[SCREENSHOT: mockups/screens/diagnose.html — Diagnose-Screen]`
- `[SCREENSHOT: mockups/screens/warning.html — Warning-Overlay]`
- `[SCREENSHOT: mockups/screens/tour-statistik.html — Tour-Statistik]`

**Sprechnotiz:** Der Diagnose-Screen ersetzt einen separaten OBD2-Adapter —
und liefert zusaetzlich Echtzeit-Sensordaten, die kommerzielle Geraete
nicht bieten.

---

### Slide 9: Companion-App (1/2)

**Titel:** Companion-App — Dashboard und Navigation

**Bullet-Points:**
- React-Native-App, eine Codebasis fuer beide mobilen Betriebssysteme
- BLE-GATT-Client, verbindet automatisch mit dem Bordcomputer
- 5 Haupt-Screens: Dashboard, Navigation, Diagnose, Settings, Tours
- Dashboard: Live-Telemetrie mit 4 Hz (Geschwindigkeit, RPM, Temp, Tank, Volt)
- Navigation: Turn-by-Turn-Manoeuver-Daten an das Bordcomputer-Display
- Auto-Reconnect mit exponentiellem Backoff (1s, 2s, 4s, ... max 30s)
- DEGRADING-Modus bei schlechtem Empfang (RSSI unter -90 dBm)

**Screenshot-Platzhalter:**
- `[SCREENSHOT: companion-app/src/screens/DashboardScreen.tsx — Dashboard]`
- `[SCREENSHOT: companion-app/src/screens/NavigationScreen.tsx — Navigation]`

**Sprechnotiz:** Die App ist der Begleiter am Handgelenk — Dashboard,
Navigation und Diagnose in einer Anwendung.

---

### Slide 10: Companion-App (2/2)

**Titel:** Companion-App — Diagnose, Settings und OTA

**Bullet-Points:**
- Diagnose-Screen: DTC-Liste, Sensor-Status, Self-Test, Log-Export
- Settings: Radumfang, Pulse-pro-Umdrehung, Kraftstoff-Kennlinie, Display-Helligkeit
- Tours-Screen: vergangene Touren, Statistik, GPX-Export
- OTA-Update: Firmware ueber BLE (4-KB-Chunks, CRC16), HTTP (TLS) oder SD-Karte
- Dual-Partition-Layout mit automatischem Rollback bei Self-Test-Fehlschlag
- SHA-256-Verifikation des geflashten Images
- Secure-Boot-V2-Signatur (Platzhalter in v1.0, fuer Produktion vorgesehen)

**Screenshot-Platzhalter:**
- `[SCREENSHOT: companion-app/src/screens/DiagnoseScreen.tsx — Diagnose]`
- `[SCREENSHOT: companion-app/src/screens/SettingsScreen.tsx — Settings]`
- `[SCREENSHOT: companion-app/src/screens/ToursScreen.tsx — Tours]`

**Sprechnotiz:** Firmware-Updates gehen drahtlos — mit Rollback-Schutz,
falls etwas schiefgeht.

---

### Slide 11: Test & Qualitaetssicherung

**Titel:** Test-Strategie und CI/CD

**Bullet-Points:**
- 3 Unit-Tests: test_dtc, test_event_loop, test_nmea_parser (mit ESP-IDF-Mocks)
- Hardware-Test-Bench: Signal-Simulatoren fuer RPM, Geschwindigkeit, Sensoren
- Feldtest-Protokoll: 8 Test-Phasen, 25 Metriken mit Pass/Fail-Schwellen
- 4 GitHub-Workflows: firmware-ci, companion-app-ci, docs-ci, pages
- Markennamen-Blockliste: automatisierter Check gegen Brand-Namen
- Performance-Profiling: CPU, Speicher, Rendering, BLE, Power
- Sicherheitsanalyse: STRIDE-Modell mit 19 Threats und MoSCoW-Mitigation

**Screenshot-Platzhalter:**
- `[SCREENSHOT: .github/workflows/ — CI-Workflow-Files]`
- `[SCREENSHOT: Feldtest-Phasen aus docs/FIELDTEST-PROTOCOL.md]`

**Sprechnotiz:** Jeder Commit wird automatisch geprueft — Firmware-Build,
TypeScript-Compile, Markennamen-Check und Unit-Tests.

---

### Slide 12: Kosten & ROI

**Titel:** Kosten und Return on Investment

**Bullet-Points:**
- Minimal-Konfiguration: ~80 EUR (DevKit, Budget-Display, Perfboard)
- Standard-Konfiguration: ~327 EUR (Custom-PCB, TFT + E-Paper, IP67-Gehaeuse)
- Premium-Konfiguration: ~417 EUR (TPMS, 2. E-Paper, Kugelgelenk)
- ROI: 38–66% ueber 5 Jahre (gegenueber kommerziellen Loesungen)
- Teuerster Einzelbauteil: GPS-Modul (25 EUR) und Oeldrucksensor (25 EUR)
- Verschleissteil: MicroSD-Karte, Lebensdauer 2–3 Jahre bei 5.000 km/Saison

**Tabelle:**

| Konfiguration | Kosten | Features |
|---|---|---|
| Minimal | ~80 EUR | Kernsystem, Schreibtisch-Test |
| Standard | ~327 EUR | Vollsystem, IP67, alle Must/Should-Features |
| Premium | ~417 EUR | Vollsystem + TPMS + 2. E-Paper |

**Screenshot-Platzhalter:**
- `[SCREENSHOT: Kosten-Aufschluesselung aus docs/TCO-ANALYSIS.md]`

**Sprechnotiz:** Fuer rund 330 Euro hat man ein vollwertiges, wasserdichtes
System — deutlich weniger als kommerzielle Bordcomputer mit aehnlichem
Funktionsumfang.

---

### Slide 13: Roadmap & Community

**Titel:** Roadmap und Community

**Bullet-Points:**
- Phase 0–6: ABGESCHLOSSEN (49 Tasks, 260.000+ Woerter)
- v1.0: Kern-Features stabil, Companion-App als Prototyp
- v1.1 (geplant): Admin-PIN, Secure Boot V2, Flash-Encryption, Rate-Limiting
- v2.0 (geplant): App-Store-Release, 2. ESP32 als Display-Controller, WLAN
- Community-Kanaele: GitHub Issues, Pull Requests, Discussions
- Contributing-Leitfaden: CONTRIBUTING.md mit Issue-Templates
- Lizenz: PolyForm Noncommercial 1.0.0 (privat und Bildung, kein kommerzieller Vertrieb)

**Screenshot-Platzhalter:**
- `[SCREENSHOT: Roadmap-Tabelle aus docs/ROADMAP.md]`
- `[SCREENSHOT: CONTRIBUTING.md — Beitragende-Leitfaden]`

**Sprechnotiz:** Das Projekt ist abgeschlossen, aber nicht fertig. v1.1
schliesst die Sicherheitsluecken, v2.0 bringt die App in die Stores.

---

### Slide 14: Call-to-Action

**Titel:** Mach mit

**Bullet-Points:**
- Repository: github.com/TitoPrausee/transalp-bordcomputer
- Fork → Feature-Branch → Pull Request
- Issue-Templates: Bug-Report und Feature-Request
- Keine Markennamen in Code oder Doku (CI prueft automatisch)
- 35 UI-Mockups zum Ausprobieren (HTML/SVG im Browser)
- React-Native-App mit Mock-BLE-Provider (ohne Hardware entwickelbar)
- Firmware mit ESP-IDF v5.4 buildbar (`idf.py build`)

**Screenshot-Platzhalter:**
- `[SCREENSHOT: mockups/screens/boot.html — Projekt-Titel als Abschlussbild]`

**Sprechnotiz:** Egal, ob du Hardware bauen, Firmware erweitern oder die
App verbessern willst — das Repository ist offen. Fork es, baue es, fahre
damit.

---

## 3. Social-Media-Kurzform

### Projektvorstellung (3–5 Saetze)

Ein Open-Source Bordcomputer fuer die Honda Transalp 600 (PD06) — ein rein
mechanisches Motorrad aus den Achtzigern, aufgeruestet mit ESP32-S3, GPS,
Dual-Display und Smartphone-Anbindung. 15.000 Zeilen Firmware, 6.000 Zeilen
Companion-App, 260.000 Woerter Dokumentation — alles Open-Source und
nachbaubar. Sensoren greifen Fahrzeugdaten direkt ab: Drehzahl, Geschwindigkeit,
Temperatur, Tankstand, Oeldruck. BLE-Verbindung zur App fuer Navigation,
Diagnose und Firmware-Updates. Vom Schreibtisch-Prototyp zum
motorradtauglichen System — dokumentiert in 7 Projektphasen mit 49 Tasks.

### Hashtag-Vorschlaege

```
#transalp #bordcomputer #esp32 #opsembedded #espidf #freertos
#motorrad #motorcycle #diy #maker #opensource #embedded
#ble #gps #telemetry #pd06 #honda #reactnative #iot
#firmware #dualdisplay #sensors #motocycling #hackathon
#3ddruck #elektrotechnik #electronics
```

### Miniatur-Bild-Beschreibung

**Vordergrund:** Das TFT-Display des Bordcomputers im Instrument-Modus —
grosse Geschwindigkeitszahl in der Mitte, farbiger Drehzahl-Balken oben,
Gang-Anzeige rechts. Das Display leuchtet in einem dunklen, technisch
wirkenden Rahmen.

**Hintergrund:** Unscharfe Honda Transalp 600 (PD06) auf einer Bergstrasse
bei Sonnenuntergang. Warme, goldene Beleuchtung, die den Kontrast zum
kuehlen, blau-gruenen Display-Glow schafft.

**Text-Overlay (oben links):** „OPEN-SOURCE BORDCOMPUTER" in fetter, weisser
Schrift mit subtiler Schattenwirkung.

**Text-Overlay (unten rechts):** „ESP32-S3 · GPS · BLE · DUAL-DISPLAY" in
kleinerer, halbtransparenter Schrift.

**Stil:** Technisch, clean, mit leichtem Retro-Touch, der das Achtziger-Jahre-
Motorrad mit moderner Elektronik verbindet. Farbschema: dunkler Hintergrund
(dunkelblau/schwarz), Display-Glow in Cyan und Gruen, Warmton im Hintergrund.

---

## Anhang: Produktions-Hinweise

### A. Mockup-Verwendung

Die HTML/SVG-Mockups im Verzeichnis `mockups/` koennen im Browser geoeffnet
und als Screenshot fuer die Slides und das Video verwendet werden. Sie sind
statisch und zeigen die jeweilige Screen-Layouts in ihrer vollen Aufloesung.

| Mockup | Verwendet in |
|---|---|
| `mockups/screens/boot.html` | Szene 1, 3, 6; Slide 1, 14 |
| `mockups/screens/instrument.html` | Szene 2, 3, 5; Slide 2, 7 |
| `mockups/screens/navigation.html` | Szene 3, 4; Slide 7, 9 |
| `mockups/screens/diagnose.html` | Szene 3, 4; Slide 8, 10 |
| `mockups/screens/warning.html` | Szene 3; Slide 8 |
| `mockups/screens/tour-statistik.html` | Szene 3, 4; Slide 8 |
| `mockups/screens/settings.html` | Szene 4; Slide 10 |
| `mockups/screens/split.html` | Reserve fuer Split-Screen-Demos |

### B. Companion-App-Screens

Die React-Native-Companion-App kann mit dem Mock-BLE-Provider ohne
physische Hardware ausgefuehrt werden:

```
cd companion-app
npm install
npx expo start
```

Der MockBleProvider erzeugt Testdaten und ermoeglicht Screen-Recordings
fuer das Demo-Video ohne Hardware-Abhaengigkeit.

| Screen | Datei | Verwendet in |
|---|---|---|
| Dashboard | `companion-app/src/screens/DashboardScreen.tsx` | Szene 4; Slide 9 |
| Navigation | `companion-app/src/screens/NavigationScreen.tsx` | Szene 4; Slide 9 |
| Diagnose | `companion-app/src/screens/DiagnoseScreen.tsx` | Szene 4; Slide 10 |
| Settings | `companion-app/src/screens/SettingsScreen.tsx` | Szene 4; Slide 10 |
| Tours | `companion-app/src/screens/ToursScreen.tsx` | Szene 4; Slide 10 |

### C. Icon-Verwendung

Die 24 SVG-Icons unter `mockups/assets/icons/` koennen fuer Slides,
Overlays und Social-Media-Grafiken verwendet werden:

| Icon | Verwendung |
|---|---|
| `icon-speed.svg` | Geschwindigkeits-Widget |
| `icon-rpm.svg` | Drehzahl-Widget |
| `icon-gear.svg` | Gang-Anzeige |
| `icon-temp.svg` | Temperatur |
| `icon-fuel.svg` | Tankstand |
| `icon-battery.svg` | Bordspannung |
| `icon-gps.svg` | GPS-Status |
| `icon-bluetooth.svg` | BLE-Verbindung |
| `icon-nav-arrow.svg` | Navigations-Pfeil |
| `icon-warning.svg` | Warnung |
| `icon-settings.svg` | Einstellungen |
| `icon-stats.svg` | Tour-Statistik |

### D. Musik- und Tonempfehlung

- Hintergrundmusik: instrumentell, elektronisch, moderates Tempo (90–110 BPM)
- Voice-Over: klar, technisch, nicht uebertrieben enthusiastisch
- Soundeffekte: subtil — Klick beim Screen-Wechsel, leiser Boot-Sound
- Gesamtlautstaerke: Voice-Over -16 LUFS, Musik -23 LUFS (RTW-Standard)

### E. Drehplan-Zusammenfassung

| Tag | Aktivitaet | Material |
|---|---|---|
| 1 | Hardware-Aufbau, Schreibtisch-Aufnahmen | Szene 2, 3 |
| 2 | Companion-App Screen-Recording | Szene 4 |
| 3 | Motorrad-Aufnahmen (Garage, Ausfahrt) | Szene 1, 5, 6 |
| 4 | Voice-Over-Aufnahme, Schnitt | Alle Szenen |

---

*Dieses Dokument ist Teil des Transalp-Bordcomputer-Projekts.
Bei Aenderungen an Mockups, Screens oder der Companion-App muss diese
Datei aktualisiert werden, damit alle Referenzen korrekt bleiben.*

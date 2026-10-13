# Troubleshooting-Guide & FAQ

> **Transalp Bordcomputer — Diagnose und Problemlösung**
> Stand: Oktober 2026
> Plattform: ESP32-S3-WROOM-1, ESP-IDF v5.4, FreeRTOS SMP
> Fahrzeug: Honda XL600V Transalp (PD06), Baujahr 1986–1999
> Zielgruppe: Anwender und Entwickler

---

## Inhaltsverzeichnis

1. [Quick-Start / Erste Diagnose](#1-quick-start--erste-diagnose)
2. [Hardware-Probleme](#2-hardware-probleme)
3. [Software-Probleme](#3-software-probleme)
4. [Diagnose-Tools](#4-diagnose-tools)
5. [FAQ](#5-faq)
6. [Recovery-Proceduren](#6-recovery-proceduren)

---

## 1. Quick-Start / Erste Diagnose

### 1.1 Bordcomputer startet nicht — Erste 5 Checks

Wenn der Bordcomputer beim Einschalten der Zündung nicht hochfährt,
arbeite diese 5 Punkte in dieser Reihenfolge ab. Die meisten
Startprobleme lassen sich innerhalb von 10 Minuten eingrenzen.

**Check 1: Spannung an der 12V-Eingangsklemme messen**

Mit einem Multimeter die Spannung am 12V-Eingang des Bordcomputers
messen (gegen Masse). Die Zündung muss eingeschaltet sein, der Motor
kann aus sein. Erwartet: 11,5–15 V. Bei weniger als 11 V startet der
ESP32-S3 nicht zuverlässig. Bei 0 V liegt das Problem an der
Spannungsversorgung (Sicherung, Kabelbruch, Relais).

**Check 2: Sicherung prüfen**

Die 5A-Sicherung zwischen Abzweigpunkt und Bordcomputer aus dem
Halter entfernen und mit dem Multimeter auf Durchgang prüfen
(Durchgangsprüfer-Funktion, Piep). Eine durchgebrannte Sicherung
deutet auf einen Kurzschluss hin. Nicht einfach eine neue Sicherung
einsetzen — zuerst die Kabel auf Kurzschlüsse (Plus nach Masse)
prüfen. Kabelverbindungen im Gehäuse auf lose Drähte oder
isolierte Kontaktstellen kontrollieren.

**Check 3: Status-LED beobachten**

Nach dem Einschalten der Zündung sollte innerhalb von 2–3 Sekunden
die Power-LED (grün) leuchten. Wenn die LED dunkel bleibt: keine
Spannung am PCB (siehe Check 1–2) oder DC/DC-Wandler defekt.
Wenn die LED rot leuchtet: Self-Test fehlgeschlagen (siehe
Abschnitt 1.4 und DTC-Tabelle in Abschnitt 4.2).
Wenn die LED grün blinkt: Boot-Vorgang hängt fest (siehe
Abschnitt 1.3, Serial Console).

**Check 4: USB-C anschließen und Serial Console prüfen**

Ein USB-C-Kabel vom Bordcomputer zum Computer anschließen. Ein
Terminal-Programm öffnen (115200 Baud, 8N1, kein Handshake).
Nach dem Einschalten sollten Boot-Meldungen erscheinen. Wenn
keine Meldungen erscheinen: USB-Treiber am Computer prüfen, anderes
Kabel versuchen (nicht alle USB-C-Kabel übertragen Daten). Wenn
Meldungen erscheinen und dann stoppen: Boot hängt in einem Modul
(siehe Backtrace-Analyse in Abschnitt 4.1).

**Check 5: SD-Karte entfernen und neu starten**

Eine defekte oder korrupte SD-Karte kann den Boot blockieren, wenn
das FAT-Dateisystem beschädigt ist. SD-Karte entnehmen und den
Bordcomputer neu starten. Wenn er jetzt bootet: SD-Karte am
Computer neu formatieren (FAT32) oder ersetzen (siehe Abschnitt
2.4). Der Bordcomputer kann auch ohne SD-Karte arbeiten, allerdings
ohne Tour-Aufzeichnung und Logging.

### 1.2 ASCII-Entscheidungsbaum: Bordcomputer startet nicht

```
                    Bordcomputer startet nicht
                              |
                    Zündung AN, Motor aus
                              |
                 +------------+------------+
                 |                         |
           Power-LED leuchtet?         Power-LED dunkel?
                 |                         |
            +----+----+              12V messen am
            |         |              PCB-Eingang
         grün       rot                   |
            |         |              +----+----+
     Self-Test       DTC auf        <11V     0V
     fehlgeschlagen  Display?        |        |
            |         |         Batterie   Sicherung
     DTC-Liste        |         prüfen    prüfen
     via Serial       |         ( leer /  ( defekt /
     Console          |         schwach ) Kurzschluss )
     abfragen         |
     (Abschnitt 4.1)  |
                 +----+----+
                 |         |
            grün         grün
            blinkt       leuchtet
            (hängt)     dauerhaft
                 |         |
            USB-C        Boot OK
            verbinden    aber Funktion
            Backtrace    fehlerhaft?
            lesen        -> DTC-Liste
            (Abschn. 4.1) abfragen
```

### 1.3 LED-Status-Codes

Der Bordcomputer verfügt über Status-LEDs, die den Systemzustand
anzeigen. Die folgende Tabelle zeigt alle LED-Zustände und ihre
Bedeutung.

| LED | Farbe | Zustand | Bedeutung |
|-----|-------|---------|-----------|
| Power | Grün | Dauerlicht | Bordcomputer mit Strom versorgt, läuft normal |
| Power | Grün | Blinkt (1 Hz) | Boot-Vorgang läuft (Self-Test aktiv) |
| Power | Grün | Blinkt schnell (5 Hz) | Boot hängt fest — Serial Console prüfen |
| Power | Aus | — | Keine Spannung, Sicherung oder DC/DC-Wandler defekt |
| GPS | Blau | Blinkt (1 Hz) | Satellitensuche (kein Fix) |
| GPS | Blau | Dauerlicht | GPS-Fix hergestellt (mindestens 4 Satelliten) |
| GPS | Blau | Aus | GPS-Modul nicht initialisiert (DTC S0501) |
| BLE | Weiß | Blinkt (2 Hz) | Pairing-Modus / Advertising aktiv |
| BLE | Weiß | Dauerlicht | Aktive Verbindung zu Smartphone |
| BLE | Weiß | Aus | Keine Verbindung, kein Advertising |
| Warnung | Rot | Dauerlicht | Aktiver DTC mit Severity ERROR oder CRITICAL |
| Warnung | Rot | Blinkt (2 Hz) | Kritische Warnung (Überhitzung, Öldruck) |
| Shift-Light | Gelb | Pulsierend | Shift-Light-Drehzahl erreicht (Standard 7.000 U/min) |
| Shift-Light | Rot | Pulsierend | Rote Zone erreicht (Standard 8.500 U/min) |

**Akustische Signale (Piezo-Sounder):**

| Signal | Bedeutung |
|--------|-----------|
| Kurzer Piep (einmal) | Bestätigung einer Tasteneingabe |
| Doppelter Piep | Shift-Light erreicht |
| Langer, durchgehender Ton | Kritische Warnung (Öldruck, Überhitzung, niedriger Kraftstoffstand) |
| Dreifacher Piep beim Booten | Self-Test abgeschlossen, alle Systeme OK |
| Aufsteigende Tonfolge | Firmware-Update erfolgreich abgeschlossen |
| Absteigende Tonfolge | Firmware-Update fehlgeschlagen oder Self-Test-Fehler |

### 1.4 Serial Console: Wie verbinden, was erwarten

Die Serial Console ist das wichtigste Diagnose-Werkzeug. Sie läuft
auf UART0 (USB-C-Anschluss) mit 115200 Baud (8N1, kein Handshake).

**Verbindung herstellen:**

1. USB-C-Kabel vom Bordcomputer zum Computer anschließen.
2. Ein Terminal-Programm öffnen, z.B. `screen /dev/cu.usbserial-XXXX 115200`
   unter macOS/Linux oder ein entsprechendes Terminal-Programm.
3. Zündung einschalten (Motor aus). Der Bordcomputer bootet und
   gibt Boot-Meldungen aus.
4. Nach dem Boot erscheint der Prompt `>`. Mit `help` werden alle
   verfügbaren Befehle angezeigt.

**Was bei einem normalen Boot zu erwarten ist:**

```
I (234) boot: ESP-IDF v5.4 ...
I (456) main: Bordcomputer starting...
I (678) dtc: DTC system initialized (0 persisted entries)
I (712) ble_svc: BLE stack initialized
I (834) selftest: === Full Self-Test ===
I (856) selftest: Power: OK (12.40V)
I (890) selftest: Display: OK (CS=GPIO10)
I (923) selftest: GPS: data received but no fix (sats=3)
I (956) selftest: BLE: stack callable (connected=no)
I (989) selftest: Storage: SD card mounted
I (1023) selftest: Sensors: OK (temp=22.3C, rpm=0)
I (1056) selftest: IMU: OK (WHO_AM_I=0x68)
I (1089) selftest: OTA: partition valid
I (1090) selftest: Self-Test PASSED (334 ms)

=== Bordcomputer Diagnostic Console ===
Type 'help' for available commands.
>
```

**Was bei einem fehlerhaften Boot zu erwarten ist:**

Wenn der Self-Test fehlschlägt, wird der entsprechende DTC gesetzt
und auf der Konsole angezeigt. Die absteigende Tonfolge ertönt.
Der Bordcomputer funktioniert mit eingeschränkter Funktionalität
weiter — fehlende Sensoren werden als "---" angezeigt.

### 1.5 Self-Test Funktion

Der Self-Test prüft beim Booten automatisch alle Hardware-Module.
Er kann auch manuell über die Serial Console oder die Companion-App
ausgelöst werden.

**Automatischer Self-Test (beim Booten):**

Der Full Self-Test prüft 8 Module in dieser Reihenfolge:

1. **Power:** Bordspannung im Bereich 11–15 V? (DTC P0401/P0402/P0403)
2. **Display:** Display-CS-Pin konfiguriert? (DTC B0101)
3. **GPS:** NMEA-Daten über UART empfangbar? (DTC S0501/S0502)
4. **BLE:** BLE-Stack aufrufbar? (kein DTC, Best-Effort)
5. **Storage:** SD-Karte gemountet? (DTC C0101)
6. **Sensors:** Sensor-Queue hat Daten? (DTC S0402)
7. **IMU:** WHO_AM_I Register = 0x68 oder 0x70? (DTC S0401)
8. **OTA:** Boot-Partition gültig? (DTC D0301)

**Manueller Self-Test über Serial Console:**

```
> selftest full     -- Vollständiger Self-Test (alle 8 Module)
> selftest quick    -- Quick-Test (nur Power, Display, Sensors)
> selftest result   -- Letztes Self-Test-Ergebnis anzeigen
```

**Auswertung:**

Jedes Modul hat einen der drei Status: PASS, FAIL oder SKIP.
Bei FAIL wird der entsprechende DTC gesetzt. Bei SKIP wurde der
Test übersprungen (z.B. weil ein Modul noch nicht initialisiert war).
SKIP wird nicht als Fehler gewertet.

**OTA-Rollback-Verifikation:**

Wenn der Bordcomputer nach einem Firmware-Update startet und die
neue Partition im Status PENDING_VERIFY ist, entscheidet der
Self-Test über die weitere Vorgehensweise:
- Self-Test PASS: Die Partition wird als gültig markiert
  (`esp_ota_mark_app_valid_cancel_rollback`). Das Update ist
  erfolgreich abgeschlossen.
- Self-Test FAIL: Ein Rollback zur vorherigen Partition wird
  ausgelöst (`esp_ota_mark_app_invalid_rollback_and_reboot`).
  Der Bordcomputer startet mit der alten Firmware neu. DTC D0302
  wird gesetzt.

---

## 2. Hardware-Probleme

### 2.1 Display

#### 2.1.1 TFT zeigt nichts / weiß / verzerrt

**Ursache 1: SPI-Kabel lose oder falsch eingesteckt.**
Das FPC-Kabel zwischen Display und PCB kann sich durch Vibration
lösen. Gehäuse öffnen (4 × M3-Schrauben am Deckel), das FPC-Kabel
aus dem Connector ziehen und wieder einsetzen. Der Connector muss
einrasten, das Kabel darf nicht unter Spannung stehen. Auf die
Polarität achten (Pin 1-Markierung).

**Ursache 2: SPI-Pin-Konfiguration falsch.**
Die Display-Pins sind in `board_config.h` definiert. Mit der
Serial Console prüfen: `info` zeigt die Konfiguration. Wenn der
Self-Test DTC B0101 meldet: Display-CS-Pin nicht konfiguriert.
In `board_config.h` prüfen, ob `DISPLAY_CS_PIN`, `DISPLAY_DC_PIN`,
`DISPLAY_RST_PIN` und `DISPLAY_SCK_PIN` korrekte GPIO-Nummern haben.

**Ursache 3: Display-Controller defekt.**
Wenn das Kabel sitzt und die Pins korrekt sind, aber das Display
nichts zeigt: Mit der Serial Console den Befehl `selftest full`
ausführen. Wenn der Display-Test FAIL meldet, kann der
Display-Controller defekt sein. Tausch des Display-Moduls erforderlich.

**Ursache 4: TFT-Backlight defekt.**
Wenn das TFT-Display bei genauer Betrachtung (im Schatten) Inhalte
zeigt, aber das Backlight nicht leuchtet: Die Backlight-LED oder
die PWM-Steuerung ist defekt. Prüfe über die Serial Console, ob die
PWM initialisiert ist. Wenn DTC B0301 (Hintergrundbeleuchtung
PWM-Fehler) gesetzt ist: PWM-GPIO in `board_config.h` prüfen.
Vorübergehende Lösung: TFT-Display gegen Beleuchtung halten, um
Inhalte zu lesen.

#### 2.1.2 E-Paper aktualisiert nicht / Ghosting

**Ursache 1: Partial-Refresh-Overflow.**
Das E-Paper-Display (296 × 128 Pixel) verwendet Partial-Refresh für
schnelle Aktualisierungen. Nach vielen Partial-Refreshes ohne
Full-Refresh können Ghosting-Artefakte auftreten (alte Inhalte
scheinen durch). Lösung: Im Menü unter Anzeige → Display-Modus
auf "Vollständig" umschalten, was einen Full-Refresh auslöst.
Alternativ den Bordcomputer neu starten (Zündung aus, 30 Sekunden
warten, wieder an).

**Ursache 2: E-Paper-Refresh fehlgeschlagen.**
Wenn der E-Paper-Refresh komplett fehlschlägt, wird DTC B0102
gesetzt. Das Display friert ein und zeigt den letzten Inhalt
statisch an. Lösung: Neustart (Zündung aus, 30 Sekunden warten,
wieder an). Wenn das Problem nach jedem Neustart auftritt:
Display-Modul ersetzen. Das E-Paper-Modul kann durch extreme
Temperatur (-20°C) oder Feuchtigkeit im Gehäuse beschädigt werden.

**Ursache 3: Temperatur unter -10°C.**
E-Paper-Displays reagieren bei Temperaturen unter -10°C träge oder
gar nicht. Die Breittemperatur-Variante (-20°C bis +60°C) verbessert
dies, ist aber langsamer im Refresh. Bei Winterfahrten das Display
vor dem Einschalten im Gehäuse aufwärmen lassen (z.B. in der
Jackentasche transportieren, bis es warm ist, dann erst montieren).

#### 2.1.3 Backlight funktioniert nicht

Siehe Abschnitt 2.1.1, Ursache 4. DTC B0301 prüfen, wenn gesetzt.

#### 2.1.4 Display flackert bei Motorlauf (EMV)

**Ursache: Elektromagnetische Einstrahlung vom CDI-Zündsystem.**
Die CDI-Zündung der PD06 erzeugt starke Hochspannungspulse
(20–40 kV an der Zündspule), die elektromagnetische Störungen
verursachen. Wenn das Display bei laufendem Motor flackert oder
zuckt, liegt das an eingekoppelten Störungen.

**Lösungen (in absteigender Wirksamkeit):**

1. **Kupferfolie innen:** Die Innenseite des Gehäuses ist mit
   selbstklebender Kupferfolie abgeschirmt. Prüfen, ob die Folie
   vollständig aufgeklebt ist und keine Lücken hat. Wichtig: Ein
   Fenster (mindestens 30 × 30 mm) für BLE- und GPS-Antenne
   freilassen.

2. **Ferritkerne an Kabeln:** An jedem Kabel, das ins Gehäuse
   mündet, sollte ein Klappferrit nah am Gehäuse angebracht sein.
   Prüfen, ob alle Sensor-Kabel und das 12V-Kabel Ferritkerne haben.
   Fehlende Ferritkerne nachrüsten.

3. **Kabelabstand zur CDI-Box:** Die CDI-Box sitzt unter der
   Sitzbank. Sensor-Kabel dürfen nicht in der Nähe der CDI-Box
   oder der Zündspulen verlegt werden. Mindestens 10 cm Abstand
   einhalten. Kabel entlang dem rechten Rahmenrohr verlegen
   (linke Seite ist näher an der Auspuffanlage).

4. **Geschirmte Kabel:** Alle Sensor-Kabel müssen geschirmt sein
   (SFTP oder S/UTP). Der Schirm wird an einem Ende auf Masse
   gelegt (Gehäuseseite), um Masseschleifen zu vermeiden.

5. **Optokoppler für RPM-Signal:** Das Pickup-Signal der CDI darf
   nie direkt an den ESP32 angeschlossen werden. Ein Optokoppler
   (PC817 für Standard-Signale, 6N137 für hohe Frequenzen) trennt
   die Hochspannung galvanisch vom Bordcomputer-Netz.

### 2.2 Sensoren

#### 2.2.1 RPM zeigt 0 oder falsche Werte

**Ursache 1: Optokoppler-Eingang kein Signal.**
Mit der Serial Console prüfen: `sensors` zeigt den RPM-Wert.
Wenn RPM konstant 0 ist, auch bei laufendem Motor: Das
Pickup-Signal erreicht den Optokoppler nicht. Prüfe:
- Ist das Pickup-Kabel (gelb/weiß) korrekt am Abzweigpunkt
  angeschlossen?
- Ist das Kabel durch bis zum Optokoppler-Eingang am PCB?
- Ist der Steckverbinder (AMP Superseal) eingerastet?

**Ursache 2: Falsches Kabel abgegriffen (Modelle bis 1995).**
Bei Modellen bis 1995 (V-H bis V-R) hat die PD06 zwei separate
Pickup-Spulen (gelb und blau). Stelle sicher, dass das richtige
Kabel abgegriffen wurde (gelb/weiß für Zündspule/Pickup). Bei
Unsicherheit: Das Signal an der Minusseite der Zündspule (Coil-)
abgreifen.

**Ursache 3: Optokoppler defekt.**
Wenn die Anzeige springt oder 0 anzeigt, kann der Optokoppler
defekt sein. Mit der Serial Console prüfen, ob Pulse am
GPIO-Eingang ankommen. Wenn nicht: Optokoppler-Modul ersetzen
(PC817 oder 6N137).

**Ursache 4: EMV-Störung.**
Wenn die Drehzahlanzeige bei laufendem Motor unruhig ist
(zuckende Werte, Sprünge): Das Sensorkabel fängt Störungen von
der CDI-Zündung ein. Siehe Abschnitt 2.1.4, EMV-Lösungen.
Zusätzlich prüfen, ob ein RC-Filter (1 kOhm + 100 nF) am
GPIO-Eingang installiert ist.

**Ursache 5: Kalibrierung falsch.**
Die Drehzahl wird berechnet als RPM = 60 × Frequenz (bei 1 Puls
pro Umdrehung). Wenn die Anzeige konstant zu hoch oder zu niedrig
ist, kann die Kalibrierung falsch sein. In den Einstellungen
prüfen, ob "Pulse pro Umdrehung" auf 1 eingestellt ist (PD06:
1 Puls pro Kurbelwellenumdrehung).

#### 2.2.2 Temperatur zeigt unrealistische Werte

**Ursache 1: Sensor-Kabel lose (offen oder kurzgeschlossen).**
Der Kühlmitteltemperatursensor ist ein NTC-Thermistor. Bei
offenem Kabel (Unterbrechung) zeigt der ADC den Maximalwert
(entspricht einer sehr niedrigen Temperatur, z.B. -40°C). Bei
kurzgeschlossenem Kabel zeigt der ADC 0 (entspricht einer sehr
hohe Temperatur, z.B. 150°C oder mehr). Steckverbinder prüfen,
Kabel auf Unterbrechung oder Kurzschluss kontrollieren.

**Ursache 2: Wasser im Stecker.**
Den Steckverbinder auf Korrosion oder Wassereintritt prüfen.
Wenn Wasser im Stecker ist: Stecker mit Isopropanol reinigen,
trocknen lassen, mit Kontaktspray behandeln. Schrumpfschlauch
über den Stecker ziehen, um zukünftigen Wassereintritt zu
verhindern.

**Ursache 3: Sensor defekt.**
Den Widerstand des Sensors bei bekannter Temperatur mit einem
Multimeter messen. Mit der Kennlinie vergleichen:
- 50°C → 154 Ohm
- 80°C → 52 Ohm
- 120°C → 16 Ohm

Wenn der Widerstand stark abweicht: Sensor ersetzen.

**Ursache 4: Kalibrierung falsch.**
In der Companion-App unter Sensor-Kalibrierung kann ein
Offset-Wert eingestellt werden, um kleine Abweichungen zu
korrigieren. Die angezeigte Temperatur mit einem
Referenzthermometer vergleichen und den Offset anpassen.

#### 2.2.3 Tankanzeige springt / falsch

**Ursache: Kraftstoffstand-Sensor nicht kalibriert.**
Die Kennlinie des Aftermarket-Kraftstoffsensors ist nicht linear
(oben flach, unten steil). Wenn die Anzeige springt oder falsche
Werte zeigt, ist die Kalibrierung wahrscheinlich fehlerhaft oder
fehlt. Die Kalibrierung erfolgt über die Companion-App:

1. Tank volltanken.
2. App öffnen, Sensor-Kalibrierung → Kraftstoffstand → "Voll"
   kalibrieren.
3. Tank auf ca. 50% entleeren (ca. 9 Liter verbrauchen).
4. "Halbvoll" kalibrieren.
5. Tank auf Reserve (ca. 3 Liter Rest) entleeren.
6. "Leer" kalibrieren.
7. Die App interpoliert die Kennlinie aus den drei Stützstellen.

Wenn die Anzeige nach der Kalibrierung immer noch springt:
Sensor-Verbindung prüfen (lose Kabel, Korrosion im Stecker).
Der Schwimmer-Sensor kann mechanisch klemmen — den Sensor manuell
durch den Tankdeckel bewegen und prüfen, ob die Anzeige flüssig
folgt.

#### 2.2.4 Gang-Anzeige fehlt / falsch

**Ursache 1: Übersetzung nicht kalibriert.**
Die Ganganzeige wird aus dem Verhältnis Geschwindigkeit zu
Drehzahl berechnet (ratio-basiert). Die Übersetzungstabelle
unterschiedet sich zwischen frühen (1987–1995) und späten
(1996–1999) Modellen. In den Einstellungen unter
Konfiguration → Übersetzung das korrekte Modell auswählen.

**Ursache 2: Kupplung gezogen.**
Bei gezogener Kupplung (Drehzahl steigt, Geschwindigkeit bleibt
gleich) wird der Gang falsch berechnet. Das ist ein bekanntes
Verhalten und kein Fehler. Der Bordcomputer zeigt in diesem Fall
kurzzeitig den falschen Gang an, bis die Kupplung wieder
losgelassen wird.

**Ursache 3: Leerlaufschalter defekt.**
Im Leerlauf zeigt der Bordcomputer "N" an (erkannt über den
Leerlaufschalter, Kabel hellgrün). Wenn "N" nicht angezeigt wird,
obwohl der Leerlauf eingelegt ist: Den Leerlaufschalter am
Schaltmechanismus prüfen. Kabel auf Durchgang messen (der
Schalter schließt im Leerlauf gegen Masse).

#### 2.2.5 IMU reagiert nicht

**Ursache 1: IMU nicht am I2C-Bus gefunden.**
Der Self-Test liest das WHO_AM_I-Register des IMU über I2C.
Erwartet: 0x68 (oder 0x70 bei neueren Revisionen). Wenn der
Self-Test DTC S0401 meldet: IMU nicht am I2C-Bus gefunden.
Prüfen:
- I2C-Adressierung korrekt? (0x68 für MPU6050/BMI270)
- SDA- und SCL-Pins in `board_config.h` korrekt?
- Pull-Up-Widerstände (4,7 kOhm) auf SDA und SCL vorhanden?
- Kabelverbindung zum IMU-Modul sitzt?

**Ursache 2: IMU-Daten unplausibel.**
Wenn der Self-Test PASS meldet, aber die Neigungsanzeige
feststeht oder falsche Werte zeigt: DTC S0402 prüfen. Die
IMU-Daten können unplausibel sein, wenn der Sensor nicht
kalibriert ist. In der Companion-App unter Sensor-Kalibrierung
die IMU-Kalibrierung durchführen (Bordcomputer flach hinlegen,
Kalibrierung starten, dann in verschiedenen Orientierungen
halten, wie in der App angezeigt).

### 2.3 GPS

#### 2.3.1 Kein Fix / dauert zu lange

**Ursache 1: Drinnen.**
GPS funktioniert nur im Freien mit freier Sicht zum Himmel. Im
Gebäude oder in einer Garage ist kein Fix möglich. Gehe nach
draußen.

**Ursache 2: Kalter Start.**
Beim ersten Einschalten oder nach längerer Standzeit (mehrere
Tage) kann der erste GPS-Fix bis zu 2 Minuten dauern. Die
GPS-LED blinkt blau während der Suche. Geduldig abwarten.

**Ursache 3: Antenne verdeckt.**
Die GPS-Antenne muss freie Sicht nach oben haben. Sie darf nicht
durch Metall verdeckt sein (Tank, Motorblock, Verkleidung).
Wenn die Antenne im Gehäuse montiert ist: Prüfen, ob die
Kupferfolie-EMV-Abschirmung ein Fenster für die Antenne hat
(mindestens 30 × 30 mm freie Fläche). Bei aktiven Antennen:
Mindestens 10 cm Abstand zwischen GPS-Antenne und GPS-Modul.

**Ursache 4: Antennenkabel defekt.**
Das Kabel zwischen GPS-Antenne und GPS-Modul darf nicht geknickt
oder beschädigt sein. Kabel auf äußere Beschädigung prüfen, auf
Knickstellen achten. Bei Beschädigung: Antenne ersetzen.

**Ursache 5: GPS-Modul defekt.**
Wenn nach 5 Minuten im Freien kein Fix erreicht wird, kann das
GPS-Modul defekt sein. Mit der Serial Console prüfen:
```
> gps
```
Wenn keine GPS-Daten angezeigt werden oder die Satellitenanzahl
konstant 0 ist: Das Modul antwortet nicht (DTC S0501). Prüfen,
ob das Modul auf Konfigurationsbefehle antwortet. Wenn nicht:
Modul ersetzen.

#### 2.3.2 Position springt / ungenau

**Ursache 1: Schlechte Satellitenkonstellation.**
Bei weniger als 6 Satelliten oder hohem HDOP (> 2,0) kann die
Position springen. Mit der Serial Console prüfen: `gps` zeigt
die Satellitenanzahl und den HDOP. Bei weniger als 4 Satelliten
ist kein zuverlässiger Fix möglich. In einem Gebiet mit freier
Sicht (ohne hohe Gebäude, Bäume oder Berge) verbessern sich die
Werte.

**Ursache 2: Reflektionen (Multipath).**
In städtischer Umgebung oder Schluchten können reflektierte
GPS-Signale die Position verfälschen. Das ist ein physikalisches
Problem, das nicht behoben werden kann. Abhilfe: Offenes Gelände
aufsuchen.

#### 2.3.3 GPX-Log fehlt Einträge

**Ursache 1: SD-Karte voll oder Schreibfehler.**
Mit der Serial Console prüfen: `stats` zeigt SD-Karten-Status.
DTC C0103 (SD-Karte voll) oder DTC C0104 (SD-Schreibfehler)
prüfen. Wenn die Karte voll ist: GPX-Dateien exportieren und
löschen, oder eine größere Karte einsetzen. Siehe Abschnitt 2.4.

**Ursache 2: Tour-Aufzeichnung nicht gestartet.**
Die Tour-Aufzeichnung startet automatisch beim GPS-Fix, wenn
"Automatische Aufzeichnung" aktiviert ist. Prüfen, ob die
Einstellung aktiv ist (Menü → Tour → Automatische Aufzeichnung).
Alternativ kann die Aufzeichnung manuell gestartet werden
(Doppeldruck der Taste).

**Ursache 3: GPS-Fix während der Tour verloren.**
Wenn der GPS-Fix während der Fahrt verloren wird (Tunnel,
Tiefgarage), werden keine Trackpunkte geschrieben. Das GPX-Log
hat dann Lücken. Der Bordcomputer zeigt die letzte bekannte
Position. Sobald der Fix zurückkehrt, wird die Aufzeichnung
fortgesetzt. DTC S0502 (GPS-Fix verloren) und S0503 (GPS-Fix
wiederhergestellt) dokumentieren die Unterbrechung.

### 2.4 SD-Karte

#### 2.4.1 Karte wird nicht erkannt

**Ursache 1: Karte nicht eingesetzt.**
SD-Karte entnehmen und wieder einsetzen. Darauf achten, dass die
Karte richtig herum eingesetzt ist (Kontakte zum PCB) und
einrastet.

**Ursache 2: Falsch formatiert.**
Die SD-Karte muss FAT32-formatiert sein. exFAT oder NTFS werden
nicht unterstützt. Am Computer neu formatieren: FAT32,
Partitionsgröße maximal 32 GB. Größere Karten werden von ESP-IDF
möglicherweise nicht korrekt erkannt. Empfohlene Größe: 8–32 GB,
Class 10.

**Ursache 3: Karte defekt.**
Wenn die Karte nach dem Formatieren immer noch nicht erkannt wird,
kann sie defekt sein. Eine neue SD-Karte einsetzen. Der Self-Test
meldet DTC C0101, wenn die Karte nicht gemountet werden kann.

#### 2.4.2 Schreibfehler / Logs fehlen

**Ursache 1: SD-Karte verschlissen.**
MicroSD-Karten haben eine begrenzte Anzahl von Schreibzyklen. Bei
intensiver Nutzung (5 Hz Logging über Monate) kann die Karte
verschleißen. DTC C0104 (SD-Schreibfehler) wird gesetzt. Lösung:
Karte ersetzen. Eine Class-10-Karte mit hoher Schreibrate
empfehlen (z.B. Industrial-Grade für höhere Langlebigkeit).

**Ursache 2: Karte voll.**
DTC C0103 (SD-Karte voll) wird gesetzt, wenn weniger als 5%
freier Speicherplatz verfügbar sind. GPX-Dateien und Logs
exportieren und löschen, oder eine größere Karte einsetzen.

#### 2.4.3 Karte voll / Formatierung

**Speicherplatz prüfen:**
Mit der Serial Console: `stats` zeigt SD-Karten-Status. Alternativ
die Karte am Computer auslesen und den freien Speicher prüfen.

**Karte formatieren:**
1. Bordcomputer ausschalten (Zündung aus).
2. SD-Karte entnehmen.
3. Am Computer mit einem Formatierungs-Tool FAT32 formatieren
   (Blockgröße 32 KB). Alle Daten auf der Karte werden gelöscht.
4. Karte wieder in den Bordcomputer einsetzen.
5. Zündung einschalten. Der Bordcomputer mountet die Karte
   automatisch und erstellt die Verzeichnisstruktur neu.

### 2.5 Power

#### 2.5.1 Bordcomputer startet nicht bei Zündung an

Siehe Abschnitt 1.1 (Erste 5 Checks) und Abschnitt 1.2
(Entscheidungsbaum).

#### 2.5.2 Resets bei Motorstart (Spannungseinbruch)

**Ursache: Spannungseinbruch beim Anlassen.**
Beim Starten des Motors zieht der Anlasser viel Strom (100–200 A).
Die Bordspannung kann dabei auf 8–10 V einbrechen. Der DC/DC-Wandler
im Bordcomputer braucht mindestens 7 V Eingangsspannung. Bei
kürzerem Einbruch (weniger als 200 ms) puffert der LiPo-Akku
(1000–2000 mAh) die Versorgung. Bei längerem Einbruch oder
schwacher Batterie startet der Bordcomputer neu.

**Lösungen:**

1. **Batterie prüfen:** Eine schwache Motorrad-Batterie (12 Ah,
   YB12A-B) verursacht stärkere Spannungseinbrüche. Batterie
   laden oder ersetzen, wenn die Leerlaufspannung unter 12,2 V
   liegt.

2. **Kabelquerschnitt prüfen:** Die 12V-Zuleitung zum Bordcomputer
   sollte mindestens 0,75 mm² (besser 1,5 mm²) haben. Zu dünne
   Kabel erhöhen den Spannungsabfall.

3. **Pufferakku prüfen:** Der LiPo-Pufferakku im Gehäuse kann
   entladen oder defekt sein. Mit der Serial Console prüfen:
   `info` zeigt die Batteriespannung und den Ladezustand. Wenn
   der Pufferakku 0% zeigt: Akku ersetzen (Typ: 3,7V LiPo,
   1000–2000 mAh, mit Schutzschaltung).

4. **Software-Seite:** Die Firmware hat einen Watchdog-Reset
   (DTC D0501) und einen Brownout-Detector (DTC D0502). Wenn
   diese DTCs nach dem Motorstart auftauchen, bestätigt das den
   Spannungseinbruch. Im Diagnose-Menü die DTC-Historie prüfen.

#### 2.5.3 Batterie entlädt sich im Stand

**Ursache 1: Deep-Sleep nicht aktiviert.**
Der Bordcomputer sollte in den Deep-Sleep-Modus wechseln, wenn die
Zündung ausgeschaltet wird und keine aktive BLE-Verbindung besteht.
Im Deep-Sleep verbraucht der ESP32-S3 weniger als 1 mA. Wenn der
Deep-Sleep nicht funktioniert, entlädt der Bordcomputer die
Motorrad-Batterie innerhalb weniger Tage.

Mit der Serial Console prüfen: `info` zeigt den Power-State.
Wenn der Status nach dem Ausschalten der Zündung nicht "SLEEP"
oder "SHUTDOWN" anzeigt: Deep-Sleep-Funktion prüfen. Der
Power-Task (`power_task`, 1 Hz, Prio 6) entscheidet über den
Übergang in den Sleep-Modus. DTC D0501 (Watchdog-Reset) kann
auf einen Task-Crash hinweisen, der den Sleep-Modus verhindert.

**Ursache 2: Zündungsplus nicht geschaltet.**
Wenn der Bordcomputer an das direkte Batterie-Plus
(schwarz/braun) statt an das geschaltete Plus (schwarz) angeschlossen
wurde, ist er immer eingeschaltet. Kabelverbindung prüfen und
gegebenenfalls auf das geschaltete Plus umklemmen.

**Ursache 3: Pufferakku entlädt sich.**
Der LiPo-Pufferakku entlädt sich langsam, wenn der Bordcomputer
längere Zeit nicht benutzt wird. Das ist normal und schadet nicht —
er wird beim nächsten Start wieder geladen. Bei Temperaturen unter
0°C blockiert die Software die Ladung automatisch, um Lithium-Plating
zu vermeiden.

#### 2.5.4 Deep-Sleep funktioniert nicht

**Ursache 1: BLE-Verbindung aktiv.**
Der Bordcomputer wechselt nicht in den Deep-Sleep, wenn eine
aktive BLE-Verbindung besteht. Prüfen, ob die Companion-App noch
verbunden ist. App schließen oder Bluetooth am Smartphone
deaktivieren.

**Ursache 2: Task blockiert den Sleep-Modus.**
Wenn ein FreeRTOS-Task nicht auf den Shutdown-Event reagiert,
kann der Bordcomputer nicht in den Deep-Sleep wechseln. Mit der
Serial Console `stats` ausführen und prüfen, ob alle Tasks im
erwarteten Zustand sind (sensor_task: BLK, gps_task: BLK, etc.).
Wenn ein Task im Zustand RDY (Ready) verbleibt und nicht in BLK
(Blocked) wechselt, blockiert er möglicherweise den Sleep.

**Ursache 3: EVT_SHUTDOWN nicht gesendet.**
Der Power-Task sendet das `EVT_SHUTDOWN`-Event, wenn er den
Übergang zum Sleep-Modus entscheidet. Wenn der Event-Bus nicht
funktioniert, erhalten andere Tasks die Information nicht. Mit
der Serial Console prüfen, ob der Power-State "SHUTDOWN" oder
"SLEEP" anzeigt. Wenn nicht: Power-Task prüfen (Kabel zum
Spannungsteiler, ADC-Konfiguration).

---

## 3. Software-Probleme

### 3.1 BLE

#### 3.1.1 App findet Bordcomputer nicht

**Ursache 1: Bluetooth am Smartphone deaktiviert.**
Bluetooth am Smartphone aktivieren. Im Betriebssystem prüfen,
ob Bluetooth eingeschaltet ist. Die Companion-App benötigt BLE
(Bluetooth Low Energy), nicht klassisches Bluetooth.

**Ursache 2: Bordcomputer im falschen Modus.**
Der Bordcomputer muss im Advertising-Modus sein, damit die App
ihn findet. Die BLE-LED blinkt weiß (2 Hz) im Advertising-Modus.
Wenn die LED aus ist: Der BLE-Stack ist nicht initialisiert
oder im Fehlerzustand. Neustart des Bordcomputers (Zündung aus
und wieder an). Mit der Serial Console prüfen: `info` zeigt
"BLE: connected" oder "disconnected". Wenn der Self-Test DTC
C0201 meldet: BLE-Advertising konnte nicht starten.

**Ursache 3: Pairing-Liste voll oder korrupt.**
Wenn bereits viele Geräte gepairt wurden, kann die Pairing-Liste
voll oder korrupt sein. In der Companion-App die Pairing-Liste
leeren und neu koppeln. Auf dem Bordcomputer kann das über die
Serial Console oder die App (Bond-Management) gemacht werden.

**Ursache 4: BLE-Konfiguration in der Firmware.**
In `ble_service.c` ist der Security Manager konfiguriert:
LE Secure Connections (ECDH P-256) mit MITM-Schutz und Bonding.
Wenn die App-Bibliothek LE Secure Connections nicht
unterstützt, schlägt das Pairing fehl. Sicherstellen, dass die
Companion-App eine aktuelle Version der BLE-Bibliothek verwendet,
die LE Secure Connections unterstützt.

#### 3.1.2 Verbindung bricht ab

**Ursache 1: Smartphone zu weit weg.**
Die BLE-Reichweite auf dem Motorrad beträgt typischerweise
5–15 Meter. Das Smartphone sollte sich in der Brust- oder
Jackentasche befinden, nicht im Rucksack weit vom Bordcomputer
entfernt. Praktische Reichweite je nach Positionierung:
- Brust-/Jackentasche: 5–15 m, stabil
- Tankrucksack: 2–5 m, meist stabil
- Rucksack am Rücken: 1–3 m, oft instabil
- Seitenkoffer: unzuverlässig

**Ursache 2: App im Hintergrund beendet.**
Manche Betriebssysteme beenden Hintergrund-Apps aggressiv, um
Akku zu sparen. In den Einstellungen des Betriebssystems die
Hintergrundaktualisierung für die Companion-App aktivieren.
Die App muss im Hintergrund laufen, um die BLE-Verbindung
aufrechtzuerhalten.

**Ursache 3: Andere Bluetooth-Geräte stören.**
Andere Bluetooth-Geräte in der Nähe (Headset, Intercom) können
stören. Andere Bluetooth-Geräte ausschalten und testen, ob die
Verbindung stabil wird.

**Ursache 4: DEGRADING-Modus.**
Bei schlechter Verbindung (RSSI < -90 dBm oder Packet-Loss > 10%)
schaltet der Bordcomputer in den DEGRADING-Zustand: Es werden nur
noch priorisierte Daten übertragen (Navigation, Anrufe), keine
Telemetrie-Updates und keine Wetter-Daten. Die Verbindung wird
automatisch wiederhergestellt, sobald die Signalqualität besser
wird. Das ist kein Fehler, sondern ein Schutzmechanismus.

**Ursache 5: BLE-Stack-Fehler.**
Wenn die Verbindung ohne erkennbaren Grund abbricht und DTC C0202
(BLE-Verbindung getrennt) wiederholt auftritt, kann ein
BLE-Stack-Fehler vorliegen (DTC C0203). Mit der Serial Console
die DTC-Historie prüfen: `dtc history`. Wenn C0203 auftritt:
Firmware-Update durchführen. Wenn das Problem bestehen bleibt:
Serial Console mit erweitertem Logging aktivieren (Log-Level auf
VERBOSE in `sdkconfig.defaults`: `CONFIG_LOG_DEFAULT_LEVEL_VERBOSE=y`).

#### 3.1.3 Pairing schlägt fehl

**Ursache 1: Inkompatible IO-Capability.**
Der Bordcomputer verwendet `BLE_SM_IO_CAP_DISP_YES_NO` (Numeric
Comparison). Das Smartphone muss diese Pairing-Methode
unterstützen. Wenn das Smartphone eine andere Methode erwartet
(z.B. Passkey-Eingabe), schlägt das Pairing fehl. App-Version
aktualisieren.

**Ursache 2: Bonding-Keys korrupt.**
Wenn das Pairing nach einem App-Update oder Betriebssystem-Update
nicht mehr funktioniert: Pairing auf beiden Seiten löschen und
neu koppeln. In der App: "Gerät entfernen". Auf dem Bordcomputer:
Bond-Management über die App oder Serial Console.

#### 3.1.4 Diagnose-Befehle werden abgelehnt (PIN)

**Ursache: Sicherheitseinstellung für kritische Characteristics.**
Bestimmte BLE-Characteristics (CONFIG_WRITE, CONFIG_IMPORT,
BOND_MGMT, SELF_TEST, OTA_CONTROL, OTA_DATA) erfordern eine
authentifizierte Verbindung (MITM-Schutz). Wenn das Pairing ohne
MITM-Schutz erfolgte (z.B. Just-Works ohne LE Secure Connections),
werden diese Characteristics abgelehnt. Lösung: Pairing löschen
und mit LE Secure Connections neu koppeln. In der App sicherstellen,
dass die Pairing-Methode Numeric Comparison unterstützt wird.

---

### 3.2 OTA

#### 3.2.1 Update startet nicht

**Ursache 1: OTA-Partitionen nicht konfiguriert.**
Die Partition Table muss die Partitionen `ota_0` und `ota_1`
enthalten. Prüfen in `partitions.csv`:
```
ota_0,    app,  ota_0,   0x310000, 0x300000,
ota_1,    app,  ota_1,   0x610000, 0x300000,
otadata,  data, ota,     0x910000, 0x2000,
```
Zusätzlich muss in `sdkconfig.defaults` gesetzt sein:
```
CONFIG_APP_ROLLBACK_ENABLE=y
```
Ohne diese Konfiguration kann das OTA-System keine Updates
durchführen. DTC D0301 (OTA-Partition ungültig) wird gesetzt.

**Ursache 2: Firmware-Image zu groß.**
Jede OTA-Partition ist 3 MB groß. Wenn das Firmware-Binary größer
als 3 MB ist, schlägt das Update fehl. Im Build-Log prüfen, welche
Größe das Binary hat. Bei zu großem Binary: Unnötige Komponenten
deaktivieren oder OTA-Partition-Größe in `partitions.csv` erhöhen
(allerdings nur, wenn Flash-Größe es zulässt — 16 MB gesamt).

**Ursache 3: BLE-Verbindung nicht authentifiziert.**
OTA_CONTROL und OTA_DATA erfordern eine authentifizierte
BLE-Verbindung. Siehe Abschnitt 3.1.4.

#### 3.2.2 Update bricht ab

**Ursache 1: BLE-Verbindung während des Updates unterbrochen.**
Das OTA-Update über BLE überträgt die Firmware in 4 KB-Chunks mit
CRC16-CCITT-Prüfung pro Chunk. Bei Verbindungsabbruch kann der
Transfer an der letzten Position fortgesetzt werden (Resume über
NVS-gestützte Offset-Speicherung). In der App "Update fortsetzen"
auswählen. Wenn das nicht möglich ist: Update neu starten.

**Ursache 2: Hash-Verifikation fehlgeschlagen.**
Nach dem Flashen wird ein SHA-256-Hash der geflashten Partition
berechnet und mit dem erwarteten Hash verglichen. Bei
Hash-Mismatch (DTC D0301) wird das Update abgelehnt. Ursache:
Übertragungsfehler oder korruptes Firmware-Image. Firmware-Image
neu herunterladen und Update wiederholen.

**Ursache 3: Flash-Schreibfehler.**
Wenn `esp_ota_write` einen Fehler zurückgibt, wird das Update
abgebrochen. Mit der Serial Console das Log prüfen. Häufige
Ursachen: Flash-Speicher voll, Partition-Table-Fehler,
Hardware-Defekt des Flash-Speichers.

#### 3.2.3 Nach Update: Boot-Loop / Brick

**Ursache: Neue Firmware bootet nicht oder Self-Test schlägt fehl.**
Das OTA-System hat einen automatischen Rollback-Mechanismus. Wenn
die neue Firmware nach dem Update den Self-Test nicht besteht,
wird automatisch die vorherige Version wiederhergestellt:

1. Boot der neuen Firmware.
2. Self-Test schlägt fehl.
3. Bootloader setzt Boot-Partition zurück.
4. Neustart mit vorheriger Firmware.
5. Diagnose-Log zeigt DTC D0302 (OTA-Rollback aktiviert).

Wenn der Bordcomputer in einem Boot-Loop feststeckt (ständiges
Neustarten ohne stabile Anzeige): USB-C anschließen und die
Serial Console beobachten. Der Rollback sollte nach spätestens
3 Resets greifen (Boot-Loop-Erkennung: mehr als 3 Resets in 60
Sekunden → automatischer Rollback).

**Wenn der automatische Rollback nicht greift:**
Manuellen Rollback über USB-C durchführen. Siehe Abschnitt 6.1
(Firmware-Recovery nach Brick).

#### 3.2.4 Rollback funktioniert nicht

**Ursache 1: Rollback nicht aktiviert.**
In `sdkconfig.defaults` muss `CONFIG_APP_ROLLBACK_ENABLE=y`
gesetzt sein. Ohne diese Option wird die neue Firmware direkt als
gültig markiert und ein Rollback ist nicht möglich.

**Ursache 2: `ota_init()` nicht in `app_main()` aufgerufen.**
Der OTA-Manager muss in `app_main()` initialisiert werden, damit
der Rollback-Check beim Booten ausgeführt wird. In
`firmware/main/main.c` prüfen, ob `ota_init()` aufgerufen wird.

**Ursache 3: Self-Test nicht im Boot-Prozess.**
Der Rollback wird durch den Self-Test ausgelöst
(`ota_rollback_check()` in `selftest.c`). Wenn der Self-Test
nicht beim Booten ausgeführt wird, wird die neue Partition nicht
validiert und der Rollback-Check übersprungen. Prüfen, ob
`selftest_run()` in der Startup-Sequenz aufgerufen wird.

### 3.3 Firmware

#### 3.3.1 Kompilierung schlägt fehl

**Ursache 1: ESP-IDF nicht korrekt installiert.**
ESP-IDF v5.4 muss installiert und die Umgebung aktiviert sein:
```bash
. ~/esp/esp-idf/export.sh
```
Prüfen: `idf.py --version` muss eine Version ausgeben. Wenn nicht:
ESP-IDF neu installieren (siehe FIRMWARE-GUIDE.md, Abschnitt 1.1).

**Ursache 2: Fehlende Komponenten-Abhängigkeiten.**
Jede Komponente deklariert ihre Abhängigkeiten in `CMakeLists.txt`
unter `REQUIRES`. Wenn eine Abhängigkeit fehlt, gibt der Compiler
einen "undefined reference"-Fehler. Prüfen, ob alle Komponenten in
`firmware/components/` vorhanden sind und die `REQUIRES` in den
`CMakeLists.txt`-Dateien korrekt sind.

**Ursache 3: sdkconfig-Konflikt.**
Wenn `sdkconfig` manuell geändert wurde, kann es zu Konflikten
mit `sdkconfig.defaults` kommen. Lösung: `sdkconfig` löschen und
neu bauen. ESP-IDF generiert eine neue `sdkconfig` aus den
Defaults.

```bash
cd firmware
rm sdkconfig
idf.py build
```

#### 3.3.2 Flash schlägt fehl

**Ursache 1: Falscher Serial-Port.**
Den korrekten Serial-Port ermitteln:
```bash
# macOS:
ls /dev/cu.usbserial-*
# Linux:
ls /dev/ttyUSB*
```
Flash mit explizitem Port:
```bash
idf.py -p /dev/cu.usbserial-XXXX flash
```

**Ursache 2: USB-Kabel ohne Datenleitung.**
Nicht alle USB-Kabel übertragen Daten. Einige sind nur
Ladekabel. Ein Datenkabel verwenden (andernfalls wird der
ESP32-S3 nicht als Serial-Gerät erkannt).

**Ursache 3: Boot-Modus.**
Der ESP32-S3 muss im Download-Modus sein, um geflasht zu werden.
Normalerweise wird der Download-Modus automatisch durch
`esptool.py` ausgelöst. Wenn das nicht klappt: GPIO0 beim
Einschalten gedrückt halten (Boot-Taste), dann flashen.

#### 3.3.3 Boot-Loop nach Flash

Mit der Serial Console den Boot-Output beobachten. Häufige
Ursachen:

- **Partition-Table-Fehler:** Die Partition-Table in `partitions.csv`
  ist nicht mit dem Flash-Layout kompatibel. Prüfen, ob die
  Offsets und Größen korrekt sind.

- **Watchdog-Reset (DTC D0501):** Ein Task blockiert und der
  Watchdog löst einen Reset aus. Im Log nach "Task watchdog"
  suchen. Den betroffenen Task identifizieren und prüfen, ob er
  `vTaskDelay` oder Event-Waits enthält.

- **Stack-Overflow:** Ein Task-Stack ist zu klein. Im Log nach
  "Stack canary watchpoint triggered" suchen. Die Stack-Größe
  des betroffenen Tasks in `board_config.h` erhöhen. Mit
  `uxTaskGetStackHighWaterMark()` den minimalen freien Stack
  prüfen — sollte mehr als 25% der Stack-Größe betragen.

- **Heap-Corruption:** Im Log nach "CORRUPT HEAP" oder
  "Memory corruption" suchen. `CONFIG_HEAP_POISONING_COMPREHENSIVE=y`
  in `sdkconfig.defaults` aktivieren, um die Heap-Prüfung zu
  verschärfen. Den Backtrace analysieren, um die Quelle der
  Korruption zu finden.

#### 3.3.4 Task-Crash / Watchdog-Reset

**Task-Crash analysieren:**
Bei einem Crash gibt der Panic-Handler einen Backtrace aus:
```
Guru Meditation Error: Core 0 panic'ed (Stack canary watchpoint triggered)
Backtrace: 0x42012a3a:0x3fceb150 0x42012c4e:0x3fceb170 ...
```
Mit dem ESP-IDF Monitor wird der Backtrace automatisch in
Funktionsnamen aufgelöst:
```
0x42012a3a: ble_task at firmware/components/ble/ble.c:42
0x42012c4e: vTaskNode at freertos/tasks.c:543
```
Die Quelldatei und Zeile identifizieren, die den Crash verursacht
hat. Den Code an dieser Stelle auf Null-Pointer, Buffer-Overflow
oder Race-Conditions prüfen.

**Watchdog-Timeout:**
Der Task-Watchdog löst aus, wenn ein Task länger als die
konfigurierte Zeit (5 s Debug, 10 s Release) ohne
`vTaskDelay` oder Event-Wait läuft. Lösung: Den betroffenen Task
identifizieren (aus dem Log) und sicherstellen, dass er
regelmäßig `vTaskDelay()` aufruft oder auf Events wartet.

#### 3.3.5 Speicher voll (Heap / Stack)

**Heap voll:**
Mit der Serial Console prüfen:
```
> stats
```
Zeigt "Free Heap" und "Min Free Heap". Wenn der freie Heap unter
20 KB fällt, kann es zu Allokationsfehlern kommen. Häufige
Ursachen:
- Memory-Leak (nicht freigegebener Speicher)
- Zu große Puffer (z.B. GPX-Buffer, BLE-Puffer)
- Fragmentierung

Heap-Tracing aktivieren (in `sdkconfig.defaults`):
```
CONFIG_HEAP_TRACING=y
```
Mit `heap_trace_start()` und `heap_trace_dump()` nicht
freigegebene Allokationen identifizieren. Siehe FIRMWARE-GUIDE.md,
Abschnitt 5.3.

**Stack-Overflow:**
Mit `uxTaskGetStackHighWaterMark()` den minimalen freien Stack
pro Task prüfen. Wenn der High-Water-Mark unter 25% der
Stack-Größe fällt: Stack-Größe erhöhen. In `board_config.h`:
```
#define TASK_STACK_SENSOR   4096   /* bei Bedarf erhöhen */
#define TASK_STACK_DISPLAY   8192
#define TASK_STACK_BLE       6144
```

### 3.4 Companion-App

#### 3.4.1 App stürzt ab

**Ursache 1: Inkompatible App-Version.**
Sicherstellen, dass die App-Version mit der Bordcomputer-Firmware
kompatibel ist. In der App unter Info → Firmware-Version prüfen.
Bei Versionskonflikt: App aktualisieren oder Firmware-Update
durchführen.

**Ursache 2: BLE-Bibliothek nicht kompatibel.**
Die App verwendet eine BLE-Bibliothek, die mit dem
NimBLE-Stack des ESP32-S3 kommuniziert. Wenn die
Bibliothek-Version nicht mit LE Secure Connections umgehen kann,
kann die App abstürzen. App-Update durchführen.

#### 3.4.2 BLE-Verbindung instabil

Siehe Abschnitt 3.1.2 (Verbindung bricht ab).

#### 3.4.3 Daten aktualisieren nicht

**Ursache 1: CCCD-Subscription fehlt.**
Die App muss sich für Notifications anmelden (CCCD-Subscribe),
um Live-Daten zu erhalten. Wenn die Daten in der App nicht
aktualisiert werden: App neu starten und neu verbinden. Wenn das
nicht hilft: Pairing löschen und neu koppeln.

**Ursache 2: DEGRADING-Modus.**
Bei schlechter Verbindung schaltet der Bordcomputer in den
DEGRADING-Modus und sendet keine Telemetrie-Updates mehr. Siehe
Abschnitt 3.1.2, Ursache 4.

#### 3.4.4 Diagnose-Screen leer

**Ursache: Diagnose-Service nicht abonniert.**
Der Diagnose-Screen in der App liest Daten vom Diagnose-Service
(0x7000: DTC_LIST, SENSOR_STATUS, SELF_TEST). Wenn der Screen
leer ist: BLE-Verbindung prüfen, App neu starten. Wenn der
Self-Test über die App gestartet wird und nichts passiert:
Authentifizierung prüfen (siehe Abschnitt 3.1.4).

---

## 4. Diagnose-Tools

### 4.1 Serial-Console: Nützliche Befehle

Die Serial Console läuft auf UART0 (USB-C) mit 115200 Baud. Sie
bietet folgende Befehle:

| Befehl | Beschreibung |
|--------|-------------|
| `dtc list` | Aktive DTCs anzeigen |
| `dtc history` | DTC-Historie anzeigen |
| `dtc clear <code>` | Einzelnen DTC löschen (z.B. `dtc clear P0101`) |
| `dtc clear all` | Alle DTCs löschen (in Historie verschieben) |
| `selftest full` | Vollständigen Self-Test ausführen (8 Module) |
| `selftest quick` | Schnellen Self-Test ausführen (Power, Display, Sensors) |
| `selftest result` | Letztes Self-Test-Ergebnis anzeigen |
| `info` | System-Info (Version, Uptime, Heap, Batterie, BLE, SD, DTCs) |
| `sensors` | Live-Sensorwerte (Temp, Druck, Höhe, Roll, Pitch, RPM, Speed, Gang, Kühlmittel, Kraftstoff, Crash) |
| `gps` | GPS-Daten (Lat, Lon, Speed, Course, Altitude, Sats, HDOP, Fix-Quality) |
| `stats` | System-Statistiken (Heap, Tasks, Tick-Rate, DTCs, Batterie) |
| `help` | Befehlsübersicht |

**Beispiel: DTC-Liste abfragen**

```
> dtc list
=== Active DTCs (2) ===
  S0501    WARN     ACTIVE    count=1   first=4567   last=4567
         Freeze: vbat=12.4V  cool=85.0C  amb=22.3C  fuel=12.0L  rpm=0  spd=0.0  roll=0.0  sats=0
  C0101    ERROR    ACTIVE    count=1   first=4567   last=4567
         Freeze: vbat=12.4V  cool=85.0C  amb=22.3C  fuel=12.0L  rpm=0  spd=0.0  roll=0.0  sats=0
```

Der Freeze-Frame zeigt die Sensorwerte zum Zeitpunkt der
DTC-Auslösung. Das hilft bei der Ursachenanalyse.

**Beispiel: Self-Test ausführen**

```
> selftest full
Running full self-test...
=== Self-Test Result ===
Overall: FAIL  Duration: 1234 ms  Time: 4567 ms
  Power       PASS  -
  Display     PASS  -
  GPS         FAIL  S0501
  BLE         PASS  -
  Storage     FAIL  C0101
  Sensors     PASS  -
  IMU         PASS  -
  OTA         PASS  -
Result: FAIL
```

### 4.2 DTC-Code-Tabelle

Das Bordcomputer-Diagnosesystem verwendet eigene Fehlercodes
(Diagnostic Trouble Codes), die an OBD-II angelehnt sind. Jeder
Code hat das Format `X0YY`, wobei X die Kategorie und YY der
spezifische Fehler ist. Die Codes überleben einen Reboot
(NVS-Persistenz).

**Kategorien:**
- **P** = Powertrain (Motor, Zündung, Bordspannung)
- **S** = Sensoren (Temperatur, Kraftstoff, IMU, GPS)
- **C** = Communication (SD-Karte, BLE, I2C, SPI, UART)
- **B** = Body (Display, Backlight, Taster)
- **D** = Diagnose-System (NVS, Heap, OTA, Self-Test)

#### Powertrain (P0xxx)

| Code | Severity | Beschreibung |
|------|----------|-------------|
| P0101 | WARN | RPM-Signal unplausibel |
| P0102 | ERROR | RPM-Signal ausgefallen |
| P0103 | WARN | RPM-Signal noisy |
| P0201 | WARN | Zündimpuls-Signalpegel niedrig |
| P0202 | ERROR | Zündimpuls-Signal ausgefallen |
| P0301 | WARN | Gang-Erkennung unplausibel |
| P0401 | WARN | Bordspannung unter 11,5V |
| P0402 | CRITICAL | Bordspannung unter 10,5V (kritisch) |
| P0403 | WARN | Bordspannung über 15,0V |
| P0404 | INFO | Bordspannung normalisiert |

#### Sensoren (S0xxx)

| Code | Severity | Beschreibung |
|------|----------|-------------|
| S0101 | ERROR | Kühlmitteltemperatur-Sensor unplausibel |
| S0102 | WARN | Kühlmitteltemperatur über 110°C (Warnung) |
| S0103 | CRITICAL | Kühlmitteltemperatur über 120°C (kritisch) |
| S0201 | ERROR | Kraftstoffstand-Sensor unplausibel |
| S0202 | WARN | Kraftstoffstand unter 10% (Reserve) |
| S0301 | ERROR | Öldruckschalter unplausibel |
| S0302 | WARN | Öldruck niedrig bei laufendem Motor |
| S0401 | ERROR | IMU nicht am I2C-Bus gefunden |
| S0402 | WARN | IMU-Daten unplausibel |
| S0501 | WARN | GPS-Modul antwortet nicht |
| S0502 | WARN | GPS-Fix verloren |
| S0503 | INFO | GPS-Fix wiederhergestellt |
| S0601 | ERROR | Barometer nicht am I2C-Bus |
| S0602 | WARN | Barometer-Daten unplausibel |

#### Communication (C0xxx)

| Code | Severity | Beschreibung |
|------|----------|-------------|
| C0101 | ERROR | SD-Karte nicht mountbar |
| C0102 | WARN | SD-Karte fast voll |
| C0103 | ERROR | SD-Karte voll |
| C0104 | WARN | SD-Schreibfehler |
| C0201 | WARN | BLE-Advertising konnte nicht starten |
| C0202 | WARN | BLE-Verbindung getrennt |
| C0203 | ERROR | BLE-Stack-Fehler |
| C0301 | ERROR | I2C-Bus-Fehler |
| C0302 | WARN | I2C-Gerät antwortet nicht |
| C0401 | WARN | SPI-Bus-Fehler (Display) |
| C0402 | ERROR | UART-Fehler (GPS) |

#### Body (B0xxx)

| Code | Severity | Beschreibung |
|------|----------|-------------|
| B0101 | ERROR | Display-Controller nicht erreichbar |
| B0102 | WARN | Display-Refresh fehlgeschlagen |
| B0301 | WARN | Hintergrundbeleuchtung PWM-Fehler |
| B0401 | WARN | Taster 1 entprellt ungültig |
| B0402 | WARN | Taster 2 entprellt ungültig |

#### Diagnose-System (D0xxx)

| Code | Severity | Beschreibung |
|------|----------|-------------|
| D0101 | ERROR | NVS-Schreibfehler |
| D0102 | WARN | NVS fast voll |
| D0103 | ERROR | RTC-Zeit nicht gesetzt |
| D0201 | ERROR | PSRAM-Test fehlgeschlagen |
| D0202 | WARN | Heap-Fragmentierung hoch |
| D0301 | ERROR | OTA-Partition ungültig |
| D0302 | WARN | OTA-Rollback aktiviert |
| D0401 | ERROR | Self-Test fehlgeschlagen |
| D0402 | INFO | Self-Test bestanden |
| D0501 | ERROR | Watchdog-Reset erkannt |
| D0502 | WARN | Brownout-Detector ausgelöst |
| D0503 | WARN | Akku altert |
| D0601 | ERROR | Kalibrierungs-Daten ungültig |

### 4.3 Self-Test: Modus und Auswertung

Siehe Abschnitt 1.5 für die Beschreibung des Self-Tests und der
8 Module. Der Self-Test kann in drei Modi ausgeführt werden:

1. **Full (beim Boot):** `selftest_run_full()` — prüft alle 8 Module.
   Wird automatisch beim Booten ausgeführt. Bei PENDING_VERIFY wird
   der OTA-Rollback-Check ausgelöst.

2. **Full (manuell):** `selftest full` auf der Serial Console oder
   über die Companion-App. Gleiche 8 Module, gleiche Logik.

3. **Quick:** `selftest_run_quick()` — nur Power, Display und Sensors.
   Schneller (weniger als 1 Sekunde). Geeignet für schnelle Checks
   während der Fahrt oder in Pausen.

### 4.4 Heap/Stack-Monitoring

**Heap-Statistiken abfragen:**
```
> stats
```
Ausgabe enthält:
- `Free Heap` — aktuell freier Heap-Speicher
- `Min Free Heap` — minimaler freier Heap seit Boot (Low-Water-Mark)

Wenn `Min Free Heap` deutlich unter `Free Heap` liegt, gab es
einen Spitzenbedarf, der möglicherweise knapp am Limit war.

**Heap-Informationen im Code:**
```c
#include "esp_heap_caps.h"

multi_heap_info_t info;
heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
// info.total_free_bytes
// info.total_allocated_bytes
// info.minimum_free_bytes
// heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)
```

**Stack-High-Water-Mark:**
```c
UBaseType_t hw = uxTaskGetStackHighWaterMark(NULL);
// hw * sizeof(StackType_t) = freier Stack in Bytes
```
Empfehlung: High-Water-Mark > 25% der Stack-Größe.

### 4.5 Task-Statistiken

Mit `stats` auf der Serial Console werden die Task-Anzahl und
die Tick-Rate angezeigt. Für detaillierte Task-Statistiken
im Code:

```c
TaskStatus_t tasks[20];
UBaseType_t count = uxTaskGetSystemState(tasks, 20, NULL);
// tasks[i].pcTaskName — Task-Name
// tasks[i].xCoreID — zugewiesener Core
// tasks[i].uxCurrentPriority — Priorität
// tasks[i].usStackHighWaterMark — minimaler freier Stack
// tasks[i].eCurrentState — RUN, BLK (Blocked), RDY (Ready)
```

**Erwartete Tasks (nach board_config.h):**

| Task | Core | Prio | Stack | Rate |
|------|------|------|-------|------|
| sensor_task | 1 | 4 | 4096 | 50 Hz |
| gps_task | 1 | 5 | 4096 | 10 Hz |
| display_task | 0 | 2 | 8192 | 30 FPS (TFT) / 1 Hz (E-Paper) |
| anim_task | 0 | 4 | 2048 | 60 Hz |
| ble_task | 0 | 3 | 6144 | Event-getrieben |
| logger_task | 0 | 3 | 4096 | 5 Hz |
| power_task | 0 | 6 | 2048 | 1 Hz |
| ota_task | 0 | 4 | 8192 | Sleep bis Update |
| diag_console | 0 | 3 | 4096 | Event-getrieben |
| diag_events | 0 | 2 | 2048 | Event-getrieben |

Ein Task, der dauerhaft im Zustand RDY statt BLK ist, kann andere
Tasks blockieren. Ein Task, der nie läuft (immer BLK), ist
möglicherweise defekt.

### 4.6 GPX-Log-Analyse

GPX-Dateien werden auf der SD-Karte unter `/tracks/YYYYMMDD_HHMMSS.gpx`
gespeichert. Sie können am Computer mit GPX-Editoren oder
Touren-Plattformen geöffnet werden.

**GPX-Datei manuell prüfen:**
Die GPX-Datei ist ein XML-Dokument. Jeder Trackpunkt hat
Breitengrad (lat), Längengrad (lon), Höhe (ele) und Zeitstempel
(time):

```xml
<trkpt lat="50.9795" lon="11.3265">
  <ele>325.0</ele>
  <time>2026-10-13T14:30:22Z</time>
</trkpt>
```

**Fehlende Einträge identifizieren:**
Wenn die GPX-Datei Lücken hat (große Zeitabstände zwischen
Trackpunkten), deutet das auf GPS-Fix-Verluste hin. Die
DTC-Historie auf dem Bordcomputer (Serial Console:
`dtc history`) zeigt, wann DTC S0502 (GPS-Fix verloren) und
S0503 (GPS-Fix wiederhergestellt) aufgetreten sind. Die
Zeitstempel korrelieren mit den Lücken im GPX-Log.

**Track-Export über BLE:**
Der Track-Export über BLE dauert bei einer 24-Stunden-Tour mit
1 Hz GPS-Updates (ca. 86.400 Punkte, ca. 8,6 MB) etwa 7 Minuten
bei 20 kB/s. Bei Abbruch kann der Export neu gestartet werden.
Alternativ die SD-Karte entnehmen und am Computer auslesen.

---

## 5. FAQ

### Allgemeine Fragen

**F1: Kann ich das System an anderen Motorrädern verwenden?**

Der Bordcomputer wurde für die Honda XL600V Transalp 600 (PD06)
entwickelt. Die Sensoren (Pickup, Kühlmitteltemperatur, Kraftstoffstand)
sind auf die PD06 abgestimmt. Grundsätzlich kann das System an
anderen Motorrädern mit CDI-Zündung (1 Pickup-Puls pro
Kurbelwellenumdrehung) verwendet werden, wenn die
Anschlusspunkte und Kabelbaum-Farbcodes angepasst werden. Die
Übersetzungstabelle für die Ganganzeige muss für das jeweilige
Modell kalibriert werden. Bei Motorrädern mit CAN-Bus oder ECU
ist der Bordcomputer nicht ohne weiteres kompatibel, da er keine
OBD-II-Schnittstelle hat.

**F2: Wie kalibriere ich die Sensoren?**

Die Sensor-Kalibrierung erfolgt über die Companion-App unter
Einstellungen → Sensor-Kalibrierung:
- **Radumfang:** Für Hall-Sensor am Vorderrad (Vorderrad 90/90-21:
  2.185 mm, Hinterrad 130/80-17: 2.010 mm).
- **Kraftstoffstand:** Drei-Punkt-Kalibrierung (Voll, Halbvoll,
  Leer) durch volltanken, entleeren und Kalibrierpunkte setzen.
- **Temperatur-Offset:** Vergleich mit Referenzthermometer,
  Offset in °C einstellen.
- **IMU:** Bordcomputer flach hinlegen, Kalibrierung starten,
  dann in verschiedenen Orientierungen halten.

**F3: Wie aktualisiere ich die Firmware?**

Es gibt drei Wege:
1. **Über die Companion-App (BLE):** App öffnen, verbinden,
   Einstellungen → Firmware-Update → "Jetzt aktualisieren". Die App
   überträgt das Firmware-Image per BLE (ca. 15 Sekunden für 300 kB
   bei 20 kB/s). Nach der Übertragung verifiziert der Bordcomputer
   den SHA-256-Hash und startet neu.
2. **Über SD-Karte:** Firmware-Binary als `firmware.bin` auf die
   SD-Karte kopieren, SD-Karte einsetzen, im Diagnose-Menü
   "Firmware-Update → Von SD-Karte" wählen.
3. **Über USB-C (Entwickler):** `idf.py -p /dev/cu.usbserial-XXXX flash`
   mit aktiviertem ESP-IDF.

Während des Updates den Motor nicht starten (Spannungsspitzen
durch den Anlasser können den Update-Prozess stören) und die
BLE-Verbindung nicht trennen.

**F4: Welche SD-Karte wird empfohlen?**

Eine MicroSD-Karte, 8–32 GB, Class 10, FAT32-formatiert.
Industrial-Grade-Karten haben eine höhere Langlebigkeit bei
intensivem Schreiben. Größere Karten (64 GB+) werden möglicherweise
nicht zuverlässig unterstützt, da ESP-IDF FAT32 mit 32 KB-Clustern
erwartet. Karten regelmäßig auf Schreibfehler prüfen (Serial
Console: `dtc list`, DTC C0104).

**F5: Ist das System wasserdicht?**

Das Gehäuse ist IP67-zertifiziert (staubdicht, geschützt gegen
kurzes Eintauchen). Die Kabelverschraubungen (PG7/PG9) haben
O-Ring-Dichtungen. Der Deckel greift über das Unterteil
(Shingling-Prinzip), so dass Wasser an der Naht nach außen
abfließt. Eine ePTFE-Druckausgleichsmembran lässt Feuchtigkeit
entweichen. Nach 2–3 Saisons sollten die Dichtungen überprüft
werden. Keinen Hochdruckreiniger direkt auf das Gehäuse oder die
Kabeldurchführungen richten.

**F6: Kann ich die Displays einzeln verwenden?**

Ja. Das E-Paper-Display (2,9", 296×128 Pixel) ist das Hauptdisplay
und kann alleine betrieben werden. Das TFT-Display (optional,
1,14–2,0", SPI) ist ein Sekundärdisplay für zeitkritische Anzeigen.
In `board_config.h` kann der Display-Typ über Kconfig ausgewählt
werden (`CONFIG_BC_DISPLAY_EPAPER`, `CONFIG_BC_DISPLAY_ILI9488`,
`CONFIG_BC_DISPLAY_ST7789`). Ohne TFT-Display entfallen
schnelle Animationen, aber alle Kernfunktionen bleiben erhalten.

**F7: Wie ändere ich die Anzeige-Konfiguration?**

Über das Menü am Bordcomputer: Taste lang drücken (>2s) zum Öffnen
des Menüs, kurz drücken zum Navigieren, lang drücken zum Bestätigen.
Unter Anzeige → Display-Modus zwischen "Vollständig", "Kompakt" und
"Nav-Fokus" wählen. Alternativ über die Companion-App unter
Einstellungen → Display-Einstellungen. Die Helligkeit des
TFT-Backlights ist unter Anzeige → Helligkeit einstellbar.

**F8: Was tun bei EMV-Problemen?**

Siehe Abschnitt 2.1.4. Die wichtigsten Maßnahmen: Kupferfolie
innen (mit Fenster für Antennen), Ferritkerne an allen Kabeln
nah am Gehäuse, geschirmte Kabel (SFTP/S/UTP), Kabelabstand zur
CDI-Box (mindestens 10 cm), Optokoppler für RPM-Signal.

### Hardware-Fragen

**F9: Wo befindet sich der Pickup (Drehzahlsignal) am PD06?**

Die Pickup-Spule befindet sich am Kurbelgehäuse. Bei Modellen
bis 1995 (V-H bis V-R) gibt es zwei separate Pickup-Spulen
(Kabel gelb/blau), ab 1996 (V-T bis V-X) eine einzelne. Das
Signal kann am Pin 5 der CDI-Box (Kabel gelb/weiß) oder an der
Minusseite der Zündspule abgegriffen werden. Zwingend einen
Optokoppler zur galvanischen Trennung verwenden.

**F10: Wie messe ich die Kühlmitteltemperatur?**

Ein Inlinesensor (NTC-Thermistor) wird in den oberen Kühlschlauch
(vom Motor zum Kühler) eingesetzt. Der Schlauch-Innendurchmesser
beträgt meist 32–36 mm. Der Sensor hat folgende Kennlinie:
50°C → 154 Ohm, 80°C → 52 Ohm, 120°C → 16 Ohm. Am ADC des ESP32
(0–3,3V) wird ein Spannungsteiler mit Referenzwiderstand
verwendet. Genauigkeit: ±2°C.

**F11: Der Bordcomputer startet nicht beim Anlassen — was tun?**

Siehe Abschnitt 2.5.2 (Resets bei Motorstart). Hauptursache:
Spannungseinbruch beim Anlassen. Batterie prüfen (Leerlaufspannung
> 12,2V), Kabelquerschnitt prüfen (mindestens 0,75 mm²), Pufferakku
prüfen (Serial Console: `info`).

**F12: Kann ich das System im Winter bei Minusgraden verwenden?**

Der Bordcomputer ist für den Temperaturbereich -10°C bis +50°C
ausgelegt. Die Breittemperatur-Variante des E-Paper-Displays
erweitert auf -20°C bis +60°C. Bei unter -10°C kann das E-Paper
träge reagieren. Der LiPo-Pufferakku wird bei unter 0°C nicht
geladen (Software-Blockade gegen Lithium-Plating). Der ESP32-S3
ist bis +85°C spezifiziert, bei direkter Sonneneinstrahlung kann
das Gehäuseinnere 60–70°C erreichen.

### Software-Fragen

**F13: Wie aktiviere ich Secure Boot und Flash-Encryption?**

Secure Boot V2 und Flash-Encryption sind standardmäßig
deaktiviert. Siehe FIRMWARE-GUIDE.md, Abschnitt 3.2–3.3. Die
Aktivierung ist irreversibel (eFuse-Burning) und sollte nur für
Produktions-Builds durchgeführt werden. In `sdkconfig.defaults`:
```
CONFIG_SECURE_BOOT_V2_ENABLED=y
CONFIG_SECURE_FLASH_ENC_ENABLED=y
```

**F14: Was bedeutet PENDING_VERIFY?**

Nach einem OTA-Update bootet die neue Firmware im Status
`ESP_OTA_IMG_PENDING_VERIFY`. Der Self-Test entscheidet über die
Gültigkeit: Bei PASS wird die Partition mit
`esp_ota_mark_app_valid_cancel_rollback()` als gültig markiert.
Bei FAIL wird `esp_ota_mark_app_invalid_rollback_and_reboot()`
aufgerufen, und der Bordcomputer startet mit der vorherigen
Firmware neu.

**F15: Wie sehe ich, welche OTA-Partition aktiv ist?**

Mit der Serial Console: Der Self-Test gibt die aktive Partition
aus:
```
I (1089) selftest: OTA: running partition 'ota_0' @ 0x310000
I (1089) selftest: OTA: partition valid (state=0)
```
State 0 = ESP_OTA_IMG_VALID, State 1 = ESP_OTA_IMG_PENDING_VERIFY,
State 2 = ESP_OTA_IMG_INVALID.

**F16: Wie aktiviere ich ausführliches Logging?**

In `sdkconfig.defaults`:
```
CONFIG_LOG_DEFAULT_LEVEL_VERBOSE=y
```
Oder für Release-Builds:
```
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
```
Auf der Serial Console kann der Log-Level gefiltert werden:
```
idf.py monitor --print-filter="*:I"
```
Filter: `*` = alle, `tag:level` = spezifisch (z.B. `ble:D` für
BLE-Debug).

**F17: Wie überprüfe ich den freien Heap-Speicher?**

Serial Console: `stats` oder `info`. Im Code:
```c
uint32_t free = esp_get_free_heap_size();
uint32_t min  = esp_get_minimum_free_heap_size();
```
Der interne Heap des ESP32-S3 hat ca. 370 KB. Bei aktivem PSRAM
(8 MB Octal) steht zusätzlicher Speicher zur Verfügung.

### Diagnose-Fragen

**F18: Wie lösche ich alle DTCs?**

Serial Console:
```
> dtc clear all
All DTCs cleared.
```
Die DTCs werden nicht physisch gelöscht, sondern in den Status
HISTORY verschoben. Sie bleiben in der NVS-Persistenz gespeichert
und können mit `dtc history` abgefragt werden. Um die Historie
komplett zu leeren: NVS-Reset durchführen (siehe Abschnitt 6.2).

**F19: Was bedeutet ein Freeze-Frame?**

Ein Freeze-Frame ist ein Snapshot der Sensorwerte zum Zeitpunkt
der DTC-Auslösung. Er enthält: Batteriespannung, Kühlmitteltemperatur,
Umgebungstemperatur, Kraftstoffstand, RPM, Geschwindigkeit,
Roll/Pitch-Winkel, GPS-Satellitenanzahl, GPS-Fix-Status,
BLE-Verbindungsstatus und SD-Karten-Status. Der Freeze-Frame hilft
bei der Ursachenanalyse, da er die Bedingungen zum Zeitpunkt des
Fehlers zeigt.

**F20: Wie oft läuft der Self-Test automatisch?**

Der Self-Test läuft einmal beim Booten (Full Self-Test). Danach
läuft er nicht automatisch erneut, außer nach einem OTA-Update
(wo er über den Rollback-Check entscheidet). Der Event-Monitor-Task
(`diag_events`, 2 s Init-Verzögerung, dann 5 s Polling) überwacht
hingegen kontinuierlich System-Events und generiert DTCs:
- GPS-Fix verloren → DTC S0502
- GPS-Fix wiederhergestellt → DTC S0503
- SD-Karten-Fehler → DTC C0104
- SD-Karte gemountet → DTC C0101 löschen
- BLE getrennt → DTC C0202
- Batterie niedrig → DTC P0401
- Batterie kritisch → DTC P0402
- Crash erkannt → DTC S0402

**F21: Kann ich den Bordcomputer ohne Motorrad testen?**

Ja. Über USB-C kann der Bordcomputer mit Strom versorgt werden
(5V über USB). Allerdings funktionieren dann die 12V-Sensoren
(Pickup, Kühlmitteltemperatur, Kraftstoffstand) nicht, da sie
das 12V-Bordnetz benötigen. GPS, IMU, Barometer, Display und
BLE funktionieren auch über USB. Für vollständige Tests ohne
Motorrad kann eine Hardware-Test-Bench verwendet werden (siehe
HARDWARE-TEST-BENCH.md), die die Sensorsignale simuliert.

**F22: Was bedeutet der DEGRADING-Modus?**

Bei schlechter BLE-Verbindung (RSSI < -90 dBm oder Packet-Loss
> 10%) schaltet der Bordcomputer in den DEGRADING-Zustand: Es
werden nur noch priorisierte Daten übertragen (Navigation,
Anrufe), keine Telemetrie-Updates und keine Wetter-Daten. Die
Verbindung wird automatisch wiederhergestellt, sobald die
Signalqualität besser wird. Das ist ein Schutzmechanismus gegen
Datenverlust bei schlechter Verbindung, kein Fehler.

**F23: Wie exportiere ich die Diagnose-Logs?**

Über die Companion-App: Diagnose → System-Logs → Export. Die
Logs werden per BLE übertragen (Log-Export-Service). Alternativ
kann die SD-Karte entnommen und die Log-Dateien direkt am
Computer ausgelesen werden. Die Logs sind im CSV-Format
gespeichert und können mit Tabellenkalkulations-Programmen
geöffnet werden.

**F24: Wie stelle ich die Werkseinstellungen wieder her?**

Siehe Abschnitt 6.4 (Factory-Reset). Im Menü:
Konfiguration → Werkseinstellungen. Alternativ über die
Companion-App unter Einstellungen → Werkseinstellungen. Dies setzt
alle Konfigurationen (Einheiten, Alarme, Kalibrierung) auf die
Standardwerte zurück. DTCs werden nicht gelöscht (dafür
`dtc clear all` auf der Serial Console verwenden).

---

## 6. Recovery-Proceduren

### 6.1 Firmware-Recovery nach Brick

Wenn der Bordcomputer nach einem fehlgeschlagenen OTA-Update oder
einem Flash-Fehler nicht mehr bootet (Boot-Loop oder komplett
dunkel), kann die Firmware über USB-C wiederhergestellt werden.

**Voraussetzung:** ESP-IDF ist auf dem Computer installiert und
die Umgebung ist aktiviert (`. ~/esp/esp-idf/export.sh`).

**Schritt 1: USB-C verbinden**

Den Bordcomputer über USB-C mit dem Computer verbinden. Den
Serial-Port ermitteln:
```bash
# macOS:
ls /dev/cu.usbserial-*
# Linux:
ls /dev/ttyUSB*
```

**Schritt 2: Download-Modus erzwingen**

Wenn der Bordcomputer in einem Boot-Loop feststeckt, kann der
automatische Rollback den Download-Modus blockieren. GPIO0 beim
Einschalten gedrückt halten, um den Download-Modus zu erzwingen.
Auf DevKits gibt es dafür meist eine BOOT-Taste.

**Schritt 3: Factory-Partition flashen**

Die Factory-Partition (0x10000, 3 MB) dient als Notfall-Backup
und kann per Hardware-Pin (GPIO0 bei Boot) ausgewählt werden.
Wenn die OTA-Partitionen korrupt sind, kann direkt auf die
Factory-Partition geflasht werden:

```bash
cd firmware
# Firmware auf Factory-Partition flashen
idf.py -p /dev/cu.usbserial-XXXX erase-otadata
idf.py -p /dev/cu.usbserial-XXXX flash
```

`erase-otadata` löscht die OTA-Selection-Data, so dass der
Bootloader auf die Factory-Partition zurückfällt.

**Schritt 4: Flash komplett löschen (Full Erase)**

Wenn auch die Factory-Partition korrupt ist, den Flash komplett
löschen und neu flashen:

```bash
# Kompletten Flash löschen
esptool.py -p /dev/cu.usbserial-XXXX erase_region 0x0 0x1000000

# Bootloader, Partition Table und Firmware flashen
cd firmware
idf.py -p /dev/cu.usbserial-XXXX flash
```

Nach dem Flashen bootet der Bordcomputer mit der geflashten
Firmware. Die NVS-Konfiguration ist gelöscht — alle
Einstellungen müssen neu vorgenommen werden (siehe Abschnitt 6.2).

### 6.2 NVS-Reset (Konfiguration zurücksetzen)

Der NVS (Non-Volatile Storage) speichert die Geräte-Konfiguration,
Kalibrierungsdaten, BLE-Bonding-Keys und DTC-Persistenz. Ein
NVS-Reset setzt alle diese Daten auf die Standardwerte zurück.

**Über die Companion-App:**
Einstellungen → Konfiguration → NVS-Reset. Bestätigen. Der
Bordcomputer startet neu.

**Über die Serial Console:**
Die Serial Console bietet keinen direkten NVS-Reset-Befehl.
Stattdessen kann über `esptool.py` die NVS-Partition gelöscht
werden:

```bash
# NVS-Partition bei 0x9000, Größe 0x6000 (24 KB)
esptool.py -p /dev/cu.usbserial-XXXX erase_region 0x9000 0x6000
```

Nach dem nächsten Boot initialisiert ESP-IDF eine leere NVS.
Alle Konfigurationen werden aus den Defaults geladen.

**Wirkung:**
- Geräte-Konfiguration → Standardwerte
- Kalibrierungsdaten → gelöscht (neu kalibrieren!)
- BLE-Bonding-Keys → gelöscht (neu koppeln!)
- DTC-Persistenz → gelöscht (alle DTCs entfernt)
- OTA-Resume-State → gelöscht

### 6.3 SD-Karten-Formatierung

Siehe Abschnitt 2.4.3. Die SD-Karte muss FAT32-formatiert sein.
Am Computer mit einem Formatierungs-Tool FAT32 formatieren
(Blockgröße 32 KB). Alle Daten auf der Karte werden gelöscht.
Der Bordcomputer erstellt die Verzeichnisstruktur (`/tracks/`,
`/logs/`) beim nächsten Mount automatisch neu.

**Formatierung über die Serial Console:**
Die Serial Console bietet keinen direkten Format-Befehl. Wenn die
Karte am Computer formatiert wurde, einfach einsetzen und den
Bordcomputer neu starten. Der Self-Test mountet die Karte
automatisch.

### 6.4 Factory-Reset

Der Factory-Reset setzt alle Konfigurationen auf die
Standardwerte zurück, ohne die Firmware oder NVS-Daten zu löschen.

**Über das Menü am Bordcomputer:**
1. Taste lang drücken (>2s), um das Menü zu öffnen.
2. Mit kurzem Drücken zu "Konfiguration" navigieren.
3. Lang drücken zum Bestätigen.
4. Zu "Werkseinstellungen" navigieren.
5. Lang drücken zum Bestätigen.

**Über die Companion-App:**
Einstellungen → Werkseinstellungen → Zurücksetzen.

**Über die Serial Console:**
Es gibt keinen direkten Factory-Reset-Befehl auf der Serial
Console. Die Konfiguration kann jedoch über das NVS zurückgesetzt
werden (siehe Abschnitt 6.2). Alternativ können einzelne Werte
über die Config-Service-Characteristic (0x6002) zurückgesetzt
werden.

**Wirkung:**
- Einheiten → km/h, °C, bar
- Shift-Light → 7.000 U/min
- Rote Zone → 8.500 U/min
- Temperatur-Warnung → 110°C
- Niedrige Spannung → 11,5V
- Hohe Spannung → 15,5V
- Kraftstoff-Warnung → 20%
- Radumfang → 2.185 mm (Vorderrad)
- Display-Modus → Vollständig
- BLE-Gerätename → "Transalp-BC"

DTCs werden durch den Factory-Reset nicht gelöscht. Um DTCs zu
löschen: `dtc clear all` auf der Serial Console. BLE-Bonding-Keys
bleiben erhalten — das Smartphone muss nicht neu gekoppelt werden,
es sei denn, ein NVS-Reset wurde durchgeführt.

---

## Anhang

### A. Schnellreferenz: Serial Console-Befehle

```
dtc list              Aktive DTCs anzeigen
dtc history           DTC-Historie
dtc clear <code>      Einzelnen DTC löschen
dtc clear all         Alle DTCs löschen
selftest full         Vollständiger Self-Test (8 Module)
selftest quick        Schneller Self-Test (Power, Display, Sensors)
selftest result       Letztes Self-Test-Ergebnis
info                  System-Info
sensors               Live-Sensorwerte
gps                   GPS-Daten
stats                 System-Statistiken
help                  Befehlsübersicht
```

### B. Schnellreferenz: Debugging-Quick-Reference

| Symptom | Erster Schritt |
|---------|---------------|
| Boot-Loop | Serial Console, Backtrace lesen |
| OTA schlägt fehl | DTC D0301 prüfen, Partition Table kontrollieren |
| BLE verbindet nicht | DTC C0201/C0202 prüfen, Pairing wiederholen |
| Display leer | SPI-Pins in `board_config.h` prüfen, DTC B0101 |
| Heap-Corruption | `CONFIG_HEAP_POISONING_COMPREHENSIVE=y`, Heap-Tracing |
| Stack-Overflow | `uxTaskGetStackHighWaterMark()` für alle Tasks |
| Watchdog-Timeout | Task-Prioritäten prüfen, `vTaskDelay` einfügen |
| Flash-Write-Fail | `esp_ota_write` Return-Code prüfen |
| GPS kein Fix | Antenne prüfen, draußen warten (Kaltstart bis 2 min) |
| SD-Karte fehlt | DTC C0101, Karte entnehmen und einsetzen |

### C. Quellen

- `docs/USER-MANUAL.md` — Benutzerhandbuch & Installationsanleitung
- `docs/FIRMWARE-GUIDE.md` — Firmware-Engineering-Guide
- `docs/SECURITY-ANALYSIS.md` — Sicherheitsanalyse & Threat Model
- `docs/ARCHITECTURE.md` — System-Architektur
- `docs/PERFORMANCE-OPTIMIZATION.md` — Performance-Optimierung
- `docs/HARDWARE-TEST-BENCH.md` — Hardware-Test-Bench
- `firmware/components/diagnostics/` — Diagnose-System (DTC, Self-Test, Console)
- `firmware/components/ble/` — BLE-Stack (NimBLE GATT-Server)

---

*Dieses Dokument wird mit jeder Firmware-Version aktualisiert.
Bei neuen Problemen oder Lösungen bitte einen Eintrag im
entsprechenden Abschnitt hinzufügen.*

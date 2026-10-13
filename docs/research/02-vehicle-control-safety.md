# Recherche: Signale senden, Steuerung & Sicherheit (Honda Transalp XL750)

> **Status:** Recherche-Dokument, Stand Oktober 2026
> **Fahrzeug:** Honda XL750 Transalp (2023+), EU-Modell
> **Projekt:** Transalp Bordcomputer (ESP32-S3)

---

## 1. Fahrzeugarchitektur: CAN-Bus und K-Line

Die XL750 Transalp hat **zwei CAN-Busse** und eine **K-Line** (ISO 9141) für
die Diagnose. Das ist wichtig: die CAN-Busse sind nicht gleichwertig.

### 1.1 Diagnoseport (6-polig, nicht J1962)

Die Transalp hat einen **6-poligen Honda-Diagnosestecker** (nicht den
16-poligen OBD2/J1962 wie Autos). Pinout laut advrider-Recherche:

| Funktion | verfügbar über | Bemerkung |
|---|---|---|
| CAN-H/CAN-L | Diag-Port | OBD-Protokoll, liefert Speed + RPM |
| K-Line | Diag-Port | ISO 9141, Zugriff auf ECU-intern (DTCs, Parameter) |

Über den CAN-Zugang im Diag-Port erhält man nur Basis-OBD-PIDs
(Geschwindigkeit, Drehzahl). Für tiefere Daten (Brems, Gang, HSTC-Status)
braucht man die K-Line oder direkten Zugriff auf den Fahrzeug-CAN-Bus
hinter dem Tacho.

### 1.2 CAN-Bus-Struktur

| Bus | Speed | Inhalte |
|---|---|---|
| Powertrain CAN (HS-CAN) | 500 kbit/s | ECU, ABS, HSTC, IMU, Dash |
| Sub-CAN (LS-CAN) | 100-125 kbit/s | Body: Licht, Blinker, Hebel, Schalter |

Der Powertrain-Bus führt alle sicherheitsrelevanten Signale. Der Sub-Bus
ist für Komfort- und Body-Funktionen.

### 1.3 Vergleich: opendbc / commaai

Honda-Personenwagen sind in [opendbc](https://github.com/commaai/opendbc)
gut dokumentiert. Für die XL750 Transalp existiert **keine DBC-Datei**.
Honda-Motorräder teilen sich CAN-IDs teilweise mit den Auto-Plattformen
(ähnliche PDU-Struktur), aber die genauen Signaldefinitionen sind
modellspezifisch und müssen reverse-engineered werden.

Honda Sportbikes (CBR1000RR-R, CBR600RR HRC) haben einen bekannten
"LOGGER"-CAN-Zugang (Yazaki-Connector, AiM-dokumentiert) mit detaillierten
Signalen für RPM, TPS, HSTC-Mode, Wheelie-Mode, Engine-Brake-Mode, ABS-Status.
Die Transalp nutzt das gleiche ECU-Prinzip, aber die CAN-IDs und Signale
sind nicht identisch — Reverse Engineering nötig.

---

## 2. Steuerungsmöglichkeiten (Write)

### 2.1 Blinker, Hupe, Licht

⚠️ **CAN-Injection in diese Signale ist nicht empfohlen.**

Die Transalp steuert Blinker, Hupe und Licht teilweise über den Sub-CAN
(Body-Bus), aber die physikalische Leistungsaufnahme (Blinker-Glühbirnen
oder LED-Arrays, Hupe als induktive Last) läuft über Relais bzw.
Leistungstreiber im Kabelbaum. Ein Eingriff per CAN würde bedeuten:
CAN-Message injecten, die das Body-Control-Modul veranlasst, den
entsprechenden Ausgang zu schalten.

Probleme:
- CAN-IDs und Signalpositionen für Body-Funktionen sind **nicht dokumentiert**
- Falsche Messages können das Body-Control-Modul verwirren, Fehlercodes
  auslösen oder (im schlimmsten Fall) Funktionen deaktivieren
- Der CAN-Bus hat keine Authentifizierung — jeder Teilnehmer kann senden
- Honda nutzt counter/rolling-code Prüfung auf dem Powertrain-Bus,
  bei Body-Bus weniger restriktiv, aber nicht trivial

**Empfehlung: Relais-Schaltung (siehe Abschnitt 5)**

### 2.2 Tacho-Ansteuerung

Der Originaltacho der Transalp ist ein TFT-Display, das direkt am
Powertrain-CAN hängt. Die Signale für Geschwindigkeit, Drehzahl, Gang,
Kühltemperatur, Kraftstoff etc. werden vom ECU gebroadcastet und vom
Tacho empfangen.

**Zusatzinstrumente ansteuern (unbedenklich):**
- Canalyzer/ESP32 mit CAN-Transceiver liest den Powertrain-Bus mit
- Daraus: Speed, RPM, Gang, Temp für eigenes Display extrahieren
- Kein Schreibzugriff nötig — nur Lesen
- Risiko: minimal (passiver Sniffer)

⚠️ **Tacho-Signale injecten (nicht empfohlen):**
- Um den Originaltacho zu manipulieren (z.B. anderen Speed anzeigen),
  müsste man die ECU-Telegramme spoofen
- Das ECU würde widersprüchliche Signale sehen → Fehlercodes,
  Safe-Mode, im schlimmsten Fall Fahrverhaltensänderungen

### 2.3 Heizgriffe, Zusatzscheinwerfer, Zubehör

Dies sind **rein elektrische Verbraucher**, die nicht über CAN gesteuert
werden. Sie werden an das 12V-Bordnetz angeschlossen und über Relais
bzw. MOSFETs geschaltet.

**Sauberer Weg:**
- ESP32 GPIO → Logic-Level MOSFET → Relais → Verbraucher
- Stromversorgung direkt von der Batterie (nicht über CAN-Bus-Leitung)
- Eigene Sicherung für jeden Kanal
- Freilaufdiode über induktive Lasten (Hupe, Relais-Spulen)

Siehe Abschnitt 5 für Schaltungsdetails.

### 2.4 Motorsteuergerät (ECU)

⚠️⚠️ **LEBENSGEFÄHRLICH — NICHT EMPFOHLEN**

Die Transalp hat Ride-by-Wire (elektronische Drosselklappe). Das ECU
steuert Motor, Drosselklappe, Einspritzung, Zündung. Ein Eingriff in
das ECU kann folgende Auswirkungen haben:

- **Mapping-Änderung:** Honda-ECUs können per K-Line umgeflasht werden
  (Honda Diagnostic System / HDS). Das ist eine Standard-Werkstattfunktion.
  TuneBoy, Dynojet Power Vision, Rexxer bieten aftermarket-Mappings an.
  Das ändert das Grundmapping — riskant, aber etabliert.

- **Drehzahlbegrenzer aufheben:** Theoretisch per Mapping-Änderung möglich.
  Bei der Transalp (758cc, 67kW) ist der Begrenzer bei ~8.500 rpm.
  Überschreitung → mechanische Zerstörung des Motors (Pleuel, Ventile).

- **HSTC/ABS-Status per CAN injecten:** Siehe Abschnitt 3.

- **Drosselklappe direkt ansteuern:** Absolut LEBENSGEFÄHRLICH. Das ECU
  nutzt redundante TPS-Sensoren und Plausibilitätsprüfungen. Ein Spoof
  der TPS-Werte würde das ECU in Notlauf zwingen oder (falls es klappt)
  unkontrollierte Motorleistung freigeben.

**Empfehlung:** ECU komplett in Ruhe lassen. Für Leistungsänderungen
gibt es Power Commander, Dynojet, Rexxer — professionelle Tools mit
Sicherheitsmechanismen.

---

## 3. Sicherheitsrelevante Systeme

### 3.1 ABS (Anti-Blockier-System)

Die Transalp hat ein Zweikreis-ABS (vorn/hinten getrennt).

**Vorhandene Funktionen (serienmäßig):**
- Front-ABS: Immer aktiv, kann nicht deaktiviert werden (hardware-seitig)
- Rear-ABS: Kann über das Dashboard-Menü (USER Mode) deaktiviert werden
- Off-Road-Modus: Bei deaktiviertem Rear-ABS schaltet das Front-ABS in
  einen weniger sensitiven Off-Road-Modus

**CAN-Signale (geschätzt, nicht bestätigt):**
- Radgeschwindigkeiten (4 Signale, falls 4 Räder einzeln) bzw. 2 Signale
  (vorn/hinten) → können auf dem Powertrain-CAN gelesen werden
- ABS-Status (aktiv/inaktiv/intervention) → wahrscheinliche CAN-ID-Bereich
  0x1xx-0x2xx (Honda-Pattern, aber unbestätigt für XL750)
- ABS-Fehlercodes → über K-Line (HDS) auslesbar

⚠️ **ABS per CAN deaktivieren:**
Möglich prinzipiell — das LSK Off-Road Dongle beweist das (siehe unten).
Es interceptiert den CAN-Bus zwischen Kabelbaum und Tacho und injectiert
Messages, die das ECU veranlassen, Rear-ABS und HSTC zu deaktivieren.
Das Dongle ist aber ein professionelles Produkt, das genau weiß, welche
CAN-IDs und Signale zu schicken sind. Selbst gebastelte CAN-Injection
hierfür ist riskant.

**Lesen von ABS-Daten: unbedenklich.**
**Schreiben in ABS-Steuergerät: ⚠️ Kritisch — nur mit bekanntem Protokoll.**

### 3.2 HSTC (Honda Selectable Torque Control = Traktionskontrolle)

HSTC ist die Honda-Traktionskontrolle mit mehreren Levels (off, 1, 2, 3).
Auf der Transalp in USER-Mode einstellbar.

**Verhalten:**
- HSTC greift in die Motorleistung ein (Drosselklappe, Zündung, Einspritzung),
  wenn es Radschlupf am Hinterrad detektiert
- Das System nutzt die Radgeschwindigkeitssensoren des ABS
- Status wird auf dem TFT-Display angezeigt

**CAN-Signale (geschätzt nach Honda CBR-Pattern):**
- HSTC-Mode (off/1/2/3) → Broadcast vom ECU
- TCS_Intervention_Status → wie stark greift HSTC ein (0-100%)
- Slip_Rate → berechnete Schlupf-Rate

⚠️ **HSTC per CAN deaktivieren:**
Gleicher Mechanismus wie ABS — das LSK-Dongle macht genau das. Der Dongle
injectiert CAN-Messages, die das ECU veranlassen, HSTC auszuschalten.
Bei Fehlmanipulation: unkontrollierte Motorleistung bei Schlupf → Sturz.

**Lesen: unbedenklich. Schreiben: ⚠️ Kritisch.**

### 3.3 Wheelie Control

Bei Honda-Sportbikes (CBR-Modellreihe) ist Wheelie Control eine separate
Funktion mit eigenem Mode (3 Levels). Die Transalp hat diese Funktion
in vereinfachter Form als Teil von HSTC.

**Signale (nach AiM ECU Stream Dokumentation für CBR HRC):**
- Wheelie_Mode → 0-3
- Pitch_Angle → Neigungswinkel (von IMU)
- Rel_PitchWheelie → Relativer Nickwinkel ab Wheelie-Start
- Pitch_Rate → Winkelgeschwindigkeit

Die Transalp hat eine IMU (6-Achsen, Bosch), deren Daten ins ECU fließen.
Ob Wheelie Control bei der Transalz separat einstellbar ist, hängt von der
Firmware ab — in USER-Mode kann HSTC (inkl. Wheelie) eingestellt werden.

⚠️ **Wheelie-Control-Daten sind lesbar über CAN (Pitch, Bank Angle).**
Ein eigenes Dashboard kann diese Daten anzeigen (interessant fürs
Bordcomputer-Projekt!). Schreibzugriff nicht empfohlen.

### 3.4 Engine Brake Control

Honda bietet bei Sportbikes eine einstellbare Motorbremswirkung
(Engine Brake Mode). Die Transalp hat dies in der Serieneinstellung
nicht frei konfigurierbar, aber das ECU kennt die Funktion intern.

**Signale (CBR HRC Referenz):**
- EB_Mode → Engine Brake Mode
- EBSLIP_Mode → Slip-basierte Motorbrems-Kontrolle

Für den Bordcomputer: reine Lesefunktion. Kein Schreibzugriff nötig.

### 3.5 Was man NICHT tun sollte

⚠️⚠️ **LEBENSGEFÄHRLICH — Absolute No-Gos:**

1. **Drosselklappen-Position spoofen:** Das ECU hat redundante TPS-Sensoren
   mit Plausibilitätsprüfung. Ein Spoof-Versuch wird erkannt → Notlauf
   oder unkontrollierte Leistungsfreigabe.

2. **Brems-Steuergerät per CAN injecten:** ABS-Hydraulikpumpe ansteuern
   → unkontrolliertes Bremsen. Lebensgefährlich.

3. **Lenkungs-Steuergerät manipulieren:** Nicht relevant für Transalp
   (keine elektronische Lenkung), aber prinzipiell: Niemals.

4. **Zündung/Einspritzung per CAN manipulieren:** Motor kann ausgehen
   oder unkontrolliert hochdrehen.

5. **IMU-Daten spoofen:** Wenn das ECU falsche Neigungsdaten bekommt,
   kann HSTC/Wheelie Control falsch reagieren → Sturzgefahr.

6. **CAN-Bus DoS (Flooding):** Zu viele Messages oder fehlerhafte
   Telegramme können den Bus lahmlegen → ECU geht in Notlauf,
   alle Assistenzsysteme fallen aus.

---

## 4. Risikoabschätzung

### 4.1 Unbedenklich (grün)

| Aktion | Begründung |
|---|---|
| CAN-Bus passiv lesen (Sniffer) | Keine Auswirkung auf das Fahrzeug |
| OBD-PIDs auslesen (Speed, RPM, Temp) | Standard-OBD, definiert und sicher |
| GPS-Daten separat aufzeichnen | Unabhängig vom Fahrzeug |
| IMU/BMP280-Daten des Bordcomputers | Eigene Sensoren, kein Fahrzeugbezug |
| Zusatzinstrumente mit gelesenen CAN-Daten betreiben | Nur Anzeige, kein Schreibzugriff |
| Zubehör über Relais schalten (Heizgriffe, Zusatzlicht) | Galvanisch getrennt vom CAN-Bus |
| Akku-Überwachung, Display, SD-Logging | Komplett autark |

### 4.2 Bedingt machbar (gelb)

| Aktion | Begründung |
|---|---|
| CAN-IDs reverse-engineeren (mit Logik-Analyzer) | Aufwendig, aber legitim. Risiko: nur Lesen, kein Schreiben |
| K-Line Diagnosezugriff (HDS-Protokoll) | Etabliert, aber man kann DTCs löschen — Vorsicht |
| Eigene CAN-Messages auf Sub-Bus (Body) senden | Nur wenn IDs bestätigt sind. Riskant bei Unklarheit |
| LSK-Dongle-ähnliche Funktion nachbauen | Das Protokoll ist unbekannt. Sehr hoher Aufwand |

### 4.3 Kritisch (rot) ⚠️

| Aktion | Begründung |
|---|---|
| ABS/TC per CAN deaktivieren (selbst gebaut) | Lebensgefährlich bei fehlerhaften Messages |
| ECU-Flashing (Mapping-Änderung) | Mechanische Zerstörung möglich |
| Drosselklappe/Bremsung per CAN injecten | Akute Lebensgefahr |
| CAN-Bus flooding / DoS | Systemausfall beim Fahren |
| IMU-Daten an ECU spoofen | Falsche Assistenzsystem-Reaktionen |

### 4.4 Rechtliche Aspekte

⚠️ **Straßenverkehrsrecht:**
- Jeder Eingriff in bauartgenehmigte Fahrzeugsysteme kann die
  Betriebserlaubnis erlöschen lassen (§19 StVZO, §3 FZV)
- ABS und HSTC sind prüfungsfähige Systeme — Deaktivierung oder
  Manipulation kann als fahrlässige Gefährdung gewertet werden
- Bei Unfällen mit manipulierten Sicherheitssystemen: grobe Fahrlässigkeit
  → Versicherungsschutz kann verweigert werden

⚠️ **Gewährleistung:**
- Jeder Eingriff in den CAN-Bus oder das ECU führt zum Verlust der
  Honda-Neuwagengarantie
- ECU-Flashes sind nachweisbar (Flash-Counter im ECU)

⚠️ **TÜV:**
- Zusätzliche elektronische Geräte müssen in der Anbauprüfung abgenommen
  sein (§19a StVZO)
- Manipulationen an Brems-/Lenksystemen fallen auf jeden Fall auf

---

## 5. Relais-Schaltung als sicherer Weg

### 5.1 Warum Relais statt CAN-Injection

Die sicherste Methode, Fahrzeugfunktionen vom ESP32 aus zu steuern,
ist die **galvanische Trennung per Relais**. Der CAN-Bus bleibt unangetastet,
die Funktion wird physikalisch parallel geschaltet.

Vorteile:
- Kein Risiko für Fahrzeug-Elektronik
- Keine CAN-Protokoll-Kenntnis nötig
- Fehlersuche klassisch (Multimeter statt Logik-Analyzer)
- Versicherungsschutz bleibt unberührt (solange nur Zubehör geschaltet wird)

Nachteile:
- Mehr Verkabelungsaufwand
- Keine Integration in das Original-Dashboard
- Relais brauchen Platz und sind mechanisch verschleißanfällig

### 5.2 Schaltungsprinzip

```
ESP32 GPIO (3.3V)
    │
    ├── 220Ω Gate-Widerstand
    │
    Gate ── Logic-Level MOSFET (IRLZ44N / AO3400)
    │
    Drain ── Relais-Spule (12V)
    │         └── Freilaufdiode (1N4007) parallel zur Spule
    │
    Source ── GND
    │
    Relais-Kontakte ── Verbraucher (12V, direkt von Batterie)
```

### 5.3 MOSFET-Relais für Motorrad

Statt klassischer mechanischer Relais sind für Motorrad-Verhältnisse
(Vibration, Feuchtigkeit, Platzmangel) **Solid-State-Relais** oder
**MOSFET-basierte Schaltmodule** besser geeignet:

| Eigenschaft | Mechanisches Relais | MOSFET-Modul |
|---|---|---|
| Verschleiß | Ja (Kontakte) | Nein |
| Vibrationsempfindlich | Mittel | Sehr gering |
| Schaltgeschwindigkeit | Langsam (~10ms) | Sehr schnell (µs) |
| Wärmeentwicklung | Gering (Kontaktwiderstand) | Gering (RDSon) |
| Galvanische Trennung | Ja | Nein (gemeinsame Masse) |
| Preis | 2-5€ | 3-8€ |
| Lebensdauer | ~100k Zyklen | >1M Zyklen |

**Empfohlene MOSFETs für ESP32 (Logic-Level, 3.3V Gate):**

| MOSFET | Vds | Rds(on) @3.3V | I_Continuous | Gehäuse | Bemerkung |
|---|---|---|---|---|---|
| IRLZ44N | 55V | 25mΩ @5V, ~35mΩ @3.3V | 47A | TO-220 | Standard, robust |
| IRL540N | 100V | 44mΩ @5V | 28A | TO-220 | Höhere Spannung |
| AO3400 | 30V | ~50mΩ @2.5V | 5.8A | SOT-23 | SMD, klein |
| BSS138 | 50V | ~3.5Ω @3.3V | 0.22A | SOT-23 | Nur für Signale |
| NDP6020P | -20V | 50mΩ | -24A | TO-220 | P-Kanal, High-Side |

Für Motorrad-Zubehör (Heizgriffe ~3-5A, Zusatzscheinwerfer LED ~2-4A,
Hupe ~5-10A峰值): IRLZ44N mit Kühlkörper, oder fertiges MOSFET-Modul
(z.B. "Mosfet Switch Module" von moto32-Projekt).

### 5.4 Ansteuerung vom ESP32

```
3.3V GPIO ── 220Ω ── Gate (MOSFET)
                      │
3.3V ── 10kΩ Pull-Down (Gate-Source)
```

- **Gate-Widerstand (220Ω):** Begrenzt den Gate-Strom beim Umschalten,
  verhindert Oszillation und RFI
- **Pull-Down (10kΩ):** Sichert, dass der MOSFET beim Boot des ESP32
  sicher ausgeschaltet bleibt (GPIO ist beim Boot im High-Z Zustand)
- **Freilaufdiode (1N4007 für DC, 1N5819 Schottky für schnelles Schalten):**
  Über jede induktive Last (Relais-Spule, Hupe, Magnetventil)

### 5.5 Referenzprojekt: moto32

Das [moto32-Projekt](https://github.com/moto32/moto32-hardware) ist ein
Open-Source-Motorrad-Controller (ESP32-S3, Motogadget M-Unit-Alternative)
mit 8 MOSFET-geschützten Ausgängen. Schaltpläne, BOM und 3D-Modelle
sind verfügbar. Das ist ein guter Referenzpunkt für die Hardware-Seite
des Bordcomputer-Projekts.

---

## 6. Bordcomputer-spezifische Empfehlung

### 6.1 Was der Bordcomputer tun soll (safe)

1. **CAN-Bus passiv lesen:** Speed, RPM, Gang, Kühltemperatur, HSTC-Status,
   Kraftstoffstand — alles über OBD-PIDs oder direktes CAN-Sniffing
2. **IMU-Daten des Fahrzeugs lesen** (falls zugänglich): Bank Angle, Pitch
   → interessant für Neigungsanzeige und Wheelie-Warning
3. **Eigene Sensoren anzeigen:** GPS-Position, Geschwindigkeit, Höhe,
   Neigung (eigener MPU6050), Temperatur (eigener BMP280)
4. **Zubehör schalten:** Heizgriffe, Zusatzscheinwerfer über MOSFET-Relais
5. **Daten loggen:** SD-Karte, GPX-Tracks, Telemetrie

### 6.2 Was der Bordcomputer NICHT tun soll ⚠️

1. Keine CAN-Messages aktiv senden auf dem Powertrain-Bus
2. Keine Manipulation von ABS, HSTC, Wheelie Control, Engine Brake
3. Kein ECU-Flashing
4. Kein Spoofen von Sensor-Daten
5. Kein CAN-Bus-Flooding

### 6.3 Erweiterte Optionen (mit klarer Kennzeichnung)

- **Off-Road-Dongle-Funktion nachbauen:** Theoretisch möglich, wenn man
  das CAN-Protokoll für ABS/TC-Deaktivierung reverse-engineered hat.
  ⚠️ Lebensgefährlich bei Implementierungsfehlern. Nur für geschlossene
  Strecke / Offroad-Bereich. Dokumentation der gefundenen CAN-IDs in
  einer separaten DBC-Datei mit Safety-Warnungen.

---

## 7. Pin-Erweiterung für Relais-Schaltung

Erweiterung der PINMAP.md um MOSFET/Relais-Ausgänge:

| Funktion | GPIO | Bemerkung |
|---|---|---|
| Zubehör 1 (Heizgriffe) | GPIO 15 | MOSFET Gate, Pull-Down 10k |
| Zubehör 2 (Zusatzscheinwerfer) | GPIO 16 | MOSFET Gate, Pull-Down 10k |
| Zubehör 3 (Reserve) | GPIO 17 | MOSFET Gate, Pull-Down 10k |
| Zubehör 4 (Reserve) | GPIO 19 | MOSFET Gate, Pull-Down 10k |

Diese Pins sind in der aktuellen PINMAP als "Frei/Reserve" markiert.
GPIO 19/20 sind USB (D+/D-) — GPIO 19 nur nutzen wenn USB-Debug nicht
gebraucht wird.

⚠️ **Boot-Strap-Pins beachten:** GPIO 0 und GPIO 46 sind Boot-Strap-Pins.
Diese nicht als MOSFET-Output verwenden (kann Boot-Probleme verursachen).

---

## 8. Quellen

- advrider.com XL750 Transalp Thread: CAN-Bus-Struktur (2 CAN + K-Line),
  Diag-Port Pinout, OBD-Verhalten
- lskelectronics.com: Off-Road Dongle — CAN-Bus-Intercept zwischen
  Kabelbaum und Tacho, zwingt ECU zu ABS/TC-Deaktivierung
- commaai/opendbc: Honda DBC-Files (Autos), CAN-Signal-Muster, Safety-Code
- AiM Sportsystems: ECU-Stream-Doku CBR HRC 2020-2025 — Signalliste für
  HSTC, Wheelie, Engine Brake, ABS-Status (über CAN-Logger-Port)
- bennetts.co.uk: CAN-Bus Motorrad-Grundlagen
- obdplanet.com: Honda OBD2 Pinout (CAN-H Pin 6, CAN-L Pin 14, K-Line Pin 7)
- moto32/moto32-hardware: Open-Source ESP32 MOSFET-Controller für Motorrad
- electronics.sempervent.github.io: ESP32 MOSFET Best Practices

---

## 9. Zusammenfassung

| Kategorie | Empfehlung |
|---|---|
| CAN-Bus lesen | ✅ Ja, passiv, sicher |
| OBD-PIDs auslesen | ✅ Ja, Standard |
| CAN-Messages senden (Powertrain) | ⚠️ Nein, lebensgefährlich |
| CAN-Messages senden (Body/Sub) | ⚠️ Nur bei bestätigten IDs, riskant |
| ABS/HSTC/Wheelie deaktivieren | ⚠️⚠️ Nein, nur über Dashboard oder LSK-Dongle |
| ECU-Flashing | ⚠️⚠️ Nur mit professionellen Tools (Power Commander etc.) |
| Zubehör schalten (Heizgriffe, Licht) | ✅ Ja, über MOSFET-Relais |
| Zusatzinstrumente betreiben | ✅ Ja, mit gelesenen CAN-Daten |
| Eigene Sensoren + Logging | ✅ Ja, komplett autark |

**Kernprinzip: Lesen ja, Schreiben nein — außer über Relais.**

# Recherche: Signale senden, Steuerung & Sicherheit (Honda Transalp XL750)

> **Zusammenfassung**: Die Transalp XL750 hat zwei getrennte CAN-Busse plus K-Line. Der OBD-Diagnoseport liefert nur Speed und RPM, nicht aber Brems- oder Gang-Informationen. Write-Zugriff auf sicherheitsrelevante Systeme (ABS, HSTC, ECU) über CAN-Injection ist technisch möglich, aber **nicht empfohlen** — der sichere Weg ist Relais/MOSFET-Schaltung für Zubehör und reiner Lesezugriff auf CAN-Daten.

---

## 1. Fahrzeug-Architektur: CAN-Bus-Struktur der XL750 Transalp

Die Transalp verwendet **zwei CAN-Busse** und eine **K-Line** (Quelle: Veridian Cruise Entwickler-Thread auf ADVrider [1]):

| Bus / Interface | Anschluss | Verfügbare Daten | Bemerkung |
|---|---|---|---|
| **CAN 1 (OBD/Diag)** | 6-poliger Diagnosestecker (unter Sitz) | Speed, RPM via OBD-Protokoll | Keine Brems-, Gang- oder Sensor-Daten |
| **CAN 2 (Internal)** | Kein freier Stecker — Kabelbaum muss angezapft werden | Wheel Speed (einzeln), Gear, RPM | Höherer Aufwand, immer noch keine Brems-Info |
| **K-Line** | Diagnosestecker (Pin 4) | ECU-Diagnose, DTCs, ggf. Live-Daten | Honda-eigenes Protokoll, kein Standard-OBD |

Die **Body Control Unit (BCU)** sitzt am linken Hauptrahmen unter dem Tank und verarbeitet Signale von ABS-Modulator, TFT-Display und Schalter [2]. Sie ist das zentrale Knotenelement für die Elektrik.

### Diagnoseport (6-polig)

Der 6-polige Diagnosestecker ist der primäre physische Zugriffspunkt. Honda nutzt hier keinen Standard-J1962 (OBD2), sondern einen kompakteren Motorrad-Stecker.

Pinout (basierend auf Honda Motorrad-Standards und KESS3-Kabel-Spezifikationen [3]):

| Pin | Signal | Nutzung |
|---|---|---|
| 1 | GND | Masse |
| 2 | CAN-H | CAN-High (Diagnose-Bus) |
| 3 | CAN-L | CAN-Low (Diagnose-Bus) |
| 4 | K-Line | KWP2000 / Honda-proprietär |
| 5 | +12V (ignitions geschaltet) | Stromversorgung Diagnosegerät |
| 6 | — | unbelegt / Reset |

**Praxis**: Ein ELM327-Bluetooth-Modul mit passendem Adapterkabel liest Speed und RPM über Standard-OBD-PIDs. Für mehr Daten (Wheel Speed, Gear) müsste der interne CAN-Bus angezapft werden — invasiv und nicht plug-and-play.

---

## 2. Steuerungsmöglichkeiten (Write)

### 2.1 Blinker, Hupe, Licht

⚠️ **Empfehlung: Relais-basiert, NICHT CAN-Injection.**

| Aktor | Original-Ansteuerung | Eingriff möglich? | Empfehlung |
|---|---|---|---|
| Blinker | BCU über Lenkerschalter (Resistor-Ladder) | CAN-Injection theoretisch, aber BCU validiert Quellsignale | **Relais parallel** zum Schalter |
| Hupe | Direkt über Lenkerschalter + Relais | CAN nicht involviert | **Relais parallel** zum Taster |
| Fernlicht | Lenkerschalter → BCU → LED-Driver | CAN-Injection unsicher | **Relais parallel** zum Schalter |
| Standlicht | BCU-gesteuert | CAN-Injection unsicher | **Relais parallel** |

**Warum nicht CAN-Injection?**

Die BCU der Transalp validiert Signalquellen. Ein CAN-Frame, der einen Blinker-Befehl simuliert, könnte von der BCU verworfen werden, wenn der Quell-ECU (z.B. Lenkerschalter-Interface) nicht korrekt authentifiziert ist. Zudem besteht die Gefahr, dass die BCU einen Fehler loggt (DTC) und das System in einen Failsafe-Modus geht. Bei Blinkern und Hupe ist der Aufwand-Nutzen-Faktor von CAN-Injection extrem schlecht — ein Relais kostet 2€ und funktioniert garantiert.

### 2.2 Tacho-Ansteuerung

Der originale TFT-Tacho (5") kommuniziert über den internen CAN-Bus mit der BCU. Die relevanten CAN-IDs sind **proprietär und nicht dokumentiert**.

| Funktion | CAN-Zugriff | Bemerkung |
|---|---|---|
| Speedometer | CAN-Lesbar (OBD PID 0x0D) | Anzeige-Wert kann gelesen werden |
| Drehzahl | CAN-Lesbar (OBD PID 0x0C) | Standard-OBD |
| Gang-Anzeige | Nur auf internem CAN-Bus | Nicht über OBD-Port |
| Kraftstoffstand | CAN-Lesbar (OBD PID 0x2F) | Falls ECU unterstützt |
| Kühlmitteltemperatur | CAN-Lesbar (OBD PID 0x05) | Standard-OBD |

⚠️ **Tacho-Spoofing (Speedometer-Manipulation) ist nicht empfohlen.**
Eine Manipulation des Tachos via CAN-Injection verstößt gegen §25 StVZO (Tachomanipulation) und §22 StVG. Strafbar, und das TeilWie geht kaputt wenn die BCU inkonsistente Signale bekommt.

**Zusatzinstrumente ansteuern**: Unbedenklich. Zusatz-Display (z.B. ESP32 + E-Paper) liest CAN-Daten nur und zeigt sie an. Kein Write-Zugriff nötig.

### 2.3 Heizgriffe, Zusatzscheinwerfer

⚠️ **Empfehlung: MOSFET-Schaltung, NIEMALS direkt am Kabelbaum.**

| Verbraucher | Strom | Schaltung | Steuerung |
|---|---|---|---|
| Heizgriffe | 2–4A (Paar) | MOSFET (IRLZ44N) oder Relais | ESP32 GPIO → MOSFET-Gate |
| Zusatzscheinwerfer LED | 2–5A | MOSFET oder Relais | ESP32 GPIO → MOSFET-Gate |
| Zubehörsteckdose | variable | Relais | ESP32 GPIO → Transistor → Relais |

**Schaltungs-Prinzip (sicherer Weg)**:

```
ESP32 GPIO ──[100Ω]──► MOSFET Gate (IRLZ44N)
                         ├── Source → GND
                         └── Drain → Load (−)
Load (+) → 12V (geschaltet, via Zündungs-Relais)

oder:

ESP32 GPIO ──[510Ω]──► 2N2222A Base
                         ├── Emitter → GND
                         └── Collector → Relay Coil (−)
                         Relay Coil (+) → 12V
                         Flyback-Diode (1N4007) parallel zur Spule!
```

**Wichtige Schutzmaßnahmen**:
- Flyback-Diode (1N4007) bei jeder induktiven Last (Relais, Hupe) — sonst stirbt der MOSFET/Transistor am Spannungsspike
- 10kΩ Pull-down am Gate/Base — verhindert Einschalten während ESP32 bootet
- Logik-Level-MOSFET (IRLZ44N, nicht IRF520) — schaltet bei 3.3V Vgs voll durch
- TVS-Diode am 12V-Eingang — schützt vor Load-Dump-Spikes (Motorrad-Generator)
- Sicherung pro Kanal — brandgefährlich ohne

### 2.4 Motorsteuergerät (ECU)

Die ECU ist ein **Keihin RH850** (Teilenummer 38770-MLC-xxx). Tuning-Anbieter (OldSkullTuning, 2WDW, SSS Motorcycles) bieten ECU-Flashes an, die folgende Parameter ändern [4][5][6]:

- Fuel Maps (IAP + TPS, pro Zylinder)
- Zündtiming
- Drosselklappen-Mapping (TBW)
- Drehzahlbegrenzer / Speed-Limiter
- O2-Sensor-Deaktivierung
- Kühlerlüfter-Temparatur

⚠️⚠️ **ECU-Flash = nicht für Bordcomputer-Projekt.**

Gründe:
- ECU-Flash braucht professionelle Tools (KESS3 ~3000€, Bench-Modus)
- **Bricking-Risiko**: ECU ist codiert mit HISS-Immobilizer. Bei Fehler ist das ECU unbrauchbar, neues ECU kostet ~1300€+ und muss zu den Schlüsseln gepairt werden [7]
- **Garantieverlust**: Honda sieht den Reflash bei Service
- **Versicherung**: Modifizierte ECU = Erlöschen der Betriebserlaubnis bei Unfällen
- Kann nicht live/vom ESP32 aus gemacht werden — das ist ein Bench-Prozess

**Fazit**: ECU-Eingriff ist out of scope für dieses Projekt. Das Bordcomputer-Projekt liest nur Daten, es schreibt nichts ins Motorsteuergerät.

---

## 3. Sicherheitsrelevante Systeme

### 3.1 Übersicht der Rider-Aids

Die Transalp hat folgende elektronische Fahrerassistenzsysteme, alle über **Throttle-by-Wire (TBW)** und die BCU realisiert [2]:

| System | Stufen | Steuerung | CAN-Zugriff? |
|---|---|---|---|
| **Engine Power (EP)** | 4 Level | TBW-Throttle-Mapping | Lesbar: Modus-Status. Schreibbar: ⚠️ nur über originale HMI |
| **Engine Brake (EB)** | 3 Level | TBW-Schleppmoment | Lesbar: Modus-Status |
| **HSTC (Torque Control)** | 5 Level + Off | TBW + Raddrehzahlsensoren | Lesbar: Level/Status. Schreibbar: ⚠️ siehe unten |
| **Wheelie Control** | integriert in HSTC | TBW | Lesbar: Status |
| **ABS** | 2 Modi (On-Road / Off-Road) | ABS-Modulator | Lesbar: Modus. Schreibbar: ⚠️ siehe unten |
| **Rear ABS OFF** | Ja/Nein | ABS-Modulator (nur hinten) | Nur über originale HMI im USER-Modus |

### 3.2 ABS

**Was macht das ABS der Transalp?**
- 2-Kanal-ABS (vorne + hinten getrennt)
- Modus 1 = On-Road (beide Räder ABS)
- Modus 2 = Off-Road (hintere ABS abschaltbar im USER-Modus)
- ABS funktioniert erst ab 10 km/h, darunter passiv

**Kann ABS über CAN deaktiviert werden?**

Theoretisch ja — die BCU sendet ABS-Modus-Kommandos über den internen CAN-Bus. Praktisch:

⚠️ **CAN-Injection zum ABS deaktivieren = LEBENSGEFÄHRLICH.**

Folgende Gründe:
1. Die CAN-IDs für ABS-Modus-Umschaltung sind **nicht dokumentiert** und müssen per Sniffing reverse-engineered werden
2. Ein fehlerhafter CAN-Frame kann das ABS-Modul in einen **Fehlerzustand** versetzen — es schaltet dann komplett ab, auch vorne
3. Timing-kritisch: Die BCU sendet periodische Keepalive-frames. Ein dazwischenfunksendes externes Gerät kann die BCU verwirren
4. Es gibt **kommerzielle Dongles** (LSK Electronics [8]) die genau das machen — die stecken sich in den BCU-Kabelbaum und emulieren den Lenkerschalter, ohne CAN zu berühren. Das ist der sichere Weg.

**LSK Off-Road Dongle** [8]:
- Plug-and-Play Modul am BCU-Stecker
- Deaktiviert hintere ABS + HSTC per Knopfdruck
- Merkt sich den Zustand nach Zündung aus/an
- Funktioniert **ohne CAN-Injection** — es emuliert die Serien-Schalter-Signale

**Empfehlung für Bordcomputer-Projekt**: ABS-Status nur **lesen** (über OBD oder internen CAN). ABS-Modus-Umschaltung über die originale HMI oder einen fertigen Dongle. **Keine selbstgebaute CAN-Injection für ABS.**

### 3.3 HSTC (Honda Selectable Torque Control)

HSTC ist Hondas Traktionskontrolle, integriert über TBW. Es reduziert Drehmoment bei Raddrehzahl-Differenz (Schlupf). Wheelie Control ist Teil davon.

**CAN-Zugriff**:
- Lesen: HSTC-Level und On/Off-Status sind auf dem internen CAN-Bus sichtbar (wenn man ihn anzapft)
- Schreiben: ⚠️ Theoretisch über CAN-Injection möglich, aber **gleiche Risiken wie ABS**
- Der OBD-Port liefert **keine** HSTC-Daten

⚠️ **HSTC über CAN zu deaktivieren kann zum ungefilterten Drehmoment am Hinterrad führen.** Bei 67kW (91PS) ist das auf nasser Straße oder Schotter ein direkter Highside-Risiko.

### 3.4 Wheelie Control / Engine Brake Control

Beide sind in die TBW-Logik der ECU integriert. Die CAN-IDs sind **proprietär und undokumentiert**.

| System | CAN-ID bekannt? | Zugriff | Risiko |
|---|---|---|---|
| Wheelie Control | Nein | Nur über ECU-Flash | ⚠️⚠️ Hoch — Deaktivierung kann zu unkontrolliertem Wheelie führen |
| Engine Brake Control | Nein | Nur über ECU-Flash | ⚠️ Mittel — weniger Schleppmoment, aber nicht lebenskritisch |

⚠️ **Klare Empfehlung: Nicht eingreifen.** Beide Systeme sind TBW-gesteuert und in die ECU-Logik eingebettet. Ein Eingriff erfordert entweder ECU-Flash oder CAN-Injection auf dem internen Bus — beides ist für ein Bordcomputer-Projekt nicht akzeptabel.

### 3.5 Immobilizer (HISS)

Die Transalp hat Hondas HISS-System (HISS = Honda Ignition Security System). Der ECU ist mit den Schlüsseln gepairt.

⚠️⚠️⚠️ **NIEMALS den Immobilizer-Bereich manipulieren.**
- Verliert man alle Schlüssel ohne Key-Code-Karte → neues ECU + Schlüssel + Schlüsselbarder + Sitzschloss (Kosten: vierstellig) [7]
- HISS kommuniziert über K-Line mit dem ECU
- Jegliches Mitsniffen oder Injecten auf der K-Line während des Startvorgangs kann den ECU-Immobilizer-Status korrumpieren

---

## 4. Risikoabschätzung

### 4.1 Risikomatrix

| Aktion | Risiko | Rechtlich | Empfehlung |
|---|---|---|---|
| CAN-Daten **lesen** (Speed, RPM, Temp) | Kein Risiko | Erlaubt (Diagnose) | ✅ **OK — das ist die Kernfunktion** |
| Zusatzinstrumente parallel betreiben | Kein Risiko | Erlaubt | ✅ **OK** |
| Zubehör über Relais/MOSFET schalten | Gering (bei korrekter Verkabelung) | Erlaubt | ✅ **OK** |
| Heizgriffe über MOSFET | Gering | Erlaubt | ✅ **OK** |
| Zusatzscheinwerfer über Relais | Gering (Last absichern!) | Erlaubt mit Freigabe | ✅ **OK** |
| Blinker/Hupe über Relais parallel | Gering | Erlaubt | ✅ **OK, aber kann Auto-Cancel stören** |
| CAN-Bus mitsniffen (intern) | Mittel (Kabelbaum) | Erlaubt | ⚠️ **Nur wenn nötig** |
| ABS-Modus über CAN-Injection | ⚠️⚠️ Hoch | Erlaubt im Stand, fahrend lebensgefährlich | ❌ **NICHT TUN** |
| HSTC über CAN-Injection | ⚠️⚠️ Hoch | Erlaubt, aber gefährlich | ❌ **NICHT TUN** |
| ECU-Flash (Tuning) | ⚠️⚠️ Hoch (Bricking) | Erlöscht Betriebserlaubnis | ❌ **Out of scope** |
| Immobilizer manipulieren | ⚠️⚠️⚠️ Extrem | Strafbar | ❌ **NIEMALS** |
| Tacho-Spoofing | Hoch | Strafbar (§25 StVZO) | ❌ **NIEMALS** |

### 4.2 Was unbedenklich ist

- **Reiner Lesezugriff** auf OBD-CAN (Speed, RPM, Temperaturen, Kraftstoff)
- **Zusatzinstrumente** (ESP32-Display) parallel zum Originalen
- **Zubehör-Schaltung** über Relais/MOSFET (Heizgriffe, Zusatzlicht, USB-Ladegerät)
- **Crash-Detection** über eigene Sensoren (IMU, Beschleunigungssensor)
- **GPS-Tracking** und Telemetrie-Logging

### 4.3 Was kritisch ist

- **ABS/TC-Deaktivierung über CAN**: Lebensgefährlich bei fehlerhafter Implementation
- **ECU-Modifikation**: Bricking-Risiko, Garantieverlust, rechtliche Probleme
- **Tacho-Manipulation**: Strafbar
- **K-Line-Manipulation**: Kann Immobilizer stören → Fahrzeug startet nicht mehr

### 4.4 Rechtliche Aspekte (Deutschland)

| Bereich | Gesetz | Bemerkung |
|---|---|---|
| Tachomanipulation | §25 StVZO, §22 StVG | Strafbar, auch Versuch |
| Betriebserlaubnis | §19 StVZO | ECU-Modifikation kann Erlöschen bewirken |
| Gewährleistung | BGB §434 | Eingriff in ECU/elektrik → Gewährleistung erloschen |
| TÜV | §29 StVZO | Änderungen an Brems-/Lenksystem müssen eingetragen sein |
| Versicherung | VVG | Bei Unfall mit modifizierten Systemen: Leistungskürzung möglich |
| HU/Spionage | §202a StGB | Mitsniffen von CAN kann "Abfangen von Daten" sein (Graubereich bei Eigentum am Fahrzeug) |

**Praxis**: Lesen von CAN-Daten am eigenen Fahrzeug ist unbedenklich. Schreiben auf CAN-Busse, die sicherheitsrelevante Systeme steuern, ist fahrlässig bis gefährlich.

---

## 5. Relais-Schaltung als sicherer Weg

### 5.1 Warum Relais statt CAN-Injection?

| Kriterium | CAN-Injection | Relais/MOSFET |
|---|---|---|
| Komplexität | Hoch (Protokoll-RE, Timing) | Gering (Hardware) |
| Risiko für Fahrzeug | Hoch (BCU/ECU- Fehlerzustand) | Gering (galvanische Trennung) |
| Kosten | Nur CAN-Transceiver | ~2–5€ pro Kanal |
| Reverse-Engineering nötig | Ja (Wochen) | Nein |
| Garantie | Erlischt bei DTCs | Unberührt (nur Zubehör) |
| Zuverlässigkeit | Abhängig von SW | Hardware-only |

### 5.2 MOSFET-Schaltung für Motorrad (vibrationssicher)

**Empfohlene Bauteile**:

| Bauteil | Typ | Einsatz |
|---|---|---|
| MOSFET | IRLZ44N (TO-220) | High-Current Lasten (Licht, Heizgriffe) |
| MOSFET-Modul | DMP4015SK3Q-13 | 8-Kanal-Board (siehe Moto32-Projekt [9]) |
| BJT | 2N2222A (TO-92) | Relais-Spulen treiben |
| Flyback-Diode | 1N4007 | Über jede Relais-Spule |
| Gate-Widerstand | 100Ω | Gate → GPIO |
| Pull-down | 10kΩ | Gate → GND |
| TVS-Diode | SMAJ33A | 12V-Eingang (Load-Dump-Schutz) |
| Sicherung | 5A–10A (pro Kanal) | Lastabhängig |

### 5.3 Schaltungs-Topologie

**Low-Side MOSFET-Schalter** (empfohlen für DC-Lasten):

```
        +12V (Zündungs-geschaltet)
          │
          ├── Last (Heizgriff / LED / etc.)
          │
          └── Drain
              │
         ┌────┴────┐
         │  IRLZ44N│
         │  (N-MOS)│
         └────┬────┘
              │ Source
              │
             GND

GPIO ──[100Ω]── Gate
  │
  └──[10kΩ]── GND  (Pull-down: MOSFET OFF beim Booten)
```

**Relais-Schalter** (für galvanische Trennung, Hupe/Licht):

```
        +12V
          │
          ┌── Relay Spule ──┐
          │      (12V)       │
          │                  │
          └── Collector      │
               (2N2222A)     │
                    │        │
               Emitter       │  1N4007 (Flyback, Kathode am +12V)
                    │        │
                   GND     GND

GPIO ──[510Ω]── Base (2N2222A)
  │
  └──[10kΩ]── GND

Relais Kontakt:
  COM → +12V (direkt, eigener Zweig)
  NO  → Last → GND
```

### 5.4 Vibrationssicherheit

Motorrad-spezifisch:
- TO-220 MOSFETs **festverschrauben** mit Wärmeleitpaste + M3-Schraube auf Alu-Träger
- Relais: **vibrationsoptimierte Typen** verwenden (z.B. Omron G8HE-1C7, automotive-grade)
- Kabel: **crimpen, nicht löten** (Lötzinn bricht bei Vibration)
- Platinen: **konformal coating** (Klarlack) gegen Feuchtigkeit
- Stecker: **Wasserfeste Typen** (Molex MX150, AMP Superseal 1.0)
- Kabelbinder alle 5cm, Schrumpfschlauch über allen Verbindungen

### 5.5 ESP32-Ansteuerung

Pin-Belegung im Bordcomputer (Erweiterung des PINMAP):

| Funktion | GPIO | Schaltung |
|---|---|---|
| Heizgriffe Links | GPIO 15 | MOSFET (IRLZ44N) |
| Heizgriffe Rechts | GPIO 16 | MOSFET (IRLZ44N) |
| Zusatzscheinwerfer | GPIO 19 | Relais (über 2N2222A) |
| Zubehörsteckdose | GPIO 20 | Relais (über 2N2222A) |
| Status LED | GPIO 48 | Direkt (mit 330Ω Vorwiderstand) |

⚠️ GPIO 19/20 sind USB-Pins. Wenn USB-Debug nicht gebraucht wird nach der Entwicklung, können sie für Relais genutzt werden. Sonst GPIO 35/36 nehmen.

### 5.6 Software-Sicherheit

In der Firmware (ESP-IDF):

- **Watchdog** auf jeden Schaltkanal: Wenn Task hängt → Kanal aus (Fail-Safe)
- **Max-On-Timer**: Heizgriffe z.B. max 30min, dann Auto-Off (Gedächtnis-Reset)
- **Zündungs-Signal**: Nur schalten, wenn Zündung AN (Stromversorgung vom Bordnetz)
- **Soft-Start**: MOSFET per PWM hochfahren (100ms Fade-In), kein harter Stromstoß
- **Strom-Messung**: Shunt + ADC, bei Überstrom → Kanal sofort OFF + Fehler-Log

---

## 6. Klare Empfehlung: Was OK, was nicht

### ✅ OK — sicher umsetzbar

1. **CAN-Daten lesen** über OBD-Port (Speed, RPM, Temperaturen)
2. **Zusatzinstrumente** (E-Paper-Display) parallel betreiben
3. **Zubehör schalten** über MOSFET (Heizgriffe, Zusatzlicht)
4. **Hupe/Blinker** über Relais parallel zum Originalschalter
5. **Crash-Detection** über eigene IMU
6. **GPS-Tracking** und Telemetrie
7. **ABS/HSTC-Status lesen** (wenn internen CAN anzapft, nur read)

### ⚠️ Nur mit Vorsicht — bedingt umsetzbar

1. **Internen CAN-Bus anzapfen** (für Wheel Speed, Gear) — invasiv, aber nur Lesezugriff
2. **Zusatzscheinwerfer** — nur mit eigener Sicherung und korrekter Lastabsicherung
3. **Blinker parallel schalten** — kann Auto-Cancel-Funktion stören

### ❌ Nicht tun — lebensgefährlich oder strafbar

1. **ABS über CAN-Injection deaktivieren** — falsche CAN-Frame → ABS totalausfall
2. **HSTC über CAN-Injection deaktivieren** — unkontrollierter Schlupf → Highside
3. **ECU-Flash** vom Bordcomputer aus — Bricking-Risiko, Immobilizer
4. **Tacho-Spoofing** — strafbar (§25 StVZO)
5. **K-Line manipulieren** — kann HISS-Immobilizer stören
6. **Motorsteuergerät-Modifikation** — out of scope, rechtlich fragwürdig

---

## 7. Quellen

1. ADVrider Forum — Veridian Cruise Development Thread (XL750 CAN-Bus-Struktur):
   https://advrider.com/f/threads/veridian-cruise-for-the-transalp-xl750.1687083

2. Honda News — 2024 XL750 Transalp Features & Benefits (BCU, CAN, Rider-Aids):
   https://hondanews.com/en-US/powersports/releases/release-cc39196e84787a20ec3cb086fb119edc-2024-honda-xl750-transalp-features-benefits

3. AltTune — Honda Transalp ECU Tuning (KESS3, 6-poliger Diagnosestecker):
   https://alttune.com/tuning/honda-transalp-a1233/

4. OldSkullTuning — Tuning-File für Keihin RH850 (38770-MLC):
   https://oldskulltuning.com/honda-transalp-xl750-tuning-file-keihin-rh850

5. 2WDW — Honda Transalp Mail-In ECU Flashing Service:
   https://2wheeldynoworks.com/products/2wdw-honda-transalp-mail-in-ecu-flashing-service

6. SSS Motorcycles — XL750 Transalp ECU Reflash V2:
   https://sssmotorcycles.com/products/honda-xl750-transalp-ecu-reflash

7. Transalp Owners Forum — ECU Tuning Diskussion (Bricking, HISS, Kosten):
   https://www.transalpowners.com/threads/transalp-ecu-tuning.483/

8. LSK Electronics — XL750 Transalp Off-Road Dongle:
   https://www.lskelectronics.com/xl750-transalp-offroad

9. Moto32 — Open Source Motorcycle Control Unit (8-Kanal MOSFET, ESP32-S3):
   https://moto32.vercel.app

10. Honda Owners Manual — XL750 Transalp 2025 (ABS, HSTC, Riding Modes):
    https://webom.hondamotopub.com/webom/HMEE/MLC250/html/GOG024001.html

11. GitHub — andreibaw/Honda_K-Line_KWP2000 (K-Line Interface ESP32):
    https://github.com/andreibaw/Honda_K-Line_KWP2000

12. GitHub — sophienyaa/Honda-Motorcycle-ECU-Tools (Diagnoseport, K-Line):
    https://github.com/sophienyaa/Honda-Motorcycle-ECU-Tools

13. Electrical Flux — Transistor as Switch for Relays (ESP32, Berechnung):
    https://electricalflux.com/wire-switches/transistor-as-switch-home-automation-relays

14. Honda Tuning Suite Wiki — K-Line Support (Honda ECU ROMs):
    https://wiki.hondatuningsuite.com/en/OBD2/K-Line

15. CS Electronics — CAN Bus Sniffer & Reverse Engineering:
    https://www.csselectronics.com/pages/can-bus-sniffer-reverse-engineering

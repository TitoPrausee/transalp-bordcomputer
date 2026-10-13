# PD06 Transalp 600 — Technische Daten, Service-Manual, Community-Wissen

Fahrzeug: Honda XL600V Transalp 600, Baujahre 1986–1999.
Modellcode: PD06 (1987–1996) bzw. PD10 (1997–1999, Produktion nach Italien verlegt).
Dieses Dokument ist die technische Datenbasis für das Bordcomputer-Projekt.

Quellen sind als Inline-Links gesetzt; Service-Manual-Quellen am Ende gesammelt.

---

## 1. Modellidentifikation und Baujahre

Die Transalp 600 lief von 1987 bis 1999 in mehreren Varianten. Der Modellcode PD06 gilt für alle 1987–1996 gebauten 600er; ab 1997 wechselt der Code auf PD10 (Produktion in Italien, Atessa-Werk). Der Motorcode ist durchgehend PD06E.

| Modell | Baujahr | Frame-No. ab | Engine-No. ab | Code |
|--------|---------|-------------|-------------|------|
| XL600V-H | 1987 | PD06-50 | PD06E-50 | PD06 |
| XL600V-J | 1988 | PD06-51 | PD06E-51 | PD06 |
| XL600V-K | 1989 | PD06-52 | PD06E-22 | PD06 |
| XL600V-L | 1990 | PD06-53 | PD06E-23 | PD06 |
| XL600V-M | 1991 | PD06-54 | PD06E-24 | PD06 |
| XL600V-N | 1992 | PD06-55 | PD06E-25 | PD06 |
| XL600V-P | 1993 | PD06-56 | PD06E-26 | PD06 |
| XL600V-R | 1994/95 | PD06-57 | PD06E-27 | PD06 |
| XL600V-T | 1996 | PD06A-T | PD06E-28 | PD06 |
| XL600V-V | 1997 | ZDCPD10A0-V | HM-PD06E-29 | PD10 |
| XL600V-W | 1998 | ZDCPD10A0-W | HM-PD06E-40 | PD10 |
| XL600V-X | 1999 | ZDCPD10A0-X | HM-PD06E-41 | PD10 |

Quelle: [motoinst.ru](https://motoinst.ru/en/Honda/Africa-Twin/XRV750/owner/intro/114984)

Die Frame-Nummer ist in die rechte Seite des Lenkkopfs gestanzt. Die Motornummer steht auf der rechten Seite des Kurbelgehäuses. Ein Farbcode-Label sitzt oben auf der Rahmen-Hinterbau-Konstruktion (sichtbar nach Abnahme der Sitzbank).

Es gibt **kein** PD07-Modell bei der Transalp — PD07 ist der Africa Twin (XRV750). Die Verwechslung kommt oft vor. Die Transalp-Reihe ist: PD06 (600, 1987–1996), PD10 (600, 1997–1999), RD10 (650, 2000–2002), RD11 (650, 2002–2006).

### Eingriffsvarianten (restricted models)

Für den deutschen Markt (Honda Area Code "G/27P") und die Schweiz (SW) wurden gedrosselte Modelle mit reduzierter Leistung verkauft. Die Drosselung erfolgte über geänderte Ansaugstutzen (Inlet Rubbers) und teilweise andere Nockenwellen. Bei 1987er Modellen waren alle CDI-Units gleich, nur die Ansaugstutzen unterschieden sich. Ab 1991 gab es auch andere Nockenwellen bei den gedrosselten Versionen.

Standard-Ansaugstutzen: 16211-MM9-000 (frühe Modelle) bzw. 16211-MS8-000.
Gedrosselt (Deutschland): 16211-MS6-710.
Gedrosselt (Schweiz): 16211-MS6-930.

Quelle: [xrv.org.uk — Restriction Details 1987-1990](https://xrv.org.uk/threads/transalp-xl600-engine-restriction-details-1987-1990.69904), [xrv.org.uk — Restriction Details 1991-1999](https://xrv.org.uk/threads/transalp-xl600-engine-restriction-details-1991-1999.69912)

---

## 2. Motor- und Getriebedaten

### Motor

| Parameter | Wert |
|-----------|------|
| Bauart | Flüssigkeitsgekühlter 4-Takt V2, 52° Zylinderwinkel |
| Ventilsteuerung | SOHC, 3 Ventile pro Zylinder, kettengetrieben |
| Zündung | Doppelzündung (twin spark), 2 Zündkerzen pro Zylinder |
| Hub | 66,0 mm |
| Bohrung | 75,0 mm |
| Hubraum | 583,2 cm³ |
| Verdichtung | 9,2 : 1 |
| Ventilspiel (kalt) | Einlass 0,15 mm, Auslass 0,20 mm (V-H: 0,10/0,10 mm) |
| Leerlauf | 1.300 ± 100 U/min |
| Motor-Trockengewicht | 59,5 kg |

Quelle: Honda Service Manual (Oktober 1988), [gummycarbs.com](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V), [bikeswiki.com](https://bikeswiki.com/Honda_XL600V_Transalp)

### Leistung und Drehmoment

Die Leistungsangaben variieren leicht je nach Quelle und Markt. Die Honda-Werksangaben sind 50 PS für die europäischen späteren Modelle; 55 PS wurden für frühe Modelle (1987–1990) und den japanischen Markt gelistet.

| Modelljahre | Leistung | Drehmoment |
|-------------|---------|-----------|
| 1987–1990 (V-H bis V-L) | 40 kW (55 PS) @ 8.000 U/min | 54 Nm @ 6.000 U/min |
| 1987 (Japan) | 38,2 kW (52 PS) @ 8.000 U/min | 53 Nm @ 6.000 U/min |
| 1991–1999 (V-M bis V-X) | 37 kW (50 PS) @ 8.000 U/min | 52–53 Nm @ 6.000 U/min |
| Deutschland (gedrosselt) | 37 kW (50 PS) @ 8.000 U/min | ~51 Nm @ 5.964 U/min |

Quelle: [torquepedia.com](https://torquepedia.com/honda/transalp/1987/xl600v), [autoevolution.com](https://www.autoevolution.com/moto/honda-xl-600-v-transalp-1987.html), [motorcyclespecs.co.za](https://motorcyclespecs.co.za/model/Honda/honda_xl600v_90.htm), [de.zxc.wiki](https://de.zxc.wiki/wiki/Honda_Transalp)

Es gibt keinen elektronischen Drehzahlbegrenzer im modernen Sinne. Die CDI-Zündung hat eine mechanische/elektronische Verstellung, die bei extrem hohen Drehzahlen den Zündfunken unterbricht, aber der rote Bereich beginnt bei ca. 9.000 U/min, die Nennleistung ist bei 8.000 erreicht. Praktisch wird der Motor vorher leistungsflach.

### Zündung

| Parameter | Wert |
|-----------|------|
| Zündsystem | CDI mit elektronischer Vorverstellung (V-H bis V-R), Digital/transistorisiert (V-T bis V-X) |
| Zündzeitpunkt | 10° v. OT im Leerlauf, 30° v. OT bei 4.500 U/min (Volllast) |
| Zündreihenfolge | 1 (vorne) – 2 (hinten), Zylinder 1 vorne rechts |
| Zündkerzen | NGK DPR8EA-9 (Standard), DPR9EA-9 (hoch), DPR7EA-9 (kalt) |
| Zündkerzenabstand | 0,8–0,9 mm |
| Pickup (Pulsgenerator) | 1 Puls pro Kurbelwellenumdrehung (ein einzelner Pickup-Sensor für beide Zylinder bei V-T bis V-X; zwei separate Pickup-Spulen bei V-H bis V-R) |

Quelle: [bikeswiki.com](https://bikeswiki.com/Honda_XL600V_Transalp), [gummycarbs.com](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V), [cdi-shop.de](https://cdi-shop.de/en/technical-specifications)

### Vergaser

| Parameter | Wert |
|-----------|------|
| Typ | 2× Keihin CV (Konstantvakuummischer) |
| Drosselklappendurchmesser | 32 mm (V-H bis V-R), 34 mm (V-T bis V-X) |
| Hauptdüse vorne | 128 (V-H bis V-R), 118 (V-T), 115 (V-V bis V-X) |
| Hauptdüse hinten | 130 (V-H bis V-R), 120 (V-T), 118 (V-V bis V-X) |
| Nadeldüse | 38 |
| Leerlaufschraube | 2,5 U (V-H/J), 2,25 U (V-K bis V-P), 1,75 U (V-R bis V-T), 2,25 U (V-V bis V-X) |
| Schwimmerhöhe | 7,0 mm |

Quelle: [xrv.org.uk](https://www.xrv.org.uk/threads/transalp-xl600-engine-restriction-details-1987-1990.69904)

### Schmierung

| Parameter | Wert |
|-----------|------|
| System | Zwangsumlauf mit Nasssumpf (wet sump) |
| Ölpumpe | Trochoid-Doppelrotor |
| Ölfüllmenge (Ölwechsel) | 2,10 Liter |
| Ölfüllmenge (Filterwechsel) | 2,30 Liter |
| Ölfüllmenge (komplette Zerlegung) | 2,90 Liter |
| Empfohlenes Öl | Honda GN4 10W-40, API SE/SF/SG |
| Öldruck (warm, 80°C) | 4,5 bar (64 psi) @ 6.000 U/min |
| Öldruckwarnleuchte | Schaltet bei ca. 0,2–0,4 bar ab |
| Ölfilter | Honda 15410-MM9-P03 (mit Africa Twin, NT650 etc. kompatibel) |
| Ölpumpenrotor-Spiel | Tipp: 0,15 mm, Körper: 0,15–0,22 mm, Axial: 0,02–0,07 mm |

Quelle: [moto4.ru Service Manual PDF](https://www.moto4.ru/downloads/Service_Repare_Manual_Honda_XL600_650V_Transalp_XRV750_AfricaTwin_87_07_st_font_fin.pdf), [gummycarbs.com](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V), [xrv.org.uk](http://xrv.org.uk/forums/transalp-technical-specs/39889-honda-xl600v-technical-specifications.html)

### Kühlung

| Parameter | Wert |
|-----------|------|
| System | Flüssigkeitskühlung (Wasser) |
| Kühlmittelfüllmenge | ca. 2,0 Liter |
| Kühlerdeckel-Öffnungsdruck | 0,9–1,25 bar (13–18 psi) |
| Thermostat öffnet bei | 80–84°C |
| Thermostat voll offen bei | 95°C |
| Thermostat-Hub (min.) | 8,0 mm |
| Kühlmittel | 50% Glykol / 50% destilliertes Wasser |
| Kühler | 1× Hauptschleichkühler vorne, mit Lüfter |

Quelle: [xrv.org.uk](http://xrv.org.uk/forums/transalp-technical-specs/39889-honda-xl600v-technical-specifications.html)

### Getriebe und Übersetzung

| Parameter | Wert (V-H bis V-R) | Wert (V-T bis V-X) |
|-----------|---------------------|---------------------|
| Primärübersetzung | 1,888 (68/36) | 1,888 (68/36) |
| Sekundärübersetzung (Kette) | 3,133 (47/15) | 3,133 (47/15) |
| 1. Gang | 2,571 (36/14) | 2,500 |
| 2. Gang | 1,777 (32/18) | 1,722 |
| 3. Gang | 1,380 (29/21) | 1,333 |
| 4. Gang | 1,125 (27/24) | 1,111 |
| 5. Gang | 0,961 (25/26) | 0,961 |
| Schaltung | Links, Rücktritt, 1-N-2-3-4-5 | |
| Kupplung | Mehrscheiben-Ölbadkupplung, Seilzug | |
| Kupplungshebel-Spiel | 10–20 mm | |
| Kupplungsfederfreilänge | 39,0 mm (Verschleißgrenze 37,4 mm) | |
| Kette | DID 525V8 / RK 525SMO, 118 Glieder (V-H bis V-R) | DID 525HV8, 118 Glieder (V-T bis V-W) |
| Kettenblatt vorne | 15 Zähne (Honda 23801-MM9-010) | |
| Kettenblatt hinten | 47 Zähne | |
| Kettenspiel | 35–45 mm | |

Quelle: [gummycarbs.com](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V), [bikeswiki.com](https://bikeswiki.com/Honda_XL600V_Transalp), [xrv.org.uk](http://xrv.org.uk/forums/transalp-technical-specs/39889-honda-xl600v-technical-specifications.html)

### Übersetzungstabelle: km/h pro 1000 U/min pro Gang

Berechnet aus: Primär × Sekundär × Gangübersetzung × Reifenumfang.
Reifen hinten: 130/80-17, Durchmesser 639,8 mm, Umfang 2,010 m.
Reifen vorne: 90/90-21, Durchmesser 695,4 mm, Umfang 2,185 m.

#### Frühe Modelle (1987–1995, V-H bis V-R)

| Gang | Gangübersetzung | Gesamtübersetzung | km/h pro 1000 rpm | km/h @ 8000 rpm |
|------|-----------------|-------------------|-------------------|-----------------|
| 1 | 2,571 | 15,208 | 7,93 | 63,4 |
| 2 | 1,777 | 10,511 | 11,47 | 91,8 |
| 3 | 1,380 | 8,163 | 14,77 | 118,2 |
| 4 | 1,125 | 6,654 | 18,12 | 145,0 |
| 5 | 0,961 | 5,684 | 21,22 | 169,7 |

#### Späte Modelle (1996–1999, V-T bis V-X)

| Gang | Gangübersetzung | Gesamtübersetzung | km/h pro 1000 rpm | km/h @ 8000 rpm |
|------|-----------------|-------------------|-------------------|-----------------|
| 1 | 2,500 | 14,788 | 8,16 | 65,2 |
| 2 | 1,722 | 10,186 | 11,84 | 94,7 |
| 3 | 1,333 | 7,885 | 15,30 | 122,4 |
| 4 | 1,111 | 6,572 | 18,35 | 146,8 |
| 5 | 0,961 | 5,684 | 21,22 | 169,7 |

Berechnungsformel: v = (n_mot / (i_prim × i_final × i_gang)) × U_reifen × 60 / 1000

wobei n_mot = Motordrehzahl [U/min], i = Übersetzungsverhältnisse, U_reifen = Reifenumfang [m].

Für die Ganganzeige-Berechnung im Bordcomputer: Wenn die Motordrehzahl (vom CDI-Pickup oder Zündspulensignal) und die Geschwindigkeit (vom Tacho-Sensor am Vorderrad) bekannt sind, kann der Gang durch Vergleich der ratio v/n mit der Tabelle oben ermittelt werden. Der 5. Gang ist bei der frühen Getriebeversion kaum vom 4. Gang zu unterscheiden (Überholgetriebe, ratio nahe 1,0).

---

## 3. Elektrik

### Lichtmaschine (Alternator)

Die Transalp hat eine Drehstrom-Lichtmaschine (three-phase alternator) mit separatem Regler/Gleichrichter.

| Parameter | Wert |
|-----------|------|
| Typ | Drehstromgenerator (AC generator) |
| Leistung | 310 W bei 5.000 U/min (V-H/J, V-R bis V-X) |
| Leistung | 350 W bei 5.000 U/min (V-K bis V-P, 1989–1993) |
| Statorspulen-Widerstand | 0,1–1,0 Ohm |

Quelle: [motoinst.ru](https://motoinst.ru/en/Honda/Africa-Twin/XRV750/electric/equipment/115153), [bikeswiki.com](https://bikeswiki.com/Honda_XL600V_Transalp)

### Regler/Gleichrichter (Regulator/Rectifier)

| Parameter | Wert |
|-----------|------|
| Typ | Shunt-Typ (OEM), separate Einheit |
| Ausgangsspannung | 13,5–15,5 V bei 5.000 U/min |
| OEM-Hersteller | Shindengen (Typ SH538D-13) |
| Bekanntes Problem | Überhitzung, Steckverbinder-Korrosion (3 gelbe Drähte) |
| Nachrüstung | MOSFET-Typ FH008/FH020AA (CBR600RR), SH775/SH847 |

Quelle: [motoinst.ru](https://motoinst.ru/en/Honda/Africa-Twin/XRV750/electric/equipment/115153), [gummycarbs.com](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V), ADV Rider Foren

### Batterie

| Parameter | Wert |
|-----------|------|
| Typ | 12V Blei-Säure (Standard) |
| Kapazität | 12 Ah |
| OEM-Type | YB12A-B |
| Ersatz (versiegelt) | YTX14AH-BS (Isolierhülle entfernen) |
| Voll geladen | 13,0–13,2 V |
| Entladen | unter 12,3 V |
| Säuredichte voll | 1,280 |
| Laderate normal | 1,2 A für 5–10 Std |
| Laderate schnell | 4,0 A für 1 Std |
| Stromleckage max | 0,1 mA |

Quelle: [motoinst.ru](https://motoinst.ru/en/Honda/Africa-Twin/XRV750/electric/equipment), [gummycarbs.com](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V)

### Verbraucher

| Verbraucher | Leistung |
|-------------|----------|
| Scheinwerfer (Fernlicht) | 60W |
| Scheinwerfer (Abblendlicht) | 55W |
| Standlicht | 4W |
| Rück-/Bremslicht | 5/21W |
| Blinker (je Stück) | 21W |
| Tacho-Beleuchtung | 1,7W × 3 |
| Leerlaufanzeige | 3,4W |
| Fernlicht-Kontrollleuchte | 1,7W |
| Blinker-Kontrollleuchte | 3,4W × 2 |
| Ölwarnleuchte | 3,4W (1,7W laut xrv.org.uk) |

Quelle: [gummycarbs.com](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V), [xrv.org.uk](http://xrv.org.uk/forums/transalp-technical-specs/39889-honda-xl600v-technical-specifications.html)

### Strombudget für Zubehör

Bei 310W Generatorleistung und ca. 5.000 U/min:

| Posten | Verbrauch |
|--------|----------|
| Abblendlicht | 55W |
| Rücklicht | 5W |
| Standlicht | 4W |
| Instrumente | ~10W |
| **Summe Serienausstattung** | ~74W |
| **Verfügbar für Zubehör** | ~236W @ 5000 rpm |

Bei Leerlauf (~1.300 U/min) sinkt die Generatorleistung auf ca. 80W. Serienmäßig wird die Batterie im Leerlauf entladen. Für das Bordcomputer-Projekt bedeutet das: Im Stand- und Niedrig-Drehzahl-Bereich muss mit Batterie-Entladung gerechnet werden. GPS, Display und Controller sollten unter 30W Verbrauch haben, um im Fahrbetrieb (ab ca. 3.000 U/min) problemlos versorgt zu sein.

### Sicherungen

| Sicherung | Wert |
|------------|------|
| Hauptsicherung | 30A |
| Teilsicherungen (UK) | 3× 10A, 1× 15A |
| Teilsicherungen (DE) | 4× 10A, 2× 15A |

Quelle: [gummycarbs.com](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V), [xrv.org.uk](http://xrv.org.uk/forums/transalp-technical-specs/39889-honda-xl600v-technical-specifications.html)

### CDI-System und Pickup-Signal

Die Transalp hat zwei CDI-Einheiten (eine pro Zylinder), die unter der Sitzbank in einer Gummihalterung sitzen. Das ist ein bekannter Schwachpunkt: Durch die Belastung durch das Sitzpolster können die internen Lötstellen brechen.

**CDI-Pinbelegung** (nach cdi-shop.de für PD06):

| Pin | Funktion |
|-----|----------|
| 1 | +12V (über Sicherung, Zündschalter, Kill-Switch) |
| 2 | Seitenständer-Überwachung |
| 3 | Zündspule 1 |
| 4 | Zündspule 2 (nur bei 2-Zylinder-CDI) |
| 5 | Pulsgenerator (Pickup) |
| 6 | Masse (Batterie-/Rahmen) |

Ein gelber Draht-Jumper auf der CDI verbindet Pin 2 und 6 bei Modellen ohne Seitenständer-Überwachung (1987). Für Modelle mit Überwachung muss der Jumper durchtrennt werden.

Quelle: [cdi-shop.de](https://cdi-shop.de/en/technical-specifications)

**Pickup-Signal für Drehzahlmessung**: Der Pulsgenerator liefert 1 Puls pro Kurbelwellenumdrehung. Bei der Zündung mit zwei separaten CDI-Units (V-H bis V-R) gibt es zwei Pickup-Spulen. Bei den transistorisierten Modellen (V-T bis V-X) gibt es eine einzelne Pickup-Spule. Für die Drehzahlmessung im Bordcomputer kann das Signal am Pickup-Kabel oder an der Zündspule (Negative-Seite, Coil-) abgegriffen werden. Die Frequenz entspricht der Motordrehzahl geteilt durch 60 (1 Puls/Umdrehung → f = n/60).

### Kabelbaum-Farbcodes (Honda Standard)

Honda verwendet eine konsistente Farbcode-Tabelle im Kabelbaum. Für die Sensor-Anzapfung am Bordcomputer sind folgende relevant:

| Farbe | Funktion |
|-------|----------|
| Schwarz (Black) | Geschaltet +12V (Zündschalter) |
| Schwarz/Weiß (Black/White) | Kill-Switch, Zündsystem |
| Blau/Weiß (Blue/White) | Geschaltet +12V (Starter-Schalter) |
| Braun (Brown) | Rücklicht, Instrumentenbeleuchtung |
| Braun/Weiß (Brown/White) | Zündschalter, Lauflicht |
| Braun/Schwarz (Brown/Black) | Gesichert (Sicherungsbox) |
| Grün (Green) | Masse (Rahmen) |
| Gelb (Yellow) | Lichtmaschine → Regler (3× gelb) |
| Gelb/Weiß (Yellow/White) | Zündspule |
| Blau (Blue) | Fernlicht |
| Weiß (White) | Abblendlicht |
| Hellgrün (Light Green) | Leerlaufschalter |
| Hellgrün/Rot (Light Green/Red) | Ölwarnschalter |
| Orange (Orange) | Blinker rechts |
| Hellblau (Light Blue) | Blinker links |
| Schwarz/Braun (Black/Brown) | Batterie + (direkt) |

Quelle: [Scribd — Honda Wiring Color Chart](https://www.scribd.com/document/1001028426/Honda-Motorcycle-Electrical-Wiring-Color-Chart), [gummycarbs.com](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V)

Hinweis: Die Transalp hat serienmäßig keinen Tankgeber mit Widerstandssignal. Die Kraftstoffanzeige funktioniert über eine Reserve-Petcock-Membran und eine Warnleuchte. Für eine elektronische Tankstandsanzeige muss ein aftermarket-Sender im Tank nachgerüstet werden (siehe Abschnitt 7).

---

## 4. Fahrwerk und Räder

### Abmessungen

| Parameter | Wert |
|-----------|------|
| Gesamtlänge | 2.265–2.280 mm (je nach Modelljahr) |
| Gesamtbreite | 865 mm (V-H bis V-P), 905 mm (V-R bis V-X) |
| Gesamthöhe | 1.275–1.310 mm (je nach Modelljahr) |
| Radstand | 1.505 mm |
| Sitzhöhe | 850 mm |
| Bodenfreiheit | 225 mm (V-H/J), 200 mm (V-K/L), 195 mm (V-M bis V-X) |
| Lenkkopfwinkel (Caster) | 28° |
| Nachlauf (Trail) | 108 mm |
| Min. Wendekreis | 2,4 m |

### Gewichte

| Parameter | Wert |
|-----------|------|
| Trockengewicht | 175 kg (V-H bis V-L), 183 kg (V-M bis V-R), 189 kg (V-T bis V-X) |
| Fahrfertig (Kerb) | 194 kg (V-H bis V-L), 202 kg (V-M bis V-X) |

### Bereifung

| Position | Größe |
|----------|-------|
| Vorne | 90/90-21 54S |
| Hinten | 130/80-17 65S (oder 120/90-17 64S je nach Modell) |

Reifenumfang berechnet:
- Vorne (90/90-21): Durchmesser 695,4 mm, Umfang 2.185 m
- Hinten (130/80-17): Durchmesser 639,8 mm, Umfang 2.010 m

Bei 100 km/h dreht das Vorderrad mit ca. 763 U/min (12,7 Hz). Dieser Wert ist wichtig für die Kalibrierung eines elektronischen Tacho-Sensors am Vorderrad.

### Bremsen

| Modell | Vorne | Hinten |
|--------|-------|--------|
| V-H bis V-T | Scheibe 276 mm, 2-Kolben-Schwimmsattel | Trommel 130 mm |
| V-V bis V-X | Doppelscheibe 256 mm, 2-Kolben-Brembo | Scheibe 240 mm, 1-Kolben |

### Fahrwerk

| Parameter | Wert |
|-----------|------|
| Gabel | Teleskopgabel 41 mm, 200 mm Federweg |
| Federbein hinten | Pro-Link Monoshock, 187 mm Federweg, Vorspannung einstellbar |
| Gabelöl | 5W, 549 cm³ pro Gabelholme |
| Gabelölstand | 125 mm |
| Gabelfederfreilänge | 571,5 mm (V-H bis V-R), 612 mm (V-T bis V-X) |

Quelle: [bikeswiki.com](https://bikeswiki.com/Honda_XL600V_Transalp), [gummycarbs.com](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V), [xrv.org.uk](http://xrv.org.uk/forums/transalp-technical-specs/39889-honda-xl600v-technical-specifications.html)

---

## 5. Tank und Kraftstoff

| Parameter | Wert |
|-----------|------|
| Tankvolumen | 18,0 Liter |
| Reserve | 2,9–3,5 Liter |
| Verbrauch (60 km/h konstant) | ca. 2,8 L/100 km (36 km/L) |
| Verbrauch (Durchschnitt) | ca. 4,5–5,5 L/100 km |
| Reichweite (praktisch) | ca. 300–350 km |
| Höchstgeschwindigkeit | 163–177 km/h (je nach Quelle) |
| Tankgeber | Serienmäßig keine Widerstandssonde, sondern Vakuum-Petcock mit Reserve-Stellung |

Quelle: [bikeswiki.com](https://bikeswiki.com/Honda_XL600V_Transalp), [gummycarbs.com](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V)

---

## 6. Service-Manual und Wartungsdaten

### Verfügbare Service-Manuals

| Manual | Typ | Link |
|--------|-----|------|
| Honda Original Service Manual (P/N 61MM900) | Werkstatthandbuch, 1986–2001 | [repairloader.com](https://repairloader.com/manual.php/8cacd4) |
| Honda Service Manual PDF (moto4.ru) | Vollständiges PDF | [moto4.ru PDF](https://www.moto4.ru/downloads/Service_Repare_Manual_Honda_XL600_650V_Transalp_XRV750_AfricaTwin_87_07_st_font_fin.pdf) |
| Honda Fahrerhandbuch (Owner's Manual) | Betriebsanleitung | [honda.de PDF](https://www.honda.de/content/dam/local/germany/motorcycles/pdf/service/fahrerhandbuecher/XL600V_Transalp_37MAW600.pdf) |
| Haynes Repair Manual | aftermarket, XL600/650V + XRV750 | [haynes.com](https://us.haynes.com/products/honda-xl600-650v-transalp-xvr750-africa-twin-xl600v-xl650v-87-07-xrv750-90-03-haynes-repair-manual) |
| Bucheli Reparaturanleitung Nr. 5095/5096 | Deutsch, XL600V | ISBN 3-7168-1790-2 |
| Scribd Service Manual | scan of Honda original | [scribd.com](https://www.scribd.com/document/607260821/Honda-600-Transalp-86-01-Service-Manual) |
| 4-stroke.net Workshop Manual | Text-only OCR version | [4-stroke.net](https://4-stroke.net/documents/workshopmanual_650v_transalp_en_1987-2002_21062017-1025-ocr_text.txt) |

### Wartungsintervalle

| Wartung | Intervall |
|---------|-----------|
| Ölwechsel | alle 6.000 km oder jährlich |
| Ölfilterwechsel | alle 12.000 km |
| Ventilspiel prüfen | alle 12.000 km (Einlass 0,15 mm, Auslass 0,20 mm kalt) |
| Zündkerzen | alle 12.000 km |
| Kühlmittel | alle 2 Jahre |
| Bremsflüssigkeit (DOT 4) | alle 2 Jahre |
| Luftfilter | alle 12.000 km, bei Staubbelastung häufiger |
| Kettenspiel | 35–45 mm, regelmäßig nachspannen |
| Gabelöl | alle 20.000 km oder bei Undichtigkeit |

### Zylinderkompression

| Parameter | Wert |
|-----------|------|
| Kompressionsdruck | 1.177 kPa (12,0 kg/cm², 171 psi) |
| Messung | bei betriebswarmem Motor, Vollgas, Anlasser |

### Ventilsteuerung

| Parameter | Wert |
|-----------|------|
| Einlass öffnet | 10° v. OT |
| Einlass schließt | 40° nach UT |
| Auslass öffnet | 40° vor UT |
| Auslass schließt | 10° nach OT |

### Anzugsdrehmomente (Auswahl)

| Bauteil | Drehmoment |
|---------|-----------|
| Ölablassschraube | 34 Nm |
| Ölfilter | 9,8 Nm |
| Ventilspiel-Kontermutter | 23 Nm |
| Zündkerzen | 14 Nm |
| Kettenrad-Schrauben (hinten) | 45 Nm |
| Vorderradachse | 64 Nm |
| Hinterradachse | 95 Nm |
| Bremssattel-Befestigung | 33 Nm |
| Bremschlauch-Hohlschraube | 34 Nm |
| Gabelfußschraube (unten) | 33 Nm |
| Gabelbrücke oben | 27 Nm |
| Stoßdämpfer-Befestigung | 44 Nm |
| Schwingenachse | 110 Nm |
| Lenkkopfmuttern | 100–105 Nm |
| Motor-Schrauben M10 | 34 Nm |
| Motor-Schrauben M12 | 54 Nm |

Quelle: [xrv.org.uk](http://xrv.org.uk/forums/transalp-technical-specs/39889-honda-xl600v-technical-specifications.html), [gummycarbs.com](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V), [repairloader.com](https://repairloader.com/manual.php/8cacd4)

---

## 7. Sensor-Einbaupunkte für Bordcomputer-Projekt

### Öldruck

| Parameter | Wert |
|-----------|------|
| Öldruckschalter Position | Motorblock, rechte Seite, unterhalb des Ölfilters |
| Gewinde | M10 × 1,0 (metrisches Feingewinde) |
| Anschluss | 1× Draht (Hellgrün/Rot), Masse über Gehäuse |
| Warnschalter-Schaltpunkt | ca. 0,2–0,4 bar (Schließer, öffnet bei Druck) |
| Öldruck (warm) | 4,5 bar @ 6.000 U/min bei 80°C Öltemperatur |

Für eine Öldruck-Anzeige mit analogen Wert muss der Serien-Schalter durch einen T-Stück-Adapter ersetzt werden, um sowohl den Warnschalter als auch einen Drucksensor (z.B. VDO, AutoMeter oder AEM) zu behalten. Der Gewindeanschluss am Motorblock ist M10 × 1,0. Ein Adapter von M10 auf 1/8 NPT ist erforderlich. Alternative: ein Öldruck-Testport am Motorblock (nicht bei allen Modelljahren vorhanden).

### Kühlmitteltemperatur

| Parameter | Wert |
|-----------|------|
| Thermostat | öffnet 80–84°C, voll offen 95°C |
| Kühlmitteltemperatur-Sensor | Serienmäßig kein elektronischer Sensor; nur Thermostat |
| Einbaupunkt für Aftermarket-Sensor | Kühlschlauch-Inline-Sensor (O-Ring-Schelle), am oberen Kühlschlauch |

Die Transalp hat serienmäßig keine elektronische Kühlmitteltemperatur-Anzeige, nur eine Warnlampe (bei einigen Modellen). Für eine Temperaturanzeige muss ein Inlinesensor in den oberen Kühlschlauch (vom Motor zum Kühler) eingesetzt werden. Gängige Grössen: Innendurchmesser des Schlauchs messen (meist 32–36 mm), dann passenden Inline-Adapter kaufen (z.B. von Trail Tech, Koso oder VDO).

### Drehzahl (RPM)

| Parameter | Wert |
|-----------|------|
| Signalquelle | CDI-Pulsgenerator (Pickup) oder Zündspule |
| Pickup-Kabel | Gelb oder Blau (je nach Zylinder), siehe Kabelbaum-Farbcodes |
| Frequenz | 1 Puls pro Kurbelwellenumdrehung (f = n_mot / 60) |
| Alternativ | Tachosignal (der mechanische Tacho hat intern keine elektrische Ausgabe) |

Für die Drehzahlmessung im Bordcomputer kann das Pickup-Signal direkt am Pin 5 der CDI-Box abgegriffen werden (mit Optokoppler oder kondensativer Kopplung, um die CDI nicht zu belasten). Alternativ das Signal an der Minusseite der Zündspule (Coil-) mit einem Null-Current-Pickup oder induktivem Klammer sensor.

### Geschwindigkeit (Tacho)

| Parameter | Wert |
|-----------|------|
| Tacho-Antrieb | Mechanisch (Tachowelle), vom Vorderrad |
| Antrieb am Vorderrad | Tachoantrieb im linken Radlager, Antriebswelle mit Vierkant |
| Übersetzung Tacho | ca. 1:1 zum Vorderrad (direkt vom Rad, kein Getriebe) |
| Radumdrehung bei 100 km/h | ca. 763 U/min (12,7 Hz) |

Die Transalp hat eine mechanische Tachowelle, kein elektronisches Signal. Für den Bordcomputer muss entweder:
1. Ein Hall-Sensor am Vorderrad (z.B. Magnet auf Felge + Sensor an Gabelbrücke), oder
2. Ein elektronischer Tacho-Adapter (wie Dakota Digital SEN-6011) auf der Tachowelle montiert werden.

Einige Aftermarket-Digitaltacho-Hersteller (z.B. Zaddox SM8) bieten Modelle an, die direkt auf die Tachowelle passen.

Quelle: [motea.com Zaddox SM8](https://www.motea.com/en/digital-speedometer-compatible-with-honda-transalp-xl-700-650-600-v-zaddox-sm8-a035558-0), [xrv.org.uk Aftermarket Speedometer Thread](https://www.xrv.org.uk/threads/aftermarket-speedometer-for-transalp-1987.132930/)

### Gangschaltung

| Parameter | Wert |
|-----------|------|
| Schaltmuster | Links, Rücktritt, 1-N-2-3-4-5 |
| Schaltwelle | Links am Motorblock, Durchmesser ca. 20 mm |
| Leerlaufschalter | Am Schaltmechanismus, 1 Draht (Hellgrün), Schließer nach Masse im Leerlauf |

Für eine Ganganzeige gibt es zwei Ansätze:
1. Ratio-basiert: Geschwindigkeit / Drehzahl → Gang (aus Übersetzungstabelle), funktioniert ohne zusätzliche Hardware, aber ungenau bei gezogener Kupplung.
2. Mikroschalter an der Schaltwelle: Zählt die Schaltvorgänge (hoch/runter), relativ aufwendig und kalibrierungsanfällig.

### Tankstand

| Parameter | Wert |
|-----------|------|
| Tank | 18 Liter, unten am Rahmen |
| Tankgeber | Serienmäßig keiner, nur Vakuum-Petcock mit Reserve |
| Petcock-Stellungen | ON / RES / PRI (Unterdruck-betätigt) |
| Tankgeber-Nachrüstung | Aftermarket-Sender (z.B. 240-33 Ohm oder 0-190 Ohm), Einbau durch Tanköffnung |

Der Tank hat eine Öffnung oben, durch die der Petcock und die Kraftstoffleitung führen. Für einen elektronischen Tankgeber muss ein Schwimmer-Sensor durch diese Öffnung oder durch eine separate Bohrung im Tankdeckel installiert werden. Alternativ kann ein externer Durchfluss-Sensor im Kraftstoffschlauch montiert werden (wie bei der Koso RX-2N Serie).

### Batteriespannung

| Parameter | Wert |
|-----------|------|
| Batterie | 12V, 12 Ah, YB12A-B |
| Ruhestrom | max. 0,1 mA |
| Ladespannung | 13,5–15,5V bei 5.000 U/min |
| Kabel | Schwarz/Braun = Batterie Plus, Grün = Masse |

Die Batteriespannung kann direkt an der Batterie oder am Zündschalter (schwarzes Kabel = geschaltetes +12V) über einen Spannungsteiler an einen ADC des Mikrocontrollers angeschlossen werden.

---

## 8. Community-Wissen und bekannte Probleme

### Bekannte Schwachstellen der PD06

Die Transalp 600 gilt als extrem zuverlässig, aber nach 30+ Jahren Betriebsdauer gibt es typische Alterserscheinungen, die beim Bordcomputer-Projekt berücksichtigt werden müssen.

**CDI-Module (Zündboxen)**

Der häufigste Ausfall. Die CDI-Einheiten sitzen unter der Sitzbank in einer Gummihalterung. Durch Vibration und Wärme brechen die internen Lötstellen an den Steckerverbindern. Symptome: Aussetzer im Zündfunken, ein Zylinder läuft nicht, Motor stirbt im Warmzustand ab. Tritt meist zwischen 30.000 und 40.000 km auf.

- Vor Modelljahr 1996: Zwei separate CDI-Boxes (eine pro Zylinder), Bauteil-Nr. 30410-MM9-003 / 30420-MM9-003
- Ab 1996 (V-T): Eine gemeinsame Digital-CDI-Box, Bauteil-Nr. 30410-MT2-003
- Aftermarket: M.S. Dual Line CDIs (zwei Zündkurven, umschaltbar durch Drahtbrücke), Rick's Motorsport Electrics, Arrowhead IHA6001
- Diagnose: CDI ausbauen, Gehäuse öffnen, Lötstellen nachsehen. Oft reines Nachlöten der abgebrochenen Pins.

Quelle: [gummycarbs.com — CDI Modules](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V), [cdi-shop.de](https://cdi-shop.de/en/technical-specifications)

**Regler/Gleichrichter**

Shunt-Typ-Regler belasten die Statorspulen dauerhaft. Der OEM-Regler (Shindengen SH538D-13) ist nicht extrem fehleranfällig, aber die 3-poligen Steckverbinder (3× gelbe Drähte vom Stator) korrodieren und brennen durch. Symptome: Batterie wird nicht geladen, Überladung (>15,5V) oder Unterladung.

- Nachrüstung: MOSFET-Regler FH008 (CBR600RR 2007–2010) oder SH775/SH847 (Series-Regulator, entlastet den Stator)
- Kabelquerschnitt beim Umbau auf mindestens 14 AWG (2,5 mm²) erhöhen

Quelle: [gummycarbs.com — Regulator/Rectifier](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V), ADV Rider Forum Threads

**Stator (Lichtmaschine)**

Die Statorspulen sind bei 310W (frühe Modelle) bzw. 350W (1989–1993) nicht überdimensioniert. Bei hoher elektrischer Last (Zusatzlicht, Heizgriffe, Bordcomputer) kann die Leistung knapp werden. Statorwicklungen gehen meist durch Überhitzung oder Isolationsfehler kaputt. Widerstand messen: 0,1–1,0 Ohm zwischen den drei gelben Drähten, kein Kurzschluss gegen Masse.

**Vergaser**

Die Keihin CV-Vergaser (32mm bis 1995, 34mm ab 1996) sind robust, aber die Membranen im Schieber altern und reißen. Symptome: schlechtes Ansprechverhalten, Drehzahl hängt, Leerlaufprobleme. Hauptdüsen und Nadeldüsen sind noch bei Honda/Keihin erhältlich. Vergasersynchronisation ist bei jedem großen Service nötig (Differenzdruckmessgerät, Ziel: <2 cmHg Differenz).

**Kühlung**

Kühlmittelpumpe dichtet ab (Telling-Rad auf der Welle). Thermostat bleibt hängen (bleibt offen → Motor wird zu kalt, bleibt zu → Überhitzung). Kühler ist ein Einzelkühler vorne, schwer zugänglich. Die Kühlmittelfüllmenge von ca. 1,75–2,0 Litern ist gering, Luft im System macht sich schnell bemerkbar (Wärmestau hinten).

**Tachoantrieb**

Mechanische Tachowelle vom Vorderrad. Das Antriebsrad im Vorderradlager verschleißt, die Tachowelle bricht. Ersatzteile werden knapp. Alternative: elektronischer Tacho-Sensor (Hall-Sensor + Magnet auf Felge, oder Dakota Digital Sendeinheit auf der Tachowelle).

**Rost und Rahmen**

Stahlrohrrahmen (Semi-Double-Cradle). Typische Roststellen: Unterer Rahmenbereich hinter dem Motor, Schweißnähte am Hinterbau, Schwinge. Die italienischen PD10-Modelle (ab 1997) haben ein schlechteres Lackierungsniveau als die japanischen PD06.

### Foren und Community-Ressourcen

| Forum / Quelle | URL | Beschreibung |
|----------------|-----|-------------|
| ADV Rider Transalp Thread | [advrider.com](https://advrider.com/f/threads/show-us-your-transalp-modifications.39170/) | Größter Transalp-Mod-Thread, hunderte Seiten |
| xrv.org.uk | [xrv.org.uk](http://xrv.org.uk/forums/) | UK-Forum, sehr detaillierte technische Specs |
| Honda Transalp Forum (DE) | [honda-transalp-forum.de](https://www.honda-transalp-forum.de/) | Deutschsprachig, Wartung und Reparatur |
| GummyCarbs Wiki | [gummycarbs.com](https://gummycarbs.com/wiki?title=Honda_Transalp_XL600V) | Tiefe technische Dokumentation, US-Fokus |
| TA-Deti Transalp Page | [ta-deti.de](http://www.ta-deti.de/ta/transalp.html) | Vollständige Spec-Tabelle, alle Modelljahre |
| 2manybikes Transalp History | [2manybikes.be](http://www.2manybikes.be/articles/honda-transalp-history) | Modellhistorie mit Fotovergleich |

### Aftermarket-Zubehör für Bordcomputer-relevante Sensoren

**Ganganzeigen**

Die Transalp hat serienmäßig keine Ganganzeige. Nachgerüstet werden meist:

- Acumen DG8 / DG10: Kabelgebunden, liest Drehzahl und Geschwindigkeit, berechnet Gang aus Ratio. Schließt an Pickup und Tacho-Signal an. Entwickelt für Carb-Bikes ohne ECU.
- Trail Tech Vapor: Kombinierte Anzeige (Speed, RPM, Temp, Volt), Hall-Sensor am Vorderrad, Inlinesensor für Kühlmittel. Ganganzeige als Option.
- Koso RX-2N / DB-02: Digital-Tacho mit Ganganzeige, Temperatur und Öldruck als Option. Sehr beliebt bei Transalp-Fahrern.

Quelle: ADV Rider Threads, [Koso DB-02RX](https://www.kosoeurope.com/)

**Öldruck-Anzeigen**

- VDO Vision Cockpit: Mechanisches Uhrwerk, Capillary-Rohr, sehr zuverlässig
- AutoMeter: Elektrisch, 1/8 NPT oder 1/8 BSP
- Koso DB-03R: Digital, Öldruck + Öltemperatur kombiniert, Sensor 1/8 BSP (nahezu identisch mit NPT in dieser Größe)

Wichtig: Der Öldruckanschluss am PD06-Motorblock ist M10 × 1,0. Es braucht einen Adapter M10→1/8 NPT oder M10→1/8 BSP, je nach Sensor. Die original Öldruckwarnleuchte sollte unbedingt erhalten bleiben (T-Stück-Adapter).

**Kühlmitteltemperatur-Sensoren**

Der Thermostatsensor der Transalp (im Thermostatgehäuse) hat folgende Widerstandskennlinie:

| Temperatur | Widerstand (XL-Modelle) |
|------------|------------------------|
| 50°C | 154 Ohm |
| 80°C | 52 Ohm |
| 120°C | 16 Ohm |

Quelle: [motoinst.ru — Cooling System Specs](https://motoinst.ru/en/Honda/Africa-Twin/XRV750/engine/cooling/115073)

Der Lüfterschalter im Kühler schaltet bei 98–102°C ein und bei 93–97°C wieder aus. Die Widerstandskennlinie des originalen Senders kann direkt vom Bordcomputer (ESP32 ADC) gelesen werden. Bei 50°C (154 Ohm) → Spannungsteiler mit z.B. 220 Ohm Referenzwiderstand ergibt ca. 3,0V, bei 120°C (16 Ohm) → ca. 0,7V. Der ADC-Bereich des ESP32 (0–3,3V) wird gut ausgenutzt.

**LED-Umrüstung**

Häufigste elektrische Modifikation. Scheinwerfer H4 auf LED spart ca. 30–40W (von 55/60W auf 15–25W), was Strom für den Bordcomputer freigibt. Blinker auf LED brauchen Lastwiderstände oder einen LED-Blinkerrelais, sonst blinkt es zu schnell. Die Transalp hat einen elektronischen Blinkerrelais (kein thermoelektrischer), bei manchen Modellen ist der Lastwiderstand im Relais integriert.

Quelle: [2manybikes.be Transalp History](http://www.2manybikes.be/articles/honda-transalp-history)

### Tuning-Mods (relevant für Sensor-Einbau)

- Größerer Ölkühler: Aftermarket-Ölkühler (z.B. von TRW oder Lextek) mit Schlauchadaptern. Verändert den Platzbedarf hinter dem Kühler.
- Größere Batterie: YTX14AHL-BS (12V, 12Ah) als wartungsfreie AGM-Ersatz. Passt mit kleiner Modifikation der Batteriebox.
- Stärkere Lichtmaschine: Nicht als Aftermarket erhältlich. Bei 350W (1989–1993) ist genug Reserve für Bordcomputer + LED-Licht. Bei 310W-Modellen ist LED-Umrüstung empfohlen.
- Africa Twin Schwinge: RD04-Schwinge ist länger und aus Aluminium. Wird für breitere Reifen genutzt. Verändert Kettenlänge auf 124 Glieder.
- Africa Twin Tank: 23–24L (RD03/RD04) passt direkt auf PD06-Rahmen. Verdoppelt die Reichweite. Verändert die Tankgeber-Situation (AT hat eine Widerstandssonde im Tank).

Quelle: [gummycarbs.com — Africa Twin Conversion](https://gummycarbs.com/wiki?title=Honda_Transalp_and_Africa_Twin)

### Was andere PD06-Fahrer messen

Aus ADV Rider und xrv.org.uk Forenthreads (Stand 2024):

1. **Drehzahl**: Standard, über Pickup oder Zündspule. Sehr zuverlässig.
2. **Geschwindigkeit**: Entweder mechanisch (Tachowelle) oder Hall-Sensor am Vorderrad.
3. **Kühlmitteltemperatur**: Am häufigsten nachgerüstet. Koso-Inline-Sensor oder direkter Anschluss an den Thermostat-Sender.
4. **Batteriespannung**: Einfachste Nachrüstung, Koso und Trail Tech haben das an Bord.
5. **Öldruck**: Seltener, da Öldruckschalter-Abgang M10×1,0 einen Adapter braucht. Meist nur bei getunten Motoren.
6. **Gang**: Ratio-basiert (Drehzahl/Speed) oder Mikroschalter an der Schaltwelle. Beides funktioniert, ratio-basiert ist einfacher.

---

## 9. Öldruck-Spezifikationen (Service Manual)

Aus dem Honda Service Manual (Scribd und 4-stroke.net):

| Parameter | Wert |
|-----------|------|
| Öldruck (warm, bei 8.000 U/min) | 190–588 kPa (1,9–5,9 bar, 28–85 psi) |
| Öldruckwarnleuchte Schaltpunkt | < 0,2 bar (Schließer, öffnet bei Druck) |
| Öldruckschalter Anzugsdrehmoment | 12 Nm (1,2 kg·m) |
| Öldruckschalter Position | Kurbelgehäuse, linke Seite, unter Ritzelabdeckung |

Der Öldruck von 190–588 kPa bei 8.000/min ist ein breiter Bereich. In der Praxis liegt der Druck bei warmem Motor und Leerlauf deutlich niedriger (ca. 0,5–1,0 bar) und steigt bei höherer Drehzahl an. Die Warnleuchte schaltet bei ca. 0,2 bar ab. Für den Bordcomputer: Wenn der Öldrucksensor-Wert unter 0,5 bar im Fahrbetrieb fällt, Warnung ausgeben.

Quelle: [Scribd Service Manual](https://www.scribd.com/document/607260821/Honda-600-Transalp-86-01-Service-Manual), [4-stroke.net](https://4-stroke.net/documents/workshopmanual_xl600_transalp_en_1987-2002_21062017-1026-ocr_text.txt)

### Kühlerlüfter-Schalter

| Parameter | Wert |
|-----------|------|
| Lüfter EIN | 98–102°C |
| Lüfter AUS | 93–97°C |
| Sensor-Position | Im Kühler (unten rechts), 2-poliger Stecker |
| Anzugsdrehmoment | ca. 18 Nm (mit neuer Dichtung) |

Quelle: [motoinst.ru — Cooling Fan Switch](https://motoinst.ru/en/Honda/Africa-Twin/XRV750/engine/cooling/115073)

---

## 10. Zusammenfassung der Sensor-Anzapfpunkte

| Sensor | Signalquelle | Anschlusspunkt | Signalart | Bordcomputer-Interface |
|--------|-------------|---------------|-----------|----------------------|
| Drehzahl | CDI-Pickup oder Zündspule (Coil-) | CDI Pin 5 oder gelb/weiß Kabel | Puls (1 pro U/min) | Optokoppler → GPIO Interrupt |
| Geschwindigkeit | Vorderrad (mechanisch) | Tachowelle oder Hall-Sensor | Puls (pro Radumdrehung) | Hall-Sensor → GPIO Interrupt |
| Kühlmitteltemp. | Thermostat-Sender | Thermostatgehäuse, grün/blau Kabel | Widerstand (NTC) | Spannungsteiler → ADC |
| Öldruck | M10×1,0 Port am Kurbelgehäuse | T-Stück mit Originalschalter | Analog (0–5V oder 4–20mA) | ADC mit OpAmp |
| Batteriespannung | Batterie oder Zündschalter | Schwarz/Braun (Batt+) oder Schwarz (geschaltet) | DC 12–15V | Spannungsteiler → ADC |
| Tankstand | Kein Serien-Sensor | Aftermarket-Schwimmer im Tank | Widerstand (0-190Ω o. 240-33Ω) | Spannungsteiler → ADC |
| Gang | Berechnet aus RPM/Speed | Software | Berechnet | Keine Hardware nötig |

Diese Tabelle ist die primäre Eingangsgröße für die Hardware-Design-Phase. Jede Signalart bestimmt die Art der Mikrocontroller-Anbindung (digital vs. analog, Interrupt-basiert vs. Polling).

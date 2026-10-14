# Deep Dive: Multi-Display-Architektur, Smartphone-Integration & Turn-by-Turn Navigation

> Stand: 13.10.2026
> Fahrzeug: Honda Transalp 600 (PD06), Baujahr 1986-1999, rein mechanisch, keine ECU
> Plattform: ESP32-S3 (siehe docs/research/03-software-stack-deep-dive.md)
> Vorherige Dokumente: 02-hardware-deep-dive, 03-software-stack-deep-dive, 04-pd06-technical-data, 05-enclosure-env-deep-dive, 06-feature-roadmap

## 0. Einleitung

Die bisherigen Deep Dives haben Hardware, Software-Stack, Fahrzeugdaten und Features einzeln betrachtet. Dieses Dokument behandelt die naechste Ebene: wie wird aus einem reinen Sensor-Dashboard ein vernetztes Motorrad-Cockpit mit Navigation und Smartphone-Anbindung?

Die Kernfrage lautet: Wie bekommen wir Turn-by-Turn-Navigation vom Handy auf ein ESP32-S3-Display, ohne auf geschlossene Protokolle wie CarPlay eines kalifornischen Technologieherstellers oder Auto-Protokolle eines Suchmaschinenbetreibers angewiesen zu sein, und wie sieht die Hardware-Architektur aus, wenn ein oder mehrere Displays eingesetzt werden?

Die Antwort ist ein dreiteiliges System:
1. Ein ESP32-S3-Bordcomputer mit einem oder zwei Displays, die ueber SPI angesteuert werden
2. Eine eigene Companion-App (React Native oder Flutter), die als Datenvermittler zwischen Smartphone-Sensoren/-Diensten und Bordcomputer fungiert
3. Ein BLE-GATT-Protokoll, das Navigationsdaten, Medien-Infos, Anrufe und Telemetrie bidirektional uebertraegt

Dieses Dokument liefert konkrete Hardware-Empfehlungen, vollstaendige GATT-Service-Definitionen, UI-Layouts als ASCII-Art, eine Companion-App-Architektur und eine UI-State-Machine fuer das Display-Rendering.

---

## 1. Multi-Display-Architektur

### 1.1 Die Grundfrage: Ein oder zwei Displays?

Am Motorrad ist Platz knapp. Der Lenkerbereich der Transalp 600 bietet Platz fuer ein Display von maximal ca. 120x80 mm, wenn Tachometer und Lichtschalter weiterhin Platz finden. Das schliesst grosse 7-Zoll-Displays aus, die man von Motorrad-Navis kennt. Realistisch sind 2.0 bis 4.0 Zoll.

Drei Konzepte werden verglichen:

**Option A: Ein einzelnes TFT-Display (3.2"-4.0")**

Ein einzelnes Display uebernimmt alles: Geschwindigkeit, Drehzahl, Gang, Temperatur, Kraftstoff, Navigation, Medien. Der Vorteil ist Einfachheit: ein SPI-Bus, ein Framebuffer, ein Treiber. Der Nachteil ist Informationsdichte: auf 320x240 oder 480x320 Pixeln muessen Instrumente und Navigation konkurrieren.

Loesung: Split-Screen-Layout oder modusbasierter Wechsel zwischen Instrument- und Voll-Nav-Modus. Bei aktiver Navigation wird Speed in einer Ecke klein angezeigt. Das funktioniert, solange das Display gross genug ist (mindestens 3.5" / 480x320).

**Option B: Zwei Displays (E-Paper + TFT)**

Display 1 ist ein 2.9" E-Paper-Modul (296x128) fuer die Kerninstrumente: Speed, RPM, Gang, Temperatur, Kraftstoff. E-Paper ist immer an, verbraucht fast keinen Strom im statischen Zustand und ist auch bei direktem Sonnenlicht perfekt ablesbar. Display 2 ist ein 2.4-3.5" TFT fuer Navigation und Smartphone-Infos, das nur angeht, wenn Bluetooth verbunden ist.

Vorteil: klare Trennung der Anzeigen, E-Paper ist immer lesbar, TFT nur an bei Bedarf. Nachteil: mehr Hardware, mehr Kabel, zwei SPI-Busse noetig, komplexere Software mit zwei Render-Pfaden.

**Option C: Ein TFT + E-Paper-Kombi (hybrid)**

Ein TFT (2.4-3.2") fuer dynamische Inhalte: Navigation, Pfeile, Karten-Snippet, Medien. Ein kleines E-Paper (1.54" oder 2.13") fuer statische Kerninstrumente. Das ist technisch die eleganteste Loesung, aber Platz am Lenker wird sehr knapp.

### 1.2 Empfehlung

Fuer die erste Iteration: **Option A mit einem 3.5" TFT (480x320, ILI9488 oder ST7796)**. Es ist die einfachste Loesung mit dem breitesten Feature-Set. Split-Screen funktioniert auf 480x320 ausreichend. E-Paper ist fuer die Kerninstrumente attraktiv, aber die Refresh-Rate (1-2 s fuer Partial-Refresh) ist zu langsam, wenn Speed und RPM sich schnell aendern. E-Paper fuer RPM ist nicht praktikabel bei 25 Hz+ Update-Rate.

E-Paper macht Sinn als Zweitdisplay fuer statische Werte (Kraftstoff, Tageskilometer, Uhrzeit), aber das ist ein MoSCoW-Should, kein Must.

Die endgueltige Empfehlung ist ein zweistufiger Ausbau:
- Phase 1: Ein 3.5" TFT (480x320) via SPI2 (FSPI), angesteuert direkt vom ESP32-S3
- Phase 2: Zusaetzlich ein 2.13" E-Paper via SPI3 (HSPI) fuer statische Werte

### 1.3 TFT-Displays fuer Motorrad-Einsatz

Die wichtigsten Kriterien fuer ein Motorrad-Display sind Helligkeit, Schnittstelle und Verfuegbarkeit.

Helligkeit ist der entscheidende Faktor fuer Sonnenlesbarkeit. Die Faustregel: 500 nits sind in halbschattigem Bereich lesbar, 1000 nits sind das Minimum fuer direkte Sonneneinstrahlung, und 1500+ nits sind Bose-oder-OEM-Niveau. Ein Standard-ILI9341-Modul liefert typischerweise 250-300 nits. Das reicht fuer den Innenraum, aber nicht fuer Sonne am Lenker.

Es gibt zwei Wege zu mehr Helligkeit: Hochhelligkeits-Module (IPS-TFT mit 1000+ nits gibt es bei Waveshare und 4D Systems) oder transflektive Displays, die Sonnenlicht zurueckstrahlen und auch ohne Hintergrundbeleuchtung lesbar sind. Transflektive Displays sind selten und teuer, aber ideal fuer Motorraeder, da sie auch bei ausgeschalteter Beleuchtung funktionieren und kaum Strom verbrauchen.

Touchscreen ist am Motorrad kontraproduktiv. Handschuhe blocken kapazitive Touchsensoren, und mit bloßen Haenden am Lenker zu tippen ist beim Fahren zu gefaehrlich. Steuerung geht ueber physikalische Taster am Lenker (siehe Feature-Roadmap Abschnitt 6.1).

| Display | Controller | Aufloesung | Schnittstelle | Preis (EUR) | Helligkeit | Bemerkung |
|---|---|---|---|---|---|---|
| ILI9341 2.4" | ILI9341 | 320x240 | SPI (4-wire) | 5-8 | 250 nits | Billigste Option, nicht sonnenlesbar |
| ILI9341 2.4" IPS | ILI9341 | 320x240 | SPI (4-wire) | 8-12 | 400 nits | Bessere Blickwinkel, immer noch zu dunkel |
| ST7789 1.54" | ST7789 | 240x240 | SPI (4-wire) | 6-10 | 500 nits | Klein, aber ausreichend hell fuer Teil-Sun |
| ST7789 2.0" | ST7789 | 320x240 | SPI (4-wire) | 8-12 | 500 nits | Kompromiss aus Groesse und Helligkeit |
| ILI9488 3.5" | ILI9488 | 480x320 | SPI (4-wire) | 8-14 | 250 nits | Viel Platz, aber nicht sonnenlesbar |
| ST7796 4.0" | ST7796 | 320x480 | SPI (4-wire) | 10-15 | 350 nits | Groesstes SPI-Display, noch dunkel |
| Waveshare 3.5" IPS | ILI9488 | 480x320 | SPI | 15-20 | 1000 nits | Sonnenlesbar, geeignet |
| 4D Systems gen4-ESP32-35 | ILI9488 | 320x480 | SPI (intern) | 35-45 | 200 nits (TN) / 250 nits (IPS) | Integrierter ESP32-S3, aber dunkel |
| Nextion 3.5" Enhanced | proprietar | 480x320 | UART | 20-30 | 250 nits | Eigener Controller, einfacher aber teuer |

Empfehlung: Waveshare 3.5" IPS (1000 nits) oder ein hochhelliges 2.0" ST7789 (500 nits) als Kompromiss, wenn der Platz am Lenker sehr knapp ist. Das 4.0" ST7796 ist die Obergrenze fuer den Lenkerbereich der Transalp.

Quellen:
- https://dirtybirdypowersports.com/blogs/news/tft-motorcycle-dash (Helligkeits-Anforderungen)
- https://aoocci.com/es/blogs/motorcycle/sunlight-readable-motorcycle-display-nits-glare (1000 nits Baseline)
- https://www.polcd-digital.com/blogs/tft-lcd-solutions/interfacing-3-5-4-3-inch-tft-lcd-st7796s-nv3047-esp32-s3-stm32-fpc-guide (ST7796S SPI bis 80 MHz)

### 1.4 SPI vs. Parallel

SPI ist die Standard-Schnittstelle fuer Displays bis 4.0" am ESP32-S3. Bei 4-wire SPI benoetigt man 6 GPIO-Pins: SCLK, MOSI, CS, DC, RST, BL (Backlight). Die Alternative ist 16-bit Parallel, die deutlich mehr Bandbreite bietet, aber 20+ GPIO-Pins belegt, die am ESP32-S3 knapp sind (viele Pins sind fuer Flash/PSRAM reserviert).

Performance bei SPI: bei 80 MHz SPI-Takt und 480x320x2 Bytes (RGB565) pro Frame ergibt sich eine theoretische maximale Framerate von ca. 80.000.000 / (480*320*2) = ca. 26 FPS fuer einen Full-Screen-Refresh. In der Praxis sind es 15-20 FPS wegen Overhead und DMA-Latenz.

Das ist ausreichend fuer ein Dashboard mit gelegentlichen UI-Updates (Zahlen aendern sich, Pfeile wechseln), aber nicht fuer fluessige Animationen oder Karten-Rendering. Fuer das geplante Use-Case (Instrumente + Turn-by-Turn) reicht SPI.

### 1.5 Framebuffer und PSRAM

Ein Full-Screen-Framebuffer fuer 480x320 bei 16-bit RGB565 benoetigt 480 * 320 * 2 = 307.200 Bytes = ca. 300 KB. Der interne SRAM des ESP32-S3 hat 512 KB, davon bleibt nach WiFi/BT-Stack und FreeRTOS etwa 200-250 KB fuer Anwendungen. Ein vollstaendiger Framebuffer passt nicht in den internen SRAM.

Loesung: PSRAM. Der ESP32-S3 mit N8R8-Konfiguration hat 8 MB octal PSRAM. Dort passen mehrere Framebuffer problemlos. Wichtig: SPI-DMA kann nicht direkt von PSRAM lesen. Das ESP-IDF-LCD-Treiber bietet dafuer den "Bounce Buffer"-Modus, bei dem Daten in kleinen Bloecken vom PSRAM-Framebuffer in interne SRAM-Bounce-Buffer kopiert und von dort per DMA an das Display gesendet werden.

Der empfohlene Ansatz laut SDK-Dokumentation:
- Framebuffer im PSRAM (full PSRAM canvas)
- Kleiner DMA-faehiger SRAM-Buffer als Transfer-Buffer (trans_size)
- In der Flush-Callback: Daten in Bloecken von PSRAM nach SRAM kopieren und pro Block an das Display senden

LVGL (Light and Versatile Graphics Library) ist die UI-Bibliothek der Wahl. Sie unterstuetzt ESP32-S3 nativ, hat einen esp_lvgl_port fuer die Integration mit esp_lcd-Treibern, und bietet Partial-Refresh, was den Performance-Bedarf drastisch senkt (nur geaenderte Bereiche werden neu gezeichnet).

LVGL-Buffer-Empfehlung: 1/4 bis 1/8 der Bildschirmgroesse als Draw-Buffer in SRAM (DMA-faehig), nicht den vollen Framebuffer. Das reduziert RAM-Verbrauch und Latenz. Bei 480x320 sind 1/4 = 38.400 Bytes = 38 KB, was in den internen SRAM passt.

Quellen:
- https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s3/api-reference/peripherals/lcd.html (esp_lcd, Framebuffer, Bounce Buffer)
- https://forum.lvgl.io/t/create-buffers-in-psram-esp32s3/11555 (Buffer-Groessen-Empfehlung)
- https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-7/Performance.pdf (LVGL FPS-Benchmarks)

### 1.6 Kann der ESP32-S3 zwei Displays gleichzeitig ansteuern?

Ja. Der ESP32-S3 hat zwei generelle SPI-Busse: SPI2 (FSPI) und SPI3 (HSPI). SPI0 und SPI1 sind fuer Flash/PSRAM reserviert und duerfen nicht verwendet werden. Bei Modulen mit octal PSRAM (N8R8-Variante) sind zusaetzlich GPIO 35, 36 und 37 fuer die PSRAM-Anbindung blockiert.

FSPI (SPI2) hat Standard-Pins: GPIO 11 (MOSI), GPIO 12 (SCLK), GPIO 13 (MISO). HSPI (SPI3) hat beim S3 keine festen Default-Pins, muss also per GPIO-Matrix auf freie Pins gemappt werden. Das funktioniert, da der ESP32-S3 eine flexible GPIO-Matrix hat.

Zwei Displays an zwei getrennten SPI-Bussen koennen gleichzeitig laufen, aber nicht gleichzeitig DMA-uebertragen. Die GDMA-Ressourcen werden geteilt. In der Praxis loest man das durch zeitversetztes Rendering: Core 0 rendert Display 1, Core 1 rendert Display 2, die SPI-Transfers werden serialisiert.

Alternative: Zwei Displays am selben SPI-Bus mit unterschiedlichen CS-Pins. Das spart Pins (SCLK, MOSI, MISO werden geteilt), erfordert aber, dass DC und RST pro Display einzeln angesteuert werden. Das funktioniert fuer einfache Updates, ist aber bei gleichzeitigem Full-Refresh beider Displays langsam.

Die dritte Alternative ist ein separater Display-Controller: ein zweiter ESP32 (Classic oder S3), der ueber UART oder ESP-NOW Nav-Daten vom Haupt-ESP32 empfaengt und sein Display selbststaendig rendert. Das entlastet den Haupt-Controller, erhoeht aber Hardware-Komplexitaet und Stromverbrauch.

Quellen:
- https://curiousscientist.tech/blog/esp32-s3-multiple-spi-buses-spi-fspi-hspi (FSPI/HSPI Pin-Belegung, GPIO 35-37 blockiert)
- https://forum.arduino.cc/t/two-spis-parallel-on-esp32-s3/1370955 (zwei SPI-Busse gleichzeitig)
- https://diyelectromusic.com/2025/07/20/arduino-with-multiple-displays-part-2 (Multi-Display ESP32-S3)

### 1.7 Display-Controller-Alternativen

**ESP32-S3 direkt (SPI zu Display)**

Der direkte Weg: ESP32-S3 steuert das Display ueber SPI mit DMA an. LVGL rendert in einen PSRAM-Framebuffer, die esp_lcd-Treiber schicken die Daten per DMA. Bei 80 MHz SPI-Takt sind 15-20 FPS fuer einen Full-Screen-Refresh moeglich, mit Partial-Refresh deutlich mehr.

Vorteil: keine zusaetzliche Hardware, volle Kontrolle, geringste Kosten.
Nachteil: der ESP32-S3 muss sowohl Sensoren auslesen, BLE verwalten, als auch rendern. Bei einem Display ist das machbar, bei zwei wird es knapp.

**Nextion-Display (integrierter Controller)**

Nextion-Displays haben einen eigenen Prozessor, der das Rendering uebernimmt. Der ESP32 kommuniziert per UART mit dem Nextion und sendet Text-/Wert-Updates. Das entlastet den ESP32 komplett vom Rendering.

Vorteil: einfachere Software, keine Framebuffer, keine LVGL-Integration.
Nachteil: Nextion ist teuer (20-30 EUR), die UART-Bandbreite limitiert komplexe UIs, und das Nextion-eigene UI-Design-Tool ist beschraenkend. Fuer ein Motorrad-Cockpit mit dynamischer Navigation ist Nextion zu starr.

**Separater Display-Controller (zweiter ESP32)**

Ein zweiter ESP32 (S3 oder Classic) uebernimmt nur das Display-Rendering. Er ist ueber UART oder ESP-NOW mit dem Haupt-ESP32 verbunden. Der Haupt-ESP32 sendet strukturierte Datenpakete (Speed, RPM, Nav-Data), der Display-Controller rendert die UI.

Vorteil: klare Trennung der Aufgaben, Haupt-ESP32 wird entlastet, Display kann auch komplexe UIs fluessig rendern.
Nachteil: zusaetzliche Hardware, doppelte Firmware-Wartung, zusaetzlicher Stromverbrauch (ca. 30-50 mA mehr).

Empfehlung: Fuer Phase 1 (ein Display) der direkte ESP32-S3-Anschluss mit LVGL. Fuer Phase 2 (zwei Displays) einen zweiten ESP32 als Display-Controller fuer das zweite Display erwegen.

---

## 2. Smartphone-Integration

### 2.1 Warum nicht echtes CarPlay oder Auto-Protokoll des Suchmaschinenbetreibers?

CarPlay des kalifornischen Technologieherstellers benoetigt eine MFi-Zertifizierung (Made for Smartphone) und einen dedizierten Hardware-IC (Accessory Interface des kalifornischen Technologieherstellers). Ohne MFi-Lizenz gibt es keinen Zugriff auf das CarPlay-Protokoll. Die Lizenzkosten und Hardware-Anforderungen machen CarPlay fuer ein DIY-Projekt unmoeglich.

Das Auto-Protokoll des Suchmaschinenbetreibers ist etwas offener, benoetigt aber ebenfalls zertifizierte Hardware und eine Headunit-Implementierung, die nicht trivial ist.

Beide Protokolle sind darauf ausgelegt, die Smartphone-UI auf einen externen Bildschirm zu spiegeln. Das ist nicht das Ziel hier: wir wollen keine komplette Handy-UI auf dem Motorrad-Display, sondern gezielt Navigationsdaten, Medien-Infos und Anrufbenachrichtigungen.

### 2.2 Die Companion-App-Loesung

Die praktikable Alternative ist eine eigene Companion-App, die auf dem Smartphone laeuft und als Bruecke zwischen Smartphone-Diensten und Bordcomputer fungiert. Die App nutzt die nativen APIs des Handys (Maps, Media, Notifications) und sendet vereinfachte, strukturierte Daten per BLE an den ESP32.

Das ist kein CarPlay-Ersatz im Sinne von Bildschirm-Spiegelung. Es ist eine reduzierte, auf das Wesentliche beschraenkte Datenuebertragung: Navigationsanweisungen als strukturierte Daten (Pfeil-Typ, Distanz, Strassenname), nicht als Bildschirm-Inhalt.

**Architekturuebersicht:**

```
    +-------------------+          +-------------------+
    |   Smartphone      |          |   ESP32-S3        |
    |                   |          |                   |
    |  +-------------+  |   BLE    |  +-------------+  |
    |  | Companion   |  | <------> |  | BLE GATT    |  |
    |  | App         |  | GATT    |  | Server      |  |
    |  |             |  |         |  |             |  |
    |  | - Maps-API  |  |         |  | - Nav-Data  |  |
    |  |   (native)  |  |         |  |   Parser    |  |
    |  | - Now Play  |  |         |  | - UI-State  |  |
    |  | - Notifilter|  |         |  |   Machine  |  |
    |  | - TTS       |  |         |  | - LVGL      |  |
    |  +-------------+  |         |  |   Render    |  |
    |                   |          |                   |
    |  Native APIs:     |          |  Sensors:         |
    |  - Maps (native)  |          |  - GPS, RPM, Temp |
    |  - AVAudioSession |          |  - Fuel, Gear     |
    |  - CallKit/CTCall |          |                   |
    |  - Notification Svc|          |  Display:        |
    |                   |          |  - TFT 480x320   |
    +-------------------+          |  - (E-Paper)     |
                                   +-------------------+
```

### 2.3 BLE als Transport

BLE (Bluetooth Low Energy) ist der Transport-Layer zwischen App und Bordcomputer. Warum BLE und nicht Bluetooth Classic? Weil BLE auf dem ESP32-S3 stabiler ist, weniger Strom verbraucht und die GATT-Struktur fuer strukturierte Daten ideal ist.

BLE-GATT (Generic Attribute Profile) organisiert Daten in Services und Characteristics. Jeder Service enthaelt mehrere Characteristics, die gelesen, beschrieben oder abonniert (Notify) werden koennen. Das passt perfekt zum Datenmodell: Navigation, Medien, Telefon, Wetter und Telemetrie sind jeweils eigene Services.

**MTU und Payload-Groesse**

Die Default-MTU von BLE ist 23 Bytes, davon sind 3 Bytes Header, also 20 Bytes Payload pro Notification. Das reicht nicht fuer einen Strassennamen. MTU kann aber vom Client (der App) auf bis zu 517 Bytes ausgehandelt werden. In der Praxis sind 185-247 Bytes ein guter Kompromiss, die meisten Smartphones unterstuetzen das.

Wichtig: Nur der GATT-Client (die App) kann die MTU-Aushandlung initiieren. Der ESP32 als Server kann `esp_ble_gatt_set_local_mtu(247)` setzen, aber die App muss den MTU-Exchange initiieren. Die Companion-App muss also nach dem Connect explizit `requestMtu(247)` aufrufen.

Bei MTU 247 sind 244 Bytes Payload pro Notification moeglich. Das reicht fuer Strassennamen (max 32 Bytes), Navigationsdaten (ca. 40 Bytes) und sogar kleine Map-Tiles (80x80 Pixel bei 1-bit = 800 Bytes, braucht aber mehrere Pakete).

Quellen:
- https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/bt/ble.html (MTU Default 23, max 517)
- https://github.com/espressif/esp-idf/issues/16057 (nur Client kann MTU-Exchange initiieren)
- https://github.com/espressif/arduino-esp32/issues/8190 (MTU-Aushandlung Praxis)

### 2.4 Kabel-Alternative: USB-C

Der ESP32-S3 hat USB-OTG und kann als USB-Host oder USB-Device laufen. Theoretisch koennte man das Handy per USB-C-Kabel verbinden. Vorteile: zuverlaessiger als BT, laedt das Handy, mehr Bandbreite.

Aber: Das mobile BS des kalifornischen Technologieherstellers bietet keinen USB-Accessory-Mode ohne MFi. Open Accessory (AOA) des anderen mobilen BS ist praktisch tot, seit Version 6.0 es deprecated hat. USB-C als Alternative zu BLE ist fuer das eine mobile BS nicht verfuegbar und fuer das andere nicht praktikabel.

Bleibt als einziger praktikabler Weg fuer beide Plattformen: BLE.

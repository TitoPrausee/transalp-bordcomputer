/*
 * nmea_parser.c — NMEA-0183 Parser fuer GGA, RMC, VTG, GSV
 *
 * Implementiert:
 *   - XOR-Checksummen-Validierung ueber alle Zeichen zwischen $ und *
 *   - Parsen von GGA, RMC, VTG, GSV Saetzen
 *   - Konvertierung: NMEA-Koordinaten -> Dezimalgrad
 *   - Konvertierung: Knoten -> km/h (1 knot = 1.852 km/h)
 *   - Statistik: gueltige/ungueltige Saetze zaehlen
 *
 * Der Parser schreibt nur Felder, die im jeweiligen Satz enthalten sind.
 * Bestehende Werte in *out bleiben unveraendert fuer fehlende Felder.
 */

#include "gps.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static const char *TAG = "nmea";

/* ---- Parser-Statistik (global, nicht thread-safe: nur GPS-Task schreibt) ---- */
static nmea_stats_t s_stats;

/* ---- Hilfsfunktionen ---- */

/*
 * XOR-Checksumme berechnen.
 * XOR ueber alle Zeichen zwischen '$' und '*' (ausschliesslich).
 * Rueckgabe: der berechnete XOR-Wert (0x00..0xFF).
 */
static uint8_t nmea_checksum(const char *sentence)
{
    const char *p = sentence;
    uint8_t cksum = 0;

    /* Zum '$' springen */
    if (*p == '$') p++;

    /* XOR bis '*' oder Stringende */
    while (*p && *p != '*') {
        cksum ^= (uint8_t)(*p);
        p++;
    }
    return cksum;
}

/*
 * Checksumme validieren.
 * Erwartet Format: $....*XX  (XX = zweistellige Hex-Checksumme)
 * Rueckgabe: true wenn berechnete und angegebene Checksumme uebereinstimmen.
 */
static bool nmea_validate_checksum(const char *sentence)
{
    const char *star = strchr(sentence, '*');
    if (!star) return false;

    uint8_t calc = nmea_checksum(sentence);

    /* Zwei Hex-Ziffern nach '*' lesen */
    const char *hex = star + 1;
    char hexbuf[3];
    hexbuf[0] = hex[0];
    hexbuf[1] = hex[1];
    hexbuf[2] = '\0';

    char *endptr = NULL;
    long given = strtol(hexbuf, &endptr, 16);
    if (endptr == hexbuf) return false;

    return (uint8_t)given == calc;
}

/*
 * Feld aus einem NMEA-Satz extrahieren.
 * NMEA-Saetze sind komma-separiert: $TYP,f1,f2,f3,...
 *
 * buf:      Aufrufer-seitiger Buffer (mindestens field_buf_size Bytes).
 * buf_size: Groesse von buf in Bytes.
 * Gibt einen Pointer auf buf zurueck (null-terminiert) wenn das Feld
 * existiert und nicht leer ist, sonst NULL.
 * Reentrant: der Buffer gehoert dem Aufrufer.
 */
static char *nmea_field(const char *sentence, int field_index,
                        char *buf, size_t buf_size)
{
    if (!sentence || !buf || buf_size == 0) return NULL;
    const char *p = sentence;
    int current = 0;
    int out_idx = 0;

    /* Zum ersten Komma (nach dem Typ) springen */
    while (*p && *p != ',') p++;
    if (*p != ',') return NULL;
    p++;

    while (current < field_index && *p) {
        if (*p == ',') current++;
        p++;
    }
    if (current < field_index) return NULL;

    /* Feld bis naechstes Komma oder '*' kopieren */
    while (*p && *p != ',' && *p != '*' && out_idx < (int)buf_size - 1) {
        buf[out_idx++] = *p++;
    }
    buf[out_idx] = '\0';

    if (out_idx == 0) return NULL;
    return buf;
}

/*
 * Float aus einem Feld lesen.
 * Rueckgabe: true wenn Feld existiert und nicht leer ist.
 */
static bool field_to_float(const char *sentence, int idx, float *out)
{
    char buf[32];
    const char *f = nmea_field(sentence, idx, buf, sizeof(buf));
    if (!f || !*f) return false;
    *out = strtof(f, NULL);
    return true;
}

/*
 * Integer aus einem Feld lesen.
 */
static bool field_to_int(const char *sentence, int idx, int *out)
{
    char buf[32];
    const char *f = nmea_field(sentence, idx, buf, sizeof(buf));
    if (!f || !*f) return false;
    *out = (int)strtol(f, NULL, 10);
    return true;
}

/*
 * NMEA-Koordinate in Dezimalgrad umwandeln.
 * Format: ddmm.mmmm  (Breitengrad: dd=Grad, mm.mmmm=Minuten)
 *         dddmm.mmmm (Laengengrad: ddd=Grad, mm.mmmm=Minuten)
 * dir: 'N'/'S' fuer Breite, 'E'/'W' fuer Laenge.
 *
 * Rueckgabe: true wenn Umwandlung erfolgreich.
 */
static bool parse_coord(const char *raw, char dir, float *out)
{
    if (!raw || !*raw || !dir) return false;

    char *endptr = NULL;
    float value = strtof(raw, &endptr);
    if (endptr == raw) return false;

    int deg;
    float minutes;

    /* Breite: ddmm.mmmm (2 Grad-Ziffern), Laenge: dddmm.mmmm (3 Ziffern).
     * (int)(value / 100) extrahiert die Grad-Anteile fuer beide Formate. */
    deg = (int)(value / 100);      /* dd bzw. ddd */
    minutes = value - (deg * 100); /* mm.mmmm */

    float decimal = (float)deg + minutes / 60.0f;

    if (dir == 'S' || dir == 'W') decimal = -decimal;

    *out = decimal;
    return true;
}

/*
 * NMEA-Zeit (hhmmss.sss) in Stunden/Minuten/Sekunden zerlegen.
 */
static bool parse_time(const char *raw, uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    if (!raw || !*raw || strlen(raw) < 6) return false;

    /* strlen(raw) >= 6 is checked above, so raw[0..5] are in bounds. The
     * host-analysis strlen mock returns a constant, which makes clang-analyzer
     * believe raw is shorter and flag raw[2..5] as uninitialized — a false
     * positive. Suppressed per TOT-61. */
    // NOLINTBEGIN(clang-analyzer-core.uninitialized.Assign)
    char buf[7];
    buf[0] = raw[0]; buf[1] = raw[1]; buf[2] = '\0';
    *hour = (uint8_t)strtol(buf, NULL, 10);

    buf[0] = raw[2]; buf[1] = raw[3]; buf[2] = '\0';
    *minute = (uint8_t)strtol(buf, NULL, 10);

    buf[0] = raw[4]; buf[1] = raw[5]; buf[2] = '\0';
    *second = (uint8_t)strtol(buf, NULL, 10);
    // NOLINTEND(clang-analyzer-core.uninitialized.Assign)

    return true;
}

/*
 * NMEA-Datum (ddmmyy) in Tag/Monat/Jahr zerlegen.
 * Jahr: yy -> 20yy (Annahme: 2000er Jahre).
 */
static bool parse_date(const char *raw, uint8_t *day, uint8_t *month, uint16_t *year)
{
    if (!raw || !*raw || strlen(raw) < 6) return false;

    char buf[5];

    buf[0] = raw[0]; buf[1] = raw[1]; buf[2] = '\0';
    *day = (uint8_t)strtol(buf, NULL, 10);

    buf[0] = raw[2]; buf[1] = raw[3]; buf[2] = '\0';
    *month = (uint8_t)strtol(buf, NULL, 10);

    buf[0] = raw[4]; buf[1] = raw[5]; buf[2] = '\0';
    int yy = (int)strtol(buf, NULL, 10);
    *year = (uint16_t)(2000 + yy);

    return true;
}

/* ---- Satz-Typ erkennen ---- */

/*
 * Den Talker+Typ Praefex ueberspringen und Satz-Typ zurueckgeben.
 * GPGGA und GNGGA etc. werden gleich behandelt.
 * Rueckgabe: Zeiger auf den 3-Zeichen Typ ohne Talker (z.B. "GGA") oder NULL.
 */
static const char *get_sentence_type(const char *sentence)
{
    /* "$GP" oder "$GN" oder "$GL" etc. ueberspringen */
    if (sentence[0] != '$') return NULL;

    /* Typ beginnt nach $ + 2 Talker-Zeichen (z.B. "GP", "GN") */
    const char *type = sentence + 3;
    if (strlen(type) < 3) return NULL;

    return type;
}

/* ---- Satz-Parser ---- */

/*
 * $GPGGA — Global Positioning System Fix Data
 *
 * Felder (0-basiert ab erstem Datenfeld nach Typ):
 * 0: UTC-Zeit hhmmss.sss
 * 1: Breite ddmm.mmmm
 * 2: N/S
 * 3: Laenge dddmm.mmmm
 * 4: E/W
 * 5: Fix-Qualitaet: 0=none, 1=GPS, 2=DGPS, ...
 * 6: Anzahl Satelliten
 * 7: HDOP
 * 8: Hoehe (m)
 * 9: M (meter)
 * 10: Geoid-Separation
 * 11: M
 * 12: DGPS Age
 * 13: DGPS Station
 */
static void parse_gga(const char *sentence, gps_data_t *out)
{
    const char *f;
    float fval;
    int ival;
    char coord_raw[32];
    char field_buf[32];

    /* Zeit */
    f = nmea_field(sentence, 0, field_buf, sizeof(field_buf));
    if (f) parse_time(f, &out->hour, &out->minute, &out->second);

    /* Breite */
    f = nmea_field(sentence, 1, field_buf, sizeof(field_buf));
    if (f) {
        strncpy(coord_raw, f, sizeof(coord_raw) - 1);
        coord_raw[sizeof(coord_raw) - 1] = '\0';
    }
    const char *ns = nmea_field(sentence, 2, field_buf, sizeof(field_buf));
    if (f && ns) {
        parse_coord(coord_raw, ns[0], &out->lat);
    }

    /* Laenge */
    f = nmea_field(sentence, 3, field_buf, sizeof(field_buf));
    if (f) {
        strncpy(coord_raw, f, sizeof(coord_raw) - 1);
        coord_raw[sizeof(coord_raw) - 1] = '\0';
    }
    const char *ew = nmea_field(sentence, 4, field_buf, sizeof(field_buf));
    if (f && ew) {
        parse_coord(coord_raw, ew[0], &out->lon);
    }

    /* Fix-Qualitaet */
    if (field_to_int(sentence, 5, &ival)) {
        switch (ival) {
        case 0:  out->fix_quality = GPS_FIX_NONE; break;
        case 1:  out->fix_quality = GPS_FIX_3D; break;
        case 2:  out->fix_quality = GPS_FIX_DGPS; break;
        default: out->fix_quality = GPS_FIX_3D; break;
        }
        out->has_fix = (ival >= 1);
    }

    /* Satelliten */
    if (field_to_int(sentence, 6, &ival)) {
        out->satellites = (uint8_t)ival;
    }

    /* HDOP */
    if (field_to_float(sentence, 7, &fval)) {
        out->hdop = fval;
    }

    /* Hoehe */
    if (field_to_float(sentence, 8, &fval)) {
        out->altitude_m = fval;
    }

    /* valid-Flag aktualisieren */
    if (out->has_fix && out->hdop < 5.0f) {
        out->valid = true;
    }
}

/*
 * $GPRMC — Recommended Minimum Navigation Information
 *
 * Felder (0-basiert ab erstem Datenfeld nach Typ):
 * 0: Zeit hhmmss.sss
 * 1: Status: A=active, V=void
 * 2: Breite ddmm.mmmm
 * 3: N/S
 * 4: Laenge dddmm.mmmm
 * 5: E/W
 * 6: Speed (knots)
 * 7: Course (degrees true)
 * 8: Datum ddmmyy
 * 9: Mag. Variation
 * 10: E/W
 * 11: Mode indicator
 */
static void parse_rmc(const char *sentence, gps_data_t *out)
{
    const char *f;
    float fval;
    char coord_raw[32];
    char field_buf[32];

    /* Zeit */
    f = nmea_field(sentence, 0, field_buf, sizeof(field_buf));
    if (f) parse_time(f, &out->hour, &out->minute, &out->second);

    /* Status */
    const char *status = nmea_field(sentence, 1, field_buf, sizeof(field_buf));
    bool rmc_active = (status && status[0] == 'A');

    /* Breite */
    f = nmea_field(sentence, 2, field_buf, sizeof(field_buf));
    if (f) {
        strncpy(coord_raw, f, sizeof(coord_raw) - 1);
        coord_raw[sizeof(coord_raw) - 1] = '\0';
    }
    const char *ns = nmea_field(sentence, 3, field_buf, sizeof(field_buf));
    if (f && ns) {
        parse_coord(coord_raw, ns[0], &out->lat);
    }

    /* Laenge */
    f = nmea_field(sentence, 4, field_buf, sizeof(field_buf));
    if (f) {
        strncpy(coord_raw, f, sizeof(coord_raw) - 1);
        coord_raw[sizeof(coord_raw) - 1] = '\0';
    }
    const char *ew = nmea_field(sentence, 5, field_buf, sizeof(field_buf));
    if (f && ew) {
        parse_coord(coord_raw, ew[0], &out->lon);
    }

    /* Speed: knots -> km/h */
    if (field_to_float(sentence, 6, &fval)) {
        out->speed_kmh = fval * 1.852f;
    }

    /* Course */
    if (field_to_float(sentence, 7, &fval)) {
        out->course_deg = fval;
    }

    /* Datum */
    f = nmea_field(sentence, 8, field_buf, sizeof(field_buf));
    if (f) parse_date(f, &out->day, &out->month, &out->year);

    /* Fix-Status aus RMC */
    out->has_fix = rmc_active;
    if (!rmc_active) {
        out->valid = false;
    }
}

/*
 * $GPVTG — Track Made Good and Ground Speed
 *
 * Felder (0-basiert ab erstem Datenfeld nach Typ):
 * 0: Course (degrees true)
 * 1: T
 * 2: Course (degrees magnetic)
 * 3: M
 * 4: Speed (knots)
 * 5: N
 * 6: Speed (km/h)
 * 7: K
 * 8: Mode indicator
 */
static void parse_vtg(const char *sentence, gps_data_t *out)
{
    float fval;

    /* Course (true) */
    if (field_to_float(sentence, 0, &fval)) {
        out->course_deg = fval;
    }

    /* Speed (knots) als Fallback */
    if (field_to_float(sentence, 4, &fval)) {
        /* Nur ueberschreiben wenn noch kein km/h-Wert vorhanden oder
         * VTG-km/h direkt aus Feld 6 lesen wir unten bevorzugt */
        out->speed_kmh = fval * 1.852f;
    }

    /* Speed (km/h) — direkt aus Satz, falls vorhanden */
    if (field_to_float(sentence, 6, &fval)) {
        out->speed_kmh = fval;
    }
}

/*
 * $GPGSV — Satellites in View
 *
 * Felder (0-basiert ab erstem Datenfeld nach Typ):
 * 0: Total number of GSV messages
 * 1: Message number
 * 2: Total satellites in view
 *
 * Pro Satellit (4 Felder, bis zu 4 pro Satz):
 *   3+4*i: PRN
 *   4+4*i: Elevation
 *   5+4*i: Azimuth
 *   6+4*i: SNR
 *
 * Wir speichern nur die Gesamtzahl der Satelliten in Sicht.
 */
static void parse_gsv(const char *sentence, gps_data_t *out)
{
    (void)out;
    int total_sats;
    if (field_to_int(sentence, 2, &total_sats)) {
        /* Nur die Gesamtzahl in Sicht — nicht die im Fix.
         * Die im Fix kommt aus GGA. */
        /* Feld satellites in gps_data_t ist "im Fix", daher
         * schreiben wir hier nicht hinein. Stattdessen fuer Statistik. */
    }
}

/* ---- Oeffentliche API ---- */

void gps_get_stats(nmea_stats_t *out)
{
    if (!out) return;
    *out = s_stats;
}

/*
 * NMEA-Satz parsen.
 *
 * sentence: vollstaendiger NMEA-Satz inkl. $ und *CC
 * out:      Ziel-Struktur
 * Rueckgabe: true wenn Satz gueltig und bekannter Typ
 */
bool nmea_parse(const char *sentence, gps_data_t *out)
{
    s_stats.total++;

    if (!sentence || !out) {
        s_stats.malformed++;
        return false;
    }

    /* Muss mit '$' beginnen */
    if (sentence[0] != '$') {
        s_stats.malformed++;
        return false;
    }

    /* Checksumme pruefen */
    if (!nmea_validate_checksum(sentence)) {
        s_stats.checksum_failed++;
        return false;
    }

    /* Satz-Typ ermitteln */
    const char *type = get_sentence_type(sentence);
    if (!type) {
        s_stats.malformed++;
        return false;
    }

    /* Erste 3 Zeichen nach Talker: GGA, RMC, VTG, GSV */
    if (strncmp(type, "GGA", 3) == 0) {
        parse_gga(sentence, out);
        s_stats.gga++;
    } else if (strncmp(type, "RMC", 3) == 0) {
        parse_rmc(sentence, out);
        s_stats.rmc++;
    } else if (strncmp(type, "VTG", 3) == 0) {
        parse_vtg(sentence, out);
        s_stats.vtg++;
    } else if (strncmp(type, "GSV", 3) == 0) {
        parse_gsv(sentence, out);
        s_stats.gsv++;
    } else {
        /* Unbekannter Satz-Typ — nicht als Fehler, aber auch nicht als gueltig */
        return false;
    }

    s_stats.valid++;
    return true;
}

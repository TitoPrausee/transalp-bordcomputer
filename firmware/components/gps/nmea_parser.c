/*
 * nmea_parser.c — NMEA-Parser fuer GGA, RMC, VTG
 *
 * Empfehlung aus docs/research/03-software-stack.md: minmea Library.
 * Dieser Stub zeigt die erwartete Funktionalitaet.
 *
 * TODO: minmea als ESP-IDF Component einbinden (components/minmea/).
 *       minmea ist pure C, eine Source-Datei, keine Abhaengigkeiten.
 *       Quelle: github.com/kosma/minmea
 *
 * TODO: UBX-Konfiguration: 10Hz, nur GGA+RMC aktivieren.
 */

#include "gps.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "nmea";

/* TODO: Sobald minmea integriert ist:
 * #include "minmea.h"
 *
 * void nmea_parse_line(const char *line, gps_data_t *data) {
 *     switch (minmea_sentence_id(line, false)) {
 *     case MINMEA_SENTENCE_RMC: {
 *         struct minmea_sentence_rmc frame;
 *         if (minmea_parse_rmc(&frame, line)) {
 *             data->lat = minmea_tocoord(&frame.latitude);
 *             data->lon = minmea_tocoord(&frame.longitude);
 *             data->speed_kmh = minmea_tofloat(&frame.speed) * 1.852f;
 *             data->course_deg = minmea_tofloat(&frame.course);
 *         }
 *         break;
 *     }
 *     case MINMEA_SENTENCE_GGA: {
 *         struct minmea_sentence_gga frame;
 *         if (minmea_parse_gga(&frame, line)) {
 *             data->satellites = frame.satellites_tracked;
 *             data->hdop = minmea_tofloat(&frame.hdop);
 *             data->altitude_m = minmea_tofloat(&frame.altitude);
 *             data->fix_quality = (frame.fix_quality >= 1) ? GPS_FIX_3D : GPS_FIX_NONE;
 *             data->valid = (data->hdop < 5.0f && frame.fix_quality >= 1);
 *         }
 *         break;
 *     }
 *     case MINMEA_SENTENCE_VTG: {
 *         struct minmea_sentence_vtg frame;
 *         if (minmea_parse_vtg(&frame, line)) {
 *             data->speed_kmh = minmea_tofloat(&frame.speed_kph);
 *         }
 *         break;
 *     }
 *     default:
 *         break;
 *     }
 * }
 */

void nmea_parse_line(const char *line, gps_data_t *data)
{
    /* Stub: alle NMEA-Saetze ignorieren, bis minmea integriert ist. */
    (void)line;
    (void)data;
}

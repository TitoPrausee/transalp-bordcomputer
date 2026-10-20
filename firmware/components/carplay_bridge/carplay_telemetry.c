/*
 * carplay_telemetry.c — Telemetry Packet Packer fuer CarPlay-Bridge
 *
 * Wandelt Sensor-/GPS-/Power-Eingangsdaten in das 12-byte
 * packed Telemetry-Packet (CHR_CP_TLM_LIVE 0x9001) um.
 *
 * Reine Funktionen ohne BLE-/ESP-IDF-Abhaengigkeiten — daher
 * vollstaendig im Host-Unit-Test mit GCC testbar.
 *
 * Siehe docs/CARPLAY-INTEGRATION.md Abschnitt GATT-Service.
 */

#include "carplay_internal.h"
#include "carplay_uuids.h"

#include <string.h>

/* ============================================================
 * Telemetry packet builder
 *
 * Konvertiert float-Eingangsdaten in das packed Little-Endian
 * Binary-Format. Rundet sinnvoll und klammert unplausible Werte.
 * ============================================================ */

void carplay_telemetry_build(carplay_telemetry_t *pkt,
                              const carplay_telemetry_input_t *in)
{
    if (pkt == NULL || in == NULL) {
        return;
    }

    memset(pkt, 0, sizeof(*pkt));

    /* Speed: 0-255 km/h (uint8) */
    float speed = in->speed_kmh;
    if (speed < 0.0f) speed = 0.0f;
    if (speed > 255.0f) speed = 255.0f;
    pkt->speed_kmh = (uint8_t)(speed + 0.5f);

    /* RPM: direkte Uebernahme (uint16) */
    pkt->rpm = in->rpm;

    /* Gear: 0 (neutral) wird zu -1 im Wire-Format */
    pkt->gear = (in->gear == 0) ? (int8_t)-1 : in->gear;

    /* Coolant temp (int8, -128..127) */
    float coolant = in->coolant_temp_c;
    if (coolant > 127.0f) coolant = 127.0f;
    if (coolant < -128.0f) coolant = -128.0f;
    pkt->coolant_temp_c = (int8_t)coolant;

    /* Fuel percent: 0-100 (uint8) */
    float fuel_pct = 0.0f;
    if (in->tank_capacity_l > 0.0f) {
        fuel_pct = (in->fuel_level_l / in->tank_capacity_l) * 100.0f;
    }
    if (fuel_pct < 0.0f) fuel_pct = 0.0f;
    if (fuel_pct > 100.0f) fuel_pct = 100.0f;
    pkt->fuel_percent = (uint8_t)(fuel_pct + 0.5f);

    /* Battery voltage in tenths of a volt (uint16) */
    float voltage = in->battery_voltage;
    if (voltage < 0.0f) voltage = 0.0f;
    if (voltage > 65.53f) voltage = 65.53f;  /* uint16 max / 1000 * 10 */
    pkt->voltage_tenth = (uint16_t)(voltage * 10.0f + 0.5f);

    /* Ambient temp (int16) */
    float ambient = in->ambient_temp_c;
    if (ambient > 32767.0f) ambient = 32767.0f;
    if (ambient < -32768.0f) ambient = -32768.0f;
    pkt->ambient_temp_c = (int16_t)ambient;

    /* Fuel-low flag: under reserve threshold */
    pkt->fuel_low = (in->fuel_reserve_l > 0.0f &&
                     in->fuel_level_l <= in->fuel_reserve_l) ? 1 : 0;

    pkt->reserved = 0;
}

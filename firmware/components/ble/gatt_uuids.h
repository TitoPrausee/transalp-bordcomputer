#ifndef _GATT_UUIDS_H_
#define _GATT_UUIDS_H_

/*
 * gatt_uuids.h — GATT Service/Characteristic UUIDs
 *
 * MUESSEN mit der Companion-App uebereinstimmen!
 * Siehe docs/research/13-connectivity-app.md fuer vollstaendige Definition.
 *
 * Format: 128-bit UUID, Base = 0000xxxx-0000-1000-8000-00805F9B34FB
 */

/* === Telemetry-Service === */
#define UUID_SVC_TELEMETRY     0xA001
#define UUID_CHR_GPS_DATA      0xA001  /* notify: lat, lon, speed, alt, sats */
#define UUID_CHR_RPM_SPEED     0xA002  /* notify: rpm, speed_kmh, gear */
#define UUID_CHR_SENSORS       0xA003  /* notify: temp, alt, roll, pitch */
#define UUID_CHR_BATTERY       0xA004  /* read: voltage, percentage */

/* === Config-Service === */
#define UUID_SVC_CONFIG        0xA002
#define UUID_CHR_DISPLAY_MODE  0xA005  /* read/write: 0=Tour, 1=Track, 2=Config */
#define UUID_CHR_LOG_INTERVAL  0xA006  /* read/write: 1-60 seconds */
#define UUID_CHR_CALIBRATION   0xA007  /* write: IMU offsets, BMP280 sea-level */
#define UUID_CHR_DEVICE_INFO   0xA008  /* read: firmware version, uptime */

/* === Navigation-Service === */
#define UUID_SVC_NAVIGATION    0xA003
#define UUID_CHR_NEXT_TURN     0xA009  /* write: direction, distance, street */
#define UUID_CHR_ETA           0xA00A  /* write: arrival_time, distance_total */
#define UUID_CHR_ROUTE_ACTIVE  0xA00B  /* read/write: bool */

/* === OTA-Service === */
#define UUID_SVC_OTA           0xA004
#define UUID_CHR_OTA_CONTROL   0xA00C  /* write: start/cancel/verify */
#define UUID_CHR_OTA_DATA      0xA00D  /* write: firmware chunks */
#define UUID_CHR_OTA_STATUS    0xA00E  /* notify: progress, errors */

#endif /* _GATT_UUIDS_H_ */

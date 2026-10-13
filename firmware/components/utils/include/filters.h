#ifndef _FILTERS_H_
#define _FILTERS_H_

#include <stdint.h>

/*
 * Signal-Filter fuer Sensor-Fusion
 */

/* Kalman 1D: fuer Glaettung von Rauschsignen (z.B. GPS-Speed) */
typedef struct {
    float q;        /* Process noise covariance */
    float r;        /* Measurement noise covariance */
    float x;        /* Estimated value */
    float p;        /* Estimation error covariance */
    float k;        /* Kalman gain */
} kalman1d_t;

void kalman1d_init(kalman1d_t *kf, float q, float r, float initial);
float kalman1d_update(kalman1d_t *kf, float measurement);

/* Complementary Filter: kombiniert Gyro (kurzfristig) und Accel (langfristig) */
typedef struct {
    float alpha;    /* Gewicht Gyro (typ. 0.98) */
    float angle;    /* Aktueller Winkel */
    float dt;       /* Zeitintervall in Sekunden */
} complementary_filter_t;

void comp_filter_init(complementary_filter_t *cf, float alpha, float dt);
float comp_filter_update(complementary_filter_t *cf, float gyro_rate, float accel_angle);

/* Moving Average */
typedef struct {
    float *buf;
    uint8_t size;
    uint8_t idx;
    uint8_t count;
    float sum;
} moving_avg_t;

void moving_avg_init(moving_avg_t *ma, float *buffer, uint8_t size);
float moving_avg_update(moving_avg_t *ma, float value);

#endif /* _FILTERS_H_ */

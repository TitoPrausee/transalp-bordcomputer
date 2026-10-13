/*
 * filters.c — Kalman, Complementary Filter, Moving Average
 *
 * Implementierung siehe docs/research/03-software-stack.md Abschnitt 5.3.
 */

#include "filters.h"
#include <math.h>

/* --- Kalman 1D --- */

void kalman1d_init(kalman1d_t *kf, float q, float r, float initial)
{
    kf->q = q;
    kf->r = r;
    kf->x = initial;
    kf->p = 1.0f;
    kf->k = 0.0f;
}

float kalman1d_update(kalman1d_t *kf, float measurement)
{
    /* Predict */
    kf->p = kf->p + kf->q;
    /* Update */
    kf->k = kf->p / (kf->p + kf->r);
    kf->x = kf->x + kf->k * (measurement - kf->x);
    kf->p = (1.0f - kf->k) * kf->p;
    return kf->x;
}

/* --- Complementary Filter --- */

void comp_filter_init(complementary_filter_t *cf, float alpha, float dt)
{
    cf->alpha = alpha;
    cf->dt = dt;
    cf->angle = 0.0f;
}

float comp_filter_update(complementary_filter_t *cf, float gyro_rate, float accel_angle)
{
    cf->angle = cf->alpha * (cf->angle + gyro_rate * cf->dt)
                + (1.0f - cf->alpha) * accel_angle;
    return cf->angle;
}

/* --- Moving Average --- */

void moving_avg_init(moving_avg_t *ma, float *buffer, uint8_t size)
{
    ma->buf = buffer;
    ma->size = size;
    ma->idx = 0;
    ma->count = 0;
    ma->sum = 0.0f;
}

float moving_avg_update(moving_avg_t *ma, float value)
{
    if (ma->count < ma->size) {
        ma->count++;
    } else {
        ma->sum -= ma->buf[ma->idx];
    }
    ma->buf[ma->idx] = value;
    ma->sum += value;
    ma->idx = (ma->idx + 1) % ma->size;
    return ma->sum / ma->count;
}

#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Utility macros and shared types */

#define ARRAY_LEN(a)   (sizeof(a) / sizeof((a)[0]))
#define CLAMP(x, lo, hi)  ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
#define DEG_TO_RAD(x)  ((x) * 0.01745329251f)
#define RAD_TO_DEG(x)  ((x) * 57.2957795131f)

/* Timestamp helper: milliseconds since boot */
static inline uint32_t millis(void)
{
    return (uint32_t)(xthal_get_ccount() / 240000);  /* 240MHz CPU */
}

#endif /* _UTILS_H_ */

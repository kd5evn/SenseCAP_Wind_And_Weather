#pragma once
#include <stdint.h>
#include <stdbool.h>

/* Canonical wind sample (design doc, section 5): 12 bytes.
 * Speeds are stored in 0.1 m/s, direction in degrees. */
typedef struct __attribute__((packed)) {
    uint32_t t;      /* Unix time, seconds */
    uint16_t avg;    /* average wind, 0.1 m/s */
    uint16_t gust;   /* gust, 0.1 m/s */
    uint16_t lull;   /* lull, 0.1 m/s */
    uint16_t dir;    /* degrees from which the wind blows */
} wind_sample_t;

#define WIND_HISTORY_LEN 720   /* 6 h at 30 s, per location */
#define WIND_FORECAST_LEN 48   /* hourly entries */

typedef struct {
    char name[32];
    wind_sample_t history[WIND_HISTORY_LEN];
    uint16_t count;
    wind_sample_t forecast[WIND_FORECAST_LEN];
    uint16_t fcount;
} wind_location_t;

typedef struct {
    float min_ms, max_ms, avg_ms, peak_gust_ms;
} wind_stats_t;

static inline float wind_ms(uint16_t v) { return v / 10.0f; }
static inline float wind_mph(uint16_t v) { return v / 10.0f * 2.2369363f; }

void wind_model_init(void);
wind_location_t *wind_model_get(int idx);
int wind_model_count(void);
/* Statistics over the last window_s seconds of history, computed on device. */
bool wind_model_stats(const wind_location_t *loc, uint32_t now, uint32_t window_s, wind_stats_t *out);

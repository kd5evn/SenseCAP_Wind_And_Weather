#include "wind_model.h"
#include <math.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_log.h"

#define MAX_LOCATIONS 8
static wind_location_t *s_loc[MAX_LOCATIONS];
static int s_count;

wind_location_t *wind_model_get(int idx) { return (idx >= 0 && idx < s_count) ? s_loc[idx] : NULL; }
int wind_model_count(void) { return s_count; }

bool wind_model_stats(const wind_location_t *loc, uint32_t now, uint32_t window_s, wind_stats_t *out)
{
    uint32_t from = now > window_s ? now - window_s : 0;
    int n = 0; double sum = 0;
    out->min_ms = 1e9f; out->max_ms = 0; out->peak_gust_ms = 0;
    for (int i = 0; i < loc->count; i++) {
        const wind_sample_t *s = &loc->history[i];
        if (s->t < from || s->t > now) continue;
        float avg = wind_ms(s->avg), lull = wind_ms(s->lull), gust = wind_ms(s->gust);
        float lo = lull > 0 ? lull : avg;
        if (lo < out->min_ms) out->min_ms = lo;
        if (avg > out->max_ms) out->max_ms = avg;
        if (gust > out->peak_gust_ms) out->peak_gust_ms = gust;
        sum += avg; n++;
    }
    if (!n) return false;
    out->avg_ms = sum / n;
    return true;
}

/* M0: synthetic data so the UI can be checked on hardware before any network code exists.
 * Replaced by provider data in milestone M2. */
static void fill_demo(wind_location_t *l, const char *name, float base, float swing)
{
    strlcpy(l->name, name, sizeof(l->name));
    uint32_t now = 1800000000u;
    l->count = 720;
    for (int i = 0; i < 720; i++) {
        float a = base + swing * sinf(i / 90.0f) + 0.6f * sinf(i / 11.0f);
        if (a < 0.5f) a = 0.5f;
        l->history[i] = (wind_sample_t){ .t = now - (719 - i) * 30, .avg = a * 10,
            .gust = a * 13 + 5, .lull = a * 7, .dir = 270 + (int)(25 * sinf(i / 60.0f)) };
    }
    l->fcount = 48;
    for (int i = 0; i < 48; i++) {
        float a = base + swing * sinf((720 + i * 120) / 90.0f);
        if (a < 0.5f) a = 0.5f;
        l->forecast[i] = (wind_sample_t){ .t = now + (i + 1) * 3600, .avg = a * 10,
            .gust = a * 13 + 5, .lull = a * 7, .dir = 270 };
    }
}

void wind_model_init(void)
{
    static const struct { const char *n; float b, s; } demo[] = {
        { "Hood River Waterfront", 6.5f, 2.5f }, { "The Hatchery", 8.0f, 3.0f } };
    for (int i = 0; i < 2; i++) {
        s_loc[i] = heap_caps_calloc(1, sizeof(wind_location_t), MALLOC_CAP_SPIRAM);
        fill_demo(s_loc[i], demo[i].n, demo[i].b, demo[i].s);
        s_count++;
    }
    ESP_LOGW("wind", "M0: showing synthetic demo data, no network yet");
}

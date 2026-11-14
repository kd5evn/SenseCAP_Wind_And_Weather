/* Wind page, wireframe in design doc section 6: top bar, headline, stat chips, graph, footer.
 * Speeds shown in mph. Swipe left/right changes location; tap a chip cycles the stats window. */
#include <math.h>
#include <stdio.h>
#include "lvgl.h"
#include "wind_page.h"
#include "wind_model.h"

#define C_BG      lv_color_hex(0x0B0F14)
#define C_PANEL   lv_color_hex(0x151B23)
#define C_TEXT    lv_color_hex(0xE8EEF4)
#define C_QUIET   lv_color_hex(0x8A97A6)
#define C_AVG     lv_color_hex(0x3DA5F5)
#define C_GUST    lv_color_hex(0xF5A83D)
#define C_LULL    lv_color_hex(0x5ED6A0)

static const uint32_t k_windows[] = { 3600, 10800, 21600 };
static const char *k_window_names[] = { "1 h", "3 h", "6 h" };

static int s_loc = 0, s_win = 2;
static uint32_t s_horizon_s = 12 * 3600;
static lv_obj_t *s_name, *s_speed, *s_gust, *s_dir, *s_age, *s_graph, *s_dots[8], *s_win_lbl;
static lv_obj_t *s_chip[4];

static const char *compass(int deg)
{
    static const char *n[] = { "N","NNE","NE","ENE","E","ESE","SE","SSE","S","SSW","SW","WSW","W","WNW","NW","NNW" };
    return n[((deg % 360) * 16 + 180) / 360 % 16];
}

static uint32_t now_s(const wind_location_t *l) { return l->count ? l->history[l->count - 1].t : 0; }

static void graph_draw(lv_event_t *e)
{
    lv_obj_t *o = lv_event_get_target(e);
    lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);
    wind_location_t *l = wind_model_get(s_loc);
    if (!l || !l->count) return;
    lv_area_t a; lv_obj_get_coords(o, &a);
    const int L = a.x1 + 8, R = a.x2 - 8, T = a.y1 + 18, B = a.y2 - 22;
    uint32_t now = now_s(l), t0 = now - 21600, t1 = now + s_horizon_s;

    float ymax = 5;
    for (int i = 0; i < l->count; i++) { float g = wind_mph(l->history[i].gust); if (g > ymax) ymax = g; }
    for (int i = 0; i < l->fcount; i++) { float g = wind_mph(l->forecast[i].gust); if (g > ymax && l->forecast[i].t <= t1) ymax = g; }
    ymax = ceilf(ymax / 5.0f) * 5.0f;   /* y axis starts at zero, tops at next round number */

    #define X(t) ((int)(L + (int64_t)((t) - t0) * (R - L) / (int64_t)(t1 - t0)))
    #define Y(v) ((int)(B - (v) / ymax * (B - T)))

    lv_draw_line_dsc_t d; lv_draw_line_dsc_init(&d);
    lv_point_t p1, p2;
    /* grid + axis labels */
    d.color = C_PANEL; d.width = 1; d.opa = LV_OPA_COVER;
    lv_draw_label_dsc_t ld; lv_draw_label_dsc_init(&ld); ld.color = C_QUIET; ld.font = &lv_font_montserrat_12;
    for (int v = 0; v <= (int)ymax; v += 5) {
        p1 = (lv_point_t){ L, Y(v) }; p2 = (lv_point_t){ R, Y(v) };
        lv_draw_line(ctx, &d, &p1, &p2);
        char b[8]; snprintf(b, sizeof b, "%d", v);
        lv_area_t la = { L + 2, Y(v) - 14, L + 40, Y(v) };
        lv_draw_label(ctx, &ld, &la, b, NULL);
    }
    /* now line */
    d.color = C_QUIET; d.dash_width = 4; d.dash_gap = 4;
    p1 = (lv_point_t){ X(now), T }; p2 = (lv_point_t){ X(now), B };
    lv_draw_line(ctx, &d, &p1, &p2);
    lv_area_t na = { X(now) - 20, T - 16, X(now) + 20, T };
    ld.align = LV_TEXT_ALIGN_CENTER; lv_draw_label(ctx, &ld, &na, "now", NULL);

    /* series: 0 = avg, 1 = gust, 2 = lull; observed solid, forecast dashed and dimmer */
    const lv_color_t col[3] = { C_AVG, C_GUST, C_LULL };
    for (int s = 0; s < 3; s++) {
        lv_draw_line_dsc_init(&d);
        d.color = col[s]; d.width = (s == 0) ? 2 : 1;
        const wind_sample_t *prev = NULL;
        for (int i = 0; i < l->count; i++) {
            const wind_sample_t *p = &l->history[i];
            uint16_t v = s == 0 ? p->avg : s == 1 ? p->gust : p->lull;
            if (prev && p->t - prev->t <= 120 && p->t >= t0) {   /* gaps > 2 intervals break the line */
                uint16_t pv = s == 0 ? prev->avg : s == 1 ? prev->gust : prev->lull;
                p1 = (lv_point_t){ X(prev->t), Y(wind_mph(pv)) }; p2 = (lv_point_t){ X(p->t), Y(wind_mph(v)) };
                lv_draw_line(ctx, &d, &p1, &p2);
            }
            prev = p;
        }
        d.opa = LV_OPA_60; d.dash_width = 5; d.dash_gap = 4;
        uint32_t pt = now; uint16_t pv = l->history[l->count - 1].avg;
        pv = s == 0 ? l->history[l->count - 1].avg : s == 1 ? l->history[l->count - 1].gust : l->history[l->count - 1].lull;
        for (int i = 0; i < l->fcount; i++) {
            const wind_sample_t *p = &l->forecast[i];
            if (p->t > t1) break;
            uint16_t v = s == 0 ? p->avg : s == 1 ? p->gust : p->lull;
            p1 = (lv_point_t){ X(pt), Y(wind_mph(pv)) }; p2 = (lv_point_t){ X(p->t), Y(wind_mph(v)) };
            lv_draw_line(ctx, &d, &p1, &p2);
            pt = p->t; pv = v;
        }
    }
    ld.align = LV_TEXT_ALIGN_LEFT;
    lv_area_t o1 = { L, B + 4, L + 160, B + 20 }; lv_draw_label(ctx, &ld, &o1, "observed, past 6 h", NULL);
    char hb[32]; snprintf(hb, sizeof hb, "forecast, %d h", (int)(s_horizon_s / 3600));
    lv_area_t o2 = { X(now) + 30, B + 4, R, B + 20 }; lv_draw_label(ctx, &ld, &o2, hb, NULL);
}

static void refresh(void)
{
    wind_location_t *l = wind_model_get(s_loc);
    if (!l || !l->count) return;
    const wind_sample_t *c = &l->history[l->count - 1];
    char b[48];
    lv_label_set_text(s_name, l->name);
    snprintf(b, sizeof b, "%.0f", wind_mph(c->avg)); lv_label_set_text(s_speed, b);
    snprintf(b, sizeof b, "G %.0f mph", wind_mph(c->gust)); lv_label_set_text(s_gust, b);
    snprintf(b, sizeof b, "%s %d\xC2\xB0", compass(c->dir), c->dir); lv_label_set_text(s_dir, b);
    lv_label_set_text(s_age, "demo data");

    wind_stats_t st;
    if (wind_model_stats(l, now_s(l), k_windows[s_win], &st)) {
        const char *k[4] = { "min", "max", "avg", "gust" };
        float v[4] = { st.min_ms, st.max_ms, st.avg_ms, st.peak_gust_ms };
        for (int i = 0; i < 4; i++) {
            snprintf(b, sizeof b, "%s %.0f", k[i], v[i] * 2.2369363f);
            lv_label_set_text(lv_obj_get_child(s_chip[i], 0), b);
        }
    }
    lv_label_set_text(s_win_lbl, k_window_names[s_win]);
    for (int i = 0; i < wind_model_count() && i < 8; i++)
        lv_obj_set_style_bg_color(s_dots[i], i == s_loc ? C_TEXT : C_QUIET, 0);
    lv_obj_invalidate(s_graph);
}

static void chip_cb(lv_event_t *e) { s_win = (s_win + 1) % 3; refresh(); }

static void gesture_cb(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    int n = wind_model_count();
    if (dir == LV_DIR_LEFT) s_loc = (s_loc + 1) % n;
    else if (dir == LV_DIR_RIGHT) s_loc = (s_loc + n - 1) % n;
    else return;
    refresh();
}

static lv_obj_t *label(lv_obj_t *p, const lv_font_t *f, lv_color_t c, lv_align_t al, int x, int y)
{
    lv_obj_t *o = lv_label_create(p);
    lv_obj_set_style_text_font(o, f, 0); lv_obj_set_style_text_color(o, c, 0);
    lv_obj_align(o, al, x, y);
    return o;
}

void wind_page_init(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, C_BG, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr, gesture_cb, LV_EVENT_GESTURE, NULL);

    /* top bar, 48 px */
    s_name = label(scr, &lv_font_montserrat_20, C_TEXT, LV_ALIGN_TOP_LEFT, 16, 12);
    label(scr, &lv_font_montserrat_14, C_QUIET, LV_ALIGN_TOP_RIGHT, -16, 16);
    lv_label_set_text(lv_obj_get_child(scr, -1), "WIND");

    /* headline row, 96 px */
    s_speed = label(scr, &lv_font_montserrat_48, C_TEXT, LV_ALIGN_TOP_LEFT, 16, 52);
    label(scr, &lv_font_montserrat_16, C_QUIET, LV_ALIGN_TOP_LEFT, 112, 80);
    lv_label_set_text(lv_obj_get_child(scr, -1), "mph");
    s_gust = label(scr, &lv_font_montserrat_24, C_GUST, LV_ALIGN_TOP_LEFT, 190, 62);
    s_dir = label(scr, &lv_font_montserrat_24, C_TEXT, LV_ALIGN_TOP_RIGHT, -16, 62);
    s_age = label(scr, &lv_font_montserrat_14, C_QUIET, LV_ALIGN_TOP_LEFT, 16, 112);

    /* stat chips, 56 px */
    for (int i = 0; i < 4; i++) {
        s_chip[i] = lv_btn_create(scr);
        lv_obj_set_size(s_chip[i], 100, 44);
        lv_obj_align(s_chip[i], LV_ALIGN_TOP_LEFT, 16 + i * 112, 138);
        lv_obj_set_style_bg_color(s_chip[i], C_PANEL, 0);
        lv_obj_set_style_radius(s_chip[i], 22, 0);
        lv_obj_set_style_shadow_width(s_chip[i], 0, 0);
        lv_obj_add_event_cb(s_chip[i], chip_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *t = lv_label_create(s_chip[i]);
        lv_obj_set_style_text_font(t, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(t, C_TEXT, 0);
        lv_obj_center(t);
    }
    s_win_lbl = label(scr, &lv_font_montserrat_12, C_QUIET, LV_ALIGN_TOP_RIGHT, -16, 184);

    /* graph, about 220 px */
    s_graph = lv_obj_create(scr);
    lv_obj_remove_style_all(s_graph);
    lv_obj_set_size(s_graph, 480, 236);
    lv_obj_align(s_graph, LV_ALIGN_TOP_LEFT, 0, 198);
    lv_obj_add_event_cb(s_graph, graph_draw, LV_EVENT_DRAW_MAIN_END, NULL);

    /* footer: page dots + source name */
    int n = wind_model_count();
    for (int i = 0; i < n && i < 8; i++) {
        s_dots[i] = lv_obj_create(scr);
        lv_obj_remove_style_all(s_dots[i]);
        lv_obj_set_size(s_dots[i], 8, 8);
        lv_obj_set_style_radius(s_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(s_dots[i], LV_OPA_COVER, 0);
        lv_obj_align(s_dots[i], LV_ALIGN_BOTTOM_MID, (i - (n - 1) / 2.0f) * 16, -14);
    }
    refresh();
}

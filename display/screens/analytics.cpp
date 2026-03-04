#ifdef ALTRUIST_INSIDE

#include "analytics.h"
#include "../driver/EPD.h"
#include "../paint_driver/GUI_Paint.h"
#include "../icons/icons/icons_15x15.h"
#include "../paint_driver/fonts/fonts.h"
#include "../utils.h"
#include "../../utils.h"
#include "../../defines.h"
#include "../../intl.h"
#include "../../config_manager/config_helpers.h"
#include <Preferences.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

namespace {
constexpr float kNoTempData = -1000.0f;
constexpr float kNoData = -1.0f;
constexpr float kEpsilon = 0.1f;
constexpr float kPi = 3.14159265f;
analytics_period_t g_analytics_period = analytics_period_t::P24H;
analytics_view_t g_analytics_view = analytics_view_t::OVERVIEW_24H;

#if defined(INTL_RU)
#define A_TXT(en, ru) ru
#else
#define A_TXT(en, ru) en
#endif

struct CategorySummary {
    const char *label;
    int score;  // 0..100, higher is better
    bool has_data;
};

struct RollingDayBucket {
    uint32_t day_key = 0; // local calendar day key (days-from-civil)
    float min_v = 0.0f;
    float max_v = 0.0f;
    float sum_v = 0.0f;
    uint16_t count = 0;
};

struct RollingMetricHistory {
    RollingDayBucket buckets[30];
};

RollingMetricHistory g_temp_hist;
RollingMetricHistory g_hum_hist;
RollingMetricHistory g_dew_hist;
RollingMetricHistory g_pm10_hist;
RollingMetricHistory g_pm25_hist;
RollingMetricHistory g_co2_hist;
RollingMetricHistory g_noise_hist;

constexpr uint8_t kAnalyticsHistVersion = 1;
constexpr uint32_t kAnalyticsPersistIntervalMs = 60UL * 60UL * 1000UL; // 1 hour
bool g_hist_loaded = false;
bool g_hist_dirty = false;
uint32_t g_hist_last_save_ms = 0;
uint32_t g_hist_last_day_key = 0;
uint32_t g_hist_first_save_ts = 0;
uint32_t g_hist_last_save_ts = 0;
uint32_t g_hist_save_count = 0;
bool g_hist_last_save_forced = false;

static void sanitizeRollingHistory(RollingMetricHistory &hist) {
    for (int i = 0; i < 30; i++) {
        RollingDayBucket &b = hist.buckets[i];
        if (b.count == 0) {
            b.day_key = 0;
            b.min_v = 0.0f;
            b.max_v = 0.0f;
            b.sum_v = 0.0f;
            continue;
        }
        if (!isfinite(b.min_v) || !isfinite(b.max_v) || !isfinite(b.sum_v)) {
            b.day_key = 0;
            b.min_v = 0.0f;
            b.max_v = 0.0f;
            b.sum_v = 0.0f;
            b.count = 0;
            continue;
        }
        if (b.max_v < b.min_v) {
            float t = b.max_v;
            b.max_v = b.min_v;
            b.min_v = t;
        }
    }
}

static void loadRollingHistoryIfNeeded() {
    if (g_hist_loaded) return;
    g_hist_loaded = true;

    Preferences prefs;
    // Use read-write open here so first boot can create namespace without noisy NOT_FOUND logs.
    if (!prefs.begin("analytics", false)) {
        return;
    }

    const uint8_t version = prefs.getUChar("ver", 0);
    if (version != kAnalyticsHistVersion) {
        prefs.end();
        return;
    }
    g_hist_first_save_ts = prefs.getULong("fst_ts", 0);
    g_hist_last_save_ts = prefs.getULong("lst_ts", 0);

    if (prefs.getBytesLength("temp") == sizeof(g_temp_hist)) prefs.getBytes("temp", &g_temp_hist, sizeof(g_temp_hist));
    if (prefs.getBytesLength("hum") == sizeof(g_hum_hist)) prefs.getBytes("hum", &g_hum_hist, sizeof(g_hum_hist));
    if (prefs.getBytesLength("dew") == sizeof(g_dew_hist)) prefs.getBytes("dew", &g_dew_hist, sizeof(g_dew_hist));
    if (prefs.getBytesLength("pm10") == sizeof(g_pm10_hist)) prefs.getBytes("pm10", &g_pm10_hist, sizeof(g_pm10_hist));
    if (prefs.getBytesLength("pm25") == sizeof(g_pm25_hist)) prefs.getBytes("pm25", &g_pm25_hist, sizeof(g_pm25_hist));
    if (prefs.getBytesLength("co2") == sizeof(g_co2_hist)) prefs.getBytes("co2", &g_co2_hist, sizeof(g_co2_hist));
    if (prefs.getBytesLength("noise") == sizeof(g_noise_hist)) prefs.getBytes("noise", &g_noise_hist, sizeof(g_noise_hist));
    prefs.end();

    sanitizeRollingHistory(g_temp_hist);
    sanitizeRollingHistory(g_hum_hist);
    sanitizeRollingHistory(g_dew_hist);
    sanitizeRollingHistory(g_pm10_hist);
    sanitizeRollingHistory(g_pm25_hist);
    sanitizeRollingHistory(g_co2_hist);
    sanitizeRollingHistory(g_noise_hist);
}

static void saveRollingHistoryIfNeeded(bool force) {
    if (!g_hist_loaded || !g_hist_dirty) return;

    const uint32_t now_ms = millis();
    if (!force && (now_ms - g_hist_last_save_ms) < kAnalyticsPersistIntervalMs) {
        return;
    }

    Preferences prefs;
    if (!prefs.begin("analytics", false)) {
        return;
    }

    prefs.putUChar("ver", kAnalyticsHistVersion);
    const uint32_t now_ts = (uint32_t)time(nullptr);
    if (now_ts > 0 && g_hist_first_save_ts == 0) {
        g_hist_first_save_ts = now_ts;
    }
    if (now_ts > 0) {
        g_hist_last_save_ts = now_ts;
    }
    prefs.putULong("fst_ts", g_hist_first_save_ts);
    prefs.putULong("lst_ts", g_hist_last_save_ts);
    prefs.putBytes("temp", &g_temp_hist, sizeof(g_temp_hist));
    prefs.putBytes("hum", &g_hum_hist, sizeof(g_hum_hist));
    prefs.putBytes("dew", &g_dew_hist, sizeof(g_dew_hist));
    prefs.putBytes("pm10", &g_pm10_hist, sizeof(g_pm10_hist));
    prefs.putBytes("pm25", &g_pm25_hist, sizeof(g_pm25_hist));
    prefs.putBytes("co2", &g_co2_hist, sizeof(g_co2_hist));
    prefs.putBytes("noise", &g_noise_hist, sizeof(g_noise_hist));
    prefs.end();

    g_hist_dirty = false;
    g_hist_last_save_ms = now_ms;
    g_hist_save_count++;
    g_hist_last_save_forced = force;
}

static bool rollingHistoryHasAnyData(const RollingMetricHistory &hist) {
    for (int i = 0; i < 30; i++) {
        if (hist.buckets[i].count > 0) return true;
    }
    return false;
}

static void formatLogTs(char *out, size_t out_sz, uint32_t ts) {
    if (!out || out_sz == 0) return;
    if (ts == 0) {
        snprintf(out, out_sz, "-");
        return;
    }
    time_t t = (time_t)ts;
    struct tm tm_v;
    localtime_r(&t, &tm_v);
    strftime(out, out_sz, "%Y-%m-%d %H:%M:%S", &tm_v);
}

static bool isValidTemp(float v) {
    return (v > -40.0f && v < 80.0f && fabsf(v - kNoTempData) > kEpsilon);
}

static bool isValidHumidity(float v) {
    return (v >= 0.0f && v <= 100.0f && fabsf(v - kNoData) > kEpsilon);
}

static bool isValidCO2(float v) {
    return (v >= 300.0f && v <= 5000.0f && fabsf(v - kNoData) > kEpsilon);
}

static bool isValidPM25(float v) {
    return (v >= 0.0f && v <= 800.0f && fabsf(v - kNoData) > kEpsilon);
}

static bool isValidPM10(float v) {
    return (v >= 0.0f && v <= 1500.0f && fabsf(v - kNoData) > kEpsilon);
}

static bool isValidNoise(float v) {
    return (v >= 0.0f && v <= 120.0f && fabsf(v - kNoData) > kEpsilon);
}

// Civil-date day serial (Howard Hinnant's days_from_civil).
// Returns a key that increments by 1 each local calendar day.
static int32_t daysFromCivil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);                   // [0, 399]
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; // [0, 365]
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;       // [0, 146096]
    return era * 146097 + (int)doe - 719468; // 1970-01-01 => 0
}

static uint32_t currentLocalDayKey(time_t now) {
    struct tm local_tm;
    localtime_r(&now, &local_tm);
    const int y = local_tm.tm_year + 1900;
    const unsigned m = (unsigned)local_tm.tm_mon + 1U;
    const unsigned d = (unsigned)local_tm.tm_mday;
    const int32_t key = daysFromCivil(y, m, d);
    return key < 0 ? 0U : (uint32_t)key;
}

static void updateRollingMetric(RollingMetricHistory &hist, float value, uint32_t day_key) {
    const uint32_t idx = day_key % 30U;
    RollingDayBucket &b = hist.buckets[idx];
    if (b.count == 0 || b.day_key != day_key) {
        b.day_key = day_key;
        b.min_v = value;
        b.max_v = value;
        b.sum_v = value;
        b.count = 1;
        return;
    }
    if (value < b.min_v) b.min_v = value;
    if (value > b.max_v) b.max_v = value;
    b.sum_v += value;
    b.count++;
}

static void fillMetricFromRolling(analytics_metric_t &metric, const RollingMetricHistory &hist, uint16_t days) {
    metric.has_24h = false;
    if (days == 0) return;
    time_t now = time(nullptr);
    if (now <= 0) return;
    uint32_t today = currentLocalDayKey(now);
    uint32_t start_day = (days > 0 && today >= (uint32_t)(days - 1)) ? (today - (uint32_t)(days - 1)) : 0;

    float min_v = 0.0f;
    float max_v = 0.0f;
    float sum_w = 0.0f;
    uint32_t total_count = 0;
    bool any = false;
    for (int i = 0; i < 30; i++) {
        const RollingDayBucket &b = hist.buckets[i];
        if (b.count == 0) continue;
        if (b.day_key < start_day || b.day_key > today) continue;
        if (!any) {
            min_v = b.min_v;
            max_v = b.max_v;
            any = true;
        } else {
            if (b.min_v < min_v) min_v = b.min_v;
            if (b.max_v > max_v) max_v = b.max_v;
        }
        sum_w += b.sum_v;
        total_count += b.count;
    }
    if (!any || total_count == 0) return;
    metric.min24h = min_v;
    metric.max24h = max_v;
    metric.avg24h = sum_w / (float)total_count;
    metric.has_24h = true;
}

static uint16_t countCoverageDays(const RollingMetricHistory &hist, uint16_t days) {
    if (days == 0) return 0;
    time_t now = time(nullptr);
    if (now <= 0) return 0;
    uint32_t today = currentLocalDayKey(now);
    uint32_t start_day = (today >= (uint32_t)(days - 1)) ? (today - (uint32_t)(days - 1)) : 0;
    uint16_t covered = 0;
    for (uint32_t d = start_day; d <= today; d++) {
        const RollingDayBucket &b = hist.buckets[d % 30U];
        if (b.count > 0 && b.day_key == d) covered++;
    }
    return covered;
}

static float computeDewPoint(float temp_c, float humidity_pct) {
    if (!isValidTemp(temp_c) || !isValidHumidity(humidity_pct) || humidity_pct <= 0.0f) {
        return kNoTempData;
    }
    const float a = 17.62f;
    const float b = 243.12f;
    const float gamma = logf(humidity_pct / 100.0f) + (a * temp_c) / (b + temp_c);
    return (b * gamma) / (a - gamma);
}

static float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static void drawArcBand(int cx, int cy, int outer_r, int inner_r,
                        float start_deg, float sweep_deg, float normalized) {
    if (outer_r < inner_r) {
        int t = outer_r;
        outer_r = inner_r;
        inner_r = t;
    }
    if (outer_r <= 0) return;

    const float fill = clamp01(normalized);
    const float end_deg = start_deg + sweep_deg * fill;
    if (fill <= 0.0f || fabsf(end_deg - start_deg) < 0.01f) return;

    // Draw radial lines instead of per-pixel rings to avoid tiny white gaps.
    float step_deg = 0.28f;
    if (outer_r >= 70) step_deg = 0.20f;
    if (outer_r <= 38) step_deg = 0.35f;

    const float span = fabsf(end_deg - start_deg);
    const int samples = (int)(span / step_deg) + 1;
    // Three staggered passes + edge stitching make the arc look more solid on e-ink.
    for (int pass = 0; pass < 3; ++pass) {
        const float pass_offset = step_deg * ((float)pass / 3.0f);
        bool have_prev = false;
        int prev_ix = 0, prev_iy = 0, prev_ox = 0, prev_oy = 0;
        for (int i = 0; i <= samples; ++i) {
            float t = (samples > 0) ? ((float)i / (float)samples) : 0.0f;
            float a = start_deg + (end_deg - start_deg) * t + pass_offset;
            if ((end_deg >= start_deg && a > end_deg) || (end_deg < start_deg && a < end_deg)) {
                a = end_deg;
            }
            float rad = a * kPi / 180.0f;
            float c = cosf(rad);
            float s = sinf(rad);

            int ix = cx + (int)roundf(c * (float)inner_r);
            int iy = cy + (int)roundf(s * (float)inner_r);
            int ox = cx + (int)roundf(c * (float)outer_r);
            int oy = cy + (int)roundf(s * (float)outer_r);
            Paint_DrawLine(ix, iy, ox, oy, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

            if (have_prev) {
                Paint_DrawLine(prev_ix, prev_iy, ix, iy, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
                Paint_DrawLine(prev_ox, prev_oy, ox, oy, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            }
            have_prev = true;
            prev_ix = ix; prev_iy = iy; prev_ox = ox; prev_oy = oy;
        }
    }

    // Fill tiny cap holes at arc ends caused by integer rounding.
    {
        float rs = start_deg * kPi / 180.0f;
        float re = end_deg * kPi / 180.0f;
        float mid = (float)(inner_r + outer_r) * 0.5f;
        Paint_DrawCircle(cx + (int)roundf(cosf(rs) * mid), cy + (int)roundf(sinf(rs) * mid),
                         1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(cx + (int)roundf(cosf(re) * mid), cy + (int)roundf(sinf(re) * mid),
                         1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
}

static int metricScoreLinearHighWorse(float value, float good_limit, float bad_limit) {
    if (value <= good_limit) return 100;
    if (value >= bad_limit) return 0;
    float t = (value - good_limit) / (bad_limit - good_limit);
    return (int)(100.0f * (1.0f - t));
}

static int metricScoreRangeBest(float value, float best_low, float best_high, float hard_low, float hard_high) {
    if (value <= hard_low || value >= hard_high) return 0;
    if (value >= best_low && value <= best_high) return 100;
    if (value < best_low) {
        float t = (value - hard_low) / (best_low - hard_low);
        return (int)(100.0f * t);
    }
    float t = (hard_high - value) / (hard_high - best_high);
    return (int)(100.0f * t);
}

static bool periodValue(const analytics_metric_t &metric, float &out) {
    if (metric.has_24h) {
        out = metric.avg24h;
        return true;
    }
    return false;
}

static const char* qualityByScore(int score, const char *good, const char *mid, const char *bad) {
    if (score >= 75) return good;
    if (score >= 45) return mid;
    return bad;
}

static void drawPrimaryValueMixedCompact(int cx, int y, const char *primary);

static void drawPanelCircle(int cx, int cy, int radius,
                            const char *title,
                            const char *primary,
                            const char *secondary,
                            const char *status,
                            float arc_fill) {
    Paint_DrawCircle(cx, cy, radius, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(cx, cy, radius - 6, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    // Use the same denser/thicker arc geometry as detail circles to reduce white spots.
    drawArcBand(cx, cy, radius + 2, radius - 8, 158.0f, 206.0f, arc_fill);
    // Re-draw borders for crisp alignment after arc fill.
    Paint_DrawCircle(cx, cy, radius, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(cx, cy, radius - 6, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    const int title_y = cy - radius + 20;
    const int top_line_y = cy - radius + 34;
    uint16_t title_w = Paint_GetStringWidth_Display(title, &Font12, &font_12_cyrillic, &font_12_ascii);
    Paint_DrawString_Display(cx - (int)title_w / 2, title_y, title,
                             &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    Paint_DrawLine(cx - radius + 28, top_line_y, cx + radius - 28, top_line_y,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    bool drew_climate_primary = false;
    if (primary) {
        char t_num[16] = {0};
        char h_num[16] = {0};
        if (sscanf(primary, "%15[^C]C/%15[^%%]%%", t_num, h_num) == 2) {
            uint16_t t_num_w = Paint_GetStringWidth_Display(t_num, &Font20, &font_20_cyrillic, &font_20_ascii);
            uint16_t t_unit_w = Paint_GetStringWidth_Display("C", &Font12, &font_12_cyrillic, &font_12_ascii);
            uint16_t slash_w = Paint_GetStringWidth_Display("/", &Font12, &font_12_cyrillic, &font_12_ascii);
            uint16_t h_num_w = Paint_GetStringWidth_Display(h_num, &Font20, &font_20_cyrillic, &font_20_ascii);
            uint16_t h_unit_w = Paint_GetStringWidth_Display("%", &Font12, &font_12_cyrillic, &font_12_ascii);
            int total_w = (int)t_num_w + (int)t_unit_w + 3 + (int)slash_w + 3 + (int)h_num_w + (int)h_unit_w;
            int x = cx - total_w / 2;
            int y = cy - 18;
            Paint_DrawString_Display(x, y, t_num, &Font20, &font_20_cyrillic, &font_20_ascii, WHITE, BLACK);
            x += (int)t_num_w;
            Paint_DrawString_Display(x + 1, y + 6, "C", &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
            x += (int)t_unit_w + 3;
            Paint_DrawString_Display(x, y + 6, "/", &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
            x += (int)slash_w + 3;
            Paint_DrawString_Display(x, y, h_num, &Font20, &font_20_cyrillic, &font_20_ascii, WHITE, BLACK);
            x += (int)h_num_w;
            Paint_DrawString_Display(x + 1, y + 6, "%", &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
            drew_climate_primary = true;
        }
    }
    if (!drew_climate_primary) {
        drawPrimaryValueMixedCompact(cx, cy - 18, primary);
    }

    uint16_t secondary_w = Paint_GetStringWidth_Display(secondary, &Font12, &font_12_cyrillic, &font_12_ascii);
    Paint_DrawString_Display(cx - (int)secondary_w / 2, cy + 10, secondary,
                             &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);

    const int bottom_line_y = cy + 24;
    // Fit divider to inner circle geometry to avoid any border overflow.
    const int inner_r = radius - 6;
    const int dy = abs(bottom_line_y - cy);
    int half_w = inner_r - 2;
    if (dy < inner_r) {
        half_w = (int)floorf(sqrtf((float)(inner_r * inner_r - dy * dy))) - 1;
    }
    if (half_w < 8) half_w = 8;
    Paint_DrawLine(cx - half_w, bottom_line_y, cx + half_w, bottom_line_y,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    uint16_t status_w = Paint_GetStringWidth_Display(status, &Font12, &font_12_cyrillic, &font_12_ascii);
    Paint_DrawString_Display(cx - (int)status_w / 2, cy + 30, status,
                             &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
}

static void drawTextBold(int x, int y, const char *text, sFONT *font) {
    Paint_DrawString_Display(x, y, text, font, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    Paint_DrawString_Display(x + 1, y, text, font, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
}

static void drawTextWrapped(int x, int y, int max_w, const char *text, sFONT *font, int max_lines) {
    if (!text || !font || max_lines <= 0) return;
    char src[220];
    snprintf(src, sizeof(src), "%s", text);

    int line = 0;
    int cursor = 0;
    const int src_len = (int)strlen(src);
    while (cursor < src_len && line < max_lines) {
        while (cursor < src_len && src[cursor] == ' ') cursor++;
        if (cursor >= src_len) break;

        char out[110] = {0};
        int out_len = 0;
        int probe = cursor;
        int last_good = cursor;
        while (probe < src_len) {
            int word_end = probe;
            while (word_end < src_len && src[word_end] != ' ') word_end++;

            char candidate[110];
            if (out_len > 0) snprintf(candidate, sizeof(candidate), "%s %.*s", out, word_end - probe, src + probe);
            else snprintf(candidate, sizeof(candidate), "%.*s", word_end - probe, src + probe);

            if (Paint_GetStringWidth_Display(candidate, font, &font_12_cyrillic, &font_12_ascii) > (uint16_t)max_w) {
                break;
            }
            snprintf(out, sizeof(out), "%s", candidate);
            out_len = (int)strlen(out);
            last_good = word_end;
            probe = word_end;
            while (probe < src_len && src[probe] == ' ') probe++;
        }

        if (out_len == 0) {
            // Single long token fallback.
            int cut = cursor + 1;
            while (cut <= src_len) {
                char candidate[110];
                snprintf(candidate, sizeof(candidate), "%.*s", cut - cursor, src + cursor);
                if (Paint_GetStringWidth_Display(candidate, font, &font_12_cyrillic, &font_12_ascii) > (uint16_t)max_w) {
                    cut--;
                    break;
                }
                cut++;
            }
            if (cut <= cursor) cut = cursor + 1;
            snprintf(out, sizeof(out), "%.*s", cut - cursor, src + cursor);
            last_good = cut;
        }

        Paint_DrawString_Display(x, y + line * 13, out, font, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        cursor = last_good;
        line++;
    }
}

static void drawPrimaryValueMixedCompact(int cx, int y, const char *primary) {
    if (!primary || primary[0] == '\0') return;
    int split = 0;
    while (primary[split] != '\0') {
        char ch = primary[split];
        bool numeric = ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-' || ch == ',');
        if (!numeric) break;
        split++;
    }
    if (split <= 0 || primary[split] == '\0') {
        uint16_t w = Paint_GetStringWidth_Display(primary, &Font20, &font_20_cyrillic, &font_20_ascii);
        Paint_DrawString_Display(cx - (int)w / 2, y, primary,
                                 &Font20, &font_20_cyrillic, &font_20_ascii, WHITE, BLACK);
        return;
    }
    char number_part[20];
    char unit_part[24];
    snprintf(number_part, sizeof(number_part), "%.*s", split, primary);
    snprintf(unit_part, sizeof(unit_part), "%s", primary + split);
    uint16_t num_w = Paint_GetStringWidth_Display(number_part, &Font20, &font_20_cyrillic, &font_20_ascii);
    uint16_t unit_w = Paint_GetStringWidth_Display(unit_part, &Font12, &font_12_cyrillic, &font_12_ascii);
    int start_x = cx - (int)(num_w + unit_w + 2) / 2;
    Paint_DrawString_Display(start_x, y, number_part,
                             &Font20, &font_20_cyrillic, &font_20_ascii, WHITE, BLACK);
    Paint_DrawString_Display(start_x + (int)num_w + 2, y + 6, unit_part,
                             &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
}

static void drawPrimaryValueMixed(int cx, int y, const char *primary) {
    if (!primary || primary[0] == '\0') return;
    int split = 0;
    while (primary[split] != '\0') {
        char ch = primary[split];
        bool numeric = ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-' || ch == ',');
        if (!numeric) break;
        split++;
    }

    if (split <= 0 || primary[split] == '\0') {
        uint16_t w = Paint_GetStringWidth_Display(primary, &Font24, &font_32_cyrillic, &font_32_ascii);
        Paint_DrawString_Display(cx - (int)w / 2, y, primary,
                                 &Font24, &font_32_cyrillic, &font_32_ascii, WHITE, BLACK);
        return;
    }

    char number_part[20];
    char unit_part[20];
    snprintf(number_part, sizeof(number_part), "%.*s", split, primary);
    snprintf(unit_part, sizeof(unit_part), "%s", primary + split);

    const bool is_air_unit = (strstr(unit_part, "ug/m3") != nullptr || strstr(unit_part, "µg/m3") != nullptr);
    sFONT *unit_font = is_air_unit ? &Font12 : &Font16;
    const Font *unit_font_cyr = is_air_unit ? &font_12_cyrillic : &font_16_cyrillic;
    const Font *unit_font_ascii = is_air_unit ? &font_12_ascii : &font_16_ascii;
    const int unit_y_offset = is_air_unit ? 16 : 12;

    uint16_t num_w = Paint_GetStringWidth_Display(number_part, &Font24, &font_32_cyrillic, &font_32_ascii);
    uint16_t unit_w = Paint_GetStringWidth_Display(unit_part, unit_font, unit_font_cyr, unit_font_ascii);
    const int gap = is_air_unit ? 1 : 0;
    int start_x = cx - (int)(num_w + unit_w + gap) / 2;

    Paint_DrawString_Display(start_x, y, number_part,
                             &Font24, &font_32_cyrillic, &font_32_ascii, WHITE, BLACK);
    // Keep the unit aligned near the bottom of the large number.
    Paint_DrawString_Display(start_x + (int)num_w + gap, y + unit_y_offset, unit_part,
                             unit_font, unit_font_cyr, unit_font_ascii, WHITE, BLACK);
}

static void drawDetailCircle(int cx, int cy, int radius,
                             const char *title,
                             const char *primary,
                             const char *secondary,
                             const char *extra,
                             const char *status,
                             const float *trend, int trend_count) {
    (void)trend;
    (void)trend_count;
    Paint_DrawCircle(cx, cy, radius, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(cx, cy, radius - 6, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    // Arc is extended slightly to the outside to reduce visible white gaps.
    drawArcBand(cx, cy, radius + 2, radius - 8, 150.0f, 220.0f, 1.0f);
    // Re-draw borders for crisp ring alignment after arc fill.
    Paint_DrawCircle(cx, cy, radius, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(cx, cy, radius - 6, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    const int title_y = cy - radius + 26;
    const int top_line_y = title_y + 14;
    uint16_t title_w = Paint_GetStringWidth_Display(title, &Font12, &font_12_cyrillic, &font_12_ascii);
    Paint_DrawString_Display(cx - (int)title_w / 2, title_y, title,
                             &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    Paint_DrawLine(cx - radius + 16, top_line_y, cx + radius - 16, top_line_y,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    int primary_y = cy - 28;
    // Noise circles have no secondary label line, so place value a bit lower.
    if (!secondary || secondary[0] == '\0') {
        primary_y = cy - 22;
    }
    drawPrimaryValueMixed(cx, primary_y, primary);
    uint16_t secondary_w = Paint_GetStringWidth_Display(secondary, &Font12, &font_12_cyrillic, &font_12_ascii);
    Paint_DrawString_Display(cx - (int)secondary_w / 2, cy + 14, secondary,
                             &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    if (extra && extra[0] != '\0') {
        uint16_t extra_w = Paint_GetStringWidth_Display(extra, &Font12, &font_12_cyrillic, &font_12_ascii);
        Paint_DrawString_Display(cx - (int)extra_w / 2, cy + 26, extra,
                                 &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    }
    Paint_DrawLine(cx - radius + 22, cy + 40, cx + radius - 22, cy + 40,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    uint16_t status_w = Paint_GetStringWidth_Display(status, &Font12, &font_12_cyrillic, &font_12_ascii);
    Paint_DrawString_Display(cx - (int)status_w / 2, cy + 46, status,
                             &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
}

static CategorySummary summarizeClimate(const analytics_screen_values_t &values) {
    int score_sum = 0;
    int count = 0;
    float v = 0.0f;
    if (periodValue(values.temp_indoor, v)) {
        score_sum += metricScoreRangeBest(v, 20.0f, 25.0f, 10.0f, 32.0f);
        count++;
    }
    if (periodValue(values.hum_indoor, v)) {
        score_sum += metricScoreRangeBest(v, 40.0f, 60.0f, 20.0f, 80.0f);
        count++;
    }
    if (periodValue(values.dew_indoor, v)) {
        score_sum += metricScoreLinearHighWorse(v, 12.0f, 22.0f);
        count++;
    }
    CategorySummary out = {"Climate", 0, count > 0};
    out.score = (count > 0) ? (score_sum / count) : 0;
    return out;
}

static CategorySummary summarizeAir(const analytics_screen_values_t &values) {
    int score_sum = 0;
    int count = 0;
    float v = 0.0f;
    if (periodValue(values.pm10, v)) {
        score_sum += metricScoreLinearHighWorse(v, 45.0f, 180.0f);
        count++;
    }
    if (periodValue(values.pm25, v)) {
        score_sum += metricScoreLinearHighWorse(v, 15.0f, 80.0f);
        count++;
    }
    CategorySummary out = {"Air", 0, count > 0};
    out.score = (count > 0) ? (score_sum / count) : 0;
    return out;
}

static CategorySummary summarizeCO2(const analytics_screen_values_t &values) {
    float v = 0.0f;
    if (!periodValue(values.co2, v)) {
        return {"CO2", 0, false};
    }
    return {"CO2", metricScoreLinearHighWorse(v, 900.0f, 1800.0f), true};
}

static CategorySummary summarizeNoise(const analytics_screen_values_t &values) {
    float v = 0.0f;
    if (!periodValue(values.noise_avg, v)) {
        return {"Noise", 0, false};
    }
    return {"Noise", metricScoreLinearHighWorse(v, 50.0f, 85.0f), true};
}

static uint16_t periodToDays(analytics_period_t period) {
    if (period == analytics_period_t::P7D) return 7;
    if (period == analytics_period_t::P30D) return 30;
    return 1;
}

static const char* periodLabel(analytics_period_t period) {
    if (period == analytics_period_t::P7D) return "7D";
    if (period == analytics_period_t::P30D) return "30D";
    return "24H";
}

static void formatMetricOrDash(char *out, size_t out_sz, const analytics_metric_t &metric, uint8_t precision) {
    if (!metric.has_current) {
        snprintf(out, out_sz, "--");
        return;
    }
    stringFromFloat(out, metric.current, precision);
}

#if defined(USE_SD_CARD)
// Individual detail pages intentionally avoid direct SD scans in render path
// to keep page switching stable on device.
#endif
} // namespace

analytics_period_t analyticsGetPeriod() {
    return g_analytics_period;
}

void analyticsSetPeriod(analytics_period_t period) {
    g_analytics_period = period;
    g_analytics_view = analytics_view_t::OVERVIEW_24H;
}

void analyticsNextPeriod() {
    if (g_analytics_period == analytics_period_t::P24H) g_analytics_period = analytics_period_t::P7D;
    else if (g_analytics_period == analytics_period_t::P7D) g_analytics_period = analytics_period_t::P30D;
    else g_analytics_period = analytics_period_t::P24H;
}

void analyticsPrevPeriod() {
    if (g_analytics_period == analytics_period_t::P24H) g_analytics_period = analytics_period_t::P30D;
    else if (g_analytics_period == analytics_period_t::P30D) g_analytics_period = analytics_period_t::P7D;
    else g_analytics_period = analytics_period_t::P24H;
}

bool analyticsNextPeriodAtEdge() {
    if (g_analytics_period == analytics_period_t::P30D) {
        return true;
    }
    analyticsNextPeriod();
    return false;
}

bool analyticsPrevPeriodAtEdge() {
    if (g_analytics_period == analytics_period_t::P24H) {
        return true;
    }
    analyticsPrevPeriod();
    return false;
}

const char* analyticsPeriodLabel() {
    return periodLabel(g_analytics_period);
}

bool analyticsHistoryPersistenceEnabled() {
    return true;
}

bool analyticsHistoryIsLoaded() {
    loadRollingHistoryIfNeeded();
    return g_hist_loaded;
}

bool analyticsHistoryHasData() {
    loadRollingHistoryIfNeeded();
    return rollingHistoryHasAnyData(g_temp_hist) ||
           rollingHistoryHasAnyData(g_hum_hist) ||
           rollingHistoryHasAnyData(g_dew_hist) ||
           rollingHistoryHasAnyData(g_pm10_hist) ||
           rollingHistoryHasAnyData(g_pm25_hist) ||
           rollingHistoryHasAnyData(g_co2_hist) ||
           rollingHistoryHasAnyData(g_noise_hist);
}

analytics_view_t analyticsGetView() {
    return g_analytics_view;
}

analytics_period_t analyticsGetViewPeriod() {
    return analytics_period_t::P24H;
}

bool analyticsNextViewAtEdge() {
    if (g_analytics_view == analytics_view_t::OVERVIEW_24H) {
        g_analytics_view = analytics_view_t::CLIMATE_24H;
        return false;
    }
    if (g_analytics_view == analytics_view_t::CLIMATE_24H) {
        g_analytics_view = analytics_view_t::CO2_24H;
        return false;
    }
    if (g_analytics_view == analytics_view_t::CO2_24H) {
        g_analytics_view = analytics_view_t::AIR_24H;
        return false;
    }
    if (g_analytics_view == analytics_view_t::AIR_24H) {
        g_analytics_view = analytics_view_t::NOISE_24H;
        return false;
    }
    return true;
}

bool analyticsPrevViewAtEdge() {
    if (g_analytics_view == analytics_view_t::NOISE_24H) {
        g_analytics_view = analytics_view_t::AIR_24H;
        return false;
    }
    if (g_analytics_view == analytics_view_t::AIR_24H) {
        g_analytics_view = analytics_view_t::CO2_24H;
        return false;
    }
    if (g_analytics_view == analytics_view_t::CO2_24H) {
        g_analytics_view = analytics_view_t::CLIMATE_24H;
        return false;
    }
    if (g_analytics_view == analytics_view_t::CLIMATE_24H) {
        g_analytics_view = analytics_view_t::OVERVIEW_24H;
        return false;
    }
    return true;
}

const char* analyticsViewLabel() {
    switch (g_analytics_view) {
        case analytics_view_t::CLIMATE_24H: return A_TXT("Climate", "Климат");
        case analytics_view_t::CO2_24H: return "CO2";
        case analytics_view_t::AIR_24H: return A_TXT("Air", "Воздух");
        case analytics_view_t::NOISE_24H: return A_TXT("Noise", "Шум");
        case analytics_view_t::OVERVIEW_24H:
        default: return A_TXT("24H", "24ч");
    }
}

static const char* analyticsViewLabelTitle() {
#if defined(INTL_RU)
    switch (g_analytics_view) {
        case analytics_view_t::CLIMATE_24H: return "климата";
        case analytics_view_t::CO2_24H: return "CO2";
        case analytics_view_t::AIR_24H: return "воздуха";
        case analytics_view_t::NOISE_24H: return "шума";
        case analytics_view_t::OVERVIEW_24H:
        default: return "24ч";
    }
#else
    return analyticsViewLabel();
#endif
}

void extractAnalyticsScreenValues(const JsonDocument &doc, analytics_screen_values_t &values) {
    loadRollingHistoryIfNeeded();

    values = analytics_screen_values_t{};
    JsonObjectConst data = doc.as<JsonObjectConst>();
    String urban_key = ATRUIST_URBAN_SENSOR;

    updateMetrics();
    bool use_bme680_for_temp_hum = (system_metrics.uptime_sec < 360);

    if (data.containsKey("SCD4x")) {
        JsonObjectConst scd = data["SCD4x"].as<JsonObjectConst>();
        if (scd.containsKey("co2")) {
            float co2 = scd["co2"]["value"].as<float>();
            if (isValidCO2(co2)) {
                values.co2.current = co2;
                values.co2.has_current = true;
            }
        }
        if (!use_bme680_for_temp_hum) {
            if (scd.containsKey("temperature")) {
                float t = scd["temperature"]["value"].as<float>();
                if (isValidTemp(t)) {
                    values.temp_indoor.current = t;
                    values.temp_indoor.has_current = true;
                }
            }
            if (scd.containsKey("humidity")) {
                float h = scd["humidity"]["value"].as<float>();
                if (isValidHumidity(h)) {
                    values.hum_indoor.current = h;
                    values.hum_indoor.has_current = true;
                }
            }
        }
    }

    if (data.containsKey("BME680")) {
        JsonObjectConst bme = data["BME680"].as<JsonObjectConst>();
        if (use_bme680_for_temp_hum) {
            if (bme.containsKey("temperature")) {
                float t = bme["temperature"]["value"].as<float>();
                if (isValidTemp(t)) {
                    values.temp_indoor.current = t;
                    values.temp_indoor.has_current = true;
                }
            }
            if (bme.containsKey("humidity")) {
                float h = bme["humidity"]["value"].as<float>();
                if (isValidHumidity(h)) {
                    values.hum_indoor.current = h;
                    values.hum_indoor.has_current = true;
                }
            }
        }
    }

    if (data.containsKey(urban_key)) {
        JsonObjectConst urban = data[urban_key].as<JsonObjectConst>();
        if (urban.containsKey("BME280_temperature")) {
            float t_urban = urban["BME280_temperature"]["value"].as<float>();
            if (isValidTemp(t_urban)) {
                values.temp_urban.current = t_urban;
                values.temp_urban.has_current = true;
            }
        }
        if (urban.containsKey("BME280_humidity")) {
            float h_urban = urban["BME280_humidity"]["value"].as<float>();
            if (isValidHumidity(h_urban)) {
                values.hum_urban.current = h_urban;
                values.hum_urban.has_current = true;
            }
        }
        if (urban.containsKey("SDS_P1")) {
            float pm10 = urban["SDS_P1"]["value"].as<float>();
            if (isValidPM10(pm10)) {
                values.pm10.current = pm10;
                values.pm10.has_current = true;
            }
        }
        if (urban.containsKey("SDS_P2")) {
            float pm25 = urban["SDS_P2"]["value"].as<float>();
            if (isValidPM25(pm25)) {
                values.pm25.current = pm25;
                values.pm25.has_current = true;
            }
        }
        if (urban.containsKey("PCBA_noiseAvg")) {
            float noise = urban["PCBA_noiseAvg"]["value"].as<float>();
            if (isValidNoise(noise)) {
                values.noise_avg.current = noise;
                values.noise_avg.has_current = true;
            }
        }
    }

    // Insight local PM values (if present in local SDS011 payload).
    if (data.containsKey("SDS011")) {
        JsonObjectConst sds = data["SDS011"].as<JsonObjectConst>();
        if (sds.containsKey("P1")) {
            float pm10_i = sds["P1"]["value"].as<float>();
            if (isValidPM10(pm10_i)) {
                values.pm10_insight.current = pm10_i;
                values.pm10_insight.has_current = true;
            }
        }
        if (sds.containsKey("P2")) {
            float pm25_i = sds["P2"]["value"].as<float>();
            if (isValidPM25(pm25_i)) {
                values.pm25_insight.current = pm25_i;
                values.pm25_insight.has_current = true;
            }
        }
    }

    if (values.temp_indoor.has_current && values.hum_indoor.has_current) {
        float dew = computeDewPoint(values.temp_indoor.current, values.hum_indoor.current);
        if (isValidTemp(dew)) {
            values.dew_indoor.current = dew;
            values.dew_indoor.has_current = true;
        }
    }

    // Update low-memory rolling aggregates (30 day buckets).
    time_t now = time(nullptr);
    bool updated = false;
    if (now > 0) {
        uint32_t day_key = currentLocalDayKey(now);
        if (values.temp_indoor.has_current) {
            updateRollingMetric(g_temp_hist, values.temp_indoor.current, day_key);
            updated = true;
        }
        if (values.hum_indoor.has_current) {
            updateRollingMetric(g_hum_hist, values.hum_indoor.current, day_key);
            updated = true;
        }
        if (values.dew_indoor.has_current) {
            updateRollingMetric(g_dew_hist, values.dew_indoor.current, day_key);
            updated = true;
        }
        if (values.pm10.has_current) {
            updateRollingMetric(g_pm10_hist, values.pm10.current, day_key);
            updated = true;
        }
        if (values.pm25.has_current) {
            updateRollingMetric(g_pm25_hist, values.pm25.current, day_key);
            updated = true;
        }
        if (values.co2.has_current) {
            updateRollingMetric(g_co2_hist, values.co2.current, day_key);
            updated = true;
        }
        if (values.noise_avg.has_current) {
            updateRollingMetric(g_noise_hist, values.noise_avg.current, day_key);
            updated = true;
        }

        if (updated) {
            g_hist_dirty = true;
            const bool day_changed = (g_hist_last_day_key != 0U && g_hist_last_day_key != day_key);
            g_hist_last_day_key = day_key;
            saveRollingHistoryIfNeeded(day_changed);
        }
    }
}

void populateAnalyticsPeriodStats(analytics_screen_values_t &values, analytics_period_t period) {
    loadRollingHistoryIfNeeded();

    const uint16_t days = periodToDays(period);
    fillMetricFromRolling(values.temp_indoor, g_temp_hist, days);
    fillMetricFromRolling(values.hum_indoor, g_hum_hist, days);
    fillMetricFromRolling(values.dew_indoor, g_dew_hist, days);
    fillMetricFromRolling(values.pm10, g_pm10_hist, days);
    fillMetricFromRolling(values.pm25, g_pm25_hist, days);
    fillMetricFromRolling(values.co2, g_co2_hist, days);
    fillMetricFromRolling(values.noise_avg, g_noise_hist, days);
}

void showAnalyticsPage(UBYTE *BlackImage, const analytics_screen_values_t &values,
                       const analytics_screen_values_t &values_7d,
                       const analytics_screen_values_t &values_30d,
                       analytics_period_t period, analytics_view_t view) {
    (void)BlackImage;
    (void)period;
    Paint_Clear(WHITE);

    struct tm timeinfo;
    const uint16_t header_top_y = 6;
    const uint16_t header_row_height = Font16.Height + 2;
    uint16_t header_bottom_border_y = header_top_y + header_row_height + 2;

    const uint16_t header_icon_size = 15;
    const uint16_t header_icon_x = 4;
    const uint16_t header_icon_y = header_top_y;
    Paint_DrawImage(chart_pie_15x15, header_icon_x, header_icon_y, header_icon_size, header_icon_size);

    if (getLocalTime(&timeinfo)) {
        char date_buf[12], time_buf[8];
        strftime(date_buf, sizeof(date_buf), "%m/%d/%Y", &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%H:%M", &timeinfo);

        int time_width = (int)Paint_GetStringWidth_Display(time_buf, &Font16, &font_16_cyrillic, &font_16_ascii);
        int time_x = (DISPLAY_WIDTH - time_width) / 2;
        Paint_DrawString_Display(time_x, header_top_y, time_buf, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

        int date_width = (int)Paint_GetStringWidth_Display(date_buf, &Font12, &font_12_cyrillic, &font_12_ascii);
        const int right_margin = 4;
        int date_x = DISPLAY_WIDTH - right_margin - date_width;
        int date_y = header_top_y + 2;
        Paint_DrawString_Display(date_x, date_y, date_buf, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        Paint_DrawString_Display(date_x + 1, date_y, date_buf, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    }

    char period_text[96];
    snprintf(period_text, sizeof(period_text), A_TXT("Analytics %s", "Аналитика %s"), analyticsViewLabelTitle());
    Paint_DrawString_Display(26, header_top_y + 2, period_text, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);

    Paint_DrawLine(0, header_bottom_border_y, DISPLAY_WIDTH, header_bottom_border_y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    const int sidebar_width = 26;
    const int content_left = 8;
    const int content_right = DISPLAY_WIDTH - sidebar_width - 6;
    const int content_width = content_right - content_left;
    const int content_top = header_bottom_border_y + 12;

    CategorySummary climate = summarizeClimate(values);
    CategorySummary co2 = summarizeCO2(values);
    CategorySummary air = summarizeAir(values);
    CategorySummary noise = summarizeNoise(values);

    auto drawOverview24hStyled = [&]() {
        const int radius = 63;
        const int x1 = content_left + content_width / 4;
        const int x2 = content_left + (content_width * 3) / 4;
        const int y1 = content_top + 63;
        const int y2 = content_top + 193;

        char t_buf[12], h_buf[12], dew_buf[12], t_u_buf[12], h_u_buf[12], co2_buf[12], pm_buf[12], pm10_buf[12], noise_buf[12];
        char pm10_i_buf[12], pm25_i_buf[12];
        formatMetricOrDash(t_buf, sizeof(t_buf), values.temp_indoor, 1);
        formatMetricOrDash(h_buf, sizeof(h_buf), values.hum_indoor, 0);
        formatMetricOrDash(dew_buf, sizeof(dew_buf), values.dew_indoor, 1);
        formatMetricOrDash(t_u_buf, sizeof(t_u_buf), values.temp_urban, 1);
        formatMetricOrDash(h_u_buf, sizeof(h_u_buf), values.hum_urban, 0);
        formatMetricOrDash(co2_buf, sizeof(co2_buf), values.co2, 0);
        formatMetricOrDash(pm_buf, sizeof(pm_buf), values.pm25, 1);
        formatMetricOrDash(pm10_buf, sizeof(pm10_buf), values.pm10, 1);
        formatMetricOrDash(pm10_i_buf, sizeof(pm10_i_buf), values.pm10_insight, 1);
        formatMetricOrDash(pm25_i_buf, sizeof(pm25_i_buf), values.pm25_insight, 1);
        formatMetricOrDash(noise_buf, sizeof(noise_buf), values.noise_avg, 0);

        char climate_primary[24], climate_secondary[32];
        char air_primary[32], air_secondary[40];
        char co2_primary[24], co2_secondary[24];
        char noise_primary[24], noise_secondary[24];
        // Common 4-circle screen: Insight climate only.
        snprintf(climate_primary, sizeof(climate_primary), "%sC/%s%%", t_buf, h_buf);
        snprintf(climate_secondary, sizeof(climate_secondary), A_TXT("Dew point - %sC", "Точка росы - %sC"), dew_buf);
        snprintf(air_primary, sizeof(air_primary), "%sug/m3", pm10_buf);
        snprintf(air_secondary, sizeof(air_secondary), "PM2.5 - %sug/m3", pm_buf);
        snprintf(co2_primary, sizeof(co2_primary), "%sppm", co2_buf);
        snprintf(co2_secondary, sizeof(co2_secondary), A_TXT("Carbon dioxide", "Углекислый газ"));
        snprintf(noise_primary, sizeof(noise_primary), "%sdB", noise_buf);
        snprintf(noise_secondary, sizeof(noise_secondary), A_TXT("Max noise - %sdB", "Макс шум - %sdB"), noise_buf);

        auto scoreFill = [](const CategorySummary &s) -> float {
            if (!s.has_data) return 0.0f;
            const int score = s.score;
            // Visual mapping by quality bands so "Good/Comfortable" looks clearly fuller.
            if (score >= 75) {
                // 75..100 -> 0.82..1.00
                float t = (float)(score - 75) / 25.0f;
                float f = 0.82f + t * 0.18f;
                if (f > 1.0f) f = 1.0f;
                return f;
            }
            if (score >= 45) {
                // 45..74 -> 0.52..0.81
                float t = (float)(score - 45) / 30.0f;
                return 0.52f + t * 0.29f;
            }
            // 0..44 -> 0.12..0.51
            float t = (float)score / 45.0f;
            return 0.12f + t * 0.39f;
        };
        drawPanelCircle(x1, y1, radius,
                        A_TXT("CLIMATE", "КЛИМАТ"),
                        climate_primary,
                        climate_secondary,
                        qualityByScore(climate.score, A_TXT("Good", "Хорошо"), A_TXT("Moderate", "Средне"), A_TXT("Unstable", "Нестаб.")),
                        scoreFill(climate));

        drawPanelCircle(x2, y1, radius,
                        A_TXT("AIR QUALITY", "ВОЗДУХ"),
                        air_primary,
                        air_secondary,
                        qualityByScore(air.score, A_TXT("Good Air", "Хорошо"), A_TXT("Moderate Air", "Средне"), A_TXT("Poor Air", "Плохо")),
                        scoreFill(air));

        drawPanelCircle(x1, y2, radius,
                        A_TXT("CO2 LEVEL", "CO2"),
                        co2_primary,
                        co2_secondary,
                        qualityByScore(co2.score, A_TXT("Normal", "Норма"), A_TXT("Elevated", "Выше"), A_TXT("High CO2", "Высоко")),
                        scoreFill(co2));

        drawPanelCircle(x2, y2, radius,
                        A_TXT("NOISE LEVEL", "ШУМ"),
                        noise_primary,
                        noise_secondary,
                        qualityByScore(noise.score, A_TXT("Low Noise", "Тихо"), A_TXT("Moderate", "Средне"), A_TXT("Noisy", "Шумно")),
                        scoreFill(noise));
    };

    auto drawDetailDual = [&](const char *page_title,
                              const char *left_primary, const char *left_secondary, const char *left_extra, const char *left_status,
                              const char *right_primary, const char *right_secondary, const char *right_extra, const char *right_status,
                              const char *left_rec_title, const char *left_rec_line1, const char *left_rec_line2,
                              const char *right_rec_title, const char *right_rec_line1, const char *right_rec_line2,
                              const char *tip_line) {
        (void)page_title;
        const int radius = 77;
        const int x1 = content_left + content_width / 4;
        const int x2 = content_left + (content_width * 3) / 4;
        const int y = content_top + 74;

        drawDetailCircle(x1, y, radius, A_TXT("LAST 7 DAYS", "7 ДНЕЙ"), left_primary, left_secondary, left_extra, left_status, nullptr, 0);
        drawDetailCircle(x2, y, radius, A_TXT("LAST 30 DAYS", "30 ДНЕЙ"), right_primary, right_secondary, right_extra, right_status, nullptr, 0);

        const int box_top = content_top + 158;
        const int box_h = 68;
        const int gap = 8;
        const int box_w = (content_width - gap) / 2;
        const int left_box_x = content_left;
        const int right_box_x = content_left + box_w + gap;
        Paint_DrawRectangle(left_box_x, box_top, left_box_x + box_w, box_top + box_h, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
        Paint_DrawRectangle(right_box_x, box_top, right_box_x + box_w, box_top + box_h, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

        const int text_max_w = box_w - 12;
        drawTextBold(left_box_x + 6, box_top + 4, left_rec_title, &Font12);
        drawTextBold(right_box_x + 6, box_top + 4, right_rec_title, &Font12);
        char left_text[180];
        char right_text[180];
        snprintf(left_text, sizeof(left_text), "%s %s", left_rec_line1, left_rec_line2);
        snprintf(right_text, sizeof(right_text), "%s %s", right_rec_line1, right_rec_line2);
        drawTextWrapped(left_box_x + 6, box_top + 20, text_max_w, left_text, &Font12, 4);
        drawTextWrapped(right_box_x + 6, box_top + 20, text_max_w, right_text, &Font12, 4);

        Paint_DrawLine(content_left, content_top + 232, content_right, content_top + 232,
                       BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        uint16_t tip_w = Paint_GetStringWidth_Display(tip_line, &Font12, &font_12_cyrillic, &font_12_ascii);
        int tip_x = content_left + (content_width - (int)tip_w) / 2;
        if (tip_x < content_left + 2) tip_x = content_left + 2;
        Paint_DrawString_Display(tip_x, content_top + 238, tip_line,
                                 &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    };

    if (view == analytics_view_t::OVERVIEW_24H) {
#ifdef DEV
        {
            char line1[180];
            snprintf(line1, sizeof(line1),
                     "hist: persist=%s loaded=%s has_data=%s",
                     analyticsHistoryPersistenceEnabled() ? "yes" : "no",
                     analyticsHistoryIsLoaded() ? "yes" : "no",
                     analyticsHistoryHasData() ? "yes" : "no");
            debug_outln_info(F("[Analytics] "), String(line1));
            char first_save[24];
            char last_save[24];
            formatLogTs(first_save, sizeof(first_save), g_hist_first_save_ts);
            formatLogTs(last_save, sizeof(last_save), g_hist_last_save_ts);
            char line1b[220];
            snprintf(line1b, sizeof(line1b),
                     "save: first=%s last=%s count=%lu last_reason=%s policy=%lus|day-change",
                     first_save, last_save, (unsigned long)g_hist_save_count,
                     g_hist_last_save_forced ? "day-change" : "interval",
                     (unsigned long)(kAnalyticsPersistIntervalMs / 1000UL));
            debug_outln_info(F("[Analytics] "), String(line1b));

            const uint16_t climate7 = countCoverageDays(g_temp_hist, 7);
            const uint16_t climate30 = countCoverageDays(g_temp_hist, 30);
            const uint16_t co27 = countCoverageDays(g_co2_hist, 7);
            const uint16_t co230 = countCoverageDays(g_co2_hist, 30);
            const uint16_t air7 = countCoverageDays(g_pm25_hist, 7);
            const uint16_t air30 = countCoverageDays(g_pm25_hist, 30);
            const uint16_t noise7 = countCoverageDays(g_noise_hist, 7);
            const uint16_t noise30 = countCoverageDays(g_noise_hist, 30);
            const uint16_t temp7 = countCoverageDays(g_temp_hist, 7);
            const uint16_t temp30 = countCoverageDays(g_temp_hist, 30);
            const uint16_t hum7 = countCoverageDays(g_hum_hist, 7);
            const uint16_t hum30 = countCoverageDays(g_hum_hist, 30);
            const uint16_t dew7 = countCoverageDays(g_dew_hist, 7);
            const uint16_t dew30 = countCoverageDays(g_dew_hist, 30);
            const uint16_t pm10_7 = countCoverageDays(g_pm10_hist, 7);
            const uint16_t pm10_30 = countCoverageDays(g_pm10_hist, 30);
            const uint16_t pm25_7 = countCoverageDays(g_pm25_hist, 7);
            const uint16_t pm25_30 = countCoverageDays(g_pm25_hist, 30);

            auto pct = [](uint16_t v, uint16_t total) -> uint16_t {
                if (total == 0) return 0;
                return (uint16_t)((100U * (uint32_t)v) / (uint32_t)total);
            };

            char line2[220];
            snprintf(line2, sizeof(line2),
                     "coverage(cat): climate=%u/7(%u%%),%u/30(%u%%) co2=%u/7(%u%%),%u/30(%u%%) air=%u/7(%u%%),%u/30(%u%%) noise=%u/7(%u%%),%u/30(%u%%)",
                     climate7, pct(climate7, 7), climate30, pct(climate30, 30),
                     co27, pct(co27, 7), co230, pct(co230, 30),
                     air7, pct(air7, 7), air30, pct(air30, 30),
                     noise7, pct(noise7, 7), noise30, pct(noise30, 30));
            debug_outln_info(F("[Analytics] "), String(line2));

            char line3[220];
            snprintf(line3, sizeof(line3),
                     "coverage(metric): temp=%u/7,%u/30 hum=%u/7,%u/30 dew=%u/7,%u/30 pm10=%u/7,%u/30 pm25=%u/7,%u/30 co2=%u/7,%u/30 noise=%u/7,%u/30",
                     temp7, temp30, hum7, hum30, dew7, dew30,
                     pm10_7, pm10_30, pm25_7, pm25_30,
                     co27, co230, noise7, noise30);
            debug_outln_info(F("[Analytics] "), String(line3));
        }
#endif
        drawOverview24hStyled();
    } else if (view == analytics_view_t::CLIMATE_24H) {
        char t7[12] = "--", h7[12] = "--", d7[12] = "--";
        char t30[12] = "--", h30[12] = "--", d30[12] = "--";
        uint16_t cov7 = countCoverageDays(g_temp_hist, 7), cov30 = countCoverageDays(g_temp_hist, 30);
        if (values_7d.temp_indoor.has_24h) {
            stringFromFloat(t7, values_7d.temp_indoor.avg24h, 1);
        }
        if (values_7d.hum_indoor.has_24h) {
            stringFromFloat(h7, values_7d.hum_indoor.avg24h, 0);
        }
        if (values_7d.dew_indoor.has_24h) {
            stringFromFloat(d7, values_7d.dew_indoor.avg24h, 1);
        }
        if (values_30d.temp_indoor.has_24h) {
            stringFromFloat(t30, values_30d.temp_indoor.avg24h, 1);
        }
        if (values_30d.hum_indoor.has_24h) {
            stringFromFloat(h30, values_30d.hum_indoor.avg24h, 0);
        }
        if (values_30d.dew_indoor.has_24h) {
            stringFromFloat(d30, values_30d.dew_indoor.avg24h, 1);
        }
        char p7[32], s7[48], st7[40], p30[24], s30[28], st30[40], r1[112], r2[112], r3[112], r4[112], wk_title[32], mo_title[32];
        snprintf(p7, sizeof(p7), "%sC", t7);
        {
            char tu[12] = "--", hu[12] = "--";
            formatMetricOrDash(tu, sizeof(tu), values.temp_urban, 1);
            formatMetricOrDash(hu, sizeof(hu), values.hum_urban, 0);
            snprintf(s7, sizeof(s7), A_TXT("RH %s%% / Dew: %sC", "RH %s%% / Т.росы: %sC"), h7, d7);
            snprintf(st7, sizeof(st7), A_TXT("Urban: %sC %s%%", "Улица: %sC %s%%"), tu, hu);
        }
        snprintf(p30, sizeof(p30), "%sC", t30);
        snprintf(s30, sizeof(s30), A_TXT("RH %s%% / Dew: %sC", "RH %s%% / Т.росы: %sC"), h30, d30);
        {
            char tu[12] = "--", hu[12] = "--";
            formatMetricOrDash(tu, sizeof(tu), values.temp_urban, 1);
            formatMetricOrDash(hu, sizeof(hu), values.hum_urban, 0);
            snprintf(st30, sizeof(st30), A_TXT("Urban: %sC %s%%", "Улица: %sC %s%%"), tu, hu);
        }
        snprintf(r1, sizeof(r1), A_TXT("Average temperature was %sC with %s%% humidity.", "Средняя температура была %sC при влажности %s%%."), t7, h7);
        snprintf(r2, sizeof(r2), A_TXT("Average dew point was %sC.", "Средняя точка росы была %sC."), d7);
        snprintf(r3, sizeof(r3), A_TXT("Average temperature was %sC with %s%% humidity.", "Средняя температура была %sC при влажности %s%%."), t30, h30);
        snprintf(r4, sizeof(r4), A_TXT("Average dew point was %sC.", "Средняя точка росы была %sC."), d30);
        snprintf(wk_title, sizeof(wk_title), A_TXT("Weekly overview", "Обзор недели"));
        snprintf(mo_title, sizeof(mo_title), A_TXT("Monthly trends", "Обзор месяца"));
        if (cov7 == 0) snprintf(r2, sizeof(r2), A_TXT("Coverage: no data yet.", "Покрытие: нет данных."));
        else snprintf(r2, sizeof(r2), A_TXT("Coverage: %u/7.", "Покрытие: %u/7."), cov7);
        if (cov30 == 0) snprintf(r4, sizeof(r4), A_TXT("Coverage: no data yet.", "Покрытие: нет данных."));
        else snprintf(r4, sizeof(r4), A_TXT("Coverage: %u/30.", "Покрытие: %u/30."), cov30);
        if (cov30 == 0) {
            snprintf(p30, sizeof(p30), "No data");
            snprintf(s30, sizeof(s30), "--");
        }
        const char *tip_line = nullptr;
        if (cov7 == 0 && cov30 == 0) {
            tip_line = A_TXT("Tip: collect data for trend insights.", "Совет: накопите данные для трендов.");
        } else if (climate.score < 45) {
            tip_line = A_TXT("Tip: ventilate and keep RH near 40-60%.", "Совет: проветривайте, RH держите 40-60%.");
        } else if (climate.score < 75) {
            tip_line = A_TXT("Tip: check temp and humidity daily.", "Совет: проверяйте температуру и RH ежедневно.");
        } else {
            tip_line = A_TXT("Tip: climate is stable, keep current routine.", "Совет: климат стабилен, сохраняйте режим.");
        }
        drawDetailDual("CLIMATE ANALYSIS",
                       p7, s7, st7, qualityByScore(climate.score, A_TXT("Comfortable", "Комфорт"), A_TXT("Moderate", "Средне"), A_TXT("Unstable", "Нестаб.")),
                       p30, s30, st30, qualityByScore(climate.score, A_TXT("Comfortable", "Комфорт"), A_TXT("Moderate", "Средне"), A_TXT("Unstable", "Нестаб.")),
                       wk_title, r1, r2,
                       mo_title, r3, r4,
                       tip_line);
    } else if (view == analytics_view_t::CO2_24H) {
        char c7[12] = "--", c30[12] = "--";
        uint16_t cov7 = countCoverageDays(g_co2_hist, 7), cov30 = countCoverageDays(g_co2_hist, 30);
        if (values_7d.co2.has_24h) {
            stringFromFloat(c7, values_7d.co2.avg24h, 0);
        }
        if (values_30d.co2.has_24h) {
            stringFromFloat(c30, values_30d.co2.avg24h, 0);
        }
        char p7[24], p30[24], r1[112], r2[112], r3[112], r4[112], wk_title[32], mo_title[32];
        snprintf(p7, sizeof(p7), "%sppm", c7);
        snprintf(p30, sizeof(p30), "%sppm", c30);
        snprintf(r1, sizeof(r1), A_TXT("Average carbon dioxide was %s ppm this week.", "Средний уровень углекислого газа за неделю составил %s ppm."), c7);
        snprintf(r2, sizeof(r2), A_TXT("Higher values may indicate weaker ventilation.", "Более высокие значения могут указывать на слабую вентиляцию."));
        snprintf(r3, sizeof(r3), A_TXT("Average carbon dioxide was %s ppm this month.", "Средний уровень углекислого газа за месяц составил %s ppm."), c30);
        snprintf(r4, sizeof(r4), A_TXT("Use this value as your indoor baseline.", "Используйте это значение как базовый ориентир для помещения."));
        snprintf(wk_title, sizeof(wk_title), A_TXT("Weekly overview", "Обзор недели"));
        snprintf(mo_title, sizeof(mo_title), A_TXT("Monthly trends", "Обзор месяца"));
        if (cov7 == 0) snprintf(r2, sizeof(r2), A_TXT("Coverage: no data yet.", "Покрытие: нет данных."));
        else snprintf(r2, sizeof(r2), A_TXT("Coverage: %u/7.", "Покрытие: %u/7."), cov7);
        if (cov30 == 0) snprintf(r4, sizeof(r4), A_TXT("Coverage: no data yet.", "Покрытие: нет данных."));
        else snprintf(r4, sizeof(r4), A_TXT("Coverage: %u/30.", "Покрытие: %u/30."), cov30);
        if (cov30 == 0) {
            snprintf(p30, sizeof(p30), "No data");
        }
        const char *tip_line = nullptr;
        if (cov7 == 0 && cov30 == 0) {
            tip_line = A_TXT("Tip: collect data to estimate CO2 baseline.", "Совет: накопите данные для базового CO2.");
        } else if (co2.score < 45) {
            tip_line = A_TXT("Tip: open windows when CO2 remains elevated.", "Совет: проветривайте при повышенном CO2.");
        } else {
            tip_line = A_TXT("Tip: maintain regular airflow in the room.", "Совет: поддерживайте регулярное проветривание.");
        }
        drawDetailDual("CO2 ANALYSIS",
                       p7, A_TXT("Carbon dioxide", "Углекислый газ"), nullptr, qualityByScore(co2.score, A_TXT("Normal", "Норма"), A_TXT("Elevated", "Выше"), A_TXT("High CO2", "Высоко")),
                       p30, A_TXT("Carbon dioxide", "Углекислый газ"), nullptr, qualityByScore(co2.score, A_TXT("Normal", "Норма"), A_TXT("Elevated", "Выше"), A_TXT("High CO2", "Высоко")),
                       wk_title, r1, r2,
                       mo_title, r3, r4,
                       tip_line);
    } else if (view == analytics_view_t::AIR_24H) {
        char pm10_7[12] = "--", pm10_30[12] = "--";
        char pm25_7[12] = "--", pm25_30[12] = "--";
        uint16_t cov7 = countCoverageDays(g_pm25_hist, 7), cov30 = countCoverageDays(g_pm25_hist, 30);
        if (values_7d.pm10.has_24h) {
            stringFromFloat(pm10_7, values_7d.pm10.avg24h, 1);
        }
        if (values_30d.pm10.has_24h) {
            stringFromFloat(pm10_30, values_30d.pm10.avg24h, 1);
        }
        if (values_7d.pm25.has_24h) {
            stringFromFloat(pm25_7, values_7d.pm25.avg24h, 1);
        }
        if (values_30d.pm25.has_24h) {
            stringFromFloat(pm25_30, values_30d.pm25.avg24h, 1);
        }
        char p7[24], s7[28], p30[24], s30[28], r1[112], r2[112], r3[112], r4[112], wk_title[32], mo_title[32];
        snprintf(p7, sizeof(p7), "%sug/m3", pm10_7);
        snprintf(s7, sizeof(s7), "PM2.5 - %s", pm25_7);
        snprintf(p30, sizeof(p30), "%sug/m3", pm10_30);
        snprintf(s30, sizeof(s30), "PM2.5 - %s", pm25_30);
        snprintf(r1, sizeof(r1), A_TXT("Average PM10 and PM2.5 were %s and %s ug/m3 this week.", "Средние PM10 и PM2.5 за неделю: %s и %s ug/m3."), pm10_7, pm25_7);
        snprintf(r2, sizeof(r2), A_TXT("Short pollution spikes are often easier to see weekly.", "Короткие всплески загрязнения обычно заметнее на недельном окне."));
        snprintf(r3, sizeof(r3), A_TXT("Average PM10 and PM2.5 were %s and %s ug/m3 this month.", "Средние PM10 и PM2.5 за месяц: %s и %s ug/m3."), pm10_30, pm25_30);
        snprintf(r4, sizeof(r4), A_TXT("Monthly values help track your long-term air baseline.", "Месячные значения помогают отслеживать долгосрочный базовый уровень качества воздуха."));
        snprintf(wk_title, sizeof(wk_title), A_TXT("Weekly overview", "Обзор недели"));
        snprintf(mo_title, sizeof(mo_title), A_TXT("Monthly trends", "Обзор месяца"));
        if (cov7 == 0) snprintf(r2, sizeof(r2), A_TXT("Coverage: no data yet.", "Покрытие: нет данных."));
        else snprintf(r2, sizeof(r2), A_TXT("Coverage: %u/7.", "Покрытие: %u/7."), cov7);
        if (cov30 == 0) snprintf(r4, sizeof(r4), A_TXT("Coverage: no data yet.", "Покрытие: нет данных."));
        else snprintf(r4, sizeof(r4), A_TXT("Coverage: %u/30.", "Покрытие: %u/30."), cov30);
        if (cov30 == 0) {
            snprintf(p30, sizeof(p30), "No data");
        }
        const char *tip_line = nullptr;
        if (cov7 == 0 && cov30 == 0) {
            tip_line = A_TXT("Tip: collect PM data to see pollution trends.", "Совет: накопите PM-данные для трендов.");
        } else if (air.score < 45) {
            tip_line = A_TXT("Tip: reduce indoor dust and ventilate more often.", "Совет: уменьшайте пыль и чаще проветривайте.");
        } else {
            tip_line = A_TXT("Tip: compare peaks with outdoor conditions.", "Совет: сравнивайте пики с условиями снаружи.");
        }
        drawDetailDual("AIR ANALYSIS",
                       p7, s7, nullptr, qualityByScore(air.score, A_TXT("Good Air", "Хорошо"), A_TXT("Moderate Air", "Средне"), A_TXT("Poor Air", "Плохо")),
                       p30, s30, nullptr, qualityByScore(air.score, A_TXT("Good Air", "Хорошо"), A_TXT("Moderate Air", "Средне"), A_TXT("Poor Air", "Плохо")),
                       wk_title, r1, r2,
                       mo_title, r3, r4,
                       tip_line);
    } else {
        char n7[12] = "--", n30[12] = "--";
        uint16_t cov7 = countCoverageDays(g_noise_hist, 7), cov30 = countCoverageDays(g_noise_hist, 30);
        if (values_7d.noise_avg.has_24h) {
            stringFromFloat(n7, values_7d.noise_avg.avg24h, 0);
        }
        if (values_30d.noise_avg.has_24h) {
            stringFromFloat(n30, values_30d.noise_avg.avg24h, 0);
        }
        char n7max[12] = "--", n30max[12] = "--";
        if (values_7d.noise_avg.has_24h) {
            stringFromFloat(n7max, values_7d.noise_avg.max24h, 0);
        }
        if (values_30d.noise_avg.has_24h) {
            stringFromFloat(n30max, values_30d.noise_avg.max24h, 0);
        }
        char s7[24], s30[24], p7[24], p30[24], r1[112], r2[112], r3[112], r4[112], wk_title[32], mo_title[32];
        snprintf(p7, sizeof(p7), "%sdB", n7);
        snprintf(p30, sizeof(p30), "%sdB", n30);
        snprintf(s7, sizeof(s7), "Max - %sdB", n7max);
        snprintf(s30, sizeof(s30), "Max - %sdB", n30max);
        snprintf(r1, sizeof(r1), A_TXT("Average noise was %s dB, maximum was %s dB.", "Средний шум был %s dB, максимум был %s dB."), n7, n7max);
        snprintf(r2, sizeof(r2), "Helps detect noisy days.");
        snprintf(r3, sizeof(r3), A_TXT("Average noise was %s dB, maximum was %s dB.", "Средний шум был %s dB, максимум был %s dB."), n30, n30max);
        snprintf(r4, sizeof(r4), "Good for longer quiet/noisy baseline.");
        snprintf(wk_title, sizeof(wk_title), A_TXT("Weekly overview", "Обзор недели"));
        snprintf(mo_title, sizeof(mo_title), A_TXT("Monthly trends", "Обзор месяца"));
        if (cov7 == 0) snprintf(r2, sizeof(r2), A_TXT("Coverage: no data yet.", "Покрытие: нет данных."));
        else snprintf(r2, sizeof(r2), A_TXT("Coverage: %u/7.", "Покрытие: %u/7."), cov7);
        if (cov30 == 0) snprintf(r4, sizeof(r4), A_TXT("Coverage: no data yet.", "Покрытие: нет данных."));
        else snprintf(r4, sizeof(r4), A_TXT("Coverage: %u/30.", "Покрытие: %u/30."), cov30);
        if (cov30 == 0) {
            snprintf(p30, sizeof(p30), "No data");
        }
        const char *tip_line = nullptr;
        if (cov7 == 0 && cov30 == 0) {
            tip_line = A_TXT("Tip: collect data to identify noisy periods.", "Совет: накопите данные по шумным периодам.");
        } else if (noise.score < 45) {
            tip_line = A_TXT("Tip: check recurring high-noise hours.", "Совет: отслеживайте повторяющиеся шумные часы.");
        } else {
            tip_line = A_TXT("Tip: monitor max noise spikes over time.", "Совет: следите за пиками максимального шума.");
        }
        drawDetailDual("NOISE ANALYSIS",
                       p7, s7, nullptr, qualityByScore(noise.score, A_TXT("Low Noise", "Тихо"), A_TXT("Moderate", "Средне"), A_TXT("Noisy", "Шумно")),
                       p30, s30, nullptr, qualityByScore(noise.score, A_TXT("Low Noise", "Тихо"), A_TXT("Moderate", "Средне"), A_TXT("Noisy", "Шумно")),
                       wk_title, r1, r2,
                       mo_title, r3, r4,
                       tip_line);
    }
}

#endif

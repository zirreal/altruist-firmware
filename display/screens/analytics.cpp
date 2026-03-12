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
#if defined(USE_SD_CARD) && defined(DEV)
#include "../../sd_card/sd_card.h"
extern SDCard sdCardLogger;
#endif
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
analytics_view_t g_analytics_view = analytics_view_t::OVERVIEW_24H;

#if defined(INTL_RU)
#define A_TXT(en, ru) ru
#else
#define A_TXT(en, ru) en
#endif

struct RollingHourBucket {
    uint32_t hour_key = 0; // local hour key (day_key*24 + hour)
    float sum_v = 0.0f;
    uint16_t count = 0;
};

struct RollingHourHistory {
    RollingHourBucket buckets[48]; // keep ~2 days of hourly bins
};

RollingHourHistory g_temp_hour_hist;
RollingHourHistory g_hum_hour_hist;
RollingHourHistory g_dew_hour_hist;
RollingHourHistory g_pm10_hour_hist;
RollingHourHistory g_pm25_hour_hist;
RollingHourHistory g_co2_hour_hist;
RollingHourHistory g_noise_hour_hist;

constexpr uint8_t kAnalyticsHistVersion = 4;
constexpr uint32_t kAnalyticsPersistIntervalMs = 60UL * 60UL * 1000UL; // 1 hour
bool g_hist_loaded = false;
bool g_hist_dirty = false;
uint32_t g_hist_last_save_ms = 0;
uint32_t g_hist_last_day_key = 0;
uint32_t g_hist_first_save_ts = 0;
uint32_t g_hist_last_save_ts = 0;
uint32_t g_hist_save_count = 0;
bool g_hist_last_save_forced = false;
#if defined(USE_SD_CARD) && defined(DEV)
uint32_t g_dev_sd_last_dump_hour_key = 0;
uint32_t g_dev_last_ingest_hour_key = 0;
static void dumpAnalyticsToSdDev(uint32_t now_ts, bool forced_save);
#endif
#if defined(DEV)
uint32_t g_dev_last_status_log_ms = 0;
#endif
static uint32_t currentLocalHourKey(time_t now);
static void formatHourKey(char *out, size_t out_sz, uint32_t hour_key);

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

    if (prefs.getBytesLength("h_temp") == sizeof(g_temp_hour_hist)) prefs.getBytes("h_temp", &g_temp_hour_hist, sizeof(g_temp_hour_hist));
    if (prefs.getBytesLength("h_hum") == sizeof(g_hum_hour_hist)) prefs.getBytes("h_hum", &g_hum_hour_hist, sizeof(g_hum_hour_hist));
    if (prefs.getBytesLength("h_dew") == sizeof(g_dew_hour_hist)) prefs.getBytes("h_dew", &g_dew_hour_hist, sizeof(g_dew_hour_hist));
    if (prefs.getBytesLength("h_pm10") == sizeof(g_pm10_hour_hist)) prefs.getBytes("h_pm10", &g_pm10_hour_hist, sizeof(g_pm10_hour_hist));
    if (prefs.getBytesLength("h_pm25") == sizeof(g_pm25_hour_hist)) prefs.getBytes("h_pm25", &g_pm25_hour_hist, sizeof(g_pm25_hour_hist));
    if (prefs.getBytesLength("h_co2") == sizeof(g_co2_hour_hist)) prefs.getBytes("h_co2", &g_co2_hour_hist, sizeof(g_co2_hour_hist));
    if (prefs.getBytesLength("h_noise") == sizeof(g_noise_hour_hist)) prefs.getBytes("h_noise", &g_noise_hour_hist, sizeof(g_noise_hour_hist));
    prefs.end();

}

static void saveRollingHistoryIfNeeded(bool force) {
    if (!g_hist_loaded || !g_hist_dirty) return;

    const uint32_t now_ms = millis();
    // After reboot, g_hist_last_save_ms is 0. Do not throttle the first save in this boot.
    if (!force && g_hist_last_save_ms != 0U &&
        (now_ms - g_hist_last_save_ms) < kAnalyticsPersistIntervalMs) {
        return;
    }

    bool persisted = false;
    const uint32_t now_ts = (uint32_t)time(nullptr);
    Preferences prefs;
    if (prefs.begin("analytics", false)) {
        prefs.putUChar("ver", kAnalyticsHistVersion);
        if (now_ts > 0 && g_hist_first_save_ts == 0) {
            g_hist_first_save_ts = now_ts;
        }
        if (now_ts > 0) {
            g_hist_last_save_ts = now_ts;
        }
        prefs.putULong("fst_ts", g_hist_first_save_ts);
        prefs.putULong("lst_ts", g_hist_last_save_ts);
        prefs.putBytes("h_temp", &g_temp_hour_hist, sizeof(g_temp_hour_hist));
        prefs.putBytes("h_hum", &g_hum_hour_hist, sizeof(g_hum_hour_hist));
        prefs.putBytes("h_dew", &g_dew_hour_hist, sizeof(g_dew_hour_hist));
        prefs.putBytes("h_pm10", &g_pm10_hour_hist, sizeof(g_pm10_hour_hist));
        prefs.putBytes("h_pm25", &g_pm25_hour_hist, sizeof(g_pm25_hour_hist));
        prefs.putBytes("h_co2", &g_co2_hour_hist, sizeof(g_co2_hour_hist));
        prefs.putBytes("h_noise", &g_noise_hour_hist, sizeof(g_noise_hour_hist));
        prefs.end();
        persisted = true;
    }

    if (!persisted) return;
#if defined(USE_SD_CARD) && defined(DEV)
    dumpAnalyticsToSdDev(now_ts, force);
#endif
    g_hist_dirty = false;
    g_hist_last_save_ms = now_ms;
    g_hist_save_count++;
    g_hist_last_save_forced = force;
#if defined(DEV)
    const uint32_t saved_hour_key = currentLocalHourKey((time_t)now_ts);
    char saved_hour_buf[24];
    formatHourKey(saved_hour_buf, sizeof(saved_hour_buf), saved_hour_key);
    debug_outln_info(F("[ANALYTICS][DEV] persisted NVS hour_key"), String(saved_hour_key));
    debug_outln_info(F("[ANALYTICS][DEV] persisted NVS local hour"), String(saved_hour_buf));
    debug_outln_info(F("[ANALYTICS][DEV] persisted reason"), String(force ? "day-change" : "interval"));
#endif
}

static bool rollingHistoryHasAnyData(const RollingHourHistory &hist) {
    for (int i = 0; i < 48; i++) {
        if (hist.buckets[i].count > 0) return true;
    }
    return false;
}

static uint8_t rollingHistoryEntryCount(const RollingHourHistory &hist) {
    uint8_t n = 0;
    for (int i = 0; i < 48; i++) {
        if (hist.buckets[i].count > 0) n++;
    }
    return n;
}

static bool rollingHistoryLastEntry(const RollingHourHistory &hist, uint32_t &hour_key, float &avg_v, uint16_t &count) {
    bool found = false;
    uint32_t best = 0;
    for (int i = 0; i < 48; i++) {
        const RollingHourBucket &b = hist.buckets[i];
        if (b.count == 0) continue;
        if (!found || b.hour_key > best) {
            found = true;
            best = b.hour_key;
            avg_v = b.sum_v / (float)b.count;
            count = b.count;
        }
    }
    hour_key = best;
    return found;
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

static uint32_t currentLocalHourKey(time_t now) {
    struct tm local_tm;
    localtime_r(&now, &local_tm);
    const uint32_t day_key = currentLocalDayKey(now);
    return day_key * 24U + (uint32_t)local_tm.tm_hour;
}

static bool shouldPersistForCurrentHour(time_t now, uint32_t current_hour_key) {
    if (now <= 0) return false;
    if (g_hist_last_save_ts == 0U) return true; // first save after boot / empty history

    const uint32_t last_saved_hour_key = currentLocalHourKey((time_t)g_hist_last_save_ts);
    if (last_saved_hour_key == current_hour_key) return false; // already persisted this hour

    struct tm local_tm;
    localtime_r(&now, &local_tm);
    const bool near_hour_start = (local_tm.tm_min <= 10);
    const bool overdue = ((uint32_t)now > g_hist_last_save_ts) &&
                         (((uint32_t)now - g_hist_last_save_ts) >= 60U * 60U);

    // Prefer saves near the top of the hour, but if we missed the window/rebooted,
    // persist as soon as we are overdue.
    return near_hour_start || overdue;
}

// Inverse of daysFromCivil (Howard Hinnant's civil_from_days).
static void civilFromDays(int32_t z, int &y, unsigned &m, unsigned &d) {
    z += 719468;
    const int era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - era * 146097);                    // [0, 146096]
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
    y = (int)yoe + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);         // [0, 365]
    const unsigned mp = (5 * doy + 2) / 153;                              // [0, 11]
    d = doy - (153 * mp + 2) / 5 + 1;                                     // [1, 31]
    m = mp + (mp < 10 ? 3 : -9);                                          // [1, 12]
    y += (m <= 2);
}

static void formatHourKey(char *out, size_t out_sz, uint32_t hour_key) {
    if (!out || out_sz == 0) return;
    const int32_t day_key = (int32_t)(hour_key / 24U);
    const unsigned hour = (unsigned)(hour_key % 24U);
    int y = 0;
    unsigned m = 1, d = 1;
    civilFromDays(day_key, y, m, d);
    snprintf(out, out_sz, "%04d-%02u-%02u %02u:00", y, m, d, hour);
}

#if defined(USE_SD_CARD) && defined(DEV)
static bool ensureAnalyticsDevSdDir() {
    if (!sdCardLock(1500)) return false;
    bool ok = true;
    if (!SD.exists(ROLLUP_ROOT_FOLDER)) ok = SD.mkdir(ROLLUP_ROOT_FOLDER);
    if (ok && !SD.exists("/sensors_rollup/dev")) ok = SD.mkdir("/sensors_rollup/dev");
    if (ok && !SD.exists("/sensors_rollup/dev/analytics")) ok = SD.mkdir("/sensors_rollup/dev/analytics");
    sdCardUnlock();
    return ok;
}

static void appendHourlyDump(String &out, const char *name, const RollingHourHistory &hist) {
    out += "\n[hourly:";
    out += name;
    out += "]\n";
    out += "hour_key,day_key,hour,avg,count\n";
    for (int i = 0; i < 48; i++) {
        const RollingHourBucket &b = hist.buckets[i];
        if (b.count == 0) continue;
        const uint32_t day_key = b.hour_key / 24U;
        const uint8_t hour = (uint8_t)(b.hour_key % 24U);
        out += String((uint32_t)b.hour_key);
        out += ",";
        out += String(day_key);
        out += ",";
        out += String((uint32_t)hour);
        out += ",";
        out += String((b.count > 0) ? (b.sum_v / (float)b.count) : 0.0f, 3);
        out += ",";
        out += String((uint32_t)b.count);
        out += "\n";
    }
}

static void dumpAnalyticsToSdDev(uint32_t now_ts, bool forced_save) {
    if (!sdCardLogger.checkInserted()) return;
    if (!ensureAnalyticsDevSdDir()) return;

    const uint32_t hour_key = (now_ts > 0)
        ? currentLocalHourKey((time_t)now_ts)
        : currentLocalHourKey(time(nullptr));
    if (!forced_save && g_dev_sd_last_dump_hour_key == hour_key) return;

    String content;
    content.reserve(8192);
    content += "ts=";
    content += String((uint32_t)now_ts);
    content += "\n";
    content += "hour_key=";
    content += String((uint32_t)hour_key);
    content += "\n";
    content += "reason=";
    content += forced_save ? "day-change" : "interval";
    content += "\n";

    appendHourlyDump(content, "temp", g_temp_hour_hist);
    appendHourlyDump(content, "hum", g_hum_hour_hist);
    appendHourlyDump(content, "dew", g_dew_hour_hist);
    appendHourlyDump(content, "pm10", g_pm10_hour_hist);
    appendHourlyDump(content, "pm25", g_pm25_hour_hist);
    appendHourlyDump(content, "co2", g_co2_hour_hist);
    appendHourlyDump(content, "noise", g_noise_hour_hist);

    const String path = String("/sensors_rollup/dev/analytics/analytics_") + String((uint32_t)hour_key) + ".txt";
    if (sdCardLogger.writeTextFile(path, content)) {
        g_dev_sd_last_dump_hour_key = hour_key;
    }
}
#endif

static void updateRollingHourMetric(RollingHourHistory &hist, float value, uint32_t hour_key) {
    const uint32_t idx = hour_key % 48U;
    RollingHourBucket &b = hist.buckets[idx];
    if (b.count == 0 || b.hour_key != hour_key) {
        b.hour_key = hour_key;
        b.sum_v = value;
        b.count = 1;
        return;
    }
    b.sum_v += value;
    b.count++;
}

static bool applyHourlyMedianForDay(analytics_metric_t &metric, const RollingHourHistory &hist, uint32_t day_key) {
    float vals[24];
    int n = 0;
    for (int i = 0; i < 48; i++) {
        const RollingHourBucket &b = hist.buckets[i];
        if (b.count == 0) continue;
        if ((b.hour_key / 24U) != day_key) continue;
        vals[n++] = b.sum_v / (float)b.count;
        if (n >= 24) break;
    }
    if (n == 0) return false;

    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (vals[j] < vals[i]) {
                float t = vals[i];
                vals[i] = vals[j];
                vals[j] = t;
            }
        }
    }
    float med = (n & 1) ? vals[n / 2] : (vals[n / 2 - 1] + vals[n / 2]) * 0.5f;
    if (!metric.has_24h) {
        metric.min24h = med;
        metric.max24h = med;
        metric.has_24h = true;
    }
    metric.avg24h = med; // robust "typical day" value
    return true;
}

static bool readHourlyDayValuesFromHistory(const RollingHourHistory &hist, uint32_t day_key, float vals[24], bool has[24]) {
    for (int i = 0; i < 24; i++) {
        vals[i] = 0.0f;
        has[i] = false;
    }
    bool any = false;
    for (int i = 0; i < 48; i++) {
        const RollingHourBucket &b = hist.buckets[i];
        if (b.count == 0) continue;
        if ((b.hour_key / 24U) != day_key) continue;
        const uint8_t h = (uint8_t)(b.hour_key % 24U);
        vals[h] = b.sum_v / (float)b.count;
        has[h] = true;
        any = true;
    }
    return any;
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

    // Faster and smoother arc: stamp small filled circles along the band centerline.
    const int band_thickness = outer_r - inner_r + 1;
    const int stamp_r = (band_thickness >= 4) ? (band_thickness / 2) : 1;
    const float mid_r = (float)(inner_r + outer_r) * 0.5f;
    float step_deg = 0.65f; // denser sampling for smoother e-ink curves
    if (outer_r <= 42) step_deg = 0.55f;
    if (outer_r >= 70) step_deg = 0.45f;
    const float span = fabsf(end_deg - start_deg);
    const int samples = (int)(span / step_deg) + 2;
    for (int i = 0; i <= samples; ++i) {
        float t = (samples > 0) ? ((float)i / (float)samples) : 0.0f;
        float a = start_deg + (end_deg - start_deg) * t;
        float rad = a * kPi / 180.0f;
        int mx = cx + (int)roundf(cosf(rad) * mid_r);
        int my = cy + (int)roundf(sinf(rad) * mid_r);
        Paint_DrawCircle(mx, my, stamp_r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        if (stamp_r > 1 && (i & 1) == 0) {
            Paint_DrawCircle(mx, my, stamp_r - 1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
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

static void drawRingBorders(int cx, int cy, int outer_r, int inner_r) {
    Paint_DrawCircle(cx, cy, outer_r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    if (outer_r > 2) {
        Paint_DrawCircle(cx, cy, outer_r - 1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    }
    Paint_DrawCircle(cx, cy, inner_r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(cx, cy, inner_r + 1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
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

} // namespace

bool analyticsHistoryPersistenceEnabled() {
    return true;
}

bool analyticsHistoryIsLoaded() {
    loadRollingHistoryIfNeeded();
    return g_hist_loaded;
}

bool analyticsHistoryHasData() {
    loadRollingHistoryIfNeeded();
    return rollingHistoryHasAnyData(g_temp_hour_hist) ||
           rollingHistoryHasAnyData(g_hum_hour_hist) ||
           rollingHistoryHasAnyData(g_dew_hour_hist) ||
           rollingHistoryHasAnyData(g_pm10_hour_hist) ||
           rollingHistoryHasAnyData(g_pm25_hour_hist) ||
           rollingHistoryHasAnyData(g_co2_hour_hist) ||
           rollingHistoryHasAnyData(g_noise_hour_hist);
}

analytics_view_t analyticsGetView() {
    return g_analytics_view;
}

bool analyticsNextViewAtEdge() {
    g_analytics_view = analytics_view_t::OVERVIEW_24H;
    return true;
}

bool analyticsPrevViewAtEdge() {
    g_analytics_view = analytics_view_t::OVERVIEW_24H;
    return true;
}

const char* analyticsViewLabel() {
    return A_TXT("Night", "Ночь");
}

static const char* analyticsViewLabelTitle() {
    return A_TXT("Night score", "Ночной индекс");
}

void extractAnalyticsScreenValues(const DynamicJsonDocument &doc, analytics_screen_values_t &values) {
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

}

void analyticsIngestHourSample(const analytics_screen_values_t &values) {
    loadRollingHistoryIfNeeded();

    // Update low-memory rolling aggregates (48 hour buckets).
    time_t now = time(nullptr);
    bool updated = false;
    if (now > 0) {
        uint32_t day_key = currentLocalDayKey(now);
        uint32_t hour_key = currentLocalHourKey(now);
        uint8_t updated_metrics = 0;
        if (values.temp_indoor.has_current) {
            updateRollingHourMetric(g_temp_hour_hist, values.temp_indoor.current, hour_key);
            updated = true;
            updated_metrics++;
        }
        if (values.hum_indoor.has_current) {
            updateRollingHourMetric(g_hum_hour_hist, values.hum_indoor.current, hour_key);
            updated = true;
            updated_metrics++;
        }
        if (values.dew_indoor.has_current) {
            updateRollingHourMetric(g_dew_hour_hist, values.dew_indoor.current, hour_key);
            updated = true;
            updated_metrics++;
        }
        if (values.pm10.has_current) {
            updateRollingHourMetric(g_pm10_hour_hist, values.pm10.current, hour_key);
            updated = true;
            updated_metrics++;
        }
        if (values.pm25.has_current) {
            updateRollingHourMetric(g_pm25_hour_hist, values.pm25.current, hour_key);
            updated = true;
            updated_metrics++;
        }
        if (values.co2.has_current) {
            updateRollingHourMetric(g_co2_hour_hist, values.co2.current, hour_key);
            updated = true;
            updated_metrics++;
        }
        if (values.noise_avg.has_current) {
            updateRollingHourMetric(g_noise_hour_hist, values.noise_avg.current, hour_key);
            updated = true;
            updated_metrics++;
        }

        if (updated) {
            g_hist_dirty = true;
            const bool day_changed = (g_hist_last_day_key != 0U && g_hist_last_day_key != day_key);
            g_hist_last_day_key = day_key;
            const bool hour_persist_due = shouldPersistForCurrentHour(now, hour_key);
            saveRollingHistoryIfNeeded(day_changed || hour_persist_due);
#if defined(DEV)
            if (g_dev_last_ingest_hour_key != hour_key) {
                char ingest_hour_buf[24];
                formatHourKey(ingest_hour_buf, sizeof(ingest_hour_buf), hour_key);
                g_dev_last_ingest_hour_key = hour_key;
                debug_outln_info(F("[ANALYTICS][DEV] hourly ingest hour_key"), String(hour_key));
                debug_outln_info(F("[ANALYTICS][DEV] hourly ingest local hour"), String(ingest_hour_buf));
                debug_outln_info(F("[ANALYTICS][DEV] hourly ingest metrics count"), String((int)updated_metrics));
                debug_outln_info(F("[ANALYTICS][DEV] save count"), String((unsigned)g_hist_save_count));
                debug_outln_info(F("[ANALYTICS][DEV] save forced"), String(g_hist_last_save_forced ? "yes" : "no"));
                debug_outln_info(F("[ANALYTICS][DEV] save due hour"), String(hour_persist_due ? "yes" : "no"));
            }
#endif
        }
    }
}

void analyticsDevLogStatus15m() {
#if defined(DEV)
    const uint32_t now_ms = millis();
    if ((now_ms - g_dev_last_status_log_ms) < (15UL * 60UL * 1000UL)) return;
    g_dev_last_status_log_ms = now_ms;

    loadRollingHistoryIfNeeded();

    char last_save_buf[24];
    formatLogTs(last_save_buf, sizeof(last_save_buf), g_hist_last_save_ts);
    debug_outln_info(F("[ANALYTICS][DEV][15m] NVS last save"), String(last_save_buf));

    auto log_metric = [&](const char *name, const RollingHourHistory &hist) {
        const uint8_t entries = rollingHistoryEntryCount(hist);
        uint32_t hk = 0;
        float avg = 0.0f;
        uint16_t cnt = 0;
        if (rollingHistoryLastEntry(hist, hk, avg, cnt)) {
            String line = String(name) + " entries=" + String((unsigned)entries) +
                          " last_hour_key=" + String((unsigned long)hk) +
                          " avg=" + String(avg, 2) +
                          " cnt=" + String((unsigned)cnt);
            debug_outln_info(F("[ANALYTICS][DEV][15m]"), line);
        } else {
            String line = String(name) + " entries=0 last=none";
            debug_outln_info(F("[ANALYTICS][DEV][15m]"), line);
        }
    };

    log_metric("co2", g_co2_hour_hist);
    log_metric("pm25", g_pm25_hour_hist);
    log_metric("noise", g_noise_hour_hist);
    log_metric("temp", g_temp_hour_hist);
    log_metric("hum", g_hum_hour_hist);
#endif
}

void populateAnalyticsPeriodStats(analytics_screen_values_t &values) {
    loadRollingHistoryIfNeeded();

    // "24H" is derived from hourly history: prefer completed local day, fallback to current day.
    time_t now = time(nullptr);
    if (now > 0) {
        uint32_t today = currentLocalDayKey(now);
        uint32_t completed_day = (today > 0U) ? (today - 1U) : 0U;
        bool ok = false;
        if (!values.temp_indoor.has_24h) {
            ok = applyHourlyMedianForDay(values.temp_indoor, g_temp_hour_hist, completed_day);
            if (!ok) applyHourlyMedianForDay(values.temp_indoor, g_temp_hour_hist, today);
        }
        if (!values.hum_indoor.has_24h) {
            ok = applyHourlyMedianForDay(values.hum_indoor, g_hum_hour_hist, completed_day);
            if (!ok) applyHourlyMedianForDay(values.hum_indoor, g_hum_hour_hist, today);
        }
        if (!values.dew_indoor.has_24h) {
            ok = applyHourlyMedianForDay(values.dew_indoor, g_dew_hour_hist, completed_day);
            if (!ok) applyHourlyMedianForDay(values.dew_indoor, g_dew_hour_hist, today);
        }
        if (!values.pm10.has_24h) {
            ok = applyHourlyMedianForDay(values.pm10, g_pm10_hour_hist, completed_day);
            if (!ok) applyHourlyMedianForDay(values.pm10, g_pm10_hour_hist, today);
        }
        if (!values.pm25.has_24h) {
            ok = applyHourlyMedianForDay(values.pm25, g_pm25_hour_hist, completed_day);
            if (!ok) applyHourlyMedianForDay(values.pm25, g_pm25_hour_hist, today);
        }
        if (!values.co2.has_24h) {
            ok = applyHourlyMedianForDay(values.co2, g_co2_hour_hist, completed_day);
            if (!ok) applyHourlyMedianForDay(values.co2, g_co2_hour_hist, today);
        }
        if (!values.noise_avg.has_24h) {
            ok = applyHourlyMedianForDay(values.noise_avg, g_noise_hour_hist, completed_day);
            if (!ok) applyHourlyMedianForDay(values.noise_avg, g_noise_hour_hist, today);
        }
    }
}

static int clampScore(int v) {
    if (v < 0) return 0;
    if (v > 100) return 100;
    return v;
}

static float impactAbove(float value, float threshold, float step, float pct_per_step) {
    if (value <= threshold || step <= 0.0f) return 0.0f;
    return -pct_per_step * ((value - threshold) / step);
}

static float nightImpactCO2(float co2, bool bio) {
    return bio
        ? impactAbove(co2, 600.0f, 100.0f, 0.8f)
        : impactAbove(co2, 750.0f, 100.0f, 0.52f);
}

static float nightImpactPM25(float pm25, bool bio) {
    return bio
        ? impactAbove(pm25, 3.0f, 10.0f, 0.5f)
        : impactAbove(pm25, 5.0f, 10.0f, 0.3f);
}

static float nightImpactNoise(float noise, bool bio) {
    return bio
        ? impactAbove(noise, 30.0f, 10.0f, 3.5f)
        : impactAbove(noise, 35.0f, 10.0f, 2.5f);
}

static float nightImpactTemp(float temp, bool bio) {
    return bio
        ? impactAbove(temp, 20.0f, 1.0f, 1.5f)
        : impactAbove(temp, 25.0f, 1.0f, 1.5f);
}

static float nightImpactHum(float hum, bool bio) {
    float outside = 0.0f;
    if (!bio) {
        // Conservative: penalize humidity outside 40..60 with softer slope.
        if (hum < 40.0f) outside = 40.0f - hum;
        else if (hum > 60.0f) outside = hum - 60.0f;
        return -0.2f * (outside / 10.0f);
    }
    // Biohacking: narrower comfort band 40..50 with stronger slope.
    if (hum < 40.0f) outside = 40.0f - hum;
    else if (hum > 50.0f) outside = hum - 50.0f;
    return -0.4f * (outside / 10.0f);
}

static int nightScoreCO2(float co2, bool bio) {
    // Methodology: sleep impact decreases above threshold, then score = 100 + impact*2.
    const float impact = nightImpactCO2(co2, bio);
    return clampScore((int)lroundf(100.0f + impact * 2.0f));
}

static int nightScorePM25(float pm25, bool bio) {
    const float impact = nightImpactPM25(pm25, bio);
    return clampScore((int)lroundf(100.0f + impact * 2.0f));
}

static int nightScoreNoise(float noise, bool bio) {
    const float impact = nightImpactNoise(noise, bio);
    return clampScore((int)lroundf(100.0f + impact * 2.0f));
}

static int nightScoreTemp(float temp, bool bio) {
    const float impact = nightImpactTemp(temp, bio);
    return clampScore((int)lroundf(100.0f + impact * 2.0f));
}

static int nightScoreHum(float hum, bool bio) {
    const float impact = nightImpactHum(hum, bio);
    return clampScore((int)lroundf(100.0f + impact * 2.0f));
}

static char gradeLetter(int score) {
    if (score >= 90) return 'A';
    if (score >= 80) return 'B';
    if (score >= 70) return 'C';
    if (score >= 60) return 'D';
    if (score >= 50) return 'E';
    return 'F';
}

static uint8_t safeHourCfg(unsigned v, uint8_t fallback) {
    return (v <= 23U) ? (uint8_t)v : fallback;
}

static uint16_t buildNightHourList(uint8_t start_h, uint8_t end_h, uint8_t out_hours[24]) {
    uint16_t n = 0;
    if (start_h == end_h) {
        for (uint8_t h = 0; h < 24; h++) out_hours[n++] = h;
        return n;
    }
    if (start_h < end_h) {
        for (uint8_t h = start_h; h < end_h; h++) out_hours[n++] = h;
        return n;
    }
    for (uint8_t h = start_h; h < 24; h++) out_hours[n++] = h;
    for (uint8_t h = 0; h < end_h; h++) out_hours[n++] = h;
    return n;
}

static uint32_t resolveLastCompletedNightEndDay(uint8_t /*start_h*/, uint8_t end_h) {
    time_t now = time(nullptr);
    if (now <= 0) return 0U;
    struct tm lt;
    localtime_r(&now, &lt);
    const uint8_t now_h = (uint8_t)lt.tm_hour;
    uint32_t today = currentLocalDayKey(now);
    if (now_h >= end_h) return today;
    return (today > 0U) ? (today - 1U) : 0U;
}

static void drawTrackArc(int cx, int cy, int outer_r, int inner_r, float start_deg, float sweep_deg, float fill_norm) {
    if (outer_r <= inner_r) return;
    if (fill_norm < 0.0f) fill_norm = 0.0f;
    if (fill_norm > 1.0f) fill_norm = 1.0f;
    // Avoid ultra-tiny residual gaps/stubs on e-ink, but keep normal
    // high/low values (e.g. 95%) visually distinct from full/empty.
    if (fill_norm > 0.995f) fill_norm = 1.0f;
    if (fill_norm < 0.005f) fill_norm = 0.0f;

    const float span = fabsf(sweep_deg);
    int steps = (int)(span * 3.2f);
    if (steps < 160) steps = 160;
    for (int i = 0; i <= steps; i++) {
        const float t = (float)i / (float)steps;
        const float a = (start_deg + sweep_deg * t) * (kPi / 180.0f);
        const int xo = cx + (int)roundf(cosf(a) * (float)outer_r);
        const int yo = cy + (int)roundf(sinf(a) * (float)outer_r);
        const int xi = cx + (int)roundf(cosf(a) * (float)inner_r);
        const int yi = cy + (int)roundf(sinf(a) * (float)inner_r);
        if (i == 0) {
            Paint_DrawPoint(xo, yo, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
            Paint_DrawPoint(xi, yi, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
        } else {
            const float tp = (float)(i - 1) / (float)steps;
            const float ap = (start_deg + sweep_deg * tp) * (kPi / 180.0f);
            const int pxo = cx + (int)roundf(cosf(ap) * (float)outer_r);
            const int pyo = cy + (int)roundf(sinf(ap) * (float)outer_r);
            const int pxi = cx + (int)roundf(cosf(ap) * (float)inner_r);
            const int pyi = cy + (int)roundf(sinf(ap) * (float)inner_r);
            Paint_DrawLine(pxo, pyo, xo, yo, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            Paint_DrawLine(pxi, pyi, xi, yi, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            // Midpoint touch-up reduces staircase gaps without increasing thickness.
            Paint_DrawPoint((pxo + xo) / 2, (pyo + yo) / 2, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
            Paint_DrawPoint((pxi + xi) / 2, (pyi + yi) / 2, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
        }
    }

    const float a0 = start_deg * (kPi / 180.0f);
    const float a1 = (start_deg + sweep_deg) * (kPi / 180.0f);
    Paint_DrawLine(cx + (int)roundf(cosf(a0) * (float)inner_r), cy + (int)roundf(sinf(a0) * (float)inner_r),
                   cx + (int)roundf(cosf(a0) * (float)outer_r), cy + (int)roundf(sinf(a0) * (float)outer_r),
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(cx + (int)roundf(cosf(a1) * (float)inner_r), cy + (int)roundf(sinf(a1) * (float)inner_r),
                   cx + (int)roundf(cosf(a1) * (float)outer_r), cy + (int)roundf(sinf(a1) * (float)outer_r),
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    drawArcBand(cx, cy, outer_r - 1, inner_r + 1, start_deg, sweep_deg, fill_norm);
}

static void drawNightSinglePage(int content_left, int content_top, int content_width,
                                const analytics_screen_values_t &values) {
    (void)values;
    float co2 = 0.0f, pm25 = 0.0f, noise = 0.0f, temp = 0.0f, hum = 0.0f;
    bool has_co2 = false, has_pm25 = false, has_noise = false, has_temp = false, has_hum = false;

    const int cx = content_left + content_width / 2;
    const int cy = content_top + 76;
    const int r = 59;

    const uint8_t night_start = safeHourCfg(cfg::analytics_night_start_hour, 22);
    const uint8_t night_end = safeHourCfg(cfg::analytics_night_end_hour, 10);
    uint8_t night_hours[24];
    const uint16_t n_hours = buildNightHourList(night_start, night_end, night_hours);
    const uint32_t end_day = resolveLastCompletedNightEndDay(night_start, night_end);
    const bool cross_midnight = (night_start >= night_end);

    float co2_day[24] = {0}, pm25_day[24] = {0}, noise_day[24] = {0}, temp_day[24] = {0}, hum_day[24] = {0};
    bool co2_has_day[24] = {false}, pm25_has_day[24] = {false}, noise_has_day[24] = {false}, temp_has_day[24] = {false}, hum_has_day[24] = {false};
    float co2_prev[24] = {0}, pm25_prev[24] = {0}, noise_prev[24] = {0}, temp_prev[24] = {0}, hum_prev[24] = {0};
    bool co2_has_prev[24] = {false}, pm25_has_prev[24] = {false}, noise_has_prev[24] = {false}, temp_has_prev[24] = {false}, hum_has_prev[24] = {false};
    readHourlyDayValuesFromHistory(g_co2_hour_hist, end_day, co2_day, co2_has_day);
    readHourlyDayValuesFromHistory(g_pm25_hour_hist, end_day, pm25_day, pm25_has_day);
    readHourlyDayValuesFromHistory(g_noise_hour_hist, end_day, noise_day, noise_has_day);
    readHourlyDayValuesFromHistory(g_temp_hour_hist, end_day, temp_day, temp_has_day);
    readHourlyDayValuesFromHistory(g_hum_hour_hist, end_day, hum_day, hum_has_day);
    if (cross_midnight && end_day > 0U) {
        readHourlyDayValuesFromHistory(g_co2_hour_hist, end_day - 1U, co2_prev, co2_has_prev);
        readHourlyDayValuesFromHistory(g_pm25_hour_hist, end_day - 1U, pm25_prev, pm25_has_prev);
        readHourlyDayValuesFromHistory(g_noise_hour_hist, end_day - 1U, noise_prev, noise_has_prev);
        readHourlyDayValuesFromHistory(g_temp_hour_hist, end_day - 1U, temp_prev, temp_has_prev);
        readHourlyDayValuesFromHistory(g_hum_hour_hist, end_day - 1U, hum_prev, hum_has_prev);
    }

    uint16_t hours_with_any_data = 0;
    float co2_sum = 0.0f, pm25_sum = 0.0f, noise_sum = 0.0f, temp_sum = 0.0f, hum_sum = 0.0f;
    uint16_t co2_count = 0, pm25_count = 0, noise_count = 0, temp_count = 0, hum_count = 0;
    for (uint16_t i = 0; i < n_hours; i++) {
        const uint8_t h = night_hours[i];
        const bool use_prev = cross_midnight && (h >= night_start);
        const bool c_has = use_prev ? co2_has_prev[h] : co2_has_day[h];
        const bool p_has = use_prev ? pm25_has_prev[h] : pm25_has_day[h];
        const bool n_has = use_prev ? noise_has_prev[h] : noise_has_day[h];
        const bool t_has = use_prev ? temp_has_prev[h] : temp_has_day[h];
        const bool hum_has = use_prev ? hum_has_prev[h] : hum_has_day[h];
        if (c_has || p_has || n_has || t_has || hum_has) hours_with_any_data++;

        if (c_has) { co2_sum += use_prev ? co2_prev[h] : co2_day[h]; co2_count++; }
        if (p_has) { pm25_sum += use_prev ? pm25_prev[h] : pm25_day[h]; pm25_count++; }
        if (n_has) { noise_sum += use_prev ? noise_prev[h] : noise_day[h]; noise_count++; }
        if (t_has) { temp_sum += use_prev ? temp_prev[h] : temp_day[h]; temp_count++; }
        if (hum_has) { hum_sum += use_prev ? hum_prev[h] : hum_day[h]; hum_count++; }
    }
    has_co2 = (co2_count > 0U);
    has_pm25 = (pm25_count > 0U);
    has_noise = (noise_count > 0U);
    has_temp = (temp_count > 0U);
    has_hum = (hum_count > 0U);
    if (has_co2) co2 = co2_sum / (float)co2_count;
    if (has_pm25) pm25 = pm25_sum / (float)pm25_count;
    if (has_noise) noise = noise_sum / (float)noise_count;
    if (has_temp) temp = temp_sum / (float)temp_count;
    if (has_hum) hum = hum_sum / (float)hum_count;
    const uint16_t min_hours_for_score = (n_hours >= 3) ? (uint16_t)((n_hours * 2U + 2U) / 3U) : n_hours;
    const bool enough_night_data = (hours_with_any_data >= min_hours_for_score);
    if (!enough_night_data) {
        char collect1[80], collect2[80];
        snprintf(collect1, sizeof(collect1), A_TXT("Night data is collecting", "Ночные данные собираются"));
        snprintf(collect2, sizeof(collect2), A_TXT("Hours: %u/%u", "Часов: %u/%u"),
                 (unsigned)hours_with_any_data, (unsigned)min_hours_for_score);
        uint16_t w1 = Paint_GetStringWidth_Display(collect1, &Font16, &font_16_cyrillic, &font_16_ascii);
        uint16_t w2 = Paint_GetStringWidth_Display(collect2, &Font16, &font_16_cyrillic, &font_16_ascii);
        const int block_h = Font16.Height * 2 + 6;
        const int msg_y = content_top + (DISPLAY_HEIGHT - content_top - block_h) / 2 - 18;
        Paint_DrawString_Display(content_left + (content_width - (int)w1) / 2, msg_y, collect1,
                                 &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
        Paint_DrawString_Display(content_left + (content_width - (int)w2) / 2, msg_y + Font16.Height + 6, collect2,
                                 &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
        return;
    }

    int s_cons_co2 = 0, s_cons_pm25 = 0, s_cons_noise = 0, s_cons_temp = 0, s_cons_hum = 0;
    int s_bio_co2 = 0, s_bio_pm25 = 0, s_bio_noise = 0, s_bio_temp = 0, s_bio_hum = 0;
    float i_cons_co2 = 0.0f, i_cons_pm25 = 0.0f, i_cons_noise = 0.0f, i_cons_temp = 0.0f, i_cons_hum = 0.0f;
    float i_bio_co2 = 0.0f, i_bio_pm25 = 0.0f, i_bio_noise = 0.0f, i_bio_temp = 0.0f, i_bio_hum = 0.0f;
    if (has_co2) {
        i_cons_co2 = nightImpactCO2(co2, false); i_bio_co2 = nightImpactCO2(co2, true);
        s_cons_co2 = nightScoreCO2(co2, false); s_bio_co2 = nightScoreCO2(co2, true);
    }
    if (has_pm25) {
        i_cons_pm25 = nightImpactPM25(pm25, false); i_bio_pm25 = nightImpactPM25(pm25, true);
        s_cons_pm25 = nightScorePM25(pm25, false); s_bio_pm25 = nightScorePM25(pm25, true);
    }
    if (has_noise) {
        i_cons_noise = nightImpactNoise(noise, false); i_bio_noise = nightImpactNoise(noise, true);
        s_cons_noise = nightScoreNoise(noise, false); s_bio_noise = nightScoreNoise(noise, true);
    }
    if (has_temp) {
        i_cons_temp = nightImpactTemp(temp, false); i_bio_temp = nightImpactTemp(temp, true);
        s_cons_temp = nightScoreTemp(temp, false); s_bio_temp = nightScoreTemp(temp, true);
    }
    if (has_hum) {
        i_cons_hum = nightImpactHum(hum, false); i_bio_hum = nightImpactHum(hum, true);
        s_cons_hum = nightScoreHum(hum, false); s_bio_hum = nightScoreHum(hum, true);
    }

    const float total_cons_impact =
        (has_co2 ? i_cons_co2 : 0.0f) +
        (has_pm25 ? i_cons_pm25 : 0.0f) +
        (has_noise ? i_cons_noise : 0.0f) +
        (has_temp ? i_cons_temp : 0.0f) +
        (has_hum ? i_cons_hum : 0.0f);
    const float total_bio_impact =
        (has_co2 ? i_bio_co2 : 0.0f) +
        (has_pm25 ? i_bio_pm25 : 0.0f) +
        (has_noise ? i_bio_noise : 0.0f) +
        (has_temp ? i_bio_temp : 0.0f) +
        (has_hum ? i_bio_hum : 0.0f);
    const int cons_score = clampScore((int)lroundf(100.0f + total_cons_impact * 2.0f));
    const int bio_score = clampScore((int)lroundf(100.0f + total_bio_impact * 2.0f));
    const int circle_score = cons_score;
    const char grade = gradeLetter(circle_score);

    drawTrackArc(cx, cy, r + 1, r - 10, -90.0f, 359.0f, (float)circle_score / 100.0f);

    auto metricFill = [&](float c_impact, float b_impact, bool has_data) -> float {
        if (!has_data || !enough_night_data) return 0.0f;
        // Strict visual mode: arcs represent sleep impact directly.
        // 0% impact -> full arc, -10% impact (or worse) -> empty arc.
        const float impact_avg = 0.5f * (c_impact + b_impact);
        float fill = 1.0f + (impact_avg / 10.0f);
        if (fill < 0.0f) fill = 0.0f;
        if (fill > 1.0f) fill = 1.0f;
        return fill;
    };
    const float f_co2 = metricFill(i_cons_co2, i_bio_co2, has_co2);
    const float f_pm25 = metricFill(i_cons_pm25, i_bio_pm25, has_pm25);
    const float f_noise = metricFill(i_cons_noise, i_bio_noise, has_noise);
    const float f_temp = metricFill(i_cons_temp, i_bio_temp, has_temp);
    const float f_hum = metricFill(i_cons_hum, i_bio_hum, has_hum);

    const int out_outer = 78;
    const int out_inner = 72;
    drawTrackArc(cx, cy, out_outer, out_inner, 210.0f, 40.0f, f_co2);
    drawTrackArc(cx, cy, out_outer, out_inner, 290.0f, 38.0f, f_pm25);
    drawTrackArc(cx, cy, out_outer, out_inner, 342.0f, 32.0f, f_noise);
    drawTrackArc(cx, cy, out_outer, out_inner, 24.0f, 38.0f, f_temp);
    drawTrackArc(cx, cy, out_outer, out_inner, 130.0f, 40.0f, f_hum);

    char score_txt[8];
    snprintf(score_txt, sizeof(score_txt), "%d", circle_score);
    uint16_t sw = Paint_GetStringWidth_Display(score_txt, &Font24, &font_36_cyrillic, &font_36_ascii);
    const int score_x = cx - (int)sw / 2;
    const int score_y = cy - 31;
    Paint_DrawString_Display(score_x, score_y, score_txt, &Font24, &font_36_cyrillic, &font_36_ascii, WHITE, BLACK);
    char grade_txt[24];
    snprintf(grade_txt, sizeof(grade_txt), "GRADE %c", grade);
    uint16_t gw = Paint_GetStringWidth_Display(grade_txt, &Font12, &font_12_cyrillic, &font_12_ascii);
    Paint_DrawString_Display(cx - (int)gw / 2, cy + 14, grade_txt, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);

    auto drawMetricLabel = [&](int x, int y, const char *name, const char *val) {
        Paint_DrawString_Display(x, y, name, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        Paint_DrawString_Display(x, y + 13, val, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    };
    char co2v[16] = "--", pmv[16] = "--", nv[16] = "--", tv[16] = "--", hv[16] = "--";
    if (has_co2) snprintf(co2v, sizeof(co2v), "%.0f", co2);
    if (has_pm25) snprintf(pmv, sizeof(pmv), "%.0f", pm25);
    if (has_noise) snprintf(nv, sizeof(nv), "%.0f", noise);
    if (has_temp) snprintf(tv, sizeof(tv), "%.0f", temp);
    if (has_hum) snprintf(hv, sizeof(hv), "%.0f%%", hum);
    drawMetricLabel(cx - 84, cy - 82, "CO2", co2v);
    drawMetricLabel(cx + 58, cy - 82, "PM2.5", pmv);
    drawMetricLabel(cx + 84, cy - 20, "dB", nv);
    drawMetricLabel(cx + 78, cy + 26, "C", tv);
    drawMetricLabel(cx - 92, cy + 28, "RH", hv);

    float max_co2 = -1.0f, max_pm25 = -1.0f, max_noise = -1.0f, max_temp = -1.0f, max_hum = -1.0f;
    float min_co2 = 1000000.0f, min_pm25 = 1000000.0f, min_noise = 1000000.0f, min_temp = 1000000.0f, min_hum = 1000000.0f;
    int max_co2_h = -1, max_pm25_h = -1, max_noise_h = -1, max_temp_h = -1, max_hum_h = -1;
    int min_co2_h = -1, min_pm25_h = -1, min_noise_h = -1, min_temp_h = -1, min_hum_h = -1;
    auto upd = [](float v, bool has, float &mx, int &mxh, float &mn, int &mnh, int h) {
        if (!has) return;
        if (v > mx) { mx = v; mxh = h; }
        if (v < mn) { mn = v; mnh = h; }
    };
    for (uint16_t i = 0; i < n_hours; i++) {
        const uint8_t h = night_hours[i];
        const bool use_prev = cross_midnight && (h >= night_start);
        const float c_val = use_prev ? co2_prev[h] : co2_day[h];
        const float p_val = use_prev ? pm25_prev[h] : pm25_day[h];
        const float n_val = use_prev ? noise_prev[h] : noise_day[h];
        const float t_val = use_prev ? temp_prev[h] : temp_day[h];
        const float h_val = use_prev ? hum_prev[h] : hum_day[h];
        const bool c_has = use_prev ? co2_has_prev[h] : co2_has_day[h];
        const bool p_has = use_prev ? pm25_has_prev[h] : pm25_has_day[h];
        const bool n_has = use_prev ? noise_has_prev[h] : noise_has_day[h];
        const bool t_has = use_prev ? temp_has_prev[h] : temp_has_day[h];
        const bool h_has = use_prev ? hum_has_prev[h] : hum_has_day[h];
        upd(c_val, c_has, max_co2, max_co2_h, min_co2, min_co2_h, h);
        upd(p_val, p_has, max_pm25, max_pm25_h, min_pm25, min_pm25_h, h);
        upd(n_val, n_has, max_noise, max_noise_h, min_noise, min_noise_h, h);
        upd(t_val, t_has, max_temp, max_temp_h, min_temp, min_temp_h, h);
        upd(h_val, h_has, max_hum, max_hum_h, min_hum, min_hum_h, h);
    }

    const int table_x = content_left + 4;
    const int table_y = content_top + 150;
    const int table_w = content_width - 8;
    const int header_h = 16;
    const int row_h = 15;
    const int table_h = header_h + row_h * 5;
    Paint_DrawRectangle(table_x, table_y, table_x + table_w, table_y + table_h, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

    const int c1 = table_x + 74;  // Max
    const int c2 = table_x + 170; // Min
    const int c3 = table_x + 258; // Conserv
    const int c4 = table_x + 302; // Biohack
    Paint_DrawLine(c1, table_y, c1, table_y + table_h, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(c2, table_y, c2, table_y + table_h, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(c3, table_y, c3, table_y + table_h, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(c4, table_y, c4, table_y + table_h, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(table_x, table_y + header_h, table_x + table_w, table_y + header_h, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    for (int rline = 1; rline <= 5; rline++) {
        Paint_DrawLine(table_x, table_y + header_h + rline * row_h, table_x + table_w, table_y + header_h + rline * row_h,
                       BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }

    auto drawCentered = [&](int x0, int x1, int y, const char *txt) {
        uint16_t w = Paint_GetStringWidth_Display(txt, &Font12, &font_12_cyrillic, &font_12_ascii);
        int x = x0 + ((x1 - x0) - (int)w) / 2;
        if (x < x0 + 1) x = x0 + 1;
        Paint_DrawString_Display(x, y, txt, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    };
    Paint_DrawString_Display(table_x + 8, table_y + 2, A_TXT("Metric", "Метрика"),
                             &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    drawCentered(c1, c2, table_y + 2, "Max");
    drawCentered(c2, c3, table_y + 2, "Min");
    drawCentered(c3, c4, table_y + 2, "Conserv");
    drawCentered(c4, table_x + table_w, table_y + 2, "Biohack");

    const char *hour_suffix = A_TXT("h", "ч");
    auto extremaCell = [&](char *out, size_t sz, float v, int h, bool has, uint8_t prec) {
        if (!has || h < 0) {
            snprintf(out, sz, "--");
            return;
        }
        snprintf(out, sz, "%.*f (%02d%s)", (int)prec, v, h, hour_suffix);
    };
    auto formatImpact = [](char *out, size_t sz, float impact, bool has_data) {
        if (!has_data) {
            snprintf(out, sz, "--");
            return;
        }
        if (fabsf(impact) < 0.05f) {
            snprintf(out, sz, "0%%");
            return;
        }
        snprintf(out, sz, "%.1f%%", impact);
    };
    auto drawRow = [&](int idx, const char *name, float mx, int mxh, bool has_mx, float mn, int mnh, bool has_mn,
                       float impact_c, float impact_b, bool has_imp_c, bool has_imp_b, uint8_t prec) {
        const int y = table_y + header_h + 2 + idx * row_h;
        char maxb[20], minb[20], cs[12], bs[12];
        extremaCell(maxb, sizeof(maxb), mx, mxh, has_mx, prec);
        extremaCell(minb, sizeof(minb), mn, mnh, has_mn, prec);
        formatImpact(cs, sizeof(cs), impact_c, has_imp_c);
        formatImpact(bs, sizeof(bs), impact_b, has_imp_b);
        Paint_DrawString_Display(table_x + 8, y, name,
                                 &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        drawCentered(c1, c2, y, maxb);
        drawCentered(c2, c3, y, minb);
        drawCentered(c3, c4, y, cs);
        drawCentered(c4, table_x + table_w, y, bs);
    };
    drawRow(0, "CO2 ppm", max_co2, max_co2_h, max_co2_h >= 0, min_co2, min_co2_h, min_co2_h >= 0, i_cons_co2, i_bio_co2, has_co2, has_co2, 0);
    drawRow(1, "Temp C", max_temp, max_temp_h, max_temp_h >= 0, min_temp, min_temp_h, min_temp_h >= 0, i_cons_temp, i_bio_temp, has_temp, has_temp, 0);
    drawRow(2, "RH %", max_hum, max_hum_h, max_hum_h >= 0, min_hum, min_hum_h, min_hum_h >= 0, i_cons_hum, i_bio_hum, has_hum, has_hum, 0);
    drawRow(3, "PM2.5 ug", max_pm25, max_pm25_h, max_pm25_h >= 0, min_pm25, min_pm25_h, min_pm25_h >= 0, i_cons_pm25, i_bio_pm25, has_pm25, has_pm25, 0);
    drawRow(4, "Noise dB", max_noise, max_noise_h, max_noise_h >= 0, min_noise, min_noise_h, min_noise_h >= 0, i_cons_noise, i_bio_noise, has_noise, has_noise, 0);

    char cons_impact_txt[12], bio_impact_txt[12];
    formatImpact(cons_impact_txt, sizeof(cons_impact_txt), total_cons_impact, true);
    formatImpact(bio_impact_txt, sizeof(bio_impact_txt), total_bio_impact, true);
    const int sum_y = table_y + table_h + 2;
    char summary_line[160];
    snprintf(summary_line, sizeof(summary_line),
             A_TXT("Conservative: %d%c, impact %s / Biohacking: %d%c, impact %s",
                   "Консервативная: %d%c, влияние %s / Биохакинг: %d%c, влияние %s"),
             cons_score, gradeLetter(cons_score), cons_impact_txt,
             bio_score, gradeLetter(bio_score), bio_impact_txt);
    uint16_t sum_w = Paint_GetStringWidth_Display(summary_line, &Font12, &font_12_cyrillic, &font_12_ascii);
    if (sum_w > (uint16_t)(table_w - 8)) {
        snprintf(summary_line, sizeof(summary_line),
                 A_TXT("Conservative %d%c (%s) / Biohacking %d%c (%s)",
                       "Консервативная %d%c (%s) / Биохакинг %d%c (%s)"),
                 cons_score, gradeLetter(cons_score), cons_impact_txt,
                 bio_score, gradeLetter(bio_score), bio_impact_txt);
        sum_w = Paint_GetStringWidth_Display(summary_line, &Font12, &font_12_cyrillic, &font_12_ascii);
    }
    if (sum_w > (uint16_t)(table_w - 8)) {
        snprintf(summary_line, sizeof(summary_line), "C %d%c (%s) / B %d%c (%s)",
                 cons_score, gradeLetter(cons_score), cons_impact_txt,
                 bio_score, gradeLetter(bio_score), bio_impact_txt);
    }
    drawCentered(table_x, table_x + table_w, sum_y + 2, summary_line);
}

void showAnalyticsPage(UBYTE *BlackImage, const analytics_screen_values_t &values) {
    (void)BlackImage;
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

    drawNightSinglePage(content_left, content_top, content_width, values);
    return;

}

#endif

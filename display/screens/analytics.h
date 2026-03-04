#ifdef ALTRUIST_INSIDE

#ifndef _ANALYTICS_SCREEN_H
#define _ANALYTICS_SCREEN_H

#include <ArduinoJson.h>
#include "../driver/EPD.h"

enum class analytics_period_t : uint8_t {
    P24H = 0,
    P7D,
    P30D
};

enum class analytics_view_t : uint8_t {
    OVERVIEW_24H = 0,
    CLIMATE_24H,
    CO2_24H,
    AIR_24H,
    NOISE_24H
};

struct analytics_metric_t {
    float current = 0.0f;
    float min24h = 0.0f;
    float max24h = 0.0f;
    float avg24h = 0.0f;
    bool has_current = false;
    bool has_24h = false;
};

struct analytics_screen_values_t {
    // Climate
    analytics_metric_t temp_indoor;
    analytics_metric_t hum_indoor;
    analytics_metric_t dew_indoor;
    analytics_metric_t temp_urban;
    analytics_metric_t hum_urban;
    // Air
    analytics_metric_t pm10;
    analytics_metric_t co2;
    analytics_metric_t pm25;
    analytics_metric_t pm10_insight;
    analytics_metric_t pm25_insight;
    // Noise
    analytics_metric_t noise_avg;
};

analytics_period_t analyticsGetPeriod();
void analyticsSetPeriod(analytics_period_t period);
void analyticsNextPeriod();
void analyticsPrevPeriod();
bool analyticsNextPeriodAtEdge();
bool analyticsPrevPeriodAtEdge();
const char* analyticsPeriodLabel();
analytics_view_t analyticsGetView();
analytics_period_t analyticsGetViewPeriod();
bool analyticsNextViewAtEdge();
bool analyticsPrevViewAtEdge();
const char* analyticsViewLabel();
bool analyticsHistoryPersistenceEnabled();
bool analyticsHistoryIsLoaded();
bool analyticsHistoryHasData();

void extractAnalyticsScreenValues(const JsonDocument &doc, analytics_screen_values_t &values);
void populateAnalyticsPeriodStats(analytics_screen_values_t &values, analytics_period_t period);
void showAnalyticsPage(UBYTE *BlackImage, const analytics_screen_values_t &values,
                       const analytics_screen_values_t &values_7d,
                       const analytics_screen_values_t &values_30d,
                       analytics_period_t period, analytics_view_t view);

#endif // _ANALYTICS_SCREEN_H

#endif

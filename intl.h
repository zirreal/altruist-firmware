#ifndef intl_h
#define intl_h

#define REGION_GLOBAL "Global"
#define REGION_EU "Europe"
#define REGION_AS "Asia"
#define REGION_AF "Africa"
#define REGION_AU "Australia"
#define REGION_NA "NorthAmerica"
#define REGION_SA "SouthAmerica"

#if defined(INTL_BG)
#include "translations/intl_bg.h"
#elif defined(INTL_CZ)
#include "translations/intl_cz.h"
#elif defined(INTL_DE)
#include "translations/intl_de.h"
#elif defined(INTL_DK)
#include "translations/intl_dk.h"
#elif defined(INTL_EN)
#include "translations/intl_en.h"
#elif defined(INTL_ES)
#include "translations/intl_es.h"
#elif defined(INTL_FR)
#include "translations/intl_fr.h"
#elif defined(INTL_HU)
#include "translations/intl_hu.h"
#elif defined(INTL_IT)
#include "translations/intl_it.h"
#elif defined(INTL_LU)
#include "translations/intl_lu.h"
#elif defined(INTL_NL)
#include "translations/intl_nl.h"
#elif defined(INTL_PL)
#include "translations/intl_pl.h"
#elif defined(INTL_PT)
#include "translations/intl_pt.h"
#elif defined(INTL_RS)
#include "translations/intl_rs.h"
#elif defined(INTL_RU)
#include "translations/intl_ru.h"
#elif defined(INTL_SE)
#include "translations/intl_se.h"
#elif defined(INTL_SK)
#include "translations/intl_sk.h"
#elif defined(INTL_TR)
#include "translations/intl_tr.h"
#elif defined(INTL_UA)
#include "translations/intl_ua.h"
#else
#warning No language defined
#include "translations/intl_en.h"
#endif

#ifndef INTL_DISP_ANALYTICS_GRADE
#define INTL_DISP_ANALYTICS_GRADE "Grade"
#endif
#ifndef INTL_DISP_ANALYTICS_COL_METRIC
#define INTL_DISP_ANALYTICS_COL_METRIC "Metric"
#endif
#ifndef INTL_DISP_ANALYTICS_COL_MAX
#define INTL_DISP_ANALYTICS_COL_MAX "Max"
#endif
#ifndef INTL_DISP_ANALYTICS_COL_MIN
#define INTL_DISP_ANALYTICS_COL_MIN "Min"
#endif
#ifndef INTL_DISP_ANALYTICS_COL_CONSERV
#define INTL_DISP_ANALYTICS_COL_CONSERV "Conserv"
#endif
#ifndef INTL_DISP_ANALYTICS_COL_BIOHACK
#define INTL_DISP_ANALYTICS_COL_BIOHACK "Biohack"
#endif
#ifndef INTL_DISP_ANALYTICS_ROW_CO2
#define INTL_DISP_ANALYTICS_ROW_CO2 "CO2 ppm"
#endif
#ifndef INTL_DISP_ANALYTICS_ROW_TEMP
#define INTL_DISP_ANALYTICS_ROW_TEMP "Temperature C"
#endif
#ifndef INTL_DISP_ANALYTICS_ROW_HUM
#define INTL_DISP_ANALYTICS_ROW_HUM "Humidity %"
#endif
#ifndef INTL_DISP_ANALYTICS_ROW_PM25
#define INTL_DISP_ANALYTICS_ROW_PM25 "PM2.5 ug/m3"
#endif
#ifndef INTL_DISP_ANALYTICS_ROW_NOISE
#define INTL_DISP_ANALYTICS_ROW_NOISE "Noise dB"
#endif
#ifndef INTL_DISP_ANALYTICS_AT
#define INTL_DISP_ANALYTICS_AT "at"
#endif
#ifndef INTL_DISP_ANALYTICS_HOUR_SUFFIX
#define INTL_DISP_ANALYTICS_HOUR_SUFFIX "h"
#endif

#endif

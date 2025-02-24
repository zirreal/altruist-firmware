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

#endif

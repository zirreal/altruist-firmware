#ifdef ALTRUIST_INSIDE

#include "../paint_driver/graphPainter.h"
#include "graph.h"
#include "../utils.h"
#include "../../utils.h"
#include "../../intl.h"
#include "../../sd_card/sd_card.h"
#include "../../config_manager/config_helpers.h"
#include "../icons/icons/icons_15x15.h"
#include "../icons/icons/icons_10x10.h"
#include "../paint_driver/fonts/fonts.h"
#include <vector>

#if defined(USE_SD_CARD)
#include "SD.h"
#include "FS.h"
#endif

#if defined(USE_SD_CARD)
namespace {
class SDGraphLockGuard {
public:
    SDGraphLockGuard() : locked(sdCardLock(2000)) {}
    ~SDGraphLockGuard() { if (locked) sdCardUnlock(); }
    bool ok() const { return locked; }
private:
    bool locked;
};
} // namespace
#endif

uint8_t current_graph_screen = 1;
static GraphValue current_graph_value = GraphValue::INSIGHT_CO2;
constexpr uint8_t kGraphValueCount = static_cast<uint8_t>(GraphValue::URBAN_PRESSURE) + 1;
static bool g_graph_had_data[kGraphValueCount] = {false};
static uint8_t g_graph_no_data_streak[kGraphValueCount] = {0};

namespace {
// Right-nav order (also used for cycling next/prev so UI is consistent)
constexpr GraphValue kGraphNavOrder[] = {
    GraphValue::INSIGHT_CO2,
    GraphValue::INSIGHT_TEMP,
    GraphValue::INSIGHT_HUM,
    GraphValue::INSIGHT_PRESSURE,
    GraphValue::URBAN_TEMP,
    GraphValue::URBAN_HUM,
    GraphValue::URBAN_PRESSURE,
    GraphValue::URBAN_AIR,
    GraphValue::URBAN_NOISE,
};
constexpr uint8_t kGraphNavOrderCount = sizeof(kGraphNavOrder) / sizeof(kGraphNavOrder[0]);

static int navIndexOf(GraphValue v) {
    for (uint8_t i = 0; i < kGraphNavOrderCount; i++) {
        if (kGraphNavOrder[i] == v) return (int)i;
    }
    return -1;
}
} // namespace

static uint8_t graphValueIndex(GraphValue value) {
    return static_cast<uint8_t>(value);
}

static void markGraphDataState(GraphValue value, bool has_data) {
    const uint8_t idx = graphValueIndex(value);
    if (idx >= kGraphValueCount) return;
    if (has_data) {
        g_graph_had_data[idx] = true;
        g_graph_no_data_streak[idx] = 0;
    } else if (g_graph_no_data_streak[idx] < 255) {
        g_graph_no_data_streak[idx]++;
    }
}

static bool shouldShowNoDataMessage(GraphValue value) {
    const uint8_t idx = graphValueIndex(value);
    if (idx >= kGraphValueCount) return true;
    // If this graph has never had data in this boot, show message immediately.
    if (!g_graph_had_data[idx]) return true;
    // If we had data before, suppress one transient empty read to avoid flicker.
    return g_graph_no_data_streak[idx] >= 2;
}

// Human‑readable title for the current graph (measure only, no source prefix)
static const char* getGraphTitle(GraphValue value) {
    switch (value) {
        case GraphValue::INSIGHT_TEMP:
        case GraphValue::URBAN_TEMP:
            return INTL_DISP_TEMPERATURE;
        case GraphValue::INSIGHT_HUM:
        case GraphValue::URBAN_HUM:
            return INTL_DISP_HUMIDITY;
        case GraphValue::INSIGHT_CO2:
            return INTL_CO2;
        case GraphValue::INSIGHT_PRESSURE:
        case GraphValue::URBAN_PRESSURE:
            return INTL_DISP_PRESSURE;
        case GraphValue::URBAN_AIR:
            return INTL_DISP_AIR_QUALITY;
        case GraphValue::URBAN_NOISE:
            return INTL_DISP_NOISE;
        default:
            return "";
    }
}

// Filter data to hourly intervals (keep only the last value in each hour)
static void filterToHourlyData(LineData &data) {
    if (data.count == 0 || !data.values || !data.timestamps) {
        return;
    }
    
    // Group data by hour and keep only the last value in each hour
    std::vector<float> hourly_values;
    std::vector<uint32_t> hourly_timestamps;
    
    uint32_t current_hour = 0;
    bool first_hour = true;
    float last_value_in_hour = 0;
    uint32_t last_timestamp_in_hour = 0;
    
    for (int i = 0; i < data.count; i++) {
        uint32_t timestamp = data.timestamps[i];
        uint32_t hour = timestamp / 3600;  // Group by hour (Unix hour)
        
        if (first_hour) {
            current_hour = hour;
            first_hour = false;
        } else if (hour != current_hour) {
            // New hour - save the last value from previous hour
            hourly_values.push_back(last_value_in_hour);
            hourly_timestamps.push_back(last_timestamp_in_hour);
            current_hour = hour;
        }
        
        // Always update to the latest value in this hour
        last_value_in_hour = data.values[i];
        last_timestamp_in_hour = timestamp;
    }
    
    if (data.count > 0) {
        hourly_values.push_back(last_value_in_hour);
        hourly_timestamps.push_back(last_timestamp_in_hour);
    }
    
    // Replace the original data with filtered data
    delete[] data.values;
    delete[] data.timestamps;
    
    data.count = hourly_values.size();
    if (data.count > 0) {
        data.values = new float[data.count];
        data.timestamps = new uint32_t[data.count];
        for (int i = 0; i < data.count; i++) {
            data.values[i] = hourly_values[i];
            data.timestamps[i] = hourly_timestamps[i];
        }
    } else {
        data.values = nullptr;
        data.timestamps = nullptr;
    }
}

#if defined(USE_SD_CARD)
// External reference to SD card logger
extern SDCard sdCardLogger;

// Track last time we checked for files (refresh every 15 minutes)
static unsigned long last_file_check_time = 0;
static const unsigned long FILE_CHECK_INTERVAL_MS = 900000; // 15 minutes

static bool checkSDCardAvailable() {
    // Use checkInserted which properly detects card removal
    // It checks card type and tries to reinitialize if needed
    bool isConnected = sdCardLogger.checkInserted();
    if (!isConnected) {
        // Card was removed - update device status
        debug_outln_verbose(F("[Graph] SD card removed - updating status"));
        return false;
    }
    return true;
}

static bool checkDataFilesExist() {
#if defined(USE_SD_CARD)
    SDGraphLockGuard sdLock;
    if (!sdLock.ok()) {
        debug_outln_verbose(F("[Graph] Failed to acquire SD lock"));
        return false;
    }
#endif
    // First, verify card is still present (detect removal)
    if (!sdCardLogger.checkInserted()) {
        debug_outln_verbose(F("[Graph] SD card removed during file check"));
        return false;
    }
    
    // Refresh SD card cache periodically to detect new files
    unsigned long now = millis();
    if (now - last_file_check_time > FILE_CHECK_INTERVAL_MS || last_file_check_time == 0) {
        debug_outln_verbose(F("[Graph] Refreshing SD card cache..."));
        sdCardLogger.refreshCache();
        last_file_check_time = now;
    }
    
    // Check if root folder exists
    if (!SD.exists("/sensors_data")) {
        debug_outln_verbose(F("[Graph] /sensors_data folder does not exist"));
        return false;
    }
    
    // First, try direct check for today's files from known sensors (faster)
    time_t now_time = time(nullptr);
    struct tm* timeinfo = localtime(&now_time);
    char dateStr[11];
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", 
             timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
    
    String dateInfo = "[Graph] Checking for date: " + String(dateStr);
    debug_outln_verbose(dateInfo);
    
    // Check common sensor folders for today's file
    const char* sensors[] = {"SCD4x", "BME680", ATRUIST_URBAN_SENSOR};
    for (int i = 0; i < 3; i++) {
        String filePath = "/sensors_data/" + String(sensors[i]) + "/" + String(dateStr) + ".csv";
        String checkMsg = "[Graph] Checking file: " + filePath;
        debug_outln_verbose(checkMsg);
        if (SD.exists(filePath)) {
            File testFile = SD.open(filePath, FILE_READ);
            if (testFile && testFile.size() > 0) {
                String foundMsg = "[Graph] Found today's file: " + filePath + " size: " + String(testFile.size());
                debug_outln_verbose(foundMsg);
                testFile.close();
                return true;
            }
            if (testFile) {
                String emptyMsg = "[Graph] File exists but empty or can't read: " + filePath;
                debug_outln_verbose(emptyMsg);
                testFile.close();
            }
        } else {
            String notFoundMsg = "[Graph] File does not exist: " + filePath;
            debug_outln_verbose(notFoundMsg);
        }
    }
    
    // If direct check didn't find files, do full directory scan
    File root = SD.open("/sensors_data");
    if (!root || !root.isDirectory()) {
        debug_outln_verbose(F("[Graph] Failed to open /sensors_data directory"));
        return false;
    }
    
    bool hasData = false;
    int sensorCount = 0;
    int fileCount = 0;
    
    File entry = root.openNextFile();
    while (entry) {
        if (entry.isDirectory()) {
            sensorCount++;
            // Check if this sensor folder has any CSV files
            String sensorName = entry.name();
            // entry.name() returns just the folder name, need to build full path
            // Remove any leading path components
            int lastSlash = sensorName.lastIndexOf('/');
            if (lastSlash >= 0) {
                sensorName = sensorName.substring(lastSlash + 1);
            }
            // Build full path
            String sensorPath = "/sensors_data/" + sensorName;
            String folderInfo = "[Graph] Checking sensor folder: " + sensorPath;
            debug_outln_verbose(folderInfo);
            File sensorDir = SD.open(sensorPath);
            if (sensorDir && sensorDir.isDirectory()) {
                File csvFile = sensorDir.openNextFile();
                while (csvFile) {
                    String fileName = csvFile.name();
                    fileCount++;
                    String fileInfo = "[Graph] Found file: " + fileName + " size: " + String(csvFile.size());
                    debug_outln_verbose(fileInfo);
                    if (fileName.endsWith(".csv") && csvFile.size() > 0) {
                        hasData = true;
                        debug_outln_verbose(F("[Graph] Found valid CSV file with data"));
                        csvFile.close();
                        sensorDir.close();
                        root.close();
                        return true;
                    }
                    csvFile.close();
                    csvFile = sensorDir.openNextFile();
                }
                sensorDir.close();
            }
        }
        entry.close();
        entry = root.openNextFile();
    }
    root.close();
    
    String checkResult = "[Graph] Checked " + String(sensorCount) + " sensors, " + String(fileCount) + " files total. Has data: " + (hasData ? "yes" : "no");
    debug_outln_verbose(checkResult);
    return hasData;
}

static uint16_t drawOneGraph(int left_x, int left_y, const char* sensor_name, const char* meas_name, const char* label) {
    GraphLineStyle line_style;
    LineData result = {nullptr, nullptr, 0};
    readSensorDataFromCSV(result, sensor_name, meas_name, 12);
    
    // Clean up memory if no data
    if (result.count == 0) {
        if (result.values) {
            delete[] result.values;
            result.values = nullptr;
        }
        if (result.timestamps) {
            delete[] result.timestamps;
            result.timestamps = nullptr;
        }
    }
    
    GraphPainter graph(left_x, left_y, GRAPH_HEIGHT, GRAPH_WIDTH);
    graph.setWhiteMode();
    if (result.count > 0) {
        graph.addLineValues(result.values, result.timestamps, result.count, label, line_style);
    }
    graph.drawGraph();
    
    // Clean up after drawing
    if (result.values) {
        delete[] result.values;
    }
    if (result.timestamps) {
        delete[] result.timestamps;
    }
    
    return graph.getGraphWidth();
}

namespace {
struct ActiveGraphSummary {
    bool has_data = false;
    // Preformatted like "Max = 54dB / Avg = 47dB"
    char headline[64] = {0};
};

static const char* urbanSuffix(const char* base, char* out, size_t out_sz) {
    snprintf(out, out_sz, "%s (U)", base);
    return out;
}
static const char* insightSuffix(const char* base, char* out, size_t out_sz) {
    snprintf(out, out_sz, "%s (I)", base);
    return out;
}

static bool isSharedMetric(GraphValue v) {
    return (v == GraphValue::INSIGHT_TEMP || v == GraphValue::URBAN_TEMP ||
            v == GraphValue::INSIGHT_HUM  || v == GraphValue::URBAN_HUM  ||
            v == GraphValue::INSIGHT_PRESSURE || v == GraphValue::URBAN_PRESSURE);
}

static char graphSourceSuffix(GraphValue v) {
    switch (v) {
        case GraphValue::URBAN_TEMP:
        case GraphValue::URBAN_HUM:
        case GraphValue::URBAN_PRESSURE:
        case GraphValue::URBAN_AIR:
        case GraphValue::URBAN_NOISE:
            return 'U';
        case GraphValue::INSIGHT_TEMP:
        case GraphValue::INSIGHT_HUM:
        case GraphValue::INSIGHT_PRESSURE:
        case GraphValue::INSIGHT_CO2:
            return 'I';
        default:
            return '\0';
    }
}

static void drawGraphNavButtonsBottomLeft(uint16_t menu_x, uint16_t menu_y, uint16_t menu_w, uint16_t menu_h) {
    // Place up/down buttons (LEFT) + hint (RIGHT) at the BOTTOM of the nav column.
    const uint16_t iconSize = 10;
    const uint16_t iconGapY = 2;

    const uint16_t padL = 0;
    const uint16_t padB = 16; // move block a bit higher from bottom
    const uint16_t iconBlockH = iconSize * 2 + iconGapY;
    uint16_t x = menu_x + padL;
    uint16_t y = (menu_h > (iconBlockH + padB)) ? (menu_y + menu_h - iconBlockH - padB) : menu_y;

    Paint_DrawImage(button_up_10x10, x, y, iconSize, iconSize);
    Paint_DrawImage(button_down_10x10, x, y + iconSize + iconGapY, iconSize, iconSize);

    // Hint text next to arrows (keep it compact & readable).
    // We keep it inside the nav column width so it never overlaps the graph.
    const char* hint1 = INTL_DISP_GRAPHS_HINT_LINE1;
    const char* hint2a = INTL_DISP_GRAPHS_HINT_LINE2;
    const char* hint2b = INTL_DISP_GRAPHS_HINT_LINE3;
    // Put hint text to the RIGHT of the icons.
    const uint16_t hintX = x + iconSize + 7; // bigger gap between icon and text
    uint16_t hintY = (y > 2) ? (y - 2) : y;

    const uint16_t maxRight = (menu_x + menu_w > 2) ? (menu_x + menu_w - 2) : (menu_x + menu_w);
    if (hintX < maxRight) {
        const uint16_t maxW = maxRight - hintX;
        auto drawFit = [&](uint16_t x0, uint16_t y0, const char* full, const char* fallback) {
            uint16_t wFull = Paint_GetStringWidth_Display(full, &Font12, &font_12_cyrillic, &font_12_ascii);
            if (wFull <= maxW) {
                Paint_DrawString_Display(x0, y0, full, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
                return;
            }
            uint16_t wFallback = Paint_GetStringWidth_Display(fallback, &Font12, &font_12_cyrillic, &font_12_ascii);
            if (wFallback <= maxW) {
                Paint_DrawString_Display(x0, y0, fallback, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
                return;
            }
            // Last resort: draw fallback anyway (it will clip within the nav column).
            Paint_DrawString_Display(x0, y0, fallback, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        };

        const uint16_t hintLineGap = 4; // extra spacing between hint lines
        // Prefer showing full "long press" even if it clips slightly.
        drawFit(hintX, hintY, hint1, "long press ->");
        hintY += Font12.Height + hintLineGap;
        drawFit(hintX, hintY, hint2a, "next/prev");
        if (hint2b && hint2b[0] != '\0') {
            hintY += Font12.Height + hintLineGap;
            drawFit(hintX, hintY, hint2b, "screen");
        }
    }
}

static void drawTriangleUp(uint16_t cx, uint16_t cy, uint16_t size, uint16_t color) {
    // Simple outline triangle
    Paint_DrawLine(cx, cy, cx - size, cy + size, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(cx, cy, cx + size, cy + size, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(cx - size, cy + size, cx + size, cy + size, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

static void drawTriangleDown(uint16_t cx, uint16_t cy, uint16_t size, uint16_t color) {
    Paint_DrawLine(cx, cy, cx - size, cy - size, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(cx, cy, cx + size, cy - size, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(cx - size, cy - size, cx + size, cy - size, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

static void drawGraphValueMenu(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, GraphValue selected) {
    // Keep this menu compact so the plot stays the primary focus.
    const uint16_t padX = 6;             // items closer to the left edge
    const uint16_t bottomPad = 8;        // leave ~8px below the last item
    const int8_t letterSpacing = 0;      // as tight as possible (no extra tracking)

    // Right-nav font: 12px glyph font.
    // Keep `font_en` as Font12 since EN rendering uses `font_ascii` when provided.
    sFONT* navFontEn = &Font12;
    const Font* navFontRu = &font_12_cyrillic;
    const Font* navFontAscii = &font_12_ascii;
#ifdef INTL_RU
    const uint16_t navLineH = font_12_cyrillic.line_height ? font_12_cyrillic.line_height : Font12.Height;
#else
    const uint16_t navLineH = font_12_ascii.line_height ? font_12_ascii.line_height : Font12.Height;
#endif
    const uint16_t rowGap = 6;
    const uint16_t groupGap = 12;

    // Top-align list. Reserve bottom space for the arrow+hint block (3 hint lines).
    const uint16_t reservedBottomH = 60;
    const uint16_t maxListBottom = (h > reservedBottomH + bottomPad) ? (y0 + h - reservedBottomH - bottomPad) : (y0 + h);
    uint16_t y = y0 + 8;

    auto drawItem = [&](const char* text, bool is_selected) {
        const uint16_t textX = x0 + padX;
        // Prevent overflow with wider fonts: truncate to fit the nav column.
        const uint16_t maxRight = (x0 + w > 2) ? (x0 + w - 2) : (x0 + w);
        const uint16_t maxW = (maxRight > textX) ? (maxRight - textX) : 0;
        const char* toDraw = text;
        char clipped[48];
        auto stringW = [&](const char* s) -> uint16_t {
            const uint16_t base = Paint_GetStringWidth_Display(s, navFontEn, navFontRu, navFontAscii);
            const size_t len = strlen(s);
            if (len <= 1) return base;
            const int32_t spaced = (int32_t)base + (int32_t)(len - 1) * (int32_t)letterSpacing;
            return (spaced > 0) ? (uint16_t)spaced : 0;
        };
        if (maxW > 0) {
            uint16_t tw = stringW(text);
            if (tw > maxW) {
                strncpy(clipped, text, sizeof(clipped));
                clipped[sizeof(clipped) - 1] = '\0';
                // Trim until it fits, then add ".."
                while (strlen(clipped) > 2) {
                    uint16_t wNow = stringW(clipped);
                    if (wNow <= maxW) break;
                    clipped[strlen(clipped) - 1] = '\0';
                }
                if (strlen(clipped) > 2) {
                    clipped[strlen(clipped) - 1] = '.';
                    clipped[strlen(clipped) - 2] = '.';
                }
                toDraw = clipped;
            }
        }
        if (is_selected) {
            // Highlight: keep it simple and reliable on e-ink:
            // - a left marker bar
            // (no bold; bold text was harder to read on e-ink)
            const uint16_t barW = 2;
            // Move marker slightly lower to align visually with glyph baseline.
            uint16_t barTop = y + 1;
            uint16_t barBottom = y + navLineH + 1;
            if (barBottom > y0 + h - 1) barBottom = y0 + h - 1;
            // Put the marker closer to the text (less empty gutter).
            uint16_t barX0 = (x0 + 2 < textX) ? (textX - 6) : x0;
            uint16_t barX1 = barX0 + barW;
            Paint_DrawRectangle(barX0, barTop, barX1, barBottom, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

            Paint_DrawString_Display_WithSpacing(textX, y, toDraw, navFontEn, navFontRu, navFontAscii, WHITE, BLACK, letterSpacing);
        } else {
            Paint_DrawString_Display_WithSpacing(textX, y, toDraw, navFontEn, navFontRu, navFontAscii, WHITE, BLACK, letterSpacing);
        }
        y += navLineH + rowGap;
    };

    char buf[48];
    // Use the shared nav order list for drawing as well.
    for (uint8_t i = 0; i < kGraphNavOrderCount; i++) {
        if (y + navLineH > maxListBottom) {
            break;
        }
        if (i == 4) {
            // separator between Insight and Urban groups
            y += groupGap;
        }
        GraphValue v = kGraphNavOrder[i];
        const char* base = getGraphTitle(v);
        // Use shorter label for Urban PM (avoid "Air quality" / long translations)
        if (v == GraphValue::URBAN_AIR) {
            base = INTL_DISP_AIR;
        }
        if (graphSourceSuffix(v) == 'U') {
            drawItem(urbanSuffix(base, buf, sizeof(buf)), selected == v);
        } else {
            drawItem(insightSuffix(base, buf, sizeof(buf)), selected == v);
        }
    }
}
} // namespace

// Draw a single, full-width graph for the currently selected value.
// The graph is positioned to sit below the header and above the bottom navigation bar.
static ActiveGraphSummary drawActiveGraph(GraphValue value,
                                         const String& urban_key,
                                         uint16_t navTop,
                                         uint16_t contentTop,
                                         uint16_t graphLeft,
                                         uint16_t graphWidth,
                                         uint16_t contentRight) {
    ActiveGraphSummary summary;
    // Horizontal and vertical margins
    const uint16_t topMargin  = contentTop;
    const uint16_t bottomGap  = 6;   // gap between graph and nav bar separator

    if (navTop <= topMargin + bottomGap + 40) {
        return summary;
    }

    uint16_t graphHeight = navTop - topMargin - bottomGap;
    uint16_t graphBottom = navTop - bottomGap;

    GraphPainter graph(graphLeft, graphBottom, graphHeight, graphWidth);
    graph.setWhiteMode();

    LineData lineData[2] = {};
    int      lineCount   = 0;

    // We'll determine hours_back after checking data, but for now use 12 as max
    // This will be updated after we calculate the actual time range
    uint8_t hours_to_read = 12;

    // Capture last values for header summary even if only one line has data.
    bool  has_pm10 = false, has_pm25 = false;
    float last_pm10 = 0,   last_pm25 = 0;
    
    auto addLine = [&](const char* sensor_name,
                       const char* field_name,
                       const char* label,
                       GraphLineStyle style,
                       bool* out_has_last = nullptr,
                       float* out_last = nullptr) {
        if (lineCount >= 2) {
            return;
        }
        LineData &ld = lineData[lineCount];
        readSensorDataFromCSV(ld, sensor_name, field_name, hours_to_read);
        if (ld.count > 0 && ld.values && ld.timestamps) {
            // Filter to hourly intervals (keep only last value in each hour)
            filterToHourlyData(ld);
            if (ld.count > 0 && ld.values && ld.timestamps) {
                if (out_has_last && out_last) {
                    *out_has_last = true;
                    *out_last = ld.values[ld.count - 1];
                }
                graph.addLineValues(ld.values, ld.timestamps, ld.count, label, style);
                lineCount++;
            }
        } else {
            if (ld.values) {
                delete[] ld.values;
                ld.values = nullptr;
            }
            if (ld.timestamps) {
                delete[] ld.timestamps;
                ld.timestamps = nullptr;
            }
            ld.count = 0;
        }
    };

    // Default solid line style
    GraphLineStyle solid;
    solid.style         = LINE_STYLE_SOLID;
    solid.width         = DOT_PIXEL_1X1;
    solid.use_main_color = true;

    // Dotted style for secondary line in combined graphs
    GraphLineStyle dotted = solid;
    dotted.style = LINE_STYLE_DOTTED;
    // Keep normal thickness - will be adjusted per graph if needed

    // Update metrics to get current uptime
    updateMetrics();
    
    // Determine which sensor to use for temperature and humidity based on uptime
    // First 5 minutes (300 seconds): use BME680
    // After 5 minutes: use SCD4x
    bool use_bme680_for_temp_hum = (system_metrics.uptime_sec < 300);

    switch (value) {
        case GraphValue::INSIGHT_TEMP:
            // Single-line Insight graphs: show only "Insight: <value>" in legend
            // Use BME680 for first 5 minutes, then switch to SCD4x
            if (use_bme680_for_temp_hum) {
                addLine("BME680", "temperature", "Insight", solid);
            } else {
                addLine("SCD4x", "temperature", "Insight", solid);
            }
            break;
        case GraphValue::INSIGHT_HUM:
            // Use BME680 for first 5 minutes, then switch to SCD4x
            if (use_bme680_for_temp_hum) {
                addLine("BME680", "humidity", "Insight", solid);
            } else {
                addLine("SCD4x", "humidity", "Insight", solid);
            }
            break;
        case GraphValue::INSIGHT_CO2:
            addLine("SCD4x", "co2", "Insight", solid);
            break;
        case GraphValue::INSIGHT_PRESSURE:
            addLine("BME680", "pressure", "Insight", solid);
            break;
        case GraphValue::URBAN_AIR: {
            // Both same thickness but different styles for distinction
            GraphLineStyle airSolid = solid;
            airSolid.width = DOT_PIXEL_1X1;  
            GraphLineStyle airDotted = solid;  
            airDotted.style = LINE_STYLE_DOTTED;
            airDotted.width = DOT_PIXEL_1X1;  
            addLine(urban_key.c_str(), "SDS_P1", "PM10", airSolid,  &has_pm10, &last_pm10);
            addLine(urban_key.c_str(), "SDS_P2", "PM2.5", airDotted, &has_pm25, &last_pm25);
            break;
        }
        case GraphValue::URBAN_NOISE:
            // Combined: Max (solid) + Avg (dotted) - keep distinct labels
            addLine(urban_key.c_str(), "PCBA_noiseMax", "Max", solid);
            addLine(urban_key.c_str(), "PCBA_noiseAvg", "Avg", dotted);
            break;
        case GraphValue::URBAN_TEMP:
            // Single-line Urban graphs: show only "Urban: <value>" in legend
            addLine(urban_key.c_str(), "BME280_temperature", "Urban", solid);
            break;
        case GraphValue::URBAN_HUM:
            addLine(urban_key.c_str(), "BME280_humidity", "Urban", solid);
            break;
        case GraphValue::URBAN_PRESSURE:
            addLine(urban_key.c_str(), "BME280_pressure", "Urban", solid);
            break;
    }

    // Precompute current summary for Air even if timestamp span logic fails.
    if (value == GraphValue::URBAN_AIR && (has_pm10 || has_pm25)) {
        char pm10_buf[8] = "--";
        char pm25_buf[8] = "--";
        if (has_pm10) snprintf(pm10_buf, sizeof(pm10_buf), "%d", (int)(last_pm10 + 0.5f));
        if (has_pm25) snprintf(pm25_buf, sizeof(pm25_buf), "%d", (int)(last_pm25 + 0.5f));
        snprintf(summary.headline, sizeof(summary.headline), "pm10/2.5 = %s/%sug/m3", pm10_buf, pm25_buf);
        summary.has_data = true;
    }

    if (lineCount > 0) {
        // Calculate actual time range from data to set dynamic show_hours
        time_t now = time(nullptr);
        uint32_t oldest_timestamp = UINT32_MAX;
        uint32_t newest_timestamp = 0;
        
        for (int i = 0; i < lineCount; i++) {
            if (lineData[i].count > 0 && lineData[i].timestamps) {
                // Find oldest and newest timestamps across all lines
                for (int j = 0; j < lineData[i].count; j++) {
                    if (lineData[i].timestamps[j] < oldest_timestamp) {
                        oldest_timestamp = lineData[i].timestamps[j];
                    }
                    if (lineData[i].timestamps[j] > newest_timestamp) {
                        newest_timestamp = lineData[i].timestamps[j];
                    }
                }
            }
        }
        
        if (oldest_timestamp != UINT32_MAX && newest_timestamp > 0) {
            markGraphDataState(value, true);
            // Compose header-style summary from the latest values (when available)
            if (value == GraphValue::URBAN_NOISE && lineCount >= 2) {
                // Expect line 0 = Max, line 1 = Avg
                float maxV = lineData[0].values ? lineData[0].values[lineData[0].count - 1] : 0;
                float avgV = lineData[1].values ? lineData[1].values[lineData[1].count - 1] : 0;
                // Match requested order/format: avg/max = 43/47db
                snprintf(summary.headline, sizeof(summary.headline), "avg/max = %d/%ddb",
                         (int)(avgV + 0.5f), (int)(maxV + 0.5f));
                summary.has_data = true;
            } else if (value == GraphValue::URBAN_AIR) {
                // Show current PM values even if only one line has data.
                if (has_pm10 || has_pm25) {
                    char pm10_buf[8] = "--";
                    char pm25_buf[8] = "--";
                    if (has_pm10) snprintf(pm10_buf, sizeof(pm10_buf), "%d", (int)(last_pm10 + 0.5f));
                    if (has_pm25) snprintf(pm25_buf, sizeof(pm25_buf), "%d", (int)(last_pm25 + 0.5f));
                    snprintf(summary.headline, sizeof(summary.headline), "pm10/2.5 = %s/%sug/m3", pm10_buf, pm25_buf);
                    summary.has_data = true;
                }
            } else if (lineCount >= 1 && lineData[0].values) {
                // Single-line graphs: show last value only (no "Now =" prefix; header already says "Current ...")
                float v = lineData[0].values[lineData[0].count - 1];
                int rounded = (int)(v + (v >= 0 ? 0.5f : -0.5f));
                const char* unit = "";
                switch (value) {
                    case GraphValue::INSIGHT_TEMP:
                    case GraphValue::URBAN_TEMP:
                        unit = "C";
                        break;
                    case GraphValue::INSIGHT_HUM:
                    case GraphValue::URBAN_HUM:
                        unit = "%";
                        break;
                    case GraphValue::INSIGHT_PRESSURE:
                    case GraphValue::URBAN_PRESSURE:
                        unit = "hPa";
                        break;
                    case GraphValue::INSIGHT_CO2:
                        unit = "ppm";
                        break;
                    default:
                        unit = "";
                        break;
                }
                if (unit[0] != '\0') {
                    snprintf(summary.headline, sizeof(summary.headline), "%d%s", rounded, unit);
                } else {
                    snprintf(summary.headline, sizeof(summary.headline), "%d", rounded);
                }
                summary.has_data = true;
            }
            // Calculate total hours of data available
            uint32_t data_span_seconds = newest_timestamp - oldest_timestamp;
            float total_data_hours_float = (float)data_span_seconds / 3600.0f;
            
            // Check if we have less than 1 hour of data
            if (total_data_hours_float < 1.0f) {
                // Show helpful message instead of graph - centered vertically in available space
                uint16_t graphCenterX = graphLeft + graphWidth / 2;
                uint16_t availableHeight = navTop - topMargin;
                uint16_t graphCenterY = topMargin + availableHeight / 2;
                
                const char* msg1 = INTL_DISP_NOT_ENOUGH_DATA_YET;
                const char* msg2 = INTL_DISP_COLLECTING_DATA;
                
                uint16_t msg1Width = Paint_GetStringWidth_Display(msg1, &Font16, &font_16_cyrillic, &font_16_ascii);
                uint16_t msg2Width = Paint_GetStringWidth_Display(msg2, &Font12, &font_12_cyrillic, &font_12_ascii);
                
                
                uint16_t line_spacing = 8;
                uint16_t total_msg_height = Font16.Height + Font12.Height + line_spacing;
                
                uint16_t msg1X = graphCenterX - msg1Width / 2;
                uint16_t msg2X = graphCenterX - msg2Width / 2;
                uint16_t msg1Y = graphCenterY - total_msg_height / 2;
                uint16_t msg2Y = msg1Y + Font16.Height + line_spacing;
                
                Paint_DrawString_Display(msg1X, msg1Y, msg1, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
                Paint_DrawString_Display(msg2X, msg2Y, msg2, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
                
                for (int i = 0; i < lineCount; i++) {
                    if (lineData[i].values) {
                        delete[] lineData[i].values;
                        lineData[i].values = nullptr;
                    }
                    if (lineData[i].timestamps) {
                        delete[] lineData[i].timestamps;
                        lineData[i].timestamps = nullptr;
                    }
                    lineData[i].count = 0;
                }
                return summary;
            }
            
            uint8_t total_data_hours = (uint8_t)(total_data_hours_float + 0.5f);  // Round to nearest
            
            // Dynamic time range logic:
            // - If <= 12 hours of data: show all available data (1, 2, 3... up to 12 hours)
            // - If > 12 hours: show sliding window of last 12 hours (new data appears as old data falls out)
            uint8_t display_hours;
            if (total_data_hours <= 12) {
                display_hours = total_data_hours;
            } else {
                display_hours = 12;  // Sliding window: show last 12 hours
                // When >12 hours, we need to re-read data with only last 12 hours for proper sliding window
                // But since we already read with 12 hours, the data is already filtered correctly
            }
            
            // Ensure at least 1 hour
            if (display_hours < 1) display_hours = 1;
            
            graph.setShowHours(display_hours);
        } else {
            // No timestamps found - show message
            markGraphDataState(value, false);
            if (!shouldShowNoDataMessage(value)) {
                for (int i = 0; i < lineCount; i++) {
                    if (lineData[i].values) {
                        delete[] lineData[i].values;
                        lineData[i].values = nullptr;
                    }
                    if (lineData[i].timestamps) {
                        delete[] lineData[i].timestamps;
                        lineData[i].timestamps = nullptr;
                    }
                    lineData[i].count = 0;
                }
                return summary;
            }
            uint16_t graphCenterX = graphLeft + graphWidth / 2;
            uint16_t graphCenterY = navTop - (navTop - topMargin) / 2;
            
            const char* msg1 = INTL_DISP_NO_DATA_AVAILABLE;
            const char* msg2 = INTL_DISP_COLLECTING_DATA;
            
            uint16_t msg1Width = Paint_GetStringWidth_Display(msg1, &Font16, &font_16_cyrillic, &font_16_ascii);
            uint16_t msg2Width = Paint_GetStringWidth_Display(msg2, &Font12, &font_12_cyrillic, &font_12_ascii);
            
            uint16_t msg1X = graphCenterX - msg1Width / 2;
            uint16_t msg2X = graphCenterX - msg2Width / 2;
            uint16_t msg1Y = graphCenterY - Font16.Height / 2 - 4;
            uint16_t msg2Y = graphCenterY + Font12.Height / 2 + 4;
            
            Paint_DrawString_Display(msg1X, msg1Y, msg1, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
            Paint_DrawString_Display(msg2X, msg2Y, msg2, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
            
            // Clean up and return early
            for (int i = 0; i < lineCount; i++) {
                if (lineData[i].values) {
                    delete[] lineData[i].values;
                    lineData[i].values = nullptr;
                }
                if (lineData[i].timestamps) {
                    delete[] lineData[i].timestamps;
                    lineData[i].timestamps = nullptr;
                }
                lineData[i].count = 0;
            }
            return summary;
        }
        
        graph.drawGraph();
    } else {
        // No data lines at all - show message
        markGraphDataState(value, false);
        if (!shouldShowNoDataMessage(value)) {
            return summary;
        }
        uint16_t graphCenterX = graphLeft + graphWidth / 2;
        uint16_t graphCenterY = navTop - (navTop - topMargin) / 2;
        
        const char* msg1 = INTL_DISP_NO_DATA_AVAILABLE;
        const char* msg2 = INTL_DISP_COLLECTING_DATA;
        
        uint16_t msg1Width = Paint_GetStringWidth_Display(msg1, &Font16, &font_16_cyrillic, &font_16_ascii);
        uint16_t msg2Width = Paint_GetStringWidth_Display(msg2, &Font12, &font_12_cyrillic, &font_12_ascii);
        
        uint16_t msg1X = graphCenterX - msg1Width / 2;
        uint16_t msg2X = graphCenterX - msg2Width / 2;
        uint16_t msg1Y = graphCenterY - Font16.Height / 2 - 4;
        uint16_t msg2Y = graphCenterY + Font12.Height / 2 + 4;
        
        Paint_DrawString_Display(msg1X, msg1Y, msg1, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
        Paint_DrawString_Display(msg2X, msg2Y, msg2, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        return summary;
    }

    // Clean up allocated buffers
    for (int i = 0; i < lineCount; i++) {
        if (lineData[i].values) {
            delete[] lineData[i].values;
            lineData[i].values = nullptr;
        }
        if (lineData[i].timestamps) {
            delete[] lineData[i].timestamps;
            lineData[i].timestamps = nullptr;
        }
        lineData[i].count = 0;
    }
    return summary;
}
#endif

// Helper function to check if we have at least 1 hour of data
static bool hasEnoughData() {
#if defined(USE_SD_CARD)
    if (!checkSDCardAvailable() || !checkDataFilesExist()) {
        return false;
    }
    
    // Robust check: try a few common fields across Insight/Urban.
    // Some installations may not log BME680 temperature but still have lots of Urban data (PM/noise/etc).
    struct Probe {
        const char* sensor;
        const char* field;
    };
    const Probe probes[] = {
        {"SCD4x", "co2"},
        {"BME680", "temperature"},
        {"BME680", "humidity"},
        {"BME680", "pressure"},
        {ATRUIST_URBAN_SENSOR, "SDS_P1"},
        {ATRUIST_URBAN_SENSOR, "SDS_P2"},
        {ATRUIST_URBAN_SENSOR, "PCBA_noiseAvg"},
        {ATRUIST_URBAN_SENSOR, "BME280_temperature"},
        {ATRUIST_URBAN_SENSOR, "BME280_humidity"},
        {ATRUIST_URBAN_SENSOR, "BME280_pressure"},
    };

    for (const auto& p : probes) {
        LineData testData = {};
        readSensorDataFromCSV(testData, p.sensor, p.field, 12);
        if (testData.count > 0 && testData.timestamps) {
            filterToHourlyData(testData);
            if (testData.count >= 2 && testData.timestamps) {
                uint32_t oldest = UINT32_MAX;
                uint32_t newest = 0;
                for (int i = 0; i < testData.count; i++) {
                    if (testData.timestamps[i] < oldest) oldest = testData.timestamps[i];
                    if (testData.timestamps[i] > newest) newest = testData.timestamps[i];
                }
                if (oldest != UINT32_MAX && newest > oldest) {
                    float total_data_hours = (float)(newest - oldest) / 3600.0f;
                    if (testData.values) { delete[] testData.values; }
                    if (testData.timestamps) { delete[] testData.timestamps; }
                    if (total_data_hours >= 1.0f) {
                        return true;
                    }
                }
            }
        }
        if (testData.values) { delete[] testData.values; }
        if (testData.timestamps) { delete[] testData.timestamps; }
    }
    return false;
#else
    return false;  // SD card not compiled in
#endif
}

// Public function to check if graphs are available
bool areGraphsAvailable() {
#if defined(USE_SD_CARD)
    // “Available” here controls navigation behavior (cycle values vs switch screens).
    // Even if we don't yet have 1h of history, we still want the graphs screen to be usable;
    // the renderer will show "collecting data" as needed.
    return checkSDCardAvailable() && checkDataFilesExist();
#else
    return false;  // SD card not compiled in
#endif
}

void drawGraphScreen() {
    // Clear screen first to prevent glitching
    Paint_Clear(WHITE);
    
    String screenMsg = "Set graph screen " + String(current_graph_screen);
    debug_outln_info(screenMsg);

    // === HEADER: left icon, left headline, right date ===
    struct tm timeinfo;
    const uint16_t header_top_y = 6;
    const uint16_t header_row_height = Font16.Height + 2;
    uint16_t header_bottom_border_y = header_top_y + header_row_height + 2;

    // Left: graphs screen icon 
    const uint16_t header_icon_size = 15;
    const uint16_t header_icon_x    = 4;
    const uint16_t header_icon_y    = header_top_y;
    Paint_DrawImage(chart_15x15, header_icon_x, header_icon_y, header_icon_size, header_icon_size);

    // Prepare date on the right (like the reference)
    bool has_time = false;
    char date_buf[16] = {0};
    int date_x = 0;
    if (getLocalTime(&timeinfo)) {
        strftime(date_buf, sizeof(date_buf), "%m/%d/%Y", &timeinfo);
        int date_width = (int)Paint_GetStringWidth_Display(date_buf, &Font16, &font_16_cyrillic, &font_16_ascii);
        const int right_margin = 4;
        date_x = DISPLAY_WIDTH - right_margin - date_width;
        has_time = true;
    }

    // Left headline (filled after we draw/compute the active graph)
    const int min_x = header_icon_x + header_icon_size + 6;
    int headline_x = min_x;
    uint16_t headline_y = header_top_y;

    // Right: date
    if (has_time) {
        int date_y = header_top_y;
        Paint_DrawString_Display(date_x, date_y, date_buf, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    }

    // Bottom border for header
    Paint_DrawLine(0, header_bottom_border_y, DISPLAY_WIDTH, header_bottom_border_y,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // No bottom nav on graphs screen (right-side menu is the navigator)
    const uint16_t navTop = DISPLAY_HEIGHT - 2;

#if defined(USE_SD_CARD)
    // Check if SD card is available
    if (!checkSDCardAvailable()) {
        // Center text vertically with proper spacing (3 lines total)
        uint16_t line_spacing = 4;
        uint16_t total_height = Font16.Height + Font12.Height * 2 + line_spacing * 2;
        uint16_t start_y = (navTop - total_height) / 2;  // Account for nav bar at bottom
        
        const char* sd1 = INTL_DISP_SD_NOT_FOUND;
        const char* sd2 = INTL_DISP_INSERT_SD;
        const char* sd3 = INTL_DISP_FAT32_FORMATTED;
        uint16_t x1 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(sd1, &Font16, &font_16_cyrillic, &font_16_ascii) / 2;
        Paint_DrawString_Display(x1, start_y, sd1, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
        uint16_t y2 = start_y + Font16.Height + line_spacing;
        uint16_t x2 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(sd2, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
        Paint_DrawString_Display(x2, y2, sd2, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        uint16_t y3 = y2 + Font12.Height + line_spacing;
        uint16_t x3 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(sd3, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
        Paint_DrawString_Display(x3, y3, sd3, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        // Continue to draw nav bar below
    }
    // Check if any data files exist (only if SD card is available)
    else if (!checkDataFilesExist()) {
        uint16_t line_spacing = 4;
        uint16_t total_height = Font16.Height + Font12.Height * 3 + line_spacing * 3;
        uint16_t start_y = (navTop - total_height) / 2;  
        
        const char* nd1 = INTL_DISP_NO_DATA_FILES;
        const char* nd2 = INTL_DISP_DEVICE_WILL_CREATE;
        const char* nd3 = INTL_DISP_FILES_AUTOMATICALLY;
        const char* nd4 = INTL_DISP_AFTER_COLLECTING;
        uint16_t x1 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(nd1, &Font16, &font_16_cyrillic, &font_16_ascii) / 2;
        Paint_DrawString_Display(x1, start_y, nd1, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
        uint16_t y2 = start_y + Font16.Height + line_spacing;
        uint16_t x2 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(nd2, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
        Paint_DrawString_Display(x2, y2, nd2, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        uint16_t y3 = y2 + Font12.Height + line_spacing;
        uint16_t x3 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(nd3, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
        Paint_DrawString_Display(x3, y3, nd3, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        uint16_t y4 = y3 + Font12.Height + line_spacing;
        uint16_t x4 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(nd4, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
        Paint_DrawString_Display(x4, y4, nd4, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        // Continue to draw nav bar below
    }
    else {
    
        String urban_key = ATRUIST_URBAN_SENSOR;
        // Layout experiment: value menu on the LEFT, graph on the RIGHT.
        // Keep the far-right screen indicator sidebar intact.
        const uint16_t indicatorSidebarW = 26; // must match `drawScreenIndicator()`
        const uint16_t contentRight = DISPLAY_WIDTH - indicatorSidebarW - 2;
        const uint16_t contentTop = header_bottom_border_y + 6;

        // Put the nav column fully flush to the left edge.
        const uint16_t plotMarginL = 3;
        // Make the graph a bit wider (nav still fits 2-line hint).
        const uint16_t menuW = 120;
        const uint16_t menuGap = 10;
        uint16_t menuX = plotMarginL;
        uint16_t menuY = contentTop;
        uint16_t menuH = (navTop > contentTop + 8) ? (navTop - contentTop - 8) : 0;

        // Plot area uses all remaining width to the RIGHT of the menu.
        uint16_t plotLeft = menuX + menuW + menuGap;
        uint16_t plotRight = contentRight;
        uint16_t plotW = (plotRight > plotLeft) ? (plotRight - plotLeft) : 0;

        ActiveGraphSummary summary = {};
        if (plotW > 40 && menuH > 40) {
            summary = drawActiveGraph(current_graph_value, urban_key, navTop, contentTop, plotLeft, plotW, contentRight);
            drawGraphValueMenu(menuX, menuY, menuW, menuH, current_graph_value);
            drawGraphNavButtonsBottomLeft(menuX, menuY, menuW, menuH);
        } else if (plotW > 40) {
            summary = drawActiveGraph(current_graph_value, urban_key, navTop, contentTop, plotLeft, plotW, contentRight);
        }

        // Header headline: "Current <Metric>: <summary>" (or just the metric name if no data yet)
        const char* metricBase = getGraphTitle(current_graph_value);
        // Use shorter label for Urban PM (matches mock text better)
        if (current_graph_value == GraphValue::URBAN_AIR) {
            metricBase = INTL_DISP_AIR;
        }
        char metric[48] = {0};
        if (isSharedMetric(current_graph_value)) {
            char suf = graphSourceSuffix(current_graph_value);
            if (suf == 'U') {
                urbanSuffix(metricBase, metric, sizeof(metric));
            } else if (suf == 'I') {
                insightSuffix(metricBase, metric, sizeof(metric));
            } else {
                snprintf(metric, sizeof(metric), "%s", metricBase);
            }
        } else {
            snprintf(metric, sizeof(metric), "%s", metricBase);
        }
        char headline[96] = {0};
        const bool hasPrefix = (INTL_DISP_GRAPHS_HEADER_PREFIX[0] != '\0');
        // Header format requested:
        // - EN: "Current - 17C" / "Current - avg/max = 43/47db"
        // - RU: prefix may be empty => show just the summary.
        if (summary.has_data && summary.headline[0] != '\0') {
            if (hasPrefix) {
                snprintf(headline, sizeof(headline), "%s - %s", INTL_DISP_GRAPHS_HEADER_PREFIX, summary.headline);
            } else {
                snprintf(headline, sizeof(headline), "%s", summary.headline);
            }
        } else {
            if (hasPrefix) {
                snprintf(headline, sizeof(headline), "%s", INTL_DISP_GRAPHS_HEADER_PREFIX);
            } else {
                snprintf(headline, sizeof(headline), "%s", metric);
            }
        }
        // Clamp headline so it doesn't overlap the date
        int max_right = has_time ? (date_x - 6) : (DISPLAY_WIDTH - 4);
        // Use 14px glyph font for header.
        sFONT* headerFontEn = &Font12;
        const Font* headerFontRu = &font_14_cyrillic;
        const Font* headerFontAscii = &font_14_ascii;
        int headline_w = (int)Paint_GetStringWidth_Display(headline, headerFontEn, headerFontRu, headerFontAscii);
        if (headline_x + headline_w > max_right) {
            // If too long, fall back to a shorter string (and if still too long, clip).
            if (summary.has_data && summary.headline[0] != '\0') {
                snprintf(headline, sizeof(headline), "%s", summary.headline);
            } else {
                snprintf(headline, sizeof(headline), "%s", metric);
            }
            headline_w = (int)Paint_GetStringWidth_Display(headline, headerFontEn, headerFontRu, headerFontAscii);
            if (headline_x + headline_w > max_right) {
                // Clip with ".."
                while (strlen(headline) > 2) {
                    headline[strlen(headline) - 1] = '\0';
                    headline_w = (int)Paint_GetStringWidth_Display(headline, headerFontEn, headerFontRu, headerFontAscii);
                    if (headline_x + headline_w <= max_right) break;
                }
                if (strlen(headline) > 2) {
                    headline[strlen(headline) - 1] = '.';
                    headline[strlen(headline) - 2] = '.';
                }
            }
        }
        Paint_DrawString_Display(headline_x, headline_y, headline, headerFontEn, headerFontRu, headerFontAscii, WHITE, BLACK);
    }
#else
    // SD card not available - show message
    uint16_t line_spacing = 4;
    uint16_t total_height = Font16.Height + Font12.Height * 2 + line_spacing * 2;
    uint16_t start_y = (navTop - total_height) / 2;  // Account for nav bar at bottom
    
    const char* sa1 = INTL_DISP_SD_NOT_AVAILABLE;
    const char* sa2 = INTL_DISP_GRAPHS_REQUIRE_SD;
    const char* sa3 = INTL_DISP_ENABLE_SD;
    uint16_t x1 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(sa1, &Font16, &font_16_cyrillic, &font_16_ascii) / 2;
    Paint_DrawString_Display(x1, start_y, sa1, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    uint16_t y2 = start_y + Font16.Height + line_spacing;
    uint16_t x2 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(sa2, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
    Paint_DrawString_Display(x2, y2, sa2, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    uint16_t y3 = y2 + Font12.Height + line_spacing;
    uint16_t x3 = DISPLAY_WIDTH / 2 - Paint_GetStringWidth_Display(sa3, &Font12, &font_12_cyrillic, &font_12_ascii) / 2;
    Paint_DrawString_Display(x3, y3, sa3, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    // (No bottom nav on this screen)
#endif

    // Bottom navigation removed.

}

void setNextGraphScreen() {
    if (current_graph_screen < 3) {
        current_graph_screen++;
    } else {
        current_graph_screen = 1;
    }
}

void setPrevGraphScreen() {
    if (current_graph_screen > 1) {
        current_graph_screen--;
    } else {
        current_graph_screen = 3;
    }
}

bool setNextGraphValue() {
    int idx = navIndexOf(current_graph_value);
    if (idx < 0) {
        current_graph_value = kGraphNavOrder[0];
        return false;
    }
    if (idx >= (int)kGraphNavOrderCount - 1) {
        // At the last item: signal to switch to the next screen (legacy behavior).
        return true;
    }
    current_graph_value = kGraphNavOrder[idx + 1];
    return false;
}

bool setPrevGraphValue() {
    int idx = navIndexOf(current_graph_value);
    if (idx < 0) {
        current_graph_value = kGraphNavOrder[0];
        return false;
    }
    if (idx == 0) {
        // At the first item: signal to switch to the previous screen (legacy behavior).
        return true;
    }
    current_graph_value = kGraphNavOrder[idx - 1];
    return false;
}

#endif

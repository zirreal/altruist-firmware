#ifdef ALTRUIST_INSIDE

#include "../paint_driver/graphPainter.h"
#include "graph.h"
#include "../utils.h"
#include "../../utils.h"
#include "../../sd_card/sd_card.h"
#include "../../config_manager/config_helpers.h"
#include "../icons/icons/15x15/buttons_nav_15x15.h"

#if defined(USE_SD_CARD)
#include "SD.h"
#include "FS.h"
#endif

uint8_t current_graph_screen = 1;
static GraphValue current_graph_value = GraphValue::INSIGHT_TEMP;

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
        debug_outln_info(F("[Graph] SD card removed - updating status"));
        return false;
    }
    return true;
}

static bool checkDataFilesExist() {
    // First, verify card is still present (detect removal)
    if (!sdCardLogger.checkInserted()) {
        debug_outln_info(F("[Graph] SD card removed during file check"));
        return false;
    }
    
    // Refresh SD card cache periodically to detect new files
    unsigned long now = millis();
    if (now - last_file_check_time > FILE_CHECK_INTERVAL_MS || last_file_check_time == 0) {
        debug_outln_info(F("[Graph] Refreshing SD card cache..."));
        sdCardLogger.refreshCache();
        last_file_check_time = now;
    }
    
    // Check if root folder exists
    if (!SD.exists("/sensors_data")) {
        debug_outln_info(F("[Graph] /sensors_data folder does not exist"));
        return false;
    }
    
    // First, try direct check for today's files from known sensors (faster)
    time_t now_time = time(nullptr);
    struct tm* timeinfo = localtime(&now_time);
    char dateStr[11];
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", 
             timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
    
    String dateInfo = "[Graph] Checking for date: " + String(dateStr);
    debug_outln_info(dateInfo);
    
    // Check common sensor folders for today's file
    const char* sensors[] = {"SCD4x", "BME680", ATRUIST_URBAN_SENSOR};
    for (int i = 0; i < 3; i++) {
        String filePath = "/sensors_data/" + String(sensors[i]) + "/" + String(dateStr) + ".csv";
        String checkMsg = "[Graph] Checking file: " + filePath;
        debug_outln_info(checkMsg);
        if (SD.exists(filePath)) {
            File testFile = SD.open(filePath, FILE_READ);
            if (testFile && testFile.size() > 0) {
                String foundMsg = "[Graph] Found today's file: " + filePath + " size: " + String(testFile.size());
                debug_outln_info(foundMsg);
                testFile.close();
                return true;
            }
            if (testFile) {
                String emptyMsg = "[Graph] File exists but empty or can't read: " + filePath;
                debug_outln_info(emptyMsg);
                testFile.close();
            }
        } else {
            String notFoundMsg = "[Graph] File does not exist: " + filePath;
            debug_outln_info(notFoundMsg);
        }
    }
    
    // If direct check didn't find files, do full directory scan
    File root = SD.open("/sensors_data");
    if (!root || !root.isDirectory()) {
        debug_outln_info(F("[Graph] Failed to open /sensors_data directory"));
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
            debug_outln_info(folderInfo);
            File sensorDir = SD.open(sensorPath);
            if (sensorDir && sensorDir.isDirectory()) {
                File csvFile = sensorDir.openNextFile();
                while (csvFile) {
                    String fileName = csvFile.name();
                    fileCount++;
                    String fileInfo = "[Graph] Found file: " + fileName + " size: " + String(csvFile.size());
                    debug_outln_info(fileInfo);
                    if (fileName.endsWith(".csv") && csvFile.size() > 0) {
                        hasData = true;
                        debug_outln_info(F("[Graph] Found valid CSV file with data"));
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
    debug_outln_info(checkResult);
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

// Draw a single, full-width graph for the currently selected value.
// The graph is positioned to sit above the bottom navigation bar.
static void drawActiveGraph(GraphValue value, const String& urban_key, uint16_t navTop) {
    // Horizontal and vertical margins
    const uint16_t marginX    = 10;
    const uint16_t topMargin  = 10;
    const uint16_t bottomGap  = 6;   // gap between graph and nav bar separator

    if (navTop <= topMargin + bottomGap + 40) {
        return;
    }

    uint16_t graphLeft   = marginX;
    uint16_t graphWidth  = DISPLAY_WIDTH - 2 * marginX;
    uint16_t graphHeight = navTop - topMargin - bottomGap;
    uint16_t graphBottom = navTop - bottomGap;

    GraphPainter graph(graphLeft, graphBottom, graphHeight, graphWidth);
    graph.setWhiteMode();

    LineData lineData[2] = {};
    int      lineCount   = 0;

    auto addLine = [&](const char* sensor_name,
                       const char* field_name,
                       const char* label,
                       GraphLineStyle style) {
        if (lineCount >= 2) {
            return;
        }
        LineData &ld = lineData[lineCount];
        readSensorDataFromCSV(ld, sensor_name, field_name, 12);
        if (ld.count > 0 && ld.values && ld.timestamps) {
            graph.addLineValues(ld.values, ld.timestamps, ld.count, label, style);
            lineCount++;
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

    switch (value) {
        case GraphValue::INSIGHT_TEMP:
            addLine("BME680", "temperature", "Insight Temp", solid);
            break;
        case GraphValue::INSIGHT_HUM:
            addLine("BME680", "humidity", "Insight Hum", solid);
            break;
        case GraphValue::INSIGHT_CO2:
            addLine("SCD4x", "co2", "Insight CO2", solid);
            break;
        case GraphValue::INSIGHT_PRESSURE:
            addLine("BME680", "pressure", "Insight Press", solid);
            break;
        case GraphValue::URBAN_AIR:
            // Combined: PM10 (solid) + PM2.5 (dotted)
            addLine(urban_key.c_str(), "SDS_P1", "PM10", solid);
            addLine(urban_key.c_str(), "SDS_P2", "PM2.5", dotted);
            break;
        case GraphValue::URBAN_NOISE:
            // Combined: Max (solid) + Avg (dotted)
            addLine(urban_key.c_str(), "PCBA_noiseMax", "Max Noise", solid);
            addLine(urban_key.c_str(), "PCBA_noiseAvg", "Avg Noise", dotted);
            break;
        case GraphValue::URBAN_TEMP:
            addLine(urban_key.c_str(), "BME280_temperature", "Urban Temp", solid);
            break;
        case GraphValue::URBAN_HUM:
            addLine(urban_key.c_str(), "BME280_humidity", "Urban Hum", solid);
            break;
        case GraphValue::URBAN_PRESSURE:
            addLine(urban_key.c_str(), "BME280_pressure", "Urban Press", solid);
            break;
    }

    if (lineCount > 0) {
        graph.drawGraph();
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
}
#endif

void drawGraphScreen() {
    // Clear screen first to prevent glitching
    Paint_Clear(WHITE);
    
    String screenMsg = "Set graph screen " + String(current_graph_screen);
    debug_outln_info(screenMsg);

    // Check if SD card is available
    if (!checkSDCardAvailable()) {
        // Center text vertically with proper spacing (3 lines total)
        uint16_t line_spacing = 4;
        uint16_t total_height = Font16.Height + Font12.Height * 2 + line_spacing * 2;
        uint16_t start_y = (DISPLAY_HEIGHT - total_height) / 2;
        
        uint16_t x1 = DISPLAY_WIDTH / 2 - strlen("SD card not found") * Font16.Width / 2;
        Paint_DrawString_EN(x1, start_y, "SD card not found", &Font16, WHITE, BLACK);
        
        uint16_t y2 = start_y + Font16.Height + line_spacing;
        uint16_t x2 = DISPLAY_WIDTH / 2 - strlen("Please insert SD card") * Font12.Width / 2;
        Paint_DrawString_EN(x2, y2, "Please insert SD card", &Font12, WHITE, BLACK);
        
        uint16_t y3 = y2 + Font12.Height + line_spacing;
        uint16_t x3 = DISPLAY_WIDTH / 2 - strlen("(FAT32 formatted)") * Font12.Width / 2;
        Paint_DrawString_EN(x3, y3, "(FAT32 formatted)", &Font12, WHITE, BLACK);
        return;
    }
    
    // Reserve space for the bottom navigation bar
    const uint16_t navBarHeight = 60;               
    const uint16_t navTop       = DISPLAY_HEIGHT - navBarHeight;

    // Check if any data files exist
    if (!checkDataFilesExist()) {
        // Center text vertically with proper spacing (4 lines total)
        uint16_t line_spacing = 4;
        uint16_t total_height = Font16.Height + Font12.Height * 3 + line_spacing * 3;
        uint16_t start_y = (DISPLAY_HEIGHT - total_height) / 2;
        
        uint16_t x1 = DISPLAY_WIDTH / 2 - strlen("No data files found") * Font16.Width / 2;
        Paint_DrawString_EN(x1, start_y, "No data files found", &Font16, WHITE, BLACK);
        
        uint16_t y2 = start_y + Font16.Height + line_spacing;
        uint16_t x2 = DISPLAY_WIDTH / 2 - strlen("Device will create") * Font12.Width / 2;
        Paint_DrawString_EN(x2, y2, "Device will create", &Font12, WHITE, BLACK);
        
        uint16_t y3 = y2 + Font12.Height + line_spacing;
        uint16_t x3 = DISPLAY_WIDTH / 2 - strlen("files automatically") * Font12.Width / 2;
        Paint_DrawString_EN(x3, y3, "files automatically", &Font12, WHITE, BLACK);
        
        uint16_t y4 = y3 + Font12.Height + line_spacing;
        uint16_t x4 = DISPLAY_WIDTH / 2 - strlen("after collecting data") * Font12.Width / 2;
        Paint_DrawString_EN(x4, y4, "after collecting data", &Font12, WHITE, BLACK);
        return;
    }
    
    // String urban_ip = cfg::chosen_altruist_urban;
    // int last_dot = urban_ip.lastIndexOf('.');
    // if (last_dot == -1) {
    //     debug_outln_info(F("Invalid IP address in cfg::chosen_altruist_urban"));
    //     return;
    // }
    // String last_octet = urban_ip.substring(last_dot + 1);
    // String urban_key = ATRUIST_URBAN_SENSOR + last_octet;
    String urban_key = ATRUIST_URBAN_SENSOR;
    // Draw single active graph based on current_graph_value
    drawActiveGraph(current_graph_value, urban_key, navTop);

    Paint_DrawLine(0, navTop, DISPLAY_WIDTH, navTop, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Left/right padding inside bar
    const uint16_t paddingX = 6;

    // Section widths: small icon column on the left, then Insight + Urban sections
    const uint16_t iconColWidth = 24; 
    uint16_t x = paddingX;

    // Icon column
    uint16_t iconAreaRight = x + iconColWidth;
    const uint16_t iconSizeWidth  = 15;
    const uint16_t iconSizeHeight = 25;
    uint16_t sectionHeaderH = Font8.Height + 6;
    uint16_t headerTop      = navTop + 1;
    uint16_t headerCenterY  = headerTop + sectionHeaderH / 2;
    uint16_t iconY          = headerCenterY - iconSizeHeight / 2 + 20;
    uint16_t iconX = x + (iconColWidth - iconSizeWidth) / 2;
    Paint_DrawImage(buttons_nav_15x15, iconX, iconY, iconSizeWidth, iconSizeHeight);

    x = iconAreaRight;

    // Vertical divider after icon column
    Paint_DrawLine(x, navTop, x, DISPLAY_HEIGHT - 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    x += 2; // small gap after divider

    // Available width for Insight + Urban sections
    uint16_t availableWidth = DISPLAY_WIDTH - x - paddingX;
    uint16_t insightWidth   = (availableWidth * 4) / 9; 
    uint16_t urbanWidth     = availableWidth - insightWidth;

    // --- Insight section ---
    uint16_t insightX       = x;
    uint16_t insightHeaderY = navTop + 1;

    Paint_DrawRectangle(insightX, insightHeaderY,
                        insightX + insightWidth - 2,
                        insightHeaderY + sectionHeaderH,
                        BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(insightX, insightHeaderY + 2, "Insight", &Font8, BLACK, WHITE);

    uint16_t insightTextY = insightHeaderY + sectionHeaderH + 4;
    uint16_t textX        = insightX + 4;

    // Temperature
    const char* tempLabel = "Temperature";
    uint16_t    tempWidth = strlen(tempLabel) * Font12.Width;
    bool        tempActive = (current_graph_value == GraphValue::INSIGHT_TEMP);
    if (tempActive) {
        // Bold-ish text
        Paint_DrawString_EN(textX,     insightTextY, tempLabel, &Font12, WHITE, BLACK);
        Paint_DrawString_EN(textX + 1, insightTextY, tempLabel, &Font12, WHITE, BLACK);
        uint16_t tempCenterX = textX + tempWidth / 2;
        uint16_t tempArrowTopY  = insightTextY + Font12.Height + 1;
        if (tempArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            tempArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t tempTipY   = tempArrowTopY;
        uint16_t tempBaseY  = tempArrowTopY + 4;
        Paint_DrawLine(tempCenterX,     tempTipY,  tempCenterX - 3, tempBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(tempCenterX,     tempTipY,  tempCenterX + 3, tempBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(tempCenterX - 3, tempBaseY, tempCenterX + 3, tempBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else {
        Paint_DrawString_EN(textX, insightTextY, tempLabel, &Font12, WHITE, BLACK);
    }

    // Humidity
    textX += tempWidth + Font12.Width; // gap between words
    const char* humLabel = "Humidity";
    bool        humActive = (current_graph_value == GraphValue::INSIGHT_HUM);
    if (humActive) {
        Paint_DrawString_EN(textX,     insightTextY, humLabel, &Font12, WHITE, BLACK);
        Paint_DrawString_EN(textX + 1, insightTextY, humLabel, &Font12, WHITE, BLACK);
        uint16_t humWidth   = strlen(humLabel) * Font12.Width;
        uint16_t humCenterX = textX + humWidth / 2;
        uint16_t humArrowTopY  = insightTextY + Font12.Height + 1;
        if (humArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            humArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t humTipY   = humArrowTopY;
        uint16_t humBaseY  = humArrowTopY + 4;
        Paint_DrawLine(humCenterX,     humTipY,  humCenterX - 3, humBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(humCenterX,     humTipY,  humCenterX + 3, humBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(humCenterX - 3, humBaseY, humCenterX + 3, humBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else {
        Paint_DrawString_EN(textX, insightTextY, humLabel, &Font12, WHITE, BLACK);
    }

    // CO2 
    uint16_t insightTextY2 = insightTextY + Font12.Height + 8;
    const char* co2Label = "CO2";
    uint16_t co2X = insightX + 4;
    bool     co2Active = (current_graph_value == GraphValue::INSIGHT_CO2);
    if (co2Active) {
        Paint_DrawString_EN(co2X,     insightTextY2, co2Label, &Font12, WHITE, BLACK);
        Paint_DrawString_EN(co2X + 1, insightTextY2, co2Label, &Font12, WHITE, BLACK);
        uint16_t co2Width   = strlen(co2Label) * Font12.Width;
        uint16_t co2CenterX = co2X + co2Width / 2;
        uint16_t co2ArrowTopY  = insightTextY2 + Font12.Height + 1;
        if (co2ArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            co2ArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t co2TipY   = co2ArrowTopY;
        uint16_t co2BaseY  = co2ArrowTopY + 4;
        Paint_DrawLine(co2CenterX,     co2TipY,  co2CenterX - 3, co2BaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(co2CenterX,     co2TipY,  co2CenterX + 3, co2BaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(co2CenterX - 3, co2BaseY, co2CenterX + 3, co2BaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else {
        Paint_DrawString_EN(co2X, insightTextY2, co2Label, &Font12, WHITE, BLACK);
    }

    // Pressure
    const char* pressLabel = "Pressure";
    uint16_t    pressX     = insightX + 4 + 4 * Font12.Width;
    bool        pressActive = (current_graph_value == GraphValue::INSIGHT_PRESSURE);
    if (pressActive) {
        Paint_DrawString_EN(pressX,     insightTextY2, pressLabel, &Font12, WHITE, BLACK);
        Paint_DrawString_EN(pressX + 1, insightTextY2, pressLabel, &Font12, WHITE, BLACK);
        uint16_t pressWidth   = strlen(pressLabel) * Font12.Width;
        uint16_t pressCenterX = pressX + pressWidth / 2;
        uint16_t pressArrowTopY  = insightTextY2 + Font12.Height + 1;
        if (pressArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            pressArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t pressTipY   = pressArrowTopY;
        uint16_t pressBaseY  = pressArrowTopY + 4;
        Paint_DrawLine(pressCenterX,     pressTipY,  pressCenterX - 3, pressBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(pressCenterX,     pressTipY,  pressCenterX + 3, pressBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(pressCenterX - 3, pressBaseY, pressCenterX + 3, pressBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else {
        Paint_DrawString_EN(pressX, insightTextY2, pressLabel, &Font12, WHITE, BLACK);
    }

    // --- Urban section ---
    uint16_t urbanX        = insightX + insightWidth;
    uint16_t urbanHeaderY  = navTop + 1;

    // Vertical divider between Insight and Urban sections
    Paint_DrawLine(urbanX - 1, navTop, urbanX - 1, DISPLAY_HEIGHT - 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    Paint_DrawRectangle(urbanX, urbanHeaderY,
                        urbanX + urbanWidth - 2,
                        urbanHeaderY + sectionHeaderH,
                        BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(urbanX, urbanHeaderY + 2, "Urban", &Font8, BLACK, WHITE);

    uint16_t urbanTextY = urbanHeaderY + sectionHeaderH + 4;
    uint16_t uTextX     = urbanX + 4;

    // Air
    const char* airLabel = "Air";
    bool        airActive = (current_graph_value == GraphValue::URBAN_AIR);
    if (airActive) {
        Paint_DrawString_EN(uTextX,     urbanTextY, airLabel, &Font12, WHITE, BLACK);
        Paint_DrawString_EN(uTextX + 1, urbanTextY, airLabel, &Font12, WHITE, BLACK);
    } else {
        Paint_DrawString_EN(uTextX, urbanTextY, airLabel, &Font12, WHITE, BLACK);
    }
    uint16_t airWidth   = strlen(airLabel) * Font12.Width;
    uint16_t airCenterX = uTextX + airWidth / 2;
    if (airActive) {
        uint16_t airArrowTopY  = urbanTextY + Font12.Height + 1;
        if (airArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            airArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t airTipY   = airArrowTopY;
        uint16_t airBaseY  = airArrowTopY + 4;
        Paint_DrawLine(airCenterX,     airTipY,  airCenterX - 3, airBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(airCenterX,     airTipY,  airCenterX + 3, airBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(airCenterX - 3, airBaseY, airCenterX + 3, airBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }

    // Noise 
    uTextX += airWidth + Font12.Width;
    const char* noiseLabel = "Noise";
    bool        noiseActive = (current_graph_value == GraphValue::URBAN_NOISE);
    if (noiseActive) {
        Paint_DrawString_EN(uTextX,     urbanTextY, noiseLabel, &Font12, WHITE, BLACK);
        Paint_DrawString_EN(uTextX + 1, urbanTextY, noiseLabel, &Font12, WHITE, BLACK);
        uint16_t noiseWidth   = strlen(noiseLabel) * Font12.Width;
        uint16_t noiseCenterX = uTextX + noiseWidth / 2;
        uint16_t noiseArrowTopY  = urbanTextY + Font12.Height + 1;
        if (noiseArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            noiseArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t noiseTipY   = noiseArrowTopY;
        uint16_t noiseBaseY  = noiseArrowTopY + 4;
        Paint_DrawLine(noiseCenterX,     noiseTipY,  noiseCenterX - 3, noiseBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(noiseCenterX,     noiseTipY,  noiseCenterX + 3, noiseBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(noiseCenterX - 3, noiseBaseY, noiseCenterX + 3, noiseBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else {
        Paint_DrawString_EN(uTextX, urbanTextY, noiseLabel, &Font12, WHITE, BLACK);
    }
    uTextX += 6 * Font12.Width;
    // Temperature
    const char* urbanTempLabel = "Temperature";
    bool        urbanTempActive = (current_graph_value == GraphValue::URBAN_TEMP);
    if (urbanTempActive) {
        Paint_DrawString_EN(uTextX,     urbanTextY, urbanTempLabel, &Font12, WHITE, BLACK);
        Paint_DrawString_EN(uTextX + 1, urbanTextY, urbanTempLabel, &Font12, WHITE, BLACK);
        uint16_t urbanTempWidth   = strlen(urbanTempLabel) * Font12.Width;
        uint16_t urbanTempCenterX = uTextX + urbanTempWidth / 2;
        uint16_t urbanTempArrowTopY  = urbanTextY + Font12.Height + 1;
        if (urbanTempArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            urbanTempArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t urbanTempTipY   = urbanTempArrowTopY;
        uint16_t urbanTempBaseY  = urbanTempArrowTopY + 4;
        Paint_DrawLine(urbanTempCenterX,     urbanTempTipY,  urbanTempCenterX - 3, urbanTempBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(urbanTempCenterX,     urbanTempTipY,  urbanTempCenterX + 3, urbanTempBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(urbanTempCenterX - 3, urbanTempBaseY, urbanTempCenterX + 3, urbanTempBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else {
        Paint_DrawString_EN(uTextX, urbanTextY, urbanTempLabel, &Font12, WHITE, BLACK);
    }

    // Second row
    uint16_t urbanTextY2 = urbanTextY + Font12.Height + 8;
    // Humidity
    const char* urbanHumLabel = "Humidity";
    uint16_t    urbanHumX     = urbanX + 4;
    bool        urbanHumActive = (current_graph_value == GraphValue::URBAN_HUM);
    if (urbanHumActive) {
        Paint_DrawString_EN(urbanHumX,     urbanTextY2, urbanHumLabel, &Font12, WHITE, BLACK);
        Paint_DrawString_EN(urbanHumX + 1, urbanTextY2, urbanHumLabel, &Font12, WHITE, BLACK);
        uint16_t urbanHumWidth   = strlen(urbanHumLabel) * Font12.Width;
        uint16_t urbanHumCenterX = urbanHumX + urbanHumWidth / 2;
        uint16_t urbanHumArrowTopY  = urbanTextY2 + Font12.Height + 1;
        if (urbanHumArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            urbanHumArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t urbanHumTipY   = urbanHumArrowTopY;
        uint16_t urbanHumBaseY  = urbanHumArrowTopY + 4;
        Paint_DrawLine(urbanHumCenterX,     urbanHumTipY,  urbanHumCenterX - 3, urbanHumBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(urbanHumCenterX,     urbanHumTipY,  urbanHumCenterX + 3, urbanHumBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(urbanHumCenterX - 3, urbanHumBaseY, urbanHumCenterX + 3, urbanHumBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else {
        Paint_DrawString_EN(urbanHumX, urbanTextY2, urbanHumLabel, &Font12, WHITE, BLACK);
    }

    // Pressure
    const char* urbanPressLabel = "Pressure";
    uint16_t    urbanPressX     = urbanX + 4 + 9 * Font12.Width;
    bool        urbanPressActive = (current_graph_value == GraphValue::URBAN_PRESSURE);
    if (urbanPressActive) {
        Paint_DrawString_EN(urbanPressX,     urbanTextY2, urbanPressLabel, &Font12, WHITE, BLACK);
        Paint_DrawString_EN(urbanPressX + 1, urbanTextY2, urbanPressLabel, &Font12, WHITE, BLACK);
        uint16_t urbanPressWidth   = strlen(urbanPressLabel) * Font12.Width;
        uint16_t urbanPressCenterX = urbanPressX + urbanPressWidth / 2;
        uint16_t urbanPressArrowTopY  = urbanTextY2 + Font12.Height + 1;
        if (urbanPressArrowTopY + 4 > DISPLAY_HEIGHT - 1) {
            urbanPressArrowTopY = DISPLAY_HEIGHT - 1 - 4;
        }
        uint16_t urbanPressTipY   = urbanPressArrowTopY;
        uint16_t urbanPressBaseY  = urbanPressArrowTopY + 4;
        Paint_DrawLine(urbanPressCenterX,     urbanPressTipY,  urbanPressCenterX - 3, urbanPressBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(urbanPressCenterX,     urbanPressTipY,  urbanPressCenterX + 3, urbanPressBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(urbanPressCenterX - 3, urbanPressBaseY, urbanPressCenterX + 3, urbanPressBaseY, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    } else {
        Paint_DrawString_EN(urbanPressX, urbanTextY2, urbanPressLabel, &Font12, WHITE, BLACK);
    }
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

void setNextGraphValue() {
    switch (current_graph_value) {
        case GraphValue::INSIGHT_TEMP:
            current_graph_value = GraphValue::INSIGHT_HUM;
            break;
        case GraphValue::INSIGHT_HUM:
            current_graph_value = GraphValue::INSIGHT_CO2;
            break;
        case GraphValue::INSIGHT_CO2:
            current_graph_value = GraphValue::INSIGHT_PRESSURE;
            break;
        case GraphValue::INSIGHT_PRESSURE:
            current_graph_value = GraphValue::URBAN_AIR;
            break;
        case GraphValue::URBAN_AIR:
            current_graph_value = GraphValue::URBAN_NOISE;
            break;
        case GraphValue::URBAN_NOISE:
            current_graph_value = GraphValue::URBAN_TEMP;
            break;
        case GraphValue::URBAN_TEMP:
            current_graph_value = GraphValue::URBAN_HUM;
            break;
        case GraphValue::URBAN_HUM:
            current_graph_value = GraphValue::URBAN_PRESSURE;
            break;
        case GraphValue::URBAN_PRESSURE:
            current_graph_value = GraphValue::INSIGHT_TEMP;
            break;
    }
}

void setPrevGraphValue() {
    switch (current_graph_value) {
        case GraphValue::INSIGHT_TEMP:
            current_graph_value = GraphValue::URBAN_PRESSURE;
            break;
        case GraphValue::INSIGHT_HUM:
            current_graph_value = GraphValue::INSIGHT_TEMP;
            break;
        case GraphValue::INSIGHT_CO2:
            current_graph_value = GraphValue::INSIGHT_HUM;
            break;
        case GraphValue::INSIGHT_PRESSURE:
            current_graph_value = GraphValue::INSIGHT_CO2;
            break;
        case GraphValue::URBAN_AIR:
            current_graph_value = GraphValue::INSIGHT_PRESSURE;
            break;
        case GraphValue::URBAN_NOISE:
            current_graph_value = GraphValue::URBAN_AIR;
            break;
        case GraphValue::URBAN_TEMP:
            current_graph_value = GraphValue::URBAN_NOISE;
            break;
        case GraphValue::URBAN_HUM:
            current_graph_value = GraphValue::URBAN_TEMP;
            break;
        case GraphValue::URBAN_PRESSURE:
            current_graph_value = GraphValue::URBAN_HUM;
            break;
    }
}

#endif

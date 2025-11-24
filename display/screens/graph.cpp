#ifdef ALTRUIST_INSIDE

#include "../paint_driver/graphPainter.h"
#include "graph.h"
#include "../utils.h"
#include "../../utils.h"
#include "../../sd_card/sd_card.h"
#include "../../config_manager/config_helpers.h"

#if defined(USE_SD_CARD)
#include "SD.h"
#include "FS.h"
#endif

uint8_t current_graph_screen = 1;

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
#endif

void drawGraphScreen() {
    // Clear screen first to prevent glitching
    Paint_Clear(WHITE);
    
    String screenMsg = "Set graph screen " + String(current_graph_screen);
    debug_outln_info(screenMsg);
#if defined(USE_SD_CARD)
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
    if (current_graph_screen == 1) {
        drawOneGraph(10, 10 + GRAPH_HEIGHT, urban_key.c_str(), "SDS_P1", "PM10");
        drawOneGraph(DISPLAY_WIDTH / 2 + 10, 10 + GRAPH_HEIGHT, urban_key.c_str(), "SDS_P2", "PM2.5");
        drawOneGraph(10, DISPLAY_HEIGHT - 10, urban_key.c_str(), "PCBA_noiseMax", "Max Noise");
        drawOneGraph(DISPLAY_WIDTH / 2 + 10, DISPLAY_HEIGHT - 10, urban_key.c_str(), "PCBA_noiseAvg", "Avg Noise");
    } else if (current_graph_screen == 2) {
        drawOneGraph(10, 10 + GRAPH_HEIGHT, urban_key.c_str(), "BME280_temperature", "Urban Temp");
        drawOneGraph(DISPLAY_WIDTH / 2 - GRAPH_HEIGHT / 2, DISPLAY_HEIGHT - 10, urban_key.c_str(), "BME280_humidity", "Urban Hum");
        drawOneGraph(DISPLAY_WIDTH / 2 + 10, 10 + GRAPH_HEIGHT, urban_key.c_str(), "BME280_pressure", "Urban Press");
    } else if (current_graph_screen == 3) {
        drawOneGraph(10, 10 + GRAPH_HEIGHT, "BME680", "temperature", "Insight Temp");
        drawOneGraph(DISPLAY_WIDTH / 2 + 10, 10 + GRAPH_HEIGHT, "BME680", "pressure", "Insight Press");
        drawOneGraph(10, DISPLAY_HEIGHT - 10, "BME680", "humidity", "Insight Hum");
        drawOneGraph(DISPLAY_WIDTH / 2 + 10, DISPLAY_HEIGHT - 10, "SCD4x", "co2", "Insight CO2");
    }
#else
    // Center text vertically with proper spacing (3 lines total)
    uint16_t line_spacing = 4;
    uint16_t total_height = Font16.Height + Font12.Height * 2 + line_spacing * 2;
    uint16_t start_y = (DISPLAY_HEIGHT - total_height) / 2;
    
    uint16_t x1 = DISPLAY_WIDTH / 2 - strlen("No history available") * Font16.Width / 2;
    Paint_DrawString_EN(x1, start_y, "No history available", &Font16, WHITE, BLACK);
    
    uint16_t y2 = start_y + Font16.Height + line_spacing;
    uint16_t x2 = DISPLAY_WIDTH / 2 - strlen("SD card support") * Font12.Width / 2;
    Paint_DrawString_EN(x2, y2, "SD card support", &Font12, WHITE, BLACK);
    
    uint16_t y3 = y2 + Font12.Height + line_spacing;
    uint16_t x3 = DISPLAY_WIDTH / 2 - strlen("not enabled") * Font12.Width / 2;
    Paint_DrawString_EN(x3, y3, "not enabled", &Font12, WHITE, BLACK);
#endif
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

#endif
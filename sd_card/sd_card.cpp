#if defined(USE_SD_CARD)

#include "sd_card.h"
#include "../defines.h"
#include "../utils.h"
#include <algorithm>

namespace {
SemaphoreHandle_t g_sd_mutex = nullptr;
volatile uint32_t g_sd_csv_write_ok = 0;
volatile uint32_t g_sd_csv_write_fail = 0;
volatile uint32_t g_sd_lock_busy = 0;

static void ensureSDMutex() {
    if (g_sd_mutex == nullptr) {
        g_sd_mutex = xSemaphoreCreateRecursiveMutex();
    }
}

class SDLockGuard {
public:
    explicit SDLockGuard(uint32_t timeout_ms = 5000) : locked(false) {
        ensureSDMutex();
        if (g_sd_mutex) {
            locked = (xSemaphoreTakeRecursive(g_sd_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE);
        }
    }
    ~SDLockGuard() {
        if (locked && g_sd_mutex) {
            xSemaphoreGiveRecursive(g_sd_mutex);
        }
    }
    bool ok() const { return locked; }
private:
    bool locked;
};
} // namespace

bool sdCardLock(uint32_t timeout_ms) {
    ensureSDMutex();
    if (!g_sd_mutex) return false;
    return xSemaphoreTakeRecursive(g_sd_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void sdCardUnlock() {
    if (g_sd_mutex) {
        xSemaphoreGiveRecursive(g_sd_mutex);
    }
}

void sdGetDevCounters(uint32_t &csv_ok, uint32_t &csv_fail, uint32_t &lock_busy) {
    csv_ok = g_sd_csv_write_ok;
    csv_fail = g_sd_csv_write_fail;
    lock_busy = g_sd_lock_busy;
}

bool SDCard::begin() {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in begin()"));
        g_sd_lock_busy++;
        return false;
    }
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS_PIN);
    if (_beginSD(SPI)) {
        refreshCache();
        return true;
    } else {
        return false;
    }
}

bool SDCard::_beginSD(SPIClass &spi) {
    if (!SD.begin(SPI_CS_PIN, spi, 40000000)) {
        debug_outln_info(F("Card Mount Failed"));
        return false;
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE) {
        debug_outln_info(F("No SD card attached"));
        return false;
    }

    cardSizeMB = (float)SD.cardSize() / (1024 * 1024);
    usedMemMB = (float)SD.usedBytes() / (1024 * 1024);
    debug_outln_verbose(F("SD Card Size (MB): "), String(cardSizeMB));
    debug_outln_verbose(F("Used space (MB): "), String(usedMemMB));
    return true;
}

bool SDCard::writeTextFile(const String& fullPath, const String& content) {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in writeTextFile()"));
        g_sd_lock_busy++;
        return false;
    }
    int slash = fullPath.lastIndexOf('/');
    if (slash > 0) {
        String folder = fullPath.substring(0, slash);
        if (!SD.exists(folder)) {
            if (!SD.mkdir(folder)) {
                debug_outln_info(F("[SDCardLogger] Failed to create folder for text file: "), folder);
                return false;
            }
        }
    }

    File file = SD.open(fullPath, FILE_WRITE);
    if (!file) {
        debug_outln_info(F("[SDCardLogger] Failed to open text file for write: "), fullPath);
        return false;
    }
    size_t written = file.print(content);
    file.close();
    if (written != content.length()) {
        debug_outln_info(F("[SDCardLogger] Short write when writing text file: "), fullPath);
        return false;
    }
    return true;
}

bool SDCard::appendTextFile(const String& fullPath, const String& content) {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in appendTextFile()"));
        g_sd_lock_busy++;
        return false;
    }
    int slash = fullPath.lastIndexOf('/');
    if (slash > 0) {
        String folder = fullPath.substring(0, slash);
        if (!SD.exists(folder)) {
            if (!SD.mkdir(folder)) {
                debug_outln_info(F("[SDCardLogger] Failed to create folder for append: "), folder);
                return false;
            }
        }
    }

    File file = SD.open(fullPath, FILE_APPEND);
    if (!file) {
        debug_outln_info(F("[SDCardLogger] Failed to open text file for append: "), fullPath);
        return false;
    }
    size_t written = file.print(content);
    file.close();
    if (written != content.length()) {
        debug_outln_info(F("[SDCardLogger] Short write when appending text file: "), fullPath);
        return false;
    }
    return true;
}

void SDCard::refreshCache() {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in refreshCache()"));
        g_sd_lock_busy++;
        return;
    }
    _sensorList.clear();
    _sensorLastFiles.clear();

    if (!SD.exists(ROOT_FOLDER)) {
        if (SD.mkdir(ROOT_FOLDER)) {
            debug_outln_verbose(F("[SDCardLogger] Root folder created"));
        } else {
            debug_outln_info("[SDCardLogger] mkdir failed");
        }
    }

    File root = SD.open(ROOT_FOLDER);
    if (!root || !root.isDirectory()) {
        debug_outln_info(F("[SDCardLogger] Root dir error"));
        return;
    }

    File entry = root.openNextFile();
    while (entry) {
        if (entry.isDirectory()) {
            String sensorName = entry.name();
            int slash = sensorName.lastIndexOf('/');
            if (slash >= 0) sensorName = sensorName.substring(slash + 1);
            _sensorList.push_back(sensorName);

            String path = ROOT_FOLDER + sensorName;
            String lastFile = _findLastFileInFolder(path);
            _sensorLastFiles[sensorName] = lastFile;
        }
        entry = root.openNextFile();
    }
    root.close();
}

std::vector<String> SDCard::getSensorList() {
    return _sensorList;
}

String SDCard::getLastFileForSensor(const String& sensorName) {
    if (_sensorLastFiles.count(sensorName)) {
        return _sensorLastFiles[sensorName];
    }
    return "";
}

String SDCard::_getCurrentDateFileName() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        debug_outln_info(F("[SDCardLogger] Failed to get time"));
        return "";
    }

    char filename[20];
    strftime(filename, sizeof(filename), "%Y-%m-%d.csv", &timeinfo);
    return String(filename);
}

String SDCard::_findLastFileInFolder(const String& path) {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in _findLastFileInFolder()"));
        g_sd_lock_busy++;
        return "";
    }
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) return "";

    String maxFile = "";
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String name = file.name();
            int slash = name.lastIndexOf('/');
            if (slash >= 0) name = name.substring(slash + 1);

            if (name > maxFile) {
                maxFile = name;
            }
        }
        file = dir.openNextFile();
    }
    dir.close();

    return maxFile;
}


void SDCard::_logCSVRow(const String& sensorName, const String& header, const String& values) {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in _logCSVRow()"));
        g_sd_lock_busy++;
        g_sd_csv_write_fail++;
        return;
    }
    String folder = ROOT_FOLDER + sensorName;
    if (!SD.exists(folder)) {
        if (SD.mkdir(folder)) {
            _sensorList.push_back(sensorName);
            debug_outln_verbose(F("[SDCardLogger] folder created: "), folder);
        } else {
            debug_outln_info(F("[SDCardLogger] folder not created: "), folder);
            return;
        }
    }

    String filename = _getCurrentDateFileName();
    String fullPath = folder + "/" + filename;

    bool fileExists = SD.exists(fullPath);
    File file = SD.open(fullPath, FILE_APPEND);
    if (!file) {
        debug_outln_info(F("[SDCardLogger] Failed to open file: "), fullPath);
        g_sd_csv_write_fail++;
        return;
    }

    debug_outln_verbose(F("[SDCardLogger] Start writing data to file "), fullPath);

    _sensorLastFiles[sensorName] = filename;

    // Если файл новый — пишем заголовок
    if (!fileExists || file.size() == 0) {
        if (file.println(header)) {
            debug_outln_verbose(F("[SDCardLogger] Header writed: "), header);
        } else {
            debug_outln_info(F("Can't write header "));
        }
    }

    if (file.println(values)) {
        debug_outln_verbose(F("[SDCardLogger] Logged to: "), fullPath);
        debug_outln_verbose(F("[SDCardLogger] Data: "), values);
        g_sd_csv_write_ok++;
    } else {
        debug_outln_info(F("[SDCardLogger] Can't write data: "));
        incrementSDWriteError();
        g_sd_csv_write_fail++;
    }
    file.close();
}


void readSensorDataFromCSV(LineData &result, const char* sensor_name, const char* field_name, int hours_back) {
    SDLockGuard lock;
    if (!lock.ok()) {
        g_sd_lock_busy++;
        result.count = 0;
        result.values = nullptr;
        result.timestamps = nullptr;
        return;
    }

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    // Временная память для хранения считанных значений (общая для обоих дней)
    std::vector<float> values_vec;
    std::vector<uint32_t> timestamps_vec;
    
    // Граница по времени
    uint32_t time_limit = now - (hours_back * 3600);
    
    int field_index = -1;
    bool field_index_found = false;
    
    // Helper function to read from a specific date file
    auto readFromDateFile = [&](int year, int month, int day) -> bool {
        char filename[64];
        snprintf(filename, sizeof(filename), "/sensors_data/%s/%04d-%02d-%02d.csv",
                 sensor_name, year, month, day);

        File file = SD.open(filename, FILE_READ);
        if (!file) {
            return false; // File doesn't exist, that's OK
        }
        
        // Читаем заголовок (только если еще не нашли field_index)
        if (!field_index_found) {
            String header_line = file.readStringUntil('\n');
            std::vector<String> headers;
            int start = 0;
            while (start < header_line.length()) {
                int comma = header_line.indexOf(',', start);
                if (comma == -1) comma = header_line.length();
                headers.push_back(header_line.substring(start, comma));
                start = comma + 1;
            }
            
            for (size_t i = 0; i < headers.size(); ++i) {
                String header = headers[i];
                header.trim();
                if (header == field_name) {
                    field_index = i;
                    field_index_found = true;
                    break;
                }
            }
            
            if (!field_index_found) {
                file.close();
                return false;
            }
        } else {
            // Skip header line if we already found field_index
            file.readStringUntil('\n');
        }

        // Чтение данных построчно
        while (file.available()) {
            String line = file.readStringUntil('\n');
            std::vector<String> parts;
            int pos = 0;
            while (pos < line.length()) {
                int comma = line.indexOf(',', pos);
                if (comma == -1) comma = line.length();
                parts.push_back(line.substring(pos, comma));
                pos = comma + 1;
            }

            if (parts.size() <= field_index) continue;

            uint32_t timestamp = parts[0].toInt();
            if (timestamp < time_limit) continue;
            
            float value;
            if (strstr(field_name, "pressure") != nullptr) {
                value = parts[field_index].toFloat() * 0.0075;
            } else {
                value = parts[field_index].toFloat();
            }

            timestamps_vec.push_back(timestamp);
            values_vec.push_back(value);
        }

        file.close();
        return true;
    };
    
    // First, try today's file
    bool today_read = readFromDateFile(timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
    
    // Calculate yesterday's date
    struct tm yesterday_tm = *timeinfo;
    yesterday_tm.tm_mday--;
    mktime(&yesterday_tm); // Normalize the date
    
    // Try yesterday's file if:
    // 1. Today's file wasn't found, OR
    // 2. We have data but it's less than 1 hour span, OR
    // 3. We need more than 24 hours of data
    bool need_yesterday = !today_read;
    if (!need_yesterday && !timestamps_vec.empty()) {
        uint32_t data_span = timestamps_vec.back() - timestamps_vec.front();
        need_yesterday = (data_span < 3600); // Less than 1 hour of data
    }
    if (hours_back > 24) {
        need_yesterday = true; // Always read yesterday if we need more than 24 hours
    }
    
    if (need_yesterday) {
        debug_outln_verbose(F("[SDCard] Reading from yesterday's file to get more data"));
        readFromDateFile(yesterday_tm.tm_year + 1900, yesterday_tm.tm_mon + 1, yesterday_tm.tm_mday);
    }

    // If still no data, return early
    if (values_vec.empty()) {
        result.count = 0;
        result.values = nullptr;
        result.timestamps = nullptr;
        return;
    }
    
    // Sort by timestamp (in case we read from two files)
    std::vector<std::pair<uint32_t, float>> combined;
    for (size_t i = 0; i < timestamps_vec.size(); i++) {
        combined.push_back({timestamps_vec[i], values_vec[i]});
    }
    std::sort(combined.begin(), combined.end());
    
    // Переносим в C-массивы
    result.count = combined.size();
    result.values = new float[result.count];
    result.timestamps = new uint32_t[result.count];

    for (int i = 0; i < result.count; ++i) {
        result.timestamps[i] = combined[i].first;
        result.values[i] = combined[i].second;
    }
    
    debug_outln_verbose(String(F("[SDCard] Read total of ")) + String(result.count) + F(" data points from CSV"));
}

bool SDCard::checkInserted() {
    SDLockGuard lock(250);
    if (!lock.ok()) {
        g_sd_lock_busy++;
        // SD can be busy in another task (graph read/log rotation/retention).
        // Don't treat temporary lock contention as card removal.
        static unsigned long last_busy_log_ms = 0;
        if (msSince(last_busy_log_ms) > 30000UL) {
            last_busy_log_ms = millis();
            debug_outln_verbose(F("[SDCardLogger] SD mutex busy in checkInserted(); will retry later"));
        }
        return true;
    }
    static sdcard_type_t last_type = CARD_NONE;

    sdcard_type_t card_type = SD.cardType();
    // Only log when the type changes, or when we hit an error below.
    if (card_type != last_type) {
        last_type = card_type;
        debug_outln_verbose(F("[SDCardLogger] Card type: "), _getCardTypeName(card_type));
    }
    
    if (card_type == CARD_NONE) {
        debug_outln_info("[SDCardLogger] No SD card present, retry begin...");
        return _beginSD(SPI);
    }
    
    // Try to actually access the card to verify it's really there
    // cardType() might return cached value, so we need to test actual access
    if (!SD.exists("/")) {
        debug_outln_info(F("[SDCardLogger] Card type OK but filesystem not accessible - card may be removed"));
        // Try to reinitialize
        SD.end();
        delay(50);
        return _beginSD(SPI);
    }
    
    // Try to open root directory to verify card is actually accessible
    File root = SD.open("/");
    if (!root) {
        debug_outln_info(F("[SDCardLogger] Cannot open root directory - card may be removed"));
        SD.end();
        delay(50);
        return _beginSD(SPI);
    }
    root.close();
    
    return true;
}

bool SDCard::applyRetentionPolicy(uint16_t sensorRetentionDays, uint16_t maxBootFiles) {
    SDLockGuard lock;
    if (!lock.ok()) {
        debug_outln_info(F("[SDCardLogger] Failed to acquire SD mutex in applyRetentionPolicy()"));
        g_sd_lock_busy++;
        return false;
    }
    if (!checkInserted()) {
        return false;
    }

    bool changed = false;
    auto toAbsolutePath = [](const String& parentDir, const String& pathOrName) -> String {
        String p = pathOrName;
        p.trim();
        if (p.length() == 0) return "";
        if (!p.startsWith("/")) {
            String base = parentDir;
            if (!base.endsWith("/")) base += "/";
            p = base + p;
        }
        return p;
    };

    // 1) sensors_data retention: remove CSV files older than cutoff date.
    // File names are expected as YYYY-MM-DD.csv so lexicographical comparison works.
    if (sensorRetentionDays > 0) {
        time_t now = time(nullptr);
        if (now > 0) {
            time_t cutoff = now - (time_t)sensorRetentionDays * 24 * 60 * 60;
            struct tm cutoff_tm;
            localtime_r(&cutoff, &cutoff_tm);
            char cutoff_name[16];
            strftime(cutoff_name, sizeof(cutoff_name), "%Y-%m-%d", &cutoff_tm);
            String cutoff_date(cutoff_name);

            File root = SD.open(ROOT_FOLDER);
            if (root && root.isDirectory()) {
                File sensorDir = root.openNextFile();
                while (sensorDir) {
                    if (sensorDir.isDirectory()) {
                        String sensorDirPath = sensorDir.name();
                        if (!sensorDirPath.startsWith("/")) {
                            sensorDirPath = String(ROOT_FOLDER) + sensorDirPath;
                        }
                        File dayFile = sensorDir.openNextFile();
                        while (dayFile) {
                            if (!dayFile.isDirectory()) {
                                String filePath = toAbsolutePath(sensorDirPath, dayFile.name());
                                int slash = filePath.lastIndexOf('/');
                                String name = (slash >= 0) ? filePath.substring(slash + 1) : filePath;
                                if (name.length() == 14 && name.endsWith(".csv")) {
                                    String datePart = name.substring(0, 10); // YYYY-MM-DD
                                    if (datePart < cutoff_date) {
                                        if (filePath.length() > 1 && filePath.startsWith("/")) {
                                            if (SD.remove(filePath)) {
                                                changed = true;
                                                debug_outln_verbose(F("[SDCardLogger] Retention removed old sensor CSV: "), filePath);
                                            }
                                        } else {
                                            debug_outln_info(F("[SDCardLogger] Retention skip invalid sensor CSV path: "), filePath);
                                        }
                                    }
                                }
                            }
                            dayFile.close();
                            dayFile = sensorDir.openNextFile();
                        }
                    }
                    sensorDir.close();
                    sensorDir = root.openNextFile();
                }
                root.close();
            }
        }
    }

    // 2) exceptions retention: keep only the newest N boot_*.txt files.
    if (maxBootFiles > 0 && SD.exists(EXCEPTIONS_FOLDER)) {
        std::vector<std::pair<uint32_t, String>> boots;
        File ex = SD.open(EXCEPTIONS_FOLDER);
        if (ex && ex.isDirectory()) {
            File f = ex.openNextFile();
            while (f) {
                if (!f.isDirectory()) {
                    String fullPath = toAbsolutePath(String(EXCEPTIONS_FOLDER), f.name());
                    int slash = fullPath.lastIndexOf('/');
                    String name = (slash >= 0) ? fullPath.substring(slash + 1) : fullPath;
                    if (name.startsWith("boot_") && name.endsWith(".txt")) {
                        int us = name.indexOf('_');
                        int dot = name.lastIndexOf('.');
                        String n = name.substring(us + 1, dot);
                        uint32_t id = (uint32_t)n.toInt();
                        boots.push_back({id, fullPath});
                    }
                }
                f.close();
                f = ex.openNextFile();
            }
            ex.close();
        }

        if (boots.size() > maxBootFiles) {
            std::sort(boots.begin(), boots.end(),
                      [](const std::pair<uint32_t, String>& a, const std::pair<uint32_t, String>& b) {
                          return a.first < b.first;
                      });
            size_t removeCount = boots.size() - maxBootFiles;
            for (size_t i = 0; i < removeCount; i++) {
                const String& path = boots[i].second;
                if (path.length() > 1 && path.startsWith("/")) {
                    if (SD.remove(path)) {
                        changed = true;
                        debug_outln_verbose(F("[SDCardLogger] Retention removed old boot log: "), path);
                    }
                } else {
                    debug_outln_info(F("[SDCardLogger] Retention skip invalid boot log path: "), path);
                }
            }
        }
    }

    if (changed) {
        refreshCache();
    }
    return true;
}

String SDCard::_getCardTypeName(sdcard_type_t type) {
    if (type == CARD_MMC) {
        return String("MMC");
    } else if (type == CARD_SD) {
        return String("SD");
    } else if (type == CARD_SDHC) {
        return String("SDHC");
    } else if (type == CARD_UNKNOWN) {
        return String("UNKNOWN");
    } else {
        return String("NONE");
    }
}

#endif
#include "pages.h"
#include "../../intl.h"
#include "../html-content.h"

void webserver_root(String &page_content, const String &robonomics_address) {

    // Enable Pagination
    page_content += FPSTR(WEB_ROOT_PAGE_CONTENT);
    page_content.replace(F("{t}"), FPSTR(INTL_CURRENT_DATA));
    page_content.replace(F("{s}"), FPSTR(INTL_DEVICE_STATUS));
    page_content.replace(F("{conf}"), FPSTR(INTL_CONFIGURATION));
    page_content.replace(F("{group}"), FPSTR(INTL_GROUP_MENU));
    page_content.replace(F("{restart}"), FPSTR(INTL_RESTART_SENSOR));
    page_content.replace(F("{debug}"), FPSTR(INTL_DEBUG_LEVEL));

    String map_url = F("https://sensors.social/");
    if (robonomics_address.length() > 0 && robonomics_address != F("Not Set")) {
        map_url += F("?sensor=");
        map_url += robonomics_address;
    }
    page_content.replace(F("https://sensors.social/"), map_url);
}
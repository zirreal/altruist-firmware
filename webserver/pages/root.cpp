#include "pages.h"
#include "../../intl.h"
#include "../html-content.h"

void webserver_root(String &page_content) {

    // Enable Pagination
    page_content += FPSTR(WEB_ROOT_PAGE_CONTENT);
    page_content.replace(F("{t}"), FPSTR(INTL_CURRENT_DATA));
    page_content.replace(F("{s}"), FPSTR(INTL_DEVICE_STATUS));
    page_content.replace(F("{conf}"), FPSTR(INTL_CONFIGURATION));
    page_content.replace(F("{restart}"), FPSTR(INTL_RESTART_SENSOR));
    page_content.replace(F("{debug}"), FPSTR(INTL_DEBUG_LEVEL));
}
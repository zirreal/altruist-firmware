#include "pages.h"
#include "../../utils.h"
#include "../../intl.h"
#include "../html-content.h"
#include "../utils.h"
#include "../../config_manager/config_helpers.h"

/*****************************************************************
 * Webserver config: show guest page                            *
 *****************************************************************/

void webserver_guest_create_body_get_part1(String& page_content, bool wificonfig_loop) {

	debug_outln_info(F("begin webserver_config_body_get ..."));
	page_content += F("<form method='POST' action='/guest' style='width:100%;'>\n"
	"<input class='radio' id='r1' name='group' type='radio' checked>"
    "<input class='radio' id='r2' name='group' type='radio'>"
    "<input class='radio' id='r3' name='group' type='radio'>"
    "<input class='radio' id='r4' name='group' type='radio'>");

	if (wificonfig_loop) {  // scan for wlan ssids
		page_content += F("<div id='wifilist'>" INTL_WIFI_NETWORKS "</div><br/>");
	}
	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_wlanssid, FPSTR(INTL_FS_WIFI_NAME), LEN_WLANSSID-1);
	add_form_input(page_content, Config_wlanpwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD-1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
	page_content += form_checkbox(Config_wlannopwd, FPSTR(INTL_NO_WLAN_PWD), false);
	page_content += F("<hr/>\n<br/><b>");

	page_content += F("Robonomics Settings");
	page_content += FPSTR(WEB_B_BR);
	page_content += FPSTR(BR_TAG);

	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_rws_owner, FPSTR(INTL_RWS_OWNER), LEN_RWS_OWNER-1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);

	page_content += FPSTR(TABLE_TAG_OPEN);
	page_content += form_select_reg();
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);

	page_content += F("<br/><b>");

	page_content += FPSTR(WEB_GPS);
	page_content += FPSTR(WEB_B_BR);
	page_content += FPSTR(BR_TAG);

	page_content += FPSTR(TABLE_TAG_OPEN);
	add_form_input(page_content, Config_coords_gps, FPSTR(INTL_COORDS), LEN_GPS_COORDS-1);
	page_content += FPSTR(TABLE_TAG_CLOSE_BR);
}

void webserver_guest_create_body_get_part2(String& page_content, bool wificonfig_loop) {
	page_content += F("</div></div>");
	page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
	page_content += FPSTR(BR_TAG);
	page_content += FPSTR(WEB_BR_FORM);
	if (wificonfig_loop) {  // scan for wlan ssids
		page_content += F("<script>window.setTimeout(load_wifi_list,1000);</script>");
	}
}
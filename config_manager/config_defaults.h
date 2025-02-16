#ifndef __CONFIG_DEFAULTS_H__
#define __CONFIG_DEFAULTS_H__

#include "../ext_def.h"
#include "../defines.h"

namespace cfg {
	extern unsigned debug;

	extern unsigned time_for_wifi_config;
	extern unsigned sending_intervall_ms;
	extern unsigned datalog_sending_intervall_ms;

	extern char current_lang[3];
	extern char current_reg[20];
	// credentials for basic auth of internal web server
	extern bool www_basicauth_enabled;
	extern char www_username[LEN_WWW_USERNAME];
	extern char www_password[LEN_CFG_PASSWORD];

	// wifi credentials
	extern char wlanssid[LEN_WLANSSID];
	extern char wlanpwd[LEN_CFG_PASSWORD];
	extern bool wlannopwd;

	// credentials of the sensor in access point mode
	extern char fs_ssid[LEN_FS_SSID];
	extern char fs_pwd[LEN_CFG_PASSWORD];

	// (in)active sensors
	extern bool dht_read;
	extern bool htu21d_read;
	extern bool dbmeter_read;
	extern bool i2snoise_read;
	extern bool ppd_read;
	extern bool sds_read;
	extern bool gc_read;
	extern bool ccs811_27_read;
	extern bool ccs811_read;
	extern bool pms_read;
	extern bool hpm_read;
	extern bool npm_read;
	extern bool sps30_read;
	extern bool bmp_read;
	extern bool bmx280_read;
	extern bool sht3x_read;
	extern bool ds18b20_read;
	extern bool dnms_read;
	extern char dnms_correction[LEN_DNMS_CORRECTION];
	extern char rws_owner[LEN_RWS_OWNER];
	extern char robonomics_public_node[LEN_ROBONOMICS_PUBLIC_NODE];
	extern char private_key[LEN_PRIVATE_KEY];
	extern char lat_gps[LEN_GPS_LAT];
	extern char lon_gps[LEN_GPS_LON];
	extern char coords_gps[LEN_GPS_COORDS];
	extern bool gps_read;
	extern char temp_correction[LEN_TEMP_CORRECTION];

	// send to "APIs"
	extern bool send2robonomics;
	extern bool send2custom;
	extern bool send2influx;
	extern bool send2csv;

	extern bool auto_update;
	extern bool use_beta;

	// (in)active displays
	extern bool has_display;
	extern bool has_sh1106;
	extern bool has_flipped_display;

	extern bool display_wifi_info;
	extern bool display_device_info;

	extern char host_influx[LEN_HOST_INFLUX];
	extern char url_influx[LEN_URL_INFLUX];
	extern unsigned port_influx;
	extern char user_influx[LEN_USER_INFLUX];
	extern char pwd_influx[LEN_CFG_PASSWORD];
	extern char measurement_name_influx[LEN_MEASUREMENT_NAME_INFLUX];
	extern bool ssl_influx;

	extern char host_custom[LEN_HOST_CUSTOM];
	extern char url_custom[LEN_URL_CUSTOM];
	extern bool ssl_custom;
	extern unsigned port_custom;
	extern char user_custom[LEN_USER_CUSTOM];
	extern char pwd_custom[LEN_CFG_PASSWORD];
	extern char donated_by[LEN_DONATED_BY];

	extern void initNonTrivials(const char* id);
}

#endif // __CONFIG_DEFAULTS_H__
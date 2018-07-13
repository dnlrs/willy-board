/*
 * WiFi.h
 */

#ifndef _WIFI_HANDLER
#define _WIFI_HANDLER

#include <cstdlib>
#include <cstdio>
#include "sdkconfig.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "time.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>
#include "lwip/sockets.h"
#include <errno.h>
#include <string>

class WiFi {
	std::string         ssid;
	std::string         passphrase;
	wifi_country_t      wifi_country;
	wifi_init_config_t  init_cfg; //configuration for initialization
	wifi_config_t       sec_cfg; //configuration for connection

	WiFi();
public:

	WiFi(const std::string& ssid,const std::string& passphrase);
	~WiFi();


	void setPassphrase(const std::string& passphrase) {
		this->passphrase = passphrase;
	}

	void setSSID(const std::string& ssid) {
		this->ssid = ssid;
	}

	void init();
	void connect();
	void disconnect();

};



#endif /* _WIFI_HANDLER */

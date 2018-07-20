#pragma once


#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include <string>
#include <string.h>
#include <iostream>
#include <mutex>
#include <condition_variable>

using namespace std;

class WiFi {
	mutex m;
	condition_variable connected_flag; //other threads subscribe to this cv 
									   //in order to know when wifi is started successfully

	WiFi();
public:

	WiFi(const string& ssid,const string& passphrase);
	~WiFi();

	void wait_connection(){
		unique_lock<mutex> ul(m);
		connected_flag.wait(ul);
	}

	void signal_connection(){
		unique_lock<mutex> ul(m);
		connected_flag.notify_all();
	}

	void enable();
	void disable();
	static esp_err_t event_handler(void *ctx, system_event_t *event);
};

extern WiFi *wifi_handler; //extern global handler for wifi
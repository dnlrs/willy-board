#pragma once

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include <iostream>
#include <thread>
#include "apps/sntp/sntp_opts.h"
#include "NetWrap.h"
#include "PacketContainer.h"

#define	WIFI_CHANNEL_MAX 				13 		//according to EU standard
#define	WIFI_CHANNEL_SWITCH_INTERVAL	500
#define PROBE_REQ_ID					0x40
#define MAC_LENGTH						6 		//bytes

#define TASK_RESET_PERIOD_S     		2 //seconds
#define STACK_SIZE 						4096
#define TIME_REFRESH_RATE				SNTP_UPDATE_DELAY / 1000 //seconds

#define SERVER_CONNECTION_ATTEMPTS		10

//#define DEFAULT_SERVER_IP 			"192.168.1.4"
//#define DEFAULT_SERVER_IP				"192.168.43.59"
//#define DEFAULT_SERVER_IP				"192.168.1.108"
//#define DEFAULT_SERVER_PORT 			27015

#define BLINK_GPIO 						2

struct server_info_t{
	string ip;
	int port;
};

class Sniffer {
	server_info_t server_addr;
public:
	Sniffer(const string& ip_addr, const int& port){
		server_addr = {};
		server_addr.ip = ip_addr;
		server_addr.port = port;
	}
	~Sniffer();

	void start();

	static void sniffer_task(void *pvParameters);
	static void sender_task(void *pvParameters);

	static void incoming_packet_cb(void* buff, wifi_promiscuous_pkt_type_t type);
};
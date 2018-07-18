#pragma once

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include <iostream>
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

#define DEFAULT_SERVER_IP 				"192.168.1.4"
#define DEFAULT_SERVER_PORT 			27015

#define BLINK_GPIO 						2


class Sniffer {
	TaskHandle_t sniffer_handle; //rtos thread handler
	TaskHandle_t sender_handle;

	static void sniffer_task(void *pvParameters);
	static void sender_task(void *pvParameters);
public:

	Sniffer();
	~Sniffer();

	static void incoming_packet_cb(void* buff, wifi_promiscuous_pkt_type_t type);
};
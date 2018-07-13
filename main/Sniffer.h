/*
 * SNIFFER.h
 */

#ifndef _SNIFFER_HANDLER
#define _SNIFFER_HANDLER

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
#include <iostream>
#include <set>
#include "WiFi.h"
#include "Packet.h"


extern WiFi *wifi_ap;

/* TO BE WRAPPED */
#define SERVER_IP "192.168.43.83"
//#define SERVER_IP "192.168.43.83"
#define SERVER_PORT 27015
#define DEFAULT_SCAN_DURATION 10


class Sniffer {

	Sniffer(){ };
public:

	
	//~Sniffer();

	static void start();
	static void incoming_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type);
	static esp_err_t sys_event_handler(void *ctx, system_event_t *event);
	static void scan();
	static void scan(long scan_duration);
	static void send_packets(int sock);
	static void stop();
	static int syncTimeline(int sock);
	static void print_container();
	static int create_link(const char *server_ip, unsigned server_port);
	static void initPhase(int sock);

};



#endif

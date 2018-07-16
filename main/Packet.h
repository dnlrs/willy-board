/*
 * PACKET.h
 */

#ifndef _PACKET_STRUCT
#define _PACKET_STRUCT

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
#include "Timeline.h"
#include "NetWrap.h"
#include <iostream>

#define MAC_LENGTH 6
#define MAX_SSID_LENGTH 32
#define MD5_HASH_LENGTH 32

using namespace std;

typedef struct {
	unsigned frame_ctrl:16;
	unsigned duration_id:16;
	uint8_t addr1[6]; /* receiver address */
	uint8_t addr2[6]; /* sender address */
	uint8_t addr3[6]; /* filtering address */
	unsigned sequence_ctrl:16;
	//uint8_t addr4[6]; /* optional */
} wifi_frame_hdr_t;

typedef struct {
	uint8_t data[0];
	uint32_t crc;
} wifi_frame_payload_t;

typedef struct {
	wifi_frame_hdr_t hdr;
	uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_frame_t;

class Packet{
	/* FIELDS IN ALPHABETICAL ORDER */
	uint8_t channel; //channel where the packet were sniffed
	std::string hash;
	uint8_t mac_addr[MAC_LENGTH]; /* sender address */
	int rssi;
	uint16_t sequence_ctrl;
	char ssid[MAX_SSID_LENGTH+1];
	uint16_t timestamp;
	
	/* USED TO CALCULATE HASH BUT NOT SAVED HERE */
	//uint32_t internal_timestamp; 
	//uint32_t crc; //deprecated

	std::string genHash(uint32_t internal_timestamp);

public:

	Packet() {};
	Packet(string s): hash(s) { };

	string temp_print(){
		return hash;
	}
	int fetchData(void *buff, Timeline *time_offset);
	void print();
	static bool isProbeReq(void *buff);
	int send(int socket);
	bool operator<(const Packet& p) const {
   		return this->hash < p.hash;
	}

	bool temp_send(NetWrap& net){
		int rssi = htonl(this->rssi);
		int w_check = write(net.get_descriptor(),&rssi,sizeof(int));
		if(w_check <= 0){
			printf("write error on rssi. errno: %s\n",strerror(errno));
			return false;
		}
		return true;
	}

};



#endif
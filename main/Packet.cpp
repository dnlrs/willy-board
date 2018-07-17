/*
 * PACKET cpp
 */

#include <cstdio>
#include <cstdlib>
#include <set>
#include <iostream>
#include <sstream>
#include <string>
#include <errno.h>
#include "Packet.h"
#include "Timeline.h"
#include "md5.h"

#define PROBE_REQ_ID 0x40
#define MAC_LENGTH 6
#define MAX_SSID_LENGTH 32

int
Packet::fetchData(void *buff, Timeline *time_offset){

	const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
	const wifi_pkt_rx_ctrl_t pkt_ctrl = (wifi_pkt_rx_ctrl_t)ppkt->rx_ctrl; //contains CHANNEL,RSSI,TIMESTAMP
	const wifi_frame_t *ipkt = (wifi_frame_t *)ppkt->payload;
	const wifi_frame_hdr_t *hdr = &ipkt->hdr; //contains FRAME_CTRL,MAC,SEQUENCE_CTRL

	if((hdr->frame_ctrl^PROBE_REQ_ID)!=0)
		return -1;

	const wifi_frame_payload_t *network_data = (wifi_frame_payload_t *)ipkt->payload; //contains SSID and CHECKSUM	
	
	sequence_ctrl = static_cast<int>(hdr->sequence_ctrl);
	rssi = static_cast<int>(pkt_ctrl.rssi);
	timestamp = 0;
	channel = static_cast<int>(pkt_ctrl.channel);

	int i,j;
	for(i=0;i<MAC_LENGTH;i++)
		mac_addr[i] = static_cast<char>(hdr->addr2[i]);
	

	ssid_length = static_cast<int>(network_data->data[1]);
	for(i=2,j=0;(i<=(ssid_length+1))&&(j<MAX_SSID_LENGTH);i++,j++)
		ssid[j] = network_data->data[i];
	ssid[j] = '\0';
	ssid_length = strlen(ssid);

	hash = genHash(); 

	return 0;
}

bool
Packet::isProbeReq(void *buff){
	const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
	const wifi_frame_t *ipkt = (wifi_frame_t *)ppkt->payload;
	const wifi_frame_hdr_t *hdr = &ipkt->hdr; //contains FRAME_CTRL,MAC,SEQUENCE_CTRL

	if((hdr->frame_ctrl^PROBE_REQ_ID)!=0)
		return false;

	return true;
}

void
Packet::print(){

	
	printf("CH=%d, RSSI=%02d, MAC=%02x:%02x:%02x:%02x:%02x:%02x, SSID=%s, SEQ=%d, TIMESTAMP: %d, HASH=%s\n",
				channel,
				rssi,
				mac_addr[0],mac_addr[1],mac_addr[2],
				mac_addr[3],mac_addr[4],mac_addr[5],
				ssid,
				sequence_ctrl,
				timestamp,
				hash.c_str()
			);
	
	
}


std::string 
Packet::genHash(){
	std::string hash;

	std::ostringstream concat_data; 

	//concatenate fields in alphabetical order
	concat_data <<(channel)
				<<(mac_addr[0])<<(mac_addr[1])<<(mac_addr[2])<<(mac_addr[3])<<(mac_addr[4])<<(mac_addr[5])
				<<(sequence_ctrl)
				<<(ssid);
				
	hash = md5(concat_data.str());

	return hash;
}

void 
Packet::serialize(char *buf){

	int *buf_int = (int *)buf;
	*buf_int = htonl(channel); //CHANNEL
	buf_int++;
	*buf_int = htonl(rssi); //RSSI
	buf_int++;
	*buf_int = htonl(sequence_ctrl); //SEQUENCE CTRL
	buf_int++;
	*buf_int = htonl(timestamp); //TIMESTAMP
	buf_int++;
	*buf_int = htonl(ssid_length); //SSID LENGTH
	buf_int++;


	char *buf_c = (char *)buf_int;

	for(int i=0; i < MAC_LENGTH; i++){
		*buf_c = mac_addr[i];
		buf_c++;
	}

	for(int i=0; i < MD5_HASH_LENGTH; i++){
		*buf_c = hash.c_str()[i];
		buf_c++;
	}

	for(int i=0; i < ssid_length; i++){
		*buf_c = ssid[i];
		buf_c++;
	}
}

void 
Packet::deserialize(char *buf){
	int channel;
	int rssi;
	int sequence_ctrl;
	int timestamp;
	int ssid_length;
	char mac_addr[MAC_LENGTH];
	char ssid[MAX_SSID_LENGTH + 1];
	char hash[MD5_HASH_LENGTH + 1];

	int *buf_int = (int *)buf;
	channel = ntohl(*buf_int); //CHANNEL
	buf_int++;
	rssi = ntohl(*buf_int); //RSSI
	buf_int++;
	sequence_ctrl = ntohl(*buf_int); //SEQUENCE CTRL
	buf_int++;
	timestamp = ntohl(*buf_int); //TIMESTAMP
	buf_int++;
	ssid_length = ntohl(*buf_int); //SSID LENGTH
	buf_int++;

	char *buf_c = (char *)buf_int;

	for(int i=0; i < MAC_LENGTH; i++){
		mac_addr[i] = *buf_c;
		buf_c++;
	}

	for(int i=0; i < MD5_HASH_LENGTH; i++){
		hash[i] = *buf_c;
		buf_c++;
	}
	hash[MD5_HASH_LENGTH] = '\0';

	for(int i=0; i < ssid_length; i++){
		ssid[i] = *buf_c;
		buf_c++;
	}
	ssid[ssid_length] = '\0';

	printf("CH=%d, RSSI=%02d, MAC=%02x:%02x:%02x:%02x:%02x:%02x, SSID=%s, SEQ=%d, TIMESTAMP: %d, HASH=%s\n",
			channel,
			rssi,
			(uint8_t)mac_addr[0],(uint8_t)mac_addr[1],(uint8_t)mac_addr[2],
			(uint8_t)mac_addr[3],(uint8_t)mac_addr[4],(uint8_t)mac_addr[5],
			ssid,
			sequence_ctrl,
			timestamp,
			hash
	);

}
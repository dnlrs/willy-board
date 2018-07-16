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
	
	this->sequence_ctrl = hdr->sequence_ctrl;
	//this->crc = network_data->crc; //deprecated
	this->rssi = pkt_ctrl.rssi;
	this->timestamp = 0;//(uint16_t)time_offset->now();
	this->channel = (uint8_t)pkt_ctrl.channel;

	int i,j;
	for(i=0;i<MAC_LENGTH;i++)
		this->mac_addr[i] = hdr->addr2[i];

	uint8_t len_ssid = network_data->data[1];
	for(i=2,j=0;(i<=(len_ssid+1))&&(j<MAX_SSID_LENGTH);i++,j++)
		this->ssid[j] = network_data->data[i];
	this->ssid[j] = '\0';

	this->hash = this->genHash(pkt_ctrl.timestamp); 

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
/*
	std::cout	<<"RSSI: "<<this->rssi
				<<"MAC: "<<mac_addr[0]<<":"<<mac_addr[1]<<":"<<mac_addr[2]<<":"<<mac_addr[3]<<":"<<mac_addr[4]<<":"<<mac_addr[5]
				<<"SSID: "<<this->ssid
				<<"SEQ: "<<this->sequence_ctrl
				<<"TIMESTAMP: "<<this->timestamp
				<<"HASH: "<<this->hash
				<<std::endl;
*/
	
	printf("CH=%d, RSSI=%02d, MAC=%02x:%02x:%02x:%02x:%02x:%02x, SSID=%s, SEQ=%d, TIMESTAMP: %d, HASH=%s\n",
				this->channel,
				this->rssi,
				this->mac_addr[0],this->mac_addr[1],this->mac_addr[2],
				this->mac_addr[3],this->mac_addr[4],this->mac_addr[5],
				this->ssid,
				this->sequence_ctrl,
				this->timestamp,
				this->hash.c_str()
			);
	
}

int 
Packet::send(int sock){
	int w_check = -1;


	//converting format
	//this list is also the order in which the fields are sent to the server
	uint8_t channel = this->channel;	
	char mac_segment;
	int rssi = htonl(this->rssi);
	uint16_t sequence_ctrl = htons(this->sequence_ctrl);
	//len of ssid
	//char ssid
	uint16_t timestamp = htons(this->timestamp);

	/* SENDING CHANNEL */ 
	w_check = write(sock,&channel,sizeof(uint8_t));
	if(w_check <= 0){
		printf("write error on channel. errno: %s\n",strerror(errno));
		return w_check;
	}

	/* SENDING HASH STRING */
	
	w_check = write(sock,(this->hash).c_str(),(this->hash).size());
	if(w_check <= 0){
		printf("write error on hash.\n");
		return w_check;
	}
	

	/* SENDING MAC ADDR */ 
	for(int j=0; j < MAC_LENGTH; j++){
		mac_segment = (char)(this->mac_addr[j]);

		w_check = write(sock,&mac_segment,sizeof(char));
		if(w_check <= 0){
			printf("write error on mac segment. errno: %s\n",strerror(errno));
			return w_check;
		}
	}


	/* SENDING RSSI */ 
	w_check = write(sock,&rssi,sizeof(int));
	if(w_check <= 0){
		printf("write error on rssi. errno: %s\n",strerror(errno));
		return w_check;
	}

	/* SENDING SEQ NUMBER */ 
	w_check = write(sock,&sequence_ctrl,sizeof(uint32_t));
	if(w_check <= 0){
		printf("write error on crc. errno: %s\n",strerror(errno));
		return w_check;
	}

	/* SENDING SSID */	
	
	int len_ssid = strlen(this->ssid);
	std::cout<<"LEN_SSID: "<<len_ssid<<std::endl;
	uint8_t len_ssid_n = (uint8_t)len_ssid;//htons(len_ssid);
	w_check = write(sock,&len_ssid_n,sizeof(uint8_t));
	if(w_check <= 0){
		printf("write error on len ssid.\n");
		return w_check;
	}

	if(len_ssid > 0){
		w_check = write(sock,this->ssid,len_ssid);
		if(w_check <= 0){
			printf("write error on ssid.\n");
			return w_check;
		}
	}
	

	/* SENDING TIMESTAMP */ 
	w_check = write(sock,&timestamp,sizeof(uint16_t));
	if(w_check <= 0){
		printf("write error on timestamp. errno: %s\n",strerror(errno));
		return w_check;
	}

	return 0;
}

std::string 
Packet::genHash(uint32_t internal_timestamp){
	std::string hash;

	std::ostringstream concat_data; 

	//concatenate fields in alphabetical order
	concat_data <<(this->channel)
				//<<(internal_timestamp)
				<<(this->mac_addr[0])<<(this->mac_addr[1])<<(this->mac_addr[2])<<(this->mac_addr[3])<<(this->mac_addr[4])<<(this->mac_addr[5])
				<<(this->sequence_ctrl)
				<<(this->ssid);
				
	hash = md5(concat_data.str());

	return hash;
}
/*
 * SNIFFER handler
 */
#include <iostream>
#include <set>
#include "Sniffer.h"
#include "WiFi.h"
#include "Packet.h"
#include "lwip/sockets.h"
#include "Timeline.h"

#define	WIFI_CHANNEL_MAX 				13 //according to EU standard
#define	WIFI_CHANNEL_SWITCH_INTERVAL	500
#define PROBE_REQ_ID					0x40
#define PACKETS_BUFFER_LIMIT 			256 //# of packets
#define MAC_LENGTH						6 //bytes

std::multiset<Packet> *container;
Timeline *time_offset;
bool skip_send;

void
Sniffer::start(){

	container = new std::multiset<Packet>();
	time_offset = new Timeline();
	skip_send = true;
}

/*
 *	CHAIN:
*						wifi.on
*		sniffing phase ---------> send phase ----> syncro phase 
*				^					 				  |
*				|                   				  |
*				---------------------------------------
*						wifi.off
 */
esp_err_t
Sniffer::sys_event_handler(void *ctx, system_event_t *event)
{
	int sock = -1;
	std::cout<<"sys_event_handler called."<<std::endl;
	switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
    	/*
		 *	INSERT CONFIGURATION PHASE HERE
		 *		wifi ssid/password (can be a cell phone or another board that works as an AP)
		 *		
		 */
    	std::cout<<"system_event_sta_start."<<std::endl;
    	wifi_ap->connect(); //sync timeline at boot time

    	//Sniffer::scan();
        break;
    
    case SYSTEM_EVENT_STA_GOT_IP:
    
    	std::cout<<"system_event_sta_got_ip."<<std::endl;

    	/* 
    	 * Send packets to the server and sync timebase with server
    	 */
        sock = Sniffer::create_link(SERVER_IP, SERVER_PORT);
        printf("SOCKET BEFORE SYNC: %d\n",sock);

        if(skip_send == false)
        	Sniffer::send_packets(sock); //first time is skipped
        else 
        	Sniffer::initPhase(sock);

		Sniffer::syncTimeline(sock); //sync and disconnect wifi 
		
		wifi_ap->disconnect(); //call another event case

        break;
    
    case SYSTEM_EVENT_STA_DISCONNECTED:
    	std::cout<<"system_event_sta_disconnected."<<std::endl;
    	
    	/*
    	 * Start sniffing process.
    	 * at the end activate wifi.
    	 */
    	if(skip_send == false)
    		Sniffer::scan();

    	wifi_ap->connect();

        break;
    default:
    	std::cout<<"event not handled."<<std::endl;
        break;
    }
    return ESP_OK;
}

void 
Sniffer::initPhase(int sock){

	int num_packets = htons(0);
	/* SENDING NUMBER OF SNIFFED PACKETS */ 
	int w_check = write(sock,&num_packets,sizeof(int));
	if(w_check <= 0){
		printf("write error on init phase. errno: %s\n",strerror(errno));
	}

}

void
Sniffer::scan(){
	Sniffer::scan(DEFAULT_SCAN_DURATION);
}

void
Sniffer::scan(long scan_duration){
	std::cout<<"loop called."<<std::endl;
	uint8_t channel = 0;

	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

	Timeline duration;

	while (duration.now() < scan_duration) {
		//#ifdef XTASK_MODE
			vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
		//#endif
		esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
		channel = (channel % WIFI_CHANNEL_MAX) + 1;
	}
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
}

void
Sniffer::incoming_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type){

	if (type != WIFI_PKT_MGMT)
		return;

	if(!Packet::isProbeReq(buff))
		return;

	//it is a probe request!
	Packet *p = new Packet();
	if( p->fetchData(buff, time_offset) !=0) //CHECK IF IT WORKS
		delete p;

	p->print();
	container->insert(*p);
}

void 
Sniffer::print_container(){

	std::cout<<"Container content: "<<std::endl;
	if(container->size() == 0)
		std::cout<<"empty."<<std::endl;
	else {
		std::set<Packet>::iterator it;
		for (it = container->begin(); it != container->end(); ++it)
		{
			Packet p = *it;
			p.print();
		}
	}	

}


void
Sniffer::send_packets(int sock){

	std::cout<<"send_packets called."<<std::endl;

    bool flag_error = false;

    tcpip_adapter_ip_info_t ip_info;
    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
    printf("IP :  %s\n", ip4addr_ntoa(&ip_info.ip));

	int num_packets = htons(container->size());
	/* SENDING NUMBER OF SNIFFED PACKETS */ 
	int w_check = write(sock,&num_packets,sizeof(int));
	if(w_check <= 0){
		printf("write error on num packets. errno: %s\n",strerror(errno));
		flag_error = true;
	} 
	std::cout<<"container size: "<<container->size()<<std::endl;


	if(container->size() > 0){
	    /* 
	     * SEND PACKETS HERE
		 */

		if(flag_error == false){
			std::set<Packet>::iterator it;
			for (it = container->begin(); it != container->end(); ++it)
			{
				Packet p = *it;

		    	if(p.send(sock) != 0){
		    		flag_error = true;
		    		break;
		    	} else
		    		p.print();

			}
		}

		container->clear(); 
	}
}

int
Sniffer::syncTimeline(int sock){
	/*
	 *	TO DO!
	 */
	char ack;
	int r_check = 0;
	printf("SOCKET IN SYNC: %d\n",sock);
	while(r_check == 0){
		r_check = read(sock,&ack,sizeof(char));
		if(r_check < 0){
			printf("read error on sync timeline. errno: %s\n",strerror(errno));
			close(sock);
			break;
		} 

		if(r_check > 0 && ack == 'a'){
			skip_send = false; 
			break;
		}
	}

	time_offset->reset();
	printf("CLOSED SOCKET HERE.\n");
	close(sock);
	return 0;
}

void
Sniffer::stop(){
	delete wifi_ap;
}

int 
Sniffer::create_link(const char *server_ip, unsigned server_port){
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	inet_pton(AF_INET, server_ip, &serverAddress.sin_addr.s_addr);
	serverAddress.sin_port = htons(server_port);

	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock < 0){
		printf("socket error. errno: %s\n",strerror(errno));
		skip_send = true;
		return -1;
	}

	
	int rc = connect(sock, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in));
	if(rc != 0){
		printf("connection failed. errno: %s\n",strerror(errno));
		skip_send = true;
		return -1;
	} else 
		printf("connected to server.\n");
	
	return sock;
}
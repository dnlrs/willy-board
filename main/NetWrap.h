#pragma once

#include "Packet.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include <string.h>
#include "lwip/sockets.h"
#include <string.h>
#include <iostream>


using namespace std;

class NetWrap{
	int socket_dsc;

	struct sockaddr_in serverAddress;

public:

	NetWrap(const char *server_ip, unsigned server_port);

	bool connect();
	bool disconnect();

	int get_descriptor(){ return socket_dsc; }
	bool send(Packet& p);

	bool wait_start(); //start is sent by server

	~NetWrap();
};


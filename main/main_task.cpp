#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <thread>
#include "Sniffer.h"
#include "PacketContainer.h"
#include "WiFi.h"

//SOME WIFI PASSWORDS ;)

#define SSID "HUAWEI_MS_NET"
#define PASSPHRASE "pdsproject"
//#define SSID "Vodafone-54858256"
//#define PASSPHRASE "_Qu4nt03bu0n4l4cqu4_"
//#define SSID "Tinkywinky"
//#define PASSPHRASE "1234554321AEEM@"
//#define SSID "Vodafone-58171850"
//#define PASSPHRASE "48fc7l97fld67hp"

//#define DEFAULT_SERVER_IP 			"192.168.1.4"
#define DEFAULT_SERVER_IP				"192.168.43.59"
//#define DEFAULT_SERVER_IP				"192.168.1.100"
#define DEFAULT_SERVER_PORT 			27015

#define PACKET_QUEUE_MAX_SIZE			128

using namespace std;

extern "C" {
	void app_main(void);
}

PacketContainer *packet_queue;
WiFi *wifi_handler;

void app_main(void)
{
	std::cout<<"probe request watchdog starting..."<<std::endl;

	packet_queue = new PacketContainer(PACKET_QUEUE_MAX_SIZE);
	wifi_handler = new WiFi(SSID,PASSPHRASE);

	wifi_handler->wait_connection();

	Sniffer sniffer(DEFAULT_SERVER_IP, DEFAULT_SERVER_PORT);
	sniffer.start(); //infinite loop inside
}

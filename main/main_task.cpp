#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "time.h"
#include <iostream>
#include <string>
#include <set>
#include "Sniffer.h"

#define SSID "HUAWEI_MS_NET"
#define PASSPHRASE "pdsproject"
//#define SSID "Vodafone-54858256"
//#define PASSPHRASE "_Qu4nt03bu0n4l4cqu4_"
//#define SSID "Tinkywinky"
//#define PASSPHRASE "1234554321AEEM@"
//#define SSID "Vodafone-58171850"
//#define PASSPHRASE "48fc7l97fld67hp"

extern "C" {
	void app_main(void);
}

WiFi *wifi_ap;

void app_main(void)
{
	std::cout<<"spy agent starting..."<<std::endl;

	Sniffer::start();

	ESP_ERROR_CHECK( esp_event_loop_init(Sniffer::sys_event_handler, NULL));

	wifi_ap = new WiFi(SSID,PASSPHRASE);
	wifi_ap->init(); //init wifi 

	esp_wifi_set_promiscuous_rx_cb(&(Sniffer::incoming_packet_handler)); //init callback for promiscuous mode
}

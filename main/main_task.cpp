#include "sdkconfig.h"

#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"

#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "PacketContainer.h"
#include "Sniffer.h"
#include "WiFi.h"

// SOME WIFI PASSWORDS ;)

#define DEFAULT_SERVER_IP "192.168.1.109"
#define DEFAULT_SERVER_PORT 27015

#define PACKET_QUEUE_MAX_SIZE 128

static constexpr char const* tag = "wwb";

extern "C" {
void app_main(void);
}

static EventGroupHandle_t sync_group;
static const int wifi_connected   = BIT0;
static const int server_connected = BIT1;

PacketContainer* packet_queue = nullptr;
WiFi* wifi_handler            = nullptr;

void
app_main(void)
{
    ESP_LOGI(tag, "Probe request watchdog starting...");

    sync_group = xEventGroupCreate();

    packet_queue = new PacketContainer(PACKET_QUEUE_MAX_SIZE);
    wifi_handler = new WiFi();

    wifi_handler->wait_connection();

    Sniffer sniffer(DEFAULT_SERVER_IP, DEFAULT_SERVER_PORT);
    sniffer.start(); // infinite loop inside
}

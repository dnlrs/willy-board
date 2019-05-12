#ifndef SNIFFER_H_INCLUDED
#define SNIFFER_H_INCLUDED
#pragma once

#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "NetWrap.h"
#include "WiFi.h"

#define SNIF_TASK_STACK_SIZE 4096
#define SNIF_TASK_PRIORITY 2
#define SNIF_TASK_RUNNING_CORE 1

// blinking led when sending packet
#define BLINK_GPIO 2

// task synchronization
extern EventGroupHandle_t sync_group;
extern const int wifi_connected_bit;
extern const int server_connected_bit;
extern const int wifi_reset_bit;
extern const int reboot_bit;    

class Sniffer {
    // reference to network wrapper (has connection to server)
    NetWrap& netw_ref;

    // sender task handle
    TaskHandle_t task_sender_handle = nullptr;

    // logging tag
    static constexpr char const* tag = "wwb-Sniffer";

private:
    void blink(int ms);

public:
    Sniffer(NetWrap& netwrap_reference);
    ~Sniffer();

    void start();

    static void sender_task(void* pvParameters);
    static void incoming_packet_cb(void* buff,
                                   wifi_promiscuous_pkt_type_t type);
};

#endif // !SNIFFER_H_INCLUDED
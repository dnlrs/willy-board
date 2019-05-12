#ifndef WIFI_H_INCLUDED
#define WIFI_H_INCLUDED
#pragma once

#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

extern EventGroupHandle_t sync_group;
extern const int wifi_connected_bit;
extern const int server_connected_bit;

extern uint8_t esp_mac[6];

class WiFi {
    // logging tag
    static constexpr char const* tag = "wwb-WiFi";

private:
    void init();
    void deinit();

    void connect();

public:
    WiFi();
    ~WiFi();

    // calls deinit(), waits a second and calls init()
    void reset();

    void disconnect();

    static esp_err_t event_handler(void* ctx, system_event_t* event);
};

#endif // !WIFI_H_INCLUDED
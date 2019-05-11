#ifndef WIFI_H_INCLUDED
#define WIFI_H_INCLUDED
#pragma once

#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_loop.h"

#include <condition_variable>
#include <mutex>
#include <string>

using std::condition_variable;
using std::mutex;
using std::string;
using std::unique_lock;

extern EventGroupHandle_t sync_group;
extern const int wifi_connected;
extern const int server_connected;

class WiFi {
    // thread-shared status variable, initialized in 'WiFi.cpp'
    bool wifi_connected = false;

    // other threads subscribe to this cv in order
    // to know when wifi is started successfully
    condition_variable connected_flag;
    mutex m;

    // logging tag
    static constexpr char const* tag = "wwb-WiFi";

private:
    void set_wifi_connected(bool is_connected)
    {
        unique_lock<mutex> ul(m);
        wifi_connected = is_connected;

        // if connected notify any waiting thread
        if (wifi_connected)
            connected_flag.notify_all();
    }

public:
    WiFi();
    ~WiFi();

    void wait_connection()
    {
        unique_lock<mutex> ul(m);
        connected_flag.wait(ul, [this]() { return wifi_connected; });
    }

    void enable();
    void disable();

    static esp_err_t event_handler(void* ctx, system_event_t* event);
};

// extern global handler for wifi
extern WiFi* wifi_handler;

#endif // !WIFI_H_INCLUDED
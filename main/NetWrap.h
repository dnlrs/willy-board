#ifndef NETWRAP_H_INCLUDED
#define NETWRAP_H_INCLUDED
#pragma once

#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "nvs_flash.h"

#include <condition_variable>
#include <mutex>

#include "Packet.h"
#include "WiFi.h"

using std::condition_variable;
using std::lock_guard;
using std::mutex;

#define TASK_STACK_SIZE 4096
#define TASK_PRIORITY 2

#define INVALID_SOCKET -1

class NetWrap {
    int socket_dsc;

    // socket ids
    int udp_socket = INVALID_SOCKET;
    int tcp_socket = INVALID_SOCKET;

    // server info
    uint32_t server_ip   = 0;
    uint16_t server_port = 0;

    struct sockaddr_in serverAddress;

    // server connection status
    bool server_connected = false;

    // sync tools
    mutex m;
    condition_variable connected_flag;

    // task handle
    TaskHandle_t task_handle = nullptr;

    // logging tag
    static constexpr char const* tag = "wwb-NetWrap";

private:
    void set_server_connected(bool is_connected)
    {
        unique_lock<mutex> ul(m);
        server_connected = is_connected;

        // if server is connected notify all waiting threads
        if (server_connected)
            connected_flag.notify_all();
    }

    void init();

    bool listen_udp_adverts();
    bool connect_to_server();

    bool is_connection_broken();
    void close_connection(int socket);

public:
    NetWrap(unsigned server_port);
    NetWrap(const char* server_ip, unsigned server_port);

    void wait_connection()
    {
        unique_lock<mutex> ul(m);
        connected_flag.wait(ul, [this]() { return server_connected; });
    }

    bool send_packet(Packet& p);

    bool connect();
    bool disconnect();

    int get_descriptor() { return socket_dsc; }

    static void netwrap_task(void* pvParameters);

    ~NetWrap();
};

#endif // !NETWRAP_H_INCLUDED
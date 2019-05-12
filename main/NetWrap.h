#ifndef NETWRAP_H_INCLUDED
#define NETWRAP_H_INCLUDED
#pragma once

#include "sdkconfig.h"

#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "nvs_flash.h"

#include "sys/select.h"
#include "sys/errno.h"

#include "Packet.h"

#define NETW_TASK_STACK_SIZE 4096
#define NETW_TASK_PRIORITY 2
#define NETW_TASK_RUNNING_CORE 0

#define INVALID_SOCKET -1

extern EventGroupHandle_t sync_group;
extern const int wifi_connected_bit;
extern const int server_connected_bit;
extern const int wifi_reset_bit;
extern const int reboot_bit;    

extern uint8_t esp_mac[6];

class NetWrap {
    // socket ids
    int udp_socket = INVALID_SOCKET;
    int tcp_socket = INVALID_SOCKET;

    // server info
    uint32_t server_ip   = 0;
    uint16_t server_port = 0;

    // task handle
    TaskHandle_t task_handle = nullptr;

    // logging tag
    static constexpr char const* tag = "wwb-NetWrap";

private:
    void init();

    bool listen_udp_adverts();
    bool connect_to_server();

    bool is_connection_broken();
    void close_connection(int socket);

    /* converts server ip to string */
    bool get_server_ip_str(char* ip_addr, int ip_addr_size);

    bool send_n(int sock, const void* dataptr, size_t size);

    bool send_mac_address();

    static void netwrap_task(void* pvParameters);

public:
    NetWrap();
    ~NetWrap();

    bool send_packet(Packet& p);
};

#endif // !NETWRAP_H_INCLUDED
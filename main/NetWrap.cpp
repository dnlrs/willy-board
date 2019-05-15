#include "NetWrap.h"

using std::string;

NetWrap::NetWrap()
{
    init();

    xTaskCreatePinnedToCore(&NetWrap::netwrap_task, "NetWrap_Task",
                            NETW_TASK_STACK_SIZE, this, NETW_TASK_PRIORITY,
                            &task_handle, NETW_TASK_RUNNING_CORE);

    if (task_handle == nullptr)
        ESP_LOGE(tag, "(NetWrap::ctor) failed to create task");
}

NetWrap::~NetWrap()
{
    init();
};

void
NetWrap::init()
{
    xEventGroupClearBits(sync_group, server_connected_bit);

    if (udp_socket != INVALID_SOCKET)
        closesocket(udp_socket);

    close_connection(tcp_socket);

    tcp_socket  = INVALID_SOCKET;
    udp_socket  = INVALID_SOCKET;
    server_ip   = 0;
    server_port = 0;
}

bool
NetWrap::send_packet(Packet& packet)
/*
    Send first packet size (in network byte order) then send the actual packet.
    Note: send() returns the total number of bytes sent, which may be less
    which the number requested to be sent.
*/
{
    char buf[MAX_PACKET_SIZE];
    uint32_t packet_len = packet.serialize(buf);

    /* sending packet size */
    uint32_t pkt_len_nbo = htonl(packet_len);
    bool rval = send_n(tcp_socket, (char*)&(pkt_len_nbo), 4);
    if (rval == false)
        return false;

    /* sending actual packet */
    rval = send_n(tcp_socket, buf, packet_len);
    if (rval == false)
        return false;

    return true;
}

bool
NetWrap::send_n(int sock, const void* dataptr, size_t size)
/*
    May update server connection status if an error occurs
    indicating that the connection has been lost.
*/
{
    char* pbuf = (char*)dataptr;

    int left_bytes    = size;
    int written_bytes = 0;

    while (left_bytes > 0) {
        written_bytes = ::send(sock, pbuf, left_bytes, 0);
        if (written_bytes < 0) {
            int error = errno;
            ESP_LOGE(tag, "(send_n) write error: %s", strerror(error));

            if (error == ECONNRESET ||   /* connection reset by peer */
                error == EDESTADDRREQ || /* socket not in connection-mode */
                error == ENOTCONN ||     /* socket is not connected */
                error == EPIPE)          /* local end has been shut down */
                xEventGroupClearBits(sync_group, server_connected_bit);

            if (error == ENETDOWN ||  /* Network interface is not configured */
                error == ENETRESET || /* Connection aborted by network */
                error == ENETUNREACH) /* Network is unreachable */
            {
                xEventGroupClearBits(sync_group, server_connected_bit);
                xEventGroupClearBits(sync_group, wifi_reset_bit);
            }
            return false;
        }

        pbuf += written_bytes;
        left_bytes -= written_bytes;
    }

    return true;
}

bool
NetWrap::send_mac_address()
{
    const uint32_t mac_size = 6;
    uint32_t header         = htonl(mac_size);

    bool rval = send_n(tcp_socket, (char*)&header, 4);
    if (rval == false)
        return false;

    rval = send_n(tcp_socket, esp_mac, mac_size);
    if (rval == false)
        return false;

    return true;
}

bool
NetWrap::listen_udp_adverts()
/*
    1. create socket
    2. bind socket to any incoming address and specific port
    3. check if socket is ready for reading for some time
    4. read from socket
    5. compare magic
    6. save server ip and port
*/
{
    init();

    udp_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket < 0) {
        ESP_LOGE(tag, "(listen_udp_adverts) socket creation error: %s",
                 strerror(errno));
        return false;
    }
    ESP_LOGV(tag, "(listen_udp_adverts) socket created.");

    struct sockaddr_in dst_addr;
    socklen_t dst_addr_len = sizeof(dst_addr);
    bzero(&dst_addr, dst_addr_len);

    dst_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dst_addr.sin_family      = AF_INET;
    dst_addr.sin_port        = htons(CONFIG_DISCOVERY_PORT);

    int rval = ::bind(udp_socket, (sockaddr*)&dst_addr, dst_addr_len);
    if (rval < 0) {
        ESP_LOGE(tag, "(listen_udp_adverts) bind error: %s", strerror(errno));
        return false;
    }
    ESP_LOGV(tag, "(listen_udp_adverts) socket bound.");

    /*
        Since socket is non-blocking, use select to check only for a limited
        amount of time if the socket is ready for reading.
    */
    struct timeval tv;
    tv.tv_sec  = 2;
    tv.tv_usec = 0;

    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(udp_socket, &rset);

    rval = ::select(udp_socket + 1, &rset, NULL, NULL, &tv);
    if (rval < 0 || rval > 1) {
        ESP_LOGE(tag, "(listen_udp_adverts) select error: %s", strerror(errno));
        return false;
    }
    else if (rval == 0) {
        ESP_LOGI(tag, "(listen_udp_adverts) timeout expired, no adverts.");
        return false;
    }

    if (FD_ISSET(udp_socket, &rset) == false) {
        // select returned 1 but the *only* socket wasn't ready for read
        ESP_LOGE(tag, "(listen_udp_adverts) unexpected ready socket.");
        return false;
    }

    char buf[128];
    struct sockaddr_in src_addr;
    socklen_t src_addr_len = sizeof(src_addr);
    bzero(&src_addr, src_addr_len);

    rval = ::recvfrom(udp_socket, &buf, (sizeof(buf) - 1), 0,
                      (sockaddr*)&src_addr, &src_addr_len);
    if (rval < 0) {
        ESP_LOGE(tag, "(listen_udp_adverts) recvfrom error: %s",
                 strerror(errno));
        return false;
    }
    buf[rval] = '\0';
    ESP_LOGV(tag, "(listen_udp_adverts) udp message received.");

    const char magic[] = "9pointspls";
    if (strncmp(buf, magic, sizeof(magic)) != 0) {
        ESP_LOGE(tag, "(listen_udp_adverts) wrong magic");
        return false;
    }
    ESP_LOGV(tag, "(listen_udp_adverts) magic matched.");

    char addr_str[IP_MAX_LEN];
    inet_ntoa_r(src_addr.sin_addr.s_addr, addr_str, (sizeof(addr_str) - 1));
    ESP_LOGI(tag, "(listen_udp_adverts) server address is %s:%d", addr_str,
             ntohs(src_addr.sin_port));

    server_ip   = src_addr.sin_addr.s_addr;
    server_port = CONFIG_SERVER_PORT;

    return true;
}

bool
NetWrap::connect_to_server()
/*
    1. create socket
    2. tcp-connect to server
    3. send mac address
    4. wait for ack from server
*/
{
    if (server_ip == 0 || server_port == 0) {
        ESP_LOGE(tag, "(connect_to_server) server ip and/or port is null");
        return false;
    }

    // create socket
    tcp_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (tcp_socket < 0) {
        ESP_LOGE(tag, "(connect_to_server) socket creation failed: %s",
                 strerror(errno));
        return false;
    }
    ESP_LOGV(tag, "(connect_to_server) socket created.");

    // tcp-connect to server
    struct sockaddr_in srv_addr;
    socklen_t srv_addr_len = sizeof(srv_addr);
    bzero(&srv_addr, srv_addr_len);

    srv_addr.sin_addr.s_addr = server_ip;
    srv_addr.sin_port        = htons(server_port);
    srv_addr.sin_family      = AF_INET;

    int rval = ::connect(tcp_socket, (sockaddr*)&srv_addr, srv_addr_len);
    if (rval < 0) {
        int error = errno;
        ESP_LOGE(tag, "(connect_to_server) socket connection failed: %s",
                 strerror(error));
        if (error == ENETDOWN ||  /* Network interface is not configured */
            error == ENETRESET || /* Connection aborted by network */
            error == ENETUNREACH) /* Network is unreachable */
            xEventGroupSetBits(sync_group, wifi_reset_bit);

        return false;
    }
    ESP_LOGI(tag, "(connect_to_server) connected to server.");

    // send mac address
    bool brval = send_mac_address();
    if (brval == false) {
        ESP_LOGE(tag, "(connect_to_server) failed to send mac address.");
        return false;
    }
    ESP_LOGD(tag, "(connect_to_server) mac address sent.");

    // wait for ack
    uint32_t ack;
    rval = ::read(tcp_socket, &ack, sizeof(ack));
    if (rval <= 0) {
        if (rval == 0)
            ESP_LOGI(tag, "(connect_to_server) server closed the connection.");
        else
            ESP_LOGE(tag, "(connect_to_server) read failed: %s",
                     strerror(errno));

        return false;
    }
    ESP_LOGD(tag, "(connect_to_server) ACK received from server.");

    return true;
}

bool
NetWrap::is_connection_broken()
/*
    If the server closes the connection in a gracefully way the connected
    socket will be ready to read and will return EOF.
*/
{
    if (tcp_socket == 0)
        return true;

    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 200000; // 200 ms

    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(tcp_socket, &rset);

    int rval = ::select(tcp_socket + 1, &rset, NULL, NULL, &tv);
    switch (rval) {
    case 0: // timeout expired
        ESP_LOGV(tag,
                 "(is_connection_broken) select timeout expired, all good.");
        return false;
        break;
    case 1: // socket is ready for reading
        if (FD_ISSET(tcp_socket, &rset)) {
            char buf[128];
            rval = ::read(tcp_socket, buf, (sizeof(buf) - 1));
            if (rval < 0) {
                ESP_LOGE(tag, "(is_connection_broken) read failed: %s",
                         strerror(errno));
                return true;
            }
            else if (rval == 0) {
                ESP_LOGI(tag,
                         "(is_connection_broken) connection closed by server.");
                return true;
            }
            else /* rval > 0 */ {
                buf[rval] = '\0';
                ESP_LOGI(tag, "(is_connection_broken) unexpected message: '%s'",
                         buf);
                return false;
            }
        }
        // select returned 1 but the *only* socket wasn't ready for read
        ESP_LOGE(tag, "(is_connection_broken) unexpected ready socket.");
        return true;
        break;
    default: // select error (rval < 0) || (rval > 1) (non sense)
        ESP_LOGE(tag, "(is_connection_broken) select failed: %s",
                 strerror(errno));
        return true;
        break;
    }

    // should never get here
    ESP_LOGE(tag, "(is_connection_broken) should never get here, fix this.");
    return true;
}

void
NetWrap::close_connection(int socket)
{
    if (socket == INVALID_SOCKET)
        return;

    if (shutdown(socket, SHUT_RDWR) < 0) {
        ESP_LOGE(tag, "(close_connection) shutdown error %s", strerror(errno));
    }
    if (closesocket(socket) < 0) {
        ESP_LOGE(tag, "(close_connection) closesocket error %s",
                 strerror(errno));
    }
}

bool
NetWrap::get_server_ip_str(char* ip, int ip_size)
{
    if (ip == nullptr || ip_size < IP_MAX_LEN) {
        ESP_LOGE(tag, "(get_server_ip_str) invalid return buffer.");
        return false;
    }

    if (server_ip == 0) {
        ESP_LOGE(tag, "(get_server_ip_str) server address is null.");
        return false;
    }

    char* rval = inet_ntoa_r(server_ip, ip, ip_size);
    if (rval == nullptr) {
        ESP_LOGE(tag, "(get_server_ip_str) address conversion failed: %s",
                 strerror(errno));
        return false;
    }

    return true;
}

void
NetWrap::netwrap_task(void* pvParameters)
{
    NetWrap* netw_handle = (NetWrap*)pvParameters;

    bool rval = true;
    while (true) {
        // check wifi connection and server connection bits, block on wifi bit
        EventBits_t rbits = xEventGroupWaitBits(sync_group, wifi_connected_bit,
                                                false, true, portMAX_DELAY);

        if ((rbits & server_connected_bit) != 0) {
            rval = netw_handle->is_connection_broken();
            if (rval == true)
                xEventGroupClearBits(sync_group, server_connected_bit);
        }
        else {
            /*
                1. initialize (and close any old connection)
                2. listen for adverts from server
                3. connect to server
                4.a convert server ip to string for sntp service
                4.b start sntp service
             */
            netw_handle->init();

            rval = netw_handle->listen_udp_adverts();
            if (rval == false)
                continue;

            rval = netw_handle->connect_to_server();
            if (rval == false)
                continue;

            char buf[IP_MAX_LEN];
            rval = netw_handle->get_server_ip_str(buf, IP_MAX_LEN);
            if (rval == false)
                continue;

            rval = Timeline::initialize_sntp(buf, strnlen(buf, IP_MAX_LEN));
            if (rval == false)
                continue;

            xEventGroupSetBits(sync_group, server_connected_bit);
        }
    }
}
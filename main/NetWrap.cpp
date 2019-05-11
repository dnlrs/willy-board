#include "NetWrap.h"

// #include "sys/errno.h"
// #include "sys/fcntl.h"
// #include "sys/select.h"

using std::string;

NetWrap::NetWrap(unsigned server_port)
{
    init();

    xTaskCreate(&NetWrap::netwrap_task, "NetWrap_Task", TASK_STACK_SIZE, this,
                TASK_PRIORITY, &task_handle);

    if (task_handle == nullptr)
        ESP_LOGE(tag, "(NetWrap::ctor) failed to create task");
}

NetWrap::NetWrap(const char* server_ip, unsigned server_port)
{
    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &serverAddress.sin_addr.s_addr);
    serverAddress.sin_port = htons(server_port);

    socket_dsc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_dsc < 0)
        printf("socket error. errno: %s\n", strerror(errno));
}

void
NetWrap::init()
{
    closesocket(udp_socket);
    close_connection(tcp_socket);

    tcp_socket  = INVALID_SOCKET;
    udp_socket  = INVALID_SOCKET;
    server_ip   = 0;
    server_port = 0;

    server_connected = false;
}

bool
NetWrap::connect()
{
    int rc = ::connect(socket_dsc, (struct sockaddr*)&serverAddress,
                       sizeof(struct sockaddr_in));
    if (rc != 0) {
        cout << "connection failed. errno: " << strerror(errno) << endl;
        return false;
    }
    else
        cout << "connected to server." << endl;

    return true;
}

bool
NetWrap::disconnect()
{
    if (socket_dsc > 0)
        close(socket_dsc);

    socket_dsc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_dsc < 0) {
        printf("socket error. errno: %s\n", strerror(errno));
        return false;
    }
    return true;
}

bool
NetWrap::send_packet(Packet& packet)
/*
    Send first packet size (in network byte order) then send the actual packet.
    Note: send() returns the total number of bytes sent, which may be less 
    which the number requested to be sent.
*/
{
    char buf[512];
    packet.serialize(buf);

    uint32_t packet_size = htonl(packet.get_packet_size());

    /* sending packet size */
    char* pbuf        = (char*)&(packet_size);

    int left_bytes    = 4;
    int written_bytes = 0;
    while (left_bytes > 0) {
        written_bytes = ::send(tcp_socket, pbuf, left_bytes, 0);
        if (written_bytes < 0) {
            ESP_LOGE(tag, "(send_packet) write error: %s", strerror(errno));
            return false;
        }

        pbuf += written_bytes;
        left_bytes -= written_bytes;
    }

    /* sending actual packet */
    pbuf = buf;

    left_bytes    = ntohl(packet_size);
    written_bytes = 0;
    while (left_bytes > 0) {
        written_bytes = ::send(tcp_socket, pbuf, left_bytes, 0);
        if (written_bytes < 0) {
            ESP_LOGE(tag, "(send_packet) write error: %s", strerror(errno));
            return false;
        }

        pbuf += written_bytes;
        left_bytes -= written_bytes;
    }

    return true;
}

NetWrap::~NetWrap()
{
    init();
};

bool
NetWrap::listen_udp_adverts()
/*
    1. create socket
    2. bind socket to any incoming address and specific port
    3. check if socket is ready for reading for some time
    4. read from socket
    5. compare magic
    6 save server ip and port
*/
{
    init();

    udp_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket < 0) {
        ESP_LOGE(tag, "(listen_udp_adverts) socket creation error: %s",
                 strerror(errno));
        return false;
    }
    ESP_LOGD(tag, "(listen_udp_adverts) socket created.");

    struct sockaddr_in dst_addr;
    socklen_t dst_addr_len = sizeof(dst_addr);
    bzero(&dst_addr, dst_addr_len);

    dst_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dst_addr.sin_family      = AF_INET;
    dst_addr.sin_port = htons(27016); // TODO: change port to config macro

    int rval = ::bind(udp_socket, (sockaddr*)&dst_addr, dst_addr_len);
    if (rval < 0) {
        ESP_LOGE(tag, "(listen_udp_adverts) bind error: %s", strerror(errno));
        return false;
    }
    ESP_LOGD(tag, "(listen_udp_adverts) socket bound.");

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
        ESP_LOGI(tag, "(listen_udp_adverts) select timeout expired, all good.");
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
    ESP_LOGD(tag, "(listen_udp_adverts) udp message received.");

    const char magic[] = "9pointspls";
    if (strncmp(buf, magic, sizeof(magic)) != 0) {
        ESP_LOGE(tag, "(listen_udp_adverts) wrong magic");
        return false;
    }
    ESP_LOGD(tag, "(listen_udp_adverts) magic matched.");

    char addr_str[13];
    inet_ntoa_r(src_addr.sin_addr.s_addr, addr_str, (sizeof(addr_str) - 1));
    ESP_LOGI(tag, "(listen_udp_adverts) server address is %s:%d", addr_str,
             ntohs(src_addr.sin_port));

    server_ip   = src_addr.sin_addr.s_addr;
    server_port = 27015; // TODO: make this global config

    return true;
}

bool
NetWrap::connect_to_server()
/*
    1. create socket
    2. tcp-connect to server
    3. wait for ack from server
*/
{
    if (server_ip == 0 || server_port == 0) {
        ESP_LOGE(tag, "(connect_to_server) server ip and/or port is null");
        return false;
    }

    tcp_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (tcp_socket < 0) {
        ESP_LOGE(tag, "(connect_to_server) socket creation failed: %s",
                 strerror(errno));
        return false;
    }
    ESP_LOGD(tag, "(connect_to_server) socket created.");

    struct sockaddr_in srv_addr;
    socklen_t srv_addr_len = sizeof(srv_addr);
    bzero(&srv_addr, srv_addr_len);

    srv_addr.sin_addr.s_addr = server_ip;
    srv_addr.sin_port        = htons(server_port);
    srv_addr.sin_family      = AF_INET;

    int rval = ::connect(tcp_socket, (sockaddr*)&srv_addr, srv_addr_len);
    if (rval < 0) {
        ESP_LOGE(tag, "(connect_to_server) socket connection failed: %s",
                 strerror(errno));
        return false;
    }
    ESP_LOGD(tag, "(connect_to_server) connected to server.");

    uint32_t ack;
    rval = read(tcp_socket, &ack, sizeof(ack));
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
        ESP_LOGD(tag,
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
    if (socket == 0)
        return;

    if (shutdown(socket, SHUT_RDWR) < 0) {
        ESP_LOGD(tag, "(close_connection) shutdown error %s", strerror(errno));
    }
    if (closesocket(socket) < 0) {
        ESP_LOGE(tag, "(close_connection) closesocket error %s",
                 strerror(errno));
    }
}

void
NetWrap::netwrap_task(void* pvParameters)
{
    NetWrap* netw_handle = (NetWrap*)pvParameters;
    bool rval            = true;

    while (true) {
        wifi_handler->wait_connection();

        if (netw_handle->server_connected) {
            rval = netw_handle->is_connection_broken();
            if (rval == true)
                netw_handle->set_server_connected(false);
        }
        else {
            /*
                1. initialize (and close any old connection)
                2. listen for adverts from server
                3. connect to server
             */
            netw_handle->init();

            rval = netw_handle->listen_udp_adverts();
            if (rval == false)
                continue;

            rval = netw_handle->connect_to_server();
            if (rval == false)
                continue;

            netw_handle->set_server_connected(true);
        }
    }
}
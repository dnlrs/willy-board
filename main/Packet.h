#ifndef PACKET_H_INCLUDED
#define PACKET_H_INCLUDED
#pragma once

#include "esp_log.h"
#include "esp_wifi_types.h"

#include "lwip/sockets.h"

#include <iostream>
#include <sstream>

#include "Timeline.h"
#include "md5.h"

#define MAC_LENGTH 6
#define MAX_SSID_LENGTH 32
#define MD5_HASH_LENGTH 32

using namespace std;

typedef struct {
    unsigned frame_ctrl : 16;
    unsigned duration_id : 16;
    uint8_t addr1[6]; /* receiver address */
    uint8_t addr2[6]; /* sender address */
    uint8_t addr3[6]; /* filtering address */
    unsigned sequence_ctrl : 16;
    // uint8_t addr4[6]; /* optional */
} wifi_frame_hdr_t;

typedef struct {
    uint8_t data[0];
    uint32_t crc;
} wifi_frame_payload_t;

typedef struct {
    wifi_frame_hdr_t hdr;
    uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_frame_t;

class Packet {
    int channel;
    int rssi;
    int sequence_ctrl;
    int timestamp;
    int ssid_length;
    char mac_addr[MAC_LENGTH];
    char ssid[MAX_SSID_LENGTH + 1];
    string hash;

    // logging tag
    static constexpr char const* tag = "wwb-Packet";

private:
    std::string genHash();

public:
    Packet() { hash = "test packet"; };
    Packet(string s) : hash(s){};

    /* Returns false if packet is not a probe request or invalid timestamp */
    bool fetchData(void* buff);

    static bool isProbeReq(void* buff);

    int get_packet_size()
    {
        return 5 * sizeof(int) +
               (ssid_length + MD5_HASH_LENGTH + MAC_LENGTH) * sizeof(char);
    }

    bool operator<(const Packet& p) const { return this->hash < p.hash; }

    void serialize(char* buf);
    void deserialize(char* buf);

    void debug_print();
    // string temp_print() { return hash; }
    // int get_timestamp() { return timestamp; }
};

#endif
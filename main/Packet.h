#ifndef PACKET_H_INCLUDED
#define PACKET_H_INCLUDED
#pragma once

#include "esp_log.h"
#include "esp_wifi.h"

#include "lwip/sockets.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Timeline.h"
#include "md5.h"
#include "packet_ieee80211.h"

#define MD5_HASH_LENGTH 32
#define MAX_PACKET_SIZE    (4 + /* channel          */ \
                            4 + /* rssi             */ \
                            4 + /* timestmap        */ \
                            2 + /* sequence control */ \
                            4 + /* number of SSIDs  */ \
                            4 + /* tags presence    */ \
                            8 + /* supported rates  */ \
                            HT_CAPABILITIES_LEN +   /* ht cap       */ \
                            EXT_CAPABILITIES_LEN +  /* ext cap      */ \
                            1 +                     /* interworking */ \
                            2 +                     /* multi band   */ \
                            VHT_CAPABILITIES_LEN +  /* vht          */ \
                            MAC_LENGTH +            /* mac          */ \
                            (10*MAX_SSID_LEN) +     /* ssids list   */ \
                            MD5_HASH_LENGTH)        /* packet hash  */ 

class Packet {
    uint32_t channel;
    int32_t  rssi;
    uint32_t timestamp;

    uint16_t sequence_control;
    uint8_t  src_mac[MAC_LENGTH];

    std::string hash;

    /* Fingerprinting */
    std::vector<std::string> ssid_list;

    uint32_t tag_presence;    // Bit flags indicating if element TAGs presence.
    uint64_t supported_rates; // Bit flags indicating supported rates.

    /* HT Capabilities */
    wlan_ht_t ht_capabilities;

    /* Extended Capabilities */
    wlan_ext_cap_t extended_capabilities; 

    /* Interworking */
    uint8_t interworking; // Access Network Options

    /* Multi Band */
    uint8_t multi_band_id;
    uint8_t multi_band_channel;

    /* VHT Capabilities */
    wlan_vht_cap_t vht_capabilities;


    // logging tag
    static constexpr char const* tag = "wwb-Packet";

private:
    bool fetch_ssid(wlan_ssid_t* data);
    bool fetch_supported_rates(wlan_supported_rates_t* data);
    bool fetch_ext_supported_rates(wlan_extended_sup_rates_t* data);
    bool fetch_ht_capabilities(wlan_ht_t* src);
    bool fetch_extended_capabilities(wlan_ext_cap_t* src);
    bool fetch_interworking(wlan_interworking_t* src);
    bool fetch_multi_band(wlan_multi_band_t* src);
    bool fetch_vht_capabilities(wlan_vht_cap_t* src);

    std::string genHash();

public:
    Packet();

    bool operator<(const Packet& p) const { return this->hash < p.hash; }

    /* Returns false if packet is not a probe request or invalid timestamp. */
    bool fetch_probr(void* buff);

    uint32_t serialize(char* buf);

    std::string str();

    static Packet forge_test_packet();
};

#endif
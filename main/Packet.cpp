/*
    Packet.cpp
*/

#include "Packet.h"

#include <iomanip>
#include <ios>

using std::string;
using std::stringstream;

Packet::Packet()
{
    channel = 0;
    rssi    = 0;
    sequence_control = 0;
    timestamp        = 0;

    for (int i = 0; i < MAC_LENGTH; i++)
        src_mac[i] = 0;
    
    tag_presence    = 0;
    supported_rates = 0;
}

bool
Packet::fetch_probr(void* buff)
/*
    1. check if packet is a management frame
    1. check the management frame is a probe request
    2. check if current timestamp is valid
    3. read packet fields from buffer
*/
{
    wifi_promiscuous_pkt_t* ppkt = (wifi_promiscuous_pkt_t*) buff;
    wifi_pkt_rx_ctrl_t* pkt_ctrl = (wifi_pkt_rx_ctrl_t*) &(ppkt->rx_ctrl);
    wlan_t* pframe = (wlan_t*) &(ppkt->payload);

    if (pframe->frame_control.type != MGT_FRAME) {
        ESP_LOGV(tag, "(fetch_probr) not a management packet, discarding packet.");
        return false;
    }

    if (pframe->frame_control.sub_type != MGT_PROBE_REQ) {
        ESP_LOGV(tag, "(fetch_probr) not a probe request, discarding packet.");
        return false;
    }

    /* Get timestamp from board */
    time_t ts = 0;
    time(&ts);

    if (!Timeline::is_valid(ts)) {
        ESP_LOGD(tag, "(fetch_probr) invalid timestamp, discarding packet.");
        return false;
    }
    timestamp = ts;

    /* Get channel and RSSI from packet radio metadata header */
    channel   = pkt_ctrl->channel;
    rssi      = pkt_ctrl->rssi;

    /* Get sequence control and source mac from management frame header */
    sequence_control = pframe->sequence_control;
    for (int i = 0; i < MAC_LENGTH; i++)
        src_mac[i] = pframe->src_mac[i];


    uint8_t* pframe_body       = &(pframe->frame_body[0]); // pointer to start of frame body
    uint32_t frame_length      = pkt_ctrl->sig_len;        // length of frame (header + body)
    uint32_t frame_body_length = frame_length - (MGMT_FRAME_HEADER_LEN + 4); // length of frame body (ignore header and CRC at end)
    
    if (pframe->frame_control.htc_order == 1) {
        pframe_body       += 4; // jump over ht_control[4] if present
        frame_body_length -= 4; // ignore HT Control header field
        ESP_LOGD(tag, "(fetch_probr) ht_control field present.");
    }

    /* 
        Loop through frame body and analyse elements
    */
    uint8_t* current_position = pframe_body;
    while ((current_position - pframe_body) < frame_body_length) {

        element_hdr* elm_hdr = (element_hdr*) current_position;

        switch ((int) elm_hdr->id) {
        case TAG_SSID: /* 1. mandatory, eventually empty */
            add_tag(TAG_SSID, &tag_presence);
            fetch_ssid((wlan_ssid_t*) current_position);
            break;

        case TAG_SUPP_RATES: /* 2. optional */
            add_tag(TAG_SUPP_RATES, &tag_presence);
            fetch_supported_rates((wlan_supported_rates_t*) current_position);
            break;

        case TAG_EXT_SUPP_RATES: /* 4. optional */
            add_tag(TAG_EXT_SUPP_RATES, &tag_presence);
            fetch_ext_supported_rates((wlan_extended_sup_rates_t*) current_position);
            break;

        case TAG_HT_CAPABILITY: /* 7. optional */
            if (fetch_ht_capabilities((wlan_ht_t*) current_position) == true)
                add_tag(TAG_HT_CAPABILITY, &tag_presence);
            break;
    
        case TAG_EXTENDED_CAPABILITIES: /* 9. optional */
            if (fetch_extended_capabilities((wlan_ext_cap_t*) current_position) == true)
                add_tag(TAG_EXTENDED_CAPABILITIES, &tag_presence);
            break;

        case TAG_SSID_LIST: /* 10. optional */
            add_tag(TAG_SSID_LIST, &tag_presence);
            {
                wlan_ssid_list_t* pssid_list = (wlan_ssid_list_t*) current_position;
                int pssid_offset = 0;
                while (pssid_offset < pssid_list->hdr.length) {
                    wlan_ssid_t* pssid = &(pssid_list->ssid_list[pssid_offset]);
                    if (fetch_ssid(pssid) == false)
                        break;
                    
                    pssid_offset += (pssid->hdr.length + 2); // ssid header and ssid string
                }
            }
            ESP_LOGD(tag, "(fetch_probr) SSSID List element present");
            break;

        case TAG_INTERWORKING: /* 12. optional */
            if (fetch_interworking((wlan_interworking_t*) current_position))
                add_tag(TAG_INTERWORKING, &tag_presence);
            break;

        case TAG_MULTI_BAND: /* 14. optional */
            if (fetch_multi_band((wlan_multi_band_t*) current_position))
                add_tag(TAG_MULTI_BAND, &tag_presence);
            break;   

        case TAG_VHT_CAPABILITY: /* 17. optional */
            if (fetch_vht_capabilities((wlan_vht_cap_t*) current_position))
                add_tag(TAG_VHT_CAPABILITY, &tag_presence);
            break;

        default:
            if (tags_description.find(elm_hdr->id) != tags_description.end()) {
                add_tag((int) elm_hdr->id, &tag_presence);
                ESP_LOGD(tag, "(fetch_probr) found %02x tag (%s)", elm_hdr->id, tags_description[elm_hdr->id].c_str());
            } else {
                ESP_LOGD(tag, "(fetch_probr) found %02x tag (No Descritpion)", elm_hdr->id);
            } 
            break;
        }

        // jump element header and element itself
        current_position += (elm_hdr->length + 2);
    }

    if ((current_position - pframe_body) != frame_body_length) {
        ESP_LOGE(tag, "(fetch_probr) packet badly parsed (%d != %d)", (current_position - pframe_body), frame_length);
        return false;
    }

    hash = genHash();
    return true;
}



bool 
Packet::fetch_ssid(wlan_ssid_t* data)
/*
    The length of the SSID information field is between 0 and 32 octets. 
    A 0 length information field is used within Probe Request management 
    frames to indicates the broadcast wildcard SSID.
*/
{
    if (data->hdr.length == 0) {
        ESP_LOGV(tag, "(fetch_ssid) ssid length is 0");
        return true; // ignore broadcast SSID
    }

    if (data->hdr.length > MAX_SSID_LEN) {
        ESP_LOGE(tag, "(fetch_ssid) ssid length greater than expected");
        return false;
    }

    string ssid((char*) &data->ssid[0], data->hdr.length);
    ssid_list.push_back(ssid);
    
    ESP_LOGV(tag, "(fetch_ssid) element present (length %d)", data->hdr.length);
    return true;
}

bool
Packet::fetch_supported_rates(wlan_supported_rates_t* data)
{
    if (data->hdr.length < 1 || data->hdr.length > MAX_SUPPORTED_RATES_LEN) {
        ESP_LOGE(tag, "(fetch_supported_rates) invalid length (%d)", data->hdr.length);
        return false;
    }

    for (int i = 0; i < data->hdr.length; i++) {
        uint8_t data_rate = data->supported_rates[i];
        if (sr_compacted.find(data_rate) == sr_compacted.end()) {
            ESP_LOGE(tag, "(fetch_supported_rates) suppported rate not found");
            continue;
        }

        supported_rates &= sr_compacted.at(data_rate);
    }

    ESP_LOGV(tag, "(fetch_supported_rates) element present");
    return true;
}

bool
Packet::fetch_ext_supported_rates(wlan_extended_sup_rates_t* data)
{
    if (data->hdr.length == 0) {
        ESP_LOGE(tag, "(fetch_ext_supported_rates) invalid length (%d)", data->hdr.length);
        return false;
    }

    for (int i = 0; i < data->hdr.length; i++) {
        uint8_t data_rate = data->extended_sup_rates[i];
        if (sr_compacted.find(data_rate) == sr_compacted.end()) {
            ESP_LOGE(tag, "(fetch_ext_supported_rates) supported rate not found");
            continue;
        }

        supported_rates &= sr_compacted.at(data_rate);
    }

    ESP_LOGV(tag, "(fetch_ext_supported_rates) element present");
    return true;
}

bool
Packet::fetch_ht_capabilities(wlan_ht_t* src)
{
    wlan_ht_t* dst = &ht_capabilities;

    dst->hdr.id     = src->hdr.id;
    dst->hdr.length = src->hdr.length;

    dst->capability_info       = src->capability_info;
    dst->ampduparam            = src->ampduparam;
    dst->extended_capabilities = src->extended_capabilities;
    dst->tbc                   = src->tbc;
    dst->asel_capabilities     = src->asel_capabilities;

    for (int i = 0; i < HT_MCSSET_LEN; i++)
        dst->mcsset[i] = src->mcsset[i];
    
    ESP_LOGV(tag, "(fetch_ht_capabilities) element present");
    return true;
}

bool
Packet::fetch_extended_capabilities(wlan_ext_cap_t* src)
{   
    if (src->hdr.length == 0) {
        ESP_LOGE(tag, "(fetch_extended_capabilies) invalid lenght");
        return false;
    }

    if (src->hdr.length > 9) {
        ESP_LOGD(tag, "(fetch_extended_capabilies) lenght is greater than expected (9), "
                 "only first 9 octects will be considered");
    }

    wlan_ext_cap_t* dst = &extended_capabilities;

    dst->hdr.id     = src->hdr.id;
    dst->hdr.length = (src->hdr.length > 9 ? 9 : src->hdr.length);

    for (int i = 0; i < dst->hdr.length; i++)
        dst->extended_capabilities[i] = src->extended_capabilities[i];
    
    ESP_LOGV(tag, "(fetch_extended_capabilies) element present");
    return true;
}

bool
Packet::fetch_interworking(wlan_interworking_t* src)
{
    interworking = src->access_network_options;
    ESP_LOGV(tag, "(fetch_interworking) element present");
    return true;
}

bool
Packet::fetch_multi_band(wlan_multi_band_t* src)
{
    multi_band_id      = src->band_id;
    multi_band_channel = src->channel_number;
    ESP_LOGV(tag, "(fetch_multi_band) element present");
    return true;
}

bool
Packet::fetch_vht_capabilities(wlan_vht_cap_t* src)
{
    wlan_vht_cap_t* dst = &vht_capabilities;

    dst->capabilities_info = src->capabilities_info;

    for (int i = 0; i < VHT_MCS_NSS_SET_LEN; i++)
        dst->supported_mcs_nss_set[i] = src->supported_mcs_nss_set[i];
    
    ESP_LOGV(tag, "(fetch_vht_capabilities) element present");
    return true;
}

void
Packet::debug_print()
{   
    stringstream concat_data;
    concat_data << 
        "\n\t" << "ch: " << channel << 
        "\n\t" << "rssi: " << rssi << 
        "\n\t" << "timestamp: " << timestamp <<
        "\n\t" << "sequence control: " << sequence_control;

    // mac        
    concat_data << "\n\t" << "mac: " << std::setw(2) << std::hex << (unsigned int) src_mac[0];
    concat_data << ":" << std::setw(2) << std::hex << (unsigned int) src_mac[1];
    concat_data << ":" << std::setw(2) << std::hex << (unsigned int) src_mac[2];
    concat_data << ":" << std::setw(2) << std::hex << (unsigned int) src_mac[3];
    concat_data << ":" << std::setw(2) << std::hex << (unsigned int) src_mac[4];
    concat_data << ":" << std::setw(2) << std::hex << (unsigned int) src_mac[5];
    
    // ssid list
    if (ssid_list.size() > 0) {
        concat_data << "\n\t" << "ssid_list:";
        for (int i = 0; i < ssid_list.size(); i++)
            concat_data << "\n\t\t[" << i << "]: " << ssid_list[i];
    }
    // hash
    concat_data << "\n\t" << "hash: " << hash;
    
    ESP_LOGD(tag, "%s", concat_data.str().c_str());
}

std::string
Packet::genHash()
{
    string hash;

    stringstream concat_data;

    // concatenate fields in alphabetical order
    concat_data << 
        (channel) << 
        (sequence_control) <<
        (src_mac[0]) << (src_mac[1]) << (src_mac[2]) << (src_mac[3]) << (src_mac[4]) << (src_mac[5]) << 
        (timestamp);
    
    for (int i = 0; i < ssid_list.size(); i++)
        concat_data << ssid_list[i];

    hash = md5(concat_data.str());

    return hash;
}

uint32_t
Packet::serialize(char* buf)
/*
    1. channel
    2. rssi
    3. timestamp
    4. sequence control
    5. number of ssid
    6. tags presence
    7. supported rates
    8. HT capabilities
    9. Extended Capabilities
    10. Interworking
    11. Multi Band
    12. VHT Capabilities
    13. device MAC address
    14. ssid list (ref: 5.)
    15. hash
*/
{
    uint32_t rval = 0; // serialized packet size

    /* uint32_t* mode */
    uint32_t* p_buf = (uint32_t*) buf;

    p_buf[0] = htonl(channel);                       // channel
    p_buf[1] = htonl(rssi);                          // RSSI
    p_buf[2] = htonl(timestamp);                     // timestamp
    p_buf[3] = htonl((uint32_t) sequence_control);   // sequence control
    p_buf[4] = htonl((uint32_t) ssid_list.size());   // number of SSIDs

    p_buf[5] = htonl(tag_presence);                      // tags presence
    p_buf[6] = htonl(((uint32_t*)&supported_rates)[0]);  // supported rates 1
    p_buf[7] = htonl(((uint32_t*)&supported_rates)[1]);  // supported rates 2

    rval += (8*sizeof(uint32_t)); 

    /* uint8_t* mode */
    uint8_t* u8p_buf = (uint8_t*) &p_buf[8];

    /* ht capabilities */
    if (is_tag_set(TAG_HT_CAPABILITY, &tag_presence)) {

        uint8_t* src = (uint8_t*) &ht_capabilities.capability_info;
        for (int i = 0; i < HT_CAPABILITIES_LEN; i++)
            *u8p_buf++ = *src++;

        rval += HT_CAPABILITIES_LEN;
    }

    /* extended capabilities */
    if (is_tag_set(TAG_EXTENDED_CAPABILITIES, &tag_presence)) {
        uint8_t* src = (uint8_t*) &extended_capabilities.extended_capabilities;
        for (int i = 0; i < EXT_CAPABILITIES_LEN; i++)
            *u8p_buf++ = *src++;

        rval += EXT_CAPABILITIES_LEN;
    }

    /* interworking */
    if (is_tag_set(TAG_INTERWORKING, &tag_presence)) {
        *u8p_buf++ = interworking;
        rval++;
    }

    /* Multi Band */
    if (is_tag_set(TAG_MULTI_BAND, &tag_presence)) {
        *u8p_buf++ = multi_band_id;
        *u8p_buf++ = multi_band_channel;
        rval += 2;
    }

    /* VHT Capabilities */
    if (is_tag_set(TAG_VHT_CAPABILITY, &tag_presence)) {
        uint8_t* src = (uint8_t*) &vht_capabilities.capabilities_info;
        for (int i = 0; i < VHT_CAPABILITIES_LEN; i++)
            *u8p_buf++ = *src++;

        rval += VHT_CAPABILITIES_LEN;
    }

    /* device mac */
    for (int i = 0; i < MAC_LENGTH; i++)
        *u8p_buf++ = src_mac[i];
    rval += MAC_LENGTH;

    /* 
        SSIDs 
            |len|"ssid-name"|
    */
    for (int i = 0; i < ssid_list.size(); i++) {
        uint8_t ssid_len = ssid_list[i].size();

        *u8p_buf++ = ssid_len;
        for (int j = 0; j < ssid_len; j++) {
            *u8p_buf++ = ssid_list[i][j];
        }            
        rval += (1+ssid_len);
    }

    /* hash */
    for (int i = 0; i < MD5_HASH_LENGTH; i++) {
        *u8p_buf++ = hash[i];
    }
    rval += MD5_HASH_LENGTH;

    ESP_LOGV(tag, "(serialize) serialized length %d (max %d)", rval, MAX_PACKET_SIZE);
    return rval;
}
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

        // jump element (header + body)
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

        add_rate(data_rate, &supported_rates);
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

        add_rate(data_rate, &supported_rates);
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

string
Packet::str()
{   
    stringstream concat_data;
    concat_data << 
        "\n\t" << "ch: " << channel << 
        "\n\t" << "rssi: " << rssi << 
        "\n\t" << "timestamp: " << timestamp <<
        "\n\t" << "sequence control: " << sequence_control;

    /*
        MAC
    */        
    concat_data << "\n\t" << "mac: " << std::setw(2) << std::hex << (unsigned int) src_mac[0];
    concat_data << ":" << std::setw(2) << std::hex << (unsigned int) src_mac[1];
    concat_data << ":" << std::setw(2) << std::hex << (unsigned int) src_mac[2];
    concat_data << ":" << std::setw(2) << std::hex << (unsigned int) src_mac[3];
    concat_data << ":" << std::setw(2) << std::hex << (unsigned int) src_mac[4];
    concat_data << ":" << std::setw(2) << std::hex << (unsigned int) src_mac[5];
    
    /* 
        fingerprint
    */
    concat_data << "\n\t" << "fingerprint: ";
    
    // Tags presence
    uint8_t* src = (uint8_t*) &tag_presence;
    for (int i = 0; i < 4; i++)
        concat_data << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) *src++;
    
    concat_data << '|';

    // Supported Rates
    src = (uint8_t*) &supported_rates;
    for (int i = 0; i < 8; i++)
        concat_data << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) *src++;
    
    concat_data << '|';

    // HT Capabilities
    src = (uint8_t*) &(ht_capabilities.capability_info);
    for (int i = 0 ; i < HT_CAPABILITIES_LEN; i++)
        concat_data << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) *src++;
    
    concat_data << '|';

    // Extended Capabilities
    src = (uint8_t*) &(extended_capabilities.extended_capabilities);
    for (int i = 0 ; i < EXT_CAPABILITIES_LEN; i++)
        concat_data << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) *src++;
    
    concat_data << '|';
    
    // Interworking
    concat_data << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) interworking;
    
    concat_data << '|';

    // Multi Band
    concat_data << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) multi_band_id;
    concat_data << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) multi_band_channel;
    
    concat_data << '|';

    // VHT Capabilities
    src = (uint8_t*) &(vht_capabilities.capabilities_info);
    for (int i = 0; i < VHT_CAPABILITIES_LEN; i++)
        concat_data << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) *src++;
    
    concat_data << '|';

    /*
        SSID List
    */
    if (ssid_list.size() > 0) {
        concat_data << "\n\t" << "ssid_list:";
        for (int i = 0; i < ssid_list.size(); i++)
            concat_data << "\n\t\t[" << i << "]: " << ssid_list[i];
    }

    /*
        HASH
    */
    concat_data << "\n\t" << "hash: " << hash;
    
    return concat_data.str();
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
    Order:
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
    /* uint32_t* mode */
    uint32_t* p_buf = (uint32_t*) buf;

    p_buf[0] = htonl(channel);                       // channel
    p_buf[1] = htonl(rssi);                          // RSSI
    p_buf[2] = htonl(timestamp);                     // timestamp
    p_buf[3] = htonl((uint32_t) sequence_control);   // sequence control
    p_buf[4] = htonl((uint32_t) ssid_list.size());   // number of SSIDs

    p_buf[5] = htonl(tag_presence);                  // tags presence

    /* uint8_t* mode */
    uint8_t* dst = (uint8_t*) &p_buf[6];
    uint8_t* src = nullptr;

    /* supported rates */
    src = (uint8_t*) & supported_rates;
    for (int i = 0; i < 8; i++)
        *dst++ = *src++;

    /* ht capabilities */
    if (is_tag_set(TAG_HT_CAPABILITY, &tag_presence)) {
        src = (uint8_t*) &ht_capabilities.capability_info;
        for (int i = 0; i < HT_CAPABILITIES_LEN; i++)
            *dst++ = *src++;
    }

    /* extended capabilities */
    if (is_tag_set(TAG_EXTENDED_CAPABILITIES, &tag_presence)) {
        src = (uint8_t*) &extended_capabilities.extended_capabilities;
        for (int i = 0; i < EXT_CAPABILITIES_LEN; i++)
            *dst++ = *src++;
    }

    /* interworking */
    if (is_tag_set(TAG_INTERWORKING, &tag_presence)) {
        *dst++ = interworking;
    }

    /* Multi Band */
    if (is_tag_set(TAG_MULTI_BAND, &tag_presence)) {
        *dst++ = multi_band_id;
        *dst++ = multi_band_channel;
    }

    /* VHT Capabilities */
    if (is_tag_set(TAG_VHT_CAPABILITY, &tag_presence)) {
        src = (uint8_t*) &vht_capabilities.capabilities_info;
        for (int i = 0; i < VHT_CAPABILITIES_LEN; i++)
            *dst++ = *src++;
    }

    /* device mac */
    for (int i = 0; i < MAC_LENGTH; i++)
        *dst++ = src_mac[i];

    /* 
        SSIDs 
            |len|"ssid-name"|
    */
    for (int i = 0; i < ssid_list.size(); i++) {
        uint8_t ssid_len = ssid_list[i].size();

        *dst++ = ssid_len;
        for (int j = 0; j < ssid_len; j++) {
            *dst++ = ssid_list[i][j];
        }            
    }

    /* hash */
    for (int i = 0; i < MD5_HASH_LENGTH; i++) {
        *dst++ = hash[i];
    }

    // serialized packet size
    uint32_t rval = (uint32_t) (dst - (uint8_t*) buf);
    ESP_LOGV(tag, "(serialize) serialized length %d (max %d)", rval, MAX_PACKET_SIZE);
    return rval;
}

Packet 
Packet::forge_test_packet()
{   
    Packet rval;
    rval.channel = 0x11223344;
    rval.rssi = -69;
    rval.timestamp = 0x11223344;
    rval.sequence_control = 0x1122;
    rval.src_mac[0] = 0xaa;
    rval.src_mac[1] = 0xbb;
    rval.src_mac[2] = 0xcc;
    rval.src_mac[3] = 0xdd;
    rval.src_mac[4] = 0xee;
    rval.src_mac[5] = 0xff;


    add_tag(TAG_SSID, &rval.tag_presence);
    add_tag(TAG_SSID_LIST, &rval.tag_presence);
    rval.ssid_list.push_back(string("ssid test 1"));
    rval.ssid_list.push_back(string("ssid test 2"));
    rval.ssid_list.push_back(string("ssid test 3"));

    add_tag(TAG_SUPP_RATES, &rval.tag_presence);
    add_tag(TAG_EXT_SUPP_RATES, &rval.tag_presence);
    rval.supported_rates = 0x1122334455667788;

    add_tag(TAG_HT_CAPABILITY, &rval.tag_presence);
    rval.ht_capabilities.capability_info = 0x1122;
    rval.ht_capabilities.ampduparam = 0x11;
    rval.ht_capabilities.mcsset[0] = 0x11;
    rval.ht_capabilities.mcsset[1] = 0x22;
    rval.ht_capabilities.mcsset[2] = 0x33;
    rval.ht_capabilities.mcsset[3] = 0x44;
    rval.ht_capabilities.mcsset[4] = 0x55;
    rval.ht_capabilities.mcsset[5] = 0x66;
    rval.ht_capabilities.mcsset[6] = 0x77;
    rval.ht_capabilities.mcsset[7] = 0x88;
    rval.ht_capabilities.mcsset[8] = 0x99;
    rval.ht_capabilities.mcsset[9] = 0x00;
    rval.ht_capabilities.mcsset[10] = 0xaa;
    rval.ht_capabilities.mcsset[11] = 0xbb;
    rval.ht_capabilities.mcsset[12] = 0xcc;
    rval.ht_capabilities.mcsset[13] = 0xdd;
    rval.ht_capabilities.mcsset[14] = 0xee;
    rval.ht_capabilities.mcsset[15] = 0xff;
    rval.ht_capabilities.extended_capabilities = 0x1122;
    rval.ht_capabilities.tbc = 0x11223344;
    rval.ht_capabilities.asel_capabilities = 0x11;

    add_tag(TAG_EXTENDED_CAPABILITIES, &rval.tag_presence);
    rval.extended_capabilities.extended_capabilities[0] = 0x11;
    rval.extended_capabilities.extended_capabilities[1] = 0x22;
    rval.extended_capabilities.extended_capabilities[2] = 0x33;
    rval.extended_capabilities.extended_capabilities[3] = 0x44;
    rval.extended_capabilities.extended_capabilities[4] = 0x55;
    rval.extended_capabilities.extended_capabilities[5] = 0x66;
    rval.extended_capabilities.extended_capabilities[6] = 0x77;
    rval.extended_capabilities.extended_capabilities[7] = 0x88;
    rval.extended_capabilities.extended_capabilities[8] = 0x99;

    add_tag(TAG_INTERWORKING, &rval.tag_presence);
    rval.interworking = 0x11;

    add_tag(TAG_MULTI_BAND, &rval.tag_presence);
    rval.multi_band_id = 0x11;
    rval.multi_band_channel = 0x11;

    add_tag(TAG_VHT_CAPABILITY, &rval.tag_presence);
    rval.vht_capabilities.capabilities_info = 0x11223344;
    rval.vht_capabilities.supported_mcs_nss_set[0] = 0x11;
    rval.vht_capabilities.supported_mcs_nss_set[1] = 0x22;
    rval.vht_capabilities.supported_mcs_nss_set[2] = 0x33;
    rval.vht_capabilities.supported_mcs_nss_set[3] = 0x44;
    rval.vht_capabilities.supported_mcs_nss_set[4] = 0x55;
    rval.vht_capabilities.supported_mcs_nss_set[5] = 0x66;
    rval.vht_capabilities.supported_mcs_nss_set[6] = 0x77;
    rval.vht_capabilities.supported_mcs_nss_set[7] = 0x88;

    rval.hash = rval.genHash();

    return rval;
}
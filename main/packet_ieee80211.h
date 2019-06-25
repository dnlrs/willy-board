/*
    ref: IEEE Std. 802.11-2016
*/
#ifndef PACKET_IEEE80211_H_INCLUDED
#define PACKET_IEEE80211_H_INCLUDED
#pragma once

#include <map>
#include <string>

#define MAC_LENGTH          6     /* MAC address length */

#define MGT_FRAME           0x00  /* Frame type is management */
#define MGT_PROBE_REQ       0x04  /* Probe request              */

/* 
    ProbeRequest Logical field codes (IEEE 802.11 encoding of tags)                 
    Proper order ($9.3.3.10)
*/
#define TAG_SSID                          0 /* SSID ($9.4.2.2) */
#define TAG_SUPP_RATES                    1 /* Supported Rates ($9.4.2.3) */
#define TAG_REQUEST                      10 /* Request information ($9.4.2.10) */
#define TAG_EXT_SUPP_RATES               50 /* Extended Supported Rates ($9.4.2.13) */
#define TAG_DS_PARAMETER                  3 /* DSSS Parameter Set ($9.4.2.4) */
#define TAG_SUPPORTED_OPERATING_CLASSES  59 /* Supported Operating Classes ($9.4.2.54) IEEE Std 802.11w-2009 */
#define TAG_HT_CAPABILITY                45 /* HT Capabilities ($9.4.2.56) IEEE Stc 802.11n/D2.0 */
#define TAG_20_40_BSS_CO_EX              72 /* 20/40 BSS Coexistence ($9.4.2.60) IEEE P802.11n/D6.0 */
#define TAG_EXTENDED_CAPABILITIES       127 /* Extended Capabilities ($9.4.2.27) IEEE Stc 802.11n/D1.10/D2.0 */
#define TAG_SSID_LIST                    84 /* SSID List ($9.4.2.73) */
#define TAG_CHANNEL_USAGE                97 /* Channel Usage ($9.4.2.86) */
#define TAG_INTERWORKING                107 /* Interworking ($9.4.2.92) IEEE Std 802.11u-2011 */
#define TAG_MESH_ID                     114 /* Mesh ID ($9.4.2.99) IEEE Std 802.11s-2011 */
#define TAG_MULTI_BAND                  158 /* Multi band ($9.4.2.138) IEEE Std 802.11ad */
#define TAG_DMG_CAPABILITIES            148 /* DMG Capabilities ($9.4.2.128) IEEE Std 802.11ad */
#define TAG_MULTIPLE_MAC_SUBLAYERS      170 /* Multiple Mac Sublayer ($9.4.2.153) IEEE Std 802.11ad */
#define TAG_VHT_CAPABILITY              191 /* VHT Capabilities ($9.4.2.158) C IEEE Std 802.11ac/D3.1 */
#define ETAG_ESTIMATED_SERVICE_PARAM     11 /* Estimated Service Parameters ($9.4.2.174) */
#define TAG_VENDOR_SPECIFIC_IE          221 /* Vendor Specific ($9.4.2.26) */


extern std::map<int, std::string> tags_description; // Tag Description
extern std::map<int, uint32_t>    tags_compacted;   // Assign to each element a bit/flag.
extern std::map<int, std::string> ieee80211_supported_rates_vals; // Supported Rates
extern std::map<int, uint64_t>    sr_compacted;     // Assign to each data rate a bit/flag.

/* manage compacted tags */
bool add_tag(int element_id, uint32_t* tag_presence);
bool is_tag_set(int element_id, uint32_t* tag_presence);

/* manage supported rates */
bool add_rate(int rate, uint64_t* supported_rates);


/*
    Frame Control ($9.2.4.1)
    Note: if htc_order is 1, ht_control field is present
*/
typedef struct {
    uint16_t version : 2;
    uint16_t type : 2;
    uint16_t sub_type : 4;
    uint16_t ds : 2;
    uint16_t frag : 1;
    uint16_t retry : 1;
    uint16_t pwrmgt : 1;
    uint16_t moredata : 1;
    uint16_t protected_frame : 1;
    uint16_t htc_order : 1;       
} wlan_fc_t;

/*
    Sequence Control ($9.2.4.4)
*/
typedef struct {
    uint16_t fragment_number : 4;
    uint16_t sequence_number : 12;
} wlan_seq_t;

/*
    Management Frame ($9.3.3.2)
*/
#define MGMT_FRAME_HEADER_LEN (2+2+(3*MAC_LENGTH)+2) // Length of Management frame-headers

typedef struct {
    /* header */
    wlan_fc_t  frame_control;
    uint16_t   duration;
    uint8_t    dst_mac[MAC_LENGTH];
    uint8_t    src_mac[MAC_LENGTH];
    uint8_t    bssid_mac[MAC_LENGTH];
    uint16_t   sequence_control; // wlan_seq_t
    uint8_t    ht_control[0]; // 0 or 4 octets
    /* body */
    uint8_t    frame_body[0]; // variable
    uint32_t   fcs;
} wlan_t;


/* 
    Element header ($9.4.2.1) 
    Common to all elements.
*/
typedef struct {
    uint8_t id;
    uint8_t length;
} element_hdr;

/*  
    SSID ($9.4.2.2) 
*/
#define MAX_SSID_LEN 32

typedef struct {
    element_hdr hdr;
    uint8_t ssid[MAX_SSID_LEN];
} wlan_ssid_t;

/*  
    Supported Rates ($9.4.2.3)
*/
#define MAX_SUPPORTED_RATES_LEN 8

typedef struct {
    element_hdr hdr;
    uint8_t     supported_rates[MAX_SUPPORTED_RATES_LEN]; // Mbps
} wlan_supported_rates_t;

/*  
    Extended Supported Rates ($9.4.2.13) 
*/
typedef struct {
    element_hdr hdr;
    uint8_t     extended_sup_rates[0]; // length: [1-255]
} wlan_extended_sup_rates_t;

/* 
    HT Capabilities ($9.4.2.56) 
*/
#define HT_MCSSET_LEN           16
#define HT_CAPABILITIES_LEN    (2 + 1 + HT_MCSSET_LEN + 2 + 4 + 1) // element header excluded

typedef struct {
    element_hdr hdr;
    uint16_t    capability_info;             // ($9.4.2.56.2)
    uint8_t     ampduparam;
    uint8_t     mcsset[HT_MCSSET_LEN];
    uint16_t    extended_capabilities;
    uint32_t    tbc;                         // Transmit Beamforming Capabilities
    uint8_t     asel_capabilities;
} wlan_ht_t;


/* 
    Extended Capabilities ($9.4.2.27) 
*/
#define EXT_CAPABILITIES_LEN     9 // because 9*8 = 72, almost all of them

typedef struct {
    element_hdr hdr;
    uint8_t     extended_capabilities[EXT_CAPABILITIES_LEN];
} wlan_ext_cap_t;


/* 
    SSID List ($9.4.2.73)
*/
typedef struct {
    element_hdr  hdr;
    wlan_ssid_t  ssid_list[0]; // variable
} wlan_ssid_list_t;

/* 
    Interworking ($9.4.2.92) 
*/
typedef struct {
    element_hdr hdr;
    uint8_t     access_network_options;
    uint8_t     venute_info[0]; // optional
    uint8_t     hessid[0];      // optional
} wlan_interworking_t;

/*
    Multi band ($9.4.2.138) 
*/
typedef struct {
    element_hdr hdr;
    uint8_t multiband_control;
    uint8_t band_id;
    uint8_t operating_class;
    uint8_t channel_number;
    uint8_t bssid[6];
    uint16_t beacon_interval;
    uint8_t tsf_offset[8];
    uint8_t mb_connection_capability;
    uint8_t fst_session_timeout;
    uint8_t sta_mac_address[0];
    uint8_t cipher_suite_count[0];
    uint8_t cipher_suite_list[0]; 
} wlan_multi_band_t;

/*
    VHT Capabilities ($9.4.2.158)
*/
#define VHT_MCS_NSS_SET_LEN     8
#define VHT_CAPABILITIES_LEN   (4+VHT_MCS_NSS_SET_LEN) // element header excluded

typedef struct {
    element_hdr hdr;
    uint32_t capabilities_info;
    uint8_t  supported_mcs_nss_set[VHT_MCS_NSS_SET_LEN];
} wlan_vht_cap_t;




/*
    Not Used
*/
/*  Request Information */
typedef struct {
    element_hdr hdr;
    uint8_t     requested_ids[0]; // variable length
} wlan_request_info_t;

/* DSSS Parameter Set */
typedef struct {
    element_hdr hdr;
    uint8_t     current_channel;
} wlan_ds_t;

/* Supported Operating Classes */
typedef struct {
    element_hdr hdr;
    uint8_t     current_op_class;
    uint8_t     operating_classes[0];
    uint8_t     current_op_class_ext_seq[0]; // Current Operating Cass Extension Sequence
    uint8_t     op_class_duple_seq[0];       // Operating class Duple Sequence
} wlan_supopeclass_t;

/* Channel usage */
typedef struct {
    element_hdr hdr;
    uint8_t     usage_mode;
    uint16_t    channel_entry;
} wlan_ch_usage_t;



#endif // !PACKET_IEEE80211_H_INCLUDED
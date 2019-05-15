#include "packet_ieee80211.h"

#include <cinttypes>
#include <map>
#include <string>

/* Tag Description */
std::map<int, std::string> tags_description = {
    {TAG_SSID,                                 "SSID parameter set"},
    {TAG_SUPP_RATES,                           "Supported Rates"},
    {TAG_DS_PARAMETER,                         "DS Parameter set"},
    {TAG_REQUEST,                              "Request"},
    {TAG_HT_CAPABILITY,                        "HT Capabilities (802.11n D1.10)"},
    {TAG_EXT_SUPP_RATES,                       "Extended Supported Rates"},
    {TAG_SUPPORTED_OPERATING_CLASSES,          "Supported Operating Classes"},
    {TAG_20_40_BSS_CO_EX,                      "20/40 BSS Coexistence"},
    {TAG_SSID_LIST,                            "SSID List"},
    {TAG_CHANNEL_USAGE,                        "Channel Usage"},
    {TAG_INTERWORKING,                         "Interworking"},
    {TAG_MESH_ID,                              "Mesh ID"},
    {TAG_EXTENDED_CAPABILITIES,                "Extended Capabilities"},
    {TAG_DMG_CAPABILITIES,                     "DMG Capabilities"},
    {TAG_MULTIPLE_MAC_SUBLAYERS,               "Multiple MAC Sublayers"},
    {TAG_VHT_CAPABILITY,                       "VHT Capabilities"},
    {TAG_VENDOR_SPECIFIC_IE,                   "Vendor Specific"}
};

/* Assign to each element a bit/flag. */
std::map<int, uint32_t> tags_compacted = {
    {TAG_SSID,                          0x80000000},
    {TAG_SUPP_RATES,                    0x40000000},
    {TAG_REQUEST,                       0x20000000},
    {TAG_EXT_SUPP_RATES,                0x10000000},
    {TAG_DS_PARAMETER,                  0x08000000},
    {TAG_SUPPORTED_OPERATING_CLASSES,   0x04000000},
    {TAG_HT_CAPABILITY,                 0x02000000},
    {TAG_20_40_BSS_CO_EX,               0x01000000},
    {TAG_EXTENDED_CAPABILITIES,         0x00800000},
    {TAG_SSID_LIST,                     0x00400000},
    {TAG_CHANNEL_USAGE,                 0x00200000},
    {TAG_INTERWORKING,                  0x00100000},
    {TAG_MESH_ID,                       0x00080000},
    {TAG_MULTI_BAND,                    0x00040000},
    {TAG_DMG_CAPABILITIES,              0x00020000},
    {TAG_MULTIPLE_MAC_SUBLAYERS,        0x00010000},
    {TAG_VHT_CAPABILITY,                0x00008000},
    {ETAG_ESTIMATED_SERVICE_PARAM,      0x00004000},
    {TAG_VENDOR_SPECIFIC_IE,            0x00002000},
};


/* Supported Rates */
std::map<int, std::string> ieee80211_supported_rates_vals = {
    { 0x02, "1" },
    { 0x03, "1.5" },
    { 0x04, "2" },
    { 0x05, "2.5" },
    { 0x06, "3" },
    { 0x09, "4.5" },
    { 0x0B, "5.5" },
    { 0x0C, "6" },
    { 0x12, "9" },
    { 0x16, "11" },
    { 0x18, "12" },
    { 0x1B, "13.5" },
    { 0x24, "18" },
    { 0x2C, "22" },
    { 0x30, "24" },
    { 0x36, "27" },
    { 0x42, "33" },
    { 0x48, "36" },
    { 0x60, "48" },
    { 0x6C, "54" },
    { 0x82, "1(B)" },
    { 0x83, "1.5(B)" },
    { 0x84, "2(B)" },
    { 0x85, "2.5(B)" },
    { 0x86, "3(B)" },
    { 0x89, "4.5(B)" },
    { 0x8B, "5.5(B)" },
    { 0x8C, "6(B)" },
    { 0x92, "9(B)" },
    { 0x96, "11(B)" },
    { 0x98, "12(B)" },
    { 0x9B, "13.5(B)" },
    { 0xA4, "18(B)" },
    { 0xAC, "22(B)" },
    { 0xB0, "24(B)" },
    { 0xB6, "27(B)" },
    { 0xC2, "33(B)" },
    { 0xC8, "36(B)" },
    { 0xE0, "48(B)" },
    { 0xEC, "54(B)" },
    { 0xFF, "BSS requires support for mandatory features of HT PHY (IEEE 802.11 - Clause 20)" },
    { 0,    "NULL"}
};

/* Assign to each data rate a bit/flag. */
std::map<int, uint64_t> sr_compacted = {
    { 0x02, 0x8000000000000000 },
    { 0x03, 0x4000000000000000 },
    { 0x04, 0x2000000000000000 },
    { 0x05, 0x1000000000000000 },
    { 0x06, 0x0800000000000000 },
    { 0x09, 0x0400000000000000 },
    { 0x0B, 0x0200000000000000 },
    { 0x0C, 0x0100000000000000 },
    { 0x12, 0x0080000000000000 },
    { 0x16, 0x0040000000000000 },
    { 0x18, 0x0020000000000000 },
    { 0x1B, 0x0010000000000000 },
    { 0x24, 0x0008000000000000 },
    { 0x2C, 0x0004000000000000 },
    { 0x30, 0x0002000000000000 },
    { 0x36, 0x0001000000000000 },
    { 0x42, 0x0000800000000000 },
    { 0x48, 0x0000400000000000 },
    { 0x60, 0x0000200000000000 },
    { 0x6C, 0x0000100000000000 },
    { 0x82, 0x0000080000000000 },
    { 0x83, 0x0000040000000000 },
    { 0x84, 0x0000020000000000 },
    { 0x85, 0x0000010000000000 },
    { 0x86, 0x0000008000000000 },
    { 0x89, 0x0000004000000000 },
    { 0x8B, 0x0000002000000000 },
    { 0x8C, 0x0000001000000000 },
    { 0x92, 0x0000000800000000 },
    { 0x96, 0x0000000400000000 },
    { 0x98, 0x0000000200000000 },
    { 0x9B, 0x0000000100000000 },
    { 0xA4, 0x0000000080000000 },
    { 0xAC, 0x0000000040000000 },
    { 0xB0, 0x0000000020000000 },
    { 0xB6, 0x0000000010000000 },
    { 0xC2, 0x0000000008000000 },
    { 0xC8, 0x0000000004000000 },
    { 0xE0, 0x0000000002000000 },
    { 0xEC, 0x0000000001000000 },
    { 0xFF, 0xffffffffffffffff },
    { 0,    0x0000000000000000 }
};


bool
add_tag(int element_id, uint32_t* tag_presence)
{
    if (tags_compacted.find(element_id) == tags_compacted.end()) {
        return false;
    }

    *tag_presence &= tags_compacted.at(element_id);
    return true;
}

bool
is_tag_set(int element_id, uint32_t* tag_presence)
{
    if (tags_compacted.find(element_id) == tags_compacted.end()) {
        return false;
    }

    return ((*tag_presence & tags_compacted.at(element_id)) != 0);
}









int deserialize_ht_capabilities(wlan_ht_t* data, uint8_t* src)
{   
    uint8_t* dst = (uint8_t*) &data->capability_info;

    for (int i = 0; i < HT_CAPABILITIES_LEN; i++) {
        *dst++ = *src++;
    }

    return HT_CAPABILITIES_LEN;
}

int deserialize_extended_capabilities(wlan_ext_cap_t* data, uint8_t* src)
{
    uint8_t* dst = (uint8_t*) &data->extended_capabilities;

    for(int i = 0; i < EXT_CAPABILITIES_LEN; i++) {
        *dst++ = *src++;
    }

    return EXT_CAPABILITIES_LEN;
}

int deserialize_vht_capabilities(wlan_vht_cap_t* data, uint8_t* src)
{
    uint8_t* dst = (uint8_t*) &data->capabilities_info;

    for (int i = 0; i < VHT_CAPABILITIES_LEN; i++) {
        *dst++ = *src++;
    }

    return VHT_CAPABILITIES_LEN;
}

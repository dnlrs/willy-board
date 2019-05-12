
#ifndef TIMELINE_H_INCLUDED
#define TIMELINE_H_INCLUDED
#pragma once

#include "sdkconfig.h"

// MUST be defined before include the sntp app
#if CONFIG_SNTP_SYNC_INTERVAL >= 15000
#define SNTP_UPDATE_DELAY CONFIG_SNTP_SYNC_INTERVAL
#endif

#include "apps/sntp/sntp.h"

#include "esp_log.h"

#include <cstring>
#include <ctime>

// timestamp is valid if it's greater than 2019-01-01
#define TIMESTAMP_VALIDATION 1546300800
#define TIMESTAMP_STR_MIN_LEN 64

// After initializing SNTP check if it is working using these params
#define TIMELINE_MAX_ATTEMPTS 10       // when checking if sntp started
#define TIMELINE_ATTEMPT_INTERVAL 1000 // ms

#define IP_MAX_LEN 16 // 255.255.255.255'\0'
#define IP_MIN_LEN 8  // 0.0.0.0'\0'

class Timeline {
    static char server_ip[IP_MAX_LEN];

    static constexpr char const* tag = "wwb-Timeline";

    Timeline(){};

public:
    static bool initialize_sntp(char* ip_server, int ip_server_size);
    static void stop_sntp();

    /*
        Checks if time_val is valid.
        If time_val is 0 checks if `time()` is valid.
        Note: A timestamp is valid if > TIMESTAMP_VALIDATION.
    */
    static bool is_valid(time_t time_val);

    /* Converts the time_val to string */
    static bool get_time_str(time_t time_val, char* buf, int buf_size);
};

#endif // !TIMELINE_H_INCLUDED
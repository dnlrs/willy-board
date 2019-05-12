#include "Timeline.h"

// definition for your static member variable
char Timeline::server_ip[IP_MAX_LEN];

bool
Timeline::initialize_sntp(char* ip_server, int ip_server_size)
{
    if (ip_server == nullptr) {
        ESP_LOGE(tag, "(initialize_sntp) invalid server address.");
        return false;
    }
    if (ip_server_size < IP_MIN_LEN || ip_server_size >= IP_MAX_LEN) {
        ESP_LOGE(tag, "(initialize_sntp) invalid server address size.");
        return false;
    }

    stop_sntp();

    // save internally server address
    strncpy(server_ip, ip_server, ip_server_size+1);
    server_ip[ip_server_size] = '\0';

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, Timeline::server_ip);
    sntp_init();

    int max_attempts = TIMELINE_MAX_ATTEMPTS;
    while (max_attempts > 0) {
        if (is_valid(0)) {
            ESP_LOGI(tag, "(initialize_sntp) sntp initialized and working.");
            return true;
        }
        max_attempts--;
        vTaskDelay(TIMELINE_ATTEMPT_INTERVAL / portTICK_PERIOD_MS);
    }

    ESP_LOGE(tag, "(initialize_sntp) sntp initialized but not working.");
    return false;
}

void
Timeline::stop_sntp()
{
    if (sntp_enabled()) {
        ESP_LOGD(tag, "(initialize_sntp) disabling sntp");
        sntp_stop();
    }
}

bool
Timeline::is_valid(time_t time_val)
{
    time_t check_time;

    if (time_val > 0)
        check_time = time_val;
    else
        time(&check_time);

    if (check_time > TIMESTAMP_VALIDATION)
        return true;

    return false;
}

bool
Timeline::get_time_str(time_t time_val, char* buf, int buf_size)
{
    if (buf == nullptr || buf_size < TIMESTAMP_STR_MIN_LEN) {
        ESP_LOGE(tag, "(get_time_str) invalid buffer or buffer size.");
        return false;
    }

    time_t ttime;
    if (time_val > 0)
        ttime = time_val;
    else
        time(&ttime);

    if (!is_valid(ttime)) {
        ESP_LOGI(tag, "(get_time_str) invalid time.");
        strncpy(buf, "invalid time", 13);
        return false;
    }

    struct tm timeinfo;
    timeinfo.tm_sec   = 0;
    timeinfo.tm_hour  = 0;
    timeinfo.tm_mday  = 0;
    timeinfo.tm_mon   = 0;
    timeinfo.tm_year  = 0;
    timeinfo.tm_wday  = 0;
    timeinfo.tm_yday  = 0;
    timeinfo.tm_isdst = 0;

    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // Rome
    tzset();
    localtime_r(&ttime, &timeinfo);
    int rval = strftime(buf, buf_size, "%c", &timeinfo);
    if (rval <= 0) {
        ESP_LOGE(tag, "(get_time_str) strftime failed.");
        return false;
    }

    return true;
}

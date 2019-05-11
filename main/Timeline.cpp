#include "Timeline.h"
#include "esp_log.h"

void
Timeline::sync_time(void)
{
    lock_guard<mutex> lg(m);

    timeinfo          = {};
    timeinfo.tm_hour  = 0;
    timeinfo.tm_mday  = 0;
    timeinfo.tm_mon   = 0;
    timeinfo.tm_year  = 0;
    timeinfo.tm_wday  = 0;
    timeinfo.tm_yday  = 0;
    timeinfo.tm_isdst = 0;

    int retry = 0;

    const int retry_count = 10;
    while (timeinfo.tm_year < (2016 - 1900) && (++retry < retry_count)) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&central_clock);
        start_offset = clock();
        localtime_r(&central_clock, &timeinfo);
    }
}

void
Timeline::initialize_sntp(char* ip_addr)
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, ip_addr);
    sntp_init();
}

void
Timeline::print_time()
{
    char strftime_buf[64];

    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // Rome
    tzset();
    {
        lock_guard<mutex> lg(m);
        localtime_r(&central_clock, &timeinfo);
    }
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(tag, "ROME/ITALY: %s", strftime_buf);
}

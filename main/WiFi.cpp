#include "WiFi.h"

#include <cstring>

WiFi::WiFi()
{
    init();
}

WiFi::~WiFi()
{
    deinit();
}

void
WiFi::init()
{
    // initialize the tcp stack
    tcpip_adapter_init();

    // initialize the wifi event handler
    ESP_ERROR_CHECK(esp_event_loop_init(WiFi::event_handler, NULL));

    // configure, initialize and start the wifi driver
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    wifi_config.sta           = {};
    strncpy((char*)wifi_config.sta.ssid, CONFIG_WIFI_SSID,
            sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, CONFIG_WIFI_PSW,
            sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGD(tag, "(init) wifi stack initialization finished.");
    ESP_LOGI(tag, "(init) connecting to AP ssid: '%s' password: '%s'",
             CONFIG_WIFI_SSID, CONFIG_WIFI_PSW);
}

void
WiFi::deinit()
{
    disconnect();
    // set as 'false' the promiscuous mode
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    // it stop soft-AP and free soft-AP control block
    ESP_ERROR_CHECK(esp_wifi_stop());
    // free all resource allocated in esp_wifi_init() and stop WiFi task
    ESP_ERROR_CHECK(esp_wifi_deinit());
}

void
WiFi::reset()
{
    deinit();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    init();
}

void
WiFi::connect()
{
    ESP_LOGD(tag, "(connect) connecting...");
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void
WiFi::disconnect()
{
    ESP_LOGD(tag, "(disconnect) disconnecting...");
    ESP_ERROR_CHECK(esp_wifi_disconnect());
}

esp_err_t
WiFi::event_handler(void* ctx, system_event_t* event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        esp_wifi_get_mac(ESP_IF_WIFI_STA, esp_mac);
        xEventGroupSetBits(sync_group, wifi_connected_bit);
        ESP_LOGI(tag,
                 "(event_handler) esp mac address is: "
                 "%02x:%02x:%02x:%02x:%02x:%02x",
                 esp_mac[0], esp_mac[1], esp_mac[2], 
                 esp_mac[3], esp_mac[4], esp_mac[5]);
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        xEventGroupClearBits(sync_group, wifi_connected_bit);
        esp_wifi_connect();
        break;

    default:
        break;
    }
    return ESP_OK;
}

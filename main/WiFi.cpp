#include "WiFi.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include <cstring>

WiFi::WiFi()
{
    wifi_connected = false;

    // initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // initialize the tcp stack
    tcpip_adapter_init();

    // initialize the wifi event handler
    ESP_ERROR_CHECK(esp_event_loop_init(WiFi::event_handler, NULL));

    // configure, initialize and start the wifi driver
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    wifi_config.sta           = {};
    strcpy((char*)wifi_config.sta.ssid, CONFIG_WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, CONFIG_WIFI_PSW);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGD(tag, "(wifi::ctor) wifi_init_sta finished.");
    ESP_LOGI(tag, "Connect to AP. ssid: '%s' password: '%s'", 
             CONFIG_WIFI_SSID, CONFIG_WIFI_PSW);
}

void
WiFi::enable()
{
    ESP_LOGD(tag, "wifi::enable called.");
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void
WiFi::disable()
{
    ESP_LOGD(tag, "wifi::disable called.");
    ESP_ERROR_CHECK(esp_wifi_disconnect());
}

esp_err_t
WiFi::event_handler(void* ctx, system_event_t* event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGD(tag, "STA_START event");
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        if (wifi_handler != nullptr) {
            wifi_handler->set_wifi_connected(true);
        }
        ESP_LOGD(tag, "STA_GOT_IP event");
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        if (wifi_handler != nullptr) {
            wifi_handler->set_wifi_connected(false);
        }
        ESP_LOGD(tag, "STA_DISCONNECTED event");
        esp_wifi_connect();
        break;

    default:
        break;
    }
    return ESP_OK;
}

WiFi::~WiFi()
{
    esp_wifi_disconnect();
    ESP_ERROR_CHECK(esp_wifi_stop());
}

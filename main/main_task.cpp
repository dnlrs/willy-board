#include "sdkconfig.h"

#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "NetWrap.h"
#include "Sniffer.h"
#include "WiFi.h"

/*
    Defines the frequency with which the main
    task checks if a request for wifi reboot or
    device reboot was made from any component
*/
#define MAIN_TASK_INTERVAL 10000 // ms

extern "C" {
void app_main(void);
}

// task synchronization
EventGroupHandle_t sync_group;
const int wifi_connected_bit   = BIT0;
const int server_connected_bit = BIT1;
const int wifi_reset_bit       = BIT5;
const int reboot_bit           = BIT6;

// ESP32 MAC address
uint8_t esp_mac[6];

// global logging tag
static constexpr char const* tag = "wwb";

void main_task(WiFi& wifi_ref);

void
app_main(void)
/*
    1. initialize NVS
    2. initialize sync group
    3. init wifi and wait for wifi connection
    4. init net wrapper (and connection to server)
    5. start main task
*/
{
    ESP_LOGI(tag, "Probe request watchdog starting...");

    esp_err_t rval = nvs_flash_init();
    if (rval == ESP_ERR_NVS_NO_FREE_PAGES ||
        rval == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        rval = nvs_flash_init();
    }
    ESP_ERROR_CHECK(rval);

    sync_group = xEventGroupCreate();
    if (sync_group == nullptr) {
        ESP_LOGE(tag, "(app_main) group creation failed.");
        esp_restart();
    }

    WiFi wifi_handler;

    xEventGroupWaitBits(sync_group, wifi_connected_bit, false, true,
                        portMAX_DELAY);

    NetWrap netw_handler;

    Sniffer snif_handler(netw_handler);

    main_task(wifi_handler); // infinite loop inside
}

void
main_task(WiFi& wifi_ref)
{
    TickType_t wifi_sleep = 2000 / portTICK_PERIOD_MS;
    TickType_t main_sleep = MAIN_TASK_INTERVAL / portTICK_PERIOD_MS;

    while (true) {
        EventBits_t rbits = xEventGroupGetBits(sync_group);

        // manage reboot request
        if ((rbits & reboot_bit) == reboot_bit) {
            for (int i = 3; i >= 0; i--) {
                ESP_LOGW(tag, "(main_task) Rebooting in %d...", i);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }

            ESP_LOGW(tag, "(main_task) rebooting... ");
            fflush(stdout);

            esp_restart();
        }

        // manage wifi restart request
        if ((rbits & wifi_reset_bit) == wifi_reset_bit) {
            wifi_ref.disconnect();

            vTaskDelay(wifi_sleep);
            
            rbits = xEventGroupGetBits(sync_group);
            if ((rbits & wifi_connected_bit) != wifi_connected_bit)
                wifi_ref.reset();
            
            xEventGroupClearBits(sync_group, wifi_reset_bit);
        }

        vTaskDelay(main_sleep);
    }
}
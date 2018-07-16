#include "WiFi.h"

WiFi::WiFi(const string& ssid,const string& passphrase){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(WiFi::event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {};
    wifi_config.sta = {};
    strcpy((char *)wifi_config.sta.ssid,ssid.c_str());
    strcpy((char *)wifi_config.sta.password,passphrase.c_str());

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    cout<<"wifi_init_sta finished."<<endl;
    cout<<"connect to ap SSID: "<< ssid <<" PASSWORD: "<< passphrase << endl;
}

void
WiFi::connect(){
	std::cout<<"wifi connect called."<<std::endl;
    ESP_ERROR_CHECK( esp_wifi_connect() );
}


void
WiFi::disconnect(){
	std::cout<<"wifi disconnect called."<<std::endl;
    
    ESP_ERROR_CHECK( esp_wifi_disconnect() );
}


esp_err_t 
WiFi::event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) 
    {
    
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:

        cout<<"got ip"<<endl;
        wifi_handler->signal_connection(); //advertise other threads that wifi is on
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        break;
    default:
        break;
    }
    return ESP_OK;
}


WiFi::~WiFi(){
    esp_wifi_disconnect();
	ESP_ERROR_CHECK( esp_wifi_stop() );
}




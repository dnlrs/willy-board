/*
 * WIFI handler
 */
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include "WiFi.h"

WiFi::WiFi(const std::string& ssid,const std::string& passphrase){
    
    /* USER DATA */
    this->ssid = ssid;
    this->passphrase = passphrase;

    /* CONFIGURATION STRUCT */
    this->init_cfg = WIFI_INIT_CONFIG_DEFAULT();
	
    this->sec_cfg = {};
	const char *ssid_var = ssid.c_str();
	const char *pass_var = passphrase.c_str();
	strcpy((char *)this->sec_cfg.sta.ssid,ssid_var);
	strcpy((char *)this->sec_cfg.sta.password,pass_var);

    /* COUNTRY DEPENDENT PARAMETERS */
    this->wifi_country = {};
    strcpy(this->wifi_country.cc,"CN");
    this->wifi_country.schan=1;
    this->wifi_country.nchan=13;
    this->wifi_country.policy=WIFI_COUNTRY_POLICY_AUTO;

}

void
WiFi::init(){
	std::cout<<"wifi init called."<<std::endl;
	nvs_flash_init();
    tcpip_adapter_init();

    ESP_ERROR_CHECK( esp_wifi_init(&(this->init_cfg) ));


	ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); // set country for channel range [1, 13]
	
	/* STORAGE SETTING */
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) ); //STATION MODE
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &(this->sec_cfg) )); //set AP configuration
    ESP_ERROR_CHECK( esp_wifi_start() ); //START
}

void
WiFi::connect(){
	std::cout<<"wifi connect called."<<std::endl;
    //ESP_ERROR_CHECK( esp_wifi_disconnect() ); //CHECK IF NEEDED
    ESP_ERROR_CHECK( esp_wifi_connect() );
}

void
WiFi::disconnect(){
	std::cout<<"wifi disconnect called."<<std::endl;
    
    ESP_ERROR_CHECK( esp_wifi_disconnect() );
}


WiFi::~WiFi(){
	ESP_ERROR_CHECK( esp_wifi_stop() );
}




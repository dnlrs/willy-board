#include "Sniffer.h"
#include "Timeline.h"

using namespace std;

Timeline *system_timeline = NULL;


Sniffer::Sniffer(): sniffer_handle(NULL), sender_handle(NULL) {
	system_timeline = new Timeline();

	TaskHandle_t sender_handle;
	xTaskCreatePinnedToCore(Sniffer::sender_task,"sender", STACK_SIZE, NULL, 2, &sender_handle, 1);
	if(sender_handle == NULL){
		cout<<"error creating sender task"<<endl;
	}

	esp_wifi_set_promiscuous_rx_cb(&Sniffer::incoming_packet_cb);
	xTaskCreatePinnedToCore(Sniffer::sniffer_task, "sniffer", STACK_SIZE, NULL, 2, &sniffer_handle, 0);
	if(sniffer_handle == NULL){
		cout<<"error creating sniffer task"<<endl;
	}

}

Sniffer::~Sniffer(){ 
	vTaskDelete(sniffer_handle);
	sniffer_handle = NULL;
	vTaskDelete(sender_handle);
	sender_handle = NULL;
}

void
Sniffer::sniffer_task(void *pvParameters){
    uint8_t channel = 0;

    cout<<"i'm sniffer task at core: "<<xPortGetCoreID()<<endl;
    
	for(;;){

		ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

		Timeline duration;

		while(true){
			esp_task_wdt_reset();
			vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
			esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
			channel = (channel % WIFI_CHANNEL_MAX) + 1;
		}
		ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    }
}

void 
Sniffer::sender_task(void *pvParameters){
    cout<<"i'm sender task at core: "<< xPortGetCoreID() <<endl;

    NetWrap net_handler(DEFAULT_SERVER_IP, DEFAULT_SERVER_PORT);
    net_handler.connect();

    
    system_timeline->initialize_sntp();
    system_timeline->sync_time();
    system_timeline->print_time();

    gpio_pad_select_gpio((gpio_num_t)BLINK_GPIO);
    gpio_set_direction((gpio_num_t)BLINK_GPIO, GPIO_MODE_OUTPUT);

    for(;;){
    	
    	Packet p;
    	packet_queue->pop(p);

    	p.print();

       	if(net_handler.send(p)){
	        gpio_set_level((gpio_num_t)BLINK_GPIO, 1);
	        vTaskDelay(10 / portTICK_RATE_MS);
	        gpio_set_level((gpio_num_t)BLINK_GPIO, 0);
	        vTaskDelay(10 / portTICK_RATE_MS);
	    }


        vTaskDelay(2000 / portTICK_RATE_MS);
    }
}


void
Sniffer::incoming_packet_cb(void* buff, wifi_promiscuous_pkt_type_t type){

	if (type != WIFI_PKT_MGMT)
		return;

	if(!Packet::isProbeReq(buff))
		return;

	//it is a probe request!
	Packet p;
	if(p.fetchData(buff, NULL) == 0) 
	{
		//p.print();
		if(packet_queue->timed_push(p, 5))
			cout<<"pushed"<<endl;
		else
			cout<<"full queue timeout"<<endl;
	}
}
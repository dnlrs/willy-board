#include "Sniffer.h"
#include "Timeline.h"
#include "WiFi.h"

using namespace std;

Timeline *system_timeline = NULL;

//wifi stack task is pinned to core 1

void
Sniffer::start(){
	system_timeline = new Timeline(); //initialize the system timeline global object (global to all sources)
	
	//setting up GPIO to blink led when packet is sent to central server
	gpio_pad_select_gpio((gpio_num_t)BLINK_GPIO); 
    gpio_set_direction((gpio_num_t)BLINK_GPIO, GPIO_MODE_OUTPUT);

    //STARTING SENDER TASK: the aim of this task is to get packets from the queue and send them to the central server
	TaskHandle_t sender_handle;
	xTaskCreatePinnedToCore(Sniffer::sender_task,"sender", STACK_SIZE, (void *) &server_addr, 2, &sender_handle, 1);
	if(sender_handle == NULL)
		ESP_LOGI("sniffer_constructor","unable to create SENDER task");

	esp_wifi_set_promiscuous_rx_cb(&Sniffer::incoming_packet_cb); //registering the callback to invoke on packet sniffed event
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true)); //this instruction makes the board able to sniff packets

	Sniffer::sniffer_task(NULL); //main_task (at core 0) now becomes the sniffer_task (specified below)
}

Sniffer::~Sniffer(){
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
}

void
Sniffer::sniffer_task(void *pvParameters){
	// this task is basically useless for the moment.
	// it can become useful to limit the sniffing period to 1 min followed by a pause state of 30 sec 

    const char *TAG = "SNIFFER";
    uint8_t channel = 0;

    ESP_LOGI(TAG, "at core %d", xPortGetCoreID());

	for(;;){
    	ESP_LOGI(TAG,"using channel %d",channel);
		vTaskDelay(60*1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void 
Sniffer::sender_task(void *pvParameters){
	const char *TAG = "SENDER";
    ESP_LOGI(TAG, "at core %d", xPortGetCoreID()); 

    server_info_t *server_addr = (server_info_t *)pvParameters;

    NetWrap net_handler((server_addr->ip).c_str(), server_addr->port); //initialize socket for server connection
    bool server_connection_alive = net_handler.connect(); //connect to the server
    bool wifi_connection_alive = true; //at this point we cannot detect wifi problems. 
    //wifi problems can be detected only when trying many times to connect to the server without success
    //or by handling errno "host unreachable"

    if(net_handler.wait_start())
    	ESP_LOGI(TAG, "start received");
    else
    	ESP_LOGI(TAG, "error -> start NOT received");

    //CLOCK SYNC initialization
    system_timeline->initialize_sntp(const_cast<char *>((server_addr->ip).c_str())); 
    if(wifi_connection_alive){
		system_timeline->sync_time(); //this instruction blocks the task on a condition variable
		system_timeline->print_time();
	}

	int num_attempts = 0;
    clock_t start_refresh_time; //measures how much time is passed since the last call of sync_time()
    for(;;){

    	start_refresh_time = clock(); 
    	while(server_connection_alive && wifi_connection_alive){
	    	Packet p;
	    	packet_queue->pop(p);

	    	p.print();

	    	if(p.get_timestamp() > TIMESTAMP_VALIDATION){ 
		       	if(net_handler.send(p)){ 
			        gpio_set_level((gpio_num_t)BLINK_GPIO, 1);
			        vTaskDelay(10 / portTICK_RATE_MS);
			        gpio_set_level((gpio_num_t)BLINK_GPIO, 0);
			        vTaskDelay(10 / portTICK_RATE_MS);
			    } else {
			    	server_connection_alive = false;
			    	ESP_LOGI(TAG, "connection lost");
			    }
			} else 
			//timestamp is not valid when packet was sniffed while connection to the server or to wifi was not alive 
			    	ESP_LOGI(TAG, "wrong timestamp. packet discarded");
			
		    
	        vTaskDelay(500 / portTICK_RATE_MS);

	        //check for central clock updates
	        if(((clock() - start_refresh_time)/(int)CLOCKS_PER_SEC) >= TIME_REFRESH_RATE){
	        	system_timeline->sync_time();
	        	start_refresh_time = clock();
	        }
	    }

	    // server connection recovery
	    if(wifi_connection_alive){
		    net_handler.disconnect(); //close socket and open a new one
		    vTaskDelay(1000 / portTICK_RATE_MS);
		    server_connection_alive = net_handler.connect(); //attempt to connect to server with the new socket

		    if(!server_connection_alive){
				ESP_LOGI(TAG, "(%d) Trying to reconnect...",num_attempts+1);
		    	num_attempts++;
		    	if(num_attempts > SERVER_CONNECTION_ATTEMPTS)
		    		wifi_connection_alive = false; //too many attempts. maybe it's a wifi problem
		    } else 
		    	system_timeline->sync_time();
	    } else {
	    	//wifi not working
	    	wifi_handler->disable();
	    	vTaskDelay(1000 / portTICK_RATE_MS);
	    	wifi_handler->enable();
	    	wifi_handler->wait_connection();
	    	wifi_connection_alive = true;
	    	num_attempts = 0;
	    }
    }

    vTaskDelete(NULL);
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
		if(packet_queue->timed_push(p, 5)) //wait for push for at most 5 seconds
			cout<<"pushed"<<endl;
		else
			cout<<"full queue timeout"<<endl;
	}
}
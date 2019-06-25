#include "Sniffer.h"
#include "PacketContainer.h"


// must be global because used in promiscuous rx callback
PacketContainer* packet_queue = nullptr;

// wifi stack task is pinned to core 1
Sniffer::Sniffer(NetWrap& netwrap_reference) : netw_ref(netwrap_reference)
{
    // create packet container
    packet_queue = new PacketContainer();

    // setting up GPIO to blink led when packet is sent to central server
    gpio_pad_select_gpio((gpio_num_t)BLINK_GPIO);
    gpio_set_direction((gpio_num_t)BLINK_GPIO, GPIO_MODE_OUTPUT);

    // STARTING SENDER TASK: the aim of this task is to get packets from the
    // queue and send them to the central server
    xTaskCreatePinnedToCore(Sniffer::sender_task, "Sender_Task",
                            SNIF_TASK_STACK_SIZE, this, SNIF_TASK_PRIORITY,
                            &task_sender_handle, SNIF_TASK_RUNNING_CORE);
    if (task_sender_handle == nullptr)
        ESP_LOGI("sniffer_constructor", "unable to create SENDER task");

    // set filter for only MGMT packets
    wifi_promiscuous_filter_t filter;
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));

    // registering the callback to invoke on packet sniffed event
    ESP_ERROR_CHECK(
        esp_wifi_set_promiscuous_rx_cb(&Sniffer::incoming_packet_cb));

    // this instruction makes the board able to sniff packets
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
}

Sniffer::~Sniffer()
{
    if (packet_queue != nullptr)
        delete packet_queue;

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
}

void
Sniffer::blink(int ms)
{
    gpio_set_level((gpio_num_t)BLINK_GPIO, 1);
    vTaskDelay(ms / portTICK_RATE_MS);
    gpio_set_level((gpio_num_t)BLINK_GPIO, 0);
    vTaskDelay(ms / portTICK_RATE_MS);
}

void
Sniffer::sender_task(void* pvParameters)
/*
    1. pop packet from queue
    2. wait for wifi and server to be connected
    3. send packet to server
*/
{
    ESP_LOGD(tag, "at core %d.", xPortGetCoreID());

    Sniffer* snif_handle = (Sniffer*)pvParameters;

    // overrides class tag variable
    const char* tag = "wwb-Sender_Task";

    EventBits_t sync_bits   = (wifi_connected_bit | server_connected_bit);
    TickType_t tics_to_wait = portMAX_DELAY;

    if (CONFIG_PACKET_DISCARD_TIME > 0)
        tics_to_wait = CONFIG_PACKET_DISCARD_TIME / portTICK_PERIOD_MS;

    while (true) {
        Packet packet;
        packet_queue->pop(packet);

        // wait for wifi and server to be connected
        EventBits_t rbits = xEventGroupWaitBits(sync_group, sync_bits, false,
                                                true, tics_to_wait);

        if ((rbits & sync_bits) != sync_bits) {
            ESP_LOGD(tag, "(sender_task) not connected, discarding packet.");
            continue;
        }

        // debug only
        // TODO: remove
        // packet = Packet::forge_test_packet();

        bool rval = snif_handle->netw_ref.send_packet(packet);
        if (rval == false) {
            ESP_LOGE(tag,
                     "(sender_task) packet sending failed, discarding packet.");
            continue;
        }
        snif_handle->blink(10);
        ESP_LOGD(tag, "(sender_task) sent packet %s", packet.str().c_str());
    }
}

void
Sniffer::incoming_packet_cb(void* buff, wifi_promiscuous_pkt_type_t type)
{
    if (type != WIFI_PKT_MGMT) {
        ESP_LOGD(tag, "(incoming_packet_cb) not a MGMT packet.");
        return;
    }

    Packet packet;
    if (packet.fetch_probr(buff) == true) {
        // if queue is full, try to push for at most 5 seconds
        if (packet_queue->timed_push(packet, 5))
            ESP_LOGD(tag, "(incoming_packet_cb) packet pushed to queue.");
        else
            ESP_LOGI(tag, "(incoming_packet_cb) full queue timeout!");
    }
}
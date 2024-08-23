/*ResPECT - Transactional networking stack development.
ESP32 - Firmware for transmitting a UDP packet bypassing the esp_netif()
library. Depends on only FreeRTOS and the standard ESP32 libraries 
*/

#define PRINT_ALL

// Standard libraries
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>

#include "freertos/FreeRTOS.h"

// WiFi driver library
#include "esp_private/wifi.h"
#include "esp_wifi.h"

// Event library
#include "esp_event.h"

// #include "soc/soc.h"

// ESP error definitions
#include "esp_err.h"
#include "esp_log.h"


#define WIFI_QEMU
#undef WIFI_QEMU

// Defining connection parameters
#ifdef WIFI_QEMU
#define WIFI_SSID "Espressif"
#define WIFI_PASS ""
#else
#define WIFI_SSID "ResPECT"
#define WIFI_PASS "twoflower"
#endif



#define SCAN_AUTH_MODE_THRESHOLD ESP_WIFI_AUTH_WPA2_PSK // Not used since it is default
#define MAX_RETRY 5

// #undef PRINT_ALL

esp_err_t ret;

uint32_t init_regs[] = {};

uint8_t s_retry_num = 0;

typedef struct __attribute__((packed)) dma_list_item {
	uint16_t size : 12;
	uint16_t length : 12;
	uint8_t _unknown : 6;
	uint8_t has_data : 1;
	uint8_t owner : 1; // What does this mean?
	void* packet;
	struct dma_list_item* next;
} dma_list_item_t;

#define PRINT_DMA_LIST_ITEM(item) {\
    printf("Size: %d\n", item->size);\
    printf("length: %d\n", item->length);\
    printf("_unknown: %d\n", item->_unknown);\
    printf("has_data: %d\n", item->has_data);\
    printf("owner: %d\n", item->owner);\
    printf("packet: %p\n", item->packet);\
}

void ppRxPkt();

void ppTxPkt(void* param1, int param2);

void lmacTxFrame(uint8_t *buffer,int param_2);

void lmacSetTxFrame(uint8_t *buffer,int param_2);

void lmacRetryTxFrame(uint8_t *buffer,int param_2);

void lmacTxDone(int param1, int param2);

void hal_mac_tx_get_blockack(int param_1, uint8_t* param_2);

void hal_mac_tx_config_edca(int param_1);

void hal_mac_clr_txq_state(int param_1,int param_2);

uint32_t hal_mac_tx_set_ppdu(uint32_t *param_1);

uint32_t mac_tx_set_plcp1(uint32_t* param1);

uint32_t mac_tx_set_plcp2(uint32_t* param1);

void hal_mac_txq_enable(int slot);

void hal_set_tx_pti(uint32_t param_1,uint32_t param_2,uint32_t param_3,uint32_t param_4,uint32_t param_5,uint32_t param_6,
                   uint32_t param_7);

uint32_t mac_tx_set_txop_q(uint32_t* param1);

uint32_t mac_tx_set_htsig(uint32_t* param1, uint32_t param2);



// // Causes crash
// uint32_t hal_mac_interrupt_get_event(void);

// void hal_mac_interrupt_clr_event(uint32_t value);

void hal_mac_set_addr(int param_1, uint8_t* param_2);

void hal_init(void);

uint32_t ic_mac_init(void);

uint32_t ic_mac_deinit(void);

void rdTxPkt(void* param1, int param2) {
#ifdef PRINT_ALL
    printf("Calling ppTxPkt\n");
#endif
    ppTxPkt(param1, param2);
#ifdef PRINT_ALL
    printf("Calling ppTxPkt\n");
#endif
}

void rdRxPkt() {
#ifdef PRINT_ALL
    printf("Calling ppRxPkt\n");
#endif
    ppRxPkt();
#ifdef PRINT_ALL
    printf("Done ppRxPkt\n");
#endif
}

void lrdcTxFrame(uint8_t *buffer,int param_2) {
#ifdef PRINT_ALL
    printf("Calling lmacTxFrame with DMA struct and parameter 2 of lmacTxFrame: %d\n", param_2);
#endif
    lmacTxFrame(buffer, param_2);
#ifdef PRINT_ALL
    printf("Done lmacTxFrame\n");
#endif
}

// Not called at all
void lrdcSetTxFrame(uint8_t *buffer,int param_2){
#ifdef PRINT_ALL
    printf("Calling lmacSetTxFrame with DMA struct and parameter 2 of lmacTxFrame: %d\n", param_2);
#endif
#ifdef PRINT_ALL
    lmacSetTxFrame(buffer, param_2);
    printf("Done lmacSetTxFrame\n");
#endif
}

void lrdcRetryTxFrame(uint8_t *buffer,int param_2){
#ifdef PRINT_ALL
    printf("Calling lmacRetryTxFrame\n");
#endif
    lmacRetryTxFrame(buffer, param_2);
#ifdef PRINT_ALL
    printf("Done lmacRetryTxFrame\n");
#endif
}

void lrdcTxDone(int param_1,int param_2){
#ifdef PRINT_ALL
    printf("Calling lmacTxDone\n");
#endif
    lmacTxDone(param_1, param_2);
#ifdef PRINT_ALL
    printf("Done lmacTxDone\n");
#endif
}

void rdr_mac_tx_get_blockack(int param_1, uint8_t* param_2){
#ifdef PRINT_ALL
    printf("Calling hal_mac_tx_get_blockack\n");
#endif
    hal_mac_tx_get_blockack(param_1, param_2);
#ifdef PRINT_ALL
    printf("Done hal_mac_tx_get_blockack\n");
#endif
}

void rdr_mac_tx_config_edca(int param_1){
#ifdef PRINT_ALL
    printf("Calling hal_mac_tx_config_edca\n");
#endif
    hal_mac_tx_config_edca(param_1);
#ifdef PRINT_ALL
    printf("Done hal_mac_tx_config_edca\n");
#endif
}

void rdr_mac_clr_txq_state(int param_1, int param_2){
#ifdef PRINT_ALL
    printf("Calling hal_mac_clr_txq_state\n");
#endif
    hal_mac_clr_txq_state(param_1, param_2);
#ifdef PRINT_ALL
    printf("Done hal_mac_clr_txq_state\n");
#endif
}

uint32_t rdr_mac_tx_set_ppdu(uint32_t *param_1){
#ifdef PRINT_ALL
    printf("Calling hal_mac_tx_set_ppdu\n");
#endif
    uint32_t value = hal_mac_tx_set_ppdu(param_1);
#ifdef PRINT_ALL
    printf("Done hal_mac_tx_set_ppdu\n");
#endif
    return value;
}

uint32_t rdr_tx_set_plcp1(uint32_t* param1){
#ifdef PRINT_ALL
    printf("Calling mac_tx_set_plcp1\n");
#endif
    uint32_t value = mac_tx_set_plcp1(param1);
#ifdef PRINT_ALL
    printf("Done mac_tx_set_plcp1\n");
#endif
    return value;
}

uint32_t rdr_tx_set_plcp2(uint32_t* param1){
#ifdef PRINT_ALL
    printf("Calling mac_tx_set_plcp2\n");
#endif
    uint32_t value = mac_tx_set_plcp2(param1);
#ifdef PRINT_ALL
    printf("Done mac_tx_set_plcp2\n");
#endif
    return value;
}

void rdr_mac_set_addr(int param_1, uint8_t* param_2){
#ifdef PRINT_ALL
    printf("Calling hal_mac_set_addr\n");
#endif
    hal_mac_set_addr(param_1, param_2);
#ifdef PRINT_ALL
    printf("Done hal_mac_set_addr\n");
#endif
}

void rdr_init(void){
#ifdef PRINT_ALL
    printf("Calling hal_init\n");
#endif
    hal_init();
#ifdef PRINT_ALL
    printf("Done hal_init\n");
#endif
}

void rdr_mac_txq_enable(int slot){
#ifdef PRINT_ALL
    printf("Calling hal_mac_txq_enable\n");
#endif
    hal_mac_txq_enable(slot);
    uint32_t plcp0_value = REG_READ(0x60033d08);
    printf("DMA struct written to PLCP0 register:\n");
    dma_list_item_t* dma_addr = (dma_list_item_t*)((plcp0_value & 0xfffff) | 0x3fc00000);
    PRINT_DMA_LIST_ITEM(dma_addr);
    for(uint16_t i = 0; i < dma_addr->length; i++){
        printf("0x%02x ", *(uint8_t *)(dma_addr->packet + i));
    }
    printf("\n");
#ifdef PRINT_ALL
    printf("Done hal_mac_txq_enable\n");
#endif
}

uint32_t rd_mac_init(void){
#ifdef PRINT_ALL
    printf("Calling ic_mac_init\n");
#endif
    uint32_t value = ic_mac_init();
#ifdef PRINT_ALL
    printf("Done ic_mac_init\n");
#endif
    return value;
}

uint32_t rd_mac_deinit(void){
#ifdef PRINT_ALL
    printf("Calling ic_mac_deinit\n");
#endif
    uint32_t value = ic_mac_deinit();
#ifdef PRINT_ALL
    printf("Done ic_mac_deinit\n");
#endif
    return value;
}

void hrd_set_tx_pti(uint32_t param_1,uint32_t param_2,uint32_t param_3,uint32_t param_4,uint32_t param_5,uint32_t param_6,
                   uint32_t param_7){
#ifdef PRINT_ALL
    printf("Calling hal_tx_set_pti\n");
#endif
    hal_set_tx_pti(param_1, param_2, param_3, param_4, param_5, param_6, param_7);
#ifdef PRINT_ALL
    printf("Done hal_tx_set_pti\n");
#endif
}

uint32_t rdr_tx_set_txop_q(uint32_t* param1){
#ifdef PRINT_ALL
    // uint32_t* ptr = 0x60034314 + (uint32_t)*(uint8_t *)(param1 + 1) * -0x13;
    printf("Calling mac_tx_set_txop_q with pointer %p. Value at pointer is 0x%08lx\n", param1, *param1);
#endif
    uint32_t value = mac_tx_set_txop_q(param1);
#ifdef PRINT_ALL
    printf("Done mac_tx_set_txop_q\n");
#endif
    return value;
}

uint32_t rdr_tx_set_htsig(uint32_t* param1, uint32_t param2){
#ifdef PRINT_ALL
    printf("Calling mac_tx_set_htsig\n");
#endif
    uint32_t value = mac_tx_set_htsig(param1, param2);
#ifdef PRINT_ALL
    printf("Done mac_tx_set_htsig\n");
#endif
    return value;
}

// Crashes. Probably due to wifi thread accessing the registers. Don't bother. Addresses are known
// uint32_t rdr_mac_interrupt_get_event(){
//     printf("Running hal_mac_interrupt_get_event\n");
//     uint32_t value = REG_READ(0x60033c3c);
//     printf("Done hal_mac_interrupt_get_event\n");
//     return value;
// }

// void rdr_mac_interrupt_clr_event(uint32_t value){
//     printf("Running hal_mac_interrupt_clr_event\n");
//     REG_WRITE(0x60033c40, value);
//     printf("Done hal_mac_interrupt_clr_event\n");
// }


const uint8_t packet[] = {
    // MAC layer
    0xc8, 0x15, 0x4e, 0xd4, 0x65, 0x1b, // Destination MAC address
    0x84, 0xf7, 0x03, 0x60, 0x81, 0x5c, // Source MAC address
    0x08, 0x00, // protocol type - IPV4
    
    // IPv4 header
    0x45, //version :4 (obv) with IHL = 5
    0x00, // DSCP and ECN
    0x00, 0x21, // Total length (33 bytes), IPv4 Header + UDP Header + "Hello"
    0x00, 0x00, // Identification and fragmentation data
    0x00, 0x00, // Flags and fragment offset
    0x05, // TTL
    0x11, // Protocol type UDP 
    0x33, 0x4a, // Header checksum (update after calculating length of total packet)
    0xc0, 0xa8, 0x00, 0x7A, // Source IP address - Arbitrary IP address for the ESP-32
    0xc0, 0xa8, 0x00, 0xb8, // Destination IP address - Laptop's IP address (assigned through DHCP by the router)

    // UDP header - 8 bytes
    0x1f, 0x45, // Source port - Both source and destination ports are random since it is UDP
    0xf0, 0xf0, // Destination port
    0x00, 0x0d, // Length
    0x00, 0x00,  // Checksum - calculate using the pseudo ipv4 header
    'h', 'e', 'l', 'l', 'o' // The message :)

};

// Event handlers for connecting to the wifi. If event notifies that station is started, perform connection
// esp_wifi_connect() is part of the blobs
void event_handler(void *arg, esp_event_base_t event_base,
                    int32_t event_id, void* event_data)
{
    // Check the event base for a WiFi event. If device disconnects, retry until MAX_RETRY is exceeded
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        ESP_ERROR_CHECK(esp_wifi_connect());
#ifdef PRINT_ALL
        printf("event_handler(): Connected...\n");
#endif
    }else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        if (s_retry_num < MAX_RETRY){
#ifdef PRINT_ALL
            printf("Retrying...\n");
#endif
            ESP_ERROR_CHECK(esp_wifi_connect());
            s_retry_num++;
        }
    }
}


void app_main(){

    // Get default configuration for the wifi drivers. See esp_wifi.h for the default config. The config also 
    // describes the esp32c3 specific OS adapter functions. 
    // NVS storage of wifi parameters was disabled in the SDKConfig menu.
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = false;

    // initialize WiFi. This may have some hardware settings. Part of the .a files
    ret = esp_wifi_init(&cfg);
    if(ret != ESP_OK){
#ifdef PRINT_ALL
        printf("Wifi could not be initialized (0x%x)\n", ret);
#endif
    }

    // Create the event loop to monitor the wifi events such as connecting and station
    // Declare the handlers for connecting to event.
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_event_handler_instance_t instance_any_id;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));

    // // Create WiFi configuration with defined SSID and PASSWORD
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    // Set to station mode. Set the WiFi configuration. Provide wifi interface handle (WIFI_IF_STA)
    // Start the esp WiFi connection. All are called directly from the blobs
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Wait for the wifi init to complete
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    while(1){
        ESP_LOGI("main", "Sending UDP packet....");
        //Prepare a packet buffer for sending
        uint8_t psize = sizeof(packet);
    
        // Call the internal tx function (also from blob)
        ret = esp_wifi_internal_tx(WIFI_IF_STA, &packet, psize);
        ESP_LOGI("main", "Sent with result %s", esp_err_to_name(ret));
        
        // Shorter packet delay to see if multiple slots are used
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    
}
/*ResPECT - Transactional networking stack development.
ESP32 - Firmware for transmitting a UDP packet bypassing the esp_netif()
library. Depends on only FreeRTOS and the standard ESP32 libraries 
*/

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

// ESP error definitions
#include "esp_err.h"
#include "esp_log.h"

// Defining connection parameters
#define WIFI_SSID "Espressif"
#define WIFI_PASS ""
#define SCAN_AUTH_MODE_THRESHOLD ESP_WIFI_AUTH_WPA2_PSK // Not used since it is default
#define MAX_RETRY 5

esp_err_t ret;

uint8_t s_retry_num = 0;

void ppRxPkt();

void ppTxPkt(void* param1, int param2);

void lmacTxFrame(uint8_t *buffer,int param_2);

void lmacSetTxFrame(uint8_t *buffer,int param_2);

void lmacRetryTxFrame(uint8_t *buffer,int param_2);

void lmacTxDone(int param1, int param2);

void hal_mac_tx_get_blockack(int param_1, uint8_t* param_2);

void hal_mac_tx_config_edca(int param_1);

void hal_mac_clr_txq_state(int param_1,int param_2);

uint32_t hal_mac_tx_set_ppdu(uint32_t *param_1, uint32_t param_2);

uint32_t mac_tx_set_plcp1(uint32_t* param1);

uint32_t mac_tx_set_plcp2(uint32_t* param1);

void hal_mac_txq_enable(int slot);

// // Causes crash
// uint32_t hal_mac_interrupt_get_event(void);

// void hal_mac_interrupt_clr_event(uint32_t value);

void hal_mac_set_addr(int param_1, uint8_t* param_2);

void hal_init(void);

uint32_t ic_mac_init(void);

void rdTxPkt(void* param1, int param2) {
    printf("Calling ppTxPkt\n");
    ppTxPkt(param1, param2);
    printf("Calling ppTxPkt\n");
}

void rdRxPkt() {
    printf("Calling ppRxPkt\n");
    ppRxPkt();
    printf("Done ppRxPkt\n");
}

void lrdcTxFrame(uint8_t *buffer,int param_2) {
    printf("Calling lmacTxFrame\n");
    lmacTxFrame(buffer, param_2);
    printf("Done lmacTxFrame\n");
}

void lrdcSetTxFrame(uint8_t *buffer,int param_2){
    printf("Calling lmacSetTxFrame\n");
    lmacSetTxFrame(buffer, param_2);
    printf("Done lmacSetTxFrame\n");
}

void lrdcRetryTxFrame(uint8_t *buffer,int param_2){
    printf("Calling lmacRetryTxFrame\n");
    lmacRetryTxFrame(buffer, param_2);
    printf("Done lmacRetryTxFrame\n");
}

void lrdcTxDone(int param_1,int param_2){
    printf("Calling lmacTxDone\n");
    lmacTxDone(param_1, param_2);
    printf("Done lmacTxDone\n");
}

void rdr_mac_tx_get_blockack(int param_1, uint8_t* param_2){
    printf("Calling hal_mac_tx_get_blockack\n");
    hal_mac_tx_get_blockack(param_1, param_2);
    printf("Done hal_mac_tx_get_blockack\n");
}

void rdr_mac_tx_config_edca(int param_1){
    printf("Calling hal_mac_tx_config_edca\n");
    hal_mac_tx_config_edca(param_1);
    printf("Done hal_mac_tx_config_edca\n");
}

void rdr_mac_clr_txq_state(int param_1, int param_2){
    printf("Calling hal_mac_clr_txq_state\n");
    hal_mac_clr_txq_state(param_1, param_2);
    printf("Done hal_mac_clr_txq_state\n");
}

uint32_t rdr_mac_tx_set_ppdu(uint32_t *param_1, uint32_t param_2){
    printf("Calling hal_mac_tx_set_ppdu\n");
    uint32_t value = hal_mac_tx_set_ppdu(param_1, param_2);
    printf("Done hal_mac_tx_set_ppdu\n");
    return value;
}

uint32_t rdr_tx_set_plcp1(uint32_t* param1){
    printf("Calling mac_tx_set_plcp1\n");
    uint32_t value = mac_tx_set_plcp1(param1);
    printf("Done mac_tx_set_plcp1\n");
    return value;
}

uint32_t rdr_tx_set_plcp2(uint32_t* param1){
    printf("Calling mac_tx_set_plcp2\n");
    uint32_t value = mac_tx_set_plcp2(param1);
    printf("Done mac_tx_set_plcp2\n");
    return value;
}

void rdr_mac_set_addr(int param_1, uint8_t* param_2){
    printf("Calling hal_mac_set_addr\n");
    hal_mac_set_addr(param_1, param_2);
    printf("Done hal_mac_set_addr\n");
}

void rdr_init(void){
    printf("Calling hal_init\n");
    hal_init();
    printf("Done hal_init\n");
}

void rdr_mac_txq_enable(int slot){
    printf("Calling hal_mac_txq_enable\n");
    hal_mac_txq_enable(slot);
    printf("Done hal_mac_txq_enable\n");
}

uint32_t rd_mac_init(void){
    printf("Calling ic_mac_init\n");
    uint32_t value = ic_mac_init();
    printf("Done ic_mac_init\n");
    return value;
}

// Crashes. Probably due to wifi thread accessing the registers. Don't bother. Addresses are known
// uint32_t rdr_mac_interrupt_get_event(){
//     printf("Calling hal_mac_interrupt_get_event\n");
//     uint32_t event_reg = 0x60033c3c;
//     uint32_t value = atomic_load((uint32_t*)event_reg);
//     printf("Done hal_mac_interrupt_get_event\n");
//     return value;
// }

// void rdr_mac_interrupt_clr_event(uint32_t value){
//     printf("Calling hal_mac_interrupt_clr_event\n");
//     uint32_t event_clr_reg = 0x60033c40;
//     atomic_store((uint32_t *)event_clr_reg, value);
//     printf("Done hal_mac_interrupt_clr_event\n");
// }


const uint8_t packet[] = {
    // MAC layer
    0x74, 0xdf, 0xbf, 0xa4, 0xf7, 0x97, // Destination MAC address
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
    0x33, 0x55, // Header checksum (update after calculating length of total packet)
    0xc0, 0xa8, 0x00, 0x7A, // Source IP address - Arbitrary IP address for the ESP-32
    0xc0, 0xa8, 0x00, 0xb7, // Destination IP address - Laptop's IP address (assigned through DHCP by the router)

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
        printf("event_handler(): Connected...\n");
    }else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        if (s_retry_num < MAX_RETRY){
            printf("Retrying...\n");
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
        printf("Wifi could not be initialized (0x%x)\n", ret);
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
        //Prepare a packet buffer for sending
        uint8_t psize = sizeof(packet);
    
        // Call the internal tx function (also from blob)
        ret = esp_wifi_internal_tx(WIFI_IF_STA, &packet, psize);
        ESP_LOGI("main", "Sent with result %s", esp_err_to_name(ret));
        
        // Shorter packet delay to see if multiple slots are used
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    
}
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

#include "sdkconfig.h"

#include "esp_attr.h"
#include "riscv/interrupt.h"
#include "rom/ets_sys.h"

// Register definitions
// MAC registers
#define WIFI_MAC_BASE 0x60033000
#define WIFI_TX_CONFIG 0x60033d04
#define WIFI_MAC_CTRL 0x60033ca0
#define WIFI_TX_PLCP0 0x60033d08
#define WIFI_TX_PLCP1 0x600342f8
#define WIFI_TX_PLCP1_2 0x600342fc
#define WIFI_TX_HTSIG 0x60034310
#define WIFI_TX_PLCP2 0x60034314
#define WIFI_TX_DURATION 0x60034318
#define WIFI_TX_STATUS 0x60033cb0
#define WIFI_TX_CLR 0x60033cac
#define WIFI_TX_GET_ERR 0x60033ca8
#define WIFI_TX_CLR_ERR 0x60033ca4
#define WIFI_INT_STATUS_GET 0x60033c3c
#define WIFI_INT_STATUS_CLR 0x60033c40
#define PWR_INT_STATUS_GET 0x60035118
#define PWR_INT_STATUS_CLR 0x6003511c
#define WIFI_MAC_CCA_REG 0x60033c50


#define WIFI_INT_STATUS_GET 0x60033c3c
#define WIFI_INT_STATUS_CLR 0x60033c40
#define PWR_INT_STATUS_GET 0x60035118
#define PWR_INT_STATUS_CLR 0x6003511c

// Intererupt registers
#define INTR_SRC_MAC 0x600c2000
#define INTR_SRC_PWR 0x600c2008
#define INTR_ENABLE_REG 0x600c2104 // Writing a 1 to corresponding enables and writing 0 disables
#define INTR_STATUS_REG 0x600c00F8

#define WIFI_INTR_NUMBER 1
#define SYSTICK_INTR_NUMBER 7 // Tick 
#define TIMER_ALARM_NUMBER 3
#define TASK_WDT_NUMBER 9

#define MAX_RETRY 5

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

#define DUMP_MEMORY(addr, len) {\
    for(int ii = 0; ii < len; ii++){\
        printf("%02x ", *(uint8_t *)(addr + ii));\
    }\
    printf("\n");\
}

#define PRINT_PACKET(item) {\
    PRINT_DMA_LIST_ITEM(item);\
    printf("Packet: ");\
    DUMP_MEMORY(item->packet, len);\
    printf("\n");\
}

#define PRINT_REG(_r) {\
    uint32_t _v = REG_READ(_r);\
    printf("0x%08x: 0x%08x\n", (unsigned int)_r, (unsigned int)_v);\
}

#define DUMP_REGS() {\
    printf("WIFI_MAC_BASE - ");\
    PRINT_REG(WIFI_MAC_BASE);\
    printf("WIFI_TX_CONFIG - ");\
    PRINT_REG(WIFI_TX_CONFIG);\
    printf("WIFI_MAC_CTRL - ");\
    PRINT_REG(WIFI_MAC_CTRL);\
    printf("WIFI_TX_PLCP0 - ");\
    PRINT_REG(WIFI_TX_PLCP0);\
    printf("WIFI_TX_PLCP1 - ");\
    PRINT_REG(WIFI_TX_PLCP1);\
    printf("WIFI_TX_PLCP1_2 - ");\
    PRINT_REG(WIFI_TX_PLCP1_2);\
    printf("WIFI_TX_HTSIG - ");\
    PRINT_REG(WIFI_TX_HTSIG);\
    printf("WIFI_TX_PLCP2 - ");\
    PRINT_REG(WIFI_TX_PLCP2);\
    printf("WIFI_TX_DURATION - ");\
    PRINT_REG(WIFI_TX_DURATION);\
}

void wDev_ProcessFiq(void);

void ppRxPkt();

void ppTxPkt(void* param1, int param2);

void lmacTxFrame(uint32_t *param1,uint32_t param2);

void lmacSetTxFrame(uint8_t *buffer,int param_2);

void lmacRetryTxFrame(uint8_t *buffer,int param_2);

void lmacTxDone(int param1, int param2);

void hal_mac_tx_get_blockack(int param_1, uint8_t* param_2);

void hal_mac_tx_config_edca(int param_1);

void hal_mac_clr_txq_state(int param_1,int param_2);

uint32_t hal_mac_tx_set_ppdu(uint32_t *param_1, uint32_t *param_2);

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

void lrdcTxFrame(uint32_t *param1,uint32_t param2) {
#ifdef PRINT_ALL
    printf("Calling lmacTxFrame\n");
#endif
    ESP_LOGI("lmacTxFrame", "Before");
    DUMP_REGS();
    printf("param1 - %p: ", param1);
    DUMP_MEMORY(param1, 100);
    printf("param2: ");
    printf("%lx\n", param2);
    lmacTxFrame(param1, param2);
    ESP_LOGI("lmacTxFrame", "After");
    DUMP_REGS();
#ifdef PRINT_ALL
    printf("Done lmacTxFrame\n");
#endif
}

// Not called at all
void lrdcSetTxFrame(uint8_t *buffer,int param_2){
    ESP_LOGI("lmacSetTxFrame", "Calling...");
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

uint32_t rdr_mac_tx_set_ppdu(uint32_t *param_1, uint32_t *param_2){
#ifdef PRINT_ALL
    printf("Calling hal_mac_tx_set_ppdu\n");
    printf("param_1 points to %08lx and param_2 points to %08lx\n", *param_1, *param_2);
    printf("%08lx: ", *param_1);
    DUMP_MEMORY(*param_1, 100);
#endif
    uint32_t value = hal_mac_tx_set_ppdu(param_1, param_2);
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

// // Hand crafted UDP packet
const uint8_t packet[] = {
    // MAC layer
    0xc8, 0x15, 0x4e, 0xd4, 0x65, 0x1b,
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

// esp32-open-mac interrupt handler
void IRAM_ATTR wifi_interrupt_handler(void){
    uint32_t mac_cause = REG_READ(WIFI_INT_STATUS_GET);
    uint32_t pwr_cause = REG_READ(PWR_INT_STATUS_GET);
    #ifdef PRINT_ALL
    ets_printf("In ISR: MAC Cause %lx, PWR cause %lx\n", mac_cause, pwr_cause);
    #endif
    wDev_ProcessFiq();    
    return;
}

void respect_setup_interrupt(){
    // This setup works perfectly "sometimes"

    ESP_LOGI("setup_intr", "Clearing existing interrupts");
    // ic_set_interrupt_handler() in ghidra
    // Mask out power interrupt and the MAC interrupt sources (temporarily)
    // From decompilation of intr_matrix_set. Both are mapped to CPU interrupt number 1
    // Disable the wifi CPU interrupt and renable after setting the handler
    REG_WRITE(INTR_SRC_MAC, 0);
    REG_WRITE(INTR_SRC_PWR, 0);

    // Disable all interrupt sources. A bit hacky but works
    uint32_t value = REG_READ(INTR_ENABLE_REG);
    REG_WRITE(INTR_ENABLE_REG, 0);
    intr_handler_set(WIFI_INTR_NUMBER, (intr_handler_t)wifi_interrupt_handler, 0);
    REG_WRITE(INTR_ENABLE_REG, value);

    // Unmask the interrupt source again. So called "routing"
    REG_WRITE(INTR_SRC_MAC, WIFI_INTR_NUMBER);
    REG_WRITE(INTR_SRC_PWR, WIFI_INTR_NUMBER);

}

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
            .ssid = CONFIG_ESP_WIFI_SSID,
            .password = CONFIG_ESP_WIFI_PASSWORD,
        },
    };

    // Set to station mode. Set the WiFi configuration. Provide wifi interface handle (WIFI_IF_STA)
    // Start the esp WiFi connection. All are called directly from the blobs
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    respect_setup_interrupt();

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
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    
}
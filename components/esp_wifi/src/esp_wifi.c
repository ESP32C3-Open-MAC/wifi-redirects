#include "esp_private/esp_wifi_private.h"
#include "esp_private/wifi.h"
#include "esp_wifi_crypto_types.h"
#include "esp_wifi_he_types.h"
#include "esp_wifi_he.h"
#include "esp_wifi_types.h"
#include "esp_wifi.h"

// esp_event library is needed to handle WiFi events
#include "esp_event.h"

// Physical modem activation and hardware related inits
#include "esp_phy_init.h"
#include "esp_private/adc_share_hw_ctrl.h"
#include "esp_wpa.h"


ESP_EVENT_DEFINE_BASE(WIFI_EVENT);

bool s_wifi_inited = false;

esp_err_t esp_wifi_deinit(void){
    // Reverse the actions performed in the init script
    esp_supplicant_deinit();
    esp_err_t deinit_ret = esp_wifi_deinit_internal();
    if(deinit_ret != ESP_OK){
        printf("Wifi was not deinited (0x%x)\n", deinit_ret);
    }
    esp_wifi_power_domain_off();
    esp_phy_modem_deinit();
    s_wifi_inited = false;
    return deinit_ret;
}

esp_err_t esp_wifi_init(const wifi_init_config_t *config)
{
    printf("Called esp_wifi_init\n");
    if (s_wifi_inited) {
        return ESP_OK;
    }

    esp_err_t result = ESP_OK;
    printf("esp_wifi_init: Called esp_wifi_power_domain_on\n");
    esp_wifi_power_domain_on();
    printf("esp_wifi_init: Done esp_wifi_power_domain_on\n");
    printf("esp_wifi_init: Called esp_wifi_init_internal\n");
    result = esp_wifi_init_internal(config);
    printf("esp_wifi_init: Done esp_wifi_init_internal\n");
    if (result == ESP_OK) {
        esp_phy_modem_init();
        result = esp_supplicant_init();
        if (result != ESP_OK) {
            printf("Failed to init supplicant (0x%x)\n", result);
            goto _deinit;
        }
    } else {
        goto _deinit;
    }

    printf("esp_wifi_init: Called adc2_cal_include\n");
    adc2_cal_include(); //This enables the ADC2 calibration constructor at start up.
    printf("esp_wifi_init: Done adc2_cal_include\n");

    s_wifi_inited = true;

    return result;

_deinit:
    ;
    esp_err_t ret = esp_wifi_deinit();

    if(ret != ESP_OK)  {
        printf("WiFi could not be deinited. Error code 0x%x\n", ret);
    }

    return result;
}
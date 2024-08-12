# Minimal connection to a WiFi AP
ESP-IDF project to configure the ESP32-C3 in WiFi mode in Station mode with the minimal code possible from the official ESP-32 drivers. Ideally, the device must not be able to do DHCP discover or ARP requests/requests by itself.

## Notes:
 - The following settings were configured in the SDK menuconfig
    - Store Phy calibration data was unchecked
    - WiFi NVS Flash was unchecked
 - esp_wifi component
    - A clone of the esp-idf version was created in the local components folder.
    - CMakeLists was updated to only build the esp_wifi.c source. The rest of the sources were removed for the sake of brevity. esp_wifi.c source only consists of direct calls to the internal blob API. Refer to esp_wifi/CMakeLists.txt for more comments (prefixed with "IM:").
    - esp_wifi is dependent on freertos or a similar pre-emptive RTOS. It needs to be provided with stubs for memory allocation (malloc), creating and writing to queue, mutexes and other functions. Refer to components/esp_wifi/esp32c3/esp_adapter.c for more information.
 - CMakeLists configuration
    - Dependencies were explicitly set to make sure a minimal build was achievable (see CMakeLists.txt in main folder for the dependency list). By default ESP-IDF builds the full set of dependencies. This was done to make sure that the project could compile without esp-netif and lwip.
    - Additional dependencies specified for WiFi connectivity.
 - The current firmware only transmits. A callback for reception has not been registered. Therefore, no rx events will be triggered. esp-netif usually takes care of all of this.

## Testing:
 - Connected ESP32 and laptop to a WiFi network without internet access.
 - Used Wireshark to identify the repeated UDP packets.

## TODO:
 - Testing other functions of esp_wifi blobs defined in components/esp_wifi/include/esp_private/wifi.h and components/esp_wifi/include/esp_wifi.h

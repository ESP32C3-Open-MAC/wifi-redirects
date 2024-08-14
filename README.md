# WiFi redirects
The symbols within the WiFi blobs were renamed using sed to call a rdr_* function defined within the main code. Function parameters were as guessed by [Ghidra](https://ghidra-sre.org/). This will not work for static function definitions which are defined and called from within the same object file but plenty of functions are available in the blobs to play around with. The rdr functions simply log what functions from within the binary are called.

This sketch is not tested on hardware but only on this [QEMU version](https://github.com/ishwarmudraje/qemu-esp32c3). To use the compiled sketch, run the following command after compilation within ESP-IDF to merge the app, bootloader and partition tables:
```
esptool.py --chip ESP32-C3 merge_bin -o flash_image.bin @flash_args --fill-flash-size 4MB
```

For running within QEMU, use
```
$QEMU_RISCV_HOME/qemu-system-riscv32 -M esp32c3-picsimlab -drive file=flash_image.bin,if=mtd,format=raw \
  -drive file=qemu_efuse.bin,if=none,format=raw,id=efuse \
  -global driver=nvram.esp32c3.efuse,property=drive,value=efuse \
  -serial stdio -gdb tcp::1234 -icount shift=3,align=off,sleep=on \
  -global driver=timer.esp32c3.timg,property=wdt_disable,value=true \
  -nic user,model=esp32c3_wifi,net=192.168.0.0/24,hostfwd=tcp::16555-192.168.0.15:80
```

Note: Make sure the WPA credentials are set to the ones allowed in the QEMU version.

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


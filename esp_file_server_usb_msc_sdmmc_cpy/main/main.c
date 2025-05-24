#include "main.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "file_server.h"
#include "wifi_server.h"
#include "sd_protocol_types.h"
#include "mount.h"
#include <stdint.h>

#include "usb_msc.h"

sdmmc_card_t *sd_card = NULL;
static const char* TAG = "MAIN";

uint8_t op_mode = HTTP_MODE;

void mode_switch_req_handler(void *param){
    op_mode = !op_mode;

    ESP_LOGI(TAG, "Current operate mode %s", op_mode ? "USB_MODE" : "HTTP_MODE");

    if(op_mode == USB_MODE){
        ESP_LOGI(TAG, "Unmount %s", !umount_sdmmc(sd_card)? "OK" : "Fail");
        my_tinyusb_msc_sdmmc_init(&sd_card);
        ESP_LOGI(TAG, "SD_CARD %s", sd_card == NULL ? "NULL" : "NOT NULL");
    } else {
        my_tinyusb_msc_sdmmc_deinit(&sd_card);
        ESP_LOGI(TAG, "Remount %s", !mount_sdmmc(&sd_card)? "OK" : "Fail");
    }
}

void app_main(void)
{
    wifi_init("HCTL", "123456789HCTL");
    mode_switch_req_cb_register(mode_switch_req_handler);

    if(op_mode ==  HTTP_MODE){
        mount_sdmmc(&sd_card);
    } else {
        my_tinyusb_msc_sdmmc_init(&sd_card);
    }
}

#include <stdio.h>
#include "esp_err.h"
#include "file_server.h"
#include "freertos/idf_additions.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "sd_protocol_types.h"

static const char* TAG = "MAIN" ;
static sdmmc_card_t *card = NULL;

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing...");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(mount_sdmmc(&card));
    wifi_init();
}

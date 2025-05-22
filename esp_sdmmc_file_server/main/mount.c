#include "esp_err.h"
#include "esp_log.h"
#include "file_server.h"
#include "sd_protocol_types.h"
#include "diskio_sdmmc.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include <stdbool.h>
#include <stdlib.h>
#include "driver/sdmmc_default_configs.h"

static const char* TAG = "SDMMC";

esp_err_t mount_sdmmc(sdmmc_card_t **card){
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_err_t ret = ESP_OK;

    slot.width = 4;
    slot.cmd = SDMMC_PIN_CMD;
    slot.clk = SDMMC_PIN_CLK;
    slot.d0  = SDMMC_PIN_D0;
    slot.d1  = SDMMC_PIN_D1;
    slot.d2  = SDMMC_PIN_D2;
    slot.d3  = SDMMC_PIN_D3;
    
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    sdmmc_card_t *_card = NULL;


    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = FILE_BUFSIZE
    };


    ret = esp_vfs_fat_sdmmc_mount(BASE_PATH, &host, &slot, &mount_config, &_card);
    if(ret!=ESP_OK){
        ESP_LOGE(TAG, "Mount vfs error %s", esp_err_to_name(ret));
    }

    sdmmc_card_print_info(stdout, _card);
    *card = _card;
    return ret;
}


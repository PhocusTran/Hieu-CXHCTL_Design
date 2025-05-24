#include "usb_msc.h"
#include "driver/sdmmc_host.h"
#include "tusb_msc_storage.h"

static const char* TAG = "USB";
sdmmc_host_t host = SDMMC_HOST_DEFAULT();
sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
static bool host_init = false;

static char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },  
    "HugeUSB",                      
    "HugeUSB Device",               
    "6969",                         
    "That USB MSC",                 
};

void storage_mount_changed_cb(tinyusb_msc_event_t *event) {
    printf("Storage mounted to application: %s\n", event->mount_changed_data.is_mounted ? "Yes" : "No");
}

esp_err_t storage_init_sdmmc(sdmmc_card_t **card)
{
    esp_err_t ret = ESP_OK;
    sdmmc_card_t *sd_card;

    ESP_LOGI(TAG, "Initializing SDCard");


    // For SD Card, set bus width to use
    slot_config.width = 4;
    slot_config.cmd = 4;
    slot_config.clk = 5;
    slot_config.d0 = 10;
    slot_config.d1 = 11;
    slot_config.d2 = 12;
    slot_config.d3 = 13;

    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    // not using ff_memalloc here, as allocation in internal RAM is preferred
    sd_card = (sdmmc_card_t *)malloc(sizeof(sdmmc_card_t));
    ESP_GOTO_ON_FALSE(sd_card, ESP_ERR_NO_MEM, clean, TAG, "could not allocate new sdmmc_card_t");

    ESP_GOTO_ON_ERROR((*host.init)(), clean, TAG, "Host Config Init fail");
    host_init = true;

    ESP_GOTO_ON_ERROR(sdmmc_host_init_slot(host.slot, (const sdmmc_slot_config_t *) &slot_config),
                      clean, TAG, "Host init slot fail");

    while (sdmmc_card_init(&host, sd_card)) {
        ESP_LOGE(TAG, "The detection pin of the slot is disconnected(Insert uSD card). Retrying...");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, sd_card);
    *card = sd_card;

    printf("init pointer %p\n\r", sd_card);

    return ESP_OK;

clean:
    if (host_init) {
        if (host.flags & SDMMC_HOST_FLAG_DEINIT_ARG) {
            host.deinit_p(host.slot);
        } else {
            (*host.deinit)();
        }
    }
    if (sd_card) {
        free(sd_card);
        sd_card = NULL;
    }

    return ret;
}

void storage_deinit_sdmmc(sdmmc_card_t **card){
    if (host_init) {
        if (host.flags & SDMMC_HOST_FLAG_DEINIT_ARG) {
            host.deinit_p(host.slot);
        } else {
            (*host.deinit)();
        }
    }
    if (*card) {
        free(*card);
        *card = NULL;
    }
}

void my_tinyusb_msc_sdmmc_deinit(sdmmc_card_t **card){
    tinyusb_driver_uninstall();
    tinyusb_msc_storage_deinit();
    storage_deinit_sdmmc(card);
}

void my_tinyusb_msc_sdmmc_init(sdmmc_card_t **card){
    ESP_ERROR_CHECK(storage_init_sdmmc(card));

    const tinyusb_msc_sdmmc_config_t config_sdmmc = (tinyusb_msc_sdmmc_config_t){
        .card = *card,
        .callback_mount_changed = storage_mount_changed_cb,  
        .mount_config.max_files = 5,
    };
    ESP_ERROR_CHECK(tinyusb_msc_storage_init_sdmmc(&config_sdmmc));

    ESP_LOGI(TAG, "USB MSC initialization");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &descriptor_config,
        .string_descriptor = string_desc_arr,
        .string_descriptor_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]),
        .external_phy = false,
        .configuration_descriptor = msc_fs_configuration_desc,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB MSC initialization DONE");
}


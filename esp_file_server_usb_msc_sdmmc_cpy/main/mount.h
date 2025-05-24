#ifndef MOUNT_H
#define MOUNT_H

#include "sd_protocol_types.h"

#define SDMMC_PIN_CMD   4
#define SDMMC_PIN_CLK   5
#define SDMMC_PIN_D0   10
#define SDMMC_PIN_D1   11
#define SDMMC_PIN_D2   12
#define SDMMC_PIN_D3   13

#define BASE_PATH "/sdcard"

esp_err_t mount_sdmmc(sdmmc_card_t **card);
esp_err_t umount_sdmmc(sdmmc_card_t *card);

#endif

#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#include "esp_err.h"
#include "diskio_impl.h"
#include "diskio_sdmmc.h"

// SDMMC pin
#define SDMMC_PIN_CMD   4
#define SDMMC_PIN_CLK   5
#define SDMMC_PIN_D0   10
#define SDMMC_PIN_D1   11
#define SDMMC_PIN_D2   12
#define SDMMC_PIN_D3   13

#define BASE_PATH "/sdcard"
#include "esp_vfs.h"

#define FILE_BUFSIZE  100 * 1024
#define MAX_FILE_PATH 50

struct file_server_buffer {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char buffer[FILE_BUFSIZE];
};

esp_err_t mount_sdmmc(sdmmc_card_t **card);
void wifi_init();
esp_err_t start_file_server();

#endif

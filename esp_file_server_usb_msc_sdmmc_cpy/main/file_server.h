#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#include "esp_err.h"
#include "diskio_impl.h"
#include "diskio_sdmmc.h"

#define FILE_BUFSIZE  100 * 1024
#define MAX_FILE_PATH 50

#include "esp_vfs.h"
#include "extran_event_callback.h"

struct file_server_buffer {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char buffer[FILE_BUFSIZE];
};

esp_err_t start_file_server();
void mode_switch_req_cb_register(event_callback_t callback);

#endif


#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_err.h"
#include "esp_http_server.h"

esp_err_t wifi_init(const char* ssid, const char* pwd);
httpd_handle_t start_webserver(void);

#endif

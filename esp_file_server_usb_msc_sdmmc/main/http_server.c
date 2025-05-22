#include "http_server.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif_types.h"
#include "esp_wifi_types_generic.h"
#include "nvs_flash.h"
#include "esp_wifi_default.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"

static const char* TAG = "HTTP";

void wifi_event_handler(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data){
    if(base == WIFI_EVENT){
        if(id == WIFI_EVENT_STA_CONNECTED) ESP_LOGI(TAG, "Connected to WIFI");
        if(id == WIFI_EVENT_STA_START) ESP_LOGI(TAG, "Wifi STA started");

    }
    else if (base == IP_EVENT){
        if (id == IP_EVENT_STA_GOT_IP){
            // ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
            ESP_LOGE("IP_EVENT ", "GOT IP: " IPSTR, IP2STR(&((ip_event_got_ip_t*)event_data)->ip_info.ip));
        }
    }
}


esp_err_t wifi_init(const char* ssid, const char* pwd){
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_netif_create_default_wifi_sta();


    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_got_ip);

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();

    esp_wifi_init(&wifi_init_config);

    // change default configuration to max out wifi performance
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_bandwidth(WIFI_MODE_STA, WIFI_BW_HT40);
    esp_wifi_set_max_tx_power(70);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);


    wifi_config_t wifi_config = {};
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    wifi_config.sta.failure_retry_cnt = 10;

    strncpy((char*)wifi_config.sta.ssid, ssid, strlen(ssid));
    strncpy((char*)wifi_config.sta.password, pwd, strlen(pwd));
    printf("ssid %s\n\r", wifi_config.sta.ssid);
    printf("password %s\n\r", wifi_config.sta.password);
    // wifi_config.sta.ssid[strlen(ssid)] = '\0';
    // wifi_config.sta.password[strlen(pwd)] = '\0';

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
    return ESP_OK;
}

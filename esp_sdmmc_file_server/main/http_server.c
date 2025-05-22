#include "esp_compiler.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "file_server.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "esp_http_server.h"
#include "http_parser.h"
#include "sys/dirent.h"

static const char* TAG = "Server";
const char *upload =
    "<div style=\"border:solid;padding:0.5em;\"><h2 for=\"newfile\">Upload file</h2>"
    "<input id=\"newfile\" type=\"file\" onchange=\"setpath()\">"
    "<label for=\"filepath\"></label>"
    "<input id=\"filepath\" type=\"text\" placeholder=\"Set path on server\" style=\"margin-right:1em;width:25em;\">"
    "<button id=\"upload\" type=\"button\" onclick=\"upload()\">Upload</button>"
    "<script>"
    "function setpath(){document.getElementById(\"filepath\").value=document.getElementById(\"newfile\").files[0].name;}"
    "function upload(){"
    "var filePath=document.getElementById(\"filepath\").value;"
    "var upload_path=\"/upload/\"+filePath;"
    "var file=document.getElementById(\"newfile\").files[0];"
    "var MAX_SIZE=200*1024;var MAX_STR=\"200KB\";"
    "if(!file){alert(\"No file selected!\");}"
    "else if(filePath.length==0){alert(\"File path on server is not set!\");}"
    "else if(filePath.indexOf(' ')>=0){alert(\"File path on server cannot have spaces!\");}"
    "else if(filePath[filePath.length-1]=='/'){alert(\"File name not specified after path!\");}"
    "else{"
    "document.getElementById(\"newfile\").disabled=true;"
    "document.getElementById(\"filepath\").disabled=true;"
    "document.getElementById(\"upload\").disabled=true;"
    "var xhttp=new XMLHttpRequest();"
    "xhttp.onreadystatechange=function(){"
    "if(xhttp.readyState==4){"
    "if(xhttp.status==200){document.open();document.write(xhttp.responseText);document.close();}"
    "else if(xhttp.status==0){alert(\"Server closed the connection abruptly!\");location.reload()}"
    "else{alert(xhttp.status+\" Error!\\n\"+xhttp.responseText);location.reload()}}};"
    "xhttp.open(\"POST\",upload_path,true);xhttp.send(file);}}"
    "</script></div>";


static int wifi_retry_cnt = 0;
void wifi_event_handler(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    printf("ID: %s %d\n", base, (int)id);
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_CONNECTED){
        ESP_LOGI("W_EVENT", "%s", "connected");
    } else if(base == WIFI_EVENT && id == WIFI_EVENT_STA_START){
        esp_wifi_connect();
    } else if(base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED){
        if(wifi_retry_cnt <= 10){
            esp_wifi_connect();
            ESP_LOGW(TAG, "Wifi retry %d.", wifi_retry_cnt++);
        } else {
            ESP_LOGW(TAG, "Wifi exceed retry, reboot.");
            esp_restart();
        }
    }

    else if (base == IP_EVENT){
        if (id == IP_EVENT_STA_GOT_IP){
            ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
            ESP_LOGI("IP_EVENT ", "Got ip: " IPSTR, IP2STR(&event->ip_info.ip));

            // start file server when obtain IP from AP
            start_file_server();
        }
    }
}

void wifi_init(){

    esp_netif_create_default_wifi_sta();

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_got_ip);


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_mode(WIFI_MODE_STA);

    // change default configuration to max out wifi performance
    esp_wifi_set_bandwidth(WIFI_MODE_STA, WIFI_BW_HT40);
    esp_wifi_set_max_tx_power(70);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "HCTL",
            .password = "123456789HCTL",
            .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

esp_err_t index_redirect_handler(httpd_req_t *r){
    httpd_resp_set_status(r, "307 Temporary redirect");
    httpd_resp_set_hdr(r, "Location", "/");
    httpd_resp_send(r, NULL, 0);

    return ESP_OK;
}


static const char* get_path_from_uri(char* dest, const char* base_path ,const char* uri, size_t dest_size){
    const size_t base_path_len = strlen(base_path);
    size_t uri_len = strlen(uri);

    const char* quest = strchr(uri, '?');
    if(quest){
        uri_len = MIN(uri_len, quest - uri);
    }

    const char* hash = strchr(uri, '#');
    if(hash){
        uri_len = MIN(uri_len, hash - uri);
    }

    if(base_path_len + uri_len + 1 > dest_size){
        // path wont fit into dest
        return NULL;
    }

    strcpy(dest, base_path);
    strlcpy(dest + base_path_len, uri, uri_len + 1);

    ESP_LOGI(TAG, "full dest %s", dest);
    ESP_LOGI(TAG, "path %s", dest + base_path_len);

    return dest + base_path_len;
}

int test1(char* dest, const char* src){

    printf("%s %s\n", dest, src);
    char* p = strchr(src, '%');
    if(!p || (sizeof(dest) < sizeof(src)) || !(p[1]=='2') || !(p[2]=='0')){
        return 1;
    }

    strncpy(dest, src, p - src);
    strcat(dest, " ");

    p = &p[3];

    while(1){
        char* _p = strchr(p, '%');
        if(_p) {
            strncat(dest, p, _p - p);
            strcat(dest, " ");
            p = &_p[3];
        }
        else{
            strcat(dest, p);
            break;
        }
    }
    printf("%s\n", dest);
    return 0;
}



// check file extension
#define EXT_CHECK(filename, ext) \
    strcasecmp(&filename[strlen(filename) - strlen(ext)], ext)

esp_err_t set_content_type_from_file(httpd_req_t *r, const char* filename){
    if(EXT_CHECK(filename, ".pdf") == 0) {
        return httpd_resp_set_type(r, "application/pdf");
    } else if(EXT_CHECK(filename, ".html") == 0){
        return httpd_resp_set_type(r, "text/html");
    } else if(EXT_CHECK(filename, ".jpeg") == 0){
        return httpd_resp_set_type(r, "image/jpeg");
    } else if(EXT_CHECK(filename, ".png") == 0){
        return httpd_resp_set_type(r, "image/png");
    }
    return httpd_resp_set_type(r, "text/plain");
}

esp_err_t esp_resp_dir_html(httpd_req_t *r, const char* file_path){
    ESP_LOGI(TAG, "%s", file_path);

    DIR *dir = opendir(file_path);

    struct dirent *entry;

    if(!dir){
        ESP_LOGE(TAG, "Read path, invalid.");
        return ESP_FAIL;
    }
    httpd_resp_sendstr_chunk(r, "<!DOCTYPE html><html><body>");
    httpd_resp_sendstr_chunk(r, upload);

    while ((entry = readdir(dir)) != NULL) {
        httpd_resp_sendstr_chunk(r, "<form style=\"background: #f0f0f0; margin: 5px 0 5px; display:flex; width:100%;\" method=\"post\" action=\"/delete");
        httpd_resp_sendstr_chunk(r, r->uri);
        httpd_resp_sendstr_chunk(r, entry->d_name);

        httpd_resp_sendstr_chunk(r, "\"><a href=\"");
        httpd_resp_sendstr_chunk(r, r->uri);
        httpd_resp_sendstr_chunk(r, entry->d_name);

        if(entry->d_type == DT_DIR) httpd_resp_sendstr_chunk(r, "/");
        httpd_resp_sendstr_chunk(r, "\">");

        // printf("%s\n\r", entry->d_name);

        httpd_resp_sendstr_chunk(r, entry->d_name);
        if(entry->d_type == DT_DIR) httpd_resp_sendstr_chunk(r, "/..");
        httpd_resp_sendstr_chunk(r, "</a><button style=\"margin-left:1em;\"type=\"submit\">Delete</button></form>");
    }

    // ESP_LOGI(TAG, "Close dir");
    closedir(dir);
    httpd_resp_sendstr_chunk(r, "</body></html>");
    httpd_resp_send_chunk(r, NULL, 0);


    ESP_LOGI(TAG, "Done!!!");
    return ESP_OK;
}

esp_err_t download_handler(httpd_req_t *r){
    esp_err_t ret = ESP_OK;
    char filepath[MAX_FILE_PATH];
    FILE *file = NULL;
    struct stat file_stat;

    const char* filename = get_path_from_uri(filepath, ((struct file_server_buffer*)r->user_ctx)->base_path, r->uri,sizeof(filepath));
    if(!filename) {
        ESP_LOGE(TAG, "filename TOO LONG");
        httpd_resp_send_err(r, HTTPD_500_INTERNAL_SERVER_ERROR, "File path TOO LONG");
        return ESP_FAIL;
    }

    if(strchr(filepath, '%')){
        char fix_filepath[sizeof(filepath)];
        strncpy(fix_filepath, filepath, sizeof(filepath));
        memset(filepath, 0, sizeof(filepath));
        if(test1(filepath, fix_filepath)) strncpy(filepath, fix_filepath, sizeof(filepath));
    }

    if(filename[strlen(filename) - 1] == '/'){
        ESP_LOGI(TAG, "File name has trailing /");
        return esp_resp_dir_html(r, filepath);
    }

    if (strcmp(filename, "/favicon.ico") == 0) {
        ESP_LOGE(TAG, "ignore favicon");
        httpd_resp_send_err(r, HTTPD_404_NOT_FOUND, "No favicon.");
        return ESP_FAIL;
    }


    if(stat(filepath, &file_stat) == -1){
        ESP_LOGE(TAG, "file name invalid");
        httpd_resp_send_err(r, HTTPD_404_NOT_FOUND, "File does not exist");
        return ESP_FAIL;
    }

    file = fopen(filepath, "r");
    if(!file){
        ESP_LOGE(TAG, "Cannot open file.");
        httpd_resp_send_err(r, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file.");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Set content type");
    set_content_type_from_file(r, filename);
  

    ESP_LOGI(TAG, "Sending file %s, size %ld bytes", filename, file_stat.st_size);
    char* chunk = ((struct file_server_buffer*)r->user_ctx)->buffer;
    size_t chunksize;

    do {
        chunksize = fread(chunk, 1, FILE_BUFSIZE, file);
        if(chunksize > 0){
            if(httpd_resp_send_chunk(r, chunk, chunksize) != ESP_OK){
                fclose(file);
                ESP_LOGE(TAG, "Fail to send file, abort!");

                httpd_resp_sendstr_chunk(r, NULL);
                httpd_resp_send_err(r, HTTPD_500_INTERNAL_SERVER_ERROR, "Fail while sending file.");
                return ESP_FAIL;
            }
        }

    } while(chunksize != 0);

    fclose(file);

    httpd_resp_set_hdr(r, "Connection", "close");
    httpd_resp_send_chunk(r, NULL, 0);

    ESP_LOGI(TAG, "Handle URI complete.");

    return ret;
}


esp_err_t file_upload_handler(httpd_req_t *r){
    char filepath[MAX_FILE_PATH];
    struct stat file_stat;
    FILE *file = NULL;

    const char* filename = get_path_from_uri(filepath, ((struct file_server_buffer*)r->user_ctx)->base_path, r->uri + strlen("/upload"), MAX_FILE_PATH);

    if(!filename){
        httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "File name too long!");
        return ESP_FAIL;
    }

    if(filename[strlen(filename) - 1] == '/'){
        ESP_LOGE(TAG, "File name has trailing /.");
        httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "File name has /.");
        return ESP_FAIL;
    }

    if(stat(filename, &file_stat) == 0){
        ESP_LOGE(TAG, "File already exsist!");
        httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "File already exsist!");
        return ESP_FAIL;
    }

    file = fopen(filepath, "w");
    if(!file){
        ESP_LOGE(TAG, "Cannot create file. %s", filepath);
        httpd_resp_send_err(r, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file.");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Recieving file...");

    char* buffer = ((struct file_server_buffer*)r->user_ctx)->buffer;

    size_t remaining = r->content_len;
    size_t recieved = 0;
    while(remaining > 0){
        /* Recieve file part by part */
        if((recieved = httpd_req_recv(r, buffer, MIN(remaining, FILE_BUFSIZE))) <= 0 ){
            /* if timeout then continue */
            if(recieved == HTTPD_SOCK_ERR_TIMEOUT){
                ESP_LOGE(TAG, "Timeout, retry");
                continue;
            }

            /* if other error, terminate operation */
            fclose(file);
            unlink(filepath);

            ESP_LOGE(TAG, "File reception failed.");
            httpd_resp_send_err(r, HTTPD_500_INTERNAL_SERVER_ERROR, "File reception failed.");
            return ESP_OK;

        }

        /* write content to file */
        if(recieved && (recieved != fwrite(buffer, 1, recieved, file))){
            /* Write fail */
            fclose(file);
            unlink(filepath);

            ESP_LOGE(TAG, "Fail to write file to storage.");
            httpd_resp_send_err(r, HTTPD_500_INTERNAL_SERVER_ERROR, "Fail to write file to storage.");
            return ESP_OK;
        }

        remaining -= recieved;
    }

    /* Close file upon completion */
    fclose(file);
    ESP_LOGI(TAG, "File transfer completed.");

    httpd_resp_set_status(r, "303 see other");
    httpd_resp_set_hdr(r, "Location", "/");
    httpd_resp_set_hdr(r, "Connection", "close");
    httpd_resp_sendstr(r, "File successfully uploaded");

    return ESP_OK;
}

esp_err_t file_delete_handler(httpd_req_t *r){
    char filepath[MAX_FILE_PATH];
    struct stat file_stat;

    const char* filename = get_path_from_uri(filepath, ((struct file_server_buffer *)r->user_ctx)->base_path, r->uri + strlen("/delete"), sizeof(filepath));
    ESP_LOGI(TAG, "Filename %s\n", filename);

    if(!filename){
        ESP_LOGE(TAG, "Filename too long!");
        httpd_resp_send_err(r, HTTPD_414_URI_TOO_LONG, "Filename too long.");
    }

    if(filename[strlen(filename) - 1] == '/'){
        ESP_LOGE(TAG, "Filename can't have / trailing.");
        httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "Filename can't have / trailing.");
    }

    if(stat(filepath, &file_stat) == -1){
        ESP_LOGE(TAG, "File doesn't exsist.");
        httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "File doesn't exsist.");
    }
    
    ESP_LOGI(TAG, "File deleted.");
    httpd_resp_set_status(r, "303 see other");
    httpd_resp_set_hdr(r, "Location", "/");
    httpd_resp_set_hdr(r, "Connection", "close");
    httpd_resp_sendstr(r, "File successfully deleted");

    unlink(filepath);
    return ESP_OK;
}

esp_err_t start_file_server(){
    static struct file_server_buffer *server_buffer = NULL;

    if(server_buffer){
        ESP_LOGE(TAG, "Server already started");
        return ESP_ERR_INVALID_STATE;
    }

    // server_buffer = calloc(1, sizeof(struct file_server_buffer));
    server_buffer = (struct file_server_buffer*)heap_caps_malloc(sizeof(struct file_server_buffer), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    memset(server_buffer, 0, sizeof(struct file_server_buffer));


    if(!server_buffer){
        ESP_LOGE(TAG, "Cannot allocate buffer for file server.");
        return ESP_ERR_NO_MEM;
    }

    strlcpy(server_buffer->base_path, BASE_PATH,
            sizeof(server_buffer->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if(httpd_start(&server, &config) != ESP_OK){
        ESP_LOGE(TAG, "Failed to start file server...");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Server started");

    // handle root and download file.
    httpd_uri_t file_download = {
        .uri = "/*",
        .handler = download_handler,
        .method = HTTP_GET,
        .user_ctx = server_buffer,
    };
    httpd_register_uri_handler(server, &file_download);
    
    // handle file upload.
    httpd_uri_t file_upload = {
        .uri = "/upload/*",
        .handler = file_upload_handler,
        .method = HTTP_POST,
        .user_ctx = server_buffer,
    };
    httpd_register_uri_handler(server, &file_upload);
    
    /* handle file upload. */
    httpd_uri_t file_delete = {
        .uri = "/delete/*",
        .handler = file_delete_handler,
        .method = HTTP_POST,
        .user_ctx = server_buffer,
    };
    httpd_register_uri_handler(server, &file_delete);

    return ESP_OK;
}


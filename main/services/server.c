#include <unistd.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <cJSON.h>
#include <esp_ota_ops.h>
#include "server.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "model/updater.h"
#include "config/app_config.h"


static esp_err_t firmware_update_put_handler(httpd_req_t *req);
static void      set_firmware_update_state(firmware_update_state_tag_t state);
static void      firmware_update_failed(httpd_req_t *req, firmware_update_failure_code_t code, esp_err_t error);
static esp_err_t home_get_handler(httpd_req_t *req);


static const char             *TAG                   = "Server";
static httpd_handle_t          server                = NULL;
static SemaphoreHandle_t       sem                   = NULL;
static firmware_update_state_t firmware_update_state = {.tag = FIRMWARE_UPDATE_STATE_TAG_NONE};


void server_init(void) {
    static StaticSemaphore_t mutex_buffer;
    sem = xSemaphoreCreateMutexStatic(&mutex_buffer);
}


firmware_update_state_t server_firmware_update_state(void) {
    xSemaphoreTake(sem, portMAX_DELAY);
    firmware_update_state_t res = firmware_update_state;
    xSemaphoreGive(sem);
    return res;
}


void *server_start(void) {
    if (server != NULL) {
        return server;
    }

    httpd_config_t config   = HTTPD_DEFAULT_CONFIG();
    config.task_priority    = 1;
    config.stack_size       = APP_CONFIG_TASK_SIZE * 10;
    config.lru_purge_enable = true;
    config.max_uri_handlers = 2;
    config.max_open_sockets = CONFIG_LWIP_MAX_SOCKETS - 3;

    /* Start the httpd server */
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    esp_err_t res = httpd_start(&server, &config);
    if (res == ESP_OK) {
        httpd_uri_t home = {
            .uri     = "/",
            .method  = HTTP_GET,
            .handler = home_get_handler,
        };
        httpd_register_uri_handler(server, &home);

        // PUT /firmware_update
        const httpd_uri_t system_firmware_update = {
            .uri     = (const char *)"/firmware_update",
            .method  = HTTP_PUT,
            .handler = firmware_update_put_handler,
        };
        httpd_register_uri_handler(server, &system_firmware_update);

        return server;
    } else {
        ESP_LOGW(TAG, "Error starting server (0x%03X)!", res);
        return NULL;
    }
}


void server_stop(void) {
    /* Stop the httpd server */
    if (server != NULL) {
        httpd_stop(server);
        server = NULL;
    }
}


/*
 * PUT
 */


static esp_err_t firmware_update_put_handler(httpd_req_t *req) {
#define BUFFER_SIZE 2048

    const esp_partition_t *update_partition = NULL;
    esp_ota_handle_t       update_handle;
    esp_err_t              err = ESP_OK;
    int                    ret;

    set_firmware_update_state(FIRMWARE_UPDATE_STATE_TAG_UPDATING);
    vTaskDelay(pdMS_TO_TICKS(1200));     // Allow time for the application to display the update page

    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "esp_ota_get_next_update_partition failed!");
        firmware_update_failed(req, FIRMWARE_UPDATE_FAILURE_CODE_MISSING_PARTITION, ESP_OK);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%lx, request size %zu", update_partition->subtype,
             update_partition->address, req->content_len);

    // The following call takes about 1000ms
    if ((err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle) != ESP_OK)) {
        ESP_LOGE(TAG, "esp_ota_begin failed (0x%04X)!", err);
        firmware_update_failed(req, FIRMWARE_UPDATE_FAILURE_CODE_OTA_BEGIN, err);
        return ESP_FAIL;
    }

    char *buffer = malloc(BUFFER_SIZE);
    if (buffer == NULL) {
        firmware_update_failed(req, FIRMWARE_UPDATE_FAILURE_CODE_OOM, ESP_OK);
        return ESP_FAIL;
    }

    size_t total    = 0;
    size_t attempts = 0;
    while (total < req->content_len) {
        ret = httpd_req_recv(req, buffer, BUFFER_SIZE);

        if (ret == 0) {
            ESP_LOGI(TAG, "Received nothing, continue...");
            if (attempts++ > 10) {
                break;
            }
        } else if (ret > 0) {
            ESP_LOGI(TAG, "Reading %i bytes into flash...", ret);
            total += ret;
            attempts = 0;

            if ((err = esp_ota_write(update_handle, buffer, ret)) != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_write failed (0x%04X)!", err);
                free(buffer);
                esp_ota_abort(update_handle);
                firmware_update_failed(req, FIRMWARE_UPDATE_FAILURE_CODE_WRITE, err);
                return ESP_FAIL;
            } else {
                vTaskDelay(2);     // Delay for WDT
            }
        } else {
            ESP_LOGW(TAG, "Error while receiving ota: %i", ret);
            free(buffer);
            esp_ota_abort(update_handle);
            firmware_update_failed(req, FIRMWARE_UPDATE_FAILURE_CODE_RECEIVE, ret);
            return ESP_FAIL;
        }
    }
    free(buffer);

    if ((err = esp_ota_end(update_handle)) != ESP_OK) {
        // Invalid image
        ESP_LOGW(TAG, "Invalid OTA image (0x%X)", err);
        esp_ota_abort(update_handle);
        firmware_update_failed(req, FIRMWARE_UPDATE_FAILURE_CODE_IMAGE, err);
        return ESP_FAIL;
    }

    if ((err = esp_ota_set_boot_partition(update_partition) != ESP_OK)) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (0x%04X)!", err);
        httpd_resp_send_500(req);
        firmware_update_failed(req, FIRMWARE_UPDATE_FAILURE_CODE_BOOT_PARTITION, err);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Update successful! Reset the device.");
    httpd_resp_send(req, "", HTTPD_RESP_USE_STRLEN);
    httpd_resp_set_hdr(req, "Connection", "close");
    set_firmware_update_state(FIRMWARE_UPDATE_STATE_TAG_SUCCESS);
    return ESP_OK;
}


static esp_err_t home_get_handler(httpd_req_t *req) {
    extern const unsigned char index_html_start[] asm("_binary_index_html_start");
    extern const unsigned char index_html_end[] asm("_binary_index_html_end");
    const size_t               index_html_size = (index_html_end - index_html_start);

    //httpd_resp_set_hdr(req, "Content-encoding", "gzip");
    httpd_resp_send(req, (const char *)index_html_start, index_html_size);

    return ESP_OK;
}


static void set_firmware_update_state(firmware_update_state_tag_t state) {
    // Use firmware_update_failed for failure scenarios
    assert(state != FIRMWARE_UPDATE_STATE_TAG_FAILURE);
    xSemaphoreTake(sem, portMAX_DELAY);
    firmware_update_state.tag = state;
    xSemaphoreGive(sem);
}


static void firmware_update_failed(httpd_req_t *req, firmware_update_failure_code_t code, esp_err_t error) {
    xSemaphoreTake(sem, portMAX_DELAY);
    firmware_update_state.tag          = FIRMWARE_UPDATE_STATE_TAG_FAILURE;
    firmware_update_state.failure_code = code;
    firmware_update_state.error        = (int32_t)error;
    xSemaphoreGive(sem);

    char string[90] = {0};
    snprintf(string, sizeof(string), "{\"desc\":\"OTA error\",\"error\":3,\"step\":%i,\"code\":%i}", code, error);

    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, string);
}

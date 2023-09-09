#include <unistd.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <cJSON.h>
#include <esp_ota_ops.h>
#include "utils/utils.h"
#include "server.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "model/model.h"
#include "model/psychrometrics.h"
#include "config/app_config.h"
#include "ws_list.h"
#include "rainmaker/device.h"


#define JSON_TOTAL                     "total"
#define JSON_DATA                      "data"
#define JSON_TOTAL_FREE_BYTES          "total_free_bytes"
#define JSON_TOTAL_ALLOCATED_BYTES     "total_allocated_bytes"
#define JSON_LARGEST_FREE_BLOCK        "largest_free_block"
#define JSON_MINIMUM_FREE_BYTES        "minimum_free_bytes"
#define JSON_ALLOCATED_BLOCKS          "allocated_blocks"
#define JSON_FREE_BLOCKS               "free_blocks"
#define JSON_TOTAL_BLOCKS              "total_blocks"
#define JSON_UPTIME                    "uptime"
#define JSON_UPDATE                    "update"
#define JSON_USER_ID                   "userid"
#define JSON_SECRET_KEY                "secretkey"
#define JSON_NODE_ID                   "nodeid"
#define JSON_TEMPERATURE               "temp"
#define JSON_HUMIDITY                  "hum"
#define JSON_ABSOLUTE_HUMIDITY         "abs_hum"
#define JSON_RADIANT_SETPOINT          "rad_sp"
#define JSON_INTEGRATION_SETPOINT      "int_sp"
#define JSON_HUMIDITY_SETPOINT         "abs_hum_sp"
#define JSON_RADIANT_STATE             "rad_state"
#define JSON_INTEGRATION_STATE         "int_state"
#define JSON_DEHUMIDIFICATION_STATE    "dehum_state"
#define JSON_WORKING_MODE_RADIANT      "wm_rad"
#define JSON_WORKING_MODE_INTEGRATION  "wm_int"
#define JSON_WORKING_MODE_DEHUMIDIFIER "wm_dehum"
#define JSON_WORKING_MODE_VENTILATION  "wm_fan"
#define JSON_SUBSCRIBE                 "subscribe"
#define JSON_UNSUBSCRIBE               "unsubscribe"

#define CHECK_ALLOC(buffer, req)                                                                                       \
    ({                                                                                                                 \
        esp_err_t retval = ESP_OK;                                                                                     \
        if ((buffer) == NULL) {                                                                                        \
            ESP_LOGE(TAG, "%s", MEMORY_ERR_STRING);                                                                    \
            retval = ESP_FAIL;                                                                                         \
            if (req != NULL) {                                                                                         \
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, MEMORY_ERR_STRING);                          \
            }                                                                                                          \
        }                                                                                                              \
        retval;                                                                                                        \
    })

#define BAD_CONTENT_REQ_RESULT(r)                                                                                      \
    httpd_resp_set_hdr(r, "Content-Type", "application/json");                                                         \
    httpd_resp_set_hdr(r, "Connection", "close");                                                                      \
    httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, CONTENT_ERR_STRING);

#define OOM_REQ_RESULT(r)                                                                                              \
    httpd_resp_set_hdr(r, "Content-Type", "application/json");                                                         \
    httpd_resp_set_hdr(r, "Connection", "close");                                                                      \
    httpd_resp_send_err(r, HTTPD_500_INTERNAL_SERVER_ERROR, MEMORY_ERR_STRING);

#define INTERNAL_ERROR_REQ_RESULT(r)                                                                                   \
    httpd_resp_set_hdr(r, "Content-Type", "application/json");                                                         \
    httpd_resp_set_hdr(r, "Connection", "close");                                                                      \
    httpd_resp_send_err(r, HTTPD_500_INTERNAL_SERVER_ERROR, INTERNAL_ERR_STRING);


static esp_err_t monitoring_get_handler(httpd_req_t *req);
static esp_err_t firmware_update_put_handler(httpd_req_t *req);
static esp_err_t user_node_mapping_post_handler(httpd_req_t *req);
static void      close_cb(httpd_handle_t hd, int sockfd);
static esp_err_t open_cb(httpd_handle_t hd, int sockfd);
static void      set_firmware_update_state(firmware_update_state_tag_t state);
static void      firmware_update_failed(httpd_req_t *req, firmware_update_failure_code_t code, esp_err_t error);
static esp_err_t check_json_content_type(httpd_req_t *req);
static cJSON    *json_create_from_request(httpd_req_t *req);


static const char *TAG                 = "Server";
static const char *MEMORY_ERR_STRING   = "{\"desc\":\"Could not allocate memory!\",\"error\":1}";
static const char *CONTENT_ERR_STRING  = "{\"desc\":\"Invalid content type\",\"error\":2}";
static const char *INTERNAL_ERR_STRING = "{\"desc\":\"Internal error\",\"error\":3}";


static httpd_handle_t server = NULL;

static SemaphoreHandle_t sem = NULL;

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


void server_firmware_update_reset(void) {
    xSemaphoreTake(sem, portMAX_DELAY);
    firmware_update_state.tag = FIRMWARE_UPDATE_STATE_TAG_NONE;
    xSemaphoreGive(sem);
}


httpd_handle_t server_start(void) {
    if (server != NULL) {
        return server;
    }

    httpd_config_t config   = HTTPD_DEFAULT_CONFIG();
    config.task_priority    = 1;
    config.stack_size       = APP_CONFIG_TASK_SIZE * 10;
    config.lru_purge_enable = true;
    config.max_uri_handlers = 3;
    config.close_fn         = close_cb;
    config.open_fn          = open_cb;
    config.max_open_sockets = CONFIG_LWIP_MAX_SOCKETS - 3;

    /* Start the httpd server */
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    esp_err_t res = httpd_start(&server, &config);
    if (res == ESP_OK) {
        // GET /monitoring
        static const httpd_uri_t monitoring = {
            .uri     = "/monitoring",
            .method  = HTTP_GET,
            .handler = monitoring_get_handler,
        };

        // PUT /firmware_update
        const httpd_uri_t system_firmware_update = {
            .uri     = (const char *)"/firmware_update",
            .method  = HTTP_PUT,
            .handler = firmware_update_put_handler,
        };
        httpd_register_uri_handler(server, &system_firmware_update);

        // POST /user_node_mapping
        const httpd_uri_t user_node_mapping = {
            .uri     = (const char *)"/user_node_mapping",
            .method  = HTTP_POST,
            .handler = user_node_mapping_post_handler,
        };
        httpd_register_uri_handler(server, &user_node_mapping);

        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &monitoring);

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
 * GET
 */

static esp_err_t monitoring_get_handler(httpd_req_t *req) {

    esp_err_t err        = ESP_OK;
    cJSON    *json       = NULL;
    char     *jsonstring = NULL;
    uint32_t  seconds    = get_millis() / 1000UL;
    /* not to be checked if used later */
    json = cJSON_CreateObject();

    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);

    err |= CHECK_ALLOC(cJSON_AddNumberToObject(json, JSON_TOTAL_FREE_BYTES, info.total_free_bytes), NULL);
    err |= CHECK_ALLOC(cJSON_AddNumberToObject(json, JSON_TOTAL_ALLOCATED_BYTES, info.total_allocated_bytes), NULL);
    err |= CHECK_ALLOC(cJSON_AddNumberToObject(json, JSON_LARGEST_FREE_BLOCK, info.largest_free_block), NULL);
    err |= CHECK_ALLOC(cJSON_AddNumberToObject(json, JSON_MINIMUM_FREE_BYTES, info.minimum_free_bytes), NULL);
    err |= CHECK_ALLOC(cJSON_AddNumberToObject(json, JSON_ALLOCATED_BLOCKS, info.allocated_blocks), NULL);
    err |= CHECK_ALLOC(cJSON_AddNumberToObject(json, JSON_FREE_BLOCKS, info.free_blocks), NULL);
    err |= CHECK_ALLOC(cJSON_AddNumberToObject(json, JSON_TOTAL_BLOCKS, info.total_blocks), NULL);
    err |= CHECK_ALLOC(cJSON_AddNumberToObject(json, JSON_UPTIME, seconds), NULL);

    jsonstring = cJSON_PrintUnformatted(json);
    err |= CHECK_ALLOC(jsonstring, req);

    if (ESP_OK == err) {
        httpd_resp_set_hdr(req, "Content-Type", "application/json");
        httpd_resp_set_hdr(req, "Connection", "close");
        httpd_resp_send(req, jsonstring, HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, MEMORY_ERR_STRING);
    }

    free(jsonstring);
    cJSON_Delete(json);

    return err;
}


/*
 * POST
 */


static esp_err_t user_node_mapping_post_handler(httpd_req_t *req) {
    cJSON *json = json_create_from_request(req);
    if (json == NULL) {
        BAD_CONTENT_REQ_RESULT(req);
        return ESP_FAIL;
    }

    cJSON *json_userid = cJSON_GetObjectItem(json, JSON_USER_ID);
    if (!cJSON_IsString(json_userid)) {
        BAD_CONTENT_REQ_RESULT(req);
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    cJSON *json_secretkey = cJSON_GetObjectItem(json, JSON_SECRET_KEY);
    if (!cJSON_IsString(json_secretkey)) {
        BAD_CONTENT_REQ_RESULT(req);
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    const char *nodeid = rainmaker_device_start_user_node_mapping(cJSON_GetStringValue(json_userid),
                                                                  cJSON_GetStringValue(json_secretkey));
    cJSON_Delete(json);
    if (nodeid == NULL) {
        INTERNAL_ERROR_REQ_RESULT(req);
        return ESP_FAIL;
    }

    size_t nodeid_len = strlen(nodeid);
    ESP_LOGI(TAG, "Allocating %zu bytes", nodeid_len);
    char *json_response = malloc(nodeid_len + 32);
    if (json_response == NULL) {
        OOM_REQ_RESULT(req);
        return ESP_FAIL;
    }

    snprintf(json_response, nodeid_len + 32, "{\"" JSON_NODE_ID "\":\"%s\"}", nodeid);

    httpd_resp_set_hdr(req, "Content-Type", "application/json");
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    free(json_response);
    return ESP_OK;
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

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x, request size %zu", update_partition->subtype,
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
                vTaskDelay(1);     // Delay for WDT
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


static void close_cb(httpd_handle_t hd, int sockfd) {
    if (ws_list_deactivate(sockfd)) {
        ESP_LOGI(TAG, "Deactivating websocket %i", sockfd);
    }
    close(sockfd);
}


static esp_err_t open_cb(httpd_handle_t hd, int sockfd) {
    ESP_LOGI(TAG, "New fd: %i", sockfd);
    return ESP_OK;
}


/**
 * @fn      json_create_from_request
 * @brief   Retieve data from request
 * @param   pointer to request
 * @return  pointer to cJSON
 * @author
 */
static cJSON *json_create_from_request(httpd_req_t *req) {
    esp_err_t err       = ESP_OK;
    cJSON    *json      = NULL;
    char     *buffer    = NULL;
    int       received  = 0;
    int       remaining = req->content_len;
    int       ret;

    if ((ESP_OK != (check_json_content_type(req))) || (req->content_len <= 0)) {
        err = ESP_FAIL;
    } else {
        buffer = malloc(req->content_len + 1);
        memset(buffer, 0, req->content_len + 1);

        while ((ESP_OK == err) && (remaining > 0)) {
            ret = httpd_req_recv(req, &buffer[received], remaining);
            if (ret <= 0) {
                if (ret != HTTPD_SOCK_ERR_TIMEOUT) {
                    err = ESP_FAIL;
                }
            } else {
                remaining -= ret;
                received += ret;
            }
        }

        json = cJSON_Parse(buffer);
        err |= CHECK_ALLOC(json, req);
    }

    if (err != ESP_OK) {
        cJSON_Delete(json);
    }

    free(buffer);

    return json;
}


/**
 * @fn check_json_content_type
 * @brief Check whether the request content type is json
 *
 * @param req
 * @return esp_err_t
 * @author Mattia Maldini
 */
static esp_err_t check_json_content_type(httpd_req_t *req) {
    size_t buf_len = httpd_req_get_hdr_value_len(req, "Content-Type");
    if (buf_len > 1) {
        char *buffer = malloc(buf_len);
        if (CHECK_ALLOC(buffer, req)) {
            return ESP_FAIL;
        }

        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Content-Type", buffer, buf_len) == ESP_OK) {
            if (strcmp(buffer, "application/json")) {
                ESP_LOGI(TAG, "buffer: %s", buffer);
                free(buffer);
                return ESP_FAIL;
            }
        }
        free(buffer);
    } else {
        return ESP_FAIL;
    }

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

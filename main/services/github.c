#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_https_ota.h"
#include <esp_ota_ops.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "github.h"
#include "esp_crt_bundle.h"
#include "json_inciter.h"
#include "model/model.h"



static esp_err_t http_event_handler(esp_http_client_event_t *evt);
static void      cleanup(void);
static uint8_t   extract_release(const char *buffer);
static esp_err_t ota_client_init_cb(esp_http_client_handle_t client);


static const char *URL_GET_LATEST_RELEASE = "https://api.github.com/repos/Maldus512/wt32-sc01-clock/releases/latest";

static const char *TAG = "Github";

static esp_http_client_handle_t client        = NULL;
static char                    *output_buffer = NULL;     // Buffer to store response of http request from event handler
static char                     name[32]      = {0};
static char                     asset_url[128] = {0};

static firmware_update_state_t firmware_update_state = {.tag = FIRMWARE_UPDATE_STATE_TAG_NONE};
static esp_https_ota_handle_t  update_handle;


void github_request_latest_release(mut_model_t *pmodel) {
    if (client != NULL) {
        return;
    }

    ESP_LOGI(TAG, "Requesting latest release from Github");
    /**
     * NOTE: All the configuration parameters for http_client must be spefied either in URL or as host and path
     * parameters. If host and path parameters are not set, query parameter will be ignored. In such cases, query
     * parameter should be specified in URL.
     *
     * If URL as well as host and path parameters are specified, values of host and path will be considered.
     */
    esp_http_client_config_t config = {
        .url               = URL_GET_LATEST_RELEASE,
        .event_handler     = http_event_handler,
        .method            = HTTP_METHOD_GET,
        .is_async          = 1,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Accept", "application/vnd.github+json");
    esp_http_client_set_header(client, "X-GitHub-Api-Version", "2022-11-28");
    model_set_latest_release_state(pmodel, HTTP_REQUEST_STATE_WAITING, 0, 0, 0);
}


void github_ota(mut_model_t *pmodel) {
    if (client != NULL) {
        // return;
    }

    esp_http_client_config_t http_config = {
        .url               = asset_url,
        .method            = HTTP_METHOD_GET,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size_tx    = 512 + 256,
    };

    esp_err_t err = ESP_OK;

    esp_https_ota_config_t ota_config = {
        .http_config         = &http_config,
        .http_client_init_cb = ota_client_init_cb,
    };

    ESP_LOGI(TAG, "Starting HTTPS OTA for %s", asset_url);
    err = esp_https_ota_begin(&ota_config, &update_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS OTA started");
        firmware_update_state.tag = FIRMWARE_UPDATE_STATE_TAG_UPDATING;
    } else {
        ESP_LOGE(TAG, "HTTPS OTA failed!");
        firmware_update_state.tag          = FIRMWARE_UPDATE_STATE_TAG_FAILURE;
        firmware_update_state.failure_code = FIRMWARE_UPDATE_FAILURE_CODE_OTA_BEGIN;
        firmware_update_state.error        = err;
    }

    pmodel->run.client_firmware_update_state = firmware_update_state;
}


uint8_t github_manage(mut_model_t *pmodel) {
    uint8_t update = 0;

    if (client != NULL) {
        esp_err_t err = esp_http_client_perform(client);

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", (int)esp_http_client_get_status_code(client),
                     (int)esp_http_client_get_content_length(client));
            ESP_LOGI(TAG, "%s", output_buffer);
            if (extract_release(output_buffer)) {
                cleanup();
                int major = 0;
                int minor = 0;
                int patch = 0;
                if (sscanf(name, "v%i.%i.%i", &major, &minor, &patch) == 3) {
                    model_set_latest_release_state(pmodel, HTTP_REQUEST_STATE_DONE, major, minor, patch);
                } else if (sscanf(name, "%i.%i.%i", &major, &minor, &patch) == 3) {
                    model_set_latest_release_state(pmodel, HTTP_REQUEST_STATE_DONE, major, minor, patch);
                } else {
                    model_set_latest_release_state(pmodel, HTTP_REQUEST_STATE_ERROR, 0, 0, 0);
                }
            } else {
                cleanup();
                model_set_latest_release_state(pmodel, HTTP_REQUEST_STATE_ERROR, 0, 0, 0);
            }
            update = 1;
        }
        // Failure
        else if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
            model_set_latest_release_state(pmodel, HTTP_REQUEST_STATE_ERROR, 0, 0, 0);
            cleanup();
            update = 1;
        }
    } else if (firmware_update_state.tag == FIRMWARE_UPDATE_STATE_TAG_UPDATING) {
        esp_err_t err = esp_https_ota_perform(update_handle);
        switch (err) {
            case ESP_ERR_HTTPS_OTA_IN_PROGRESS:
                break;

            case ESP_OK: {
                if (esp_https_ota_is_complete_data_received(update_handle)) {
                    if (esp_https_ota_finish(update_handle) == ESP_OK) {
                        firmware_update_state.tag = FIRMWARE_UPDATE_STATE_TAG_SUCCESS;
                    } else {
                        firmware_update_state.tag          = FIRMWARE_UPDATE_STATE_TAG_FAILURE;
                        firmware_update_state.failure_code = FIRMWARE_UPDATE_FAILURE_CODE_IMAGE;
                        firmware_update_state.error        = ESP_OK;
                    }
                } else {
                    firmware_update_state.tag          = FIRMWARE_UPDATE_STATE_TAG_FAILURE;
                    firmware_update_state.failure_code = FIRMWARE_UPDATE_FAILURE_CODE_IMAGE;
                    firmware_update_state.error        = ESP_OK;
                }
                break;
            }

            case ESP_ERR_OTA_VALIDATE_FAILED:
                firmware_update_state.tag          = FIRMWARE_UPDATE_STATE_TAG_FAILURE;
                firmware_update_state.failure_code = FIRMWARE_UPDATE_FAILURE_CODE_IMAGE;
                firmware_update_state.error        = err;
                break;
            case ESP_ERR_NO_MEM:
                firmware_update_state.tag          = FIRMWARE_UPDATE_STATE_TAG_FAILURE;
                firmware_update_state.failure_code = FIRMWARE_UPDATE_FAILURE_CODE_OOM;
                firmware_update_state.error        = err;
                break;
            case ESP_ERR_FLASH_BASE:
                firmware_update_state.tag          = FIRMWARE_UPDATE_STATE_TAG_FAILURE;
                firmware_update_state.failure_code = FIRMWARE_UPDATE_FAILURE_CODE_WRITE;
                firmware_update_state.error        = err;
                break;
            default:
                firmware_update_state.tag          = FIRMWARE_UPDATE_STATE_TAG_FAILURE;
                firmware_update_state.failure_code = FIRMWARE_UPDATE_FAILURE_CODE_RECEIVE;
                firmware_update_state.error        = err;
                break;
        }
        pmodel->run.client_firmware_update_state = firmware_update_state;
        update                                   = 1;
    }

    return update;
}


static esp_err_t ota_client_init_cb(esp_http_client_handle_t client) {
    esp_http_client_set_header(client, "Accept", "application/octet-stream");
    esp_http_client_set_header(client, "X-GitHub-Api-Version", "2022-11-28");

    return ESP_OK;
}


static uint8_t extract_release(const char *buffer) {
    json_inciter_element_t root_element;
    json_inciter_parse_value(buffer, &root_element);

    json_inciter_element_t name_element = {0};
    if (json_inciter_find_value_in_object(root_element, "name", &name_element) == JSON_INCITER_OK) {
        if (name_element.tag == JSON_INCITER_ELEMENT_TAG_STRING) {
            json_inciter_copy_content(name, sizeof(name), name_element);
            ESP_LOGI(TAG, "Name found: %s", name);
        } else {
            return 0;
        }
    } else {
        return 0;
    }

    json_inciter_element_t assets_element = {0};
    if (json_inciter_find_value_in_object(root_element, "assets", &assets_element) == JSON_INCITER_OK) {
        if (assets_element.tag == JSON_INCITER_ELEMENT_TAG_ARRAY) {
            json_inciter_t json_inciter_state = JSON_INCITER_OK;
            const char    *assets_buffer      = json_inciter_element_content_start(assets_element);
            do {
                json_inciter_element_t value;
                json_inciter_parse_value(assets_buffer, &value);

                if (value.tag != JSON_INCITER_ELEMENT_TAG_OBJECT) {
                    break;
                }
                /* <3 <-- me wife did that */

                json_inciter_element_t download_url_element = {0};
                if (json_inciter_find_value_in_object(value, "url", &download_url_element) == JSON_INCITER_OK) {
                    json_inciter_copy_content(asset_url, sizeof(asset_url), download_url_element);
                    ESP_LOGI(TAG, "Url found: %s", asset_url);
                    return 1;
                }

                assets_buffer = JSON_INCITER_ELEMENT_NEXT_START(value);
                json_inciter_state =
                    json_inciter_next_element_start(assets_buffer, JSON_INCITER_ELEMENT_TAG_ARRAY, &assets_buffer);
            } while (json_inciter_state == JSON_INCITER_OK);
        }
    }

    return 0;
}


static void cleanup(void) {
    if (client != NULL) {
        esp_http_client_cleanup(client);
        client = NULL;
    }
    if (output_buffer != NULL) {
        free(output_buffer);
        output_buffer = NULL;
    }
}


static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    static int output_len;     // Stores number of bytes read
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary
             * data. However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
                        output_len    = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                    ESP_LOGI(TAG, "Copied %i bytes", evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int       mbedtls_err = 0;
            esp_err_t err         = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            break;
    }
    return ESP_OK;
}

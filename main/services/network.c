#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "network.h"
#include "server.h"
#include "model/updater.h"


static void      network_event_handler(void *arg, esp_event_base_t event_base, int event_id, void *event_data);
static void      network_connected(uint32_t ip);
static void      network_disconnected(void);
static void      network_connecting(void);
static esp_err_t scan_networks(uint8_t channel);


static const uint32_t EVENT_CONNECTED  = BIT0;
static const uint32_t EVENT_STOPPED    = BIT1;
static const uint32_t EVENT_SCAN_RETRY = BIT3;
static const uint32_t EVENT_SCAN_DONE  = BIT4;


static const char *TAG = "Network";

static EventGroupHandle_t wifi_event_group               = NULL;
static SemaphoreHandle_t  sem                            = NULL;
static uint32_t           ip_address                     = 0;
static wifi_state_t       wifi_state                     = WIFI_STATE_DISCONNECTED;
static uint16_t           ap_list_count                  = 0;
static wifi_ap_record_t   ap_list[MAX_AP_SCAN_LIST_SIZE] = {0};
static uint8_t            connect_after_stop             = 0;


void network_init(void) {
    static StaticEventGroup_t event_group_buffer;
    wifi_event_group = xEventGroupCreateStatic(&event_group_buffer);
    static StaticSemaphore_t semaphore_buffer;
    sem = xSemaphoreCreateMutexStatic(&semaphore_buffer);

    /* Initialize networking stack */
    ESP_ERROR_CHECK(esp_netif_init());
    /* Create default event loop needed by the
     * main app and the provisioning service */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize Wi-Fi with default config */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Set our event handling */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, network_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, network_event_handler, NULL));

    esp_netif_create_default_wifi_sta();
}


void network_connect_to(char *ssid, char *psk) {
    ESP_LOGI(TAG, "Trying to connect to %s, %s", ssid, psk);
    wifi_config_t config = {0};
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_get_config(WIFI_IF_STA, &config));
    strcpy((char *)config.sta.ssid, ssid);
    strcpy((char *)config.sta.password, psk);
    esp_wifi_set_config(WIFI_IF_STA, &config);

    xSemaphoreTake(sem, portMAX_DELAY);
    switch (wifi_state) {
        case WIFI_STATE_DISCONNECTED:
            esp_wifi_connect();
            break;
        case WIFI_STATE_CONNECTING:
            connect_after_stop = 1;
            esp_wifi_stop();
            break;
        case WIFI_STATE_CONNECTED:
            esp_wifi_disconnect();
            break;
    }
    xSemaphoreGive(sem);
}


void network_start_sta(void) {
    /* Start Wi-Fi in station mode with credentials set during provisioning */
    wifi_config_t config = {0};
    esp_wifi_get_config(WIFI_IF_STA, &config);
    ESP_LOGI(TAG, "Starting connection for %s %s", config.sta.ssid, config.sta.password);

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_start());
}


void network_stop(void) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_stop());
}


void network_scan_access_points(uint8_t channel) {
    xEventGroupClearBits(wifi_event_group, EVENT_SCAN_DONE);
    if (scan_networks(channel) != ESP_OK) {
        ESP_LOGI(TAG, "Temporarily unable to scan");
        xEventGroupSetBits(wifi_event_group, EVENT_SCAN_RETRY);
    }
}


int network_get_scan_result(model_updater_t updater) {
    uint32_t res = xEventGroupWaitBits(wifi_event_group, EVENT_SCAN_DONE, pdFALSE, pdTRUE, 0);
    if ((res & EVENT_SCAN_DONE) > 0) {
        xSemaphoreTake(sem, portMAX_DELAY);
        model_updater_clear_aps(updater);
        for (size_t i = 0; i < ap_list_count; i++) {
            model_updater_add_ap(updater, (char *)ap_list[i].ssid, ap_list[i].rssi);
        }
        model_updater_set_scanning(updater, 0);
        xSemaphoreGive(sem);
        xEventGroupClearBits(wifi_event_group, EVENT_SCAN_DONE);
        return 1;
    } else {
        return 0;
    }
}


void network_get_state(model_updater_t updater) {
    xSemaphoreTake(sem, portMAX_DELAY);
    if ((wifi_state == WIFI_STATE_CONNECTED || wifi_state == WIFI_STATE_CONNECTING)) {
        wifi_config_t config = {0};
        esp_wifi_get_config(WIFI_IF_STA, &config);
        model_updater_update_wifi_state(updater, (const char *)config.sta.ssid, ip_address, wifi_state);
    } else {
        model_updater_update_wifi_state(updater, NULL, 0, wifi_state);
    }
    xSemaphoreGive(sem);
}


static void network_event_handler(void *arg, esp_event_base_t event_base, int event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START: {
                wifi_mode_t mode = WIFI_MODE_STA;
                ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_get_mode(&mode));
                if (mode == WIFI_MODE_STA) {
                    wifi_config_t config = {0};
                    esp_wifi_get_config(WIFI_IF_STA, &config);

                    ESP_LOGI(TAG, "STA started");
                    if (strlen((char *)config.sta.ssid) > 0) {
                        xSemaphoreTake(sem, portMAX_DELAY);
                        wifi_state = WIFI_STATE_CONNECTING;
                        xSemaphoreGive(sem);
                        esp_wifi_connect();
                    } else {
                        ESP_LOGI(TAG, "No saved network");
                    }
                } else {
                    ESP_LOGW(TAG, "STA started in APSTA MODE");
                }
                break;
            }

            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "Access Point started");
                xEventGroupClearBits(wifi_event_group, EVENT_STOPPED);
                break;

            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "AP Stopped");
                xEventGroupSetBits(wifi_event_group, EVENT_STOPPED);
                break;

            case WIFI_EVENT_STA_STOP:
                ESP_LOGI(TAG, "STA Stopped");
                network_disconnected();
                xEventGroupSetBits(wifi_event_group, EVENT_STOPPED);

                xSemaphoreTake(sem, portMAX_DELAY);
                if (connect_after_stop) {
                    connect_after_stop = 0;
                    esp_wifi_start();
                }
                xSemaphoreGive(sem);
                break;

            case WIFI_EVENT_AP_STACONNECTED:
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG, "Device connected to (or disconnected from) our AP");
                break;

            case WIFI_EVENT_SCAN_DONE: {
                uint16_t number = MAX_AP_SCAN_LIST_SIZE;

                xSemaphoreTake(sem, portMAX_DELAY);
                esp_err_t err = esp_wifi_scan_get_ap_records(&number, ap_list);
                if (err == ESP_OK) {
                    if (number >= MAX_AP_SCAN_LIST_SIZE) {
                        number = MAX_AP_SCAN_LIST_SIZE;
                    }

                    ap_list_count = number;
                } else {
                    number = 0;
                }
                xSemaphoreGive(sem);

                ESP_LOGI(TAG, "Wifi scan done, %i networks found", number);
                xEventGroupSetBits(wifi_event_group, EVENT_SCAN_DONE);

                uint8_t should_rescan = xEventGroupGetBits(wifi_event_group) & EVENT_SCAN_RETRY;

                if (should_rescan) {
                    xEventGroupClearBits(wifi_event_group, EVENT_SCAN_RETRY);
                    network_connecting();
                    esp_wifi_connect();
                }
                break;
            }

            case WIFI_EVENT_STA_CONNECTED: {
                ESP_LOGI(TAG, "Connected to a network");
                break;
            }

            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;

                switch (disconnected->reason) {
                    case WIFI_REASON_AUTH_EXPIRE:
                    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                    case WIFI_REASON_BEACON_TIMEOUT:
                    case WIFI_REASON_AUTH_FAIL:
                    case WIFI_REASON_ASSOC_FAIL:
                    case WIFI_REASON_HANDSHAKE_TIMEOUT:
                        ESP_LOGW(TAG, "connect to the AP fail : auth Error.");
                        break;

                    case WIFI_REASON_NO_AP_FOUND:
                        ESP_LOGW(TAG, "connect to the AP fail : not found.");
                        break;

                    default:
                        /* None of the expected reasons */
                        break;
                }

                uint8_t should_scan = xEventGroupGetBits(wifi_event_group) & EVENT_SCAN_RETRY;

                if (should_scan) {
                    ESP_LOGI(TAG, "Putting connection efforts aside to scan");
                    network_disconnected();
                    scan_networks(0);
                } else {
                    // Retry indefinitely
                    ESP_LOGI(TAG, "Retrying...");
                    network_connecting();
                    esp_wifi_connect();
                }
                break;
            }

            default:
                ESP_LOGI(TAG, "Unknown WiFi event %i", event_id);
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                ESP_LOGI(TAG, "Got ip:%s", ip4addr_ntoa((const ip4_addr_t *)&event->ip_info.ip));

                xEventGroupClearBits(wifi_event_group, EVENT_STOPPED);
                network_connected(event->ip_info.ip.addr);
                break;
            }

            default:
                ESP_LOGI(TAG, "Unknown IP event %i", event_id);
                break;
        }
    }
}


static void network_connected(uint32_t ip) {
    xEventGroupSetBits(wifi_event_group, EVENT_CONNECTED);
    xSemaphoreTake(sem, portMAX_DELAY);
    ip_address = ip;
    wifi_state = WIFI_STATE_CONNECTED;
    xSemaphoreGive(sem);
    server_start();
}


static void network_disconnected(void) {
    xEventGroupClearBits(wifi_event_group, EVENT_CONNECTED);
    xSemaphoreTake(sem, portMAX_DELAY);
    wifi_state = WIFI_STATE_DISCONNECTED;
    xSemaphoreGive(sem);
    server_stop();
}


static void network_connecting(void) {
    xEventGroupClearBits(wifi_event_group, EVENT_CONNECTED);
    xSemaphoreTake(sem, portMAX_DELAY);
    wifi_state = WIFI_STATE_CONNECTING;
    xSemaphoreGive(sem);
}


static esp_err_t scan_networks(uint8_t channel) {
    wifi_scan_config_t config = {
        .ssid        = NULL,
        .bssid       = NULL,
        .channel     = channel,
        .show_hidden = 0,
        .scan_type   = WIFI_SCAN_TYPE_ACTIVE,
    };
    return esp_wifi_scan_start(&config, 0);
}

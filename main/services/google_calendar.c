#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <esp_log.h>
#include <esp_http_client.h>
#include "esp_tls.h"
#include "esp_crt_bundle.h"


#define MAX_TASK_MESSAGES      8
#define MAX_HTTP_OUTPUT_BUFFER 4096
#define MIN(x, y)              ((x) > (y) ? (y) : (x))


typedef enum {
    TASK_MESSAGE_EXAMPLE = 0,
} task_message_t;


static void      task(void *args);
static esp_err_t http_event_handler(esp_http_client_event_t *evt);


static const char   *TAG   = "GoogleCalendar";
static QueueHandle_t queue = NULL;


void google_calendar_init(void) {
    assert(queue == NULL);

    static StaticQueue_t  queue_buffer;
    static task_message_t queue_storage[MAX_TASK_MESSAGES];
    queue = xQueueCreateStatic(MAX_TASK_MESSAGES, sizeof(task_message_t), (uint8_t *)queue_storage, &queue_buffer);

    static uint8_t      stack_buffer[4096 * 2];
    static StaticTask_t task_buffer;
    xTaskCreateStatic(task, TAG, sizeof(stack_buffer), NULL, 1, stack_buffer, &task_buffer);

    ESP_LOGI(TAG, "Initialized");
}


void google_calendar_example(void) {
    task_message_t message = TASK_MESSAGE_EXAMPLE;
    xQueueSend(queue, &message, 0);
}


static void task(void *args) {
    (void)args;
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

    for (;;) {
        task_message_t message;
        if (xQueueReceive(queue, &message, portMAX_DELAY)) {
            switch (message) {
                case TASK_MESSAGE_EXAMPLE: {
                    esp_http_client_config_t config = {
                        .host          = "httpbin.org",
                        .path          = "/get",
                        .query         = "esp",
                        .event_handler = http_event_handler,
                        .user_data     = local_response_buffer,     // Pass address of local buffer to get response
                        .disable_auto_redirect = true,
                        .method                = HTTP_METHOD_GET,
                    };
                    esp_http_client_handle_t client = esp_http_client_init(&config);

                    // GET
                    esp_err_t err = esp_http_client_perform(client);
                    if (err == ESP_OK) {
                        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %" PRIu64,
                                 esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
                    } else {
                        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
                    }
                    ESP_LOGI(TAG, "%s", local_response_buffer);
                    break;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}


static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    static char *output_buffer;     // Buffer to store response of http request from event handler
    static int   output_len;        // Stores number of bytes read
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
                int copy_len = 0;
                if (evt->user_data) {
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    const int buffer_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        output_buffer = (char *)malloc(buffer_len);
                        output_len    = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (buffer_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int       mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
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
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

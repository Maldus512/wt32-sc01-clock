#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include "peripherals/storage.h"
#include "persistance.h"
#include <esp_log.h>


const char *PERSISTANCE_NORMAL_BRIGHTNESS_KEY  = "NORMALBR";
const char *PERSISTANCE_STANDBY_BRIGHTNESS_KEY = "STANDBYBR";
const char *PERSISTANCE_STANDBY_DELAY_KEY      = "STANDBYDELAY";


static const char *TAG = "Persistance";


void persistance_load(mut_model_t *pmodel) {
    storage_load_uint8(&pmodel->config.normal_brightness, (char *)PERSISTANCE_NORMAL_BRIGHTNESS_KEY);
    storage_load_uint8(&pmodel->config.standby_brightness, (char *)PERSISTANCE_STANDBY_BRIGHTNESS_KEY);
    storage_load_uint16(&pmodel->config.standby_delay_seconds, (char *)PERSISTANCE_STANDBY_DELAY_KEY);
}


void persistance_save_variable(void *old_value, const void *memory, uint16_t size, void *user_ptr, void *arg) {
    (void)old_value;
    (void)user_ptr;

    ESP_LOGI(TAG, "Saving variable %s of size %zu", (char *)arg, size);
    switch (size) {
        case sizeof(uint8_t):
            storage_save_uint8((uint8_t *)memory, arg);
            break;
        case sizeof(uint16_t):
            storage_save_uint16((uint16_t *)memory, arg);
            break;
        case sizeof(uint32_t):
            storage_save_uint32((uint32_t *)memory, arg);
            break;
        case sizeof(uint64_t):
            storage_save_uint64((uint64_t *)memory, arg);
            break;
        default:
            storage_save_blob((void *)memory, size, arg);
            break;
    }
}

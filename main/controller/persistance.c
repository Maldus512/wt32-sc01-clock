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
const char *PERSISTANCE_ALARM_NUM_KEY          = "ALARMNUM";
const char *PERSISTANCE_NIGHT_MODE_KEY         = "NIGHTMODE";
const char *PERSISTANCE_NIGHT_MODE_START_KEY   = "NIGHTSTART";
const char *PERSISTANCE_NIGHT_MODE_END_KEY     = "NIGHTEND";

static const char *ALARM_KEY_FMT = "ALARM%i";


static const char *TAG = "Persistance";


void persistance_load(mut_model_t *pmodel) {
    storage_load_uint8(&pmodel->config.normal_brightness, (char *)PERSISTANCE_NORMAL_BRIGHTNESS_KEY);
    storage_load_uint8(&pmodel->config.standby_brightness, (char *)PERSISTANCE_STANDBY_BRIGHTNESS_KEY);
    storage_load_uint16(&pmodel->config.standby_delay_seconds, (char *)PERSISTANCE_STANDBY_DELAY_KEY);
    storage_load_uint16(&pmodel->config.num_alarms, (char *)PERSISTANCE_ALARM_NUM_KEY);
    storage_load_uint8(&pmodel->config.night_mode, (char *)PERSISTANCE_NIGHT_MODE_KEY);
    storage_load_uint32(&pmodel->config.night_mode_start, (char *)PERSISTANCE_NIGHT_MODE_START_KEY);
    storage_load_uint32(&pmodel->config.night_mode_end, (char *)PERSISTANCE_NIGHT_MODE_END_KEY);

    for (size_t i = 0; i < pmodel->config.num_alarms; i++) {
        char string[32] = {0};
        snprintf(string, sizeof(string), ALARM_KEY_FMT, (int)i);
        storage_load_blob(&pmodel->config.alarms[i], sizeof(alarm_t), string);
    }
}


void persistance_save_variable(void *old_value, const void *memory, uint16_t size, void *user_ptr, void *arg) {
    (void)old_value;
    (void)user_ptr;

    ESP_LOGI(TAG, "Saving variable %s of size %zu", (char *)arg, (size_t)size);
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


void persistance_save_alarm(size_t alarm_num, alarm_t alarm) {
    char string[32] = {0};
    snprintf(string, sizeof(string), ALARM_KEY_FMT, (int)alarm_num);
    storage_save_blob(&alarm, sizeof(alarm), string);
}

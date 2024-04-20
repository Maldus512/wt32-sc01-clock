#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "model.h"
#include <esp_log.h>
#include "config/app_config.h"


#define SECONDS_IN_DAY (24UL * 60UL * 60UL)


static const char *TAG = "Model";


void model_init(mut_model_t *pmodel) {
    assert(pmodel != NULL);
    (void)TAG;

    memset(pmodel->config.alarms, 0, sizeof(pmodel->config.alarms));
    pmodel->config.military_time         = 1;
    pmodel->config.num_alarms            = 0;
    pmodel->config.normal_brightness     = 80;
    pmodel->config.standby_brightness    = 20;
    pmodel->config.standby_delay_seconds = 30;
    pmodel->config.night_mode            = 0;
    pmodel->config.night_mode_start      = 0;
    pmodel->config.night_mode_end        = 0;

    pmodel->run.ap_list_size                     = 0;
    pmodel->run.ip_addr                          = 0;
    pmodel->run.wifi_state                       = WIFI_STATE_DISCONNECTED;
    pmodel->run.scanning                         = 0;
    pmodel->run.server_firmware_update_state.tag = FIRMWARE_UPDATE_STATE_TAG_NONE;
    pmodel->run.client_firmware_update_state.tag = FIRMWARE_UPDATE_STATE_TAG_NONE;
    pmodel->run.new_release_notified             = 0;
    pmodel->run.latest_release_request_state     = HTTP_REQUEST_STATE_NONE;
    pmodel->run.latest_release_major             = 0;
    pmodel->run.latest_release_minor             = 0;
    pmodel->run.latest_release_patch             = 0;
    strcpy(pmodel->run.ssid, "");
}


uint8_t model_is_new_release_available(model_t *pmodel) {
    assert(pmodel != NULL);
    if (pmodel->run.latest_release_request_state == HTTP_REQUEST_STATE_DONE &&
        ((pmodel->run.latest_release_major > APP_CONFIG_FIRMWARE_VERSION_MAJOR) ||
         (pmodel->run.latest_release_major == APP_CONFIG_FIRMWARE_VERSION_MAJOR &&
          pmodel->run.latest_release_minor > APP_CONFIG_FIRMWARE_VERSION_MINOR) ||
         (pmodel->run.latest_release_major == APP_CONFIG_FIRMWARE_VERSION_MAJOR &&
          pmodel->run.latest_release_minor == APP_CONFIG_FIRMWARE_VERSION_MINOR &&
          pmodel->run.latest_release_patch > APP_CONFIG_FIRMWARE_VERSION_PATCH))) {
        return 1;
    } else {
        return 0;
    }
}


firmware_update_state_t model_get_firmware_update_state(model_t *pmodel) {
    assert(pmodel != NULL);
    if (pmodel->run.client_firmware_update_state.tag != FIRMWARE_UPDATE_STATE_TAG_NONE) {
        return pmodel->run.client_firmware_update_state;
    } else {
        return pmodel->run.server_firmware_update_state;
    }
}


uint8_t model_get_standby_brightness(model_t *pmodel) {
    assert(pmodel != NULL);
    // During FUP keep the brightness up to make sure the user can interact with the display
    if (model_get_firmware_update_state(pmodel).tag != FIRMWARE_UPDATE_STATE_TAG_NONE) {
        return pmodel->config.normal_brightness;
    } else if (pmodel->config.night_mode) {
        time_t    now    = time(NULL);
        struct tm now_tm = *localtime(&now);

        uint32_t seconds_now = now_tm.tm_hour * 60 * 60 + now_tm.tm_min * 60 + now_tm.tm_sec;

        if (pmodel->config.night_mode_start > pmodel->config.night_mode_end) {
            if (seconds_now > pmodel->config.night_mode_start || seconds_now < pmodel->config.night_mode_end) {
                return 0;
            } else {
                return pmodel->config.standby_brightness;
            }
        } else {
            if (seconds_now > pmodel->config.night_mode_start && seconds_now < pmodel->config.night_mode_end) {
                return 0;
            } else {
                return pmodel->config.standby_brightness;
            }
        }
    } else {
        return pmodel->config.standby_brightness;
    }
}


void model_set_latest_release_state(mut_model_t *pmodel, http_request_state_t request_state, uint16_t major,
                                    uint16_t minor, uint16_t patch) {
    assert(pmodel != NULL);
    pmodel->run.latest_release_request_state = request_state;
    pmodel->run.latest_release_major         = major;
    pmodel->run.latest_release_minor         = minor;
    pmodel->run.latest_release_patch         = patch;
}


void model_set_night_mode(mut_model_t *pmodel, uint8_t night_mode) {
    assert(pmodel != NULL);
    pmodel->config.night_mode = night_mode;
}


const char *model_get_ssid(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->run.ssid;
}


alarm_t model_get_alarm(model_t *pmodel, size_t num) {
    assert(pmodel != NULL);
    return pmodel->config.alarms[num];
}


uint8_t model_get_nth_alarm(model_t *pmodel, size_t *alarm_num, size_t nth) {
    assert(pmodel != NULL);
    size_t count = 0;

    for (size_t i = 0; i < pmodel->config.num_alarms; i++) {
        if (model_is_alarm_expired(pmodel, i)) {
            continue;
        }

        if (count == nth) {
            *alarm_num = i;
            return 1;
        } else {
            count++;
        }
    }

    return 0;
}


uint8_t model_get_nth_alarm_for_day(model_t *pmodel, size_t *alarm_num, size_t nth, uint16_t day, uint16_t month,
                                    uint16_t year) {
    assert(pmodel != NULL);
    size_t count = 0;

    for (size_t i = 0; i < pmodel->config.num_alarms; i++) {
        if (model_is_alarm_expired(pmodel, i)) {
            continue;
        }

        time_t    alarm_time = pmodel->config.alarms[i].timestamp;
        struct tm alarm_tm   = *localtime(&alarm_time);
        if (alarm_tm.tm_mday == day && alarm_tm.tm_mon == month && alarm_tm.tm_year == year) {
            if (count == nth) {
                *alarm_num = i;
                return 1;
            } else {
                count++;
            }
        }
    }

    return 0;
}


size_t model_get_active_alarms(model_t *pmodel) {
    assert(pmodel != NULL);
    size_t count = 0;
    for (size_t i = 0; i < pmodel->config.num_alarms; i++) {
        if (!model_is_alarm_expired(pmodel, i)) {
            count++;
        }
    }
    return count;
}


uint8_t model_is_alarm_expired(model_t *pmodel, size_t alarm_num) {
    time_t    now    = time(NULL);
    struct tm now_tm = *localtime(&now);

    if (alarm_num >= pmodel->config.num_alarms) {
        return 1;
    } else {
        time_t    alarm_time = pmodel->config.alarms[alarm_num].timestamp;
        struct tm alarm_tm   = *localtime(&alarm_time);

        uint8_t same_day = (alarm_tm.tm_yday == now_tm.tm_yday && alarm_tm.tm_year == now_tm.tm_year);
        uint8_t passed   = alarm_time < now;
        return !same_day && passed;
    }
}

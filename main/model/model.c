#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "model.h"
#include <esp_log.h>


#define SECONDS_IN_DAY (24UL * 60UL * 60UL)


static const char *TAG = "Model";


void model_init(mut_model_t *pmodel) {
    assert(pmodel != NULL);

    memset(pmodel->config.alarms, 0, sizeof(pmodel->config.alarms));
    pmodel->config.military_time         = 1;
    pmodel->config.num_alarms            = 0;
    pmodel->config.normal_brightness     = 80;
    pmodel->config.standby_brightness    = 20;
    pmodel->config.standby_delay_seconds = 30;

    pmodel->run.ap_list_size              = 0;
    pmodel->run.ip_addr                   = 0;
    pmodel->run.wifi_state                = WIFI_STATE_DISCONNECTED;
    pmodel->run.scanning                  = 0;
    pmodel->run.firmware_update_state.tag = FIRMWARE_UPDATE_STATE_TAG_NONE;
    strcpy(pmodel->run.ssid, "");
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

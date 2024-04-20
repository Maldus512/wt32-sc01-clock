#include <time.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "updater.h"
#include <esp_log.h>


#define MODIFIER(name, field, multiplier, min, max)                                                                    \
    void model_updater_modify_##name(model_updater_t updater, int change) {                                            \
        assert(updater != NULL);                                                                                       \
        mut_model_t *pmodel = updater->pmodel;                                                                         \
        if (change > 0) {                                                                                              \
            pmodel->field += change * multiplier;                                                                      \
            if (pmodel->field > max) {                                                                                 \
                pmodel->field = max;                                                                                   \
            }                                                                                                          \
        } else {                                                                                                       \
            if ((long)pmodel->field + change * multiplier > min) {                                                     \
                pmodel->field += change * multiplier;                                                                  \
            } else {                                                                                                   \
                pmodel->field = min;                                                                                   \
            }                                                                                                          \
        }                                                                                                              \
    }


#define SETTER(name, field)                                                                                            \
    uint8_t model_updater_set_##name(model_updater_t updater, typeof(((model_t *)0)->field) value) {                   \
        assert(updater != NULL);                                                                                       \
        mut_model_t *pmodel = updater->pmodel;                                                                         \
        if (pmodel->field != value) {                                                                                  \
            pmodel->field = value;                                                                                     \
            return 1;                                                                                                  \
        } else {                                                                                                       \
            return 0;                                                                                                  \
        }                                                                                                              \
    }


#define TOGGLER(name, field)                                                                                           \
    void model_updater_toggle_##name(model_updater_t updater) {                                                        \
        assert(updater != NULL);                                                                                       \
        mut_model_t *pmodel = updater->pmodel;                                                                         \
        pmodel->field       = !pmodel->field;                                                                          \
    }



struct model_updater {
    mut_model_t *pmodel;
};


static const char *TAG = "ModelUpdater";


model_updater_t model_updater_init(mut_model_t *pmodel) {
    (void)TAG;
    model_updater_t updater = malloc(sizeof(struct model_updater));
    updater->pmodel         = pmodel;
    model_init(pmodel);

    return updater;
}


mut_model_t *model_updater_read(model_updater_t updater) {
    assert(updater != NULL);
    return (mut_model_t *)updater->pmodel;
}


void model_updater_set_military_time(model_updater_t updater, uint8_t military_time) {
    assert(updater != NULL);
    updater->pmodel->config.military_time = military_time;
}


void model_updater_set_normal_brightness(model_updater_t updater, uint8_t brightness) {
    assert(updater != NULL);
    updater->pmodel->config.normal_brightness = brightness - (brightness % 5);
}


void model_updater_set_standby_brightness(model_updater_t updater, uint8_t brightness) {
    assert(updater != NULL);
    updater->pmodel->config.standby_brightness = brightness - (brightness % 5);
}


void model_updater_set_standby_delay(model_updater_t updater, uint16_t delay) {
    assert(updater != NULL);
    updater->pmodel->config.standby_delay_seconds = delay - (delay % 5);
}


void model_updater_update_wifi_state(model_updater_t updater, const char *ssid, uint32_t ip_addr,
                                     wifi_state_t wifi_state) {
    assert(updater != NULL);
    if (ssid != NULL) {
        snprintf(updater->pmodel->run.ssid, sizeof(updater->pmodel->run.ssid), "%s", ssid);
    } else {
        strcpy(updater->pmodel->run.ssid, "");
    }
    updater->pmodel->run.ip_addr    = ip_addr;
    updater->pmodel->run.wifi_state = wifi_state;
}


void model_updater_clear_aps(model_updater_t updater) {
    assert(updater != NULL);
    updater->pmodel->run.ap_list_size = 0;
}


void model_updater_add_ap(model_updater_t updater, const char *ssid, int16_t rssi) {
    assert(updater != NULL);
    size_t i = updater->pmodel->run.ap_list_size;

    if (i + 1 < MAX_AP_SCAN_LIST_SIZE) {
        snprintf(updater->pmodel->run.ap_list[i].ssid, sizeof(updater->pmodel->run.ap_list[i].ssid), "%s", ssid);
        updater->pmodel->run.ap_list[i].rssi = rssi;
        updater->pmodel->run.ap_list_size++;
    }
}


void model_updater_set_alarm_description(model_updater_t updater, size_t alarm_num, const char *description) {
    assert(updater != NULL);
    snprintf(updater->pmodel->config.alarms[alarm_num].description,
             sizeof(updater->pmodel->config.alarms[alarm_num].description), "%s", description);
}


void model_updater_set_alarm_time(model_updater_t updater, size_t alarm_num, unsigned long timestamp) {
    assert(updater != NULL);
    updater->pmodel->config.alarms[alarm_num].timestamp = timestamp;
}


size_t model_updater_add_alarm(model_updater_t updater, uint16_t day, uint16_t month, uint16_t year) {
    assert(updater != NULL);
    size_t  alarm_num = 0;
    uint8_t found     = 0;

    for (size_t i = 0; i < updater->pmodel->config.num_alarms; i++) {
        if (model_is_alarm_expired(updater->pmodel, i)) {
            alarm_num = i;
            found     = 1;
            break;
        }
    }

    if (!found) {
        alarm_num = updater->pmodel->config.num_alarms;
        if (updater->pmodel->config.num_alarms < MAX_ALARMS) {
            updater->pmodel->config.num_alarms++;
        }
    }

    model_updater_set_alarm_description(updater, alarm_num, "New event");

    time_t    now    = time(NULL);
    struct tm tm_now = *localtime(&now);
    tm_now.tm_mday   = day;
    tm_now.tm_mon    = month;
    tm_now.tm_year   = year;
    tm_now.tm_sec    = 0;
    time_t timestamp = mktime(&tm_now);

    model_updater_set_alarm_time(updater, alarm_num, timestamp);

    return alarm_num;
}


SETTER(scanning, run.scanning);

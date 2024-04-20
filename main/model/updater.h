#ifndef MODEL_UPDATER_H_INCLUDED
#define MODEL_UPDATER_H_INCLUDED


#include <stdlib.h>
#include "model.h"


#define SETTER(name, field)                                                                                            \
    uint8_t model_updater_set_##name(model_updater_t updater, typeof(((model_t *)0)->field) value);


typedef struct model_updater *model_updater_t;


model_updater_t model_updater_init(mut_model_t *pmodel);
mut_model_t    *model_updater_read(model_updater_t updater);
void            model_updater_set_military_time(model_updater_t updater, uint8_t military_time);
void            model_updater_set_normal_brightness(model_updater_t updater, uint8_t brightness);
void            model_updater_set_standby_brightness(model_updater_t updater, uint8_t brightness);
void            model_updater_set_standby_delay(model_updater_t updater, uint16_t delay);
void            model_updater_update_wifi_state(model_updater_t updater, const char *ssid, uint32_t ip_addr,
                                                wifi_state_t wifi_state);
void            model_updater_clear_aps(model_updater_t updater);
void            model_updater_add_ap(model_updater_t updater, const char *ssid, int16_t rssi);
size_t          model_updater_add_alarm(model_updater_t updater, uint16_t day, uint16_t month, uint16_t year);
void            model_updater_set_alarm_time(model_updater_t updater, size_t alarm_num, unsigned long timestamp);
void            model_updater_set_alarm_description(model_updater_t updater, size_t alarm_num, const char *description);

SETTER(scanning, run.scanning);

#undef SETTER

#endif

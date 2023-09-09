#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "model.h"


void model_init(mut_model_t *pmodel) {
    assert(pmodel != NULL);

    pmodel->config.military_time         = 1;
    pmodel->config.num_alarms            = 0;
    pmodel->config.normal_brightness     = 80;
    pmodel->config.standby_brightness    = 20;
    pmodel->config.standby_delay_seconds = 30;

    pmodel->run.ap_list_size = 0;
    pmodel->run.ip_addr = 0;
    pmodel->run.wifi_state = WIFI_STATE_DISCONNECTED;
    pmodel->run.scanning = 0;
    snprintf(pmodel->run.ssid, sizeof(pmodel->run.ssid), "");
}


const char *model_get_ssid(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->run.ssid;
}

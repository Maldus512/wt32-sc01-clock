#include "services/network.h"


void network_start_sta() {}


void network_scan_access_points(uint8_t channel) {}


void network_get_state(model_updater_t updater) {}


int network_get_scan_result(model_updater_t updater) {
    model_t *pmodel = model_updater_read(updater);

    if (model_get_scanning(pmodel)) {
        model_updater_clear_aps(updater);
        model_updater_add_ap(updater, "Ciao WiFi", 0);
        model_updater_set_scanning(updater, 0);
        return 1;
    } else {
        return 0;
    }
}


void network_init() {}


void network_connect_to(char *ssid, char *psk) {}

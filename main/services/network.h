#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED


#include <stdint.h>
#include "model/updater.h"


void network_init(model_t *pmodel);
void network_stop(void);
void network_scan_access_points(uint8_t channel);
void network_stop(void);
void network_start_sta(void);
int  network_get_scan_result(model_updater_t updater);
int  network_is_connected(uint32_t *ip, char *ssid);
void network_connect_to(char *ssid, char *psk);
void rainmaker_device_stop(void);
void network_get_state(model_updater_t updater);


#endif

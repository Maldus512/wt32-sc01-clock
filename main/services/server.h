#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED


#include <esp_http_server.h>
#include "model/model.h"
#include "cJSON.h"


void                    server_init(void);
void                    server_stop(void);
httpd_handle_t          server_start(void);
firmware_update_state_t server_firmware_update_state(void);
void                    server_firmware_update_reset(void);


#endif
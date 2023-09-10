#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED


#include "model/model.h"


void                    server_init(void);
void                    server_stop(void);
void                   *server_start(void);
firmware_update_state_t server_firmware_update_state(void);


#endif

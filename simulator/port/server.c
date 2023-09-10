#include "services/server.h"

void server_init() {}


firmware_update_state_t server_firmware_update_state() {
    return (firmware_update_state_t){.tag = FIRMWARE_UPDATE_STATE_TAG_NONE};
}

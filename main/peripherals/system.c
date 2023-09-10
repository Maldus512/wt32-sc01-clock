#include "system.h"
#include "esp_system.h"


void system_reset(void) {
    esp_restart();
}

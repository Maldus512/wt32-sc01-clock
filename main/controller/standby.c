#include "services/system_time.h"
#include "peripherals/tft.h"
#include "view/view.h"
#include "config/app_config.h"
#include "standby.h"


typedef enum {
    STANDBY_STATE_OFF = 0,
    STANDBY_STATE_ON,
    STANDBY_STATE_POKED,
} standby_state_t;


static standby_state_t standby_state    = STANDBY_STATE_OFF;
static unsigned long   last_activity_ts = 0;


void standby_manage(model_t *pmodel) {
    switch (standby_state) {
        case STANDBY_STATE_OFF:
            if (is_expired(last_activity_ts, get_millis(), model_get_standby_delay_seconds(pmodel) * 1000UL)) {
                last_activity_ts = get_millis();
                standby_state    = STANDBY_STATE_ON;
            }
            break;

        case STANDBY_STATE_POKED:
            tft_backlight_set(model_get_normal_brightness(pmodel));
            standby_state = STANDBY_STATE_OFF;
            break;

        case STANDBY_STATE_ON:
            tft_backlight_set(model_get_standby_brightness(pmodel));
            break;
    }
}


void standby_poke(void) {
    if (standby_state == STANDBY_STATE_ON) {
        standby_state = STANDBY_STATE_POKED;
    }
    last_activity_ts = get_millis();
}

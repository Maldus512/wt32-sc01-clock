#include "model/model.h"
#include "view/view.h"
#include "controller.h"
#include "esp_log.h"
#include "services/system_time.h"
#include "lvgl.h"


static const char *TAG = "Gui";


void controller_gui_manage(void) {
    (void)TAG;
    static unsigned long last_invoked = 0;

    if (last_invoked != get_millis()) {
        if (last_invoked > 0) {
            lv_tick_inc(time_interval(last_invoked, get_millis()));
        }
        last_invoked = get_millis();
    }

    lv_timer_handler();
}

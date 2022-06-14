#include "model/model.h"
#include "view/view.h"
#include "controller.h"
#include "esp_log.h"
#include "gel/timer/timecheck.h"
#include "utils/utils.h"
#include "lvgl.h"
//#include "peripherals/buzzer.h"
//#include "standby.h"


static const char *TAG = "Gui";


void controller_gui_manage(model_t *pmodel) {
    (void)TAG;
    static unsigned long last_invoked = 0;
    view_message_t       umsg;
    view_event_t         event;

    if (last_invoked != get_millis()) {
        if (last_invoked > 0) {
            lv_tick_inc(time_interval(last_invoked, get_millis()));
        }
        last_invoked = get_millis();
    }

    lv_timer_handler();

    while (view_get_next_msg(pmodel, &umsg, &event)) {
        if (event.code == VIEW_EVENT_CODE_LVGL &&
            (event.event == LV_EVENT_CLICKED || event.event == LV_EVENT_VALUE_CHANGED)) {
            // buzzer_beep(1, 50, 0, model_get_tone_level(pmodel));
            // standby_poke(pmodel);
        }

        controller_process_message(pmodel, &umsg.cmsg);
        view_process_msg(umsg.vmsg, pmodel);
    }
}
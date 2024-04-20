#include "watcher.h"
#include "peripherals/tft.h"
#include "model/model.h"
#include "services/system_time.h"
#include "persistance.h"


static void backlight_update(void *old_value, const void *memory, uint16_t size, void *user_ptr, void *arg);


static watcher_t watcher;


void observer_init(model_t *pmodel) {
    watcher_init(&watcher, NULL);
    WATCHER_ADD_ENTRY(&watcher, &pmodel->config.normal_brightness, backlight_update, NULL);
    WATCHER_ADD_ENTRY_DELAYED(&watcher, &pmodel->config.normal_brightness, persistance_save_variable,
                              (void *)PERSISTANCE_NORMAL_BRIGHTNESS_KEY, 4000UL);
    WATCHER_ADD_ENTRY_DELAYED(&watcher, &pmodel->config.standby_brightness, persistance_save_variable,
                              (void *)PERSISTANCE_STANDBY_BRIGHTNESS_KEY, 4000UL);
    WATCHER_ADD_ENTRY_DELAYED(&watcher, &pmodel->config.standby_delay_seconds, persistance_save_variable,
                              (void *)PERSISTANCE_STANDBY_DELAY_KEY, 4000UL);
    WATCHER_ADD_ENTRY_DELAYED(&watcher, &pmodel->config.night_mode, persistance_save_variable,
                              (void *)PERSISTANCE_NIGHT_MODE_KEY, 4000UL);
    WATCHER_ADD_ENTRY_DELAYED(&watcher, &pmodel->config.night_mode_start, persistance_save_variable,
                              (void *)PERSISTANCE_NIGHT_MODE_START_KEY, 4000UL);
    WATCHER_ADD_ENTRY_DELAYED(&watcher, &pmodel->config.night_mode_end, persistance_save_variable,
                              (void *)PERSISTANCE_NIGHT_MODE_END_KEY, 4000UL);
    WATCHER_ADD_ENTRY(&watcher, &pmodel->config.num_alarms, persistance_save_variable,
                      (void *)PERSISTANCE_ALARM_NUM_KEY);
}


void observer_manage(void) {
    watcher_watch(&watcher, get_millis());
}


static void backlight_update(void *old_value, const void *memory, uint16_t size, void *user_ptr, void *arg) {
    (void)old_value;
    (void)size;
    (void)user_ptr;
    (void)arg;
    uint8_t *backlight = (uint8_t *)memory;

    tft_backlight_set(*backlight);
}

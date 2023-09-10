#include <time.h>
#include "FreeRTOS.h"
#include "model/updater.h"
#include "task.h"
#include "esp_log.h"
#include "sdl/sdl.h"

#include "model/model.h"
#include "view/view.h"
#include "controller/controller.h"
#include "controller/gui.h"
#include "controller/persistance.h"


static const char *TAG = "Main";


void app_main(void *arg) {
    (void)arg;

    mut_model_t model = {0};

    lv_init();
    sdl_init();

    model_updater_t updater = model_updater_init(&model);
    persistance_load(&model);
    view_init(updater, controller_process_message, sdl_display_flush, sdl_mouse_read);
    controller_init(updater);


    struct tm tm = {.tm_mday = 8, .tm_mon = 8, .tm_year = 123};
    model.config.num_alarms = 1;
    model.config.alarms[0].timestamp = mktime(&tm);

    ESP_LOGI(TAG, "Begin main loop");
    for (;;) {
        controller_manage(updater);

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    vTaskDelete(NULL);
}

#include "FreeRTOS.h"
#include "model/model_updater.h"
#include "task.h"
#include "esp_log.h"
#include "sdl/sdl.h"

#include "model/model.h"
#include "view/view.h"
#include "controller/controller.h"
#include "controller/gui.h"


static const char *TAG = "Main";


void app_main(void *arg) {
    (void)arg;

    mut_model_t model = {0};

    lv_init();
    sdl_init();

    model_updater_t updater = model_updater_init(&model);
    view_init(updater, controller_process_message, sdl_display_flush, sdl_mouse_read);
    controller_init(updater);

    ESP_LOGI(TAG, "Begin main loop");
    for (;;) {
        controller_manage(updater);

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    vTaskDelete(NULL);
}

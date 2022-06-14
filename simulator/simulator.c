#include "FreeRTOS.h"
#include "task.h"
#include "esp_log.h"
#include "sdl/sdl.h"

#include "model/model.h"
#include "view/view.h"
#include "controller/controller.h"
#include "controller/gui.h"


static const char *TAG = "Main";


void app_main(void *arg) {
    model_t model;
    (void)arg;

    lv_init();
    sdl_init();

    model_init(&model);
    view_init(&model, sdl_display_flush, sdl_mouse_read);
    controller_init(&model);

    ESP_LOGI(TAG, "Begin main loop");
    for (;;) {
        controller_gui_manage(&model);
        controller_manage(&model);

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    vTaskDelete(NULL);
}
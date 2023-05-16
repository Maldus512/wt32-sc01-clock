#include "controller.h"
#include "model/model.h"
#include "view/view.h"
#include "gui.h"


void controller_init(model_updater_t updater, mut_model_t *model) {
    (void)updater;
    (void)model;

    view_change_page(&page_main);
}


void controller_process_message(pman_handle_t handle, void *msg) {
    (void)handle;
    (void)msg;
}


void controller_manage(model_updater_t updater, mut_model_t *model) {
    (void)updater;
    (void)model;

    controller_gui_manage();
}

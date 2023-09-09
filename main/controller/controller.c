#include "controller.h"
#include "model/model.h"
#include "view/view.h"
#include "gui.h"
#include "observer.h"
#include "standby.h"
#include "services/network.h"


void controller_init(model_updater_t updater) {
    (void)updater;

    observer_init(model_updater_read(updater));
    network_start_sta();

    view_change_page(&page_main);
}


void controller_process_message(pman_handle_t handle, void *msg) {
    model_updater_t updater = pman_get_user_data(handle);

    view_controller_msg_t *cmsg = msg;
    switch (cmsg->tag) {
        case VIEW_CONTROLLER_MESSAGE_TAG_NOTHING:
            break;

        case VIEW_CONTROLLER_MESSAGE_TAG_SCAN_AP: {
            model_updater_set_scanning(updater, 1);
            network_scan_access_points(0);
            break;
        }
    }
}


void controller_manage(model_updater_t updater) {
    (void)updater;

    network_get_state(updater);
    network_get_scan_result(updater);
    controller_gui_manage();
    observer_manage();
    standby_manage(model_updater_read(updater));
}

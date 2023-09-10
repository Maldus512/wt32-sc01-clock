#include "controller.h"
#include "model/model.h"
#include "view/view.h"
#include "gui.h"
#include "observer.h"
#include "standby.h"
#include "persistance.h"
#include "services/network.h"
#include "services/server.h"
#include "peripherals/system.h"
#include <esp_log.h>


static const char *TAG = "Controller";


void controller_init(model_updater_t updater) {
    (void)updater;

    network_init();
    server_init();

    observer_init(model_updater_read(updater));
    network_start_sta();

    view_change_page(&page_main);
}


void controller_process_message(pman_handle_t handle, void *msg) {
    model_updater_t updater = pman_get_user_data(handle);
    model_t        *pmodel  = model_updater_read(updater);

    view_controller_msg_t *cmsg = msg;

    if (cmsg != NULL) {
        switch (cmsg->tag) {
            case VIEW_CONTROLLER_MESSAGE_TAG_SCAN_AP: {
                ESP_LOGI(TAG, "Scanning for networks");
                model_updater_set_scanning(updater, 1);
                network_scan_access_points(0);
                break;
            }

            case VIEW_CONTROLLER_MESSAGE_TAG_CONNECT_TO:
                ESP_LOGI(TAG, "Connection request %s %s", cmsg->as.connect_to.ssid, cmsg->as.connect_to.psk);
                network_connect_to(cmsg->as.connect_to.ssid, cmsg->as.connect_to.psk);
                break;

            case VIEW_CONTROLLER_MESSAGE_TAG_SAVE_ALARM:
                persistance_save_alarm(cmsg->as.save_alarm.num, model_get_alarm(pmodel, cmsg->as.save_alarm.num));
                break;

            case VIEW_CONTROLLER_MESSAGE_TAG_RESET:
                system_reset();
                break;
        }
        lv_mem_free(cmsg);
    }
}


void controller_manage(model_updater_t updater) {
    model_t *pmodel = model_updater_read(updater);

    network_get_state(updater);
    if (network_get_scan_result(updater)) {
        view_event((view_event_t){.tag = VIEW_EVENT_TAG_WIFI_SCAN_DONE});
    }

    model_updater_set_firmware_update_state(updater, server_firmware_update_state());
    if (model_get_firmware_update_state(pmodel).tag != FIRMWARE_UPDATE_STATE_TAG_NONE &&
        !view_is_current_page_id(VIEW_PAGE_ID_OTA)) {
        view_change_page(&page_ota);
    }

    controller_gui_manage();
    observer_manage();
    standby_manage(model_updater_read(updater));
    view_manage();
}

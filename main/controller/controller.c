#include "controller.h"
#include "model/model.h"
#include "view/view.h"
#include "gui.h"
#include "observer.h"
#include "standby.h"
#include "persistance.h"
#include "services/network.h"
#include "services/server.h"
#include "services/google_calendar.h"
#include "services/system_time.h"
#include "services/github.h"
#include "peripherals/system.h"
#include <esp_log.h>


static const char *TAG = "Controller";


void controller_init(model_updater_t updater) {
    (void)updater;

    network_init();
    server_init();
    google_calendar_init();

    observer_init(model_updater_read(updater));
    network_start_sta();

    view_change_page(&page_main);
}


void controller_process_message(pman_handle_t handle, void *msg) {
    model_updater_t updater = pman_get_user_data(handle);
    mut_model_t    *pmodel  = model_updater_read(updater);

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

            case VIEW_CONTROLLER_MESSAGE_TAG_OTA:
                github_ota(pmodel);
                break;
        }
        lv_mem_free(cmsg);
    }
}


void controller_manage(model_updater_t updater) {
    static unsigned long timestamp          = 0;
    static unsigned long update_ts          = 0;
    static uint8_t       first_update_check = 1;
    mut_model_t         *pmodel             = model_updater_read(updater);

    if (model_get_wifi_state(pmodel) == WIFI_STATE_CONNECTED) {
        if (is_expired(timestamp, get_millis(), 10000UL)) {
            // ESP_LOGI(TAG, "Attempting http request");
            // google_calendar_example();
            timestamp = get_millis();
        }

        unsigned long timeout = pmodel->run.latest_release_request_state == HTTP_REQUEST_STATE_ERROR
                                    ? 1UL * 3600UL * 1000UL
                                    : 12UL * 3600UL * 1000UL;
        if (is_expired(update_ts, get_millis(), timeout) || first_update_check) {
            github_request_latest_release(pmodel);
            first_update_check = 0;
            update_ts          = get_millis();
        }
    }

    network_get_state(updater);
    if (network_get_scan_result(updater)) {
        view_event((view_event_t){.tag = VIEW_EVENT_TAG_WIFI_SCAN_DONE});
    }

    pmodel->run.server_firmware_update_state = server_firmware_update_state();
    if ((pmodel->run.server_firmware_update_state.tag != FIRMWARE_UPDATE_STATE_TAG_NONE ||
         pmodel->run.client_firmware_update_state.tag != FIRMWARE_UPDATE_STATE_TAG_NONE) &&
        !view_is_current_page_id(VIEW_PAGE_ID_OTA)) {
        view_change_page(&page_ota);
    }

    controller_gui_manage();
    observer_manage();
    standby_manage(pmodel);
    view_manage();
    github_manage(pmodel);
}

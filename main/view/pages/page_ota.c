#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include "model/updater.h"
#include "view/view.h"
#include "view/common.h"
#include "view/theme/style.h"
#include "src/extra/libs/qrcode/lv_qrcode.h"
#include "config/app_config.h"
#include <esp_log.h>


enum {
    WATCHER_OTA_STATE_ID,
    BTN_RESET_ID,
};


struct page_data {
    lv_obj_t *spinner;
    lv_obj_t *lbl_state;
    lv_obj_t *btn_reset;
};


static void update_page(model_t *pmodel, struct page_data *pdata);


static const char *TAG = "PageOta";


static void *create_page(pman_handle_t handle, void *extra) {
    (void)TAG;
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_mem_alloc(sizeof(struct page_data));
    assert(pdata != NULL);

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata   = state;
    model_updater_t   updater = pman_get_user_data(handle);
    model_t          *pmodel  = model_updater_read(updater);

    VIEW_ADD_WATCHED_VARIABLE(&pmodel->run.firmware_update_state.tag, WATCHER_OTA_STATE_ID);

    lv_obj_t *spinner = lv_spinner_create(lv_scr_act(), 2000, 48);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 32);
    pdata->spinner = spinner;

    lv_obj_t *lbl = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, LV_PCT(90));
    pdata->lbl_state = lbl;
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 32);

    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lbl           = lv_label_create(btn);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
    lv_label_set_text(lbl, "Reset");
    lv_obj_set_size(btn, 128, 64);
    lv_obj_center(lbl);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 32);
    view_register_object_default_callback(btn, BTN_RESET_ID);
    pdata->btn_reset = btn;

    update_page(pmodel, pdata);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    (void)handle;

    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata   = state;
    model_updater_t   updater = pman_get_user_data(handle);
    model_t          *pmodel  = model_updater_read(updater);

    switch (event.tag) {
        case PMAN_EVENT_TAG_TIMER: {
            break;
        }

        case PMAN_EVENT_TAG_LVGL: {
            view_obj_data_t *obj_data = lv_obj_get_user_data(lv_event_get_current_target(event.as.lvgl));
            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_CLICKED: {
                    switch (obj_data->id) {
                        case BTN_RESET_ID:
                            msg.user_msg =
                                view_controller_msg((view_controller_msg_t){.tag = VIEW_CONTROLLER_MESSAGE_TAG_RESET});
                            break;
                    }
                }
                default:
                    break;
            }
            break;
        }

        case PMAN_EVENT_TAG_USER: {
            view_event_t *view_event = event.as.user;
            switch (view_event->tag) {
                case VIEW_EVENT_TAG_VARIABLE_WATCHER: {
                    update_page(pmodel, pdata);
                    break;
                }

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }

    return msg;
}


static void update_page(model_t *pmodel, struct page_data *pdata) {
    firmware_update_state_t state = model_get_firmware_update_state(pmodel);

    switch (state.tag) {
        case FIRMWARE_UPDATE_STATE_TAG_SUCCESS:
        case FIRMWARE_UPDATE_STATE_TAG_NONE:
            view_common_set_hidden(pdata->spinner, 1);
            view_common_set_hidden(pdata->btn_reset, 0);
            lv_label_set_text(pdata->lbl_state, "Update done");
            break;

        case FIRMWARE_UPDATE_STATE_TAG_UPDATING:
            view_common_set_hidden(pdata->spinner, 0);
            view_common_set_hidden(pdata->btn_reset, 1);
            lv_label_set_text(pdata->lbl_state, "Updating...");
            break;

        case FIRMWARE_UPDATE_STATE_TAG_FAILURE: {
            view_common_set_hidden(pdata->spinner, 1);
            view_common_set_hidden(pdata->btn_reset, 0);
            lv_label_set_text_fmt(pdata->lbl_state, "%s (%i-0x%X)", "Update failed!", state.failure_code, state.error);
            break;
        }
    }
}



const pman_page_t page_ota = {
    .id            = VIEW_PAGE_ID_OTA,
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};

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
    BTN_BACK_ID,
    BTN_CREATE_ID,
    BTN_MODIFY_ID,
};


struct page_data {
    lv_obj_t *list;

    unsigned long       timestamp;
    lv_calendar_date_t *date;
};


static void update_list(model_t *pmodel, struct page_data *pdata);


static const char *TAG = "PageAlarms";


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_mem_alloc(sizeof(struct page_data));
    assert(pdata != NULL);
    pdata->date = extra;

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata   = state;
    model_updater_t   updater = pman_get_user_data(handle);
    model_t          *pmodel  = model_updater_read(updater);

    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 56, 56);
    lv_obj_t *symbol = lv_label_create(btn);
    lv_obj_set_style_text_font(symbol, STYLE_FONT_BIG, LV_STATE_DEFAULT);
    lv_label_set_text(symbol, LV_SYMBOL_LEFT);
    lv_obj_center(symbol);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 8, 8);
    view_register_object_default_callback(btn, BTN_BACK_ID);

    lv_obj_t *lbl = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(lbl, "%02i/%02i/%02i", pdata->date->day, pdata->date->month, pdata->date->year);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *list = lv_list_create(lv_scr_act());
    lv_obj_set_style_text_font(list, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
    lv_obj_set_size(list, LV_HOR_RES, LV_VER_RES - 80);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
    pdata->list = list;

    update_list(pmodel, pdata);
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
                case LV_EVENT_CLICKED:
                    switch (obj_data->id) {
                        case BTN_BACK_ID:
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;
                        case BTN_CREATE_ID: {
                            size_t alarm_num = model_updater_add_alarm(
                                updater, pdata->date->day, pdata->date->month - 1, pdata->date->year - 1900);
                            msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE_EXTRA(&page_alarm, (void *)(uintptr_t)alarm_num);
                            break;
                        }
                        case BTN_MODIFY_ID: {
                            size_t alarm_num = obj_data->number;
                            msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE_EXTRA(&page_alarm, (void *)(uintptr_t)alarm_num);
                            break;
                        }
                        default:
                            break;
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


static void destroy_page(void *state, void *extra) {
    lv_mem_free(extra);
    lv_mem_free(state);
}


static void update_list(model_t *pmodel, struct page_data *pdata) {
    lv_obj_clean(pdata->list);

    uint8_t found = 0;
    size_t  count = 0;
    do {
        size_t alarm_num = 0;
        found = model_get_nth_alarm_for_day(pmodel, &alarm_num, count, pdata->date->day, pdata->date->month - 1,
                                            pdata->date->year - 1900);
        if (found) {
            count++;
            char      string[MAX_DESCRIPTION_LEN + 32] = {0};
            time_t    timestamp                        = model_get_alarm(pmodel, alarm_num).timestamp;
            struct tm time_tm                          = *localtime(&timestamp);
            snprintf(string, sizeof(string), "[%02i:%02i] %s", time_tm.tm_hour, time_tm.tm_min,
                     model_get_alarm(pmodel, alarm_num).description);
            lv_obj_t *btn = lv_list_add_btn(pdata->list, NULL, string);
            lv_label_set_long_mode(lv_obj_get_child(btn, 0), LV_LABEL_LONG_DOT);
            view_register_object_default_callback_with_number(btn, BTN_MODIFY_ID, alarm_num);
        }
    } while (found);

    if (model_get_active_alarms(pmodel) < MAX_ALARMS) {
        lv_obj_t *btn = lv_list_add_btn(pdata->list, LV_SYMBOL_PLUS, "New event");
        view_register_object_default_callback(btn, BTN_CREATE_ID);
    }
}


const pman_page_t page_alarms = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};

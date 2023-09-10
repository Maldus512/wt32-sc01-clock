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
    KEYBOARD_ID,
    TEXTAREA_ID,
    ROLLER_HOUR_ID,
    ROLLER_MINUTE_ID,
    BTN_DELETE_ID,
};


struct page_data {
    lv_obj_t *textarea;
    lv_obj_t *keyboard;
    lv_obj_t *roller_hour;
    lv_obj_t *roller_minute;

    size_t alarm_num;
};


static void mask_event_cb(lv_event_t *e);


static const char *TAG = "PageAlarm";


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_mem_alloc(sizeof(struct page_data));
    assert(pdata != NULL);
    pdata->alarm_num = (size_t)(uintptr_t)extra;

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata   = state;
    model_updater_t   updater = pman_get_user_data(handle);
    model_t          *pmodel  = model_updater_read(updater);

    alarm_t   alarm      = model_get_alarm(pmodel, pdata->alarm_num);
    time_t    alarm_time = alarm.timestamp;
    struct tm alarm_tm   = *localtime(&alarm_time);

    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 56, 56);
    lv_obj_t *symbol = lv_label_create(btn);
    lv_obj_set_style_text_font(symbol, STYLE_FONT_BIG, LV_STATE_DEFAULT);
    lv_label_set_text(symbol, LV_SYMBOL_LEFT);
    lv_obj_center(symbol);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 8, 8);
    view_register_object_default_callback(btn, BTN_BACK_ID);

    lv_obj_t *ta = lv_textarea_create(lv_scr_act());
    lv_obj_set_style_text_font(ta, STYLE_FONT_TINY, LV_STATE_DEFAULT);
    lv_textarea_set_one_line(ta, 0);
    lv_textarea_set_text(ta, alarm.description);
    lv_textarea_set_max_length(ta, 33);
    lv_obj_set_size(ta, LV_HOR_RES - 80, 64);
    lv_obj_align(ta, LV_ALIGN_TOP_RIGHT, -8, 4);
    lv_obj_add_state(ta, LV_STATE_FOCUSED);
    view_register_object_default_callback(ta, TEXTAREA_ID);
    pdata->textarea = ta;

    lv_obj_t *roller_hour = lv_roller_create(lv_scr_act());
    lv_roller_set_visible_row_count(roller_hour, 3);
    lv_roller_set_options(
        roller_hour, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23",
        LV_ROLLER_MODE_INFINITE);
    lv_obj_set_width(roller_hour, 96);
    lv_obj_align(roller_hour, LV_ALIGN_CENTER, -64, 32);
    lv_obj_add_event_cb(roller_hour, mask_event_cb, LV_EVENT_ALL, NULL);
    lv_roller_set_selected(roller_hour, alarm_tm.tm_hour, LV_ANIM_OFF);
    view_register_object_default_callback(roller_hour, ROLLER_HOUR_ID);
    pdata->roller_hour = roller_hour;

    lv_obj_t *lbl = lv_label_create(lv_scr_act());
    lv_label_set_text(lbl, ":");
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 32);

    lv_obj_t *roller_minute = lv_roller_create(lv_scr_act());
    lv_roller_set_visible_row_count(roller_minute, 3);
    lv_roller_set_options(roller_minute, "00\n05\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_width(roller_minute, 96);
    lv_obj_add_event_cb(roller_minute, mask_event_cb, LV_EVENT_ALL, NULL);
    lv_roller_set_selected(roller_minute, alarm_tm.tm_min / 5, LV_ANIM_OFF);
    lv_obj_align(roller_minute, LV_ALIGN_CENTER, 64, 32);
    view_register_object_default_callback(roller_minute, ROLLER_MINUTE_ID);
    pdata->roller_minute = roller_minute;

    btn = lv_btn_create(lv_scr_act());
    lv_obj_set_style_bg_color(btn, STYLE_RED, LV_STATE_DEFAULT);
    lv_obj_set_size(btn, 64, 64);
    symbol = lv_label_create(btn);
    lv_obj_set_style_text_font(symbol, STYLE_FONT_BIG, LV_STATE_DEFAULT);
    lv_label_set_text(symbol, LV_SYMBOL_TRASH);
    lv_obj_center(symbol);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -8, 32);
    view_register_object_default_callback(btn, BTN_DELETE_ID);

    lv_obj_t *kb = lv_keyboard_create(lv_scr_act());
    lv_obj_set_style_text_font(kb, STYLE_FONT_SMALL, LV_STATE_DEFAULT | LV_PART_ITEMS);
    lv_obj_set_style_border_color(kb, STYLE_GRAY, LV_STATE_DEFAULT | LV_PART_ITEMS);
    lv_obj_set_style_border_opa(kb, LV_OPA_MAX, LV_STATE_DEFAULT | LV_PART_ITEMS);
    lv_obj_set_style_border_width(kb, 2, LV_STATE_DEFAULT | LV_PART_ITEMS);
    lv_obj_set_style_bg_color(kb, lv_color_make(0, 0, 0), LV_STATE_DEFAULT | LV_PART_ITEMS | LV_PART_MAIN);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_set_size(kb, LV_HOR_RES, LV_VER_RES - 96);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    view_register_object_default_callback(kb, KEYBOARD_ID);
    pdata->keyboard = kb;

    view_common_set_hidden(pdata->keyboard, 1);
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
                case LV_EVENT_READY: {
                    switch (obj_data->id) {
                        case KEYBOARD_ID:
                            view_common_set_hidden(pdata->keyboard, 1);
                            break;
                    }
                    break;
                }

                case LV_EVENT_CANCEL: {
                    switch (obj_data->id) {
                        case KEYBOARD_ID:
                            view_common_set_hidden(pdata->keyboard, 1);
                            break;
                    }
                    break;
                }

                case LV_EVENT_CLICKED:
                    switch (obj_data->id) {
                        case TEXTAREA_ID:
                            view_common_set_hidden(pdata->keyboard, 0);
                            break;
                        case BTN_BACK_ID: {
                            model_updater_set_alarm_description(
                                updater, pdata->alarm_num,
                                lv_textarea_get_text(pdata->textarea));     // invalidate the alarm

                            time_t    alarm_time = model_get_alarm(pmodel, pdata->alarm_num).timestamp;
                            struct tm alarm_tm   = *localtime(&alarm_time);
                            alarm_tm.tm_hour     = lv_roller_get_selected(pdata->roller_hour);
                            alarm_tm.tm_min      = lv_roller_get_selected(pdata->roller_minute) * 5;
                            model_updater_set_alarm_time(updater, pdata->alarm_num, mktime(&alarm_tm));

                            msg.user_msg = view_controller_msg(
                                (view_controller_msg_t){.tag = VIEW_CONTROLLER_MESSAGE_TAG_SAVE_ALARM,
                                                        .as  = {.save_alarm = {.num = pdata->alarm_num}}});
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;
                        }
                        case BTN_DELETE_ID:
                            model_updater_set_alarm_time(updater, pdata->alarm_num, 0);     // invalidate the alarm
                            msg.user_msg = view_controller_msg(
                                (view_controller_msg_t){.tag = VIEW_CONTROLLER_MESSAGE_TAG_SAVE_ALARM,
                                                        .as  = {.save_alarm = {.num = pdata->alarm_num}}});
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;
                        default:
                            break;
                    }
                    break;

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
    (void)extra;
    lv_mem_free(state);
}


static void mask_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t       *obj  = lv_event_get_target(e);

    static int16_t mask_top_id    = -1;
    static int16_t mask_bottom_id = -1;

    if (code == LV_EVENT_COVER_CHECK) {
        lv_event_set_cover_res(e, LV_COVER_RES_MASKED);

    } else if (code == LV_EVENT_DRAW_MAIN_BEGIN) {
        /* add mask */
        const lv_font_t *font       = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
        lv_coord_t       line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
        lv_coord_t       font_h     = lv_font_get_line_height(font);

        lv_area_t roller_coords;
        lv_obj_get_coords(obj, &roller_coords);

        lv_area_t rect_area;
        rect_area.x1 = roller_coords.x1;
        rect_area.x2 = roller_coords.x2;
        rect_area.y1 = roller_coords.y1;
        rect_area.y2 = roller_coords.y1 + (lv_obj_get_height(obj) - font_h - line_space) / 2;

        lv_draw_mask_fade_param_t *fade_mask_top = lv_mem_buf_get(sizeof(lv_draw_mask_fade_param_t));
        lv_draw_mask_fade_init(fade_mask_top, &rect_area, LV_OPA_TRANSP, rect_area.y1, LV_OPA_COVER, rect_area.y2);
        mask_top_id = lv_draw_mask_add(fade_mask_top, NULL);

        rect_area.y1 = rect_area.y2 + font_h + line_space - 1;
        rect_area.y2 = roller_coords.y2;

        lv_draw_mask_fade_param_t *fade_mask_bottom = lv_mem_buf_get(sizeof(lv_draw_mask_fade_param_t));
        lv_draw_mask_fade_init(fade_mask_bottom, &rect_area, LV_OPA_COVER, rect_area.y1, LV_OPA_TRANSP, rect_area.y2);
        mask_bottom_id = lv_draw_mask_add(fade_mask_bottom, NULL);

    } else if (code == LV_EVENT_DRAW_POST_END) {
        lv_draw_mask_fade_param_t *fade_mask_top    = lv_draw_mask_remove_id(mask_top_id);
        lv_draw_mask_fade_param_t *fade_mask_bottom = lv_draw_mask_remove_id(mask_bottom_id);
        lv_draw_mask_free_param(fade_mask_top);
        lv_draw_mask_free_param(fade_mask_bottom);
        lv_mem_buf_release(fade_mask_top);
        lv_mem_buf_release(fade_mask_bottom);
        mask_top_id    = -1;
        mask_bottom_id = -1;
    }
}


const pman_page_t page_alarm = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};

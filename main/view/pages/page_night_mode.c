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
    BTN_CONFIRM_ID,
    ROLLER_HOUR_ID,
    ROLLER_MINUTE_ID,
};


typedef enum {
    PAGE_STATE_START = 0,
    PAGE_STATE_END,
} page_state_t;


struct page_data {
    lv_obj_t *lbl_title;
    lv_obj_t *roller_hour;
    lv_obj_t *roller_minute;

    page_state_t page_state;
};


static void mask_event_cb(lv_event_t *e);
static void update_page(model_t *pmodel, struct page_data *pdata);


static const char *TAG = "PageNightMode";


static void *create_page(pman_handle_t handle, void *extra) {
    (void)TAG;
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_mem_alloc(sizeof(struct page_data));
    assert(pdata != NULL);
    pdata->page_state = PAGE_STATE_START;

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
    lv_obj_set_style_text_font(lbl, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, LV_HOR_RES - 96);
    lv_obj_align(lbl, LV_ALIGN_TOP_RIGHT, -8, 4);
    pdata->lbl_title = lbl;

    lv_obj_t *roller_hour = lv_roller_create(lv_scr_act());
    lv_roller_set_visible_row_count(roller_hour, 3);
    lv_roller_set_options(
        roller_hour, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23",
        LV_ROLLER_MODE_INFINITE);
    lv_obj_set_width(roller_hour, 96);
    lv_obj_align(roller_hour, LV_ALIGN_CENTER, -64, 32);
    lv_obj_add_event_cb(roller_hour, mask_event_cb, LV_EVENT_ALL, NULL);
    view_register_object_default_callback(roller_hour, ROLLER_HOUR_ID);
    pdata->roller_hour = roller_hour;

    lbl = lv_label_create(lv_scr_act());
    lv_label_set_text(lbl, ":");
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 32);

    lv_obj_t *roller_minute = lv_roller_create(lv_scr_act());
    lv_roller_set_visible_row_count(roller_minute, 3);
    lv_roller_set_options(roller_minute, "00\n05\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55", LV_ROLLER_MODE_INFINITE);
    lv_obj_set_width(roller_minute, 96);
    lv_obj_add_event_cb(roller_minute, mask_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_align(roller_minute, LV_ALIGN_CENTER, 64, 32);
    view_register_object_default_callback(roller_minute, ROLLER_MINUTE_ID);
    pdata->roller_minute = roller_minute;

    btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 64, 64);
    symbol = lv_label_create(btn);
    lv_obj_set_style_text_font(symbol, STYLE_FONT_BIG, LV_STATE_DEFAULT);
    lv_label_set_text(symbol, LV_SYMBOL_OK);
    lv_obj_center(symbol);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -8, 32);
    view_register_object_default_callback(btn, BTN_CONFIRM_ID);

    update_page(pmodel, pdata);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    (void)handle;

    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata   = state;
    model_updater_t   updater = pman_get_user_data(handle);
    mut_model_t      *pmodel  = model_updater_read(updater);

    switch (event.tag) {
        case PMAN_EVENT_TAG_TIMER: {
            break;
        }

        case PMAN_EVENT_TAG_LVGL: {
            view_obj_data_t *obj_data = lv_obj_get_user_data(lv_event_get_current_target(event.as.lvgl));
            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_CLICKED:
                    switch (obj_data->id) {
                        case BTN_BACK_ID: {
                            switch (pdata->page_state) {
                                case PAGE_STATE_START: {
                                    msg.stack_msg = PMAN_STACK_MSG_BACK();
                                    break;
                                }
                                case PAGE_STATE_END: {
                                    pdata->page_state = PAGE_STATE_START;
                                    break;
                                }
                            }
                            break;
                        }

                        case BTN_CONFIRM_ID: {
                            switch (pdata->page_state) {
                                case PAGE_STATE_START: {
                                    uint32_t start_hour = lv_roller_get_selected(pdata->roller_hour);
                                    uint32_t start_min  = lv_roller_get_selected(pdata->roller_minute) * 5;

                                    pmodel->config.night_mode_start = start_hour * 3600 + start_min * 60;

                                    pdata->page_state = PAGE_STATE_END;

                                    update_page(pmodel, pdata);
                                    break;
                                }

                                case PAGE_STATE_END: {
                                    uint32_t end_hour = lv_roller_get_selected(pdata->roller_hour);
                                    uint32_t end_min  = lv_roller_get_selected(pdata->roller_minute) * 5;

                                    pmodel->config.night_mode_end = end_hour * 3600 + end_min * 60;

                                    msg.stack_msg = PMAN_STACK_MSG_BACK();
                                    break;
                                }
                            }
                            break;
                        }

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


static void update_page(model_t *pmodel, struct page_data *pdata) {
    switch (pdata->page_state) {
        case PAGE_STATE_START: {
            lv_label_set_text(pdata->lbl_title, "Select start time");

            uint32_t start_hour = pmodel->config.night_mode_start / 3600;
            uint32_t start_min  = (pmodel->config.night_mode_start % 3600) / 60;

            lv_roller_set_selected(pdata->roller_hour, start_hour, LV_ANIM_OFF);
            lv_roller_set_selected(pdata->roller_minute, start_min / 5, LV_ANIM_OFF);
            break;
        }

        case PAGE_STATE_END: {
            lv_label_set_text(pdata->lbl_title, "Select end time");

            uint32_t end_hour = pmodel->config.night_mode_end / 3600;
            uint32_t end_min  = (pmodel->config.night_mode_end % 3600) / 60;

            lv_roller_set_selected(pdata->roller_hour, end_hour, LV_ANIM_OFF);
            lv_roller_set_selected(pdata->roller_minute, end_min / 5, LV_ANIM_OFF);
            break;
        }
    }
}


const pman_page_t page_night_mode = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};

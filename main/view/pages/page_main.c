#include <stdlib.h>
#include "lvgl.h"
#include "model/model.h"
#include "view/view.h"
#include "view/view_types.h"
#include "view/theme/style.h"
#include "view/intl/intl.h"
#include "gel/pagemanager/page_manager.h"




struct page_data {};


static void *create_page(model_t *pmodel, void *extra) {
    struct page_data *pdata = malloc(sizeof(struct page_data));
    return pdata;
}


static void open_page(model_t *pmodel, void *args) {
    struct page_data *pdata = args;
    (void)pdata;
    lv_obj_t *btn, *lbl;

    btn = lv_btn_create(lv_scr_act());
    lbl = lv_label_create(btn);
    lv_label_set_text(lbl, view_intl_get_string(pmodel, STRINGS_HELLO_WORLD));
    lv_obj_center(lbl);
    lv_obj_center(btn);

    btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 64, 64);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 16, -16);

    btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 64, 64);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -16, -16);

    btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 64, 64);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 16, 16);

    btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 64, 64);
    lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -16, 16);
}


static view_message_t page_event(model_t *pmodel, void *args, view_event_t event) {
    view_message_t msg = VIEW_NULL_MESSAGE;

    switch (event.code) {

        default:
            break;
    }

    return msg;
}


const pman_page_t page_main = {
    .create        = create_page,
    .destroy       = view_destroy_all,
    .open          = open_page,
    .close         = view_close_all,
    .process_event = page_event,
};
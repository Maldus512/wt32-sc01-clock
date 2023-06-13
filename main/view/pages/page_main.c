#include <assert.h>
#include <stdlib.h>
#include "lvgl.h"
#include "model/model_updater.h"
#include "view/view.h"
#include "view/theme/style.h"
#include "view/intl/intl.h"


struct page_data {
    view_controller_msg_t cmsg;
};


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_mem_alloc(sizeof(struct page_data));
    assert(pdata != NULL);
    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    struct page_data *pdata = state;
    (void)pdata;

    model_updater_t updater = pman_get_user_data(handle);
    const model_t  *model   = model_updater_read(updater);

    lv_obj_t *btn, *lbl;

    btn = lv_btn_create(lv_scr_act());
    lbl = lv_label_create(btn);
    lv_label_set_text(lbl, view_intl_get_string(model, STRINGS_HELLO_WORLD));
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


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    (void)handle;

    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata = state;

    msg.user_msg    = &pdata->cmsg;
    pdata->cmsg.tag = VIEW_CONTROLLER_MESSAGE_TAG_NOTHING;

    switch (event.tag) {
        default:
            break;
    }

    return msg;
}


const pman_page_t page_main = {
    .create        = create_page,
    .destroy       = pman_destroy_all,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};

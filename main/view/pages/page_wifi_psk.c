#include <stdlib.h>
#include "lvgl.h"
#include "model/model.h"
#include "view/view.h"
#include "view/common.h"
#include "view/theme/style.h"


#define KB_BTN(width) width


enum {
    BACK_BTN_ID,
    PASSWORD_KB_ID,
};


struct page_data {
    char     *ssid;
    lv_obj_t *textarea;
    lv_obj_t *keyboard;
};


static const char *const kb_map_spec[] = {
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    LV_SYMBOL_BACKSPACE,
    "\n",
    "abc",
    "+",
    "-",
    "/",
    "*",
    "=",
    "%",
    "!",
    "?",
    "#",
    "<",
    ">",
    "\n",
    "\\",
    "@",
    "$",
    "(",
    ")",
    "{",
    "}",
    "[",
    "]",
    ";",
    "\"",
    "'",
    "\n",
    LV_SYMBOL_KEYBOARD,
    "&",
    " ",
    "\u20AC",     // Euro symbol
    LV_SYMBOL_OK,
    "",
};

static const lv_btnmatrix_ctrl_t kb_ctrl_spec_map[] = {
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    LV_BTNMATRIX_CTRL_CHECKED | 2,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    KB_BTN(1),
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    LV_BTNMATRIX_CTRL_CHECKED | 2,
    6,
    LV_BTNMATRIX_CTRL_CHECKED | 2,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
};


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;

    struct page_data *pdata = lv_mem_alloc(sizeof(struct page_data));
    assert(pdata != NULL);
    pdata->ssid = extra;

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
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, 0);

    view_register_object_default_callback(btn, BACK_BTN_ID);

    lv_obj_t *ta = lv_textarea_create(lv_scr_act());
    lv_textarea_set_one_line(ta, 1);
    lv_textarea_set_text(ta, "");
    lv_textarea_set_max_length(ta, 33);
    lv_obj_set_width(ta, LV_HOR_RES - 60);
    lv_obj_clear_flag(ta, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(ta, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_state(ta, LV_STATE_FOCUSED);
    pdata->textarea = ta;

    lv_obj_t *kb = lv_keyboard_create(lv_scr_act());
    lv_obj_set_style_text_font(kb, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT | LV_PART_ITEMS);
    lv_obj_set_style_border_color(kb, STYLE_GRAY, LV_STATE_DEFAULT | LV_PART_ITEMS);
    lv_obj_set_style_border_opa(kb, LV_OPA_MAX, LV_STATE_DEFAULT | LV_PART_ITEMS);
    lv_obj_set_style_border_width(kb, 2, LV_STATE_DEFAULT | LV_PART_ITEMS);
    lv_obj_set_style_bg_color(kb, lv_color_make(0, 0, 0), LV_STATE_DEFAULT | LV_PART_ITEMS | LV_PART_MAIN);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_set_size(kb, LV_HOR_RES, LV_VER_RES - 64 - 40);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_SPECIAL, (const char **)kb_map_spec, kb_ctrl_spec_map);
    view_register_object_default_callback(kb, PASSWORD_KB_ID);
    pdata->keyboard = kb;
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    (void)handle;

    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata   = state;
    model_updater_t   updater = pman_get_user_data(handle);
    model_t          *pmodel  = model_updater_read(updater);

    msg.user_msg  = &view_cmsg;
    view_cmsg.tag = VIEW_CONTROLLER_MESSAGE_TAG_NOTHING;

    switch (event.tag) {
        case PMAN_EVENT_TAG_LVGL: {
            lv_obj_t        *target   = lv_event_get_current_target(event.as.lvgl);
            view_obj_data_t *obj_data = lv_obj_get_user_data(target);

            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_CLICKED: {
                    switch (obj_data->id) {
                        case BACK_BTN_ID:
                            msg.stack_msg = PMAN_STACK_MSG_BACK();
                            break;
                    }
                    break;
                }

                case LV_EVENT_VALUE_CHANGED: {
                    switch (obj_data->id) {
                        case PASSWORD_KB_ID: {
                            const char *string = lv_textarea_get_text(pdata->textarea);
                            size_t      len    = strlen(string);
                            if (len >= 8 || len == 0) {
                                lv_obj_set_style_border_color(pdata->textarea, STYLE_GRAY, LV_STATE_DEFAULT);
                            }
                            break;
                        }

                        default:
                            break;
                    }
                    break;
                }
                case LV_EVENT_READY: {
                    switch (obj_data->id) {
                        case PASSWORD_KB_ID: {
                            const char *string = lv_textarea_get_text(pdata->textarea);
                            size_t      len    = strlen(string);
                            if (len > 0 && len < 8) {
                                lv_obj_set_style_border_color(pdata->textarea, STYLE_RED, LV_STATE_DEFAULT);
                            } else {
                                strcpy(view_cmsg.as.connect_to.ssid, pdata->ssid);
                                strcpy(view_cmsg.as.connect_to.psk, string);
                                view_cmsg.tag = VIEW_CONTROLLER_MESSAGE_TAG_CONNECT_TO;
                                msg.stack_msg = PMAN_STACK_MSG_BACK();
                            }
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


const pman_page_t page_wifi_psk = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = pman_close_all,
    .process_event = page_event,
};

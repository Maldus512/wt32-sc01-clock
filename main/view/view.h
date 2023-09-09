#ifndef VIEW_H_INCLUDED
#define VIEW_H_INCLUDED


#include "model/updater.h"
#include "page_manager.h"


#define VIEW_ADD_WATCHED_VARIABLE(ptr, code) view_add_watched_variable((void *)ptr, sizeof(*ptr), code)


typedef enum {
    VIEW_CONTROLLER_MESSAGE_TAG_NOTHING = 0,
    VIEW_CONTROLLER_MESSAGE_TAG_SCAN_AP,
    VIEW_CONTROLLER_MESSAGE_TAG_CONNECT_TO,
} view_controller_msg_tag_t;

typedef struct {
    view_controller_msg_tag_t tag;
    union {
        struct {
            char ssid[MAX_SSID_SIZE];
            char psk[MAX_SSID_SIZE];
        } connect_to;
    } as;
} view_controller_msg_t;

typedef struct {
    int id;
    int number;
} view_obj_data_t;

typedef enum {
    VIEW_EVENT_TAG_WIFI_SCAN_DONE,
    VIEW_EVENT_TAG_VARIABLE_WATCHER,
} view_event_tag_t;

typedef struct {
    view_event_tag_t tag;
    union {
        struct {
            int code;
        } page_watcher;
    } as;
} view_event_t;


void view_init(model_updater_t updater, pman_user_msg_cb_t controller_cb,
               void (*flush_cb)(struct _lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p),
               void (*read_cb)(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data));
void view_change_page(const pman_page_t *page);
void view_register_object_default_callback(lv_obj_t *obj, int id);
void view_register_object_default_callback_with_number(lv_obj_t *obj, int id, int number);
void view_event(view_event_t event);
void view_add_watched_variable(void *ptr, size_t size, int code);


extern view_controller_msg_t view_cmsg;
extern const pman_page_t     page_main, page_wifi_psk;


#endif

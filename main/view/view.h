#ifndef VIEW_H_INCLUDED
#define VIEW_H_INCLUDED


#include "model/model_updater.h"
#include "page_manager.h"


typedef enum {
    VIEW_CONTROLLER_MESSAGE_TAG_NOTHING = 0,
} view_controller_msg_tag_t;

typedef struct {
    view_controller_msg_tag_t tag;
    union {
    } as;
} view_controller_msg_t;


void view_init(model_updater_t updater, pman_user_msg_cb_t controller_cb,
               void (*flush_cb)(struct _lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p),
               void (*read_cb)(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data));

void view_change_page(const pman_page_t *page);


extern const pman_page_t page_main;


#endif

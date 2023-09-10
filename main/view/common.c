#include "view.h"
#include "common.h"


void view_common_set_hidden(lv_obj_t *obj, int hidden) {
    if (((obj->flags & LV_OBJ_FLAG_HIDDEN) == 0) && hidden) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else if (((obj->flags & LV_OBJ_FLAG_HIDDEN) > 0) && !hidden) {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}


view_controller_msg_t *view_controller_msg(view_controller_msg_t msg) {
    view_controller_msg_t *alloc_msg = lv_mem_alloc(sizeof(view_controller_msg_t));
    assert(alloc_msg != NULL);
    memcpy(alloc_msg, &msg, sizeof(view_controller_msg_t));
    return alloc_msg;
}

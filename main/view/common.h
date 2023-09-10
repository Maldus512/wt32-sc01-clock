#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED


#include "view.h"


void                   view_common_set_hidden(lv_obj_t *obj, int hidden);
view_controller_msg_t *view_controller_msg(view_controller_msg_t msg);


#endif

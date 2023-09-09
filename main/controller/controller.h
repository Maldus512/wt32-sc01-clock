#ifndef CONTROLLER_H_INCLUDED
#define CONTROLLER_H_INCLUDED

#include "model/updater.h"
#include "model/model.h"
#include "view/view.h"


void controller_init(model_updater_t updater);
void controller_manage(model_updater_t updater);
void controller_process_message(pman_handle_t handle, void *msg);


#endif

#ifndef CONTROLLER_H_INCLUDED
#define CONTROLLER_H_INCLUDED

#include "model/model_updater.h"
#include "model/model.h"
#include "view/view.h"


void controller_init(model_updater_t updater, mut_model_t *model);
void controller_manage(model_updater_t updater, mut_model_t *model);
void controller_process_message(pman_handle_t handle, void *msg);


#endif

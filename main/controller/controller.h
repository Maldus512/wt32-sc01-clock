#ifndef CONTROLLER_H_INCLUDED
#define CONTROLLER_H_INCLUDED

#include "model/model.h"
#include "view/view.h"


void controller_init(model_t *model);
void controller_process_message(model_t *pmodel, view_controller_message_t *msg);
void controller_manage(model_t *pmodel);

#endif
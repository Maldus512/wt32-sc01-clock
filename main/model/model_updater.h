#ifndef MODEL_UPDATER_H_INCLUDED
#define MODEL_UPDATER_H_INCLUDED


#include <stdlib.h>
#include "model.h"


typedef struct model_updater *model_updater_t;


model_updater_t model_updater_init(mut_model_t *pmodel);
const model_t  *model_updater_read(model_updater_t updater);


#endif

#include <stdlib.h>
#include <assert.h>
#include "model.h"


void model_init(mut_model_t *model) {
    assert(model != NULL);

    model->config.language = 0;
}

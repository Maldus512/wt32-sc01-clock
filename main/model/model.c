#include <stdlib.h>
#include <assert.h>
#include "model.h"


void model_init(mut_model_t *pmodel) {
    assert(pmodel != NULL);

    pmodel->config.language = 0;
}

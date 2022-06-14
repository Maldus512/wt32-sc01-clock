#include <stdlib.h>
#include <assert.h>
#include "model.h"

void model_init(model_t *pmodel) {
    assert(pmodel != NULL);
    pmodel->configuration.language = 0;
}


uint16_t model_get_language(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->configuration.language;
}
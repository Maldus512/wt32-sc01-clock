#include <assert.h>
#include "model_updater.h"



struct model_updater {
    mut_model_t *model;
};


model_updater_t model_updater_init(mut_model_t *model) {
    model_updater_t updater = malloc(sizeof(struct model_updater));
    updater->model          = model;

    return updater;
}


const model_t *model_updater_get(model_updater_t updater) {
    assert(updater != NULL);
    return (const model_t *)updater->model;
}

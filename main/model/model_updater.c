#include <assert.h>
#include "model_updater.h"


#define MODIFIER(name, field, multiplier, min, max)                                                                    \
    void model_updater_modify_##name(model_updater_t updater, int change) {                                            \
        assert(updater != NULL);                                                                                       \
        mut_model_t *pmodel = updater->pmodel;                                                                         \
        if (change > 0) {                                                                                              \
            pmodel->field += change * multiplier;                                                                      \
            if (pmodel->field > max) {                                                                                 \
                pmodel->field = max;                                                                                   \
            }                                                                                                          \
        } else {                                                                                                       \
            if ((long)pmodel->field + change * multiplier > min) {                                                     \
                pmodel->field += change * multiplier;                                                                  \
            } else {                                                                                                   \
                pmodel->field = min;                                                                                   \
            }                                                                                                          \
        }                                                                                                              \
    }


#define SETTER(name, field)                                                                                            \
    uint8_t model_updater_set_##name(model_updater_t updater, typeof(((model_t *)0)->field) value) {                   \
        assert(updater != NULL);                                                                                       \
        mut_model_t *pmodel = updater->pmodel;                                                                         \
        if (pmodel->field != value) {                                                                                  \
            pmodel->field = value;                                                                                     \
            return 1;                                                                                                  \
        } else {                                                                                                       \
            return 0;                                                                                                  \
        }                                                                                                              \
    }


#define TOGGLER(name, field)                                                                                           \
    void model_updater_toggle_##name(model_updater_t updater) {                                                        \
        assert(updater != NULL);                                                                                       \
        mut_model_t *pmodel = updater->pmodel;                                                                         \
        pmodel->field       = !pmodel->field;                                                                          \
    }



struct model_updater {
    mut_model_t *pmodel;
};


model_updater_t model_updater_init(mut_model_t *pmodel) {
    model_updater_t updater = malloc(sizeof(struct model_updater));
    updater->pmodel         = pmodel;

    return updater;
}


const model_t *model_updater_read(model_updater_t updater) {
    assert(updater != NULL);
    return (const model_t *)updater->pmodel;
}

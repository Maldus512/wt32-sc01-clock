#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>


#define GETTER(name, field)                                                                                            \
    static inline                                                                                                      \
        __attribute__((always_inline, const)) typeof(((mut_model_t *)0)->field) model_get_##name(model_t *model) {     \
        assert(model != NULL);                                                                                         \
        return model->field;                                                                                           \
    }

#define SETTER(name, field)                                                                                            \
    static inline __attribute__((always_inline))                                                                       \
    uint8_t model_set_##name(mut_model_t *model, typeof(((model_t *)0)->field) value) {                                \
        assert(model != NULL);                                                                                         \
        if (model->field != value) {                                                                                   \
            model->field = value;                                                                                      \
            return 1;                                                                                                  \
        } else {                                                                                                       \
            return 0;                                                                                                  \
        }                                                                                                              \
    }

#define TOGGLER(name, field)                                                                                           \
    static inline __attribute__((always_inline)) void model_toggle_##name(model_t *model) {                            \
        assert(model != NULL);                                                                                         \
        model->field = !model->field;                                                                                  \
    }

#define GETTERNSETTER(name, field)                                                                                     \
    GETTER(name, field)                                                                                                \
    SETTER(name, field)



struct model {
    struct {
        uint16_t language;
    } config;
};


typedef const struct model model_t;
typedef struct model       mut_model_t;


void model_init(mut_model_t *model);


GETTERNSETTER(language, config.language);


#endif

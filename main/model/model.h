#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>


#define GETTER(name, field)                                                                                            \
    static inline                                                                                                      \
        __attribute__((always_inline, const)) typeof(((mut_model_t *)0)->field) model_get_##name(model_t *pmodel) {     \
        assert(pmodel != NULL);                                                                                         \
        return pmodel->field;                                                                                           \
    }


struct model {
    struct {
        uint16_t language;
    } config;
};


typedef const struct model model_t;
typedef struct model       mut_model_t;


void model_init(mut_model_t *pmodel);


GETTER(language, config.language);


#endif

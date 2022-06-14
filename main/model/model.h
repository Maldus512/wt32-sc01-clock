#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED

#include <stdint.h>

typedef struct {
    struct {
        uint16_t language;
    } configuration;
} model_t;


void model_init(model_t *pmodel);

uint16_t model_get_language(model_t *pmodel);


#endif
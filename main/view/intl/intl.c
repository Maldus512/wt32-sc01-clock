#include <assert.h>
#include "intl.h"


const char *view_intl_get_string_in_language(uint16_t language, strings_t id) {
    return strings[id][language];
}


const char *view_intl_get_string(model_t *pmodel, strings_t id) {
    return view_intl_get_string_in_language(model_get_language(pmodel), id);
}
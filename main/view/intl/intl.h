#ifndef INTL_H_INCLUDED
#define INTL_H_INCLUDED

#include <stdint.h>
#include "AUTOGEN_FILE_strings.h"
#include "model/model.h"


const char *view_intl_get_string(const model_t *model, strings_t id);
const char *view_intl_get_string_in_language(uint16_t language, strings_t id);


#endif

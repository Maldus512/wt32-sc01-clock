#ifndef PERSISTANCE_H_INCLUDED
#define PERSISTANCE_H_INCLUDED


#include "model/model.h"


void persistance_load(mut_model_t *model);
void persistance_save_variable(void *old_value, const void *memory, uint16_t size, void *user_ptr, void *arg);


extern const char *PERSISTANCE_NORMAL_BRIGHTNESS_KEY;
extern const char *PERSISTANCE_STANDBY_BRIGHTNESS_KEY;
extern const char *PERSISTANCE_STANDBY_DELAY_KEY;


#endif

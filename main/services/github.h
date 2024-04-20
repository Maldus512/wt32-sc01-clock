#ifndef GITHUB_H_INCLUDED
#define GITHUB_H_INCLUDED


#include "model/model.h"


void    github_request_latest_release(mut_model_t *pmodel);
uint8_t github_manage(mut_model_t *pmodel);
void    github_ota(mut_model_t *pmodel);


#endif

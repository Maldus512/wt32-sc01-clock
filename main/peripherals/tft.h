#ifndef TFT_H_INCLUDED
#define TFT_H_INCLUDED


#include "lvgl.h"
#include <stdint.h>


void tft_init(void (*touch_cb)(void));
void tft_backlight_set(uint8_t percentage);
void tft_touch_read_cb(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data);


#endif

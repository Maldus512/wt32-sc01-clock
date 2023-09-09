#ifndef STYLE_H_INCLUDED
#define STYLE_H_INCLUDED

#include "lvgl.h"


LV_FONT_DECLARE(font_digital_7_184);
LV_FONT_DECLARE(font_digital_7_64);


#define STYLE_FONT_BIG    (&lv_font_montserrat_48)
#define STYLE_FONT_MEDIUM (&lv_font_montserrat_32)
#define STYLE_FONT_SMALL  (&lv_font_montserrat_24)
#define STYLE_FONT_TINY   (&lv_font_montserrat_16)


#define STYLE_MAIN_COLOR ((lv_color_t)LV_COLOR_MAKE(0x20, 0xC2, 0x0E))
#define STYLE_GRAY       ((lv_color_t)LV_COLOR_MAKE(0xA1, 0xA1, 0xA1))
#define STYLE_RED        ((lv_color_t)LV_COLOR_MAKE(0xE1, 0x00, 0x00))


void style_init(void);

#endif

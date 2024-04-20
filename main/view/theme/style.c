#include "lvgl.h"
#include "style.h"


#define LV_STYLE_CONST_NULL                                                                                            \
    {                                                                                                                  \
        LV_STYLE_PROP_INV, { 0 }                                                                                       \
    }


static const lv_style_const_prop_t style_padless_cont_props[] = {
    LV_STYLE_CONST_PAD_BOTTOM(0), LV_STYLE_CONST_PAD_TOP(0),    LV_STYLE_CONST_PAD_LEFT(0), LV_STYLE_CONST_PAD_RIGHT(0),
    LV_STYLE_CONST_PAD_ROW(0),    LV_STYLE_CONST_PAD_COLUMN(0), LV_STYLE_CONST_NULL,
};
LV_STYLE_CONST_INIT(style_padless_cont, style_padless_cont_props);


static const lv_style_const_prop_t style_transparent_cont_props[] = {
    LV_STYLE_CONST_RADIUS(0),
    LV_STYLE_CONST_BORDER_WIDTH(0),
    LV_STYLE_CONST_BG_OPA(0),
    LV_STYLE_CONST_NULL,
};
LV_STYLE_CONST_INIT(style_transparent_cont, style_transparent_cont_props);


void style_init(void) {}

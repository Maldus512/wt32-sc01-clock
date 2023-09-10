#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include "model/updater.h"
#include "view/view.h"
#include "view/common.h"
#include "view/theme/style.h"
#include "src/extra/libs/qrcode/lv_qrcode.h"
#include "config/app_config.h"
#include <esp_log.h>


LV_IMG_DECLARE(img_bell);


#define FLAG_WIDTH       64
#define FLAG_HEIGHT      64
#define FLAG_LEFT_LIMIT  -36
#define FLAG_RIGHT_LIMIT 444


enum {
    TIMER_TIME_ID,
    TIMER_ALARMS_ID,
    OBJ_FLAG_ID,
    BTN_CONNECT_ID,
    BTN_REFRESH_ID,
    BTN_BELL_ID,
    CB_MILITARY_TIME_ID,
    SLIDER_NORMAL_BRIGHTNESS_ID,
    SLIDER_STANDBY_BRIGHTNESS_ID,
    SLIDER_STANDBY_DELAY_ID,
    CALENDAR_ID,
    WATCHER_WIFI_ID,
    TAB_ID,
};


typedef enum {
    MENU_STATE_CLOSED = 0,
    MENU_STATE_OPENED,
} menu_state_t;


struct page_data {
    lv_obj_t *lbl_colon;
    lv_obj_t *lbl_hours;
    lv_obj_t *lbl_minutes;
    lv_obj_t *lbl_seconds;
    lv_obj_t *lbl_date;
    lv_obj_t *lbl_ampm;
    lv_obj_t *lbl_alarms;

    lv_obj_t *btn_bell;
    lv_obj_t *btn_flag;

    lv_obj_t *obj_menu;

    struct {
        lv_obj_t *spinner;
        lv_obj_t *lbl_network;
        lv_obj_t *btn_refresh;
        lv_obj_t *list_networks;
    } wifi;

    struct {
        lv_obj_t *calendar;
    } alarms;

    struct {
        lv_obj_t *cb_military_time;
        lv_obj_t *lbl_normal_brightness;
        lv_obj_t *lbl_standby_brightness;
        lv_obj_t *lbl_standby_delay;
        lv_obj_t *slider_normal_brightness;
        lv_obj_t *slider_standby_brightness;
        lv_obj_t *slider_standby_delay;
    } settings;

    struct {
        lv_obj_t *qr;
    } info;

    size_t tab;
    size_t nth_alarm_today;

    pman_timer_t *timer_time;
    pman_timer_t *timer_alarms;

    menu_state_t menu_state;
};


static void update_time(model_t *pmodel, struct page_data *pdata, uint8_t calendar);
static void update_wifi_list(model_t *pmodel, struct page_data *pdata);
static void update_menu(struct page_data *pdata, int32_t x);
static void update_settings(model_t *pmodel, struct page_data *pdata);
static void update_wifi_state(model_t *pmodel, struct page_data *pdata);
static void btns_value_changed_event_cb(lv_event_t *e);
static void update_alarms(model_t *pmodel, struct page_data *pdata, uint8_t next);


static const char *TAG = "PageMain";


static void *create_page(pman_handle_t handle, void *extra) {
    (void)handle;
    (void)extra;

    struct page_data *pdata = lv_mem_alloc(sizeof(struct page_data));
    assert(pdata != NULL);

    pdata->timer_time   = PMAN_REGISTER_TIMER_ID(handle, 500UL, TIMER_TIME_ID);
    pdata->timer_alarms = PMAN_REGISTER_TIMER_ID(handle, 10000UL, TIMER_ALARMS_ID);
    pdata->menu_state   = MENU_STATE_CLOSED;
    pdata->tab          = 0;

    pdata->nth_alarm_today = 0;

    return pdata;
}


static void open_page(pman_handle_t handle, void *state) {
    lv_obj_t *lbl, *slider, *img, *btn;

    struct page_data *pdata   = state;
    model_updater_t   updater = pman_get_user_data(handle);
    model_t          *pmodel  = model_updater_read(updater);

    VIEW_ADD_WATCHED_VARIABLE(&pmodel->run.scanning, WATCHER_WIFI_ID);
    VIEW_ADD_WATCHED_VARIABLE(&pmodel->run.wifi_state, WATCHER_WIFI_ID);

    pman_timer_resume(pdata->timer_time);
    pman_timer_resume(pdata->timer_alarms);

    lv_obj_t *obj_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_opa(obj_screen, LV_OPA_50, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj_screen, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(obj_screen, 0, LV_STATE_DEFAULT);
    lv_obj_set_size(obj_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(obj_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(obj_screen);

    lbl = lv_label_create(obj_screen);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_MEDIUM, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, STYLE_MAIN_COLOR, LV_STATE_DEFAULT);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl, LV_HOR_RES - FLAG_WIDTH / 2 - 32);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 16, 16);
    pdata->lbl_alarms = lbl;

    lbl = lv_label_create(obj_screen);
    lv_obj_set_style_text_font(lbl, &font_digital_7_184, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, STYLE_MAIN_COLOR, LV_STATE_DEFAULT);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(lbl, ":");
    pdata->lbl_colon = lbl;

    btn = lv_btn_create(obj_screen);
    lv_obj_set_style_bg_color(btn, STYLE_MAIN_COLOR, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    img = lv_img_create(btn);
    lv_obj_set_style_img_recolor(img, STYLE_MAIN_COLOR, LV_STATE_DEFAULT);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_img_set_src(img, &img_bell);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(btn, 72, 72);
    view_register_object_default_callback(btn, BTN_BELL_ID);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    pdata->btn_bell = btn;

    lbl = lv_label_create(obj_screen);
    lv_obj_set_style_text_font(lbl, &font_digital_7_184, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, STYLE_MAIN_COLOR, LV_STATE_DEFAULT);
    pdata->lbl_hours = lbl;

    lbl = lv_label_create(obj_screen);
    lv_obj_set_style_text_font(lbl, &font_digital_7_184, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, STYLE_MAIN_COLOR, LV_STATE_DEFAULT);
    pdata->lbl_minutes = lbl;

    lbl = lv_label_create(obj_screen);
    lv_obj_set_style_text_font(lbl, &font_digital_7_64, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, STYLE_MAIN_COLOR, LV_STATE_DEFAULT);
    pdata->lbl_seconds = lbl;

    lbl = lv_label_create(obj_screen);
    lv_obj_set_style_text_font(lbl, &font_digital_7_64, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, STYLE_MAIN_COLOR, LV_STATE_DEFAULT);
    pdata->lbl_date = lbl;

    lbl = lv_label_create(obj_screen);
    lv_obj_set_style_text_font(lbl, &font_digital_7_64, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lbl, STYLE_MAIN_COLOR, LV_STATE_DEFAULT);
    pdata->lbl_ampm = lbl;

    lv_obj_t *btn_flag = lv_btn_create(obj_screen);
    lv_obj_clear_flag(btn_flag, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(btn_flag, FLAG_WIDTH, FLAG_HEIGHT);
    lv_obj_align_to(btn_flag, obj_screen, LV_ALIGN_OUT_RIGHT_TOP, -FLAG_WIDTH / 2, 8);
    lv_obj_add_flag(btn_flag, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(btn_flag, LV_OBJ_FLAG_IGNORE_LAYOUT);

    lbl = lv_label_create(btn_flag);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
    lv_label_set_text(lbl, LV_SYMBOL_LEFT);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, -8, 0);

    lbl = lv_label_create(btn_flag);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
    lv_label_set_text(lbl, LV_SYMBOL_RIGHT);
    lv_obj_align(lbl, LV_ALIGN_RIGHT_MID, 8, 0);

    view_register_object_default_callback(btn_flag, OBJ_FLAG_ID);

    lv_obj_t *obj_menu = lv_obj_create(obj_screen);
    lv_obj_set_style_pad_all(obj_menu, 0, LV_STATE_DEFAULT);
    lv_obj_set_size(obj_menu, LV_HOR_RES, LV_VER_RES);
    lv_obj_move_foreground(btn_flag);
    lv_obj_align_to(obj_menu, btn_flag, LV_ALIGN_OUT_RIGHT_TOP, -FLAG_WIDTH / 2, 0);

    lv_obj_t *tabview = lv_tabview_create(obj_menu, LV_DIR_RIGHT, 64);
    lv_obj_remove_event_cb(lv_tabview_get_tab_btns(tabview), NULL);
    lv_obj_add_event_cb(lv_tabview_get_tab_btns(tabview), btns_value_changed_event_cb, LV_EVENT_VALUE_CHANGED, pdata);

    /*Add 3 tabs (the tabs are page (lv_page) and can be scrolled*/
    lv_obj_t *tab_alarms   = lv_tabview_add_tab(tabview, LV_SYMBOL_BELL);
    lv_obj_t *tab_wifi     = lv_tabview_add_tab(tabview, LV_SYMBOL_WIFI);
    lv_obj_t *tab_settings = lv_tabview_add_tab(tabview, LV_SYMBOL_SETTINGS);
    lv_obj_t *tab_info     = lv_tabview_add_tab(tabview, LV_SYMBOL_LIST);
    lv_obj_set_style_pad_all(tab_alarms, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(tab_wifi, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(tab_settings, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(tab_info, 0, LV_STATE_DEFAULT);

    /* Alarms tab */
    lv_obj_t *obj_alarms = lv_obj_create(tab_alarms);
    lv_obj_set_style_pad_ver(obj_alarms, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(obj_alarms, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(obj_alarms, FLAG_WIDTH / 2, LV_STATE_DEFAULT);
    lv_obj_set_size(obj_alarms, LV_PCT(100), LV_PCT(100));

    lv_obj_t *calendar = lv_calendar_create(obj_alarms);
    lv_obj_set_size(calendar, 380, LV_PCT(100));
    lv_obj_align(calendar, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(calendar, STYLE_FONT_SMALL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(calendar, 0, LV_STATE_DEFAULT);

    /*Highlight a few days*/
    static lv_calendar_date_t highlighted_days[MAX_ALARMS];

    uint8_t found = 0;
    size_t  count = 0;
    do {
        size_t alarm_num = 0;
        found            = model_get_nth_alarm(pmodel, &alarm_num, count);
        if (found) {
            time_t    timestamp           = model_get_alarm(pmodel, alarm_num).timestamp;
            struct tm alarm_tm            = *localtime(&timestamp);
            highlighted_days[count].year  = alarm_tm.tm_year + 1900;
            highlighted_days[count].month = alarm_tm.tm_mon + 1;
            highlighted_days[count].day   = alarm_tm.tm_mday;
            count++;
        }
    } while (found);


    lv_calendar_set_highlighted_dates(calendar, highlighted_days, count);

    lv_calendar_header_arrow_create(calendar);
    view_register_object_default_callback(calendar, CALENDAR_ID);

    lv_obj_t *btnmatrix = lv_calendar_get_btnmatrix(calendar);
    lv_obj_set_style_bg_color(btnmatrix, STYLE_MAIN_COLOR, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btnmatrix, STYLE_MAIN_COLOR, LV_PART_ITEMS | LV_STATE_PRESSED);

    pdata->alarms.calendar = calendar;

    /* WiFi tab */
    lv_obj_t *obj_wifi = lv_obj_create(tab_wifi);
    lv_obj_set_style_pad_all(obj_wifi, 0, LV_STATE_DEFAULT);
    lv_obj_set_size(obj_wifi, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(obj_wifi, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *spinner = lv_spinner_create(obj_wifi, 2000, 48);
    lv_obj_center(spinner);
    pdata->wifi.spinner = spinner;

    lbl = lv_label_create(obj_wifi);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl, 380);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, FLAG_WIDTH / 2 + 8, 8);
    pdata->wifi.lbl_network = lbl;

    lv_obj_t *list_networks = lv_list_create(obj_wifi);
    lv_obj_set_style_text_font(list_networks, STYLE_FONT_SMALL, LV_STATE_DEFAULT);
    lv_obj_set_size(list_networks, LV_PCT(100), LV_VER_RES - FLAG_HEIGHT - 16);
    lv_obj_align(list_networks, LV_ALIGN_BOTTOM_MID, 0, 0);
    pdata->wifi.list_networks = list_networks;

    lv_obj_t *btn_refresh = lv_btn_create(obj_wifi);
    lv_obj_set_size(btn_refresh, 64, 64);
    lbl = lv_label_create(btn_refresh);
    lv_label_set_text(lbl, LV_SYMBOL_REFRESH);
    lv_obj_center(lbl);
    lv_obj_align(btn_refresh, LV_ALIGN_TOP_RIGHT, -8, 4);
    view_register_object_default_callback(btn_refresh, BTN_REFRESH_ID);
    pdata->wifi.btn_refresh = btn_refresh;

    /* Settings tab */
    lv_obj_t *obj_settings = lv_obj_create(tab_settings);
    lv_obj_set_style_pad_all(obj_settings, 0, LV_STATE_DEFAULT);
    lv_obj_set_size(obj_settings, LV_PCT(100), LV_PCT(100));

    lv_obj_t *checkbox = lv_checkbox_create(obj_settings);
    lv_obj_set_style_text_font(checkbox, STYLE_FONT_SMALL, LV_STATE_DEFAULT | LV_PART_MAIN);
    lv_obj_set_style_text_font(checkbox, STYLE_FONT_SMALL, LV_STATE_DEFAULT | LV_PART_INDICATOR);
    lv_obj_set_style_text_font(checkbox, STYLE_FONT_SMALL, LV_STATE_CHECKED | LV_PART_INDICATOR);
    lv_checkbox_set_text(checkbox, "Military time");
    lv_obj_align(checkbox, LV_ALIGN_TOP_LEFT, FLAG_WIDTH / 2 + 96, 32);
    view_register_object_default_callback(checkbox, CB_MILITARY_TIME_ID);
    pdata->settings.cb_military_time = checkbox;

    lbl = lv_label_create(obj_settings);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_TINY, LV_STATE_DEFAULT);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, 96);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 8, 112);
    lv_label_set_text(lbl, "Brightness");
    lv_obj_t *prev_lbl = lbl;

    lbl = lv_label_create(obj_settings);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_TINY, LV_STATE_DEFAULT);
    lv_obj_align_to(lbl, prev_lbl, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    pdata->settings.lbl_normal_brightness = lbl;

    slider = lv_slider_create(obj_settings);
    lv_slider_set_range(slider, 0, 100);
    lv_obj_set_size(slider, 200, 32);
    lv_obj_align(slider, LV_ALIGN_TOP_RIGHT, -32, 112);
    view_register_object_default_callback(slider, SLIDER_NORMAL_BRIGHTNESS_ID);
    pdata->settings.slider_normal_brightness = slider;

    lbl = lv_label_create(obj_settings);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_TINY, LV_STATE_DEFAULT);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, 96);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 8, 112 + 64);
    lv_label_set_text(lbl, "Standby brightness");
    prev_lbl = lbl;

    lbl = lv_label_create(obj_settings);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_TINY, LV_STATE_DEFAULT);
    lv_obj_align_to(lbl, prev_lbl, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    pdata->settings.lbl_standby_brightness = lbl;

    slider = lv_slider_create(obj_settings);
    lv_slider_set_range(slider, 0, 100);
    lv_obj_set_size(slider, 200, 32);
    lv_obj_align(slider, LV_ALIGN_TOP_RIGHT, -32, 112 + 64);
    view_register_object_default_callback(slider, SLIDER_STANDBY_BRIGHTNESS_ID);
    pdata->settings.slider_standby_brightness = slider;

    lbl = lv_label_create(obj_settings);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_TINY, LV_STATE_DEFAULT);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, 96);
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 8, 112 + 128 + 16);
    lv_label_set_text(lbl, "Standby delay");
    prev_lbl = lbl;

    lbl = lv_label_create(obj_settings);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_TINY, LV_STATE_DEFAULT);
    lv_obj_align_to(lbl, prev_lbl, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    pdata->settings.lbl_standby_delay = lbl;

    slider = lv_slider_create(obj_settings);
    lv_slider_set_range(slider, 5, 300);
    lv_obj_set_size(slider, 200, 32);
    lv_obj_align(slider, LV_ALIGN_TOP_RIGHT, -32, 128 + 128);
    view_register_object_default_callback(slider, SLIDER_STANDBY_DELAY_ID);
    pdata->settings.slider_standby_delay = slider;

    /* Info tab */
    lv_obj_t *obj_info = lv_obj_create(tab_info);
    lv_obj_set_style_pad_all(obj_info, 0, LV_STATE_DEFAULT);
    lv_obj_set_size(obj_info, LV_PCT(100), LV_PCT(100));

    lbl = lv_label_create(obj_info);
    lv_obj_set_style_text_font(lbl, STYLE_FONT_TINY, LV_STATE_DEFAULT);
    lv_label_set_text_fmt(lbl, "Version %i.%i.%i, built with love and ESP-IDF %s", APP_CONFIG_FIRMWARE_VERSION_MAJOR,
                          APP_CONFIG_FIRMWARE_VERSION_MINOR, APP_CONFIG_FIRMWARE_VERSION_PATCH, IDF_VER);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, 360);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, LV_STATE_DEFAULT);
    lv_obj_align(lbl, LV_ALIGN_TOP_RIGHT, -8, 8);

    const char *www = "https://github.com/Maldus512/wt32-sc01-clock";
    lv_obj_t   *qr  = lv_qrcode_create(obj_info, 180, lv_color_black(), lv_color_white());
    // lv_obj_set_style_border_color(qr, STYLE_BG_COLOR, LV_STATE_DEFAULT);
    // lv_obj_set_style_border_width(qr, 8, LV_STATE_DEFAULT);
    lv_qrcode_update(qr, www, strlen(www));
    lv_obj_align(qr, LV_ALIGN_CENTER, 0, 32);
    pdata->info.qr = qr;

    pdata->btn_flag = btn_flag;
    pdata->obj_menu = obj_menu;

    lv_coord_t x = 0;
    switch (pdata->menu_state) {
        case MENU_STATE_CLOSED:
            x = FLAG_RIGHT_LIMIT;
            break;
        case MENU_STATE_OPENED:
            x = FLAG_LEFT_LIMIT;
            break;
    }
    update_menu(pdata, x);

    update_time(pmodel, pdata, 1);
    update_settings(pmodel, pdata);
    update_wifi_list(pmodel, pdata);
    update_wifi_state(pmodel, pdata);
    update_alarms(pmodel, pdata, 0);
    lv_tabview_set_act(tabview, pdata->tab, LV_ANIM_OFF);
}


static pman_msg_t page_event(pman_handle_t handle, void *state, pman_event_t event) {
    (void)handle;

    pman_msg_t msg = PMAN_MSG_NULL;

    struct page_data *pdata   = state;
    model_updater_t   updater = pman_get_user_data(handle);
    model_t          *pmodel  = model_updater_read(updater);

    switch (event.tag) {
        case PMAN_EVENT_TAG_TIMER: {
            switch ((uintptr_t)pman_timer_get_user_data(event.as.timer)) {
                case TIMER_TIME_ID:
                    update_time(pmodel, pdata, 0);
                    break;

                case TIMER_ALARMS_ID:
                    update_alarms(pmodel, pdata, 1);
                    break;
            }
            break;
        }

        case PMAN_EVENT_TAG_LVGL: {
            view_obj_data_t *obj_data = lv_obj_get_user_data(lv_event_get_current_target(event.as.lvgl));
            if (obj_data == NULL) {
                ESP_LOGI(TAG, "Orphan event %i", lv_event_get_code(event.as.lvgl));
                break;
            }

            switch (lv_event_get_code(event.as.lvgl)) {
                case LV_EVENT_VALUE_CHANGED: {
                    switch (obj_data->id) {
                        case CB_MILITARY_TIME_ID: {
                            model_updater_set_military_time(
                                updater, (lv_obj_get_state(pdata->settings.cb_military_time) & LV_STATE_CHECKED) > 0);
                            update_settings(pmodel, pdata);
                            break;
                        }

                        case SLIDER_NORMAL_BRIGHTNESS_ID: {
                            model_updater_set_normal_brightness(
                                updater, lv_slider_get_value(pdata->settings.slider_normal_brightness));
                            update_settings(pmodel, pdata);
                            break;
                        }

                        case SLIDER_STANDBY_BRIGHTNESS_ID: {
                            model_updater_set_standby_brightness(
                                updater, lv_slider_get_value(pdata->settings.slider_standby_brightness));
                            update_settings(pmodel, pdata);
                            break;
                        }

                        case SLIDER_STANDBY_DELAY_ID: {
                            model_updater_set_standby_delay(updater,
                                                            lv_slider_get_value(pdata->settings.slider_standby_delay));
                            update_settings(pmodel, pdata);
                            break;
                        }

                        case CALENDAR_ID: {
                            lv_calendar_date_t *date = lv_mem_alloc(sizeof(lv_calendar_date_t));
                            assert(date != NULL);

                            time_t    now_time = time(NULL);
                            struct tm now_tm   = *localtime(&now_time);

                            if (lv_calendar_get_pressed_date(pdata->alarms.calendar, date) == LV_RES_OK) {
                                struct tm then_tm = {
                                    .tm_year = date->year - 1900,
                                    .tm_mon  = date->month - 1,
                                    .tm_mday = date->day,
                                    .tm_hour = 23,
                                    .tm_min  = 59,
                                    .tm_sec  = 59,
                                };
                                time_t then_time = mktime(&then_tm);
                                if (then_time >= now_time) {
                                    msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE_EXTRA(&page_alarms, date);
                                }
                            }
                            break;
                        }
                    }
                    break;
                }

                case LV_EVENT_CLICKED: {
                    switch (obj_data->id) {
                        case BTN_BELL_ID: {
                            lv_calendar_date_t *date = lv_mem_alloc(sizeof(lv_calendar_date_t));
                            assert(date != NULL);

                            time_t    now_time = time(NULL);
                            struct tm now_tm   = *localtime(&now_time);

                            date->day     = now_tm.tm_mday;
                            date->month   = now_tm.tm_mon + 1;
                            date->year    = now_tm.tm_year + 1900;
                            msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE_EXTRA(&page_alarms, date);
                            break;
                        }

                        case BTN_CONNECT_ID: {
                            char *ssid = lv_mem_alloc(MAX_SSID_SIZE);
                            assert(ssid != NULL);
                            snprintf(ssid, MAX_SSID_SIZE, "%s", pmodel->run.ap_list[obj_data->number].ssid);

                            msg.stack_msg = PMAN_STACK_MSG_PUSH_PAGE_EXTRA(&page_wifi_psk, (void *)ssid);
                            break;
                        }

                        case BTN_REFRESH_ID:
                            msg.user_msg = view_controller_msg(
                                (view_controller_msg_t){.tag = VIEW_CONTROLLER_MESSAGE_TAG_SCAN_AP});
                            break;

                        case OBJ_FLAG_ID: {
                            lv_coord_t x = 0;

                            switch (pdata->menu_state) {
                                case MENU_STATE_CLOSED:
                                    x = FLAG_LEFT_LIMIT;
                                    break;
                                case MENU_STATE_OPENED:
                                    x = FLAG_RIGHT_LIMIT;
                                    break;
                            }

                            update_menu(pdata, x);
                            break;
                        }
                    }
                    break;
                }

                case LV_EVENT_PRESSED: {
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case PMAN_EVENT_TAG_USER: {
            view_event_t *view_event = event.as.user;
            switch (view_event->tag) {
                case VIEW_EVENT_TAG_WIFI_SCAN_DONE:
                    ESP_LOGI(TAG, "Wifi scan done");
                    update_wifi_list(pmodel, pdata);
                    break;

                case VIEW_EVENT_TAG_VARIABLE_WATCHER: {
                    switch (view_event->as.page_watcher.code) {
                        case WATCHER_WIFI_ID:
                            ESP_LOGI(TAG, "Wifi state change: %i %i", model_get_wifi_state(pmodel),
                                     model_get_scanning(pmodel));
                            update_wifi_state(pmodel, pdata);
                            break;
                    }
                    break;
                }
            }
            break;
        }


        default:
            break;
    }

    return msg;
}


static void close_page(void *state) {
    struct page_data *pdata = state;
    pman_timer_pause(pdata->timer_time);
    pman_timer_pause(pdata->timer_alarms);
    lv_obj_clean(lv_scr_act());
}


static void destroy_page(void *state, void *extra) {
    (void)extra;
    struct page_data *pdata = state;
    pman_timer_delete(pdata->timer_time);
    pman_timer_delete(pdata->timer_alarms);
    lv_mem_free(pdata);
}


static void update_time(model_t *pmodel, struct page_data *pdata, uint8_t calendar) {
    time_t     now       = time(NULL);
    struct tm *tm_struct = localtime(&now);

    uint16_t hours = tm_struct->tm_hour;
    if (!model_get_military_time(pmodel)) {
        hours = hours % 12;
        hours = hours == 0 ? 12 : hours;
    }

    lv_label_set_text_fmt(pdata->lbl_hours, "%02i", hours);
    lv_label_set_text_fmt(pdata->lbl_minutes, "%02i", tm_struct->tm_min);
    lv_label_set_text_fmt(pdata->lbl_seconds, ":%02i", tm_struct->tm_sec);
    lv_label_set_text_fmt(pdata->lbl_date, "%02i/%02i/%02i", tm_struct->tm_mday, tm_struct->tm_mon + 1,
                          tm_struct->tm_year % 100);
    lv_label_set_text(pdata->lbl_ampm, tm_struct->tm_hour > 13 ? "PM" : "AM");

    lv_obj_align_to(pdata->lbl_hours, pdata->lbl_colon, LV_ALIGN_OUT_LEFT_MID, 0, 0);
    lv_obj_align_to(pdata->lbl_minutes, pdata->lbl_colon, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_obj_align_to(pdata->lbl_seconds, pdata->lbl_minutes, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);
    lv_obj_align_to(pdata->lbl_date, pdata->lbl_hours, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_align_to(pdata->lbl_ampm, pdata->lbl_colon, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    view_common_set_hidden(pdata->lbl_ampm, model_get_military_time(pmodel));

    if (pdata->menu_state == MENU_STATE_CLOSED || calendar) {
        lv_calendar_set_today_date(pdata->alarms.calendar, tm_struct->tm_year + 1900, tm_struct->tm_mon + 1,
                                   tm_struct->tm_mday);
        lv_calendar_set_showed_date(pdata->alarms.calendar, tm_struct->tm_year + 1900, tm_struct->tm_mon + 1);
    }
}


static void update_settings(model_t *pmodel, struct page_data *pdata) {
    if (model_get_military_time(pmodel)) {
        lv_obj_add_state(pdata->settings.cb_military_time, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(pdata->settings.cb_military_time, LV_STATE_CHECKED);
    }

    lv_slider_set_value(pdata->settings.slider_normal_brightness, model_get_normal_brightness(pmodel), LV_ANIM_OFF);
    lv_label_set_text_fmt(pdata->settings.lbl_normal_brightness, "%i%%", model_get_normal_brightness(pmodel));

    lv_slider_set_value(pdata->settings.slider_standby_brightness, model_get_standby_brightness(pmodel), LV_ANIM_OFF);
    lv_label_set_text_fmt(pdata->settings.lbl_standby_brightness, "%i%%", model_get_standby_brightness(pmodel));

    lv_slider_set_value(pdata->settings.slider_standby_delay, model_get_standby_delay_seconds(pmodel), LV_ANIM_OFF);
    lv_label_set_text_fmt(pdata->settings.lbl_standby_delay, "%is", model_get_standby_delay_seconds(pmodel));
}


static void update_menu(struct page_data *pdata, int32_t x) {
    if (x < FLAG_LEFT_LIMIT) {
        x = FLAG_LEFT_LIMIT;
    } else if (x > FLAG_RIGHT_LIMIT) {
        x = FLAG_RIGHT_LIMIT;
    }

    lv_obj_set_x(pdata->btn_flag, x);
    lv_obj_align_to(pdata->obj_menu, pdata->btn_flag, LV_ALIGN_OUT_RIGHT_TOP, -FLAG_WIDTH / 2, -8);

    if (x == FLAG_LEFT_LIMIT) {
        pdata->menu_state = MENU_STATE_OPENED;
    } else if (x == FLAG_RIGHT_LIMIT) {
        pdata->menu_state = MENU_STATE_CLOSED;
    }
}


static void update_wifi_state(model_t *pmodel, struct page_data *pdata) {
    if (model_get_scanning(pmodel)) {
        view_common_set_hidden(pdata->wifi.list_networks, 1);
        view_common_set_hidden(pdata->wifi.btn_refresh, 1);
        view_common_set_hidden(pdata->wifi.spinner, 0);
    } else {
        view_common_set_hidden(pdata->wifi.list_networks, 0);
        view_common_set_hidden(pdata->wifi.spinner, 1);
        view_common_set_hidden(pdata->wifi.btn_refresh, 0);
    }

    switch (model_get_wifi_state(pmodel)) {
        case WIFI_STATE_CONNECTED: {
            uint32_t ip = model_get_ip_addr(pmodel);
            lv_label_set_text_fmt(pdata->wifi.lbl_network, "%s\n%i.%i.%i.%i", model_get_ssid(pmodel), IP_PART(ip, 0),
                                  IP_PART(ip, 1), IP_PART(ip, 2), IP_PART(ip, 3));
            break;
        }

        case WIFI_STATE_CONNECTING:
            if (strlen(model_get_ssid(pmodel)) > 0) {
                lv_label_set_text_fmt(pdata->wifi.lbl_network, "Connecting to \"%s\"", model_get_ssid(pmodel));
            } else {
                lv_label_set_text(pdata->wifi.lbl_network, "No network");
            }
            break;

        case WIFI_STATE_DISCONNECTED:
            lv_label_set_text(pdata->wifi.lbl_network, "No network");
            break;
    }
}


static void update_wifi_list(model_t *pmodel, struct page_data *pdata) {
    lv_obj_clean(pdata->wifi.list_networks);
    for (size_t i = 0; i < model_get_available_networks_count(pmodel); i++) {
        char string[64] = {0};
        snprintf(string, sizeof(string), "[%3i] %s", pmodel->run.ap_list[i].rssi, pmodel->run.ap_list[i].ssid);
        lv_obj_t *btn = lv_list_add_btn(pdata->wifi.list_networks, NULL, string);
        lv_obj_t *lbl = lv_obj_get_child(btn, 0);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
        view_register_object_default_callback_with_number(btn, BTN_CONNECT_ID, i);
    }
}


static void update_alarms(model_t *pmodel, struct page_data *pdata, uint8_t next) {
    size_t alarm_num = 0;

    time_t    time_now = time(NULL);
    struct tm tm_now   = *localtime(&time_now);

    if (next) {
        pdata->nth_alarm_today++;
        if (!model_get_nth_alarm_for_day(pmodel, &alarm_num, pdata->nth_alarm_today, tm_now.tm_mday, tm_now.tm_mon,
                                         tm_now.tm_year)) {
            pdata->nth_alarm_today = 0;
        }
    }

    if (model_get_nth_alarm_for_day(pmodel, &alarm_num, pdata->nth_alarm_today, tm_now.tm_mday, tm_now.tm_mon,
                                    tm_now.tm_year)) {
        lv_label_set_text(pdata->lbl_alarms, model_get_alarm(pmodel, alarm_num).description);
        view_common_set_hidden(pdata->lbl_alarms, 0);
        view_common_set_hidden(pdata->btn_bell, 0);
        view_common_set_hidden(pdata->lbl_colon, 1);
    } else {
        view_common_set_hidden(pdata->lbl_alarms, 1);
        view_common_set_hidden(pdata->btn_bell, 1);
        view_common_set_hidden(pdata->lbl_colon, 0);
    }
}


static void btns_value_changed_event_cb(lv_event_t *e) {
    lv_obj_t         *btns  = lv_event_get_target(e);
    struct page_data *pdata = lv_event_get_user_data(e);

    lv_obj_t *tv = lv_obj_get_parent(btns);
    uint32_t  id = lv_btnmatrix_get_selected_btn(btns);
    pdata->tab   = id;
    lv_tabview_set_act(tv, id, LV_ANIM_OFF);
}


const pman_page_t page_main = {
    .create        = create_page,
    .destroy       = destroy_page,
    .open          = open_page,
    .close         = close_page,
    .process_event = page_event,
};

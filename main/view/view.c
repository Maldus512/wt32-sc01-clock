#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "page_manager.h"
#include "watcher.h"
#include "config/app_config.h"
#include "model/updater.h"
#include "view.h"
#include "theme/style.h"
#include "theme/theme.h"
#include "services/system_time.h"
#include "esp_log.h"


#define DISPLAY_HORIZONTAL_RESOLUTION LV_HOR_RES_MAX
#define DISPLAY_VERTICAL_RESOLUTION   LV_VER_RES_MAX
#define MAX_VIEW_EVENTS               32
#ifdef SIMULATED_APPLICATION
#define BUFFER_SIZE (DISPLAY_HORIZONTAL_RESOLUTION * 200)
#else
#include "lvgl_helpers.h"
#define BUFFER_SIZE DISP_BUF_SIZE
#endif


static void view_watcher_cb(void *old_value, const void *memory, uint16_t size, void *user_ptr, void *arg);
static void close_page_global_cb(void *user_ptr, void *page_state);


view_controller_msg_t view_cmsg   = {0};
static const char    *TAG         = "View";
static pman_t         pman        = {0};
static QueueHandle_t  event_queue = NULL;
static watcher_t      watcher;


void view_init(model_updater_t updater, pman_user_msg_cb_t controller_cb,
               void (*flush_cb)(struct _lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p),
               void (*read_cb)(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data)) {
    (void)TAG;

    event_queue = xQueueCreate(MAX_VIEW_EVENTS, sizeof(view_event_t));

    lv_init();

    watcher_init(&watcher, NULL);

    /*A static or global variable to store the buffers*/
    static lv_disp_draw_buf_t disp_buf;

    /*Static or global buffer(s). The second buffer is optional*/
    static lv_color_t buf_1[BUFFER_SIZE] = {0};

    /*Initialize `disp_buf` with the buffer(s). With only one buffer use NULL instead buf_2 */
    lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, BUFFER_SIZE);

    static lv_disp_drv_t disp_drv;                     /*A variable to hold the drivers. Must be static or global.*/
    lv_disp_drv_init(&disp_drv);                       /*Basic initialization*/
    disp_drv.draw_buf = &disp_buf;                     /*Set an initialized buffer*/
    disp_drv.flush_cb = flush_cb;                      /*Set a flush callback to draw to the display*/
    disp_drv.hor_res  = DISPLAY_HORIZONTAL_RESOLUTION; /*Set the horizontal resolution in pixels*/
    disp_drv.ver_res  = DISPLAY_VERTICAL_RESOLUTION;   /*Set the vertical resolution in pixels*/

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
    style_init();
    theme_init(disp);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv); /*Basic initialization*/
    indev_drv.long_press_repeat_time = 250UL;
    indev_drv.type                   = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb                = read_cb;
    /*Register the driver in LVGL and save the created input device object*/
    lv_indev_t *touch_indev = lv_indev_drv_register(&indev_drv);

    pman_init(&pman, updater, touch_indev, controller_cb, close_page_global_cb);
}


void view_change_page(const pman_page_t *page) {
    pman_change_page(&pman, *page);
}


void view_register_object_default_callback(lv_obj_t *obj, int id) {
    view_register_object_default_callback_with_number(obj, id, 0);
}


void view_register_object_default_callback_with_number(lv_obj_t *obj, int id, int number) {
    view_obj_data_t *data = lv_mem_alloc(sizeof(view_obj_data_t));
    assert(data != NULL);
    data->id     = id;
    data->number = number;

    lv_obj_set_user_data(obj, data);
    pman_register_obj_event(&pman, obj, LV_EVENT_ALL);
    pman_set_obj_self_destruct(obj);
}


void view_manage(void) {
    watcher_watch(&watcher, get_millis());

    view_event_t event = {0};
    while (xQueueReceive(event_queue, &event, 0)) {
        pman_event(&pman, (pman_event_t){.tag = PMAN_EVENT_TAG_USER, .as = {.user = &event}});
    }
}


void view_event(view_event_t event) {
    xQueueSend(event_queue, &event, 0);
}


void view_add_watched_variable(void *ptr, size_t size, int code) {
    assert(watcher.entries.num < MAX_VIEW_EVENTS - 1);
    watcher_add_entry(&watcher, ptr, size, view_watcher_cb, (void *)(uintptr_t)code);
}


static void view_watcher_cb(void *old_value, const void *memory, uint16_t size, void *user_ptr, void *arg) {
    (void)old_value;
    (void)memory;
    (void)size;
    (void)user_ptr;
    view_event((view_event_t){.tag = VIEW_EVENT_TAG_VARIABLE_WATCHER, .as.page_watcher.code = (uintptr_t)arg});
}


static void close_page_global_cb(void *user_ptr, void *page_state) {
    (void)user_ptr;
    (void)page_state;
    xQueueReset(event_queue);

    watcher_destroy(&watcher);
    watcher_init(&watcher, NULL);
}

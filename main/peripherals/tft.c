#include "sdkconfig.h"
#include "lvgl_helpers.h"
#include "esp_log.h"

#include "lvgl_tft/disp_spi.h"
#include "lvgl_touch/tp_spi.h"

#include "lvgl_spi_conf.h"

#include "lvgl_i2c/i2c_manager.h"

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif


static const char *TAG              = "Tft";
static void       *backlight_handle = NULL;
static void (*on_touch_cb)(void);


void tft_init(void (*touch_cb)(void)) {
    on_touch_cb = touch_cb;

    lvgl_i2c_init(I2C_NUM_0);

    /* Since LVGL v8 LV_HOR_RES_MAX and LV_VER_RES_MAX are not defined, so
     * print it only if they are defined. */
#if (LVGL_VERSION_MAJOR < 8)
    ESP_LOGI(TAG, "Display hor size: %d, ver size: %d", LV_HOR_RES_MAX, LV_VER_RES_MAX);
#endif

    ESP_LOGI(TAG, "Display buffer size: %d", DISP_BUF_SIZE);

#if defined(CONFIG_LV_TFT_DISPLAY_CONTROLLER_FT81X)
    ESP_LOGI(TAG, "Initializing SPI master for FT81X");

    lvgl_spi_driver_init(TFT_SPI_HOST, DISP_SPI_MISO, DISP_SPI_MOSI, DISP_SPI_CLK, SPI_BUS_MAX_TRANSFER_SZ, 1,
                         DISP_SPI_IO2, DISP_SPI_IO3);

    disp_spi_add_device(TFT_SPI_HOST);
    backlight_handle = disp_driver_init();

#if defined(CONFIG_LV_TOUCH_CONTROLLER_FT81X)
    touch_driver_init();
#endif

    return;
#endif

#if defined(SHARED_SPI_BUS)
    ESP_LOGI(TAG, "Initializing shared SPI master");

    lvgl_spi_driver_init(TFT_SPI_HOST, TP_SPI_MISO, DISP_SPI_MOSI, DISP_SPI_CLK, SPI_BUS_MAX_TRANSFER_SZ, 1, -1, -1);

    disp_spi_add_device(TFT_SPI_HOST);
    tp_spi_add_device(TOUCH_SPI_HOST);

    backlight_handle = disp_driver_init();
    touch_driver_init();

    return;
#endif

/* Display controller initialization */
#if defined CONFIG_LV_TFT_DISPLAY_PROTOCOL_SPI
    ESP_LOGI(TAG, "Initializing SPI master for display");

    lvgl_spi_driver_init(TFT_SPI_HOST, DISP_SPI_MISO, DISP_SPI_MOSI, DISP_SPI_CLK, SPI_BUS_MAX_TRANSFER_SZ, 1,
                         DISP_SPI_IO2, DISP_SPI_IO3);

    disp_spi_add_device(TFT_SPI_HOST);

    backlight_handle = disp_driver_init();
#elif defined(CONFIG_LV_I2C_DISPLAY)
    backlight_handle = disp_driver_init();
#else
#error "No protocol defined for display controller"
#endif

/* Touch controller initialization */
#if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
#if defined(CONFIG_LV_TOUCH_DRIVER_PROTOCOL_SPI)
    ESP_LOGI(TAG, "Initializing SPI master for touch");

    lvgl_spi_driver_init(TOUCH_SPI_HOST, TP_SPI_MISO, TP_SPI_MOSI, TP_SPI_CLK, 0 /* Defaults to 4094 */, 2, -1, -1);

    tp_spi_add_device(TOUCH_SPI_HOST);

    touch_driver_init();
#elif defined(CONFIG_LV_I2C_TOUCH)
    touch_driver_init();
#elif defined(CONFIG_LV_TOUCH_DRIVER_ADC)
    touch_driver_init();
#elif defined(CONFIG_LV_TOUCH_DRIVER_DISPLAY)
    touch_driver_init();
#else
#error "No protocol defined for touch controller"
#endif
#else
#endif
}


void tft_backlight_set(uint8_t percentage) {
    disp_backlight_set(backlight_handle, percentage);
}


void tft_touch_read_cb(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    ft6x36_read(indev_drv, data);
    if (data->state == LV_INDEV_STATE_PRESSED) {     // Ignore no touch and multi touch
        if (on_touch_cb != NULL) {
            on_touch_cb();
        }
    }
}

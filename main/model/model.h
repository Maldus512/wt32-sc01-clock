#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>


#define GETTER(name, field)                                                                                            \
    static inline                                                                                                      \
        __attribute__((always_inline, const)) typeof(((mut_model_t *)0)->field) model_get_##name(model_t *pmodel) {    \
        assert(pmodel != NULL);                                                                                        \
        return pmodel->field;                                                                                          \
    }
#define IP_PART(ip, p) ((uint8_t)((ip >> (8 * p)) & 0xFF))

#define MAX_SSID_SIZE         33
#define MAX_AP_SCAN_LIST_SIZE 16
#define MAX_ALARMS            64
#define MAX_DESCRIPTION_LEN   64


typedef enum {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
} wifi_state_t;


typedef enum {
    FIRMWARE_UPDATE_STATE_TAG_NONE = 0,
    FIRMWARE_UPDATE_STATE_TAG_UPDATING,
    FIRMWARE_UPDATE_STATE_TAG_SUCCESS,
    FIRMWARE_UPDATE_STATE_TAG_FAILURE,
} firmware_update_state_tag_t;


typedef enum {
    FIRMWARE_UPDATE_FAILURE_CODE_MISSING_PARTITION = 0x01,
    FIRMWARE_UPDATE_FAILURE_CODE_OTA_BEGIN,
    FIRMWARE_UPDATE_FAILURE_CODE_OOM,
    FIRMWARE_UPDATE_FAILURE_CODE_WRITE,
    FIRMWARE_UPDATE_FAILURE_CODE_RECEIVE,
    FIRMWARE_UPDATE_FAILURE_CODE_IMAGE,
    FIRMWARE_UPDATE_FAILURE_CODE_BOOT_PARTITION,
} firmware_update_failure_code_t;


typedef enum {
    HTTP_REQUEST_STATE_NONE = 0,
    HTTP_REQUEST_STATE_DONE,
    HTTP_REQUEST_STATE_WAITING,
    HTTP_REQUEST_STATE_ERROR,
} http_request_state_t;


typedef struct {
    firmware_update_state_tag_t    tag;
    firmware_update_failure_code_t failure_code;
    uint32_t                       error;
} firmware_update_state_t;

typedef struct {
    uint64_t timestamp;
    char     description[MAX_DESCRIPTION_LEN + 1];
} alarm_t;


struct model {
    struct {
        uint16_t num_alarms;
        alarm_t  alarms[MAX_ALARMS];
        uint8_t  military_time;
        uint8_t  normal_brightness;
        uint8_t  standby_brightness;
        uint16_t standby_delay_seconds;
        uint8_t  night_mode;
        uint32_t night_mode_start;
        uint32_t night_mode_end;
    } config;

    struct {
        size_t ap_list_size;
        struct {
            char    ssid[MAX_SSID_SIZE];
            int16_t rssi;
        } ap_list[MAX_AP_SCAN_LIST_SIZE];
        uint8_t                 scanning;
        wifi_state_t            wifi_state;
        uint32_t                ip_addr;
        char                    ssid[MAX_SSID_SIZE];
        firmware_update_state_t server_firmware_update_state;
        firmware_update_state_t client_firmware_update_state;

        uint8_t              new_release_notified;
        http_request_state_t latest_release_request_state;
        uint16_t             latest_release_major;
        uint16_t             latest_release_minor;
        uint16_t             latest_release_patch;
    } run;
};


typedef const struct model model_t;
typedef struct model       mut_model_t;


void        model_init(mut_model_t *pmodel);
const char *model_get_ssid(model_t *pmodel);
alarm_t     model_get_alarm(model_t *pmodel, size_t num);
size_t      model_get_active_alarms(model_t *pmodel);
uint8_t     model_is_alarm_expired(model_t *pmodel, size_t alarm_num);
uint8_t     model_get_nth_alarm_for_day(model_t *pmodel, size_t *alarm_num, size_t nth, uint16_t day, uint16_t month,
                                        uint16_t year);
uint8_t     model_get_nth_alarm(model_t *pmodel, size_t *alarm_num, size_t nth);
void        model_set_latest_release_state(mut_model_t *pmodel, http_request_state_t request_state, uint16_t major,
                                           uint16_t minor, uint16_t patch);
void        model_set_night_mode(mut_model_t *pmodel, uint8_t night_mode);
uint8_t     model_get_standby_brightness(model_t *pmodel);
uint8_t     model_is_new_release_available(model_t *pmodel);
firmware_update_state_t model_get_firmware_update_state(model_t *pmodel);


GETTER(military_time, config.military_time);
GETTER(normal_brightness, config.normal_brightness);
GETTER(standby_delay_seconds, config.standby_delay_seconds);

GETTER(wifi_state, run.wifi_state);
GETTER(scanning, run.scanning);
GETTER(available_networks_count, run.ap_list_size);
GETTER(ip_addr, run.ip_addr);


#endif

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
#define IP_PART(ip, p) ((ip >> (8 * p)) & 0xFF)

#define MAX_SSID_SIZE         33
#define MAX_AP_SCAN_LIST_SIZE 16
#define MAX_ALARMS            64
#define MAX_DESCRIPTION_LEN   64


typedef enum {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
} wifi_state_t;


typedef struct {
    uint64_t timestamp;
    char     description[MAX_DESCRIPTION_LEN + 1];
    uint8_t  whole_day;
} alarm_t;


struct model {
    struct {
        size_t   num_alarms;
        alarm_t  alarms[MAX_ALARMS];
        uint8_t  military_time;
        uint8_t  normal_brightness;
        uint8_t  standby_brightness;
        uint16_t standby_delay_seconds;
    } config;

    struct {
        size_t ap_list_size;
        struct {
            char    ssid[MAX_SSID_SIZE];
            int16_t rssi;
        } ap_list[MAX_AP_SCAN_LIST_SIZE];
        uint8_t      scanning;
        wifi_state_t wifi_state;
        uint32_t     ip_addr;
        char         ssid[MAX_SSID_SIZE];
    } run;
};


typedef const struct model model_t;
typedef struct model       mut_model_t;


void        model_init(mut_model_t *pmodel);
const char *model_get_ssid(model_t *pmodel);


GETTER(military_time, config.military_time);
GETTER(normal_brightness, config.normal_brightness);
GETTER(standby_brightness, config.standby_brightness);
GETTER(standby_delay_seconds, config.standby_delay_seconds);

GETTER(wifi_state, run.wifi_state);
GETTER(scanning, run.scanning);
GETTER(available_networks_count, run.ap_list_size);
GETTER(ip_addr, run.ip_addr);


#endif

#ifndef SYSTEM_TIME_H_INCLUDED
#define SYSTEM_TIME_H_INCLUDED


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"


typedef unsigned long timestamp_t;


#define time_after(a, b)          ((long)((b) - (a)) < 0)
#define time_after_or_equal(a, b) ((long)((b) - (a)) <= 0)
#define time_before(a, b)         time_after(b, a)

#define is_expired(start, current, delay) is_loosely_expired(start, current, delay)


#define get_millis() (xTaskGetTickCount() * portTICK_PERIOD_MS)


static inline __attribute__((always_inline)) timestamp_t time_interval(timestamp_t a, timestamp_t b) {
    if (time_after(a, b))
        return -((timestamp_t)b - (timestamp_t)a);
    else
        return (timestamp_t)b - (timestamp_t)a;
}


static inline __attribute__((always_inline)) int is_loosely_expired(timestamp_t start, timestamp_t current,
                                                                    timestamp_t delay) {
    return time_after_or_equal(current, start + delay);
}


static inline __attribute__((always_inline)) int is_strictly_expired(timestamp_t start, timestamp_t current,
                                                                     timestamp_t delay) {
    return time_after(current, start + delay);
}


#endif 

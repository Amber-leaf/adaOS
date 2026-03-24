#ifndef TIMING_H_
#define TIMING_H_

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t cpu_time_t;

typedef struct cpu_timer {
    bool bound;
    uint64_t synced_cpu_id;

    cpu_time_t (*ticks_callback)(void);
    cpu_time_t (*ms_callback)(void);
    void (*sleep_ms_callback)(uint32_t);
} cpu_timer_t;

typedef struct rtc {
    void (*unix_timestamp_callback)(void);

    // TODO: hours/min/sec/etc... setters and getters.
} rtc_t;

cpu_timer_t get_timer();
void bind_timer(cpu_time_t (*ticks_callback)(void), cpu_time_t (*ms_callback)(void), void (*sleep_ms_callback)(uint32_t));

#endif
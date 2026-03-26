#include "header/timing.h"
#include "../../util/header/log.h"
#include "header/cpu.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

cpu_timer_t timers[10]; // TODO: figure out how many we are supposed to have. 10
                        // seems safe.
uint8_t bound_index;

void bind_timer(cpu_time_t (*ticks_callback)(void),
                cpu_time_t (*ms_callback)(void),
                void (*sleep_ms_callback)(uint32_t)) {
  k_debug("binding");
  cpu_timer_t new_timer;
  new_timer.ticks_callback = ticks_callback;
  new_timer.ms_callback = ms_callback;
  new_timer.sleep_ms_callback = sleep_ms_callback;
  k_debug("before");

  new_timer.synced_cpu_id = get_cpu()->id;
  k_debug("got cpu");

  new_timer.bound = true;
  timers[bound_index++] = new_timer;
}

cpu_timer_t get_timer() {
  if (get_cpu()->timer.bound)
    return get_cpu()->timer;
  else {
    for (uint8_t i = 0; i < bound_index; i++) {
      if (get_cpu()->id == timers[i].synced_cpu_id) {
        get_cpu()->timer = timers[i];
        return timers[i];
      }
    }
    k_wrn("Timer for %d is unbound, scheduler is gonna tweak.", get_cpu()->id);
    return get_cpu()->timer;
  }
}
#include "../util/header/log.h"
#include <stdbool.h>
#include <stdint.h>

uint64_t pit_ticks = 0;

extern bool pit_as_scheduler_timer;

void pit_irq() {
  if (!pit_as_scheduler_timer) {
    pit_ticks++;
  } else {
    k_debug("pit non timer");
  }
}

uint64_t get_pit_ticks() { return pit_ticks; }

void reset_pit_ticks() { pit_ticks = 0; }
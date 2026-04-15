#include "../scheduler/header/scheduler.h"

#include <stdbool.h>
#include <stdint.h>

extern bool interrupt_as_timer;

uint64_t apic_ticks = 0;

void apic_timer_irq() {
  apic_ticks++;

  if (!interrupt_as_timer) {
    preempt();
  }
}

uint64_t get_apic_ticks() { return apic_ticks; }

void reset_apic_ticks() { apic_ticks = 0; }
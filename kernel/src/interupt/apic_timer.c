#include "../platform/x86_64/header/apic.h"
#include "../util/header/log.h"

#include <stdbool.h>
#include <stdint.h>

extern bool interrupt_as_timer;

uint64_t apic_ticks = 0;

void apic_timer_irq() {
  if (interrupt_as_timer) {
    apic_ticks++;
  } else {
    k_debug("APIC Interrupt non-timer");
    apic_restore_state();
  }
}

uint64_t get_apic_ticks() { return apic_ticks; }

void reset_apic_ticks() { apic_ticks = 0; }
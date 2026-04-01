#include "../platform/x86_64/header/apic.h"
#include "../scheduler/header/scheduler.h"
#include "../util/header/printf.h"

#include <stdbool.h>
#include <stdint.h>

extern bool interrupt_as_timer;

uint64_t apic_ticks = 0;

void apic_timer_irq() {
  //__asm__ __volatile__("cli");

  apic_ticks++;

  if (!interrupt_as_timer) {
    // interrupt_as_timer = true;
    // apic_interrupt_ms(1000);

    preempt();
  }

  //__asm__ __volatile__("sti");
}

uint64_t get_apic_ticks() { return apic_ticks; }

void reset_apic_ticks() { apic_ticks = 0; }
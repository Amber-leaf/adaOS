#include <stdint.h>

uint64_t apic_ticks = 0;

void apic_timer_irq() { apic_ticks++; }

uint64_t get_apic_ticks() { return apic_ticks; }

void reset_apic_ticks() { apic_ticks = 0; }
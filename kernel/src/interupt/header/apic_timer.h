#ifndef APIC_TIMER_H_
#define APIC_TIMER_H_

#include <stdint.h>
void apic_timer_irq();

uint64_t get_apic_ticks();
void reset_apic_ticks();

#endif
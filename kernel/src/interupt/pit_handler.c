#include <stdbool.h>
#include <stdint.h>

uint64_t pit_ticks = 0;

void pit_irq() { pit_ticks++; }

uint64_t get_pit_ticks() { return pit_ticks; }

void reset_pit_ticks() { pit_ticks = 0; }
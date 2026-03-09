#include <stdint.h>

uint64_t ticks = 0;

void pit_irq() { ticks++; }

uint64_t get_ticks() { return ticks; }

void reset_ticks() { ticks = 0; }
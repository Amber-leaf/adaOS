#include <stdint.h>

uint64_t ticks = 0;

void pit_irq() { ticks++; }
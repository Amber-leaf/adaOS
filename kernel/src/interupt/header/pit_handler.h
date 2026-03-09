#ifndef PIT_HANDLER_H_
#define PIT_HANDLER_H_

#include <stdint.h>
void pit_irq();

uint64_t get_ticks();
void reset_ticks();

#endif
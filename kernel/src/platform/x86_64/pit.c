#include "header/pit.h"
#include "header/pic.h"
#include "header/port.h"
#include <stddef.h>

#define PIT_CHANNEL0_PORT 0x40
#define PIT_CONTROL_PORT 0x43

#define PIT_FREQUENCY 1193182

#define PIT_GOAL_FREQUENCY 1000 // Hz

size_t goal_frequency;

void setup_pit() {
  __asm__ __volatile__("cli");

  goal_frequency = PIT_GOAL_FREQUENCY;
  uint32_t divisor = PIT_FREQUENCY / PIT_GOAL_FREQUENCY;
  outb(PIT_CONTROL_PORT, 0x36 | 0x02);
  outb(PIT_CHANNEL0_PORT, divisor & 0xFF);
  outb(PIT_CHANNEL0_PORT, divisor >> 8);

  unmask_irq(0);

  __asm__ __volatile__("sti");
}
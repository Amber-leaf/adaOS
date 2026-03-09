#include "header/pit.h"
#include "../../interupt/header/pit_handler.h"

#include "header/pic.h"
#include "header/port.h"

#include <stddef.h>

#define PIT_CHANNEL0_PORT 0x40
#define PIT_CONTROL_PORT 0x43

#define PIT_FREQUENCY 1193182

#define PIT_GOAL_FREQUENCY 1000 // Hz

uint32_t goal_frequency;

void pit_sleep_ms(uint32_t ms) {
  uint64_t start_ticks = get_ticks();
  uint64_t end_ticks = (start_ticks + (ms * goal_frequency) / 1000) - 1;

  while (get_ticks() < end_ticks) {
    uint64_t current_ticks = get_ticks();

    if (current_ticks < start_ticks) {
      start_ticks = current_ticks;
      end_ticks = start_ticks + (ms * goal_frequency) / 1000;
    }

    asm volatile("hlt");
  }
}

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
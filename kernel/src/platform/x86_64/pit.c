#include "header/pit.h"
#include "../../interupt/header/pit_handler.h"

#include "header/pic.h"
#include "header/port.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PIT_FREQUENCY 1193182

#define PIT_GOAL_FREQUENCY 1000 // Hz

#define PIT_CHANNEL0_PORT 0x40
#define PIT_CHANNEL2_PORT 0x42
#define PIT_CONTROL_PORT 0x43

uint32_t timer_goal_frequency;

void set_pit_frequency(uint32_t frequency) {
  uint32_t divisor = PIT_FREQUENCY / frequency;
  outb(PIT_CONTROL_PORT, 0x36);
  outb(PIT_CHANNEL0_PORT, divisor & 0xFF);
  outb(PIT_CHANNEL0_PORT, divisor >> 8);
}

void set_speaker_frequency(uint32_t frequency) {
  uint32_t divisor = PIT_FREQUENCY / frequency;
  outb(PIT_CONTROL_PORT, 0xB6);
  outb(PIT_CHANNEL2_PORT, divisor & 0xFF);
  outb(PIT_CHANNEL2_PORT, divisor >> 8);
}

void play_sound(uint32_t frequency) {
  set_speaker_frequency(frequency);
  uint8_t tmp = inb(0x61);
  if ((tmp & 3) != 3) {
    outb(0x61, tmp | 3);
  }
}

void sound_off() {
  uint8_t tmp = inb(0x61) & 0xFC;
  outb(0x61, tmp);
}

void pit_sleep_ms(uint32_t ms) {
  set_pit_frequency(timer_goal_frequency);

  uint64_t start_ticks = get_pit_ticks();
  uint64_t end_ticks = (start_ticks + (ms * timer_goal_frequency) / 1000) - 1;

  while (get_pit_ticks() < end_ticks) {
    uint64_t current_ticks = get_pit_ticks();

    if (current_ticks < start_ticks) {
      start_ticks = current_ticks;
      end_ticks = start_ticks + (ms * timer_goal_frequency) / 1000;
    }

    asm volatile("hlt");
  }
}

void setup_pit() {
  __asm__ __volatile__("cli");

  timer_goal_frequency = PIT_GOAL_FREQUENCY;
  set_pit_frequency(timer_goal_frequency);

  unmask_irq(0);

  __asm__ __volatile__("sti");
}
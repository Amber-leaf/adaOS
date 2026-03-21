#include <stdint.h>

#include "../header/core.h"
#include "../platform/x86_64/header/apic.h"
#include "../platform/x86_64/header/pit.h"
#include "header/log.h"
#include "header/state.h"

#define PANIC_SOUND_INTERVAL 200
#define PANIC_SOUND_FREQUENCY 400
#define PANIC_BEEPS 6

uint8_t panics = 0;

void __attribute__((noreturn))
panic(char *msg) { // TODO: add proper cleanup and tracing.
  if (panics > 0) {
    k_err("Something is super fucky, panic panicked %d time(s). yay osdev >:3",
          panics);
    hcf(); // just stop, i dont even trust the pit to play sound if we get
           // here here.
  }

  panics++;

  k_err("Unrecoverable error: %s Halt.", msg);

  if (get_global_state().pit_initialized) {
    for (uint8_t i = 0; i < PANIC_BEEPS; i++) {
      play_sound(PANIC_SOUND_FREQUENCY);
      if (get_global_state().apic_initialized == 0xf0) {
        apic_sleep_ms(PANIC_SOUND_INTERVAL);
      } else {
        pit_sleep_ms(PANIC_SOUND_INTERVAL);
      }
      sound_off();
      if (get_global_state().apic_initialized == 0xf0) {
        apic_sleep_ms(PANIC_SOUND_INTERVAL);
      }
    }
  }

  hcf();
}
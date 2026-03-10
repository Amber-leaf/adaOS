#include "../header/core.h"
#include "../platform/x86_64//header/pit.h"

#include "header/log.h"
#include <stdint.h>

#define PANIC_SOUND_INTERVAL 200
#define PANIC_SOUND_FREQUENCY 400
#define PANIC_BEEPS 6

void __attribute__((noreturn)) panic(char *msg) { // todo
  k_err("Unrecoverable error: %s Halt.", msg);

  if (get_pit_configured()) {
    for (uint8_t i = 0; i < PANIC_BEEPS; i++) {
      play_sound(PANIC_SOUND_FREQUENCY);
      pit_sleep_ms(PANIC_SOUND_INTERVAL);
      sound_off();
      pit_sleep_ms(PANIC_SOUND_INTERVAL);
    }
  }

  hcf();
}
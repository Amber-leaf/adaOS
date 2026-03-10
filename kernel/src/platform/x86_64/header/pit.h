#ifndef PIT_H_
#define PIT_H_

#include <stdint.h>
#include <stdbool.h>

void setup_pit();
void pit_sleep_ms(uint32_t ms);
void play_sound(uint32_t frequency);
void sound_off();
bool get_pit_configured();

#endif
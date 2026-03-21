#include "header/random.h"
#include "../util/header/log.h"

#include <stdint.h>

// Taken from https://en.wikipedia.org/wiki/Mersenne_Twister.
mt_state_t local_state;

void setup_random(uint32_t seed) {
  mt_state_t *state = &local_state;
  uint32_t *state_array = &(state->state_array[0]);

  state_array[0] = seed;

  for (int i = 1; i < _n; i++) {
    seed = _f * (seed ^ (seed >> (_w - 2))) +
           i; // Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier.
    state_array[i] = seed;
  }

  state->state_index = 0;

  local_state = *state;
}

uint32_t random_uint32() {
  mt_state_t *state = &local_state;
  uint32_t *state_array = &(state->state_array[0]);

  int k = state->state_index; // point to current state location
                              // 0 <= state_index <= n-1   always

  //  int k = k - n;                   // point to state n iterations before
  //  if (k < 0) k += n;               // modulo n circular indexing
  // the previous 2 lines actually do nothing
  //  for illustration only

  int j = k - (_n - 1); // point to state n-1 iterations before
  if (j < 0)
    j += _n; // modulo n circular indexing

  uint32_t x = (state_array[k] & UMASK) | (state_array[j] & LMASK);

  uint32_t xA = x >> 1;
  if (x & 0x00000001UL)
    xA ^= _a;

  j = k - (_n - _m); // point to state n-m iterations before
  if (j < 0)
    j += _n; // modulo n circular indexing

  x = state_array[j] ^ xA; // compute next value in the state
  state_array[k++] = x;    // update new state value

  if (k >= _n)
    k = 0; // modulo n circular indexing
  state->state_index = k;

  uint32_t y = x ^ (x >> _u); // tempering
  y = y ^ ((y << _s) & _b);
  y = y ^ ((y << _t) & _c);
  uint32_t z = y ^ (y >> _l);

  local_state = *state;

  return z;
}

// note that this is not cryptographicly secure. (min - max]
uint32_t random_range(uint32_t min, uint32_t max) {
  if (min > max) {
    k_err("Bad random range!");
    return -1;
  }

  if (max == 0) {
    return min;
  }

  uint32_t range = max - min + 1;
  return min + (random_uint32() % range);
}

mt_state_t get_state() { return local_state; }

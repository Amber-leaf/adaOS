#ifndef RANDOM_H_
#define RANDOM_H_

#include <stdint.h>
#define _n 624
#define _m 397
#define _w 32
#define _r 31
#define UMASK (0xffffffffUL << _r)
#define LMASK (0xffffffffUL >> (_w - _r))
#define _a 0x9908b0dfUL
#define _u 11
#define _s 7
#define _t 15
#define _l 18
#define _b 0x9d2c5680UL
#define _c 0xefc60000UL
#define _f 1812433253UL

typedef struct {
  uint32_t state_array[_n]; // the array for the state vector
  int state_index; // index into state vector array, 0 <= state_index <= n-1
                   // always
} mt_state_t;

void setup_random(uint32_t seed);
uint32_t random_uint32();
uint32_t random_range(uint32_t min, uint32_t max);

#endif
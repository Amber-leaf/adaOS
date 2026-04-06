#include "header/state.h"

#include <stddef.h>
#include <stdint.h>

struct global_state state;
inline struct global_state get_global_state() { return state; }

void set_state(size_t index, uint8_t value) {
  uint8_t *ptr = (uint8_t *)&state + (index * sizeof(uint8_t));

  ptr[0] = value;
}

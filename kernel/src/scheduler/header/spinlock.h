#include "stdint.h"
#include "stdbool.h"

typedef struct spinlock {
  bool locked;
  uint64_t saved_flags;
} spinlock_t;

#define SPINLOCK_INIT_VALUE {false, 0}

bool spinlock_try(spinlock_t *lock);
void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);
void spinlock_release_no_sti(spinlock_t *lock);
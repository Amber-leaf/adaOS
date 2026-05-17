#include "header/spinlock.h"
#include "../util/header/printf.h"
#include "stdbool.h"
#include "stdint.h"

inline bool spinlock_try(spinlock_t *lock) {
  return __sync_bool_compare_and_swap(&lock->locked, false, true);
}

inline void spinlock_acquire(spinlock_t *lock) {
  uint64_t flags;
  __asm__ __volatile__("pushfq\n\t"
                       "pop %0\n\t"
                       "cli"
                       : "=r"(flags)
                       :
                       : "memory");
  while (!__sync_bool_compare_and_swap(&lock->locked, false, true)) {
    while (lock->locked) {
      serial_printf_("  -spinning on %s-  ", lock->debug_name);

      asm volatile("pause" : : : "memory");
    }
  }
  lock->saved_flags = flags;
}

inline void spinlock_release(spinlock_t *lock) {
  uint64_t flags = lock->saved_flags;
  __atomic_store_n(&lock->locked, false, __ATOMIC_RELEASE);
  if (flags & (1 << 9)) {
    __asm__ __volatile__("sti");
  }
}

inline void spinlock_release_no_sti(spinlock_t *lock) {
  __atomic_store_n(&lock->locked, false, __ATOMIC_RELEASE);
}
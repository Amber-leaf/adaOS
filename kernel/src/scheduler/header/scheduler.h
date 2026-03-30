#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "../../memory/virtual/header/vmm.h"

#include <stddef.h>
#include <stdint.h>

#define SPINLOCK_INIT_VALUE false

#define SPINLOCK_DEFINE(name) spinlock_t name = SPINLOCK_INIT_VALUE

#define SIGKILL 0;
#define SIGSTOP 1;

#define STATUS_WAIT 0
#define STATUS_DEAD 1
#define STATUS_RUNNING 2
#define STATUS_WAIT_ON_EXIT 3


typedef bool spinlock_t;
typedef uint32_t thread_id_t;
typedef uint8_t signal_t;

typedef struct thread {
  thread_id_t id;

  uintptr_t *stack_ptr;

  void *stack_base;
  void *stack_bounds;

  bool kernel_thread;

  uint8_t status;
  uint8_t priority;
  int32_t allotment;

  pagemap_t *pagemap;

  uint64_t times_ran;

} thread_t;

void setup_scheduler();

static inline bool spinlock_try(spinlock_t *lock) {
  return __sync_bool_compare_and_swap(lock, false, true);
}

static inline void spinlock_acquire(spinlock_t *lock) {
  while (!__sync_bool_compare_and_swap(lock, false, true)) {
    while (*lock)
      asm volatile("pause" : : : "memory");
  }
}

static inline void spinlock_release(spinlock_t *lock) {
  __atomic_store_n(lock, 0, __ATOMIC_RELEASE);
}

#endif
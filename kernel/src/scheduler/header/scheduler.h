#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "../../platform/x86_64/header/timing.h"
#include <stdint.h>

typedef bool spinlock_t;

#define SPINLOCK_INIT_VALUE false

#define SPINLOCK_INIT(x) x = SPINLOCK_INIT_VALUE
#define SPINLOCK_DEFINE(x) spinlock_t x = SPINLOCK_INIT_VALUE

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

typedef struct proccess {
  struct thread *threads;
  uint32_t num_threads;
  uint8_t priority;

} proccess_t;

typedef struct thread {
  struct proccess root_proccess;

  struct thread *next_thread;
  struct thread *prev_thread;

} thread_t;

typedef struct execution_queue_entry {
  struct execution_queue_entry *previous;
  proccess_t execution_target;
  struct execution_queue_entry *next;

} execution_queue_entry_t;

struct execution_queue {
  cpu_time_t preempt_quantum;
  struct execution_queue *higher;
  struct execution_queue *lower;

  void (*schedule_callback)(proccess_t proccess);
};

#endif
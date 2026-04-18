#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "../../memory/virtual/header/vmm.h"
#include "../../util/header/printf.h"
#include "spinlock.h"

#include <stddef.h>
#include <stdint.h>

#define SPINLOCK_DEFINE(name) static spinlock_t name = SPINLOCK_INIT_VALUE

#define THREAD_CONDITION_MAX_WAITERS 32

#define STATUS_READY 0
#define STATUS_RUNNING 1
#define STATUS_ASLEEP 2

#define FLAGS_NONE 1 << 0

#define ARGS_NONE (args_t){0}

#define CAST64(x) (uint64_t)(x)

#define ARGS_0()                                                               \
  CAST64(0), CAST64(0), CAST64(0), CAST64(0), CAST64(0), CAST64(0)
#define ARGS_1(a)                                                              \
  CAST64(a), CAST64(0), CAST64(0), CAST64(0), CAST64(0), CAST64(0)
#define ARGS_2(a, b)                                                           \
  CAST64(a), CAST64(b), CAST64(0), CAST64(0), CAST64(0), CAST64(0)
#define ARGS_3(a, b, c)                                                        \
  CAST64(a), CAST64(b), CAST64(c), CAST64(0), CAST64(0), CAST64(0)
#define ARGS_4(a, b, c, d)                                                     \
  CAST64(a), CAST64(b), CAST64(c), CAST64(d), CAST64(0), CAST64(0)
#define ARGS_5(a, b, c, d, e)                                                  \
  CAST64(a), CAST64(b), CAST64(c), CAST64(d), CAST64(e), CAST64(0)
#define ARGS_6(a, b, c, d, e, f)                                               \
  CAST64(a), CAST64(b), CAST64(c), CAST64(d), CAST64(e), CAST64(f)

#define ARGS_COUNT(_1, _2, _3, _4, _5, _6, N, ...) N
#define ARGS_N(...) ARGS_COUNT(__VA_ARGS__, 6, 5, 4, 3, 2, 1, 0)

#define ARGS_DISPATCH(N, ...) ARGS_##N(__VA_ARGS__)
#define ARGS_DISPATCH2(N, ...) ARGS_DISPATCH(N, __VA_ARGS__)

#define ARGS(...) ((args_t){ARGS_DISPATCH2(ARGS_N(__VA_ARGS__), __VA_ARGS__)})

typedef uint32_t thread_id_t;
typedef uint8_t signal_t;

typedef struct thread {
  thread_id_t id;

  uintptr_t stack_ptr;

  void *stack_base;
  void *stack_bounds;

  bool kernel_thread;

  uint8_t status;

  bool awaiting_death;

  uint8_t priority;
  int32_t allotment;

  pagemap_t *pagemap;

  uint64_t times_ran;

  char *name;
} thread_t;

typedef struct thread_condition {
  thread_t** waiters;
  uint64_t num_waiters;
} thread_condition_t;

typedef struct args {
  uint64_t arg1, arg2, arg3, arg4, arg5, arg6;
} args_t;

void setup_scheduler();

void preempt();

thread_t *thread_start(void (*entrypoint)(void), pagemap_t *pagemap, char *name,
                       uint8_t flags, args_t args);
void thread_yield();

thread_t *thread_self();

void thread_sleep(thread_t *thread);

void thread_awake(thread_t *thread);

void thread_condition_signal(thread_condition_t* thread_condition);

void thread_condition_wait(thread_condition_t* thread_condition, spinlock_t lock);

void thread_condition_init(thread_condition_t *cond);

void pop_thread(thread_t *thread);

#endif
#include "header/scheduler.h"
#include "../header/core.h"
#include "../memory/header/heap.h"
#include "../memory/header/memmap.h"
#include "../platform/x86_64/header/apic.h"
#include "../platform/x86_64/header/gdt.h"
#include "header/spinlock.h"

#include "../util/header/log.h"
#include "../util/header/panic.h"
#include "../util/header/printf.h"
#include "workers/header/reaper.h"

#include <stdbool.h>
#include <stdint.h>

#define THREAD_STACK_SIZE (PAGE_SIZE * 20)

#define S_QUANTUM 0x150

#define BASE_PREEMPT_QUANTUM_MS 10

#define NUM_QUEUES 8

#define INITIAL_THREAD_BUFFER 256

#define MAX_ALLOTMENT 2

#define KERNEL_THREAD_MAX_PRIORITY 0
#define USER_THREAD_MAX_PRIORITY 2

SPINLOCK_DEFINE(scheduler_lock);
SPINLOCK_DEFINE(threads_lock);
SPINLOCK_DEFINE(thread_start_lock);
SPINLOCK_DEFINE(reorder_lock);

thread_id_t new_id = 0;

extern pagemap_t *kernel_pagemap;

thread_t **threads;
thread_t **sleeping_threads;

uint32_t thread_count = 0;
uint32_t sleeping_thread_count = 0;

thread_t *running_thread;
thread_t *last_running_thread;

uint32_t in_queue_thread_index = 0; // TODO: CPU local

extern void task_switch(spinlock_t *spinlock, uintptr_t *old_sp,
                        uintptr_t new_sp);
extern void thread_init_trampoline();

uint32_t times_preempted = 0;

void dump_threads() {
  spinlock_acquire(&threads_lock);
  for (uint8_t i = 0; i < thread_count; i++) {
    thread_t *thread = threads[i];
    k_debug("[thread %d]:", thread->id);
    k_debug("- name: %s", thread->name);
    k_debug("- priority: %d", thread->priority);
    k_debug("- allotment: %d", thread->allotment);
    k_debug("- times ran: %d", thread->times_ran);
    k_debug("- status: %d", thread->status);
  }
  spinlock_release(&threads_lock);
}

void reorder_threads_list() {
  // spinlock_acquire(&reorder_lock);

  for (uint32_t i = 1; i < thread_count; i++) {
    struct thread *key = threads[i];

    int32_t j = i - 1;

    while (j >= 0 && threads[j]->priority > key->priority) {
      threads[j + 1] = threads[j];
      j--;
    }
    threads[j + 1] = key;
  }

  // spinlock_release(&reorder_lock);
}

thread_t *thread_self(void) {
  spinlock_acquire(&threads_lock);
  thread_t *t = running_thread;
  spinlock_release(&threads_lock);
  return t;
}

void pop_thread(thread_t *thread) {
  spinlock_acquire(&scheduler_lock);

  // TODO: Shrink array if possible once done.
  for (uint32_t i = 0; i < thread_count; i++) {
    if (threads[i]->id == thread->id) {
      threads[i] = NULL;

      for (uint32_t q = i; q < thread_count - 1; q++) {
        threads[q] = threads[q + 1];
      }

      threads[thread_count--] = NULL;

      reorder_threads_list();

      spinlock_release(&scheduler_lock);

      return;
    }
  }

  k_err("Could not pop thread %d! Could not match IDs with a running thread!",
        thread->id);

  in_queue_thread_index = 0;

  spinlock_release(&scheduler_lock);
}

void thread_await_death() {
  k_debug("Thread %d (%s) exiting", running_thread->id, running_thread->name);

  spinlock_acquire(&scheduler_lock);

  running_thread->zombie = true;

  in_queue_thread_index = 0;

  spinlock_release(&scheduler_lock);

  while (true) {
  }
}

void thread_yield() {
  thread_self()->allotment = MAX_ALLOTMENT;
  preempt();
}

void thread_sleep(thread_t *thread) {
  if (thread == NULL) {
    k_err("thread_sleep: NULL thread");
    return;
  }

  spinlock_acquire(&scheduler_lock);
  k_debug("thread sleep");

  if (sleeping_thread_count >= INITIAL_THREAD_BUFFER) {
    k_err("thread_sleep: sleeping thread buffer full");
    spinlock_release(&scheduler_lock);
    return;
  }

  bool found = false;
  for (uint32_t i = 0; i < thread_count; i++) {
    if (threads[i]->id == thread->id) {
      for (uint32_t q = i; q < thread_count - 1; q++) {
        threads[q] = threads[q + 1];
      }
      threads[--thread_count] = NULL;
      found = true;
      break;
    }
  }

  if (!found) {
    k_err("thread_sleep: thread %d not found in active queue", thread->id);
    spinlock_release(&scheduler_lock);
    return;
  }

  thread->status = STATUS_ASLEEP;
  sleeping_threads[sleeping_thread_count++] = thread;

  in_queue_thread_index = 0;

  spinlock_release(&scheduler_lock);

  if (thread == running_thread) {
    preempt();
  }
}

void thread_awake(thread_t *thread) {
  if (thread == NULL) {
    k_err("thread_awake: NULL thread");
    return;
  }

  spinlock_acquire(&threads_lock);

  if (thread->status != STATUS_ASLEEP) {
    k_err("thread_awake: thread %d is not sleeping (status %d)", thread->id,
          thread->status);
    spinlock_release(&threads_lock);
    return;
  }

  if (thread_count >= INITIAL_THREAD_BUFFER) {
    k_err("thread_awake: active thread buffer full");
    spinlock_release(&threads_lock);
    return;
  }

  bool found = false;
  for (uint32_t i = 0; i < sleeping_thread_count; i++) {
    if (sleeping_threads[i]->id == thread->id) {
      for (uint32_t q = i; q < sleeping_thread_count - 1; q++) {
        sleeping_threads[q] = sleeping_threads[q + 1];
      }
      sleeping_threads[--sleeping_thread_count] = NULL;
      found = true;
      break;
    }
  }

  if (!found) {
    k_err("thread_awake: thread %d not found in sleeping queue", thread->id);
    spinlock_release(&threads_lock);
    return;
  }

  thread->status = STATUS_READY;
  thread->allotment = MAX_ALLOTMENT;
  threads[thread_count++] = thread;

  reorder_threads_list();

  spinlock_release(&threads_lock);
}

void thread_debug(char *s) {
  k_debug("Thread %s (%d): ", running_thread->name, running_thread->id);
  k_debug("- Status: %d", running_thread->status);
  k_debug("- Args: %s", s);
}

static thread_t *thread_allocate(void (*entrypoint)(void), pagemap_t *pagemap,
                                 char *name, args_t args) {
  thread_t *thread = kmalloc(sizeof(thread_t));

  if (thread_count > INITIAL_THREAD_BUFFER) {
    k_todo("Make threads array expandable.");
    return NULL;
  }

  memset(thread, 0x00, sizeof(thread_t));

  thread->stack_base = kmalloc(THREAD_STACK_SIZE);

  memset(thread->stack_base, 0x00, THREAD_STACK_SIZE);

  thread->stack_bounds =
      (void *)(((uintptr_t)thread->stack_base + THREAD_STACK_SIZE) & ~0xFULL);

  thread->id = new_id++;

  thread->allotment = MAX_ALLOTMENT;

  thread->status = STATUS_READY;

  thread->times_ran = 0;

  if (pagemap != NULL) {
    thread->pagemap = pagemap;
  } else {
    thread->pagemap = kernel_pagemap;
  }

  thread->kernel_thread = thread->pagemap == kernel_pagemap;

  thread->priority = (thread->kernel_thread ? KERNEL_THREAD_MAX_PRIORITY
                                            : USER_THREAD_MAX_PRIORITY);

  thread->name = name;

  uintptr_t *stack = (uintptr_t *)thread->stack_bounds;

  *--stack = 0; // padding
  *--stack = (uintptr_t)thread_await_death;
  *--stack = (uintptr_t)entrypoint;
  *--stack = (uintptr_t)thread_init_trampoline;

  *--stack = args.arg1; // rbx
  *--stack = args.arg2; // rbp
  *--stack = args.arg3; // r12
  *--stack = args.arg4; // r13
  *--stack = args.arg5; // r14
  *--stack = args.arg6; // r15

  thread->stack_ptr = (uintptr_t)stack;

  return thread;
}

thread_t *thread_start(void (*entrypoint)(void), pagemap_t *pagemap, char *name,
                       uint8_t flags, args_t args) {
  spinlock_acquire(&thread_start_lock);

  if (flags != FLAGS_NONE && flags != 0) {
    k_todo("Thread Flags");
  }

  thread_t *t = thread_allocate(entrypoint, pagemap, name, args);

  t->status = STATUS_READY;

  threads[thread_count++] = t;

  reorder_threads_list();

  spinlock_release(&thread_start_lock);

  return t;
}

void preempt() {
  spinlock_acquire(&scheduler_lock);

  if (!thread_count) {
    // Asked to preempt with no threads! just wait a bit and hope for some work.
    apic_interrupt_ms(500);
    spinlock_release(&scheduler_lock);
    return;
  }

  if (running_thread != NULL) {
    last_running_thread = running_thread;
  }

  uint32_t queue_number = threads[0]->priority;
  uint32_t queue_end_index = 0;

  bool s = times_preempted++ % S_QUANTUM == 0;

  for (uint32_t i = 0; i < thread_count; i++) {
    if (s) {
      threads[i]->priority =
          (threads[i]->kernel_thread ? KERNEL_THREAD_MAX_PRIORITY
                                     : USER_THREAD_MAX_PRIORITY);
      threads[i]->allotment = MAX_ALLOTMENT;
    } else {
      if (threads[i]->priority == queue_number) {
        queue_end_index = i;
      } else {
        break;
      }
    }
  }

retry:
  if (in_queue_thread_index > queue_end_index) {
    in_queue_thread_index = 0;
  }

  running_thread = threads[in_queue_thread_index++];

  if (running_thread->allotment <= 0) {
    running_thread->priority++;
    if (running_thread->priority > NUM_QUEUES) {
      running_thread->priority = NUM_QUEUES;
    } else {
      running_thread->allotment = MAX_ALLOTMENT;
      reorder_threads_list();
    }
  } else {
    running_thread->allotment--;
  }

  if (last_running_thread == running_thread) {
    apic_interrupt_ms(BASE_PREEMPT_QUANTUM_MS * (running_thread->priority + 1));

    spinlock_release(&scheduler_lock);
    return;
  }

  switch_to_pagemap(running_thread->pagemap);

  switch_tss_stack((uintptr_t)running_thread->stack_bounds);

  running_thread->times_ran++;

  running_thread->status = STATUS_RUNNING;

  uint64_t new_rsp = running_thread->stack_ptr;
  uint64_t *old_rsp;

  if (last_running_thread) {
    last_running_thread->status = STATUS_READY;
    old_rsp = &last_running_thread->stack_ptr;
  } else {
    old_rsp = &new_rsp;
  }

  if (!new_rsp) {
    panic("Bad stack ptr");
    spinlock_release(&scheduler_lock);
    return;
  }

  spinlock_release(&scheduler_lock);
  apic_interrupt_ms(BASE_PREEMPT_QUANTUM_MS * (running_thread->priority + 1));

  task_switch(&scheduler_lock, old_rsp, new_rsp);
}

void setup_scheduler() {
  threads = kmalloc(sizeof(thread_t *) * INITIAL_THREAD_BUFFER);
  sleeping_threads = kmalloc(sizeof(thread_t *) * INITIAL_THREAD_BUFFER);

  apic_interrupt_ms(BASE_PREEMPT_QUANTUM_MS);

  thread_start((void *)reaper_t_main, NULL, "Reaper", FLAGS_NONE,
               ARGS(&thread_count, &threads));
}
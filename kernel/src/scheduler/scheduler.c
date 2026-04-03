#include "header/scheduler.h"
#include "../header/core.h"
#include "../memory/header/heap.h"
#include "../memory/header/memmap.h"
#include "../platform/x86_64/header/apic.h"
#include "../platform/x86_64/header/gdt.h"

#include "../util/header/log.h"
#include "../util/header/printf.h"

#include <stdbool.h>
#include <stdint.h>

#define THREAD_STACK_SIZE (PAGE_SIZE * 20)

#define S_QUANTUM 0x200

#define BASE_PREEMPT_QUANTUM_MS 10

#define NUM_QUEUES 8

#define INITIAL_THREAD_BUFFER 256

#define MAX_ALLOTMENT 2

SPINLOCK_DEFINE(scheduler_lock);
SPINLOCK_DEFINE(threads_lock);

thread_id_t new_id = 0;

extern pagemap_t *kernel_pagemap;

thread_t **threads;
thread_t **sleeping_threads;

uint32_t thread_count;
thread_t *running_thread;
thread_t *last_running_thread;

extern void task_switch(uintptr_t *old_sp, uintptr_t new_sp);
extern void thread_init_trampoline();

uint32_t times_preempted = 0;

uint32_t in_queue_thread_index = 0;

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
  spinlock_acquire(&threads_lock);
  for (uint32_t i = 1; i < thread_count; i++) {
    struct thread *key = threads[i];

    int32_t j = i - 1;

    while (j >= 0 && threads[j]->priority > key->priority) {
      threads[j + 1] = threads[j];
      j--;
    }
    threads[j + 1] = key;
  }
  spinlock_release(&threads_lock);
}

thread_t *thread_self() {
  spinlock_acquire(&threads_lock);
  thread_t *t = running_thread;
  spinlock_release(&threads_lock);
  return t;
}

void pop_thread(thread_t *thread) {
  spinlock_acquire(&threads_lock);

  // TODO: Shrink array if possible once done.
  for (uint32_t i = 0; i < thread_count; i++) {
    if (threads[i]->id == thread->id) {
      threads[i] = NULL;

      for (uint32_t q = i; q < thread_count - 1; q++) {
        threads[q] = threads[q + 1];
      }

      threads[thread_count--] = NULL;

      spinlock_release(&threads_lock);

      reorder_threads_list();

      return;
    }
  }

  k_err("Could not kill thread %d! Could not match IDs with a running thread!",
        thread->id);

  in_queue_thread_index = 0;

  spinlock_release(&threads_lock);
}

void thread_destroy(thread_t *thread) {
  pop_thread(thread);

  spinlock_acquire(&threads_lock);

  kfree(thread->stack_base);
  kfree(thread);
  in_queue_thread_index = 0;

  spinlock_release(&threads_lock);
}

void thread_await_death() {
  k_debug("Thread %d exiting", running_thread->id);

  thread_destroy(running_thread);

  while (true) {
  }
}

void thread_yield() {
  running_thread->allotment = MAX_ALLOTMENT;
  preempt();
}

void thread_debug(char *c) {
  k_debug("Thread %s (%d): ", running_thread->name, running_thread->id);
  k_debug("- Status: %d", running_thread->status);
  k_debug("- args: %s", c);
}

thread_t *thread_allocate(void *(*entrypoint)(void *), pagemap_t *pagemap,
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

  thread->priority = 0;

  thread->status = STATUS_READY;

  thread->times_ran = 0;

  if (pagemap != NULL) {
    thread->pagemap = pagemap;
  } else {
    thread->pagemap = kernel_pagemap;
  }

  thread->kernel_thread = pagemap == kernel_pagemap;

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

void thread_start(void *(*entrypoint)(void *), pagemap_t *pagemap,
                  char name[16], uint8_t flags, args_t args) {
  spinlock_acquire(&threads_lock);

  if (flags != FLAGS_NONE && flags != 0) {
    k_todo("Thread Flags");
  }

  thread_t *t = thread_allocate(entrypoint, pagemap, name, args);

  t->status = STATUS_READY;

  threads[thread_count++] = t;

  spinlock_release(&threads_lock);

  reorder_threads_list();
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
      threads[i]->priority = 0;
      threads[i]->allotment = MAX_ALLOTMENT;
    } else {
      if (threads[i]->priority == queue_number) {
        queue_end_index = i;
      } else {
        break;
      }
    }
  }

  if (in_queue_thread_index >= queue_end_index) {
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

  switch_to_pagemap(running_thread->pagemap);

  switch_tss_stack((uintptr_t)running_thread->stack_bounds);

  running_thread->times_ran++;

  running_thread->status = STATUS_RUNNING;

  apic_interrupt_ms(BASE_PREEMPT_QUANTUM_MS * (running_thread->priority + 1));

  uint64_t new_rsp = running_thread->stack_ptr;
  uint64_t *old_rsp;

  if (last_running_thread) {
    last_running_thread->status = STATUS_READY;
    old_rsp = &last_running_thread->stack_ptr;
  } else {
    old_rsp = &new_rsp;
  }

  if (!new_rsp) {
    k_err("Bad stack ptr");
    spinlock_release(&scheduler_lock);
    return;
  }

  spinlock_release(&scheduler_lock);

  task_switch(old_rsp, new_rsp);
}

void setup_scheduler() {
  threads = kmalloc(sizeof(thread_t *) * INITIAL_THREAD_BUFFER);

  apic_interrupt_ms(BASE_PREEMPT_QUANTUM_MS * 4);
}
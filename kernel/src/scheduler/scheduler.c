#include "header/scheduler.h"
#include "../header/core.h"
#include "../memory/header/heap.h"
#include "../memory/header/memmap.h"
#include "../platform/x86_64/header/apic.h"
#include "../platform/x86_64/header/gdt.h"
#include "../platform/x86_64/header/pic.h"
#include "../platform/x86_64/header/pit.h"

#include "../util/header/log.h"
#include "../util/header/print_lowlevel.h"
#include "../util/header/printf.h"

#include <stdbool.h>
#include <stdint.h>

#define THREAD_STACK_SIZE (PAGE_SIZE * 20)

#define S_QUANTUM 0xff

#define BASE_PREEMPT_QUANTUM_MS 10

#define NUM_QUEUES 8

#define INITIAL_THREAD_BUFFER 256

#define MAX_ALLOTMENT 2

SPINLOCK_DEFINE(scheduler_lock);
SPINLOCK_DEFINE(threads_lock);

thread_id_t new_id = 0;

extern pagemap_t *kernel_pagemap;

thread_t **threads;
uint32_t thread_count;
thread_t *running_thread;
thread_t *last_running_thread;

extern void task_switch(uintptr_t *old_sp, uintptr_t new_sp);

uint32_t times_preempted = 0;

uint32_t in_queue_thread_index = 0;

void dump_threads() {
  __asm__ __volatile__("cli");
  spinlock_acquire(&threads_lock);
  for (uint8_t i = 0; i < thread_count; i++) {
    thread_t *thread = threads[i];
    k_debug("[thread %d]:", thread->id);
    k_debug("- priority: %d", thread->priority);
    k_debug("- allotment: %d", thread->allotment);
    k_debug("- times ran: %d", thread->times_ran);
  }
  spinlock_release(&threads_lock);
  __asm__ __volatile__("sti");
}

void reorder_threads_list() {
  //__asm__ __volatile__("cli");
  // spinlock_acquire(&threads_lock);
  for (uint32_t i = 1; i < thread_count; i++) {
    struct thread *key = threads[i];

    int32_t j = i - 1;

    while (j >= 0 && threads[j]->priority > key->priority) {
      threads[j + 1] = threads[j];
      j--;
    }
    threads[j + 1] = key;
  }
  // spinlock_release(&threads_lock);
  //__asm__ __volatile__("sti");
}

void thread_destroy(thread_t *thread) {
  // spinlock_acquire(&threads_lock);

  // TODO: Shrink array if possible once done.
  for (uint32_t i = 0; i < thread_count; i++) {
    if (threads[i]->id == thread->id) {
      threads[i] = NULL;

      for (uint32_t q = i; q < thread_count - 1; q++) {
        threads[q] = threads[q + 1];
      }

      threads[thread_count--] = NULL;

      reorder_threads_list();

      kfree(thread->stack_base);
      kfree(thread);
      // spinlock_release(&threads_lock);
      return;
    }
  }

  k_err("Could not kill thread %d! Could not match IDs with a running thread!",
        thread->id);
  // spinlock_release(&threads_lock);
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

void thread_debug() {
top:
  k_debug("thread %d", running_thread->id);
  // unmask_irq(0);
  // unmask_irq(4);

  //__asm__ __volatile__("int $0xf1");

  // thread_yield();
  // goto top;
}

thread_t *make_inital_kernel_thread() {
  thread_t *thread = kmalloc(sizeof(thread_t));

  memset(thread, 0x00, sizeof(thread_t));

  thread->stack_base = kmalloc(16);

  thread->stack_bounds =
      (void *)(((uintptr_t)thread->stack_base + 16) & ~0xFULL);

  thread->id = new_id++;

  thread->allotment = MAX_ALLOTMENT;

  thread->priority = 0;

  thread->status = STATUS_WAIT;

  thread->times_ran = 0;

  thread->kernel_thread = true;

  thread->stack_ptr = (uintptr_t)thread->stack_bounds;

  return thread;
}

thread_t *thread_create(void (*entrypoint)(void), pagemap_t *pagemap) {
  thread_t *thread = kmalloc(sizeof(thread_t));

  thread_count++;

  if (thread_count > INITIAL_THREAD_BUFFER) {
    k_todo("Make threads array expandable.");
  }

  memset(thread, 0x00, sizeof(thread_t));

  thread->stack_base = kmalloc(THREAD_STACK_SIZE);

  memset(thread->stack_base, 0x00, THREAD_STACK_SIZE);

  thread->stack_bounds =
      (void *)(((uintptr_t)thread->stack_base + THREAD_STACK_SIZE) & ~0xFULL);

  thread->id = new_id++;

  thread->allotment = MAX_ALLOTMENT;

  thread->priority = 0;

  thread->status = STATUS_WAIT;

  thread->times_ran = 0;

  if (pagemap != NULL) {
    thread->pagemap = pagemap;
  } else {
    thread->pagemap = kernel_pagemap;
  }

  thread->kernel_thread = pagemap == kernel_pagemap;

  uintptr_t *stack = (uintptr_t *)thread->stack_bounds;

  *--stack = 0; // padding
  *--stack = (uintptr_t)thread_await_death;
  *--stack = (uintptr_t)entrypoint;
  *--stack = 0;

  *--stack = 0; // rbx
  *--stack = 0; // rbp
  *--stack = 0; // r12
  *--stack = 0; // r13
  *--stack = 0; // r14
  *--stack = 0; // r15

  thread->stack_ptr = (uintptr_t)stack;

  return thread;
}

void preempt() {
  __asm__ __volatile__("cli");
  serial_printf_("preempt\n");

  if (!thread_count) {
    k_err("Asked to preempt with no threads!");
    return;
  }

  // spinlock_acquire(&scheduler_lock);

  serial_printf_("0\n");

  if (running_thread != NULL) {
    last_running_thread = running_thread;
  }

  serial_printf_("1\n");

  uint32_t queue_number = threads[0]->priority;
  uint32_t queue_end_index = 0;

  serial_printf_("2\n");

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

  serial_printf_("a\n");

  if (in_queue_thread_index >= queue_end_index) {
    in_queue_thread_index = 0;
  }

  running_thread = threads[in_queue_thread_index++];

  if (running_thread->allotment <= 0) {
    running_thread->priority++;
    if (running_thread->priority > NUM_QUEUES) {
      running_thread->priority = NUM_QUEUES;
    }

    running_thread->allotment = MAX_ALLOTMENT;
    reorder_threads_list();
  } else {
    running_thread->allotment--;
  }

  switch_to_pagemap(running_thread->pagemap);

  switch_tss_stack((uintptr_t)running_thread->stack_bounds);

  serial_printf_("b\n");

  running_thread->times_ran++;

  k_debug("Runing thread %d (pri: %d, allot: %d)", running_thread->id,
          running_thread->priority, running_thread->allotment);

  // spinlock_release(&scheduler_lock);

  serial_printf_("try switch\n");

  apic_interrupt_ms(BASE_PREEMPT_QUANTUM_MS);

  uint64_t *old_rsp = &last_running_thread->stack_ptr;
  uint64_t new_rsp = running_thread->stack_ptr;

  if (new_rsp < 0x1000000) {
    k_err("Bad stack ptr");
    return;
  }

  __asm__ __volatile__("sti");

  task_switch(old_rsp, new_rsp);
}

void setup_scheduler() {
  threads = kmalloc(sizeof(thread_t *) * INITIAL_THREAD_BUFFER);

  last_running_thread = make_inital_kernel_thread();

  for (uint8_t i = 0; i < 15; i++) {
    thread_t *t = thread_create(thread_debug, kernel_pagemap);
    t->priority = 0;
    threads[i] = t;
  }

  k_debug("thread count: %d", thread_count);

  reorder_threads_list();

  // apic_interrupt_ms(
  //   BASE_PREEMPT_QUANTUM_MS /* * (running_thread->priority + 1)*/);

  apic_interrupt_ms(1000);

  // preempt();

  // dump_threads();
}

void signal_thread(thread_id_t id, signal_t signal) {}

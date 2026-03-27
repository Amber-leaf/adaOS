#include "header/scheduler.h"
#include "../header/core.h"
#include "../memory/header/heap.h"
#include "../memory/header/memmap.h"
#include "../util/header/log.h"
#include "../util/header/panic.h"
#include <stdbool.h>
#include <stdint.h>

#define THREAD_STACK_SIZE (PAGE_SIZE * 20)

#define NUM_QUEUES 4

#define S_QUANTUM_MS 2000

#define PREEMPT_QUANTUM_MS 10

#define QUEUE_MAX_THREADS 48 // 48 for now, kinda low.

thread_id_t new_id = 0;

extern pagemap_t *kernel_pagemap;

struct execution_queue *queues[NUM_QUEUES];

void setup_scheduler() {
  struct execution_queue default_queue;
  default_queue.entries = kmalloc(
      sizeof(thread_t) * QUEUE_MAX_THREADS); // TODO: figure out a way to do
                                             // this that uses less memory.
  default_queue.num_entries = 0;
  default_queue.last_added = 0;
  default_queue.preempt_quantum = PREEMPT_QUANTUM_MS;

  for (uint32_t i = 0; i < NUM_QUEUES; i++) {
    default_queue.priority = i;
    queues[i] = &default_queue;
  }

  // make interrupt.
}

// Logic is mostly similar to this flowchart:
// https://fs.suwiesi.dev/public/MLFQ.png
void schedule() {
  thread_t *thread;
  struct execution_queue *found_in_queue;

  for (uint32_t i = 0; i < NUM_QUEUES; i++) {
    struct execution_queue *queue = queues[i];
    if (queue->num_entries == 0) {
      if (i == NUM_QUEUES) {
        // we have no threads. I guess just sleep?
        k_err("TODO: setup interrupts so this can work.");
        thread = NULL;
      }
      continue;
    } else if (queue->num_entries == 1) {
      thread = queue->entries[0];
      found_in_queue = queue;
      break;
    } else if (queue->num_entries > 1) {
      for (uint32_t q = 0; q < queue->num_entries; q++) {
        thread = queue->entries[q];
        if (!(q + 1 > queue->num_entries)) {
          queue->entries[q] = queue->entries[q + 1];
          queue->entries[q + 1] = thread;
        } else {
          thread = queue->entries[0];
          found_in_queue = queue;
          break;
        }
      }
    }
  }

  if (thread == NULL) {
    panic("Scheduler could not find a thread to schedule");
  }

  if (thread->allotment <= 0) {
    uint32_t priority = found_in_queue->priority - 1;
    
    queues[priority]->entries[++queues[priority]->num_entries] = thread;
    
    for (uint32_t q = 0; q < queues[priority]->num_entries; q++) {

    }
  }
}

void thread_await_death(thread_id_t id) {
  k_debug("Thread %d exiting", id);
  while (true) {
  }
}

thread_t *create_thread(void (*entrypoint)(void), pagemap_t *pagemap) {
  thread_t *thread = kmalloc(sizeof(thread_t));

  memset(thread, 0x00, sizeof(thread_t));

  thread->stack_base = kmalloc(THREAD_STACK_SIZE);

  memset(thread->stack_base, 0x00, THREAD_STACK_SIZE);

  thread->stack_bounds =
      (void *)(((uintptr_t)thread->stack_base + THREAD_STACK_SIZE) & ~0xFULL);

  thread->id = new_id++;

  if (pagemap != NULL) {
    thread->pagemap = pagemap;
  } else {
    thread->pagemap = kernel_pagemap;
  }

  thread->kernel_thread = pagemap != NULL;

  uintptr_t *stack = (uintptr_t *)thread->stack_bounds;

  *--stack = (uintptr_t)thread_await_death;
  *--stack = (uintptr_t)entrypoint;

  thread->stack_ptr = stack;

  return thread;
}

void signal_thread(thread_id_t id, signal_t signal) {}

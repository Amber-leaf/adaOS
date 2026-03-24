#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <stdint.h>
#include "../../platform/x86_64/header/timing.h"
typedef struct proccess {


} proccess_t;

typedef struct thread {
    struct proccess root_proccess;

    struct thread *next_thread;
	struct thread *prev_thread;



} thread_t;


typedef struct execution_queue_entry {
    struct execution_queue_entry* previous;
    proccess_t execution_target;
    struct execution_queue_entry* next;

} execution_queue_entry_t;

struct execution_queue {
    uint8_t priority;
    cpu_time_t preempt_quantum;
    struct execution_queue* higher;
    struct execution_queue* lower;

    void (*schedule_callback)(proccess_t proccess);
};

#endif
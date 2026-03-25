#ifndef CPU_H_
#define CPU_H_

#include <stdint.h>
#include "../../../scheduler/header/scheduler.h"
#include "timing.h"

typedef uint64_t cpu_id_t;

typedef struct cpu {
    cpu_id_t local_id;

    uint32_t lapic_id;
    uintptr_t lapic_base;
    
    proccess_t proccess;

    cpu_timer_t timer;
} cpu_t;

cpu_t *this_cpu(void);
void setup_cpus(void);

#endif
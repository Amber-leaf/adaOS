#ifndef CPU_H_
#define CPU_H_

#include <stdint.h>
#include "../../../scheduler/header/scheduler.h"

typedef uint64_t cpu_id_t;

typedef struct cpu {
    cpu_id_t id;

    uint32_t lapic_id;
    uintptr_t lapic_base;
} cpu_t;

cpu_t *get_cpu(void);
void setup_cpus(void);

#endif
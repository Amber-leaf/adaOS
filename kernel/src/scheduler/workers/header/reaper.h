#ifndef REAPER_H_
#define REAPER_H_

#include "../../header/spinlock.h"
#include "../../header/scheduler.h"

void reaper_t_main(uint32_t *thread_count, thread_t ***threads);

#endif
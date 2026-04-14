#include "../../memory/header/heap.h"
#include "../../util/header/log.h"
#include "../header/scheduler.h"
#include "../header/spinlock.h"
#include <stdbool.h>
#include <stdint.h>

void reaper_t_main(uint32_t *thread_count, thread_t ***threads) {
  while (true) {
    for (uint32_t i = *thread_count; i-- > 0;) {
      thread_t *thread = (*threads)[i];
      if (thread->status == STATUS_ZOMBIE) {
        uint32_t id = thread->id;
        const char *name = thread->name;

        pop_thread(thread);
        kfree(thread->stack_base);
        kfree(thread);

        k_debug("Reaped %d (%s)", id, name);
      } else {
        k_debug("not reaping %s, status is %d", thread->name, thread->status);
      }
    }
    // thread_yield();
  }
}
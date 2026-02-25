#include "../header/core.h"
#include "../util/header/log.h"
#include "header/interupt_defines.h"
#include "header/ist.h"

char *dump_registers(struct cpu_status *context) {
  // todo
  return "TODO";
}

void unimplemented_exception(struct cpu_status *context) {
  k_log("Hit unimplemented ISR 0x%x (%d).", context->vector_number,
        context->vector_number);
}

void exception_handler(struct cpu_status *context) {
  switch (context->vector_number) {
  default:
    unimplemented_exception(context);
    break;
  }
}

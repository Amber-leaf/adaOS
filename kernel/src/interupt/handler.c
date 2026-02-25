#include "../platform/x86_64/header/apic.h"
#include "../util/header/log.h"
#include "../util/header/panic.h"
#include "../util/header/printf.h"

#include "header/interupt_defines.h"
#include "header/ist.h"

void print_cpu_status(struct cpu_status *context) {
  printf("RAX: %016llx  RBX: %016llx\n", context->rax, context->rbx);
  printf("RCX: %016llx  RDX: %016llx\n", context->rcx, context->rdx);
  printf("RSI: %016llx  RDI: %016llx\n", context->rsi, context->rdi);
  printf("RBP: %016llx\n", context->rbp);
  printf("R8:  %016llx  R9:  %016llx\n", context->r8, context->r9);
  printf("R10: %016llx  R11: %016llx\n", context->r10, context->r11);
  printf("R12: %016llx  R13: %016llx\n", context->r12, context->r13);
  printf("R14: %016llx  R15: %016llx\n", context->r14, context->r15);
  printf("Vector: %llu  Error Code: %016llx\n", context->vector_number,
         context->error_code);
  printf("RIP: %016llx  CS:  %016llx\n", context->iret_rip, context->iret_cs);
  printf("RSP: %016llx  SS:  %016llx\n", context->iret_rsp, context->iret_ss);
  printf("RFLAGS: %016llx\n", context->iret_flags);
}

void non_fatal_unimplemented_exception(struct cpu_status *context) {
  k_log("Hit unimplemented ISR 0x%x (%d)", context->vector_number,
        context->vector_number);

  // print_cpu_status(context);
}

void non_fatal_unimplemented_exception_msg(struct cpu_status *context,
                                           char *msg) {
  k_log("Hit unimplemented ISR %s 0x%x (%d)", msg, context->vector_number,
        context->vector_number);

  // print_cpu_status(context);
}

void unimplemented_exception(struct cpu_status *context) {
  k_log("Hit fatal unimplemented ISR 0x%x (%d)", context->vector_number,
        context->vector_number);

  print_cpu_status(context);

  panic();
}

void unimplemented_exception_msg(struct cpu_status *context, char *msg) {
  k_log("Hit fatal unimplemented ISR %s 0x%x (%d)", msg, context->vector_number,
        context->vector_number);

  print_cpu_status(context);

  panic();
}

// todo make these msg variants.
void exception_handler(struct cpu_status *context) {
  send_eio();

  switch (context->vector_number) {
  case 0:
    unimplemented_exception(context);
    break; // #DE Divide By Zero Error
  case 1:
    unimplemented_exception(context);
    break; // #DB Debug
  case 2:
    unimplemented_exception(context);
    break; // #NMI Non-Maskable Interrupt
  case 3:
    unimplemented_exception(context);
    break; // #BP Breakpoint
  case 4:
    non_fatal_unimplemented_exception(context);
    break; // #OF Overflow
  case 5:
    unimplemented_exception(context);
    break; // #BR Bound Range Exceeded
  case 6:
    unimplemented_exception(context);
    break; // #UD Invalid Opcode
  case 7:
    non_fatal_unimplemented_exception(context);
    break; // #NM Device Not Available
  case 8:
    unimplemented_exception(context);
    break; // #DF Double Fault
  case 9:
    non_fatal_unimplemented_exception(context);
    break; // Unused (was x87 Segment Overrun)
  case 10:
    unimplemented_exception(context);
    break; // #TS Invalid TSS (has error code)
  case 11:
    unimplemented_exception(context);
    break; // #NP Segment Not Present (has error code)
  case 12:
    unimplemented_exception(context);
    break; // #SS Stack-Segment Fault (has error code)
  case 13:
    unimplemented_exception(context);
    break; // #GP General Protection (has error code)
  case 14:
    non_fatal_unimplemented_exception(context);
    break; // #PF Page Fault (has error code)
  case 15:
    non_fatal_unimplemented_exception(context);
    break; // Currently Unused
  case 16:
    non_fatal_unimplemented_exception(context);
    break; // #MF x87 FPU Error
  case 17:
    unimplemented_exception(context);
    break; // #AC Alignment Check (error code: always 0)
  case 18:
    unimplemented_exception(context);
    break; // #MC Machine Check
  case 19:
    unimplemented_exception(context);
    break; // #XF SIMD (SSE/AVX) Error

  case 0xf0:
    non_fatal_unimplemented_exception_msg(context,
                                          "apic spurious vector handler");
    break;
  case 0xf1:
    non_fatal_unimplemented_exception_msg(context, "apic timer handler");
    break;
  case 0xf2:
    non_fatal_unimplemented_exception_msg(context, "apic thermal handler");
    break;
  case 0xf3:
    non_fatal_unimplemented_exception_msg(context,
                                          "apic performance counter handler");
    break;
  case 0xf4:
  case 0xf5:
    non_fatal_unimplemented_exception_msg(context, "apic lint handler");
    break;
  case 0xf6:
    unimplemented_exception_msg(context, "apic internal error!");

  // 20-31: Currently Unused
  default:
    non_fatal_unimplemented_exception(context);
    break;
  }
}

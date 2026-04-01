#include <stdint.h>

#include "../header/core.h"
#include "../platform/x86_64/header/apic.h"
#include "../platform/x86_64/header/port.h"
#include "../util/header/log.h"
#include "../util/header/panic.h"
#include "../util/header/printf.h"
#include "header/apic_timer.h"
#include "header/isr.h"
#include "header/pit_handler.h"
#include "header/rs232_handler.h"

extern bool interrupt_as_timer;

void print_cpu_status_interrupt(struct interrupt_cpu_status *context) {
  printf_("Vector: %llu  Error Code: %016llx\n---\n", context->vector_number,
          context->error_code);
  printf_("RAX: %016llx  RBX: %016llx\n", context->rax, context->rbx);
  printf_("RCX: %016llx  RDX: %016llx\n", context->rcx, context->rdx);
  printf_("RSI: %016llx  RDI: %016llx\n", context->rsi, context->rdi);
  printf_("RBP: %016llx\n", context->rbp);
  printf_("R8:  %016llx  R9:  %016llx\n", context->r8, context->r9);
  printf_("R10: %016llx  R11: %016llx\n", context->r10, context->r11);
  printf_("R12: %016llx  R13: %016llx\n", context->r12, context->r13);
  printf_("R14: %016llx  R15: %016llx\n", context->r14, context->r15);
  printf_("RIP: %016llx  CS:  %016llx\n", context->iret_rip, context->iret_cs);
  printf_("RSP: %016llx  SS:  %016llx\n", context->iret_rsp, context->iret_ss);
  printf_("RFLAGS: %016llx\n", context->iret_flags);
}

void unimplemented_fault(char *msg, struct interrupt_cpu_status *context) {
  k_log("Hit fault: '%s (%d, 0x%x)'", msg, context->vector_number,
        context->vector_number);
  print_cpu_status_interrupt(context);
}

void unimplemented_trap(char *msg, struct interrupt_cpu_status *context) {
  k_log("Hit trap: '%s (%d, 0x%x)'", msg, context->vector_number,
        context->vector_number);
}

void unimplemented_abort(char *msg, struct interrupt_cpu_status *context) {
  print_cpu_status_interrupt(context);
  panic(msg);
}

void exception_handler(struct interrupt_cpu_status *context) {
  if (context->vector_number > 0xf0) {
    send_eio();
  } else {
    outb(0x20, 0x20);
  }

  switch (context->vector_number) {
  case 0x0: // #DE Division Error
    unimplemented_fault("Division Error (#DE)", context);
    break;
  case 0x1: // #DB Debug
    // unimplemented_trap("Debug (#DB)", context);
    break;
  case 0x2: // NMI Non-maskable Interrupt
    unimplemented_fault("Non-maskable Interrupt", context);
    break;
  case 0x3: // #BP Breakpoint
    unimplemented_trap("Breakpoint (#BP)", context);
    break;
  case 0x4: // #OF Overflow
    unimplemented_trap("Overflow (#OF)", context);
    break;
  case 0x5: // #BR Bound Range Exceeded
    unimplemented_fault("Bound Range Exceeded (#BR)", context);
    break;
  case 0x6: // #UD Invalid Opcode
    unimplemented_fault("Invalid Opcode (#UD)", context);
    hcf();
    break;
  case 0x7: // #NM Device Not Available
    unimplemented_fault("Device Not Available (#NM)", context);
    break;
  case 0x8: // #DF Double Fault
    unimplemented_abort("Double Fault (#DF)", context);
    break;
  case 0x9: // Coprocessor Segment Overrun (deprecated)
    unimplemented_fault("Coprocessor Segment Overrun", context);
    break;
  case 0xA: // #TS Invalid TSS
    unimplemented_fault("Invalid TSS (#TS)", context);
    break;
  case 0xB: // #NP Segment Not Present
    unimplemented_fault("Segment Not Present (#NP)", context);
    break;
  case 0xC: // #SS Stack-Segment Fault
    unimplemented_fault("Stack-Segment Fault (#SS)", context);
    break;
  case 0xD: // #GP General Protection Fault
    unimplemented_fault("General Protection Fault (#GP)", context);
    print_cpu_status_interrupt(context);
    hcf();
    break;
  case 0xE: // #PF Page Fault
    unimplemented_fault("Page Fault (#PF)", context);
    hcf();
    break;
  case 0xF: // Reserved
    unimplemented_fault("Reserved (0xF)", context);
    break;
  case 0x10: // #MF x86 Floating-Point Exception
    unimplemented_fault("x86 Floating-Point Exception (#MF)", context);
    break;
  case 0x11: // #AC Alignment Check
    unimplemented_fault("Alignment Check (#AC)", context);
    break;
  case 0x12: // #MC Machine Check
    unimplemented_abort("Machine Check (#MC)", context);
    break;
  case 0x13: // #XM/#XF SIMD Floating-Point Exception
    unimplemented_fault("SIMD Floating-Point Exception (#XM/#XF)", context);
    break;
  case 0x14: // #VE Virtualization Exception
    unimplemented_fault("Virtualization Exception (#VE)", context);
    break;
  case 0x15: // #CP Control Protection Exception
    unimplemented_fault("Control Protection Exception (#CP)", context);
    break;
  case 0x16:
  case 0x17:
  case 0x18:
  case 0x19:
  case 0x1A:
  case 0x1B: // Reserved 22-27
    unimplemented_fault("Reserved", context);
    break;
  case 0x1C: // #HV Hypervisor Injection Exception
    unimplemented_fault("Hypervisor Injection Exception (#HV)", context);
    break;
  case 0x1D: // #VC VMM Communication Exception
    unimplemented_fault("VMM Communication Exception (#VC)", context);
    break;
  case 0x1E: // #SX Security Exception
    unimplemented_fault("Security Exception (#SX)", context);
    break;
  case 0x1F: // Reserved
    unimplemented_fault("Reserved", context);
    break;
  case 0x20:
    pit_irq();
    break;
  case 0x24:
    rs232_irq(context);
    break;
  case 0x70:
    unimplemented_fault("Syscall", context);
    break;
  case 0xf0:
    unimplemented_fault("APIC Spurious Vector", context);
    break;
  case 0xf1:
    apic_timer_irq();
    break;
  case 0xf2:
    unimplemented_abort("APIC Thermal Shutdown", context);
    break;
  case 0xf3:
    unimplemented_fault("APIC Performance Counter", context);
    break;
  case 0xf4:
  case 0xf5:
    unimplemented_fault("APIC LINT", context);
    break;
  case 0xf6:
    unimplemented_abort("APIC Fatal Error", context);
    break;
  case 0xff:
    unimplemented_abort("Unhandled NMI", context);
    break;

  default:
    unimplemented_fault("Unknown Exception", context);
    print_cpu_status_interrupt(context);
    break;
  }
}

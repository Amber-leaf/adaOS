#include "../driver/header/rs232.h"
#include "../header/core.h"
#include "../memory/header/memmap.h"
#include "../util/header/log.h"
#include "../util/header/print_lowlevel.h"
#include "../util/header/printf.h"

#include "header/isr.h"
#include <stdbool.h>
#include <stdint.h>

bool is_first_entry = true;
char line_buffer[255] = {0x00};

uint8_t buffer_index;
extern uint64_t free_pages;
extern uint64_t used_pages;

void print_cpu_status_(struct cpu_status *context) {
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

// TODO: this code is a mess.
void rs232_irq(struct cpu_status *context) {
  char byte = serial_readb_unsafe_nonblocking();

  if (byte == '\b') {
    if (buffer_index == 0) {
      return;
    } else {
      serial_writeb_unsafe_nonblocking(byte);
    }
    line_buffer[buffer_index--] = 0x00;
    return;
  } else {
    serial_writeb_unsafe_nonblocking(byte);
  }

  line_buffer[buffer_index++] = byte;

  if (byte == '\r') {
    if (!memcmp(line_buffer, "reg", 3)) {
      print_cpu_status_(context);
    } else if (!memcmp(line_buffer, "clear", 5)) {
      clear();
    } else if (!memcmp(line_buffer, "memmap", 6)) {
      serial_print_mem_map();
    } else if (!memcmp(line_buffer, "free", 4)) {
      uint64_t length = free_pages * PAGE_SIZE;

      uint64_t gib = length / 1073741824;
      uint64_t remainder = length % 1073741824;

      uint64_t decimal = (remainder * 100) / 1073741824;

      printf_("Free pages: %d, used pages: %d. Total free: %d.%02dGib\n",
              free_pages, used_pages, gib, decimal);
    } else if (line_buffer[0] != '\r' && !is_first_entry) {
      serial_write_string("Unkown Command.\n");
    }

    for (uint8_t i = 0; i < 255; i++) {
      line_buffer[i] = 0x00;
    }
    buffer_index = 0;
    serial_write_string("adaOS Kernel Debug Shell > ");
    is_first_entry = false;
  }
}
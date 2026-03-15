#include "../driver/header/rs232.h"
#include "../header/core.h"
#include "../memory/header/memmap.h"
#include "../util/header/print_lowlevel.h"
#include "../util/header/printf.h"

#include "header/isr.h"
#include <stdbool.h>
#include <stdint.h>

#define PROMPT "adaOS Kernel Debug Shell > "

#define ADD_COMMAND(name, len, func)                                           \
  if (!memcmp(line_buffer, name, len)) {                                       \
    func;                                                                      \
    goto newline;                                                              \
  }

bool terminal_running = false;
char line_buffer[255] = {0x00};

uint8_t buffer_index;

extern uint64_t free_pages;
extern uint64_t used_pages;

void print_cpu_status(struct interrupt_cpu_status *context) {
  serial_printf_("RAX: %016llx  RBX: %016llx\n", context->rax, context->rbx);
  serial_printf_("RCX: %016llx  RDX: %016llx\n", context->rcx, context->rdx);
  serial_printf_("RSI: %016llx  RDI: %016llx\n", context->rsi, context->rdi);
  serial_printf_("RBP: %016llx\n", context->rbp);
  serial_printf_("R8:  %016llx  R9:  %016llx\n", context->r8, context->r9);
  serial_printf_("R10: %016llx  R11: %016llx\n", context->r10, context->r11);
  serial_printf_("R12: %016llx  R13: %016llx\n", context->r12, context->r13);
  serial_printf_("R14: %016llx  R15: %016llx\n", context->r14, context->r15);
  serial_printf_("RIP: %016llx  CS:  %016llx\n", context->iret_rip,
                 context->iret_cs);
  serial_printf_("RSP: %016llx  SS:  %016llx\n", context->iret_rsp,
                 context->iret_ss);
  serial_printf_("RFLAGS: %016llx\n", context->iret_flags);
}

void serial_print_free_ram() {
  uint64_t length = free_pages * PAGE_SIZE;
  uint64_t gib = length / 1073741824;
  uint64_t remainder = length % 1073741824;
  uint64_t decimal = (remainder * 100) / 1073741;

  serial_printf_("Free pages: %d, used pages: %d. Total free: %d.%02dGib\n",
                 free_pages, used_pages, gib, decimal);
}

void rs232_irq(struct interrupt_cpu_status *context) {
  char byte = serial_readb_unsafe_nonblocking();

  // If we haven't started the terminal yet, ignore all input.
  if (byte != '\r' && !terminal_running) {
    return;
  }

  // Deal with backspace.
  if (byte == '\b') {
    if (buffer_index == 0) {
      return;
    } else {
      serial_writeb_unsafe_nonblocking(byte);
    }
    // Zero and decrement index.
    line_buffer[buffer_index--] = 0x00;
    return;
  }

  // Echo what we got in.
  serial_writeb_unsafe_nonblocking(byte);

  // Write the new input to the buffer.
  line_buffer[buffer_index++] = byte;

  if (byte == '\r') { // If we returned...
    // Check against all commands.
    // TODO: make it support parameters and not have to be in descending order
    // of size.
    ADD_COMMAND("memmap", 6, serial_print_mem_map());
    ADD_COMMAND("clear", 5, clear());
    ADD_COMMAND("free", 4, serial_print_free_ram());
    ADD_COMMAND("reg", 3, print_cpu_status(context));

    if (terminal_running && line_buffer[0] != '\r') {
      serial_printf_("Unkown Command.\n");
    }

  newline:
    memset(line_buffer, 0x00, buffer_index); // Clear the buffer.
    buffer_index = 0;

    // Write the new prompt.
    serial_write_string(PROMPT);
    terminal_running = true;
  }
}
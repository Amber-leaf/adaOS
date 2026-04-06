#ifndef STATE_H_
#define STATE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FRAMEBUFFER_INITIALIZED 0
#define CPUID_SUPPORTED 1
#define GDT_INITIALIZED 2
#define IDT_INITIALIZED 3
#define PIC_INITIALIZED 4
#define PIT_INITIALIZED 5
#define PMM_INITIALIZED 6
#define VMM_INITIALIZED 7
#define HEAP_INITIALIZED 8
#define APIC_INITIALIZED 9
#define PIT_TIMER_RUNNING 10
#define SERIAL_INTERRUPT_INITIALIZED 11
#define SMP_INITIALIZED 12

struct global_state {
  uint8_t framebuffer_initialized;
  uint8_t cpuid_supported;
  uint8_t gdt_initialized;
  uint8_t idt_initialized;
  uint8_t pic_initialized;
  uint8_t pit_initialized;
  uint8_t pmm_initialized;
  uint8_t vmm_initialized;
  uint8_t heap_initialized;
  uint8_t apic_initialized; // 0xf0 is bootstrap apic, 0xff is full apic
  uint8_t pit_timer_running;
  uint8_t serial_interrupt_initialized;
  uint8_t smp_initialized;
};
struct global_state get_global_state();
void set_state(size_t index, uint8_t value);

#endif
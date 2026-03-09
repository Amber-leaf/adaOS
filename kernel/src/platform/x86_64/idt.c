#include "header/idt.h"
#include "../../util/header/log.h"

#define GDT_OFFSET_KERNEL_CODE 0x8

#define IDT_VECTORS 255

extern void *isr_stub_table[];

__attribute__((aligned(0x10))) static idt_entry_t
    idt[IDT_VECTORS]; // Create an array of IDT entries; aligned for performance

static struct idt_r idtr;

void idt_set_descriptor(uint8_t index, void *isr, uint8_t flags) {
  idt_entry_t *descriptor = &idt[index];

  descriptor->offset_low = (uint64_t)isr & 0xFFFF;
  descriptor->selector = GDT_OFFSET_KERNEL_CODE;
  descriptor->ist = 0;
  descriptor->type_attr = flags;
  descriptor->offset_mid = ((uint64_t)isr >> 16) & 0xFFFF;
  descriptor->offset_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
  descriptor->zero = 0;
}

void setup_idt(void) {
  idtr.base = (uintptr_t)&idt[0];
  idtr.bounds = (uint16_t)sizeof(idt_entry_t) * IDT_VECTORS - 1;
  for (uint8_t vector = 0; vector < 32; vector++) {
    idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
  }

  extern void isr_0x20();
  extern void isr_0x70();
  extern void isr_0xf0();
  extern void isr_0xf1();
  extern void isr_0xf2();
  extern void isr_0xf3();
  extern void isr_0xf4();
  extern void isr_0xf5();
  extern void isr_0xf6();

  k_log("isr0x%x addr: %p", 0x20, isr_0x20);

  idt_set_descriptor(0x20, isr_0x20, 0x8E);
  idt_set_descriptor(0x70, isr_0x70, 0x8E);
  idt_set_descriptor(0xf0, isr_0xf0, 0x8E);
  idt_set_descriptor(0xf1, isr_0xf1, 0x8E);
  idt_set_descriptor(0xf2, isr_0xf2, 0x8E);
  idt_set_descriptor(0xf3, isr_0xf3, 0x8E);
  idt_set_descriptor(0xf4, isr_0xf4, 0x8E);
  idt_set_descriptor(0xf5, isr_0xf5, 0x8E);
  idt_set_descriptor(0xf6, isr_0xf6, 0x8E);

  lidt(&idtr);
}
#include "header/idt.h"

#define GDT_OFFSET_KERNEL_CODE 0x8

#define IDT_VECTORS 255

#define IDT_DEFAULT_FLAGS 0x8E

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
    idt_set_descriptor(vector, isr_stub_table[vector], IDT_DEFAULT_FLAGS);
  }

  extern void isr_0x20();
  extern void isr_0x24();
  extern void isr_0x70();
  extern void isr_0xf0();
  extern void isr_0xf1();
  extern void isr_0xf2();
  extern void isr_0xf3();
  extern void isr_0xf4();
  extern void isr_0xf5();
  extern void isr_0xf6();

  idt_set_descriptor(0x20, isr_0x20, IDT_DEFAULT_FLAGS);
  idt_set_descriptor(0x24, isr_0x24, IDT_DEFAULT_FLAGS);
  idt_set_descriptor(0x70, isr_0x70, IDT_DEFAULT_FLAGS);
  idt_set_descriptor(0xf0, isr_0xf0, IDT_DEFAULT_FLAGS);
  idt_set_descriptor(0xf1, isr_0xf1, IDT_DEFAULT_FLAGS);
  idt_set_descriptor(0xf2, isr_0xf2, IDT_DEFAULT_FLAGS);
  idt_set_descriptor(0xf3, isr_0xf3, IDT_DEFAULT_FLAGS);
  idt_set_descriptor(0xf4, isr_0xf4, IDT_DEFAULT_FLAGS);
  idt_set_descriptor(0xf5, isr_0xf5, IDT_DEFAULT_FLAGS);
  idt_set_descriptor(0xf6, isr_0xf6, IDT_DEFAULT_FLAGS);

  lidt(&idtr);
}
#include "header/idt.h"

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
  idtr.bounds = (uint16_t)sizeof(struct idt_r) * IDT_VECTORS - 1;

  for (uint8_t vector = 0; vector < IDT_VECTORS; vector++) {
    idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
  }

  lidt(&idtr);
}
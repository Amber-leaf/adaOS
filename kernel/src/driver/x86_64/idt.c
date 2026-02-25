#include "header/idt.h"

#define GDT_OFFSET_KERNEL_CODE 0x8
#define IDT_VECTORS 32

extern void *isr_stub_table[];

__attribute__((aligned(0x10))) static struct IDTEntry
    idt[256]; // Create an array of IDT entries; aligned for performance

static struct IDTDesc idtd;

void idt_set_descriptor(uint8_t index, void *isr, uint8_t flags) {
  struct IDTEntry *descriptor = &idt[index];

  descriptor->offset_low = (uint64_t)isr & 0xFFFF;
  descriptor->selector = GDT_OFFSET_KERNEL_CODE;
  descriptor->ist = 0;
  descriptor->type_attr = flags;
  descriptor->offset_mid = ((uint64_t)isr >> 16) & 0xFFFF;
  descriptor->offset_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
  descriptor->zero = 0;
}

void make_idt(void) {
  idtd.base = (uintptr_t)&idt[0];
  idtd.bounds = (uint16_t)sizeof(struct IDTDesc) * IDT_VECTORS - 1;

  for (uint8_t vector = 0; vector < 32; vector++) {
    idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
    // vectors[vector] = true;
  }

  lidt(&idtd);
}
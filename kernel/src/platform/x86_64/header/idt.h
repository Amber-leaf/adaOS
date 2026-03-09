#ifndef IDT_H_
#define IDT_H_

#include <stdint.h>

typedef struct __attribute__((packed)) idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} idt_entry_t;

struct __attribute__((packed)) idt_r {
    uint16_t bounds;
    uintptr_t base;
};

extern void lidt(struct idt_r* idtd);

extern void setup_idt();

#endif
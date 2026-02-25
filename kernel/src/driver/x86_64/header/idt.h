#ifndef IDT_H_
#define IDT_H_


#include <stdint.h>

struct __attribute__((packed)) IDTEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
};

struct __attribute__((packed)) IDTDesc {
    uint16_t bounds;
    uint64_t base;
};

extern void lidt(struct IDTDesc* idtd);

extern void make_idt();

#endif
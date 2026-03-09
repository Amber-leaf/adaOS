#ifndef GDT_H_
#define GDT_H_

#include <stdint.h>

typedef struct __attribute__((packed)) gdt__gdt_entry {
    uint16_t bounds_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  bounds_high : 4;
    uint8_t  flags : 4;
    uint8_t  base_high;
} gtd__gdt_entry_t;

typedef union __attribute__((packed)) gdt_entry {
    struct gdt__gdt_entry gdt_gdt_entry;
    uintptr_t tss_addr;
} gdt_entry_t;

struct __attribute__((packed)) gdt_r {
    uint16_t bounds;
    uintptr_t base;
};

typedef struct __attribute__ ((packed)) gdt__tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1, rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3, io_map_base;
} gdt__tss_entry_t;

extern void lgdt(struct gdt_r* gdtd);
extern void ltr(uint16_t ltr);
extern void reload_segments();

extern void setup_gdt();

#endif
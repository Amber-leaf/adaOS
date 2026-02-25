#ifndef GDT_H_
#define GDT_H_
#include <stdint.h>

struct TSSAddr {
    uint64_t tss_addr;
};

struct __attribute__((packed)) GDTGDTEntry {
    uint16_t bounds_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  bounds_high : 4;
    uint8_t  flags : 4;
    uint8_t  base_high;
};

union __attribute__((packed)) GDTEntry {
    struct GDTGDTEntry gdt_gdt_entry;
    struct TSSAddr tss_addr;
};

struct __attribute__((packed)) GDTDesc {
    uint16_t bounds;
    uint64_t base;
};

struct __attribute__ ((packed)) TSS {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1, rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3, io_map_base;
};

extern void lgdt(struct GDTDesc* gdtr);
extern void ltr(uint16_t ltr);
extern void reload_segments();
extern void make_gdt();

#endif
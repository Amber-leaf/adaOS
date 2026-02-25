#include "header/gdt.h"
#include <stdint.h>

__attribute__((aligned(16))) uint8_t kernel_stack[4096];

__attribute__((aligned(16))) uint8_t df_stack[4096];

__attribute__((aligned(16))) uint8_t nmi_stack[4096];

static union GDTEntry gdt[8];
static struct GDTDesc gdtd;
struct TSS tss;

// make a GDT entry.
void make_gdt_entry(int index, uint8_t access, uint8_t flags) {
  gdt[index].gdt_gdt_entry.bounds_low = 0;
  gdt[index].gdt_gdt_entry.base_low = 0;
  gdt[index].gdt_gdt_entry.base_mid = 0;
  gdt[index].gdt_gdt_entry.access = access;
  gdt[index].gdt_gdt_entry.bounds_high = 0;
  gdt[index].gdt_gdt_entry.flags = flags;
  gdt[index].gdt_gdt_entry.base_high = 0;
}

// take over two entries in the GDT and put the TSS there instead.
void make_tts_gdt_entry(int index) {
  uint64_t tss_base = (uint64_t)&tss;
  gdt[index].gdt_gdt_entry.bounds_low = sizeof(tss) - 1;
  gdt[index].gdt_gdt_entry.base_low = tss_base & 0xffff;
  gdt[index].gdt_gdt_entry.base_mid = (tss_base >> 16) & 0xff;
  gdt[index].gdt_gdt_entry.access = 0x89; // executable, 10001001
  gdt[index].gdt_gdt_entry.bounds_high = ((sizeof(tss) - 1) >> 16) & 0x0F;
  gdt[index].gdt_gdt_entry.flags = 0;
  gdt[index].gdt_gdt_entry.base_high = (tss_base >> 24) & 0xff;
  gdt[index + 1].tss_addr.tss_addr = tss_base >> 32;
}

void make_gdt() {
  make_gdt_entry(0, 0, 0);
  make_gdt_entry(1, 0x9A, 0xA); // kernel, rwx, 64bit
  make_gdt_entry(2, 0x92, 0xC); // kernel, rw, 64bit
  make_gdt_entry(3, 0xFA, 0xC); // user, rwx, 32bit
  make_gdt_entry(4, 0xF2, 0xC); // user, rw, 64bit
  make_gdt_entry(5, 0xFA, 0xA); // user, rwx, 64bit
  make_tts_gdt_entry(6);

  // tell the interrupt handler where our stack is.
  tss.ist[1] = (uint64_t)(df_stack + sizeof(df_stack));
  tss.ist[2] = (uint64_t)(nmi_stack + sizeof(nmi_stack));
  tss.rsp0 = (uint64_t)(kernel_stack + sizeof(kernel_stack));

  gdtd.bounds = sizeof(gdt) - 1;
  gdtd.base = (uint64_t)&gdt;

  // acctualy load the GDT.
  lgdt(&gdtd);

  reload_segments();

  ltr(0x30);
}

#ifndef IOAPIC_H_
#define IOAPIC_H_

#define IO_APIC_ID 0x0
#define IO_APIC_VER 0x1
#define IO_APIC_ARB 0x2
#define IO_APIC_REDIRECTION_OFFSET 0x10

#define LO_REDIRECTION_FOR_N(n) (IO_APIC_REDIRECTION_OFFSET + n * 2)
#define HI_REDIRECTION_FOR_N(n) ((IO_APIC_REDIRECTION_OFFSET + n * 2) + 1)
void setup_ioapic();

#endif
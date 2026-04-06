#include "header/port.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW2_M 0x20
#define ICW2_S 0x28

#define ICW4_8086 0x01

void unmask_irq(uint8_t irq) {
  uint16_t port;
  uint8_t value;

  if (irq < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
    irq -= 8;
  }

  value = inb(port);

  value &= ~(1 << irq);

  outb(port, value);
}

void mask_irq(uint8_t irq) {
  uint16_t port;
  uint8_t value;

  if (irq < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
    irq -= 8;
  }

  value = inb(port);

  value |= (1 << irq);

  outb(port, value);
}

void setup_pic() {
  __asm__ __volatile__("cli");

  outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
  outb(PIC1_DATA, ICW2_M);
  outb(PIC2_DATA, ICW2_S);
  outb(PIC1_DATA, 0x04);
  outb(PIC2_DATA, 0x02);
  outb(PIC1_DATA, ICW4_8086);
  outb(PIC2_DATA, ICW4_8086);
  outb(PIC1_DATA, 0xFF);
  outb(PIC2_DATA, 0xFF);

  __asm__ __volatile__("sti");
}

#include "header/rs232.h"
#include "../platform/x86_64/header/pic.h"
#include "../platform/x86_64/header/port.h"

#include "../util/header/log.h"
#include "../util/header/state.h"

#include <stdbool.h>
#include <stdint.h>

#define WRITE_REG(offset, value) (outb(COM1 + offset, value))
#define READ_REG(offset) inb(COM1 + offset)

#define TEST_BYTE 0x3a

void check_com1() {
  WRITE_REG(SCRATCH, 0xae);

  if (READ_REG(SCRATCH) != 0xae) {
    k_err("COM1 was bad!");
  }
}

bool data_in_fifo() { return READ_REG(LINE_STATUS) & 1; }

bool transmitter_ready() { return READ_REG(LINE_STATUS) & 1 << 5; }

char serial_readb_unsafe_nonblocking() {
  char data;
  data = READ_REG(0);

  // FIXME: Enter key only inputs carrige return, add newline. For now \r just
  // does not work.
  // switch (data) { case 0x0A:
  //   k_debug("Got newline");
  //   break;
  // case 0x0D:
  //   k_debug("Got return");
  //   break;
  // default:
  //   break;
  // }

  return data;
}

char serial_readb_blocking() {
  while (!data_in_fifo()) {
  }
  return serial_readb_unsafe_nonblocking();
}

void serial_writeb_unsafe_nonblocking(char byte) {
  switch (byte) {
  case 0x0A:
  case 0x0D:
    WRITE_REG(0, '\r');
    WRITE_REG(0, '\n');
    break;

  default:
    WRITE_REG(0, byte);
  }
}

void serial_writeb_blocking(char byte) {
  while (!transmitter_ready()) {
  }
  serial_writeb_unsafe_nonblocking(byte);
}

void serial_write_string(char *s) {
  uint8_t i = 0;

  for (;;) {
    switch (s[i]) {
    case 0x00:
      return;

    default:
      serial_writeb_blocking(s[i]);
    }
    i++;
  }
}
void setup_serial(uint32_t baud) {
  check_com1();

  if (MAX_BAUD % baud != 0) {
    k_err("Bad baud rate!");
    set_state(SERIAL_INTERRUPT_INITIALIZED, false);
    return;
  }

  uint8_t lsb = MAX_BAUD / baud;

  WRITE_REG(LINE_CONTROL, 0x00);
  WRITE_REG(INTERRUPT_ENABLE, 0x01); // enable interrupts
  WRITE_REG(LINE_CONTROL, 0x80);     // enable DLAB to change baud rate
  WRITE_REG(DIVISOR_LSB, lsb);       // set divisor for baud rate
  WRITE_REG(DIVISOR_MSB, 0x00);      // high byte of previous
  WRITE_REG(LINE_CONTROL, 0x03);     // set 8N1 mode
  WRITE_REG(FIFO, 0x07);             // set  FIFO with 1 byte threshold
  WRITE_REG(MODEM_CONTROL, 0x0B);    // enable the irqs
  WRITE_REG(MODEM_CONTROL, 0x1E);    // enable loopback for testing
  WRITE_REG(0, TEST_BYTE);

  uint8_t inbyte = READ_REG(0);

  if (inbyte != TEST_BYTE) {
    k_err("Loopback got wrong byte! Got 0x%x, expected 0x%x", inbyte,
          TEST_BYTE);
    set_state(SERIAL_INTERRUPT_INITIALIZED, false);
    return;
  }

  WRITE_REG(MODEM_CONTROL, 0x0F); // disable loopback

  unmask_irq(4);

  set_state(SERIAL_INTERRUPT_INITIALIZED, true);
}

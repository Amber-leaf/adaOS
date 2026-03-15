#ifndef RS232_H_ /* Include guard */
#define RS232_H_

#include <stdint.h>
#include <stdbool.h>

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8
#define COM5 0x5F8
#define COM6 0x4F8
#define COM7 0x5E8
#define COM8 0x4E8

// Serial is weird in that the same address do different things depending on the top bit of line control
#define IO 0
#define DIVISOR_LSB 0

#define DIVISOR_MSB 1
#define INTERRUPT_ENABLE 1

#define INTERRUPT_IDENTIFICATION 2
#define FIFO 2

#define LINE_CONTROL 3
#define MODEM_CONTROL 4
#define LINE_STATUS 5
#define MODEM_STATUS 6
#define SCRATCH 7

#define MAX_BAUD 115200

void setup_serial(uint32_t baud);
char serial_readb_blocking();
void serial_writeb_blocking(char byte);
void serial_write_string(char *s);

void serial_writeb_unsafe_nonblocking(char byte);
char serial_readb_unsafe_nonblocking();

bool data_in_fifo();

#endif
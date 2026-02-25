#ifndef PORT_H_
#define PORT_H_


#include <stdint.h>

void outb(unsigned short port, unsigned char val);
unsigned char inb(unsigned short port);

#endif
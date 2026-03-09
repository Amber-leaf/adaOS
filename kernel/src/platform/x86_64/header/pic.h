#ifndef PIC_H_
#define PIC_H_

#include <stdint.h>
void setup_pic();
void unmask_irq(uint8_t irq);

#endif
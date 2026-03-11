#ifndef AP_H_
#define AP_H_

#include <stdint.h>
void setup_multiproc();
void ap_goto(void *address, uint32_t processor_id);

#endif
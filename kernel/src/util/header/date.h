#ifndef DATE_H_
#define DATE_H_

#include <stdint.h>
#include <stddef.h>

int ms_to_iso8601(int64_t ms_epoch, char *buf, size_t buf_size);

#endif
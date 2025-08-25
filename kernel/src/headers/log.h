#ifndef LOG_H_   /* Include guard */
#define LOG_H_
#include <stdint.h>

void k_debug(char *format, ...);
void k_log(char *format, ...);
void k_ok(char *format, ...);
void k_wrn(char *format, ...);
void k_err(char *format, ...);

#endif
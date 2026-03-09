#include "../header/core.h"
#include "header/log.h"

void __attribute__((noreturn)) panic(char *msg) { // todo
  k_err("Unrecoverable error: %s Halt.", msg);
  hcf();
}
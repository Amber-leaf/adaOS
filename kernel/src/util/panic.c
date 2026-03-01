#include "../header/core.h"
#include "header/log.h"

__attribute__((noreturn));
void panic(char *msg) { // todo
  k_err("Unrecoverable error: %s. Halt.", msg);
  hcf();
}
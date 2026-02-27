#include "../header/core.h"
#include "header/log.h"

void panic(char *msg) { // todo
  k_err("Unrecoverable error: %s. Halt.", msg);
  hcf();
}
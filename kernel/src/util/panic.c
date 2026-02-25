#include "../header/core.h"
#include "header/log.h"

void panic() { // todo
  k_err("Unrecoverable error. Halt.");
  hcf();
}
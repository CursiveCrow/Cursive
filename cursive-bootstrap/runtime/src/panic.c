#include "rt_internal.h"

void cursive_x3a_x3aruntime_x3a_x3apanic(uint32_t code) {
  c0_trace_emit_rule("PanicSym");
  ExitProcess(code);
}

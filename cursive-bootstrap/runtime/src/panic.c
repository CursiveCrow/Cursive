#include "rt_internal.h"

void cursive_x3a_x3aruntime_x3a_x3apanic(uint32_t code) {
  c0_trace_emit_rule("PanicSym");
  if (c0_parallel_in_panic_scope()) {
    c0_parallel_raise_panic(code);
    return;
  }
  ExitProcess(code);
}

void cursive0_panic(uint32_t code) {
  c0_trace_emit_rule("PanicSym");
  cursive_x3a_x3aruntime_x3a_x3apanic(code);
}

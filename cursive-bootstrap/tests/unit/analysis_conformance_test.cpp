#include <cassert>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/types/conformance.h"

namespace {

using cursive0::analysis::C0Conforming;
using cursive0::analysis::ConformanceInput;
using cursive0::analysis::PhaseOrderResult;
using cursive0::analysis::RejectIllFormed;
using cursive0::analysis::WF;

}  // namespace

int main() {
  SPEC_COV("Reject-IllFormed");

  PhaseOrderResult phases_ok;
  phases_ok.phase1_ok = true;
  phases_ok.phase3_ok = true;
  phases_ok.phase4_ok = true;

  ConformanceInput ok_input;
  ok_input.phase_orders = phases_ok;
  ok_input.subset_ok = true;

  assert(WF(phases_ok));
  assert(C0Conforming(ok_input));
  assert(!RejectIllFormed(ok_input));

  PhaseOrderResult phases_missing;
  phases_missing.phase1_ok = true;
  phases_missing.phase3_ok = false;
  phases_missing.phase4_ok = true;

  ConformanceInput phase_fail;
  phase_fail.phase_orders = phases_missing;
  phase_fail.subset_ok = true;

  assert(!WF(phases_missing));
  assert(!C0Conforming(phase_fail));
  assert(RejectIllFormed(phase_fail));

  PhaseOrderResult phases_no_phase4;
  phases_no_phase4.phase1_ok = true;
  phases_no_phase4.phase3_ok = true;
  phases_no_phase4.phase4_ok = false;

  ConformanceInput phase4_fail;
  phase4_fail.phase_orders = phases_no_phase4;
  phase4_fail.subset_ok = true;

  assert(!WF(phases_no_phase4));
  assert(!C0Conforming(phase4_fail));
  assert(RejectIllFormed(phase4_fail));

  PhaseOrderResult phases_no_phase1;
  phases_no_phase1.phase1_ok = false;
  phases_no_phase1.phase3_ok = true;
  phases_no_phase1.phase4_ok = true;

  ConformanceInput phase1_fail;
  phase1_fail.phase_orders = phases_no_phase1;
  phase1_fail.subset_ok = true;

  assert(!WF(phases_no_phase1));
  assert(!C0Conforming(phase1_fail));
  assert(RejectIllFormed(phase1_fail));

  ConformanceInput subset_fail;
  subset_fail.phase_orders = phases_ok;
  subset_fail.subset_ok = false;

  assert(!C0Conforming(subset_fail));
  assert(RejectIllFormed(subset_fail));

  return 0;
}

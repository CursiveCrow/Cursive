// =============================================================================
// ABI Type: Function Types (ABI-Func)
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.2.2 ABI Type Lowering
//   - ABI-Func rule: ABITy(TypeFunc(params, R)) => <PtrSize, PtrAlign>
//   - Function types are represented as function pointers.
//
// =============================================================================

#include "05_codegen/abi/abi.h"
#include "05_codegen/layout/layout.h"
#include "00_core/spec_trace.h"

namespace cursive::codegen {

std::optional<ABIType> ABITyFunc(const analysis::TypeFunc& /*func*/) {
  SPEC_RULE("ABI-Func");
  return ABIType{kPtrSize, kPtrAlign};
}

}  // namespace cursive::codegen

// =============================================================================
// ABI Type: Raw Pointers (ABI-RawPtr)
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.2.2 ABI Type Lowering
//   - ABI-RawPtr rule: ABITy(TypeRawPtr(q, U)) => <PtrSize, PtrAlign>
//
// =============================================================================

#include "05_codegen/abi/abi.h"
#include "05_codegen/layout/layout.h"
#include "00_core/spec_trace.h"

namespace cursive::codegen {

std::optional<ABIType> ABITyRawPtr(const analysis::TypeRawPtr& /*rawptr*/) {
  SPEC_RULE("ABI-RawPtr");
  return ABIType{kPtrSize, kPtrAlign};
}

}  // namespace cursive::codegen

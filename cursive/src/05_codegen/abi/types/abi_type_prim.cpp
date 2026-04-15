// =============================================================================
// ABI Type: Primitives (ABI-Prim)
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.2.2 ABI Type Lowering
//   - ABI-Prim rule: ABITy(TypePrim(name)) => <PrimSize(name), PrimAlign(name)>
//
// =============================================================================

#include "05_codegen/abi/abi.h"
#include "05_codegen/layout/layout.h"
#include "00_core/spec_trace.h"

namespace cursive::codegen {

std::optional<ABIType> ABITyPrim(const analysis::TypePrim& prim) {
  SPEC_RULE("ABI-Prim");
  const auto size = PrimSize(prim.name);
  const auto align = PrimAlign(prim.name);
  if (!size.has_value() || !align.has_value()) {
    return std::nullopt;
  }
  return ABIType{*size, *align};
}

}  // namespace cursive::codegen

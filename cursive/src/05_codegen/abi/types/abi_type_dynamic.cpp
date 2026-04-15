// =============================================================================
// ABI Type: Dynamic Objects (ABI-Dynamic)
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.2.2 ABI Type Lowering
//   - ABI-Dynamic rule: ABITy(TypeDynamic(Cl)) => DynLayout(Cl)
//   - Dynamic objects are fat pointers: (data, vtable).
//
// =============================================================================

#include "05_codegen/abi/abi.h"
#include "05_codegen/layout/layout.h"
#include "00_core/spec_trace.h"

namespace cursive::codegen {

std::optional<ABIType> ABITyDynamic(const analysis::TypeDynamic& /*dyn*/) {
  SPEC_RULE("ABI-Dynamic");
  const auto dyn_layout = DynLayoutOf();
  return dyn_layout.layout;
}

}  // namespace cursive::codegen

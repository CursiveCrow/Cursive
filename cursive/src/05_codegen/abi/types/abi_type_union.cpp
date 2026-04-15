// =============================================================================
// ABI Type: Unions (ABI-Union)
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.2.2 ABI Type Lowering
//   - ABI-Union rule: ABITy(TypeUnion([T1,...,Tn])) => UnionLayout.layout
//
// =============================================================================

#include "05_codegen/abi/abi.h"
#include "05_codegen/layout/layout.h"
#include "00_core/spec_trace.h"

namespace cursive::codegen {

std::optional<ABIType> ABITyUnion(const analysis::ScopeContext& ctx,
                                  const analysis::TypeUnion& uni) {
  SPEC_RULE("ABI-Union");
  const auto layout = UnionLayoutOf(ctx, uni);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  return layout->layout;
}

}  // namespace cursive::codegen

#include "cursive0/codegen/layout/layout.h"

#include "cursive0/core/assert_spec.h"

namespace cursive0::codegen {

DynLayout DynLayoutOf() {
  SPEC_RULE("Layout-DynamicClass");
  DynLayout out;
  out.layout = Layout{2 * kPtrSize, kPtrAlign};
  out.fields.push_back(
      cursive0::analysis::MakeTypeRawPtr(cursive0::analysis::RawPtrQual::Imm,
                                     cursive0::analysis::MakeTypePrim("()")));
  out.fields.push_back(
      cursive0::analysis::MakeTypeRawPtr(cursive0::analysis::RawPtrQual::Imm,
                                     cursive0::analysis::MakeTypePath({"VTable"})));
  return out;
}

}  // namespace cursive0::codegen

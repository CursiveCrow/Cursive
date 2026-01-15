#include "cursive0/codegen/layout.h"

#include "cursive0/core/assert_spec.h"

namespace cursive0::codegen {

DynLayout DynLayoutOf() {
  SPEC_RULE("Layout-DynamicClass");
  DynLayout out;
  out.layout = Layout{2 * kPtrSize, kPtrAlign};
  out.fields.push_back(
      cursive0::sema::MakeTypeRawPtr(cursive0::sema::RawPtrQual::Imm,
                                     cursive0::sema::MakeTypePrim("()")));
  out.fields.push_back(
      cursive0::sema::MakeTypeRawPtr(cursive0::sema::RawPtrQual::Imm,
                                     cursive0::sema::MakeTypePath({"VTable"})));
  return out;
}

}  // namespace cursive0::codegen

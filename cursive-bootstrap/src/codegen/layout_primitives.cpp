#include "cursive0/codegen/layout.h"

#include "cursive0/core/assert_spec.h"

namespace cursive0::codegen {

std::optional<std::uint64_t> PrimSize(std::string_view name) {
  SPEC_RULE("Size-Prim");
  if (name == "i8" || name == "u8") return 1;
  if (name == "i16" || name == "u16") return 2;
  if (name == "i32" || name == "u32") return 4;
  if (name == "i64" || name == "u64") return 8;
  if (name == "i128" || name == "u128") return 16;
  if (name == "f16") return 2;
  if (name == "f32") return 4;
  if (name == "f64") return 8;
  if (name == "bool") return 1;
  if (name == "char") return 4;
  if (name == "usize" || name == "isize") return kPtrSize;
  if (name == "()" || name == "!") return 0;
  return std::nullopt;
}

std::optional<std::uint64_t> PrimAlign(std::string_view name) {
  SPEC_RULE("Align-Prim");
  if (name == "i8" || name == "u8") return 1;
  if (name == "i16" || name == "u16") return 2;
  if (name == "i32" || name == "u32") return 4;
  if (name == "i64" || name == "u64") return 8;
  if (name == "i128" || name == "u128") return 16;
  if (name == "f16") return 2;
  if (name == "f32") return 4;
  if (name == "f64") return 8;
  if (name == "bool") return 1;
  if (name == "char") return 4;
  if (name == "usize" || name == "isize") return kPtrAlign;
  if (name == "()" || name == "!") return 1;
  return std::nullopt;
}

}  // namespace cursive0::codegen

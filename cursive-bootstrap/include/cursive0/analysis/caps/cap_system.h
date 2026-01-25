#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

struct SystemMethodSig {
  Permission recv_perm;
  std::vector<syntax::Param> params;
  TypeRef ret;
};

std::optional<SystemMethodSig> LookupSystemMethodSig(std::string_view name);

// C0X Extension: Context method signatures for execution domains (§18.2)
struct ContextMethodSig {
  Permission recv_perm;
  std::vector<syntax::Param> params;
  TypeRef ret;
};

std::optional<ContextMethodSig> LookupContextMethodSig(std::string_view name);

syntax::RecordDecl BuildContextRecordDecl();
syntax::RecordDecl BuildSystemRecordDecl();

}  // namespace cursive0::analysis

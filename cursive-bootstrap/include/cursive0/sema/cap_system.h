#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

struct SystemMethodSig {
  Permission recv_perm;
  std::vector<syntax::Param> params;
  TypeRef ret;
};

std::optional<SystemMethodSig> LookupSystemMethodSig(std::string_view name);

syntax::RecordDecl BuildContextRecordDecl();
syntax::RecordDecl BuildSystemRecordDecl();

}  // namespace cursive0::sema

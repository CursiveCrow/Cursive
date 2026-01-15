#pragma once

#include <optional>
#include <string_view>

#include "cursive0/sema/context.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

struct ValuePathTypeResult {
  bool ok = true;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

ValuePathTypeResult ValuePathType(const ScopeContext& ctx,
                                  const syntax::ModulePath& path,
                                  std::string_view name);

ValuePathTypeResult ProcType(const ScopeContext& ctx,
                             const syntax::ProcedureDecl& decl);

}  // namespace cursive0::sema

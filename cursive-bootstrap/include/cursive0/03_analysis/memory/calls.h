#pragma once

#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/place_types.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/03_analysis/generics/monomorphize.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

struct ExprTypeResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

using ExprTypeFn = std::function<ExprTypeResult(const syntax::ExprPtr&)>;

struct CallTypeResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
  bool record_callee = false;
};

CallTypeResult TypeCall(const ScopeContext& ctx,
                        const syntax::ExprPtr& callee,
                        const std::vector<syntax::Arg>& args,
                        const ExprTypeFn& type_expr,
                        const PlaceTypeFn* type_place = nullptr);

// Type check a generic procedure call with type substitution (§13.1.2 T-Generic-Call)
CallTypeResult TypeCallWithSubst(const ScopeContext& ctx,
                                 const syntax::ExprPtr& callee,
                                 const std::vector<syntax::Arg>& args,
                                 const TypeSubst& subst,
                                 const ExprTypeFn& type_expr,
                                 const PlaceTypeFn* type_place = nullptr);

bool IsRecordCallee(const ScopeContext& ctx,
                    const syntax::ExprPtr& callee,
                    const std::vector<syntax::Arg>& args);

}  // namespace cursive0::analysis

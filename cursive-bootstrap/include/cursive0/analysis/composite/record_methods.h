#pragma once

#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/analysis/memory/calls.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/place_types.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::analysis {

struct LowerTypeResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

using LowerTypeFn =
    std::function<LowerTypeResult(const std::shared_ptr<syntax::Type>&)>;

struct RecvTypeResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

RecvTypeResult RecvTypeForReceiver(const ScopeContext& ctx,
                                   const TypeRef& base,
                                   const syntax::Receiver& receiver,
                                   const LowerTypeFn& lower_type);

std::optional<ParamMode> RecvModeOf(const syntax::Receiver& receiver);

struct RecvBaseTypeResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  Permission perm = Permission::Const;
  TypeRef base;
};

RecvBaseTypeResult RecvBaseType(const syntax::ExprPtr& base,
                                const std::optional<ParamMode>& mode,
                                const PlaceTypeFn& type_place,
                                const ExprTypeFn& type_expr);

struct RecvArgOkResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
};

RecvArgOkResult RecvArgOk(const syntax::ExprPtr& base,
                          const std::optional<ParamMode>& mode,
                          const ExprTypeFn& type_expr);

struct ArgsOkResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
};

ArgsOkResult ArgsOk(const ScopeContext& ctx,
                    const std::vector<syntax::Param>& params,
                    const std::vector<syntax::Arg>& args,
                    const ExprTypeFn& type_expr,
                    const PlaceTypeFn* type_place,
                    const LowerTypeFn& lower_type);

struct StaticMethodLookup {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  const syntax::MethodDecl* record_method = nullptr;
  const syntax::ClassMethodDecl* class_method = nullptr;
  syntax::ClassPath owner_class;
};

StaticMethodLookup LookupMethodStatic(const ScopeContext& ctx,
                                      const TypeRef& base,
                                      std::string_view name);

}  // namespace cursive0::analysis

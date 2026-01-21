#pragma once

#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/eval/control.h"
#include "cursive0/eval/state.h"
#include "cursive0/eval/value.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::eval {

enum class DropStatus {
  Ok,
  Panic,
};

enum class CleanupStatus {
  Ok,
  Panic,
  Abort,
};

DropStatus DropActionOut(const SemanticsContext& ctx,
                         const Binding& binding,
                         Sigma& sigma);
DropStatus DropStaticActionOut(const SemanticsContext& ctx,
                               const analysis::PathKey& path,
                               std::string_view name,
                               Sigma& sigma);
DropStatus DropValueOut(const SemanticsContext& ctx,
                        const analysis::TypeRef& type,
                        const Value& value,
                        const std::set<std::string>& fields,
                        Sigma& sigma);
DropStatus DropSubvalue(const SemanticsContext& ctx,
                        const syntax::Expr& place,
                        const analysis::TypeRef& type,
                        const Value& value,
                        Sigma& sigma);

CleanupStatus Cleanup(const SemanticsContext& ctx,
                      const std::vector<CleanupItem>& items,
                      Sigma& sigma);
CleanupStatus CleanupScope(const SemanticsContext& ctx,
                           const ScopeEntry& scope,
                           Sigma& sigma);

}  // namespace cursive0::eval

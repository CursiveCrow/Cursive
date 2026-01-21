#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/sema/context.h"
#include "cursive0/sema/scopes_lookup.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

struct ResolveExprResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  syntax::ExprPtr expr;
};

struct ResolveArgsResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::vector<syntax::Arg> args;
};

struct ResolveFieldInitsResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  std::vector<syntax::FieldInit> fields;
};

struct ResolveTypePathResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  syntax::TypePath path;
};

struct ResolveRecordPathResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  syntax::TypePath path;
};

struct ResolveEnumPathResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  syntax::TypePath path;
};

struct ResolveQualifiedFormResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  syntax::ExprPtr expr;
};

using ResolveExprFn =
    ResolveExprResult (*)(const ScopeContext& ctx,
                          const NameMapTable& name_maps,
                          const ModuleNames& module_names,
                          const syntax::ExprPtr& expr);

using ResolveTypePathFn =
    ResolveTypePathResult (*)(const ScopeContext& ctx,
                              const NameMapTable& name_maps,
                              const ModuleNames& module_names,
                              const syntax::TypePath& path);

struct ResolveQualContext {
  const ScopeContext* ctx = nullptr;
  const NameMapTable* name_maps = nullptr;
  const ModuleNames* module_names = nullptr;
  ResolveExprFn resolve_expr = nullptr;
  ResolveTypePathFn resolve_type_path = nullptr;
  CanAccessFn can_access = nullptr;
};

ResolveArgsResult ResolveArgs(const ResolveQualContext& ctx,
                              const std::vector<syntax::Arg>& args);

ResolveFieldInitsResult ResolveFieldInits(
    const ResolveQualContext& ctx,
    const std::vector<syntax::FieldInit>& fields);

ResolveRecordPathResult ResolveRecordPath(const ResolveQualContext& ctx,
                                          const syntax::ModulePath& path,
                                          std::string_view name);

ResolveEnumPathResult ResolveEnumUnit(const ResolveQualContext& ctx,
                                      const syntax::ModulePath& path,
                                      std::string_view name);

ResolveEnumPathResult ResolveEnumTuple(const ResolveQualContext& ctx,
                                       const syntax::ModulePath& path,
                                       std::string_view name);

ResolveEnumPathResult ResolveEnumRecord(const ResolveQualContext& ctx,
                                        const syntax::ModulePath& path,
                                        std::string_view name);

ResolveQualifiedFormResult ResolveQualifiedForm(const ResolveQualContext& ctx,
                                                const syntax::Expr& expr);

}  // namespace cursive0::sema

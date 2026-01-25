#include "cursive0/analysis/memory/init_planner.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/core/diagnostics.h"
#include "cursive0/core/symbols.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/resolve/scopes_lookup.h"

namespace cursive0::analysis {

namespace {

using ModuleSet = std::set<std::size_t>;
using UsingMap = std::map<IdKey, syntax::ModulePath>;

static inline void SpecDefsInitPlanner() {
  SPEC_DEF("AliasExpand", "5.12");
  SPEC_DEF("ModulePrefix", "5.12");
  SPEC_DEF("Reachable", "5.12");
  SPEC_DEF("TypeRefsTy", "5.12");
  SPEC_DEF("TypeRefsRef", "5.12");
  SPEC_DEF("TypeRefsExpr", "5.12");
  SPEC_DEF("TypeRefsPat", "5.12");
  SPEC_DEF("ValueRefs", "5.12");
  SPEC_DEF("ValueRefsArgs", "5.12");
  SPEC_DEF("ValueRefsFields", "5.12");
  SPEC_DEF("TypeDeps", "5.12");
  SPEC_DEF("ValueDepsEager", "5.12");
  SPEC_DEF("ValueDepsLazy", "5.12");
  SPEC_DEF("WF-Acyclic-Eager", "5.12");
  SPEC_DEF("Topo", "7.1");
}

std::vector<std::string> SplitModulePath(std::string_view path) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (start <= path.size()) {
    const std::size_t pos = path.find("::", start);
    if (pos == std::string_view::npos) {
      parts.emplace_back(path.substr(start));
      break;
    }
    parts.emplace_back(path.substr(start, pos - start));
    start = pos + 2;
  }
  return parts;
}

std::string ModuleKey(const syntax::ModulePath& path) {
  return core::StringOfPath(path);
}

bool PathPrefix(const syntax::ModulePath& path,
                const syntax::ModulePath& pref) {
  if (pref.size() > path.size()) {
    return false;
  }
  for (std::size_t i = 0; i < pref.size(); ++i) {
    if (!IdEq(path[i], pref[i])) {
      return false;
    }
  }
  return true;
}

syntax::ModulePath AliasExpand(const syntax::ModulePath& path,
                               const AliasMap& alias) {
  SpecDefsInitPlanner();
  if (path.empty()) {
    return path;
  }
  const auto key = IdKeyOf(path.front());
  const auto it = alias.find(key);
  if (it == alias.end()) {
    SPEC_RULE("AliasExpand-None");
    return path;
  }
  SPEC_RULE("AliasExpand-Yes");
  syntax::ModulePath out = it->second;
  out.insert(out.end(), path.begin() + 1, path.end());
  return out;
}

struct InitEnv {
  syntax::ModulePath self;
  std::size_t self_index = 0;
  const std::vector<syntax::ModulePath>* modules = nullptr;
  const std::map<std::string, std::size_t>* module_index = nullptr;
  AliasMap alias;
  UsingMap using_value;
  UsingMap using_type;
};

std::optional<std::size_t> ModuleIndex(const InitEnv& env,
                                       const syntax::ModulePath& path) {
  if (!env.module_index) {
    return std::nullopt;
  }
  const auto key = ModuleKey(path);
  const auto it = env.module_index->find(key);
  if (it == env.module_index->end()) {
    return std::nullopt;
  }
  return it->second;
}

void Merge(ModuleSet& dst, const ModuleSet& src) {
  dst.insert(src.begin(), src.end());
}

UsingMap BuildUsingValueMap(const NameMap& names) {
  UsingMap out;
  for (const auto& [key, ent] : names) {
    if (ent.source != EntitySource::Using) {
      continue;
    }
    if (!ent.origin_opt.has_value()) {
      continue;
    }
    if (ent.kind == EntityKind::Value) {
      out.emplace(key, *ent.origin_opt);
    }
  }
  return out;
}

UsingMap BuildUsingTypeMap(const NameMap& names) {
  UsingMap out;
  for (const auto& [key, ent] : names) {
    if (ent.source != EntitySource::Using) {
      continue;
    }
    if (!ent.origin_opt.has_value()) {
      continue;
    }
    if (ent.kind == EntityKind::Type || ent.kind == EntityKind::Class) {
      out.emplace(key, *ent.origin_opt);
    }
  }
  return out;
}

std::optional<syntax::ModulePath> ModulePrefix(const syntax::ModulePath& path,
                                               const InitEnv& env) {
  SpecDefsInitPlanner();
  if (!env.modules) {
    SPEC_RULE("ModulePrefix-None");
    return std::nullopt;
  }
  const auto expanded = AliasExpand(path, env.alias);
  std::optional<syntax::ModulePath> best;
  std::size_t best_len = 0;
  for (const auto& mod : *env.modules) {
    if (!PathPrefix(expanded, mod)) {
      continue;
    }
    if (!best.has_value() || mod.size() > best_len) {
      best = mod;
      best_len = mod.size();
    }
  }
  if (!best.has_value()) {
    SPEC_RULE("ModulePrefix-None");
    return std::nullopt;
  }
  SPEC_RULE("ModulePrefix-Direct");
  return best;
}

ModuleSet SingleModule(const InitEnv& env,
                       const std::optional<syntax::ModulePath>& path_opt) {
  ModuleSet out;
  if (!path_opt.has_value()) {
    return out;
  }
  const auto index = ModuleIndex(env, *path_opt);
  if (index.has_value()) {
    out.insert(*index);
  }
  return out;
}

syntax::ExprPtr MakeExpr(const core::Span& span, const syntax::ExprNode& node) {
  auto expr = std::make_shared<syntax::Expr>();
  expr->span = span;
  expr->node = node;
  return expr;
}

syntax::ExprPtr WrapBlockExpr(const std::shared_ptr<syntax::Block>& block) {
  if (!block) {
    return nullptr;
  }
  syntax::BlockExpr node;
  node.block = block;
  return MakeExpr(block->span, node);
}

std::vector<syntax::ExprPtr> ArgsExprs(
    const std::vector<syntax::Arg>& args) {
  std::vector<syntax::ExprPtr> out;
  out.reserve(args.size());
  for (const auto& arg : args) {
    out.push_back(arg.value);
  }
  return out;
}

std::vector<syntax::ExprPtr> FieldExprs(
    const std::vector<syntax::FieldInit>& fields) {
  std::vector<syntax::ExprPtr> out;
  out.reserve(fields.size());
  for (const auto& field : fields) {
    out.push_back(field.value);
  }
  return out;
}

std::vector<syntax::ExprPtr> OptExprs(const syntax::ExprPtr& lhs,
                                      const syntax::ExprPtr& rhs) {
  std::vector<syntax::ExprPtr> out;
  if (lhs) {
    out.push_back(lhs);
  }
  if (rhs) {
    out.push_back(rhs);
  }
  return out;
}

std::vector<syntax::ExprPtr> ChildrenLtr(const syntax::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> std::vector<syntax::ExprPtr> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::LiteralExpr> ||
                      std::is_same_v<T, syntax::PtrNullExpr> ||
                      std::is_same_v<T, syntax::IdentifierExpr> ||
                      std::is_same_v<T, syntax::QualifiedNameExpr> ||
                      std::is_same_v<T, syntax::PathExpr> ||
                      std::is_same_v<T, syntax::ErrorExpr>) {
          return {};
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          return node.elements;
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          return node.elements;
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          return FieldExprs(node.fields);
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          if (!node.payload_opt.has_value()) {
            return {};
          }
          return std::visit(
              [&](const auto& payload) -> std::vector<syntax::ExprPtr> {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::EnumPayloadParen>) {
                  return payload.elements;
                } else {
                  return FieldExprs(payload.fields);
                }
              },
              *node.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return {node.base};
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return {node.base};
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return {node.base, node.index};
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          auto out = ArgsExprs(node.args);
          out.insert(out.begin(), node.callee);
          return out;
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          auto out = ArgsExprs(node.args);
          out.insert(out.begin(), node.receiver);
          return out;
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          return {node.lhs, node.rhs};
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          return OptExprs(node.lhs, node.rhs);
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          return {node.cond};
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          return {WrapBlockExpr(node.body)};
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          return {node.cond, WrapBlockExpr(node.body)};
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          return {node.iter, WrapBlockExpr(node.body)};
        } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
          return {};
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
          return {};
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          return {};
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          return {};
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          if (std::holds_alternative<syntax::ParenArgs>(node.args)) {
            return ArgsExprs(std::get<syntax::ParenArgs>(node.args).args);
          }
          return FieldExprs(std::get<syntax::BraceArgs>(node.args).fields);
        } else {
          return {};
        }
      },
      expr.node);
}

ModuleSet TypeRefsTy(const syntax::Type& type, const InitEnv& env);
ModuleSet TypeRefsExpr(const syntax::Expr& expr, const InitEnv& env);
ModuleSet TypeRefsPat(const syntax::Pattern& pat, const InitEnv& env);

ModuleSet TypeRefsTypePath(const syntax::TypePath& path, const InitEnv& env) {
  SpecDefsInitPlanner();
  ModuleSet out;
  if (path.empty()) {
    SPEC_RULE("TypeRef-Path-Local");
    return out;
  }

  if (path.size() == 1) {
    const auto key = IdKeyOf(path[0]);
    const auto it = env.using_type.find(key);
    if (it != env.using_type.end() && !PathEq(it->second, env.self)) {
      SPEC_RULE("TypeRef-Using");
      return SingleModule(env, it->second);
    }
    SPEC_RULE("TypeRef-Path-Local");
    return out;
  }

  const auto mp = ModulePrefix(path, env);
  if (mp.has_value() && !PathEq(*mp, env.self)) {
    SPEC_RULE("TypeRef-Path");
    return SingleModule(env, mp);
  }
  SPEC_RULE("TypeRef-Path-Local");
  return out;
}

ModuleSet TypeRefsTy(const syntax::Type& type, const InitEnv& env) {
  SpecDefsInitPlanner();
  return std::visit(
      [&](const auto& node) -> ModuleSet {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypePathType>) {
          return TypeRefsTypePath(node.path, env);
        } else if constexpr (std::is_same_v<T, syntax::TypeDynamic>) {
          SPEC_RULE("TypeRef-Dynamic");
          return TypeRefsTypePath(node.path, env);
        } else if constexpr (std::is_same_v<T, syntax::TypeModalState>) {
          SPEC_RULE("TypeRef-ModalState");
          ModuleSet out = TypeRefsTypePath(node.path, env);
          for (const auto& arg : node.generic_args) {
            if (!arg) {
              continue;
            }
            Merge(out, TypeRefsTy(*arg, env));
          }
          return out;
        } else if constexpr (std::is_same_v<T, syntax::TypePermType>) {
          SPEC_RULE("TypeRef-Perm");
          if (!node.base) {
            return {};
          }
          return TypeRefsTy(*node.base, env);
        } else if constexpr (std::is_same_v<T, syntax::TypeRefine>) {
          ModuleSet out;
          if (node.base) {
            Merge(out, TypeRefsTy(*node.base, env));
          }
          if (node.predicate) {
            Merge(out, TypeRefsExpr(*node.predicate, env));
          }
          return out;
        } else if constexpr (std::is_same_v<T, syntax::TypePrim>) {
          SPEC_RULE("TypeRef-Prim");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::TypeTuple>) {
          SPEC_RULE("TypeRef-Tuple");
          ModuleSet out;
          for (const auto& elem : node.elements) {
            if (!elem) {
              continue;
            }
            Merge(out, TypeRefsTy(*elem, env));
          }
          return out;
        } else if constexpr (std::is_same_v<T, syntax::TypeArray>) {
          SPEC_RULE("TypeRef-Array");
          ModuleSet out;
          if (node.element) {
            Merge(out, TypeRefsTy(*node.element, env));
          }
          if (node.length) {
            Merge(out, TypeRefsExpr(*node.length, env));
          }
          return out;
        } else if constexpr (std::is_same_v<T, syntax::TypeSlice>) {
          SPEC_RULE("TypeRef-Slice");
          if (!node.element) {
            return {};
          }
          return TypeRefsTy(*node.element, env);
        } else if constexpr (std::is_same_v<T, syntax::TypeUnion>) {
          SPEC_RULE("TypeRef-Union");
          ModuleSet out;
          for (const auto& elem : node.types) {
            if (!elem) {
              continue;
            }
            Merge(out, TypeRefsTy(*elem, env));
          }
          return out;
        } else if constexpr (std::is_same_v<T, syntax::TypeFunc>) {
          SPEC_RULE("TypeRef-Func");
          ModuleSet out;
          for (const auto& param : node.params) {
            if (!param.type) {
              continue;
            }
            Merge(out, TypeRefsTy(*param.type, env));
          }
          if (node.ret) {
            Merge(out, TypeRefsTy(*node.ret, env));
          }
          return out;
        } else if constexpr (std::is_same_v<T, syntax::TypeString>) {
          SPEC_RULE("TypeRef-String");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::TypeBytes>) {
          SPEC_RULE("TypeRef-Bytes");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::TypePtr>) {
          SPEC_RULE("TypeRef-Ptr");
          if (!node.element) {
            return {};
          }
          return TypeRefsTy(*node.element, env);
        } else if constexpr (std::is_same_v<T, syntax::TypeRawPtr>) {
          SPEC_RULE("TypeRef-RawPtr");
          if (!node.element) {
            return {};
          }
          return TypeRefsTy(*node.element, env);
        } else {
          SPEC_RULE("TypeRef-Range");
          return {};
        }
      },
      type.node);
}

ModuleSet TypeRefsRef(const std::variant<syntax::TypePath, syntax::ModalStateRef>& ref,
                      const InitEnv& env) {
  SpecDefsInitPlanner();
  return std::visit(
      [&](const auto& node) -> ModuleSet {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypePath>) {
          SPEC_RULE("TypeRef-Ref-Path");
          return TypeRefsTypePath(node, env);
        } else {
          SPEC_RULE("TypeRef-Ref-ModalState");
          syntax::TypeModalState state;
          state.path = node.path;
          state.generic_args = {};
          state.state = node.state;
          syntax::Type fake;
          fake.node = state;
          return TypeRefsTy(fake, env);
        }
      },
      ref);
}

ModuleSet TypeRefsExprs(const std::vector<syntax::FieldInit>& fields,
                        const InitEnv& env) {
  SpecDefsInitPlanner();
  if (fields.empty()) {
    SPEC_RULE("TypeRefsExprs-Empty");
    return {};
  }
  ModuleSet out;
  for (const auto& field : fields) {
    SPEC_RULE("TypeRefsExprs-Cons");
    if (field.value) {
      Merge(out, TypeRefsExpr(*field.value, env));
    }
  }
  return out;
}

ModuleSet TypeRefsEnumPayload(const std::optional<syntax::EnumPayload>& payload,
                              const InitEnv& env) {
  SpecDefsInitPlanner();
  if (!payload.has_value()) {
    SPEC_RULE("TypeRefsEnumPayload-None");
    return {};
  }
  return std::visit(
      [&](const auto& node) -> ModuleSet {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::EnumPayloadParen>) {
          SPEC_RULE("TypeRefsEnumPayload-Tuple");
          ModuleSet out;
          for (const auto& elem : node.elements) {
            if (!elem) {
              continue;
            }
            Merge(out, TypeRefsExpr(*elem, env));
          }
          return out;
        } else {
          SPEC_RULE("TypeRefsEnumPayload-Record");
          return TypeRefsExprs(node.fields, env);
        }
      },
      *payload);
}

ModuleSet TypeRefsFields(const std::vector<syntax::FieldPattern>& fields,
                         const InitEnv& env);
ModuleSet TypeRefsPayload(const std::optional<syntax::EnumPayloadPattern>& payload,
                           const InitEnv& env);

ModuleSet TypeRefsPat(const syntax::Pattern& pat, const InitEnv& env) {
  SpecDefsInitPlanner();
  return std::visit(
      [&](const auto& node) -> ModuleSet {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
          SPEC_RULE("TypeRef-RecordPattern");
          ModuleSet out = TypeRefsTypePath(node.path, env);
          Merge(out, TypeRefsFields(node.fields, env));
          return out;
        } else if constexpr (std::is_same_v<T, syntax::EnumPattern>) {
          SPEC_RULE("TypeRef-EnumPattern");
          ModuleSet out = TypeRefsTypePath(node.path, env);
          Merge(out, TypeRefsPayload(node.payload_opt, env));
          return out;
        } else if constexpr (std::is_same_v<T, syntax::LiteralPattern>) {
          SPEC_RULE("TypeRef-LiteralPattern");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::WildcardPattern>) {
          SPEC_RULE("TypeRef-WildcardPattern");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          SPEC_RULE("TypeRef-IdentifierPattern");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
          SPEC_RULE("TypeRef-TypedPattern");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
          SPEC_RULE("TypeRef-TuplePattern");
          ModuleSet out;
          for (const auto& elem : node.elements) {
            if (!elem) {
              continue;
            }
            Merge(out, TypeRefsPat(*elem, env));
          }
          return out;
        } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
          if (!node.fields_opt.has_value()) {
            SPEC_RULE("TypeRef-ModalPattern-None");
            return {};
          }
          SPEC_RULE("TypeRef-ModalPattern-Record");
          return TypeRefsFields(node.fields_opt->fields, env);
        } else if constexpr (std::is_same_v<T, syntax::RangePattern>) {
          SPEC_RULE("TypeRef-RangePattern");
          ModuleSet out;
          if (node.lo) {
            Merge(out, TypeRefsPat(*node.lo, env));
          }
          if (node.hi) {
            Merge(out, TypeRefsPat(*node.hi, env));
          }
          return out;
        } else {
          return {};
        }
      },
      pat.node);
}

ModuleSet TypeRefsFields(const std::vector<syntax::FieldPattern>& fields,
                         const InitEnv& env) {
  SpecDefsInitPlanner();
  if (fields.empty()) {
    SPEC_RULE("TypeRefsFields-Empty");
    return {};
  }
  ModuleSet out;
  for (const auto& field : fields) {
    SPEC_RULE("TypeRefsFields-Cons");
    if (field.pattern_opt) {
      SPEC_RULE("TypeRef-Field-Explicit");
      Merge(out, TypeRefsPat(*field.pattern_opt, env));
    } else {
      SPEC_RULE("TypeRef-Field-Implicit");
    }
  }
  return out;
}

ModuleSet TypeRefsPayload(const std::optional<syntax::EnumPayloadPattern>& payload,
                           const InitEnv& env) {
  SpecDefsInitPlanner();
  if (!payload.has_value()) {
    SPEC_RULE("TypeRefsPayload-None");
    return {};
  }
  return std::visit(
      [&](const auto& node) -> ModuleSet {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TuplePayloadPattern>) {
          SPEC_RULE("TypeRefsPayload-Tuple");
          ModuleSet out;
          for (const auto& elem : node.elements) {
            if (!elem) {
              continue;
            }
            Merge(out, TypeRefsPat(*elem, env));
          }
          return out;
        } else {
          SPEC_RULE("TypeRefsPayload-Record");
          return TypeRefsFields(node.fields, env);
        }
      },
      *payload);
}

syntax::ModulePath FullPath(const syntax::ModulePath& path,
                            std::string_view name) {
  syntax::ModulePath out = path;
  out.emplace_back(name);
  return out;
}

syntax::ModulePath EnumPath(const syntax::ModulePath& path) {
  if (path.size() < 2) {
    return {};
  }
  return syntax::ModulePath(path.begin(), path.end() - 1);
}

ModuleSet TypeRefsExpr(const syntax::Expr& expr, const InitEnv& env) {
  SpecDefsInitPlanner();
  return std::visit(
      [&](const auto& node) -> ModuleSet {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          SPEC_RULE("TypeRef-RecordExpr");
          ModuleSet out = TypeRefsRef(node.target, env);
          Merge(out, TypeRefsExprs(node.fields, env));
          return out;
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          SPEC_RULE("TypeRef-EnumLiteral");
          syntax::TypePath enum_path = EnumPath(node.path);
          ModuleSet out = TypeRefsTypePath(enum_path, env);
          Merge(out, TypeRefsEnumPayload(node.payload_opt, env));
          return out;
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          if (std::holds_alternative<syntax::BraceArgs>(node.args)) {
            SPEC_RULE("TypeRef-QualBrace");
            const auto& fields = std::get<syntax::BraceArgs>(node.args).fields;
            syntax::TypePath full = FullPath(node.path, node.name);
            ModuleSet out = TypeRefsTypePath(full, env);
            Merge(out, TypeRefsExprs(fields, env));
            return out;
          }
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          SPEC_RULE("TypeRef-Cast");
          ModuleSet out;
          if (node.value) {
            Merge(out, TypeRefsExpr(*node.value, env));
          }
          if (node.type) {
            Merge(out, TypeRefsTy(*node.type, env));
          }
          return out;
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          SPEC_RULE("TypeRef-Transmute");
          ModuleSet out;
          if (node.value) {
            Merge(out, TypeRefsExpr(*node.value, env));
          }
          if (node.from) {
            Merge(out, TypeRefsTy(*node.from, env));
          }
          if (node.to) {
            Merge(out, TypeRefsTy(*node.to, env));
          }
          return out;
        }

        ModuleSet out;
        for (const auto& child : ChildrenLtr(expr)) {
          if (!child) {
            continue;
          }
          Merge(out, TypeRefsExpr(*child, env));
        }
        SPEC_RULE("TypeRef-Expr-Sub");
        return out;
      },
      expr.node);
}

ModuleSet ValueRefs(const syntax::Expr& expr, const InitEnv& env);

ModuleSet ValueRefsArgs(const std::vector<syntax::Arg>& args,
                        const InitEnv& env) {
  SpecDefsInitPlanner();
  if (args.empty()) {
    SPEC_RULE("ValueRefsArgs-Empty");
    return {};
  }
  ModuleSet out;
  for (const auto& arg : args) {
    SPEC_RULE("ValueRefsArgs-Cons");
    if (arg.value) {
      Merge(out, ValueRefs(*arg.value, env));
    }
  }
  return out;
}

ModuleSet ValueRefsFields(const std::vector<syntax::FieldInit>& fields,
                          const InitEnv& env) {
  SpecDefsInitPlanner();
  if (fields.empty()) {
    SPEC_RULE("ValueRefsFields-Empty");
    return {};
  }
  ModuleSet out;
  for (const auto& field : fields) {
    SPEC_RULE("ValueRefsFields-Cons");
    if (field.value) {
      Merge(out, ValueRefs(*field.value, env));
    }
  }
  return out;
}

ModuleSet ValueRefsQualified(const syntax::ModulePath& path,
                             const InitEnv& env) {
  const auto mp = ModulePrefix(path, env);
  if (mp.has_value() && !PathEq(*mp, env.self)) {
    SPEC_RULE("ValueRef-Qual");
    return SingleModule(env, mp);
  }
  SPEC_RULE("ValueRef-Qual-Local");
  return {};
}

ModuleSet ValueRefs(const syntax::Expr& expr, const InitEnv& env) {
  SpecDefsInitPlanner();
  return std::visit(
      [&](const auto& node) -> ModuleSet {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          const auto key = IdKeyOf(node.name);
          const auto it = env.using_value.find(key);
          if (it != env.using_value.end() && !PathEq(it->second, env.self)) {
            SPEC_RULE("ValueRef-Ident");
            return SingleModule(env, it->second);
          }
          SPEC_RULE("ValueRef-Ident-Local");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::QualifiedNameExpr>) {
          return ValueRefsQualified(node.path, env);
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          if (std::holds_alternative<syntax::ParenArgs>(node.args)) {
            ModuleSet args =
                ValueRefsArgs(std::get<syntax::ParenArgs>(node.args).args, env);
            const auto mp = ModulePrefix(node.path, env);
            if (mp.has_value() && !PathEq(*mp, env.self)) {
              SPEC_RULE("ValueRef-QualApply");
              Merge(args, SingleModule(env, mp));
              return args;
            }
            SPEC_RULE("ValueRef-QualApply-Local");
            return args;
          }
          SPEC_RULE("ValueRef-QualApply-Brace");
          return ValueRefsFields(
              std::get<syntax::BraceArgs>(node.args).fields, env);
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          return ValueRefsQualified(node.path, env);
        }

        ModuleSet out;
        for (const auto& child : ChildrenLtr(expr)) {
          if (!child) {
            continue;
          }
          Merge(out, ValueRefs(*child, env));
        }
        SPEC_RULE("ValueRef-Expr-Sub");
        return out;
      },
      expr.node);
}

void CollectExprNodesFromType(const std::shared_ptr<syntax::Type>& type,
                              std::vector<syntax::ExprPtr>& out);

void CollectExprNodes(const syntax::ExprPtr& expr,
                      std::vector<syntax::ExprPtr>& out);

void CollectExprNodesFromBlock(const std::shared_ptr<syntax::Block>& block,
                               std::vector<syntax::ExprPtr>& out);

void CollectExprNodesFromStmt(const syntax::Stmt& stmt,
                              std::vector<syntax::ExprPtr>& out) {
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::LetStmt> ||
                      std::is_same_v<T, syntax::VarStmt>) {
          CollectExprNodes(node.binding.init, out);
          CollectExprNodesFromType(node.binding.type_opt, out);
        } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt> ||
                             std::is_same_v<T, syntax::ShadowVarStmt>) {
          CollectExprNodes(node.init, out);
          CollectExprNodesFromType(node.type_opt, out);
        } else if constexpr (std::is_same_v<T, syntax::AssignStmt> ||
                             std::is_same_v<T, syntax::CompoundAssignStmt>) {
          CollectExprNodes(node.place, out);
          CollectExprNodes(node.value, out);
        } else if constexpr (std::is_same_v<T, syntax::ExprStmt>) {
          CollectExprNodes(node.value, out);
        } else if constexpr (std::is_same_v<T, syntax::DeferStmt>) {
          CollectExprNodesFromBlock(node.body, out);
        } else if constexpr (std::is_same_v<T, syntax::RegionStmt>) {
          CollectExprNodes(node.opts_opt, out);
          CollectExprNodesFromBlock(node.body, out);
        } else if constexpr (std::is_same_v<T, syntax::FrameStmt>) {
          CollectExprNodesFromBlock(node.body, out);
        } else if constexpr (std::is_same_v<T, syntax::ReturnStmt>) {
          CollectExprNodes(node.value_opt, out);
        } else if constexpr (std::is_same_v<T, syntax::ResultStmt>) {
          CollectExprNodes(node.value, out);
        } else if constexpr (std::is_same_v<T, syntax::BreakStmt>) {
          CollectExprNodes(node.value_opt, out);
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockStmt>) {
          CollectExprNodesFromBlock(node.body, out);
        }
      },
      stmt);
}

void CollectExprNodesFromBlock(const std::shared_ptr<syntax::Block>& block,
                               std::vector<syntax::ExprPtr>& out) {
  if (!block) {
    return;
  }
  for (const auto& stmt : block->stmts) {
    CollectExprNodesFromStmt(stmt, out);
  }
  CollectExprNodes(block->tail_opt, out);
}

void CollectExprNodesFromType(const std::shared_ptr<syntax::Type>& type,
                              std::vector<syntax::ExprPtr>& out) {
  if (!type) {
    return;
  }
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypePermType>) {
          CollectExprNodesFromType(node.base, out);
        } else if constexpr (std::is_same_v<T, syntax::TypeUnion>) {
          for (const auto& elem : node.types) {
            CollectExprNodesFromType(elem, out);
          }
        } else if constexpr (std::is_same_v<T, syntax::TypeFunc>) {
          for (const auto& param : node.params) {
            CollectExprNodesFromType(param.type, out);
          }
          CollectExprNodesFromType(node.ret, out);
        } else if constexpr (std::is_same_v<T, syntax::TypeTuple>) {
          for (const auto& elem : node.elements) {
            CollectExprNodesFromType(elem, out);
          }
        } else if constexpr (std::is_same_v<T, syntax::TypeArray>) {
          CollectExprNodesFromType(node.element, out);
          CollectExprNodes(node.length, out);
        } else if constexpr (std::is_same_v<T, syntax::TypeSlice>) {
          CollectExprNodesFromType(node.element, out);
        } else if constexpr (std::is_same_v<T, syntax::TypePtr>) {
          CollectExprNodesFromType(node.element, out);
        } else if constexpr (std::is_same_v<T, syntax::TypeRawPtr>) {
          CollectExprNodesFromType(node.element, out);
        } else if constexpr (std::is_same_v<T, syntax::TypeRefine>) {
          CollectExprNodesFromType(node.base, out);
          CollectExprNodes(node.predicate, out);
        }
      },
      type->node);
}

void CollectExprNodes(const syntax::ExprPtr& expr,
                      std::vector<syntax::ExprPtr>& out) {
  if (!expr) {
    return;
  }
  out.push_back(expr);
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          CollectExprNodes(node.value, out);
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          CollectExprNodes(node.lhs, out);
          CollectExprNodes(node.rhs, out);
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          CollectExprNodes(node.value, out);
          CollectExprNodesFromType(node.type, out);
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          CollectExprNodes(node.value, out);
          CollectExprNodesFromType(node.from, out);
          CollectExprNodesFromType(node.to, out);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          CollectExprNodes(node.value, out);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          CollectExprNodes(node.place, out);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          CollectExprNodes(node.place, out);
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          CollectExprNodes(node.value, out);
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          for (const auto& elem : node.elements) {
            CollectExprNodes(elem, out);
          }
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          for (const auto& elem : node.elements) {
            CollectExprNodes(elem, out);
          }
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          for (const auto& field : node.fields) {
            CollectExprNodes(field.value, out);
          }
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          if (!node.payload_opt.has_value()) {
            return;
          }
          std::visit(
              [&](const auto& payload) {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::EnumPayloadParen>) {
                  for (const auto& elem : payload.elements) {
                    CollectExprNodes(elem, out);
                  }
                } else {
                  for (const auto& field : payload.fields) {
                    CollectExprNodes(field.value, out);
                  }
                }
              },
              *node.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          CollectExprNodes(node.cond, out);
          CollectExprNodes(node.then_expr, out);
          CollectExprNodes(node.else_expr, out);
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          CollectExprNodes(node.value, out);
          for (const auto& arm : node.arms) {
            CollectExprNodes(arm.guard_opt, out);
            CollectExprNodes(arm.body, out);
          }
        } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          CollectExprNodesFromBlock(node.body, out);
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          CollectExprNodes(node.cond, out);
          CollectExprNodesFromBlock(node.body, out);
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          CollectExprNodes(node.iter, out);
          CollectExprNodesFromType(node.type_opt, out);
          CollectExprNodesFromBlock(node.body, out);
        } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
          CollectExprNodesFromBlock(node.block, out);
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
          CollectExprNodesFromBlock(node.block, out);
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          CollectExprNodes(node.lhs, out);
          CollectExprNodes(node.rhs, out);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          CollectExprNodes(node.base, out);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          CollectExprNodes(node.base, out);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          CollectExprNodes(node.base, out);
          CollectExprNodes(node.index, out);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          CollectExprNodes(node.callee, out);
          for (const auto& arg : node.args) {
            CollectExprNodes(arg.value, out);
          }
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          CollectExprNodes(node.receiver, out);
          for (const auto& arg : node.args) {
            CollectExprNodes(arg.value, out);
          }
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          CollectExprNodes(node.value, out);
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          if (std::holds_alternative<syntax::ParenArgs>(node.args)) {
            for (const auto& arg : std::get<syntax::ParenArgs>(node.args).args) {
              CollectExprNodes(arg.value, out);
            }
          } else {
            for (const auto& field : std::get<syntax::BraceArgs>(node.args).fields) {
              CollectExprNodes(field.value, out);
            }
          }
        }
      },
      expr->node);
}

void CollectPatNodesFromPattern(const syntax::PatternPtr& pattern,
                                std::vector<syntax::PatternPtr>& out,
                                std::vector<syntax::ExprPtr>& expr_out);

void CollectPatNodesFromPattern(const syntax::PatternPtr& pattern,
                                std::vector<syntax::PatternPtr>& out,
                                std::vector<syntax::ExprPtr>& expr_out) {
  if (!pattern) {
    return;
  }
  out.push_back(pattern);
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
          for (const auto& elem : node.elements) {
            CollectPatNodesFromPattern(elem, out, expr_out);
          }
        } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
          for (const auto& field : node.fields) {
            CollectPatNodesFromPattern(field.pattern_opt, out, expr_out);
          }
        } else if constexpr (std::is_same_v<T, syntax::EnumPattern>) {
          if (!node.payload_opt.has_value()) {
            return;
          }
          std::visit(
              [&](const auto& payload) {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::TuplePayloadPattern>) {
                  for (const auto& elem : payload.elements) {
                    CollectPatNodesFromPattern(elem, out, expr_out);
                  }
                } else {
                  for (const auto& field : payload.fields) {
                    CollectPatNodesFromPattern(field.pattern_opt, out, expr_out);
                  }
                }
              },
              *node.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
          if (!node.fields_opt.has_value()) {
            return;
          }
          for (const auto& field : node.fields_opt->fields) {
            CollectPatNodesFromPattern(field.pattern_opt, out, expr_out);
          }
        } else if constexpr (std::is_same_v<T, syntax::RangePattern>) {
          CollectPatNodesFromPattern(node.lo, out, expr_out);
          CollectPatNodesFromPattern(node.hi, out, expr_out);
        } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
          CollectExprNodesFromType(node.type, expr_out);
        }
      },
      pattern->node);
}

void CollectPatNodesFromExpr(const syntax::ExprPtr& expr,
                             std::vector<syntax::PatternPtr>& out,
                             std::vector<syntax::ExprPtr>& expr_out);

void CollectPatNodesFromStmt(const syntax::Stmt& stmt,
                             std::vector<syntax::PatternPtr>& out,
                             std::vector<syntax::ExprPtr>& expr_out) {
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::LetStmt> ||
                      std::is_same_v<T, syntax::VarStmt>) {
          CollectPatNodesFromPattern(node.binding.pat, out, expr_out);
        }
      },
      stmt);
}

void CollectPatNodesFromBlock(const std::shared_ptr<syntax::Block>& block,
                              std::vector<syntax::PatternPtr>& out,
                              std::vector<syntax::ExprPtr>& expr_out) {
  if (!block) {
    return;
  }
  for (const auto& stmt : block->stmts) {
    CollectPatNodesFromStmt(stmt, out, expr_out);
  }
  CollectPatNodesFromExpr(block->tail_opt, out, expr_out);
}

void CollectPatNodesFromExpr(const syntax::ExprPtr& expr,
                             std::vector<syntax::PatternPtr>& out,
                             std::vector<syntax::ExprPtr>& expr_out) {
  if (!expr) {
    return;
  }
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          for (const auto& arm : node.arms) {
            CollectPatNodesFromPattern(arm.pattern, out, expr_out);
          }
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          CollectPatNodesFromPattern(node.pattern, out, expr_out);
          CollectExprNodesFromType(node.type_opt, expr_out);
          CollectPatNodesFromBlock(node.body, out, expr_out);
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          CollectPatNodesFromExpr(node.cond, out, expr_out);
          CollectPatNodesFromExpr(node.then_expr, out, expr_out);
          CollectPatNodesFromExpr(node.else_expr, out, expr_out);
        } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
          CollectPatNodesFromBlock(node.block, out, expr_out);
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
          CollectPatNodesFromBlock(node.block, out, expr_out);
        } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          CollectPatNodesFromBlock(node.body, out, expr_out);
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          CollectPatNodesFromBlock(node.body, out, expr_out);
        }
      },
      expr->node);
}

struct EdgeSets {
  std::vector<std::set<std::size_t>> type_edges;
  std::vector<std::set<std::size_t>> eager_edges;
  std::vector<std::set<std::size_t>> lazy_edges;
};

void AddEdges(std::vector<std::set<std::size_t>>& edges,
              std::size_t from,
              const ModuleSet& deps) {
  for (const auto dep : deps) {
    if (dep < edges.size()) {
      edges[dep].insert(from);
    }
  }
}

ModuleSet TypeRefsFromPathList(const std::vector<syntax::ClassPath>& paths,
                               const InitEnv& env) {
  ModuleSet out;
  for (const auto& path : paths) {
    if (path.empty()) {
      continue;
    }
    syntax::TypePath as_type(path.begin(), path.end());
    Merge(out, TypeRefsTypePath(as_type, env));
  }
  return out;
}

void CollectTypePositions(const syntax::ASTModule& module,
                          const InitEnv& env,
                          ModuleSet& out,
                          std::vector<syntax::ExprPtr>& type_pos_exprs) {
  for (const auto& item : module.items) {
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::StaticDecl>) {
            if (node.binding.type_opt) {
              Merge(out, TypeRefsTy(*node.binding.type_opt, env));
            }
          } else if constexpr (std::is_same_v<T, syntax::ProcedureDecl>) {
            for (const auto& param : node.params) {
              if (param.type) {
                Merge(out, TypeRefsTy(*param.type, env));
              }
            }
            if (node.return_type_opt) {
              Merge(out, TypeRefsTy(*node.return_type_opt, env));
            }
          } else if constexpr (std::is_same_v<T, syntax::RecordDecl>) {
            Merge(out, TypeRefsFromPathList(node.implements, env));
            for (const auto& member : node.members) {
              std::visit(
                  [&](const auto& mem) {
                    using M = std::decay_t<decltype(mem)>;
                    if constexpr (std::is_same_v<M, syntax::FieldDecl>) {
                      if (mem.type) {
                        Merge(out, TypeRefsTy(*mem.type, env));
                      }
                    } else if constexpr (std::is_same_v<M, syntax::MethodDecl>) {
                      if (std::holds_alternative<syntax::ReceiverExplicit>(
                              mem.receiver)) {
                        const auto& recv =
                            std::get<syntax::ReceiverExplicit>(mem.receiver);
                        if (recv.type) {
                          Merge(out, TypeRefsTy(*recv.type, env));
                        }
                      }
                      for (const auto& param : mem.params) {
                        if (param.type) {
                          Merge(out, TypeRefsTy(*param.type, env));
                        }
                      }
                      if (mem.return_type_opt) {
                        Merge(out, TypeRefsTy(*mem.return_type_opt, env));
                      }
                    }
                  },
                  member);
            }
          } else if constexpr (std::is_same_v<T, syntax::EnumDecl>) {
            Merge(out, TypeRefsFromPathList(node.implements, env));
            for (const auto& variant : node.variants) {
              if (!variant.payload_opt.has_value()) {
                continue;
              }
              std::visit(
                  [&](const auto& payload) {
                    using P = std::decay_t<decltype(payload)>;
                    if constexpr (std::is_same_v<P, syntax::VariantPayloadTuple>) {
                      for (const auto& elem : payload.elements) {
                        if (elem) {
                          Merge(out, TypeRefsTy(*elem, env));
                        }
                      }
                    } else {
                      for (const auto& field : payload.fields) {
                        if (field.type) {
                          Merge(out, TypeRefsTy(*field.type, env));
                        }
                      }
                    }
                  },
                  *variant.payload_opt);
            }
            for (const auto& variant : node.variants) {
              if (!variant.discriminant_opt.has_value()) {
                continue;
              }
              syntax::LiteralExpr lit;
              lit.literal = *variant.discriminant_opt;
              auto expr = std::make_shared<syntax::Expr>();
              expr->span = lit.literal.span;
              expr->node = lit;
              type_pos_exprs.push_back(expr);
            }
          } else if constexpr (std::is_same_v<T, syntax::ModalDecl>) {
            Merge(out, TypeRefsFromPathList(node.implements, env));
          } else if constexpr (std::is_same_v<T, syntax::ClassDecl>) {
            Merge(out, TypeRefsFromPathList(node.supers, env));
            for (const auto& item : node.items) {
              std::visit(
                  [&](const auto& ci) {
                    using C = std::decay_t<decltype(ci)>;
                    if constexpr (std::is_same_v<C, syntax::ClassFieldDecl>) {
                      if (ci.type) {
                        Merge(out, TypeRefsTy(*ci.type, env));
                      }
                    } else if constexpr (std::is_same_v<C, syntax::ClassMethodDecl>) {
                      if (std::holds_alternative<syntax::ReceiverExplicit>(
                              ci.receiver)) {
                        const auto& recv =
                            std::get<syntax::ReceiverExplicit>(ci.receiver);
                        if (recv.type) {
                          Merge(out, TypeRefsTy(*recv.type, env));
                        }
                      }
                      for (const auto& param : ci.params) {
                        if (param.type) {
                          Merge(out, TypeRefsTy(*param.type, env));
                        }
                      }
                      if (ci.return_type_opt) {
                        Merge(out, TypeRefsTy(*ci.return_type_opt, env));
                      }
                    }
                  },
                  item);
            }
          } else if constexpr (std::is_same_v<T, syntax::TypeAliasDecl>) {
            if (node.type) {
              Merge(out, TypeRefsTy(*node.type, env));
            }
          }
        },
        item);
  }
}

void CollectArraySizeExprs(const std::shared_ptr<syntax::Type>& type,
                           std::vector<syntax::ExprPtr>& out) {
  if (!type) {
    return;
  }
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypeArray>) {
          if (node.length) {
            out.push_back(node.length);
          }
          CollectArraySizeExprs(node.element, out);
        } else if constexpr (std::is_same_v<T, syntax::TypePermType>) {
          CollectArraySizeExprs(node.base, out);
        } else if constexpr (std::is_same_v<T, syntax::TypeTuple>) {
          for (const auto& elem : node.elements) {
            CollectArraySizeExprs(elem, out);
          }
        } else if constexpr (std::is_same_v<T, syntax::TypeUnion>) {
          for (const auto& elem : node.types) {
            CollectArraySizeExprs(elem, out);
          }
        } else if constexpr (std::is_same_v<T, syntax::TypeFunc>) {
          for (const auto& param : node.params) {
            CollectArraySizeExprs(param.type, out);
          }
          CollectArraySizeExprs(node.ret, out);
        } else if constexpr (std::is_same_v<T, syntax::TypeSlice>) {
          CollectArraySizeExprs(node.element, out);
        } else if constexpr (std::is_same_v<T, syntax::TypePtr>) {
          CollectArraySizeExprs(node.element, out);
        } else if constexpr (std::is_same_v<T, syntax::TypeRawPtr>) {
          CollectArraySizeExprs(node.element, out);
        } else if constexpr (std::is_same_v<T, syntax::TypeRefine>) {
          CollectArraySizeExprs(node.base, out);
        }
      },
      type->node);
}

void CollectTypePosExprs(const syntax::ASTModule& module,
                         std::vector<syntax::ExprPtr>& out) {
  for (const auto& item : module.items) {
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::StaticDecl>) {
            CollectArraySizeExprs(node.binding.type_opt, out);
          } else if constexpr (std::is_same_v<T, syntax::ProcedureDecl>) {
            for (const auto& param : node.params) {
              CollectArraySizeExprs(param.type, out);
            }
            CollectArraySizeExprs(node.return_type_opt, out);
          } else if constexpr (std::is_same_v<T, syntax::RecordDecl>) {
            for (const auto& member : node.members) {
              std::visit(
                  [&](const auto& mem) {
                    using M = std::decay_t<decltype(mem)>;
                    if constexpr (std::is_same_v<M, syntax::FieldDecl>) {
                      CollectArraySizeExprs(mem.type, out);
                    } else if constexpr (std::is_same_v<M, syntax::MethodDecl>) {
                      if (std::holds_alternative<syntax::ReceiverExplicit>(
                              mem.receiver)) {
                        const auto& recv =
                            std::get<syntax::ReceiverExplicit>(mem.receiver);
                        CollectArraySizeExprs(recv.type, out);
                      }
                      for (const auto& param : mem.params) {
                        CollectArraySizeExprs(param.type, out);
                      }
                      CollectArraySizeExprs(mem.return_type_opt, out);
                    }
                  },
                  member);
            }
          } else if constexpr (std::is_same_v<T, syntax::EnumDecl>) {
            for (const auto& variant : node.variants) {
              if (!variant.payload_opt.has_value()) {
                continue;
              }
              std::visit(
                  [&](const auto& payload) {
                    using P = std::decay_t<decltype(payload)>;
                    if constexpr (std::is_same_v<P, syntax::VariantPayloadTuple>) {
                      for (const auto& elem : payload.elements) {
                        CollectArraySizeExprs(elem, out);
                      }
                    } else {
                      for (const auto& field : payload.fields) {
                        CollectArraySizeExprs(field.type, out);
                      }
                    }
                  },
                  *variant.payload_opt);
            }
          } else if constexpr (std::is_same_v<T, syntax::ModalDecl>) {
            for (const auto& state : node.states) {
              for (const auto& member : state.members) {
                std::visit(
                    [&](const auto& mem) {
                      using M = std::decay_t<decltype(mem)>;
                      if constexpr (std::is_same_v<M, syntax::StateFieldDecl>) {
                        CollectArraySizeExprs(mem.type, out);
                      } else if constexpr (std::is_same_v<M, syntax::StateMethodDecl>) {
                        for (const auto& param : mem.params) {
                          CollectArraySizeExprs(param.type, out);
                        }
                        CollectArraySizeExprs(mem.return_type_opt, out);
                      } else if constexpr (std::is_same_v<M, syntax::TransitionDecl>) {
                        for (const auto& param : mem.params) {
                          CollectArraySizeExprs(param.type, out);
                        }
                      }
                    },
                    member);
              }
            }
          } else if constexpr (std::is_same_v<T, syntax::ClassDecl>) {
            for (const auto& member : node.items) {
              std::visit(
                  [&](const auto& mem) {
                    using M = std::decay_t<decltype(mem)>;
                    if constexpr (std::is_same_v<M, syntax::ClassFieldDecl>) {
                      CollectArraySizeExprs(mem.type, out);
                    } else if constexpr (std::is_same_v<M, syntax::ClassMethodDecl>) {
                      if (std::holds_alternative<syntax::ReceiverExplicit>(
                              mem.receiver)) {
                        const auto& recv =
                            std::get<syntax::ReceiverExplicit>(mem.receiver);
                        CollectArraySizeExprs(recv.type, out);
                      }
                      for (const auto& param : mem.params) {
                        CollectArraySizeExprs(param.type, out);
                      }
                      CollectArraySizeExprs(mem.return_type_opt, out);
                    }
                  },
                  member);
            }
          } else if constexpr (std::is_same_v<T, syntax::TypeAliasDecl>) {
            CollectArraySizeExprs(node.type, out);
          }
        },
        item);
  }
}

ModuleSet TypeDepsForModule(const syntax::ASTModule& module,
                            const InitEnv& env) {
  ModuleSet deps;
  std::vector<syntax::ExprPtr> type_pos_exprs;
  CollectTypePositions(module, env, deps, type_pos_exprs);

  std::vector<syntax::ExprPtr> expr_nodes;
  CollectExprNodesFromBlock(nullptr, expr_nodes);
  for (const auto& item : module.items) {
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::StaticDecl>) {
            CollectExprNodes(node.binding.init, expr_nodes);
            CollectExprNodesFromType(node.binding.type_opt, expr_nodes);
          } else if constexpr (std::is_same_v<T, syntax::ProcedureDecl>) {
            CollectExprNodesFromBlock(node.body, expr_nodes);
            for (const auto& param : node.params) {
              CollectExprNodesFromType(param.type, expr_nodes);
            }
            CollectExprNodesFromType(node.return_type_opt, expr_nodes);
          } else if constexpr (std::is_same_v<T, syntax::RecordDecl>) {
            for (const auto& member : node.members) {
              std::visit(
                  [&](const auto& mem) {
                    using M = std::decay_t<decltype(mem)>;
                    if constexpr (std::is_same_v<M, syntax::FieldDecl>) {
                      CollectExprNodes(mem.init_opt, expr_nodes);
                      CollectExprNodesFromType(mem.type, expr_nodes);
                    } else if constexpr (std::is_same_v<M, syntax::MethodDecl>) {
                      CollectExprNodesFromBlock(mem.body, expr_nodes);
                      if (std::holds_alternative<syntax::ReceiverExplicit>(
                              mem.receiver)) {
                        const auto& recv =
                            std::get<syntax::ReceiverExplicit>(mem.receiver);
                        CollectExprNodesFromType(recv.type, expr_nodes);
                      }
                      for (const auto& param : mem.params) {
                        CollectExprNodesFromType(param.type, expr_nodes);
                      }
                      CollectExprNodesFromType(mem.return_type_opt, expr_nodes);
                    }
                  },
                  member);
            }
          } else if constexpr (std::is_same_v<T, syntax::EnumDecl>) {
            for (const auto& variant : node.variants) {
              if (!variant.payload_opt.has_value()) {
                continue;
              }
              std::visit(
                  [&](const auto& payload) {
                    using P = std::decay_t<decltype(payload)>;
                    if constexpr (std::is_same_v<P, syntax::VariantPayloadTuple>) {
                      for (const auto& elem : payload.elements) {
                        CollectExprNodesFromType(elem, expr_nodes);
                      }
                    } else {
                      for (const auto& field : payload.fields) {
                        CollectExprNodesFromType(field.type, expr_nodes);
                        CollectExprNodes(field.init_opt, expr_nodes);
                      }
                    }
                  },
                  *variant.payload_opt);
            }
          } else if constexpr (std::is_same_v<T, syntax::ModalDecl>) {
            for (const auto& state : node.states) {
              for (const auto& member : state.members) {
                std::visit(
                    [&](const auto& mem) {
                      using M = std::decay_t<decltype(mem)>;
                      if constexpr (std::is_same_v<M, syntax::StateFieldDecl>) {
                        CollectExprNodesFromType(mem.type, expr_nodes);
                      } else if constexpr (std::is_same_v<M, syntax::StateMethodDecl>) {
                        CollectExprNodesFromBlock(mem.body, expr_nodes);
                        for (const auto& param : mem.params) {
                          CollectExprNodesFromType(param.type, expr_nodes);
                        }
                        CollectExprNodesFromType(mem.return_type_opt, expr_nodes);
                      } else if constexpr (std::is_same_v<M, syntax::TransitionDecl>) {
                        CollectExprNodesFromBlock(mem.body, expr_nodes);
                        for (const auto& param : mem.params) {
                          CollectExprNodesFromType(param.type, expr_nodes);
                        }
                      }
                    },
                    member);
              }
            }
          } else if constexpr (std::is_same_v<T, syntax::ClassDecl>) {
            for (const auto& member : node.items) {
              std::visit(
                  [&](const auto& mem) {
                    using M = std::decay_t<decltype(mem)>;
                    if constexpr (std::is_same_v<M, syntax::ClassFieldDecl>) {
                      CollectExprNodesFromType(mem.type, expr_nodes);
                    } else if constexpr (std::is_same_v<M, syntax::ClassMethodDecl>) {
                      if (mem.body_opt) {
                        CollectExprNodesFromBlock(mem.body_opt, expr_nodes);
                      }
                      if (std::holds_alternative<syntax::ReceiverExplicit>(
                              mem.receiver)) {
                        const auto& recv =
                            std::get<syntax::ReceiverExplicit>(mem.receiver);
                        CollectExprNodesFromType(recv.type, expr_nodes);
                      }
                      for (const auto& param : mem.params) {
                        CollectExprNodesFromType(param.type, expr_nodes);
                      }
                      CollectExprNodesFromType(mem.return_type_opt, expr_nodes);
                    }
                  },
                  member);
            }
          } else if constexpr (std::is_same_v<T, syntax::TypeAliasDecl>) {
            CollectExprNodesFromType(node.type, expr_nodes);
          }
        },
        item);
  }

  std::vector<syntax::PatternPtr> pat_nodes;
  for (const auto& item : module.items) {
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::StaticDecl>) {
            CollectPatNodesFromPattern(node.binding.pat, pat_nodes, expr_nodes);
          } else if constexpr (std::is_same_v<T, syntax::ProcedureDecl>) {
            CollectPatNodesFromBlock(node.body, pat_nodes, expr_nodes);
          } else if constexpr (std::is_same_v<T, syntax::RecordDecl>) {
            for (const auto& member : node.members) {
              std::visit(
                  [&](const auto& mem) {
                    using M = std::decay_t<decltype(mem)>;
                    if constexpr (std::is_same_v<M, syntax::MethodDecl>) {
                      CollectPatNodesFromBlock(mem.body, pat_nodes, expr_nodes);
                    }
                  },
                  member);
            }
          } else if constexpr (std::is_same_v<T, syntax::ClassDecl>) {
            for (const auto& member : node.items) {
              std::visit(
                  [&](const auto& mem) {
                    using M = std::decay_t<decltype(mem)>;
                    if constexpr (std::is_same_v<M, syntax::ClassMethodDecl>) {
                      CollectPatNodesFromBlock(mem.body_opt, pat_nodes,
                                               expr_nodes);
                    }
                  },
                  member);
            }
          } else if constexpr (std::is_same_v<T, syntax::ModalDecl>) {
            for (const auto& state : node.states) {
              for (const auto& member : state.members) {
                std::visit(
                    [&](const auto& mem) {
                      using M = std::decay_t<decltype(mem)>;
                      if constexpr (std::is_same_v<M, syntax::StateMethodDecl>) {
                        CollectPatNodesFromBlock(mem.body, pat_nodes, expr_nodes);
                      } else if constexpr (std::is_same_v<M, syntax::TransitionDecl>) {
                        CollectPatNodesFromBlock(mem.body, pat_nodes, expr_nodes);
                      }
                    },
                    member);
              }
            }
          }
        },
        item);
  }

  for (const auto& pat : pat_nodes) {
    if (!pat) {
      continue;
    }
    Merge(deps, TypeRefsPat(*pat, env));
  }

  for (const auto& expr : expr_nodes) {
    if (!expr) {
      continue;
    }
    Merge(deps, TypeRefsExpr(*expr, env));
  }

  CollectTypePosExprs(module, type_pos_exprs);
  for (const auto& expr : type_pos_exprs) {
    if (!expr) {
      continue;
    }
    Merge(deps, TypeRefsExpr(*expr, env));
  }

  return deps;
}

ModuleSet ValueDepsEagerForModule(const syntax::ASTModule& module,
                                  const InitEnv& env) {
  ModuleSet deps;
  for (const auto& item : module.items) {
    const auto* decl = std::get_if<syntax::StaticDecl>(&item);
    if (!decl) {
      continue;
    }
    if (decl->binding.init) {
      Merge(deps, ValueRefs(*decl->binding.init, env));
    }
  }
  return deps;
}

void CollectFieldInitExprs(const syntax::ASTModule& module,
                           std::vector<syntax::ExprPtr>& out) {
  for (const auto& item : module.items) {
    const auto* rec = std::get_if<syntax::RecordDecl>(&item);
    if (!rec) {
      continue;
    }
    for (const auto& member : rec->members) {
      if (auto field = std::get_if<syntax::FieldDecl>(&member)) {
        if (field->init_opt) {
          out.push_back(field->init_opt);
        }
      }
    }
  }
}

void CollectBodyExprNodes(const std::shared_ptr<syntax::Block>& body,
                          std::vector<syntax::ExprPtr>& out) {
  CollectExprNodesFromBlock(body, out);
}

ModuleSet ValueDepsLazyForModule(const syntax::ASTModule& module,
                                 const InitEnv& env) {
  ModuleSet deps;
  std::vector<syntax::ExprPtr> exprs;
  CollectFieldInitExprs(module, exprs);

  for (const auto& item : module.items) {
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::ProcedureDecl>) {
            CollectBodyExprNodes(node.body, exprs);
          } else if constexpr (std::is_same_v<T, syntax::RecordDecl>) {
            for (const auto& member : node.members) {
              if (auto method = std::get_if<syntax::MethodDecl>(&member)) {
                CollectBodyExprNodes(method->body, exprs);
              }
            }
          } else if constexpr (std::is_same_v<T, syntax::ClassDecl>) {
            for (const auto& member : node.items) {
              if (auto method = std::get_if<syntax::ClassMethodDecl>(&member)) {
                if (method->body_opt) {
                  CollectBodyExprNodes(method->body_opt, exprs);
                }
              }
            }
          }
        },
        item);
  }

  for (const auto& expr : exprs) {
    if (!expr) {
      continue;
    }
    Merge(deps, ValueRefs(*expr, env));
  }
  return deps;
}

bool Reachable(std::size_t u,
               std::size_t v,
               const std::vector<std::set<std::size_t>>& edges,
               std::vector<bool>& visiting) {
  SpecDefsInitPlanner();
  if (edges[u].count(v) > 0) {
    SPEC_RULE("Reachable-Edge");
    return true;
  }
  for (const auto w : edges[u]) {
    if (visiting[w]) {
      continue;
    }
    visiting[w] = true;
    if (Reachable(w, v, edges, visiting)) {
      SPEC_RULE("Reachable-Step");
      return true;
    }
  }
  return false;
}

bool AcyclicEager(const std::vector<std::set<std::size_t>>& edges) {
  SpecDefsInitPlanner();
  const std::size_t n = edges.size();
  for (std::size_t v = 0; v < n; ++v) {
    std::vector<bool> visiting(n, false);
    visiting[v] = true;
    if (Reachable(v, v, edges, visiting)) {
      return false;
    }
  }
  SPEC_RULE("WF-Acyclic-Eager");
  return true;
}

std::vector<std::size_t> TopoOrder(
    const std::vector<std::set<std::size_t>>& edges,
    bool& ok) {
  SpecDefsInitPlanner();
  std::vector<std::size_t> order;
  const std::size_t n = edges.size();
  std::vector<std::size_t> indeg(n, 0);
  for (std::size_t u = 0; u < n; ++u) {
    for (const auto v : edges[u]) {
      if (v < n) {
        indeg[v] += 1;
      }
    }
  }
  std::set<std::size_t> ready;
  for (std::size_t i = 0; i < n; ++i) {
    if (indeg[i] == 0) {
      ready.insert(i);
    }
  }
  while (!ready.empty()) {
    const std::size_t u = *ready.begin();
    ready.erase(ready.begin());
    order.push_back(u);
    for (const auto v : edges[u]) {
      if (v >= n) {
        continue;
      }
      if (indeg[v] == 0) {
        continue;
      }
      indeg[v] -= 1;
      if (indeg[v] == 0) {
        ready.insert(v);
      }
    }
  }
  ok = order.size() == n;
  if (ok) {
    SPEC_RULE("Topo-Ok");
  } else {
    SPEC_RULE("Topo-Cycle");
  }
  return order;
}

std::vector<std::pair<std::size_t, std::size_t>> FlattenEdges(
    const std::vector<std::set<std::size_t>>& edges) {
  std::vector<std::pair<std::size_t, std::size_t>> out;
  for (std::size_t u = 0; u < edges.size(); ++u) {
    for (const auto v : edges[u]) {
      out.emplace_back(u, v);
    }
  }
  return out;
}

void EmitInitDiag(core::DiagnosticStream& diags, std::string_view diag_id) {
  if (diag_id != "Topo-Cycle") {
    return;
  }
  if (auto diag = core::MakeDiagnostic("E-MOD-1401")) {
    diag->span.reset();
    diags = core::Emit(diags, *diag);
  }
}

}  // namespace

InitPlanResult BuildInitPlan(const ScopeContext& ctx,
                             const NameMapTable& name_maps) {
  SpecDefsInitPlanner();
  InitPlanResult result;

  std::vector<syntax::ModulePath> modules;
  std::map<std::string, std::size_t> module_index;
  auto add_module = [&](const syntax::ModulePath& path) {
    const std::string key = ModuleKey(path);
    if (module_index.find(key) != module_index.end()) {
      return;
    }
    module_index.emplace(key, modules.size());
    modules.push_back(path);
  };
  if (ctx.project) {
    modules.reserve(ctx.project->modules.size());
    for (const auto& mod : ctx.project->modules) {
      add_module(SplitModulePath(mod.path));
    }
  }
  if (modules.empty()) {
    modules.reserve(ctx.sigma.mods.size());
  }
  for (const auto& mod : ctx.sigma.mods) {
    add_module(mod.path);
  }

  EdgeSets edges;
  edges.type_edges.resize(modules.size());
  edges.eager_edges.resize(modules.size());
  edges.lazy_edges.resize(modules.size());

  for (const auto& mod : ctx.sigma.mods) {
    const auto key = PathKeyOf(mod.path);
    const auto it = name_maps.find(key);
    NameMap names;
    if (it != name_maps.end()) {
      names = it->second;
    }

    InitEnv env;
    env.self = mod.path;
    env.modules = &modules;
    env.module_index = &module_index;
    env.alias = AliasMapOf(names);
    env.using_value = BuildUsingValueMap(names);
    env.using_type = BuildUsingTypeMap(names);
    const auto self_index = ModuleIndex(env, env.self);
    if (!self_index.has_value()) {
      continue;
    }
    env.self_index = *self_index;

    const ModuleSet type_deps = TypeDepsForModule(mod, env);
    const ModuleSet eager_deps = ValueDepsEagerForModule(mod, env);
    const ModuleSet lazy_deps = ValueDepsLazyForModule(mod, env);

    AddEdges(edges.type_edges, *self_index, type_deps);
    AddEdges(edges.eager_edges, *self_index, eager_deps);
    AddEdges(edges.lazy_edges, *self_index, lazy_deps);
  }

  InitPlan plan;
  plan.graph.modules = modules;
  plan.graph.type_edges = FlattenEdges(edges.type_edges);
  plan.graph.eager_edges = FlattenEdges(edges.eager_edges);
  plan.graph.lazy_edges = FlattenEdges(edges.lazy_edges);

  const bool acyclic = AcyclicEager(edges.eager_edges);
  bool topo_ok = false;
  if (acyclic) {
    const auto order = TopoOrder(edges.eager_edges, topo_ok);
    if (topo_ok) {
      plan.init_order.reserve(order.size());
      for (const auto idx : order) {
        if (idx < modules.size()) {
          plan.init_order.push_back(modules[idx]);
        }
      }
    }
  } else {
    EmitInitDiag(result.diags, "Topo-Cycle");
  }

  plan.topo_ok = topo_ok;
  result.plan = std::move(plan);
  result.ok = !core::HasError(result.diags);
  return result;
}

}  // namespace cursive0::analysis

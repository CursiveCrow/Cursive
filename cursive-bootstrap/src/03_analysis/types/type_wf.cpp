// =================================================================
// File: 03_analysis/types/type_wf.cpp
// Construct: Type Well-Formedness Checking
// Spec Section: 5.2.12
// Spec Rules: WF-Prim, WF-Perm, WF-Tuple, WF-Array, WF-Slice,
//             WF-Union, WF-Func, WF-Path, WF-Dynamic, WF-Opaque,
//             WF-Refine-Type, WF-String, WF-Bytes, WF-Ptr, WF-RawPtr,
//             WF-ModalState
// =================================================================
#include "cursive0/03_analysis/types/type_wf.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/contracts/contract_check.h"
#include "cursive0/03_analysis/modal/modal.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/expr/common.h"
#include "cursive0/03_analysis/types/type_expr.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_stmt.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsTypeWF() {
  SPEC_DEF("WF-Prim", "5.2.12");
  SPEC_DEF("WF-Perm", "5.2.12");
  SPEC_DEF("WF-Tuple", "5.2.12");
  SPEC_DEF("WF-Array", "5.2.12");
  SPEC_DEF("WF-Slice", "5.2.12");
  SPEC_DEF("WF-Union", "5.2.12");
  SPEC_DEF("WF-Union-TooFew", "5.2.12");
  SPEC_DEF("WF-Func", "5.2.12");
  SPEC_DEF("WF-Path", "5.2.12");
  SPEC_DEF("WF-Dynamic", "5.2.12");
  SPEC_DEF("WF-Dynamic-Err", "5.2.12");
  SPEC_DEF("WF-Opaque", "5.2.12");
  SPEC_DEF("WF-Opaque-Err", "5.2.12");
  SPEC_DEF("WF-Refine-Type", "5.2.12");
  SPEC_DEF("WF-String", "5.2.12");
  SPEC_DEF("WF-Bytes", "5.2.12");
  SPEC_DEF("WF-Ptr", "5.2.12");
  SPEC_DEF("WF-RawPtr", "5.2.12");
  SPEC_DEF("WF-ModalState", "5.2.12");
}

static bool IsSelfTypePath(const TypePath& path) {
  return path.size() == 1 && IdEq(path[0], "Self");
}

static bool IsBuiltinCapClassPath(const TypePath& path) {
  return path.size() == 1 &&
         (IdEq(path[0], "FileSystem") || IdEq(path[0], "HeapAllocator") ||
          IdEq(path[0], "Reactor"));
}

static TypeWfResult TypeWFImpl(const ScopeContext& ctx, const TypeRef& type) {
  SpecDefsTypeWF();
  if (!type) {
    return {};
  }
  return std::visit(
      [&](const auto& node) -> TypeWfResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, TypePrim>) {
          if (!expr::IsPrimTypeName(node.name)) {
            return {};
          }
          SPEC_RULE("WF-Prim");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypePerm>) {
          const auto base = TypeWFImpl(ctx, node.base);
          if (!base.ok) {
            return base;
          }
          SPEC_RULE("WF-Perm");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeTuple>) {
          for (const auto& elem : node.elements) {
            const auto wf = TypeWFImpl(ctx, elem);
            if (!wf.ok) {
              return wf;
            }
          }
          SPEC_RULE("WF-Tuple");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeArray>) {
          const auto wf = TypeWFImpl(ctx, node.element);
          if (!wf.ok) {
            return wf;
          }
          SPEC_RULE("WF-Array");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeSlice>) {
          const auto wf = TypeWFImpl(ctx, node.element);
          if (!wf.ok) {
            return wf;
          }
          SPEC_RULE("WF-Slice");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeUnion>) {
          if (node.members.size() < 2) {
            SPEC_RULE("WF-Union-TooFew");
            return {false, "WF-Union-TooFew"};
          }
          for (const auto& member : node.members) {
            const auto wf = TypeWFImpl(ctx, member);
            if (!wf.ok) {
              return wf;
            }
          }
          SPEC_RULE("WF-Union");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeFunc>) {
          for (const auto& param : node.params) {
            const auto wf = TypeWFImpl(ctx, param.type);
            if (!wf.ok) {
              return wf;
            }
          }
          const auto ret = TypeWFImpl(ctx, node.ret);
          if (!ret.ok) {
            return ret;
          }
          SPEC_RULE("WF-Func");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypePathType>) {
          if (IsSelfTypePath(node.path)) {
            SPEC_RULE("WF-Path");
            return {true, std::nullopt};
          }
          if (ctx.sigma.types.find(PathKeyOf(node.path)) == ctx.sigma.types.end()) {
            return {};
          }
          SPEC_RULE("WF-Path");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeDynamic>) {
          if (IsBuiltinCapClassPath(node.path)) {
            SPEC_RULE("WF-Dynamic");
            return {true, std::nullopt};
          }
          if (ctx.sigma.classes.find(PathKeyOf(node.path)) == ctx.sigma.classes.end()) {
            SPEC_RULE("WF-Dynamic-Err");
            return {false, "Superclass-Undefined"};
          }
          SPEC_RULE("WF-Dynamic");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeOpaque>) {
          syntax::Path class_path;
          class_path.reserve(node.class_path.size());
          for (const auto& comp : node.class_path) {
            class_path.push_back(comp);
          }
          if (ctx.sigma.classes.find(PathKeyOf(class_path)) == ctx.sigma.classes.end()) {
            SPEC_RULE("WF-Opaque-Err");
            return {false, "Superclass-Undefined"};
          }
          SPEC_RULE("WF-Opaque");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeRefine>) {
          const auto base = TypeWFImpl(ctx, node.base);
          if (!base.ok) {
            return base;
          }
          if (!node.predicate) {
            return {false, std::nullopt};
          }
          TypeEnv env;
          TypeScope scope;
          scope.emplace(IdKeyOf("self"),
                        TypeBinding{syntax::Mutability::Let, node.base});
          env.scopes.push_back(std::move(scope));
          StmtTypeContext type_ctx;
          type_ctx.return_type = MakeTypePrim("bool");
          const auto pred_type = TypeExpr(ctx, type_ctx, node.predicate, env);
          if (!pred_type.ok) {
            return {false, pred_type.diag_id};
          }
          if (!IsPrimType(pred_type.type, "bool")) {
            return {false, "E-TYP-1955"};
          }
          const auto purity = CheckPurity(node.predicate);
          if (!purity.ok) {
            return {false, std::optional<std::string_view>{"E-TYP-1954"}};
          }
          SPEC_RULE("WF-Refine-Type");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeString>) {
          SPEC_RULE("WF-String");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeBytes>) {
          SPEC_RULE("WF-Bytes");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypePtr>) {
          const auto wf = TypeWFImpl(ctx, node.element);
          if (!wf.ok) {
            return wf;
          }
          SPEC_RULE("WF-Ptr");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeRawPtr>) {
          const auto wf = TypeWFImpl(ctx, node.element);
          if (!wf.ok) {
            return wf;
          }
          SPEC_RULE("WF-RawPtr");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeModalState>) {
          const auto* decl = LookupModalDecl(ctx, node.path);
          if (!decl || !HasState(*decl, node.state)) {
            return {};
          }
          SPEC_RULE("WF-ModalState");
          return {true, std::nullopt};
        } else if constexpr (std::is_same_v<T, TypeRange>) {
          return {true, std::nullopt};
        } else {
          return {};
        }
      },
      type->node);
}

}  // namespace

TypeWfResult TypeWF(const ScopeContext& ctx, const TypeRef& type) {
  return TypeWFImpl(ctx, type);
}

}  // namespace cursive0::analysis

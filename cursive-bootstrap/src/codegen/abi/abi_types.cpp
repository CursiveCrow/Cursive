// ABI Type Lowering implementation (§6.2.2)
// Implements the ABITy judgment which maps semantic types to ABI types.

#include "cursive0/codegen/abi/abi.h"

#include <type_traits>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/generics/monomorphize.h"
#include "cursive0/analysis/resolve/scopes.h"

namespace cursive0::codegen {
namespace {

// Helper to resolve type alias bodies
std::optional<analysis::TypeRef> ResolveAliasBody(
    const analysis::ScopeContext& ctx,
    const analysis::TypePath& path) {
  syntax::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(analysis::PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return std::nullopt;
  }
  const auto* alias = std::get_if<syntax::TypeAliasDecl>(&it->second);
  if (!alias) {
    return std::nullopt;
  }
  return LowerTypeForLayout(ctx, alias->type);
}

// Check if a TypePathType resolves to a record declaration
bool IsRecordDecl(const analysis::ScopeContext& ctx, const analysis::TypePath& path) {
  syntax::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(analysis::PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return false;
  }
  return std::holds_alternative<syntax::RecordDecl>(it->second);
}

// Check if a TypePathType resolves to an enum declaration
bool IsEnumDecl(const analysis::ScopeContext& ctx, const analysis::TypePath& path) {
  syntax::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(analysis::PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return false;
  }
  return std::holds_alternative<syntax::EnumDecl>(it->second);
}

// Check if a TypePathType resolves to a modal declaration
bool IsModalDecl(const analysis::ScopeContext& ctx, const analysis::TypePath& path) {
  syntax::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(analysis::PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return false;
  }
  return std::holds_alternative<syntax::ModalDecl>(it->second);
}

// Check if a TypePathType resolves to a type alias declaration
bool IsAliasDecl(const analysis::ScopeContext& ctx, const analysis::TypePath& path) {
  syntax::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(analysis::PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return false;
  }
  return std::holds_alternative<syntax::TypeAliasDecl>(it->second);
}

}  // namespace

std::optional<ABIType> ABITy(const analysis::ScopeContext& ctx,
                            const analysis::TypeRef& type) {
  if (!type) {
    return std::nullopt;
  }

  // (ABI-Perm)
  // Strip permission wrapper - ABI type is the same as the base type.
  // ABITy(TypePerm(p, T)) ⇓ τ  when  ABITy(T) ⇓ τ
  if (const auto* perm = std::get_if<analysis::TypePerm>(&type->node)) {
    SPEC_RULE("ABI-Perm");
    return ABITy(ctx, perm->base);
  }

  // (ABI-Prim)
  // Primitive types use PrimSize/PrimAlign.
  // ABITy(TypePrim(name)) ⇓ <s, a>  when sizeof(TypePrim(name)) = s ∧ alignof(TypePrim(name)) = a
  if (const auto* prim = std::get_if<analysis::TypePrim>(&type->node)) {
    SPEC_RULE("ABI-Prim");
    const auto size = PrimSize(prim->name);
    const auto align = PrimAlign(prim->name);
    if (!size.has_value() || !align.has_value()) {
      return std::nullopt;
    }
    return ABIType{*size, *align};
  }

  // (ABI-Ptr)
  // ABITy(TypePtr(U, s)) ⇓ <PtrSize, PtrAlign>
  if (std::holds_alternative<analysis::TypePtr>(type->node)) {
    SPEC_RULE("ABI-Ptr");
    return ABIType{kPtrSize, kPtrAlign};
  }

  // (ABI-RawPtr)
  // ABITy(TypeRawPtr(q, U)) ⇓ <PtrSize, PtrAlign>
  if (std::holds_alternative<analysis::TypeRawPtr>(type->node)) {
    SPEC_RULE("ABI-RawPtr");
    return ABIType{kPtrSize, kPtrAlign};
  }

  // (ABI-Func)
  // ABITy(TypeFunc(params, R)) ⇓ <PtrSize, PtrAlign>
  if (std::holds_alternative<analysis::TypeFunc>(type->node)) {
    SPEC_RULE("ABI-Func");
    return ABIType{kPtrSize, kPtrAlign};
  }

  // (ABI-Slice)
  // ABITy(TypeSlice(U)) ⇓ <2 × PtrSize, PtrAlign>
  if (std::holds_alternative<analysis::TypeSlice>(type->node)) {
    SPEC_RULE("ABI-Slice");
    return ABIType{2 * kPtrSize, kPtrAlign};
  }

  // (ABI-Dynamic)
  // ABITy(TypeDynamic(Cl)) ⇓ <size, align>  via DynLayout(Cl)
  if (std::holds_alternative<analysis::TypeDynamic>(type->node)) {
    SPEC_RULE("ABI-Dynamic");
    const auto dyn_layout = DynLayoutOf();
    return dyn_layout.layout;
  }

  // (ABI-Range)
  // ABITy(TypeRange) ⇓ <size, align>  via sizeof/alignof
  if (std::holds_alternative<analysis::TypeRange>(type->node)) {
    SPEC_RULE("ABI-Range");
    const auto size = SizeOf(ctx, type);
    const auto align = AlignOf(ctx, type);
    if (!size.has_value() || !align.has_value()) {
      return std::nullopt;
    }
    return ABIType{*size, *align};
  }

  // (ABI-Tuple)
  // ABITy(TypeTuple([T₁,...,Tₙ])) ⇓ <size, align>  via TupleLayout
  if (const auto* tuple = std::get_if<analysis::TypeTuple>(&type->node)) {
    SPEC_RULE("ABI-Tuple");
    const auto layout = RecordLayoutOf(ctx, tuple->elements);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    return layout->layout;
  }

  // (ABI-Array)
  // ABITy(TypeArray(T, e)) ⇓ <size, align>  via sizeof/alignof
  if (std::holds_alternative<analysis::TypeArray>(type->node)) {
    SPEC_RULE("ABI-Array");
    const auto size = SizeOf(ctx, type);
    const auto align = AlignOf(ctx, type);
    if (!size.has_value() || !align.has_value()) {
      return std::nullopt;
    }
    return ABIType{*size, *align};
  }

  // (ABI-Union)
  // ABITy(TypeUnion([T₁,...,Tₙ])) ⇓ <size, align>  via UnionLayout
  if (const auto* uni = std::get_if<analysis::TypeUnion>(&type->node)) {
    SPEC_RULE("ABI-Union");
    const auto layout = UnionLayoutOf(ctx, *uni);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    return layout->layout;
  }

  // (ABI-StringBytes)
  // ABITy(T) ⇓ <size, align>  for T ∈ {TypeString(@View), TypeString(@Managed), TypeBytes(@View), TypeBytes(@Managed)}
  if (const auto* str = std::get_if<analysis::TypeString>(&type->node)) {
    SPEC_RULE("ABI-StringBytes");
    const auto size = SizeOf(ctx, type);
    const auto align = AlignOf(ctx, type);
    if (!size.has_value() || !align.has_value()) {
      return std::nullopt;
    }
    return ABIType{*size, *align};
  }
  if (const auto* bytes = std::get_if<analysis::TypeBytes>(&type->node)) {
    SPEC_RULE("ABI-StringBytes");
    const auto size = SizeOf(ctx, type);
    const auto align = AlignOf(ctx, type);
    if (!size.has_value() || !align.has_value()) {
      return std::nullopt;
    }
    return ABIType{*size, *align};
  }

  // (ABI-Modal) - for TypeModalState
  // ABITy(T) ⇓ <size, align>  when modal state type, via ModalLayout
  if (std::holds_alternative<analysis::TypeModalState>(type->node)) {
    SPEC_RULE("ABI-Modal");
    const auto size = SizeOf(ctx, type);
    const auto align = AlignOf(ctx, type);
    if (!size.has_value() || !align.has_value()) {
      return std::nullopt;
    }
    return ABIType{*size, *align};
  }

  // Handle TypePathType - may be Record, Enum, Modal, or Alias
  if (const auto* path_type = std::get_if<analysis::TypePathType>(&type->node)) {
    // (ABI-Alias)
    // ABITy(TypePath(p)) ⇓ τ  when  AliasBody(p) = ty ∧ ABITy(ty) ⇓ τ
    if (IsAliasDecl(ctx, path_type->path)) {
      SPEC_RULE("ABI-Alias");
      const auto resolved = ResolveAliasBody(ctx, path_type->path);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      analysis::TypeRef inst = *resolved;
      // Apply generic substitution when alias has params and path has args.
      syntax::Path syntax_path;
      syntax_path.reserve(path_type->path.size());
      for (const auto& comp : path_type->path) {
        syntax_path.push_back(comp);
      }
      const auto it = ctx.sigma.types.find(analysis::PathKeyOf(syntax_path));
      if (it != ctx.sigma.types.end()) {
        if (const auto* alias = std::get_if<syntax::TypeAliasDecl>(&it->second)) {
          if (alias->generic_params &&
              !alias->generic_params->params.empty() &&
              !path_type->generic_args.empty()) {
            analysis::TypeSubst subst =
                analysis::BuildSubstitution(alias->generic_params->params,
                                            path_type->generic_args);
            inst = analysis::InstantiateType(inst, subst);
          }
        }
      }
      return ABITy(ctx, inst);
    }

    // (ABI-Record)
    // ABITy(TypePath(p)) ⇓ <size, align>  when  RecordDecl(p) = R ∧ RecordLayout(fields) ⇓ <size, align, _>
    if (IsRecordDecl(ctx, path_type->path)) {
      SPEC_RULE("ABI-Record");
      const auto layout = LayoutOf(ctx, type);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      return *layout;
    }

    // (ABI-Enum)
    // ABITy(TypePath(p)) ⇓ <size, align>  when  EnumDecl(p) = E ∧ EnumLayout(E) ⇓ <size, align, _, _>
    if (IsEnumDecl(ctx, path_type->path)) {
      SPEC_RULE("ABI-Enum");
      const auto layout = LayoutOf(ctx, type);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      return *layout;
    }

    // (ABI-Modal) - for TypePath to modal declaration
    // ABITy(TypePath(p)) ⇓ <size, align>  when  Σ.Types[p] = modal M ∧ ModalLayout(M) ⇓ <size, align, _, _>
    if (IsModalDecl(ctx, path_type->path)) {
      SPEC_RULE("ABI-Modal");
      const auto layout = LayoutOf(ctx, type);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      return *layout;
    }
  }

  return std::nullopt;
}

}  // namespace cursive0::codegen

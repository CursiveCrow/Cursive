// =================================================================
// File: 03_analysis/types/type_predicates.cpp
// Construct: Type Predicates (Bitcopy, Clone, Eq, Ord, Cast)
// Spec Section: 5.2.12
// Spec Rules: BitcopyType, CloneType, EqType, OrdType, CastValid
// =================================================================
#include "cursive0/03_analysis/types/type_predicates.h"

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/composite/classes.h"
#include "cursive0/03_analysis/generics/monomorphize.h"
#include "cursive0/03_analysis/modal/modal.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/expr/common.h"
#include "cursive0/03_analysis/types/type_lower.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsPredicates() {
  SPEC_DEF("BitcopyType", "5.2.12");
  SPEC_DEF("CloneType", "5.2.12");
  SPEC_DEF("EqType", "5.2.12");
  SPEC_DEF("OrdType", "5.2.12");
  SPEC_DEF("CastValid", "5.2.12");
}

static bool BuiltinBitcopyType(const TypeRef& type) {
  if (!type) {
    return false;
  }
  return std::visit(
      [](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, TypePrim>) {
          return expr::IsPrimTypeName(node.name);
        } else if constexpr (std::is_same_v<T, TypePtr> ||
                             std::is_same_v<T, TypeRawPtr> ||
                             std::is_same_v<T, TypeSlice> ||
                             std::is_same_v<T, TypeFunc> ||
                             std::is_same_v<T, TypeDynamic> ||
                             std::is_same_v<T, TypeRange>) {
          return true;
        } else if constexpr (std::is_same_v<T, TypeString>) {
          return node.state.has_value() &&
                 *node.state == StringState::View;
        } else if constexpr (std::is_same_v<T, TypeBytes>) {
          return node.state.has_value() &&
                 *node.state == BytesState::View;
        } else {
          return false;
        }
      },
      type->node);
}

static bool IsBitcopyClassPath(const syntax::ClassPath& path) {
  return !path.empty() && IdEq(path.back(), "Bitcopy");
}

static bool ImplementsBitcopy(const ScopeContext& ctx, const TypeRef& type) {
  const auto* path_type = std::get_if<TypePathType>(&type->node);
  if (!path_type) {
    return false;
  }

  syntax::Path syntax_path;
  syntax_path.reserve(path_type->path.size());
  for (const auto& comp : path_type->path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return false;
  }

  auto has_bitcopy = [](const std::vector<syntax::ClassPath>& impls) {
    for (const auto& impl : impls) {
      if (IsBitcopyClassPath(impl)) {
        return true;
      }
    }
    return false;
  };

  if (const auto* record = std::get_if<syntax::RecordDecl>(&it->second)) {
    return has_bitcopy(record->implements);
  }
  if (const auto* enum_decl = std::get_if<syntax::EnumDecl>(&it->second)) {
    return has_bitcopy(enum_decl->implements);
  }
  if (const auto* modal_decl = std::get_if<syntax::ModalDecl>(&it->second)) {
    return has_bitcopy(modal_decl->implements);
  }
  return false;
}

}  // namespace

TypeRef StripPerm(const TypeRef& type) {
  SpecDefsPredicates();
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return StripPerm(perm->base);
  }
  if (const auto* refine = std::get_if<TypeRefine>(&type->node)) {
    return StripPerm(refine->base);
  }
  return type;
}

bool BitcopyType(const ScopeContext& ctx, const TypeRef& type) {
  SpecDefsPredicates();
  if (!type) {
    return false;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    if (perm->perm == Permission::Unique) {
      return false;
    }
    return BitcopyType(ctx, perm->base);
  }
  if (const auto* refine = std::get_if<TypeRefine>(&type->node)) {
    return BitcopyType(ctx, refine->base);
  }
  if (const auto* opaque = std::get_if<TypeOpaque>(&type->node)) {
    if (opaque->origin) {
      const auto it = ctx.sigma.opaque_underlying.find(opaque->origin);
      if (it != ctx.sigma.opaque_underlying.end()) {
        return BitcopyType(ctx, it->second);
      }
    }
    return false;
  }
  if (const auto* tuple = std::get_if<TypeTuple>(&type->node)) {
    for (const auto& elem : tuple->elements) {
      if (!BitcopyType(ctx, elem)) {
        return false;
      }
    }
    return true;
  }
  if (const auto* array = std::get_if<TypeArray>(&type->node)) {
    return BitcopyType(ctx, array->element);
  }
  // §12877: Union types are Bitcopy if all member types are Bitcopy
  if (const auto* union_type = std::get_if<TypeUnion>(&type->node)) {
    for (const auto& member : union_type->members) {
      if (!BitcopyType(ctx, member)) {
        return false;
      }
    }
    return true;
  }
  // §12884-12887: Structural Bitcopy derivation for Records, Enums, Modals, Type Aliases
  if (const auto* path_type = std::get_if<TypePathType>(&type->node)) {
    syntax::Path syntax_path;
    syntax_path.reserve(path_type->path.size());
    for (const auto& comp : path_type->path) {
      syntax_path.push_back(comp);
    }
    const auto it = ctx.sigma.types.find(PathKeyOf(syntax_path));
    if (it != ctx.sigma.types.end()) {
      // Type aliases: recurse through to the underlying type
      // §13.1: Generic type aliases require substitution of type arguments
      if (const auto* alias = std::get_if<syntax::TypeAliasDecl>(&it->second)) {
        const auto lowered = LowerType(ctx, alias->type);
        if (lowered.ok && lowered.type) {
          // Check if alias has generic params and we have generic args
          if (alias->generic_params && !alias->generic_params->params.empty() &&
              !path_type->generic_args.empty()) {
            // Build substitution mapping type params to type args
            TypeSubst subst = BuildSubstitution(alias->generic_params->params,
                                                path_type->generic_args);
            // Apply substitution to get the instantiated type
            TypeRef instantiated = InstantiateType(lowered.type, subst);
            return BitcopyType(ctx, instantiated);
          }
          return BitcopyType(ctx, lowered.type);
        }
      }
      // §12884: Records are Bitcopy if all fields have Bitcopy types
      if (const auto* record = std::get_if<syntax::RecordDecl>(&it->second)) {
        for (const auto& member : record->members) {
          if (const auto* field = std::get_if<syntax::FieldDecl>(&member)) {
            const auto lowered = LowerType(ctx, field->type);
            if (!lowered.ok || !lowered.type || !BitcopyType(ctx, lowered.type)) {
              return ImplementsBitcopy(ctx, type);
            }
          }
        }
        return true;
      }
      // §12885: Enums are Bitcopy if all variant payload types are Bitcopy
      if (const auto* enum_decl = std::get_if<syntax::EnumDecl>(&it->second)) {
        for (const auto& variant : enum_decl->variants) {
          if (!variant.payload_opt.has_value()) {
            continue;
          }
          const bool payload_ok = std::visit(
              [&](const auto& payload) {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::VariantPayloadTuple>) {
                  for (const auto& elem : payload.elements) {
                    if (!elem) continue;
                    const auto lowered = LowerType(ctx, elem);
                    if (!lowered.ok || !lowered.type ||
                        !BitcopyType(ctx, lowered.type)) {
                      return false;
                    }
                  }
                } else {
                  for (const auto& field : payload.fields) {
                    const auto lowered = LowerType(ctx, field.type);
                    if (!lowered.ok || !lowered.type ||
                        !BitcopyType(ctx, lowered.type)) {
                      return false;
                    }
                  }
                }
                return true;
              },
              *variant.payload_opt);
          if (!payload_ok) {
            return ImplementsBitcopy(ctx, type);
          }
        }
        return true;
      }
      // §12887: Modal general types are Bitcopy if all states' payloads are Bitcopy
      // §13.1: Generic modals require substitution of type arguments
      if (const auto* modal_decl = std::get_if<syntax::ModalDecl>(&it->second)) {
        // Build substitution if modal has generic params and we have args
        TypeSubst subst;
        if (modal_decl->generic_params && !modal_decl->generic_params->params.empty() &&
            !path_type->generic_args.empty()) {
          subst = BuildSubstitution(modal_decl->generic_params->params,
                                    path_type->generic_args);
        }
        for (const auto& state : modal_decl->states) {
          for (const auto& member : state.members) {
            if (const auto* field = std::get_if<syntax::StateFieldDecl>(&member)) {
              const auto lowered = LowerType(ctx, field->type);
              if (!lowered.ok || !lowered.type) {
                return ImplementsBitcopy(ctx, type);
              }
              // Apply substitution if we have one
              TypeRef field_type = lowered.type;
              if (!subst.empty()) {
                field_type = InstantiateType(lowered.type, subst);
              }
              if (!BitcopyType(ctx, field_type)) {
                return ImplementsBitcopy(ctx, type);
              }
            }
          }
        }
        return true;
      }
    }
  }
  // §12886: Modal state types are Bitcopy if all payload fields for that state are Bitcopy
  if (const auto* modal_state = std::get_if<TypeModalState>(&type->node)) {
    const auto* decl = LookupModalDecl(ctx, modal_state->path);
    if (decl) {
      const auto* state_block = LookupModalState(*decl, modal_state->state);
      if (state_block) {
        for (const auto& member : state_block->members) {
          if (const auto* field = std::get_if<syntax::StateFieldDecl>(&member)) {
            const auto lowered = LowerType(ctx, field->type);
            if (!lowered.ok || !lowered.type || !BitcopyType(ctx, lowered.type)) {
              return false;
            }
          }
        }
        return true;
      }
    }
  }
  return BuiltinBitcopyType(type) || ImplementsBitcopy(ctx, type);
}

bool CloneType(const ScopeContext& ctx, const TypeRef& type) {
  SpecDefsPredicates();
  if (!type) {
    return false;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return CloneType(ctx, perm->base);
  }
  if (const auto* refine = std::get_if<TypeRefine>(&type->node)) {
    return CloneType(ctx, refine->base);
  }
  if (const auto* opaque = std::get_if<TypeOpaque>(&type->node)) {
    if (opaque->origin) {
      const auto it = ctx.sigma.opaque_underlying.find(opaque->origin);
      if (it != ctx.sigma.opaque_underlying.end()) {
        return CloneType(ctx, it->second);
      }
    }
    return false;
  }
  if (BitcopyType(ctx, type)) {
    return true;
  }
  const syntax::ClassPath clone_path = {"Clone"};
  return TypeImplementsClass(ctx, type, clone_path);
}

bool EqType(const TypeRef& type) {
  SpecDefsPredicates();
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }

  if (const auto* prim = std::get_if<TypePrim>(&stripped->node)) {
    if (expr::IsNumericType(prim->name) || prim->name == "bool" ||
        prim->name == "char") {
      return true;
    }
  }

  if (std::holds_alternative<TypePtr>(stripped->node)) {
    return true;
  }

  if (std::holds_alternative<TypeRawPtr>(stripped->node)) {
    return true;
  }

  if (std::holds_alternative<TypeString>(stripped->node)) {
    return true;
  }

  if (std::holds_alternative<TypeBytes>(stripped->node)) {
    return true;
  }

  return false;
}

bool OrdType(const TypeRef& type) {
  SpecDefsPredicates();
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }

  const auto* prim = std::get_if<TypePrim>(&stripped->node);
  if (!prim) {
    return false;
  }

  return expr::IsIntType(prim->name) || expr::IsFloatType(prim->name) ||
         prim->name == "char";
}

bool CastValid(const TypeRef& source, const TypeRef& target) {
  SpecDefsPredicates();
  const auto src = StripPerm(source);
  const auto tgt = StripPerm(target);

  if (!src || !tgt) {
    return false;
  }

  const auto* src_prim = std::get_if<TypePrim>(&src->node);
  const auto* tgt_prim = std::get_if<TypePrim>(&tgt->node);

  if (!src_prim || !tgt_prim) {
    return false;
  }

  // Numeric to numeric
  if (expr::IsNumericType(src_prim->name) && expr::IsNumericType(tgt_prim->name)) {
    return true;
  }

  // Bool to int
  if (src_prim->name == "bool" && expr::IsIntType(tgt_prim->name)) {
    return true;
  }

  // Int to bool
  if (expr::IsIntType(src_prim->name) && tgt_prim->name == "bool") {
    return true;
  }

  // Char to u32
  if (src_prim->name == "char" && tgt_prim->name == "u32") {
    return true;
  }

  // U32 to char
  if (src_prim->name == "u32" && tgt_prim->name == "char") {
    return true;
  }

  return false;
}

}  // namespace cursive0::analysis

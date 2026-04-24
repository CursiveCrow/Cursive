// =============================================================================
// MIGRATION MAPPING: type_equiv.cpp
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   Section 5.2.1: Type Equivalence (lines 8514-8592)
//   - T-Equiv-Prim (lines 8514-8517): Primitive type equivalence
//   - T-Equiv-Perm (lines 8519-8522): Permission type equivalence
//   - T-Equiv-Tuple (lines 8524-8527): Tuple type equivalence
//   - T-Equiv-Array (lines 8529-8532): Array type equivalence
//   - T-Equiv-Slice (lines 8534-8537): Slice type equivalence
//   - T-Equiv-Func (lines 8539-8542): Function type equivalence
//   - T-Equiv-Closure (lines 8544-8547): Closure type equivalence
//   - T-Equiv-Union (lines 8549-8552): Union type equivalence (unordered)
//   - T-Equiv-Path (lines 8554-8557): Named type equivalence
//   - T-Equiv-ModalState (lines 8559-8562): Modal state equivalence
//   - T-Equiv-String (lines 8564-8567): String type equivalence
//   - T-Equiv-Bytes (lines 8569-8572): Bytes type equivalence
//   - T-Equiv-Range (lines 8574-8577): Range type equivalence
//   - T-Equiv-Ptr (lines 8579-8582): Safe pointer equivalence
//   - T-Equiv-RawPtr (lines 8584-8587): Raw pointer equivalence
//   - T-Equiv-Dynamic (lines 8589-8592): Dynamic class equivalence
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_equiv.cpp
//   Lines 1-602: Full implementation
//   Key functions:
//   - TypeEquiv(Env, Type, Type) -> bool (main entry)
//   - TypeEquivImpl(Env, Type, Type) -> bool (recursive comparison)
//   - MembersEq() for union member comparison
//   - TypeKey computation for union normalization
//
// KEY CONTENT TO MIGRATE:
//   TYPE EQUIVALENCE JUDGMENT (Gamma |- T equiv U):
//   1. Primitive types: same name implies equivalence
//   2. Permission types: same permission and equivalent inner type
//   3. Tuple types: same arity, pairwise equivalent elements
//   4. Array types: same length (via ConstLen), equivalent element type
//   5. Slice types: equivalent element type
//   6. Function types: same modes, pairwise equivalent params, equiv return
//   7. Union types: UNORDERED - MembersEq checks permutation equivalence
//   8. Named types (TypePath): same path implies equivalence
//   9. Modal state types: same modal ref and state name
//   10. String/Bytes: same storage type (View/Managed)
//   11. Ptr types: same state, equivalent pointee
//   12. RawPtr types: same qualifier (imm/mut), equivalent pointee
//   13. Dynamic types: same class path
//
//   UNION NORMALIZATION:
//   - Unions are unordered: i32 | bool equiv bool | i32
//   - MembersEq computes via permutation check
//   - TypeKey provides deterministic ordering for comparisons
//
//   CONSTLEN EVALUATION:
//   - Array lengths compared via constant expression evaluation
//   - See const_len.cpp for ConstLen implementation
//
// DEPENDENCIES:
//   - ConstLen() for array length evaluation
//   - TypeKey for union normalization
//   - Type representation (TypePrim, TypeTuple, TypeArray, etc.)
//   - Env for type parameter lookups
//
// SPEC RULES:
//   - T-Equiv-*: All type equivalence rules
//   - MembersEq predicate for unions (line 8512)
//
// RESULT:
//   bool indicating whether types are equivalent
//
// NOTES:
//   - Type equivalence is reflexive, symmetric, transitive
//   - No subtyping between numeric types (i32 is NOT equiv i64)
//   - Union equivalence requires exact membership (after normalization)
//
// =============================================================================

#include "04_analysis/typing/type_equiv.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "00_core/assert_spec.h"
#include "04_analysis/contracts/verification.h"
#include "04_analysis/resolve/scopes.h"

namespace cursive::analysis {

namespace {

static inline void SpecDefsTypeEquiv() {
  SPEC_DEF("TypeEqJudg", "5.2.1");
  SPEC_DEF("ConstLenJudg", "5.2.1");
  SPEC_DEF("MembersEq", "5.2.1");
}

static bool TypePathEq(const TypePath& lhs, const TypePath& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (IdKeyOf(lhs[i]) != IdKeyOf(rhs[i])) {
      return false;
    }
  }
  return true;
}

static bool SpanEq(const core::Span& lhs, const core::Span& rhs) {
  return lhs.file == rhs.file &&
         lhs.start_offset == rhs.start_offset &&
         lhs.end_offset == rhs.end_offset;
}

static void CollectConjuncts(const ast::ExprPtr& expr,
                             std::vector<ast::ExprPtr>& out) {
  if (!expr) {
    return;
  }
  if (const auto* binary = std::get_if<ast::BinaryExpr>(&expr->node)) {
    if (binary->op == "&&") {
      CollectConjuncts(binary->lhs, out);
      CollectConjuncts(binary->rhs, out);
      return;
    }
  }
  out.push_back(expr);
}

struct RefineUnpack {
  TypeRef base;
  std::vector<ast::ExprPtr> predicates;
};

static RefineUnpack UnpackRefine(const TypeRef& type) {
  RefineUnpack out{type, {}};
  TypeRef cur = type;
  while (cur) {
    const auto* refine = std::get_if<TypeRefine>(&cur->node);
    if (!refine) {
      break;
    }
    CollectConjuncts(refine->predicate, out.predicates);
    cur = refine->base;
    out.base = cur;
  }
  return out;
}

static bool PredicatesImply(const std::vector<ast::ExprPtr>& premises,
                            const std::vector<ast::ExprPtr>& goals) {
  if (goals.empty()) {
    return true;
  }
  StaticProofContext proof_ctx;
  const core::Span default_span = goals.front() ? goals.front()->span : core::Span{};
  for (const auto& premise : premises) {
    if (!premise) {
      continue;
    }
    AddFact(proof_ctx, premise, default_span);
  }
  for (const auto& goal : goals) {
    if (!goal) {
      return false;
    }
    const auto proof = StaticProof(proof_ctx, goal);
    if (!proof.provable) {
      return false;
    }
  }
  return true;
}

static bool PredicateEquiv(const std::vector<ast::ExprPtr>& lhs_preds,
                           const std::vector<ast::ExprPtr>& rhs_preds) {
  if (lhs_preds.empty() && rhs_preds.empty()) {
    return true;
  }
  if (lhs_preds.empty() || rhs_preds.empty()) {
    return false;
  }
  if (lhs_preds.size() == rhs_preds.size()) {
    bool struct_equal = true;
    for (std::size_t i = 0; i < lhs_preds.size(); ++i) {
      if (!ExprStructEqual(lhs_preds[i], rhs_preds[i])) {
        struct_equal = false;
        break;
      }
    }
    if (struct_equal) {
      return true;
    }
  }
  return PredicatesImply(lhs_preds, rhs_preds) &&
         PredicatesImply(rhs_preds, lhs_preds);
}

struct TypePairKey {
  TypeRef lhs;
  TypeRef rhs;

  bool operator==(const TypePairKey& other) const {
    return lhs.get() == other.lhs.get() && rhs.get() == other.rhs.get();
  }
};

struct TypePairKeyHash {
  std::size_t operator()(const TypePairKey& key) const {
    const auto lhs = reinterpret_cast<std::uintptr_t>(key.lhs.get());
    const auto rhs = reinterpret_cast<std::uintptr_t>(key.rhs.get());
    return static_cast<std::size_t>((lhs >> 4U) ^ (rhs << 3U) ^ rhs);
  }
};

static TypePairKey MakeTypePairKey(TypeRef lhs, TypeRef rhs) {
  if (std::less<const Type*>{}(rhs.get(), lhs.get())) {
    std::swap(lhs, rhs);
  }
  return TypePairKey{std::move(lhs), std::move(rhs)};
}

static std::unordered_map<TypePairKey, TypeEquivResult, TypePairKeyHash>&
TypeEquivMemo() {
  thread_local std::unordered_map<TypePairKey, TypeEquivResult, TypePairKeyHash>
      memo;
  return memo;
}

}  // namespace

TypeEquivResult TypeEquiv(const TypeRef& lhs, const TypeRef& rhs) {
  SpecDefsTypeEquiv();
  if (!lhs || !rhs) {
    return {true, std::nullopt, false};
  }
  if (lhs.get() == rhs.get()) {
    return {true, std::nullopt, true};
  }
  const TypePairKey memo_key = MakeTypePairKey(lhs, rhs);
  auto& memo = TypeEquivMemo();
  if (const auto it = memo.find(memo_key); it != memo.end()) {
    return it->second;
  }

  TypeEquivResult result = std::visit(
      [&](const auto& node) -> TypeEquivResult {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, TypePrim>) {
          const auto* other = std::get_if<TypePrim>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Prim");
          return {true, std::nullopt, node.name == other->name};
        } else if constexpr (std::is_same_v<T, TypeVar>) {
          const auto* other = std::get_if<TypeVar>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          return {true, std::nullopt, node.id == other->id};
        } else if constexpr (std::is_same_v<T, TypePerm>) {
          const auto* other = std::get_if<TypePerm>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Perm");
          if (node.perm != other->perm) {
            return {true, std::nullopt, false};
          }
          return TypeEquiv(node.base, other->base);
        } else if constexpr (std::is_same_v<T, TypeTuple>) {
          const auto* other = std::get_if<TypeTuple>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Tuple");
          if (node.elements.size() != other->elements.size()) {
            return {true, std::nullopt, false};
          }
          for (std::size_t i = 0; i < node.elements.size(); ++i) {
            const auto res = TypeEquiv(node.elements[i], other->elements[i]);
            if (!res.ok || !res.equiv) {
              return res.ok ? TypeEquivResult{true, std::nullopt, false} : res;
            }
          }
          return {true, std::nullopt, true};
        } else if constexpr (std::is_same_v<T, TypeArray>) {
          const auto* other = std::get_if<TypeArray>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Array");
          if (node.length != other->length) {
            return {true, std::nullopt, false};
          }
          return TypeEquiv(node.element, other->element);
        } else if constexpr (std::is_same_v<T, TypeSlice>) {
          const auto* other = std::get_if<TypeSlice>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Slice");
          return TypeEquiv(node.element, other->element);
        } else if constexpr (std::is_same_v<T, TypeFunc>) {
          const auto* other = std::get_if<TypeFunc>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Func");
          if (node.params.size() != other->params.size()) {
            return {true, std::nullopt, false};
          }
          for (std::size_t i = 0; i < node.params.size(); ++i) {
            const auto& lp = node.params[i];
            const auto& rp = other->params[i];
            if (lp.mode != rp.mode) {
              return {true, std::nullopt, false};
            }
            const auto res = TypeEquiv(lp.type, rp.type);
            if (!res.ok || !res.equiv) {
              return res.ok ? TypeEquivResult{true, std::nullopt, false} : res;
            }
          }
          return TypeEquiv(node.ret, other->ret);
        } else if constexpr (std::is_same_v<T, TypeClosure>) {
          const auto* other = std::get_if<TypeClosure>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Closure");
          if (node.params.size() != other->params.size()) {
            return {true, std::nullopt, false};
          }
          for (std::size_t i = 0; i < node.params.size(); ++i) {
            const auto& lp = node.params[i];
            const auto& rp = other->params[i];
            if (lp.first != rp.first) {
              return {true, std::nullopt, false};
            }
            const auto res = TypeEquiv(lp.second, rp.second);
            if (!res.ok || !res.equiv) {
              return res.ok ? TypeEquivResult{true, std::nullopt, false} : res;
            }
          }
          if (node.deps_opt.has_value() != other->deps_opt.has_value()) {
            return {true, std::nullopt, false};
          }
          if (node.deps_opt.has_value()) {
            if (node.deps_opt->size() != other->deps_opt->size()) {
              return {true, std::nullopt, false};
            }
            for (std::size_t i = 0; i < node.deps_opt->size(); ++i) {
              const auto& ld = (*node.deps_opt)[i];
              const auto& rd = (*other->deps_opt)[i];
              if (ld.name != rd.name) {
                return {true, std::nullopt, false};
              }
              const auto res = TypeEquiv(ld.type, rd.type);
              if (!res.ok || !res.equiv) {
                return res.ok ? TypeEquivResult{true, std::nullopt, false} : res;
              }
            }
          }
          return TypeEquiv(node.ret, other->ret);
        } else if constexpr (std::is_same_v<T, TypeUnion>) {
          const auto* other = std::get_if<TypeUnion>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Union");
          const auto lhs_sorted = SortUnionMembers(node.members);
          const auto rhs_sorted = SortUnionMembers(other->members);
          if (lhs_sorted.size() != rhs_sorted.size()) {
            return {true, std::nullopt, false};
          }
          for (std::size_t i = 0; i < lhs_sorted.size(); ++i) {
            const auto res = TypeEquiv(lhs_sorted[i], rhs_sorted[i]);
            if (!res.ok || !res.equiv) {
              return res.ok ? TypeEquivResult{true, std::nullopt, false} : res;
            }
          }
          return {true, std::nullopt, true};
          } else if constexpr (std::is_same_v<T, TypePathType>) {
            const auto* other_path = AppliedTypePath(*rhs);
            const auto* other_args = AppliedTypeArgs(*rhs);
            if (!other_path || !other_args) {
              return {true, std::nullopt, false};
            }
            SPEC_RULE("T-Equiv-Path");
            if (!TypePathEq(node.path, *other_path)) {
              return {true, std::nullopt, false};
            }
            if (node.generic_args.size() != other_args->size()) {
              return {true, std::nullopt, false};
            }
            for (std::size_t i = 0; i < node.generic_args.size(); ++i) {
              const auto res = TypeEquiv(node.generic_args[i], (*other_args)[i]);
              if (!res.ok || !res.equiv) {
                return res.ok ? TypeEquivResult{true, std::nullopt, false} : res;
              }
            }
            return {true, std::nullopt, true};
          } else if constexpr (std::is_same_v<T, TypeApply>) {
            const auto* other_path = AppliedTypePath(*rhs);
            const auto* other_args = AppliedTypeArgs(*rhs);
            if (!other_path || !other_args) {
              return {true, std::nullopt, false};
            }
            SPEC_RULE("T-Equiv-Apply");
            if (!TypePathEq(node.path, *other_path)) {
              return {true, std::nullopt, false};
            }
            if (node.args.size() != other_args->size()) {
              return {true, std::nullopt, false};
            }
            for (std::size_t i = 0; i < node.args.size(); ++i) {
              const auto res = TypeEquiv(node.args[i], (*other_args)[i]);
              if (!res.ok || !res.equiv) {
                return res.ok ? TypeEquivResult{true, std::nullopt, false} : res;
              }
            }
            return {true, std::nullopt, true};
          } else if constexpr (std::is_same_v<T, TypeModalState>) {
          const auto* other = std::get_if<TypeModalState>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-ModalState");
          if (!TypePathEq(node.path, other->path) || node.state != other->state) {
            return {true, std::nullopt, false};
          }
          if (node.generic_args.size() != other->generic_args.size()) {
            return {true, std::nullopt, false};
          }
          for (std::size_t i = 0; i < node.generic_args.size(); ++i) {
            const auto res = TypeEquiv(node.generic_args[i], other->generic_args[i]);
            if (!res.ok || !res.equiv) {
              return res.ok ? TypeEquivResult{true, std::nullopt, false} : res;
            }
          }
          return {true, std::nullopt, true};
        } else if constexpr (std::is_same_v<T, TypeString>) {
          const auto* other = std::get_if<TypeString>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-String");
          return {true, std::nullopt, node.state == other->state};
        } else if constexpr (std::is_same_v<T, TypeBytes>) {
          const auto* other = std::get_if<TypeBytes>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Bytes");
          return {true, std::nullopt, node.state == other->state};
        } else if constexpr (std::is_same_v<T, TypeRange>) {
          const auto* other = std::get_if<TypeRange>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Range");
          return TypeEquiv(node.base, other->base);
        } else if constexpr (std::is_same_v<T, TypeRangeInclusive>) {
          const auto* other = std::get_if<TypeRangeInclusive>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-RangeInclusive");
          return TypeEquiv(node.base, other->base);
        } else if constexpr (std::is_same_v<T, TypeRangeFrom>) {
          const auto* other = std::get_if<TypeRangeFrom>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-RangeFrom");
          return TypeEquiv(node.base, other->base);
        } else if constexpr (std::is_same_v<T, TypeRangeTo>) {
          const auto* other = std::get_if<TypeRangeTo>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-RangeTo");
          return TypeEquiv(node.base, other->base);
        } else if constexpr (std::is_same_v<T, TypeRangeToInclusive>) {
          const auto* other = std::get_if<TypeRangeToInclusive>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-RangeToInclusive");
          return TypeEquiv(node.base, other->base);
        } else if constexpr (std::is_same_v<T, TypeRangeFull>) {
          const auto* other = std::get_if<TypeRangeFull>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-RangeFull");
          return {true, std::nullopt, true};
        } else if constexpr (std::is_same_v<T, TypePtr>) {
          const auto* other = std::get_if<TypePtr>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Ptr");
          if (node.state != other->state) {
            return {true, std::nullopt, false};
          }
          return TypeEquiv(node.element, other->element);
        } else if constexpr (std::is_same_v<T, TypeRawPtr>) {
          const auto* other = std::get_if<TypeRawPtr>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-RawPtr");
          if (node.qual != other->qual) {
            return {true, std::nullopt, false};
          }
          return TypeEquiv(node.element, other->element);
        } else if constexpr (std::is_same_v<T, TypeDynamic>) {
          const auto* other = std::get_if<TypeDynamic>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Dynamic");
          return {true, std::nullopt, TypePathEq(node.path, other->path)};
        } else if constexpr (std::is_same_v<T, TypeOpaque>) {
          const auto* other = std::get_if<TypeOpaque>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          SPEC_RULE("T-Equiv-Opaque");
          if (!TypePathEq(node.class_path, other->class_path)) {
            return {true, std::nullopt, false};
          }
          return {true, std::nullopt, SpanEq(node.origin_span, other->origin_span)};
        } else if constexpr (std::is_same_v<T, TypeRefine>) {
          const auto* other = std::get_if<TypeRefine>(&rhs->node);
          if (!other) {
            return {true, std::nullopt, false};
          }
          const auto lhs_refine = UnpackRefine(lhs);
          const auto rhs_refine = UnpackRefine(rhs);
          const auto base = TypeEquiv(lhs_refine.base, rhs_refine.base);
          if (!base.ok || !base.equiv) {
            return base.ok ? TypeEquivResult{true, std::nullopt, false} : base;
          }
          SPEC_RULE("T-Equiv-Refine");
          return {true, std::nullopt,
                  PredicateEquiv(lhs_refine.predicates, rhs_refine.predicates)};
        } else {
          return {true, std::nullopt, false};
        }
      },
      lhs->node);
  memo.emplace(memo_key, result);
  return result;
}

}  // namespace cursive::analysis

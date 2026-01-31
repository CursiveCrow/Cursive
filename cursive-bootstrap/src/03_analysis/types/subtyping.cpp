#include "cursive0/03_analysis/types/subtyping.h"

#include <array>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/modal/modal.h"
#include "cursive0/03_analysis/modal/modal_widen.h"
#include "cursive0/03_analysis/contracts/verification.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/type_equiv.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/03_analysis/caps/cap_concurrency.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsSubtyping() {
  SPEC_DEF("SubtypingJudg", "5.2.2");
  SPEC_DEF("PermSubJudg", "5.2.2");
  SPEC_DEF("Member", "5.2.2");
  SPEC_DEF("NicheCompatible", "5.7");
}

static constexpr std::array<std::string_view, 12> kIntTypes = {
    "i8",   "i16",  "i32",  "i64",  "i128", "u8",
    "u16",  "u32",  "u64",  "u128", "isize", "usize"};

static constexpr std::array<std::string_view, 3> kFloatTypes = {"f16",
                                                                "f32",
                                                                "f64"};

static bool IsIntType(std::string_view name) {
  for (const auto& t : kIntTypes) {
    if (name == t) {
      return true;
    }
  }
  return false;
}

static bool IsFloatType(std::string_view name) {
  for (const auto& t : kFloatTypes) {
    if (name == t) {
      return true;
    }
  }
  return false;
}

static bool IsNumericMismatch(const TypeRef& lhs, const TypeRef& rhs) {
  const auto* lprim = std::get_if<TypePrim>(&lhs->node);
  const auto* rprim = std::get_if<TypePrim>(&rhs->node);
  if (!lprim || !rprim) {
    return false;
  }
  if (IsIntType(lprim->name) && IsIntType(rprim->name) &&
      lprim->name != rprim->name) {
    return true;
  }
  if (IsFloatType(lprim->name) && IsFloatType(rprim->name) &&
      lprim->name != rprim->name) {
    return true;
  }
  return false;
}

static bool IsNeverType(const TypeRef& type) {
  const auto* prim = std::get_if<TypePrim>(&type->node);
  return prim && prim->name == "!";
}

static bool SpanEq(const core::Span& lhs, const core::Span& rhs) {
  return lhs.file == rhs.file &&
         lhs.start_offset == rhs.start_offset &&
         lhs.end_offset == rhs.end_offset;
}

static bool PermSub(Permission lhs, Permission rhs) {
  SpecDefsSubtyping();
  // C0X Extension: Permission lattice with shared
  // Lattice: unique <: shared <: const
  // Reflexivity:
  if (lhs == Permission::Const && rhs == Permission::Const) {
    SPEC_RULE("Perm-Const");
    return true;
  }
  if (lhs == Permission::Unique && rhs == Permission::Unique) {
    SPEC_RULE("Perm-Unique");
    return true;
  }
  if (lhs == Permission::Shared && rhs == Permission::Shared) {
    SPEC_RULE("Perm-Shared");
    return true;
  }
  // Transitivity: unique <: shared <: const
  if (lhs == Permission::Unique && rhs == Permission::Const) {
    SPEC_RULE("Perm-Unique-Const");
    return true;
  }
  if (lhs == Permission::Unique && rhs == Permission::Shared) {
    SPEC_RULE("Perm-Unique-Shared");
    return true;
  }
  if (lhs == Permission::Shared && rhs == Permission::Const) {
    SPEC_RULE("Perm-Shared-Const");
    return true;
  }
  return false;
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

static SubtypingResult Member(const TypeRef& type, const TypeUnion& uni) {
  SpecDefsSubtyping();
  for (const auto& member : uni.members) {
    const auto res = TypeEquiv(type, member);
    if (!res.ok) {
      return {false, res.diag_id, false};
    }
    if (res.equiv) {
      return {true, std::nullopt, true};
    }
  }
  return {true, std::nullopt, false};
}

}  // namespace

SubtypingResult Subtyping(const ScopeContext& ctx,
                          const TypeRef& lhs,
                          const TypeRef& rhs) {
  SpecDefsSubtyping();
  if (!lhs || !rhs) {
    return {true, std::nullopt, false};
  }

  const auto equiv = TypeEquiv(lhs, rhs);
  if (!equiv.ok) {
    return {false, equiv.diag_id, false};
  }
  if (equiv.equiv) {
    return {true, std::nullopt, true};
  }

  if (const auto* lref = std::get_if<TypeRefine>(&lhs->node)) {
    if (const auto* rref = std::get_if<TypeRefine>(&rhs->node)) {
      const auto base_eq = TypeEquiv(lref->base, rref->base);
      if (!base_eq.ok) {
        return {false, base_eq.diag_id, false};
      }
      if (!base_eq.equiv) {
        return {true, std::nullopt, false};
      }
      SPEC_RULE("Sub-Refine");
      if (!lref->predicate || !rref->predicate) {
        return {true, std::nullopt, false};
      }
      StaticProofContext proof_ctx;
      AddFact(proof_ctx, lref->predicate, rref->predicate->span);
      const auto proof = StaticProof(proof_ctx, rref->predicate);
      if (proof.provable) {
        return {true, std::nullopt, true};
      }
      return {true, std::optional<std::string_view>{"E-TYP-1953"}, false};
    }
    SPEC_RULE("Sub-Refine-Elim");
    return Subtyping(ctx, lref->base, rhs);
  }
  if (std::holds_alternative<TypeRefine>(rhs->node)) {
    return {true, std::nullopt, false};
  }

  if (const auto* lopaque = std::get_if<TypeOpaque>(&lhs->node)) {
    const auto* ropaque = std::get_if<TypeOpaque>(&rhs->node);
    if (!ropaque) {
      return {true, std::nullopt, false};
    }
    SPEC_RULE("Sub-Opaque");
    if (SpanEq(lopaque->origin_span, ropaque->origin_span)) {
      return {true, std::nullopt, true};
    }
    return {true, std::optional<std::string_view>{"Opaque-Type-Mismatch"}, false};
  }
  if (std::holds_alternative<TypeOpaque>(rhs->node)) {
    return {true, std::nullopt, false};
  }

  if (IsNumericMismatch(lhs, rhs)) {
    return {true, std::nullopt, false};
  }

  if (IsNeverType(lhs)) {
    SPEC_RULE("Sub-Never");
    return {true, std::nullopt, true};
  }

  const auto lhs_async = AsyncSigOf(ctx, lhs);
  const auto rhs_async = AsyncSigOf(ctx, rhs);
  if (lhs_async.has_value() && rhs_async.has_value()) {
    const auto out_sub = Subtyping(ctx, lhs_async->out, rhs_async->out);
    if (!out_sub.ok) {
      return {false, out_sub.diag_id, false};
    }
    if (!out_sub.subtype) {
      return {true, std::nullopt, false};
    }
    const auto in_sub = Subtyping(ctx, rhs_async->in, lhs_async->in);
    if (!in_sub.ok) {
      return {false, in_sub.diag_id, false};
    }
    if (!in_sub.subtype) {
      return {true, std::nullopt, false};
    }
    const auto res_sub = Subtyping(ctx, lhs_async->result, rhs_async->result);
    if (!res_sub.ok) {
      return {false, res_sub.diag_id, false};
    }
    if (!res_sub.subtype) {
      return {true, std::nullopt, false};
    }
    const auto err_sub = Subtyping(ctx, lhs_async->err, rhs_async->err);
    if (!err_sub.ok) {
      return {false, err_sub.diag_id, false};
    }
    if (!err_sub.subtype) {
      return {true, std::nullopt, false};
    }
    SPEC_RULE("Sub-Async");
    return {true, std::nullopt, true};
  }

  if (const auto* ldyn = std::get_if<TypeDynamic>(&lhs->node)) {
    if (const auto* rdyn = std::get_if<TypeDynamic>(&rhs->node)) {
      if (IsExecutionDomainTypePath(rdyn->path)) {
        if (rdyn->path.size() == 1 && ldyn->path.size() == 1) {
          const auto& lname = ldyn->path[0];
          if (lname == "CpuDomain" || lname == "GpuDomain" || lname == "InlineDomain") {
            return {true, std::nullopt, true};
          }
        }
      }
    }
  }

  if (const auto* lstr = std::get_if<TypeString>(&lhs->node)) {
    if (const auto* rstr = std::get_if<TypeString>(&rhs->node)) {
      if (!rstr->state.has_value() && lstr->state.has_value()) {
        SPEC_RULE("Sub-String-Modal");
        return {true, std::nullopt, true};
      }
    }
  }

  if (const auto* lbytes = std::get_if<TypeBytes>(&lhs->node)) {
    if (const auto* rbytes = std::get_if<TypeBytes>(&rhs->node)) {
      if (!rbytes->state.has_value() && lbytes->state.has_value()) {
        SPEC_RULE("Sub-Bytes-Modal");
        return {true, std::nullopt, true};
      }
    }
  }

  const auto* lperm = std::get_if<TypePerm>(&lhs->node);
  const auto* rperm = std::get_if<TypePerm>(&rhs->node);
  if (lperm || rperm) {
    const auto lhs_perm = lperm ? lperm->perm : Permission::Const;
    const auto rhs_perm = rperm ? rperm->perm : Permission::Const;
    const auto lhs_base = lperm ? lperm->base : lhs;
    const auto rhs_base = rperm ? rperm->base : rhs;
    SPEC_RULE("Sub-Perm");
    if (!PermSub(lhs_perm, rhs_perm)) {
      return {true, std::nullopt, false};
    }
    return Subtyping(ctx, lhs_base, rhs_base);
  }

  if (const auto* lstr = std::get_if<TypeString>(&lhs->node)) {
    if (const auto* rstr = std::get_if<TypeString>(&rhs->node)) {
      if (!rstr->state.has_value() && lstr->state.has_value()) {
        SPEC_RULE("Sub-String-Widen");
        return {true, std::nullopt, true};
      }
    }
  }

  if (const auto* lbytes = std::get_if<TypeBytes>(&lhs->node)) {
    if (const auto* rbytes = std::get_if<TypeBytes>(&rhs->node)) {
      if (!rbytes->state.has_value() && lbytes->state.has_value()) {
        SPEC_RULE("Sub-Bytes-Widen");
        return {true, std::nullopt, true};
      }
    }
  }

  if (const auto* ltuple = std::get_if<TypeTuple>(&lhs->node)) {
    if (const auto* rtuple = std::get_if<TypeTuple>(&rhs->node)) {
      SPEC_RULE("Sub-Tuple");
      if (ltuple->elements.size() != rtuple->elements.size()) {
        return {true, std::nullopt, false};
      }
      for (std::size_t i = 0; i < ltuple->elements.size(); ++i) {
        const auto res = Subtyping(ctx, ltuple->elements[i],
                                   rtuple->elements[i]);
        if (!res.ok || !res.subtype) {
          return res.ok ? SubtypingResult{true, std::nullopt, false} : res;
        }
      }
      return {true, std::nullopt, true};
    }
  }

  if (const auto* larray = std::get_if<TypeArray>(&lhs->node)) {
    if (const auto* rarray = std::get_if<TypeArray>(&rhs->node)) {
      SPEC_RULE("Sub-Array");
      if (larray->length != rarray->length) {
        return {true, std::nullopt, false};
      }
      return Subtyping(ctx, larray->element, rarray->element);
    }
  }

  if (const auto* lslice = std::get_if<TypeSlice>(&lhs->node)) {
    if (const auto* rslice = std::get_if<TypeSlice>(&rhs->node)) {
      SPEC_RULE("Sub-Slice");
      return Subtyping(ctx, lslice->element, rslice->element);
    }
  }

  if (std::holds_alternative<TypeRange>(lhs->node) &&
      std::holds_alternative<TypeRange>(rhs->node)) {
    SPEC_RULE("Sub-Range");
    return {true, std::nullopt, true};
  }

  if (const auto* lptr = std::get_if<TypePtr>(&lhs->node)) {
    if (const auto* rptr = std::get_if<TypePtr>(&rhs->node)) {
      SPEC_RULE("Sub-Ptr-State");
      if (!rptr->state.has_value() && lptr->state.has_value()) {
        const auto state = *lptr->state;
        if (state == PtrState::Valid || state == PtrState::Null) {
          const auto elem_eq = TypeEquiv(lptr->element, rptr->element);
          if (!elem_eq.ok) {
            return {false, elem_eq.diag_id, false};
          }
          return {true, std::nullopt, elem_eq.equiv};
        }
      }
      return {true, std::nullopt, false};
    }
  }

  if (const auto* lmodal = std::get_if<TypeModalState>(&lhs->node)) {
    if (const auto* rmodal = std::get_if<TypeModalState>(&rhs->node)) {
      if (TypePathEq(lmodal->path, rmodal->path) &&
          !IdEq(lmodal->state, rmodal->state)) {
        SPEC_RULE("Modal-Incomparable");
        return {true, std::nullopt, false};
      }
      if (TypePathEq(lmodal->path, rmodal->path) &&
          lmodal->state == rmodal->state) {
        if (lmodal->generic_args.size() != rmodal->generic_args.size()) {
          return {true, std::nullopt, false};
        }
        for (std::size_t i = 0; i < lmodal->generic_args.size(); ++i) {
          const auto res = TypeEquiv(lmodal->generic_args[i], rmodal->generic_args[i]);
          if (!res.ok) {
            return {false, res.diag_id, false};
          }
          if (!res.equiv) {
            return {true, std::nullopt, false};
          }
        }
      }
    }
  }

  if (const auto* lmodal = std::get_if<TypeModalState>(&lhs->node)) {
    if (const auto* rpath = std::get_if<TypePathType>(&rhs->node)) {
      if (!TypePathEq(lmodal->path, rpath->path)) {
        return {true, std::nullopt, false};
      }
      if (lmodal->generic_args.size() != rpath->generic_args.size()) {
        return {true, std::nullopt, false};
      }
      for (std::size_t i = 0; i < lmodal->generic_args.size(); ++i) {
        const auto res = TypeEquiv(lmodal->generic_args[i], rpath->generic_args[i]);
        if (!res.ok) {
          return {false, res.diag_id, false};
        }
        if (!res.equiv) {
          return {true, std::nullopt, false};
        }
      }
      SPEC_RULE("Sub-Modal-Niche");
      if (!NicheCompatible(ctx, lmodal->path, lmodal->state)) {
        return {true, std::nullopt, false};
      }
      return {true, std::nullopt, true};
    }
  }

  if (const auto* lfunc = std::get_if<TypeFunc>(&lhs->node)) {
    if (const auto* rfunc = std::get_if<TypeFunc>(&rhs->node)) {
      SPEC_RULE("Sub-Func");
      if (lfunc->params.size() != rfunc->params.size()) {
        return {true, std::nullopt, false};
      }
      for (std::size_t i = 0; i < lfunc->params.size(); ++i) {
        const auto& lp = lfunc->params[i];
        const auto& rp = rfunc->params[i];
        if (lp.mode != rp.mode) {
          return {true, std::nullopt, false};
        }
        const auto res = Subtyping(ctx, rp.type, lp.type);
        if (!res.ok || !res.subtype) {
          return res.ok ? SubtypingResult{true, std::nullopt, false} : res;
        }
      }
      return Subtyping(ctx, lfunc->ret, rfunc->ret);
    }
  }

  if (const auto* lunion = std::get_if<TypeUnion>(&lhs->node)) {
    if (const auto* runion = std::get_if<TypeUnion>(&rhs->node)) {
      SPEC_RULE("Sub-Union-Width");
      for (const auto& member : lunion->members) {
        const auto res = Member(member, *runion);
        if (!res.ok || !res.subtype) {
          return res.ok ? SubtypingResult{true, std::nullopt, false} : res;
        }
      }
      return {true, std::nullopt, true};
    }
  }

  if (const auto* runion = std::get_if<TypeUnion>(&rhs->node)) {
    SPEC_RULE("Sub-Member-Union");
    return Member(lhs, *runion);
  }

  return {true, std::nullopt, false};
}

}  // namespace cursive0::analysis

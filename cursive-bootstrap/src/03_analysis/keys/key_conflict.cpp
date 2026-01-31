#include "cursive0/03_analysis/keys/key_conflict.h"

#include <algorithm>

#include "cursive0/00_core/assert_spec.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsKeyConflict() {
  SPEC_DEF("KeyConflict", "C0X.5.X");
  SPEC_DEF("Overlap", "C0X.5.X");
  SPEC_DEF("ConflictRule", "C0X.5.X");
  SPEC_DEF("CanonicalOrder", "C0X.5.X");
}

}  // namespace

bool KeysConflict(const KeyPath& p1, KeyAccessMode m1,
                  const KeyPath& p2, KeyAccessMode m2) {
  SpecDefsKeyConflict();
  SPEC_RULE("K-Conflict");
  
  // Conflict rule: Conflict((P1, M1), (P2, M2)) = Overlap(P1, P2) ∧ (M1 = Write ∨ M2 = Write)
  if (!PathsOverlap(p1, p2)) {
    return false;
  }
  
  // No conflict if both are read-only
  if (m1 == KeyAccessMode::Read && m2 == KeyAccessMode::Read) {
    return false;
  }
  
  // Conflict: overlapping paths with at least one write
  return true;
}

ConflictResult CheckAcquisitionConflict(const KeyContext& ctx,
                                        const KeyPath& path,
                                        KeyAccessMode mode,
                                        const core::Span& span) {
  SpecDefsKeyConflict();
  SPEC_RULE("K-CheckAcquisition");
  
  ConflictResult result;
  
  for (const auto& held : ctx.HeldKeys()) {
    if (KeysConflict(held.path, held.mode, path, mode)) {
      result.conflict = true;
      result.diag_id = "E-CON-0012";  // Key conflict error
      result.span = span;
      result.path1 = held.path;
      result.path2 = path;
      return result;
    }
  }
  
  return result;
}

ConflictResult CheckBlockConflict(
    const std::vector<std::pair<KeyPath, KeyAccessMode>>& keys1,
    const std::vector<std::pair<KeyPath, KeyAccessMode>>& keys2,
    const core::Span& span) {
  SpecDefsKeyConflict();
  SPEC_RULE("K-CheckBlockConflict");
  
  ConflictResult result;
  
  for (const auto& [p1, m1] : keys1) {
    for (const auto& [p2, m2] : keys2) {
      if (KeysConflict(p1, m1, p2, m2)) {
        result.conflict = true;
        result.diag_id = "E-CON-0060";  // Parallel key conflict
        result.span = span;
        result.path1 = p1;
        result.path2 = p2;
        return result;
      }
    }
  }
  
  return result;
}

OrderValidation ValidateAcquisitionOrder(
    const std::vector<std::pair<KeyPath, KeyAccessMode>>& keys) {
  SpecDefsKeyConflict();
  SPEC_RULE("K-ValidateOrder");
  
  OrderValidation result;
  
  // Check if keys are in canonical (lexicographic) order
  for (std::size_t i = 1; i < keys.size(); ++i) {
    if (!KeyPathLess(keys[i - 1].first, keys[i].first)) {
      result.ok = false;
      result.diag_id = "W-CON-0001";  // Non-canonical order warning
      break;
    }
  }
  
  // Compute suggested order
  result.suggested_order = {};
  auto sorted = keys;
  std::stable_sort(sorted.begin(), sorted.end(),
                   [](const auto& a, const auto& b) {
                     return KeyPathLess(a.first, b.first);
                   });
  for (const auto& [path, _] : sorted) {
    result.suggested_order.push_back(path);
  }
  
  return result;
}

std::vector<std::pair<KeyPath, KeyAccessMode>> CanonicalOrder(
    const std::vector<std::pair<KeyPath, KeyAccessMode>>& keys) {
  SpecDefsKeyConflict();
  SPEC_RULE("K-CanonicalOrder");
  
  auto sorted = keys;
  std::stable_sort(sorted.begin(), sorted.end(),
                   [](const auto& a, const auto& b) {
                     return KeyPathLess(a.first, b.first);
                   });
  return sorted;
}

bool StaticallyDisjoint(const syntax::ExprPtr& idx1, const syntax::ExprPtr& idx2) {
  SpecDefsKeyConflict();
  SPEC_RULE("K-StaticallyDisjoint");
  
  if (!idx1 || !idx2) {
    return false;
  }
  
  // Simple case: both are literal integers with different values
  const auto* lit1 = std::get_if<syntax::LiteralExpr>(&idx1->node);
  const auto* lit2 = std::get_if<syntax::LiteralExpr>(&idx2->node);
  
  if (lit1 && lit2) {
    if (lit1->literal.kind == syntax::TokenKind::IntLiteral &&
        lit2->literal.kind == syntax::TokenKind::IntLiteral) {
      return lit1->literal.lexeme != lit2->literal.lexeme;
    }
  }
  
  // More sophisticated analysis would involve:
  // - Constant propagation
  // - Linear integer reasoning (i ≠ j if i - j ≠ 0)
  // - Type-derived bounds
  
  return false;
}

}  // namespace cursive0::analysis

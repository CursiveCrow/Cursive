#include "cursive0/analysis/contracts/verification.h"

#include "cursive0/core/assert_spec.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsVerification() {
  SPEC_DEF("StaticProof", "C0X.5.W");
  SPEC_DEF("Entailment", "C0X.5.W");
  SPEC_DEF("VerificationFact", "C0X.5.W");
  SPEC_DEF("Ent-True", "C0X.5.W");
  SPEC_DEF("Ent-Fact", "C0X.5.W");
  SPEC_DEF("Ent-And", "C0X.5.W");
  SPEC_DEF("Ent-Or", "C0X.5.W");
  SPEC_DEF("Ent-Linear", "C0X.5.W");
}

// Check if expression is a literal true
bool IsLiteralTrue(const syntax::ExprPtr& expr) {
  if (!expr) return false;
  if (const auto* lit = std::get_if<syntax::LiteralExpr>(&expr->node)) {
    return lit->literal.kind == syntax::TokenKind::BoolLiteral &&
           lit->literal.lexeme == "true";
  }
  return false;
}

// Check if expression is a literal false
bool IsLiteralFalse(const syntax::ExprPtr& expr) {
  if (!expr) return false;
  if (const auto* lit = std::get_if<syntax::LiteralExpr>(&expr->node)) {
    return lit->literal.kind == syntax::TokenKind::BoolLiteral &&
           lit->literal.lexeme == "false";
  }
  return false;
}

// Simple structural equality of expressions
bool ExprStructEqual(const syntax::ExprPtr& a, const syntax::ExprPtr& b) {
  if (!a && !b) return true;
  if (!a || !b) return false;
  
  return std::visit(
      [&](const auto& node_a) -> bool {
        using T = std::decay_t<decltype(node_a)>;
        const auto* node_b = std::get_if<T>(&b->node);
        if (!node_b) return false;
        
        if constexpr (std::is_same_v<T, syntax::LiteralExpr>) {
          return node_a.literal.lexeme == node_b->literal.lexeme;
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return node_a.name == node_b->name;
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          return node_a.op == node_b->op &&
                 ExprStructEqual(node_a.lhs, node_b->lhs) &&
                 ExprStructEqual(node_a.rhs, node_b->rhs);
        }
        return false;
      },
      a->node);
}

}  // namespace

StaticProofResult StaticProof(
    const StaticProofContext& ctx,
    const syntax::ExprPtr& predicate) {
  SpecDefsVerification();
  SPEC_RULE("StaticProof");
  
  StaticProofResult result;
  
  // Try each entailment rule
  
  // Ent-True
  if (EntTrue(predicate)) {
    result.provable = true;
    result.explanation = "Trivially true";
    return result;
  }
  
  // Ent-Fact
  if (EntFact(ctx, predicate)) {
    result.provable = true;
    result.explanation = "Matches known fact";
    return result;
  }
  
  // Ent-And
  if (const auto* binary = std::get_if<syntax::BinaryExpr>(&predicate->node)) {
    if (binary->op == "&&") {
      if (EntAnd(ctx, binary->lhs, binary->rhs)) {
        result.provable = true;
        result.explanation = "Both conjuncts provable";
        return result;
      }
    }
    
    // Ent-Or
    if (binary->op == "||") {
      if (EntOr(ctx, binary->lhs, binary->rhs)) {
        result.provable = true;
        result.explanation = "At least one disjunct provable";
        return result;
      }
    }
  }
  
  // Ent-Linear
  if (EntLinear(ctx, predicate)) {
    result.provable = true;
    result.explanation = "Linear integer reasoning";
    return result;
  }
  
  result.provable = false;
  result.diag_id = "E-SEM-2850";  // Cannot prove predicate
  return result;
}

bool EntTrue(const syntax::ExprPtr& expr) {
  SpecDefsVerification();
  SPEC_RULE("Ent-True");
  return IsLiteralTrue(expr);
}

bool EntFact(const StaticProofContext& ctx, const syntax::ExprPtr& expr) {
  SpecDefsVerification();
  SPEC_RULE("Ent-Fact");
  
  for (const auto& fact : ctx.facts) {
    if (FactDominates(fact, expr->span) && 
        ExprStructEqual(fact.predicate, expr)) {
      return true;
    }
  }
  return false;
}

bool EntAnd(const StaticProofContext& ctx,
            const syntax::ExprPtr& left,
            const syntax::ExprPtr& right) {
  SpecDefsVerification();
  SPEC_RULE("Ent-And");
  
  auto left_proof = StaticProof(ctx, left);
  if (!left_proof.provable) return false;
  
  auto right_proof = StaticProof(ctx, right);
  return right_proof.provable;
}

bool EntOr(const StaticProofContext& ctx,
           const syntax::ExprPtr& left,
           const syntax::ExprPtr& right) {
  SpecDefsVerification();
  SPEC_RULE("Ent-Or");
  
  auto left_proof = StaticProof(ctx, left);
  if (left_proof.provable) return true;
  
  auto right_proof = StaticProof(ctx, right);
  return right_proof.provable;
}

bool EntLinear(const StaticProofContext& ctx, const syntax::ExprPtr& expr) {
  SpecDefsVerification();
  SPEC_RULE("Ent-Linear");
  
  // Simple linear integer reasoning
  // Proves relations like x + 1 > x, 0 <= n (for unsigned), etc.
  
  if (const auto* binary = std::get_if<syntax::BinaryExpr>(&expr->node)) {
    // x + 1 > x  is always true for integers without overflow
    // x < x + 1  is always true
    // etc.
    
    // Simplified: just check for obvious tautologies
    if (binary->op == ">" || binary->op == ">=") {
      // Check for x + c > x pattern
      if (const auto* add = std::get_if<syntax::BinaryExpr>(&binary->lhs->node)) {
        if (add->op == "+" && ExprStructEqual(add->lhs, binary->rhs)) {
          // (x + c) > x: true if c > 0
          auto c_val = EvaluateConstant(add->rhs);
          if (c_val.known && c_val.value > 0) {
            return true;
          }
        }
      }
    }
  }
  
  return false;
}

ConstValue EvaluateConstant(const syntax::ExprPtr& expr) {
  SpecDefsVerification();
  
  ConstValue result;
  if (!expr) return result;
  
  if (const auto* lit = std::get_if<syntax::LiteralExpr>(&expr->node)) {
    if (lit->literal.kind == syntax::TokenKind::IntLiteral) {
      try {
        result.value = std::stoll(std::string(lit->literal.lexeme));
        result.known = true;
      } catch (...) {
        // Conversion failed
      }
    } else if (lit->literal.kind == syntax::TokenKind::BoolLiteral) {
      result.bool_value = lit->literal.lexeme == "true";
      result.is_bool = true;
      result.known = true;
    }
  }
  
  return result;
}

syntax::ExprPtr SimplifyBoolean(const syntax::ExprPtr& expr) {
  // Boolean algebra simplification
  // true && P -> P
  // false && P -> false
  // true || P -> true
  // false || P -> P
  // !true -> false
  // !false -> true
  // !!P -> P
  
  if (!expr) return expr;
  
  // Simplified implementation - return as-is
  return expr;
}

bool IsReachable(const StaticProofContext& ctx, const core::Span& location) {
  // Simplified: always reachable
  return true;
}

TypeBounds GetTypeBounds(const TypeRef& type) {
  TypeBounds bounds;
  
  if (!type) return bounds;
  
  // Check for primitive integer types
  if (const auto* prim = std::get_if<TypePrim>(&type->node)) {
    if (prim->name == "u8") {
      bounds.has_min = bounds.has_max = true;
      bounds.min = 0;
      bounds.max = 255;
    } else if (prim->name == "u16") {
      bounds.has_min = bounds.has_max = true;
      bounds.min = 0;
      bounds.max = 65535;
    } else if (prim->name == "u32") {
      bounds.has_min = bounds.has_max = true;
      bounds.min = 0;
      bounds.max = 4294967295LL;
    } else if (prim->name == "i8") {
      bounds.has_min = bounds.has_max = true;
      bounds.min = -128;
      bounds.max = 127;
    } else if (prim->name == "i16") {
      bounds.has_min = bounds.has_max = true;
      bounds.min = -32768;
      bounds.max = 32767;
    } else if (prim->name == "i32") {
      bounds.has_min = bounds.has_max = true;
      bounds.min = -2147483648LL;
      bounds.max = 2147483647LL;
    }
    // i64/u64 bounds would overflow int64
  }
  
  return bounds;
}

void AddFact(StaticProofContext& ctx,
             const syntax::ExprPtr& predicate,
             const core::Span& location) {
  VerificationFact fact;
  fact.predicate = predicate;
  fact.location = location;
  fact.scope_id = ctx.current_scope;
  ctx.facts.push_back(fact);
}

bool FactDominates(const VerificationFact& fact, const core::Span& location) {
  // Simplified: fact dominates if it's in the same file and before location
  return fact.location.file == location.file &&
         fact.location.start_offset <= location.start_offset;
}

}  // namespace cursive0::analysis

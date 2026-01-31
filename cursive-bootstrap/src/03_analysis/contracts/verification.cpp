#include "cursive0/03_analysis/contracts/verification.h"

#include <limits>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>

#include "cursive0/00_core/assert_spec.h"

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
bool ExprStructEqualInternal(const syntax::ExprPtr& a,
                             const syntax::ExprPtr& b) {
  if (!a && !b) return true;
  if (!a || !b) return false;
  
  return std::visit(
      [&](const auto& node_a) -> bool {
        using T = std::decay_t<decltype(node_a)>;
        const auto* node_b = std::get_if<T>(&b->node);
        if (!node_b) return false;
        
        if constexpr (std::is_same_v<T, syntax::LiteralExpr>) {
          return node_a.literal.kind == node_b->literal.kind &&
                 node_a.literal.lexeme == node_b->literal.lexeme;
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return node_a.name == node_b->name;
        } else if constexpr (std::is_same_v<T, syntax::QualifiedNameExpr>) {
          return node_a.path == node_b->path && node_a.name == node_b->name;
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          return node_a.path == node_b->path && node_a.name == node_b->name;
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          return node_a.op == node_b->op &&
                 ExprStructEqualInternal(node_a.lhs, node_b->lhs) &&
                 ExprStructEqualInternal(node_a.rhs, node_b->rhs);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          return node_a.op == node_b->op &&
                 ExprStructEqualInternal(node_a.value, node_b->value);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return node_a.name == node_b->name &&
                 ExprStructEqualInternal(node_a.base, node_b->base);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return node_a.index.lexeme == node_b->index.lexeme &&
                 ExprStructEqualInternal(node_a.base, node_b->base);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return ExprStructEqualInternal(node_a.base, node_b->base) &&
                 ExprStructEqualInternal(node_a.index, node_b->index);
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          if (!ExprStructEqualInternal(node_a.callee, node_b->callee)) {
            return false;
          }
          if (node_a.args.size() != node_b->args.size()) {
            return false;
          }
          for (std::size_t i = 0; i < node_a.args.size(); ++i) {
            if (node_a.args[i].moved != node_b->args[i].moved) {
              return false;
            }
            if (!ExprStructEqualInternal(node_a.args[i].value,
                                         node_b->args[i].value)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
          if (node_a.path != node_b->path || node_a.name != node_b->name) {
            return false;
          }
          if (node_a.args.index() != node_b->args.index()) {
            return false;
          }
          if (std::holds_alternative<syntax::ParenArgs>(node_a.args)) {
            const auto& lhs = std::get<syntax::ParenArgs>(node_a.args);
            const auto& rhs = std::get<syntax::ParenArgs>(node_b->args);
            if (lhs.args.size() != rhs.args.size()) {
              return false;
            }
            for (std::size_t i = 0; i < lhs.args.size(); ++i) {
              if (lhs.args[i].moved != rhs.args[i].moved) {
                return false;
              }
              if (!ExprStructEqualInternal(lhs.args[i].value,
                                           rhs.args[i].value)) {
                return false;
              }
            }
            return true;
          }
          const auto& lhs = std::get<syntax::BraceArgs>(node_a.args);
          const auto& rhs = std::get<syntax::BraceArgs>(node_b->args);
          if (lhs.fields.size() != rhs.fields.size()) {
            return false;
          }
          for (std::size_t i = 0; i < lhs.fields.size(); ++i) {
            if (lhs.fields[i].name != rhs.fields[i].name) {
              return false;
            }
            if (!ExprStructEqualInternal(lhs.fields[i].value,
                                         rhs.fields[i].value)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          if (node_a.name != node_b->name) {
            return false;
          }
          if (!ExprStructEqualInternal(node_a.receiver, node_b->receiver)) {
            return false;
          }
          if (node_a.args.size() != node_b->args.size()) {
            return false;
          }
          for (std::size_t i = 0; i < node_a.args.size(); ++i) {
            if (node_a.args[i].moved != node_b->args[i].moved) {
              return false;
            }
            if (!ExprStructEqualInternal(node_a.args[i].value,
                                         node_b->args[i].value)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, syntax::EntryExpr>) {
          return ExprStructEqualInternal(node_a.expr, node_b->expr);
        } else if constexpr (std::is_same_v<T, syntax::ResultExpr>) {
          return true;
        }
        return false;
      },
      a->node);
}

struct LinearExpr {
  std::map<std::string, std::int64_t> terms;
  std::int64_t constant = 0;
};

struct DiffConstraint {
  std::string x;
  std::string y;
  std::int64_t c = 0;  // x - y <= c
};

struct PredConstraints {
  bool ok = false;
  bool constant = false;
  bool value = false;
  std::vector<DiffConstraint> constraints;
};

static bool CheckedAdd(std::int64_t a, std::int64_t b, std::int64_t& out) {
#if defined(__GNUC__) || defined(__clang__)
  return !__builtin_add_overflow(a, b, &out);
#else
  if ((b > 0 && a > std::numeric_limits<std::int64_t>::max() - b) ||
      (b < 0 && a < std::numeric_limits<std::int64_t>::min() - b)) {
    return false;
  }
  out = a + b;
  return true;
#endif
}

static bool CheckedSub(std::int64_t a, std::int64_t b, std::int64_t& out) {
#if defined(__GNUC__) || defined(__clang__)
  return !__builtin_sub_overflow(a, b, &out);
#else
  if ((b < 0 && a > std::numeric_limits<std::int64_t>::max() + b) ||
      (b > 0 && a < std::numeric_limits<std::int64_t>::min() + b)) {
    return false;
  }
  out = a - b;
  return true;
#endif
}

static bool CheckedMul(std::int64_t a, std::int64_t b, std::int64_t& out) {
#if defined(__GNUC__) || defined(__clang__)
  return !__builtin_mul_overflow(a, b, &out);
#else
  if (a == 0 || b == 0) {
    out = 0;
    return true;
  }
  if (a == -1 && b == std::numeric_limits<std::int64_t>::min()) {
    return false;
  }
  if (b == -1 && a == std::numeric_limits<std::int64_t>::min()) {
    return false;
  }
  out = a * b;
  return (out / b) == a;
#endif
}

static bool AddTerm(LinearExpr& expr,
                    const std::string& var,
                    std::int64_t coeff) {
  if (coeff == 0) {
    return true;
  }
  auto it = expr.terms.find(var);
  if (it == expr.terms.end()) {
    expr.terms.emplace(var, coeff);
    return true;
  }
  std::int64_t sum = 0;
  if (!CheckedAdd(it->second, coeff, sum)) {
    return false;
  }
  if (sum == 0) {
    expr.terms.erase(it);
    return true;
  }
  it->second = sum;
  return true;
}

static bool MergeLinear(LinearExpr& dst,
                        const LinearExpr& src,
                        std::int64_t scale) {
  std::int64_t scaled_const = 0;
  if (!CheckedMul(src.constant, scale, scaled_const)) {
    return false;
  }
  if (!CheckedAdd(dst.constant, scaled_const, dst.constant)) {
    return false;
  }
  for (const auto& [var, coeff] : src.terms) {
    std::int64_t scaled_coeff = 0;
    if (!CheckedMul(coeff, scale, scaled_coeff)) {
      return false;
    }
    if (!AddTerm(dst, var, scaled_coeff)) {
      return false;
    }
  }
  return true;
}

static std::optional<std::int64_t> ParseIntLiteral(std::string_view lexeme) {
  std::string cleaned;
  cleaned.reserve(lexeme.size());
  for (const char ch : lexeme) {
    if (ch != '_') {
      cleaned.push_back(ch);
    }
  }
  try {
    std::size_t idx = 0;
    const auto value = std::stoll(cleaned, &idx, 0);
    if (idx != cleaned.size()) {
      return std::nullopt;
    }
    return value;
  } catch (...) {
    return std::nullopt;
  }
}

static std::optional<std::string> ExprKey(const syntax::ExprPtr& expr) {
  if (!expr) {
    return std::nullopt;
  }
  if (const auto* ident = std::get_if<syntax::IdentifierExpr>(&expr->node)) {
    return ident->name;
  }
  if (const auto* path = std::get_if<syntax::PathExpr>(&expr->node)) {
    std::string out;
    for (std::size_t i = 0; i < path->path.size(); ++i) {
      if (i > 0) {
        out.append("::");
      }
      out.append(path->path[i]);
    }
    if (!path->path.empty()) {
      out.append("::");
    }
    out.append(path->name);
    return out;
  }
  if (const auto* qual = std::get_if<syntax::QualifiedNameExpr>(&expr->node)) {
    std::string out;
    for (std::size_t i = 0; i < qual->path.size(); ++i) {
      if (i > 0) {
        out.append("::");
      }
      out.append(qual->path[i]);
    }
    if (!qual->path.empty()) {
      out.append("::");
    }
    out.append(qual->name);
    return out;
  }
  if (const auto* field = std::get_if<syntax::FieldAccessExpr>(&expr->node)) {
    const auto base_key = ExprKey(field->base);
    if (!base_key.has_value()) {
      return std::nullopt;
    }
    std::string out = *base_key;
    out.push_back('.');
    out.append(field->name);
    return out;
  }
  if (const auto* tuple = std::get_if<syntax::TupleAccessExpr>(&expr->node)) {
    const auto base_key = ExprKey(tuple->base);
    if (!base_key.has_value()) {
      return std::nullopt;
    }
    std::string out = *base_key;
    out.push_back('.');
    out.append(tuple->index.lexeme);
    return out;
  }
  if (const auto* index = std::get_if<syntax::IndexAccessExpr>(&expr->node)) {
    const auto base_key = ExprKey(index->base);
    if (!base_key.has_value()) {
      return std::nullopt;
    }
    if (!index->index) {
      return std::nullopt;
    }
    const auto* lit = std::get_if<syntax::LiteralExpr>(&index->index->node);
    if (!lit || lit->literal.kind != syntax::TokenKind::IntLiteral) {
      return std::nullopt;
    }
    const auto parsed = ParseIntLiteral(lit->literal.lexeme);
    if (!parsed.has_value()) {
      return std::nullopt;
    }
    std::string out = *base_key;
    out.push_back('[');
    out.append(std::to_string(*parsed));
    out.push_back(']');
    return out;
  }
  if (const auto* entry = std::get_if<syntax::EntryExpr>(&expr->node)) {
    if (!entry->expr) {
      return std::nullopt;
    }
    const auto inner_key = ExprKey(entry->expr);
    if (!inner_key.has_value()) {
      return std::nullopt;
    }
    std::string out = "@entry(";
    out.append(*inner_key);
    out.push_back(')');
    return out;
  }
  if (std::holds_alternative<syntax::ResultExpr>(expr->node)) {
    return std::string("@result");
  }
  return std::nullopt;
}

static bool LinearizeExpr(const syntax::ExprPtr& expr, LinearExpr& out) {
  if (!expr) {
    return false;
  }
  if (const auto* lit = std::get_if<syntax::LiteralExpr>(&expr->node)) {
    if (lit->literal.kind != syntax::TokenKind::IntLiteral) {
      return false;
    }
    const auto parsed = ParseIntLiteral(lit->literal.lexeme);
    if (!parsed.has_value()) {
      return false;
    }
    out.constant = *parsed;
    return true;
  }

  if (const auto key = ExprKey(expr); key.has_value()) {
    if (!AddTerm(out, *key, 1)) {
      return false;
    }
    return true;
  }

  if (const auto* unary = std::get_if<syntax::UnaryExpr>(&expr->node)) {
    if (unary->op != "-" && unary->op != "+") {
      return false;
    }
    LinearExpr inner;
    if (!LinearizeExpr(unary->value, inner)) {
      return false;
    }
    const std::int64_t scale = (unary->op == "-") ? -1 : 1;
    if (!MergeLinear(out, inner, scale)) {
      return false;
    }
    return true;
  }

  if (const auto* binary = std::get_if<syntax::BinaryExpr>(&expr->node)) {
    if (binary->op == "+" || binary->op == "-") {
      LinearExpr left;
      LinearExpr right;
      if (!LinearizeExpr(binary->lhs, left) ||
          !LinearizeExpr(binary->rhs, right)) {
        return false;
      }
      if (!MergeLinear(out, left, 1)) {
        return false;
      }
      const std::int64_t scale = (binary->op == "-") ? -1 : 1;
      if (!MergeLinear(out, right, scale)) {
        return false;
      }
      return true;
    }
    if (binary->op == "*") {
      LinearExpr left;
      LinearExpr right;
      if (!LinearizeExpr(binary->lhs, left) ||
          !LinearizeExpr(binary->rhs, right)) {
        return false;
      }
      if (!left.terms.empty() && !right.terms.empty()) {
        return false;
      }
      if (left.terms.empty()) {
        if (!MergeLinear(out, right, left.constant)) {
          return false;
        }
        return true;
      }
      if (!MergeLinear(out, left, right.constant)) {
        return false;
      }
      return true;
    }
  }

  return false;
}

static bool NegateLinear(LinearExpr& expr) {
  if (!CheckedMul(expr.constant, -1, expr.constant)) {
    return false;
  }
  for (auto& [var, coeff] : expr.terms) {
    if (!CheckedMul(coeff, -1, coeff)) {
      return false;
    }
  }
  return true;
}

static bool EvaluateComparison(std::int64_t lhs,
                               std::int64_t rhs,
                               std::string_view op,
                               bool& value) {
  if (op == "==") {
    value = lhs == rhs;
    return true;
  }
  if (op == "!=") {
    value = lhs != rhs;
    return true;
  }
  if (op == "<") {
    value = lhs < rhs;
    return true;
  }
  if (op == "<=") {
    value = lhs <= rhs;
    return true;
  }
  if (op == ">") {
    value = lhs > rhs;
    return true;
  }
  if (op == ">=") {
    value = lhs >= rhs;
    return true;
  }
  return false;
}

static bool AddDiffConstraint(const LinearExpr& diff,
                              std::int64_t bound,
                              std::vector<DiffConstraint>& out,
                              bool& constant_eval,
                              bool& constant_value) {
  if (diff.terms.empty()) {
    constant_eval = true;
    constant_value = diff.constant <= bound;
    return true;
  }
  if (diff.terms.size() == 1) {
    const auto& [var, coeff] = *diff.terms.begin();
    std::int64_t rhs = 0;
    if (!CheckedSub(bound, diff.constant, rhs)) {
      return false;
    }
    if (coeff == 1) {
      out.push_back({var, "__zero", rhs});
      return true;
    }
    if (coeff == -1) {
      out.push_back({"__zero", var, rhs});
      return true;
    }
    return false;
  }
  if (diff.terms.size() == 2) {
    auto it = diff.terms.begin();
    const auto& var1 = it->first;
    const auto coeff1 = it->second;
    ++it;
    const auto& var2 = it->first;
    const auto coeff2 = it->second;
    std::int64_t rhs = 0;
    if (!CheckedSub(bound, diff.constant, rhs)) {
      return false;
    }
    if (coeff1 == 1 && coeff2 == -1) {
      out.push_back({var1, var2, rhs});
      return true;
    }
    if (coeff1 == -1 && coeff2 == 1) {
      out.push_back({var2, var1, rhs});
      return true;
    }
  }
  return false;
}

static PredConstraints BuildConstraintsFromPredicate(
    const syntax::ExprPtr& expr) {
  PredConstraints out;
  const auto* binary = expr
                           ? std::get_if<syntax::BinaryExpr>(&expr->node)
                           : nullptr;
  if (!binary) {
    return out;
  }

  if (binary->op != "==" && binary->op != "!=" &&
      binary->op != "<" && binary->op != "<=" &&
      binary->op != ">" && binary->op != ">=") {
    return out;
  }

  LinearExpr lhs;
  LinearExpr rhs;
  if (!LinearizeExpr(binary->lhs, lhs) || !LinearizeExpr(binary->rhs, rhs)) {
    return out;
  }
  if (!MergeLinear(lhs, rhs, -1)) {
    return out;
  }

  if (lhs.terms.empty()) {
    bool value = false;
    if (!EvaluateComparison(lhs.constant, 0, binary->op, value)) {
      return out;
    }
    out.ok = true;
    out.constant = true;
    out.value = value;
    return out;
  }

  if (binary->op == "!=") {
    return out;
  }

  auto add_bound = [&](std::int64_t bound, LinearExpr diff) -> bool {
    bool constant_eval = false;
    bool constant_value = false;
    if (!AddDiffConstraint(diff, bound, out.constraints, constant_eval,
                           constant_value)) {
      return false;
    }
    if (constant_eval) {
      out.constant = true;
      out.value = constant_value;
      out.constraints.clear();
    }
    return true;
  };

  if (binary->op == "==" ) {
    LinearExpr diff = lhs;
    if (!add_bound(0, diff)) {
      return out;
    }
    if (out.constant) {
      out.ok = true;
      return out;
    }
    if (!NegateLinear(diff)) {
      return out;
    }
    if (!add_bound(0, diff)) {
      return out;
    }
    out.ok = true;
    return out;
  }

  if (binary->op == ">=" || binary->op == ">") {
    if (!NegateLinear(lhs)) {
      return out;
    }
  }

  std::int64_t bound = 0;
  if (binary->op == "<" || binary->op == ">") {
    if (!CheckedSub(0, 1, bound)) {
      return out;
    }
  }
  if (!add_bound(bound, lhs)) {
    return out;
  }
  out.ok = true;
  return out;
}

static bool EntailsConstraints(const std::vector<DiffConstraint>& facts,
                               const std::vector<DiffConstraint>& targets) {
  if (targets.empty()) {
    return false;
  }
  std::unordered_map<std::string, std::size_t> indices;
  std::vector<std::string> vars;
  auto add_var = [&](const std::string& var) {
    if (indices.find(var) != indices.end()) {
      return;
    }
    indices[var] = vars.size();
    vars.push_back(var);
  };

  add_var("__zero");
  for (const auto& c : facts) {
    add_var(c.x);
    add_var(c.y);
  }
  for (const auto& c : targets) {
    add_var(c.x);
    add_var(c.y);
  }

  const std::size_t n = vars.size();
  const auto INF = std::numeric_limits<std::int64_t>::max() / 4;
  std::vector<std::vector<std::int64_t>> dist(
      n, std::vector<std::int64_t>(n, INF));
  for (std::size_t i = 0; i < n; ++i) {
    dist[i][i] = 0;
  }
  for (const auto& c : facts) {
    const auto from = indices[c.y];
    const auto to = indices[c.x];
    if (c.c < dist[from][to]) {
      dist[from][to] = c.c;
    }
  }

  for (std::size_t k = 0; k < n; ++k) {
    for (std::size_t i = 0; i < n; ++i) {
      if (dist[i][k] >= INF) {
        continue;
      }
      for (std::size_t j = 0; j < n; ++j) {
        if (dist[k][j] >= INF) {
          continue;
        }
        std::int64_t sum = 0;
        if (!CheckedAdd(dist[i][k], dist[k][j], sum)) {
          continue;
        }
        if (sum < dist[i][j]) {
          dist[i][j] = sum;
        }
      }
    }
  }

  for (const auto& target : targets) {
    const auto from = indices[target.y];
    const auto to = indices[target.x];
    if (dist[from][to] > target.c) {
      return false;
    }
  }
  return true;
}

}  // namespace

bool ExprStructEqual(const syntax::ExprPtr& a, const syntax::ExprPtr& b) {
  return ExprStructEqualInternal(a, b);
}

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

  const auto const_eval = EvaluateConstant(predicate);
  if (const_eval.known && const_eval.is_bool) {
    if (const_eval.bool_value) {
      result.provable = true;
      result.explanation = "Constant evaluation";
      return result;
    }
    result.provable = false;
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
  if (!expr) {
    return false;
  }

  const auto target = BuildConstraintsFromPredicate(expr);
  if (!target.ok) {
    return false;
  }
  if (target.constant) {
    return target.value;
  }

  std::vector<DiffConstraint> facts;
  for (const auto& fact : ctx.facts) {
    if (!FactDominates(fact, expr->span)) {
      continue;
    }
    const auto pred = BuildConstraintsFromPredicate(fact.predicate);
    if (!pred.ok) {
      continue;
    }
    if (pred.constant) {
      if (!pred.value) {
        // Inconsistent fact set; be conservative.
        continue;
      }
      continue;
    }
    facts.insert(facts.end(), pred.constraints.begin(), pred.constraints.end());
  }

  return EntailsConstraints(facts, target.constraints);
}

ConstValue EvaluateConstant(const syntax::ExprPtr& expr) {
  SpecDefsVerification();
  
  ConstValue result;
  if (!expr) {
    return result;
  }

  auto parse_int = [](std::string_view lexeme) -> std::optional<std::int64_t> {
    std::string cleaned;
    cleaned.reserve(lexeme.size());
    for (const char ch : lexeme) {
      if (ch != '_') {
        cleaned.push_back(ch);
      }
    }
    try {
      std::size_t idx = 0;
      const auto value = std::stoll(cleaned, &idx, 0);
      if (idx != cleaned.size()) {
        return std::nullopt;
      }
      return value;
    } catch (...) {
      return std::nullopt;
    }
  };

  if (const auto* lit = std::get_if<syntax::LiteralExpr>(&expr->node)) {
    if (lit->literal.kind == syntax::TokenKind::IntLiteral) {
      const auto parsed = parse_int(lit->literal.lexeme);
      if (parsed.has_value()) {
        result.value = *parsed;
        result.known = true;
      }
      return result;
    }
    if (lit->literal.kind == syntax::TokenKind::BoolLiteral) {
      result.bool_value = lit->literal.lexeme == "true";
      result.is_bool = true;
      result.known = true;
      return result;
    }
  }

  if (const auto* unary = std::get_if<syntax::UnaryExpr>(&expr->node)) {
    const auto inner = EvaluateConstant(unary->value);
    if (!inner.known) {
      return result;
    }
    if (unary->op == "-" && !inner.is_bool) {
      result.known = true;
      result.value = -inner.value;
      return result;
    }
    if (unary->op == "+" && !inner.is_bool) {
      return inner;
    }
    if (unary->op == "!" && inner.is_bool) {
      result.known = true;
      result.is_bool = true;
      result.bool_value = !inner.bool_value;
      return result;
    }
    return result;
  }

  if (const auto* binary = std::get_if<syntax::BinaryExpr>(&expr->node)) {
    const auto lhs = EvaluateConstant(binary->lhs);
    const auto rhs = EvaluateConstant(binary->rhs);
    if (!lhs.known || !rhs.known) {
      return result;
    }
    if (!lhs.is_bool && !rhs.is_bool) {
      if (binary->op == "+" || binary->op == "-" ||
          binary->op == "*" || binary->op == "/" ||
          binary->op == "%") {
        std::int64_t out = 0;
        if (binary->op == "+") {
          out = lhs.value + rhs.value;
        } else if (binary->op == "-") {
          out = lhs.value - rhs.value;
        } else if (binary->op == "*") {
          out = lhs.value * rhs.value;
        } else if (binary->op == "/") {
          if (rhs.value == 0) {
            return result;
          }
          out = lhs.value / rhs.value;
        } else if (binary->op == "%") {
          if (rhs.value == 0) {
            return result;
          }
          out = lhs.value % rhs.value;
        }
        result.known = true;
        result.value = out;
        return result;
      }
      if (binary->op == "==" || binary->op == "!=" ||
          binary->op == "<" || binary->op == "<=" ||
          binary->op == ">" || binary->op == ">=") {
        result.known = true;
        result.is_bool = true;
        if (binary->op == "==") {
          result.bool_value = lhs.value == rhs.value;
        } else if (binary->op == "!=") {
          result.bool_value = lhs.value != rhs.value;
        } else if (binary->op == "<") {
          result.bool_value = lhs.value < rhs.value;
        } else if (binary->op == "<=") {
          result.bool_value = lhs.value <= rhs.value;
        } else if (binary->op == ">") {
          result.bool_value = lhs.value > rhs.value;
        } else if (binary->op == ">=") {
          result.bool_value = lhs.value >= rhs.value;
        }
        return result;
      }
      return result;
    }

    if (lhs.is_bool && rhs.is_bool) {
      if (binary->op == "&&") {
        result.known = true;
        result.is_bool = true;
        result.bool_value = lhs.bool_value && rhs.bool_value;
        return result;
      }
      if (binary->op == "||") {
        result.known = true;
        result.is_bool = true;
        result.bool_value = lhs.bool_value || rhs.bool_value;
        return result;
      }
      if (binary->op == "==" || binary->op == "!=") {
        result.known = true;
        result.is_bool = true;
        result.bool_value =
            (binary->op == "==") ? (lhs.bool_value == rhs.bool_value)
                                 : (lhs.bool_value != rhs.bool_value);
        return result;
      }
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

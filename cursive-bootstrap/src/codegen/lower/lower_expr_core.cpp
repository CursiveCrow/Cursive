#include "cursive0/codegen/lower/lower_expr.h"
#include "cursive0/codegen/lower/lower_pat.h"
#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/codegen/abi/abi.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/caps/cap_concurrency.h"
#include "cursive0/analysis/types/type_expr.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <variant>
#include <unordered_set>
#include <unordered_map>

namespace cursive0::codegen {

namespace {

std::string StripIntSuffix(std::string text) {
  static const char* suffixes[] = {
      "isize", "usize", "i128", "u128", "i64", "u64", "i32", "u32", "i16", "u16", "i8", "u8"
  };
  for (const char* suf : suffixes) {
    const std::string_view sv{suf};
    if (text.size() >= sv.size() &&
        text.compare(text.size() - sv.size(), sv.size(), sv) == 0) {
      return text.substr(0, text.size() - sv.size());
    }
  }
  return text;
}

std::optional<std::uint64_t> ParseIntLiteralLexeme(const std::string& lexeme) {
  std::string text = StripIntSuffix(lexeme);
  if (text.rfind("0b", 0) == 0 || text.rfind("0B", 0) == 0) {
    text.erase(0, 2);
    if (text.empty()) {
      return std::nullopt;
    }
    std::uint64_t out = 0;
    for (char c : text) {
      if (c == '_') {
        continue;
      }
      if (c != '0' && c != '1') {
        return std::nullopt;
      }
      out = (out << 1) | (c == '1');
    }
    return out;
  }
  if (text.rfind("0o", 0) == 0 || text.rfind("0O", 0) == 0) {
    text.erase(0, 2);
    if (text.empty()) {
      return std::nullopt;
    }
    std::uint64_t out = 0;
    for (char c : text) {
      if (c == '_') {
        continue;
      }
      if (c < '0' || c > '7') {
        return std::nullopt;
      }
      out = (out << 3) | static_cast<std::uint64_t>(c - '0');
    }
    return out;
  }
  try {
    size_t idx = 0;
    std::uint64_t out = std::stoull(text, &idx, 0);
    if (idx != text.size()) {
      return std::nullopt;
    }
    return out;
  } catch (...) {
    return std::nullopt;
  }
}

bool HasDynamicAttr(const syntax::AttributeList& attrs) {
  for (const auto& attr : attrs) {
    if (attr.name == "dynamic") {
      return true;
    }
  }
  return false;
}

std::vector<std::uint8_t> EncodeU64BE(std::uint64_t value) {
  std::vector<std::uint8_t> bytes;
  if (value == 0) {
    bytes.push_back(0);
    return bytes;
  }
  while (value > 0) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFF));
    value >>= 8;
  }
  std::reverse(bytes.begin(), bytes.end());
  return bytes;
}

syntax::ExprPtr WrapBlockExpr(const std::shared_ptr<syntax::Block>& block) {
  if (!block) {
    return nullptr;
  }
  auto expr = std::make_shared<syntax::Expr>();
  expr->span = block->span;
  expr->node = syntax::BlockExpr{block};
  return expr;
}

bool NeedsIndexCheck(const syntax::Expr& base, const LowerCtx& ctx) {
  if (!ctx.expr_type) {
    return true;
  }
  analysis::TypeRef base_type = ctx.expr_type(base);
  analysis::TypeRef stripped = analysis::StripPerm(base_type);
  if (stripped && std::holds_alternative<analysis::TypeArray>(stripped->node)) {
    return ctx.dynamic_checks;
  }
  return true;
}

bool IsPlaceExprForTemp(const syntax::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::AttributedExpr>) {
          return node.expr ? IsPlaceExprForTemp(*node.expr) : false;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return true;
        }
        return false;
      },
      expr.node);
}

bool IsTempValueExpr(const syntax::Expr& expr) {
  return !IsPlaceExprForTemp(expr);
}

bool DispatchHasReduce(const syntax::DispatchExpr& expr) {
  for (const auto& opt : expr.opts) {
    if (opt.kind == syntax::DispatchOptionKind::Reduce) {
      return true;
    }
  }
  return false;
}

bool IsCollectableParallelExpr(const syntax::Expr& expr, bool& needs_wait) {
  if (std::holds_alternative<syntax::SpawnExpr>(expr.node)) {
    needs_wait = true;
    return true;
  }
  if (const auto* dispatch = std::get_if<syntax::DispatchExpr>(&expr.node)) {
    if (DispatchHasReduce(*dispatch)) {
      needs_wait = false;
      return true;
    }
  }
  return false;
}

std::vector<syntax::ExprPtr> ArgsExprs(const std::vector<syntax::Arg>& args) {
  if (args.empty()) {
    SPEC_RULE("ArgsExprs-Empty");
    return {};
  }
  SPEC_RULE("ArgsExprs-Cons");
  std::vector<syntax::ExprPtr> result;
  result.reserve(args.size());
  for (const auto& arg : args) {
    result.push_back(arg.value);
  }
  return result;
}

std::vector<syntax::ExprPtr> FieldExprs(const std::vector<syntax::FieldInit>& fields) {
  if (fields.empty()) {
    SPEC_RULE("FieldExprs-Empty");
    return {};
  }
  SPEC_RULE("FieldExprs-Cons");
  std::vector<syntax::ExprPtr> result;
  result.reserve(fields.size());
  for (const auto& field : fields) {
    result.push_back(field.value);
  }
  return result;
}

std::vector<syntax::ExprPtr> OptExprs(const syntax::ExprPtr& lo_opt,
                                     const syntax::ExprPtr& hi_opt) {
  if (!lo_opt && !hi_opt) {
    SPEC_RULE("OptExprs-None");
    return {};
  }
  if (lo_opt && !hi_opt) {
    SPEC_RULE("OptExprs-Lo");
    return {lo_opt};
  }
  if (!lo_opt && hi_opt) {
    SPEC_RULE("OptExprs-Hi");
    return {hi_opt};
  }
  SPEC_RULE("OptExprs-Both");
  return {lo_opt, hi_opt};
}




IRRange ToIRRange(const RangeVal& range) {
  IRRange out;
  out.kind = range.kind;
  out.lo = range.lo;
  out.hi = range.hi;
  return out;
}

std::vector<std::uint8_t> LEBytesU64(std::uint64_t value, std::size_t n) {
  std::vector<std::uint8_t> bytes;
  bytes.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    bytes.push_back(static_cast<std::uint8_t>((value >> (8 * i)) & 0xFFu));
  }
  return bytes;
}

IRValue USizeImmediate(std::uint64_t value) {
  IRValue v;
  v.kind = IRValue::Kind::Immediate;
  v.name = std::to_string(value);
  v.bytes = LEBytesU64(value, static_cast<std::size_t>(kPtrSize));
  return v;
}

IRValue MakeNullPtr(const analysis::TypeRef& ptr_type,
                    LowerCtx& ctx,
                    std::vector<IRPtr>& parts) {
  IRValue zero = USizeImmediate(0);
  IRValue null_ptr = ctx.FreshTempValue("null_ptr");
  IRTransmute trans;
  trans.from = analysis::MakeTypePrim("usize");
  trans.to = ptr_type;
  trans.value = zero;
  trans.result = null_ptr;
  parts.push_back(MakeIR(std::move(trans)));
  ctx.RegisterValueType(null_ptr, ptr_type);
  return null_ptr;
}

analysis::Permission PermissionOfType(const analysis::TypeRef& type) {
  if (!type) {
    return analysis::Permission::Const;
  }
  if (const auto* perm = std::get_if<analysis::TypePerm>(&type->node)) {
    return perm->perm;
  }
  return analysis::Permission::Const;
}

bool IsUnitType(const analysis::TypeRef& type) {
  if (!type) {
    return false;
  }
  if (const auto* prim = std::get_if<analysis::TypePrim>(&type->node)) {
    return prim->name == "()";
  }
  return false;
}

struct CaptureBinding {
  std::string name;
  analysis::TypeRef type;
  bool explicit_move = false;
};

static void CollectPatternNames(const syntax::Pattern& pat,
                                std::vector<std::string>& out);

static void CollectFieldPatNames(const syntax::FieldPattern& field,
                                 std::vector<std::string>& out) {
  if (field.pattern_opt) {
    CollectPatternNames(*field.pattern_opt, out);
    return;
  }
  out.push_back(field.name);
}

static void CollectPatternNames(const syntax::Pattern& pat,
                                std::vector<std::string>& out) {
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          out.push_back(node.name);
        } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
          out.push_back(node.name);
        } else if constexpr (std::is_same_v<T, syntax::TuplePattern>) {
          for (const auto& elem : node.elements) {
            if (elem) {
              CollectPatternNames(*elem, out);
            }
          }
        } else if constexpr (std::is_same_v<T, syntax::RecordPattern>) {
          for (const auto& field : node.fields) {
            CollectFieldPatNames(field, out);
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
                    if (elem) {
                      CollectPatternNames(*elem, out);
                    }
                  }
                } else {
                  for (const auto& field : payload.fields) {
                    CollectFieldPatNames(field, out);
                  }
                }
              },
              *node.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::ModalPattern>) {
          if (!node.fields_opt.has_value()) {
            return;
          }
          for (const auto& field : node.fields_opt->fields) {
            CollectFieldPatNames(field, out);
          }
        } else if constexpr (std::is_same_v<T, syntax::RangePattern>) {
          if (node.lo) {
            CollectPatternNames(*node.lo, out);
          }
          if (node.hi) {
            CollectPatternNames(*node.hi, out);
          }
        }
      },
      pat.node);
}

struct ScopedNames {
  std::vector<std::unordered_set<std::string>> scopes;

  void Push() { scopes.emplace_back(); }
  void Pop() { if (!scopes.empty()) scopes.pop_back(); }
  bool IsLocal(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      if (it->find(name) != it->end()) {
        return true;
      }
    }
    return false;
  }
  void Add(const std::string& name) {
    if (!scopes.empty()) {
      scopes.back().insert(name);
    }
  }
  void AddAll(const std::vector<std::string>& names) {
    for (const auto& name : names) {
      Add(name);
    }
  }
};

static std::optional<std::string> PlaceRootName(const syntax::ExprPtr& expr) {
  if (!expr) {
    return std::nullopt;
  }
  return std::visit(
      [&](const auto& node) -> std::optional<std::string> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return node.name;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return PlaceRootName(node.base);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return PlaceRootName(node.base);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return PlaceRootName(node.base);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return PlaceRootName(node.value);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          return PlaceRootName(node.place);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          return PlaceRootName(node.place);
        }
        return std::nullopt;
      },
      expr->node);
}

struct CaptureCollector {
  LowerCtx& ctx;
  const std::unordered_set<std::string>& explicit_moves;
  std::unordered_map<std::string, CaptureBinding> captures;
  std::vector<std::string> order;
  ScopedNames locals;

  void RecordCapture(std::string_view name) {
    const std::string key(name);
    if (locals.IsLocal(key)) {
      return;
    }
    const auto* binding = ctx.GetBindingState(key);
    if (!binding || !binding->type) {
      return;
    }
    if (captures.find(key) != captures.end()) {
      return;
    }
    CaptureBinding entry;
    entry.name = key;
    entry.type = binding->type;
    entry.explicit_move = explicit_moves.find(key) != explicit_moves.end();
    captures.emplace(key, entry);
    order.push_back(key);
  }

  void VisitExpr(const syntax::ExprPtr& expr) {
    if (!expr) {
      return;
    }
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
            RecordCapture(node.name);
          } else if constexpr (std::is_same_v<T, syntax::AttributedExpr>) {
            VisitExpr(node.expr);
          } else if constexpr (std::is_same_v<T, syntax::QualifiedApplyExpr>) {
            if (std::holds_alternative<syntax::ParenArgs>(node.args)) {
              const auto& args = std::get<syntax::ParenArgs>(node.args).args;
              for (const auto& arg : args) {
                VisitExpr(arg.value);
              }
            } else {
              const auto& fields = std::get<syntax::BraceArgs>(node.args).fields;
              for (const auto& field : fields) {
                VisitExpr(field.value);
              }
            }
          } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
            VisitExpr(node.lhs);
            VisitExpr(node.rhs);
          } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
            VisitExpr(node.lhs);
            VisitExpr(node.rhs);
          } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
            VisitExpr(node.place);
          } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
            VisitExpr(node.place);
          } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
            for (const auto& elem : node.elements) {
              VisitExpr(elem);
            }
          } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
            for (const auto& elem : node.elements) {
              VisitExpr(elem);
            }
          } else if constexpr (std::is_same_v<T, syntax::ArrayRepeatExpr>) {
            VisitExpr(node.value);
            VisitExpr(node.count);
          } else if constexpr (std::is_same_v<T, syntax::SizeofExpr>) {
            // No sub-expressions to visit (type is not a runtime expression)
          } else if constexpr (std::is_same_v<T, syntax::AlignofExpr>) {
            // No sub-expressions to visit (type is not a runtime expression)
          } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
            for (const auto& field : node.fields) {
              VisitExpr(field.value);
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
                      VisitExpr(elem);
                    }
                  } else {
                    for (const auto& field : payload.fields) {
                      VisitExpr(field.value);
                    }
                  }
                },
                *node.payload_opt);
          } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
            VisitExpr(node.cond);
            VisitExpr(node.then_expr);
            VisitExpr(node.else_expr);
          } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
            VisitExpr(node.value);
            for (const auto& arm : node.arms) {
              locals.Push();
              if (arm.pattern) {
                std::vector<std::string> names;
                CollectPatternNames(*arm.pattern, names);
                locals.AddAll(names);
              }
              VisitExpr(arm.guard_opt);
              VisitExpr(arm.body);
              locals.Pop();
            }
          } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
            VisitExpr(node.cond);
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
            VisitExpr(node.iter);
            locals.Push();
            if (node.pattern) {
              std::vector<std::string> names;
              CollectPatternNames(*node.pattern, names);
              locals.AddAll(names);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
            locals.Pop();
          } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
            if (node.block) {
              VisitBlock(*node.block);
            }
          } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
            if (node.block) {
              VisitBlock(*node.block);
            }
          } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
            VisitExpr(node.base);
          } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
            VisitExpr(node.base);
          } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
            VisitExpr(node.base);
            VisitExpr(node.index);
          } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
            VisitExpr(node.callee);
            for (const auto& arg : node.args) {
              VisitExpr(arg.value);
            }
          } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
            VisitExpr(node.receiver);
            for (const auto& arg : node.args) {
              VisitExpr(arg.value);
            }
          } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, syntax::EntryExpr>) {
            VisitExpr(node.expr);
          } else if constexpr (std::is_same_v<T, syntax::ParallelExpr>) {
            VisitExpr(node.domain);
            for (const auto& opt : node.opts) {
              VisitExpr(opt.value);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::SpawnExpr>) {
            for (const auto& opt : node.opts) {
              VisitExpr(opt.value);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::WaitExpr>) {
            VisitExpr(node.handle);
          } else if constexpr (std::is_same_v<T, syntax::DispatchExpr>) {
            VisitExpr(node.range);
            if (node.key_clause.has_value()) {
              RecordCapture(node.key_clause->key_path.root);
              for (const auto& seg : node.key_clause->key_path.segs) {
                if (const auto* idx = std::get_if<syntax::KeySegIndex>(&seg)) {
                  VisitExpr(idx->expr);
                }
              }
            }
            for (const auto& opt : node.opts) {
              VisitExpr(opt.chunk_expr);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
          }
        },
        expr->node);
  }

  void VisitStmt(const syntax::Stmt& stmt) {
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::LetStmt> ||
                        std::is_same_v<T, syntax::VarStmt>) {
            VisitExpr(node.binding.init);
            if (node.binding.pat) {
              std::vector<std::string> names;
              CollectPatternNames(*node.binding.pat, names);
              locals.AddAll(names);
            }
          } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt> ||
                               std::is_same_v<T, syntax::ShadowVarStmt>) {
            VisitExpr(node.init);
            locals.Add(node.name);
          } else if constexpr (std::is_same_v<T, syntax::AssignStmt>) {
            VisitExpr(node.place);
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, syntax::CompoundAssignStmt>) {
            VisitExpr(node.place);
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, syntax::ExprStmt>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, syntax::DeferStmt>) {
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::RegionStmt>) {
            VisitExpr(node.opts_opt);
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::FrameStmt>) {
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::ReturnStmt>) {
            VisitExpr(node.value_opt);
          } else if constexpr (std::is_same_v<T, syntax::BreakStmt>) {
            VisitExpr(node.value_opt);
          } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockStmt>) {
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::KeyBlockStmt>) {
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::StaticAssertStmt>) {
            VisitExpr(node.condition);
          }
        },
        stmt);
  }

  void VisitBlock(const syntax::Block& block) {
    locals.Push();
    for (const auto& stmt : block.stmts) {
      VisitStmt(stmt);
    }
    VisitExpr(block.tail_opt);
    locals.Pop();
  }
};

static std::vector<CaptureBinding> CollectCaptures(
    const syntax::Block& body,
    LowerCtx& ctx,
    const std::unordered_set<std::string>& explicit_moves) {
  CaptureCollector collector{ctx, explicit_moves, {}, {}, {}};
  collector.VisitBlock(body);
  std::vector<CaptureBinding> result;
  result.reserve(collector.order.size());
  for (const auto& key : collector.order) {
    result.push_back(collector.captures.at(key));
  }
  return result;
}

static std::unordered_set<std::string> CollectSpawnMoveCaptures(
    const syntax::SpawnExpr& expr) {
  std::unordered_set<std::string> moves;
  for (const auto& opt : expr.opts) {
    if (opt.kind != syntax::SpawnOptionKind::MoveCapture) {
      continue;
    }
    const auto root = PlaceRootName(opt.value);
    if (root.has_value()) {
      moves.insert(*root);
    }
  }
  return moves;
}

static void MergeFailures(LowerCtx& base, const LowerCtx& branch) {
  if (branch.resolve_failed) {
    base.resolve_failed = true;
  }
  if (branch.codegen_failed) {
    base.codegen_failed = true;
  }
  for (const auto& name : branch.resolve_failures) {
    if (std::find(base.resolve_failures.begin(), base.resolve_failures.end(), name) ==
        base.resolve_failures.end()) {
      base.resolve_failures.push_back(name);
    }
  }
}

static void MergeMoveStates(LowerCtx& base,
                            const std::vector<const LowerCtx*>& branches) {
  for (auto& [name, stack] : base.binding_states) {
    if (stack.empty()) {
      continue;
    }
    auto& state = stack.back();

    bool moved_any = state.is_moved;
    std::set<std::string> fields;
    if (!moved_any) {
      fields.insert(state.moved_fields.begin(), state.moved_fields.end());
    }

    for (const auto* branch : branches) {
      if (!branch) {
        continue;
      }
      const BindingState* bstate = branch->GetBindingState(name);
      if (!bstate) {
        continue;
      }
      if (bstate->is_moved) {
        moved_any = true;
      } else if (!moved_any) {
        fields.insert(bstate->moved_fields.begin(), bstate->moved_fields.end());
      }
    }

    if (moved_any) {
      state.is_moved = true;
      state.moved_fields.clear();
    } else {
      state.is_moved = false;
      state.moved_fields.assign(fields.begin(), fields.end());
    }
  }
}

struct LowerCtxSnapshot {
  std::vector<ScopeInfo> scope_stack;
  std::unordered_map<std::string, std::vector<BindingState>> binding_states;
  std::unordered_map<std::string, DerivedValueInfo> derived_values;
  std::vector<TempValue>* temp_sink = nullptr;
  int temp_depth = 0;
  std::optional<int> suppress_temp_at_depth;
  std::vector<ParallelCollectItem>* parallel_collect = nullptr;
  int parallel_collect_depth = 0;
  std::optional<CaptureEnvInfo> capture_env;
  analysis::TypeRef proc_ret_type;

  explicit LowerCtxSnapshot(const LowerCtx& ctx)
      : scope_stack(ctx.scope_stack),
        binding_states(ctx.binding_states),
        derived_values(ctx.derived_values),
        temp_sink(ctx.temp_sink),
        temp_depth(ctx.temp_depth),
        suppress_temp_at_depth(ctx.suppress_temp_at_depth),
        parallel_collect(ctx.parallel_collect),
        parallel_collect_depth(ctx.parallel_collect_depth),
        capture_env(ctx.capture_env),
        proc_ret_type(ctx.proc_ret_type) {}

  void Restore(LowerCtx& ctx) const {
    ctx.scope_stack = scope_stack;
    ctx.binding_states = binding_states;
    // Merge derived_values: preserve new values created during the nested
    // lowering (e.g., capture_ptr_N) while restoring values from the snapshot.
    // New derived values are referenced from generated IR and must be preserved.
    for (const auto& [key, value] : derived_values) {
      ctx.derived_values[key] = value;
    }
    ctx.temp_sink = temp_sink;
    ctx.temp_depth = temp_depth;
    ctx.suppress_temp_at_depth = suppress_temp_at_depth;
    ctx.parallel_collect = parallel_collect;
    ctx.parallel_collect_depth = parallel_collect_depth;
    ctx.capture_env = capture_env;
    ctx.proc_ret_type = proc_ret_type;
  }
};

}  // namespace

// Helpers moved to lower_expr.h / ir_model.cpp

// ============================================================================
// §6.4 LowerList - Lower a list of expressions in LTR order
// ============================================================================

std::pair<IRPtr, std::vector<IRValue>> LowerList(
    const std::vector<syntax::ExprPtr>& exprs, LowerCtx& ctx) {
  SPEC_RULE("LowerList-Empty");
  SPEC_RULE("LowerList-Cons");
  
  if (exprs.empty()) {
    return {EmptyIR(), {}};
  }
  
  std::vector<IRPtr> ir_parts;
  std::vector<IRValue> values;
  
  for (const auto& expr : exprs) {
    // Element values are consumed by the parent aggregate (tuple/array), so
    // they should not be tracked as temporaries requiring cleanup.
    auto prev_suppress = ctx.suppress_temp_at_depth;
    ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
    auto result = LowerExpr(*expr, ctx);
    ctx.suppress_temp_at_depth = prev_suppress;
    ir_parts.push_back(result.ir);
    values.push_back(result.value);
  }
  
  return {SeqIR(std::move(ir_parts)), std::move(values)};
}

// ============================================================================
// §6.4 LowerFieldInits - Lower field initializers in LTR order
// ============================================================================

std::pair<IRPtr, std::vector<std::pair<std::string, IRValue>>> LowerFieldInits(
    const std::vector<syntax::FieldInit>& fields, LowerCtx& ctx, bool suppress_temps) {
  SPEC_RULE("LowerFieldInits-Empty");
  SPEC_RULE("LowerFieldInits-Cons");
  
  if (fields.empty()) {
    return {EmptyIR(), {}};
  }
  
  std::vector<IRPtr> ir_parts;
  std::vector<std::pair<std::string, IRValue>> field_values;
  
  for (const auto& field : fields) {
    auto prev_suppress = ctx.suppress_temp_at_depth;
    if (suppress_temps) {
      ctx.suppress_temp_at_depth = ctx.temp_depth + 1;
    }
    auto result = LowerExpr(*field.value, ctx);
    ctx.suppress_temp_at_depth = prev_suppress;
    ir_parts.push_back(result.ir);
    field_values.emplace_back(field.name, result.value);
  }
  
  return {SeqIR(std::move(ir_parts)), std::move(field_values)};
}

// ============================================================================
// §6.4 LowerOpt - Lower optional expression
// ============================================================================

std::pair<IRPtr, std::optional<IRValue>> LowerOpt(
    const syntax::ExprPtr& expr_opt, LowerCtx& ctx) {
  SPEC_RULE("LowerOpt-None");
  SPEC_RULE("LowerOpt-Some");
  
  if (!expr_opt) {
    return {EmptyIR(), std::nullopt};
  }
  
  auto result = LowerExpr(*expr_opt, ctx);
  return {result.ir, result.value};
}

// ============================================================================
// §6.4 Children_LTR - Evaluation order helpers
// ============================================================================

std::vector<syntax::ExprPtr> ChildrenLTR(const syntax::Expr& expr) {
  return std::visit(
      [](const auto& node) -> std::vector<syntax::ExprPtr> {
        using T = std::decay_t<decltype(node)>;
        
        if constexpr (std::is_same_v<T, syntax::ErrorExpr>) {
          return {};
        } else if constexpr (std::is_same_v<T, syntax::LiteralExpr>) {
          SPEC_RULE("EvalOrder-Literal");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          SPEC_RULE("EvalOrder-Ident");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          SPEC_RULE("EvalOrder-Path");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::PtrNullExpr>) {
          SPEC_RULE("EvalOrder-PtrNull");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          SPEC_RULE("EvalOrder-Tuple");
          return node.elements;
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          SPEC_RULE("EvalOrder-Array");
          return node.elements;
        } else if constexpr (std::is_same_v<T, syntax::ArrayRepeatExpr>) {
          SPEC_RULE("EvalOrder-ArrayRepeat");
          return {node.value, node.count};
        } else if constexpr (std::is_same_v<T, syntax::SizeofExpr>) {
          SPEC_RULE("EvalOrder-Sizeof");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::AlignofExpr>) {
          SPEC_RULE("EvalOrder-Alignof");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          SPEC_RULE("EvalOrder-Record");
          return FieldExprs(node.fields);
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          if (!node.payload_opt.has_value()) {
            SPEC_RULE("EvalOrder-Enum-Unit");
            return {};
          }
          return std::visit(
              [](const auto& payload) -> std::vector<syntax::ExprPtr> {
                using P = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<P, syntax::EnumPayloadParen>) {
                  SPEC_RULE("EvalOrder-Enum-Tuple");
                  return payload.elements;
                } else {
                  SPEC_RULE("EvalOrder-Enum-Record");
                  return FieldExprs(payload.fields);
                }
              },
              *node.payload_opt);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          SPEC_RULE("EvalOrder-FieldAccess");
          return {node.base};
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          SPEC_RULE("EvalOrder-TupleAccess");
          return {node.base};
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          SPEC_RULE("EvalOrder-IndexAccess");
          return {node.base, node.index};
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          SPEC_RULE("EvalOrder-Call");
          auto result = ArgsExprs(node.args);
          result.insert(result.begin(), node.callee);
          return result;
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          SPEC_RULE("EvalOrder-MethodCall");
          auto result = ArgsExprs(node.args);
          result.insert(result.begin(), node.receiver);
          return result;
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          SPEC_RULE("EvalOrder-Unary");
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          SPEC_RULE("EvalOrder-Binary");
          return {node.lhs, node.rhs};
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          SPEC_RULE("EvalOrder-Cast");
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          SPEC_RULE("EvalOrder-Transmute");
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          SPEC_RULE("EvalOrder-Propagate");
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          SPEC_RULE("EvalOrder-Range");
          return OptExprs(node.lhs, node.rhs);
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          SPEC_RULE("EvalOrder-If");
          return {node.cond};
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          SPEC_RULE("EvalOrder-Match");
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          SPEC_RULE("EvalOrder-Loop");
          return {WrapBlockExpr(node.body)};
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          SPEC_RULE("EvalOrder-Loop");
          return {node.cond, WrapBlockExpr(node.body)};
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          SPEC_RULE("EvalOrder-Loop");
          return {node.iter, WrapBlockExpr(node.body)};
        } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
          SPEC_RULE("EvalOrder-Block");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
          SPEC_RULE("EvalOrder-UnsafeBlock");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          SPEC_RULE("EvalOrder-Move");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          SPEC_RULE("EvalOrder-AddressOf");
          return {};
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          SPEC_RULE("EvalOrder-Deref");
          return {node.value};
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          SPEC_RULE("EvalOrder-Alloc");
          return {node.value};
        } else {
          return {};
        }
      },
      expr.node);
}

// ============================================================================
// §6.4 Core Expression Lowering
// ============================================================================

// Forward declarations for specialized lowering functions
LowerResult LowerLiteral(const syntax::Expr& expr, const syntax::LiteralExpr& lit, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Literal");

  IRValue value;
  value.kind = IRValue::Kind::Immediate;
  value.name = lit.literal.lexeme;

  if (lit.literal.kind == syntax::TokenKind::StringLiteral) {
    SPEC_RULE("StringLiteralVal");
    if (auto decoded = DecodeStringLiteralBytes(lit.literal.lexeme)) {
      value.bytes = std::move(*decoded);
    } else {
      ctx.ReportCodegenFailure();
    }
  }

  if (ctx.expr_type) {
    const auto lit_type = ctx.expr_type(expr);
    if (lit_type) {
      if (auto bytes = EncodeConst(lit_type, lit.literal)) {
        value.bytes = std::move(*bytes);
      }
    }
  }

  if (value.bytes.empty() && lit.literal.kind == syntax::TokenKind::IntLiteral) {
    if (auto parsed = ParseIntLiteralLexeme(lit.literal.lexeme)) {
      value.bytes = EncodeU64BE(*parsed);
    }
  } else if (value.bytes.empty() && lit.literal.kind == syntax::TokenKind::BoolLiteral) {
    value.bytes = {static_cast<std::uint8_t>(lit.literal.lexeme == "true" ? 1 : 0)};
  } else if (value.bytes.empty() && lit.literal.kind == syntax::TokenKind::NullLiteral) {
    value.bytes = {0};
  }

  return LowerResult{EmptyIR(), value};
}

// §6.4 Lower-Expr-Ident-Local / Lower-Expr-Ident-Path / Lower-Expr-Ident-Path
LowerResult LowerIdentifier(const syntax::IdentifierExpr& expr, LowerCtx& ctx) {
  const std::string& name = expr.name;  // Identifier is std::string

  if (ctx.GetBindingState(name)) {
    // Local variable
    SPEC_RULE("Lower-Expr-Ident-Local");

    IRReadVar read;
    read.name = name;

    IRValue value;
    value.kind = IRValue::Kind::Local;
    value.name = name;

    return LowerResult{MakeIR(std::move(read)), value};
  }

  if (const auto* capture = ctx.LookupCapture(name)) {
    SPEC_RULE("Lower-Expr-Ident-Capture");
    IRPtr ir = EmptyIR();
    IRValue field_ptr = ctx.CaptureFieldPtr(*capture);
    IRValue value = ctx.FreshTempValue("capture_val");
    if (capture->by_ref) {
      IRValue captured_ptr = ctx.FreshTempValue("capture_ptr");
      IRReadPtr load_ptr;
      load_ptr.ptr = field_ptr;
      load_ptr.result = captured_ptr;
      ctx.RegisterValueType(captured_ptr, capture->field_type);
      IRReadPtr load_val;
      load_val.ptr = captured_ptr;
      load_val.result = value;
      ir = SeqIR({MakeIR(std::move(load_ptr)), MakeIR(std::move(load_val))});
    } else {
      IRReadPtr load_val;
      load_val.ptr = field_ptr;
      load_val.result = value;
      ir = MakeIR(std::move(load_val));
    }
    ctx.RegisterValueType(value, capture->value_type);
    return LowerResult{ir, value};
  }

  if (!ctx.resolve_name) {
    ctx.ReportResolveFailure(name);
    SPEC_RULE("Lower-Expr-Ident-Path");

    IRReadPath read;
    read.path = ctx.module_path;
    read.name = name;

    IRValue value;
    value.kind = IRValue::Kind::Symbol;
    value.name = name;

    return LowerResult{MakeIR(std::move(read)), value};
  }

  auto resolved = ctx.resolve_name(name);

  if (!resolved.has_value() || resolved->empty()) {
    ctx.ReportResolveFailure(name);
    SPEC_RULE("Lower-Expr-Ident-Path");

    IRReadPath read;
    read.path = ctx.module_path;
    read.name = name;

    IRValue value;
    value.kind = IRValue::Kind::Symbol;
    value.name = name;

    return LowerResult{MakeIR(std::move(read)), value};
  }

  // Path (imported/global name)
  SPEC_RULE("Lower-Expr-Ident-Path");

  std::vector<std::string> full = *resolved;
  const std::string resolved_name = full.back();
  full.pop_back();

  IRReadPath read;
  read.path = std::move(full);
  read.name = resolved_name;

  IRValue value;
  value.kind = IRValue::Kind::Symbol;
  value.name = resolved_name;

  return LowerResult{MakeIR(std::move(read)), value};
}

// §6.4 Lower-Expr-Path
LowerResult LowerPath(const syntax::PathExpr& expr, LowerCtx& /*ctx*/) {
  SPEC_RULE("Lower-Expr-Path");
  
  IRReadPath read;
  // Path is std::vector<Identifier> where Identifier is std::string
  read.path = expr.path;
  read.name = expr.name;
  
  IRValue value;
  value.kind = IRValue::Kind::Symbol;
  value.name = expr.name;
  
  return LowerResult{MakeIR(std::move(read)), value};
}

// §6.4 Lower-Expr-PtrNull
LowerResult LowerPtrNull(const syntax::PtrNullExpr& /*expr*/, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-PtrNull");
  
  IRValue value;
  value.kind = IRValue::Kind::Immediate;
  value.name = "null";
  // Null pointer is represented as 0x0
  value.bytes = {0, 0, 0, 0, 0, 0, 0, 0};  // 64-bit null
  
  return LowerResult{EmptyIR(), value};
}

// §6.4 Lower-Expr-Error
LowerResult LowerError(const syntax::ErrorExpr& /*expr*/, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Error");
  
  // Error expressions lower to a panic
  IRValue value = ctx.FreshTempValue("unreachable");
  
  return LowerResult{LowerPanic(PanicReason::ErrorExpr, ctx), value};
}

// §6.4 Lower-Expr-Tuple
LowerResult LowerTuple(const syntax::TupleExpr& expr, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Tuple");
  
  auto [ir, values] = LowerList(expr.elements, ctx);
  
  IRValue tuple_value = ctx.FreshTempValue("tuple");
  DerivedValueInfo info;
  info.kind = DerivedValueInfo::Kind::TupleLit;
  info.elements = values;
  ctx.RegisterDerivedValue(tuple_value, info);
  
  return LowerResult{ir, tuple_value};
}

// §6.4 Lower-Expr-Array
LowerResult LowerArray(const syntax::ArrayExpr& expr, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Array");
  
  auto [ir, values] = LowerList(expr.elements, ctx);
  
  IRValue array_value = ctx.FreshTempValue("array");
  DerivedValueInfo info;
  info.kind = DerivedValueInfo::Kind::ArrayLit;
  info.elements = values;
  ctx.RegisterDerivedValue(array_value, info);
  
  return LowerResult{ir, array_value};
}

// §6.4 Lower-Expr-Record
LowerResult LowerRecord(const syntax::RecordExpr& expr, LowerCtx& ctx) {
  SPEC_RULE("Lower-Expr-Record");
  
  // Field initializer values are consumed by the record construction, so they
  // should not be tracked as temporaries. This applies to both regular records
  // (TypePath) and modal state records (ModalStateRef).
  auto [ir, field_values] = LowerFieldInits(expr.fields, ctx, /*suppress_temps=*/true);
  
  IRValue record_value = ctx.FreshTempValue("record");
  DerivedValueInfo info;
  info.kind = DerivedValueInfo::Kind::RecordLit;
  info.fields = field_values;
  ctx.RegisterDerivedValue(record_value, info);
  
  return LowerResult{ir, record_value};
}

// §6.4 Lower-Expr-Enum-Unit / Lower-Expr-Enum-Tuple / Lower-Expr-Enum-Record
LowerResult LowerEnumLiteral(const syntax::EnumLiteralExpr& expr, LowerCtx& ctx) {
  if (!expr.payload_opt.has_value()) {
    // Unit variant
    SPEC_RULE("Lower-Expr-Enum-Unit");
    
    IRValue enum_value = ctx.FreshTempValue("enum_unit");
    DerivedValueInfo info;
    info.kind = DerivedValueInfo::Kind::EnumLit;
    info.variant = expr.path.empty() ? std::string() : expr.path.back();
    ctx.RegisterDerivedValue(enum_value, info);
    
    return LowerResult{EmptyIR(), enum_value};
  }
  
  return std::visit(
      [&ctx, &expr](const auto& payload) -> LowerResult {
        using T = std::decay_t<decltype(payload)>;
        
        if constexpr (std::is_same_v<T, syntax::EnumPayloadParen>) {
          // Tuple variant
          SPEC_RULE("Lower-Expr-Enum-Tuple");
          
          auto [ir, values] = LowerList(payload.elements, ctx);
          
          IRValue enum_value = ctx.FreshTempValue("enum_tuple");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::EnumLit;
          info.variant = expr.path.empty() ? std::string() : expr.path.back();
          info.payload_elems = values;
          ctx.RegisterDerivedValue(enum_value, info);
          
          return LowerResult{ir, enum_value};
        } else {
          // Record variant
          SPEC_RULE("Lower-Expr-Enum-Record");
          
          auto [ir, field_values] = LowerFieldInits(payload.fields, ctx, true);
          
          IRValue enum_value = ctx.FreshTempValue("enum_record");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::EnumLit;
          info.variant = expr.path.empty() ? std::string() : expr.path.back();
          info.payload_fields = field_values;
          ctx.RegisterDerivedValue(enum_value, info);
          
          return LowerResult{ir, enum_value};
        }
      },
      *expr.payload_opt);
}

// ============================================================================
// Anchor function for SPEC_RULE coverage
// ============================================================================

void AnchorExprLoweringRules() {
  // §6.4 Evaluation Order
  SPEC_RULE("EvalOrder-Literal");
  SPEC_RULE("EvalOrder-PtrNull");
  SPEC_RULE("EvalOrder-Ident");
  SPEC_RULE("EvalOrder-Path");
  SPEC_RULE("EvalOrder-Tuple");
  SPEC_RULE("EvalOrder-Array");
  SPEC_RULE("EvalOrder-Record");
  SPEC_RULE("EvalOrder-Enum-Unit");
  SPEC_RULE("EvalOrder-Enum-Tuple");
  SPEC_RULE("EvalOrder-Enum-Record");
  SPEC_RULE("EvalOrder-FieldAccess");
  SPEC_RULE("EvalOrder-TupleAccess");
  SPEC_RULE("EvalOrder-IndexAccess");
  SPEC_RULE("EvalOrder-Call");
  SPEC_RULE("EvalOrder-MethodCall");
  SPEC_RULE("EvalOrder-Unary");
  SPEC_RULE("EvalOrder-Binary");
  SPEC_RULE("EvalOrder-Cast");
  SPEC_RULE("EvalOrder-Transmute");
  SPEC_RULE("EvalOrder-Propagate");
  SPEC_RULE("EvalOrder-Range");
  SPEC_RULE("EvalOrder-If");
  SPEC_RULE("EvalOrder-Match");
  SPEC_RULE("EvalOrder-Loop");
  SPEC_RULE("EvalOrder-Block");
  SPEC_RULE("EvalOrder-UnsafeBlock");
  SPEC_RULE("EvalOrder-Move");
  SPEC_RULE("EvalOrder-AddressOf");
  SPEC_RULE("EvalOrder-Deref");
  SPEC_RULE("EvalOrder-Alloc");

  // §6.4 Eval helpers
  SPEC_RULE("ArgsExprs-Empty");
  SPEC_RULE("ArgsExprs-Cons");
  SPEC_RULE("FieldExprs-Empty");
  SPEC_RULE("FieldExprs-Cons");
  SPEC_RULE("OptExprs-None");
  SPEC_RULE("OptExprs-Lo");
  SPEC_RULE("OptExprs-Hi");
  SPEC_RULE("OptExprs-Both");

  // §6.4 Expression Lowering
  SPEC_RULE("Lower-Expr-Correctness");
  SPEC_RULE("Lower-Expr-Literal");
  SPEC_RULE("Lower-Expr-PtrNull");
  SPEC_RULE("Lower-Expr-Ident-Local");
  SPEC_RULE("Lower-Expr-Ident-Path");
  SPEC_RULE("Lower-Expr-Path");
  SPEC_RULE("Lower-Expr-Error");
  SPEC_RULE("Lower-Expr-Tuple");
  SPEC_RULE("Lower-Expr-Array");
  SPEC_RULE("Lower-Expr-Record");
  SPEC_RULE("Lower-Expr-Enum-Unit");
  SPEC_RULE("Lower-Expr-Enum-Tuple");
  SPEC_RULE("Lower-Expr-Enum-Record");
  SPEC_RULE("Lower-Expr-FieldAccess");
  SPEC_RULE("Lower-Expr-TupleAccess");
  SPEC_RULE("Lower-Expr-Index-Scalar");
  SPEC_RULE("Lower-Expr-Index-Scalar-OOB");
  SPEC_RULE("Lower-Expr-Index-Range");
  SPEC_RULE("Lower-Expr-Index-Range-OOB");
  SPEC_RULE("Lower-Expr-MethodCall");
  SPEC_RULE("Lower-Expr-Unary");
  SPEC_RULE("Lower-Expr-Binary");
  SPEC_RULE("Lower-Expr-Cast");
  SPEC_RULE("Lower-Expr-Transmute");
  SPEC_RULE("Lower-Expr-Range");
  SPEC_RULE("Lower-Expr-If");
  SPEC_RULE("Lower-Expr-Match");
  SPEC_RULE("Lower-Expr-LoopInf");
  SPEC_RULE("Lower-Expr-LoopCond");
  SPEC_RULE("Lower-Expr-LoopIter");
  SPEC_RULE("Lower-Expr-Block");
  SPEC_RULE("Lower-Expr-UnsafeBlock");
  SPEC_RULE("Lower-Expr-Move");
  SPEC_RULE("Lower-Expr-AddressOf");
  SPEC_RULE("Lower-Expr-Deref");
  SPEC_RULE("Lower-Expr-Alloc");

  // §6.4 List Lowering
  SPEC_RULE("LowerList-Empty");
  SPEC_RULE("LowerList-Cons");
  SPEC_RULE("LowerFieldInits-Empty");
  SPEC_RULE("LowerFieldInits-Cons");
  SPEC_RULE("LowerOpt-None");
  SPEC_RULE("LowerOpt-Some");
}


// ============================================================================
// §6.4 Main LowerExpr Dispatcher
// ============================================================================

// Forward declarations for expression handlers defined in other files
LowerResult LowerCallExpr(const syntax::CallExpr& expr, LowerCtx& ctx);
LowerResult LowerBinaryExpr(const syntax::BinaryExpr& expr, LowerCtx& ctx);
LowerResult LowerCastExpr(const syntax::CastExpr& expr, LowerCtx& ctx);
LowerResult LowerPropagateExpr(const syntax::PropagateExpr& expr, LowerCtx& ctx);
LowerResult LowerIfExpr(const syntax::IfExpr& expr, LowerCtx& ctx);

// §6.4 LowerExpr - main expression lowering dispatcher
static LowerResult LowerExprImpl(const syntax::Expr& expr, LowerCtx& ctx) {
  return std::visit(
      [&ctx, &expr](const auto& node) -> LowerResult {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, syntax::ErrorExpr>) {
          return LowerError(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::AttributedExpr>) {
          const bool prev_dynamic = ctx.dynamic_checks;
          if (HasDynamicAttr(node.attrs)) {
            ctx.dynamic_checks = true;
          }
          LowerResult out = node.expr ? LowerExpr(*node.expr, ctx)
                                      : LowerResult{};
          ctx.dynamic_checks = prev_dynamic;
          return out;
        } else if constexpr (std::is_same_v<T, syntax::LiteralExpr>) {
          return LowerLiteral(expr, node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return LowerIdentifier(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::PathExpr>) {
          return LowerPath(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::PtrNullExpr>) {
          return LowerPtrNull(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::TupleExpr>) {
          return LowerTuple(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::ArrayExpr>) {
          return LowerArray(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::ArrayRepeatExpr>) {
          SPEC_RULE("Lower-Expr-ArrayRepeat");
          // Lower value and count expressions
          auto value_result = LowerExpr(*node.value, ctx);
          auto count_result = LowerExpr(*node.count, ctx);
          // Build array with repeated value
          IRValue array_value = ctx.FreshTempValue("array_repeat");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::ArrayRepeat;
          info.repeat_value = value_result.value;
          info.repeat_count = count_result.value;
          ctx.RegisterDerivedValue(array_value, info);
          return LowerResult{SeqIR({value_result.ir, count_result.ir}), array_value};
        } else if constexpr (std::is_same_v<T, syntax::SizeofExpr>) {
          SPEC_RULE("Lower-Expr-Sizeof");
          if (!ctx.sigma) {
            ctx.ReportCodegenFailure();
            return LowerResult{EmptyIR(), IRValue{}};
          }
          analysis::ScopeContext scope;
          scope.sigma = *ctx.sigma;
          scope.current_module = ctx.module_path;
          auto lowered = LowerTypeForLayout(scope, node.type);
          if (!lowered) {
            ctx.ReportCodegenFailure();
            return LowerResult{EmptyIR(), IRValue{}};
          }
          auto layout = LayoutOf(scope, *lowered);
          if (!layout) {
            ctx.ReportCodegenFailure();
            return LowerResult{EmptyIR(), IRValue{}};
          }
          IRValue value;
          value.kind = IRValue::Kind::Immediate;
          value.name = std::to_string(layout->size);
          value.bytes = EncodeU64BE(layout->size);
          return LowerResult{EmptyIR(), value};
        } else if constexpr (std::is_same_v<T, syntax::AlignofExpr>) {
          SPEC_RULE("Lower-Expr-Alignof");
          if (!ctx.sigma) {
            ctx.ReportCodegenFailure();
            return LowerResult{EmptyIR(), IRValue{}};
          }
          analysis::ScopeContext scope;
          scope.sigma = *ctx.sigma;
          scope.current_module = ctx.module_path;
          auto lowered = LowerTypeForLayout(scope, node.type);
          if (!lowered) {
            ctx.ReportCodegenFailure();
            return LowerResult{EmptyIR(), IRValue{}};
          }
          auto layout = LayoutOf(scope, *lowered);
          if (!layout) {
            ctx.ReportCodegenFailure();
            return LowerResult{EmptyIR(), IRValue{}};
          }
          IRValue value;
          value.kind = IRValue::Kind::Immediate;
          value.name = std::to_string(layout->align);
          value.bytes = EncodeU64BE(layout->align);
          return LowerResult{EmptyIR(), value};
        } else if constexpr (std::is_same_v<T, syntax::RecordExpr>) {
          return LowerRecord(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::EnumLiteralExpr>) {
          return LowerEnumLiteral(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          SPEC_RULE("Lower-Expr-FieldAccess");
          auto base_result = LowerExpr(*node.base, ctx);
          IRValue field_value = ctx.FreshTempValue("field");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::Field;
          info.base = base_result.value;
          info.field = node.name;
          ctx.RegisterDerivedValue(field_value, info);
          return LowerResult{base_result.ir, field_value};
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          SPEC_RULE("Lower-Expr-TupleAccess");
          auto base_result = LowerExpr(*node.base, ctx);
          IRValue elem_value = ctx.FreshTempValue("tuple_elem");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::Tuple;
          info.base = base_result.value;
          info.tuple_index = static_cast<std::size_t>(std::stoull(node.index.lexeme));
          ctx.RegisterDerivedValue(elem_value, info);
          return LowerResult{base_result.ir, elem_value};
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          auto base_result = LowerExpr(*node.base, ctx);
          if (std::holds_alternative<syntax::RangeExpr>(node.index->node)) {
            SPEC_RULE("Lower-Expr-Index-Range");
            const auto& range_node = std::get<syntax::RangeExpr>(node.index->node);
            auto range_result = LowerRangeExpr(range_node, ctx);
            IRCheckRange check;
            check.base = base_result.value;
            check.range = ToIRRange(range_result.value);
            IRValue slice_value = ctx.FreshTempValue("slice");
            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::Slice;
            info.base = base_result.value;
            info.range = ToIRRange(range_result.value);
            ctx.RegisterDerivedValue(slice_value, info);
            return LowerResult{SeqIR({base_result.ir, range_result.ir, MakeIR(std::move(check)),
                                      PanicCheck(ctx)}),
                               slice_value};
          }
          SPEC_RULE("Lower-Expr-Index-Scalar");
          auto index_result = LowerExpr(*node.index, ctx);
          const bool needs_check = NeedsIndexCheck(*node.base, ctx);
          IRCheckIndex check;
          check.base = base_result.value;
          check.index = index_result.value;
          IRValue elem_value = ctx.FreshTempValue("index_elem");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::Index;
          info.base = base_result.value;
          info.index = index_result.value;
          ctx.RegisterDerivedValue(elem_value, info);
          std::vector<IRPtr> seq;
          seq.push_back(base_result.ir);
          seq.push_back(index_result.ir);
          if (needs_check) {
            seq.push_back(MakeIR(std::move(check)));
            seq.push_back(PanicCheck(ctx));
          }
          return LowerResult{SeqIR(std::move(seq)), elem_value};
        } else if constexpr (std::is_same_v<T, syntax::CallExpr>) {
          return LowerCallExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::MethodCallExpr>) {
          return LowerMethodCall(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::UnaryExpr>) {
          SPEC_RULE("Lower-Expr-Unary");
          return LowerUnOp(node.op, *node.value, ctx);
        } else if constexpr (std::is_same_v<T, syntax::BinaryExpr>) {
          return LowerBinaryExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::CastExpr>) {
          return LowerCastExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::TransmuteExpr>) {
          SPEC_RULE("Lower-Expr-Transmute");
          analysis::TypeRef to_type;
          analysis::TypeRef from_type;
          if (ctx.sigma) {
            analysis::ScopeContext scope;
            scope.sigma = *ctx.sigma;
            scope.current_module = ctx.module_path;
            if (node.to) {
              if (auto lowered = LowerTypeForLayout(scope, node.to)) {
                to_type = *lowered;
              }
            }
            if (node.from) {
              if (auto lowered = LowerTypeForLayout(scope, node.from)) {
                from_type = *lowered;
              }
            }
          }
          if (!to_type && ctx.expr_type) {
            to_type = ctx.expr_type(expr);
          }
          if (!from_type && ctx.expr_type) {
            from_type = ctx.expr_type(*node.value);
          }
          return LowerTransmute(from_type, to_type, *node.value, ctx);
        } else if constexpr (std::is_same_v<T, syntax::PropagateExpr>) {
          return LowerPropagateExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::RangeExpr>) {
          SPEC_RULE("Lower-Expr-Range");
          auto range_result = LowerRangeExpr(node, ctx);
          IRValue range_value = ctx.FreshTempValue("range");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::RangeLit;
          info.range = ToIRRange(range_result.value);
          ctx.RegisterDerivedValue(range_value, info);
          return LowerResult{range_result.ir, range_value};
        } else if constexpr (std::is_same_v<T, syntax::IfExpr>) {
          return LowerIfExpr(node, ctx);
        } else if constexpr (std::is_same_v<T, syntax::MatchExpr>) {
          SPEC_RULE("Lower-Expr-Match");
          return LowerMatch(*node.value, node.arms, ctx);
        } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
          SPEC_RULE("Lower-Expr-LoopInf");
          return LowerLoop(expr, ctx);
        } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
          SPEC_RULE("Lower-Expr-LoopCond");
          return LowerLoop(expr, ctx);
        } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
          SPEC_RULE("Lower-Expr-LoopIter");
          return LowerLoop(expr, ctx);
        } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
          SPEC_RULE("Lower-Expr-Block");
          return LowerBlock(*node.block, ctx);
        } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
          SPEC_RULE("Lower-Expr-UnsafeBlock");
          return LowerBlock(*node.block, ctx);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          SPEC_RULE("Lower-Expr-Move");
          return LowerMovePlace(*node.place, ctx);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          SPEC_RULE("Lower-Expr-AddressOf");
          return LowerAddrOf(*node.place, ctx);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          SPEC_RULE("Lower-Expr-Deref");
          auto ptr_result = LowerExpr(*node.value, ctx);
          analysis::TypeRef ptr_type;
          if (ctx.expr_type) {
            ptr_type = ctx.expr_type(*node.value);
          }
          if (ptr_type) {
            auto deref_result = LowerRawDeref(ptr_result.value, ptr_type, ctx);
            return LowerResult{SeqIR({ptr_result.ir, deref_result.ir}), deref_result.value};
          }
          IRValue value = ctx.FreshTempValue("deref");
          IRReadPtr read;
          read.ptr = ptr_result.value;
          read.result = value;
          return LowerResult{SeqIR({ptr_result.ir, MakeIR(std::move(read))}), value};
        } else if constexpr (std::is_same_v<T, syntax::AllocExpr>) {
          SPEC_RULE("Lower-Expr-Alloc");
          auto value_result = LowerExpr(*node.value, ctx);
          analysis::TypeRef value_type;
          if (ctx.expr_type) {
            value_type = ctx.expr_type(*node.value);
          }
          if (!value_type) {
            value_type = ctx.LookupValueType(value_result.value);
          }
          IRAlloc alloc;
          alloc.value = value_result.value;
          alloc.type = value_type;
          if (node.region_opt) {
            syntax::IdentifierExpr ident;
            ident.name = *node.region_opt;
            syntax::Expr region_expr;
            region_expr.span = expr.span;
            region_expr.node = ident;
            auto region_result = LowerExpr(region_expr, ctx);
            alloc.region = region_result.value;
            IRValue ptr_value = ctx.FreshTempValue("alloc_ptr");
            alloc.result = ptr_value;
            IRValue alloc_val = ctx.FreshTempValue("alloc_val");
            DerivedValueInfo info;
            info.kind = DerivedValueInfo::Kind::LoadFromAddr;
            info.base = ptr_value;
            ctx.RegisterDerivedValue(alloc_val, info);
            if (value_type) {
              ctx.RegisterValueType(alloc_val, value_type);
            }
            return LowerResult{SeqIR({value_result.ir, region_result.ir, MakeIR(std::move(alloc))}),
                               alloc_val};
          }
          IRValue ptr_value = ctx.FreshTempValue("alloc_ptr");
          alloc.result = ptr_value;
          IRValue alloc_val = ctx.FreshTempValue("alloc_val");
          DerivedValueInfo info;
          info.kind = DerivedValueInfo::Kind::LoadFromAddr;
          info.base = ptr_value;
          ctx.RegisterDerivedValue(alloc_val, info);
          if (value_type) {
            ctx.RegisterValueType(alloc_val, value_type);
          }
          return LowerResult{SeqIR({value_result.ir, MakeIR(std::move(alloc))}), alloc_val};
        // C0X Extension: Structured Concurrency (§11.3)
        } else if constexpr (std::is_same_v<T, syntax::ParallelExpr>) {
          SPEC_RULE("Lower-Expr-Parallel");
          // Lower domain expression
          auto domain_result = LowerExpr(*node.domain, ctx);
          // Lower body
          std::vector<ParallelCollectItem> collected;
          auto* prev_collect = ctx.parallel_collect;
          int prev_depth = ctx.parallel_collect_depth;
          bool explicit_result = false;
          if (node.body) {
            if (node.body->tail_opt) {
              bool needs_wait = false;
              if (!IsCollectableParallelExpr(*node.body->tail_opt, needs_wait)) {
                explicit_result = true;
              }
            }
          }
          if (!explicit_result) {
            ctx.parallel_collect = &collected;
            ctx.parallel_collect_depth = 0;
          }
          LowerResult body_result;
          if (node.body) {
            body_result = LowerBlock(*node.body, ctx);
          } else {
            body_result = LowerResult{EmptyIR(), ctx.FreshTempValue("parallel_unit")};
          }
          ctx.parallel_collect = prev_collect;
          ctx.parallel_collect_depth = prev_depth;
          // Create IRParallel
          IRParallel parallel;
          parallel.domain = domain_result.value;
          parallel.body = body_result.ir;
          parallel.result = ctx.FreshTempValue("parallel_join");
          // Handle options (cancel, name)
          for (const auto& opt : node.opts) {
            if (opt.kind == syntax::ParallelOptionKind::Cancel && opt.value) {
              auto cancel_result = LowerExpr(*opt.value, ctx);
              parallel.cancel_token = cancel_result.value;
            } else if (opt.kind == syntax::ParallelOptionKind::Name && opt.value) {
              if (const auto* lit = std::get_if<syntax::LiteralExpr>(&opt.value->node)) {
                parallel.name = lit->literal.lexeme;
              }
            }
          }
          std::vector<IRPtr> parts;
          parts.push_back(domain_result.ir);
          parts.push_back(MakeIR(std::move(parallel)));

          IRValue result_value;
          if (explicit_result) {
            result_value = body_result.value;
          } else if (collected.empty()) {
            result_value.kind = IRValue::Kind::Opaque;
            result_value.name = "unit";
          } else {
            std::vector<IRValue> elems;
            elems.reserve(collected.size());
            for (const auto& item : collected) {
              if (item.needs_wait) {
                IRWait wait;
                wait.handle = item.value;
                wait.result = ctx.FreshTempValue("parallel_wait");
                parts.push_back(MakeIR(std::move(wait)));
                elems.push_back(wait.result);
              } else {
                elems.push_back(item.value);
              }
            }
            if (elems.size() == 1) {
              result_value = elems[0];
            } else {
              IRValue tuple_value = ctx.FreshTempValue("parallel_tuple");
              DerivedValueInfo info;
              info.kind = DerivedValueInfo::Kind::TupleLit;
              info.elements = std::move(elems);
              ctx.RegisterDerivedValue(tuple_value, info);
              result_value = tuple_value;
            }
          }

          return LowerResult{SeqIR(std::move(parts)), result_value};
        } else if constexpr (std::is_same_v<T, syntax::SpawnExpr>) {
          SPEC_RULE("Lower-Expr-Spawn");
          std::unordered_set<std::string> explicit_moves =
              CollectSpawnMoveCaptures(node);
          std::vector<CaptureBinding> captures;
          if (node.body) {
            captures = CollectCaptures(*node.body, ctx, explicit_moves);
          }

          std::vector<analysis::TypeRef> env_fields;
          std::vector<IRValue> env_values;
          std::vector<IRPtr> env_parts;

          CaptureEnvInfo env_info;
          env_info.captures.clear();

          for (std::size_t i = 0; i < captures.size(); ++i) {
            const auto& cap = captures[i];
            const auto perm = PermissionOfType(cap.type);
            const bool by_ref =
                perm == analysis::Permission::Const ||
                perm == analysis::Permission::Shared;
            analysis::TypeRef field_type = cap.type;
            if (by_ref) {
              field_type = analysis::MakeTypePtr(cap.type, analysis::PtrState::Valid);
            }

            env_fields.push_back(field_type);
            CaptureAccess access;
            access.index = i;
            access.value_type = cap.type;
            access.field_type = field_type;
            access.by_ref = by_ref;
            env_info.captures[cap.name] = access;

            IRValue field_val;
            IRPtr field_ir = EmptyIR();
            if (by_ref) {
              IRValue ptr = ctx.FreshTempValue("capture_addr");
              DerivedValueInfo info;
              info.kind = DerivedValueInfo::Kind::AddrLocal;
              info.name = cap.name;
              ctx.RegisterDerivedValue(ptr, info);
              ctx.RegisterValueType(
                  ptr,
                  analysis::MakeTypePtr(cap.type, analysis::PtrState::Valid));
              field_val = ptr;
            } else if (cap.explicit_move) {
              syntax::Expr ident_expr;
              ident_expr.node = syntax::IdentifierExpr{cap.name};
              auto move_res = LowerMovePlace(ident_expr, ctx);
              field_val = move_res.value;
              field_ir = move_res.ir;
            } else {
              field_val.kind = IRValue::Kind::Local;
              field_val.name = cap.name;
            }
            if (field_ir &&
                !std::holds_alternative<IROpaque>(field_ir->node)) {
              env_parts.push_back(field_ir);
            }
            env_values.push_back(field_val);
          }

          analysis::TypeRef env_type = analysis::MakeTypeTuple(env_fields);
          env_info.env_type = env_type;

          IRValue env_tuple = ctx.FreshTempValue("spawn_env");
          DerivedValueInfo tuple_info;
          tuple_info.kind = DerivedValueInfo::Kind::TupleLit;
          tuple_info.elements = env_values;
          ctx.RegisterDerivedValue(env_tuple, tuple_info);
          ctx.RegisterValueType(env_tuple, env_type);

          const std::string env_var_name =
              ctx.FreshTempValue("spawn_env_var").name;
          IRBindVar bind_env;
          bind_env.name = env_var_name;
          bind_env.value = env_tuple;
          bind_env.type = env_type;
          env_parts.push_back(MakeIR(std::move(bind_env)));

          IRValue env_ptr = ctx.FreshTempValue("spawn_env_ptr");
          DerivedValueInfo addr_info;
          addr_info.kind = DerivedValueInfo::Kind::AddrLocal;
          addr_info.name = env_var_name;
          ctx.RegisterDerivedValue(env_ptr, addr_info);
          ctx.RegisterValueType(
              env_ptr,
              analysis::MakeTypePtr(env_type, analysis::PtrState::Valid));

          analysis::ScopeContext scope;
          if (ctx.sigma) {
            scope.sigma = *ctx.sigma;
            scope.current_module = ctx.module_path;
          }
          std::uint64_t env_size_val = 0;
          if (ctx.sigma) {
            if (const auto size = codegen::SizeOf(scope, env_type)) {
              env_size_val = *size;
            } else {
              ctx.ReportCodegenFailure();
            }
          }
          IRValue env_size = USizeImmediate(env_size_val);

          analysis::TypeRef body_type = analysis::MakeTypePrim("()");
          if (node.body && node.body->tail_opt && ctx.expr_type) {
            body_type = ctx.expr_type(*node.body->tail_opt);
          }
          if (!body_type) {
            body_type = analysis::MakeTypePrim("()");
          }
          std::uint64_t result_size_val = 0;
          if (ctx.sigma) {
            if (const auto size = codegen::SizeOf(scope, body_type)) {
              result_size_val = *size;
            } else {
              ctx.ReportCodegenFailure();
            }
          }
          IRValue result_size = USizeImmediate(result_size_val);

          std::string wrapper_sym =
              "__c0x_spawn_body_" + std::to_string(ctx.synth_proc_counter++);
          ProcIR proc;
          proc.symbol = wrapper_sym;
          proc.ret = analysis::MakeTypePrim("()");

          analysis::TypeRef env_ptr_type =
              analysis::MakeTypePtr(env_type, analysis::PtrState::Valid);
          analysis::TypeRef result_ptr_type =
              analysis::MakeTypePtr(body_type, analysis::PtrState::Valid);

          // Note: env and result are passed by value (Move mode) because the C
          // runtime calls body(env_ptr, result_ptr, panic_out) directly, not
          // by reference. Using nullopt mode would generate ByRef ABI which
          // expects pointer-to-pointer semantics.
          IRParam env_param;
          env_param.mode = analysis::ParamMode::Move;
          env_param.name = "env";
          env_param.type = env_ptr_type;
          proc.params.push_back(env_param);

          IRParam result_param;
          result_param.mode = analysis::ParamMode::Move;
          result_param.name = "result";
          result_param.type = result_ptr_type;
          proc.params.push_back(result_param);

          IRParam panic_param;
          panic_param.mode = analysis::ParamMode::Move;
          panic_param.name = std::string(kPanicOutName);
          panic_param.type = PanicOutType();
          proc.params.push_back(panic_param);

          {
            LowerCtxSnapshot snapshot(ctx);
            ctx.scope_stack.clear();
            ctx.binding_states.clear();
            ctx.derived_values.clear();
            ctx.temp_sink = nullptr;
            ctx.temp_depth = 0;
            ctx.suppress_temp_at_depth.reset();
            ctx.parallel_collect = nullptr;
            ctx.parallel_collect_depth = 0;
            ctx.capture_env.reset();
            ctx.proc_ret_type = analysis::MakeTypePrim("()");

            ctx.PushScope(false, false);
            ctx.RegisterVar(env_param.name, env_ptr_type, false, false);
            ctx.RegisterVar(result_param.name, result_ptr_type, false, false);
            ctx.RegisterVar(panic_param.name, panic_param.type, true, false);

            CaptureEnvInfo wrapper_env = env_info;
            IRValue env_param_val;
            env_param_val.kind = IRValue::Kind::Local;
            env_param_val.name = env_param.name;
            wrapper_env.env_param = env_param_val;
            ctx.capture_env = wrapper_env;

            LowerResult body_result;
            if (node.body) {
              body_result = LowerBlock(*node.body, ctx);
            } else {
              IRValue unit;
              unit.kind = IRValue::Kind::Opaque;
              unit.name = "unit";
              body_result = LowerResult{EmptyIR(), unit};
            }

            IRPtr store_ir = EmptyIR();
            if (!IsUnitType(body_type)) {
              IRWritePtr write;
              IRValue result_ptr_val;
              result_ptr_val.kind = IRValue::Kind::Local;
              result_ptr_val.name = result_param.name;
              write.ptr = result_ptr_val;
              write.value = body_result.value;
              store_ir = MakeIR(std::move(write));
            }

            CleanupPlan cleanup_plan = ComputeCleanupPlanForCurrentScope(ctx);
            IRPtr cleanup_ir = EmitCleanup(cleanup_plan, ctx);
            ctx.PopScope();

            IRValue unit_ret;
            unit_ret.kind = IRValue::Kind::Opaque;
            unit_ret.name = "unit";
            IRReturn ret;
            ret.value = unit_ret;

            std::vector<IRPtr> parts;
            if (body_result.ir) {
              parts.push_back(body_result.ir);
            }
            if (store_ir && !std::holds_alternative<IROpaque>(store_ir->node)) {
              parts.push_back(store_ir);
            }
            if (cleanup_ir && !std::holds_alternative<IROpaque>(cleanup_ir->node)) {
              parts.push_back(cleanup_ir);
            }
            parts.push_back(MakeIR(std::move(ret)));
            proc.body = SeqIR(std::move(parts));

            snapshot.Restore(ctx);
          }

          ctx.RegisterProcSig(proc);
          ctx.RegisterProcModule(proc.symbol, ctx.module_path);
          ctx.extra_procs.push_back(proc);

          IRSpawn spawn;
          spawn.body = EmptyIR();
          IRValue unit_body;
          unit_body.kind = IRValue::Kind::Opaque;
          unit_body.name = "unit";
          spawn.body_result = unit_body;
          spawn.captured_env = SeqIR(std::move(env_parts));
          spawn.env_ptr = env_ptr;
          spawn.env_size = env_size;
          spawn.body_fn.kind = IRValue::Kind::Symbol;
          spawn.body_fn.name = wrapper_sym;
          spawn.result_size = result_size;
          spawn.result = ctx.FreshTempValue("spawn_handle");
          IRValue spawn_result = spawn.result;
          // Handle options (name, affinity, priority)
          for (const auto& opt : node.opts) {
            if (opt.kind == syntax::SpawnOptionKind::Name && opt.value) {
              if (const auto* lit = std::get_if<syntax::LiteralExpr>(&opt.value->node)) {
                spawn.name = lit->literal.lexeme;
              }
            }
          }
          return LowerResult{MakeIR(std::move(spawn)), spawn_result};
        } else if constexpr (std::is_same_v<T, syntax::WaitExpr>) {
          SPEC_RULE("Lower-Expr-Wait");
          // Lower handle expression
          auto handle_result = LowerExpr(*node.handle, ctx);
          // Create IRWait
          IRWait wait;
          wait.handle = handle_result.value;
          wait.result = ctx.FreshTempValue("wait_result");
          IRValue wait_result = wait.result;

          // Register result type: extract T from Spawned<T>
          if (ctx.expr_type) {
            if (auto handle_type = ctx.expr_type(*node.handle)) {
              if (auto inner = analysis::ExtractSpawnedInner(handle_type)) {
                ctx.RegisterValueType(wait_result, *inner);
              }
            }
          }
          
          return LowerResult{SeqIR({handle_result.ir, MakeIR(std::move(wait))}), wait_result};
        } else if constexpr (std::is_same_v<T, syntax::DispatchExpr>) {
          SPEC_RULE("Lower-Expr-Dispatch");
          auto range_result = LowerExpr(*node.range, ctx);

          std::unordered_set<std::string> explicit_moves;
          std::vector<CaptureBinding> captures;
          if (node.body) {
            captures = CollectCaptures(*node.body, ctx, explicit_moves);
          }

          std::vector<analysis::TypeRef> env_fields;
          std::vector<IRValue> env_values;
          std::vector<IRPtr> env_parts;

          CaptureEnvInfo env_info;
          env_info.captures.clear();

          for (std::size_t i = 0; i < captures.size(); ++i) {
            const auto& cap = captures[i];
            const auto perm = PermissionOfType(cap.type);
            const bool by_ref =
                perm == analysis::Permission::Const ||
                perm == analysis::Permission::Shared;
            analysis::TypeRef field_type = cap.type;
            if (by_ref) {
              field_type = analysis::MakeTypePtr(cap.type, analysis::PtrState::Valid);
            }

            env_fields.push_back(field_type);
            CaptureAccess access;
            access.index = i;
            access.value_type = cap.type;
            access.field_type = field_type;
            access.by_ref = by_ref;
            env_info.captures[cap.name] = access;

            IRValue field_val;
            IRPtr field_ir = EmptyIR();
            if (by_ref) {
              IRValue ptr = ctx.FreshTempValue("capture_addr");
              DerivedValueInfo info;
              info.kind = DerivedValueInfo::Kind::AddrLocal;
              info.name = cap.name;
              ctx.RegisterDerivedValue(ptr, info);
              ctx.RegisterValueType(
                  ptr,
                  analysis::MakeTypePtr(cap.type, analysis::PtrState::Valid));
              field_val = ptr;
            } else if (cap.explicit_move) {
              syntax::Expr ident_expr;
              ident_expr.node = syntax::IdentifierExpr{cap.name};
              auto move_res = LowerMovePlace(ident_expr, ctx);
              field_val = move_res.value;
              field_ir = move_res.ir;
            } else {
              field_val.kind = IRValue::Kind::Local;
              field_val.name = cap.name;
            }
            if (field_ir &&
                !std::holds_alternative<IROpaque>(field_ir->node)) {
              env_parts.push_back(field_ir);
            }
            env_values.push_back(field_val);
          }

          analysis::TypeRef env_type = analysis::MakeTypeTuple(env_fields);
          env_info.env_type = env_type;

          IRValue env_tuple = ctx.FreshTempValue("dispatch_env");
          DerivedValueInfo tuple_info;
          tuple_info.kind = DerivedValueInfo::Kind::TupleLit;
          tuple_info.elements = env_values;
          ctx.RegisterDerivedValue(env_tuple, tuple_info);
          ctx.RegisterValueType(env_tuple, env_type);

          const std::string env_var_name =
              ctx.FreshTempValue("dispatch_env_var").name;
          IRBindVar bind_env;
          bind_env.name = env_var_name;
          bind_env.value = env_tuple;
          bind_env.type = env_type;
          env_parts.push_back(MakeIR(std::move(bind_env)));

          IRValue env_ptr = ctx.FreshTempValue("dispatch_env_ptr");
          DerivedValueInfo addr_info;
          addr_info.kind = DerivedValueInfo::Kind::AddrLocal;
          addr_info.name = env_var_name;
          ctx.RegisterDerivedValue(env_ptr, addr_info);
          ctx.RegisterValueType(
              env_ptr,
              analysis::MakeTypePtr(env_type, analysis::PtrState::Valid));

          analysis::TypeRef elem_type = analysis::MakeTypePrim("usize");
          analysis::TypeRef body_type = analysis::MakeTypePrim("()");
          if (node.body && node.body->tail_opt && ctx.expr_type) {
            body_type = ctx.expr_type(*node.body->tail_opt);
          }
          if (!body_type) {
            body_type = analysis::MakeTypePrim("()");
          }

          bool has_reduce = false;
          bool use_custom_reduce = false;
          std::optional<std::string> reduce_op;
          std::optional<std::string> custom_reduce_name;
          for (const auto& opt : node.opts) {
            if (opt.kind == syntax::DispatchOptionKind::Reduce) {
              has_reduce = true;
              switch (opt.reduce_op) {
                case syntax::ReduceOp::Add: reduce_op = "+"; break;
                case syntax::ReduceOp::Mul: reduce_op = "*"; break;
                case syntax::ReduceOp::Min: reduce_op = "min"; break;
                case syntax::ReduceOp::Max: reduce_op = "max"; break;
                case syntax::ReduceOp::And: reduce_op = "and"; break;
                case syntax::ReduceOp::Or: reduce_op = "or"; break;
                case syntax::ReduceOp::Custom:
                  use_custom_reduce = true;
                  reduce_op = std::nullopt;
                  custom_reduce_name = opt.custom_reduce_name;
                  break;
              }
            }
          }

          analysis::ScopeContext scope;
          if (ctx.sigma) {
            scope.sigma = *ctx.sigma;
            scope.current_module = ctx.module_path;
          }
          std::uint64_t elem_size_val = 0;
          if (ctx.sigma) {
            if (const auto size = codegen::SizeOf(scope, elem_type)) {
              elem_size_val = *size;
            } else {
              ctx.ReportCodegenFailure();
            }
          }
          IRValue elem_size = USizeImmediate(elem_size_val);

          std::uint64_t result_size_val = 0;
          if (has_reduce && ctx.sigma) {
            if (const auto size = codegen::SizeOf(scope, body_type)) {
              result_size_val = *size;
            } else {
              ctx.ReportCodegenFailure();
            }
          }
          IRValue result_size = USizeImmediate(result_size_val);

          IRValue result_ptr;
          if (has_reduce) {
            IRValue uninit = ctx.FreshTempValue("dispatch_result_init");
            const std::string result_name =
                ctx.FreshTempValue("dispatch_result_var").name;
            IRBindVar bind_result;
            bind_result.name = result_name;
            bind_result.value = uninit;
            bind_result.type = body_type;
            env_parts.push_back(MakeIR(std::move(bind_result)));

            DerivedValueInfo res_addr;
            res_addr.kind = DerivedValueInfo::Kind::AddrLocal;
            res_addr.name = result_name;
            result_ptr = ctx.FreshTempValue("dispatch_result_ptr");
            ctx.RegisterDerivedValue(result_ptr, res_addr);
            auto res_ptr_type =
                analysis::MakeTypeRawPtr(analysis::RawPtrQual::Mut, body_type);
            ctx.RegisterValueType(result_ptr, res_ptr_type);
          } else {
            auto res_ptr_type =
                analysis::MakeTypeRawPtr(analysis::RawPtrQual::Mut,
                                         analysis::MakeTypePrim("u8"));
            result_ptr = MakeNullPtr(res_ptr_type, ctx, env_parts);
          }

          std::optional<IRValue> reduce_fn;
          if (use_custom_reduce) {
            std::string wrapper_sym =
                "__c0x_dispatch_reduce_" + std::to_string(ctx.synth_proc_counter++);
            ProcIR proc;
            proc.symbol = wrapper_sym;
            proc.ret = analysis::MakeTypePrim("()");

            analysis::TypeRef lhs_ptr =
                analysis::MakeTypeRawPtr(analysis::RawPtrQual::Imm, body_type);
            analysis::TypeRef rhs_ptr =
                analysis::MakeTypeRawPtr(analysis::RawPtrQual::Imm, body_type);
            analysis::TypeRef out_ptr =
                analysis::MakeTypeRawPtr(analysis::RawPtrQual::Mut, body_type);

            // Note: lhs, rhs, and out are passed by value (Move mode) because
            // the C runtime calls reduce_fn(lhs_ptr, rhs_ptr, out_ptr, panic_out)
            // directly, not by reference.
            IRParam lhs_param;
            lhs_param.mode = analysis::ParamMode::Move;
            lhs_param.name = "lhs";
            lhs_param.type = lhs_ptr;
            proc.params.push_back(lhs_param);

            IRParam rhs_param;
            rhs_param.mode = analysis::ParamMode::Move;
            rhs_param.name = "rhs";
            rhs_param.type = rhs_ptr;
            proc.params.push_back(rhs_param);

            IRParam out_param;
            out_param.mode = analysis::ParamMode::Move;
            out_param.name = "out";
            out_param.type = out_ptr;
            proc.params.push_back(out_param);

            IRParam panic_param;
            panic_param.mode = analysis::ParamMode::Move;
            panic_param.name = std::string(kPanicOutName);
            panic_param.type = PanicOutType();
            proc.params.push_back(panic_param);

            {
              LowerCtxSnapshot snapshot(ctx);
              ctx.scope_stack.clear();
              ctx.binding_states.clear();
              ctx.derived_values.clear();
              ctx.temp_sink = nullptr;
              ctx.temp_depth = 0;
              ctx.suppress_temp_at_depth.reset();
              ctx.parallel_collect = nullptr;
              ctx.parallel_collect_depth = 0;
              ctx.capture_env.reset();
              ctx.proc_ret_type = analysis::MakeTypePrim("()");

              ctx.PushScope(false, false);
              ctx.RegisterVar(lhs_param.name, lhs_param.type, false, false);
              ctx.RegisterVar(rhs_param.name, rhs_param.type, false, false);
              ctx.RegisterVar(out_param.name, out_param.type, false, false);
              ctx.RegisterVar(panic_param.name, panic_param.type, true, false);

              IRValue lhs_val = ctx.FreshTempValue("reduce_lhs");
              IRValue rhs_val = ctx.FreshTempValue("reduce_rhs");
              IRValue lhs_ptr_val;
              lhs_ptr_val.kind = IRValue::Kind::Local;
              lhs_ptr_val.name = lhs_param.name;
              IRReadPtr read_lhs;
              read_lhs.ptr = lhs_ptr_val;
              read_lhs.result = lhs_val;
              IRValue rhs_ptr_val;
              rhs_ptr_val.kind = IRValue::Kind::Local;
              rhs_ptr_val.name = rhs_param.name;
              IRReadPtr read_rhs;
              read_rhs.ptr = rhs_ptr_val;
              read_rhs.result = rhs_val;

              ctx.RegisterValueType(lhs_val, body_type);
              ctx.RegisterValueType(rhs_val, body_type);

              std::vector<IRPtr> parts;
              parts.push_back(MakeIR(std::move(read_lhs)));
              parts.push_back(MakeIR(std::move(read_rhs)));

              IRValue callee;
              callee.kind = IRValue::Kind::Symbol;
              if (custom_reduce_name.has_value()) {
                callee.name = *custom_reduce_name;
                if (ctx.resolve_name) {
                  if (const auto resolved = ctx.resolve_name(*custom_reduce_name)) {
                    std::vector<std::string> full = *resolved;
                    const std::string resolved_name = full.back();
                    full.pop_back();
                    IRReadPath read_path;
                    read_path.path = std::move(full);
                    read_path.name = resolved_name;
                    parts.push_back(MakeIR(std::move(read_path)));
                    callee.name = resolved_name;
                  } else {
                    ctx.ReportResolveFailure(*custom_reduce_name);
                  }
                } else {
                  ctx.ReportResolveFailure(*custom_reduce_name);
                }
              }

              IRCall call_reduce;
              call_reduce.callee = callee;
              call_reduce.args = {lhs_val, rhs_val};
              call_reduce.result = ctx.FreshTempValue("reduce_val");
              ctx.RegisterValueType(call_reduce.result, body_type);
              parts.push_back(MakeIR(std::move(call_reduce)));
              parts.push_back(PanicCheck(ctx));

              IRValue out_ptr_val;
              out_ptr_val.kind = IRValue::Kind::Local;
              out_ptr_val.name = out_param.name;
              IRWritePtr write;
              write.ptr = out_ptr_val;
              write.value = call_reduce.result;
              parts.push_back(MakeIR(std::move(write)));

              CleanupPlan cleanup_plan = ComputeCleanupPlanForCurrentScope(ctx);
              IRPtr cleanup_ir = EmitCleanup(cleanup_plan, ctx);
              if (cleanup_ir && !std::holds_alternative<IROpaque>(cleanup_ir->node)) {
                parts.push_back(cleanup_ir);
              }
              ctx.PopScope();

              IRValue unit_ret;
              unit_ret.kind = IRValue::Kind::Opaque;
              unit_ret.name = "unit";
              IRReturn ret;
              ret.value = unit_ret;
              parts.push_back(MakeIR(std::move(ret)));
              proc.body = SeqIR(std::move(parts));

              snapshot.Restore(ctx);
            }

            ctx.RegisterProcSig(proc);
            ctx.RegisterProcModule(proc.symbol, ctx.module_path);
            ctx.extra_procs.push_back(proc);

            IRValue fn_val;
            fn_val.kind = IRValue::Kind::Symbol;
            fn_val.name = wrapper_sym;
            reduce_fn = fn_val;
          }

          

          std::string wrapper_sym =
              "__c0x_dispatch_body_" + std::to_string(ctx.synth_proc_counter++);
          ProcIR proc;
          proc.symbol = wrapper_sym;
          proc.ret = analysis::MakeTypePrim("()");

          analysis::TypeRef elem_ptr_type =
              analysis::MakeTypePtr(elem_type, analysis::PtrState::Valid);
          analysis::TypeRef env_ptr_type =
              analysis::MakeTypePtr(env_type, analysis::PtrState::Valid);
          analysis::TypeRef result_ptr_type =
              analysis::MakeTypePtr(body_type, analysis::PtrState::Valid);

          // Note: elem, env, and result are passed by value (Move mode) because
          // the C runtime calls body(elem_ptr, env_ptr, result_ptr, panic_out)
          // directly, not by reference.
          IRParam elem_param;
          elem_param.mode = analysis::ParamMode::Move;
          elem_param.name = "elem";
          elem_param.type = elem_ptr_type;
          proc.params.push_back(elem_param);

          IRParam env_param;
          env_param.mode = analysis::ParamMode::Move;
          env_param.name = "env";
          env_param.type = env_ptr_type;
          proc.params.push_back(env_param);

          IRParam result_param;
          result_param.mode = analysis::ParamMode::Move;
          result_param.name = "result";
          result_param.type = result_ptr_type;
          proc.params.push_back(result_param);

          IRParam panic_param;
          panic_param.mode = analysis::ParamMode::Move;
          panic_param.name = std::string(kPanicOutName);
          panic_param.type = PanicOutType();
          proc.params.push_back(panic_param);

          {
            LowerCtxSnapshot snapshot(ctx);
            ctx.scope_stack.clear();
            ctx.binding_states.clear();
            ctx.derived_values.clear();
            ctx.temp_sink = nullptr;
            ctx.temp_depth = 0;
            ctx.suppress_temp_at_depth.reset();
            ctx.parallel_collect = nullptr;
            ctx.parallel_collect_depth = 0;
            ctx.capture_env.reset();
            ctx.proc_ret_type = analysis::MakeTypePrim("()");

            ctx.PushScope(false, false);
            ctx.RegisterVar(elem_param.name, elem_param.type, false, false);
            ctx.RegisterVar(env_param.name, env_param.type, false, false);
            ctx.RegisterVar(result_param.name, result_param.type, false, false);
            ctx.RegisterVar(panic_param.name, panic_param.type, true, false);

            CaptureEnvInfo wrapper_env = env_info;
            IRValue env_param_val;
            env_param_val.kind = IRValue::Kind::Local;
            env_param_val.name = env_param.name;
            wrapper_env.env_param = env_param_val;
            ctx.capture_env = wrapper_env;

            IRValue elem_ptr_val;
            elem_ptr_val.kind = IRValue::Kind::Local;
            elem_ptr_val.name = elem_param.name;
            IRValue elem_val = ctx.FreshTempValue("dispatch_elem");
            IRReadPtr read_elem;
            read_elem.ptr = elem_ptr_val;
            read_elem.result = elem_val;
            ctx.RegisterValueType(elem_val, elem_type);

            IRPtr bind_ir = EmptyIR();
            if (node.pattern) {
              RegisterPatternBindings(*node.pattern, elem_type, ctx);
              bind_ir = LowerBindPattern(*node.pattern, elem_val, ctx);
            }

            LowerResult body_result;
            if (node.body) {
              body_result = LowerBlock(*node.body, ctx);
            } else {
              IRValue unit_val;
              unit_val.kind = IRValue::Kind::Opaque;
              unit_val.name = "unit";
              body_result = LowerResult{EmptyIR(), unit_val};
            }

            IRPtr store_ir = EmptyIR();
            if (!IsUnitType(body_type)) {
              IRWritePtr write;
              IRValue result_ptr_val;
              result_ptr_val.kind = IRValue::Kind::Local;
              result_ptr_val.name = result_param.name;
              write.ptr = result_ptr_val;
              write.value = body_result.value;
              store_ir = MakeIR(std::move(write));
            }

            CleanupPlan cleanup_plan = ComputeCleanupPlanForCurrentScope(ctx);
            IRPtr cleanup_ir = EmitCleanup(cleanup_plan, ctx);
            ctx.PopScope();

            IRValue unit_ret;
            unit_ret.kind = IRValue::Kind::Opaque;
            unit_ret.name = "unit";
            IRReturn ret;
            ret.value = unit_ret;

            std::vector<IRPtr> parts;
            parts.push_back(MakeIR(std::move(read_elem)));
            if (bind_ir && !std::holds_alternative<IROpaque>(bind_ir->node)) {
              parts.push_back(bind_ir);
            }
            if (body_result.ir) {
              parts.push_back(body_result.ir);
            }
            if (store_ir && !std::holds_alternative<IROpaque>(store_ir->node)) {
              parts.push_back(store_ir);
            }
            if (cleanup_ir && !std::holds_alternative<IROpaque>(cleanup_ir->node)) {
              parts.push_back(cleanup_ir);
            }
            parts.push_back(MakeIR(std::move(ret)));
            proc.body = SeqIR(std::move(parts));

            snapshot.Restore(ctx);
          }

          ctx.RegisterProcSig(proc);
          ctx.RegisterProcModule(proc.symbol, ctx.module_path);
          ctx.extra_procs.push_back(proc);

          IRDispatch dispatch;
          dispatch.pattern = node.pattern;
          dispatch.range = range_result.value;
          dispatch.body = EmptyIR();
          IRValue unit_body;
          unit_body.kind = IRValue::Kind::Opaque;
          unit_body.name = "unit";
          dispatch.body_result = unit_body;
          dispatch.captured_env = SeqIR(std::move(env_parts));
          dispatch.env_ptr = env_ptr;
          dispatch.body_fn.kind = IRValue::Kind::Symbol;
          dispatch.body_fn.name = wrapper_sym;
          dispatch.elem_size = elem_size;
          dispatch.result_size = result_size;
          dispatch.result_ptr = result_ptr;
          dispatch.reduce_op = reduce_op;
          dispatch.reduce_fn = reduce_fn;
          dispatch.result = ctx.FreshTempValue("dispatch_result");
          IRValue dispatch_result = dispatch.result;

          IRPtr chunk_ir = EmptyIR();
          for (const auto& opt : node.opts) {
            if (opt.kind == syntax::DispatchOptionKind::Ordered) {
              dispatch.ordered = true;
            } else if (opt.kind == syntax::DispatchOptionKind::Chunk &&
                       opt.chunk_expr) {
              auto chunk_result = LowerExpr(*opt.chunk_expr, ctx);
              chunk_ir = chunk_result.ir;
              dispatch.chunk_size = chunk_result.value;
            }
          }

          std::vector<IRPtr> call_parts;
          call_parts.push_back(range_result.ir);
          if (chunk_ir && !std::holds_alternative<IROpaque>(chunk_ir->node)) {
            call_parts.push_back(chunk_ir);
          }
          call_parts.push_back(MakeIR(std::move(dispatch)));
          return LowerResult{SeqIR(std::move(call_parts)), dispatch_result};

        // C0X Extension: Asynchronous Operations (§19)

        } else if constexpr (std::is_same_v<T, syntax::YieldExpr>) {
          SPEC_RULE("Lower-Expr-Yield");
          // §19.2.2 Yield expression lowering
          auto value_result = LowerExpr(*node.value, ctx);

          IRYield yield;
          yield.release = node.release;
          yield.value = value_result.value;
          yield.result = ctx.FreshTempValue("yield_in");
          // state_index will be assigned during async procedure transformation
          yield.state_index = 0;

          if (node.release) {
            yield.keys_record = ctx.FreshTempValue("keys_record");
          }

          IRValue yield_result = yield.result;
          return LowerResult{SeqIR({value_result.ir, MakeIR(std::move(yield))}),
                             yield_result};

        } else if constexpr (std::is_same_v<T, syntax::YieldFromExpr>) {
          SPEC_RULE("Lower-Expr-YieldFrom");
          // §19.2.3 Yield-from expression lowering
          auto source_result = LowerExpr(*node.value, ctx);

          analysis::TypeRef source_type;
          if (ctx.expr_type) {
            source_type = ctx.expr_type(*node.value);
          }

          const std::string source_name =
              ctx.FreshTempValue("yield_from_source").name;
          IRBindVar bind_source;
          bind_source.name = source_name;
          bind_source.value = source_result.value;
          bind_source.type = source_type;

          IRValue source_local;
          source_local.kind = IRValue::Kind::Local;
          source_local.name = source_name;
          if (source_type) {
            ctx.RegisterValueType(source_local, source_type);
          }

          IRYieldFrom yf;
          yf.release = node.release;
          yf.source = source_local;
          yf.result = ctx.FreshTempValue("yield_from_result");

          // Get source type for later emission
          yf.source_type = source_type;
          // state_index will be assigned during async procedure transformation
          yf.state_index = 0;

          IRValue yf_result = yf.result;
          return LowerResult{SeqIR({source_result.ir,
                                    MakeIR(std::move(bind_source)),
                                    MakeIR(std::move(yf))}),
                             yf_result};

        } else if constexpr (std::is_same_v<T, syntax::SyncExpr>) {
          SPEC_RULE("Lower-Expr-Sync");
          // §19.3.3 Sync expression lowering
          auto async_result = LowerExpr(*node.value, ctx);

          IRSync sync;
          sync.async_value = async_result.value;
          sync.result = ctx.FreshTempValue("sync_result");

          // Get async type info for emission
          if (ctx.expr_type) {
            sync.async_type = ctx.expr_type(*node.value);
            // Extract result_type and error_type from async type
            if (auto sig = analysis::GetAsyncSig(sync.async_type)) {
              sync.result_type = sig->result;
              sync.error_type = sig->err;
            }
          }

          IRValue sync_result = sync.result;
          return LowerResult{SeqIR({async_result.ir, MakeIR(std::move(sync))}),
                             sync_result};

        } else if constexpr (std::is_same_v<T, syntax::RaceExpr>) {
          SPEC_RULE("Lower-Expr-Race");
          // §19.3.4 Race expression lowering
          // Determine mode from first handler
          bool is_streaming = !node.arms.empty() &&
                              node.arms[0].handler.kind == syntax::RaceHandlerKind::Yield;

          std::vector<IRRaceArm> ir_arms;
          std::vector<LowerCtx> arm_ctxs;
          ir_arms.reserve(node.arms.size());
          arm_ctxs.reserve(node.arms.size());
          for (const auto& arm : node.arms) {
            IRRaceArm ir_arm;
            auto async_result = LowerExpr(*arm.expr, ctx);
            ir_arm.async_ir = async_result.ir;
            ir_arm.async_value = async_result.value;
            ir_arm.pattern = arm.pattern;

            analysis::TypeRef async_type;
            analysis::TypeRef pat_type;
            if (ctx.expr_type) {
              async_type = ctx.expr_type(*arm.expr);
              if (auto sig = analysis::GetAsyncSig(async_type)) {
                pat_type = is_streaming ? sig->out : sig->result;
              }
            }

            IRValue match_value = ctx.FreshTempValue("race_match");
            ir_arm.match_value = match_value;

            LowerCtx arm_ctx = ctx;
            arm_ctx.PushScope(false, false);
            if (pat_type) {
              arm_ctx.RegisterValueType(match_value, pat_type);
            }
            if (arm.pattern) {
              RegisterPatternBindings(*arm.pattern, pat_type, arm_ctx);
            }
            IRPtr bind_ir = arm.pattern
                                ? LowerBindPattern(*arm.pattern, match_value, arm_ctx)
                                : EmptyIR();

            // Lower handler
            auto handler_result = LowerExpr(*arm.handler.value, arm_ctx);

            CleanupPlan cleanup_plan = ComputeCleanupPlanForCurrentScope(arm_ctx);
            CleanupPlan remainder =
                ComputeCleanupPlanRemainder(CleanupTarget::CurrentScope, arm_ctx);
            IRPtr cleanup_ir = EmitCleanupWithRemainder(cleanup_plan, remainder, arm_ctx);
            arm_ctx.PopScope();

            ir_arm.handler_ir = SeqIR({bind_ir, handler_result.ir, cleanup_ir});
            ir_arm.handler_result = handler_result.value;

            for (const auto& [name, type] : arm_ctx.value_types) {
              if (!ctx.value_types.count(name)) {
                ctx.value_types.emplace(name, type);
              }
            }
            for (const auto& [name, info] : arm_ctx.derived_values) {
              if (!ctx.derived_values.count(name)) {
                ctx.derived_values.emplace(name, info);
              }
            }
            for (const auto& [name, type] : arm_ctx.static_types) {
              if (!ctx.static_types.count(name)) {
                ctx.static_types.emplace(name, type);
              }
            }
            for (const auto& [name, type] : arm_ctx.drop_glue_types) {
              if (!ctx.drop_glue_types.count(name)) {
                ctx.drop_glue_types.emplace(name, type);
              }
            }
            ctx.temp_counter = std::max(ctx.temp_counter, arm_ctx.temp_counter);
            ir_arms.push_back(std::move(ir_arm));
            arm_ctxs.push_back(std::move(arm_ctx));
          }

          std::vector<const LowerCtx*> branches;
          branches.reserve(arm_ctxs.size());
          for (const auto& arm_ctx : arm_ctxs) {
            branches.push_back(&arm_ctx);
          }
          MergeMoveStates(ctx, branches);
          for (const auto& arm_ctx : arm_ctxs) {
            MergeFailures(ctx, arm_ctx);
          }

          if (is_streaming) {
            IRRaceYield race;
            race.arms = std::move(ir_arms);
            race.result = ctx.FreshTempValue("race_yield_result");
            if (ctx.expr_type) {
              race.stream_type = ctx.expr_type(expr);
            }
            IRValue result = race.result;
            return LowerResult{MakeIR(std::move(race)), result};
          } else {
            IRRaceReturn race;
            race.arms = std::move(ir_arms);
            race.result = ctx.FreshTempValue("race_return_result");
            if (ctx.expr_type && !node.arms.empty()) {
              race.result_type = ctx.expr_type(*node.arms.front().handler.value);
            }
            IRValue result = race.result;
            return LowerResult{MakeIR(std::move(race)), result};
          }

        } else if constexpr (std::is_same_v<T, syntax::AllExpr>) {
          SPEC_RULE("Lower-Expr-All");
          // §19.3.5 All expression lowering
          IRAll all;
          std::vector<analysis::TypeRef> tuple_elems;
          std::vector<analysis::TypeRef> error_types;
          tuple_elems.reserve(node.exprs.size());
          error_types.reserve(node.exprs.size());
          for (const auto& expr_ptr : node.exprs) {
            auto result = LowerExpr(*expr_ptr, ctx);
            all.async_irs.push_back(result.ir);
            all.async_values.push_back(result.value);
            if (ctx.expr_type) {
              analysis::TypeRef async_type = ctx.expr_type(*expr_ptr);
              if (auto sig = analysis::GetAsyncSig(async_type)) {
                tuple_elems.push_back(sig->result);
                error_types.push_back(sig->err);
              }
            }
          }
          all.result = ctx.FreshTempValue("all_result");

          if (tuple_elems.size() == node.exprs.size()) {
            all.tuple_type = analysis::MakeTypeTuple(tuple_elems);
          }
          all.error_types = std::move(error_types);

          IRValue result = all.result;
          return LowerResult{MakeIR(std::move(all)), result};

        } else {
          IRValue value = ctx.FreshTempValue("unknown_expr");
          return LowerResult{EmptyIR(), value};
        }
      },
      expr.node);
}

LowerResult LowerExpr(const syntax::Expr& expr, LowerCtx& ctx) {
  ctx.temp_depth += 1;
  LowerResult result = LowerExprImpl(expr, ctx);
  const int depth = ctx.temp_depth;

  if (ctx.expr_type) {
    ctx.RegisterValueType(result.value, ctx.expr_type(expr));
  }
  ctx.temp_depth -= 1;

  bool suppress = ctx.suppress_temp_at_depth.has_value() &&
                  *ctx.suppress_temp_at_depth == depth;
  if (suppress) {
    ctx.suppress_temp_at_depth.reset();
    return result;
  }

  if (ctx.temp_sink && IsTempValueExpr(expr)) {
    analysis::TypeRef type;
    if (ctx.expr_type) {
      type = ctx.expr_type(expr);
    }
    ctx.RegisterTempValue(result.value, type);
  }

  return result;
}

// ============================================================================
// §6.8 LowerCtx Scope Tracking Methods
// ============================================================================

void LowerCtx::PushScope(bool is_loop, bool is_region) {
  ScopeInfo scope;
  scope.is_loop = is_loop;
  scope.is_region = is_region;
  scope_stack.push_back(std::move(scope));
}

void LowerCtx::PopScope() {
  if (scope_stack.empty()) {
    return;
  }

  const auto& vars = scope_stack.back().variables;
  for (auto it = vars.rbegin(); it != vars.rend(); ++it) {
    auto map_it = binding_states.find(*it);
    if (map_it == binding_states.end()) {
      continue;
    }
    if (!map_it->second.empty()) {
      map_it->second.pop_back();
    }
    if (map_it->second.empty()) {
      binding_states.erase(map_it);
    }
  }

  scope_stack.pop_back();
}

std::vector<std::string> LowerCtx::CurrentScopeVars() const {
  if (scope_stack.empty()) {
    return {};
  }
  return scope_stack.back().variables;
}

std::vector<std::string> LowerCtx::VarsToLoopScope() const {
  std::vector<std::string> vars;

  // Collect variables from innermost scope outward until we hit a loop scope
  for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
    for (const auto& var : it->variables) {
      vars.push_back(var);
    }
    if (it->is_loop) {
      break;
    }
  }

  return vars;
}

std::vector<std::string> LowerCtx::VarsToFunctionRoot() const {
  std::vector<std::string> vars;

  // Collect all variables from all scopes
  for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
    for (const auto& var : it->variables) {
      vars.push_back(var);
    }
  }

  return vars;
}

void LowerCtx::RegisterVar(const std::string& name,
                            analysis::TypeRef type,
                            bool has_responsibility,
                            bool is_immovable,
                            analysis::ProvenanceKind prov,
                            std::optional<std::string> prov_region) {
  if (!scope_stack.empty()) {
    scope_stack.back().variables.push_back(name);
    if (has_responsibility) {
      CleanupItem item;
      item.kind = CleanupItem::Kind::DropBinding;
      item.name = name;
      scope_stack.back().cleanup_items.push_back(std::move(item));
    }
  }

  BindingState state;
  state.type = type;
  state.has_responsibility = has_responsibility;
  state.is_immovable = is_immovable;
  state.is_moved = false;
  state.prov = prov;
  state.prov_region = std::move(prov_region);

  binding_states[name].push_back(std::move(state));
}

void LowerCtx::MarkMoved(const std::string& name) {
  auto it = binding_states.find(name);
  if (it == binding_states.end() || it->second.empty()) {
    SPEC_RULE("UpdateValid-Err");
    ReportCodegenFailure();
    return;
  }
  SPEC_RULE("UpdateValid-MoveRoot");
  it->second.back().is_moved = true;
}

void LowerCtx::MarkFieldMoved(const std::string& name, const std::string& field) {
  auto it = binding_states.find(name);
  if (it == binding_states.end() || it->second.empty()) {
    SPEC_RULE("UpdateValid-Err");
    ReportCodegenFailure();
    return;
  }
  SPEC_RULE("UpdateValid-PartialMove-Init");
  SPEC_RULE("UpdateValid-PartialMove-Step");
  it->second.back().moved_fields.push_back(field);
}

const BindingState* LowerCtx::GetBindingState(const std::string& name) const {
  auto it = binding_states.find(name);
  if (it != binding_states.end() && !it->second.empty()) {
    return &it->second.back();
  }
  return nullptr;
}

std::optional<analysis::ProvenanceKind> LowerCtx::LookupExprProv(
    const syntax::Expr& expr) const {
  const auto it = expr_prov.find(&expr);
  if (it == expr_prov.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<std::string> LowerCtx::LookupExprRegion(
    const syntax::Expr& expr) const {
  const auto it = expr_region.find(&expr);
  if (it == expr_region.end()) {
    return std::nullopt;
  }
  return it->second;
}

void LowerCtx::RegisterDefer(const IRPtr& defer_ir) {
  if (!scope_stack.empty()) {
    CleanupItem item;
    item.kind = CleanupItem::Kind::DeferBlock;
    item.defer_ir = defer_ir;
    scope_stack.back().cleanup_items.push_back(std::move(item));
  }
}

void LowerCtx::RegisterRegionRelease(const std::string& name) {
  if (!scope_stack.empty()) {
    CleanupItem item;
    item.kind = CleanupItem::Kind::ReleaseRegion;
    item.name = name;
    scope_stack.back().cleanup_items.push_back(std::move(item));
  }
}

void LowerCtx::RegisterTempValue(const IRValue& value, const analysis::TypeRef& type) {
  if (!temp_sink) {
    return;
  }
  TempValue temp;
  temp.value = value;
  temp.type = type;
  temp_sink->push_back(std::move(temp));
}



void LowerCtx::RegisterDerivedValue(const IRValue& value, const DerivedValueInfo& info) {
  if (value.kind != IRValue::Kind::Opaque) {
    return;
  }
  derived_values[value.name] = info;
}

const DerivedValueInfo* LowerCtx::LookupDerivedValue(const IRValue& value) const {
  if (value.kind != IRValue::Kind::Opaque) {
    return nullptr;
  }
  auto it = derived_values.find(value.name);
  if (it == derived_values.end()) {
    return nullptr;
  }
  return &it->second;
}

const CaptureAccess* LowerCtx::LookupCapture(const std::string& name) const {
  if (!capture_env.has_value()) {
    return nullptr;
  }
  auto it = capture_env->captures.find(name);
  if (it == capture_env->captures.end()) {
    return nullptr;
  }
  return &it->second;
}

IRValue LowerCtx::CaptureFieldPtr(const CaptureAccess& access) {
  IRValue ptr = FreshTempValue("capture_ptr");
  if (!capture_env.has_value()) {
    return ptr;
  }
  DerivedValueInfo info;
  info.kind = DerivedValueInfo::Kind::AddrTuple;
  info.base = capture_env->env_param;
  info.tuple_index = access.index;
  RegisterDerivedValue(ptr, info);
  auto elem_ptr = analysis::MakeTypeRawPtr(analysis::RawPtrQual::Mut, access.field_type);
  RegisterValueType(ptr, elem_ptr);
  return ptr;
}

IRValue LowerCtx::FreshTempValue(std::string_view prefix) {
  IRValue value;
  value.kind = IRValue::Kind::Opaque;
  value.name = std::string(prefix) + "_" + std::to_string(temp_counter++);
  return value;
}

std::string LowerCtx::FreshRegionAlias() {
  auto name_used = [this](const std::string& name) -> bool {
    auto it = binding_states.find(name);
    if (it != binding_states.end() && !it->second.empty()) {
      return true;
    }
    for (const auto& scope : scope_stack) {
      if (std::find(scope.region_tags.begin(), scope.region_tags.end(), name) !=
          scope.region_tags.end()) {
        return true;
      }
    }
    return false;
  };

  for (std::size_t i = 0;; ++i) {
    std::string name = std::string("region$") + std::to_string(i);
    if (!name_used(name)) {
      return name;
    }
  }
}

void LowerCtx::ReserveRegionTag(const std::string& name) {
  if (!scope_stack.empty()) {
    scope_stack.back().region_tags.push_back(name);
  }
}

void LowerCtx::ReportResolveFailure(const std::string& name) {
  resolve_failed = true;
  if (std::find(resolve_failures.begin(), resolve_failures.end(), name) ==
      resolve_failures.end()) {
    resolve_failures.push_back(name);
  }
}

void LowerCtx::ReportCodegenFailure(std::source_location loc) {
  codegen_failed = true;
  if (std::getenv("CURSIVE0_DEBUG_OBJ")) {
    std::cerr << "[cursivec0] codegen failure at " << loc.file_name() << ":"
              << loc.line() << "\n";
  }
}

void LowerCtx::RegisterValueType(const IRValue& value, analysis::TypeRef type) {
  if (!type) {
    return;
  }
  if (value.kind != IRValue::Kind::Opaque) {
    return;
  }
  value_types[value.name] = type;
}

analysis::TypeRef LowerCtx::LookupValueType(const IRValue& value) const {
  if (value.kind == IRValue::Kind::Local) {
    if (const auto* state = GetBindingState(value.name)) {
      return state->type;
    }
    return nullptr;
  }
  if (value.kind == IRValue::Kind::Symbol) {
    return LookupStaticType(value.name);
  }
  if (value.kind != IRValue::Kind::Opaque) {
    return nullptr;
  }
  auto it = value_types.find(value.name);
  if (it != value_types.end()) {
    return it->second;
  }
  return nullptr;
}

void LowerCtx::RegisterStaticType(const std::string& sym, analysis::TypeRef type) {
  if (!type) {
    return;
  }
  static_types[sym] = type;
}

analysis::TypeRef LowerCtx::LookupStaticType(const std::string& sym) const {
  auto it = static_types.find(sym);
  if (it != static_types.end()) {
    return it->second;
  }
  return nullptr;
}

void LowerCtx::RegisterStaticModule(const std::string& sym,
                                    const syntax::ModulePath& module_path) {
  static_modules[sym] = module_path;
}

const std::vector<std::string>* LowerCtx::LookupStaticModule(
    const std::string& sym) const {
  auto it = static_modules.find(sym);
  if (it != static_modules.end()) {
    return &it->second;
  }
  return nullptr;
}

void LowerCtx::RegisterDropGlueType(const std::string& sym, analysis::TypeRef type) {
  if (!type) {
    return;
  }
  drop_glue_types[sym] = type;
}

analysis::TypeRef LowerCtx::LookupDropGlueType(const std::string& sym) const {
  auto it = drop_glue_types.find(sym);
  if (it != drop_glue_types.end()) {
    return it->second;
  }
  return nullptr;
}

void LowerCtx::RegisterRecordCtor(const std::string& sym,
                                  const std::vector<std::string>& path) {
  record_ctor_paths[sym] = path;
}

const std::vector<std::string>* LowerCtx::LookupRecordCtor(
    const std::string& sym) const {
  auto it = record_ctor_paths.find(sym);
  if (it != record_ctor_paths.end()) {
    return &it->second;
  }
  return nullptr;
}

void LowerCtx::RegisterProcSig(const ProcIR& proc) {
  ProcSigInfo info;
  info.params = proc.params;
  info.ret = proc.ret;
  proc_sigs[proc.symbol] = std::move(info);
}

const LowerCtx::ProcSigInfo* LowerCtx::LookupProcSig(const std::string& sym) const {
  auto it = proc_sigs.find(sym);
  if (it != proc_sigs.end()) {
    return &it->second;
  }
  return nullptr;
}

void LowerCtx::RegisterProcModule(const std::string& sym, const syntax::ModulePath& module_path) {
  proc_modules[sym] = module_path;
}

const std::vector<std::string>* LowerCtx::LookupProcModule(const std::string& sym) const {
  auto it = proc_modules.find(sym);
  if (it != proc_modules.end()) {
    return &it->second;
  }
  return nullptr;
}

const LowerCtx::AsyncProcInfo* LowerCtx::LookupAsyncProc(const std::string& sym) const {
  auto it = async_procs.find(sym);
  if (it != async_procs.end()) {
    return &it->second;
  }
  return nullptr;
}

}  // namespace cursive0::codegen

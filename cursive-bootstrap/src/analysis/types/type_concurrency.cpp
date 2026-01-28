// C0X Extension: Structured Concurrency Type Checking (§10, §18)

#include "cursive0/analysis/types/type_stmt.h"

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/diagnostic_messages.h"
#include "cursive0/analysis/types/type_expr.h"
#include "cursive0/analysis/types/type_equiv.h"
#include "cursive0/analysis/types/subtyping.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/analysis/caps/cap_concurrency.h"
#include "cursive0/analysis/resolve/scopes.h"

#include <unordered_set>
#include <unordered_map>

namespace cursive0::analysis {

namespace {

// Helper: Create unit type (empty tuple)
static TypeRef MakeUnit() {
  return MakeTypeTuple({});
}

static TypeRef MakeNever() {
  return MakeTypePrim("!");
}

static TypeRef StripPermOnce(const TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return perm->base;
  }
  if (const auto* refine = std::get_if<TypeRefine>(&type->node)) {
    return refine->base;
  }
  return type;
}

static TypeRef StripPermAll(const TypeRef& type) {
  TypeRef cur = type;
  while (cur) {
    if (const auto* perm = std::get_if<TypePerm>(&cur->node)) {
      cur = perm->base;
      continue;
    }
    if (const auto* refine = std::get_if<TypeRefine>(&cur->node)) {
      cur = refine->base;
      continue;
    }
    break;
  }
  return cur;
}

static Permission PermOfType(const TypeRef& type) {
  if (!type) {
    return Permission::Const;
  }
  if (const auto* perm = std::get_if<TypePerm>(&type->node)) {
    return perm->perm;
  }
  return Permission::Const;
}

static bool IsRawPointerType(const TypeRef& type) {
  const auto stripped = StripPermOnce(type);
  if (!stripped) {
    return false;
  }
  return std::holds_alternative<TypeRawPtr>(stripped->node);
}

static bool IsManagedHeapType(const TypeRef& type) {
  const auto stripped = StripPermOnce(type);
  if (!stripped) {
    return false;
  }
  if (const auto* str = std::get_if<TypeString>(&stripped->node)) {
    return str->state.has_value() && *str->state == StringState::Managed;
  }
  if (const auto* bytes = std::get_if<TypeBytes>(&stripped->node)) {
    return bytes->state.has_value() && *bytes->state == BytesState::Managed;
  }
  return false;
}

static bool IsPrimType(const TypeRef& type, std::string_view name) {
  const auto stripped = StripPermOnce(type);
  if (!stripped) {
    return false;
  }
  const auto* prim = std::get_if<TypePrim>(&stripped->node);
  return prim && prim->name == name;
}

static bool IsTypePathName(const TypePath& path, std::string_view name) {
  return path.size() == 1 && IdEq(path[0], name);
}

static bool IsNumericType(const TypeRef& type) {
  const auto stripped = StripPermOnce(type);
  if (!stripped) {
    return false;
  }
  const auto* prim = std::get_if<TypePrim>(&stripped->node);
  if (!prim) {
    return false;
  }
  const auto& name = prim->name;
  return name == "i8" || name == "i16" || name == "i32" || name == "i64" ||
         name == "i128" || name == "isize" ||
         name == "u8" || name == "u16" || name == "u32" || name == "u64" ||
         name == "u128" || name == "usize" ||
         name == "f16" || name == "f32" || name == "f64";
}

// Helper: Check if type implements ExecutionDomain class (§18.2.4)
static bool IsExecutionDomain(const TypeRef& type) {
  if (!type) return false;
  
  // Accept $ExecutionDomain dynamic class type
  const auto* dyn = std::get_if<TypeDynamic>(&type->node);
  if (dyn && IsExecutionDomainTypePath(dyn->path)) {
    return true;
  }
  
  // Accept concrete types that implement ExecutionDomain
  const auto* path = std::get_if<TypePathType>(&type->node);
  if (path && IsExecutionDomainTypePath(path->path)) {
    return true;
  }
  
  return false;
}

// Re-export from cap_concurrency.h for local use
using ::cursive0::analysis::MakeSpawnedType;
using ::cursive0::analysis::ExtractSpawnedInner;

// §18.3 Capture Analysis
//
// §18.3.1 const - Capture by reference allowed
// §18.3.2 shared - Capture by reference with key sync
// §18.3.3 unique - MUST NOT implicitly capture, require explicit move
//
// Returns diagnostic code if capture rule violated, std::nullopt if OK
static std::optional<std::string_view> CheckCapturePermission(
    const TypeRef& type,
    bool is_explicit_move) {
  if (!type) return std::nullopt;
  
  // Check if the type has unique permission
  const auto perm = PermOfType(type);
  if (perm == Permission::Unique && !is_explicit_move) {
    // §18.3.3: unique binding captured without explicit move
    return "E-CON-0120";
  }
  
  return std::nullopt;
}

// Check if type is for GPU domain (for GPU capture restrictions §18.2.2)
static bool IsGpuDomain(const TypeRef& domain_type) {
  if (!domain_type) return false;
  
  const auto* dyn = std::get_if<TypeDynamic>(&domain_type->node);
  if (dyn && !dyn->path.empty()) {
    const auto& name = dyn->path.back();
    return name == "GpuDomain" || name == "gpu";
  }
  
  const auto* path = std::get_if<TypePathType>(&domain_type->node);
  if (path && !path->path.empty()) {
    const auto& name = path->path.back();
    return name == "GpuDomain" || name == "gpu";
  }
  
  return false;
}

// GPU-specific capture check (§18.2.2)
static std::optional<std::string_view> CheckGpuCapture(
    const TypeRef& type,
    const TypeRef& domain_type) {
  if (!IsGpuDomain(domain_type)) return std::nullopt;
  if (!type) return std::nullopt;
  
  // §18.2.2: shared binding in GPU domain
  const auto* perm_type = std::get_if<TypePerm>(&type->node);
  if (perm_type && perm_type->perm == Permission::Shared) {
    return "E-CON-0150";
  }
  
  // §18.2.2: host pointer captured in GPU domain
  const auto stripped = StripPermOnce(type);
  if (stripped && std::holds_alternative<TypePtr>(stripped->node)) {
    return "E-CON-0151";
  }
  if (stripped && std::holds_alternative<TypeRawPtr>(stripped->node)) {
    return "E-CON-0151";
  }
  
  // §18.2.2: heap-provenanced value captured in GPU domain
  if (IsManagedHeapType(type)) {
    return "E-CON-0152";
  }
  
  return std::nullopt;
}

static std::optional<IdKey> PlaceRoot(const syntax::ExprPtr& expr) {
  if (!expr) {
    return std::nullopt;
  }
  return std::visit(
      [&](const auto& node) -> std::optional<IdKey> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return IdKeyOf(node.name);
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return PlaceRoot(node.base);
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return PlaceRoot(node.base);
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return PlaceRoot(node.base);
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return PlaceRoot(node.value);
        } else if constexpr (std::is_same_v<T, syntax::MoveExpr>) {
          return PlaceRoot(node.place);
        } else if constexpr (std::is_same_v<T, syntax::AddressOfExpr>) {
          return PlaceRoot(node.place);
        } else {
          return std::nullopt;
        }
      },
      expr->node);
}

static void CollectPatternNames(const syntax::Pattern& pat,
                                std::vector<IdKey>& out);

static void CollectFieldPatNames(const syntax::FieldPattern& field,
                                 std::vector<IdKey>& out) {
  if (field.pattern_opt) {
    CollectPatternNames(*field.pattern_opt, out);
    return;
  }
  out.push_back(IdKeyOf(field.name));
}

static void CollectPatternNames(const syntax::Pattern& pat,
                                std::vector<IdKey>& out) {
  std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
          out.push_back(IdKeyOf(node.name));
        } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
          out.push_back(IdKeyOf(node.name));
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
  std::vector<std::unordered_set<IdKey>> scopes;

  void Push() { scopes.emplace_back(); }
  void Pop() {
    if (!scopes.empty()) {
      scopes.pop_back();
    }
  }
  bool IsLocal(const IdKey& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      if (it->find(name) != it->end()) {
        return true;
      }
    }
    return false;
  }
  void Add(const IdKey& name) {
    if (!scopes.empty()) {
      scopes.back().insert(name);
    }
  }
  void AddAll(const std::vector<IdKey>& names) {
    for (const auto& name : names) {
      Add(name);
    }
  }
};

struct CaptureBinding {
  IdKey name;
  TypeBinding binding;
  bool explicit_move = false;
};

struct CaptureCollector {
  const TypeEnv& env;
  const std::unordered_set<IdKey>& explicit_moves;
  std::unordered_map<IdKey, CaptureBinding> captures;
  std::vector<IdKey> order;
  ScopedNames locals;

  void RecordCapture(std::string_view name) {
    const IdKey key = IdKeyOf(name);
    if (locals.IsLocal(key)) {
      return;
    }
    const auto binding = BindOf(env, name);
    if (!binding.has_value()) {
      return;
    }
    if (captures.find(key) != captures.end()) {
      return;
    }
    CaptureBinding entry;
    entry.name = key;
    entry.binding = *binding;
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
                std::vector<IdKey> names;
                CollectPatternNames(*arm.pattern, names);
                locals.AddAll(names);
              }
              VisitExpr(arm.guard_opt);
              VisitExpr(arm.body);
              locals.Pop();
            }
          } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
            if (node.invariant_opt.has_value()) {
              VisitExpr(node.invariant_opt->predicate);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
            VisitExpr(node.cond);
            if (node.invariant_opt.has_value()) {
              VisitExpr(node.invariant_opt->predicate);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
            VisitExpr(node.iter);
            if (node.invariant_opt.has_value()) {
              VisitExpr(node.invariant_opt->predicate);
            }
            locals.Push();
            if (node.pattern) {
              std::vector<IdKey> names;
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
              std::vector<IdKey> names;
              CollectPatternNames(*node.binding.pat, names);
              locals.AddAll(names);
            }
          } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt> ||
                               std::is_same_v<T, syntax::ShadowVarStmt>) {
            VisitExpr(node.init);
            locals.Add(IdKeyOf(node.name));
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
    const TypeEnv& env,
    const std::unordered_set<IdKey>& explicit_moves) {
  CaptureCollector collector{env, explicit_moves, {}, {}, {}};
  collector.VisitBlock(body);
  std::vector<CaptureBinding> result;
  result.reserve(collector.order.size());
  for (const auto& key : collector.order) {
    result.push_back(collector.captures.at(key));
  }
  return result;
}

static std::unordered_set<IdKey> CollectSpawnMoveCaptures(
    const syntax::SpawnExpr& expr) {
  std::unordered_set<IdKey> moves;
  for (const auto& opt : expr.opts) {
    if (opt.kind != syntax::SpawnOptionKind::MoveCapture) {
      continue;
    }
    const auto root = PlaceRoot(opt.value);
    if (root.has_value()) {
      moves.insert(*root);
    }
  }
  return moves;
}

static const syntax::SpawnExpr* AsSpawnExpr(const syntax::ExprPtr& expr) {
  if (!expr) {
    return nullptr;
  }
  return std::get_if<syntax::SpawnExpr>(&expr->node);
}

static const syntax::DispatchExpr* AsDispatchExpr(const syntax::ExprPtr& expr) {
  if (!expr) {
    return nullptr;
  }
  return std::get_if<syntax::DispatchExpr>(&expr->node);
}

struct UseAfterMoveChecker {
  const std::unordered_set<IdKey>& moved;
  bool found = false;
  ScopedNames locals;

  void VisitExpr(const syntax::ExprPtr& expr) {
    if (!expr || found) {
      return;
    }
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
            const IdKey key = IdKeyOf(node.name);
            if (!locals.IsLocal(key) && moved.find(key) != moved.end()) {
              found = true;
            }
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
                std::vector<IdKey> names;
                CollectPatternNames(*arm.pattern, names);
                locals.AddAll(names);
              }
              VisitExpr(arm.guard_opt);
              VisitExpr(arm.body);
              locals.Pop();
            }
          } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
            if (node.invariant_opt.has_value()) {
              VisitExpr(node.invariant_opt->predicate);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
            VisitExpr(node.cond);
            if (node.invariant_opt.has_value()) {
              VisitExpr(node.invariant_opt->predicate);
            }
            if (node.body) {
              VisitBlock(*node.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
            VisitExpr(node.iter);
            if (node.invariant_opt.has_value()) {
              VisitExpr(node.invariant_opt->predicate);
            }
            locals.Push();
            if (node.pattern) {
              std::vector<IdKey> names;
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
          } else if constexpr (std::is_same_v<T, syntax::SpawnExpr>) {
            for (const auto& opt : node.opts) {
              VisitExpr(opt.value);
            }
          } else if constexpr (std::is_same_v<T, syntax::WaitExpr>) {
            VisitExpr(node.handle);
          } else if constexpr (std::is_same_v<T, syntax::DispatchExpr>) {
            VisitExpr(node.range);
            if (node.key_clause.has_value()) {
              const IdKey key = IdKeyOf(node.key_clause->key_path.root);
              if (!locals.IsLocal(key) && moved.find(key) != moved.end()) {
                found = true;
                return;
              }
              for (const auto& seg : node.key_clause->key_path.segs) {
                if (const auto* idx = std::get_if<syntax::KeySegIndex>(&seg)) {
                  VisitExpr(idx->expr);
                }
              }
            }
            for (const auto& opt : node.opts) {
              VisitExpr(opt.chunk_expr);
            }
          }
        },
        expr->node);
  }

  void VisitStmt(const syntax::Stmt& stmt) {
    if (found) {
      return;
    }
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::LetStmt> ||
                        std::is_same_v<T, syntax::VarStmt>) {
            VisitExpr(node.binding.init);
            if (node.binding.pat) {
              std::vector<IdKey> names;
              CollectPatternNames(*node.binding.pat, names);
              locals.AddAll(names);
            }
          } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt> ||
                               std::is_same_v<T, syntax::ShadowVarStmt>) {
            VisitExpr(node.init);
            locals.Add(IdKeyOf(node.name));
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
          }
        },
        stmt);
  }

  void VisitBlock(const syntax::Block& block) {
    locals.Push();
    for (const auto& stmt : block.stmts) {
      VisitStmt(stmt);
      if (found) {
        break;
      }
    }
    VisitExpr(block.tail_opt);
    locals.Pop();
  }
};

static bool StmtUsesMoved(const syntax::Stmt& stmt,
                          const std::unordered_set<IdKey>& moved) {
  UseAfterMoveChecker checker{moved, false, {}};
  checker.locals.Push();
  checker.VisitStmt(stmt);
  checker.locals.Pop();
  return checker.found;
}

static bool ExprUsesMoved(const syntax::ExprPtr& expr,
                          const std::unordered_set<IdKey>& moved) {
  UseAfterMoveChecker checker{moved, false, {}};
  checker.locals.Push();
  checker.VisitExpr(expr);
  checker.locals.Pop();
  return checker.found;
}

static std::optional<syntax::ExprPtr> WorkExprOfStmt(
    const syntax::Stmt& stmt) {
  return std::visit(
      [&](const auto& node) -> std::optional<syntax::ExprPtr> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::ExprStmt>) {
          if (AsSpawnExpr(node.value) || AsDispatchExpr(node.value)) {
            return node.value;
          }
        } else if constexpr (std::is_same_v<T, syntax::LetStmt> ||
                             std::is_same_v<T, syntax::VarStmt>) {
          if (node.binding.init &&
              (AsSpawnExpr(node.binding.init) || AsDispatchExpr(node.binding.init))) {
            return node.binding.init;
          }
        } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt> ||
                             std::is_same_v<T, syntax::ShadowVarStmt>) {
          if (AsSpawnExpr(node.init) || AsDispatchExpr(node.init)) {
            return node.init;
          }
        }
        return std::nullopt;
      },
      stmt);
}

static bool DispatchHasReduce(const syntax::DispatchExpr& expr) {
  for (const auto& opt : expr.opts) {
    if (opt.kind == syntax::DispatchOptionKind::Reduce) {
      return true;
    }
  }
  return false;
}

static bool IsCollectableParallelExpr(const syntax::ExprPtr& expr,
                                      bool& is_spawn) {
  if (!expr) {
    return false;
  }
  if (std::holds_alternative<syntax::SpawnExpr>(expr->node)) {
    is_spawn = true;
    return true;
  }
  if (const auto* dispatch = std::get_if<syntax::DispatchExpr>(&expr->node)) {
    if (DispatchHasReduce(*dispatch)) {
      is_spawn = false;
      return true;
    }
  }
  return false;
}

static std::vector<TypeRef> CollectParallelResultTypes(
    const ScopeContext& ctx,
    const StmtTypeContext& type_ctx,
    const syntax::Block& block,
    const TypeEnv& env,
    const ExprTypeFn& type_expr,
    const IdentTypeFn& type_ident,
    const PlaceTypeFn& type_place,
    std::optional<std::string_view>& diag_id,
    bool& has_explicit_result) {
  std::vector<TypeRef> out;
  has_explicit_result = false;
  TypeEnv current = PushScope(env);
  StmtTypeContext scan_ctx = type_ctx;
  scan_ctx.diags = nullptr;
  scan_ctx.unique_captures = nullptr;
  scan_ctx.env_ref = &current;

  auto scan_type_expr = [&](const syntax::ExprPtr& expr) {
    return TypeExpr(ctx, scan_ctx, expr, current);
  };
  auto scan_type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                              current);
  };
  auto scan_type_place = [&](const syntax::ExprPtr& expr) -> PlaceTypeResult {
    return TypePlace(ctx, scan_ctx, expr, current);
  };

  auto get_expr_type = [&](const syntax::ExprPtr& expr,
                           const TypeEnv& expr_env) -> std::optional<TypeRef> {
    if (!expr) {
      return std::nullopt;
    }
    if (ctx.expr_types) {
      const auto it = ctx.expr_types->find(expr.get());
      if (it != ctx.expr_types->end()) {
        return it->second;
      }
    }
    const auto typed = TypeExpr(ctx, scan_ctx, expr, expr_env);
    if (!typed.ok) {
      diag_id = typed.diag_id;
      return std::nullopt;
    }
    return typed.type;
  };

  for (const auto& stmt : block.stmts) {
    if (const auto* expr_stmt = std::get_if<syntax::ExprStmt>(&stmt)) {
      bool is_spawn = false;
      if (IsCollectableParallelExpr(expr_stmt->value, is_spawn)) {
        const auto expr_type = get_expr_type(expr_stmt->value, current);
        if (!expr_type) {
          return out;
        }
        if (is_spawn) {
          const auto inner = ExtractSpawnedInner(*expr_type);
          if (!inner) {
            diag_id = "E-CON-0132";
            return out;
          }
          out.push_back(*inner);
        } else {
          out.push_back(*expr_type);
        }
      }
    }

    const auto typed = TypeStmt(ctx, scan_ctx, stmt, current, scan_type_expr,
                                scan_type_ident, scan_type_place);
    if (!typed.ok) {
      diag_id = typed.diag_id;
      return out;
    }
    if (!typed.flow.results.empty()) {
      has_explicit_result = true;
    }
    current = typed.env;
  }

  if (block.tail_opt) {
    bool is_spawn = false;
    if (IsCollectableParallelExpr(block.tail_opt, is_spawn)) {
      const auto expr_type = get_expr_type(block.tail_opt, current);
      if (!expr_type) {
        return out;
      }
      if (is_spawn) {
        const auto inner = ExtractSpawnedInner(*expr_type);
        if (!inner) {
          diag_id = "E-CON-0132";
          return out;
        }
        out.push_back(*inner);
      } else {
        out.push_back(*expr_type);
      }
    } else {
      has_explicit_result = true;
    }
  }

  return out;
}

struct IntroResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeEnv env;
};

static bool InScope(const TypeEnv& env, const IdKey& key) {
  if (env.scopes.empty()) {
    return false;
  }
  return env.scopes.back().find(key) != env.scopes.back().end();
}

static bool InOuter(const TypeEnv& env, const IdKey& key) {
  if (env.scopes.size() < 2) {
    return false;
  }
  for (std::size_t i = 0; i + 1 < env.scopes.size(); ++i) {
    if (env.scopes[i].find(key) != env.scopes[i].end()) {
      return true;
    }
  }
  return false;
}

static IntroResult IntroBinding(const TypeEnv& env,
                                std::string_view name,
                                const TypeBinding& binding) {
  if (ReservedGen(name)) {
    SPEC_RULE("Intro-Reserved-Gen-Err");
    return {false, "Intro-Reserved-Gen-Err", env};
  }
  if (ReservedCursive(name)) {
    SPEC_RULE("Intro-Reserved-Cursive-Err");
    return {false, "Intro-Reserved-Cursive-Err", env};
  }

  const auto key = IdKeyOf(name);
  if (env.scopes.empty()) {
    return {false, std::nullopt, env};
  }
  if (InScope(env, key)) {
    SPEC_RULE("Intro-Dup");
    return {false, std::nullopt, env};
  }
  if (InOuter(env, key)) {
    SPEC_RULE("Intro-Shadow-Required");
    return {false, "Intro-Shadow-Required", env};
  }

  TypeEnv out = env;
  out.scopes.back().emplace(key, binding);
  SPEC_RULE("Intro-Ok");
  return {true, std::nullopt, std::move(out)};
}

static IntroResult IntroAll(const TypeEnv& env,
                            const std::vector<std::pair<std::string, TypeRef>>& binds) {
  if (binds.empty()) {
    SPEC_RULE("IntroAll-Empty");
    return {true, std::nullopt, env};
  }
  TypeEnv current = env;
  for (const auto& [name, type] : binds) {
    TypeBinding binding{syntax::Mutability::Let, type};
    const auto res = IntroBinding(current, name, binding);
    if (!res.ok) {
      return res;
    }
    current = res.env;
    SPEC_RULE("IntroAll-Cons");
  }
  return {true, std::nullopt, std::move(current)};
}

static TypeRef MakeTypePathWithArgs(TypePath path, std::vector<TypeRef> args) {
  TypePathType node;
  node.path = std::move(path);
  node.generic_args = std::move(args);
  return MakeType(node);
}

static bool AllEqTypes(const std::vector<TypeRef>& types,
                       std::optional<std::string_view>& diag_id) {
  if (types.empty()) {
    return true;
  }
  const auto& base = types.front();
  for (std::size_t i = 1; i < types.size(); ++i) {
    const auto eq = TypeEquiv(base, types[i]);
    if (!eq.ok) {
      diag_id = eq.diag_id;
      return false;
    }
    if (!eq.equiv) {
      return false;
    }
  }
  return true;
}

struct YieldFinder {
  bool want_from = false;
  bool found = false;

  void VisitExpr(const syntax::ExprPtr& expr) {
    if (!expr || found) {
      return;
    }
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::YieldExpr>) {
            if (!want_from) {
              found = true;
              return;
            }
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, syntax::YieldFromExpr>) {
            if (want_from) {
              found = true;
              return;
            }
            VisitExpr(node.value);
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
                  } else if constexpr (std::is_same_v<P, syntax::EnumPayloadBrace>) {
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
              VisitExpr(arm.guard_opt);
              VisitExpr(arm.body);
            }
          } else if constexpr (std::is_same_v<T, syntax::LoopInfiniteExpr>) {
            if (node.invariant_opt.has_value()) {
              VisitExpr(node.invariant_opt->predicate);
            }
            VisitBlock(node.body);
          } else if constexpr (std::is_same_v<T, syntax::LoopConditionalExpr>) {
            VisitExpr(node.cond);
            if (node.invariant_opt.has_value()) {
              VisitExpr(node.invariant_opt->predicate);
            }
            VisitBlock(node.body);
          } else if constexpr (std::is_same_v<T, syntax::LoopIterExpr>) {
            VisitExpr(node.iter);
            if (node.invariant_opt.has_value()) {
              VisitExpr(node.invariant_opt->predicate);
            }
            VisitBlock(node.body);
          } else if constexpr (std::is_same_v<T, syntax::BlockExpr>) {
            VisitBlock(node.block);
          } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockExpr>) {
            VisitBlock(node.block);
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
          } else if constexpr (std::is_same_v<T, syntax::SyncExpr>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, syntax::RaceExpr>) {
            for (const auto& arm : node.arms) {
              VisitExpr(arm.expr);
              VisitExpr(arm.handler.value);
            }
          } else if constexpr (std::is_same_v<T, syntax::AllExpr>) {
            for (const auto& elem : node.exprs) {
              VisitExpr(elem);
            }
          } else if constexpr (std::is_same_v<T, syntax::ParallelExpr>) {
            VisitExpr(node.domain);
            for (const auto& opt : node.opts) {
              VisitExpr(opt.value);
            }
            VisitBlock(node.body);
          } else if constexpr (std::is_same_v<T, syntax::SpawnExpr>) {
            for (const auto& opt : node.opts) {
              VisitExpr(opt.value);
            }
            VisitBlock(node.body);
          } else if constexpr (std::is_same_v<T, syntax::WaitExpr>) {
            VisitExpr(node.handle);
          } else if constexpr (std::is_same_v<T, syntax::DispatchExpr>) {
            VisitExpr(node.range);
            for (const auto& opt : node.opts) {
              if (opt.kind == syntax::DispatchOptionKind::Chunk) {
                VisitExpr(opt.chunk_expr);
              }
            }
            VisitBlock(node.body);
          } else if constexpr (std::is_same_v<T, syntax::ResultExpr> ||
                               std::is_same_v<T, syntax::IdentifierExpr> ||
                               std::is_same_v<T, syntax::QualifiedNameExpr> ||
                               std::is_same_v<T, syntax::PathExpr> ||
                               std::is_same_v<T, syntax::PtrNullExpr> ||
                               std::is_same_v<T, syntax::LiteralExpr> ||
                               std::is_same_v<T, syntax::ErrorExpr>) {
            return;
          } else {
            return;
          }
        },
        expr->node);
  }

  void VisitStmt(const syntax::Stmt& stmt) {
    if (found) {
      return;
    }
    std::visit(
        [&](const auto& node) {
          using T = std::decay_t<decltype(node)>;
          if constexpr (std::is_same_v<T, syntax::LetStmt> ||
                        std::is_same_v<T, syntax::VarStmt>) {
            VisitExpr(node.binding.init);
          } else if constexpr (std::is_same_v<T, syntax::ShadowLetStmt> ||
                               std::is_same_v<T, syntax::ShadowVarStmt>) {
            VisitExpr(node.init);
          } else if constexpr (std::is_same_v<T, syntax::AssignStmt> ||
                               std::is_same_v<T, syntax::CompoundAssignStmt>) {
            VisitExpr(node.place);
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, syntax::ExprStmt>) {
            VisitExpr(node.value);
          } else if constexpr (std::is_same_v<T, syntax::DeferStmt>) {
            VisitBlock(node.body);
          } else if constexpr (std::is_same_v<T, syntax::RegionStmt>) {
            VisitExpr(node.opts_opt);
            VisitBlock(node.body);
          } else if constexpr (std::is_same_v<T, syntax::FrameStmt>) {
            VisitBlock(node.body);
          } else if constexpr (std::is_same_v<T, syntax::ReturnStmt>) {
            VisitExpr(node.value_opt);
          } else if constexpr (std::is_same_v<T, syntax::BreakStmt>) {
            VisitExpr(node.value_opt);
          } else if constexpr (std::is_same_v<T, syntax::UnsafeBlockStmt>) {
            VisitBlock(node.body);
          } else if constexpr (std::is_same_v<T, syntax::KeyBlockStmt>) {
            VisitBlock(node.body);
          } else {
            return;
          }
        },
        stmt);
  }

  void VisitBlock(const std::shared_ptr<syntax::Block>& block) {
    if (!block || found) {
      return;
    }
    for (const auto& stmt : block->stmts) {
      VisitStmt(stmt);
      if (found) {
        return;
      }
    }
    VisitExpr(block->tail_opt);
  }
};

static bool YieldInExpr(const syntax::ExprPtr& expr) {
  YieldFinder finder;
  finder.want_from = false;
  finder.VisitExpr(expr);
  return finder.found;
}

static bool YieldFromInExpr(const syntax::ExprPtr& expr) {
  YieldFinder finder;
  finder.want_from = true;
  finder.VisitExpr(expr);
  return finder.found;
}

}  // namespace

// -----------------------------------------------------------------------------
// Async signature helper (§5.2.9, §5.4.5, §19)
// -----------------------------------------------------------------------------

std::optional<AsyncSig> AsyncSigOf(const ScopeContext& /*ctx*/,
                                   const TypeRef& type) {
  const auto stripped = StripPermAll(type);
  if (!stripped) {
    return std::nullopt;
  }

  auto sig_from_args = [&](const std::vector<TypeRef>& args) -> std::optional<AsyncSig> {
    if (args.empty() || args.size() > 4) {
      return std::nullopt;
    }
    AsyncSig sig;
    sig.out = args[0];
    sig.in = args.size() > 1 ? args[1] : MakeUnit();
    sig.result = args.size() > 2 ? args[2] : MakeUnit();
    sig.err = args.size() > 3 ? args[3] : MakeNever();
    return sig;
  };

  if (const auto* modal = std::get_if<TypeModalState>(&stripped->node)) {
    if (IsTypePathName(modal->path, "Async")) {
      return sig_from_args(modal->generic_args);
    }
  }
  if (const auto* path = std::get_if<TypePathType>(&stripped->node)) {
    if (IsTypePathName(path->path, "Async")) {
      return sig_from_args(path->generic_args);
    }
    if (IsTypePathName(path->path, "Sequence")) {
      if (path->generic_args.size() != 1) {
        return std::nullopt;
      }
      AsyncSig sig;
      sig.out = path->generic_args[0];
      sig.in = MakeUnit();
      sig.result = MakeUnit();
      sig.err = MakeNever();
      return sig;
    }
    if (IsTypePathName(path->path, "Future")) {
      if (path->generic_args.empty() || path->generic_args.size() > 2) {
        return std::nullopt;
      }
      AsyncSig sig;
      sig.out = MakeUnit();
      sig.in = MakeUnit();
      sig.result = path->generic_args[0];
      sig.err = path->generic_args.size() > 1 ? path->generic_args[1] : MakeNever();
      return sig;
    }
    if (IsTypePathName(path->path, "Stream")) {
      if (path->generic_args.size() != 2) {
        return std::nullopt;
      }
      AsyncSig sig;
      sig.out = path->generic_args[0];
      sig.in = MakeUnit();
      sig.result = MakeUnit();
      sig.err = path->generic_args[1];
      return sig;
    }
    if (IsTypePathName(path->path, "Pipe")) {
      if (path->generic_args.size() != 2) {
        return std::nullopt;
      }
      AsyncSig sig;
      sig.out = path->generic_args[1];
      sig.in = path->generic_args[0];
      sig.result = MakeUnit();
      sig.err = MakeNever();
      return sig;
    }
    if (IsTypePathName(path->path, "Exchange")) {
      if (path->generic_args.size() != 1) {
        return std::nullopt;
      }
      AsyncSig sig;
      sig.out = path->generic_args[0];
      sig.in = path->generic_args[0];
      sig.result = path->generic_args[0];
      sig.err = MakeNever();
      return sig;
    }
  }

  return std::nullopt;
}

// §18.1.1 T-Parallel: Type checking for parallel block expression
//
// Γ ⊢ D : `$ExecutionDomain`    Γ_P = Γ[parallel_context ↦ D]    Γ_P ⊢ B : T
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ `parallel` D {B} : T
//
ExprTypeResult TypeParallelExpr(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::ParallelExpr& expr,
                                const TypeEnv& env,
                                const ExprTypeFn& type_expr,
                                const IdentTypeFn& type_ident,
                                const PlaceTypeFn& type_place) {
  SPEC_RULE("T-Parallel");
  ExprTypeResult result;

  // Type check domain expression
  ExprTypeResult domain_result = type_expr(expr.domain);
  if (!domain_result.ok) {
    result.diag_id = domain_result.diag_id;
    return result;
  }

  // Check domain implements ExecutionDomain (§18.1.1)
  if (!IsExecutionDomain(domain_result.type)) {
    result.diag_id = "E-CON-0101";  // Domain does not implement ExecutionDomain
    return result;
  }

  // GPU domain cannot be nested inside another GPU parallel block (§18.2.3)
  if (type_ctx.in_parallel &&
      IsGpuDomain(type_ctx.parallel_domain) &&
      IsGpuDomain(domain_result.type)) {
    result.diag_id = "E-CON-0153";
    return result;
  }

  // Create parallel context for body type checking
  StmtTypeContext parallel_ctx = type_ctx;
  parallel_ctx.in_parallel = true;
  parallel_ctx.parallel_domain = domain_result.type;
  std::unordered_set<IdKey> unique_captures;
  parallel_ctx.unique_captures = &unique_captures;
  TypeEnv parallel_env = env;
  parallel_ctx.env_ref = &parallel_env;

  // Type check body with parallel context
  if (!expr.body) {
    result.ok = true;
    result.type = MakeUnit();
    return result;
  }

  // Create type checking functions that use the parallel context
  auto active_env = [&]() -> const TypeEnv& {
    return parallel_ctx.env_ref ? *parallel_ctx.env_ref : env;
  };
  auto parallel_type_expr = [&](const syntax::ExprPtr& inner) {
    return TypeExpr(ctx, parallel_ctx, inner, active_env());
  };
  auto parallel_type_ident = [&](std::string_view name) -> ExprTypeResult {
    return TypeIdentifierExpr(ctx, syntax::IdentifierExpr{std::string(name)},
                              active_env());
  };
  auto parallel_type_place = [&](const syntax::ExprPtr& inner) -> PlaceTypeResult {
    return TypePlace(ctx, parallel_ctx, inner, active_env());
  };

  ExprTypeResult body_result = TypeBlock(ctx, parallel_ctx, *expr.body, parallel_env,
                                         parallel_type_expr, parallel_type_ident,
                                         parallel_type_place);
  if (!body_result.ok) {
    result.diag_id = body_result.diag_id;
    return result;
  }

  // Use-after-move check for explicit spawn moves (§18.3.3)
  std::unordered_set<IdKey> moved;
  for (const auto& stmt : expr.body->stmts) {
    if (StmtUsesMoved(stmt, moved)) {
      result.diag_id = "E-CON-0122";
      return result;
    }
    const auto work_expr = WorkExprOfStmt(stmt);
    if (!work_expr.has_value()) {
      continue;
    }
    if (const auto* spawn = AsSpawnExpr(*work_expr)) {
      const auto moves = CollectSpawnMoveCaptures(*spawn);
      moved.insert(moves.begin(), moves.end());
    }
  }
  if (ExprUsesMoved(expr.body->tail_opt, moved)) {
    result.diag_id = "E-CON-0122";
    return result;
  }

  // Determine result type from collected work item results (§18.1.3)
  std::optional<std::string_view> collect_diag;
  bool has_explicit_result = false;
  const auto results = CollectParallelResultTypes(
      ctx,
      parallel_ctx,
      *expr.body,
      env,
      parallel_type_expr,
      parallel_type_ident,
      parallel_type_place,
      collect_diag,
      has_explicit_result);
  if (collect_diag.has_value()) {
    result.diag_id = collect_diag;
    return result;
  }
  if (has_explicit_result) {
    result.ok = true;
    result.type = body_result.type;
    return result;
  }
  if (results.empty()) {
    result.ok = true;
    result.type = MakeUnit();
    return result;
  }
  if (results.size() == 1) {
    result.ok = true;
    result.type = results[0];
    return result;
  }
  result.ok = true;
  result.type = MakeTypeTuple(results);
  return result;
}

// §18.4.1 T-Spawn: Type checking for spawn expression
//
// Γ[parallel_context] = D    Γ_capture ⊢ e : T
// ────────────────────────────────────────────────────────────
// Γ ⊢ `spawn` {e} : Spawned⟨T⟩
//
ExprTypeResult TypeSpawnExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::SpawnExpr& expr,
                             const TypeEnv& env,
                             const ExprTypeFn& type_expr,
                             const IdentTypeFn& type_ident,
                             const PlaceTypeFn& type_place) {
  SPEC_RULE("T-Spawn");
  ExprTypeResult result;

  // Check that we're inside a parallel block (§18.4.1)
  if (!type_ctx.in_parallel) {
    result.diag_id = "E-CON-0130";  // spawn without enclosing parallel block
    return result;
  }

  // Type check spawn body
  if (!expr.body) {
    result.ok = true;
    result.type = MakeSpawnedType(MakeUnit());
    return result;
  }

  ExprTypeResult body_result = TypeBlock(ctx, type_ctx, *expr.body, env,
                                         type_expr, type_ident, type_place);
  if (!body_result.ok) {
    result.diag_id = body_result.diag_id;
    return result;
  }

  // Capture analysis for spawn body (§18.3)
  const auto explicit_moves = CollectSpawnMoveCaptures(expr);
  const auto captures = CollectCaptures(*expr.body, env, explicit_moves);
  for (const auto& capture : captures) {
    const auto& binding = capture.binding;
    if (IsRawPointerType(binding.type)) {
      result.diag_id = "E-CON-0103";
      return result;
    }
    if (const auto gpu_diag = CheckGpuCapture(binding.type, type_ctx.parallel_domain)) {
      result.diag_id = *gpu_diag;
      return result;
    }
    if (const auto cap_diag = CheckCapturePermission(binding.type, capture.explicit_move)) {
      result.diag_id = *cap_diag;
      return result;
    }
    const auto perm = PermOfType(binding.type);
    if (binding.mut == syntax::Mutability::Var &&
        perm != Permission::Shared &&
        !(perm == Permission::Unique && capture.explicit_move)) {
      result.diag_id = "E-CON-0131";
      return result;
    }
    if (perm == Permission::Unique && type_ctx.unique_captures) {
      if (type_ctx.unique_captures->find(capture.name) !=
          type_ctx.unique_captures->end()) {
        result.diag_id = "E-CON-0121";
        return result;
      }
      type_ctx.unique_captures->insert(capture.name);
    }
  }

  // Return type is Spawned<T> where T is body type (§18.4.2)
  result.ok = true;
  result.type = MakeSpawnedType(body_result.type);
  return result;
}

// §10.3 T-Wait: Type checking for wait expression
//
// Γ ⊢ h : Spawned⟨T⟩
// ──────────────────────────────────────────
// Γ ⊢ `wait` h : T
//
// Key restriction: "wait is well-formed only when the current key context is empty"
//
// Note: wait implicitly consumes (moves) the handle, so we use place typing
// to allow non-Bitcopy Spawned values.
//
ExprTypeResult TypeWaitExpr(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::WaitExpr& expr,
                            const TypeEnv& env,
                            const ExprTypeFn& type_expr,
                            const PlaceTypeFn& type_place) {
  SPEC_RULE("T-Wait");
  ExprTypeResult result;

  // Check key restriction (§10.3)
  if (type_ctx.keys_held) {
    result.diag_id = "E-CON-0133";  // wait while key is held
    return result;
  }

  // Type check handle expression as a place (wait implicitly moves the handle)
  // This allows non-Bitcopy Spawned values to be used directly
  PlaceTypeResult handle_result = type_place(expr.handle);
  if (!handle_result.ok) {
    // If place typing fails, try value typing (for expressions that are values)
    ExprTypeResult value_result = type_expr(expr.handle);
    if (!value_result.ok) {
      result.diag_id = value_result.diag_id;
      return result;
    }
    // Check that handle is Spawned<T>
    const auto stripped = StripPermAll(value_result.type);
    auto inner = ExtractSpawnedInner(stripped);
    if (inner) {
      result.ok = true;
      result.type = *inner;
      return result;
    }
    const auto future_args = ExtractTrackedArgs(stripped);
    if (!future_args.has_value()) {
      result.diag_id = "E-CON-0132";  // wait operand is not Spawned/Tracked
      return result;
    }
    result.ok = true;
    result.type = MakeTypeUnion({future_args->first, future_args->second});
    return result;
  }

  const auto stripped = StripPermAll(handle_result.type);
  auto inner = ExtractSpawnedInner(stripped);
  if (inner) {
    result.ok = true;
    result.type = *inner;
    return result;
  }
  const auto future_args = ExtractTrackedArgs(stripped);
  if (!future_args.has_value()) {
    result.diag_id = "E-CON-0132";  // wait operand is not Spawned/Tracked
    return result;
  }

  result.ok = true;
  result.type = MakeTypeUnion({future_args->first, future_args->second});
  return result;
}

// §18.5.1 T-Dispatch: Type checking for dispatch expression
//
// Without reduction:
// Γ ⊢ range : Range⟨I⟩    Γ, i : I ⊢ B : T
// ──────────────────────────────────────────────────────────
// Γ ⊢ `dispatch` i `in range` {B} : ()
//
// With reduction:
// Γ ⊢ range : Range⟨I⟩    Γ, i : I ⊢ B : T    Γ ⊢ ⊕ : (T, T) → T
// ─────────────────────────────────────────────────────────────────────
// Γ ⊢ `dispatch` i `in range [reduce: ⊕]` {B} : T
//
ExprTypeResult TypeDispatchExpr(const ScopeContext& ctx,
                                const StmtTypeContext& type_ctx,
                                const syntax::DispatchExpr& expr,
                                const TypeEnv& env,
                                const ExprTypeFn& type_expr,
                                const IdentTypeFn& type_ident,
                                const PlaceTypeFn& type_place) {
  SPEC_RULE("T-Dispatch");
  ExprTypeResult result;

  // Check that we're inside a parallel block (§18.5.1)
  if (!type_ctx.in_parallel) {
    result.diag_id = "E-CON-0140";  // dispatch without enclosing parallel block
    return result;
  }

  // Type check range expression
  ExprTypeResult range_result = type_expr(expr.range);
  if (!range_result.ok) {
    result.diag_id = range_result.diag_id;
    return result;
  }

  // Check range is iterable (Range<I> or similar)
  const auto stripped_range = StripPermOnce(range_result.type);
  const auto* range_type = stripped_range
                               ? std::get_if<TypeRange>(&stripped_range->node)
                               : nullptr;
  if (!range_type) {
    result.diag_id = "E-CON-0141";  // range expression not iterable
    return result;
  }
  // Range type - default to usize for iteration
  TypeRef index_type = MakeTypePrim("usize");

  // Validate key clause root binding (§18.5.1)
  if (expr.key_clause.has_value()) {
    const auto binding = BindOf(env, expr.key_clause->key_path.root);
    if (!binding.has_value()) {
      result.diag_id = "E-CON-0143";  // key clause ill-formed
      return result;
    }
  }

  // Add pattern binding to environment
  TypeEnv body_env = PushScope(env);
  if (expr.pattern) {
    PatternTypeResult pat_result = TypePattern(ctx, expr.pattern, index_type);
    if (!pat_result.ok) {
      result.diag_id = pat_result.diag_id;
      return result;
    }
    // Add bindings to environment
    for (const auto& [name, type] : pat_result.bindings) {
      body_env.scopes.back()[name] = TypeBinding{syntax::Mutability::Let, type};
    }
  }

  // Type check body
  if (!expr.body) {
    result.ok = true;
    result.type = MakeUnit();
    return result;
  }

  ExprTypeResult body_result = TypeBlock(ctx, type_ctx, *expr.body, body_env,
                                         type_expr, type_ident, type_place);
  if (!body_result.ok) {
    result.diag_id = body_result.diag_id;
    return result;
  }

  // Capture analysis for dispatch body (§18.3)
  const std::unordered_set<IdKey> explicit_moves;
  const auto captures = CollectCaptures(*expr.body, env, explicit_moves);
  for (const auto& capture : captures) {
    const auto& binding = capture.binding;
    if (IsRawPointerType(binding.type)) {
      result.diag_id = "E-CON-0103";
      return result;
    }
    if (const auto gpu_diag = CheckGpuCapture(binding.type, type_ctx.parallel_domain)) {
      result.diag_id = *gpu_diag;
      return result;
    }
    if (const auto cap_diag = CheckCapturePermission(binding.type, capture.explicit_move)) {
      result.diag_id = *cap_diag;
      return result;
    }
    const auto perm = PermOfType(binding.type);
    if (binding.mut == syntax::Mutability::Var &&
        perm != Permission::Shared &&
        !(perm == Permission::Unique && capture.explicit_move)) {
      result.diag_id = "E-CON-0131";
      return result;
    }
    if (perm == Permission::Unique && type_ctx.unique_captures) {
      if (type_ctx.unique_captures->find(capture.name) !=
          type_ctx.unique_captures->end()) {
        result.diag_id = "E-CON-0121";
        return result;
      }
      type_ctx.unique_captures->insert(capture.name);
    }
  }

  // Check for reduce option to determine result type
  bool has_reduce = false;
  bool ordered = false;
  const syntax::DispatchOption* reduce_opt = nullptr;
  for (const auto& opt : expr.opts) {
    if (opt.kind == syntax::DispatchOptionKind::Reduce) {
      has_reduce = true;
      reduce_opt = &opt;
    } else if (opt.kind == syntax::DispatchOptionKind::Ordered) {
      ordered = true;
    } else if (opt.kind == syntax::DispatchOptionKind::Chunk) {
      if (opt.chunk_expr) {
        ExprTypeResult chunk_result = type_expr(opt.chunk_expr);
        if (!chunk_result.ok) {
          result.diag_id = chunk_result.diag_id;
          return result;
        }
      }
    }
  }

  if (has_reduce && reduce_opt) {
    // Validate reduce operator type compatibility (§18.5.1)
    if (reduce_opt->reduce_op == syntax::ReduceOp::Custom) {
      ExprTypeResult op_result = type_ident(reduce_opt->custom_reduce_name);
      if (!op_result.ok) {
        result.diag_id = op_result.diag_id;
        return result;
      }
      const auto stripped = StripPermOnce(op_result.type);
      const auto* func = stripped ? std::get_if<TypeFunc>(&stripped->node) : nullptr;
      if (!func || func->params.size() != 2) {
        result.diag_id = "E-CON-0142";
        return result;
      }
      const auto eq_left = TypeEquiv(func->params[0].type, body_result.type);
      const auto eq_right = TypeEquiv(func->params[1].type, body_result.type);
      const auto eq_ret = TypeEquiv(func->ret, body_result.type);
      if (!eq_left.ok || !eq_left.equiv ||
          !eq_right.ok || !eq_right.equiv ||
          !eq_ret.ok || !eq_ret.equiv) {
        result.diag_id = "E-CON-0142";
        return result;
      }
      if (type_ctx.diags && !ordered) {
        if (const auto diag = core::MakeDiagnostic("W-CON-0140", reduce_opt->span)) {
          *type_ctx.diags = core::Emit(*type_ctx.diags, *diag);
        }
      }
    } else if (reduce_opt->reduce_op == syntax::ReduceOp::And ||
               reduce_opt->reduce_op == syntax::ReduceOp::Or) {
      if (!IsPrimType(body_result.type, "bool")) {
        result.diag_id = "E-CON-0142";
        return result;
      }
    } else {
      if (!IsNumericType(body_result.type)) {
        result.diag_id = "E-CON-0142";
        return result;
      }
    }

    // T-Dispatch-Reduce: result type is body type
    result.ok = true;
    result.type = body_result.type;
    return result;
  }

  // T-Dispatch: result type is unit
  result.ok = true;
  result.type = MakeUnit();
  return result;
}

// -----------------------------------------------------------------------------
// §19 Async expressions
// -----------------------------------------------------------------------------

ExprTypeResult TypeYieldExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::YieldExpr& expr,
                             const TypeEnv& env,
                             const ExprTypeFn& type_expr) {
  SPEC_RULE("T-Yield");
  ExprTypeResult result;

  const auto async_sig = AsyncSigOf(ctx, type_ctx.return_type);
  if (!async_sig.has_value()) {
    result.diag_id = "E-CON-0210";
    return result;
  }

  const auto value_result = type_expr(expr.value);
  if (!value_result.ok) {
    result.diag_id = value_result.diag_id;
    return result;
  }

  const auto sub = Subtyping(ctx, value_result.type, async_sig->out);
  if (!sub.ok) {
    result.diag_id = sub.diag_id;
    return result;
  }
  if (!sub.subtype) {
    result.diag_id = "E-CON-0211";
    return result;
  }

  if (type_ctx.keys_held && !expr.release) {
    result.diag_id = "E-CON-0213";
    return result;
  }

  result.ok = true;
  result.type = async_sig->in;
  return result;
}

ExprTypeResult TypeYieldFromExpr(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::YieldFromExpr& expr,
                                 const TypeEnv& env,
                                 const ExprTypeFn& type_expr) {
  SPEC_RULE("T-Yield-From");
  ExprTypeResult result;

  const auto async_sig = AsyncSigOf(ctx, type_ctx.return_type);
  if (!async_sig.has_value()) {
    result.diag_id = "E-CON-0220";
    return result;
  }

  const auto value_result = type_expr(expr.value);
  if (!value_result.ok) {
    result.diag_id = value_result.diag_id;
    return result;
  }

  const auto child_sig = AsyncSigOf(ctx, value_result.type);
  if (!child_sig.has_value()) {
    result.diag_id = "E-CON-0221";
    return result;
  }

  const auto out_eq = TypeEquiv(async_sig->out, child_sig->out);
  if (!out_eq.ok) {
    result.diag_id = out_eq.diag_id;
    return result;
  }
  if (!out_eq.equiv) {
    result.diag_id = "E-CON-0221";
    return result;
  }

  const auto in_eq = TypeEquiv(async_sig->in, child_sig->in);
  if (!in_eq.ok) {
    result.diag_id = in_eq.diag_id;
    return result;
  }
  if (!in_eq.equiv) {
    result.diag_id = "E-CON-0222";
    return result;
  }

  const auto err_sub = Subtyping(ctx, child_sig->err, async_sig->err);
  if (!err_sub.ok) {
    result.diag_id = err_sub.diag_id;
    return result;
  }
  if (!err_sub.subtype) {
    result.diag_id = "E-CON-0225";
    return result;
  }

  if (type_ctx.keys_held && !expr.release) {
    result.diag_id = "E-CON-0224";
    return result;
  }

  result.ok = true;
  result.type = child_sig->result;
  return result;
}

ExprTypeResult TypeSyncExpr(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::SyncExpr& expr,
                            const TypeEnv& env,
                            const ExprTypeFn& type_expr) {
  SPEC_RULE("T-Sync");
  ExprTypeResult result;

  if (YieldInExpr(expr.value)) {
    result.diag_id = "E-CON-0212";
    return result;
  }
  if (YieldFromInExpr(expr.value)) {
    result.diag_id = "E-CON-0223";
    return result;
  }

  if (AsyncSigOf(ctx, type_ctx.return_type).has_value()) {
    result.diag_id = "E-CON-0250";
    return result;
  }

  const auto value_result = type_expr(expr.value);
  if (!value_result.ok) {
    result.diag_id = value_result.diag_id;
    return result;
  }

  const auto async_sig = AsyncSigOf(ctx, value_result.type);
  if (!async_sig.has_value() || !IsPrimType(async_sig->out, "()")) {
    result.diag_id = "E-CON-0251";
    return result;
  }
  if (!IsPrimType(async_sig->in, "()")) {
    result.diag_id = "E-CON-0252";
    return result;
  }

  result.ok = true;
  result.type = MakeTypeUnion({async_sig->result, async_sig->err});
  return result;
}

ExprTypeResult TypeRaceExpr(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::RaceExpr& expr,
                            const TypeEnv& env,
                            const ExprTypeFn& type_expr,
                            const IdentTypeFn& type_ident,
                            const PlaceTypeFn& type_place) {
  SPEC_RULE("T-Race");
  ExprTypeResult result;

  if (expr.arms.size() < 2) {
    result.diag_id = "E-CON-0260";
    return result;
  }

  bool any_return = false;
  bool any_yield = false;
  for (const auto& arm : expr.arms) {
    if (arm.handler.kind == syntax::RaceHandlerKind::Yield) {
      any_yield = true;
    } else {
      any_return = true;
    }
  }
  if (any_return && any_yield) {
    result.diag_id = "E-CON-0263";
    return result;
  }

  const bool yield_mode = any_yield;
  std::vector<TypeRef> handler_types;
  std::vector<TypeRef> error_types;
  handler_types.reserve(expr.arms.size());
  error_types.reserve(expr.arms.size());

  for (const auto& arm : expr.arms) {
    const auto expr_result = type_expr(arm.expr);
    if (!expr_result.ok) {
      result.diag_id = expr_result.diag_id;
      return result;
    }
    const auto async_sig = AsyncSigOf(ctx, expr_result.type);
    if (!async_sig.has_value()) {
      result.diag_id = "E-CON-0261";
      return result;
    }

    if (!yield_mode) {
      if (!IsPrimType(async_sig->out, "()")) {
        result.diag_id = "E-CON-0262";
        return result;
      }
      if (!IsPrimType(async_sig->in, "()")) {
        result.diag_id = "E-CON-0261";
        return result;
      }
    } else {
      if (!IsPrimType(async_sig->in, "()")) {
        result.diag_id = "E-CON-0261";
        return result;
      }
    }

    const TypeRef pat_type = yield_mode ? async_sig->out : async_sig->result;
    const auto pat = TypePattern(ctx, arm.pattern, pat_type);
    if (!pat.ok) {
      result.diag_id = pat.diag_id;
      return result;
    }

    TypeEnv inner = PushScope(env);
    const auto intro = IntroAll(inner, pat.bindings);
    if (!intro.ok) {
      result.diag_id = intro.diag_id;
      return result;
    }

    const auto handler_result = TypeExpr(ctx, type_ctx, arm.handler.value, intro.env);
    if (!handler_result.ok) {
      result.diag_id = handler_result.diag_id;
      return result;
    }

    handler_types.push_back(handler_result.type);
    error_types.push_back(async_sig->err);
  }

  std::optional<std::string_view> eq_diag;
  if (!AllEqTypes(handler_types, eq_diag)) {
    if (eq_diag.has_value()) {
      result.diag_id = eq_diag;
      return result;
    }
    result.diag_id = "E-CON-0261";
    return result;
  }

  if (!yield_mode) {
    std::vector<TypeRef> members;
    members.reserve(1 + error_types.size());
    members.push_back(handler_types.front());
    members.insert(members.end(), error_types.begin(), error_types.end());
    result.ok = true;
    result.type = MakeTypeUnion(std::move(members));
    return result;
  }

  std::vector<TypeRef> err_members;
  err_members.reserve(error_types.size());
  for (const auto& err : error_types) {
    err_members.push_back(err);
  }
  TypeRef err_union = MakeTypeUnion(std::move(err_members));
  result.ok = true;
  result.type = MakeTypePathWithArgs({"Stream"}, {handler_types.front(), err_union});
  return result;
}

ExprTypeResult TypeAllExpr(const ScopeContext& ctx,
                           const StmtTypeContext& type_ctx,
                           const syntax::AllExpr& expr,
                           const TypeEnv& env,
                           const ExprTypeFn& type_expr) {
  SPEC_RULE("T-All");
  ExprTypeResult result;

  std::vector<TypeRef> result_types;
  std::vector<TypeRef> error_types;
  result_types.reserve(expr.exprs.size());
  error_types.reserve(expr.exprs.size());

  for (const auto& elem : expr.exprs) {
    const auto elem_result = type_expr(elem);
    if (!elem_result.ok) {
      result.diag_id = elem_result.diag_id;
      return result;
    }
    const auto async_sig = AsyncSigOf(ctx, elem_result.type);
    if (!async_sig.has_value() || !IsPrimType(async_sig->out, "()")) {
      result.diag_id = "E-CON-0270";
      return result;
    }
    if (!IsPrimType(async_sig->in, "()")) {
      result.diag_id = "E-CON-0271";
      return result;
    }
    result_types.push_back(async_sig->result);
    error_types.push_back(async_sig->err);
  }

  const auto tuple_type = MakeTypeTuple(std::move(result_types));
  std::vector<TypeRef> members;
  members.reserve(1 + error_types.size());
  members.push_back(tuple_type);
  members.insert(members.end(), error_types.begin(), error_types.end());
  result.ok = true;
  result.type = MakeTypeUnion(std::move(members));
  return result;
}

}  // namespace cursive0::analysis

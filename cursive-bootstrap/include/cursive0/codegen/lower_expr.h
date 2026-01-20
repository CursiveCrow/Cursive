#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <source_location>
#include <string>
#include <unordered_map>
#include <vector>

#include "cursive0/codegen/ir_model.h"
#include "cursive0/sema/context.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::codegen {

// Forward declarations
struct LowerCtx;

// §6.4 LowerResult - result of lowering an expression
// Contains the IR to execute and the resulting value
struct LowerResult {
  IRPtr ir;
  IRValue value;
};

// ScopeInfo tracks variables declared in a scope for cleanup
struct CleanupItem {
  enum class Kind {
    DropBinding,
    DeferBlock,
  };

  Kind kind = Kind::DropBinding;
  std::string name;
  IRPtr defer_ir;
};

struct TempValue {
  IRValue value;
  sema::TypeRef type;
};

struct ScopeInfo {
  std::vector<std::string> variables;     // Variables in declaration order
  std::vector<CleanupItem> cleanup_items; // Cleanup items in append order
  bool is_loop = false;                   // True if this is a loop scope
  bool is_region = false;                 // True if this is a region scope
};

// BindingState tracks the state of a binding for cleanup purposes
struct BindingState {
  sema::TypeRef type;                     // Type of the binding
  bool has_responsibility = true;         // Does this binding own cleanupSigma
  bool is_immovable = false;             // Immovable binding (:=)
  bool is_moved = false;                  // Has this binding been movedSigma
  std::vector<std::string> moved_fields;  // Fields that have been moved (for partial moves)
};


// DerivedValueInfo tracks how to materialize an IRValue produced without explicit IR
struct DerivedValueInfo {
  enum class Kind {
    Field,
    Tuple,
    Index,
    Slice,
    EnumPayloadIndex,
    EnumPayloadField,
    ModalField,
    UnionPayload,
    TupleLit,
    ArrayLit,
    RecordLit,
    DynLit,
    DynData,
    DynVTableDrop,
    EnumLit,
    RangeLit,
    AddrLocal,
    AddrStatic,
    AddrField,
    AddrTuple,
    AddrIndex,
    AddrDeref,
  };

  Kind kind = Kind::Field;
  IRValue base;
  std::string field;
  std::size_t tuple_index = 0;
  std::size_t union_index = 0;
  IRValue index;
  IRRange range;
  std::vector<IRValue> elements;
  std::vector<std::pair<std::string, IRValue>> fields;
  std::string variant;
  std::string modal_state;
  std::vector<IRValue> payload_elems;
  std::vector<std::pair<std::string, IRValue>> payload_fields;
  std::vector<std::string> static_path;
  std::string name;
  std::string vtable_sym;
};

// LowerCtx - context for lowering operations
// Contains type information and scope state needed during lowering
struct LowerCtx {
  // Type environment - maps expressions to their types
  // In full implementation, this would be populated during type checking
  
  // Sigma for type declaration lookups (record fields, etc.)
  const sema::Sigma* sigma = nullptr;
  
  // Current procedure return type (for propagate expressions)
  sema::TypeRef proc_ret_type;
  
  // Module path for symbol resolution
  std::vector<std::string> module_path;
  
  // Expression type lookup (populated by type checking phase)
  std::function<sema::TypeRef(const syntax::Expr&)> expr_type;
  
  // Name resolution lookup
  std::function<std::optional<std::vector<std::string>>(const std::string&)> resolve_name;
  // Type name resolution lookup
  std::function<std::optional<std::vector<std::string>>(const std::string&)> resolve_type_name;

  // Track missing resolution or lowering failures.
  bool resolve_failed = false;
  bool codegen_failed = false;
  std::vector<std::string> resolve_failures;

  // IR value and symbol type tracking.
  std::unordered_map<std::string, sema::TypeRef> value_types;
  std::unordered_map<std::string, sema::TypeRef> static_types;
  std::unordered_map<std::string, sema::TypeRef> drop_glue_types;
  std::unordered_map<std::string, std::vector<std::string>> static_modules;
  std::unordered_map<std::string, std::vector<std::string>> record_ctor_paths;

  struct ProcSigInfo {
    std::vector<IRParam> params;
    sema::TypeRef ret;
  };
  std::unordered_map<std::string, ProcSigInfo> proc_sigs;
  std::unordered_map<std::string, std::vector<std::string>> proc_modules;
  std::optional<std::string> main_symbol;
  std::vector<syntax::ModulePath> init_order;
  std::vector<syntax::ModulePath> init_modules;
  std::vector<std::pair<std::size_t, std::size_t>> init_eager_edges;

  void ReportResolveFailure(const std::string& name);
  void ReportCodegenFailure(
      std::source_location loc = std::source_location::current());

  void RegisterValueType(const IRValue& value, sema::TypeRef type);
  sema::TypeRef LookupValueType(const IRValue& value) const;
  void RegisterStaticType(const std::string& sym, sema::TypeRef type);
  sema::TypeRef LookupStaticType(const std::string& sym) const;
  void RegisterStaticModule(const std::string& sym, const syntax::ModulePath& module_path);
  const std::vector<std::string>* LookupStaticModule(const std::string& sym) const;
  void RegisterDropGlueType(const std::string& sym, sema::TypeRef type);
  sema::TypeRef LookupDropGlueType(const std::string& sym) const;
  void RegisterRecordCtor(const std::string& sym, const std::vector<std::string>& path);
  const std::vector<std::string>* LookupRecordCtor(const std::string& sym) const;
  void RegisterProcSig(const ProcIR& proc);
  const ProcSigInfo* LookupProcSig(const std::string& sym) const;
  void RegisterProcModule(const std::string& sym, const syntax::ModulePath& module_path);
  const std::vector<std::string>* LookupProcModule(const std::string& sym) const;
  
  // =========================================================================
  // §6.8 Scope tracking for cleanup
  // =========================================================================
  
  // Stack of scope information for cleanup
  std::vector<ScopeInfo> scope_stack;
  
  // Map from binding name to its state
  std::unordered_map<std::string, std::vector<BindingState>> binding_states;

  // Map from temporary value names to derived value info
  std::unordered_map<std::string, DerivedValueInfo> derived_values;

  // Current temp sink for statement-scoped temporaries
  std::vector<TempValue>* temp_sink = nullptr;
  int temp_depth = 0;
  std::optional<int> suppress_temp_at_depth;
  std::uint64_t temp_counter = 0;
  
  // Push a new scope
  void PushScope(bool is_loop = false, bool is_region = false);
  
  // Pop the current scope
  void PopScope();
  
  // Get variables in current scope
  std::vector<std::string> CurrentScopeVars() const;
  
  // Get all variables from current scope up to (but not including) loop scope
  // Used for break/continue cleanup
  std::vector<std::string> VarsToLoopScope() const;
  
  // Get all variables from current scope to function root
  // Used for return cleanup
  std::vector<std::string> VarsToFunctionRoot() const;
  
  // Register a variable in the current scope
  void RegisterVar(const std::string& name,
                   sema::TypeRef type,
                   bool has_responsibility = true,
                   bool is_immovable = false);
  
  // Mark a variable as moved
  void MarkMoved(const std::string& name);
  
  // Mark a field of a variable as moved
  void MarkFieldMoved(const std::string& name, const std::string& field);
  
  // Get binding state
  const BindingState* GetBindingState(const std::string& name) const;
  
  // Register a defer block in the current scope
  void RegisterDefer(const IRPtr& defer_ir);

  // Register a temporary value for cleanup
  void RegisterTempValue(const IRValue& value, const sema::TypeRef& type);

  void RegisterDerivedValue(const IRValue& value, const DerivedValueInfo& info);
  const DerivedValueInfo* LookupDerivedValue(const IRValue& value) const;

  // Generate a unique temporary value placeholder.
  IRValue FreshTempValue(std::string_view prefix);
  
};

// Helper to create IR nodes
template <typename T>
IRPtr MakeIR(T&& node) {
  return std::make_shared<IR>(IR{std::forward<T>(node)});
}

// Helper to create a sequence of IR nodes (moved to ir_model.h)
// IRPtr SeqIR(std::vector<IRPtr> items);

// Helper to create an empty IR (no-op)
inline IRPtr EmptyIR() {
  return MakeIR(IROpaque{});
}

// ============================================================================
// §6.4 Expression Lowering Judgments
// ============================================================================

// §6.4 LowerExpr - main expression lowering entry point
// Lowers any expression form to IR + value
LowerResult LowerExpr(const syntax::Expr& expr, LowerCtx& ctx);

// §6.4 LowerList - lower a list of expressions (LTR order)
// Returns IR sequence and list of values
std::pair<IRPtr, std::vector<IRValue>> LowerList(
    const std::vector<syntax::ExprPtr>& exprs, LowerCtx& ctx);

// §6.4 LowerFieldInits - lower field initializers (LTR order)
std::pair<IRPtr, std::vector<std::pair<std::string, IRValue>>> LowerFieldInits(
    const std::vector<syntax::FieldInit>& fields, LowerCtx& ctx);

// §6.4 LowerOpt - lower an optional expression
std::pair<IRPtr, std::optional<IRValue>> LowerOpt(
    const syntax::ExprPtr& expr_opt, LowerCtx& ctx);

// ============================================================================
// §6.4 Place Lowering Judgments  
// ============================================================================

// §6.4 LowerReadPlace - lower a place expression for reading
LowerResult LowerReadPlace(const syntax::Expr& place, LowerCtx& ctx);

// §6.4 LowerWritePlace - lower a place expression for writing
IRPtr LowerWritePlace(const syntax::Expr& place, const IRValue& value, LowerCtx& ctx);

// §6.4 LowerWritePlaceSub - lower subplace write (no drop)
IRPtr LowerWritePlaceSub(const syntax::Expr& place, const IRValue& value, LowerCtx& ctx);

// §6.4 LowerAddrOf - lower address-of expression
LowerResult LowerAddrOf(const syntax::Expr& place, LowerCtx& ctx);

// §6.4 LowerMovePlace - lower move expression
LowerResult LowerMovePlace(const syntax::Expr& place, LowerCtx& ctx);

// §6.4 LowerPlace - lower a place to a place representation
IRPlace LowerPlace(const syntax::Expr& place, LowerCtx& ctx);

// ============================================================================
// §6.4 Operator Lowering
// ============================================================================

// §6.4 LowerUnOp - lower unary operator
LowerResult LowerUnOp(const std::string& op, const syntax::Expr& operand, LowerCtx& ctx);

// §6.4 LowerBinOp - lower binary operator (non-short-circuit)
LowerResult LowerBinOp(const std::string& op,
                       const syntax::Expr& lhs,
                       const syntax::Expr& rhs,
                       LowerCtx& ctx);

// §6.4 LowerCast - lower cast expression
LowerResult LowerCast(const syntax::Expr& expr, sema::TypeRef target_type, LowerCtx& ctx);

// ============================================================================
// §6.4 Call Lowering
// ============================================================================

using ParamModeList = std::vector<std::optional<sema::ParamMode>>;

// §6.4 LowerArgs - lower call arguments
std::pair<IRPtr, std::vector<IRValue>> LowerArgs(
    const ParamModeList& params,
    const std::vector<syntax::Arg>& args,
    LowerCtx& ctx);

// §6.4 LowerMethodCall - lower method call expression
LowerResult LowerMethodCall(const syntax::MethodCallExpr& expr, LowerCtx& ctx);

// ============================================================================
// §6.4 Control Flow Expression Lowering
// ============================================================================

// §6.4 LowerBlock - lower a block expression
LowerResult LowerBlock(const syntax::Block& block, LowerCtx& ctx);

// §6.4 LowerMatch - lower match expression
LowerResult LowerMatch(const syntax::Expr& scrutinee,
                       const std::vector<syntax::MatchArm>& arms,
                       LowerCtx& ctx);

// §6.4 LowerLoop - lower loop expression
LowerResult LowerLoop(const syntax::Expr& loop, LowerCtx& ctx);

// ============================================================================
// §6.4 Evaluation Order
// ============================================================================

// §6.4 Children_LTR - get child expressions in evaluation order
std::vector<syntax::ExprPtr> ChildrenLTR(const syntax::Expr& expr);

// Emit SPEC_RULE anchors for all §6.4 rules
void AnchorExprLoweringRules();

}  // namespace cursive0::codegen

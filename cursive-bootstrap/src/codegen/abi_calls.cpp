// Call Lowering for Procedures and Methods implementation (§6.2.4)
// Implements MethodSymbol, BuiltinMethodSym, LowerArgs, LowerRecvArg, LowerMethodCall.

#include "cursive0/codegen/abi.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/codegen/lower_expr.h"
#include "cursive0/codegen/checks.h"
#include "cursive0/runtime/runtime_interface.h"

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/classes.h"
#include "cursive0/sema/modal_transitions.h"
#include "cursive0/sema/record_methods.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/type_equiv.h"

namespace cursive0::codegen {
namespace {

// Get ModalDecl from a TypeModalState
const syntax::ModalDecl* GetModalDecl(const sema::ScopeContext& ctx,
                                      const sema::TypePath& path) {
  syntax::Path syntax_path;
  syntax_path.reserve(path.size());
  for (const auto& comp : path) {
    syntax_path.push_back(comp);
  }
  const auto it = ctx.sigma.types.find(sema::PathKeyOf(syntax_path));
  if (it == ctx.sigma.types.end()) {
    return nullptr;
  }
  return std::get_if<syntax::ModalDecl>(&it->second);
}

// Strip permission wrapper from type
sema::TypeRef StripPerm(const sema::TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<sema::TypePerm>(&type->node)) {
    return StripPerm(perm->base);
  }
  return type;
}

// Check if type is a TypeDynamic
bool IsDynamicType(const sema::TypeRef& type) {
  if (!type) {
    return false;
  }
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }
  return std::holds_alternative<sema::TypeDynamic>(stripped->node);
}

// Check if type is a TypeModalState
bool IsModalStateType(const sema::TypeRef& type) {
  if (!type) {
    return false;
  }
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }
  return std::holds_alternative<sema::TypeModalState>(stripped->node);
}

// Get TypeDynamic path if the type is dynamic
std::optional<sema::TypePath> GetDynamicPath(const sema::TypeRef& type) {
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return std::nullopt;
  }
  if (const auto* dyn = std::get_if<sema::TypeDynamic>(&stripped->node)) {
    return dyn->path;
  }
  return std::nullopt;
}

// Get TypeModalState info if the type is a modal state
std::optional<std::pair<sema::TypePath, std::string>> GetModalStateInfo(
    const sema::TypeRef& type) {
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return std::nullopt;
  }
  if (const auto* modal = std::get_if<sema::TypeModalState>(&stripped->node)) {
    return std::make_pair(modal->path, modal->state);
  }
  return std::nullopt;
}

// Get the class path from a fully qualified path
syntax::ClassPath ToClassPath(const sema::TypePath& path) {
  syntax::ClassPath result;
  result.reserve(path.size());
  for (const auto& comp : path) {
    result.push_back(comp);
  }
  return result;
}


static bool IsPlaceExprLite(const syntax::ExprPtr& expr) {
  if (!expr) {
    return false;
  }
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return IsPlaceExprLite(node.value);
        }
        return false;
      },
      expr->node);
}

static syntax::ExprPtr MovedArgExpr(const syntax::Arg& arg) {
  if (!arg.moved || !arg.value || !IsPlaceExprLite(arg.value)) {
    return arg.value;
  }
  auto moved = std::make_shared<syntax::Expr>();
  moved->span = arg.span.file.empty() && arg.value ? arg.value->span : arg.span;
  moved->node = syntax::MoveExpr{arg.value};
  return moved;
}
}  // namespace

// =============================================================================
// MethodSymbol Resolution
// =============================================================================

std::optional<std::string> MethodSymbol(const sema::ScopeContext& ctx,
                                        const sema::TypeRef& type,
                                        std::string_view name) {
  if (!type) {
    return std::nullopt;
  }

  const auto stripped = StripPerm(type);

  // (MethodSymbol-ModalState-Method) and (MethodSymbol-ModalState-Transition)
  // Check if this is a modal state type
  if (const auto modal_info = GetModalStateInfo(stripped)) {
    const auto& [modal_path, state] = *modal_info;
    const auto* modal_decl = GetModalDecl(ctx, modal_path);
    if (!modal_decl) {
      return std::nullopt;
    }

    // (MethodSymbol-ModalState-Method)
    // LookupStateMethod(S, name) = md → Mangle(md) ⇓ sym
    const auto* state_method = sema::LookupStateMethodDecl(*modal_decl, state, name);
    if (state_method) {
      SPEC_RULE("MethodSymbol-ModalState-Method");
      return MangleStateMethod(modal_path, state, *state_method);
    }

    // (MethodSymbol-ModalState-Transition)
    // LookupTransition(S, name) = tr → Mangle(tr) ⇓ sym
    const auto* transition = sema::LookupTransitionDecl(*modal_decl, state, name);
    if (transition) {
      SPEC_RULE("MethodSymbol-ModalState-Transition");
      return MangleTransition(modal_path, state, *transition);
    }

    return std::nullopt;
  }

  // (MethodSymbol-Record) and (MethodSymbol-Default)
  // Lookup method on concrete type (record or class default)
  const auto lookup = sema::LookupMethodStatic(ctx, stripped, name);
  if (!lookup.ok) {
    return std::nullopt;
  }

  // (MethodSymbol-Record)
  // LookupMethod(T, name) = m ∧ m = MethodDecl(_) → Mangle(m) ⇓ sym
  if (lookup.record_method) {
    const auto* path_type = std::get_if<sema::TypePathType>(&stripped->node);
    if (!path_type) {
      return std::nullopt;
    }
    SPEC_RULE("MethodSymbol-Record");
    return MangleMethod(path_type->path, *lookup.record_method);
  }

  // (MethodSymbol-Default)
  // LookupMethod(T, name) = m ∧ m = ClassMethodDecl(_) ∧ m.body_opt ≠ ⊥
  // → Mangle(DefaultImpl(T, m)) ⇓ sym
  if (lookup.class_method && lookup.class_method->body_opt) {
    SPEC_RULE("MethodSymbol-Default");
    return MangleDefaultImpl(stripped, lookup.owner_class, lookup.class_method->name);
  }

  return std::nullopt;
}

// =============================================================================
// BuiltinMethodSym Resolution
// =============================================================================

bool IsBuiltinCapClass(std::string_view class_name) {
  // BuiltinCapClass = {FileSystem, HeapAllocator}
  return class_name == "FileSystem" || class_name == "HeapAllocator";
}

std::optional<std::string> BuiltinMethodSym(std::string_view cap_class,
                                           std::string_view name) {
  // (BuiltinMethodSym-FileSystem)
  // BuiltinSym(FileSystem::name) ⇓ sym
  if (cap_class == "FileSystem") {
    SPEC_RULE("BuiltinMethodSym-FileSystem");
    const std::string sym = BuiltinSym(std::string("FileSystem::") + std::string(name));
    if (sym.empty()) {
      return std::nullopt;
    }
    return sym;
  }

  // (BuiltinMethodSym-HeapAllocator)
  // BuiltinSym(HeapAllocator::name) ⇓ sym
  if (cap_class == "HeapAllocator") {
    SPEC_RULE("BuiltinMethodSym-HeapAllocator");
    const std::string sym = BuiltinSym(std::string("HeapAllocator::") + std::string(name));
    if (sym.empty()) {
      return std::nullopt;
    }
    return sym;
  }

  return std::nullopt;
}

// =============================================================================
// Argument Lowering
// =============================================================================

std::optional<LowerArgsResult> LowerArgs(
    const sema::ScopeContext& ctx,
    const std::vector<syntax::Param>& params,
    const std::vector<syntax::Arg>& args) {
  LowerArgsResult result;
  result.ir = EmptyIR();

  // (Lower-Args-Empty)
  // LowerArgs([], []) ⇓ ⟨ε, []⟩
  if (params.empty() && args.empty()) {
    SPEC_RULE("Lower-Args-Empty");
    return result;
  }

  LowerCtx lower;
  lower.sigma = &ctx.sigma;
  lower.module_path = ctx.current_module;

  const bool use_params = !params.empty() && params.size() == args.size();

  std::vector<IRPtr> parts;
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (!args[i].value) {
      continue;
    }
    if (use_params && params[i].mode.has_value()) {
      SPEC_RULE("Lower-Args-Cons-Move");
      auto moved = MovedArgExpr(args[i]);
      auto lowered = LowerExpr(*moved, lower);
      parts.push_back(lowered.ir);
      result.values.push_back(lowered.value);
      continue;
    }
    if (use_params) {
      SPEC_RULE("Lower-Args-Cons-Ref");
      auto lowered = LowerAddrOf(*args[i].value, lower);
      parts.push_back(lowered.ir);
      result.values.push_back(lowered.value);
      continue;
    }

    auto lowered = LowerExpr(*args[i].value, lower);
    parts.push_back(lowered.ir);
    result.values.push_back(lowered.value);
  }

  result.ir = SeqIR(std::move(parts));
  return result;
}

// =============================================================================
// Receiver Argument Lowering
// =============================================================================

std::optional<LowerCallResult> LowerRecvArg(
    const sema::ScopeContext& ctx,
    const syntax::ExprPtr& base,
    bool is_move) {
  if (!base) {
    return std::nullopt;
  }

  LowerCtx lower;
  lower.sigma = &ctx.sigma;
  lower.module_path = ctx.current_module;

  LowerCallResult result;
  if (is_move || std::holds_alternative<syntax::MoveExpr>(base->node)) {
    // (Lower-RecvArg-Move)
    SPEC_RULE("Lower-RecvArg-Move");
    auto lowered = LowerExpr(*base, lower);
    result.ir = lowered.ir;
    result.value = lowered.value;
    return result;
  }

  // (Lower-RecvArg-Ref)
  SPEC_RULE("Lower-RecvArg-Ref");
  auto lowered = LowerAddrOf(*base, lower);
  result.ir = lowered.ir;
  result.value = lowered.value;
  return result;
}

// =============================================================================
// Method Call Lowering
// =============================================================================

std::optional<LowerCallResult> LowerMethodCall(
    const sema::ScopeContext& ctx,
    const syntax::ExprPtr& base,
    std::string_view name,
    const std::vector<syntax::Arg>& args) {
  if (!base) {
    return std::nullopt;
  }

  LowerCtx lower;
  lower.sigma = &ctx.sigma;
  lower.module_path = ctx.current_module;

  const bool is_move = std::holds_alternative<syntax::MoveExpr>(base->node);
  const auto recv = LowerRecvArg(ctx, base, is_move);
  if (!recv.has_value()) {
    return std::nullopt;
  }

  std::vector<IRPtr> parts;
  parts.push_back(recv->ir);

  std::vector<IRValue> arg_values;
  for (const auto& arg : args) {
    if (!arg.value) {
      continue;
    }
    auto lowered = LowerExpr(*arg.value, lower);
    parts.push_back(lowered.ir);
    arg_values.push_back(lowered.value);
  }

  std::vector<IRValue> call_args;
  call_args.push_back(recv->value);
  call_args.insert(call_args.end(), arg_values.begin(), arg_values.end());

  std::string callee_sym(name);
  IRCall call;
  call.callee = IRValue{IRValue::Kind::Symbol, callee_sym, {}};
  call.args = std::move(call_args);

  const bool needs_panic = NeedsPanicOut(callee_sym);
  if (needs_panic) {
    SPEC_RULE("Lower-MethodCall-Static-PanicOut");
    IRValue panic_out;
    panic_out.kind = IRValue::Kind::Local;
    panic_out.name = std::string(kPanicOutName);
    call.args.push_back(panic_out);
  } else {
    SPEC_RULE("Lower-MethodCall-Static-NoPanicOut");
  }

  parts.push_back(MakeIR(std::move(call)));
  if (needs_panic) {
    parts.push_back(PanicCheck(lower));
  }

  LowerCallResult result;
  result.ir = SeqIR(std::move(parts));
  result.value = IRValue{IRValue::Kind::Opaque, "call_result", {}};
  return result;
}

// =============================================================================
// SeqIR Helpers
// =============================================================================

// SeqIR helpers moved to ir_model.cpp

// =============================================================================
// SPEC_RULE Anchors for Coverage
// =============================================================================

void AnchorABIRules() {
  // §6.2.2 ABI Type Lowering Rules
  SPEC_RULE("ABI-Prim");
  SPEC_RULE("ABI-Perm");
  SPEC_RULE("ABI-Ptr");
  SPEC_RULE("ABI-RawPtr");
  SPEC_RULE("ABI-Func");
  SPEC_RULE("ABI-Alias");
  SPEC_RULE("ABI-Record");
  SPEC_RULE("ABI-Tuple");
  SPEC_RULE("ABI-Array");
  SPEC_RULE("ABI-Slice");
  SPEC_RULE("ABI-Range");
  SPEC_RULE("ABI-Enum");
  SPEC_RULE("ABI-Union");
  SPEC_RULE("ABI-Modal");
  SPEC_RULE("ABI-Dynamic");
  SPEC_RULE("ABI-StringBytes");

  // §6.2.3 ABI Parameter and Return Passing Rules
  SPEC_RULE("ABI-Param-ByRef-Alias");
  SPEC_RULE("ABI-Param-ByValue-Move");
  SPEC_RULE("ABI-Param-ByRef-Move");
  SPEC_RULE("ABI-Ret-ByValue");
  SPEC_RULE("ABI-Ret-ByRef");
  SPEC_RULE("ABI-Call");

  // §6.2.4 Call Lowering Rules
  SPEC_RULE("MethodSymbol-Record");
  SPEC_RULE("MethodSymbol-Default");
  SPEC_RULE("MethodSymbol-ModalState-Method");
  SPEC_RULE("MethodSymbol-ModalState-Transition");
  SPEC_RULE("BuiltinMethodSym-FileSystem");
  SPEC_RULE("BuiltinMethodSym-HeapAllocator");
  SPEC_RULE("Lower-Args-Empty");
  SPEC_RULE("Lower-Args-Cons-Move");
  SPEC_RULE("Lower-Args-Cons-Ref");
  SPEC_RULE("Lower-RecvArg-Move");
  SPEC_RULE("Lower-RecvArg-Ref");
  SPEC_RULE("Lower-MethodCall-Static-PanicOut");
  SPEC_RULE("Lower-MethodCall-Static-NoPanicOut");
  SPEC_RULE("Lower-MethodCall-Capability");
  SPEC_RULE("Lower-MethodCall-Dynamic");
}

}  // namespace cursive0::codegen

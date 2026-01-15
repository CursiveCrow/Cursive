#include "cursive0/codegen/dyn_dispatch.h"

#include <variant>

#include "cursive0/codegen/checks.h"
#include "cursive0/codegen/cleanup.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/codegen/layout.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/classes.h"
#include "cursive0/sema/record_methods.h"
#include "cursive0/sema/type_expr.h"

namespace cursive0::codegen {

// ============================================================================
// Helper functions
// ============================================================================

// Helpers moved to lower_expr.h

// ============================================================================
// §5.4.1 SelfOccurs - Check if Self type occurs in a type
// ============================================================================

// Forward declaration
static bool SelfOccursInType(const syntax::Type& type);

static bool SelfOccursInType(const std::shared_ptr<syntax::Type>& type) {
  if (!type) {
    return false;
  }
  return SelfOccursInType(*type);
}

static bool SelfOccursInType(const syntax::Type& type) {
  SPEC_DEF("SelfOccurs", "5.4.1");

  return std::visit(
      [](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;

        // SelfOccurs(TypePath(["Self"])) = true
        if constexpr (std::is_same_v<T, syntax::TypePathType>) {
          return node.path.size() == 1 && node.path[0] == "Self";
        }
        // SelfOccurs(TypePerm(p, ty)) = SelfOccurs(ty)
        else if constexpr (std::is_same_v<T, syntax::TypePermType>) {
          return SelfOccursInType(node.base);
        }
        // SelfOccurs(TypeTuple([...])) = any element SelfOccurs
        else if constexpr (std::is_same_v<T, syntax::TypeTuple>) {
          for (const auto& elem : node.elements) {
            if (SelfOccursInType(elem)) {
              return true;
            }
          }
          return false;
        }
        // SelfOccurs(TypeArray(ty, e)) = SelfOccurs(ty)
        else if constexpr (std::is_same_v<T, syntax::TypeArray>) {
          return SelfOccursInType(node.element);
        }
        // SelfOccurs(TypeSlice(ty)) = SelfOccurs(ty)
        else if constexpr (std::is_same_v<T, syntax::TypeSlice>) {
          return SelfOccursInType(node.element);
        }
        // SelfOccurs(TypePtr(ty, s)) = SelfOccurs(ty)
        else if constexpr (std::is_same_v<T, syntax::TypePtr>) {
          return SelfOccursInType(node.element);
        }
        // SelfOccurs(TypeRawPtr(q, ty)) = SelfOccurs(ty)
        else if constexpr (std::is_same_v<T, syntax::TypeRawPtr>) {
          return SelfOccursInType(node.element);
        }
        // SelfOccurs(TypeFunc(...)) = any param or return SelfOccurs
        else if constexpr (std::is_same_v<T, syntax::TypeFunc>) {
          for (const auto& param : node.params) {
            if (SelfOccursInType(param.type)) {
              return true;
            }
          }
          return SelfOccursInType(node.ret);
        }
        // SelfOccurs(TypeUnion([...])) = any member SelfOccurs
        else if constexpr (std::is_same_v<T, syntax::TypeUnion>) {
          for (const auto& ty : node.types) {
            if (SelfOccursInType(ty)) {
              return true;
            }
          }
          return false;
        }
        // All other types: SelfOccurs = false
        // TypePrim, TypeString, TypeBytes, TypeModalState, TypeDynamic
        else {
          return false;
        }
      },
      type.node);
}

// SelfOccurs(m) ⟺ SelfOccurs(ReturnType(m)) ∨ ∃⟨_,_,ty⟩ ∈ m.params. SelfOccurs(ty)
static bool SelfOccursInMethod(const syntax::ClassMethodDecl& method) {
  SPEC_DEF("SelfOccurs", "5.4.1");

  // Check return type
  if (method.return_type_opt && SelfOccursInType(method.return_type_opt)) {
    return true;
  }

  // Check each parameter type
  for (const auto& param : method.params) {
    if (SelfOccursInType(param.type)) {
      return true;
    }
  }

  return false;
}

// ============================================================================
// §6.10 VTable Eligibility
// ============================================================================

bool IsVTableEligible(const syntax::ClassMethodDecl& method) {
  SPEC_DEF("vtable_eligible", "5.4.1");

  // vtable_eligible(m) ⟺ HasReceiver(m) ∧ ¬SelfOccurs(m)
  //
  // Per §5.4.1:
  //   HasReceiver(m) ⟺ m.receiver ≠ ⊥
  //   SelfOccurs(m) ⟺ SelfOccurs(ReturnType(m)) ∨ ∃⟨_,_,ty⟩ ∈ m.params. SelfOccurs(ty)
  //
  // In Cursive, ClassMethodDecl always has a receiver field (variant type).
  // The receiver variant is either ReceiverShorthand (~, ~!, ~%) or ReceiverExplicit.
  // A ClassMethodDecl with a receiver is an instance method - always has a receiver per grammar.
  //
  // Therefore HasReceiver(m) = true for all ClassMethodDecl.
  // The only check needed is ¬SelfOccurs(m).

  // HasReceiver(m) - ClassMethodDecl always has receiver per grammar
  // The Receiver variant is never "empty" - it's either shorthand or explicit
  const bool has_receiver = true;

  // ¬SelfOccurs(m) - Self must not occur in params or return type
  const bool self_occurs = SelfOccursInMethod(method);

  return has_receiver && !self_occurs;
}

std::vector<std::string> VTableEligible(const syntax::ClassDecl& class_decl) {
  SPEC_DEF("VTableEligible", "");
  
  std::vector<std::string> eligible;
  
  for (const auto& item : class_decl.items) {
    if (const auto* method = std::get_if<syntax::ClassMethodDecl>(&item)) {
      if (IsVTableEligible(*method)) {
        eligible.push_back(method->name);
      }
    }
  }
  
  return eligible;
}

// ============================================================================
// §6.10 DispatchSym - Symbol resolution for vtable slots
// ============================================================================

std::string DispatchSym(const sema::TypeRef& type,
                        const sema::TypePath& class_path,
                        const std::string& method_name,
                        const syntax::ClassDecl& class_decl,
                        LowerCtx& ctx) {
  sema::ScopeContext scope;
  if (ctx.sigma) {
    scope.sigma = *ctx.sigma;
    scope.current_module = ctx.module_path;
  }

  // Find the class method declaration
  const auto* class_method = sema::LookupClassMethod(scope, class_path, method_name);
  if (!class_method) {
    return "";
  }

  const auto stripped = sema::StripPerm(type);
  const auto lookup = sema::LookupMethodStatic(scope, stripped, method_name);

  // (DispatchSym-Impl): Type implements the method
  if (lookup.record_method) {
    const auto* path_type = stripped ? std::get_if<sema::TypePathType>(&stripped->node) : nullptr;
    if (!path_type) {
      return "";
    }
    SPEC_RULE("DispatchSym-Impl");
    return MangleMethod(path_type->path, *lookup.record_method);
  }

  // (DispatchSym-Default-None): No implementation → use default impl symbol
  if (class_method->body_opt) {
    SPEC_RULE("DispatchSym-Default-None");
    return MangleDefaultImpl(stripped, class_path, class_method->name);
  }

  return "";
}

// ============================================================================
// §6.10 VTable - Vtable generation
// ============================================================================

VTableInfo VTable(const sema::TypeRef& type,
                  const sema::TypePath& class_path,
                  const syntax::ClassDecl& class_decl,
                  LowerCtx& ctx) {
  SPEC_RULE("VTable-Order");
  
  VTableInfo info;
  
  // Generate vtable symbol
  info.symbol = MangleVTable(type, class_path);
  
  // Get type size and alignment
  // Use layout helpers when available
  sema::ScopeContext scope;
  if (ctx.sigma) {
    scope.sigma = *ctx.sigma;
    scope.current_module = ctx.module_path;
  }
  const auto size = cursive0::codegen::SizeOf(scope, type);
  const auto align = cursive0::codegen::AlignOf(scope, type);
  info.type_size = size.value_or(0);
  info.type_align = align.value_or(1);
  
  // Get drop glue symbol
  info.drop_sym = DropGlueSym(type, ctx);
  
  // Get method symbols in vtable order
  auto eligible = VTableEligible(class_decl);
  info.method_syms.reserve(eligible.size());
  
  for (const auto& method_name : eligible) {
    std::string sym = DispatchSym(type, class_path, method_name, class_decl, ctx);
    info.method_syms.push_back(std::move(sym));
  }
  
  return info;
}

GlobalVTable EmitVTable(const sema::TypeRef& type,
                        const sema::TypePath& class_path,
                        const syntax::ClassDecl& class_decl,
                        LowerCtx& ctx) {
  SPEC_DEF("EmitVTable", "");
  
  VTableInfo info = VTable(type, class_path, class_decl, ctx);
  
  GlobalVTable gvt;
  gvt.symbol = info.symbol;
  gvt.header.size = info.type_size;
  gvt.header.align = info.type_align;
  gvt.header.drop_sym = info.drop_sym;
  gvt.slots = std::move(info.method_syms);
  
  return gvt;
}

// ============================================================================
// §6.10 VSlot - Vtable slot lookup
// ============================================================================

std::optional<std::size_t> VSlot(const syntax::ClassDecl& class_decl,
                                  const std::string& method_name) {
  SPEC_RULE("VSlot-Entry");
  
  // (VSlot-Entry)
  // VTableEligible(Cl) = [m_0,...,m_{k-1}]
  // m_i.name = method.name
  // VSlot(Cl, method) ⇓ i
  
  auto eligible = VTableEligible(class_decl);
  
  for (std::size_t i = 0; i < eligible.size(); ++i) {
    if (eligible[i] == method_name) {
      return i;
    }
  }
  
  return std::nullopt;  // Method not found in vtable
}

// ============================================================================
// §6.10 DynPack - Fat pointer creation
// ============================================================================

DynPackResult DynPack(const sema::TypeRef& type,
                      const syntax::Expr& expr,
                      const sema::TypePath& class_path,
                      const syntax::ClassDecl& class_decl,
                      LowerCtx& ctx) {
  SPEC_RULE("Lower-Dynamic-Form");
  
  // (Lower-Dynamic-Form)
  // IsPlace(e)
  // LowerAddrOf(e) ⇓ ⟨IR, addr⟩
  // T_e = ExprType(e), T = StripPerm(T_e)
  // T <: Cl
  // DynPack(T, e) ⇓ ⟨RawPtr(imm, addr), VTable(T, Cl)⟩
  
  DynPackResult result;
  
  // Lower the address of the expression
  LowerResult addr_result = LowerAddrOf(expr, ctx);
  result.ir = addr_result.ir;
  result.data_ptr = addr_result.value;
  result.data_ptr.kind = IRValue::Kind::Opaque;  // Mark as raw pointer
  
  // Get the vtable for this type implementing the class
  VTableInfo vtable_info = VTable(type, class_path, class_decl, ctx);
  result.vtable_sym = vtable_info.symbol;
  
  return result;
}

// ============================================================================
// §6.10 LowerDynCall - Dynamic dispatch call lowering
// ============================================================================

LowerResult LowerDynCall(const IRValue& base_ptr,
                         const std::string& vtable_sym,
                         const syntax::ClassDecl& class_decl,
                         const std::string& method_name,
                         const std::vector<IRValue>& args,
                         LowerCtx& ctx) {
  SPEC_RULE("Lower-DynCall");
  
  // (Lower-DynCall)
  // VSlot(Cl, name) ⇓ i
  // LowerDynCall(base, name, args) ⇓ SeqIR(CallVTable(base, i, args), PanicCheck)
  
  auto slot_opt = VSlot(class_decl, method_name);
  
  if (!slot_opt.has_value()) {
    // Method not found - return error result
    IRValue error_value;
    error_value.kind = IRValue::Kind::Opaque;
    error_value.name = "dyncall_error";
    return LowerResult{EmptyIR(), error_value};
  }
  
  std::size_t slot = *slot_opt;
  
  // Create CallVTable IR
  IRCallVTable call;
  call.base = base_ptr;
  call.slot = slot;
  call.args = args;
  
  // Result value
  IRValue result_value;
  result_value.kind = IRValue::Kind::Opaque;
  result_value.name = "dyncall_result";
  
  // Sequence: CallVTable + PanicCheck
  std::vector<IRPtr> parts;
  parts.push_back(MakeIR(std::move(call)));
  parts.push_back(PanicCheck(ctx));
  
  return LowerResult{SeqIR(std::move(parts)), result_value};
}

// ============================================================================
// §6.10 Dynamic Object Layout
// ============================================================================

DynObjectLayout GetDynObjectLayout() {
  SPEC_DEF("DynObjectLayout", "");
  
  // A dynamic object (trait object) on Win64 x86_64-pc-windows-msvc:
  // - data_ptr: 8 bytes at offset 0
  // - vtable_ptr: 8 bytes at offset 8
  // Total size: 16 bytes
  // Alignment: 8 bytes (pointer alignment)
  
  DynObjectLayout layout;
  layout.size = 16;
  layout.align = 8;
  layout.data_offset = 0;
  layout.vtable_offset = 8;
  
  return layout;
}

// ============================================================================
// Spec Rule Anchors
// ============================================================================

void AnchorDynDispatchRules() {
  // §6.10 Dynamic Dispatch
  SPEC_RULE("DispatchSym-Impl");
  SPEC_RULE("DispatchSym-Default-None");
  SPEC_RULE("DispatchSym-Default-Mismatch");
  SPEC_RULE("VTable-Order");
  SPEC_RULE("VSlot-Entry");
  SPEC_RULE("Lower-Dynamic-Form");
  SPEC_RULE("Lower-DynCall");
  
  // Definitions
  SPEC_DEF("VTableEligible", "");
  SPEC_DEF("vtable_eligible", "");
  SPEC_DEF("EmitVTable", "");
  SPEC_DEF("DynObjectLayout", "");
}

}  // namespace cursive0::codegen

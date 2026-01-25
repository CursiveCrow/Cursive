#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cursive0/codegen/ir_model.h"
#include "cursive0/codegen/layout/layout.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::codegen {

// =============================================================================
// §6.2.1 Default Calling Convention
// =============================================================================

// CallConvDefault = Cursive0ABI (Win64 x86_64-pc-windows-msvc)

// =============================================================================
// §6.2.2 ABI Type Lowering
// =============================================================================

// ABIType = { <size, align> | size ∈ ℕ ∧ align ∈ ℕ }
// Reuses existing Layout struct which has the same structure.
using ABIType = Layout;

// ABITy(T) ⇓ <size, align>
// Maps a semantic type to its ABI representation (size and alignment).
std::optional<ABIType> ABITy(const analysis::ScopeContext& ctx,
                            const analysis::TypeRef& type);

// =============================================================================
// §6.2.3 ABI Parameter and Return Passing
// =============================================================================

// PassKind = {ByValue, ByRef, SRet}
enum class PassKind {
  ByValue,
  ByRef,
  SRet,
};

// ByValMax = 16 bytes
constexpr std::uint64_t kByValMax = 16;

// ByValAlign = PtrAlign (8 bytes on Win64)
constexpr std::uint64_t kByValAlign = kPtrAlign;

// ByValOk(T) ⟺ sizeof(T) ≤ ByValMax ∧ alignof(T) ≤ ByValAlign
bool ByValOk(const analysis::ScopeContext& ctx, const analysis::TypeRef& type);

// ABIParam(mode, T) ⇓ PassKind
// Determines how a parameter of type T with given mode should be passed.
std::optional<PassKind> ABIParam(const analysis::ScopeContext& ctx,
                                 std::optional<analysis::ParamMode> mode,
                                 const analysis::TypeRef& type);

// ABIRet(T) ⇓ PassKind
// Determines how a return value of type T should be passed.
std::optional<PassKind> ABIRet(const analysis::ScopeContext& ctx,
                               const analysis::TypeRef& type);

// Result of ABICall judgment
struct ABICallInfo {
  std::vector<PassKind> param_kinds;
  PassKind ret_kind = PassKind::ByValue;
  bool has_sret = false;
};

// ABICall([⟨m₁,T₁⟩,...,⟨mₙ,Tₙ⟩], R) ⇓ ⟨[k₁,...,kₙ], kᵣ, (kᵣ = SRet)⟩
std::optional<ABICallInfo> ABICall(
    const analysis::ScopeContext& ctx,
    const std::vector<std::pair<std::optional<analysis::ParamMode>, analysis::TypeRef>>& params,
    const analysis::TypeRef& ret);

// -----------------------------------------------------------------------------
// Panic Out-Parameter (Cursive0)
// -----------------------------------------------------------------------------

// PanicRecordFields = [⟨panic, bool⟩, ⟨code, u32⟩]
// Returns the TypeRef for the PanicRecord struct type.
analysis::TypeRef PanicRecordType();

// PanicOutType = rawptr[mut, PanicRecord]
analysis::TypeRef PanicOutType();

// PanicOutName = "__panic"
constexpr std::string_view kPanicOutName = "__panic";

// NeedsPanicOut(callee) ⟺ callee ≠ RecordCtor(_) ∧ callee ≠ EntrySym ∧ RuntimeSig(callee) undefined
bool NeedsPanicOut(std::string_view callee_sym);

// PanicOutParams(params, callee) - appends panic out-param if needed
std::vector<std::tuple<std::optional<analysis::ParamMode>, std::string, analysis::TypeRef>>
PanicOutParams(
    const std::vector<std::tuple<std::optional<analysis::ParamMode>, std::string, analysis::TypeRef>>& params,
    std::string_view callee_sym);

// =============================================================================
// §6.2.4 Call Lowering for Procedures and Methods
// =============================================================================

// Result of LowerArgs judgment
struct LowerArgsResult {
  IRPtr ir;
  std::vector<IRValue> values;
};

// Result of LowerRecvArg / LowerMethodCall judgments
struct LowerCallResult {
  IRPtr ir;
  IRValue value;
};

// -----------------------------------------------------------------------------
// Symbol Resolution
// -----------------------------------------------------------------------------

// MethodSymbol(T, name) ⇓ sym
// Resolves the mangled symbol for a method call on type T.
std::optional<std::string> MethodSymbol(const analysis::ScopeContext& ctx,
                                        const analysis::TypeRef& type,
                                        std::string_view name);

// BuiltinMethodSym(CapClass, name) ⇓ sym
// Resolves the symbol for a builtin capability method.
std::optional<std::string> BuiltinMethodSym(std::string_view cap_class,
                                           std::string_view name);

// BuiltinCapClass = {FileSystem, HeapAllocator}
bool IsBuiltinCapClass(std::string_view class_name);

// -----------------------------------------------------------------------------
// Argument Lowering
// -----------------------------------------------------------------------------

// LowerArgs(params, args) ⇓ ⟨IR, [v₁,...,vₙ]⟩
// Lowers argument expressions to IR values.
// Note: This is a simplified interface; full implementation requires
// LowerExpr/LowerAddrOf functions from expression lowering (T-LOWER-001).
std::optional<LowerArgsResult> LowerArgs(
    const analysis::ScopeContext& ctx,
    const std::vector<syntax::Param>& params,
    const std::vector<syntax::Arg>& args);

// LowerRecvArg(base) ⇓ ⟨IR, v_self⟩
// Lowers the receiver expression for a method call.
std::optional<LowerCallResult> LowerRecvArg(
    const analysis::ScopeContext& ctx,
    const syntax::ExprPtr& base,
    bool is_move);

// -----------------------------------------------------------------------------
// Method Call Lowering
// -----------------------------------------------------------------------------

// LowerMethodCall(MethodCall(base, name, args)) ⇓ ⟨IR, v_call⟩
// Generates IR for a complete method call.
std::optional<LowerCallResult> LowerMethodCall(
    const analysis::ScopeContext& ctx,
    const syntax::ExprPtr& base,
    std::string_view name,
    const std::vector<syntax::Arg>& args);

// SeqIR helper - REMOVED (moved to ir_model.h)

// =============================================================================
// SPEC_RULE anchors for coverage tracking
// =============================================================================

// Emits SPEC_RULE anchors for all §6.2.2, §6.2.3, §6.2.4 rules.
void AnchorABIRules();

}  // namespace cursive0::codegen

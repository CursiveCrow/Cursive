// ABI Parameter and Return Passing implementation (§6.2.3)
// Implements ABIParam, ABIRet, ABICall judgments and panic out-parameter support.

#include "cursive0/codegen/abi.h"

#include <algorithm>
#include <unordered_set>

#include "cursive0/core/assert_spec.h"

namespace cursive0::codegen {
namespace {

// Known runtime symbols that do NOT need a panic out-parameter.
// This includes the panic handler itself, context init, and other runtime-defined symbols.
const std::unordered_set<std::string_view>& RuntimeSymbols() {
  static const std::unordered_set<std::string_view> syms = {
      // PanicSym
      "cursive::runtime::panic",
      // ContextInitSym
      "cursive::runtime::context_init",
      // StringDropSym
      "cursive::runtime::string::drop_managed",
      // BytesDropSym
      "cursive::runtime::bytes::drop_managed",
      // Region symbols
      "cursive::runtime::region::new_scoped",
      "cursive::runtime::region::alloc",
      "cursive::runtime::region::reset_unchecked",
      "cursive::runtime::region::freeze",
      "cursive::runtime::region::thaw",
      "cursive::runtime::region::free_unchecked",
      // Filesystem symbols
      "cursive::runtime::fs::open_read",
      "cursive::runtime::fs::open_write",
      "cursive::runtime::fs::open_append",
      "cursive::runtime::fs::create_write",
      "cursive::runtime::fs::read_file",
      "cursive::runtime::fs::read_bytes",
      "cursive::runtime::fs::write_file",
      "cursive::runtime::fs::write_stdout",
      "cursive::runtime::fs::write_stderr",
      "cursive::runtime::fs::exists",
      "cursive::runtime::fs::remove",
      "cursive::runtime::fs::open_dir",
      "cursive::runtime::fs::create_dir",
      "cursive::runtime::fs::ensure_dir",
      "cursive::runtime::fs::kind",
      "cursive::runtime::fs::restrict",
      // Heap symbols
      "cursive::runtime::heap::with_quota",
      "cursive::runtime::heap::alloc_raw",
      "cursive::runtime::heap::dealloc_raw",
      // String/Bytes builtin symbols
      "cursive::runtime::string::from",
      "cursive::runtime::string::as_view",
      "cursive::runtime::string::to_managed",
      "cursive::runtime::string::clone_with",
      "cursive::runtime::string::append",
      "cursive::runtime::string::length",
      "cursive::runtime::string::is_empty",
      "cursive::runtime::bytes::with_capacity",
      "cursive::runtime::bytes::from_slice",
      "cursive::runtime::bytes::as_view",
      "cursive::runtime::bytes::to_managed",
      "cursive::runtime::bytes::view",
      "cursive::runtime::bytes::view_string",
      "cursive::runtime::bytes::append",
      "cursive::runtime::bytes::length",
      "cursive::runtime::bytes::is_empty",
  };
  return syms;
}

// EntrySym is the program entry point
constexpr std::string_view kEntrySym = "main";

// Check if a symbol looks like a record constructor
// Record constructors are identified by path structure: they look like type paths
bool IsRecordCtorSymbol(std::string_view sym) {
  // Record constructor symbols typically don't contain "::" method separators
  // after the type path, and don't have function-like mangled suffixes.
  // This is a simplified heuristic - proper detection requires symbol table lookup.
  // For now, assume symbols ending in alphanumeric without method suffix are record ctors.
  return false;  // Conservative: assume not a record ctor
}

}  // namespace

// ByValOk(T) ⟺ sizeof(T) ≤ ByValMax ∧ alignof(T) ≤ ByValAlign
bool ByValOk(const sema::ScopeContext& ctx, const sema::TypeRef& type) {
  const auto size = SizeOf(ctx, type);
  const auto align = AlignOf(ctx, type);
  if (!size.has_value() || !align.has_value()) {
    return false;
  }
  return *size <= kByValMax && *align <= kByValAlign;
}

// ABIParam(mode, T) ⇓ PassKind
std::optional<PassKind> ABIParam(const sema::ScopeContext& ctx,
                                 std::optional<sema::ParamMode> mode,
                                 const sema::TypeRef& type) {
  if (!type) {
    return std::nullopt;
  }

  const auto size = SizeOf(ctx, type);
  if (!size.has_value()) {
    return std::nullopt;
  }

  // (ABI-Param-ByRef-Alias)
  // Non-move parameters (aliased/borrowed) are always passed by reference.
  // mode = ⊥ → ByRef
  if (!mode.has_value()) {
    SPEC_RULE("ABI-Param-ByRef-Alias");
    return PassKind::ByRef;
  }

  // mode = move
  // (ABI-Param-ByValue-Move)
  // Move param with zero size or ByValOk: pass by value
  // sizeof(T) = 0 ∨ ByValOk(T) → ByValue
  if (*size == 0 || ByValOk(ctx, type)) {
    SPEC_RULE("ABI-Param-ByValue-Move");
    return PassKind::ByValue;
  }

  // (ABI-Param-ByRef-Move)
  // Move param that is too large: pass by reference
  // sizeof(T) > 0 ∧ ¬ByValOk(T) → ByRef
  SPEC_RULE("ABI-Param-ByRef-Move");
  return PassKind::ByRef;
}

// ABIRet(T) ⇓ PassKind
std::optional<PassKind> ABIRet(const sema::ScopeContext& ctx,
                               const sema::TypeRef& type) {
  if (!type) {
    return std::nullopt;
  }

  const auto size = SizeOf(ctx, type);
  if (!size.has_value()) {
    return std::nullopt;
  }

  // (ABI-Ret-ByValue)
  // Zero-sized or ByValOk return types are passed by value.
  // sizeof(T) = 0 ∨ ByValOk(T) → ByValue
  if (*size == 0 || ByValOk(ctx, type)) {
    SPEC_RULE("ABI-Ret-ByValue");
    return PassKind::ByValue;
  }

  // (ABI-Ret-ByRef)
  // Large return types use sret (struct return).
  // sizeof(T) > 0 ∧ ¬ByValOk(T) → SRet
  SPEC_RULE("ABI-Ret-ByRef");
  return PassKind::SRet;
}

// ABICall([⟨m₁,T₁⟩,...,⟨mₙ,Tₙ⟩], R) ⇓ ⟨[k₁,...,kₙ], kᵣ, (kᵣ = SRet)⟩
std::optional<ABICallInfo> ABICall(
    const sema::ScopeContext& ctx,
    const std::vector<std::pair<std::optional<sema::ParamMode>, sema::TypeRef>>& params,
    const sema::TypeRef& ret) {
  SPEC_RULE("ABI-Call");

  ABICallInfo info;
  info.param_kinds.reserve(params.size());

  // Compute PassKind for each parameter
  for (const auto& [mode, type] : params) {
    const auto kind = ABIParam(ctx, mode, type);
    if (!kind.has_value()) {
      return std::nullopt;
    }
    info.param_kinds.push_back(*kind);
  }

  // Compute PassKind for return type
  const auto ret_kind = ABIRet(ctx, ret);
  if (!ret_kind.has_value()) {
    return std::nullopt;
  }
  info.ret_kind = *ret_kind;
  info.has_sret = (*ret_kind == PassKind::SRet);

  return info;
}

// -----------------------------------------------------------------------------
// Panic Out-Parameter Support (Cursive0)
// -----------------------------------------------------------------------------

// PanicRecord type: { panic: bool, code: u32 }
sema::TypeRef PanicRecordType() {
  // PanicRecordFields = [⟨panic, bool⟩, ⟨code, u32⟩]
  // We represent this as a tuple type (bool, u32) for simplicity.
  // In a full implementation, this would be a named record type.
  std::vector<sema::TypeRef> fields;
  fields.push_back(sema::MakeTypePrim("bool"));
  fields.push_back(sema::MakeTypePrim("u32"));
  return sema::MakeTypeTuple(std::move(fields));
}

// PanicOutType = rawptr[mut, PanicRecord]
sema::TypeRef PanicOutType() {
  return sema::MakeTypeRawPtr(sema::RawPtrQual::Mut, PanicRecordType());
}

// NeedsPanicOut(callee) ⟺ callee ≠ RecordCtor(_) ∧ callee ≠ EntrySym ∧ RuntimeSig(callee) undefined
bool NeedsPanicOut(std::string_view callee_sym) {
  // Check if this is the entry symbol (main)
  if (callee_sym == kEntrySym) {
    return false;
  }

  // Check if this is a known runtime symbol
  const auto& runtime_syms = RuntimeSymbols();
  if (runtime_syms.find(callee_sym) != runtime_syms.end()) {
    return false;
  }

  // Check if this looks like a record constructor
  if (IsRecordCtorSymbol(callee_sym)) {
    return false;
  }

  // All other user-defined procedures need a panic out-parameter
  return true;
}

// PanicOutParams(params, callee) - appends panic out-param if needed
std::vector<std::tuple<std::optional<sema::ParamMode>, std::string, sema::TypeRef>>
PanicOutParams(
    const std::vector<std::tuple<std::optional<sema::ParamMode>, std::string, sema::TypeRef>>& params,
    std::string_view callee_sym) {
  auto result = params;
  if (NeedsPanicOut(callee_sym)) {
    // Append ⟨move, "__panic", PanicOutType⟩
    result.push_back({sema::ParamMode::Move, std::string(kPanicOutName), PanicOutType()});
  }
  return result;
}

}  // namespace cursive0::codegen

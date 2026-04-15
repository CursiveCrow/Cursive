// =============================================================================
// Panic Out-Parameter Support (§6.2.3)
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.2.3 Panic Out-Parameter (lines 15287-15301)
//   - PanicRecordFields = [<panic, bool>, <code, u32>]
//   - PanicOutType = rawptr[mut, PanicRecord]
//   - NeedsPanicOut predicate
//
// =============================================================================

#include "05_codegen/abi/abi.h"
#include "04_analysis/typing/types.h"
#include "05_codegen/intrinsics/builtins.h"
#include "05_codegen/intrinsics/intrinsics_interface.h"

#include <string>
#include <unordered_set>

namespace cursive::codegen {
namespace {

// Known runtime symbols that do NOT need a panic out-parameter.
// This includes the panic handler itself, context init, and other runtime-defined symbols.
const std::unordered_set<std::string>& RuntimeSymbols() {
  static const std::unordered_set<std::string> syms = [] {
    std::unordered_set<std::string> out;
    auto add = [&out](std::string sym) {
      if (!sym.empty()) {
        out.insert(std::move(sym));
      }
    };

    for (const auto& sym : RuntimeBuiltinNoPanicOutSyms()) {
      add(sym);
    }

    // Structured concurrency runtime symbols.
    add(ConcurrencySymParallelBegin());
    add(ConcurrencySymParallelJoin());
    add(ConcurrencySymSpawnCreate());
    add(ConcurrencySymSpawnWait());
    add(ConcurrencySymDispatchRun());
    add(ConcurrencySymCancelTokenNew());
    add(ConcurrencySymCancelTokenCancel());
    add(ConcurrencySymCancelTokenIsCancelled());
    add(ConcurrencySymParallelWorkPanic());
    add("cursive_panic");

    return out;
  }();
  return syms;
}

// EntrySym is the program entry point.
constexpr std::string_view kEntrySym = "main";

// Check if a symbol looks like a record constructor.
bool IsRecordCtorSymbol(std::string_view /*sym*/) {
  // Record constructor symbols typically don't contain "::" method separators
  // after the type path, and don't have function-like mangled suffixes.
  // Conservative: assume not a record ctor.
  return false;
}

}  // namespace

// PanicRecord type: { panic: bool, code: u32 }
analysis::TypeRef PanicRecordType() {
  std::vector<analysis::TypeRef> fields;
  fields.push_back(analysis::MakeTypePrim("bool"));
  fields.push_back(analysis::MakeTypePrim("u32"));
  return analysis::MakeTypeTuple(std::move(fields));
}

// PanicOutType = rawptr[mut, PanicRecord]
analysis::TypeRef PanicOutType() {
  return analysis::MakeTypeRawPtr(analysis::RawPtrQual::Mut, PanicRecordType());
}

// HostedEnvParamType = rawptr[mut, u8]
analysis::TypeRef HostedEnvParamType() {
  return analysis::MakeTypeRawPtr(
      analysis::RawPtrQual::Mut,
      analysis::MakeTypePrim("u8"));
}

IRParam HostedEnvParam() {
  IRParam param;
  param.mode = analysis::ParamMode::Move;
  param.name = std::string(kHostedEnvParamName);
  param.type = HostedEnvParamType();
  return param;
}

// NeedsPanicOut(callee) iff callee != RecordCtor(_) and callee != EntrySym
// and RuntimeSig(callee) undefined
bool NeedsPanicOut(std::string_view callee_sym) {
  if (callee_sym == kEntrySym) {
    return false;
  }

  const auto& runtime_syms = RuntimeSymbols();
  if (runtime_syms.find(std::string(callee_sym)) != runtime_syms.end()) {
    return false;
  }

  if (IsRecordCtorSymbol(callee_sym)) {
    return false;
  }

  return true;
}

// PanicOutParams(params, callee) - appends panic out-param if needed
std::vector<std::tuple<std::optional<analysis::ParamMode>, std::string, analysis::TypeRef>>
PanicOutParams(
    const std::vector<std::tuple<std::optional<analysis::ParamMode>, std::string, analysis::TypeRef>>& params,
    std::string_view callee_sym) {
  auto result = params;
  if (NeedsPanicOut(callee_sym)) {
    result.push_back({analysis::ParamMode::Move, std::string(kPanicOutName), PanicOutType()});
  }
  return result;
}

}  // namespace cursive::codegen

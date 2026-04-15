// =============================================================================
// Builtin Symbol Resolution Implementation
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.9 BuiltinSym and BuiltinModalSym
//   - Section 6.12.14 String/Bytes builtins
//   - Section 18.2 ExecutionDomain builtins
//   - Section 18.6 CancelToken builtins
//   - Section 19 Async/Reactor builtins
//
// This file provides symbol resolution for compiler-known built-in
// procedures and methods. These symbols map to runtime library functions.
//
// =============================================================================

#include "05_codegen/intrinsics/builtins.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "00_core/assert_spec.h"
#include "00_core/symbols.h"
#include "04_analysis/modal/builtin_modal_intrinsics.h"

namespace cursive::codegen {

namespace {

using BuiltinSymbolFactory = std::string (*)();

struct BuiltinSymbolEntry {
  std::string_view qualified_name;
  BuiltinSymbolFactory factory = nullptr;
};

template <std::size_t N>
std::string LookupBuiltinSymbol(
    std::string_view qualified_name,
    const std::array<BuiltinSymbolEntry, N>& entries) {
  for (const auto& entry : entries) {
    if (qualified_name == entry.qualified_name) {
      return entry.factory ? entry.factory() : std::string{};
    }
  }
  return {};
}

void AppendRuntimeSymbol(std::vector<std::string>& out, std::string symbol) {
  if (!symbol.empty()) {
    out.push_back(std::move(symbol));
  }
}

template <std::size_t N>
void AppendRuntimeSymbols(
    std::vector<std::string>& out,
    const std::array<BuiltinSymbolFactory, N>& factories) {
  for (const auto factory : factories) {
    if (!factory) {
      continue;
    }
    AppendRuntimeSymbol(out, factory());
  }
}

void SortUniqueSymbols(std::vector<std::string>& syms) {
  std::sort(syms.begin(), syms.end());
  syms.erase(std::unique(syms.begin(), syms.end()), syms.end());
}

std::string LookupBuiltinModalRuntimeSymbol(
    const analysis::TypePath& modal_path,
    std::optional<std::string_view> state,
    std::string_view member_name) {
  const auto symbol =
      analysis::LookupBuiltinModalRuntimeSymbol(modal_path, state, member_name);
  return symbol.value_or(std::string{});
}

}  // namespace

// =============================================================================
// Section 6.9 RegionLayout
// =============================================================================

struct RegionLayoutInfo {
  std::uint64_t size = 0;
  std::uint64_t align = 0;
  struct Field {
    std::string name;
    std::uint64_t offset = 0;
  };
  std::vector<Field> fields;
};

RegionLayoutInfo RegionLayout() {
  SPEC_RULE("RegionLayout");

  // Per Section 6.9 (RegionLayout):
  // ModalLayout(Region) produces size, align, disc, payload
  // RegionLayout produces size, align, [("disc", disc), ("payload", payload)]
  //
  // Region is a modal type with a discriminant and payload.
  // For Win64 x86_64-pc-windows-msvc target:
  // - Discriminant: 1 byte at offset 0
  // - Payload: pointer-sized (8 bytes) at offset 8 (aligned)
  // Total size: 16 bytes, alignment: 8

  RegionLayoutInfo layout;
  layout.size = 16;
  layout.align = 8;
  layout.fields.push_back({"disc", 0});
  layout.fields.push_back({"payload", 8});
  return layout;
}

// =============================================================================
// Section 6.9 BuiltinModalSym - Region method symbols
// =============================================================================

std::string BuiltinModalSymRegionNewScoped() {
  SPEC_RULE("BuiltinModalSym-NewScoped");
  return core::PathSig({"cursive", "runtime", "region", "new_scoped"});
}

std::string BuiltinModalSymRegionAlloc() {
  SPEC_RULE("BuiltinModalSym-Alloc");
  return core::PathSig({"cursive", "runtime", "region", "alloc"});
}

std::string BuiltinModalSymRegionMark() {
  SPEC_RULE("BuiltinModalSym-Mark");
  return core::PathSig({"cursive", "runtime", "region", "mark"});
}

std::string BuiltinModalSymRegionResetTo() {
  SPEC_RULE("BuiltinModalSym-ResetTo");
  return core::PathSig({"cursive", "runtime", "region", "reset_to"});
}

std::string BuiltinModalSymRegionResetUnchecked() {
  SPEC_RULE("BuiltinModalSym-ResetUnchecked");
  return core::PathSig({"cursive", "runtime", "region", "reset_unchecked"});
}

std::string BuiltinModalSymRegionFreeze() {
  SPEC_RULE("BuiltinModalSym-Freeze");
  return core::PathSig({"cursive", "runtime", "region", "freeze"});
}

std::string BuiltinModalSymRegionThaw() {
  SPEC_RULE("BuiltinModalSym-Thaw");
  return core::PathSig({"cursive", "runtime", "region", "thaw"});
}

std::string BuiltinModalSymRegionFreeUnchecked() {
  SPEC_RULE("BuiltinModalSym-FreeUnchecked");
  return core::PathSig({"cursive", "runtime", "region", "free_unchecked"});
}

std::string BuiltinModalSymRegionAddrIsActive() {
  SPEC_RULE("BuiltinModalSym-AddrIsActive");
  return core::PathSig({"cursive", "runtime", "region", "addr_is_active"});
}

std::string BuiltinModalSymRegionAddrTagFrom() {
  SPEC_RULE("BuiltinModalSym-AddrTagFrom");
  return core::PathSig({"cursive", "runtime", "region", "addr_tag_from"});
}

std::string BuiltinModalSymRegionScopeEnter() {
  return core::PathSig({"cursive", "runtime", "region", "scope_enter"});
}

std::string BuiltinModalSymRegionScopeExit() {
  return core::PathSig({"cursive", "runtime", "region", "scope_exit"});
}

std::string BuiltinModalSymRegionAddrTagScope() {
  return core::PathSig({"cursive", "runtime", "region", "addr_tag_scope"});
}

std::string BuiltinModalSym(const std::string& method) {
  SPEC_DEF("BuiltinModalSym", "");
  static const std::array<BuiltinSymbolEntry, 11> kRegionModalMethods = {{
      {"new_scoped", &BuiltinModalSymRegionNewScoped},
      {"alloc", &BuiltinModalSymRegionAlloc},
      {"mark", &BuiltinModalSymRegionMark},
      {"reset_to", &BuiltinModalSymRegionResetTo},
      {"reset_unchecked", &BuiltinModalSymRegionResetUnchecked},
      {"freeze", &BuiltinModalSymRegionFreeze},
      {"thaw", &BuiltinModalSymRegionThaw},
      {"free_unchecked", &BuiltinModalSymRegionFreeUnchecked},
      {"scope_enter", &BuiltinModalSymRegionScopeEnter},
      {"scope_exit", &BuiltinModalSymRegionScopeExit},
      {"addr_tag_scope", &BuiltinModalSymRegionAddrTagScope},
  }};
  return LookupBuiltinSymbol(method, kRegionModalMethods);
}

// =============================================================================
// Section 6.9 BuiltinSym - FileSystem capability methods
// =============================================================================

std::string BuiltinSymFileSystemOpenRead() {
  SPEC_RULE("BuiltinSym-FileSystem-OpenRead");
  return core::PathSig({"cursive", "runtime", "fs", "open_read"});
}

std::string BuiltinSymFileSystemOpenWrite() {
  SPEC_RULE("BuiltinSym-FileSystem-OpenWrite");
  return core::PathSig({"cursive", "runtime", "fs", "open_write"});
}

std::string BuiltinSymFileSystemOpenAppend() {
  SPEC_RULE("BuiltinSym-FileSystem-OpenAppend");
  return core::PathSig({"cursive", "runtime", "fs", "open_append"});
}

std::string BuiltinSymFileSystemCreateWrite() {
  SPEC_RULE("BuiltinSym-FileSystem-CreateWrite");
  return core::PathSig({"cursive", "runtime", "fs", "create_write"});
}

std::string BuiltinSymFileSystemReadFile() {
  SPEC_RULE("BuiltinSym-FileSystem-ReadFile");
  return core::PathSig({"cursive", "runtime", "fs", "read_file"});
}

std::string BuiltinSymFileSystemReadBytes() {
  SPEC_RULE("BuiltinSym-FileSystem-ReadBytes");
  return core::PathSig({"cursive", "runtime", "fs", "read_bytes"});
}

std::string BuiltinSymFileSystemWriteFile() {
  SPEC_RULE("BuiltinSym-FileSystem-WriteFile");
  return core::PathSig({"cursive", "runtime", "fs", "write_file"});
}

std::string BuiltinSymFileSystemWriteStdout() {
  SPEC_RULE("BuiltinSym-FileSystem-WriteStdout");
  return core::PathSig({"cursive", "runtime", "fs", "write_stdout"});
}

std::string BuiltinSymFileSystemWriteStderr() {
  SPEC_RULE("BuiltinSym-FileSystem-WriteStderr");
  return core::PathSig({"cursive", "runtime", "fs", "write_stderr"});
}

std::string BuiltinSymFileSystemExists() {
  SPEC_RULE("BuiltinSym-FileSystem-Exists");
  return core::PathSig({"cursive", "runtime", "fs", "exists"});
}

std::string BuiltinSymFileSystemRemove() {
  SPEC_RULE("BuiltinSym-FileSystem-Remove");
  return core::PathSig({"cursive", "runtime", "fs", "remove"});
}

std::string BuiltinSymFileSystemOpenDir() {
  SPEC_RULE("BuiltinSym-FileSystem-OpenDir");
  return core::PathSig({"cursive", "runtime", "fs", "open_dir"});
}

std::string BuiltinSymFileSystemCreateDir() {
  SPEC_RULE("BuiltinSym-FileSystem-CreateDir");
  return core::PathSig({"cursive", "runtime", "fs", "create_dir"});
}

std::string BuiltinSymFileSystemEnsureDir() {
  SPEC_RULE("BuiltinSym-FileSystem-EnsureDir");
  return core::PathSig({"cursive", "runtime", "fs", "ensure_dir"});
}

std::string BuiltinSymFileSystemKind() {
  SPEC_RULE("BuiltinSym-FileSystem-Kind");
  return core::PathSig({"cursive", "runtime", "fs", "kind"});
}

std::string BuiltinSymFileSystemRestrict() {
  SPEC_RULE("BuiltinSym-FileSystem-Restrict");
  return core::PathSig({"cursive", "runtime", "fs", "restrict"});
}

// =============================================================================
// Section 6.9 BuiltinSym - Network capability methods
// =============================================================================

std::string BuiltinSymNetworkRestrictHost() {
  SPEC_RULE("BuiltinSym-Network-RestrictHost");
  return core::PathSig({"cursive", "runtime", "net", "restrict_to_host"});
}

// =============================================================================
// Section 6.9 BuiltinSym - HeapAllocator capability methods
// =============================================================================

std::string BuiltinSymHeapAllocatorWithQuota() {
  SPEC_RULE("BuiltinSym-HeapAllocator-WithQuota");
  return core::PathSig({"cursive", "runtime", "heap", "with_quota"});
}

std::string BuiltinSymHeapAllocatorAllocRaw() {
  SPEC_RULE("BuiltinSym-HeapAllocator-AllocRaw");
  return core::PathSig({"cursive", "runtime", "heap", "alloc_raw"});
}

std::string BuiltinSymHeapAllocatorDeallocRaw() {
  SPEC_RULE("BuiltinSym-HeapAllocator-DeallocRaw");
  return core::PathSig({"cursive", "runtime", "heap", "dealloc_raw"});
}

// =============================================================================
// Section 5.9.4 System builtins
// =============================================================================

std::string BuiltinSymSystemExit() {
  SPEC_RULE("BuiltinSym-System-Exit");
  return core::PathSig({"cursive", "runtime", "system", "exit"});
}

std::string BuiltinSymSystemGetEnv() {
  SPEC_RULE("BuiltinSym-System-GetEnv");
  return core::PathSig({"cursive", "runtime", "system", "get_env"});
}

std::string BuiltinSymSystemRun() {
  SPEC_RULE("BuiltinSym-System-Run");
  return core::PathSig({"cursive", "runtime", "system", "run"});
}

// =============================================================================
// Section 18.2 ExecutionDomain builtins
// =============================================================================

std::string BuiltinSymExecutionDomainName() {
  SPEC_DEF("BuiltinSym-ExecutionDomain-Name", "Section 18.2.4");
  return core::PathSig({"cursive", "runtime", "execution_domain", "name"});
}

std::string BuiltinSymExecutionDomainMaxConcurrency() {
  SPEC_DEF("BuiltinSym-ExecutionDomain-MaxConcurrency", "Section 18.2.4");
  return core::PathSig({"cursive", "runtime", "execution_domain", "max_concurrency"});
}

// =============================================================================
// Section 18.2 Context execution domain constructors
// =============================================================================

std::string BuiltinSymContextCpu() {
  SPEC_DEF("BuiltinSym-Context-Cpu", "Section 18.2.1");
  return core::PathSig({"cursive", "runtime", "context", "cpu"});
}

std::string BuiltinSymContextGpu() {
  SPEC_DEF("BuiltinSym-Context-Gpu", "Section 18.2.2");
  return core::PathSig({"cursive", "runtime", "context", "gpu"});
}

std::string BuiltinSymContextInline() {
  SPEC_DEF("BuiltinSym-Context-Inline", "Section 18.2.3");
  return core::PathSig({"cursive", "runtime", "context", "inline"});
}

// =============================================================================
// Section 18.6 CancelToken builtins// =============================================================================
// Section 18.2 GPU intrinsics
// =============================================================================

std::string BuiltinSymGpuGlobalId() {
  SPEC_DEF("BuiltinSym-Gpu-GlobalId", "Section 18.2.2.4");
  return core::PathSig({"cursive", "runtime", "gpu", "global_id"});
}

std::string BuiltinSymGpuLocalId() {
  SPEC_DEF("BuiltinSym-Gpu-LocalId", "Section 18.2.2.4");
  return core::PathSig({"cursive", "runtime", "gpu", "local_id"});
}

std::string BuiltinSymGpuWorkgroupId() {
  SPEC_DEF("BuiltinSym-Gpu-WorkgroupId", "Section 18.2.2.4");
  return core::PathSig({"cursive", "runtime", "gpu", "workgroup_id"});
}

std::string BuiltinSymGpuWorkgroupSize() {
  SPEC_DEF("BuiltinSym-Gpu-WorkgroupSize", "Section 18.2.2.4");
  return core::PathSig({"cursive", "runtime", "gpu", "workgroup_size"});
}

std::string BuiltinSymGpuGlobalSize() {
  SPEC_DEF("BuiltinSym-Gpu-GlobalSize", "Section 18.2.2.4");
  return core::PathSig({"cursive", "runtime", "gpu", "global_size"});
}

std::string BuiltinSymGpuNumWorkgroups() {
  SPEC_DEF("BuiltinSym-Gpu-NumWorkgroups", "Section 18.2.2.4");
  return core::PathSig({"cursive", "runtime", "gpu", "num_workgroups"});
}

std::string BuiltinSymGpuLinearId() {
  SPEC_DEF("BuiltinSym-Gpu-LinearId", "Section 18.2.2.4");
  return core::PathSig({"cursive", "runtime", "gpu", "linear_id"});
}

std::string BuiltinSymGpuBarrier() {
  SPEC_DEF("BuiltinSym-Gpu-Barrier", "Section 18.2.2.4");
  return core::PathSig({"cursive", "runtime", "gpu", "barrier"});
}

std::string BuiltinSymGpuMemoryBarrier() {
  SPEC_DEF("BuiltinSym-Gpu-MemoryBarrier", "Section 18.2.2.4");
  return core::PathSig({"cursive", "runtime", "gpu", "memory_barrier"});
}

std::string BuiltinSymGpuWorkgroupBarrier() {
  SPEC_DEF("BuiltinSym-Gpu-WorkgroupBarrier", "Section 18.2.2.4");
  return core::PathSig({"cursive", "runtime", "gpu", "workgroup_barrier"});
}


// =============================================================================

std::string BuiltinSymCancelTokenNew() {
  SPEC_DEF("BuiltinSym-CancelToken-New", "Section 18.6.1");
  return core::PathSig({"CancelToken", "new"});
}

std::string BuiltinSymCancelTokenActiveCancel() {
  SPEC_DEF("BuiltinSym-CancelToken-Active-cancel", "Section 18.6.1");
  return core::PathSig({"CancelToken", "Active", "cancel"});
}

std::string BuiltinSymCancelTokenActiveIsCancelled() {
  SPEC_DEF("BuiltinSym-CancelToken-Active-is_cancelled", "Section 18.6.1");
  return core::PathSig({"CancelToken", "Active", "is_cancelled"});
}

std::string BuiltinSymCancelTokenActiveChild() {
  SPEC_DEF("BuiltinSym-CancelToken-Active-child", "Section 18.6.1");
  return core::PathSig({"CancelToken", "Active", "child"});
}

std::string BuiltinSymCancelTokenActiveWaitCancelled() {
  SPEC_DEF("BuiltinSym-CancelToken-Active-wait_cancelled", "Section 18.6.1");
  return core::PathSig({"CancelToken", "Active", "wait_cancelled"});
}

// =============================================================================
// Section 6.12.14 String/Bytes builtin symbols
// =============================================================================

std::string BuiltinSymStringFrom() {
  SPEC_DEF("BuiltinSym-string-from", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "from"});
}

std::string BuiltinSymStringAsView() {
  SPEC_DEF("BuiltinSym-string-as_view", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "as_view"});
}

std::string BuiltinSymStringToManaged() {
  SPEC_DEF("BuiltinSym-string-to_managed", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "to_managed"});
}

std::string BuiltinSymStringCloneWith() {
  SPEC_DEF("BuiltinSym-string-clone_with", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "clone_with"});
}

std::string BuiltinSymStringAppend() {
  SPEC_DEF("BuiltinSym-string-append", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "append"});
}

std::string BuiltinSymStringLength() {
  SPEC_DEF("BuiltinSym-string-length", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "length"});
}

std::string BuiltinSymStringIsEmpty() {
  SPEC_DEF("BuiltinSym-string-is_empty", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "is_empty"});
}

std::string BuiltinSymStringDropManaged() {
  SPEC_RULE("StringDropSym-Decl");
  SPEC_DEF("BuiltinSym-string-drop_managed", "Section 6.12.14");
  std::string sym = core::PathSig({"cursive", "runtime", "string", "drop_managed"});
  if (sym.empty()) {
    SPEC_RULE("StringDropSym-Err");
  }
  return sym;
}

std::string BuiltinSymBytesWithCapacity() {
  SPEC_DEF("BuiltinSym-bytes-with_capacity", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "with_capacity"});
}

std::string BuiltinSymBytesFromSlice() {
  SPEC_DEF("BuiltinSym-bytes-from_slice", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "from_slice"});
}

std::string BuiltinSymBytesAsView() {
  SPEC_DEF("BuiltinSym-bytes-as_view", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "as_view"});
}

std::string BuiltinSymBytesAsSlice() {
  SPEC_DEF("BuiltinSym-bytes-as_slice", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "as_slice"});
}

std::string BuiltinSymBytesToManaged() {
  SPEC_DEF("BuiltinSym-bytes-to_managed", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "to_managed"});
}

std::string BuiltinSymBytesView() {
  SPEC_DEF("BuiltinSym-bytes-view", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "view"});
}

std::string BuiltinSymBytesViewString() {
  SPEC_DEF("BuiltinSym-bytes-view_string", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "view_string"});
}

std::string BuiltinSymBytesAppend() {
  SPEC_DEF("BuiltinSym-bytes-append", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "append"});
}

std::string BuiltinSymBytesLength() {
  SPEC_DEF("BuiltinSym-bytes-length", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "length"});
}

std::string BuiltinSymBytesIsEmpty() {
  SPEC_DEF("BuiltinSym-bytes-is_empty", "Section 6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "is_empty"});
}

std::string BuiltinSymBytesDropManaged() {
  SPEC_RULE("BytesDropSym-Decl");
  SPEC_DEF("BuiltinSym-bytes-drop_managed", "Section 6.12.14");
  std::string sym = core::PathSig({"cursive", "runtime", "bytes", "drop_managed"});
  if (sym.empty()) {
    SPEC_RULE("BytesDropSym-Err");
  }
  return sym;
}

// =============================================================================
// Section 19 Reactor builtins
// =============================================================================

std::string BuiltinSymReactorRun() {
  SPEC_DEF("BuiltinSym-Reactor-Run", "Section 19");
  return core::PathSig({"cursive", "runtime", "reactor", "run"});
}

std::string BuiltinSymReactorRegister() {
  SPEC_DEF("BuiltinSym-Reactor-Register", "Section 19");
  return core::PathSig({"cursive", "runtime", "reactor", "register"});
}

// =============================================================================
// Section 19 Async builtins
// =============================================================================

std::string BuiltinSymAsyncResume() {
  SPEC_DEF("BuiltinSym-Async-Resume", "Section 19.2.2");
  return core::PathSig({"cursive", "runtime", "async", "resume"});
}

std::string BuiltinSymAsyncGetDiscriminant() {
  SPEC_DEF("BuiltinSym-Async-GetDiscriminant", "Section 19");
  return core::PathSig({"cursive", "runtime", "async", "get_discriminant"});
}

std::string BuiltinSymAsyncGetSuspendedOutput() {
  SPEC_DEF("BuiltinSym-Async-GetSuspendedOutput", "Section 19");
  return core::PathSig({"cursive", "runtime", "async", "get_suspended_output"});
}

std::string BuiltinSymAsyncGetCompletedValue() {
  SPEC_DEF("BuiltinSym-Async-GetCompletedValue", "Section 19");
  return core::PathSig({"cursive", "runtime", "async", "get_completed_value"});
}

std::string BuiltinSymAsyncGetFailedError() {
  SPEC_DEF("BuiltinSym-Async-GetFailedError", "Section 19");
  return core::PathSig({"cursive", "runtime", "async", "get_failed_error"});
}

std::string BuiltinSymAsyncCreateCompleted() {
  SPEC_DEF("BuiltinSym-Async-CreateCompleted", "Section 19");
  return core::PathSig({"cursive", "runtime", "async", "create_completed"});
}

std::string BuiltinSymAsyncCreateFailed() {
  SPEC_DEF("BuiltinSym-Async-CreateFailed", "Section 19");
  return core::PathSig({"cursive", "runtime", "async", "create_failed"});
}

std::string BuiltinSymAsyncCreateSuspended() {
  SPEC_DEF("BuiltinSym-Async-CreateSuspended", "Section 19");
  return core::PathSig({"cursive", "runtime", "async", "create_suspended"});
}

std::string BuiltinSymAsyncAllocFrame() {
  SPEC_DEF("BuiltinSym-Async-AllocFrame", "Section 19");
  return core::PathSig({"cursive", "runtime", "async", "alloc_frame"});
}

std::string BuiltinSymAsyncFreeFrame() {
  SPEC_DEF("BuiltinSym-Async-FreeFrame", "Section 19");
  return core::PathSig({"cursive", "runtime", "async", "free_frame"});
}

// =============================================================================
// Section 6.8 Panic symbol
// =============================================================================

std::string RuntimePanicSym() {
  SPEC_RULE("PanicSym");
  return core::PathSig({"cursive", "runtime", "panic"});
}

// =============================================================================
// Section 6.12.5 Context initialization symbol
// =============================================================================

std::string ContextInitSym() {
  SPEC_RULE("ContextInitSym-Decl");
  SPEC_DEF("ContextInitSym", "Section 6.12.5");
  return core::PathSig({"cursive", "runtime", "context_init"});
}

// =============================================================================
// Runtime conformance emission symbols
// =============================================================================

std::string RuntimeConformanceEmitSym() {
  return core::PathSig({"cursive", "runtime", "conformance", "emit"});
}

std::string RuntimeConformanceEmitIntSym() {
  return core::PathSig({"cursive", "runtime", "conformance", "emit_int"});
}

std::string RuntimeConformanceEmitBoolSym() {
  return core::PathSig({"cursive", "runtime", "conformance", "emit_bool"});
}

std::string RuntimeConformanceEmitFloatSym() {
  return core::PathSig({"cursive", "runtime", "conformance", "emit_float"});
}

std::string RuntimeConformanceEmitPtrSym() {
  return core::PathSig({"cursive", "runtime", "conformance", "emit_ptr"});
}

std::string RuntimeConformanceEmitStringSym() {
  return core::PathSig({"cursive", "runtime", "conformance", "emit_string"});
}

std::string RuntimeConformanceEmitStringManagedSym() {
  return core::PathSig({"cursive", "runtime", "conformance", "emit_string_managed"});
}

std::string RuntimeConformanceEmitBytesSym() {
  return core::PathSig({"cursive", "runtime", "conformance", "emit_bytes"});
}

std::string RuntimeConformanceEmitBytesManagedSym() {
  return core::PathSig({"cursive", "runtime", "conformance", "emit_bytes_managed"});
}

std::string RuntimeConformanceSetSinkSym() {
  return core::PathSig({"cursive", "runtime", "conformance", "set_sink"});
}

std::string RuntimeConformanceSetRootSym() {
  return core::PathSig({"cursive", "runtime", "conformance", "set_root"});
}

std::string RuntimeConformanceSetLogFilterSym() {
  return core::PathSig({"cursive", "runtime", "conformance", "set_log_filter"});
}

std::string RuntimeConformanceSetMinLevelSym() {
  return core::PathSig({"cursive", "runtime", "conformance", "set_min_level"});
}

std::vector<std::string> RuntimeLinkRequiredSyms() {
  std::vector<std::string> syms;

  AppendRuntimeSymbol(syms, RuntimePanicSym());
  AppendRuntimeSymbol(syms, BuiltinSymStringDropManaged());
  AppendRuntimeSymbol(syms, BuiltinSymBytesDropManaged());
  AppendRuntimeSymbol(syms, ContextInitSym());
  AppendRuntimeSymbol(syms, RuntimeConformanceEmitSym());
  AppendRuntimeSymbol(syms, RuntimeConformanceEmitIntSym());
  AppendRuntimeSymbol(syms, RuntimeConformanceEmitBoolSym());
  AppendRuntimeSymbol(syms, RuntimeConformanceEmitFloatSym());
  AppendRuntimeSymbol(syms, RuntimeConformanceEmitPtrSym());
  AppendRuntimeSymbol(syms, RuntimeConformanceEmitStringSym());
  AppendRuntimeSymbol(syms, RuntimeConformanceEmitStringManagedSym());
  AppendRuntimeSymbol(syms, RuntimeConformanceEmitBytesSym());
  AppendRuntimeSymbol(syms, RuntimeConformanceEmitBytesManagedSym());
  AppendRuntimeSymbol(syms, RuntimeConformanceSetSinkSym());
  AppendRuntimeSymbol(syms, RuntimeConformanceSetRootSym());
  AppendRuntimeSymbol(syms, RuntimeConformanceSetLogFilterSym());
  AppendRuntimeSymbol(syms, RuntimeConformanceSetMinLevelSym());

  static const std::array<BuiltinSymbolFactory, 13> kRegionSymbols = {{
      &BuiltinModalSymRegionNewScoped,
      &BuiltinModalSymRegionAlloc,
      &BuiltinModalSymRegionMark,
      &BuiltinModalSymRegionResetTo,
      &BuiltinModalSymRegionResetUnchecked,
      &BuiltinModalSymRegionFreeze,
      &BuiltinModalSymRegionThaw,
      &BuiltinModalSymRegionFreeUnchecked,
      &BuiltinModalSymRegionAddrIsActive,
      &BuiltinModalSymRegionAddrTagFrom,
      &BuiltinModalSymRegionScopeEnter,
      &BuiltinModalSymRegionScopeExit,
      &BuiltinModalSymRegionAddrTagScope,
  }};
  AppendRuntimeSymbols(syms, kRegionSymbols);

  static const std::array<BuiltinSymbolFactory, 7> kStringSymbols = {{
      &BuiltinSymStringFrom,
      &BuiltinSymStringAsView,
      &BuiltinSymStringToManaged,
      &BuiltinSymStringCloneWith,
      &BuiltinSymStringAppend,
      &BuiltinSymStringLength,
      &BuiltinSymStringIsEmpty,
  }};
  AppendRuntimeSymbols(syms, kStringSymbols);

  static const std::array<BuiltinSymbolFactory, 10> kBytesSymbols = {{
      &BuiltinSymBytesWithCapacity,
      &BuiltinSymBytesFromSlice,
      &BuiltinSymBytesAsView,
      &BuiltinSymBytesToManaged,
      &BuiltinSymBytesView,
      &BuiltinSymBytesViewString,
      &BuiltinSymBytesAsSlice,
      &BuiltinSymBytesAppend,
      &BuiltinSymBytesLength,
      &BuiltinSymBytesIsEmpty,
  }};
  AppendRuntimeSymbols(syms, kBytesSymbols);

  static const std::array<BuiltinSymbolFactory, 16> kFileSystemSymbols = {{
      &BuiltinSymFileSystemOpenRead,
      &BuiltinSymFileSystemOpenWrite,
      &BuiltinSymFileSystemOpenAppend,
      &BuiltinSymFileSystemCreateWrite,
      &BuiltinSymFileSystemReadFile,
      &BuiltinSymFileSystemReadBytes,
      &BuiltinSymFileSystemWriteFile,
      &BuiltinSymFileSystemWriteStdout,
      &BuiltinSymFileSystemWriteStderr,
      &BuiltinSymFileSystemExists,
      &BuiltinSymFileSystemRemove,
      &BuiltinSymFileSystemOpenDir,
      &BuiltinSymFileSystemCreateDir,
      &BuiltinSymFileSystemEnsureDir,
      &BuiltinSymFileSystemKind,
      &BuiltinSymFileSystemRestrict,
  }};
  AppendRuntimeSymbols(syms, kFileSystemSymbols);

  static const std::array<BuiltinSymbolFactory, 1> kNetworkSymbols = {{
      &BuiltinSymNetworkRestrictHost,
  }};
  AppendRuntimeSymbols(syms, kNetworkSymbols);

  static const std::array<BuiltinSymbolFactory, 3> kHeapSymbols = {{
      &BuiltinSymHeapAllocatorWithQuota,
      &BuiltinSymHeapAllocatorAllocRaw,
      &BuiltinSymHeapAllocatorDeallocRaw,
  }};
  AppendRuntimeSymbols(syms, kHeapSymbols);

  static const std::array<BuiltinSymbolFactory, 3> kSystemSymbols = {{
      &BuiltinSymSystemExit,
      &BuiltinSymSystemGetEnv,
      &BuiltinSymSystemRun,
  }};
  AppendRuntimeSymbols(syms, kSystemSymbols);

  SortUniqueSymbols(syms);
  return syms;
}

std::vector<std::string> RuntimeBuiltinNoPanicOutSyms() {
  std::vector<std::string> syms = RuntimeLinkRequiredSyms();
  static const std::array<BuiltinSymbolFactory, 24> kAdditionalSymbols = {{
      &BuiltinSymAsyncResume,
      &BuiltinSymAsyncAllocFrame,
      &BuiltinSymAsyncFreeFrame,
      &BuiltinSymExecutionDomainName,
      &BuiltinSymExecutionDomainMaxConcurrency,
      &BuiltinSymContextCpu,
      &BuiltinSymContextGpu,
      &BuiltinSymContextInline,
      &BuiltinSymGpuGlobalId,
      &BuiltinSymGpuLocalId,
      &BuiltinSymGpuWorkgroupId,
      &BuiltinSymGpuWorkgroupSize,
      &BuiltinSymGpuGlobalSize,
      &BuiltinSymGpuNumWorkgroups,
      &BuiltinSymGpuLinearId,
      &BuiltinSymGpuBarrier,
      &BuiltinSymGpuMemoryBarrier,
      &BuiltinSymGpuWorkgroupBarrier,
      &BuiltinSymCancelTokenNew,
      &BuiltinSymCancelTokenActiveCancel,
      &BuiltinSymCancelTokenActiveIsCancelled,
      &BuiltinSymCancelTokenActiveChild,
      &BuiltinSymCancelTokenActiveWaitCancelled,
  }};
  AppendRuntimeSymbols(syms, kAdditionalSymbols);

  static const std::array<BuiltinSymbolFactory, 2> kReactorSymbols = {{
      &BuiltinSymReactorRun,
      &BuiltinSymReactorRegister,
  }};
  AppendRuntimeSymbols(syms, kReactorSymbols);

  SortUniqueSymbols(syms);
  return syms;
}

// =============================================================================
// Dispatch function for BuiltinSym by qualified name
// =============================================================================

std::string BuiltinSym(const std::string& qualified_name) {
  SPEC_DEF("BuiltinSym", "");

  static const std::array<BuiltinSymbolEntry, 16> kFileSystemBuiltins = {{
      {"FileSystem::open_read", &BuiltinSymFileSystemOpenRead},
      {"FileSystem::open_write", &BuiltinSymFileSystemOpenWrite},
      {"FileSystem::open_append", &BuiltinSymFileSystemOpenAppend},
      {"FileSystem::create_write", &BuiltinSymFileSystemCreateWrite},
      {"FileSystem::read_file", &BuiltinSymFileSystemReadFile},
      {"FileSystem::read_bytes", &BuiltinSymFileSystemReadBytes},
      {"FileSystem::write_file", &BuiltinSymFileSystemWriteFile},
      {"FileSystem::write_stdout", &BuiltinSymFileSystemWriteStdout},
      {"FileSystem::write_stderr", &BuiltinSymFileSystemWriteStderr},
      {"FileSystem::exists", &BuiltinSymFileSystemExists},
      {"FileSystem::remove", &BuiltinSymFileSystemRemove},
      {"FileSystem::open_dir", &BuiltinSymFileSystemOpenDir},
      {"FileSystem::create_dir", &BuiltinSymFileSystemCreateDir},
      {"FileSystem::ensure_dir", &BuiltinSymFileSystemEnsureDir},
      {"FileSystem::kind", &BuiltinSymFileSystemKind},
      {"FileSystem::restrict", &BuiltinSymFileSystemRestrict},
  }};
  static const std::array<BuiltinSymbolEntry, 1> kNetworkBuiltins = {{
      {"Network::restrict_to_host", &BuiltinSymNetworkRestrictHost},
  }};
  static const std::array<BuiltinSymbolEntry, 3> kHeapBuiltins = {{
      {"HeapAllocator::with_quota", &BuiltinSymHeapAllocatorWithQuota},
      {"HeapAllocator::alloc_raw", &BuiltinSymHeapAllocatorAllocRaw},
      {"HeapAllocator::dealloc_raw", &BuiltinSymHeapAllocatorDeallocRaw},
  }};
  static const std::array<BuiltinSymbolEntry, 3> kSystemBuiltins = {{
      {"System::exit", &BuiltinSymSystemExit},
      {"System::get_env", &BuiltinSymSystemGetEnv},
      {"System::run", &BuiltinSymSystemRun},
  }};
  static const std::array<BuiltinSymbolEntry, 2> kExecutionDomainBuiltins = {{
      {"ExecutionDomain::name", &BuiltinSymExecutionDomainName},
      {"ExecutionDomain::max_concurrency", &BuiltinSymExecutionDomainMaxConcurrency},
  }};
  static const std::array<BuiltinSymbolEntry, 3> kContextBuiltins = {{
      {"Context::cpu", &BuiltinSymContextCpu},
      {"Context::gpu", &BuiltinSymContextGpu},
      {"Context::inline", &BuiltinSymContextInline},
  }};
  static const std::array<BuiltinSymbolEntry, 10> kGpuBuiltins = {{
      {"gpu_global_id", &BuiltinSymGpuGlobalId},
      {"gpu_local_id", &BuiltinSymGpuLocalId},
      {"gpu_workgroup_id", &BuiltinSymGpuWorkgroupId},
      {"gpu_workgroup_size", &BuiltinSymGpuWorkgroupSize},
      {"gpu_global_size", &BuiltinSymGpuGlobalSize},
      {"gpu_num_workgroups", &BuiltinSymGpuNumWorkgroups},
      {"gpu_linear_id", &BuiltinSymGpuLinearId},
      {"gpu_barrier", &BuiltinSymGpuBarrier},
      {"gpu_memory_barrier", &BuiltinSymGpuMemoryBarrier},
      {"gpu_workgroup_barrier", &BuiltinSymGpuWorkgroupBarrier},
  }};
  static const std::array<BuiltinSymbolEntry, 5> kCancelTokenBuiltins = {{
      {"CancelToken::new", &BuiltinSymCancelTokenNew},
      {"CancelToken::Active::cancel", &BuiltinSymCancelTokenActiveCancel},
      {"CancelToken::Active::is_cancelled", &BuiltinSymCancelTokenActiveIsCancelled},
      {"CancelToken::Active::child", &BuiltinSymCancelTokenActiveChild},
      {"CancelToken::Active::wait_cancelled", &BuiltinSymCancelTokenActiveWaitCancelled},
  }};
  static const std::array<BuiltinSymbolEntry, 2> kReactorBuiltins = {{
      {"Reactor::run", &BuiltinSymReactorRun},
      {"Reactor::register", &BuiltinSymReactorRegister},
  }};
  static const std::array<BuiltinSymbolEntry, 7> kStringBuiltins = {{
      {"string::from", &BuiltinSymStringFrom},
      {"string::as_view", &BuiltinSymStringAsView},
      {"string::to_managed", &BuiltinSymStringToManaged},
      {"string::clone_with", &BuiltinSymStringCloneWith},
      {"string::append", &BuiltinSymStringAppend},
      {"string::length", &BuiltinSymStringLength},
      {"string::is_empty", &BuiltinSymStringIsEmpty},
  }};
  static const std::array<BuiltinSymbolEntry, 10> kBytesBuiltins = {{
      {"bytes::with_capacity", &BuiltinSymBytesWithCapacity},
      {"bytes::from_slice", &BuiltinSymBytesFromSlice},
      {"bytes::as_view", &BuiltinSymBytesAsView},
      {"bytes::as_slice", &BuiltinSymBytesAsSlice},
      {"bytes::to_managed", &BuiltinSymBytesToManaged},
      {"bytes::view", &BuiltinSymBytesView},
      {"bytes::view_string", &BuiltinSymBytesViewString},
      {"bytes::append", &BuiltinSymBytesAppend},
      {"bytes::length", &BuiltinSymBytesLength},
      {"bytes::is_empty", &BuiltinSymBytesIsEmpty},
  }};
  static const std::array<BuiltinSymbolEntry, 8> kRegionBuiltins = {{
      {"Region::new_scoped", &BuiltinModalSymRegionNewScoped},
      {"Region::alloc", &BuiltinModalSymRegionAlloc},
      {"Region::mark", &BuiltinModalSymRegionMark},
      {"Region::reset_to", &BuiltinModalSymRegionResetTo},
      {"Region::reset_unchecked", &BuiltinModalSymRegionResetUnchecked},
      {"Region::freeze", &BuiltinModalSymRegionFreeze},
      {"Region::thaw", &BuiltinModalSymRegionThaw},
      {"Region::free_unchecked", &BuiltinModalSymRegionFreeUnchecked},
  }};

  // FileSystem methods
  if (const auto sym = LookupBuiltinSymbol(qualified_name, kFileSystemBuiltins);
      !sym.empty()) {
    return sym;
  }

  // Network methods
  if (const auto sym = LookupBuiltinSymbol(qualified_name, kNetworkBuiltins);
      !sym.empty()) {
    return sym;
  }

  // HeapAllocator methods
  if (const auto sym = LookupBuiltinSymbol(qualified_name, kHeapBuiltins);
      !sym.empty()) {
    return sym;
  }

  // System methods
  if (const auto sym = LookupBuiltinSymbol(qualified_name, kSystemBuiltins);
      !sym.empty()) {
    return sym;
  }

  // ExecutionDomain methods
  if (const auto sym = LookupBuiltinSymbol(qualified_name, kExecutionDomainBuiltins);
      !sym.empty()) {
    return sym;
  }

  // Context execution domain constructors
  if (const auto sym = LookupBuiltinSymbol(qualified_name, kContextBuiltins);
      !sym.empty()) {
    return sym;
  }

  // CancelToken builtins
  // GPU intrinsics (unqualified identifiers)
  if (const auto sym = LookupBuiltinSymbol(qualified_name, kGpuBuiltins);
      !sym.empty()) {
    return sym;
  }


  if (const auto sym = LookupBuiltinSymbol(qualified_name, kCancelTokenBuiltins);
      !sym.empty()) {
    return sym;
  }

  // Reactor builtins (Section 19)
  if (const auto sym = LookupBuiltinSymbol(qualified_name, kReactorBuiltins);
      !sym.empty()) {
    return sym;
  }

  // String builtins
  if (const auto sym = LookupBuiltinSymbol(qualified_name, kStringBuiltins);
      !sym.empty()) {
    return sym;
  }

  // Bytes builtins
  if (const auto sym = LookupBuiltinSymbol(qualified_name, kBytesBuiltins);
      !sym.empty()) {
    return sym;
  }

  // Handle unknown string:: or bytes:: prefixed names
  if (qualified_name.rfind("string::", 0) == 0) {
    SPEC_RULE("BuiltinSym-String-Err");
    return "";
  }
  if (qualified_name.rfind("bytes::", 0) == 0) {
    SPEC_RULE("BuiltinSym-Bytes-Err");
    return "";
  }

  // Region methods (alternate qualified form)
  if (const auto sym = LookupBuiltinSymbol(qualified_name, kRegionBuiltins);
      !sym.empty()) {
    return sym;
  }

  // Unknown builtin - return empty string
  return "";
}

// =============================================================================
// Spec Rule Anchors
// =============================================================================

void AnchorBuiltinSymRules() {
  // Section 6.9 Runtime Interface
  SPEC_RULE("RegionLayout");
  SPEC_RULE("BuiltinModalSym-NewScoped");
  SPEC_RULE("BuiltinModalSym-Alloc");
  SPEC_RULE("BuiltinModalSym-Mark");
  SPEC_RULE("BuiltinModalSym-ResetTo");
  SPEC_RULE("BuiltinModalSym-ResetUnchecked");
  SPEC_RULE("BuiltinModalSym-Freeze");
  SPEC_RULE("BuiltinModalSym-Thaw");
  SPEC_RULE("BuiltinModalSym-FreeUnchecked");
  SPEC_RULE("BuiltinModalSym-AddrIsActive");
  SPEC_RULE("BuiltinModalSym-AddrTagFrom");
  SPEC_RULE("BuiltinSym-FileSystem-OpenRead");
  SPEC_RULE("BuiltinSym-FileSystem-OpenWrite");
  SPEC_RULE("BuiltinSym-FileSystem-OpenAppend");
  SPEC_RULE("BuiltinSym-FileSystem-CreateWrite");
  SPEC_RULE("BuiltinSym-FileSystem-ReadFile");
  SPEC_RULE("BuiltinSym-FileSystem-ReadBytes");
  SPEC_RULE("BuiltinSym-FileSystem-WriteFile");
  SPEC_RULE("BuiltinSym-FileSystem-WriteStdout");
  SPEC_RULE("BuiltinSym-FileSystem-WriteStderr");
  SPEC_RULE("BuiltinSym-FileSystem-Exists");
  SPEC_RULE("BuiltinSym-FileSystem-Remove");
  SPEC_RULE("BuiltinSym-FileSystem-OpenDir");
  SPEC_RULE("BuiltinSym-FileSystem-CreateDir");
  SPEC_RULE("BuiltinSym-FileSystem-EnsureDir");
  SPEC_RULE("BuiltinSym-FileSystem-Kind");
  SPEC_RULE("BuiltinSym-FileSystem-Restrict");
  SPEC_RULE("BuiltinSym-Network-RestrictHost");
  SPEC_RULE("BuiltinSym-HeapAllocator-WithQuota");
  SPEC_RULE("BuiltinSym-HeapAllocator-AllocRaw");
  SPEC_RULE("BuiltinSym-HeapAllocator-DeallocRaw");
  SPEC_RULE("BuiltinSym-System-Exit");
  SPEC_RULE("BuiltinSym-System-GetEnv");
  SPEC_RULE("BuiltinSym-System-Run");

  // Section 6.8 Panic
  SPEC_RULE("PanicSym");
}

}  // namespace cursive::codegen


// =============================================================================
// Intrinsic Interface Implementation
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.9 Runtime Interface
//   - Section 6.12 Codegen Parameters
//
// This file implements the runtime function interface, providing symbol
// lookup and categorization for runtime library functions.
//
// =============================================================================

#include "05_codegen/intrinsics/intrinsics_interface.h"

#include <unordered_map>
#include <unordered_set>

#include "00_core/assert_spec.h"
#include "00_core/symbols.h"
#include "05_codegen/intrinsics/builtins.h"

namespace cursive::codegen {

namespace {

std::string BuiltinSymFileReadReadAll() {
  return core::PathSig({"File", "Read", "read_all"});
}

std::string BuiltinSymFileReadReadAllBytes() {
  return core::PathSig({"File", "Read", "read_all_bytes"});
}

std::string BuiltinSymFileWriteWrite() {
  return core::PathSig({"File", "Write", "write"});
}

std::string BuiltinSymFileWriteFlush() {
  return core::PathSig({"File", "Write", "flush"});
}

std::string BuiltinSymFileWriteClose() {
  return core::PathSig({"File", "Write", "close"});
}

std::string BuiltinSymFileAppendWrite() {
  return core::PathSig({"File", "Append", "write"});
}

std::string BuiltinSymFileAppendFlush() {
  return core::PathSig({"File", "Append", "flush"});
}

std::string BuiltinSymFileAppendClose() {
  return core::PathSig({"File", "Append", "close"});
}

std::string BuiltinSymFileReadClose() {
  return core::PathSig({"File", "Read", "close"});
}

std::string BuiltinSymDirIterOpenNext() {
  return core::PathSig({"DirIter", "Open", "next"});
}

std::string BuiltinSymDirIterOpenClose() {
  return core::PathSig({"DirIter", "Open", "close"});
}

}  // namespace

// =============================================================================
// Runtime Symbol Categorization
// =============================================================================

namespace {

using RuntimeCategoryMap = std::unordered_map<std::string, RuntimeSymbolCategory>;

void AddRuntimeSymbol(RuntimeCategoryMap& categories,
                      RuntimeSymbolCategory category,
                      std::string symbol) {
  if (!symbol.empty()) {
    categories.emplace(std::move(symbol), category);
  }
}

RuntimeCategoryMap BuildRuntimeCategoryMap() {
  RuntimeCategoryMap categories;

  // Core
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Core, RuntimePanicSym());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Core, ContextInitSym());

  // Conformance
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Conformance, RuntimeConformanceEmitSym());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Conformance, RuntimeConformanceEmitIntSym());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Conformance, RuntimeConformanceEmitBoolSym());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Conformance, RuntimeConformanceEmitFloatSym());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Conformance, RuntimeConformanceEmitPtrSym());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Conformance, RuntimeConformanceEmitStringSym());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Conformance, RuntimeConformanceEmitStringManagedSym());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Conformance, RuntimeConformanceEmitBytesSym());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Conformance, RuntimeConformanceEmitBytesManagedSym());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Conformance, RuntimeConformanceSetSinkSym());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Conformance, RuntimeConformanceSetRootSym());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Conformance, RuntimeConformanceSetLogFilterSym());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Conformance, RuntimeConformanceSetMinLevelSym());

  // Region
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Region, BuiltinModalSymRegionNewScoped());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Region, BuiltinModalSymRegionAlloc());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Region, BuiltinModalSymRegionMark());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Region, BuiltinModalSymRegionResetTo());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Region, BuiltinModalSymRegionResetUnchecked());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Region, BuiltinModalSymRegionFreeze());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Region, BuiltinModalSymRegionThaw());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Region, BuiltinModalSymRegionFreeUnchecked());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Region, BuiltinModalSymRegionAddrIsActive());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Region, BuiltinModalSymRegionAddrTagFrom());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Region, BuiltinModalSymRegionScopeEnter());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Region, BuiltinModalSymRegionScopeExit());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Region, BuiltinModalSymRegionAddrTagScope());

  // String
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::String, BuiltinSymStringFrom());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::String, BuiltinSymStringAsView());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::String, BuiltinSymStringToManaged());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::String, BuiltinSymStringCloneWith());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::String, BuiltinSymStringAppend());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::String, BuiltinSymStringLength());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::String, BuiltinSymStringIsEmpty());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::String, BuiltinSymStringDropManaged());

  // Bytes
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Bytes, BuiltinSymBytesWithCapacity());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Bytes, BuiltinSymBytesFromSlice());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Bytes, BuiltinSymBytesAsView());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Bytes, BuiltinSymBytesAsSlice());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Bytes, BuiltinSymBytesToManaged());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Bytes, BuiltinSymBytesView());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Bytes, BuiltinSymBytesViewString());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Bytes, BuiltinSymBytesAppend());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Bytes, BuiltinSymBytesLength());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Bytes, BuiltinSymBytesIsEmpty());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Bytes, BuiltinSymBytesDropManaged());

  // FileSystem
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemOpenRead());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemOpenWrite());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemOpenAppend());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemCreateWrite());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemReadFile());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemReadBytes());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemWriteFile());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemWriteStdout());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemWriteStderr());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemExists());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemRemove());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemOpenDir());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemCreateDir());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemEnsureDir());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemKind());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileSystemRestrict());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileReadReadAll());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileReadReadAllBytes());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileWriteWrite());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileWriteFlush());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileWriteClose());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileAppendWrite());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileAppendFlush());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileAppendClose());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymFileReadClose());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymDirIterOpenNext());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::FileSystem, BuiltinSymDirIterOpenClose());

  // Network
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Network, BuiltinSymNetworkRestrictHost());

  // HeapAllocator
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::HeapAllocator, BuiltinSymHeapAllocatorWithQuota());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::HeapAllocator, BuiltinSymHeapAllocatorAllocRaw());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::HeapAllocator, BuiltinSymHeapAllocatorDeallocRaw());

  // System
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::System, BuiltinSymSystemExit());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::System, BuiltinSymSystemGetEnv());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::System, BuiltinSymSystemRun());

  // ExecutionDomain and Context domain builtins
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymExecutionDomainName());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymExecutionDomainMaxConcurrency());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymContextCpu());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymContextGpu());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymContextInline());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymGpuGlobalId());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymGpuLocalId());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymGpuWorkgroupId());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymGpuWorkgroupSize());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymGpuGlobalSize());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymGpuNumWorkgroups());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymGpuLinearId());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymGpuBarrier());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymGpuMemoryBarrier());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::ExecutionDomain, BuiltinSymGpuWorkgroupBarrier());

  // CancelToken
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::CancelToken, BuiltinSymCancelTokenNew());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::CancelToken, BuiltinSymCancelTokenActiveCancel());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::CancelToken, BuiltinSymCancelTokenActiveIsCancelled());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::CancelToken, BuiltinSymCancelTokenActiveChild());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::CancelToken, BuiltinSymCancelTokenActiveWaitCancelled());

  // Async
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Async, BuiltinSymAsyncResume());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Async, BuiltinSymAsyncGetDiscriminant());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Async, BuiltinSymAsyncGetSuspendedOutput());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Async, BuiltinSymAsyncGetCompletedValue());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Async, BuiltinSymAsyncGetFailedError());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Async, BuiltinSymAsyncCreateCompleted());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Async, BuiltinSymAsyncCreateFailed());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Async, BuiltinSymAsyncCreateSuspended());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Async, BuiltinSymAsyncAllocFrame());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Async, BuiltinSymAsyncFreeFrame());

  // Reactor
  // Note: Reactor does not currently have a dedicated RuntimeSymbolCategory.
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Unknown, BuiltinSymReactorRun());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Unknown, BuiltinSymReactorRegister());

  // Structured concurrency runtime
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymParallelBegin());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymParallelJoin());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymSpawnCreate());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymSpawnWait());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymDispatchRun());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymCancelTokenNew());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymCancelTokenCancel());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymCancelTokenIsCancelled());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymParallelWorkPanic());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymKeyScopeEnter());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymKeyScopeExit());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymKeyCheckConflict());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymKeyAcquire());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymKeyReleaseOne());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymKeyReleaseAll());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymKeyReacquireOne());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymKeyReacquire());
  AddRuntimeSymbol(categories, RuntimeSymbolCategory::Concurrency, ConcurrencySymKeyReleaseSnapshotDiscard());

  return categories;
}

const RuntimeCategoryMap& GetRuntimeCategoryMap() {
  static const RuntimeCategoryMap categories = BuildRuntimeCategoryMap();
  return categories;
}

}  // namespace

RuntimeSymbolCategory CategorizeRuntimeSymbol(const std::string& symbol) {
  const auto& categories = GetRuntimeCategoryMap();
  if (const auto it = categories.find(symbol); it != categories.end()) {
    return it->second;
  }

  // Preserve prefix-based fallback categorization for runtime symbols not
  // explicitly registered in the map.
  if (symbol.rfind(kConcurrencyPrefix, 0) == 0) {
    return RuntimeSymbolCategory::Concurrency;
  }
  if (symbol.rfind(kRuntimePrefix, 0) == 0) {
    std::string suffix = symbol.substr(std::string(kRuntimePrefix).length());
    if (suffix.rfind("panic", 0) == 0 || suffix.rfind("context", 0) == 0) {
      return RuntimeSymbolCategory::Core;
    }
    if (suffix.rfind("region", 0) == 0) {
      return RuntimeSymbolCategory::Region;
    }
    if (suffix.rfind("string", 0) == 0) {
      return RuntimeSymbolCategory::String;
    }
    if (suffix.rfind("bytes", 0) == 0) {
      return RuntimeSymbolCategory::Bytes;
    }
    if (suffix.rfind("fs", 0) == 0) {
      return RuntimeSymbolCategory::FileSystem;
    }
    if (suffix.rfind("net", 0) == 0) {
      return RuntimeSymbolCategory::Network;
    }
    if (suffix.rfind("heap", 0) == 0) {
      return RuntimeSymbolCategory::HeapAllocator;
    }
    if (suffix.rfind("system", 0) == 0) {
      return RuntimeSymbolCategory::System;
    }
    if (suffix.rfind("execution_domain", 0) == 0) {
      return RuntimeSymbolCategory::ExecutionDomain;
    }
    if (suffix.rfind("async", 0) == 0) {
      return RuntimeSymbolCategory::Async;
    }
    if (suffix.rfind("conformance", 0) == 0) {
      return RuntimeSymbolCategory::Conformance;
    }
  }

  return RuntimeSymbolCategory::Unknown;
}

// =============================================================================
// Runtime Function Lookup
// =============================================================================

namespace {

/// Static set of known runtime function symbols.
std::unordered_set<std::string> BuildRuntimeSymbolSet() {
  std::unordered_set<std::string> symbols;
  const auto& categories = GetRuntimeCategoryMap();
  for (const auto& [symbol, _category] : categories) {
    symbols.insert(symbol);
  }
  return symbols;
}

const std::unordered_set<std::string>& GetRuntimeSymbolSet() {
  static const std::unordered_set<std::string> symbols = BuildRuntimeSymbolSet();
  return symbols;
}

}  // namespace

bool IsRuntimeFunction(const std::string& symbol) {
  return GetRuntimeSymbolSet().count(symbol) > 0;
}

std::optional<RuntimeFuncInfo> GetRuntimeFuncInfo(const std::string& symbol) {
  if (!IsRuntimeFunction(symbol)) {
    return std::nullopt;
  }

  auto make_param = [](std::string name,
                       const analysis::TypeRef& type,
                       std::optional<analysis::ParamMode> mode = std::nullopt) {
    IRParam param;
    param.name = std::move(name);
    param.mode = mode;
    param.type = type;
    return param;
  };

  auto make_union2 = [](analysis::TypeRef lhs, analysis::TypeRef rhs) {
    std::vector<analysis::TypeRef> members;
    members.reserve(2);
    members.push_back(std::move(lhs));
    members.push_back(std::move(rhs));
    return analysis::MakeTypeUnion(std::move(members));
  };
  auto make_union3 = [](analysis::TypeRef lhs,
                        analysis::TypeRef mid,
                        analysis::TypeRef rhs) {
    std::vector<analysis::TypeRef> members;
    members.reserve(3);
    members.push_back(std::move(lhs));
    members.push_back(std::move(mid));
    members.push_back(std::move(rhs));
    return analysis::MakeTypeUnion(std::move(members));
  };

  const analysis::TypeRef t_unit = analysis::MakeTypePrim("()");
  const analysis::TypeRef t_never = analysis::MakeTypePrim("!");
  const analysis::TypeRef t_i32 = analysis::MakeTypePrim("i32");
  const analysis::TypeRef t_u32 = analysis::MakeTypePrim("u32");
  const analysis::TypeRef t_u64 = analysis::MakeTypePrim("u64");
  const analysis::TypeRef t_u8 = analysis::MakeTypePrim("u8");
  const analysis::TypeRef t_bool = analysis::MakeTypePrim("bool");
  const analysis::TypeRef t_f64 = analysis::MakeTypePrim("f64");
  const analysis::TypeRef t_usize = analysis::MakeTypePrim("usize");
  const analysis::TypeRef t_usize3 =
      analysis::MakeTypeTuple({t_usize, t_usize, t_usize});
  const analysis::TypeRef t_raw_imm_u8 =
      analysis::MakeTypeRawPtr(analysis::RawPtrQual::Imm, t_u8);
  const analysis::TypeRef t_string_view =
      analysis::MakeTypeString(analysis::StringState::View);
  const analysis::TypeRef t_string_managed =
      analysis::MakeTypeString(analysis::StringState::Managed);
  const analysis::TypeRef t_bytes_view =
      analysis::MakeTypeBytes(analysis::BytesState::View);
  const analysis::TypeRef t_bytes_managed =
      analysis::MakeTypeBytes(analysis::BytesState::Managed);
  const analysis::TypeRef t_slice_u8 = analysis::MakeTypeSlice(t_u8);
  const analysis::TypeRef t_dispatch_range =
      analysis::MakeTypeTuple({t_u8, t_u64, t_u64});
  const analysis::TypeRef t_alloc_err = analysis::MakeTypePath({"AllocationError"});
  const analysis::TypeRef t_execution_domain =
      analysis::MakeTypeDynamic({"ExecutionDomain"});
  const analysis::TypeRef t_file_system =
      analysis::MakeTypeDynamic({"FileSystem"});
  const analysis::TypeRef t_network = analysis::MakeTypeDynamic({"Network"});
  const analysis::TypeRef t_heap_alloc = analysis::MakeTypeDynamic({"HeapAllocator"});
  const analysis::TypeRef t_context = analysis::MakeTypePath({"Context"});
  const analysis::TypeRef t_io_error = analysis::MakeTypePath({"IoError"});
  const analysis::TypeRef t_dir_entry = analysis::MakeTypePath({"DirEntry"});
  const analysis::TypeRef t_file_read =
      analysis::MakeTypeModalState({"File"}, "Read");
  const analysis::TypeRef t_file_write =
      analysis::MakeTypeModalState({"File"}, "Write");
  const analysis::TypeRef t_file_append =
      analysis::MakeTypeModalState({"File"}, "Append");
  const analysis::TypeRef t_file_closed =
      analysis::MakeTypeModalState({"File"}, "Closed");
  const analysis::TypeRef t_dir_iter_open =
      analysis::MakeTypeModalState({"DirIter"}, "Open");
  const analysis::TypeRef t_dir_iter_closed =
      analysis::MakeTypeModalState({"DirIter"}, "Closed");
  const analysis::TypeRef t_region_options = analysis::MakeTypePath({"RegionOptions"});
  const analysis::TypeRef t_region_active =
      analysis::MakeTypeModalState({"Region"}, "Active");
  const analysis::TypeRef t_region_frozen =
      analysis::MakeTypeModalState({"Region"}, "Frozen");
  const analysis::TypeRef t_region_freed =
      analysis::MakeTypeModalState({"Region"}, "Freed");
  const analysis::TypeRef t_cancel_token_active =
      analysis::MakeTypeModalState({"CancelToken"}, "Active");
  const analysis::TypeRef t_async_unit =
      analysis::MakeTypePath({"Async"}, {t_unit});
  const analysis::TypeRef t_async_resume =
      analysis::MakeTypeTuple({
          t_u8,
          analysis::MakeTypeArray(t_u8, 7),
          analysis::MakeTypeArray(t_u8, 16),
      });
  const analysis::TypeRef t_raw_mut_u8 =
      analysis::MakeTypeRawPtr(analysis::RawPtrQual::Mut, t_u8);
  const analysis::TypeRef t_const_string_view =
      analysis::MakeTypePerm(analysis::Permission::Const, t_string_view);
  const analysis::TypeRef t_const_string_managed =
      analysis::MakeTypePerm(analysis::Permission::Const, t_string_managed);
  const analysis::TypeRef t_const_bytes_view =
      analysis::MakeTypePerm(analysis::Permission::Const, t_bytes_view);
  const analysis::TypeRef t_const_bytes_managed =
      analysis::MakeTypePerm(analysis::Permission::Const, t_bytes_managed);
  const analysis::TypeRef t_const_slice_u8 =
      analysis::MakeTypePerm(analysis::Permission::Const, t_slice_u8);
  const analysis::TypeRef t_unique_string_managed =
      analysis::MakeTypePerm(analysis::Permission::Unique, t_string_managed);
  const analysis::TypeRef t_unique_bytes_managed =
      analysis::MakeTypePerm(analysis::Permission::Unique, t_bytes_managed);

  RuntimeFuncInfo info;
  info.symbol = symbol;
  info.abi = "C";
  auto add_trace_span_params = [&](RuntimeFuncInfo& target) {
    target.params.push_back(make_param("file", t_const_string_view));
    target.params.push_back(
        make_param("start_line", t_u64, analysis::ParamMode::Move));
    target.params.push_back(
        make_param("start_col", t_u64, analysis::ParamMode::Move));
    target.params.push_back(
        make_param("end_line", t_u64, analysis::ParamMode::Move));
    target.params.push_back(
        make_param("end_col", t_u64, analysis::ParamMode::Move));
  };

  // Core runtime
  if (symbol == RuntimePanicSym()) {
    info.noreturn = true;
    info.params.push_back(make_param("code", t_u32, analysis::ParamMode::Move));
    info.ret = t_never;
    return info;
  }
  if (symbol == ContextInitSym()) {
    info.ret = t_context;
    return info;
  }
  if (symbol == RuntimeConformanceEmitSym()) {
    info.params.push_back(make_param("rule_id", t_const_string_view));
    add_trace_span_params(info);
    info.params.push_back(make_param("payload", t_const_string_view));
    info.ret = t_unit;
    return info;
  }
  if (symbol == RuntimeConformanceEmitIntSym()) {
    info.params.push_back(make_param("rule_id", t_const_string_view));
    add_trace_span_params(info);
    info.params.push_back(make_param("payload_prefix", t_const_string_view));
    info.params.push_back(make_param("raw", t_u64, analysis::ParamMode::Move));
    info.params.push_back(make_param("bits", t_u8, analysis::ParamMode::Move));
    info.params.push_back(make_param("is_signed", t_bool, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == RuntimeConformanceEmitBoolSym()) {
    info.params.push_back(make_param("rule_id", t_const_string_view));
    add_trace_span_params(info);
    info.params.push_back(make_param("payload_prefix", t_const_string_view));
    info.params.push_back(make_param("actual", t_bool, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == RuntimeConformanceEmitFloatSym()) {
    info.params.push_back(make_param("rule_id", t_const_string_view));
    add_trace_span_params(info);
    info.params.push_back(make_param("payload_prefix", t_const_string_view));
    info.params.push_back(make_param("actual", t_f64, analysis::ParamMode::Move));
    info.params.push_back(make_param("bits", t_u8, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == RuntimeConformanceEmitPtrSym()) {
    info.params.push_back(make_param("rule_id", t_const_string_view));
    add_trace_span_params(info);
    info.params.push_back(make_param("payload_prefix", t_const_string_view));
    info.params.push_back(make_param("actual", t_raw_imm_u8, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == RuntimeConformanceEmitStringSym()) {
    info.params.push_back(make_param("rule_id", t_const_string_view));
    add_trace_span_params(info);
    info.params.push_back(make_param("payload_prefix", t_const_string_view));
    info.params.push_back(make_param("actual", t_const_string_view));
    info.ret = t_unit;
    return info;
  }
  if (symbol == RuntimeConformanceEmitStringManagedSym()) {
    info.params.push_back(make_param("rule_id", t_const_string_view));
    add_trace_span_params(info);
    info.params.push_back(make_param("payload_prefix", t_const_string_view));
    info.params.push_back(make_param("actual", t_const_string_managed));
    info.ret = t_unit;
    return info;
  }
  if (symbol == RuntimeConformanceEmitBytesSym()) {
    info.params.push_back(make_param("rule_id", t_const_string_view));
    add_trace_span_params(info);
    info.params.push_back(make_param("payload_prefix", t_const_string_view));
    info.params.push_back(make_param("actual", t_const_bytes_view));
    info.ret = t_unit;
    return info;
  }
  if (symbol == RuntimeConformanceEmitBytesManagedSym()) {
    info.params.push_back(make_param("rule_id", t_const_string_view));
    add_trace_span_params(info);
    info.params.push_back(make_param("payload_prefix", t_const_string_view));
    info.params.push_back(make_param("actual", t_const_bytes_managed));
    info.ret = t_unit;
    return info;
  }
  if (symbol == RuntimeConformanceSetSinkSym()) {
    info.params.push_back(make_param("sink_kind", t_u8, analysis::ParamMode::Move));
    info.params.push_back(make_param("path", t_raw_imm_u8, analysis::ParamMode::Move));
    info.params.push_back(make_param("path_len", t_u64, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == RuntimeConformanceSetRootSym()) {
    info.params.push_back(make_param("path", t_raw_imm_u8, analysis::ParamMode::Move));
    info.params.push_back(make_param("path_len", t_u64, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == RuntimeConformanceSetLogFilterSym()) {
    info.params.push_back(make_param("mask", t_u8, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == RuntimeConformanceSetMinLevelSym()) {
    info.params.push_back(make_param("level", t_u8, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }

  // FileSystem methods.
  // These are C ABI hooks and must carry concrete runtime signatures so
  // aggregate return lowering (including hidden sret when required by the
  // platform ABI) is applied correctly.
  if (symbol == BuiltinSymFileSystemOpenRead()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = make_union2(t_file_read, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemOpenWrite()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = make_union2(t_file_write, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemOpenAppend()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = make_union2(t_file_append, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemCreateWrite()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = make_union2(t_file_write, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemReadFile()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = make_union2(t_string_managed, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemReadBytes()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = make_union2(t_bytes_managed, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemWriteFile()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.params.push_back(make_param("data", t_bytes_view));
    info.ret = make_union2(t_unit, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemWriteStdout()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("data", t_string_view));
    info.ret = make_union2(t_unit, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemWriteStderr()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("data", t_string_view));
    info.ret = make_union2(t_unit, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemExists()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = t_bool;
    return info;
  }
  if (symbol == BuiltinSymFileSystemRemove()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = make_union2(t_unit, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemOpenDir()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = make_union2(t_dir_iter_open, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemCreateDir()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = make_union2(t_unit, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemEnsureDir()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = make_union2(t_unit, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemKind()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = make_union2(analysis::MakeTypePath({"FileKind"}), t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileSystemRestrict()) {
    info.params.push_back(make_param("self", t_file_system));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = t_file_system;
    return info;
  }

  if (symbol == BuiltinSymNetworkRestrictHost()) {
    info.params.push_back(make_param("self", t_network));
    info.params.push_back(make_param("host", t_string_view));
    info.ret = t_network;
    return info;
  }

  // File/DirIter modal methods.
  if (symbol == BuiltinSymFileReadReadAll()) {
    info.params.push_back(make_param("self", t_file_read));
    info.ret = make_union2(t_string_managed, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileReadReadAllBytes()) {
    info.params.push_back(make_param("self", t_file_read));
    info.ret = make_union2(t_bytes_managed, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileWriteWrite()) {
    info.params.push_back(make_param("self", t_file_write));
    info.params.push_back(make_param("data", t_bytes_view));
    info.ret = make_union2(t_unit, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileWriteFlush()) {
    info.params.push_back(make_param("self", t_file_write));
    info.ret = make_union2(t_unit, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileWriteClose()) {
    // `close` is a transition; receiver is move self per TransitionParams.
    info.params.push_back(
        make_param("self", t_file_write, analysis::ParamMode::Move));
    info.ret = t_file_closed;
    return info;
  }
  if (symbol == BuiltinSymFileAppendWrite()) {
    info.params.push_back(make_param("self", t_file_append));
    info.params.push_back(make_param("data", t_bytes_view));
    info.ret = make_union2(t_unit, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileAppendFlush()) {
    info.params.push_back(make_param("self", t_file_append));
    info.ret = make_union2(t_unit, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymFileAppendClose()) {
    // `close` is a transition; receiver is move self per TransitionParams.
    info.params.push_back(
        make_param("self", t_file_append, analysis::ParamMode::Move));
    info.ret = t_file_closed;
    return info;
  }
  if (symbol == BuiltinSymFileReadClose()) {
    // `close` is a transition; receiver is move self per TransitionParams.
    info.params.push_back(
        make_param("self", t_file_read, analysis::ParamMode::Move));
    info.ret = t_file_closed;
    return info;
  }
  if (symbol == BuiltinSymDirIterOpenNext()) {
    info.params.push_back(make_param("self", t_dir_iter_open));
    info.ret = make_union3(t_dir_entry, t_unit, t_io_error);
    return info;
  }
  if (symbol == BuiltinSymDirIterOpenClose()) {
    // `close` is a transition; receiver is move self per TransitionParams.
    info.params.push_back(
        make_param("self", t_dir_iter_open, analysis::ParamMode::Move));
    info.ret = t_dir_iter_closed;
    return info;
  }

  // HeapAllocator methods.
  // These are C ABI hooks and must carry concrete runtime signatures so
  // aggregate return lowering (including hidden sret when required by the
  // platform ABI) is applied correctly.
  if (symbol == BuiltinSymHeapAllocatorWithQuota()) {
    info.params.push_back(make_param("self", t_heap_alloc));
    info.params.push_back(make_param("size", t_usize));
    info.ret = t_heap_alloc;
    return info;
  }
  if (symbol == BuiltinSymHeapAllocatorAllocRaw()) {
    info.params.push_back(make_param("self", t_heap_alloc));
    info.params.push_back(make_param("count", t_usize));
    info.ret = t_raw_mut_u8;
    return info;
  }
  if (symbol == BuiltinSymHeapAllocatorDeallocRaw()) {
    info.params.push_back(make_param("self", t_heap_alloc));
    info.params.push_back(make_param("ptr", t_raw_mut_u8));
    info.params.push_back(make_param("count", t_usize));
    info.ret = t_unit;
    return info;
  }

  // System methods.
  if (symbol == BuiltinSymSystemExit()) {
    info.noreturn = true;
    info.params.push_back(make_param("code", t_i32, analysis::ParamMode::Move));
    info.ret = t_never;
    return info;
  }
  if (symbol == BuiltinSymSystemGetEnv()) {
    info.params.push_back(make_param("key", t_string_view));
    info.ret = t_string_view;
    return info;
  }
  if (symbol == BuiltinSymSystemRun()) {
    info.params.push_back(make_param("command", t_string_view));
    info.ret = t_i32;
    return info;
  }

  // ExecutionDomain and Context builtins.
  if (symbol == BuiltinSymExecutionDomainName()) {
    info.params.push_back(make_param("self", t_execution_domain));
    info.ret = t_string_view;
    return info;
  }
  if (symbol == BuiltinSymExecutionDomainMaxConcurrency()) {
    info.params.push_back(make_param("self", t_execution_domain));
    info.ret = t_usize;
    return info;
  }
  if (symbol == BuiltinSymContextCpu() || symbol == BuiltinSymContextGpu() ||
      symbol == BuiltinSymContextInline()) {
    info.params.push_back(make_param("self", t_context));
    info.ret = t_execution_domain;
    return info;
  }

  // Region methods
  if (symbol == BuiltinModalSymRegionNewScoped()) {
    info.params.push_back(make_param("options", t_region_options));
    info.ret = t_region_active;
    return info;
  }
  if (symbol == BuiltinModalSymRegionAlloc()) {
    info.params.push_back(make_param("self", t_region_active));
    info.params.push_back(make_param("size", t_usize, analysis::ParamMode::Move));
    info.params.push_back(make_param("align", t_usize, analysis::ParamMode::Move));
    info.ret = t_raw_mut_u8;
    return info;
  }
  if (symbol == BuiltinModalSymRegionMark()) {
    info.params.push_back(make_param("self", t_region_active));
    info.ret = t_usize;
    return info;
  }
  if (symbol == BuiltinModalSymRegionResetTo()) {
    info.params.push_back(make_param("self", t_region_active));
    info.params.push_back(make_param("mark", t_usize, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == BuiltinModalSymRegionResetUnchecked()) {
    info.params.push_back(make_param("self", t_region_active));
    info.ret = t_region_active;
    return info;
  }
  if (symbol == BuiltinModalSymRegionFreeze()) {
    info.params.push_back(make_param("self", t_region_active));
    info.ret = t_region_frozen;
    return info;
  }
  if (symbol == BuiltinModalSymRegionThaw()) {
    info.params.push_back(make_param("self", t_region_frozen));
    info.ret = t_region_active;
    return info;
  }
  if (symbol == BuiltinModalSymRegionFreeUnchecked()) {
    info.params.push_back(make_param("self", t_region_active));
    info.ret = t_region_freed;
    return info;
  }

  // Region runtime helpers used by cleanup/provenance instrumentation.
  // These are C ABI hooks and use by-value integers/raw pointers.
  if (symbol == BuiltinModalSymRegionScopeEnter()) {
    info.params.push_back(
        make_param("scope_id", t_usize, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == BuiltinModalSymRegionScopeExit()) {
    info.params.push_back(
        make_param("scope_id", t_usize, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == BuiltinModalSymRegionAddrTagScope()) {
    info.params.push_back(
        make_param("addr", t_raw_imm_u8, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("scope_id", t_usize, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == BuiltinModalSymRegionAddrIsActive()) {
    info.params.push_back(
        make_param("addr", t_raw_imm_u8, analysis::ParamMode::Move));
    info.ret = t_bool;
    return info;
  }
  if (symbol == BuiltinModalSymRegionAddrTagFrom()) {
    info.params.push_back(
        make_param("addr", t_raw_imm_u8, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("base", t_raw_imm_u8, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }


  // GPU intrinsics
  if (symbol == BuiltinSymGpuGlobalId() || symbol == BuiltinSymGpuLocalId() ||
      symbol == BuiltinSymGpuWorkgroupId() ||
      symbol == BuiltinSymGpuWorkgroupSize() ||
      symbol == BuiltinSymGpuGlobalSize() ||
      symbol == BuiltinSymGpuNumWorkgroups()) {
    info.ret = t_usize3;
    return info;
  }
  if (symbol == BuiltinSymGpuLinearId()) {
    info.ret = t_usize;
    return info;
  }
  if (symbol == BuiltinSymGpuBarrier() ||
      symbol == BuiltinSymGpuMemoryBarrier() ||
      symbol == BuiltinSymGpuWorkgroupBarrier()) {
    info.ret = t_unit;
    return info;
  }

  // CancelToken builtins and modal methods.
  if (symbol == BuiltinSymCancelTokenNew()) {
    info.ret = t_cancel_token_active;
    return info;
  }
  if (symbol == BuiltinSymCancelTokenActiveCancel()) {
    info.params.push_back(make_param("self", t_cancel_token_active));
    info.ret = t_unit;
    return info;
  }
  if (symbol == BuiltinSymCancelTokenActiveIsCancelled()) {
    info.params.push_back(make_param("self", t_cancel_token_active));
    info.ret = t_bool;
    return info;
  }
  if (symbol == BuiltinSymCancelTokenActiveChild()) {
    info.params.push_back(make_param("self", t_cancel_token_active));
    info.ret = t_cancel_token_active;
    return info;
  }
  if (symbol == BuiltinSymCancelTokenActiveWaitCancelled()) {
    info.params.push_back(make_param("self", t_cancel_token_active));
    info.ret = t_async_unit;
    return info;
  }

  // Async runtime helpers.
  if (symbol == BuiltinSymAsyncResume()) {
    info.params.push_back(
        make_param("suspended", t_raw_imm_u8, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("input", t_raw_imm_u8, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("panic_out", t_raw_mut_u8, analysis::ParamMode::Move));
    info.ret = t_async_resume;
    return info;
  }
  if (symbol == BuiltinSymAsyncAllocFrame()) {
    info.params.push_back(
        make_param("size", t_u64, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("align", t_u64, analysis::ParamMode::Move));
    info.ret = t_raw_mut_u8;
    return info;
  }
  if (symbol == BuiltinSymAsyncFreeFrame()) {
    info.params.push_back(
        make_param("frame", t_raw_mut_u8, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }

  // Internal drop hooks
  if (symbol == BuiltinSymStringDropManaged()) {
    info.params.push_back(
        make_param("value", t_string_managed, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == BuiltinSymBytesDropManaged()) {
    info.params.push_back(
        make_param("value", t_bytes_managed, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }

  // string builtins
  if (symbol == BuiltinSymStringFrom()) {
    info.params.push_back(make_param("source", t_string_view));
    info.params.push_back(make_param("heap", t_heap_alloc));
    info.ret = make_union2(t_string_managed, t_alloc_err);
    return info;
  }
  if (symbol == BuiltinSymStringAsView()) {
    info.params.push_back(make_param("self", t_const_string_managed));
    info.ret = t_string_view;
    return info;
  }
  if (symbol == BuiltinSymStringToManaged()) {
    info.params.push_back(make_param("self", t_const_string_view));
    info.params.push_back(make_param("heap", t_heap_alloc));
    info.ret = make_union2(t_string_managed, t_alloc_err);
    return info;
  }
  if (symbol == BuiltinSymStringCloneWith()) {
    info.params.push_back(make_param("self", t_const_string_managed));
    info.params.push_back(make_param("heap", t_heap_alloc));
    info.ret = make_union2(t_string_managed, t_alloc_err);
    return info;
  }
  if (symbol == BuiltinSymStringAppend()) {
    info.params.push_back(make_param("self", t_unique_string_managed));
    info.params.push_back(make_param("data", t_string_view));
    info.params.push_back(make_param("heap", t_heap_alloc));
    info.ret = make_union2(t_unit, t_alloc_err);
    return info;
  }
  if (symbol == BuiltinSymStringLength()) {
    info.params.push_back(make_param("self", t_const_string_view));
    info.ret = t_usize;
    return info;
  }
  if (symbol == BuiltinSymStringIsEmpty()) {
    info.params.push_back(make_param("self", t_const_string_view));
    info.ret = t_bool;
    return info;
  }

  // bytes builtins
  if (symbol == BuiltinSymBytesWithCapacity()) {
    info.params.push_back(make_param("cap", t_usize));
    info.params.push_back(make_param("heap", t_heap_alloc));
    info.ret = make_union2(t_bytes_managed, t_alloc_err);
    return info;
  }
  if (symbol == BuiltinSymBytesFromSlice()) {
    info.params.push_back(make_param("data", t_const_slice_u8));
    info.params.push_back(make_param("heap", t_heap_alloc));
    info.ret = make_union2(t_bytes_managed, t_alloc_err);
    return info;
  }
  if (symbol == BuiltinSymBytesAsView()) {
    info.params.push_back(make_param("self", t_const_bytes_managed));
    info.ret = t_bytes_view;
    return info;
  }
  if (symbol == BuiltinSymBytesAsSlice()) {
    info.params.push_back(make_param("self", t_const_bytes_view));
    info.ret = t_const_slice_u8;
    return info;
  }
  if (symbol == BuiltinSymBytesToManaged()) {
    info.params.push_back(make_param("self", t_const_bytes_view));
    info.params.push_back(make_param("heap", t_heap_alloc));
    info.ret = make_union2(t_bytes_managed, t_alloc_err);
    return info;
  }
  if (symbol == BuiltinSymBytesView()) {
    info.params.push_back(make_param("data", t_const_slice_u8));
    info.ret = t_bytes_view;
    return info;
  }
  if (symbol == BuiltinSymBytesViewString()) {
    info.params.push_back(make_param("data", t_string_view));
    info.ret = t_bytes_view;
    return info;
  }
  if (symbol == BuiltinSymBytesAppend()) {
    info.params.push_back(make_param("self", t_unique_bytes_managed));
    info.params.push_back(make_param("data", t_bytes_view));
    info.params.push_back(make_param("heap", t_heap_alloc));
    info.ret = make_union2(t_unit, t_alloc_err);
    return info;
  }
  if (symbol == BuiltinSymBytesLength()) {
    info.params.push_back(make_param("self", t_const_bytes_view));
    info.ret = t_usize;
    return info;
  }
  if (symbol == BuiltinSymBytesIsEmpty()) {
    info.params.push_back(make_param("self", t_const_bytes_view));
    info.ret = t_bool;
    return info;
  }

  // Structured concurrency runtime.
  if (symbol == ConcurrencySymParallelBegin()) {
    // C0DynObject is ABI-lowered by reference on Win64 for this call boundary.
    info.params.push_back(make_param("domain", t_execution_domain));
    info.params.push_back(
        make_param("cancel_token", t_usize, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("name", t_raw_imm_u8, analysis::ParamMode::Move));
    info.ret = t_raw_mut_u8;
    return info;
  }
  if (symbol == ConcurrencySymParallelJoin()) {
    info.params.push_back(
        make_param("ctx", t_raw_mut_u8, analysis::ParamMode::Move));
    info.ret = t_i32;
    return info;
  }
  if (symbol == ConcurrencySymSpawnCreate()) {
    info.params.push_back(
        make_param("env", t_raw_mut_u8, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("env_size", t_usize, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("body", t_raw_imm_u8, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("hosted_env", t_raw_mut_u8, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("result_size", t_usize, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("affinity_mask", t_usize, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("priority_hint", t_i32, analysis::ParamMode::Move));
    info.ret = t_raw_mut_u8;
    return info;
  }
  if (symbol == ConcurrencySymSpawnWait()) {
    info.params.push_back(
        make_param("handle", t_raw_mut_u8, analysis::ParamMode::Move));
    info.ret = t_raw_mut_u8;
    return info;
  }
  if (symbol == ConcurrencySymDispatchRun()) {
    // C0Range and C0StringView are aggregate ABI parameters at this boundary.
    info.params.push_back(make_param("range", t_dispatch_range));
    info.params.push_back(
        make_param("elem_size", t_usize, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("result_size", t_usize, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("body", t_raw_imm_u8, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("hosted_env", t_raw_mut_u8, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("captured_env", t_raw_mut_u8, analysis::ParamMode::Move));
    info.params.push_back(make_param("reduce_op", t_string_view));
    info.params.push_back(
        make_param("reduce_result", t_raw_mut_u8, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("reduce_fn", t_raw_imm_u8, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("ordered", t_i32, analysis::ParamMode::Move));
    info.params.push_back(
        make_param("chunk_size", t_usize, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == ConcurrencySymKeyScopeEnter()) {
    info.ret = t_raw_mut_u8;
    return info;
  }
  if (symbol == ConcurrencySymKeyScopeExit()) {
    info.params.push_back(
        make_param("scope", t_raw_mut_u8, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == ConcurrencySymKeyCheckConflict()) {
    info.params.push_back(make_param("path", t_string_view));
    info.params.push_back(
        make_param("mode", t_u8, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == ConcurrencySymKeyAcquire()) {
    info.params.push_back(
        make_param("scope", t_raw_mut_u8, analysis::ParamMode::Move));
    info.params.push_back(make_param("path", t_string_view));
    info.params.push_back(
        make_param("mode", t_u8, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == ConcurrencySymKeyReleaseOne()) {
    info.params.push_back(
        make_param("scope", t_raw_mut_u8, analysis::ParamMode::Move));
    info.params.push_back(make_param("path", t_string_view));
    info.ret = t_raw_mut_u8;
    return info;
  }
  if (symbol == ConcurrencySymKeyReleaseAll()) {
    info.ret = t_raw_mut_u8;
    return info;
  }
  if (symbol == ConcurrencySymKeyReacquireOne()) {
    info.params.push_back(
        make_param("released", t_raw_mut_u8, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }
  if (symbol == ConcurrencySymKeyReacquire() ||
      symbol == ConcurrencySymKeyReleaseSnapshotDiscard()) {
    info.params.push_back(
        make_param("released", t_raw_mut_u8, analysis::ParamMode::Move));
    info.ret = t_unit;
    return info;
  }

  return std::nullopt;
}

// =============================================================================
// Structured Concurrency Runtime Symbols
// =============================================================================

std::string ConcurrencySymParallelBegin() {
  return "cursive_parallel_begin";
}

std::string ConcurrencySymParallelJoin() {
  return "cursive_parallel_join";
}

std::string ConcurrencySymSpawnCreate() {
  return "cursive_spawn_create";
}

std::string ConcurrencySymSpawnWait() {
  return "cursive_spawn_wait";
}

std::string ConcurrencySymDispatchRun() {
  return "cursive_dispatch_run";
}

std::string ConcurrencySymCancelTokenNew() {
  return "cursive_cancel_token_new";
}

std::string ConcurrencySymCancelTokenCancel() {
  return "cursive_cancel_token_cancel";
}

std::string ConcurrencySymCancelTokenIsCancelled() {
  return "cursive_cancel_token_is_cancelled";
}

std::string ConcurrencySymParallelWorkPanic() {
  return "cursive_parallel_work_panic";
}

std::string ConcurrencySymKeyScopeEnter() {
  return "cursive_key_scope_enter";
}

std::string ConcurrencySymKeyScopeExit() {
  return "cursive_key_scope_exit";
}

std::string ConcurrencySymKeyCheckConflict() {
  return "cursive_key_check_conflict";
}

std::string ConcurrencySymKeyAcquire() {
  return "cursive_key_acquire";
}

std::string ConcurrencySymKeyReleaseOne() {
  return "cursive_key_release_one";
}

std::string ConcurrencySymKeyReleaseAll() {
  return "cursive_key_release_all";
}

std::string ConcurrencySymKeyReacquireOne() {
  return "cursive_key_reacquire_one";
}

std::string ConcurrencySymKeyReacquire() {
  return "cursive_key_reacquire";
}

std::string ConcurrencySymKeyReleaseSnapshotDiscard() {
  return "cursive_key_release_snapshot_discard";
}

// =============================================================================
// LLVM Intrinsic Mapping
// =============================================================================

LLVMIntrinsicKind GetLLVMIntrinsic(const std::string& op, const analysis::TypeRef& type) {
  if (!type) {
    return LLVMIntrinsicKind::None;
  }

  // Check for primitive type
  const auto* prim = std::get_if<analysis::TypePrim>(&type->node);
  if (!prim) {
    return LLVMIntrinsicKind::None;
  }

  const std::string& type_name = prim->name;

  // Float intrinsics
  if (type_name == "f32" || type_name == "f64" || type_name == "f16") {
    if (op == "sqrt") return LLVMIntrinsicKind::Sqrt;
    if (op == "pow") return LLVMIntrinsicKind::Pow;
    if (op == "abs") return LLVMIntrinsicKind::Fabs;
    if (op == "floor") return LLVMIntrinsicKind::Floor;
    if (op == "ceil") return LLVMIntrinsicKind::Ceil;
    if (op == "round") return LLVMIntrinsicKind::Round;
    if (op == "trunc") return LLVMIntrinsicKind::Trunc;
  }

  // Integer overflow intrinsics (for checked arithmetic)
  bool is_signed = type_name[0] == 'i';
  if (type_name.find_first_of("iu") == 0 && type_name.length() > 1) {
    if (op == "add_checked") {
      return is_signed ? LLVMIntrinsicKind::SAddWithOverflow
                       : LLVMIntrinsicKind::UAddWithOverflow;
    }
    if (op == "sub_checked") {
      return is_signed ? LLVMIntrinsicKind::SSubWithOverflow
                       : LLVMIntrinsicKind::USubWithOverflow;
    }
    if (op == "mul_checked") {
      return is_signed ? LLVMIntrinsicKind::SMulWithOverflow
                       : LLVMIntrinsicKind::UMulWithOverflow;
    }
  }

  // Memory intrinsics are handled separately
  if (op == "memcpy") return LLVMIntrinsicKind::Memcpy;
  if (op == "memmove") return LLVMIntrinsicKind::Memmove;
  if (op == "memset") return LLVMIntrinsicKind::Memset;

  return LLVMIntrinsicKind::None;
}

std::string GetLLVMIntrinsicName(LLVMIntrinsicKind kind, const analysis::TypeRef& type) {
  std::string suffix;
  if (type) {
    if (const auto* prim = std::get_if<analysis::TypePrim>(&type->node)) {
      suffix = "." + prim->name;
    }
  }

  switch (kind) {
    case LLVMIntrinsicKind::None:
      return "";
    case LLVMIntrinsicKind::SAddWithOverflow:
      return "llvm.sadd.with.overflow" + suffix;
    case LLVMIntrinsicKind::UAddWithOverflow:
      return "llvm.uadd.with.overflow" + suffix;
    case LLVMIntrinsicKind::SSubWithOverflow:
      return "llvm.ssub.with.overflow" + suffix;
    case LLVMIntrinsicKind::USubWithOverflow:
      return "llvm.usub.with.overflow" + suffix;
    case LLVMIntrinsicKind::SMulWithOverflow:
      return "llvm.smul.with.overflow" + suffix;
    case LLVMIntrinsicKind::UMulWithOverflow:
      return "llvm.umul.with.overflow" + suffix;
    case LLVMIntrinsicKind::Memcpy:
      return "llvm.memcpy";
    case LLVMIntrinsicKind::Memmove:
      return "llvm.memmove";
    case LLVMIntrinsicKind::Memset:
      return "llvm.memset";
    case LLVMIntrinsicKind::Sqrt:
      return "llvm.sqrt" + suffix;
    case LLVMIntrinsicKind::Pow:
      return "llvm.pow" + suffix;
    case LLVMIntrinsicKind::Fabs:
      return "llvm.fabs" + suffix;
    case LLVMIntrinsicKind::Floor:
      return "llvm.floor" + suffix;
    case LLVMIntrinsicKind::Ceil:
      return "llvm.ceil" + suffix;
    case LLVMIntrinsicKind::Round:
      return "llvm.round" + suffix;
    case LLVMIntrinsicKind::Trunc:
      return "llvm.trunc" + suffix;
    case LLVMIntrinsicKind::AtomicRMW:
      return "llvm.atomicrmw";
    case LLVMIntrinsicKind::CmpXchg:
      return "llvm.cmpxchg";
  }

  return "";
}

// =============================================================================
// Spec Rule Anchors
// =============================================================================

void AnchorRuntimeInterfaceRules() {
  SPEC_RULE("RuntimeDecls");
  SPEC_RULE("RuntimeDeclsCover");
  SPEC_RULE("RuntimeDeclsOk");
  SPEC_RULE("DeclAttrsOk");
  SPEC_RULE("RuntimeSym-Panic");
}

}  // namespace cursive::codegen



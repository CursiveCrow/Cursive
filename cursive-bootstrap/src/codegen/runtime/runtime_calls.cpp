#include "cursive0/runtime/runtime_interface.h"

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"

namespace cursive0::codegen {

// ============================================================================
// §6.9 RegionLayout
// ============================================================================

RegionLayoutInfo RegionLayout() {
  SPEC_RULE("RegionLayout");
  
  // Per §6.9 (RegionLayout):
  // ModalLayout(Region) ⇓ ⟨size, align, disc, payload⟩
  // RegionLayout ⇓ ⟨size, align, [⟨"disc", disc⟩, ⟨"payload", payload⟩]⟩
  //
  // Region is a modal type with a discriminant and payload.
  // The exact layout depends on the target; for Win64 x86_64-pc-windows-msvc:
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

// ============================================================================
// §6.9 RegionSym - Region method symbols
// ============================================================================

std::string RegionSymNewScoped() {
  SPEC_RULE("RegionSym-NewScoped");
  return core::PathSig({"cursive", "runtime", "region", "new_scoped"});
}

std::string RegionSymAlloc() {
  SPEC_RULE("RegionSym-Alloc");
  return core::PathSig({"cursive", "runtime", "region", "alloc"});
}

std::string RegionSymMark() {
  SPEC_RULE("RegionSym-Mark");
  return core::PathSig({"cursive", "runtime", "region", "mark"});
}

std::string RegionSymResetTo() {
  SPEC_RULE("RegionSym-ResetTo");
  return core::PathSig({"cursive", "runtime", "region", "reset_to"});
}

std::string RegionSymResetUnchecked() {
  SPEC_RULE("RegionSym-ResetUnchecked");
  return core::PathSig({"cursive", "runtime", "region", "reset_unchecked"});
}

std::string RegionSymFreeze() {
  SPEC_RULE("RegionSym-Freeze");
  return core::PathSig({"cursive", "runtime", "region", "freeze"});
}

std::string RegionSymThaw() {
  SPEC_RULE("RegionSym-Thaw");
  return core::PathSig({"cursive", "runtime", "region", "thaw"});
}

std::string RegionSymFreeUnchecked() {
  SPEC_RULE("RegionSym-FreeUnchecked");
  return core::PathSig({"cursive", "runtime", "region", "free_unchecked"});
}

std::string RegionSymAddrIsActive() {
  SPEC_RULE("RegionSym-AddrIsActive");
  return core::PathSig({"cursive", "runtime", "region", "addr_is_active"});
}

std::string RegionSymAddrTagFrom() {
  SPEC_RULE("RegionSym-AddrTagFrom");
  return core::PathSig({"cursive", "runtime", "region", "addr_tag_from"});
}

std::string RegionSym(const std::string& method) {
  SPEC_DEF("RegionSym", "");
  
  if (method == "new_scoped") return RegionSymNewScoped();
  if (method == "alloc") return RegionSymAlloc();
  if (method == "mark") return RegionSymMark();
  if (method == "reset_to") return RegionSymResetTo();
  if (method == "reset_unchecked") return RegionSymResetUnchecked();
  if (method == "freeze") return RegionSymFreeze();
  if (method == "thaw") return RegionSymThaw();
  if (method == "free_unchecked") return RegionSymFreeUnchecked();
  
  // Unknown method - return empty string
  return "";
}

// ============================================================================
// §6.9 BuiltinSym - FileSystem capability methods
// ============================================================================

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

// ============================================================================
// §6.9 BuiltinSym - HeapAllocator capability methods
// ============================================================================

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

// ============================================================================
// §18.2 ExecutionDomain builtins
// ============================================================================

std::string BuiltinSymExecutionDomainName() {
  SPEC_DEF("BuiltinSym-ExecutionDomain-Name", "§18.2.4");
  return core::PathSig({"cursive", "runtime", "execution_domain", "name"});
}

std::string BuiltinSymExecutionDomainMaxConcurrency() {
  SPEC_DEF("BuiltinSym-ExecutionDomain-MaxConcurrency", "§18.2.4");
  return core::PathSig({"cursive", "runtime", "execution_domain", "max_concurrency"});
}

// ============================================================================
// §18.2 Context execution domain constructors
// ============================================================================

std::string BuiltinSymContextCpu() {
  SPEC_DEF("BuiltinSym-Context-Cpu", "§18.2.1");
  return core::PathSig({"cursive", "runtime", "context", "cpu"});
}

std::string BuiltinSymContextGpu() {
  SPEC_DEF("BuiltinSym-Context-Gpu", "§18.2.2");
  return core::PathSig({"cursive", "runtime", "context", "gpu"});
}

std::string BuiltinSymContextInline() {
  SPEC_DEF("BuiltinSym-Context-Inline", "§18.2.3");
  return core::PathSig({"cursive", "runtime", "context", "inline"});
}

// ============================================================================
// §18.6 CancelToken builtins
// ============================================================================

std::string BuiltinSymCancelTokenNew() {
  SPEC_DEF("BuiltinSym-CancelToken-New", "§18.6.1");
  return core::PathSig({"CancelToken", "new"});
}

// ============================================================================
// §6.12.14 String/Bytes builtin symbols
// ============================================================================

std::string BuiltinSymStringFrom() {
  SPEC_DEF("BuiltinSym-string-from", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "from"});
}

std::string BuiltinSymStringAsView() {
  SPEC_DEF("BuiltinSym-string-as_view", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "as_view"});
}

std::string BuiltinSymStringToManaged() {
  SPEC_DEF("BuiltinSym-string-to_managed", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "to_managed"});
}

std::string BuiltinSymStringCloneWith() {
  SPEC_DEF("BuiltinSym-string-clone_with", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "clone_with"});
}

std::string BuiltinSymStringAppend() {
  SPEC_DEF("BuiltinSym-string-append", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "append"});
}

std::string BuiltinSymStringLength() {
  SPEC_DEF("BuiltinSym-string-length", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "length"});
}

std::string BuiltinSymStringIsEmpty() {
  SPEC_DEF("BuiltinSym-string-is_empty", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "string", "is_empty"});
}

std::string BuiltinSymStringDropManaged() {
  SPEC_RULE("StringDropSym-Decl");
  SPEC_DEF("BuiltinSym-string-drop_managed", "??6.12.14");
  std::string sym = core::PathSig({"cursive", "runtime", "string", "drop_managed"});
  if (sym.empty()) {
    SPEC_RULE("StringDropSym-Err");
  }
  return sym;
}


std::string BuiltinSymBytesWithCapacity() {
  SPEC_DEF("BuiltinSym-bytes-with_capacity", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "with_capacity"});
}

std::string BuiltinSymBytesFromSlice() {
  SPEC_DEF("BuiltinSym-bytes-from_slice", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "from_slice"});
}

std::string BuiltinSymBytesAsView() {
  SPEC_DEF("BuiltinSym-bytes-as_view", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "as_view"});
}

std::string BuiltinSymBytesToManaged() {
  SPEC_DEF("BuiltinSym-bytes-to_managed", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "to_managed"});
}

std::string BuiltinSymBytesView() {
  SPEC_DEF("BuiltinSym-bytes-view", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "view"});
}

std::string BuiltinSymBytesViewString() {
  SPEC_DEF("BuiltinSym-bytes-view_string", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "view_string"});
}

std::string BuiltinSymBytesAppend() {
  SPEC_DEF("BuiltinSym-bytes-append", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "append"});
}

std::string BuiltinSymBytesLength() {
  SPEC_DEF("BuiltinSym-bytes-length", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "length"});
}

std::string BuiltinSymBytesIsEmpty() {
  SPEC_DEF("BuiltinSym-bytes-is_empty", "§6.12.14");
  return core::PathSig({"cursive", "runtime", "bytes", "is_empty"});
}

std::string BuiltinSymBytesDropManaged() {
  SPEC_RULE("BytesDropSym-Decl");
  SPEC_DEF("BuiltinSym-bytes-drop_managed", "??6.12.14");
  std::string sym = core::PathSig({"cursive", "runtime", "bytes", "drop_managed"});
  if (sym.empty()) {
    SPEC_RULE("BytesDropSym-Err");
  }
  return sym;
}


// ============================================================================
// §19 Reactor builtins
// ============================================================================

std::string BuiltinSymReactorRun() {
  SPEC_DEF("BuiltinSym-Reactor-Run", "§19");
  return core::PathSig({"cursive", "runtime", "reactor", "run"});
}

std::string BuiltinSymReactorRegister() {
  SPEC_DEF("BuiltinSym-Reactor-Register", "§19");
  return core::PathSig({"cursive", "runtime", "reactor", "register"});
}

// ============================================================================
// §6.8 Panic symbol
// ============================================================================

std::string RuntimePanicSym() {
  SPEC_RULE("PanicSym");
  return core::PathSig({"cursive", "runtime", "panic"});
}

// ============================================================================
// §6.12.5 Context initialization symbol
// ============================================================================

std::string ContextInitSym() {
  SPEC_RULE("ContextInitSym-Decl");
  SPEC_DEF("ContextInitSym", "§6.12.5");
  return core::PathSig({"cursive", "runtime", "context_init"});
}

// ============================================================================
// Spec trace emission symbol
// ============================================================================

std::string RuntimeSpecTraceEmitSym() {
  return core::PathSig({"cursive", "runtime", "spec_trace", "emit"});
}

// ============================================================================
// Dispatch function for BuiltinSym by qualified name
// ============================================================================

std::string BuiltinSym(const std::string& qualified_name) {
  SPEC_DEF("BuiltinSym", "");
  
  // FileSystem methods
  if (qualified_name == "FileSystem::open_read") return BuiltinSymFileSystemOpenRead();
  if (qualified_name == "FileSystem::open_write") return BuiltinSymFileSystemOpenWrite();
  if (qualified_name == "FileSystem::open_append") return BuiltinSymFileSystemOpenAppend();
  if (qualified_name == "FileSystem::create_write") return BuiltinSymFileSystemCreateWrite();
  if (qualified_name == "FileSystem::read_file") return BuiltinSymFileSystemReadFile();
  if (qualified_name == "FileSystem::read_bytes") return BuiltinSymFileSystemReadBytes();
  if (qualified_name == "FileSystem::write_file") return BuiltinSymFileSystemWriteFile();
  if (qualified_name == "FileSystem::write_stdout") return BuiltinSymFileSystemWriteStdout();
  if (qualified_name == "FileSystem::write_stderr") return BuiltinSymFileSystemWriteStderr();
  if (qualified_name == "FileSystem::exists") return BuiltinSymFileSystemExists();
  if (qualified_name == "FileSystem::remove") return BuiltinSymFileSystemRemove();
  if (qualified_name == "FileSystem::open_dir") return BuiltinSymFileSystemOpenDir();
  if (qualified_name == "FileSystem::create_dir") return BuiltinSymFileSystemCreateDir();
  if (qualified_name == "FileSystem::ensure_dir") return BuiltinSymFileSystemEnsureDir();
  if (qualified_name == "FileSystem::kind") return BuiltinSymFileSystemKind();
  if (qualified_name == "FileSystem::restrict") return BuiltinSymFileSystemRestrict();
  
  // HeapAllocator methods
  if (qualified_name == "HeapAllocator::with_quota") return BuiltinSymHeapAllocatorWithQuota();
  if (qualified_name == "HeapAllocator::alloc_raw") return BuiltinSymHeapAllocatorAllocRaw();
  if (qualified_name == "HeapAllocator::dealloc_raw") return BuiltinSymHeapAllocatorDeallocRaw();

  // ExecutionDomain methods
  if (qualified_name == "ExecutionDomain::name") return BuiltinSymExecutionDomainName();
  if (qualified_name == "ExecutionDomain::max_concurrency") return BuiltinSymExecutionDomainMaxConcurrency();

  // Context execution domain constructors
  if (qualified_name == "Context::cpu") return BuiltinSymContextCpu();
  if (qualified_name == "Context::gpu") return BuiltinSymContextGpu();
  if (qualified_name == "Context::inline") return BuiltinSymContextInline();

  // CancelToken builtins
  if (qualified_name == "CancelToken::new") return BuiltinSymCancelTokenNew();

  // Reactor builtins (§19)
  if (qualified_name == "Reactor::run") return BuiltinSymReactorRun();
  if (qualified_name == "Reactor::register") return BuiltinSymReactorRegister();
  
  // String builtins
  if (qualified_name == "string::from") return BuiltinSymStringFrom();
  if (qualified_name == "string::as_view") return BuiltinSymStringAsView();
  if (qualified_name == "string::to_managed") return BuiltinSymStringToManaged();
  if (qualified_name == "string::clone_with") return BuiltinSymStringCloneWith();
  if (qualified_name == "string::append") return BuiltinSymStringAppend();
  if (qualified_name == "string::length") return BuiltinSymStringLength();
  if (qualified_name == "string::is_empty") return BuiltinSymStringIsEmpty();
  if (qualified_name == "string::drop_managed") return BuiltinSymStringDropManaged();
  
  // Bytes builtins
  if (qualified_name == "bytes::with_capacity") return BuiltinSymBytesWithCapacity();
  if (qualified_name == "bytes::from_slice") return BuiltinSymBytesFromSlice();
  if (qualified_name == "bytes::as_view") return BuiltinSymBytesAsView();
  if (qualified_name == "bytes::to_managed") return BuiltinSymBytesToManaged();
  if (qualified_name == "bytes::view") return BuiltinSymBytesView();
  if (qualified_name == "bytes::view_string") return BuiltinSymBytesViewString();
  if (qualified_name == "bytes::append") return BuiltinSymBytesAppend();
  if (qualified_name == "bytes::length") return BuiltinSymBytesLength();
  if (qualified_name == "bytes::is_empty") return BuiltinSymBytesIsEmpty();
  if (qualified_name == "bytes::drop_managed") return BuiltinSymBytesDropManaged();
  
  if (qualified_name.rfind("string::", 0) == 0) {
    SPEC_RULE("BuiltinSym-String-Err");
    return "";
  }
  if (qualified_name.rfind("bytes::", 0) == 0) {
    SPEC_RULE("BuiltinSym-Bytes-Err");
    return "";
  }

  // Region methods (alternate qualified form)
  if (qualified_name == "Region::new_scoped") return RegionSymNewScoped();
  if (qualified_name == "Region::alloc") return RegionSymAlloc();
  if (qualified_name == "Region::mark") return RegionSymMark();
  if (qualified_name == "Region::reset_to") return RegionSymResetTo();
  if (qualified_name == "Region::reset_unchecked") return RegionSymResetUnchecked();
  if (qualified_name == "Region::freeze") return RegionSymFreeze();
  if (qualified_name == "Region::thaw") return RegionSymThaw();
  if (qualified_name == "Region::free_unchecked") return RegionSymFreeUnchecked();
  
  // Unknown builtin - return empty string
  return "";
}

// ============================================================================
// Spec Rule Anchors
// ============================================================================

void AnchorRuntimeInterfaceRules() {
  // §6.9 Runtime Interface
  SPEC_RULE("RegionLayout");
  SPEC_RULE("RegionSym-NewScoped");
  SPEC_RULE("RegionSym-Alloc");
  SPEC_RULE("RegionSym-Mark");
  SPEC_RULE("RegionSym-ResetTo");
  SPEC_RULE("RegionSym-ResetUnchecked");
  SPEC_RULE("RegionSym-Freeze");
  SPEC_RULE("RegionSym-Thaw");
  SPEC_RULE("RegionSym-FreeUnchecked");
  SPEC_RULE("RegionSym-AddrIsActive");
  SPEC_RULE("RegionSym-AddrTagFrom");
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
  SPEC_RULE("BuiltinSym-HeapAllocator-WithQuota");
  SPEC_RULE("BuiltinSym-HeapAllocator-AllocRaw");
  SPEC_RULE("BuiltinSym-HeapAllocator-DeallocRaw");
  
  // §6.8 Panic
  SPEC_RULE("PanicSym");
}

}  // namespace cursive0::codegen

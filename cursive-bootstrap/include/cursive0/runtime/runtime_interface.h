#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace cursive0::codegen {

// ============================================================================
// §6.9 Built-ins Runtime Interface
// ============================================================================
// RuntimeIfcJudg = {RegionLayout, RegionSym, BuiltinSym}

// ============================================================================
// §6.9 RegionLayout
// ============================================================================

struct RegionLayoutField {
  std::string name;
  std::uint64_t offset;
};

struct RegionLayoutInfo {
  std::uint64_t size;
  std::uint64_t align;
  std::vector<RegionLayoutField> fields;
};

// (RegionLayout)
// Returns the layout of the Region modal type
RegionLayoutInfo RegionLayout();

// ============================================================================
// §6.9 RegionSym - Region method symbols
// ============================================================================

// (RegionSym-NewScoped)
// RegionSym(Region::new_scoped) = PathSig(["cursive", "runtime", "region", "new_scoped"])
std::string RegionSymNewScoped();

// (RegionSym-Alloc)
// RegionSym(Region::alloc) = PathSig(["cursive", "runtime", "region", "alloc"])
std::string RegionSymAlloc();

// (RegionSym-Mark)
// RegionSym(Region::mark) = PathSig(["cursive", "runtime", "region", "mark"])
std::string RegionSymMark();

// (RegionSym-ResetTo)
// RegionSym(Region::reset_to) = PathSig(["cursive", "runtime", "region", "reset_to"])
std::string RegionSymResetTo();

// (RegionSym-ResetUnchecked)
// RegionSym(Region::reset_unchecked) = PathSig(["cursive", "runtime", "region", "reset_unchecked"])
std::string RegionSymResetUnchecked();

// (RegionSym-Freeze)
// RegionSym(Region::freeze) = PathSig(["cursive", "runtime", "region", "freeze"])
std::string RegionSymFreeze();

// (RegionSym-Thaw)
// RegionSym(Region::thaw) = PathSig(["cursive", "runtime", "region", "thaw"])
std::string RegionSymThaw();

// (RegionSym-FreeUnchecked)
// RegionSym(Region::free_unchecked) = PathSig(["cursive", "runtime", "region", "free_unchecked"])
std::string RegionSymFreeUnchecked();

// (RegionSym-AddrIsActive)
// RegionAddrActiveSym = PathSig(["cursive", "runtime", "region", "addr_is_active"])
std::string RegionSymAddrIsActive();

// (RegionSym-AddrTagFrom)
// RegionAddrTagFromSym = PathSig(["cursive", "runtime", "region", "addr_tag_from"])
std::string RegionSymAddrTagFrom();

// Dispatch function for RegionSym by method name
std::string RegionSym(const std::string& method);

// ============================================================================
// §6.9 BuiltinSym - FileSystem capability methods
// ============================================================================

// (BuiltinSym-FileSystem-OpenRead)
std::string BuiltinSymFileSystemOpenRead();

// (BuiltinSym-FileSystem-OpenWrite)
std::string BuiltinSymFileSystemOpenWrite();

// (BuiltinSym-FileSystem-OpenAppend)
std::string BuiltinSymFileSystemOpenAppend();

// (BuiltinSym-FileSystem-CreateWrite)
std::string BuiltinSymFileSystemCreateWrite();

// (BuiltinSym-FileSystem-ReadFile)
std::string BuiltinSymFileSystemReadFile();

// (BuiltinSym-FileSystem-ReadBytes)
std::string BuiltinSymFileSystemReadBytes();

// (BuiltinSym-FileSystem-WriteFile)
std::string BuiltinSymFileSystemWriteFile();

// (BuiltinSym-FileSystem-WriteStdout)
std::string BuiltinSymFileSystemWriteStdout();

// (BuiltinSym-FileSystem-WriteStderr)
std::string BuiltinSymFileSystemWriteStderr();

// (BuiltinSym-FileSystem-Exists)
std::string BuiltinSymFileSystemExists();

// (BuiltinSym-FileSystem-Remove)
std::string BuiltinSymFileSystemRemove();

// (BuiltinSym-FileSystem-OpenDir)
std::string BuiltinSymFileSystemOpenDir();

// (BuiltinSym-FileSystem-CreateDir)
std::string BuiltinSymFileSystemCreateDir();

// (BuiltinSym-FileSystem-EnsureDir)
std::string BuiltinSymFileSystemEnsureDir();

// (BuiltinSym-FileSystem-Kind)
std::string BuiltinSymFileSystemKind();

// (BuiltinSym-FileSystem-Restrict)
std::string BuiltinSymFileSystemRestrict();

// ============================================================================
// §6.9 BuiltinSym - HeapAllocator capability methods
// ============================================================================

// (BuiltinSym-HeapAllocator-WithQuota)
std::string BuiltinSymHeapAllocatorWithQuota();

// (BuiltinSym-HeapAllocator-AllocRaw)
std::string BuiltinSymHeapAllocatorAllocRaw();

// (BuiltinSym-HeapAllocator-DeallocRaw)
std::string BuiltinSymHeapAllocatorDeallocRaw();

// ============================================================================
// §18.2 ExecutionDomain builtins
// ============================================================================

// (BuiltinSym-ExecutionDomain-Name)
std::string BuiltinSymExecutionDomainName();

// (BuiltinSym-ExecutionDomain-MaxConcurrency)
std::string BuiltinSymExecutionDomainMaxConcurrency();

// ============================================================================
// §18.2 Context execution domain constructors
// ============================================================================

// (BuiltinSym-Context-Cpu)
std::string BuiltinSymContextCpu();

// (BuiltinSym-Context-Gpu)
std::string BuiltinSymContextGpu();

// (BuiltinSym-Context-Inline)
std::string BuiltinSymContextInline();

// ============================================================================
// §18.6 CancelToken builtins
// ============================================================================

// (BuiltinSym-CancelToken-New)
std::string BuiltinSymCancelTokenNew();

// ============================================================================
// §19 Reactor builtins
// ============================================================================

// (BuiltinSym-Reactor-Run)
// Reactor::run - starts the reactor event loop
std::string BuiltinSymReactorRun();

// (BuiltinSym-Reactor-Register)
// Reactor::register - registers a handler with the reactor
std::string BuiltinSymReactorRegister();

// ============================================================================
// §19 Async builtins
// ============================================================================

// (BuiltinSym-Async-Resume)
// Async@Suspended::resume - resumes an async value with input
// Signature: (async_ptr: *mut Async<Out, In, Result, E>, input: In) -> ()
// Modifies async in place, transitioning to next state
std::string BuiltinSymAsyncResume();

// (BuiltinSym-Async-GetDiscriminant)
// Gets the discriminant of an async value (0=Suspended, 1=Completed, 2=Failed)
// Signature: (async_ptr: *imm Async) -> u8
std::string BuiltinSymAsyncGetDiscriminant();

// (BuiltinSym-Async-GetSuspendedOutput)
// Gets the output field from @Suspended state
// Signature: (async_ptr: *imm Async<Out, In, Result, E>) -> Out
std::string BuiltinSymAsyncGetSuspendedOutput();

// (BuiltinSym-Async-GetCompletedValue)
// Gets the value field from @Completed state
// Signature: (async_ptr: *imm Async<Out, In, Result, E>) -> Result
std::string BuiltinSymAsyncGetCompletedValue();

// (BuiltinSym-Async-GetFailedError)
// Gets the error field from @Failed state
// Signature: (async_ptr: *imm Async<Out, In, Result, E>) -> E
std::string BuiltinSymAsyncGetFailedError();

// (BuiltinSym-Async-CreateCompleted)
// Creates an Async value in @Completed state with given value
// Signature: (value: Result) -> Async<Out, In, Result, E>
std::string BuiltinSymAsyncCreateCompleted();

// (BuiltinSym-Async-CreateFailed)
// Creates an Async value in @Failed state with given error
// Signature: (error: E) -> Async<Out, In, Result, E>
std::string BuiltinSymAsyncCreateFailed();

// (BuiltinSym-Async-CreateSuspended)
// Creates an Async value in @Suspended state with given output and frame
// Signature: (output: Out, frame: *mut Frame) -> Async<Out, In, Result, E>
std::string BuiltinSymAsyncCreateSuspended();

// (BuiltinSym-Async-AllocFrame)
// Allocates async frame storage
// Signature: (size: usize, align: usize) -> *mut u8
std::string BuiltinSymAsyncAllocFrame();

// (BuiltinSym-Async-FreeFrame)
// Frees async frame storage
// Signature: (frame: *mut u8) -> ()
std::string BuiltinSymAsyncFreeFrame();

// ============================================================================
// §6.12.14 String/Bytes builtin symbols
// ============================================================================

// String builtins
std::string BuiltinSymStringFrom();
std::string BuiltinSymStringAsView();
std::string BuiltinSymStringToManaged();
std::string BuiltinSymStringCloneWith();
std::string BuiltinSymStringAppend();
std::string BuiltinSymStringLength();
std::string BuiltinSymStringIsEmpty();
std::string BuiltinSymStringDropManaged();

// Bytes builtins
std::string BuiltinSymBytesWithCapacity();
std::string BuiltinSymBytesFromSlice();
std::string BuiltinSymBytesAsView();
std::string BuiltinSymBytesAsSlice();
std::string BuiltinSymBytesToManaged();
std::string BuiltinSymBytesView();
std::string BuiltinSymBytesViewString();
std::string BuiltinSymBytesAppend();
std::string BuiltinSymBytesLength();
std::string BuiltinSymBytesIsEmpty();
std::string BuiltinSymBytesDropManaged();

// ============================================================================
// §6.8 Panic symbol
// ============================================================================

// (PanicSym)
// PanicSym = PathSig(["cursive", "runtime", "panic"])
std::string RuntimePanicSym();

// ============================================================================
// §6.12.5 Context initialization symbol
// ============================================================================

std::string ContextInitSym();

// ============================================================================
// Spec trace emission symbol
// ============================================================================

std::string RuntimeSpecTraceEmitSym();

// ============================================================================
// Dispatch function for BuiltinSym by qualified name
// ============================================================================

// Returns the mangled symbol for a builtin by its qualified name
// e.g., "FileSystem::open_read" -> BuiltinSymFileSystemOpenRead()
std::string BuiltinSym(const std::string& qualified_name);

// ============================================================================
// Spec Rule Anchors
// ============================================================================

void AnchorRuntimeInterfaceRules();

}  // namespace cursive0::codegen

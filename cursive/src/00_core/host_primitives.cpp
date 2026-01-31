// =============================================================================
// MIGRATION MAPPING: host_primitives.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 1.7 "Host Primitives" (lines 727-744)
//     - FSPrim: filesystem operations set
//       {FSOpenRead, FSOpenWrite, FSOpenAppend, FSCreateWrite, FSReadFile,
//        FSReadBytes, FSWriteFile, FSWriteStdout, FSWriteStderr, FSExists,
//        FSRemove, FSOpenDir, FSCreateDir, FSEnsureDir, FSKind, FSRestrict}
//     - FilePrim: file handle operations
//       {FileReadAll, FileReadAllBytes, FileWrite, FileFlush, FileClose}
//     - DirPrim: directory iterator operations
//       {DirNext, DirClose}
//     - SystemPrim: {SystemGetEnv, SystemExit}
//     - HeapPrim: {HeapWithQuota, HeapAllocRaw, HeapDeallocRaw}
//     - ReactorPrim: {ReactorRun, ReactorRegister}
//     - CancelPrim: {CancelNew, CancelChild, CancelDoCancel, CancelIsCancelled, CancelWaitCancelled}
//     - HostPrim: union of all primitive sets
//     - HostPrimDiag: compiler diagnostic primitives
//       {ParseTOML, ReadBytes, WriteFile, ResolveTool, ResolveRuntimeLib,
//        Invoke, AssembleIR, InvokeLinker}
//     - HostPrimRuntime: FSPrim ∪ FilePrim ∪ DirPrim ∪ SystemPrim ∪ HeapPrim ∪ ReactorPrim ∪ CancelPrim
//     - MapsToDiagOrRuntime(p): true if p in HostPrimDiag ∪ HostPrimRuntime
//     - HostPrimFail(p): failure outside mapped primitives is ill-formed
//
// SOURCE FILE: cursive-bootstrap/src/00_core/host_primitives.cpp
//   - Lines 1-113 (entire file)
//
// CONTENT TO MIGRATE:
//   - IsFSPrim(prim) -> bool (lines 20-44)
//     Checks if primitive is a filesystem operation
//   - IsFilePrim(prim) -> bool (lines 46-59)
//     Checks if primitive is a file handle operation
//   - IsDirPrim(prim) -> bool (lines 61-71)
//     Checks if primitive is a directory operation
//   - IsHostPrimDiag(prim) -> bool (lines 73-88)
//     Checks if primitive is a compiler diagnostic operation
//   - IsHostPrimRuntime(prim) -> bool (lines 90-93)
//     Returns IsFSPrim || IsFilePrim || IsDirPrim
//     Note: spec includes SystemPrim, HeapPrim, ReactorPrim, CancelPrim
//   - MapsToDiagOrRuntime(prim) -> bool (lines 95-98)
//     Returns IsHostPrimDiag || IsHostPrimRuntime
//   - HostPrimFail(prim, failed) -> bool (lines 100-106)
//     Aborts if failed and not mapped to diag/runtime
//   - HostPrimFailureIllFormed(prim, failed) -> bool (lines 108-111)
//     Returns true if failure would be ill-formed
//
// DEPENDENCIES:
//   - cursive/include/00_core/host_primitives.h (header)
//     - HostPrim enum with all primitive values
//   - cursive/include/00_core/assert_spec.h
//     - SPEC_DEF macro
//   - <cstdlib> for std::abort()
//
// REFACTORING NOTES:
//   1. IsHostPrimRuntime currently only checks FS/File/Dir primitives
//      TODO: Add System, Heap, Reactor, Cancel primitive checks per spec
//   2. All SPEC_DEF traces point to "1.7"
//   3. HostPrimFail calls std::abort() for unmapped failures
//   4. Switch statements use explicit case enumeration - maintain for clarity
//   5. Consider adding IsSystemPrim, IsHeapPrim, IsReactorPrim, IsCancelPrim
//      helpers to match spec's primitive set definitions
//   6. HostPrim enum must include all primitives from spec section 1.7
//
// =============================================================================

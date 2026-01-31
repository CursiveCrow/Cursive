#include "cursive0/00_core/host_primitives.h"

#include <cstdlib>

#include "cursive0/00_core/assert_spec.h"

namespace cursive0::core {

static inline void SpecDefsHostPrimitives() {
  SPEC_DEF("HostPrim", "1.7");
  SPEC_DEF("HostPrimDiag", "1.7");
  SPEC_DEF("HostPrimRuntime", "1.7");
  SPEC_DEF("MapsToDiagOrRuntime", "1.7");
  SPEC_DEF("HostPrimFail", "1.7");
  SPEC_DEF("FSPrim", "1.7");
  SPEC_DEF("FilePrim", "1.7");
  SPEC_DEF("DirPrim", "1.7");
}

bool IsFSPrim(HostPrim prim) {
  SpecDefsHostPrimitives();
  // FSPrim is the set of FS operations defined by section 7.7 (FSJudg/FSOp).
  switch (prim) {
    case HostPrim::FSOpenRead:
    case HostPrim::FSOpenWrite:
    case HostPrim::FSOpenAppend:
    case HostPrim::FSCreateWrite:
    case HostPrim::FSReadFile:
    case HostPrim::FSReadBytes:
    case HostPrim::FSWriteFile:
    case HostPrim::FSWriteStdout:
    case HostPrim::FSWriteStderr:
    case HostPrim::FSExists:
    case HostPrim::FSRemove:
    case HostPrim::FSOpenDir:
    case HostPrim::FSCreateDir:
    case HostPrim::FSEnsureDir:
    case HostPrim::FSKind:
    case HostPrim::FSRestrict:
      return true;
    default:
      return false;
  }
}

bool IsFilePrim(HostPrim prim) {
  SpecDefsHostPrimitives();
  // FilePrim is the set of file-handle operations defined by section 7.7 (FileJudg).
  switch (prim) {
    case HostPrim::FileReadAll:
    case HostPrim::FileReadAllBytes:
    case HostPrim::FileWrite:
    case HostPrim::FileFlush:
    case HostPrim::FileClose:
      return true;
    default:
      return false;
  }
}

bool IsDirPrim(HostPrim prim) {
  SpecDefsHostPrimitives();
  // DirPrim is the set of directory-iterator operations defined by section 7.7 (DirJudg).
  switch (prim) {
    case HostPrim::DirNext:
    case HostPrim::DirClose:
      return true;
    default:
      return false;
  }
}

bool IsHostPrimDiag(HostPrim prim) {
  SpecDefsHostPrimitives();
  switch (prim) {
    case HostPrim::ParseTOML:
    case HostPrim::ReadBytes:
    case HostPrim::WriteFile:
    case HostPrim::ResolveTool:
    case HostPrim::ResolveRuntimeLib:
    case HostPrim::Invoke:
    case HostPrim::AssembleIR:
    case HostPrim::InvokeLinker:
      return true;
    default:
      return false;
  }
}

bool IsHostPrimRuntime(HostPrim prim) {
  SpecDefsHostPrimitives();
  return IsFSPrim(prim) || IsFilePrim(prim) || IsDirPrim(prim);
}

bool MapsToDiagOrRuntime(HostPrim prim) {
  SpecDefsHostPrimitives();
  return IsHostPrimDiag(prim) || IsHostPrimRuntime(prim);
}

bool HostPrimFail(HostPrim prim, bool failed) {
  SpecDefsHostPrimitives();
  if (failed && !MapsToDiagOrRuntime(prim)) {
    std::abort();
  }
  return failed;
}

bool HostPrimFailureIllFormed(HostPrim prim, bool failed) {
  SpecDefsHostPrimitives();
  return failed && !MapsToDiagOrRuntime(prim);
}

}  // namespace cursive0::core

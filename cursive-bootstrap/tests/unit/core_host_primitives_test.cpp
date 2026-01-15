#include <cassert>
#include <array>

#include "cursive0/core/host_primitives.h"

namespace {

using cursive0::core::HostPrim;
using cursive0::core::HostPrimFail;
using cursive0::core::HostPrimFailureIllFormed;
using cursive0::core::IsDirPrim;
using cursive0::core::IsFilePrim;
using cursive0::core::IsFSPrim;
using cursive0::core::IsHostPrimDiag;
using cursive0::core::IsHostPrimRuntime;
using cursive0::core::MapsToDiagOrRuntime;

constexpr std::array<HostPrim, 8> kDiagPrims = {
    HostPrim::ParseTOML,
    HostPrim::ReadBytes,
    HostPrim::WriteFile,
    HostPrim::ResolveTool,
    HostPrim::ResolveRuntimeLib,
    HostPrim::Invoke,
    HostPrim::AssembleIR,
    HostPrim::InvokeLinker,
};

constexpr std::array<HostPrim, 16> kFSPrims = {
    HostPrim::FSOpenRead,
    HostPrim::FSOpenWrite,
    HostPrim::FSOpenAppend,
    HostPrim::FSCreateWrite,
    HostPrim::FSReadFile,
    HostPrim::FSReadBytes,
    HostPrim::FSWriteFile,
    HostPrim::FSWriteStdout,
    HostPrim::FSWriteStderr,
    HostPrim::FSExists,
    HostPrim::FSRemove,
    HostPrim::FSOpenDir,
    HostPrim::FSCreateDir,
    HostPrim::FSEnsureDir,
    HostPrim::FSKind,
    HostPrim::FSRestrict,
};

constexpr std::array<HostPrim, 5> kFilePrims = {
    HostPrim::FileReadAll,
    HostPrim::FileReadAllBytes,
    HostPrim::FileWrite,
    HostPrim::FileFlush,
    HostPrim::FileClose,
};

constexpr std::array<HostPrim, 2> kDirPrims = {
    HostPrim::DirNext,
    HostPrim::DirClose,
};

}  // namespace

int main() {
  for (auto prim : kDiagPrims) {
    assert(IsHostPrimDiag(prim));
    assert(!IsHostPrimRuntime(prim));
    assert(MapsToDiagOrRuntime(prim));
    assert(!HostPrimFailureIllFormed(prim, true));
  }

  for (auto prim : kFSPrims) {
    assert(IsFSPrim(prim));
    assert(!IsHostPrimDiag(prim));
    assert(IsHostPrimRuntime(prim));
    assert(MapsToDiagOrRuntime(prim));
    assert(!HostPrimFailureIllFormed(prim, true));
  }

  for (auto prim : kFilePrims) {
    assert(IsFilePrim(prim));
    assert(!IsHostPrimDiag(prim));
    assert(IsHostPrimRuntime(prim));
    assert(MapsToDiagOrRuntime(prim));
    assert(!HostPrimFailureIllFormed(prim, true));
  }

  for (auto prim : kDirPrims) {
    assert(IsDirPrim(prim));
    assert(!IsHostPrimDiag(prim));
    assert(IsHostPrimRuntime(prim));
    assert(MapsToDiagOrRuntime(prim));
    assert(!HostPrimFailureIllFormed(prim, true));
  }

  assert(!HostPrimFail(HostPrim::ParseTOML, false));
  assert(HostPrimFail(HostPrim::ParseTOML, true));

  assert(!HostPrimFailureIllFormed(HostPrim::ReadBytes, true));

  return 0;
}

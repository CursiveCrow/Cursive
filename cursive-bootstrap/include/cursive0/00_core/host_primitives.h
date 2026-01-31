#pragma once

namespace cursive0::core {

enum class HostPrim {
  ParseTOML,
  ReadBytes,
  WriteFile,
  ResolveTool,
  ResolveRuntimeLib,
  Invoke,
  AssembleIR,
  InvokeLinker,

  FSOpenRead,
  FSOpenWrite,
  FSOpenAppend,
  FSCreateWrite,
  FSReadFile,
  FSReadBytes,
  FSWriteFile,
  FSWriteStdout,
  FSWriteStderr,
  FSExists,
  FSRemove,
  FSOpenDir,
  FSCreateDir,
  FSEnsureDir,
  FSKind,
  FSRestrict,

  FileReadAll,
  FileReadAllBytes,
  FileWrite,
  FileFlush,
  FileClose,

  DirNext,
  DirClose,
};

bool IsFSPrim(HostPrim prim);
bool IsFilePrim(HostPrim prim);
bool IsDirPrim(HostPrim prim);

bool IsHostPrimDiag(HostPrim prim);
bool IsHostPrimRuntime(HostPrim prim);
bool MapsToDiagOrRuntime(HostPrim prim);

bool HostPrimFail(HostPrim prim, bool failed);
bool HostPrimFailureIllFormed(HostPrim prim, bool failed);

}  // namespace cursive0::core

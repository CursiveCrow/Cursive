#include <cassert>
#include <string>

#include "cursive0/runtime/runtime_interface.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"

namespace {

using cursive0::codegen::BuiltinSym;
using cursive0::codegen::BuiltinSymBytesDropManaged;
using cursive0::codegen::BuiltinSymBytesIsEmpty;
using cursive0::codegen::BuiltinSymFileSystemCreateDir;
using cursive0::codegen::BuiltinSymFileSystemCreateWrite;
using cursive0::codegen::BuiltinSymFileSystemEnsureDir;
using cursive0::codegen::BuiltinSymFileSystemExists;
using cursive0::codegen::BuiltinSymFileSystemKind;
using cursive0::codegen::BuiltinSymFileSystemOpenAppend;
using cursive0::codegen::BuiltinSymFileSystemOpenDir;
using cursive0::codegen::BuiltinSymFileSystemOpenRead;
using cursive0::codegen::BuiltinSymFileSystemOpenWrite;
using cursive0::codegen::BuiltinSymFileSystemReadBytes;
using cursive0::codegen::BuiltinSymFileSystemReadFile;
using cursive0::codegen::BuiltinSymFileSystemRemove;
using cursive0::codegen::BuiltinSymFileSystemRestrict;
using cursive0::codegen::BuiltinSymFileSystemWriteFile;
using cursive0::codegen::BuiltinSymFileSystemWriteStderr;
using cursive0::codegen::BuiltinSymFileSystemWriteStdout;
using cursive0::codegen::BuiltinSymHeapAllocatorAllocRaw;
using cursive0::codegen::BuiltinSymHeapAllocatorDeallocRaw;
using cursive0::codegen::BuiltinSymHeapAllocatorWithQuota;
using cursive0::codegen::BuiltinSymStringDropManaged;
using cursive0::codegen::ContextInitSym;
using cursive0::codegen::RegionLayout;
using cursive0::codegen::RegionSymAlloc;
using cursive0::codegen::RegionSymFreeUnchecked;
using cursive0::codegen::RegionSymFreeze;
using cursive0::codegen::RegionSymNewScoped;
using cursive0::codegen::RegionSymResetUnchecked;
using cursive0::codegen::RegionSymThaw;
using cursive0::codegen::RuntimePanicSym;
using cursive0::core::PathSig;

}  // namespace

int main() {
  SPEC_COV("RegionLayout");
  const auto layout = RegionLayout();
  assert(layout.size == 16);
  assert(layout.align == 8);
  assert(layout.fields.size() == 2);
  assert(layout.fields[0].name == "disc");
  assert(layout.fields[0].offset == 0);
  assert(layout.fields[1].name == "payload");
  assert(layout.fields[1].offset == 8);

  SPEC_COV("RegionSym-NewScoped");
  assert(RegionSymNewScoped() ==
         PathSig({"cursive", "runtime", "region", "new_scoped"}));

  SPEC_COV("RegionSym-Alloc");
  assert(RegionSymAlloc() ==
         PathSig({"cursive", "runtime", "region", "alloc"}));

  SPEC_COV("RegionSym-ResetUnchecked");
  assert(RegionSymResetUnchecked() ==
         PathSig({"cursive", "runtime", "region", "reset_unchecked"}));

  SPEC_COV("RegionSym-Freeze");
  assert(RegionSymFreeze() ==
         PathSig({"cursive", "runtime", "region", "freeze"}));

  SPEC_COV("RegionSym-Thaw");
  assert(RegionSymThaw() ==
         PathSig({"cursive", "runtime", "region", "thaw"}));

  SPEC_COV("RegionSym-FreeUnchecked");
  assert(RegionSymFreeUnchecked() ==
         PathSig({"cursive", "runtime", "region", "free_unchecked"}));

  SPEC_COV("BuiltinSym-FileSystem-OpenRead");
  assert(BuiltinSymFileSystemOpenRead() ==
         PathSig({"cursive", "runtime", "fs", "open_read"}));

  SPEC_COV("BuiltinSym-FileSystem-OpenWrite");
  assert(BuiltinSymFileSystemOpenWrite() ==
         PathSig({"cursive", "runtime", "fs", "open_write"}));

  SPEC_COV("BuiltinSym-FileSystem-OpenAppend");
  assert(BuiltinSymFileSystemOpenAppend() ==
         PathSig({"cursive", "runtime", "fs", "open_append"}));

  SPEC_COV("BuiltinSym-FileSystem-CreateWrite");
  assert(BuiltinSymFileSystemCreateWrite() ==
         PathSig({"cursive", "runtime", "fs", "create_write"}));

  SPEC_COV("BuiltinSym-FileSystem-ReadFile");
  assert(BuiltinSymFileSystemReadFile() ==
         PathSig({"cursive", "runtime", "fs", "read_file"}));

  SPEC_COV("BuiltinSym-FileSystem-ReadBytes");
  assert(BuiltinSymFileSystemReadBytes() ==
         PathSig({"cursive", "runtime", "fs", "read_bytes"}));

  SPEC_COV("BuiltinSym-FileSystem-WriteFile");
  assert(BuiltinSymFileSystemWriteFile() ==
         PathSig({"cursive", "runtime", "fs", "write_file"}));

  SPEC_COV("BuiltinSym-FileSystem-WriteStdout");
  assert(BuiltinSymFileSystemWriteStdout() ==
         PathSig({"cursive", "runtime", "fs", "write_stdout"}));

  SPEC_COV("BuiltinSym-FileSystem-WriteStderr");
  assert(BuiltinSymFileSystemWriteStderr() ==
         PathSig({"cursive", "runtime", "fs", "write_stderr"}));

  SPEC_COV("BuiltinSym-FileSystem-Exists");
  assert(BuiltinSymFileSystemExists() ==
         PathSig({"cursive", "runtime", "fs", "exists"}));

  SPEC_COV("BuiltinSym-FileSystem-Remove");
  assert(BuiltinSymFileSystemRemove() ==
         PathSig({"cursive", "runtime", "fs", "remove"}));

  SPEC_COV("BuiltinSym-FileSystem-OpenDir");
  assert(BuiltinSymFileSystemOpenDir() ==
         PathSig({"cursive", "runtime", "fs", "open_dir"}));

  SPEC_COV("BuiltinSym-FileSystem-CreateDir");
  assert(BuiltinSymFileSystemCreateDir() ==
         PathSig({"cursive", "runtime", "fs", "create_dir"}));

  SPEC_COV("BuiltinSym-FileSystem-EnsureDir");
  assert(BuiltinSymFileSystemEnsureDir() ==
         PathSig({"cursive", "runtime", "fs", "ensure_dir"}));

  SPEC_COV("BuiltinSym-FileSystem-Kind");
  assert(BuiltinSymFileSystemKind() ==
         PathSig({"cursive", "runtime", "fs", "kind"}));

  SPEC_COV("BuiltinSym-FileSystem-Restrict");
  assert(BuiltinSymFileSystemRestrict() ==
         PathSig({"cursive", "runtime", "fs", "restrict"}));

  SPEC_COV("BuiltinSym-HeapAllocator-WithQuota");
  assert(BuiltinSymHeapAllocatorWithQuota() ==
         PathSig({"cursive", "runtime", "heap", "with_quota"}));

  SPEC_COV("BuiltinSym-HeapAllocator-AllocRaw");
  assert(BuiltinSymHeapAllocatorAllocRaw() ==
         PathSig({"cursive", "runtime", "heap", "alloc_raw"}));

  SPEC_COV("BuiltinSym-HeapAllocator-DeallocRaw");
  assert(BuiltinSymHeapAllocatorDeallocRaw() ==
         PathSig({"cursive", "runtime", "heap", "dealloc_raw"}));

  SPEC_COV("PanicSym");
  assert(RuntimePanicSym() ==
         PathSig({"cursive", "runtime", "panic"}));

  SPEC_COV("StringDropSym-Decl");
  assert(BuiltinSymStringDropManaged() ==
         PathSig({"cursive", "runtime", "string", "drop_managed"}));

  SPEC_COV("BytesDropSym-Decl");
  assert(BuiltinSymBytesDropManaged() ==
         PathSig({"cursive", "runtime", "bytes", "drop_managed"}));

  SPEC_COV("ContextInitSym-Decl");
  assert(ContextInitSym() ==
         PathSig({"cursive", "runtime", "context_init"}));

  assert(BuiltinSymBytesIsEmpty() ==
         PathSig({"cursive", "runtime", "bytes", "is_empty"}));
  assert(BuiltinSym("bytes::is_empty") ==
         PathSig({"cursive", "runtime", "bytes", "is_empty"}));

  SPEC_COV("BuiltinSym-String-Err");
  assert(BuiltinSym("string::not_a_builtin") == "");

  SPEC_COV("BuiltinSym-Bytes-Err");
  assert(BuiltinSym("bytes::not_a_builtin") == "");

  return 0;
}

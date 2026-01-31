// =============================================================================
// MIGRATION MAPPING: link.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 2.5 (lines 1478-1524)
//   - 2.5. Output Artifacts and Linking (Linking subsection)
//   - LinkJudg rules: ResolveRuntimeLib, Link
//   - Link-Ok, Link-NotFound, Link-Runtime-Missing, Link-Runtime-Incompatible, Link-Fail
//
// SOURCE FILE: cursive-bootstrap/src/01_project/link.cpp
//   - Lines 1-600 (entire file)
//
// =============================================================================
// CONTENT TO MIGRATE
// =============================================================================
//
// 1. Helper functions (lines 26-41)
//    - MakeExternalDiag() / EmitExternal()
//
// 2. ReadFileBytes() (lines 42-50)
//    - PURPOSE: Read file contents as string
//
// 3. COFF/Archive parsing utilities (lines 52-267)
//    - ReadU16() / ReadU32() (lines 52-59)
//    - ParseDecimal() (lines 61-84)
//    - TrimArchiveName() (lines 86-95)
//    - IsSpecialArchiveMember() (lines 97-99)
//    - CoffSymbolName() (lines 101-154)
//    - ParseCoffSymbols() (lines 156-205)
//    - ParseArchiveSymbols() (lines 207-236)
//    - LinkerSymsForInputs() (lines 238-267)
//    - MissingRuntimeSym() (lines 269-282)
//    PURPOSE: Extract symbol names from COFF objects and archives
//    for runtime library compatibility checking
//
// 4. RuntimeLibPath() (lines 296-298)
//    - PURPOSE: Get runtime library path
//    - SPEC RULE: RuntimeLibPath(P) = P.root / `runtime` / RuntimeLibName
//    - SPEC REF: Line 1480
//    - RuntimeLibName = "cursive0_rt.lib"
//
// 5. RuntimeRequiredSyms() (lines 300-387)
//    - PURPOSE: Get list of required runtime symbols
//    - SPEC RULE: RuntimeRequiredSyms = RuntimeSyms
//    - SPEC REF: Lines 1493-1494
//    - Returns mangled symbol names for:
//      * cursive::runtime::panic
//      * cursive::runtime::string::drop_managed
//      * cursive::runtime::bytes::drop_managed
//      * cursive::runtime::context_init
//      * cursive::runtime::spec_trace::emit
//      * Region procedures: new_scoped, alloc, mark, reset_to, etc.
//      * String builtins: from, as_view, to_managed, clone_with, etc.
//      * Bytes builtins: with_capacity, from_slice, as_view, etc.
//      * FileSystem methods: open_read, write_file, exists, etc.
//      * Heap methods: with_quota, alloc_raw, dealloc_raw
//    - DEPENDENCIES: core::PathSig for symbol mangling
//
// 6. ResolveRuntimeLib() (lines 389-399)
//    - PURPOSE: Resolve runtime library path and verify existence
//    - SPEC RULES:
//      * ResolveRuntimeLib-Ok (spec lines 1482-1485)
//      * ResolveRuntimeLib-Err (spec lines 1487-1490)
//
// 7. LinkerSyms() (lines 401-406)
//    - PURPOSE: Get symbols from linker inputs
//    - SPEC RULE: LinkerSyms : Path x List(Path) x Path -> Set(Symbol)
//    - SPEC REF: Line 1492
//
// 8. InvokeLinker() (lines 408-528)
//    - PURPOSE: Invoke lld-link or link.exe
//    - Platform-specific implementations (Windows CreateProcess / POSIX fork+exec)
//    - SPEC RULE: InvokeLinker(t, Objs ++ [lib], ExePath(P))
//    - SPEC REF: Lines 1497-1498
//    - Link flags: /OUT:, /ENTRY:main, /SUBSYSTEM:CONSOLE, /NODEFAULTLIB
//
// 9. LinkWithDeps() (lines 540-587)
//    - PURPOSE: Main link function with dependency injection
//    - SPEC RULES:
//      * Link-Ok (spec lines 1501-1504)
//      * Link-NotFound (spec lines 1506-1509)
//      * Link-Runtime-Missing (spec lines 1511-1514)
//      * Link-Runtime-Incompatible (spec lines 1516-1519)
//      * Link-Fail (spec lines 1521-1524)
//    - DIAGNOSTICS:
//      * E-OUT-0404: Link invocation failed
//      * E-OUT-0405: Linker not found
//      * E-OUT-0407: Runtime library missing
//      * E-OUT-0408: Runtime library incompatible
//
// 10. Link() (lines 589-598)
//     - PURPOSE: Public entry point with default dependencies
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// Headers required:
//   - "cursive0/01_project/link.h"
//   - "cursive0/00_core/assert_spec.h" (SPEC_RULE macro)
//   - "cursive0/00_core/diagnostic_messages.h" (MakeDiagnostic)
//   - "cursive0/00_core/host_primitives.h" (HostPrimFail)
//   - "cursive0/00_core/symbols.h" (PathSig for symbol mangling)
//   - "cursive0/01_project/outputs.h" (ExePath)
//   - "cursive0/01_project/tool_resolution.h" (ResolveTool)
//   - <algorithm>
//   - <cstdint>
//   - <fstream>
//   - <sstream>
//   - <unordered_set>
//   - Platform headers:
//     Windows: <windows.h>
//     POSIX: <sys/types.h>, <sys/wait.h>, <unistd.h>
//
// Types from header (link.h):
//   - LinkStatus { Ok, NotFound, RuntimeMissing, RuntimeIncompatible, Fail }
//   - LinkResult { LinkStatus status; DiagnosticStream diags; }
//   - LinkDeps { resolve_tool, resolve_runtime_lib, linker_syms, invoke_linker }
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. COFF parsing is complex but necessary for symbol extraction
//    RECOMMENDATION: Consider moving to a separate coff_parser module
//
// 2. The Windows linker invocation uses CreateProcessW with proper
//    argument quoting - this is important for paths with spaces
//
// 3. The link function tries lld-link first, then falls back to link.exe
//    (MSVC linker) on Windows
//
// 4. RuntimeRequiredSyms() must be kept in sync with the actual runtime library
//    RECOMMENDATION: Consider generating this list from the runtime source
//
// =============================================================================
// SPEC RULE ANNOTATIONS (use SPEC_RULE macro)
// =============================================================================
//
// Line 393: SPEC_RULE("ResolveRuntimeLib-Err");
// Line 397: SPEC_RULE("ResolveRuntimeLib-Ok");
// Line 550: SPEC_RULE("Link-NotFound");
// Line 558: SPEC_RULE("Link-Runtime-Missing");
// Line 570: SPEC_RULE("Link-Runtime-Incompatible");
// Line 578: SPEC_RULE("Link-Fail");
// Line 584: SPEC_RULE("Link-Ok");
//
// =============================================================================


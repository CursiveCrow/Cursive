// =============================================================================
// MIGRATION MAPPING: ir_assembly.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 2.6 (lines 1651-1681)
//   - 2.6. Tool Resolution and IR Assembly Inputs
//   - AssembleIR-Ok / AssembleIR-Err rules
//
// SOURCE FILE: cursive-bootstrap/src/01_project/ir_assembly.cpp
//   - Lines 1-358 (entire file)
//
// =============================================================================
// CONTENT TO MIGRATE
// =============================================================================
//
// 1. Windows-specific helpers (lines 26-216, inside #ifdef _WIN32)
//    - QuoteWinArg() (lines 27-66)
//      PURPOSE: Properly quote Windows command-line arguments
//      Handles backslash escaping before quotes per Windows conventions
//
//    - BuildCommandLine() (lines 68-79)
//      PURPOSE: Build command line for llvm-as
//      Format: tool -o - -
//      (reads from stdin, writes to stdout)
//
//    - WriteAll(HANDLE) (lines 81-97)
//      PURPOSE: Write all bytes to Windows HANDLE
//
//    - InvokeDefault() Windows version (lines 99-216)
//      PURPOSE: Invoke llvm-as process on Windows
//      Uses CreateProcessW with piped stdin/stdout
//      Spawns reader thread for output capture
//
// 2. POSIX-specific helpers (lines 218-335, inside #else)
//    - WriteAll(int fd) (lines 220-238)
//      PURPOSE: Write all bytes to file descriptor
//
//    - InvokeDefault() POSIX version (lines 240-333)
//      PURPOSE: Invoke llvm-as process on POSIX
//      Uses fork/exec with piped stdin/stdout
//      Spawns reader thread for output capture
//
// 3. AssembleIRWithDeps() (lines 339-350)
//    - PURPOSE: Assemble LLVM IR text to bitcode with dependency injection
//    - SPEC RULES:
//      * AssembleIR-Ok (spec lines 1673-1676)
//      * AssembleIR-Err (spec lines 1678-1680)
//    - SPEC REF: Invoke(a, t) -> b
//
// 4. AssembleIR() (lines 352-355)
//    - PURPOSE: Public entry point with default implementation
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// Headers required:
//   - "cursive0/01_project/ir_assembly.h"
//   - "cursive0/00_core/assert_spec.h" (SPEC_RULE macro)
//   - "cursive0/00_core/host_primitives.h" (HostPrimFail)
//   - <atomic>
//   - <string>
//   - <thread>
//   - <vector>
//   - <optional>
//   - <filesystem>
//   - Platform headers:
//     Windows: <windows.h>
//     POSIX: <errno.h>, <fcntl.h>, <sys/types.h>, <sys/wait.h>, <unistd.h>
//
// Types from header (ir_assembly.h):
//   - InvokeFn = std::function<std::optional<std::string>(const path&, string_view)>
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. The Windows and POSIX implementations are largely parallel
//    RECOMMENDATION: Consider abstracting common patterns
//
// 2. Both implementations use a reader thread to avoid deadlock
//    when the child process writes more than pipe buffer size
//
// 3. The tool invocation redirects stderr to /dev/null (NUL on Windows)
//    This suppresses llvm-as error messages
//    RECOMMENDATION: Consider capturing stderr for diagnostic purposes
//
// 4. The command format "tool -o - -" is specific to llvm-as
//    RECOMMENDATION: Document this assumption
//
// =============================================================================
// SPEC RULE ANNOTATIONS (use SPEC_RULE macro)
// =============================================================================
//
// Line 345: SPEC_RULE("AssembleIR-Err");
// Line 348: SPEC_RULE("AssembleIR-Ok");
//
// =============================================================================


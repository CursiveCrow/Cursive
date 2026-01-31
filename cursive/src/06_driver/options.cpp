// =============================================================================
// MIGRATION MAPPING: options.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §2 (lines 746-760)
//   "Phase 0: Build/Project Model"
//   AssemblyKind = {`executable`, `library`}
//   Assembly = ⟨name, kind, root, out_dir, emit_ir, source_root, outputs, modules⟩
//   Project = ⟨root, assemblies, assembly, source_root, outputs, modules⟩
//
//   C0updated.md §2.1 (lines 854-862)
//   "(WF-Assembly-EmitIR)"
//   e ∈ {⊥, `none`, `ll`, `bc`}
//
//   C0updated.md §6.12 (line 1412-1414)
//   "LLVM Target Constants"
//   LLVMTriple = "x86_64-pc-windows-msvc"
//   LLVMDataLayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
//
// SOURCE FILE: cursive-bootstrap/src/06_driver/main.cpp
//   Lines 74-85: CliOptions struct (partial - compiler options embedded)
//   Lines 93-96: InternalFlagsEnabled()
//
// ADDITIONAL SOURCE: cursive-bootstrap/src/01_project/project.h
//   (Project and Assembly structures define configurable options)
//
// DEPENDENCIES:
//   - cursive/include/01_project/project.h (Project, Assembly structures)
//   - <string>, <optional>
//
// PURPOSE:
//   This module handles compiler configuration options that affect compilation
//   behavior. These are distinct from CLI parsing (cli.cpp) and represent the
//   internal configuration state used by the compiler.
//
// REFACTORING NOTES:
//   - Separate CLI options (cli.cpp) from compiler options (options.cpp)
//   - CLI options are parsed from command line
//   - Compiler options represent the resolved configuration
//   - Some options come from manifest (Cursive.toml), some from CLI
//
// IMPLEMENTATION REQUIREMENTS:
//
// 1. EmitIRMode enum
//    Corresponds to spec emit_ir values.
//    enum class EmitIRMode {
//      None,  // No IR emission (default)
//      LL,    // LLVM IR text format (.ll)
//      BC     // LLVM bitcode format (.bc)
//    };
//
// 2. CompilerOptions struct
//    Resolved compiler configuration.
//    struct CompilerOptions {
//      // From project/manifest
//      std::string assembly_name;
//      AssemblyKind assembly_kind;
//      std::filesystem::path source_root;
//      std::filesystem::path output_root;
//      EmitIRMode emit_ir = EmitIRMode::None;
//
//      // From CLI
//      bool phase1_only = false;
//      bool no_output = false;
//      bool dump_ast = false;
//      bool dump_project = false;
//      bool diag_json = false;
//
//      // Spec tracing
//      std::optional<std::string> spec_trace_path;
//    };
//
// 3. TargetOptions struct
//    LLVM target configuration (from spec §6.12).
//    struct TargetOptions {
//      std::string triple = "x86_64-pc-windows-msvc";
//      std::string data_layout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128";
//      std::string cpu = "generic";
//      std::string features = "";
//    };
//
// 4. ResolveOptions(cli_opts, project) -> CompilerOptions
//    Merge CLI options with project configuration.
//    CLI options override project defaults where applicable.
//
// 5. InternalFlagsEnabled() -> bool (SOURCE: lines 93-96)
//    Returns true if CURSIVE0_INTERNAL_FLAGS environment variable is set.
//    Internal flags (--phase1-only, --no-output, --dump-ast, --emit-ir)
//    require this to be enabled.
//
// 6. DefaultTargetOptions() -> TargetOptions
//    Returns target options for the default x86_64-pc-windows-msvc target.
//
// EMIT IR MODES (from spec §2.1, lines 854-862):
//   - None: No IR output, only object files
//   - LL: Emit textual LLVM IR (.ll files)
//   - BC: Emit LLVM bitcode (.bc files, requires llvm-as)
//
// ENVIRONMENT VARIABLES:
//   CURSIVE0_INTERNAL_FLAGS  - Enable internal/debug flags
//   CURSIVE0_DEBUG_PHASES    - Enable phase logging
//   CURSIVE0_DEBUG_PIPELINE  - Enable pipeline debug output
//   CURSIVE0_DEBUG_OBJ       - Enable object emission debug output
//
// =============================================================================

// TODO: Implement compiler options management


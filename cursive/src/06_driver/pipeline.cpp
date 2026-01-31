// =============================================================================
// MIGRATION MAPPING: pipeline.cpp
// =============================================================================
//
// SPEC REFERENCE:
//   C0updated.md §2.5 (lines 1526-1643)
//   "Output Pipeline (Big-Step)" and "Output Pipeline (Small-Step)"
//   OutputPipeline(P) ⇓ (Objs, IRs, ExePath(P))
//
//   C0updated.md §1.1 (lines 232-240)
//   "Conformance"
//   WF(P) ⇔ ∃ Γ. Project(Γ) = P ∧ ∀ j ∈ ReqJudgments(P). Γ ⊢ j ⇓ ok
//   ReqJudgments(P) = [Phase1Order(P), Phase3Order(P), Phase4Order(P)]
//
//   C0updated.md §6 (lines 14221-14326)
//   "Phase 4: Code Generation"
//   CodegenProject(P), CodegenModule(m), CodegenItem(item)
//
// SOURCE FILE: cursive-bootstrap/src/06_driver/main.cpp
//   Lines 295-322: InspectC0Subset() - C0 conformance checking
//   Lines 324-345: ModuleCodegen, LLVMModuleBundle, CodegenCache structs
//   Lines 347-368: EnsureLLVMInit(), EnsureDir()
//   Lines 370-382: WriteFile()
//   Lines 384-502: EmitLLVMModule(), EmitIRForModule(), EmitObjForModule()
//   Lines 504-618: BuildCodegenCache()
//   Lines 672-881: Main pipeline execution (parse, resolve, typecheck, codegen)
//   Lines 885-895: Phase conformance checking
//
// DEPENDENCIES:
//   - cursive/include/00_core/diagnostics.h (DiagnosticStream)
//   - cursive/include/01_project/project.h (Project)
//   - cursive/include/01_project/outputs.h (OutputPipelineWithDeps)
//   - cursive/include/02_source/parser/parse_modules.h (ParseModulesWithDeps)
//   - cursive/include/04_analysis/resolve/resolver.h (ResolveModules)
//   - cursive/include/04_analysis/typing/typecheck.h (TypecheckModules)
//   - cursive/include/04_analysis/conformance/conformance.h (ConformanceInput, RejectIllFormed)
//   - LLVM headers for code generation
//
// PURPOSE:
//   This module implements the compilation pipeline that orchestrates all
//   compiler phases. It handles:
//   1. Phase 1: Parsing and C0 subset conformance
//   2. Phase 3: Resolution and type checking
//   3. Phase 4: Code generation and linking
//   4. Conformance validation
//   5. Error aggregation across phases
//
// REFACTORING NOTES:
//   - Extract the large main() pipeline logic into dedicated functions
//   - Create a PipelineResult type to capture all phase outputs
//   - Consider a phase-by-phase execution model for better error handling
//   - LLVM initialization should be lazy/once
//
// IMPLEMENTATION REQUIREMENTS:
//
// 1. ModuleCodegen struct (SOURCE: lines 324-333)
//    Holds per-module codegen state.
//    struct ModuleCodegen {
//      ModulePath path;
//      std::string path_key;
//      IRDecls decls;
//      std::unordered_map<std::string, TypeRef> value_types;
//      std::unordered_map<std::string, DerivedValueInfo> derived_values;
//      std::uint64_t temp_counter = 0;
//      std::unordered_map<std::string, TypeRef> drop_glue_types;
//      std::optional<std::string> main_symbol;
//    };
//
// 2. LLVMModuleBundle struct (SOURCE: lines 335-338)
//    LLVM module with its context.
//    struct LLVMModuleBundle {
//      std::unique_ptr<llvm::LLVMContext> ctx;
//      std::unique_ptr<llvm::Module> module;
//    };
//
// 3. CodegenCache struct (SOURCE: lines 340-345)
//    Caches codegen state for all modules.
//    struct CodegenCache {
//      LowerCtx ctx;
//      std::vector<ModuleCodegen> modules;
//      std::unordered_map<std::string, std::size_t> index;
//      bool ok = true;
//    };
//
// 4. PipelineResult struct (NEW)
//    Result of pipeline execution.
//    struct PipelineResult {
//      bool phase1_ok;
//      bool resolve_ok;
//      bool typecheck_ok;
//      bool phase4_ok;
//      bool subset_ok;
//      DiagnosticStream diags;
//    };
//
// 5. InspectC0Subset(source) -> InspectResult (SOURCE: lines 295-322)
//    Check C0 subset conformance for a source file.
//    Runs token-level conformance checks:
//    - CheckC0SubsetPermTokens
//    - CheckC0UnwindAttrUnsupportedTokens
//    - CheckC0AttrSyntaxUnsupportedTokens
//    - CheckC0UnsupportedConstructsTokens
//
// 6. EnsureLLVMInit() -> void (SOURCE: lines 347-353)
//    Initialize LLVM targets (once, thread-safe).
//    Uses std::once_flag for lazy initialization.
//
// 7. EnsureDir(path) -> bool (SOURCE: lines 355-368)
//    Ensure directory exists, creating if necessary.
//    Returns true on success.
//
// 8. WriteFile(path, bytes) -> bool (SOURCE: lines 370-382)
//    Write bytes to file, truncating if exists.
//    Returns true on success.
//
// 9. EmitLLVMModule(cache, module, project) -> std::optional<LLVMModuleBundle>
//    (SOURCE: lines 384-412)
//    Generate LLVM module from IR declarations.
//
// 10. EmitIRForModule(cache, module, project) -> std::optional<std::string>
//     (SOURCE: lines 414-428)
//     Generate LLVM IR text for a module.
//
// 11. EmitObjForModule(cache, module, project) -> std::optional<std::string>
//     (SOURCE: lines 430-502)
//     Generate object file bytes for a module.
//     Handles LLVM target setup, verification, and codegen.
//
// 12. BuildCodegenCache(project, sema_ctx, name_maps, typechecked) -> CodegenCache
//     (SOURCE: lines 504-618)
//     Build codegen cache for all modules in project.
//     Sets up LowerCtx with callbacks for name resolution.
//
// 13. RunPipeline(project, options) -> PipelineResult (NEW - refactored from main)
//     Execute full compilation pipeline.
//     Phases:
//     a. Parse modules (Phase 1)
//     b. Visibility checks
//     c. Name map collection
//     d. Module resolution (Phase 3)
//     e. Type checking (Phase 3)
//     f. Code generation (Phase 4, if not phase1_only)
//
// 14. CheckConformance(result) -> bool (SOURCE: lines 885-895)
//     Check phase conformance and reject ill-formed programs.
//     Uses ConformanceInput and RejectIllFormed.
//
// PHASE ORDER (from spec §1.1):
//   Phase1Order(P) - Parsing succeeded
//   Phase3Order(P) - Resolution and type checking succeeded
//   Phase4Order(P) - Code generation succeeded
//
// OUTPUT PIPELINE (from spec §2.5):
//   OutState = {OutStart, OutDirs, OutObjs, OutIR, OutLink, OutDone, Error}
//   Pipeline: OutStart -> OutDirs -> OutObjs -> OutIR -> OutLink -> OutDone
//
// DIAGNOSTIC CODES (from spec §8.3):
//   E-OUT-0401: Output directory error
//   E-OUT-0402: Object file collision
//   E-OUT-0403: Codegen failure
//   E-OUT-0404: IR emission failure
//   E-OUT-0411: Linker not found
//   E-OUT-0412: Runtime library missing
//   E-OUT-0413: Link failure
//
// =============================================================================

// TODO: Migrate pipeline execution from bootstrap main.cpp


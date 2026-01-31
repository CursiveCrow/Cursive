// =============================================================================
// MIGRATION MAPPING: outputs.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md Section 2.5 (lines 1329-1649)
//   - 2.5. Output Artifacts and Linking
//   - Output Root, Output Paths
//   - Object File Naming, Executable Naming
//   - Module Emission Order
//   - Output Pipeline (Big-Step and Small-Step)
//   - Emit Objects, Emit IR
//
// SOURCE FILE: cursive-bootstrap/src/01_project/outputs.cpp
//   - Lines 1-514 (entire file)
//
// =============================================================================
// CONTENT TO MIGRATE
// =============================================================================
//
// 1. Helper functions (lines 22-152)
//    - MakeExternalDiag() / EmitExternal() (lines 23-35)
//    - EmitIrMode() (lines 37-42)
//    - IsExecutable() (lines 44-46)
//    - IsDistinct() (lines 48-58)
//    - UnderPath() (lines 60-65)
//    - EnsureDirDefault() (lines 67-80)
//    - WriteFileDefault() (lines 82-95)
//    - CodegenObjDefault() / CodegenIRDefault() (lines 97-106)
//    - ResolveToolDefault() / AssembleIRDefault() (lines 108-116)
//    - InvokeLinkerDefault() / LinkerSymsDefault() (lines 118-129)
//    - JoinList() / RenderList() (lines 131-151)
//
// 2. ComputeOutputPaths() (lines 155-166)
//    - PURPOSE: Compute output directory structure
//    - SPEC RULES:
//      * OutputPaths(R, A).root = R/A.out_dir or R/`build`
//      * OutputPaths(R, A).obj_dir = root/`obj`
//      * OutputPaths(R, A).ir_dir = root/`ir`
//      * OutputPaths(R, A).bin_dir = root/`bin`
//    - SPEC REF: Lines 1340-1345
//
// 3. ObjPath() / IRPath() / ExePath() (lines 168-186)
//    - PURPOSE: Compute specific output file paths
//    - SPEC RULES:
//      * ObjPath(P, m) = O / `obj` / (MangleModulePath(path(m)) ++ ".obj")
//      * IRPath(P, m, e) = O / `ir` / (MangleModulePath(path(m)) ++ ext(e))
//      * ExePath(P) = O / `bin` / (assembly_name ++ ".exe")
//    - SPEC REF: Lines 1393-1397
//    - DEPENDENCIES: core::MangleModulePath from symbols.h
//
// 4. ObjPaths() / IRPaths() (lines 188-212)
//    - PURPOSE: Compute paths for all modules
//    - SPEC REF: Lines 1399-1400
//
// 5. RequiredOutputs() (lines 214-227)
//    - PURPOSE: Compute all required output paths
//    - SPEC RULE: RequiredOutputs(P) = ObjPaths U IRSet U ExeSet
//    - SPEC REF: Line 200-201
//
// 6. OutputHygiene() (lines 229-237)
//    - PURPOSE: Verify all outputs are under output root
//    - SPEC RULE: OutputHygiene(P) <=> forall p in RequiredOutputs(P). Under(p, OutputRoot(P))
//    - SPEC REF: Line 1338
//
// 7. DumpProject() (lines 239-301)
//    - PURPOSE: Generate project summary for command-line output
//    - SPEC RULE: DumpProject(P, dump) format
//    - SPEC REF: Lines 767-773
//
// 8. OutputPipelineWithDeps() (lines 303-497)
//    - PURPOSE: Main output pipeline with dependency injection
//    - SPEC RULES (Output Pipeline Small-Step):
//      * Out-Start (spec lines 1552-1554)
//      * Out-Dirs-Ok / Out-Dirs-Err (spec lines 1556-1564)
//      * Out-Obj-Collision (spec lines 1566-1569)
//      * Out-Obj-Cons (spec lines 1571-1574)
//      * Out-Obj-Err (spec lines 1576-1579)
//      * Out-Obj-Done (spec lines 1581-1583)
//      * Out-IR-None / Out-IR-None-NoLink (spec lines 1585-1593)
//      * Out-IR-Collision (spec lines 1595-1598)
//      * Out-IR-Cons-LL (spec lines 1600-1603)
//      * Out-IR-Cons-BC (spec lines 1605-1608)
//      * Out-IR-Err (spec lines 1610-1614)
//      * Out-IR-Done / Out-IR-Done-NoLink (spec lines 1616-1624)
//      * Out-Link-Ok (spec lines 1626-1629)
//      * Out-Link-NotFound (spec lines 1631-1634)
//      * Out-Link-Runtime-Missing (spec lines 1636-1638)
//      * Out-Link-Runtime-Incompatible (spec lines 1641-1644)
//      * Out-Link-Fail (spec lines 1646-1649)
//    - SPEC RULES (Big-Step):
//      * Output-Pipeline (spec lines 1531-1534)
//      * Output-Pipeline-Lib (spec lines 1536-1539)
//      * Output-Pipeline-Err (spec lines 1541-1544)
//    - SPEC RULES (Codegen):
//      * CodegenObj-LLVM (spec lines 1422-1425)
//      * CodegenIR-LLVM (spec lines 1427-1430)
//    - SPEC RULES (Emit):
//      * Emit-Objects-Empty (spec lines 1440-1442)
//      * Emit-Objects-Cons (spec lines 1444-1447)
//      * Emit-IR-None (spec lines 1451-1454)
//      * Emit-IR-Cons-LL (spec lines 1456-1459)
//      * Emit-IR-Cons-BC (spec lines 1461-1464)
//      * Emit-IR-Err (spec lines 1473-1476)
//    - DIAGNOSTICS:
//      * E-OUT-0401: Directory creation error
//      * E-OUT-0402: Object file generation error
//      * E-OUT-0403: IR file generation error
//      * E-OUT-0404: Linker invocation error
//      * E-OUT-0405: Linker not found
//      * E-OUT-0406: Output path collision
//      * E-OUT-0407: Runtime library missing
//      * E-OUT-0408: Runtime library incompatible
//
// 9. OutputPipeline() (lines 499-512)
//    - PURPOSE: Public entry point with default dependencies
//
// =============================================================================
// DEPENDENCIES
// =============================================================================
//
// Headers required:
//   - "cursive0/01_project/outputs.h"
//   - "cursive0/00_core/assert_spec.h" (SPEC_RULE macro)
//   - "cursive0/00_core/diagnostic_messages.h" (MakeDiagnostic)
//   - "cursive0/00_core/diagnostics.h" (DiagnosticStream, Emit, HasError)
//   - "cursive0/00_core/host_primitives.h" (HostPrimFail)
//   - "cursive0/00_core/path.h" (Normalize, Prefix)
//   - "cursive0/00_core/symbols.h" (MangleModulePath)
//   - "cursive0/01_project/link.h" (LinkWithDeps, LinkDeps, LinkResult)
//   - "cursive0/01_project/ir_assembly.h" (AssembleIR)
//   - "cursive0/01_project/tool_resolution.h" (ResolveTool)
//   - "cursive0/01_project/project.h" (Project, ModuleInfo)
//   - <algorithm>
//   - <fstream>
//   - <sstream>
//   - <unordered_set>
//   - <filesystem>
//
// Types from header (outputs.h):
//   - OutputPaths { root, obj_dir, ir_dir, bin_dir }
//   - OutputArtifacts { objs, irs, exe }
//   - OutputPipelineResult { std::optional<OutputArtifacts> artifacts; DiagnosticStream diags; }
//   - OutputPipelineDeps { ensure_dir, codegen_obj, codegen_ir, write_file, resolve_tool, assemble_ir, resolve_runtime_lib, invoke_linker, linker_syms }
//
// =============================================================================
// REFACTORING NOTES
// =============================================================================
//
// 1. OutputPipelineDeps enables dependency injection for testing
//    RECOMMENDATION: Maintain this pattern
//
// 2. The pipeline is quite complex with many interleaved operations
//    RECOMMENDATION: Consider breaking into smaller focused functions
//
// 3. WriteFileDefault uses std::ofstream - consider platform-specific optimizations
//
// 4. The spec defines extensive small-step semantics for the output pipeline
//    The implementation closely follows the spec state machine
//
// =============================================================================
// SPEC RULE ANNOTATIONS - see source file for complete list
// =============================================================================
//
// Line 307: SPEC_RULE("Out-Start");
// Line 318-320: SPEC_RULE("Out-Dirs-Err"); SPEC_RULE("Output-Pipeline-Err");
// Line 323: SPEC_RULE("Out-Dirs-Ok");
// Line 327-329: SPEC_RULE("Out-Obj-Collision"); SPEC_RULE("Output-Pipeline-Err");
// Line 338-341: SPEC_RULE("Out-Obj-Err"); SPEC_RULE("Output-Pipeline-Err");
// Line 343: SPEC_RULE("CodegenObj-LLVM");
// Line 351-352: SPEC_RULE("Emit-Objects-Cons"); SPEC_RULE("Out-Obj-Cons");
// ... (many more - see source file)
//
// =============================================================================


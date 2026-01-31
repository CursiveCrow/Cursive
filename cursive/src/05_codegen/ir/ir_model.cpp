// =============================================================================
// MIGRATION MAPPING: ir_model.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.0 Codegen Model and Judgments (lines 14196-14347)
//   - CodegenJudg definitions (line 14204)
//   - IRDecls, ModuleIR (lines 14218-14219)
//   - ProcIR, PanicOutParam (lines 14234-14237)
//   - CG-Project through CG-Place rules (lines 14221-14347)
//   - Section 6.4 ExecIR semantics (lines 15808-15992)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/ir_model.cpp
//   - ValidateValue (lines 11-21)
//   - ValidatePlace (lines 23-25)
//   - ValidateRange (lines 28-36)
//   - ValidateIRPtr (lines 38-43)
//   - ValidateMatchArm (lines 45-53)
//   - ValidateIncoming (lines 55-60)
//   - ValidateIR (lines 64-264)
//   - ValidateDecl (lines 266-299)
//   - ValidateModuleIR (lines 301-308)
//   - AnchorCodegenModelRules (lines 310-333)
//   - AnchorIRExecRules (lines 335-377)
//   - SeqIR overloads (lines 379-413)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/ir/ir_model.h (IR node types)
//   - cursive/include/00_core/assert_spec.h (SPEC_RULE macro)
//   - analysis::TypeRef for type information in IR nodes
//
// REFACTORING NOTES:
//   1. Keep all IR node variants from bootstrap (IROpaque, IRSeq, IRCall, etc.)
//   2. Preserve validation functions for IR structural integrity
//   3. SeqIR is a convenience function for building IR sequences
//   4. AnchorCodegenModelRules and AnchorIRExecRules provide spec traceability
//   5. Consider adding more granular validation error reporting
//   6. IR nodes support parallelism constructs (IRParallel, IRSpawn, etc.)
//   7. Async IR nodes (IRYield, IRYieldFrom, IRSync, IRRace, IRAll) for async support
//
// SPEC RULES IMPLEMENTED:
//   - CG-Project, CG-Module, CG-Item-* (item codegen judgments)
//   - CG-Expr, CG-Stmt, CG-Block, CG-Place (expression/statement judgments)
//   - ExecIR-* (IR execution semantics)
//   - LoopIterIR-* (iterator loop execution)
//   - MoveState-* (move state tracking)
// =============================================================================

#include "cursive/05_codegen/ir/ir_model.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/ir_model.cpp

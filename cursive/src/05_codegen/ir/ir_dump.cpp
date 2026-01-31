// =============================================================================
// MIGRATION MAPPING: ir_dump.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.0 (lines 14196-14347) for IR structure
//   - Section 6.4 (lines 15665-15992) for expression lowering
//   - No explicit dump format specified; this is debugging infrastructure
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/ir_dump.cpp
//   - Dumper struct with formatting state (lines 13-18)
//   - NormalizeOpaqueName (lines 19-33)
//   - IsSimpleIdent (lines 35-50)
//   - ShouldCombineAddrOf (lines 52-60)
//   - Indent helper (lines 62-64)
//   - DumpImmediateBytes (lines 66-81)
//   - Dump(IRValue) (lines 83-111)
//   - DumpRange (lines 113-149)
//   - DumpPattern (lines 151-185)
//   - DumpCall (lines 187-198)
//   - Dump(IRPtr) (lines 200-203)
//   - DumpNode overloads for each IR node type (lines 205-773)
//   - DumpIR(IRDecls) (lines 778-813)
//   - DumpIR(IRPtr) (lines 815-819)
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/ir/ir_model.h (IR node types)
//   - cursive/include/05_codegen/ir/ir_dump.h (dump interface)
//   - analysis::TypeToString for type printing
//   - syntax::RangeKind for range formatting
//
// REFACTORING NOTES:
//   1. Preserve human-readable dump format for debugging
//   2. Handle all IR node types including:
//      - Basic: IROpaque, IRSeq, IRCall, IRCallVTable
//      - Variables: IRReadVar, IRBindVar, IRStoreVar, IRStoreVarNoDrop
//      - Places: IRReadPlace, IRWritePlace, IRAddrOf
//      - Pointers: IRReadPtr, IRWritePtr
//      - Operations: IRUnaryOp, IRBinaryOp, IRCast, IRTransmute
//      - Checks: IRCheckIndex, IRCheckRange, IRCheckSliceLen, IRCheckOp, IRCheckCast
//      - Control: IRReturn, IRResult, IRBreak, IRContinue, IRDefer
//      - Structured: IRIf, IRBlock, IRLoop, IRMatch, IRRegion, IRFrame
//      - Low-level: IRBranch, IRPhi
//      - Panic: IRClearPanic, IRPanicCheck, IRInitPanicHandle, IRLowerPanic
//      - Poison: IRCheckPoison
//      - Parallelism: IRParallel, IRSpawn, IRWait, IRDispatch (§19)
//      - Async: IRYield, IRYieldFrom, IRSync, IRRaceReturn, IRRaceYield, IRAll (§19.2-19.3)
//   3. Smart formatting for common patterns (addr_of + bind, call + bind)
//   4. Indentation-aware output for nested structures
//   5. Display map for opaque value renaming
//
// OUTPUT FORMAT:
//   - Procedures: proc @symbol { ... }
//   - Global constants: global_const @symbol bytes=N
//   - Global zeros: global_zero @symbol size=N
//   - VTables: vtable @symbol size=N align=N drop=@sym slots=[...]
// =============================================================================

#include "cursive/05_codegen/ir/ir_dump.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/ir_dump.cpp

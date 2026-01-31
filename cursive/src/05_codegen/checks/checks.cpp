// =============================================================================
// MIGRATION MAPPING: checks.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.11 Runtime Checks (lines 17200-17265)
//   - Check-Index-Ok/Check-Index-Err rules (lines 17235-17250)
//   - Check-Range-Ok/Check-Range-Err rules (lines 17250-17254)
//   - SliceBounds computation (lines 17245-17250)
//   - Lower-Transmute rule (lines 17255-17258)
//   - Lower-RawDeref-* rules (lines 17267-17285)
//   - IndexCheckRequired predicate
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/checks.cpp
//   - Lines 1-95: PoisonSetFor helper, StringImmediate, EmitRuntimeTrace
//   - Lines 97-165: PanicCode mapping, PanicReasonString
//   - Lines 167-250: Immediate parsing helpers
//   - Lines 253-349: CheckIndex, CheckRange, SliceBounds
//   - Lines 351-397: PanicSym, LowerPanic, ClearPanic, PanicCheck, InitPanicHandle
//   - Lines 402-447: LowerRangeExpr
//   - Lines 449-549: LowerTransmute, LowerRawDeref
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/checks/checks.h
//   - cursive/include/05_codegen/ir/ir_model.h (IRLowerPanic, IRPanicCheck)
//   - cursive/include/05_codegen/layout/layout.h (SizeOf)
//   - cursive/include/05_codegen/cleanup/cleanup.h (CleanupPlan)
//   - cursive/include/runtime/runtime_interface.h
//
// REFACTORING NOTES:
//   1. Runtime checks verify safety conditions at runtime
//   2. Index/range checks for array/slice bounds
//   3. Pointer state checks (null, expired)
//   4. Transmute size validation
//   5. SliceBounds computes (start, end) from range
//   6. Checks may be elided if statically proven safe
//
// PANIC CODES:
//   0x0001 - ErrorExpr
//   0x0002 - ErrorStmt
//   0x0003 - DivZero
//   0x0004 - Overflow
//   0x0005 - Shift
//   0x0006 - Bounds
//   0x0007 - Cast
//   0x0008 - NullDeref
//   0x0009 - ExpiredDeref
//   0x000A - InitPanic
//   0x000B - ContractPre
//   0x000C - ContractPost
//   0x000D - AsyncFailed
//   0x00FF - Other
//
// SLICEBOUNDS COMPUTATION:
//   Full: (0, len)
//   From(start): (start, len)
//   To(end): (0, end)
//   ToInclusive(end): (0, end+1)
//   Exclusive(start, end): (start, end)
//   Inclusive(start, end): (start, end+1)
//   Returns nullopt if invalid
//
// RAW DEREF LOWERING:
//   - Valid: emit ReadPtr
//   - Null: emit LowerPanic(NullDeref)
//   - Expired: emit LowerPanic(ExpiredDeref)
//   - Raw (*imm/*mut): emit unchecked ReadPtr
// =============================================================================

#include "cursive/05_codegen/checks/checks.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/checks.cpp


// =============================================================================
// MIGRATION MAPPING: ub_model.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 1.2 "Behavior Types" (lines 392-445)
//     - BehaviorClass (StaticJudgSet members)
//     - StaticUndefined(J): premise contains bottom
//     - DiagIdOf(J): maps judgment to diagnostic ID
//     - Static-Undefined rule: emits diagnostic code for static undefined
//     - Static-Undefined-NoCode rule: when no code defined
//     - OutsideConformance: no requirements imposed
//     - CheckKind = {PatternExhaustiveness, TypeCompatibility, PermissionViolations,
//                    ProvenanceEscape, ArrayBounds, SafePointerValidity,
//                    IntegerOverflow, SliceBounds, IntDivisionByZero}
//     - StaticCheck vs RuntimeCheck classification
//     - RuntimeBehavior(IntegerOverflow/SliceBounds/IntDivisionByZero) = Panic
//   - UVB (Unspecified Value Behavior) relations
//     - When dynamic undefined conditions occur
//
// SOURCE FILE: cursive-bootstrap/src/00_core/ub_model.cpp
//   - Lines 1-49 (entire file)
//
// CONTENT TO MIGRATE:
//   - SpecDefsUBModel() (lines 7-14) [static inline]
//     Traces UB model definitions to spec section 1.2
//   - BehaviorOfDynamicUndefined(dynamic_undefined) -> BehaviorClass (lines 16-23)
//     Returns UVB if dynamic undefined, Specified otherwise
//     Implements Dynamic-Undefined-UVB rule
//   - DynamicUndefinedReadPtr(perm, read_addr_defined) -> bool (lines 25-29)
//     Returns true if reading from undefined address
//   - DynamicUndefinedWritePtr(perm) -> bool (lines 31-35)
//     Returns true if writing through immutable pointer
//   - StaticUndefinedCode(spec_map, c0_map, id) -> optional<DiagCode> (lines 37-47)
//     Implements Static-Undefined and Static-Undefined-NoCode rules
//     Returns diagnostic code if defined, nullopt otherwise
//
// DEPENDENCIES:
//   - cursive/include/00_core/ub_model.h (header)
//     - BehaviorClass enum (Specified, UVB, etc.)
//     - RawPtrPermission enum (Imm, Mut)
//   - cursive/include/00_core/diagnostic_codes.h
//     - DiagCode, DiagId types
//     - DiagCodeMap type
//     - Code() function
//   - cursive/include/00_core/assert_spec.h
//     - SPEC_DEF macro
//     - SPEC_RULE macro
//
// REFACTORING NOTES:
//   1. SPEC_DEF traces to "1.2":
//      - "BehaviorClass", "UVBRel", "DynamicUndefined", "Behavior"
//      - "StaticUndefined", "DiagIdOf"
//   2. SPEC_RULE traces:
//      - "Dynamic-Undefined-UVB" in BehaviorOfDynamicUndefined()
//      - "Static-Undefined" and "Static-Undefined-NoCode" in StaticUndefinedCode()
//   3. UVB = Unspecified Value Behavior (vs Undefined Behavior)
//   4. RawPtrPermission::Imm means immutable - writes are undefined
//   5. DynamicUndefinedReadPtr currently ignores perm parameter
//      (void cast in source) - may need review
//   6. Consider expanding CheckKind handling per spec classification
//
// =============================================================================

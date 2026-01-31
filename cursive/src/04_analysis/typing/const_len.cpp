// =============================================================================
// MIGRATION MAPPING: const_len.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2.1: Type Equivalence (lines 8490-8593)
//   - ConstLenJudg = {ConstLen}
//   - Rules: ConstLen-Lit, ConstLen-Path, ConstLen-Err
//   - ConstLen-Lit (line 8497-8500): Integer literal evaluation
//   - ConstLen-Path (line 8502-8505): Path-based constant lookup
//   - ConstLen-Err (line 8507-8510): Error case
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_equiv.cpp
//   Lines 311-384 (ConstLen function and helpers)
//
// KEY CONTENT TO MIGRATE:
//   - SpecDefsTypeEquiv() helper referencing "5.2.1" (lines 23-27)
//   - kPointerSizeBytes, kPointerSizeBits constants (lines 29-30)
//   - kIntSuffixes array for parsing (lines 32-34)
//   - EndsWith() string helper (lines 36-41)
//   - StripIntSuffix() to extract numeric core (lines 43-54)
//   - DigitValue() for hex/oct/bin/dec parsing (lines 56-85)
//   - ParseIntCore() for full integer parsing (lines 87-134)
//     * Handles 0x (hex), 0o (octal), 0b (binary), decimal
//     * Handles underscore separators
//     * Uses UInt128 for large value support
//   - IntWidthOf() for type width lookup (lines 136-156)
//   - IsUnsignedIntType(), IsSignedIntType() helpers (lines 158-166)
//   - InRangeUnsigned(), InRangeSigned() range checks (lines 168-206)
//   - ParseIntLiteralUsize() for usize-constrained parsing (lines 208-222)
//   - FindModule() for module lookup (lines 242-250)
//   - FindStaticInit() for static initializer lookup (lines 252-275)
//   - ModuleNamesForConstLen() helper (lines 277-287)
//   - ResolveValuePathForConstLen() for qualified name resolution (lines 289-307)
//   - ConstLen() main function (lines 311-384)
//     * LiteralExpr case: Parse integer, validate usize range (ConstLen-Lit)
//     * IdentifierExpr case: Resolve name, find static, recurse (ConstLen-Path)
//     * PathExpr case: Resolve qualified path, find static, recurse (ConstLen-Path)
//     * Default case: Error (ConstLen-Err)
//
// DEPENDENCIES:
//   - ScopeContext for resolution context
//   - syntax::ExprPtr, syntax::LiteralExpr, syntax::IdentifierExpr, syntax::PathExpr
//   - core::UInt128 for 128-bit arithmetic
//   - ResolveValueName() from resolve/scopes_lookup.h
//   - ResolveQualified() from resolve/scopes_lookup.h
//   - IdEq() from resolve/scopes.h
//   - syntax::ASTModule, syntax::StaticDecl for static lookup
//   - CollectNameMaps() from resolve/collect_toplevel.h
//
// REFACTORING NOTES:
//   1. Integer parsing utilities (ParseIntCore, etc.) could be shared with
//      literal typing in literals.cpp
//   2. Consider caching parsed static initializers to avoid repeated parsing
//   3. The UInt128 dependency is for handling i128/u128 constants
//   4. Module lookup could be optimized with an index
//   5. Error messages should include the expression span for diagnostics
//
// SPEC RULES IMPLEMENTED:
//   - ConstLen-Lit: Integer literal constant evaluation
//   - ConstLen-Path: Path-based constant lookup (identifier and qualified)
//   - ConstLen-Err: Error case for non-constant expressions
//
// RESULT TYPE:
//   ConstLenResult { bool ok; optional<string_view> diag_id; optional<uint64_t> value; }
//
// =============================================================================


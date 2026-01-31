/*
 * =============================================================================
 * MIGRATION MAPPING: contract_intrinsics.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 14.5.1 "@result Intrinsic" (lines 23290-23330)
 *   - C0updated.md, Section 14.5.2 "@entry Intrinsic" (lines 23340-23400)
 *   - C0updated.md, Section 14.9 "Foreign Contracts" (lines 23760-23900)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/contracts/contract_check.cpp (lines 1-263)
 *     (intrinsics handling integrated into contract checking)
 *
 * FUNCTIONS TO MIGRATE:
 *   - ResolveResultIntrinsic(Postcond* post, TypeRef return_ty) -> TypeRef
 *       Resolve @result to the procedure's return type
 *   - ResolveEntryIntrinsic(Expr* entry_expr, Context* ctx) -> EntryCapture
 *       Handle @entry(expr) capturing of entry values
 *   - ValidateEntryType(TypeRef ty) -> bool
 *       Ensure @entry argument is BitcopyType or CloneType
 *   - ResolveForeignAssumes(ExternProc* proc) -> Vec<Assumption>
 *       Process @foreign_assumes predicates
 *   - ResolveForeignEnsures(ExternProc* proc) -> Vec<Guarantee>
 *       Process @foreign_ensures predicates
 *   - HandleForeignError(Postcond* post) -> ErrorCondition
 *       Process @foreign_ensures(@error: pred)
 *   - HandleForeignNull(Postcond* post) -> NullCondition
 *       Process @foreign_ensures(@null_result: pred)
 *
 * DEPENDENCIES:
 *   - Intrinsic AST nodes (@result, @entry, @foreign_*)
 *   - Type checking for BitcopyType/CloneType
 *   - Extern procedure declarations
 *
 * REFACTORING NOTES:
 *   1. @result: References return value in postcondition only
 *   2. @entry(expr): Captures value of expr at procedure entry
 *   3. @entry requires expr's type to be BitcopyType or CloneType
 *   4. Foreign contracts use special syntax:
 *      - |= @foreign_assumes(pred, pred, ...)
 *      - |= @foreign_ensures(pred)
 *      - |= @foreign_ensures(@error: pred)
 *      - |= @foreign_ensures(@null_result: pred)
 *   5. Foreign contracts describe expectations, not Cursive semantics
 *
 * INTRINSIC SEMANTICS:
 *   @result:
 *     - Available only in postcondition context
 *     - Type matches procedure return type
 *     - Cannot appear in precondition
 *
 *   @entry(expr):
 *     - Captures value of expr at entry time
 *     - expr must be BitcopyType or CloneType
 *     - Used to compare post-state with pre-state
 *     Example: @result > @entry(self.value)
 *
 *   @foreign_assumes:
 *     - Predicates caller must establish
 *     - Verified at call sites
 *
 *   @foreign_ensures:
 *     - Predicates callee guarantees
 *     - Assumed true after call
 *
 * DIAGNOSTIC CODES:
 *   - E-CON-0020: @result in precondition
 *   - E-CON-0021: @entry type not copyable
 *   - E-CON-0022: Invalid foreign contract syntax
 *   - E-CON-0023: @foreign_* on non-extern procedure
 *
 * =============================================================================
 */

/*
 * =============================================================================
 * MIGRATION MAPPING: prov_expr.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 21.4.2 "Expression Provenance" (lines 25110-25180)
 *   - C0updated.md, Section 10.5 "Memory Provenance" (lines 22510-22600)
 *   - C0updated.md, Section 6.5 "Expression Evaluation" (lines 16200-16400)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/memory/regions.cpp (lines 1-2036)
 *     (provenance tracking integrated into region analysis)
 *
 * FUNCTIONS TO MIGRATE:
 *   - TrackExprProvenance(Expr* expr) -> Provenance
 *       Track provenance through expression evaluation
 *   - PropagateProvenance(Expr* expr, Provenance src) -> void
 *       Propagate provenance from source to result
 *   - MergeProvenance(Provenance a, Provenance b) -> Provenance
 *       Merge provenance from multiple sources
 *   - AddressOfProvenance(Expr* place) -> Provenance
 *       Compute provenance for &place expression
 *   - DerefProvenance(Expr* ptr) -> Provenance
 *       Compute provenance for *ptr expression
 *   - AllocProvenance(Region* region) -> Provenance
 *       Create provenance for ^alloc expression
 *
 * DEPENDENCIES:
 *   - Provenance representation
 *   - Expression AST
 *   - Region tracking
 *
 * REFACTORING NOTES:
 *   1. Every pointer value has associated provenance
 *   2. Provenance flows through assignments and calls
 *   3. &place creates pointer with place's provenance
 *   4. *ptr inherits provenance from ptr
 *   5. ^alloc creates provenance for current region
 *   6. Provenance merge for unions/conditionals
 *
 * EXPRESSION PROVENANCE RULES:
 *   - Literal: No provenance (values, not pointers)
 *   - Identifier: Provenance of binding
 *   - &place: Provenance of place's root
 *   - *ptr: Provenance of ptr value
 *   - ^value: Current region provenance
 *   - Call result: Provenance from return value
 *   - Binary: Merge of operand provenance (for ptr arithmetic)
 *
 * PROVENANCE MERGE:
 *   - Same source: Keep that provenance
 *   - Different sources: Create merged provenance
 *   - Conflicting sources: May be ill-formed
 *
 * DIAGNOSTIC CODES:
 *   - E-PROV-0010: Provenance tracking lost
 *   - E-PROV-0011: Incompatible provenance merge
 *   - E-PROV-0012: Address of temporary
 *
 * =============================================================================
 */

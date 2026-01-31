/*
 * =============================================================================
 * MIGRATION MAPPING: prov_place.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 21.4 "Provenance Tracking" (lines 25010-25200)
 *   - C0updated.md, Section 21.4.1 "Place Expressions" (lines 25020-25100)
 *   - C0updated.md, Section 10.5 "Memory Provenance" (lines 22510-22600)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/memory/regions.cpp (lines 1-2036)
 *     (provenance tracking integrated into region analysis)
 *
 * FUNCTIONS TO MIGRATE:
 *   - IsPlaceExpr(Expr* expr) -> bool
 *       Determine if expression is a place (lvalue)
 *   - ComputePlaceProvenance(Expr* place) -> Provenance
 *       Compute provenance of a place expression
 *   - GetPlaceRoot(Expr* place) -> Binding*
 *       Find root binding of a place expression
 *   - PlaceProjection(Expr* place) -> Vec<Projection>
 *       Get field/index projections from root
 *   - ValidatePlaceAccess(Expr* place, Permission perm) -> bool
 *       Validate access permission for place
 *   - PlaceOverlaps(Expr* a, Expr* b) -> bool
 *       Check if two places may alias
 *
 * DEPENDENCIES:
 *   - Provenance representation
 *   - Binding tracking
 *   - Projection (field, index, deref)
 *
 * REFACTORING NOTES:
 *   1. Place expressions are memory locations (lvalues)
 *   2. Places have provenance from their source (stack, heap, region)
 *   3. Place projection: x.field, x[i], *ptr
 *   4. Root is the original binding
 *   5. Overlapping places require permission compatibility
 *   6. Consider SSA-style place tracking
 *
 * PLACE EXPRESSIONS:
 *   - Identifier: x
 *   - Field access: place.field
 *   - Index: place[expr]
 *   - Dereference: *ptr
 *
 * PROVENANCE SOURCES:
 *   - Stack: Local bindings
 *   - Region: ^alloc within region block
 *   - Heap: $HeapAllocator allocations
 *   - Static: Module-level statics
 *
 * PROJECTION KINDS:
 *   - Field(name): .field access
 *   - Index(expr): [i] access
 *   - Deref: *ptr dereference
 *
 * DIAGNOSTIC CODES:
 *   - E-PROV-0001: Invalid place expression
 *   - E-PROV-0002: Place root not found
 *   - E-PROV-0003: Incompatible place access
 *
 * =============================================================================
 */

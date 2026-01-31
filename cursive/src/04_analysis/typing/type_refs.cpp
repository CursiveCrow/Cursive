// =============================================================================
// MIGRATION MAPPING: type_refs.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 3.3.2.3: Type Representation (referenced in types.cpp SPEC_DEFs)
//   Section 6.1.4.1: Type Key definitions for ordering
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/types.cpp
//   Lines 1-1017 (complete file)
//
// KEY CONTENT TO MIGRATE:
//   - SpecDefsPermissionSets() for PermSet_C0, S_Perms (lines 15-18)
//   - SpecDefsTypeRepr() for Type, TypeCtor, etc. (lines 20-31)
//   - SpecDefsTypePaths() for TypePaths definition (lines 33-35)
//   - SpecDefsTypeKey() for key definitions (lines 37-52)
//
//   TYPE CONSTRUCTION FUNCTIONS:
//   - MakeType() - base type constructor (lines 359-362)
//   - MakeTypePrim() - primitive type (lines 364-367)
//   - MakeTypeRange() - range type (lines 369-372)
//   - MakeTypePerm() - permission-qualified type (lines 374-377)
//   - MakeTypeUnion() - union type (lines 379-382)
//   - MakeTypeFunc() - function type (lines 384-387)
//   - MakeTypeTuple() - tuple type (lines 389-392)
//   - MakeTypeArray() - array type with length (lines 394-397)
//   - MakeTypeSlice() - slice type (lines 399-402)
//   - MakeTypePtr() - safe pointer type with state (lines 404-407)
//   - MakeTypeRawPtr() - raw pointer type (lines 409-412)
//   - MakeTypeString() - string type with state (lines 414-417)
//   - MakeTypeBytes() - bytes type with state (lines 419-422)
//   - MakeTypeDynamic() - dynamic class type (lines 424-427)
//   - MakeTypeModalState() - modal state type (lines 429-434)
//   - MakeTypePath() - named type path (lines 436-444)
//   - MakeTypeOpaque() - opaque type (lines 446-451)
//   - MakeTypeRefine() - refinement type (lines 453-457)
//
//   TYPE STRING CONVERSION:
//   - AppendTypeString() - internal string builder (lines 459-575)
//   - TypeToString() overloads (lines 577-590)
//
//   TYPE KEY SYSTEM (for ordering):
//   - KeyAtom class methods (lines 592-618)
//   - TagKeyOf() - variant tag key (lines 89-132)
//   - PermKeyOf() - permission key (lines 134-146)
//   - PtrStateKeyOf() - pointer state key (lines 148-161)
//   - QualKeyOf() - raw pointer qualifier key (lines 163-165)
//   - ModeKeyOf() - parameter mode key (lines 167-172)
//   - StateKeyOf() - string/bytes state key (lines 174-186)
//   - TypeKeyOf() - complete type key computation (lines 620-768)
//   - TypeKeyEqual() - key equality (lines 770-781)
//   - TypeKeyLess() - key ordering (lines 783-786)
//   - SortUnionMembers() - canonical union ordering (lines 788-809)
//
//   TYPE PATH COLLECTION:
//   - CollectTypePaths() - extract all paths from type (lines 811-865)
//   - TypePaths() overloads (lines 867-882)
//
//   ASYNC TYPE HELPERS:
//   - IdEq() - identifier equality (lines 886-888)
//   - IsAsyncPath() - check for Async type (lines 893-895)
//   - IsAsyncAliasPath() - check for async aliases (lines 898-903)
//   - IsAsyncType() - determine if type is async (lines 907-927)
//   - GetAsyncSig() - extract async signature (lines 929-1014)
//     * Handles: Async<Out,In,Result,E>, Future<T;E>, Sequence<T>,
//                Stream<T;E>, Pipe<In;Out>, Exchange<T>
//
//   PERMISSION UTILITIES:
//   - IsPermInC0() - permission set membership (lines 337-343)
//   - ParsePermissionKeyword() - keyword to permission (lines 345-357)
//
// DEPENDENCIES:
//   - core::Span for source locations
//   - syntax::Expr, syntax::Type for AST references
//   - project::Fold, project::Utf8LexLess for deterministic ordering
//
// REFACTORING NOTES:
//   1. Type construction functions are factory functions - consider builder pattern
//   2. TypeKey system is for deterministic ordering (important for union normalization)
//   3. Async signature extraction is complex - could be its own module
//   4. PathToString and FoldPath are used for key generation
//   5. SortUnionMembers is critical for type equivalence (unions are unordered)
//   6. Consider separating into:
//      - type_construct.cpp (MakeType* functions)
//      - type_key.cpp (TypeKey system)
//      - type_async.cpp (async helpers)
//
// TYPE NODE VARIANTS:
//   TypePrim, TypeTuple, TypeArray, TypeSlice, TypeFunc, TypePathType,
//   TypeModalState, TypeString, TypeBytes, TypeDynamic, TypePtr, TypeRawPtr,
//   TypeUnion, TypePerm, TypeRange, TypeOpaque, TypeRefine
//
// =============================================================================


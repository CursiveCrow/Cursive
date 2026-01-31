// ===========================================================================
// MIGRATION MAPPING: ast_types.h
// ===========================================================================
//
// PURPOSE:
//   Type AST node definitions. Contains all struct definitions for type
//   nodes in the Cursive grammar, plus the TypeNode variant and Type wrapper.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.3.2.3 - Type Nodes (Lines 2875-2898)
//
//   Type = (Span, TypeNode)                             (2877)
//   TypeNode variants:                                  (2878)
//     TypePrim    - primitive type name
//     TypePerm    - permission-qualified type (const/unique/shared T)
//     TypeUnion   - union type (T1 | T2)
//     TypeFunc    - function type ((T1, T2) -> R)
//     TypeTuple   - tuple type ((T1, T2))
//     TypeArray   - fixed array type ([T; n])
//     TypeSlice   - slice type ([T])
//     TypePtr     - safe pointer type (Ptr<T>@State)
//     TypeRawPtr  - raw pointer type (*imm T, *mut T)
//     TypeString  - string type (string@State)
//     TypeBytes   - bytes type (bytes@State)
//     TypeDynamic - dynamic class type ($ClassName)
//     TypeModal   - modal state type (Modal@State)
//     TypePath    - named type path (Module::Type)
//     TypeOpaque  - opaque type (opaque Path)
//     TypeRefine  - refinement type (T where { pred })
//
//   Supporting types:                                   (2886-2894)
//     Perm = const | unique | shared                    (2886)
//     Qual = imm | mut                                  (2887)
//     PtrStateOpt = Valid | Null | Expired | None       (2888)
//     StringStateOpt = Managed | View | None            (2889)
//     BytesStateOpt = Managed | View | None             (2890)
//     ParamMode = move                                  (2893)
//     ParamType = (ParamMode?, Type)                    (2894)
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\include\cursive0\02_syntax\ast.h
// ---------------------------------------------------------------------------
//
// 1. TYPE NODE STRUCTS (Lines 79-172)
//    ─────────────────────────────────────────────────────────────────────────
//    struct TypePrim { Identifier name; };              (Lines 79-81)
//
//    struct TypePermType {                              (Lines 83-86)
//      TypePerm perm;
//      shared_ptr<Type> base;
//    };
//
//    struct TypeUnion {                                 (Lines 88-90)
//      vector<shared_ptr<Type>> types;
//    };
//
//    struct TypeFuncParam {                             (Lines 92-95)
//      optional<ParamMode> mode;
//      shared_ptr<Type> type;
//    };
//
//    struct TypeFunc {                                  (Lines 97-100)
//      vector<TypeFuncParam> params;
//      shared_ptr<Type> ret;
//    };
//
//    struct TypeTuple {                                 (Lines 102-104)
//      vector<shared_ptr<Type>> elements;
//    };
//
//    struct TypeArray {                                 (Lines 106-109)
//      shared_ptr<Type> element;
//      ExprPtr length;
//    };
//
//    struct TypeSlice {                                 (Lines 111-113)
//      shared_ptr<Type> element;
//    };
//
//    struct TypePtr {                                   (Lines 115-118)
//      shared_ptr<Type> element;
//      optional<PtrState> state;
//    };
//
//    struct TypeRawPtr {                                (Lines 120-123)
//      RawPtrQual qual;
//      shared_ptr<Type> element;
//    };
//
//    struct TypeString {                                (Lines 125-127)
//      optional<StringState> state;
//    };
//
//    struct TypeBytes {                                 (Lines 129-131)
//      optional<BytesState> state;
//    };
//
//    struct TypeDynamic {                               (Lines 133-135)
//      TypePath path;
//    };
//
//    struct TypeModalState {                            (Lines 137-141)
//      TypePath path;
//      vector<shared_ptr<Type>> generic_args;
//      Identifier state;
//    };
//
//    struct TypePathType {                              (Lines 143-146)
//      TypePath path;
//      vector<shared_ptr<Type>> generic_args;  // C0X Extension
//    };
//
//    struct TypeOpaque {                                (Lines 148-150)
//      TypePath path;
//    };
//
//    struct TypeRefine {                                (Lines 152-155)
//      shared_ptr<Type> base;
//      ExprPtr predicate;
//    };
//
// 2. TYPE NODE VARIANT (Lines 157-172)
//    ─────────────────────────────────────────────────────────────────────────
//    using TypeNode = variant<TypePrim, TypePermType, TypeUnion, TypeFunc,
//                              TypeTuple, TypeArray, TypeSlice, TypePtr,
//                              TypeRawPtr, TypeString, TypeBytes, TypeDynamic,
//                              TypeModalState, TypePathType, TypeOpaque,
//                              TypeRefine>;
//
// 3. TYPE WRAPPER STRUCT (Lines 174-177)
//    ─────────────────────────────────────────────────────────────────────────
//    struct Type {
//      core::Span span;
//      TypeNode node;
//    };
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   From ast_fwd.h:
//     - Identifier, TypePath, ExprPtr aliases
//     - Forward declarations
//
//   From ast_enums.h:
//     - TypePerm, RawPtrQual, PtrState, StringState, BytesState, ParamMode
//
//   From 00_core/span.h:
//     - core::Span
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Consider grouping related types:
//      - Primitive/basic: TypePrim, TypePermType
//      - Compound: TypeUnion, TypeFunc, TypeTuple
//      - Collection: TypeArray, TypeSlice
//      - Pointer: TypePtr, TypeRawPtr
//      - String/bytes: TypeString, TypeBytes
//      - Named: TypePath, TypeDynamic, TypeModalState, TypeOpaque
//      - Advanced: TypeRefine
//
//   2. TypeFuncParam is a helper struct used only by TypeFunc.
//      Keep it adjacent to TypeFunc for clarity.
//
//   3. The generic_args field on TypePathType and TypeModalState is
//      a C0X extension. Consider #ifdef for C0-only builds.
//
//   4. TypeRefine contains an ExprPtr for the predicate - this creates
//      a dependency on expressions. Use forward declaration.
//
//   5. All type nodes are relatively small (1-3 fields). The variant
//      size will be dominated by the largest member (TypeFunc with
//      vector of params).
//
//   6. Consider adding type node helpers:
//      bool is_primitive(const Type&);
//      bool is_pointer(const Type&);
//      bool is_reference(const Type&);
//
// ===========================================================================

// TODO: Migrate type definitions from ast.h lines 79-177


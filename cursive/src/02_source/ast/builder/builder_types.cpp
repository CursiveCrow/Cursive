// ===========================================================================
// MIGRATION MAPPING: build_types.cpp
// ===========================================================================
//
// PURPOSE:
//   Factory functions for constructing Type AST nodes. Provides builders
//   for all 15+ type node variants defined in ast_types.h.
//
// ===========================================================================
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ===========================================================================
//   Section 3.3.4 - ParseType rules
//   Defines parsing rules that construct type nodes.
//
//   Type node variants (from spec Section 3.3.2):
//   - TypePrim: Primitive type names
//   - TypePerm: Permission-qualified types (const, unique, shared)
//   - TypeUnion: Union type (T1 | T2)
//   - TypeFunc: Function type ((T1, T2) -> R)
//   - TypeTuple: Tuple type ((T1, T2))
//   - TypeArray: Fixed-size array ([T; n])
//   - TypeSlice: Dynamic slice ([T])
//   - TypePtr: Safe pointer (Ptr<T>@State)
//   - TypeRawPtr: Raw pointer (*imm T, *mut T)
//   - TypeString: String type (string@State)
//   - TypeBytes: Bytes type (bytes@State)
//   - TypeDynamic: Dynamic class object ($ClassName)
//   - TypeModalState: Modal in specific state (Modal@State)
//   - TypePath: Named type with optional generics (Path<T, U>)
//   - TypeOpaque: Opaque type (opaque Path)
//   - TypeRefine: Refinement type (T where { pred })
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parser_types.cpp
// ---------------------------------------------------------------------------
//   Type construction patterns:
//   - make_shared<Type> with TypeNode variant
//   - Permission-wrapped type construction
//   - Generic argument parsing into TypeApply
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Internal:
//   - ast/nodes/ast_fwd.h: Type, shared_ptr aliases
//   - ast/nodes/ast_enums.h: TypePerm, PtrState, StringState, BytesState, RawPtrQual
//   - ast/nodes/ast_types.h: All type node variant definitions
//   - build_common.cpp: span_from, span_between
//   - 00_core/span.h: Span
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Each builder creates a Type wrapper containing the specific variant
//   2. All builders take Span as final parameter for consistency
//   3. Validate that nested types have valid spans before wrapping
//   4. Permission builders should ensure base type is non-null
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above
//
// namespace cursive::ast {
//
// // ---------------------------------------------------------------------------
// // PRIMITIVE AND NAMED TYPES
// // ---------------------------------------------------------------------------
//
// /// Creates a primitive type node.
// /// @param name Type identifier (e.g., "i32", "bool")
// /// @param span Source location
// shared_ptr<Type> make_type_prim(Identifier name, Span span);
//
// /// Creates a named type with optional generic arguments.
// /// @param path Type path (e.g., Vec, std::collections::HashMap)
// /// @param args Generic type arguments (use commas: <T, U>)
// /// @param span Source location
// shared_ptr<Type> make_type_path(TypePath path, vector<shared_ptr<Type>> args, Span span);
//
// /// Creates an opaque type.
// /// @param path Type path
// /// @param span Source location
// shared_ptr<Type> make_type_opaque(TypePath path, Span span);
//
// // ---------------------------------------------------------------------------
// // PERMISSION-QUALIFIED TYPES
// // ---------------------------------------------------------------------------
//
// /// Creates a permission-qualified type.
// /// @param perm Permission qualifier (const, unique, shared)
// /// @param base Underlying type
// /// @param span Source location
// shared_ptr<Type> make_type_perm(TypePerm perm, shared_ptr<Type> base, Span span);
//
// // ---------------------------------------------------------------------------
// // COMPOSITE TYPES
// // ---------------------------------------------------------------------------
//
// /// Creates a union type.
// /// Note: Union types are unordered (A | B == B | A)
// /// @param members Union member types (at least 2)
// /// @param span Source location
// shared_ptr<Type> make_type_union(vector<shared_ptr<Type>> members, Span span);
//
// /// Creates a function type.
// /// @param params Parameter types with optional move qualifier
// /// @param ret Return type
// /// @param span Source location
// shared_ptr<Type> make_type_func(vector<TypeFuncParam> params, shared_ptr<Type> ret, Span span);
//
// /// Creates a tuple type.
// /// For single-element tuple, use (T;) syntax in source.
// /// @param elements Tuple element types
// /// @param span Source location
// shared_ptr<Type> make_type_tuple(vector<shared_ptr<Type>> elements, Span span);
//
// // ---------------------------------------------------------------------------
// // ARRAY AND SLICE TYPES
// // ---------------------------------------------------------------------------
//
// /// Creates a fixed-size array type.
// /// @param element Element type
// /// @param length Compile-time length expression
// /// @param span Source location
// shared_ptr<Type> make_type_array(shared_ptr<Type> element, ExprPtr length, Span span);
//
// /// Creates a slice type.
// /// @param element Element type
// /// @param span Source location
// shared_ptr<Type> make_type_slice(shared_ptr<Type> element, Span span);
//
// // ---------------------------------------------------------------------------
// // POINTER TYPES
// // ---------------------------------------------------------------------------
//
// /// Creates a safe pointer type.
// /// @param element Pointee type
// /// @param state Optional pointer state (@Valid, @Null, @Expired)
// /// @param span Source location
// shared_ptr<Type> make_type_ptr(shared_ptr<Type> element, optional<PtrState> state, Span span);
//
// /// Creates a raw pointer type.
// /// @param qual Mutability qualifier (imm or mut)
// /// @param element Pointee type
// /// @param span Source location
// shared_ptr<Type> make_type_raw_ptr(RawPtrQual qual, shared_ptr<Type> element, Span span);
//
// // ---------------------------------------------------------------------------
// // STRING AND BYTES TYPES
// // ---------------------------------------------------------------------------
//
// /// Creates a string type.
// /// @param state Optional string state (@View, @Managed)
// /// @param span Source location
// shared_ptr<Type> make_type_string(optional<StringState> state, Span span);
//
// /// Creates a bytes type.
// /// @param state Optional bytes state (@View, @Managed)
// /// @param span Source location
// shared_ptr<Type> make_type_bytes(optional<BytesState> state, Span span);
//
// // ---------------------------------------------------------------------------
// // DYNAMIC AND MODAL TYPES
// // ---------------------------------------------------------------------------
//
// /// Creates a dynamic class object type.
// /// @param path Class path (prefixed with $ in source)
// /// @param span Source location
// shared_ptr<Type> make_type_dynamic(TypePath path, Span span);
//
// /// Creates a modal type in a specific state.
// /// @param path Modal type path
// /// @param args Generic arguments
// /// @param state State identifier (e.g., Connected, Disconnected)
// /// @param span Source location
// shared_ptr<Type> make_type_modal_state(TypePath path, vector<shared_ptr<Type>> args, Identifier state, Span span);
//
// // ---------------------------------------------------------------------------
// // REFINEMENT TYPES
// // ---------------------------------------------------------------------------
//
// /// Creates a refinement type.
// /// @param base Base type being refined
// /// @param predicate Predicate expression (must be pure)
// /// @param span Source location
// shared_ptr<Type> make_type_refine(shared_ptr<Type> base, ExprPtr predicate, Span span);
//
// } // namespace cursive::ast


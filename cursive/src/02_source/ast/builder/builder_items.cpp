// ===========================================================================
// MIGRATION MAPPING: build_items.cpp
// ===========================================================================
//
// PURPOSE:
//   Factory functions for constructing Declaration/Item AST nodes.
//   Provides builders for all 30+ declaration variants defined in ast_items.h.
//
// ===========================================================================
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ===========================================================================
//   Section 3.3.8 - ParseItem rules
//   Defines parsing rules that construct item/declaration nodes.
//
//   Declaration categories (from spec Section 4):
//   - Imports: using, import
//   - Values: static let, static var
//   - Procedures: procedure declarations
//   - Types: record, enum, modal, class, type alias
//   - Extern: extern blocks
//
//   Key declaration variants:
//   - UsingDecl: Imports items into scope (using path::item)
//   - ImportDecl: Imports module as namespace (import path)
//   - StaticDecl: Module-level static binding
//   - ProcedureDecl: Procedure definition
//   - RecordDecl: Record type definition
//   - EnumDecl: Enum type definition
//   - ModalDecl: Modal type definition with states
//   - ClassDecl: Class (interface) definition
//   - TypeAliasDecl: Type alias definition
//   - ExternBlock: Foreign function declarations
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\parser_items.cpp
// ---------------------------------------------------------------------------
//   Declaration construction patterns:
//   - ProcedureDecl with all fields (attrs, vis, name, generics, params, ret, contract, body)
//   - RecordDecl with members
//   - EnumDecl with variants
//   - Generic parameters (semicolon-separated) and where clause construction
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Internal:
//   - ast/nodes/ast_fwd.h: Identifier, Path, TypePath
//   - ast/nodes/ast_enums.h: Visibility, Mutability
//   - ast/nodes/ast_common.h: DocList
//   - ast/nodes/ast_attributes.h: AttributeList
//   - ast/nodes/ast_items.h: All declaration node variant definitions
//   - ast/nodes/ast_stmts.h: Block
//   - build_common.cpp: span_from, span_between
//   - 00_core/span.h: Span
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Generic params use semicolons <T; U>, generic args use commas <T, U>
//   2. All declarations have visibility, span, and doc fields
//   3. Attributes (AttributeList) precede applicable declarations
//   4. Where clauses use PredicateName(Type) syntax, not Rust-style
//   5. Error item for parser error recovery
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above
//
// namespace cursive::ast {
//
// // ---------------------------------------------------------------------------
// // IMPORT DECLARATIONS
// // ---------------------------------------------------------------------------
//
// /// Creates a using declaration.
// /// Brings items directly into scope.
// /// @param vis Visibility modifier
// /// @param clause Using clause (path, items, alias, or wildcard)
// /// @param span Source location
// /// @param doc Documentation comments
// ASTItem make_using_decl(Visibility vis, UsingClause clause, Span span, DocList doc);
//
// /// Creates an import declaration.
// /// Brings module into scope as namespace.
// /// @param vis Visibility modifier
// /// @param path Module path
// /// @param alias Optional alias (import path as alias)
// /// @param items Optional specific items to import
// /// @param span Source location
// /// @param doc Documentation comments
// ASTItem make_import_decl(Visibility vis, Path path, optional<Identifier> alias, vector<Identifier> items, Span span, DocList doc);
//
// // ---------------------------------------------------------------------------
// // STATIC DECLARATIONS
// // ---------------------------------------------------------------------------
//
// /// Creates a static declaration.
// /// Module-level static binding.
// /// @param vis Visibility modifier
// /// @param mut Mutability (let = immutable, var = mutable)
// /// @param binding The binding structure
// /// @param span Source location
// /// @param doc Documentation comments
// ASTItem make_static_decl(Visibility vis, Mutability mut, Binding binding, Span span, DocList doc);
//
// // ---------------------------------------------------------------------------
// // PROCEDURE DECLARATIONS
// // ---------------------------------------------------------------------------
//
// /// Creates a procedure declaration.
// /// @param attrs Attribute list
// /// @param vis Visibility modifier
// /// @param name Procedure name
// /// @param generics Optional generic parameters (use semicolons: <T; U>)
// /// @param params Parameter list
// /// @param ret Return type (always explicit)
// /// @param contract Optional contract clause (preconditions/postconditions)
// /// @param body Procedure body block
// /// @param span Source location
// /// @param doc Documentation comments
// ASTItem make_procedure_decl(
//     AttributeList attrs,
//     Visibility vis,
//     Identifier name,
//     optional<GenericParams> generics,
//     vector<Param> params,
//     shared_ptr<Type> ret,
//     optional<ContractClause> contract,
//     shared_ptr<Block> body,
//     Span span,
//     DocList doc
// );
//
// // ---------------------------------------------------------------------------
// // TYPE DECLARATIONS
// // ---------------------------------------------------------------------------
//
// /// Creates a record declaration.
// /// @param attrs Attribute list
// /// @param vis Visibility modifier
// /// @param name Record name
// /// @param generics Optional generic parameters
// /// @param implements List of implemented classes
// /// @param invariant Optional type invariant (where { predicate })
// /// @param members Record members (fields and methods)
// /// @param span Source location
// /// @param doc Documentation comments
// ASTItem make_record_decl(
//     AttributeList attrs,
//     Visibility vis,
//     Identifier name,
//     optional<GenericParams> generics,
//     vector<ClassPath> implements,
//     optional<ExprPtr> invariant,
//     vector<RecordMember> members,
//     Span span,
//     DocList doc
// );
//
// /// Creates an enum declaration.
// /// @param attrs Attribute list
// /// @param vis Visibility modifier
// /// @param name Enum name
// /// @param generics Optional generic parameters
// /// @param variants Enum variants
// /// @param span Source location
// /// @param doc Documentation comments
// ASTItem make_enum_decl(
//     AttributeList attrs,
//     Visibility vis,
//     Identifier name,
//     optional<GenericParams> generics,
//     vector<VariantDecl> variants,
//     Span span,
//     DocList doc
// );
//
// /// Creates a modal declaration.
// /// @param vis Visibility modifier
// /// @param name Modal type name
// /// @param generics Optional generic parameters
// /// @param states State blocks
// /// @param span Source location
// /// @param doc Documentation comments
// ASTItem make_modal_decl(
//     Visibility vis,
//     Identifier name,
//     optional<GenericParams> generics,
//     vector<StateBlock> states,
//     Span span,
//     DocList doc
// );
//
// /// Creates a class declaration.
// /// @param vis Visibility modifier
// /// @param name Class name
// /// @param generics Optional generic parameters
// /// @param supers Parent classes
// /// @param items Class items (methods, associated types)
// /// @param span Source location
// /// @param doc Documentation comments
// ASTItem make_class_decl(
//     Visibility vis,
//     Identifier name,
//     optional<GenericParams> generics,
//     vector<ClassPath> supers,
//     vector<ClassItem> items,
//     Span span,
//     DocList doc
// );
//
// /// Creates a type alias declaration.
// /// @param vis Visibility modifier
// /// @param name Alias name
// /// @param generics Optional generic parameters
// /// @param type Aliased type
// /// @param span Source location
// /// @param doc Documentation comments
// ASTItem make_type_alias_decl(
//     Visibility vis,
//     Identifier name,
//     optional<GenericParams> generics,
//     shared_ptr<Type> type,
//     Span span,
//     DocList doc
// );
//
// // ---------------------------------------------------------------------------
// // EXTERN DECLARATIONS
// // ---------------------------------------------------------------------------
//
// /// Creates an extern block.
// /// @param abi ABI string ("C", "C-unwind", etc.)
// /// @param items Extern procedure declarations
// /// @param span Source location
// /// @param doc Documentation comments
// ASTItem make_extern_block(
//     string abi,
//     vector<ExternItem> items,
//     Span span,
//     DocList doc
// );
//
// /// Creates an extern procedure declaration (inside extern block).
// /// @param name Procedure name
// /// @param params Parameter list (must be FfiSafe types)
// /// @param ret Return type (must be FfiSafe)
// /// @param contracts Foreign contracts
// /// @param span Source location
// ExternItem make_extern_procedure(
//     Identifier name,
//     vector<Param> params,
//     shared_ptr<Type> ret,
//     ForeignContracts contracts,
//     Span span
// );
//
// // ---------------------------------------------------------------------------
// // GENERIC PARAMETER HELPERS
// // ---------------------------------------------------------------------------
//
// /// Creates a generic parameter list.
// /// Note: Parameters use semicolons as separators.
// /// @param params Type parameters
// /// @param where_clause Optional where clause predicates
// GenericParams make_generic_params(vector<TypeParam> params, optional<WhereClause> where_clause);
//
// /// Creates a type parameter.
// /// @param name Parameter name
// /// @param bounds Class bounds (T <: Comparable)
// /// @param default_type Optional default type
// TypeParam make_type_param(Identifier name, vector<ClassPath> bounds, shared_ptr<Type> default_type);
//
// /// Creates a where clause.
// /// Note: Uses PredicateName(Type) syntax, not Rust-style.
// /// @param predicates Predicate list (Bitcopy(T), Clone(U), T <: Class)
// WhereClause make_where_clause(vector<WherePredicate> predicates);
//
// // ---------------------------------------------------------------------------
// // CONTRACT HELPERS
// // ---------------------------------------------------------------------------
//
// /// Creates a contract clause for procedure.
// /// @param preconditions Precondition expressions
// /// @param postconditions Postcondition expressions (may use @result, @entry)
// ContractClause make_contract_clause(vector<ExprPtr> preconditions, vector<ExprPtr> postconditions);
//
// /// Creates foreign contracts for extern procedures.
// /// @param assumes Foreign assumes predicates
// /// @param ensures Foreign ensures predicates
// /// @param ensures_error Error condition predicates
// /// @param ensures_null Null result predicates
// ForeignContracts make_foreign_contracts(
//     vector<ExprPtr> assumes,
//     vector<ExprPtr> ensures,
//     vector<ExprPtr> ensures_error,
//     vector<ExprPtr> ensures_null
// );
//
// // ---------------------------------------------------------------------------
// // ERROR ITEM
// // ---------------------------------------------------------------------------
//
// /// Creates an error item for parser error recovery.
// /// @param span Error location
// ASTItem make_error_item(Span span);
//
// } // namespace cursive::ast


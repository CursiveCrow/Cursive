// ===========================================================================
// MIGRATION MAPPING: validate_items.cpp
// ===========================================================================
//
// PURPOSE:
//   Validation functions for Declaration/Item AST nodes. Checks declaration
//   well-formedness, context constraints, visibility rules, and declaration-
//   specific validation.
//
// ===========================================================================
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ===========================================================================
//   Section 4 - Declarations
//   Defines declaration syntax and rules.
//
//   Section 4.1 - Procedures
//   - Return type always explicit
//   - Non-unit procedures must end with return
//   - main signature: public procedure main(move ctx: Context) -> i32
//
//   Section 4.2 - Records
//   - Receiver shorthand: ~, ~!, ~%
//   - Method vs static procedure distinction
//   - implements clause validation
//
//   Section 4.3 - Enums
//   - Variant validation
//   - Discriminant validation
//
//   Section 4.4 - Modal Types
//   - State block validation
//   - Transition only in modal context
//
//   Section 4.5 - Classes
//   - Abstract method validation
//   - Associated type validation
//
//   Section 4.8 - Generics
//   - Parameters use semicolons <T; U>
//   - Arguments use commas <T, U>
//   - Where clause syntax: PredicateName(T)
//
//   Section 4.15 - Extern Blocks
//   - FfiSafe type validation
//   - Foreign contract syntax
//
//   Section 4.17 - Visibility
//   - public, internal, private, protected
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\src\02_syntax\keyword_policy.cpp
// ---------------------------------------------------------------------------
//   CheckMethodContext function
//   - Validates receiver only in method context
//   - Validates override only on methods
//   - Validates transition only in modal states
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Internal:
//   - ast/nodes/ast_items.h: All declaration node variant definitions
//   - ast/nodes/ast_enums.h: Visibility, Mutability
//   - ast/nodes/ast_attributes.h: AttributeList
//   - validate_common.cpp: ValidationResult, ValidationContext
//   - validate_types.cpp: validate_type
//   - validate_stmts.cpp: validate_block
//   - 00_core/span.h: Span for error locations
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Item validation sets up context for nested validation
//   2. Generic parameter vs argument separator is critical check
//   3. Method context validation is complex (receiver, override, etc.)
//   4. FFI validation requires FfiSafe type checking
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above
//
// namespace cursive::ast::validate {
//
// // ---------------------------------------------------------------------------
// // TOP-LEVEL ITEM VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates a declaration/item node and all its children.
// /// @param item Item to validate
// /// @param ctx Validation context
// ValidationResult validate_item(const ASTItem& item, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // METHOD CONTEXT VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates method context constraints across a file.
// /// Checks:
// /// - Receiver only in method context
// /// - Override only on method declarations
// /// - Transition only in modal state blocks
// /// @param file File to validate
// /// @param ctx Validation context
// ValidationResult validate_method_context(const ASTFile& file, ValidationContext& ctx);
//
// /// Checks if receiver is valid in current context.
// /// @param receiver Receiver to check
// /// @param in_method Whether currently in method context
// /// @param ctx Validation context
// bool is_valid_receiver(const Receiver& receiver, bool in_method, ValidationContext& ctx);
//
// /// Checks if override is valid in current context.
// /// @param method Method declaration to check
// /// @param ctx Validation context
// bool is_valid_override_context(const MethodDecl& method, ValidationContext& ctx);
//
// /// Checks if transition is valid in current context.
// /// @param trans Transition declaration to check
// /// @param state Current state block
// /// @param ctx Validation context
// bool is_valid_transition_context(const TransitionDecl& trans, const StateBlock& state, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // IMPORT DECLARATION VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates using declaration.
// /// @param decl Using declaration to check
// /// @param ctx Validation context
// bool is_valid_using_decl(const UsingDecl& decl, ValidationContext& ctx);
//
// /// Validates import declaration.
// /// @param decl Import declaration to check
// /// @param ctx Validation context
// bool is_valid_import_decl(const ImportDecl& decl, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // STATIC DECLARATION VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates static declaration.
// /// @param decl Static declaration to check
// /// @param ctx Validation context
// bool is_valid_static_decl(const StaticDecl& decl, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // PROCEDURE DECLARATION VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates procedure declaration.
// /// Checks:
// /// - Return type is present
// /// - Non-unit procedure ends with return
// /// - Parameters are valid
// /// - Generic parameters use semicolons
// /// - Contract clauses are valid
// /// - Body is valid
// /// @param decl Procedure declaration to check
// /// @param ctx Validation context
// bool is_valid_procedure_decl(const ProcedureDecl& decl, ValidationContext& ctx);
//
// /// Validates main procedure signature.
// /// Expected: public procedure main(move ctx: Context) -> i32
// /// @param decl Procedure declaration to check
// /// @param ctx Validation context
// bool is_valid_main_signature(const ProcedureDecl& decl, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // TYPE DECLARATION VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates record declaration.
// /// Checks:
// /// - Generic parameters use semicolons
// /// - Implements clause references valid classes
// /// - Members are valid
// /// - Type invariant (if present) is valid
// /// @param decl Record declaration to check
// /// @param ctx Validation context
// bool is_valid_record_decl(const RecordDecl& decl, ValidationContext& ctx);
//
// /// Validates enum declaration.
// /// Checks:
// /// - At least one variant
// /// - Variants are valid
// /// - Discriminants (if present) are valid
// /// @param decl Enum declaration to check
// /// @param ctx Validation context
// bool is_valid_enum_decl(const EnumDecl& decl, ValidationContext& ctx);
//
// /// Validates modal declaration.
// /// Checks:
// /// - At least one state
// /// - States are valid
// /// - Transitions only within states
// /// @param decl Modal declaration to check
// /// @param ctx Validation context
// bool is_valid_modal_decl(const ModalDecl& decl, ValidationContext& ctx);
//
// /// Validates class declaration.
// /// Checks:
// /// - Items are valid (methods, associated types)
// /// - Super classes are valid
// /// @param decl Class declaration to check
// /// @param ctx Validation context
// bool is_valid_class_decl(const ClassDecl& decl, ValidationContext& ctx);
//
// /// Validates type alias declaration.
// /// @param decl Type alias declaration to check
// /// @param ctx Validation context
// bool is_valid_type_alias_decl(const TypeAliasDecl& decl, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // EXTERN DECLARATION VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates extern block.
// /// Checks:
// /// - ABI string is valid
// /// - All items are valid extern items
// /// - All types are FfiSafe
// /// @param block Extern block to check
// /// @param ctx Validation context
// bool is_valid_extern_block(const ExternBlock& block, ValidationContext& ctx);
//
// /// Validates extern procedure declaration.
// /// Checks:
// /// - All parameter types are FfiSafe
// /// - Return type is FfiSafe
// /// - Foreign contracts are valid
// /// @param proc Extern procedure to check
// /// @param ctx Validation context
// bool is_valid_extern_procedure(const ExternProcedure& proc, ValidationContext& ctx);
//
// /// Checks if a type is FfiSafe (can cross FFI boundary).
// /// FfiSafe: primitives (except bool), raw pointers, arrays of FfiSafe,
// ///          [[layout(C)]] records/enums with FfiSafe fields
// /// NOT FfiSafe: bool, modals, Ptr<T>, $Class, tuples, unions, slices,
// ///              string, bytes, Context
// /// @param type Type to check
// bool is_ffi_safe_type(const Type& type);
//
// // ---------------------------------------------------------------------------
// // GENERIC PARAMETER VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates generic parameter list.
// /// Checks:
// /// - Parameters use semicolons as separators (not commas)
// /// - Type parameters are valid
// /// - Where clause uses correct syntax
// /// @param params Generic parameters to check
// /// @param ctx Validation context
// bool is_valid_generic_params(const GenericParams& params, ValidationContext& ctx);
//
// /// Validates where clause.
// /// Checks:
// /// - Uses PredicateName(Type) syntax (not Rust-style)
// /// - Predicates are valid (Bitcopy, Clone, Drop, FfiSafe)
// /// - Class constraints use <: syntax
// /// @param clause Where clause to check
// /// @param ctx Validation context
// bool is_valid_where_clause(const WhereClause& clause, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // CONTRACT VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates contract clause.
// /// Checks:
// /// - Preconditions are valid pure expressions
// /// - Postconditions are valid, may use @result and @entry
// /// @param clause Contract clause to check
// /// @param ctx Validation context
// bool is_valid_contract_clause(const ContractClause& clause, ValidationContext& ctx);
//
// /// Validates foreign contracts.
// /// Checks:
// /// - Uses @foreign_assumes syntax
// /// - Uses @foreign_ensures syntax
// /// @param contracts Foreign contracts to check
// /// @param ctx Validation context
// bool is_valid_foreign_contracts(const ForeignContracts& contracts, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // ATTRIBUTE VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates attribute list.
// /// Checks:
// /// - All attributes are valid for the declaration kind
// /// - No duplicate attributes (unless allowed)
// /// @param attrs Attribute list to check
// /// @param decl_kind Kind of declaration being attributed
// /// @param ctx Validation context
// bool is_valid_attributes(const AttributeList& attrs, ItemKind decl_kind, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // VISIBILITY VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates visibility for declaration context.
// /// @param vis Visibility to check
// /// @param decl_kind Kind of declaration
// /// @param ctx Validation context
// bool is_valid_visibility(Visibility vis, ItemKind decl_kind, ValidationContext& ctx);
//
// } // namespace cursive::ast::validate


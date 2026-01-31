// ===========================================================================
// MIGRATION MAPPING: ast_items.h
// ===========================================================================
//
// PURPOSE:
//   Declaration/Item AST node definitions. Contains all struct definitions
//   for top-level declarations and their sub-components (fields, methods,
//   variants, state blocks, etc.), plus the ASTItem variant.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.3.2.5 - Declaration Nodes (Lines 2680-2873)
//
//   ASTItem variant types:                              (2682)
//
//   IMPORT/USING:
//     UsingDecl  - using declarations
//     ImportDecl - import declarations (C0X)
//
//   EXTERN:
//     ExternBlock - extern block with ABI specifier
//
//   VALUES:
//     StaticDecl - static/const declarations
//
//   PROCEDURES:
//     ProcedureDecl - procedure declarations
//
//   TYPES:
//     RecordDecl    - record type declarations
//     EnumDecl      - enum type declarations
//     ModalDecl     - modal type declarations
//     ClassDecl     - class/interface declarations
//     TypeAliasDecl - type alias declarations
//
//   ERROR:
//     ErrorItem - parse error placeholder
//
//   Supporting structures:
//     GenericParams, TypeParam, WhereClause, WherePredicate
//     ContractClause, TypeInvariant
//     Param, Receiver
//     FieldDecl, MethodDecl, RecordMember
//     VariantDecl, VariantPayload
//     StateBlock, StateMember, TransitionDecl
//     ClassItem, AssociatedTypeDecl
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\include\cursive0\02_syntax\ast.h
// ---------------------------------------------------------------------------
//
// 1. VISIBILITY AND BASIC ENUMS (Lines 854-888)
//    ─────────────────────────────────────────────────────────────────────────
//    enum class Visibility { Public, Internal, Private, Protected };
//    enum class Mutability { Let, Var };
//    struct Param { optional<ParamMode> mode; Identifier name; shared_ptr<Type> type; Span span; };
//    enum class ReceiverPerm { Const, Unique, Shared };
//    struct ReceiverShorthand { ReceiverPerm perm; };
//    struct ReceiverExplicit { optional<ParamMode> mode_opt; shared_ptr<Type> type; };
//    using Receiver = variant<ReceiverShorthand, ReceiverExplicit>;
//
// 2. USING DECLARATIONS (Lines 890-916)
//    ─────────────────────────────────────────────────────────────────────────
//    struct UsingSpec { Identifier name; optional<Identifier> alias_opt; };
//    struct UsingPath { ModulePath path; optional<Identifier> alias_opt; };
//    struct UsingList { ModulePath module_path; vector<UsingSpec> specs; };
//    struct UsingWildcard { ModulePath module_path; };
//    using UsingClause = variant<UsingPath, UsingList, UsingWildcard>;
//    struct UsingDecl { Visibility vis; UsingClause clause; Span span; DocList doc; };
//
// 3. IMPORT DECLARATION (Lines 918-929) - C0X Extension
//    ─────────────────────────────────────────────────────────────────────────
//    struct ImportDecl {
//      Visibility vis;
//      Path path;
//      optional<Identifier> alias;
//      vector<Identifier> items;
//      Span span;
//      DocList doc;
//    };
//
// 4. STATIC DECLARATION (Lines 931-937)
//    ─────────────────────────────────────────────────────────────────────────
//    struct StaticDecl {
//      Visibility vis;
//      Mutability mut;
//      Binding binding;
//      Span span;
//      DocList doc;
//    };
//
// 5. GENERICS (Lines 939-984) - C0X Extension
//    ─────────────────────────────────────────────────────────────────────────
//    enum class Variance { Covariant, Contravariant, Invariant, Bivariant };
//    struct TypeBound { ClassPath class_path; };
//    struct TypeParam { Identifier name; vector<TypeBound> bounds; shared_ptr<Type> default_type; Span span; };
//    struct GenericParams { vector<TypeParam> params; Span span; };
//    struct GenericArgs { vector<shared_ptr<Type>> args; Span span; };
//    struct WherePredicate { Identifier type_param; vector<TypeBound> bounds; Span span; };
//    struct WhereClause { vector<WherePredicate> predicates; Span span; };
//
// 6. CONTRACTS (Lines 986-1022) - C0X Extension
//    ─────────────────────────────────────────────────────────────────────────
//    struct ContractClause { ExprPtr precondition; ExprPtr postcondition; Span span; };
//    enum class ForeignContractKind { Assumes, Ensures };
//    struct ForeignContractClause { ForeignContractKind kind; vector<ExprPtr> predicates; Span span; };
//    enum class ContractIntrinsicKind { Result, Entry };
//    struct ContractIntrinsicExpr { ContractIntrinsicKind kind; ExprPtr expr; };
//    struct TypeInvariant { ExprPtr predicate; Span span; };
//
// 7. PROCEDURE DECLARATION (Lines 1024-1036)
//    ─────────────────────────────────────────────────────────────────────────
//    struct ProcedureDecl {
//      AttributeList attrs;
//      Visibility vis;
//      Identifier name;
//      optional<GenericParams> generic_params;
//      vector<Param> params;
//      shared_ptr<Type> return_type_opt;
//      optional<WhereClause> where_clause;
//      optional<ContractClause> contract;
//      shared_ptr<Block> body;
//      Span span;
//      DocList doc;
//    };
//
// 8. EXTERN DECLARATIONS (Lines 1038-1073)
//    ─────────────────────────────────────────────────────────────────────────
//    struct ExternAbiString { Token literal; };
//    struct ExternAbiIdent { Identifier name; };
//    using ExternAbi = variant<ExternAbiString, ExternAbiIdent>;
//    struct ExternProcDecl { ... };  // Similar to ProcedureDecl + foreign contracts
//    using ExternItem = variant<ExternProcDecl>;
//    struct ExternBlock { AttributeList attrs; Visibility vis; optional<ExternAbi> abi_opt; vector<ExternItem> items; Span span; DocList doc; };
//
// 9. RECORD DECLARATION (Lines 1075-1113)
//    ─────────────────────────────────────────────────────────────────────────
//    struct FieldDecl { AttributeList attrs; Visibility vis; bool key_boundary; Identifier name; shared_ptr<Type> type; shared_ptr<Expr> init_opt; Span span; optional<DocList> doc_opt; };
//    struct MethodDecl { AttributeList attrs; Visibility vis; bool override_flag; Identifier name; Receiver receiver; vector<Param> params; shared_ptr<Type> return_type_opt; optional<ContractClause> contract; shared_ptr<Block> body; Span span; optional<DocList> doc_opt; };
//    using RecordMember = variant<FieldDecl, MethodDecl>;
//    struct RecordDecl { AttributeList attrs; Visibility vis; Identifier name; optional<GenericParams> generic_params; vector<ClassPath> implements; optional<WhereClause> where_clause; optional<TypeInvariant> invariant; vector<RecordMember> members; Span span; DocList doc; };
//
// 10. ENUM DECLARATION (Lines 1115-1144)
//     ────────────────────────────────────────────────────────────────────────
//     struct VariantPayloadTuple { vector<shared_ptr<Type>> elements; };
//     struct VariantPayloadRecord { vector<FieldDecl> fields; };
//     using VariantPayload = variant<VariantPayloadTuple, VariantPayloadRecord>;
//     struct VariantDecl { Identifier name; optional<VariantPayload> payload_opt; optional<Token> discriminant_opt; Span span; optional<DocList> doc_opt; };
//     struct EnumDecl { AttributeList attrs; Visibility vis; Identifier name; optional<GenericParams> generic_params; vector<ClassPath> implements; optional<WhereClause> where_clause; optional<TypeInvariant> invariant; vector<VariantDecl> variants; Span span; DocList doc; };
//
// 11. MODAL DECLARATION (Lines 1146-1194)
//     ────────────────────────────────────────────────────────────────────────
//     struct StateFieldDecl { Visibility vis; bool key_boundary; Identifier name; shared_ptr<Type> type; Span span; optional<DocList> doc_opt; };
//     struct StateMethodDecl { Visibility vis; Identifier name; vector<Param> params; shared_ptr<Type> return_type_opt; shared_ptr<Block> body; Span span; optional<DocList> doc_opt; };
//     struct TransitionDecl { Visibility vis; Identifier name; vector<Param> params; Identifier target_state; shared_ptr<Block> body; Span span; optional<DocList> doc_opt; };
//     using StateMember = variant<StateFieldDecl, StateMethodDecl, TransitionDecl>;
//     struct StateBlock { Identifier name; vector<StateMember> members; Span span; optional<DocList> doc_opt; };
//     struct ModalDecl { Visibility vis; Identifier name; optional<GenericParams> generic_params; vector<ClassPath> implements; optional<WhereClause> where_clause; optional<TypeInvariant> invariant; vector<StateBlock> states; Span span; DocList doc; };
//
// 12. CLASS DECLARATION (Lines 1196-1259)
//     ────────────────────────────────────────────────────────────────────────
//     struct ClassFieldDecl { Visibility vis; bool key_boundary; Identifier name; shared_ptr<Type> type; Span span; optional<DocList> doc_opt; };
//     struct ClassMethodDecl { Visibility vis; bool static_dispatch_only; Identifier name; Receiver receiver; vector<Param> params; shared_ptr<Type> return_type_opt; shared_ptr<Block> body_opt; Span span; optional<DocList> doc_opt; };
//     struct AssociatedTypeDecl { Visibility vis; Identifier name; shared_ptr<Type> default_type; Span span; optional<DocList> doc_opt; };
//     struct AbstractFieldDecl { Visibility vis; bool key_boundary; Identifier name; shared_ptr<Type> type; Span span; optional<DocList> doc_opt; };
//     struct AbstractStateDecl { Visibility vis; Identifier name; vector<AbstractFieldDecl> fields; Span span; optional<DocList> doc_opt; };
//     using ClassItem = variant<ClassFieldDecl, ClassMethodDecl, AssociatedTypeDecl, AbstractFieldDecl, AbstractStateDecl>;
//     struct ClassDecl { Visibility vis; bool modal; Identifier name; optional<GenericParams> generic_params; vector<ClassPath> supers; optional<WhereClause> where_clause; vector<ClassItem> items; Span span; DocList doc; };
//
// 13. TYPE ALIAS DECLARATION (Lines 1261-1269)
//     ────────────────────────────────────────────────────────────────────────
//     struct TypeAliasDecl {
//       Visibility vis;
//       Identifier name;
//       optional<GenericParams> generic_params;
//       shared_ptr<Type> type;
//       optional<WhereClause> where_clause;
//       Span span;
//       DocList doc;
//     };
//
// 14. ERROR ITEM (Lines 1271-1273)
//     ────────────────────────────────────────────────────────────────────────
//     struct ErrorItem { Span span; };
//
// 15. ASTITEM VARIANT (Lines 1275-1285)
//     ────────────────────────────────────────────────────────────────────────
//     using ASTItem = variant<UsingDecl, ImportDecl, ExternBlock, StaticDecl,
//                              ProcedureDecl, RecordDecl, EnumDecl, ModalDecl,
//                              ClassDecl, TypeAliasDecl, ErrorItem>;
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   From ast_fwd.h:
//     - All basic aliases (Identifier, Path, TypePath, etc.)
//     - ExprPtr, PatternPtr for contract predicates
//
//   From ast_enums.h:
//     - Visibility, Mutability, ReceiverPerm, ParamMode, Variance
//     - ForeignContractKind, ContractIntrinsicKind
//
//   From ast_stmts.h:
//     - Binding, Block
//
//   From ast_attributes.h:
//     - AttributeList
//
//   From 00_core/span.h:
//     - core::Span
//
//   From lexer:
//     - Token, DocList
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. This is the second-largest AST file. Consider splitting:
//      - items_basic.h: UsingDecl, ImportDecl, StaticDecl, TypeAliasDecl
//      - items_procedure.h: ProcedureDecl, ExternBlock, Param, Receiver
//      - items_record.h: RecordDecl, FieldDecl, MethodDecl
//      - items_enum.h: EnumDecl, VariantDecl
//      - items_modal.h: ModalDecl, StateBlock, TransitionDecl
//      - items_class.h: ClassDecl, ClassItem variants
//      - items_generic.h: GenericParams, WhereClause, TypeParam
//      - items_contract.h: ContractClause, TypeInvariant
//
//   2. Many declaration types share common fields:
//      - attrs, vis, name, span, doc
//      Consider a DeclBase struct for shared fields.
//
//   3. FieldDecl is used by RecordDecl, VariantPayloadRecord,
//      and StateBlock. Consider a shared field type.
//
//   4. The key_boundary field (for key system) is present on
//      FieldDecl, StateFieldDecl, ClassFieldDecl, AbstractFieldDecl.
//      This is a C0X extension.
//
//   5. Contract-related types (ContractClause, TypeInvariant)
//      could move to a contracts.h file in the AST directory.
//
//   6. Generic-related types (GenericParams, WhereClause)
//      could move to a generics.h file in the AST directory.
//
// ===========================================================================

// TODO: Migrate declaration definitions from ast.h lines 854-1285


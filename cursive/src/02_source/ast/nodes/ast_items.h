// ===========================================================================
// ast_items.h - Declaration/Item AST Node Definitions
// ===========================================================================
//
// This file contains all struct definitions for top-level declarations (items)
// in the Cursive grammar, plus the ASTItem variant and supporting types.
//
// SPEC REFERENCE: CursiveSpecification.md Section 3.3.2.5 - Declaration Nodes
//
// ASTItem variants:
//   UsingDecl     - using declarations (using path::{items})
//   ImportDecl    - import declarations (import path as alias)
//   ExternBlock   - extern block with ABI (extern "C" { })
//   StaticDecl    - static/const declarations (public let X: T = v)
//   ProcedureDecl - procedure declarations
//   RecordDecl    - record type declarations
//   EnumDecl      - enum type declarations
//   ModalDecl     - modal type declarations
//   ClassDecl     - class/interface declarations
//   TypeAliasDecl - type alias declarations
//   ErrorItem     - parse-error sentinel item
//
// Supporting structures:
//   GenericParams, TypeParam, TypeBound, WhereClause, WherePredicate
//   ContractClause, ForeignContractClause, TypeInvariant
//   Param, Receiver (ReceiverShorthand, ReceiverExplicit)
//   FieldDecl, MethodDecl, RecordMember
//   VariantDecl, VariantPayload (VariantPayloadTuple, VariantPayloadRecord)
//   StateBlock, StateMember, StateFieldDecl, StateMethodDecl, TransitionDecl
//   ClassItem, ClassFieldDecl, ClassMethodDecl, AssociatedTypeDecl,
//   AbstractFieldDecl, AbstractStateDecl
//   ExternAbi (ExternAbiString, ExternAbiIdent), ExternItem, ExternProcDecl
//   UsingClause (UsingItem, UsingList, UsingWildcard), UsingSpec
//
// ===========================================================================

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "cursive/src/02_source/ast/ast_common.h"
#include "cursive/src/02_source/ast/nodes/ast_attributes.h"
#include "cursive/src/02_source/ast/nodes/ast_stmts.h"

namespace cursive::ast
{

    // ===========================================================================
    // Parameter Types
    // ===========================================================================

    // Procedure parameter: mode? name: type
    struct Param
    {
        std::optional<ParamMode> mode;
        Identifier name;
        std::optional<SpliceIdentNode> name_splice_opt;
        TypePtr type;
        core::Span span;
    };

    // ===========================================================================
    // Using Declaration Types
    // ===========================================================================

    // Single item in a using list: name or name as alias
    struct UsingSpec
    {
        Identifier name;
        std::optional<Identifier> alias_opt;
    };

    // Single imported item: using module::item or using module::item as alias
    struct UsingItem
    {
        ModulePath module_path;
        Identifier name;
        std::optional<Identifier> alias_opt;
    };

    // Using list: using module::{item1, item2}
    struct UsingList
    {
        ModulePath module_path;
        std::vector<UsingSpec> specs;
    };

    // Using wildcard: using module::*
    struct UsingWildcard
    {
        ModulePath module_path;
    };

    // Using clause is one of three forms
    using UsingClause = std::variant<UsingItem, UsingList, UsingWildcard>;

    // Using declaration: using module::item | using module::item as alias | using module::{items} | using module::*
    struct UsingDecl
    {
        AttrOpt attrs_opt;
        Visibility vis;
        UsingClause clause;
        core::Span span;
        DocList doc;
    };

    // ===========================================================================
    // Import Declaration (C0X Extension)
    // ===========================================================================

    // Import declaration for cross-assembly imports
    // import path;
    // import path as alias;
    struct ImportDecl
    {
        AttrOpt attrs_opt;
        Visibility vis;
        Path path;
        std::optional<Identifier> alias_opt;
        core::Span span;
        DocList doc;
    };

    // ===========================================================================
    // Static Declaration
    // ===========================================================================

    // Static/const declaration: public let NAME: Type = value
    struct StaticDecl
    {
        AttrOpt attrs_opt;
        Visibility vis;
        Mutability mut;
        Binding binding;
        core::Span span;
        DocList doc;
    };

    // ===========================================================================
    // Generics System (C0X Extension)
    // ===========================================================================

    // Type bound: T <: Class or T <: Class<Args...>
    struct TypeBound
    {
        ClassPath class_path;
        std::vector<TypePtr> generic_args;
    };

    // Type parameter: T <: Clone = DefaultType
    struct TypeParam
    {
        Identifier name;
        std::vector<TypeBound> bounds; // <: bound list
        TypePtr default_type;          // optional = default (may be null)
        core::Span span;
    };

    // Generic parameters list: <T; U <: Class>
    // Note: Parameters use semicolons as separators
    struct GenericParams
    {
        std::vector<TypeParam> params;
        core::Span span;
    };

    // Generic arguments list: <Foo, Bar>
    // Note: Arguments use commas as separators
    struct GenericArgs
    {
        std::vector<TypePtr> args;
        core::Span span;
    };

    // Where clause predicate: Bitcopy(T)
    struct WherePredicate
    {
        Identifier predicate; // Predicate name (Bitcopy, Clone, Drop, FfiSafe)
        TypePtr type;         // Type being constrained
        core::Span span;
    };

    // Where clause: where Bitcopy(T), Clone(U)
    struct WhereClause
    {
        std::vector<WherePredicate> predicates;
        core::Span span;
    };

    // ===========================================================================
    // Contract System (C0X Extension)
    // ===========================================================================

    // Contract clause: |: pre => post
    struct ContractClause
    {
        ExprPtr precondition;  // pre (may be null for => post form)
        ExprPtr postcondition; // post (may be null for |: pre form)
        core::Span span;
    };

    // ForeignContractKind is defined in ast_enums.h

    // Foreign contract clause: |: @foreign_assumes(...) or |: @foreign_ensures(...)
    struct ForeignContractClause
    {
        ForeignContractKind kind;
        std::vector<ExprPtr> predicates;
        core::Span span;
    };

    // ContractIntrinsicKind is defined in ast_enums.h

    // Contract intrinsic expression: @result or @entry(expr)
    struct ContractIntrinsicExpr
    {
        ContractIntrinsicKind kind;
        ExprPtr expr; // only for @entry(expr), null for @result
    };

    // Type invariant: where { predicate }
    struct TypeInvariant
    {
        ExprPtr predicate;
        core::Span span;
    };

    // ===========================================================================
    // Procedure Declaration
    // ===========================================================================

    // Procedure declaration
    // public procedure name<T>(params) -> ReturnType |: contract { body }
    struct ProcedureDecl
    {
        AttributeList attrs;
        Visibility vis;
        Identifier name;
        std::optional<GenericParams> generic_params;
        std::optional<WhereClause> predicate_clause_opt;
        std::vector<Param> params;
        TypePtr return_type_opt; // may be null if not specified
        std::optional<ContractClause> contract;
        BlockPtr body;
        core::Span span;
        DocList doc;
    };

    // Compile-time procedure declaration
    struct ComptimeProcedureDecl
    {
        AttributeList attrs;
        Visibility vis;
        Identifier name;
        std::optional<GenericParams> generic_params;
        std::vector<Param> params;
        TypePtr return_type_opt;
        std::optional<ContractClause> contract;
        BlockPtr body;
        core::Span span;
        DocList doc;
    };

    // ===========================================================================
    // Extern Block Declarations
    // ===========================================================================

    // Extern ABI string: extern "C"
    struct ExternAbiString
    {
        Token literal;
    };

    // Extern ABI identifier: extern ABI_ID
    struct ExternAbiIdent
    {
        Identifier name;
    };

    // Extern ABI specifier
    using ExternAbi = std::variant<ExternAbiString, ExternAbiIdent>;

    // Extern procedure declaration (inside extern block)
    struct ExternProcDecl
    {
        AttributeList attrs;
        Visibility vis;
        Identifier name;
        std::optional<GenericParams> generic_params;
        std::optional<WhereClause> where_clause;
        std::vector<Param> params;
        TypePtr return_type_opt;
        std::optional<ContractClause> contract;
        std::optional<std::vector<ForeignContractClause>> foreign_contracts_opt;
        core::Span span;
        DocList doc;
    };

    // Extern item (currently only procedures)
    using ExternItem = std::variant<ExternProcDecl>;

    // Extern block: extern "C" { procedures... }
    struct ExternBlock
    {
        AttrOpt attrs_opt;
        Visibility vis;
        std::optional<ExternAbi> abi_opt;
        std::vector<ExternItem> items;
        core::Span span;
        DocList doc;
    };

    // ===========================================================================
    // Record Declaration
    // ===========================================================================

    // Field declaration in a record
    struct FieldDecl
    {
        AttributeList attrs;
        Visibility vis;
        bool key_boundary = false; // # boundary marker for key system
        Identifier name;
        TypePtr type;
        ExprPtr init_opt; // optional default initializer
        core::Span span;
        std::optional<DocList> doc_opt;
    };

    // Method declaration in a record
    struct MethodDecl
    {
        AttributeList attrs;
        Visibility vis;
        bool override_flag = false; // override keyword present
        Identifier name;
        std::optional<GenericParams> generic_params;
        Receiver receiver;
        std::vector<Param> params;
        TypePtr return_type_opt;
        std::optional<ContractClause> contract;
        BlockPtr body;
        core::Span span;
        std::optional<DocList> doc_opt;
    };

    // Record member is a field, method, or associated type binding
    using RecordMember = std::variant<FieldDecl, MethodDecl, AssociatedTypeDecl>;

    // Record declaration
    // public record Name<T> <: Class1, Class2 where Bitcopy(T) where { invariant } { members }
    struct RecordDecl
    {
        AttributeList attrs;
        Visibility vis;
        Identifier name;
        std::optional<GenericParams> generic_params;
        std::optional<WhereClause> predicate_clause_opt;
        std::vector<ClassPath> implements; // <: implemented classes
        std::vector<RecordMember> members;
        std::optional<TypeInvariant> invariant_opt;
        core::Span span;
        DocList doc;
    };

    // ===========================================================================
    // Enum Declaration
    // ===========================================================================

    // Tuple-style variant payload: Variant(T1, T2)
    struct VariantPayloadTuple
    {
        std::vector<TypePtr> elements;
    };

    // Record-style variant payload: Variant{ field1: T1, field2: T2 }
    struct VariantPayloadRecord
    {
        std::vector<FieldDecl> fields;
    };

    // Variant payload is either tuple or record style
    using VariantPayload = std::variant<VariantPayloadTuple, VariantPayloadRecord>;

    // Enum variant declaration
    struct VariantDecl
    {
        Identifier name;
        std::optional<VariantPayload> payload_opt;
        std::optional<Token> discriminant_opt; // = discriminant_value
        core::Span span;
        std::optional<DocList> doc_opt;
    };

    // Enum declaration
    // public enum Name<T> <: Class where Bitcopy(T) where { invariant } { variants }
    struct EnumDecl
    {
        AttributeList attrs;
        Visibility vis;
        Identifier name;
        std::optional<GenericParams> generic_params;
        std::optional<WhereClause> predicate_clause_opt;
        std::vector<ClassPath> implements;
        std::vector<VariantDecl> variants;
        std::optional<TypeInvariant> invariant_opt;
        core::Span span;
        DocList doc;
    };

    // ===========================================================================
    // Modal Declaration
    // ===========================================================================

    // Field declaration in a modal state block
    struct StateFieldDecl
    {
        AttributeList attrs;
        Visibility vis;
        bool key_boundary = false; // # boundary marker for key system
        Identifier name;
        TypePtr type;
        core::Span span;
        std::optional<DocList> doc_opt;
    };

    // Method declaration in a modal state block
    struct StateMethodDecl
    {
        AttributeList attrs;
        Visibility vis;
        Identifier name;
        std::optional<GenericParams> generic_params;
        Receiver receiver;
        std::vector<Param> params;
        TypePtr return_type_opt;
        std::optional<ContractClause> contract;
        BlockPtr body;
        core::Span span;
        std::optional<DocList> doc_opt;
    };

    // Transition declaration in a modal state block
    struct TransitionDecl
    {
        AttributeList attrs;
        Visibility vis;
        Identifier name;
        std::vector<Param> params;
        Identifier target_state; // -> @TargetState
        BlockPtr body;
        core::Span span;
        std::optional<DocList> doc_opt;
    };

    // State member is a field, method, or transition
    using StateMember = std::variant<StateFieldDecl, StateMethodDecl, TransitionDecl>;

    // State block in a modal type
    struct StateBlock
    {
        Identifier name; // State name (e.g., "Connected")
        std::vector<StateMember> members;
        core::Span span;
        std::optional<DocList> doc_opt;
    };

    // Modal type declaration
    // public modal Name<T> <: Class where Bitcopy(T) where { invariant } { states }
    struct ModalDecl
    {
        AttributeList attrs;
        Visibility vis;
        Identifier name;
        std::optional<GenericParams> generic_params;
        std::optional<WhereClause> predicate_clause_opt;
        std::vector<ClassPath> implements;
        std::vector<StateBlock> states;
        std::optional<TypeInvariant> invariant_opt;
        core::Span span;
        DocList doc;
    };

    // ===========================================================================
    // Class Declaration
    // ===========================================================================

    // Field declaration in a class
    struct ClassFieldDecl
    {
        AttributeList attrs;
        Visibility vis;
        bool key_boundary = false; // # boundary marker for key system
        Identifier name;
        TypePtr type;
        core::Span span;
        std::optional<DocList> doc_opt;
    };

    // Method declaration in a class
    struct ClassMethodDecl
    {
        AttributeList attrs;
        Visibility vis;
        Identifier name;
        std::optional<GenericParams> generic_params;
        Receiver receiver;
        std::vector<Param> params;
        TypePtr return_type_opt;
        std::optional<ContractClause> contract;
        BlockPtr body_opt; // may be null for abstract methods
        core::Span span;
        std::optional<DocList> doc_opt;
    };

    // Associated type declaration in a class
    struct AssociatedTypeDecl
    {
        AttributeList attrs;
        Visibility vis;
        Identifier name;
        TypePtr default_type; // optional default (may be null)
        core::Span span;
        std::optional<DocList> doc_opt;
    };

    // Abstract field declaration for modal classes
    struct AbstractFieldDecl
    {
        AttributeList attrs;
        Visibility vis;
        bool key_boundary = false;
        Identifier name;
        TypePtr type;
        core::Span span;
        std::optional<DocList> doc_opt;
    };

    // Abstract state declaration for modal classes
    struct AbstractStateDecl
    {
        AttributeList attrs;
        Visibility vis;
        Identifier name;
        std::vector<AbstractFieldDecl> fields;
        core::Span span;
        std::optional<DocList> doc_opt;
    };

    // Class item is one of several kinds
    using ClassItem = std::variant<
        ClassFieldDecl,
        ClassMethodDecl,
        AssociatedTypeDecl,
        AbstractFieldDecl,
        AbstractStateDecl>;

    // Class declaration
    // public class Name<T> <: Super where Bitcopy(T) { items }
    // public modal class Name<T> <: Super where Bitcopy(T) { items }
    struct ClassDecl
    {
        AttributeList attrs;
        Visibility vis;
        bool modal = false; // modal class flag
        Identifier name;
        std::optional<GenericParams> generic_params;
        std::optional<WhereClause> predicate_clause_opt;
        std::vector<ClassPath> supers; // <: super classes
        std::vector<ClassItem> items;
        core::Span span;
        DocList doc;
    };

    // ===========================================================================
    // Type Alias Declaration
    // ===========================================================================

    // Type alias declaration
    // public type Name<T> = Type where Bitcopy(T)
    struct TypeAliasDecl
    {
        AttributeList attrs;
        Visibility vis;
        Identifier name;
        std::optional<GenericParams> generic_params;
        std::optional<WhereClause> predicate_clause_opt;
        TypePtr type;
        core::Span span;
        DocList doc;
    };

    enum class DeriveClauseKind
    {
        Emits,
        Requires,
    };

    struct DeriveClause
    {
        DeriveClauseKind kind;
        Identifier name;
        core::Span span;
    };

    // Derive target declaration
    struct DeriveTargetDecl
    {
        Identifier name;
        std::vector<DeriveClause> contract_opt;
        BlockPtr body;
        core::Span span;
        DocList doc;
    };

    // ===========================================================================
    // Error Item
    // ===========================================================================

    // Sentinel item for parse errors in the item list
    struct ErrorItem
    {
        core::Span span;
        DocList doc;
    };

    // ===========================================================================
    // ASTItem Variant
    // ===========================================================================

    // ASTItem is the variant holding all top-level declaration types.
    using ASTItem = std::variant<
        UsingDecl,
        ImportDecl,
        ExternBlock,
        StaticDecl,
        ProcedureDecl,
        ComptimeProcedureDecl,
        RecordDecl,
        EnumDecl,
        ModalDecl,
        ClassDecl,
        TypeAliasDecl,
        DeriveTargetDecl,
        ErrorItem>;

    inline const AttributeList& AttrListOf(const UsingDecl& item) {
        return AttrListOf(item.attrs_opt);
    }
    inline const AttributeList& AttrListOf(const ImportDecl& item) {
        return AttrListOf(item.attrs_opt);
    }
    inline const AttributeList& AttrListOf(const ExternBlock& item) {
        return AttrListOf(item.attrs_opt);
    }
    inline const AttributeList& AttrListOf(const StaticDecl& item) {
        return AttrListOf(item.attrs_opt);
    }
    inline const AttributeList& AttrListOf(const ProcedureDecl& item) { return item.attrs; }
    inline const AttributeList& AttrListOf(const ComptimeProcedureDecl& item) { return item.attrs; }
    inline const AttributeList& AttrListOf(const RecordDecl& item) { return item.attrs; }
    inline const AttributeList& AttrListOf(const EnumDecl& item) { return item.attrs; }
    inline const AttributeList& AttrListOf(const ModalDecl& item) { return item.attrs; }
    inline const AttributeList& AttrListOf(const ClassDecl& item) { return item.attrs; }
    inline const AttributeList& AttrListOf(const TypeAliasDecl& item) { return item.attrs; }
    inline const AttributeList& AttrListOf(const DeriveTargetDecl&) {
        return EmptyAttributeList();
    }
    inline const AttributeList& AttrListOf(const ErrorItem&) {
        return EmptyAttributeList();
    }

    inline const AttributeList& AttrListOf(const ASTItem& item) {
        return std::visit(
            [](const auto& entry) -> const AttributeList& { return AttrListOf(entry); },
            item);
    }

    inline AttributeList AttrByName(const ASTItem& item, std::string_view name) {
        return AttrByName(AttrListOf(item), name);
    }

    inline std::vector<Identifier> DeriveReqs(const DeriveTargetDecl& dt) {
        std::vector<Identifier> out;
        for (const auto& clause : dt.contract_opt) {
            if (clause.kind == DeriveClauseKind::Requires) {
                out.push_back(clause.name);
            }
        }
        return out;
    }

    inline std::vector<Identifier> DeriveEmits(const DeriveTargetDecl& dt) {
        std::vector<Identifier> out;
        for (const auto& clause : dt.contract_opt) {
            if (clause.kind == DeriveClauseKind::Emits) {
                out.push_back(clause.name);
            }
        }
        return out;
    }

} // namespace cursive::ast

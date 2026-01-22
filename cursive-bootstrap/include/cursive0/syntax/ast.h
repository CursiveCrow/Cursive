#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "cursive0/core/span.h"
#include "cursive0/syntax/lexer.h"

namespace cursive0::syntax {

using DocList = std::vector<DocComment>;
using Identifier = std::string;
using Path = std::vector<Identifier>;
using ModulePath = Path;
using TypePath = Path;
using ClassPath = Path;

struct Expr;
struct Type;
struct Pattern;
struct Block;

using ExprPtr = std::shared_ptr<Expr>;
using PatternPtr = std::shared_ptr<Pattern>;

enum class ParamMode {
  Move,
};

enum class TypePerm {
  Const,
  Unique,
  Shared,
};

enum class RawPtrQual {
  Imm,
  Mut,
};

enum class StringState {
  Managed,
  View,
};

enum class BytesState {
  Managed,
  View,
};

// C0X Extension: Attribute System AST nodes

// Attribute argument value
struct AttributeArg {
  std::optional<Identifier> key;  // Named arg: key = value
  std::variant<Token, ExprPtr> value;  // Token literal or expression
};

// Single attribute: [[name]] or [[name(args)]]
struct AttributeItem {
  Identifier name;
  std::vector<AttributeArg> args;
  core::Span span;
};

// Attribute list (multiple attributes)
using AttributeList = std::vector<AttributeItem>;

enum class PtrState {
  Valid,
  Null,
  Expired,
};

struct TypePrim {
  Identifier name;
};

struct TypePermType {
  TypePerm perm;
  std::shared_ptr<Type> base;
};

struct TypeUnion {
  std::vector<std::shared_ptr<Type>> types;
};

struct TypeFuncParam {
  std::optional<ParamMode> mode;
  std::shared_ptr<Type> type;
};

struct TypeFunc {
  std::vector<TypeFuncParam> params;
  std::shared_ptr<Type> ret;
};

struct TypeTuple {
  std::vector<std::shared_ptr<Type>> elements;
};

struct TypeArray {
  std::shared_ptr<Type> element;
  ExprPtr length;
};

struct TypeSlice {
  std::shared_ptr<Type> element;
};

struct TypePtr {
  std::shared_ptr<Type> element;
  std::optional<PtrState> state;
};

struct TypeRawPtr {
  RawPtrQual qual;
  std::shared_ptr<Type> element;
};

struct TypeString {
  std::optional<StringState> state;
};

struct TypeBytes {
  std::optional<BytesState> state;
};

struct TypeDynamic {
  TypePath path;
};

struct TypeModalState {
  TypePath path;
  Identifier state;
};

struct TypePathType {
  TypePath path;
  std::vector<std::shared_ptr<Type>> generic_args;  // C0X Extension: Foo<T, U>
};

using TypeNode = std::variant<TypePrim,
                              TypePermType,
                              TypeUnion,
                              TypeFunc,
                              TypeTuple,
                              TypeArray,
                              TypeSlice,
                              TypePtr,
                              TypeRawPtr,
                              TypeString,
                              TypeBytes,
                              TypeDynamic,
                              TypeModalState,
                              TypePathType>;

struct Type {
  core::Span span;
  TypeNode node;
};

struct ErrorExpr {};

struct LiteralExpr {
  Token literal;
};

struct IdentifierExpr {
  Identifier name;
};

struct QualifiedNameExpr {
  ModulePath path;
  Identifier name;
};

struct Arg {
  bool moved = false;
  ExprPtr value;
  core::Span span;
};

struct ParenArgs {
  std::vector<Arg> args;
};

struct FieldInit {
  Identifier name;
  ExprPtr value;
  core::Span span;
};

struct BraceArgs {
  std::vector<FieldInit> fields;
};

using ApplyArgs = std::variant<ParenArgs, BraceArgs>;

struct QualifiedApplyExpr {
  ModulePath path;
  Identifier name;
  ApplyArgs args;
};

struct PathExpr {
  ModulePath path;
  Identifier name;
};

struct EnumPayloadParen {
  std::vector<ExprPtr> elements;
};

struct EnumPayloadBrace {
  std::vector<FieldInit> fields;
};

using EnumPayload = std::variant<EnumPayloadParen, EnumPayloadBrace>;

struct EnumLiteralExpr {
  Path path;
  std::optional<EnumPayload> payload_opt;
};

enum class RangeKind {
  To,
  ToInclusive,
  Full,
  From,
  Exclusive,
  Inclusive,
};

struct RangeExpr {
  RangeKind kind;
  ExprPtr lhs;
  ExprPtr rhs;
};

struct BinaryExpr {
  Identifier op;
  ExprPtr lhs;
  ExprPtr rhs;
};

struct CastExpr {
  ExprPtr value;
  std::shared_ptr<Type> type;
};

struct UnaryExpr {
  Identifier op;
  ExprPtr value;
};

struct DerefExpr {
  ExprPtr value;
};

struct AddressOfExpr {
  ExprPtr place;
};

struct MoveExpr {
  ExprPtr place;
};

struct AllocExpr {
  std::optional<Identifier> region_opt;
  ExprPtr value;
};

struct PtrNullExpr {};

struct TupleExpr {
  std::vector<ExprPtr> elements;
};

struct ArrayExpr {
  std::vector<ExprPtr> elements;
};

struct ModalStateRef {
  TypePath path;
  Identifier state;
};

struct RecordExpr {
  std::variant<TypePath, ModalStateRef> target;
  std::vector<FieldInit> fields;
};

struct IfExpr {
  ExprPtr cond;
  ExprPtr then_expr;
  ExprPtr else_expr;
};

struct LiteralPattern {
  Token literal;
};

struct WildcardPattern {};

struct IdentifierPattern {
  Identifier name;
};

struct TypedPattern {
  Identifier name;
  std::shared_ptr<Type> type;
};

struct TuplePattern {
  std::vector<PatternPtr> elements;
};

struct FieldPattern {
  Identifier name;
  PatternPtr pattern_opt;
  core::Span span;
};

struct RecordPattern {
  TypePath path;
  std::vector<FieldPattern> fields;
};

struct TuplePayloadPattern {
  std::vector<PatternPtr> elements;
};

struct RecordPayloadPattern {
  std::vector<FieldPattern> fields;
};

using EnumPayloadPattern = std::variant<TuplePayloadPattern, RecordPayloadPattern>;

struct EnumPattern {
  TypePath path;
  Identifier name;
  std::optional<EnumPayloadPattern> payload_opt;
};

struct ModalRecordPayload {
  std::vector<FieldPattern> fields;
};

struct ModalPattern {
  Identifier state;
  std::optional<ModalRecordPayload> fields_opt;
};

struct RangePattern {
  RangeKind kind;
  PatternPtr lo;
  PatternPtr hi;
};

using PatternNode =
    std::variant<LiteralPattern,
                 WildcardPattern,
                 IdentifierPattern,
                 TypedPattern,
                 TuplePattern,
                 RecordPattern,
                 EnumPattern,
                 ModalPattern,
                 RangePattern>;

struct Pattern {
  core::Span span;
  PatternNode node;
};

struct MatchArm {
  std::shared_ptr<Pattern> pattern;
  ExprPtr guard_opt;
  ExprPtr body;
  core::Span span;
};

struct MatchExpr {
  ExprPtr value;
  std::vector<MatchArm> arms;
};

struct LoopInfiniteExpr {
  std::shared_ptr<Block> body;
};

struct LoopConditionalExpr {
  ExprPtr cond;
  std::shared_ptr<Block> body;
};

struct LoopIterExpr {
  std::shared_ptr<Pattern> pattern;
  std::shared_ptr<Type> type_opt;
  ExprPtr iter;
  std::shared_ptr<Block> body;
};

struct BlockExpr {
  std::shared_ptr<Block> block;
};

struct UnsafeBlockExpr {
  std::shared_ptr<Block> block;
};

struct TransmuteExpr {
  std::shared_ptr<Type> from;
  std::shared_ptr<Type> to;
  ExprPtr value;
};

struct FieldAccessExpr {
  ExprPtr base;
  Identifier name;
};

struct TupleAccessExpr {
  ExprPtr base;
  Token index;
};

struct IndexAccessExpr {
  ExprPtr base;
  ExprPtr index;
};

struct CallExpr {
  ExprPtr callee;
  std::vector<Arg> args;
};

struct MethodCallExpr {
  ExprPtr receiver;
  Identifier name;
  std::vector<Arg> args;
};

struct PropagateExpr {
  ExprPtr value;
};

// C0X Extension: Contract intrinsic expressions (@result, @entry)
struct ResultExpr {
  // @result - represents the return value in postconditions
};

struct EntryExpr {
  // @entry(expr) - represents value at procedure entry
  ExprPtr expr;
};

using ExprNode = std::variant<ErrorExpr,
                              LiteralExpr,
                              IdentifierExpr,
                              QualifiedNameExpr,
                              QualifiedApplyExpr,
                              PathExpr,
                              RangeExpr,
                              BinaryExpr,
                              CastExpr,
                              UnaryExpr,
                              DerefExpr,
                              AddressOfExpr,
                              MoveExpr,
                              AllocExpr,
                              PtrNullExpr,
                              TupleExpr,
                              ArrayExpr,
                              RecordExpr,
                              EnumLiteralExpr,
                              IfExpr,
                              MatchExpr,
                              LoopInfiniteExpr,
                              LoopConditionalExpr,
                              LoopIterExpr,
                              BlockExpr,
                              UnsafeBlockExpr,
                              TransmuteExpr,
                              FieldAccessExpr,
                              TupleAccessExpr,
                              IndexAccessExpr,
                              CallExpr,
                              MethodCallExpr,
                              PropagateExpr,
                              ResultExpr,
                              EntryExpr>;

struct Expr {
  core::Span span;
  ExprNode node;
};

struct Binding {
  std::shared_ptr<Pattern> pat;
  std::shared_ptr<Type> type_opt;
  Token op;
  std::shared_ptr<Expr> init;
  core::Span span;
};

struct LetStmt {
  Binding binding;
  core::Span span;
};

struct VarStmt {
  Binding binding;
  core::Span span;
};

struct ShadowLetStmt {
  Identifier name;
  std::shared_ptr<Type> type_opt;
  std::shared_ptr<Expr> init;
  core::Span span;
};

struct ShadowVarStmt {
  Identifier name;
  std::shared_ptr<Type> type_opt;
  std::shared_ptr<Expr> init;
  core::Span span;
};

struct AssignStmt {
  ExprPtr place;
  ExprPtr value;
  core::Span span;
};

struct CompoundAssignStmt {
  ExprPtr place;
  Identifier op;
  ExprPtr value;
  core::Span span;
};

struct ExprStmt {
  ExprPtr value;
  core::Span span;
};

struct DeferStmt {
  std::shared_ptr<Block> body;
  core::Span span;
};

struct RegionStmt {
  ExprPtr opts_opt;
  std::optional<Identifier> alias_opt;
  std::shared_ptr<Block> body;
  core::Span span;
};

struct FrameStmt {
  std::optional<Identifier> target_opt;
  std::shared_ptr<Block> body;
  core::Span span;
};

struct ReturnStmt {
  ExprPtr value_opt;
  core::Span span;
};

struct ResultStmt {
  ExprPtr value;
  core::Span span;
};

struct BreakStmt {
  ExprPtr value_opt;
  core::Span span;
};

struct ContinueStmt {
  core::Span span;
};

struct UnsafeBlockStmt {
  std::shared_ptr<Block> body;
  core::Span span;
};

struct ErrorStmt {
  core::Span span;
};

// C0X Extension: Key System AST nodes
enum class KeyMode {
  Read,
  Write,
};

enum class KeyBlockMod {
  Dynamic,
  Speculative,
  Release,
};

struct KeySegField {
  bool marked;  // # boundary marker
  Identifier name;
};

struct KeySegIndex {
  bool marked;  // # boundary marker
  ExprPtr expr;
};

using KeySeg = std::variant<KeySegField, KeySegIndex>;

struct KeyPathExpr {
  Identifier root;
  std::vector<KeySeg> segs;
  core::Span span;
};

struct KeyBlockStmt {
  std::vector<KeyPathExpr> paths;
  std::vector<KeyBlockMod> mods;
  std::optional<KeyMode> mode;
  std::shared_ptr<Block> body;
  core::Span span;
};

using Stmt = std::variant<LetStmt,
                          VarStmt,
                          ShadowLetStmt,
                          ShadowVarStmt,
                          AssignStmt,
                          CompoundAssignStmt,
                          ExprStmt,
                          DeferStmt,
                          RegionStmt,
                          FrameStmt,
                          ReturnStmt,
                          ResultStmt,
                          BreakStmt,
                          ContinueStmt,
                          UnsafeBlockStmt,
                          KeyBlockStmt,
                          ErrorStmt>;

struct Block {
  std::vector<Stmt> stmts;
  ExprPtr tail_opt;
  core::Span span;
};

enum class Visibility {
  Public,
  Internal,
  Private,
  Protected,
};

enum class Mutability {
  Let,
  Var,
};

struct Param {
  std::optional<ParamMode> mode;
  Identifier name;
  std::shared_ptr<Type> type;
  core::Span span;
};

enum class ReceiverPerm {
  Const,
  Unique,
  Shared,
};

struct ReceiverShorthand {
  ReceiverPerm perm;
};

struct ReceiverExplicit {
  std::optional<ParamMode> mode_opt;
  std::shared_ptr<Type> type;
};

using Receiver = std::variant<ReceiverShorthand, ReceiverExplicit>;

struct UsingSpec {
  Identifier name;
  std::optional<Identifier> alias_opt;
};

struct UsingPath {
  ModulePath path;
  std::optional<Identifier> alias_opt;
};

struct UsingList {
  ModulePath module_path;
  std::vector<UsingSpec> specs;
};

using UsingClause = std::variant<UsingPath, UsingList>;

struct UsingDecl {
  Visibility vis;
  UsingClause clause;
  core::Span span;
  DocList doc;
};

// C0X Extension: Import declaration for cross-assembly imports
// import path;
// import path as alias;
// import path::{item1, item2};
struct ImportDecl {
  Visibility vis;
  Path path;
  std::optional<Identifier> alias;  // import path as alias
  std::vector<Identifier> items;    // import path::{items} (empty = import all)
  core::Span span;
  DocList doc;
};

struct StaticDecl {
  Visibility vis;
  Mutability mut;
  Binding binding;
  core::Span span;
  DocList doc;
};

// C0X Extension: Generics AST nodes
// Variance of a type parameter
enum class Variance {
  Covariant,      // +
  Contravariant,  // -
  Invariant,      // neither + nor -
  Bivariant,      // both + and -
};

// Type bound (e.g., T <: Clone + Ord)
struct TypeBound {
  ClassPath class_path;
};

// Type parameter declaration (e.g., T <: Clone = DefaultType)
struct TypeParam {
  Identifier name;
  std::vector<TypeBound> bounds;  // <: bound list
  std::shared_ptr<Type> default_type;  // optional = default
  core::Span span;
};

// Generic parameters list <T; U <: Class>
struct GenericParams {
  std::vector<TypeParam> params;
  core::Span span;
};

// Generic arguments list <Foo, Bar>
struct GenericArgs {
  std::vector<std::shared_ptr<Type>> args;
  core::Span span;
};

// Where clause predicate (e.g., T <: Ord)
struct WherePredicate {
  Identifier type_param;
  std::vector<TypeBound> bounds;
  core::Span span;
};

// Where clause
struct WhereClause {
  std::vector<WherePredicate> predicates;
  core::Span span;
};

// C0X Extension: Contract System AST nodes

// Contract clause: |= pre => post
struct ContractClause {
  ExprPtr precondition;   // pre (may be null for => post form)
  ExprPtr postcondition;  // post (may be null for |= pre form)
  core::Span span;
};

// Contract intrinsic: @result or @entry(expr)
enum class ContractIntrinsicKind {
  Result,  // @result - the return value
  Entry,   // @entry(expr) - value at procedure entry
};

struct ContractIntrinsicExpr {
  ContractIntrinsicKind kind;
  ExprPtr expr;  // only for @entry(expr)
};

// Type invariant: where { predicate }
struct TypeInvariant {
  ExprPtr predicate;
  core::Span span;
};

// Loop invariant: where { predicate }
struct LoopInvariant {
  ExprPtr predicate;
  core::Span span;
};

struct ProcedureDecl {
  AttributeList attrs;  // C0X Extension
  Visibility vis;
  Identifier name;
  std::optional<GenericParams> generic_params;  // C0X Extension
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;
  std::optional<WhereClause> where_clause;  // C0X Extension
  std::optional<ContractClause> contract;  // C0X Extension
  std::shared_ptr<Block> body;
  core::Span span;
  DocList doc;
};

struct FieldDecl {
  AttributeList attrs;  // C0X Extension
  Visibility vis;
  bool key_boundary = false;  // C0X Extension: # boundary marker for key system
  Identifier name;
  std::shared_ptr<Type> type;
  std::shared_ptr<Expr> init_opt;
  core::Span span;
  std::optional<DocList> doc_opt;
};

struct MethodDecl {
  AttributeList attrs;  // C0X Extension
  Visibility vis;
  bool override_flag = false;
  Identifier name;
  Receiver receiver;
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;
  std::optional<ContractClause> contract;  // C0X Extension
  std::shared_ptr<Block> body;
  core::Span span;
  std::optional<DocList> doc_opt;
};

using RecordMember = std::variant<FieldDecl, MethodDecl>;

struct RecordDecl {
  AttributeList attrs;  // C0X Extension
  Visibility vis;
  Identifier name;
  std::optional<GenericParams> generic_params;  // C0X Extension
  std::vector<ClassPath> implements;
  std::optional<WhereClause> where_clause;  // C0X Extension
  std::optional<TypeInvariant> invariant;  // C0X Extension
  std::vector<RecordMember> members;
  core::Span span;
  DocList doc;
};

struct VariantPayloadTuple {
  std::vector<std::shared_ptr<Type>> elements;
};

struct VariantPayloadRecord {
  std::vector<FieldDecl> fields;
};

using VariantPayload = std::variant<VariantPayloadTuple, VariantPayloadRecord>;

struct VariantDecl {
  Identifier name;
  std::optional<VariantPayload> payload_opt;
  std::optional<Token> discriminant_opt;
  core::Span span;
  std::optional<DocList> doc_opt;
};

struct EnumDecl {
  AttributeList attrs;  // C0X Extension
  Visibility vis;
  Identifier name;
  std::optional<GenericParams> generic_params;  // C0X Extension
  std::vector<ClassPath> implements;
  std::optional<WhereClause> where_clause;  // C0X Extension
  std::optional<TypeInvariant> invariant;  // C0X Extension
  std::vector<VariantDecl> variants;
  core::Span span;
  DocList doc;
};

struct StateFieldDecl {
  Visibility vis;
  bool key_boundary = false;  // C0X Extension: # boundary marker for key system
  Identifier name;
  std::shared_ptr<Type> type;
  core::Span span;
  std::optional<DocList> doc_opt;
};

struct StateMethodDecl {
  Visibility vis;
  Identifier name;
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;
  std::shared_ptr<Block> body;
  core::Span span;
  std::optional<DocList> doc_opt;
};

struct TransitionDecl {
  Visibility vis;
  Identifier name;
  std::vector<Param> params;
  Identifier target_state;
  std::shared_ptr<Block> body;
  core::Span span;
  std::optional<DocList> doc_opt;
};

using StateMember = std::variant<StateFieldDecl, StateMethodDecl, TransitionDecl>;

struct StateBlock {
  Identifier name;
  std::vector<StateMember> members;
  core::Span span;
  std::optional<DocList> doc_opt;
};

struct ModalDecl {
  Visibility vis;
  Identifier name;
  std::optional<GenericParams> generic_params;  // C0X Extension
  std::vector<ClassPath> implements;
  std::optional<WhereClause> where_clause;  // C0X Extension
  std::vector<StateBlock> states;
  core::Span span;
  DocList doc;
};

struct ClassFieldDecl {
  Visibility vis;
  bool key_boundary = false;  // C0X Extension: # boundary marker for key system
  Identifier name;
  std::shared_ptr<Type> type;
  core::Span span;
  std::optional<DocList> doc_opt;
};

struct ClassMethodDecl {
  Visibility vis;
  bool static_dispatch_only = false;  // C0X Extension: [[static_dispatch_only]]
  Identifier name;
  Receiver receiver;
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;
  std::shared_ptr<Block> body_opt;
  core::Span span;
  std::optional<DocList> doc_opt;
};

// C0X Extension: Associated type declaration in class
struct AssociatedTypeDecl {
  Visibility vis;
  Identifier name;
  std::shared_ptr<Type> default_type;  // optional default
  core::Span span;
  std::optional<DocList> doc_opt;
};

// C0X Extension: Abstract field declaration for modal classes
struct AbstractFieldDecl {
  Visibility vis;
  bool key_boundary = false;
  Identifier name;
  std::shared_ptr<Type> type;
  core::Span span;
  std::optional<DocList> doc_opt;
};

// C0X Extension: Abstract state declaration for modal classes
struct AbstractStateDecl {
  Visibility vis;
  Identifier name;
  std::vector<AbstractFieldDecl> fields;
  core::Span span;
  std::optional<DocList> doc_opt;
};

using ClassItem = std::variant<ClassFieldDecl, ClassMethodDecl, 
                               AssociatedTypeDecl, AbstractFieldDecl,
                               AbstractStateDecl>;

struct ClassDecl {
  Visibility vis;
  bool modal = false;  // C0X Extension: modal class flag
  Identifier name;
  std::optional<GenericParams> generic_params;  // C0X Extension
  std::vector<ClassPath> supers;
  std::optional<WhereClause> where_clause;  // C0X Extension
  std::vector<ClassItem> items;
  core::Span span;
  DocList doc;
};

struct TypeAliasDecl {
  Visibility vis;
  Identifier name;
  std::shared_ptr<Type> type;
  core::Span span;
  DocList doc;
};

struct ErrorItem {
  core::Span span;
};

using ASTItem = std::variant<UsingDecl,
                             ImportDecl,  // C0X Extension
                             StaticDecl,
                             ProcedureDecl,
                             RecordDecl,
                             EnumDecl,
                             ModalDecl,
                             ClassDecl,
                             TypeAliasDecl,
                             ErrorItem>;

struct UnsafeSpanSet {
  std::string path;
  std::vector<core::Span> spans;
};

struct ASTModule {
  Path path;
  std::vector<ASTItem> items;
  DocList module_doc;
  std::vector<UnsafeSpanSet> unsafe_spans;
};

struct ASTFile {
  std::string path;
  std::vector<ASTItem> items;
  DocList module_doc;
  std::vector<core::Span> unsafe_spans;
};

}  // namespace cursive0::syntax

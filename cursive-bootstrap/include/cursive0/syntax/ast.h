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
struct LoopInvariant;

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
  std::vector<std::shared_ptr<Type>> generic_args;
  Identifier state;
};

struct TypePathType {
  TypePath path;
  std::vector<std::shared_ptr<Type>> generic_args;  // C0X Extension: Foo<T, U>
};

struct TypeOpaque {
  TypePath path;
};

struct TypeRefine {
  std::shared_ptr<Type> base;
  ExprPtr predicate;
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
                              TypePathType,
                              TypeOpaque,
                              TypeRefine>;

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

struct ArrayRepeatExpr {
  ExprPtr value;
  ExprPtr count;
};

struct SizeofExpr {
  std::shared_ptr<Type> type;
};

struct AlignofExpr {
  std::shared_ptr<Type> type;
};

struct GenericTypeRef {
  TypePath path;
  std::vector<std::shared_ptr<Type>> generic_args;
};

struct ModalStateRef {
  TypePath path;
  std::vector<std::shared_ptr<Type>> generic_args;
  Identifier state;
};

struct RecordExpr {
  std::variant<TypePath, GenericTypeRef, ModalStateRef> target;
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

// Loop invariant: where { predicate }
struct LoopInvariant {
  ExprPtr predicate;
  core::Span span;
};

struct LoopInfiniteExpr {
  std::optional<LoopInvariant> invariant_opt;
  std::shared_ptr<Block> body;
};

struct LoopConditionalExpr {
  ExprPtr cond;
  std::optional<LoopInvariant> invariant_opt;
  std::shared_ptr<Block> body;
};

struct LoopIterExpr {
  std::shared_ptr<Pattern> pattern;
  std::shared_ptr<Type> type_opt;
  ExprPtr iter;
  std::optional<LoopInvariant> invariant_opt;
  std::shared_ptr<Block> body;
};

struct BlockExpr {
  std::shared_ptr<Block> block;
};

struct UnsafeBlockExpr {
  std::shared_ptr<Block> block;
};

// C0X Extension: Attributed expression (e.g., [[dynamic]] expr)
struct AttributedExpr {
  AttributeList attrs;
  ExprPtr expr;
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
  std::vector<std::shared_ptr<Type>> generic_args;  // §13.1.2 generic_call
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

// C0X Extension: Async/await expressions (§19)
struct YieldExpr {
  bool release = false;  // true when `yield release`
  ExprPtr value;
};

struct YieldFromExpr {
  bool release = false;  // true when `yield release from`
  ExprPtr value;
};

struct SyncExpr {
  ExprPtr value;
};

enum class RaceHandlerKind {
  Return,  // handler is an expression
  Yield,   // handler is `yield <expr>`
};

struct RaceHandler {
  RaceHandlerKind kind = RaceHandlerKind::Return;
  ExprPtr value;
};

struct RaceArm {
  ExprPtr expr;
  std::shared_ptr<Pattern> pattern;
  RaceHandler handler;
};

struct RaceExpr {
  std::vector<RaceArm> arms;
};

struct AllExpr {
  std::vector<ExprPtr> exprs;
};

// C0X Extension: Structured Concurrency AST nodes (§3.5, §18)

// §18.1.1 Parallel block option kinds
enum class ParallelOptionKind {
  Cancel,  // cancel: CancelToken
  Name,    // name: string literal
};

// §18.1.1 Parallel block option
struct ParallelOption {
  ParallelOptionKind kind;
  ExprPtr value;  // CancelToken expr for Cancel, string literal for Name
  core::Span span;
};

// §18.1.1 Parallel block expression
struct ParallelExpr {
  ExprPtr domain;                       // $ExecutionDomain
  std::vector<ParallelOption> opts;     // [cancel:, name:]
  std::shared_ptr<Block> body;
};

// §18.4.1 Spawn option kinds
enum class SpawnOptionKind {
  Name,      // name: string literal
  Affinity,  // affinity: CpuSet expression
  Priority,  // priority: Priority expression
  MoveCapture, // move <expr> capture
};

// §18.4.1 Spawn option
struct SpawnOption {
  SpawnOptionKind kind;
  ExprPtr value;
  core::Span span;
};

// §18.4.1 Spawn expression
struct SpawnExpr {
  std::vector<SpawnOption> opts;     // [name:, affinity:, priority:, move]
  std::shared_ptr<Block> body;
};

// §10.3 Wait expression
struct WaitExpr {
  ExprPtr handle;  // Spawned<T>
};

// §18.5.1 Dispatch option kinds
enum class DispatchOptionKind {
  Reduce,   // reduce: operator
  Ordered,  // ordered flag
  Chunk,    // chunk: usize expression
};

// §18.5.1 Reduce operator (built-in or custom identifier)
enum class ReduceOp {
  Add,   // +
  Mul,   // *
  Min,   // min
  Max,   // max
  And,   // and
  Or,    // or
  Custom, // identifier
};

// §18.5.1 Dispatch option
struct DispatchOption {
  DispatchOptionKind kind;
  ReduceOp reduce_op = ReduceOp::Add;       // for Reduce
  Identifier custom_reduce_name;             // for Custom reduce
  ExprPtr chunk_expr;                        // for Chunk
  core::Span span;
};

// C0X Extension: Key System types (forward declarations for dispatch key clause)
enum class KeyMode {
  Read,
  Write,
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

// §18.5.1 Key clause for dispatch
struct DispatchKeyClause {
  KeyPathExpr key_path;
  KeyMode mode;
  core::Span span;
};

// §18.5.1 Dispatch expression
struct DispatchExpr {
  std::shared_ptr<Pattern> pattern;           // loop variable pattern
  ExprPtr range;                              // Range<I>
  std::optional<DispatchKeyClause> key_clause; // key path_expr mode
  std::vector<DispatchOption> opts;           // [reduce:, ordered, chunk:]
  std::shared_ptr<Block> body;
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
                              ArrayRepeatExpr,
                              SizeofExpr,
                              AlignofExpr,
                              RecordExpr,
                              EnumLiteralExpr,
                              IfExpr,
                              MatchExpr,
                              LoopInfiniteExpr,
                              LoopConditionalExpr,
                              LoopIterExpr,
                              BlockExpr,
                              UnsafeBlockExpr,
                              AttributedExpr,
                              TransmuteExpr,
                              FieldAccessExpr,
                              TupleAccessExpr,
                              IndexAccessExpr,
                              CallExpr,
                              MethodCallExpr,
                              PropagateExpr,
                              ResultExpr,
                              EntryExpr,
                              YieldExpr,
                              YieldFromExpr,
                              SyncExpr,
                              RaceExpr,
                              AllExpr,
                              // C0X Extension: Structured Concurrency
                              ParallelExpr,
                              SpawnExpr,
                              WaitExpr,
                              DispatchExpr>;

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

struct StaticAssertStmt {
  ExprPtr condition;
  core::Span span;
};

// C0X Extension: Key System AST nodes (KeyBlockMod and KeyBlockStmt)
enum class KeyBlockMod {
  Dynamic,
  Speculative,
  Release,
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
                          BreakStmt,
                          ContinueStmt,
                          UnsafeBlockStmt,
                          KeyBlockStmt,
                          StaticAssertStmt,
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

struct UsingWildcard {
  ModulePath module_path;
};

using UsingClause = std::variant<UsingPath, UsingList, UsingWildcard>;

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

// Foreign contract clauses (extern blocks)
enum class ForeignContractKind {
  Assumes,
  Ensures,
};

struct ForeignContractClause {
  ForeignContractKind kind;
  std::vector<ExprPtr> predicates;
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

// Extern ABI specifier (extern "C" or extern ABI_ID)
struct ExternAbiString {
  Token literal;
};

struct ExternAbiIdent {
  Identifier name;
};

using ExternAbi = std::variant<ExternAbiString, ExternAbiIdent>;

// Extern procedure declaration (inside extern block)
struct ExternProcDecl {
  AttributeList attrs;  // C0X Extension
  Visibility vis;
  Identifier name;
  std::optional<GenericParams> generic_params;  // C0X Extension
  std::optional<WhereClause> where_clause;  // C0X Extension
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;
  std::optional<ContractClause> contract;  // C0X Extension
  std::optional<std::vector<ForeignContractClause>> foreign_contracts_opt;  // C0X Extension
  core::Span span;
  DocList doc;
};

using ExternItem = std::variant<ExternProcDecl>;

struct ExternBlock {
  AttributeList attrs;  // C0X Extension
  Visibility vis;
  std::optional<ExternAbi> abi_opt;
  std::vector<ExternItem> items;
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
  std::optional<TypeInvariant> invariant;  // C0X Extension
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
  std::optional<GenericParams> generic_params;  // C0X Extension
  std::shared_ptr<Type> type;
  std::optional<WhereClause> where_clause;  // C0X Extension
  core::Span span;
  DocList doc;
};

struct ErrorItem {
  core::Span span;
};

using ASTItem = std::variant<UsingDecl,
                             ImportDecl,  // C0X Extension
                             ExternBlock,
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

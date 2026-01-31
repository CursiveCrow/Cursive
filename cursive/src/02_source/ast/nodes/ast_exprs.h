// ===========================================================================
// MIGRATION MAPPING: ast_exprs.h
// ===========================================================================
//
// PURPOSE:
//   Expression AST node definitions. Contains all struct definitions for
//   expression nodes in the Cursive grammar, plus the ExprNode variant
//   and Expr wrapper. This is the largest AST category with 50+ node types.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.3.2.4 - Expression Nodes (Lines referenced throughout 3.3.8)
//
//   Expr = (Span, ExprNode)
//   ExprNode variants by category:
//
//   ERROR/BASIC:
//     ErrorExpr      - parse error placeholder
//     LiteralExpr    - literals (int, float, string, char, bool)
//     IdentifierExpr - simple identifier reference
//
//   QUALIFIED:
//     QualifiedNameExpr  - module::name reference
//     QualifiedApplyExpr - module::name(args) or module::name{fields}
//     PathExpr           - module::name path
//
//   OPERATORS:
//     RangeExpr   - range expressions (a..b, a..=b, etc.)
//     BinaryExpr  - binary operations (a + b)
//     UnaryExpr   - unary operations (-a, !a)
//     CastExpr    - type cast (expr as Type)
//
//   MEMORY:
//     DerefExpr     - dereference (*ptr)
//     AddressOfExpr - address-of (&place)
//     MoveExpr      - move expression (move x)
//     AllocExpr     - region allocation (^value, region^value)
//     PtrNullExpr   - null pointer (Ptr::null())
//
//   AGGREGATE:
//     TupleExpr       - tuple construction ((a, b))
//     ArrayExpr       - array literal ([a, b, c])
//     ArrayRepeatExpr - array repeat ([value; count])
//     RecordExpr      - record literal (Type{field: value})
//     EnumLiteralExpr - enum variant (Enum::Variant(payload))
//
//   INTRINSIC:
//     SizeofExpr  - sizeof(Type)
//     AlignofExpr - alignof(Type)
//
//   CONTROL FLOW:
//     IfExpr              - if expression
//     MatchExpr           - match expression
//     LoopInfiniteExpr    - loop { }
//     LoopConditionalExpr - loop cond { }
//     LoopIterExpr        - loop x in iter { }
//     BlockExpr           - block expression { }
//     UnsafeBlockExpr     - unsafe { }
//
//   POSTFIX:
//     FieldAccessExpr - field access (x.field)
//     TupleAccessExpr - tuple access (x.0)
//     IndexAccessExpr - index access (x[i])
//     CallExpr        - function call (f(args))
//     MethodCallExpr  - method call (x~>method(args))
//     PropagateExpr   - error propagation (expr?)
//
//   SPECIAL:
//     AttributedExpr - [[attr]] expr
//     TransmuteExpr  - transmute<From, To>(value)
//
//   CONTRACT (C0X):
//     ResultExpr - @result
//     EntryExpr  - @entry(expr)
//
//   ASYNC/CONCURRENCY (C0X):
//     YieldExpr     - yield expr, yield release expr
//     YieldFromExpr - yield from expr
//     SyncExpr      - sync expr
//     RaceExpr      - race { arm, arm }
//     AllExpr       - all { expr, expr }
//     ParallelExpr  - parallel domain { }
//     SpawnExpr     - spawn { }
//     WaitExpr      - wait handle
//     DispatchExpr  - dispatch i in range { }
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\include\cursive0\02_syntax\ast.h
// ---------------------------------------------------------------------------
//
// 1. BASIC EXPRESSIONS (Lines 179-192)
//    ─────────────────────────────────────────────────────────────────────────
//    struct ErrorExpr {};                               (Line 179)
//    struct LiteralExpr { Token literal; };             (Lines 181-183)
//    struct IdentifierExpr { Identifier name; };        (Lines 185-187)
//    struct QualifiedNameExpr {                         (Lines 189-192)
//      ModulePath path; Identifier name;
//    };
//
// 2. ARGUMENT TYPES (Lines 194-214)
//    ─────────────────────────────────────────────────────────────────────────
//    struct Arg { bool moved; ExprPtr value; Span span; };
//    struct ParenArgs { vector<Arg> args; };
//    struct FieldInit { Identifier name; ExprPtr value; Span span; };
//    struct BraceArgs { vector<FieldInit> fields; };
//    using ApplyArgs = variant<ParenArgs, BraceArgs>;
//
// 3. QUALIFIED EXPRESSIONS (Lines 216-240)
//    ─────────────────────────────────────────────────────────────────────────
//    struct QualifiedApplyExpr { ModulePath path; Identifier name; ApplyArgs args; };
//    struct PathExpr { ModulePath path; Identifier name; };
//    struct EnumPayloadParen { vector<ExprPtr> elements; };
//    struct EnumPayloadBrace { vector<FieldInit> fields; };
//    using EnumPayload = variant<EnumPayloadParen, EnumPayloadBrace>;
//    struct EnumLiteralExpr { Path path; optional<EnumPayload> payload_opt; };
//
// 4. OPERATOR EXPRESSIONS (Lines 242-271)
//    ─────────────────────────────────────────────────────────────────────────
//    enum class RangeKind { ... };                      (Lines 242-249)
//    struct RangeExpr { RangeKind kind; ExprPtr lhs, rhs; };
//    struct BinaryExpr { Identifier op; ExprPtr lhs, rhs; };
//    struct CastExpr { ExprPtr value; shared_ptr<Type> type; };
//    struct UnaryExpr { Identifier op; ExprPtr value; };
//
// 5. MEMORY EXPRESSIONS (Lines 273-303)
//    ─────────────────────────────────────────────────────────────────────────
//    struct DerefExpr { ExprPtr value; };
//    struct AddressOfExpr { ExprPtr place; };
//    struct MoveExpr { ExprPtr place; };
//    struct AllocExpr { optional<Identifier> region_opt; ExprPtr value; };
//    struct PtrNullExpr {};
//    struct TupleExpr { vector<ExprPtr> elements; };
//    struct ArrayExpr { vector<ExprPtr> elements; };
//    struct ArrayRepeatExpr { ExprPtr value; ExprPtr count; };
//
// 6. INTRINSIC EXPRESSIONS (Lines 305-322)
//    ─────────────────────────────────────────────────────────────────────────
//    struct SizeofExpr { shared_ptr<Type> type; };
//    struct AlignofExpr { shared_ptr<Type> type; };
//    struct GenericTypeRef { TypePath path; vector<shared_ptr<Type>> generic_args; };
//    struct ModalStateRef { TypePath path; vector<shared_ptr<Type>> generic_args; Identifier state; };
//
// 7. AGGREGATE EXPRESSIONS (Lines 324-333)
//    ─────────────────────────────────────────────────────────────────────────
//    struct RecordExpr { variant<TypePath, GenericTypeRef, ModalStateRef> target; vector<FieldInit> fields; };
//    struct IfExpr { ExprPtr cond; ExprPtr then_expr; ExprPtr else_expr; };
//
// 8. PATTERN/MATCH SUPPORT (Lines 412-422)
//    ─────────────────────────────────────────────────────────────────────────
//    struct MatchArm { shared_ptr<Pattern> pattern; ExprPtr guard_opt; ExprPtr body; Span span; };
//    struct MatchExpr { ExprPtr value; vector<MatchArm> arms; };
//
// 9. LOOP EXPRESSIONS (Lines 424-447)
//    ─────────────────────────────────────────────────────────────────────────
//    struct LoopInvariant { ExprPtr predicate; Span span; };
//    struct LoopInfiniteExpr { optional<LoopInvariant> invariant_opt; shared_ptr<Block> body; };
//    struct LoopConditionalExpr { ExprPtr cond; optional<LoopInvariant> invariant_opt; shared_ptr<Block> body; };
//    struct LoopIterExpr { shared_ptr<Pattern> pattern; shared_ptr<Type> type_opt; ExprPtr iter; optional<LoopInvariant> invariant_opt; shared_ptr<Block> body; };
//
// 10. BLOCK EXPRESSIONS (Lines 449-467)
//     ────────────────────────────────────────────────────────────────────────
//     struct BlockExpr { shared_ptr<Block> block; };
//     struct UnsafeBlockExpr { shared_ptr<Block> block; };
//     struct AttributedExpr { AttributeList attrs; ExprPtr expr; };
//     struct TransmuteExpr { shared_ptr<Type> from; shared_ptr<Type> to; ExprPtr value; };
//
// 11. POSTFIX EXPRESSIONS (Lines 469-498)
//     ────────────────────────────────────────────────────────────────────────
//     struct FieldAccessExpr { ExprPtr base; Identifier name; };
//     struct TupleAccessExpr { ExprPtr base; Token index; };
//     struct IndexAccessExpr { ExprPtr base; ExprPtr index; };
//     struct CallExpr { ExprPtr callee; vector<shared_ptr<Type>> generic_args; vector<Arg> args; };
//     struct MethodCallExpr { ExprPtr receiver; Identifier name; vector<Arg> args; };
//     struct PropagateExpr { ExprPtr value; };
//
// 12. CONTRACT EXPRESSIONS (Lines 500-508)
//     ────────────────────────────────────────────────────────────────────────
//     struct ResultExpr {};
//     struct EntryExpr { ExprPtr expr; };
//
// 13. ASYNC EXPRESSIONS (Lines 510-547)
//     ────────────────────────────────────────────────────────────────────────
//     struct YieldExpr { bool release; ExprPtr value; };
//     struct YieldFromExpr { bool release; ExprPtr value; };
//     struct SyncExpr { ExprPtr value; };
//     enum class RaceHandlerKind { Return, Yield };
//     struct RaceHandler { RaceHandlerKind kind; ExprPtr value; };
//     struct RaceArm { ExprPtr expr; shared_ptr<Pattern> pattern; RaceHandler handler; };
//     struct RaceExpr { vector<RaceArm> arms; };
//     struct AllExpr { vector<ExprPtr> exprs; };
//
// 14. CONCURRENCY EXPRESSIONS (Lines 549-662)
//     ────────────────────────────────────────────────────────────────────────
//     enum class ParallelOptionKind { Cancel, Name };
//     struct ParallelOption { ParallelOptionKind kind; ExprPtr value; Span span; };
//     struct ParallelExpr { ExprPtr domain; vector<ParallelOption> opts; shared_ptr<Block> body; };
//     enum class SpawnOptionKind { Name, Affinity, Priority, MoveCapture };
//     struct SpawnOption { SpawnOptionKind kind; ExprPtr value; Span span; };
//     struct SpawnExpr { vector<SpawnOption> opts; shared_ptr<Block> body; };
//     struct WaitExpr { ExprPtr handle; };
//     enum class DispatchOptionKind { Reduce, Ordered, Chunk };
//     enum class ReduceOp { Add, Mul, Min, Max, And, Or, Custom };
//     struct DispatchOption { ... };
//     struct KeySegField { bool marked; Identifier name; };
//     struct KeySegIndex { bool marked; ExprPtr expr; };
//     using KeySeg = variant<KeySegField, KeySegIndex>;
//     struct KeyPathExpr { Identifier root; vector<KeySeg> segs; Span span; };
//     struct DispatchKeyClause { KeyPathExpr key_path; KeyMode mode; Span span; };
//     struct DispatchExpr { shared_ptr<Pattern> pattern; ExprPtr range; optional<DispatchKeyClause> key_clause; vector<DispatchOption> opts; shared_ptr<Block> body; };
//
// 15. EXPRNODE VARIANT (Lines 664-712)
//     ────────────────────────────────────────────────────────────────────────
//     using ExprNode = variant<ErrorExpr, LiteralExpr, ..., DispatchExpr>;
//     (50+ variants)
//
// 16. EXPR WRAPPER (Lines 714-717)
//     ────────────────────────────────────────────────────────────────────────
//     struct Expr { Span span; ExprNode node; };
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   From ast_fwd.h:
//     - ExprPtr, PatternPtr, TypePath, Identifier, ModulePath
//
//   From ast_enums.h:
//     - RangeKind, RaceHandlerKind, ParallelOptionKind, SpawnOptionKind,
//       DispatchOptionKind, ReduceOp, KeyMode
//
//   From 00_core/span.h:
//     - core::Span
//
//   From lexer:
//     - Token (for LiteralExpr)
//
//   From ast_attributes.h:
//     - AttributeList (for AttributedExpr)
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. This is the largest AST file. Consider further splitting by category:
//      - exprs_basic.h: Error, Literal, Identifier, Qualified
//      - exprs_operators.h: Binary, Unary, Cast, Range
//      - exprs_memory.h: Deref, AddressOf, Move, Alloc
//      - exprs_aggregate.h: Tuple, Array, Record, Enum
//      - exprs_control.h: If, Match, Loop variants, Block
//      - exprs_postfix.h: Field, Tuple, Index access, Call, Method
//      - exprs_async.h: Yield, Sync, Race, All, Parallel, Spawn, Dispatch
//
//   2. Helper structs like Arg, FieldInit, MatchArm could go in a
//      separate exprs_support.h to reduce main file size.
//
//   3. The concurrency expressions are a C0X extension. Consider
//      #ifdef CURSIVE_C0X guards around them.
//
//   4. KeyPathExpr is used by both DispatchExpr and KeyBlockStmt.
//      Consider moving it to a shared location.
//
//   5. ExprNode variant is large (50+ types). This affects:
//      - Compile times (variant instantiation)
//      - Object size (variant size = largest member + tag)
//      - Visit performance (large switch table)
//
// ===========================================================================

// TODO: Migrate expression definitions from ast.h lines 179-717


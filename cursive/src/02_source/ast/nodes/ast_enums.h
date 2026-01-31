// ===========================================================================
// MIGRATION MAPPING: ast_enums.h
// ===========================================================================
//
// PURPOSE:
//   Enumeration types used across AST node categories. Centralizing enums
//   prevents circular dependencies between category headers while keeping
//   related values together.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.3.2.3 - Type Nodes (Lines 2875-2898)
//     - Perm = const | unique | shared (2886)
//     - Qual = imm | mut (2887)
//     - PtrStateOpt, StringStateOpt, BytesStateOpt (2888-2890)
//     - ParamMode (2893)
//
//   Section 3.3.2.4 - Expression Nodes
//     - RangeKind (exclusive, inclusive, from, to, full)
//     - RaceHandlerKind (return, yield)
//     - ReduceOp (add, mul, min, max, and, or, custom)
//
//   Section 3.3.2.6 - Statement Nodes
//     - KeyMode (read, write)
//     - KeyBlockMod (dynamic, speculative, release)
//
//   Section 3.3.2.5 - Declaration Nodes (Lines 2680-2873)
//     - Visibility (public, internal, private, protected)
//     - Mutability (let, var)
//     - ReceiverPerm (const, unique, shared)
//     - Variance (covariant, contravariant, invariant, bivariant)
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\include\cursive0\02_syntax\ast.h
// ---------------------------------------------------------------------------
//
// 1. TYPE-RELATED ENUMS (Lines 30-77)
//    ─────────────────────────────────────────────────────────────────────────
//    enum class ParamMode { Move };                    (Lines 30-32)
//    enum class TypePerm { Const, Unique, Shared };    (Lines 34-38)
//    enum class RawPtrQual { Imm, Mut };               (Lines 40-43)
//    enum class StringState { Managed, View };         (Lines 45-48)
//    enum class BytesState { Managed, View };          (Lines 50-53)
//    enum class PtrState { Valid, Null, Expired };     (Lines 73-77)
//
// 2. EXPRESSION-RELATED ENUMS (Lines 242-249, 525-528, 552-613)
//    ─────────────────────────────────────────────────────────────────────────
//    enum class RangeKind { To, ToInclusive, Full, From, Exclusive, Inclusive };
//                                                      (Lines 242-249)
//    enum class RaceHandlerKind { Return, Yield };     (Lines 525-528)
//    enum class ParallelOptionKind { Cancel, Name };   (Lines 552-555)
//    enum class SpawnOptionKind { Name, Affinity, Priority, MoveCapture };
//                                                      (Lines 572-577)
//    enum class DispatchOptionKind { Reduce, Ordered, Chunk };
//                                                      (Lines 598-602)
//    enum class ReduceOp { Add, Mul, Min, Max, And, Or, Custom };
//                                                      (Lines 605-613)
//
// 3. KEY SYSTEM ENUMS (Lines 625-628, 816-820)
//    ─────────────────────────────────────────────────────────────────────────
//    enum class KeyMode { Read, Write };               (Lines 625-628)
//    enum class KeyBlockMod { Dynamic, Speculative, Release };
//                                                      (Lines 816-820)
//
// 4. DECLARATION-RELATED ENUMS (Lines 854-877, 941-946, 996-999, 1008-1011)
//    ─────────────────────────────────────────────────────────────────────────
//    enum class Visibility { Public, Internal, Private, Protected };
//                                                      (Lines 854-859)
//    enum class Mutability { Let, Var };               (Lines 861-864)
//    enum class ReceiverPerm { Const, Unique, Shared };(Lines 873-877)
//    enum class Variance { Covariant, Contravariant, Invariant, Bivariant };
//                                                      (Lines 941-946)
//    enum class ForeignContractKind { Assumes, Ensures };
//                                                      (Lines 996-999)
//    enum class ContractIntrinsicKind { Result, Entry };
//                                                      (Lines 1008-1011)
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   None - this file is self-contained enums only.
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Group enums by semantic domain for clarity:
//      - Permission/qualifier enums (TypePerm, RawPtrQual, ReceiverPerm)
//      - State enums (PtrState, StringState, BytesState)
//      - Mode enums (ParamMode, KeyMode, Mutability)
//      - Kind enums (RangeKind, RaceHandlerKind, OptionKinds)
//      - Visibility/access enums (Visibility, Variance)
//
//   2. Consider string conversion functions:
//      const char* to_string(TypePerm perm);
//      const char* to_string(Visibility vis);
//      These should go in enums.cpp.
//
//   3. Consider parsing helpers:
//      std::optional<TypePerm> parse_perm(string_view s);
//      These are used by parser and should be accessible.
//
//   4. Some enums like RangeKind have semantic implications for
//      code generation - document these in the enum comments.
//
// ===========================================================================

// TODO: Migrate enum definitions from ast.h
//
// namespace cursive::ast {
//
// // Permission and qualifier enums
// enum class ParamMode { Move };
// enum class TypePerm { Const, Unique, Shared };
// enum class RawPtrQual { Imm, Mut };
// enum class ReceiverPerm { Const, Unique, Shared };
//
// // State enums
// enum class PtrState { Valid, Null, Expired };
// enum class StringState { Managed, View };
// enum class BytesState { Managed, View };
//
// // Mode and mutability enums
// enum class KeyMode { Read, Write };
// enum class Mutability { Let, Var };
//
// // Visibility and variance
// enum class Visibility { Public, Internal, Private, Protected };
// enum class Variance { Covariant, Contravariant, Invariant, Bivariant };
//
// // Expression-specific kinds
// enum class RangeKind { To, ToInclusive, Full, From, Exclusive, Inclusive };
// enum class RaceHandlerKind { Return, Yield };
// enum class ReduceOp { Add, Mul, Min, Max, And, Or, Custom };
//
// // Concurrency option kinds
// enum class ParallelOptionKind { Cancel, Name };
// enum class SpawnOptionKind { Name, Affinity, Priority, MoveCapture };
// enum class DispatchOptionKind { Reduce, Ordered, Chunk };
//
// // Key system modifiers
// enum class KeyBlockMod { Dynamic, Speculative, Release };
//
// // Contract kinds
// enum class ForeignContractKind { Assumes, Ensures };
// enum class ContractIntrinsicKind { Result, Entry };
//
// } // namespace cursive::ast


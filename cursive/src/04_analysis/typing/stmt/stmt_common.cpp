// =============================================================================
// MIGRATION MAPPING: stmt/stmt_common.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2: Static Semantics - Statements
//   - Statement typing judgments
//   - StmtResult type
//   - Statement list typing
//   - Block typing
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Common statement typing utilities
//
// KEY CONTENT TO MIGRATE:
//   STATEMENT COMMON UTILITIES:
//   - Statement dispatch to specific handlers
//   - StmtResult type
//   - Statement list typing
//   - Environment threading
//   - Control flow tracking
//
//   STATEMENT DISPATCH:
//   TypeStmt(stmt, Gamma) =
//     match stmt.kind {
//       LetStmt => let_stmt.cpp
//       VarStmt => var_stmt.cpp
//       ShadowLet => shadow_let_stmt.cpp
//       ShadowVar => shadow_var_stmt.cpp
//       Assign => assign_stmt.cpp
//       CompoundAssign => compound_assign_stmt.cpp
//       ExprStmt => expr_stmt.cpp
//       Return => return_stmt.cpp
//       Break => break_stmt.cpp
//       Continue => continue_stmt.cpp
//       Defer => defer_stmt.cpp
//       Region => region_stmt.cpp
//       Frame => frame_stmt.cpp
//       KeyBlock => key_block_stmt.cpp
//       Unsafe => unsafe_block_stmt.cpp
//       Error => error_stmt.cpp
//     }
//
//   STMT RESULT:
//   StmtResult {
//     Gamma': Environment,  // Updated environment
//     Res: Type?,           // Return type (if returns)
//     Brk: Set<Type>,       // Break types
//     BrkVoid: bool,        // Break without value
//   }
//
//   STATEMENT LIST:
//   TypeStmtList([s1, s2, ...], Gamma):
//   - Thread environment through statements
//   - Collect control flow effects
//   - Last statement's type is block type
//
//   ENVIRONMENT THREADING:
//   Gamma_0 |- s1 => Gamma_1
//   Gamma_1 |- s2 => Gamma_2
//   ...
//   Result is final Gamma
//
//   CONTROL FLOW EFFECTS:
//   - Return: sets Res type
//   - Break: adds to Brk set
//   - Continue: no type effect
//   - Normal: threads environment
//
// DEPENDENCIES:
//   - All statement-specific typing modules
//   - Environment management
//   - ControlFlowTracker
//   - StmtResult construction
//
// SPEC RULES:
//   - Statement typing judgments
//   - Environment threading
//
// RESULT:
//   Dispatches to specific statement handlers
//   Manages environment and control flow
//
// =============================================================================


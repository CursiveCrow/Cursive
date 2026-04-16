// =============================================================================
// using_local_stmt.cpp - Typing for UsingLocalStmt
// =============================================================================
//
// SPEC REFERENCE:
//   CursiveSpecification.md §18.3.4 Static Semantics
//     (T-UsingLocalStmt), (T-UsingLocalStmt-Err) delegate to the
//     §7.2 UsingAlias judgment. The alias binding is added to the current
//     scope; the alias's Entity is the same Entity that `source` resolves to.
//
// The actual scope update is performed by the resolver (see
// resolve_stmt_seq.cpp). This file exists to provide a typing no-op so the
// statement type-checker recognises UsingLocalStmt as a valid statement form.
// =============================================================================

#include "04_analysis/typing/type_stmt.h"

#include "00_core/assert_spec.h"
#include "02_source/ast/ast.h"

namespace cursive::analysis {

// UsingLocalStmt contributes no type and no runtime effect; resolution
// handles the symbol-table update. Typing returns Ok with the environment
// unchanged.
StmtTypeResult TypeUsingLocalStmt(const ast::UsingLocalStmt& stmt,
                                  const TypeEnv& env) {
  SPEC_RULE("T-UsingLocalStmt");
  (void)stmt;
  StmtTypeResult result;
  result.ok = true;
  result.env = env;
  return result;
}

}  // namespace cursive::analysis

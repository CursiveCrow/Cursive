#pragma once

#include "cursive0/semantics/control.h"
#include "cursive0/semantics/state.h"
#include "cursive0/semantics/value.h"

namespace cursive0 {
namespace project {
struct Project;
}  // namespace project
namespace sema {
struct ScopeContext;
}  // namespace sema
namespace syntax {
struct Expr;
struct ASTModule;
}  // namespace syntax
}  // namespace cursive0

namespace cursive0::semantics {

struct InterpreterResult {
  bool has_value = false;
  bool has_control = false;
  Value value{};
  Control control{};
};

InterpreterResult InterpretExpr(const syntax::Expr& expr,
                                Sigma& sigma,
                                sema::ScopeContext& scope);

InterpreterResult InterpretModule(const syntax::ASTModule& module,
                                  Sigma& sigma,
                                  sema::ScopeContext& scope);

InterpreterResult InterpretProject(const project::Project& project,
                                   Sigma& sigma,
                                   sema::ScopeContext& scope);

}  // namespace cursive0::semantics

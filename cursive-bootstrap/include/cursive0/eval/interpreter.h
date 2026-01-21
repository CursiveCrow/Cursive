#pragma once

#include "cursive0/eval/control.h"
#include "cursive0/eval/state.h"
#include "cursive0/eval/value.h"

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

namespace cursive0::eval {

struct InterpreterResult {
  bool has_value = false;
  bool has_control = false;
  Value value{};
  Control control{};
};

InterpreterResult InterpretExpr(const syntax::Expr& expr,
                                Sigma& sigma,
                                analysis::ScopeContext& scope);

InterpreterResult InterpretModule(const syntax::ASTModule& module,
                                  Sigma& sigma,
                                  analysis::ScopeContext& scope);

InterpreterResult InterpretProject(const project::Project& project,
                                   Sigma& sigma,
                                   analysis::ScopeContext& scope);

}  // namespace cursive0::eval

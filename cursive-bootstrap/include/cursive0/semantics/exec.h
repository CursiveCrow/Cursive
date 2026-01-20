#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/semantics/control.h"
#include "cursive0/semantics/match.h"
#include "cursive0/semantics/state.h"
#include "cursive0/semantics/value.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::semantics {

struct BindResult {
  bool ok = false;
  std::vector<Binding> bindings;
};

using BindListInput = std::vector<std::pair<std::string, Value>>;

std::optional<BindEnv> BindPatternVal(const SemanticsContext& ctx,
                                      const syntax::Pattern& pattern,
                                      const Value& value);
BindListInput BindOrder(const syntax::Pattern& pattern,
                        const BindEnv& env);
BindResult BindList(Sigma& sigma,
                    const BindListInput& binds,
                    BindInfo info = {});
BindResult BindPattern(Sigma& sigma,
                       const SemanticsContext& ctx,
                       const syntax::Pattern& pattern,
                       const Value& value,
                       BindInfo info = {});

bool BlockEnter(Sigma& sigma,
                const BindListInput& binds,
                BindInfo info,
                ScopeId* out_scope);
Outcome EvalBlockBodySigma(const SemanticsContext& ctx,
                           const syntax::Block& block,
                           Sigma& sigma);
Outcome EvalBlockSigma(const SemanticsContext& ctx,
                       const syntax::Block& block,
                       Sigma& sigma);
Outcome EvalBlockBindSigma(const SemanticsContext& ctx,
                           const syntax::Pattern& pattern,
                           const Value& value,
                           const syntax::Block& block,
                           BindInfo info,
                           Sigma& sigma);
Outcome EvalInScopeSigma(const SemanticsContext& ctx,
                         const syntax::Block& block,
                         ScopeId scope,
                         Sigma& sigma);

StmtOut ExecSigma(const SemanticsContext& ctx,
                  const syntax::Stmt& stmt,
                  Sigma& sigma);
StmtOut ExecSeqSigma(const SemanticsContext& ctx,
                     const std::vector<syntax::Stmt>& stmts,
                     Sigma& sigma);

}  // namespace cursive0::semantics

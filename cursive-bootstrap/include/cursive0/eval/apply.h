#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "cursive0/analysis/types/types.h"
#include "cursive0/eval/control.h"
#include "cursive0/eval/state.h"
#include "cursive0/eval/value.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::eval {

struct MethodTarget {
  enum class Kind {
    Record,
    Class,
    State,
  };
  Kind kind = Kind::Record;
  const syntax::MethodDecl* record_method = nullptr;
  const syntax::ClassMethodDecl* class_method = nullptr;
  const syntax::StateMethodDecl* state_method = nullptr;
  analysis::TypePath owner_class;
};

Outcome ApplyProcSigma(const SemanticsContext& ctx,
                       const syntax::ProcedureDecl& proc,
                       const std::vector<BindingValue>& args,
                       Sigma& sigma);

Outcome ApplyRegionProc(const SemanticsContext& ctx,
                        std::string_view name,
                        const std::vector<BindingValue>& args,
                        Sigma& sigma);

Outcome ApplyRecordCtorSigma(const SemanticsContext& ctx,
                             const analysis::TypePath& path,
                             Sigma& sigma);

Outcome ApplyMethodSigma(const SemanticsContext& ctx,
                         const syntax::Expr& base,
                         std::string_view name,
                         const Value& self_value,
                         const BindingValue& self_arg,
                         const std::vector<BindingValue>& args,
                         const MethodTarget& target,
                         Sigma& sigma);

}  // namespace cursive0::eval

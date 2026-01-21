#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "cursive0/eval/control.h"
#include "cursive0/eval/state.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::eval {

using EvalOptResult = std::variant<std::optional<Value>, Control>;
using EvalListResult = std::variant<std::vector<Value>, Control>;
using EvalFieldInitsResult =
    std::variant<std::vector<std::pair<std::string, Value>>, Control>;
using AddrResult = std::variant<Addr, Control>;

bool IsAddrCtrl(const AddrResult& out);

Outcome ReadPtrSigma(const Value& ptr, Sigma& sigma);
StmtOut WritePtrSigma(const Value& ptr, const Value& value, Sigma& sigma);
Outcome ReadPlaceSigma(const SemanticsContext& ctx,
                       const syntax::Expr& expr,
                       Sigma& sigma);
StmtOut WritePlaceSigma(const SemanticsContext& ctx,
                        const syntax::Expr& expr,
                        const Value& value,
                        Sigma& sigma);
StmtOut WritePlaceSubSigma(const SemanticsContext& ctx,
                           const syntax::Expr& expr,
                           const Value& value,
                           Sigma& sigma);
Outcome MovePlaceSigma(const SemanticsContext& ctx,
                       const syntax::Expr& expr,
                       Sigma& sigma);
AddrResult AddrOfSigma(const SemanticsContext& ctx,
                       const syntax::Expr& expr,
                       Sigma& sigma);
std::optional<std::string> PlaceRoot(const syntax::Expr& expr);
std::optional<std::string> FieldHead(const syntax::Expr& expr);
std::optional<Value> EvalBinOp(std::string_view op,
                               const Value& lhs,
                               const Value& rhs);
std::optional<RegionTarget> ActiveTarget(const Sigma& sigma);
std::optional<RegionTarget> ResolveTarget(const Sigma& sigma,
                                          RegionTarget target);
std::optional<RegionTag> ResolveTag(const Sigma& sigma, RegionTarget target);
std::optional<RegionTarget> RegionTargetOf(const Value& value);
std::optional<Value> RegionAlloc(Sigma& sigma,
                                 RegionTarget target,
                                 const Value& value);

Outcome EvalSigma(const SemanticsContext& ctx,
                  const syntax::Expr& expr,
                  Sigma& sigma);

EvalOptResult EvalOptSigma(const SemanticsContext& ctx,
                           const syntax::ExprPtr& expr_opt,
                           Sigma& sigma);

EvalListResult EvalListSigma(const SemanticsContext& ctx,
                             const std::vector<syntax::ExprPtr>& exprs,
                             Sigma& sigma);

EvalFieldInitsResult EvalFieldInitsSigma(
    const SemanticsContext& ctx,
    const std::vector<syntax::FieldInit>& fields,
    Sigma& sigma);

}  // namespace cursive0::eval

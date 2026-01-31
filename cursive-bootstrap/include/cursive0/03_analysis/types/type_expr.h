#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/03_analysis/memory/calls.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/place_types.h"
#include "cursive0/03_analysis/types/type_infer.h"
#include "cursive0/03_analysis/types/type_lower.h"
#include "cursive0/03_analysis/types/type_stmt.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/03_analysis/types/type_layout.h"
#include "cursive0/03_analysis/types/type_wf.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

// §5.2.12 Expression Typing (Cursive0)

// Core expression typing - infers type of expression
ExprTypeResult TypeExpr(const ScopeContext& ctx,
                        const StmtTypeContext& type_ctx,
                        const syntax::ExprPtr& expr,
                        const TypeEnv& env);

// Check expression against expected type
CheckResult CheckExprAgainst(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::ExprPtr& expr,
                             const TypeRef& expected,
                             const TypeEnv& env);

// Check place expression against expected type
CheckResult CheckPlaceAgainst(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::ExprPtr& expr,
                              const TypeRef& expected,
                              const TypeEnv& env);

// Place expression typing - for lvalues
PlaceTypeResult TypePlace(const ScopeContext& ctx,
                          const StmtTypeContext& type_ctx,
                          const syntax::ExprPtr& expr,
                          const TypeEnv& env);

// Individual expression form handlers - Identifiers and Paths
ExprTypeResult TypeIdentifierExpr(const ScopeContext& ctx,
                                  const syntax::IdentifierExpr& expr,
                                  const TypeEnv& env);

ExprTypeResult TypePathExpr(const ScopeContext& ctx,
                            const syntax::PathExpr& expr);

// Field Access
ExprTypeResult TypeFieldAccessExpr(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::FieldAccessExpr& expr,
                                   const TypeEnv& env);

PlaceTypeResult TypeFieldAccessPlace(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::FieldAccessExpr& expr,
                                     const TypeEnv& env);

// Tuple Access
ExprTypeResult TypeTupleAccessExpr(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::TupleAccessExpr& expr,
                                   const TypeEnv& env);

PlaceTypeResult TypeTupleAccessPlace(const ScopeContext& ctx,
                                     const StmtTypeContext& type_ctx,
                                     const syntax::TupleAccessExpr& expr,
                                     const TypeEnv& env);

// Binary Operators
ExprTypeResult TypeBinaryExpr(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::BinaryExpr& expr,
                              const TypeEnv& env);

// Unary Operators
ExprTypeResult TypeUnaryExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::UnaryExpr& expr,
                             const TypeEnv& env,
                             const core::Span& span = core::Span{});

// Cast Expression
ExprTypeResult TypeCastExpr(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::CastExpr& expr,
                            const TypeEnv& env);

// If Expression
ExprTypeResult TypeIfExpr(const ScopeContext& ctx,
                          const StmtTypeContext& type_ctx,
                          const syntax::IfExpr& expr,
                          const TypeEnv& env);

CheckResult CheckIfExpr(const ScopeContext& ctx,
                        const StmtTypeContext& type_ctx,
                        const syntax::IfExpr& expr,
                        const TypeRef& expected,
                        const TypeEnv& env);

// Range Expression
ExprTypeResult TypeRangeExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::RangeExpr& expr,
                             const TypeEnv& env);

// Address-Of Expression
ExprTypeResult TypeAddressOfExpr(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::AddressOfExpr& expr,
                                 const TypeEnv& env);

// Dereference Expression
ExprTypeResult TypeDerefExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::DerefExpr& expr,
                             const TypeEnv& env);

PlaceTypeResult TypeDerefPlace(const ScopeContext& ctx,
                               const StmtTypeContext& type_ctx,
                               const syntax::DerefExpr& expr,
                               const TypeEnv& env);

// Move Expression
ExprTypeResult TypeMoveExpr(const ScopeContext& ctx,
                            const StmtTypeContext& type_ctx,
                            const syntax::MoveExpr& expr,
                            const TypeEnv& env);

// Alloc Expression
ExprTypeResult TypeAllocExpr(const ScopeContext& ctx,
                             const StmtTypeContext& type_ctx,
                             const syntax::AllocExpr& expr,
                             const TypeEnv& env);

// Transmute Expression
ExprTypeResult TypeTransmuteExpr(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::TransmuteExpr& expr,
                                 const TypeEnv& env);

// Propagate Expression (Sigma)
ExprTypeResult TypePropagateExpr(const ScopeContext& ctx,
                                 const StmtTypeContext& type_ctx,
                                 const syntax::PropagateExpr& expr,
                                 const TypeEnv& env);

// Record Literal
ExprTypeResult TypeRecordExpr(const ScopeContext& ctx,
                              const StmtTypeContext& type_ctx,
                              const syntax::RecordExpr& expr,
                              const TypeEnv& env);

// Enum Literal
ExprTypeResult TypeEnumLiteralExpr(const ScopeContext& ctx,
                                   const StmtTypeContext& type_ctx,
                                   const syntax::EnumLiteralExpr& expr,
                                   const TypeEnv& env);

// Error Expression
ExprTypeResult TypeErrorExpr(const ScopeContext& ctx,
                             const syntax::ErrorExpr& expr);

// Method Call Expression
ExprTypeResult TypeMethodCallExpr(const ScopeContext& ctx,
                                  const StmtTypeContext& type_ctx,
                                  const syntax::MethodCallExpr& expr,
                                  const TypeEnv& env,
                                  const core::Span& span = core::Span{});

// Helper predicates
bool IsPlaceExpr(const syntax::ExprPtr& expr);
bool BitcopyType(const ScopeContext& ctx, const TypeRef& type);
bool CloneType(const ScopeContext& ctx, const TypeRef& type);
bool EqType(const TypeRef& type);
bool OrdType(const TypeRef& type);
bool CastValid(const TypeRef& source, const TypeRef& target);
bool IsInUnsafeSpan(const ScopeContext& ctx, const core::Span& span);
TypeRef StripPerm(const TypeRef& type);

// Internal helpers exposed for split expression files
bool IsCapabilityType(const TypeRef& type);
bool IsImpureType(const TypeRef& type);
bool ParamsPure(const ScopeContext& ctx,
                const std::vector<TypeFuncParam>& params);
bool ParamsPure(const ScopeContext& ctx,
                const std::vector<syntax::Param>& params,
                const std::function<LowerTypeResult(
                    const std::shared_ptr<syntax::Type>&)>& lower_type);
TypeRef SubstSelfType(const TypeRef& self, const TypeRef& type);
bool PermSub(Permission lhs, Permission rhs);
Permission PermOfType(const TypeRef& type);
bool IsPrimType(const TypeRef& type, std::string_view name);

// NOTE: SizeOf and AlignOf are declared in type_layout.h

}  // namespace cursive0::analysis

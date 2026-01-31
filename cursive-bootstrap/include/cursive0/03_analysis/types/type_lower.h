// =================================================================
// File: 03_analysis/types/type_lower.h
// Construct: Syntax Type to Analysis Type Lowering
// Spec Section: 5.2.3, 5.2.9
// =================================================================
#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::analysis {

// Result of lowering a syntax type to an analysis type
struct LowerTypeResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

// §5.2.3 WF-Apply: Lower syntax type to analysis type
LowerTypeResult LowerType(const ScopeContext& ctx,
                          const std::shared_ptr<syntax::Type>& type);

// Helper functions for lowering syntax constructs to analysis types

Permission LowerPermission(syntax::TypePerm perm);

std::optional<ParamMode> LowerParamMode(
    const std::optional<syntax::ParamMode>& mode);

RawPtrQual LowerRawPtrQual(syntax::RawPtrQual qual);

std::optional<StringState> LowerStringState(
    const std::optional<syntax::StringState>& state);

std::optional<BytesState> LowerBytesState(
    const std::optional<syntax::BytesState>& state);

std::optional<PtrState> LowerPtrState(
    const std::optional<syntax::PtrState>& state);

}  // namespace cursive0::analysis

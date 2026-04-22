#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "04_analysis/typing/context.h"
#include "04_analysis/modal/modal.h"
#include "04_analysis/resolve/scopes.h"
#include "04_analysis/typing/types.h"
#include "02_source/ast/ast.h"

namespace cursive::analysis {

const ast::StateFieldDecl* LookupModalFieldDecl(const ast::ModalDecl& decl,
                                                   std::string_view state,
                                                   std::string_view name);

struct ModalPayloadFieldTypeResult {
  bool ok = false;
  std::optional<std::string_view> diag_id;
  TypeRef type;
};

ModalPayloadFieldTypeResult ModalPayloadFieldType(
    const ScopeContext& ctx,
    const ast::ModalDecl& decl,
    std::string_view state,
    std::string_view name,
    const std::vector<TypeRef>& modal_args);

bool ModalFieldVisible(const ScopeContext& ctx, const TypePath& modal_path);

}  // namespace cursive::analysis

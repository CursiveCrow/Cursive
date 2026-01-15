#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "cursive0/sema/context.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

constexpr std::uint64_t kWidenLargePayloadThresholdBytes = 256;

std::optional<std::string_view> PayloadState(const ScopeContext& ctx,
                                             const syntax::ModalDecl& decl);

bool NicheApplies(const ScopeContext& ctx, const syntax::ModalDecl& decl);

bool NicheCompatible(const ScopeContext& ctx,
                     const TypePath& modal_path,
                     std::string_view state);

bool WidenWarnCond(const ScopeContext& ctx,
                   const TypePath& modal_path,
                   std::string_view state);

}  // namespace cursive0::sema

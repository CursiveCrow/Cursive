#pragma once

#include <span>

#include "02_source/ast/ast.h"

namespace cursive::analysis {

bool FindInnermostDynamic(std::span<const ast::AttributeList* const> ancestors);

bool ComputeDynamicContext(
    std::span<const ast::AttributeList* const> ancestors,
    bool fallback_dynamic = false);

}  // namespace cursive::analysis


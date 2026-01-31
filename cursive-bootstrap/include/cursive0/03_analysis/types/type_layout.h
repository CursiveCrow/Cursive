// =================================================================
// File: 03_analysis/types/type_layout.h
// Construct: Type Layout Computation
// Spec Section: 5.2.12
// =================================================================
#pragma once

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/types.h"

namespace cursive0::analysis {

// Target pointer size and alignment
constexpr std::uint64_t kPtrSize = 8;
constexpr std::uint64_t kPtrAlign = 8;

// Primitive type size (returns nullopt for unknown types)
std::optional<std::uint64_t> PrimSize(std::string_view name);

// Primitive type alignment (returns nullopt for unknown types)
std::optional<std::uint64_t> PrimAlign(std::string_view name);

// Align a value up to the given alignment
std::uint64_t AlignUp(std::uint64_t value, std::uint64_t align);

// Compute discriminant type name for union/enum with given number of variants
std::optional<std::string_view> DiscTypeName(std::uint64_t max_disc);

// Compute discriminant type layout (size, align) for union/enum
std::optional<std::pair<std::uint64_t, std::uint64_t>> DiscTypeLayout(
    std::uint64_t max_disc);

// Compute sizeof for a type
std::optional<std::uint64_t> SizeOf(const ScopeContext& ctx,
                                    const TypeRef& type);

// Compute alignof for a type
std::optional<std::uint64_t> AlignOf(const ScopeContext& ctx,
                                     const TypeRef& type);

}  // namespace cursive0::analysis

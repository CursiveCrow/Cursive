#pragma once

// =============================================================================
// pattern/enum_pattern.h - Enum Pattern Lowering
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Lines 16772-16780
//   - TagOf-Enum: Get enum discriminant for variant
//   - EnumValuePath, VariantIndex, EnumDisc
//
// Provides functions for:
//   - RegisterEnumPatternBindings: Register variable bindings for enum patterns
//   - LowerBindEnumPattern: Lower enum pattern binding to IR
//   - PatternCheckEnum: Check if a value matches an enum pattern
//
// =============================================================================

#include "05_codegen/lower/lower_pat.h"

#include <functional>
#include <optional>
#include <string>

namespace cursive::ast {
struct EnumPattern;
struct Pattern;
}  // namespace cursive::ast

namespace cursive::codegen {

// ============================================================================
// RegisterEnumPatternBindings
// ============================================================================
//
// Registers variable bindings introduced by an enum pattern.
// Called from RegisterPatternBindings dispatch in pattern_common.cpp.
//
// Parameters:
//   - node: The EnumPattern AST node
//   - hint: Type hint for the pattern (the enum type)
//   - ctx: Lowering context
//   - is_immovable: Whether bindings should be immovable (:=)
//   - prov: Provenance kind for bindings
//   - prov_region: Optional region name for provenance
//   - walk: Callback to recursively register nested patterns
//
void RegisterEnumPatternBindings(
    const ast::EnumPattern& node,
    const analysis::TypeRef& hint,
    LowerCtx& ctx,
    bool is_immovable,
    analysis::ProvenanceKind prov,
    std::optional<std::string> prov_region,
    std::optional<std::string> prov_region_tag,
    std::function<void(const ast::Pattern&, analysis::TypeRef)> walk);

// ============================================================================
// LowerBindEnumPattern
// ============================================================================
//
// Lowers an enum pattern binding to IR.
// Creates derived values for payload extraction and recursively binds
// elements (tuple payload) or fields (record payload).
//
// Parameters:
//   - pat: The EnumPattern AST node
//   - value: The scrutinee value being matched
//   - ctx: Lowering context
//   - lookup_bind_type: Callback to look up a binding's type
//   - lookup_bind_prov: Callback to look up a binding's provenance
//   - lookup_bind_region: Callback to look up a binding's region
//   - lower_bind: Callback to recursively lower nested pattern bindings
//
// Returns:
//   IR sequence that performs the binding operations
//
IRPtr LowerBindEnumPattern(
    const ast::EnumPattern& pat,
    const IRValue& value,
    LowerCtx& ctx,
    std::function<analysis::TypeRef(const std::string&)> lookup_bind_type,
    std::function<analysis::ProvenanceKind(const std::string&)> lookup_bind_prov,
    std::function<std::optional<std::string>(const std::string&)>
        lookup_bind_region,
    std::function<std::optional<std::string>(const std::string&)>
        lookup_bind_region_tag,
    std::function<IRPtr(const ast::Pattern&, const IRValue&)> lower_bind);

// ============================================================================
// PatternCheckEnum
// ============================================================================
//
// Checks if a value matches an enum pattern by comparing discriminant tags.
// Called from PatternCheck dispatch in pattern_common.cpp.
//
// Parameters:
//   - pat: The EnumPattern AST node
//   - value: The scrutinee value being matched
//   - ctx: Lowering context
//
// Returns:
//   IRValue representing the boolean result of the tag comparison
//
// SPEC: CursiveSpecification.md Lines 16777-16780 (TagOf-Enum)
//
IRValue PatternCheckEnum(const ast::EnumPattern& pat,
                         const IRValue& value,
                         LowerCtx& ctx);

}  // namespace cursive::codegen

// =============================================================================
// VTable Emission Implementation
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.10 VTable Emission (lines 17597-17645)
//   - Mangle-VTable rule (lines 15517-15520)
//   - Linkage-VTable rule (lines 15620-15623)
//   - DynLayout rule (lines 17556-17558)
//
// =============================================================================

#include "05_codegen/dyn_dispatch/vtable_emit.h"

#include <algorithm>
#include <variant>

#include "00_core/assert_spec.h"
#include "05_codegen/symbols/mangle.h"

namespace cursive::codegen {

// =============================================================================
// Platform Constants
// =============================================================================

namespace {

/// Pointer size for the target platform (64-bit).
constexpr std::uint64_t kPtrSize = 8;

/// Pointer alignment for the target platform.
constexpr std::uint64_t kPtrAlign = 8;

/// Prefix used for vtable symbols.
constexpr std::string_view kVTablePrefix = "_ZTV";  // C++ style vtable prefix

}  // namespace

// =============================================================================
// VTable Header Layout Implementation
// =============================================================================

VTableHeaderLayout GetVTableHeaderLayout() {
  VTableHeaderLayout layout;
  layout.size_offset = 0;
  layout.align_offset = kPtrSize;
  layout.drop_offset = 2 * kPtrSize;
  layout.slots_offset = 3 * kPtrSize;
  layout.slot_size = kPtrSize;
  return layout;
}

// =============================================================================
// VTable Symbol Resolution Implementation
// =============================================================================

std::string ResolveVTableSlotSymbol(
    const analysis::TypeRef& impl_type,
    const analysis::TypePath& class_path,
    const std::string& method_name,
    [[maybe_unused]] LowerCtx& ctx) {
  SPEC_RULE("DispatchSym-Impl");

  // Build the method symbol for the implementing type
  // This will be resolved during VTable generation
  std::vector<std::string> path;

  // Get the type path
  if (impl_type) {
    auto type_path = PathOfType(impl_type);
    path.insert(path.end(), type_path.begin(), type_path.end());
  }

  path.push_back(method_name);

  return ScopedSym(path);
}

std::string GetDropGlueSymbol(const analysis::TypeRef& type) {
  SPEC_RULE("DropGlueSym");

  if (!type) {
    return "";
  }

  // Build drop glue symbol: __drop_<type_path>
  auto type_path = PathOfType(type);
  std::vector<std::string> path = {"__drop"};
  path.insert(path.end(), type_path.begin(), type_path.end());

  return ScopedSym(path);
}

// =============================================================================
// VTable IR Generation Implementation
// =============================================================================

GlobalVTable GenerateVTableIR(
    const analysis::TypeRef& type,
    const analysis::TypePath& class_path,
    const ast::ClassDecl& class_decl,
    LowerCtx& ctx) {
  SPEC_RULE("VTable-Order");

  GlobalVTable vtable;

  // Generate the vtable symbol
  vtable.symbol = MangleVTable(type, class_path);

  // Get type size and alignment
  // These would normally come from the layout module
  // Initialize with defaults; finalized values are filled during emission
  vtable.header.size = 0;   // Will be computed from type layout
  vtable.header.align = 1;  // Will be computed from type layout

  // Get drop glue symbol
  vtable.header.drop_sym = GetDropGlueSymbol(type);

  // Get vtable-eligible methods
  auto eligible_methods = VTableEligible(class_decl);

  // Generate slot symbols in order
  vtable.slots.reserve(eligible_methods.size());
  for (const auto& method_name : eligible_methods) {
    std::string slot_sym = ResolveVTableSlotSymbol(type, class_path, method_name, ctx);
    vtable.slots.push_back(slot_sym);
  }

  return vtable;
}

// =============================================================================
// VTable Validation Implementation
// =============================================================================

bool ValidateVTable(
    const GlobalVTable& vtable,
    std::vector<std::string>& error_messages) {
  SPEC_RULE("ValidateVTable");

  bool valid = true;

  // Check that the symbol is not empty
  if (vtable.symbol.empty()) {
    error_messages.push_back("VTable symbol is empty");
    valid = false;
  }

  // Check that drop symbol is present (can be empty for Bitcopy types)
  // We don't require drop_sym to be non-empty

  // Check that all slots have symbols
  for (std::size_t i = 0; i < vtable.slots.size(); ++i) {
    if (vtable.slots[i].empty()) {
      error_messages.push_back("VTable slot " + std::to_string(i) + " has empty symbol");
      valid = false;
    }
  }

  return valid;
}

bool IsValidSlotTarget(
    [[maybe_unused]] const std::string& symbol,
    [[maybe_unused]] LowerCtx& ctx) {
  // A symbol is a valid slot target if it's a non-empty string
  // More detailed validation would require looking up the symbol
  // in the symbol table and checking its signature
  return !symbol.empty();
}

// =============================================================================
// VTable References Implementation
// =============================================================================

std::vector<std::string> CollectVTableRefs(const IRDecls& decls) {
  SPEC_RULE("VTableRefs");

  std::vector<std::string> vtable_refs;

  for (const auto& decl : decls) {
    if (const auto* vtable = std::get_if<GlobalVTable>(&decl)) {
      vtable_refs.push_back(vtable->symbol);
    }
  }

  return vtable_refs;
}

bool IsVTableSymbol(const std::string& symbol) {
  // Check if the symbol starts with the vtable prefix
  // or contains "vtable" in the mangled name
  if (symbol.find(kVTablePrefix) == 0) {
    return true;
  }
  if (symbol.find("vtable") != std::string::npos) {
    return true;
  }
  return false;
}

// =============================================================================
// Spec Rule Anchors
// =============================================================================

void AnchorVTableEmitRules() {
  // VTable generation
  SPEC_RULE("VTable-Order");
  SPEC_RULE("VTable-Slot");

  // Symbol resolution
  SPEC_RULE("DispatchSym-Impl");
  SPEC_RULE("DispatchSym-Default-None");
  SPEC_RULE("DispatchSym-Default-Mismatch");
  SPEC_RULE("DropGlueSym");

  // LLVM emission
  SPEC_RULE("EmitVTable");
  SPEC_RULE("EmitVTable-Header");
  SPEC_RULE("EmitVTable-Slots");
  SPEC_RULE("EmitVTable-Err");
  SPEC_RULE("LowerIRDecl-VTable");

  // Validation
  SPEC_RULE("ValidateVTable");
  SPEC_RULE("VTableRefs");
}

}  // namespace cursive::codegen

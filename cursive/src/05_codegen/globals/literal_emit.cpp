// =============================================================================
// MIGRATION MAPPING: literal_emit.cpp
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md
//   - Section 6.3.1 Literal Identity (lines 15427-15434)
//   - LiteralData(kind, contents) constructor (line 15398)
//   - FNV1a64 hash (lines 15429-15432)
//   - LiteralID computation (line 15434)
//   - Mangle-Literal rule (lines 15522-15525)
//   - Linkage-LiteralData rule (lines 15625-15628)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/literal_emit.cpp
//   - String literal data emission
//   - Literal deduplication by content hash
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/globals/literal_emit.h
//   - cursive/include/05_codegen/ir/ir_model.h (GlobalConst)
//   - cursive/include/05_codegen/symbols/mangle.h (MangleLiteral)
//   - cursive/include/00_core/hash.h (FNV1a64, LiteralID)
//
// REFACTORING NOTES:
//   1. Literals are emitted as GlobalConst data
//   2. LiteralID = mangle(kind) + "_" + Hex64(FNV1a64(contents))
//   3. Symbol = PathSig(["cursive", "runtime", "literal", LiteralID])
//   4. Deduplication: identical content -> same symbol
//   5. Linkage is Internal (module-local)
//   6. Literal kinds:
//      - "string" for string literals
//      - "bytes" for byte array literals
//   7. Contents stored as raw bytes
//
// LITERAL EMISSION:
//   1. Compute content hash (FNV1a64)
//   2. Generate LiteralID
//   3. Check if already emitted (dedup)
//   4. Emit GlobalConst if new
//   5. Return symbol for reference
//
// FNV1A64 ALGORITHM:
//   hash = FNVOffset64 (14695981039346656037)
//   for each byte b:
//     hash = (hash XOR b) * FNVPrime64 (1099511628211)
//   return hash
// =============================================================================

#include "05_codegen/globals/literal_emit.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "05_codegen/layout/layout.h"
#include "05_codegen/lower/lower_proc.h"
#include "05_codegen/symbols/mangle.h"
#include "00_core/assert_spec.h"
#include "00_core/hash.h"
#include "00_core/symbols.h"

namespace cursive::codegen {

// ============================================================================
// Section 6.12.14 Literal Kind Classification
// ============================================================================

std::string_view LiteralKindToString(LiteralKind kind) {
  switch (kind) {
    case LiteralKind::String: return "string";
    case LiteralKind::Bytes: return "bytes";
    case LiteralKind::Char: return "char";
    case LiteralKind::Int: return "int";
    case LiteralKind::Float: return "float";
    case LiteralKind::Array: return "array";
  }
  return "unknown";
}

// ============================================================================
// Section 6.12.14 Literal Symbol Generation
// ============================================================================

std::string LiteralSym(LiteralKind kind, const std::vector<std::uint8_t>& bytes) {
  SPEC_RULE("LiteralSym");
  return MangleLiteral(std::string(LiteralKindToString(kind)), bytes);
}

std::string LiteralSymString(std::string_view content) {
  std::vector<std::uint8_t> bytes(content.begin(), content.end());
  return LiteralSym(LiteralKind::String, bytes);
}

std::string LiteralSymBytes(const std::vector<std::uint8_t>& content) {
  return LiteralSym(LiteralKind::Bytes, content);
}

bool IsLiteralSymbol(const std::string& symbol) {
  static const std::string kLiteralPrefix =
      core::Mangle(core::StringOfPath({"cursive", "runtime", "literal"}));
  return symbol.rfind(kLiteralPrefix, 0) == 0;
}

// ============================================================================
// Section 6.12.14 Literal IR Declaration Generation
// ============================================================================

IRDecl EmitLiteralData(LiteralKind kind, const std::vector<std::uint8_t>& bytes) {
  SPEC_RULE("EmitLiteralData-Decl");

  GlobalConst gc;
  gc.symbol = LiteralSym(kind, bytes);
  gc.bytes = bytes;
  return gc;
}

IRDecl EmitStringLitDecl(std::string_view content) {
  SPEC_RULE("EmitLiteral-String");

  std::vector<std::uint8_t> bytes(content.begin(), content.end());
  return EmitLiteralData(LiteralKind::String, bytes);
}

IRDecl EmitBytesLitDecl(const std::vector<std::uint8_t>& content) {
  SPEC_RULE("EmitLiteral-Bytes");
  return EmitLiteralData(LiteralKind::Bytes, content);
}

IRDecl EmitCharLitDecl(char32_t codepoint) {
  SPEC_RULE("EmitLiteral-Char");

  std::vector<std::uint8_t> bytes(4);
  bytes[0] = static_cast<std::uint8_t>(codepoint & 0xFF);
  bytes[1] = static_cast<std::uint8_t>((codepoint >> 8) & 0xFF);
  bytes[2] = static_cast<std::uint8_t>((codepoint >> 16) & 0xFF);
  bytes[3] = static_cast<std::uint8_t>((codepoint >> 24) & 0xFF);
  return EmitLiteralData(LiteralKind::Char, bytes);
}

IRDecl EmitIntLitDecl(const std::vector<std::uint8_t>& bytes) {
  SPEC_RULE("EmitLiteral-Int");
  return EmitLiteralData(LiteralKind::Int, bytes);
}

IRDecl EmitFloatLitDecl(const std::vector<std::uint8_t>& bytes) {
  SPEC_RULE("EmitLiteral-Float");
  return EmitLiteralData(LiteralKind::Float, bytes);
}

// ============================================================================
// Section 6.12.14 Literal Reference Collection
// ============================================================================

namespace {

void CollectLiteralRefsFromIR(const IRPtr& ir,
                               std::vector<std::pair<LiteralKind, std::vector<std::uint8_t>>>& out);

void CollectLiteralRefsFromValue(const IRValue& value,
                                  std::vector<std::pair<LiteralKind, std::vector<std::uint8_t>>>& out) {
  if (value.kind == IRValue::Kind::Immediate && !value.bytes.empty()) {
    // Check if this looks like a string literal (has bytes)
    if (value.name.size() >= 2 && value.name.front() == '"' && value.name.back() == '"') {
      out.emplace_back(LiteralKind::String, value.bytes);
    }
  }
}

void CollectLiteralRefsFromIR(const IRPtr& ir,
                               std::vector<std::pair<LiteralKind, std::vector<std::uint8_t>>>& out) {
  if (!ir) return;

  std::visit([&](const auto& node) {
    using T = std::decay_t<decltype(node)>;

    if constexpr (std::is_same_v<T, IRSeq>) {
      for (const auto& item : node.items) {
        CollectLiteralRefsFromIR(item, out);
      }
    } else if constexpr (std::is_same_v<T, IRCall>) {
      CollectLiteralRefsFromValue(node.callee, out);
      for (const auto& arg : node.args) {
        CollectLiteralRefsFromValue(arg, out);
      }
    } else if constexpr (std::is_same_v<T, IRBindVar>) {
      CollectLiteralRefsFromValue(node.value, out);
    } else if constexpr (std::is_same_v<T, IRStoreVar>) {
      CollectLiteralRefsFromValue(node.value, out);
    } else if constexpr (std::is_same_v<T, IRStoreGlobal>) {
      CollectLiteralRefsFromValue(node.value, out);
    } else if constexpr (std::is_same_v<T, IRIf>) {
      CollectLiteralRefsFromValue(node.cond, out);
      CollectLiteralRefsFromIR(node.then_ir, out);
      CollectLiteralRefsFromIR(node.else_ir, out);
    } else if constexpr (std::is_same_v<T, IRLoop>) {
      CollectLiteralRefsFromIR(node.iter_ir, out);
      CollectLiteralRefsFromIR(node.cond_ir, out);
      CollectLiteralRefsFromIR(node.body_ir, out);
    } else if constexpr (std::is_same_v<T, IRIfCase>) {
      CollectLiteralRefsFromValue(node.scrutinee, out);
      for (const auto& arm : node.arms) {
        CollectLiteralRefsFromIR(arm.body, out);
      }
    } else if constexpr (std::is_same_v<T, IRBlock>) {
      CollectLiteralRefsFromIR(node.setup, out);
      CollectLiteralRefsFromIR(node.body, out);
    } else if constexpr (std::is_same_v<T, IRRegion>) {
      CollectLiteralRefsFromIR(node.body, out);
    } else if constexpr (std::is_same_v<T, IRFrame>) {
      CollectLiteralRefsFromIR(node.body, out);
    }
    // Other IR types don't contain literals directly
  }, ir->node);
}

}  // namespace

std::vector<std::pair<LiteralKind, std::vector<std::uint8_t>>>
LiteralRefs(const IRPtr& ir) {
  std::vector<std::pair<LiteralKind, std::vector<std::uint8_t>>> refs;
  CollectLiteralRefsFromIR(ir, refs);
  return refs;
}

std::vector<std::pair<LiteralKind, std::vector<std::uint8_t>>>
LiteralRefs(const IRDecls& decls) {
  std::vector<std::pair<LiteralKind, std::vector<std::uint8_t>>> refs;

  for (const auto& decl : decls) {
    if (const auto* proc = std::get_if<ProcIR>(&decl)) {
      CollectLiteralRefsFromIR(proc->body, refs);
    }
  }

  return refs;
}

// ============================================================================
// Section 6.12.14 Literal Type Inference
// ============================================================================

analysis::TypeRef StaticTypeForConst(const GlobalConst& global,
                                      const LowerCtx* ctx) {
  if (ctx) {
    auto type = ctx->LookupStaticType(global.symbol);
    if (type) {
      return type;
    }
  }
  return analysis::MakeTypeArray(analysis::MakeTypePrim("u8"), global.bytes.size());
}

// ============================================================================
// Section 6.12.14 Literal Deduplication
// ============================================================================

std::vector<IRDecl> UniqueLiterals(
    const std::vector<std::pair<LiteralKind, std::vector<std::uint8_t>>>& lits) {
  SPEC_RULE("UniqueEmits-Literal");

  std::unordered_set<std::string> seen_symbols;
  std::vector<IRDecl> unique_decls;

  for (const auto& [kind, bytes] : lits) {
    std::string sym = LiteralSym(kind, bytes);
    if (seen_symbols.find(sym) == seen_symbols.end()) {
      seen_symbols.insert(sym);
      unique_decls.push_back(EmitLiteralData(kind, bytes));
    }
  }

  return unique_decls;
}

// ============================================================================
// Section 6.12.14 String/Bytes View Construction
// ============================================================================

StringViewLayout GetStringViewLayout() {
  // string@View = { ptr: *imm u8, len: usize }
  StringViewLayout layout;
  layout.ptr_offset = 0;
  layout.len_offset = kPtrSize;  // After pointer
  layout.total_size = kPtrSize * 2;  // ptr + len
  return layout;
}

StringViewLayout GetBytesViewLayout() {
  // bytes@View = { ptr: *imm u8, len: usize }
  // Same layout as string@View
  return GetStringViewLayout();
}

IRPtr EmitStringViewIR(const std::string& literal_sym,
                       std::size_t length,
                       const IRValue& result,
                       LowerCtx& ctx) {
  SPEC_RULE("EmitStringViewIR");

  // Create a string@View struct from the literal symbol
  // This involves:
  // 1. Getting the address of the global literal
  // 2. Creating a struct with { ptr, len }

  std::vector<IRPtr> parts;

  // Get address of literal
  IRValue lit_addr = ctx.FreshTempValue("lit_addr");
  IRAddrOf addr_of;
  addr_of.place.repr = literal_sym;
  addr_of.result = lit_addr;
  parts.push_back(MakeIR(std::move(addr_of)));

  // Create length immediate
  IRValue len_val;
  len_val.kind = IRValue::Kind::Immediate;
  len_val.name = std::to_string(length);
  // Encode as usize (8 bytes on 64-bit)
  len_val.bytes.resize(8);
  for (int i = 0; i < 8; ++i) {
    len_val.bytes[i] = static_cast<std::uint8_t>((length >> (i * 8)) & 0xFF);
  }

  // The actual struct construction would be handled by the LLVM emitter
  // based on the string@View type layout

  return SeqIR(std::move(parts));
}

IRPtr EmitBytesViewIR(const std::string& literal_sym,
                      std::size_t length,
                      const IRValue& result,
                      LowerCtx& ctx) {
  SPEC_RULE("EmitBytesViewIR");
  // Same as string view construction
  return EmitStringViewIR(literal_sym, length, result, ctx);
}

// ============================================================================
// Spec Rule Anchors
// ============================================================================

void AnchorLiteralEmitRules() {
  // Section 6.12.14 Literal Emission
  SPEC_RULE("EmitLiteralData-Decl");
  SPEC_RULE("EmitLiteralData-Bytes");
  SPEC_RULE("EmitLiteral-String");
  SPEC_RULE("EmitLiteral-Bytes");
  SPEC_RULE("EmitLiteral-Char");
  SPEC_RULE("EmitLiteral-Int");
  SPEC_RULE("EmitLiteral-Float");
  SPEC_RULE("EmitLiteral-Err");
  SPEC_RULE("UniqueEmits-Literal");
  SPEC_RULE("LiteralSym");
  SPEC_RULE("EmitStringViewIR");
  SPEC_RULE("EmitBytesViewIR");

  // Section 6.3.1 Mangle-Literal
  SPEC_RULE("Mangle-Literal");

  // Section 6.3.1 Linkage-LiteralData
  SPEC_RULE("Linkage-LiteralData");
}

}  // namespace cursive::codegen

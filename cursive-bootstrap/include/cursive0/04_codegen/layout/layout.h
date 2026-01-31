#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "cursive0/00_core/int128.h"
#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/composite/enums.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/02_syntax/ast.h"

namespace cursive0::codegen {

struct Layout {
  std::uint64_t size = 0;
  std::uint64_t align = 1;
};

struct RecordLayout {
  Layout layout;
  std::vector<std::uint64_t> offsets;
};

struct EnumLayout {
  Layout layout;
  std::string disc_type;
  std::uint64_t payload_size = 0;
  std::uint64_t payload_align = 1;
};

struct UnionLayout {
  Layout layout;
  bool niche = false;
  std::optional<Layout> niche_payload_layout;
  std::optional<std::string> disc_type;
  std::uint64_t payload_size = 0;
  std::uint64_t payload_align = 1;
  std::vector<cursive0::analysis::TypeRef> member_list;
};

struct ModalLayout {
  Layout layout;
  bool niche = false;
  std::optional<Layout> niche_payload_layout;
  std::optional<std::string> disc_type;
  std::uint64_t payload_size = 0;
};

struct DynLayout {
  Layout layout;
  std::vector<cursive0::analysis::TypeRef> fields;
};

// Primitive layout constants (Cursive0)
constexpr std::uint64_t kPtrSize = 8;
constexpr std::uint64_t kPtrAlign = 8;

std::optional<std::uint64_t> PrimSize(std::string_view name);
std::optional<std::uint64_t> PrimAlign(std::string_view name);

std::optional<cursive0::analysis::TypeRef> LowerTypeForLayout(
    const cursive0::analysis::ScopeContext& ctx,
    const std::shared_ptr<cursive0::syntax::Type>& type);

std::optional<Layout> LayoutOf(const cursive0::analysis::ScopeContext& ctx,
                               const cursive0::analysis::TypeRef& type);
std::optional<std::uint64_t> SizeOf(const cursive0::analysis::ScopeContext& ctx,
                                    const cursive0::analysis::TypeRef& type);
std::optional<std::uint64_t> AlignOf(const cursive0::analysis::ScopeContext& ctx,
                                     const cursive0::analysis::TypeRef& type);

std::optional<RecordLayout> RecordLayoutOf(
    const cursive0::analysis::ScopeContext& ctx,
    const std::vector<cursive0::analysis::TypeRef>& fields);

std::optional<RecordLayout> RangeLayoutOf(
    const cursive0::analysis::ScopeContext& ctx);

std::optional<EnumLayout> EnumLayoutOf(
    const cursive0::analysis::ScopeContext& ctx,
    const cursive0::syntax::EnumDecl& decl);

std::optional<UnionLayout> UnionLayoutOf(
    const cursive0::analysis::ScopeContext& ctx,
    const cursive0::analysis::TypeUnion& uni);

std::optional<ModalLayout> ModalLayoutOf(
    const cursive0::analysis::ScopeContext& ctx,
    const cursive0::syntax::ModalDecl& decl);

DynLayout DynLayoutOf();

// Value representations for ValueBits helpers.

struct Value;

struct BoolVal { bool value = false; };
struct CharVal { std::uint32_t value = 0; };
struct IntVal { std::string type; core::UInt128 value; };
struct FloatVal { std::string type; std::uint64_t bits = 0; };
struct UnitVal {};
struct PtrVal { cursive0::analysis::PtrState state; std::uint64_t addr = 0; };
struct RawPtrVal { cursive0::analysis::RawPtrQual qual; std::uint64_t addr = 0; };
struct TupleVal { std::vector<struct Value> elements; };
struct ArrayVal { std::vector<struct Value> elements; };
struct SliceVal { RawPtrVal ptr; std::uint64_t length = 0; };

// Value representation for range types (distinct from checks.h RangeVal)
enum class ValueRangeKind {
  To,
  ToInclusive,
  Full,
  From,
  Exclusive,
  Inclusive,
};

struct ValueRangeVal {
  ValueRangeKind kind = ValueRangeKind::Full;
  std::optional<std::uint64_t> lo;
  std::optional<std::uint64_t> hi;
};

struct RecordVal {
  std::vector<std::pair<std::string, struct Value>> fields;
};

struct EnumPayloadTupleVal { std::vector<struct Value> elements; };
struct EnumPayloadRecordVal { std::vector<std::pair<std::string, struct Value>> fields; };
using EnumPayloadVal = std::variant<EnumPayloadTupleVal, EnumPayloadRecordVal>;

struct EnumVal {
  std::string variant;
  std::optional<EnumPayloadVal> payload;
};

struct ModalVal {
  std::string state;
  std::shared_ptr<struct Value> payload;
};

struct UnionVal {
  cursive0::analysis::TypeRef member;
  std::shared_ptr<struct Value> value;
};

struct DynamicVal {
  std::uint64_t data = 0;
  std::uint64_t vtable = 0;
};

struct StringVal { std::vector<std::uint8_t> bytes; };
struct BytesVal { std::vector<std::uint8_t> bytes; };

struct Value {
  std::variant<BoolVal,
               CharVal,
               IntVal,
               FloatVal,
               UnitVal,
               PtrVal,
               RawPtrVal,
               TupleVal,
               ArrayVal,
               SliceVal,
               ValueRangeVal,
               RecordVal,
               EnumVal,
               ModalVal,
               UnionVal,
               DynamicVal,
               StringVal,
               BytesVal>
      node;
};

std::optional<std::vector<std::uint8_t>> EncodeConst(
    const cursive0::analysis::TypeRef& type,
    const cursive0::syntax::Token& lit);

// Decode a string literal lexeme into its UTF-8 byte sequence.
std::optional<std::vector<std::uint8_t>> DecodeStringLiteralBytes(
    std::string_view lexeme);

bool ValidValue(const cursive0::analysis::ScopeContext& ctx,
                const cursive0::analysis::TypeRef& type,
                const std::vector<std::uint8_t>& bits);

std::optional<std::vector<std::uint8_t>> ValueBits(
    const cursive0::analysis::ScopeContext& ctx,
    const cursive0::analysis::TypeRef& type,
    const Value& value);

}  // namespace cursive0::codegen

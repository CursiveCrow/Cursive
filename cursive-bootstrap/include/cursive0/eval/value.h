#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/sema/types.h"
#include "cursive0/semantics/address.h"

namespace cursive0::semantics {

struct Value;

struct BoolVal {
  bool value = false;
  bool operator==(const BoolVal& other) const { return value == other.value; }
};

struct CharVal {
  std::uint32_t value = 0;
  bool operator==(const CharVal& other) const { return value == other.value; }
};

struct UnitVal {
  bool operator==(const UnitVal&) const { return true; }
};

struct IntVal {
  std::string type;
  bool negative = false;
  core::UInt128 magnitude{};
};

struct FloatVal {
  std::string type;
  double value = 0.0;
};

struct PtrVal {
  sema::PtrState state = sema::PtrState::Valid;
  Addr addr = 0;
};

struct RawPtrVal {
  sema::RawPtrQual qual = sema::RawPtrQual::Imm;
  Addr addr = 0;
};

struct TupleVal {
  std::vector<Value> elements;
};

struct ArrayVal {
  std::vector<Value> elements;
};

enum class RangeKind {
  To,
  ToInclusive,
  Full,
  From,
  Exclusive,
  Inclusive,
};

struct RangeVal {
  RangeKind kind = RangeKind::Full;
  std::shared_ptr<Value> lo;
  std::shared_ptr<Value> hi;
};

struct SliceVal {
  std::vector<Value> base;
  RangeVal range;
};

struct RecordVal {
  sema::TypeRef record_type;
  std::vector<std::pair<std::string, Value>> fields;
};

struct EnumPayloadTupleVal {
  std::vector<Value> elements;
};

struct EnumPayloadRecordVal {
  std::vector<std::pair<std::string, Value>> fields;
};

using EnumPayloadVal =
    std::variant<EnumPayloadTupleVal, EnumPayloadRecordVal>;

struct EnumVal {
  sema::TypePath path;
  std::optional<EnumPayloadVal> payload;
};

struct ModalVal {
  std::string state;
  std::shared_ptr<Value> payload;
};

struct UnionVal {
  sema::TypeRef member;
  std::shared_ptr<Value> value;
};

struct DynamicVal {
  sema::TypePath class_path;
  RawPtrVal data;
  sema::TypeRef concrete_type;
};

struct StringVal {
  sema::StringState state = sema::StringState::View;
  std::vector<std::uint8_t> bytes;
};

struct BytesVal {
  sema::BytesState state = sema::BytesState::View;
  std::vector<std::uint8_t> bytes;
};

struct ProcRefVal {
  sema::TypePath module_path;
  std::string name;
};

struct RecordCtorVal {
  sema::TypePath path;
};

struct Value {
  using Variant = std::variant<BoolVal,
                               CharVal,
                               UnitVal,
                               IntVal,
                               FloatVal,
                               PtrVal,
                               RawPtrVal,
                               TupleVal,
                               ArrayVal,
                               RangeVal,
                               SliceVal,
                               RecordVal,
                               EnumVal,
                               ModalVal,
                               UnionVal,
                               DynamicVal,
                               StringVal,
                               BytesVal,
                               ProcRefVal,
                               RecordCtorVal>;

  Variant node;
};

bool TypeEqual(const sema::TypeRef& lhs, const sema::TypeRef& rhs);
bool ValueEqual(const Value& lhs, const Value& rhs);
std::string ValueToString(const Value& value);

}  // namespace cursive0::semantics

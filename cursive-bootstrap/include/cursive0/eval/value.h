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
#include "cursive0/analysis/types/types.h"
#include "cursive0/eval/address.h"

namespace cursive0::eval {

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
  analysis::PtrState state = analysis::PtrState::Valid;
  Addr addr = 0;
};

struct RawPtrVal {
  analysis::RawPtrQual qual = analysis::RawPtrQual::Imm;
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
  analysis::TypeRef record_type;
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
  analysis::TypePath path;
  std::optional<EnumPayloadVal> payload;
};

struct ModalVal {
  std::string state;
  std::shared_ptr<Value> payload;
};

struct UnionVal {
  analysis::TypeRef member;
  std::shared_ptr<Value> value;
};

struct DynamicVal {
  analysis::TypePath class_path;
  RawPtrVal data;
  analysis::TypeRef concrete_type;
};

struct StringVal {
  analysis::StringState state = analysis::StringState::View;
  std::vector<std::uint8_t> bytes;
};

struct BytesVal {
  analysis::BytesState state = analysis::BytesState::View;
  std::vector<std::uint8_t> bytes;
};

struct ProcRefVal {
  analysis::TypePath module_path;
  std::string name;
};

struct RecordCtorVal {
  analysis::TypePath path;
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

bool TypeEqual(const analysis::TypeRef& lhs, const analysis::TypeRef& rhs);
bool ValueEqual(const Value& lhs, const Value& rhs);
std::string ValueToString(const Value& value);

}  // namespace cursive0::eval

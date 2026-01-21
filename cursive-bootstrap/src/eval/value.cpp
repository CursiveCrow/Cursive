#include "cursive0/eval/value.h"

#include "cursive0/core/symbols.h"

#include <cmath>
#include <sstream>
#include <limits>
#include <iomanip>
#include <type_traits>
#include <string_view>

namespace cursive0::eval {

namespace {

bool UInt128Equal(core::UInt128 lhs, core::UInt128 rhs) {
  return core::UInt128High(lhs) == core::UInt128High(rhs) &&
         core::UInt128Low(lhs) == core::UInt128Low(rhs);
}

bool IntValEqual(const IntVal& lhs, const IntVal& rhs) {
  return lhs.type == rhs.type && lhs.negative == rhs.negative &&
         UInt128Equal(lhs.magnitude, rhs.magnitude);
}

bool FloatValEqual(const FloatVal& lhs, const FloatVal& rhs) {
  if (lhs.type != rhs.type) {
    return false;
  }
  if (std::isnan(lhs.value) || std::isnan(rhs.value)) {
    return false;
  }
  return lhs.value == rhs.value;
}

bool PtrValEqual(const PtrVal& lhs, const PtrVal& rhs) {
  return lhs.state == rhs.state && lhs.addr == rhs.addr;
}

bool RawPtrValEqual(const RawPtrVal& lhs, const RawPtrVal& rhs) {
  return lhs.qual == rhs.qual && lhs.addr == rhs.addr;
}

bool TupleValEqual(const TupleVal& lhs, const TupleVal& rhs);
bool ArrayValEqual(const ArrayVal& lhs, const ArrayVal& rhs);
bool RangeValEqual(const RangeVal& lhs, const RangeVal& rhs);
bool SliceValEqual(const SliceVal& lhs, const SliceVal& rhs);
bool RecordValEqual(const RecordVal& lhs, const RecordVal& rhs);
bool EnumValEqual(const EnumVal& lhs, const EnumVal& rhs);
bool ModalValEqual(const ModalVal& lhs, const ModalVal& rhs);
bool UnionValEqual(const UnionVal& lhs, const UnionVal& rhs);
bool DynamicValEqual(const DynamicVal& lhs, const DynamicVal& rhs);
bool StringValEqual(const StringVal& lhs, const StringVal& rhs);
bool BytesValEqual(const BytesVal& lhs, const BytesVal& rhs);
bool ProcRefValEqual(const ProcRefVal& lhs, const ProcRefVal& rhs);
bool RecordCtorValEqual(const RecordCtorVal& lhs, const RecordCtorVal& rhs);

bool ValuePtrEqual(const std::shared_ptr<Value>& lhs,
                   const std::shared_ptr<Value>& rhs) {
  if (!lhs || !rhs) {
    return lhs == rhs;
  }
  return ValueEqual(*lhs, *rhs);
}

bool TupleValEqual(const TupleVal& lhs, const TupleVal& rhs) {
  if (lhs.elements.size() != rhs.elements.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.elements.size(); ++i) {
    if (!ValueEqual(lhs.elements[i], rhs.elements[i])) {
      return false;
    }
  }
  return true;
}

bool ArrayValEqual(const ArrayVal& lhs, const ArrayVal& rhs) {
  if (lhs.elements.size() != rhs.elements.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.elements.size(); ++i) {
    if (!ValueEqual(lhs.elements[i], rhs.elements[i])) {
      return false;
    }
  }
  return true;
}

bool RangeValEqual(const RangeVal& lhs, const RangeVal& rhs) {
  return lhs.kind == rhs.kind && ValuePtrEqual(lhs.lo, rhs.lo) &&
         ValuePtrEqual(lhs.hi, rhs.hi);
}

bool SliceValEqual(const SliceVal& lhs, const SliceVal& rhs) {
  if (lhs.base.size() != rhs.base.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.base.size(); ++i) {
    if (!ValueEqual(lhs.base[i], rhs.base[i])) {
      return false;
    }
  }
  return RangeValEqual(lhs.range, rhs.range);
}

bool RecordValEqual(const RecordVal& lhs, const RecordVal& rhs) {
  if (!TypeEqual(lhs.record_type, rhs.record_type)) {
    return false;
  }
  if (lhs.fields.size() != rhs.fields.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.fields.size(); ++i) {
    if (lhs.fields[i].first != rhs.fields[i].first) {
      return false;
    }
    if (!ValueEqual(lhs.fields[i].second, rhs.fields[i].second)) {
      return false;
    }
  }
  return true;
}

bool EnumPayloadEqual(const EnumPayloadVal& lhs, const EnumPayloadVal& rhs) {
  if (lhs.index() != rhs.index()) {
    return false;
  }
  return std::visit(
      [&](const auto& left) {
        using T = std::decay_t<decltype(left)>;
        const auto& right = std::get<T>(rhs);
        if constexpr (std::is_same_v<T, EnumPayloadTupleVal>) {
          if (left.elements.size() != right.elements.size()) {
            return false;
          }
          for (std::size_t i = 0; i < left.elements.size(); ++i) {
            if (!ValueEqual(left.elements[i], right.elements[i])) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, EnumPayloadRecordVal>) {
          if (left.fields.size() != right.fields.size()) {
            return false;
          }
          for (std::size_t i = 0; i < left.fields.size(); ++i) {
            if (left.fields[i].first != right.fields[i].first) {
              return false;
            }
            if (!ValueEqual(left.fields[i].second, right.fields[i].second)) {
              return false;
            }
          }
          return true;
        }
        return false;
      },
      lhs);
}

bool EnumValEqual(const EnumVal& lhs, const EnumVal& rhs) {
  if (lhs.path != rhs.path) {
    return false;
  }
  if (lhs.payload.has_value() != rhs.payload.has_value()) {
    return false;
  }
  if (!lhs.payload.has_value()) {
    return true;
  }
  return EnumPayloadEqual(*lhs.payload, *rhs.payload);
}

bool ModalValEqual(const ModalVal& lhs, const ModalVal& rhs) {
  if (lhs.state != rhs.state) {
    return false;
  }
  if (!lhs.payload || !rhs.payload) {
    return lhs.payload == rhs.payload;
  }
  return ValueEqual(*lhs.payload, *rhs.payload);
}

bool UnionValEqual(const UnionVal& lhs, const UnionVal& rhs) {
  if (!TypeEqual(lhs.member, rhs.member)) {
    return false;
  }
  if (!lhs.value || !rhs.value) {
    return lhs.value == rhs.value;
  }
  return ValueEqual(*lhs.value, *rhs.value);
}

bool DynamicValEqual(const DynamicVal& lhs, const DynamicVal& rhs) {
  return lhs.class_path == rhs.class_path &&
         RawPtrValEqual(lhs.data, rhs.data) &&
         TypeEqual(lhs.concrete_type, rhs.concrete_type);
}

bool StringValEqual(const StringVal& lhs, const StringVal& rhs) {
  return lhs.state == rhs.state && lhs.bytes == rhs.bytes;
}

bool BytesValEqual(const BytesVal& lhs, const BytesVal& rhs) {
  return lhs.state == rhs.state && lhs.bytes == rhs.bytes;
}

bool ProcRefValEqual(const ProcRefVal& lhs, const ProcRefVal& rhs) {
  return lhs.module_path == rhs.module_path && lhs.name == rhs.name;
}

bool RecordCtorValEqual(const RecordCtorVal& lhs, const RecordCtorVal& rhs) {
  return lhs.path == rhs.path;
}


std::string JoinParts(const std::vector<std::string>& parts, std::string_view sep) {
  std::string out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      out.append(sep);
    }
    out.append(parts[i]);
  }
  return out;
}

std::string UInt128ToString(core::UInt128 value) {
  const auto high = core::UInt128High(value);
  const auto low = core::UInt128Low(value);
  if (high == 0) {
    return std::to_string(low);
  }
  std::ostringstream oss;
  oss << "0x" << std::hex << std::uppercase << high
      << std::setw(16) << std::setfill('0') << low;
  return oss.str();
}

std::string BytesHex(const std::vector<std::uint8_t>& bytes) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  std::string out;
  out.reserve(bytes.size() * 2);
  for (auto b : bytes) {
    out.push_back(kHex[(b >> 4) & 0xF]);
    out.push_back(kHex[b & 0xF]);
  }
  return out;
}

std::string PtrStateName(analysis::PtrState state) {
  switch (state) {
    case analysis::PtrState::Valid:
      return "valid";
    case analysis::PtrState::Null:
      return "null";
    case analysis::PtrState::Expired:
      return "expired";
  }
  return "unknown";
}

std::string RawPtrQualName(analysis::RawPtrQual qual) {
  switch (qual) {
    case analysis::RawPtrQual::Imm:
      return "const";
    case analysis::RawPtrQual::Mut:
      return "mut";
  }
  return "unknown";
}

std::string StringStateName(analysis::StringState state) {
  switch (state) {
    case analysis::StringState::View:
      return "View";
    case analysis::StringState::Managed:
      return "Managed";
  }
  return "Unknown";
}

std::string BytesStateName(analysis::BytesState state) {
  switch (state) {
    case analysis::BytesState::View:
      return "View";
    case analysis::BytesState::Managed:
      return "Managed";
  }
  return "Unknown";
}

std::string RangeKindName(RangeKind kind) {
  switch (kind) {
    case RangeKind::To:
      return "to";
    case RangeKind::ToInclusive:
      return "to_inclusive";
    case RangeKind::Full:
      return "full";
    case RangeKind::From:
      return "from";
    case RangeKind::Exclusive:
      return "exclusive";
    case RangeKind::Inclusive:
      return "inclusive";
  }
  return "unknown";
}

std::string ValueToStringImpl(const Value& value);

std::string RangeToString(const RangeVal& range) {
  const auto lo = range.lo ? ValueToStringImpl(*range.lo) : "_";
  const auto hi = range.hi ? ValueToStringImpl(*range.hi) : "_";
  return "range(" + RangeKindName(range.kind) + "," + lo + "," + hi + ")";
}

std::string ValueToStringImpl(const Value& value) {
  return std::visit(
      [&](const auto& node) -> std::string {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, BoolVal>) {
          return node.value ? "true" : "false";
        } else if constexpr (std::is_same_v<T, CharVal>) {
          return "char(" + std::to_string(node.value) + ")";
        } else if constexpr (std::is_same_v<T, UnitVal>) {
          return "()";
        } else if constexpr (std::is_same_v<T, IntVal>) {
          const std::string sign = node.negative ? "-" : "";
          return node.type + "(" + sign + UInt128ToString(node.magnitude) + ")";
        } else if constexpr (std::is_same_v<T, FloatVal>) {
          std::ostringstream oss;
          oss << std::setprecision(std::numeric_limits<double>::max_digits10)
              << node.value;
          return node.type + "(" + oss.str() + ")";
        } else if constexpr (std::is_same_v<T, PtrVal>) {
          return "ptr(" + PtrStateName(node.state) + "," +
              std::to_string(node.addr) + ")";
        } else if constexpr (std::is_same_v<T, RawPtrVal>) {
          return "rawptr(" + RawPtrQualName(node.qual) + "," +
              std::to_string(node.addr) + ")";
        } else if constexpr (std::is_same_v<T, TupleVal>) {
          std::vector<std::string> elems;
          elems.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            elems.push_back(ValueToStringImpl(elem));
          }
          return "(" + JoinParts(elems, ", ") + ")";
        } else if constexpr (std::is_same_v<T, ArrayVal>) {
          std::vector<std::string> elems;
          elems.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            elems.push_back(ValueToStringImpl(elem));
          }
          return "[" + JoinParts(elems, ", ") + "]";
        } else if constexpr (std::is_same_v<T, RangeVal>) {
          return RangeToString(node);
        } else if constexpr (std::is_same_v<T, SliceVal>) {
          std::vector<std::string> elems;
          elems.reserve(node.base.size());
          for (const auto& elem : node.base) {
            elems.push_back(ValueToStringImpl(elem));
          }
          return "slice([" + JoinParts(elems, ", ") + "]," +
              RangeToString(node.range) + ")";
        } else if constexpr (std::is_same_v<T, RecordVal>) {
          std::vector<std::string> fields;
          fields.reserve(node.fields.size());
          for (const auto& field : node.fields) {
            fields.push_back(field.first + "=" + ValueToStringImpl(field.second));
          }
          const std::string type_str = node.record_type
              ? analysis::TypeToString(node.record_type)
              : std::string("<unknown>");
          return "record(" + type_str + "){" + JoinParts(fields, ", ") + "}";
        } else if constexpr (std::is_same_v<T, EnumVal>) {
          std::string out = "enum(" + core::StringOfPath(node.path);
          if (!node.payload.has_value()) {
            return out + ")";
          }
          const auto& payload = *node.payload;
          if (std::holds_alternative<EnumPayloadTupleVal>(payload)) {
            const auto& tuple = std::get<EnumPayloadTupleVal>(payload);
            std::vector<std::string> elems;
            elems.reserve(tuple.elements.size());
            for (const auto& elem : tuple.elements) {
              elems.push_back(ValueToStringImpl(elem));
            }
            return out + ",(" + JoinParts(elems, ", ") + "))";
          }
          const auto& rec = std::get<EnumPayloadRecordVal>(payload);
          std::vector<std::string> fields;
          fields.reserve(rec.fields.size());
          for (const auto& field : rec.fields) {
            fields.push_back(field.first + "=" + ValueToStringImpl(field.second));
          }
          return out + ",{" + JoinParts(fields, ", ") + "})";
        } else if constexpr (std::is_same_v<T, ModalVal>) {
          const auto payload = node.payload ? ValueToStringImpl(*node.payload) : "_";
          return "modal(" + node.state + "," + payload + ")";
        } else if constexpr (std::is_same_v<T, UnionVal>) {
          const std::string member = node.member
              ? analysis::TypeToString(node.member)
              : std::string("<unknown>");
          const auto payload = node.value ? ValueToStringImpl(*node.value) : "_";
          return "union(" + member + "," + payload + ")";
        } else if constexpr (std::is_same_v<T, DynamicVal>) {
          return "dynamic(" + core::StringOfPath(node.class_path) + "," +
              std::to_string(node.data.addr) + ")";
        } else if constexpr (std::is_same_v<T, StringVal>) {
          return "string@" + StringStateName(node.state) + "(" + BytesHex(node.bytes) + ")";
        } else if constexpr (std::is_same_v<T, BytesVal>) {
          return "bytes@" + BytesStateName(node.state) + "(" + BytesHex(node.bytes) + ")";
        } else if constexpr (std::is_same_v<T, ProcRefVal>) {
          return "proc(" + core::StringOfPath(node.module_path) + "::" + node.name + ")";
        } else if constexpr (std::is_same_v<T, RecordCtorVal>) {
          return "record_ctor(" + core::StringOfPath(node.path) + ")";
        }
        return "<unknown>";
      },
      value.node);
}

}  // namespace

bool TypeEqual(const analysis::TypeRef& lhs, const analysis::TypeRef& rhs) {
  if (lhs == rhs) {
    return true;
  }
  if (!lhs || !rhs) {
    return false;
  }
  return analysis::TypeKeyEqual(analysis::TypeKeyOf(lhs), analysis::TypeKeyOf(rhs));
}

bool ValueEqual(const Value& lhs, const Value& rhs) {
  if (lhs.node.index() != rhs.node.index()) {
    return false;
  }
  return std::visit(
      [&](const auto& left) {
        using T = std::decay_t<decltype(left)>;
        const auto& right = std::get<T>(rhs.node);
        if constexpr (std::is_same_v<T, BoolVal>) {
          return left == right;
        } else if constexpr (std::is_same_v<T, CharVal>) {
          return left == right;
        } else if constexpr (std::is_same_v<T, UnitVal>) {
          return left == right;
        } else if constexpr (std::is_same_v<T, IntVal>) {
          return IntValEqual(left, right);
        } else if constexpr (std::is_same_v<T, FloatVal>) {
          return FloatValEqual(left, right);
        } else if constexpr (std::is_same_v<T, PtrVal>) {
          return PtrValEqual(left, right);
        } else if constexpr (std::is_same_v<T, RawPtrVal>) {
          return RawPtrValEqual(left, right);
        } else if constexpr (std::is_same_v<T, TupleVal>) {
          return TupleValEqual(left, right);
        } else if constexpr (std::is_same_v<T, ArrayVal>) {
          return ArrayValEqual(left, right);
        } else if constexpr (std::is_same_v<T, RangeVal>) {
          return RangeValEqual(left, right);
        } else if constexpr (std::is_same_v<T, SliceVal>) {
          return SliceValEqual(left, right);
        } else if constexpr (std::is_same_v<T, RecordVal>) {
          return RecordValEqual(left, right);
        } else if constexpr (std::is_same_v<T, EnumVal>) {
          return EnumValEqual(left, right);
        } else if constexpr (std::is_same_v<T, ModalVal>) {
          return ModalValEqual(left, right);
        } else if constexpr (std::is_same_v<T, UnionVal>) {
          return UnionValEqual(left, right);
        } else if constexpr (std::is_same_v<T, DynamicVal>) {
          return DynamicValEqual(left, right);
        } else if constexpr (std::is_same_v<T, StringVal>) {
          return StringValEqual(left, right);
        } else if constexpr (std::is_same_v<T, BytesVal>) {
          return BytesValEqual(left, right);
        } else if constexpr (std::is_same_v<T, ProcRefVal>) {
          return ProcRefValEqual(left, right);
        } else if constexpr (std::is_same_v<T, RecordCtorVal>) {
          return RecordCtorValEqual(left, right);
        }
        return false;
      },
      lhs.node);
}

std::string ValueToString(const Value& value) {
  return ValueToStringImpl(value);
}

}  // namespace cursive0::eval

#include "cursive0/eval/builtins.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/core/path.h"
#include "cursive0/core/unicode.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/memory/string_bytes.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/eval/eval.h"
#include "cursive0/eval/state.h"

namespace cursive0::eval {

namespace {

bool PathEq(const analysis::TypePath& path, std::initializer_list<std::string_view> parts) {
  if (path.size() != parts.size()) {
    return false;
  }
  std::size_t i = 0;
  for (const auto part : parts) {
    if (path[i++] != part) {
      return false;
    }
  }
  return true;
}

bool TargetExists(const Sigma& sigma, RegionTarget target) {
  for (const auto& entry : sigma.region_stack) {
    if (entry.target == target) {
      return true;
    }
  }
  return false;
}

bool TagExists(const Sigma& sigma, RegionTag tag) {
  for (const auto& entry : sigma.region_stack) {
    if (entry.tag == tag) {
      return true;
    }
  }
  return false;
}

RegionTarget FreshArena(Sigma& sigma) {
  RegionTarget candidate = sigma.next_region_target;
  while (TargetExists(sigma, candidate)) {
    ++candidate;
  }
  sigma.next_region_target = candidate + 1;
  return candidate;
}

RegionTag FreshTag(Sigma& sigma) {
  RegionTag candidate = sigma.next_region_tag;
  while (TagExists(sigma, candidate)) {
    ++candidate;
  }
  sigma.next_region_tag = candidate + 1;
  return candidate;
}

void RetagRegionEntries(Sigma& sigma, RegionTarget target) {
  for (auto it = sigma.region_stack.rbegin();
       it != sigma.region_stack.rend();
       ++it) {
    if (it->target != target) {
      continue;
    }
    it->tag = FreshTag(sigma);
  }
}

void RemoveRegionEntries(Sigma& sigma, RegionTarget target) {
  auto& stack = sigma.region_stack;
  stack.erase(std::remove_if(stack.begin(), stack.end(),
                             [target](const RegionEntry& entry) {
                               return entry.target == target;
                             }),
              stack.end());
}

Value UnionWrap(const analysis::TypeRef& member, Value value) {
  UnionVal uni;
  uni.member = member;
  uni.value = std::make_shared<Value>(std::move(value));
  return Value{uni};
}

Value UnitValue() {
  return Value{UnitVal{}};
}

Value IoFailureUnion() {
  EnumVal err;
  err.path = {"IoError", "IoFailure"};
  Value err_val{err};
  return UnionWrap(analysis::MakeTypePath({"IoError"}), std::move(err_val));
}

Value FileClosedValue() {
  RecordVal rec;
  rec.record_type = analysis::MakeTypeModalState({"File"}, "Closed");
  return Value{rec};
}

Value DirIterClosedValue() {
  RecordVal rec;
  rec.record_type = analysis::MakeTypeModalState({"DirIter"}, "Closed");
  return Value{rec};
}

std::optional<std::string_view> ModalStateOf(const Value& value) {
  const auto* rec = std::get_if<RecordVal>(&value.node);
  if (!rec || !rec->record_type) {
    return std::nullopt;
  }
  const auto* modal = std::get_if<analysis::TypeModalState>(&rec->record_type->node);
  if (!modal) {
    return std::nullopt;
  }
  return modal->state;
}

bool StateHas(std::string_view state, std::string_view key) {
  return state.find(key) != std::string_view::npos;
}

IntVal USizeVal(std::uint64_t value) {
  IntVal out;
  out.type = "usize";
  out.negative = false;
  out.magnitude = core::UInt128FromU64(value);
  return out;
}

Value RegionValue(std::string_view state, RegionTarget handle) {
  RecordVal rec;
  rec.record_type = analysis::MakeTypeModalState({"Region"}, std::string(state));
  rec.fields = {{"handle", Value{USizeVal(handle)}}};
  return Value{rec};
}

Value AllocErrorValue(std::uint64_t size) {
  EnumVal err;
  err.path = {"AllocationError", "OutOfMemory"};
  EnumPayloadTupleVal tuple;
  tuple.elements.push_back(Value{USizeVal(size)});
  err.payload = std::move(tuple);
  return Value{err};
}

Value AllocErrorUnion(std::uint64_t size) {
  return UnionWrap(analysis::MakeTypePath({"AllocationError"}),
                   AllocErrorValue(size));
}

std::optional<Value> BindingValueToValue(const BindingValue& value,
                                         const Sigma& sigma) {
  if (const auto* val = std::get_if<Value>(&value)) {
    return *val;
  }
  const auto alias = std::get<Alias>(value);
  return ReadAddr(sigma, alias.addr);
}

std::optional<Addr> BindingValueAddr(const BindingValue& value) {
  if (const auto* alias = std::get_if<Alias>(&value)) {
    return alias->addr;
  }
  return std::nullopt;
}

bool HeapValid(const Value& value, const Sigma& sigma) {
  const auto* dyn = std::get_if<DynamicVal>(&value.node);
  if (!dyn) {
    return false;
  }
  if (!PathEq(dyn->class_path, {"HeapAllocator"})) {
    return false;
  }
  const auto tag = AddrTag(sigma, dyn->data.addr);
  return tag.has_value() && TagActive(sigma, *tag);
}

std::optional<std::vector<std::uint8_t>> StringBytesOf(const Value& value) {
  const auto* str = std::get_if<StringVal>(&value.node);
  if (!str) {
    return std::nullopt;
  }
  return str->bytes;
}

std::optional<std::vector<std::uint8_t>> BytesBytesOf(const Value& value) {
  const auto* bytes = std::get_if<BytesVal>(&value.node);
  if (!bytes) {
    return std::nullopt;
  }
  return bytes->bytes;
}

std::optional<std::uint8_t> U8OfValue(const Value& value) {
  const auto* int_val = std::get_if<IntVal>(&value.node);
  if (!int_val || int_val->negative || !analysis::IdEq(int_val->type, "u8")) {
    return std::nullopt;
  }
  if (!core::UInt128FitsU64(int_val->magnitude)) {
    return std::nullopt;
  }
  const auto raw = core::UInt128ToU64(int_val->magnitude);
  if (raw > 0xFFu) {
    return std::nullopt;
  }
  return static_cast<std::uint8_t>(raw);
}

std::optional<std::uint64_t> USizeOfValue(const Value& value) {
  const auto* int_val = std::get_if<IntVal>(&value.node);
  if (!int_val || int_val->negative || !analysis::IdEq(int_val->type, "usize")) {
    return std::nullopt;
  }
  if (!core::UInt128FitsU64(int_val->magnitude)) {
    return std::nullopt;
  }
  return core::UInt128ToU64(int_val->magnitude);
}

std::optional<std::pair<std::size_t, std::size_t>> SliceBounds(
    const RangeVal& range,
    std::size_t len) {
  auto index_num = [](const Value& value) -> std::optional<std::size_t> {
    const auto* int_val = std::get_if<IntVal>(&value.node);
    if (!int_val || int_val->negative) {
      return std::nullopt;
    }
    if (!core::UInt128FitsU64(int_val->magnitude)) {
      return std::nullopt;
    }
    return static_cast<std::size_t>(core::UInt128ToU64(int_val->magnitude));
  };
  auto inc_index = [&](const Value& value) -> std::optional<std::size_t> {
    const auto idx = index_num(value);
    if (!idx.has_value()) {
      return std::nullopt;
    }
    return *idx + 1;
  };

  std::optional<std::size_t> start;
  std::optional<std::size_t> end;
  if (range.lo) {
    start = index_num(*range.lo);
  }
  if (range.hi) {
    end = index_num(*range.hi);
  }
  switch (range.kind) {
    case RangeKind::Exclusive:
      break;
    case RangeKind::Inclusive: {
      if (!end.has_value()) {
        return std::nullopt;
      }
      const auto inc = inc_index(*range.hi);
      if (!inc.has_value()) {
        return std::nullopt;
      }
      end = *inc;
      break;
    }
    case RangeKind::From:
      end = len;
      break;
    case RangeKind::To:
      start = 0;
      break;
    case RangeKind::ToInclusive: {
      if (!range.hi) {
        return std::nullopt;
      }
      start = 0;
      const auto inc = inc_index(*range.hi);
      if (!inc.has_value()) {
        return std::nullopt;
      }
      end = *inc;
      break;
    }
    case RangeKind::Full:
      start = 0;
      end = len;
      break;
  }
  if (!start.has_value() || !end.has_value()) {
    return std::nullopt;
  }
  if (*start > *end || *end > len) {
    return std::nullopt;
  }
  return std::make_pair(*start, *end);
}

std::optional<std::vector<std::uint8_t>> SliceBytesOf(const Value& value) {
  if (const auto* slice = std::get_if<SliceVal>(&value.node)) {
    const auto bounds = SliceBounds(slice->range, slice->base.size());
    if (!bounds.has_value()) {
      return std::nullopt;
    }
    std::vector<std::uint8_t> out;
    const auto start = bounds->first;
    const auto end = bounds->second;
    out.reserve(end - start);
    for (std::size_t i = start; i < end; ++i) {
      const auto byte = U8OfValue(slice->base[i]);
      if (!byte.has_value()) {
        return std::nullopt;
      }
      out.push_back(*byte);
    }
    return out;
  }
  if (const auto* array = std::get_if<ArrayVal>(&value.node)) {
    std::vector<std::uint8_t> out;
    out.reserve(array->elements.size());
    for (const auto& elem : array->elements) {
      const auto byte = U8OfValue(elem);
      if (!byte.has_value()) {
        return std::nullopt;
      }
      out.push_back(*byte);
    }
    return out;
  }
  return std::nullopt;
}


std::string_view IoErrorName(IoErrorKind kind) {
  switch (kind) {
    case IoErrorKind::NotFound:
      return "NotFound";
    case IoErrorKind::PermissionDenied:
      return "PermissionDenied";
    case IoErrorKind::AlreadyExists:
      return "AlreadyExists";
    case IoErrorKind::InvalidPath:
      return "InvalidPath";
    case IoErrorKind::Busy:
      return "Busy";
    case IoErrorKind::IoFailure:
      return "IoFailure";
  }
  return "IoFailure";
}

Value IoErrorUnion(IoErrorKind kind) {
  EnumVal err;
  err.path = {"IoError", std::string(IoErrorName(kind))};
  Value err_val{err};
  return UnionWrap(analysis::MakeTypePath({"IoError"}), std::move(err_val));
}

Value FileKindValue(FsEntryKind kind) {
  EnumVal out;
  switch (kind) {
    case FsEntryKind::File:
      out.path = {"FileKind", "File"};
      break;
    case FsEntryKind::Dir:
      out.path = {"FileKind", "Dir"};
      break;
    case FsEntryKind::Other:
      out.path = {"FileKind", "Other"};
      break;
  }
  return Value{out};
}

Value StringManagedValue(const std::vector<std::uint8_t>& bytes) {
  StringVal out;
  out.state = analysis::StringState::Managed;
  out.bytes = bytes;
  return Value{out};
}

Value BytesManagedValue(const std::vector<std::uint8_t>& bytes) {
  BytesVal out;
  out.state = analysis::BytesState::Managed;
  out.bytes = bytes;
  return Value{out};
}

Value StringUnionManaged(const std::vector<std::uint8_t>& bytes) {
  return UnionWrap(analysis::MakeTypeString(analysis::StringState::Managed),
                   StringManagedValue(bytes));
}

Value BytesUnionManaged(const std::vector<std::uint8_t>& bytes) {
  return UnionWrap(analysis::MakeTypeBytes(analysis::BytesState::Managed),
                   BytesManagedValue(bytes));
}

Value UnitUnionValue() {
  return UnionWrap(analysis::MakeTypePrim("()"), UnitValue());
}

Value FileUnionValue(std::string_view state, std::uint64_t handle) {
  auto base = analysis::MakeTypeModalState({"File"}, std::string(state));
  auto type = analysis::MakeTypePerm(analysis::Permission::Unique, base);
  RecordVal rec;
  rec.record_type = base;
  rec.fields = {{"handle", Value{USizeVal(handle)}}};
  return UnionWrap(type, Value{rec});
}

Value DirIterUnionValue(std::uint64_t handle) {
  auto base = analysis::MakeTypeModalState({"DirIter"}, "Open");
  auto type = analysis::MakeTypePerm(analysis::Permission::Unique, base);
  RecordVal rec;
  rec.record_type = base;
  rec.fields = {{"handle", Value{USizeVal(handle)}}};
  return UnionWrap(type, Value{rec});
}

Value DirEntryValue(const std::string& name,
                    const std::string& path,
                    FsEntryKind kind) {
  RecordVal rec;
  rec.record_type = analysis::MakeTypePath({"DirEntry"});
  rec.fields = {
      {"name", StringManagedValue(std::vector<std::uint8_t>(name.begin(), name.end()))},
      {"path", StringManagedValue(std::vector<std::uint8_t>(path.begin(), path.end()))},
      {"kind", FileKindValue(kind)},
  };
  return Value{rec};
}

Value DirEntryUnionValue(const std::string& name,
                         const std::string& path,
                         FsEntryKind kind) {
  return UnionWrap(analysis::MakeTypePath({"DirEntry"}),
                   DirEntryValue(name, path, kind));
}

std::optional<std::string> StringViewPath(const Value& value) {
  const auto* str = std::get_if<StringVal>(&value.node);
  if (!str) {
    return std::nullopt;
  }
  if (str->state != analysis::StringState::View &&
      str->state != analysis::StringState::Managed) {
    return std::nullopt;
  }
  return std::string(str->bytes.begin(), str->bytes.end());
}

std::optional<std::vector<std::uint8_t>> BytesViewData(const Value& value) {
  const auto* bytes = std::get_if<BytesVal>(&value.node);
  if (!bytes) {
    return std::nullopt;
  }
  if (bytes->state != analysis::BytesState::View &&
      bytes->state != analysis::BytesState::Managed) {
    return std::nullopt;
  }
  return bytes->bytes;
}

bool IsUnicodeScalar(std::uint32_t value) {
  if (value > 0x10FFFFu) {
    return false;
  }
  if (value >= 0xD800u && value <= 0xDFFFu) {
    return false;
  }
  return true;
}

std::optional<std::pair<std::uint32_t, std::size_t>> DecodeUtf8One(
    const unsigned char* data,
    std::size_t size,
    std::size_t offset) {
  if (offset >= size) {
    return std::nullopt;
  }
  const std::uint8_t b0 = data[offset];
  if (b0 < 0x80u) {
    return std::make_pair(static_cast<std::uint32_t>(b0), 1u);
  }
  if ((b0 & 0xE0u) == 0xC0u) {
    if (offset + 1 >= size) {
      return std::nullopt;
    }
    const std::uint8_t b1 = data[offset + 1];
    if ((b1 & 0xC0u) != 0x80u) {
      return std::nullopt;
    }
    const std::uint32_t cp =
        ((static_cast<std::uint32_t>(b0) & 0x1Fu) << 6) |
        (static_cast<std::uint32_t>(b1) & 0x3Fu);
    if (cp < 0x80u) {
      return std::nullopt;
    }
    if (!IsUnicodeScalar(cp)) {
      return std::nullopt;
    }
    return std::make_pair(cp, 2u);
  }
  if ((b0 & 0xF0u) == 0xE0u) {
    if (offset + 2 >= size) {
      return std::nullopt;
    }
    const std::uint8_t b1 = data[offset + 1];
    const std::uint8_t b2 = data[offset + 2];
    if ((b1 & 0xC0u) != 0x80u || (b2 & 0xC0u) != 0x80u) {
      return std::nullopt;
    }
    const std::uint32_t cp =
        ((static_cast<std::uint32_t>(b0) & 0x0Fu) << 12) |
        ((static_cast<std::uint32_t>(b1) & 0x3Fu) << 6) |
        (static_cast<std::uint32_t>(b2) & 0x3Fu);
    if (cp < 0x800u) {
      return std::nullopt;
    }
    if (!IsUnicodeScalar(cp)) {
      return std::nullopt;
    }
    return std::make_pair(cp, 3u);
  }
  if ((b0 & 0xF8u) == 0xF0u) {
    if (offset + 3 >= size) {
      return std::nullopt;
    }
    const std::uint8_t b1 = data[offset + 1];
    const std::uint8_t b2 = data[offset + 2];
    const std::uint8_t b3 = data[offset + 3];
    if ((b1 & 0xC0u) != 0x80u ||
        (b2 & 0xC0u) != 0x80u ||
        (b3 & 0xC0u) != 0x80u) {
      return std::nullopt;
    }
    const std::uint32_t cp =
        ((static_cast<std::uint32_t>(b0) & 0x07u) << 18) |
        ((static_cast<std::uint32_t>(b1) & 0x3Fu) << 12) |
        ((static_cast<std::uint32_t>(b2) & 0x3Fu) << 6) |
        (static_cast<std::uint32_t>(b3) & 0x3Fu);
    if (cp < 0x10000u) {
      return std::nullopt;
    }
    if (!IsUnicodeScalar(cp)) {
      return std::nullopt;
    }
    return std::make_pair(cp, 4u);
  }
  return std::nullopt;
}

bool Utf8Valid(const std::vector<std::uint8_t>& bytes) {
  const auto* data = reinterpret_cast<const unsigned char*>(bytes.data());
  std::size_t offset = 0;
  while (offset < bytes.size()) {
    const auto decoded = DecodeUtf8One(data, bytes.size(), offset);
    if (!decoded.has_value()) {
      return false;
    }
    offset += decoded->second;
  }
  return true;
}

struct ResolvedPath {
  std::string path;
  bool invalid = false;
};

std::optional<std::string> RestrictPath(std::string_view base,
                                       std::string_view path) {
  if (core::AbsPath(path)) {
    return std::nullopt;
  }
  const auto canon_base = core::Canon(core::Normalize(base));
  if (!canon_base.has_value()) {
    return std::nullopt;
  }
  const std::string joined = core::Normalize(core::Join(*canon_base, path));
  const auto canon_joined = core::Canon(joined);
  if (!canon_joined.has_value()) {
    return std::nullopt;
  }
  if (!core::Prefix(*canon_joined, *canon_base)) {
    return std::nullopt;
  }
  return *canon_joined;
}

std::optional<ResolvedPath> ResolveFsPathForOp(const Sigma& sigma,
                                               Addr fs_handle,
                                               std::string_view path) {
  const auto handle_it = sigma.fs_handles.find(fs_handle);
  if (handle_it == sigma.fs_handles.end()) {
    return std::nullopt;
  }
  if (!handle_it->second.parent.has_value()) {
    const auto canon = core::Canon(path);
    if (!canon.has_value()) {
      return ResolvedPath{"", true};
    }
    return ResolvedPath{*canon, false};
  }
  const auto restricted = RestrictPath(handle_it->second.base, path);
  if (!restricted.has_value()) {
    return ResolvedPath{"", true};
  }
  const auto next = ResolveFsPathForOp(sigma, *handle_it->second.parent,
                                       *restricted);
  if (!next.has_value()) {
    return std::nullopt;
  }
  return next;
}

std::optional<Addr> FileSystemAddr(const Value& value, const Sigma& sigma) {
  const auto* dyn = std::get_if<DynamicVal>(&value.node);
  if (!dyn) {
    return std::nullopt;
  }
  if (!PathEq(dyn->class_path, {"FileSystem"})) {
    return std::nullopt;
  }
  const auto tag = AddrTag(sigma, dyn->data.addr);
  if (!tag.has_value() || !TagActive(sigma, *tag)) {
        return std::nullopt;
  }
  return dyn->data.addr;
}

bool EntryExists(const FsState& fs, const std::string& path) {
  return fs.entries.find(path) != fs.entries.end();
}

FsEntryKind EntryKindOf(const FsState& fs, const std::string& path) {
  const auto it = fs.entries.find(path);
  if (it == fs.entries.end()) {
    return FsEntryKind::Other;
  }
  return it->second.kind;
}

std::size_t FileLenAt(const FsState& fs, const std::string& path) {
  const auto it = fs.entries.find(path);
  if (it == fs.entries.end() || it->second.kind != FsEntryKind::File) {
    return 0;
  }
  return it->second.bytes.size();
}

std::optional<IoErrorKind> FailMap(const FsState& fs,
                                   FsOp op,
                                   const std::string& path) {
  const auto it = fs.failmap.find({op, path});
  if (it == fs.failmap.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<IoErrorKind> FsPathError(FsOp op,
                                       const FsState& fs,
                                       const std::string& path) {
  const bool exists = EntryExists(fs, path);
  const FsEntryKind kind = EntryKindOf(fs, path);
  const bool requires_existing = (op == FsOp::OpenRead ||
                                  op == FsOp::OpenWrite ||
                                  op == FsOp::OpenAppend ||
                                  op == FsOp::ReadFile ||
                                  op == FsOp::ReadBytes ||
                                  op == FsOp::OpenDir ||
                                  op == FsOp::Kind ||
                                  op == FsOp::Remove);
  if (requires_existing && !exists) {
    return IoErrorKind::NotFound;
  }
  const auto fail = FailMap(fs, op, path);
  if (fail.has_value() && *fail == IoErrorKind::PermissionDenied) {
    return IoErrorKind::PermissionDenied;
  }
  if (op == FsOp::CreateWrite && exists) {
    return IoErrorKind::AlreadyExists;
  }
  if ((op == FsOp::CreateDir || op == FsOp::EnsureDir) &&
      exists && kind != FsEntryKind::Dir) {
    return IoErrorKind::AlreadyExists;
  }
  if (op == FsOp::OpenDir && exists && kind != FsEntryKind::Dir) {
    return IoErrorKind::InvalidPath;
  }
  if (fail.has_value() && *fail == IoErrorKind::Busy) {
    return IoErrorKind::Busy;
  }
  if (fail.has_value() && *fail == IoErrorKind::IoFailure) {
    return IoErrorKind::IoFailure;
  }
  return std::nullopt;
}

bool LexBytes(std::string_view a, std::string_view b) {
  return std::lexicographical_compare(
      a.begin(), a.end(), b.begin(), b.end(),
      [](unsigned char lhs, unsigned char rhs) { return lhs < rhs; });
}

std::string EntryKey(std::string_view name) {
  return core::CaseFold(core::NFC(name));
}

bool EntryOrder(const std::string& a, const std::string& b) {
  const std::string key_a = EntryKey(a);
  const std::string key_b = EntryKey(b);
  if (key_a == key_b) {
    return LexBytes(a, b);
  }
  return LexBytes(key_a, key_b);
}

std::vector<std::string> DirEntriesSorted(const FsState& fs,
                                          const std::string& path) {
  const auto it = fs.entries.find(path);
  if (it == fs.entries.end() || it->second.kind != FsEntryKind::Dir) {
    return {};
  }
  std::vector<std::string> names = it->second.names;
  names.erase(std::remove_if(names.begin(), names.end(),
                             [](const std::string& name) {
                               return name == "." || name == "..";
                             }),
              names.end());
  std::sort(names.begin(), names.end(), EntryOrder);
  return names;
}

std::optional<std::uint64_t> RecordHandleOf(const Value& value) {
  const auto* rec = std::get_if<RecordVal>(&value.node);
  if (!rec) {
    return std::nullopt;
  }
  for (const auto& [name, field_value] : rec->fields) {
    if (analysis::IdEq(name, "handle")) {
      return USizeOfValue(field_value);
    }
  }
  return std::nullopt;
}

std::optional<IoErrorKind> FileReadAllBytes(FsState& fs,
                                            std::uint64_t handle,
                                            std::vector<std::uint8_t>* out) {
  auto it = fs.handles.find(handle);
  if (it == fs.handles.end() || it->second.state == FsHandleState::Closed) {
    return IoErrorKind::IoFailure;
  }
  const auto entry_it = fs.entries.find(it->second.path);
  if (entry_it == fs.entries.end() || entry_it->second.kind != FsEntryKind::File) {
    return IoErrorKind::IoFailure;
  }
  it->second.len = entry_it->second.bytes.size();
  it->second.pos = it->second.len;
  if (out) {
    *out = entry_it->second.bytes;
  }
  return std::nullopt;
}

std::optional<IoErrorKind> FileWriteData(FsState& fs,
                                         std::uint64_t handle,
                                         const std::vector<std::uint8_t>& data) {
  auto it = fs.handles.find(handle);
  if (it == fs.handles.end() || it->second.state == FsHandleState::Closed) {
    return IoErrorKind::IoFailure;
  }
  const auto entry_it = fs.entries.find(it->second.path);
  if (entry_it == fs.entries.end() || entry_it->second.kind != FsEntryKind::File) {
    return IoErrorKind::IoFailure;
  }
  auto& handle_state = it->second;
  auto& bytes = entry_it->second.bytes;
  handle_state.len = bytes.size();
  if (handle_state.state == FsHandleState::OpenAppend) {
    handle_state.pos = handle_state.len;
  }
  if (bytes.size() < handle_state.pos) {
    bytes.resize(handle_state.pos, 0);
  }
  const std::size_t end = handle_state.pos + data.size();
  if (bytes.size() < end) {
    bytes.resize(end, 0);
  }
  std::copy(data.begin(), data.end(), bytes.begin() +
            static_cast<std::vector<std::uint8_t>::difference_type>(handle_state.pos));
  handle_state.pos = end;
  handle_state.len = std::max(handle_state.len, handle_state.pos);
  if (bytes.size() < handle_state.len) {
    bytes.resize(handle_state.len, 0);
  }
  return std::nullopt;
}

std::optional<IoErrorKind> FileFlushHandle(FsState& fs, std::uint64_t handle) {
  auto it = fs.handles.find(handle);
  if (it == fs.handles.end() || it->second.state == FsHandleState::Closed) {
    return IoErrorKind::IoFailure;
  }
  fs.flushed.insert(handle);
  return std::nullopt;
}

void FileCloseHandle(FsState& fs, std::uint64_t handle) {
  auto it = fs.handles.find(handle);
  if (it == fs.handles.end()) {
    return;
  }
  it->second.state = FsHandleState::Closed;
}

void AddEntryName(FsState& fs, const std::string& path) {
  const std::string name = core::Basename(path);
  if (name.empty() || name == "." || name == "..") {
    return;
  }
  auto comps = core::PathComps(path);
  if (comps.empty()) {
    return;
  }
  comps.pop_back();
  const std::string parent = core::JoinComp(comps);
  const auto parent_it = fs.entries.find(parent);
  if (parent_it == fs.entries.end() || parent_it->second.kind != FsEntryKind::Dir) {
    return;
  }
  auto& names = parent_it->second.names;
  if (std::find(names.begin(), names.end(), name) == names.end()) {
    names.push_back(name);
  }
}

void RemoveEntryName(FsState& fs, const std::string& path) {
  const std::string name = core::Basename(path);
  if (name.empty()) {
    return;
  }
  auto comps = core::PathComps(path);
  if (comps.empty()) {
    return;
  }
  comps.pop_back();
  const std::string parent = core::JoinComp(comps);
  const auto parent_it = fs.entries.find(parent);
  if (parent_it == fs.entries.end() || parent_it->second.kind != FsEntryKind::Dir) {
    return;
  }
  auto& names = parent_it->second.names;
  names.erase(std::remove(names.begin(), names.end(), name), names.end());
}

}  // namespace

bool IsRegionProcName(std::string_view name) {
  return name == "new_scoped" || name == "alloc" ||
         name == "reset_unchecked" || name == "freeze" ||
         name == "thaw" || name == "free_unchecked";
}

std::optional<Value> BuiltinCall(const analysis::TypePath& module_path,
                                 std::string_view name,
                                 const std::vector<BindingValue>& args,
                                 Sigma& sigma) {
  if (PathEq(module_path, {"Region"})) {
    if (!IsRegionProcName(name)) {
      return std::nullopt;
    }
    if (name == "new_scoped") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto opts_val = BindingValueToValue(args[0], sigma);
      if (!opts_val.has_value()) {
        return std::nullopt;
      }
      const RegionTarget target = FreshArena(sigma);
      if (!ArenaNew(sigma, target)) {
        return std::nullopt;
      }
      RegionEntry entry;
      entry.tag = target;
      entry.target = target;
      entry.scope = CurrentScopeId(sigma);
      entry.mark = std::nullopt;
      sigma.region_stack.push_back(entry);
      SPEC_RULE("Region-New-Scoped");
      return RegionValue("Active", target);
    }
    if (name == "alloc") {
      if (args.size() != 2) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      const auto value_val = BindingValueToValue(args[1], sigma);
      if (!self_val.has_value() || !value_val.has_value()) {
        return std::nullopt;
      }
      const auto handle = RegionTargetOf(*self_val);
      if (!handle.has_value()) {
        return std::nullopt;
      }
      const auto resolved = ResolveTarget(sigma, *handle);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      const auto alloc_val = RegionAlloc(sigma, *resolved, *value_val);
      if (!alloc_val.has_value()) {
        return std::nullopt;
      }
      SPEC_RULE("Region-Alloc-Proc");
      return *alloc_val;
    }
    if (name == "reset_unchecked") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      if (!self_val.has_value()) {
        return std::nullopt;
      }
      const auto handle = RegionTargetOf(*self_val);
      if (!handle.has_value()) {
        return std::nullopt;
      }
      if (!ArenaClear(sigma, *handle)) {
        return std::nullopt;
      }
      RetagRegionEntries(sigma, *handle);
      SPEC_RULE("Region-Reset-Proc");
      return RegionValue("Active", *handle);
    }
    if (name == "freeze") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      if (!self_val.has_value()) {
        return std::nullopt;
      }
      const auto handle = RegionTargetOf(*self_val);
      if (!handle.has_value()) {
        return std::nullopt;
      }
      SPEC_RULE("Region-Freeze-Proc");
      return RegionValue("Frozen", *handle);
    }
    if (name == "thaw") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      if (!self_val.has_value()) {
        return std::nullopt;
      }
      const auto handle = RegionTargetOf(*self_val);
      if (!handle.has_value()) {
        return std::nullopt;
      }
      SPEC_RULE("Region-Thaw-Proc");
      return RegionValue("Active", *handle);
    }
    if (name == "free_unchecked") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      if (!self_val.has_value()) {
        return std::nullopt;
      }
      const auto handle = RegionTargetOf(*self_val);
      if (!handle.has_value()) {
        return std::nullopt;
      }
      if (!ArenaRemove(sigma, *handle)) {
        return std::nullopt;
      }
      RemoveRegionEntries(sigma, *handle);
      SPEC_RULE("Region-Free-Proc");
      return RegionValue("Freed", *handle);
    }
    return std::nullopt;
  }
  if (PathEq(module_path, {"string"})) {
    if (!analysis::IsStringBuiltinName(name)) {
      return std::nullopt;
    }
    if (name == "from") {
      if (args.size() != 2) {
        return std::nullopt;
      }
      const auto source_val = BindingValueToValue(args[0], sigma);
      const auto heap_val = BindingValueToValue(args[1], sigma);
      if (!source_val.has_value() || !heap_val.has_value()) {
        return std::nullopt;
      }
      const auto* source = std::get_if<StringVal>(&source_val->node);
      if (!source || source->state != analysis::StringState::View) {
        return std::nullopt;
      }
      if (!HeapValid(*heap_val, sigma)) {
        SPEC_RULE("StringFrom-Err");
        return AllocErrorUnion(static_cast<std::uint64_t>(source->bytes.size()));
      }
      StringVal out;
      out.state = analysis::StringState::Managed;
      out.bytes = source->bytes;
      SPEC_RULE("StringFrom-Ok");
      return UnionWrap(analysis::MakeTypeString(analysis::StringState::Managed), Value{out});
    }
    if (name == "as_view") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      if (!self_val.has_value()) {
        return std::nullopt;
      }
      const auto* self = std::get_if<StringVal>(&self_val->node);
      if (!self || self->state != analysis::StringState::Managed) {
        return std::nullopt;
      }
      StringVal out;
      out.state = analysis::StringState::View;
      out.bytes = self->bytes;
      SPEC_RULE("StringAsView-Ok");
      return Value{out};
    }
    if (name == "to_managed") {
      if (args.size() != 2) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      const auto heap_val = BindingValueToValue(args[1], sigma);
      if (!self_val.has_value() || !heap_val.has_value()) {
        return std::nullopt;
      }
      const auto* self = std::get_if<StringVal>(&self_val->node);
      if (!self || self->state != analysis::StringState::View) {
        return std::nullopt;
      }
      if (!HeapValid(*heap_val, sigma)) {
        SPEC_RULE("StringToManaged-Err");
        return AllocErrorUnion(static_cast<std::uint64_t>(self->bytes.size()));
      }
      StringVal out;
      out.state = analysis::StringState::Managed;
      out.bytes = self->bytes;
      SPEC_RULE("StringToManaged-Ok");
      return UnionWrap(analysis::MakeTypeString(analysis::StringState::Managed), Value{out});
    }
    if (name == "clone_with") {
      if (args.size() != 2) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      const auto heap_val = BindingValueToValue(args[1], sigma);
      if (!self_val.has_value() || !heap_val.has_value()) {
        return std::nullopt;
      }
      const auto* self = std::get_if<StringVal>(&self_val->node);
      if (!self || self->state != analysis::StringState::Managed) {
        return std::nullopt;
      }
      if (!HeapValid(*heap_val, sigma)) {
        SPEC_RULE("StringCloneWith-Err");
        return AllocErrorUnion(static_cast<std::uint64_t>(self->bytes.size()));
      }
      StringVal out;
      out.state = analysis::StringState::Managed;
      out.bytes = self->bytes;
      SPEC_RULE("StringCloneWith-Ok");
      return UnionWrap(analysis::MakeTypeString(analysis::StringState::Managed), Value{out});
    }
    if (name == "append") {
      if (args.size() != 3) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      const auto data_val = BindingValueToValue(args[1], sigma);
      const auto heap_val = BindingValueToValue(args[2], sigma);
      if (!self_val.has_value() || !data_val.has_value() || !heap_val.has_value()) {
        return std::nullopt;
      }
      const auto* self = std::get_if<StringVal>(&self_val->node);
      const auto* data = std::get_if<StringVal>(&data_val->node);
      if (!self || self->state != analysis::StringState::Managed || !data ||
          data->state != analysis::StringState::View) {
        return std::nullopt;
      }
      if (!HeapValid(*heap_val, sigma)) {
        SPEC_RULE("StringAppend-Err");
        return AllocErrorUnion(static_cast<std::uint64_t>(self->bytes.size() + data->bytes.size()));
      }
      const auto addr = BindingValueAddr(args[0]);
      if (!addr.has_value()) {
        return std::nullopt;
      }
      StringVal updated;
      updated.state = analysis::StringState::Managed;
      updated.bytes = self->bytes;
      updated.bytes.insert(updated.bytes.end(), data->bytes.begin(), data->bytes.end());
      if (!WriteAddr(sigma, *addr, Value{updated})) {
        return std::nullopt;
      }
      SPEC_RULE("StringAppend-Ok");
      return UnionWrap(analysis::MakeTypePrim("()"), UnitValue());
    }
    if (name == "length") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      if (!self_val.has_value()) {
        return std::nullopt;
      }
      const auto* self = std::get_if<StringVal>(&self_val->node);
      if (!self || self->state != analysis::StringState::View) {
        return std::nullopt;
      }
      IntVal len = USizeVal(static_cast<std::uint64_t>(self->bytes.size()));
      SPEC_RULE("StringLength");
      return Value{len};
    }
    if (name == "is_empty") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      if (!self_val.has_value()) {
        return std::nullopt;
      }
      const auto* self = std::get_if<StringVal>(&self_val->node);
      if (!self || self->state != analysis::StringState::View) {
        return std::nullopt;
      }
      BoolVal out;
      out.value = self->bytes.empty();
      SPEC_RULE("StringIsEmpty");
      return Value{out};
    }
  }

  if (PathEq(module_path, {"bytes"})) {
    if (!analysis::IsBytesBuiltinName(name)) {
      return std::nullopt;
    }
    if (name == "with_capacity") {
      if (args.size() != 2) {
        return std::nullopt;
      }
      const auto cap_val = BindingValueToValue(args[0], sigma);
      const auto heap_val = BindingValueToValue(args[1], sigma);
      if (!cap_val.has_value() || !heap_val.has_value()) {
        return std::nullopt;
      }
      const auto* cap = std::get_if<IntVal>(&cap_val->node);
      if (!cap || cap->negative || !analysis::IdEq(cap->type, "usize") ||
          !core::UInt128FitsU64(cap->magnitude)) {
        return std::nullopt;
      }
      const auto cap_size = core::UInt128ToU64(cap->magnitude);
      if (!HeapValid(*heap_val, sigma)) {
        SPEC_RULE("BytesWithCapacity-Err");
        return AllocErrorUnion(cap_size);
      }
      BytesVal out;
      out.state = analysis::BytesState::Managed;
      out.bytes.clear();
      out.bytes.reserve(static_cast<std::size_t>(cap_size));
      SPEC_RULE("BytesWithCapacity-Ok");
      return UnionWrap(analysis::MakeTypeBytes(analysis::BytesState::Managed), Value{out});
    }
    if (name == "from_slice") {
      if (args.size() != 2) {
        return std::nullopt;
      }
      const auto data_val = BindingValueToValue(args[0], sigma);
      const auto heap_val = BindingValueToValue(args[1], sigma);
      if (!data_val.has_value() || !heap_val.has_value()) {
        return std::nullopt;
      }
      const auto bytes = SliceBytesOf(*data_val);
      if (!bytes.has_value()) {
        return std::nullopt;
      }
      if (!HeapValid(*heap_val, sigma)) {
        SPEC_RULE("BytesFromSlice-Err");
        return AllocErrorUnion(static_cast<std::uint64_t>(bytes->size()));
      }
      BytesVal out;
      out.state = analysis::BytesState::Managed;
      out.bytes = *bytes;
      SPEC_RULE("BytesFromSlice-Ok");
      return UnionWrap(analysis::MakeTypeBytes(analysis::BytesState::Managed), Value{out});
    }
    if (name == "as_view") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      if (!self_val.has_value()) {
        return std::nullopt;
      }
      const auto* self = std::get_if<BytesVal>(&self_val->node);
      if (!self || self->state != analysis::BytesState::Managed) {
        return std::nullopt;
      }
      BytesVal out;
      out.state = analysis::BytesState::View;
      out.bytes = self->bytes;
      SPEC_RULE("BytesAsView-Ok");
      return Value{out};
    }
    if (name == "to_managed") {
      if (args.size() != 2) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      const auto heap_val = BindingValueToValue(args[1], sigma);
      if (!self_val.has_value() || !heap_val.has_value()) {
        return std::nullopt;
      }
      const auto* self = std::get_if<BytesVal>(&self_val->node);
      if (!self || self->state != analysis::BytesState::View) {
        return std::nullopt;
      }
      if (!HeapValid(*heap_val, sigma)) {
        SPEC_RULE("BytesToManaged-Err");
        return AllocErrorUnion(static_cast<std::uint64_t>(self->bytes.size()));
      }
      BytesVal out;
      out.state = analysis::BytesState::Managed;
      out.bytes = self->bytes;
      SPEC_RULE("BytesToManaged-Ok");
      return UnionWrap(analysis::MakeTypeBytes(analysis::BytesState::Managed), Value{out});
    }
    if (name == "view") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto data_val = BindingValueToValue(args[0], sigma);
      if (!data_val.has_value()) {
        return std::nullopt;
      }
      const auto bytes = SliceBytesOf(*data_val);
      if (!bytes.has_value()) {
        return std::nullopt;
      }
      BytesVal out;
      out.state = analysis::BytesState::View;
      out.bytes = *bytes;
      SPEC_RULE("BytesView-Ok");
      return Value{out};
    }
    if (name == "view_string") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto data_val = BindingValueToValue(args[0], sigma);
      if (!data_val.has_value()) {
        return std::nullopt;
      }
      const auto* data = std::get_if<StringVal>(&data_val->node);
      if (!data || data->state != analysis::StringState::View) {
        return std::nullopt;
      }
      BytesVal out;
      out.state = analysis::BytesState::View;
      out.bytes = data->bytes;
      SPEC_RULE("BytesViewString-Ok");
      return Value{out};
    }
    if (name == "append") {
      if (args.size() != 3) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      const auto data_val = BindingValueToValue(args[1], sigma);
      const auto heap_val = BindingValueToValue(args[2], sigma);
      if (!self_val.has_value() || !data_val.has_value() || !heap_val.has_value()) {
        return std::nullopt;
      }
      const auto* self = std::get_if<BytesVal>(&self_val->node);
      const auto* data = std::get_if<BytesVal>(&data_val->node);
      if (!self || self->state != analysis::BytesState::Managed || !data ||
          data->state != analysis::BytesState::View) {
        return std::nullopt;
      }
      if (!HeapValid(*heap_val, sigma)) {
        SPEC_RULE("BytesAppend-Err");
        return AllocErrorUnion(static_cast<std::uint64_t>(self->bytes.size() + data->bytes.size()));
      }
      const auto addr = BindingValueAddr(args[0]);
      if (!addr.has_value()) {
        return std::nullopt;
      }
      BytesVal updated;
      updated.state = analysis::BytesState::Managed;
      updated.bytes = self->bytes;
      updated.bytes.insert(updated.bytes.end(), data->bytes.begin(), data->bytes.end());
      if (!WriteAddr(sigma, *addr, Value{updated})) {
        return std::nullopt;
      }
      SPEC_RULE("BytesAppend-Ok");
      return UnionWrap(analysis::MakeTypePrim("()"), UnitValue());
    }
    if (name == "length") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      if (!self_val.has_value()) {
        return std::nullopt;
      }
      const auto* self = std::get_if<BytesVal>(&self_val->node);
      if (!self || self->state != analysis::BytesState::View) {
        return std::nullopt;
      }
      IntVal len = USizeVal(static_cast<std::uint64_t>(self->bytes.size()));
      SPEC_RULE("BytesLength");
      return Value{len};
    }
    if (name == "is_empty") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto self_val = BindingValueToValue(args[0], sigma);
      if (!self_val.has_value()) {
        return std::nullopt;
      }
      const auto* self = std::get_if<BytesVal>(&self_val->node);
      if (!self || self->state != analysis::BytesState::View) {
        return std::nullopt;
      }
      BoolVal out;
      out.value = self->bytes.empty();
      SPEC_RULE("BytesIsEmpty");
      return Value{out};
    }
  }

  return std::nullopt;
}

std::optional<Value> PrimCall(const analysis::TypePath& owner,
                              std::string_view name,
                              const Value& self,
                              const std::vector<Value>& args,
                              Sigma& sigma) {
  if (PathEq(owner, {"FileSystem"})) {
    const auto fs_addr = FileSystemAddr(self, sigma);
    if (!fs_addr.has_value()) {
      return std::nullopt;
    }

    auto resolve_path = [&](const Value& arg) -> std::optional<ResolvedPath> {
      const auto path = StringViewPath(arg);
      if (!path.has_value()) {
        return std::nullopt;
      }
      return ResolveFsPathForOp(sigma, *fs_addr, *path);
    };

    if (name == "open_dir") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto resolved = resolve_path(args[0]);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      if (resolved->invalid) {
        SPEC_RULE("Prim-FS-OpenDir");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      if (const auto err = FsPathError(FsOp::OpenDir, sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-OpenDir");
        return IoErrorUnion(*err);
      }
      const std::uint64_t handle = sigma.fs_state.next_handle++;
      DirIterState state;
      state.fs_handle = *fs_addr;
      state.path = resolved->path;
      state.entries = DirEntriesSorted(sigma.fs_state, resolved->path);
      state.pos = 0;
      sigma.fs_state.diriters[handle] = std::move(state);
      SPEC_RULE("Prim-FS-OpenDir");
      return DirIterUnionValue(handle);
    }

    if (name == "restrict") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto base = StringViewPath(args[0]);
      if (!base.has_value()) {
        return std::nullopt;
      }
      const Addr addr = AllocAddr(sigma);
      WriteAddr(sigma, addr, Value{UnitVal{}});
      const auto tag = AddrTag(sigma, *fs_addr);
      if (tag.has_value()) {
        sigma.addr_tags[addr] = *tag;
      } else if (const auto scope_id = CurrentScopeId(sigma); scope_id != 0) {
        sigma.addr_tags[addr] = RuntimeTag{RuntimeTagKind::ScopeTag, scope_id};
      }
      sigma.fs_handles[addr] = FileSystemHandle{*fs_addr, *base};
      RawPtrVal raw;
      raw.qual = analysis::RawPtrQual::Imm;
      raw.addr = addr;
      DynamicVal dyn;
      dyn.class_path = {"FileSystem"};
      dyn.data = raw;
      dyn.concrete_type = nullptr;
      SPEC_RULE("Prim-FS-Restrict");
      return Value{dyn};
    }
    if (name == "exists") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto path = StringViewPath(args[0]);
      if (!path.has_value()) {
        return std::nullopt;
      }
      const auto resolved = ResolveFsPathForOp(sigma, *fs_addr, *path);
      BoolVal value;
      if (!resolved.has_value() || resolved->invalid) {
        value.value = false;
      } else {
        value.value = EntryExists(sigma.fs_state, resolved->path);
      }
      SPEC_RULE("Prim-FS-Exists");
      return Value{value};
    }



    if (name == "open_read") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto resolved = resolve_path(args[0]);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      if (resolved->invalid) {
        SPEC_RULE("Prim-FS-OpenRead");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      if (const auto err = FsPathError(FsOp::OpenRead, sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-OpenRead");
        return IoErrorUnion(*err);
      }
      if (EntryKindOf(sigma.fs_state, resolved->path) != FsEntryKind::File) {
        SPEC_RULE("Prim-FS-OpenRead");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      const std::uint64_t handle = sigma.fs_state.next_handle++;
      FsHandle handle_state;
      handle_state.state = FsHandleState::OpenRead;
      handle_state.pos = 0;
      handle_state.len = FileLenAt(sigma.fs_state, resolved->path);
      handle_state.path = resolved->path;
      sigma.fs_state.handles[handle] = handle_state;
      SPEC_RULE("Prim-FS-OpenRead");
      return FileUnionValue("Read", handle);
    }
    if (name == "open_write") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto resolved = resolve_path(args[0]);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      if (resolved->invalid) {
        SPEC_RULE("Prim-FS-OpenWrite");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      if (const auto err = FsPathError(FsOp::OpenWrite, sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-OpenWrite");
        return IoErrorUnion(*err);
      }
      if (EntryKindOf(sigma.fs_state, resolved->path) != FsEntryKind::File) {
        SPEC_RULE("Prim-FS-OpenWrite");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      const std::uint64_t handle = sigma.fs_state.next_handle++;
      FsHandle handle_state;
      handle_state.state = FsHandleState::OpenWrite;
      handle_state.pos = 0;
      handle_state.len = FileLenAt(sigma.fs_state, resolved->path);
      handle_state.path = resolved->path;
      sigma.fs_state.handles[handle] = handle_state;
      SPEC_RULE("Prim-FS-OpenWrite");
      return FileUnionValue("Write", handle);
    }
    if (name == "open_append") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto resolved = resolve_path(args[0]);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      if (resolved->invalid) {
        SPEC_RULE("Prim-FS-OpenAppend");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      if (const auto err = FsPathError(FsOp::OpenAppend, sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-OpenAppend");
        return IoErrorUnion(*err);
      }
      if (EntryKindOf(sigma.fs_state, resolved->path) != FsEntryKind::File) {
        SPEC_RULE("Prim-FS-OpenAppend");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      const std::uint64_t handle = sigma.fs_state.next_handle++;
      FsHandle handle_state;
      handle_state.state = FsHandleState::OpenAppend;
      handle_state.len = FileLenAt(sigma.fs_state, resolved->path);
      handle_state.pos = handle_state.len;
      handle_state.path = resolved->path;
      sigma.fs_state.handles[handle] = handle_state;
      SPEC_RULE("Prim-FS-OpenAppend");
      return FileUnionValue("Append", handle);
    }
    if (name == "create_write") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto resolved = resolve_path(args[0]);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      if (resolved->invalid) {
        SPEC_RULE("Prim-FS-CreateWrite");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      if (const auto err = FsPathError(FsOp::CreateWrite, sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-CreateWrite");
        return IoErrorUnion(*err);
      }
      FsEntry entry;
      entry.kind = FsEntryKind::File;
      entry.bytes.clear();
      sigma.fs_state.entries[resolved->path] = entry;
      AddEntryName(sigma.fs_state, resolved->path);
      const std::uint64_t handle = sigma.fs_state.next_handle++;
      FsHandle handle_state;
      handle_state.state = FsHandleState::OpenWrite;
      handle_state.pos = 0;
      handle_state.len = 0;
      handle_state.path = resolved->path;
      sigma.fs_state.handles[handle] = handle_state;
      SPEC_RULE("Prim-FS-CreateWrite");
      return FileUnionValue("Write", handle);
    }
    if (name == "read_file") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto resolved = resolve_path(args[0]);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      if (resolved->invalid) {
        SPEC_RULE("Prim-FS-ReadFile");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      if (const auto err = FsPathError(FsOp::ReadFile, sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-ReadFile");
        return IoErrorUnion(*err);
      }
      if (EntryKindOf(sigma.fs_state, resolved->path) != FsEntryKind::File) {
        SPEC_RULE("Prim-FS-ReadFile");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      const std::uint64_t handle = sigma.fs_state.next_handle++;
      FsHandle handle_state;
      handle_state.state = FsHandleState::OpenRead;
      handle_state.pos = 0;
      handle_state.len = FileLenAt(sigma.fs_state, resolved->path);
      handle_state.path = resolved->path;
      sigma.fs_state.handles[handle] = handle_state;
      std::vector<std::uint8_t> bytes;
      const auto read_err = FileReadAllBytes(sigma.fs_state, handle, &bytes);
      FileCloseHandle(sigma.fs_state, handle);
      if (read_err.has_value()) {
        SPEC_RULE("Prim-FS-ReadFile");
        return IoErrorUnion(*read_err);
      }
      if (!Utf8Valid(bytes)) {
        SPEC_RULE("Prim-FS-ReadFile");
        return IoErrorUnion(IoErrorKind::IoFailure);
      }
      SPEC_RULE("Prim-FS-ReadFile");
      return StringUnionManaged(bytes);
    }
    if (name == "read_bytes") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto resolved = resolve_path(args[0]);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      if (resolved->invalid) {
        SPEC_RULE("Prim-FS-ReadBytes");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      if (const auto err = FsPathError(FsOp::ReadBytes, sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-ReadBytes");
        return IoErrorUnion(*err);
      }
      if (EntryKindOf(sigma.fs_state, resolved->path) != FsEntryKind::File) {
        SPEC_RULE("Prim-FS-ReadBytes");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      const std::uint64_t handle = sigma.fs_state.next_handle++;
      FsHandle handle_state;
      handle_state.state = FsHandleState::OpenRead;
      handle_state.pos = 0;
      handle_state.len = FileLenAt(sigma.fs_state, resolved->path);
      handle_state.path = resolved->path;
      sigma.fs_state.handles[handle] = handle_state;
      std::vector<std::uint8_t> bytes;
      const auto read_err = FileReadAllBytes(sigma.fs_state, handle, &bytes);
      FileCloseHandle(sigma.fs_state, handle);
      if (read_err.has_value()) {
        SPEC_RULE("Prim-FS-ReadBytes");
        return IoErrorUnion(*read_err);
      }
      SPEC_RULE("Prim-FS-ReadBytes");
      return BytesUnionManaged(bytes);
    }
    if (name == "write_file") {
      if (args.size() != 2) {
        return std::nullopt;
      }
      const auto resolved = resolve_path(args[0]);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      if (resolved->invalid) {
        SPEC_RULE("Prim-FS-WriteFile");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      const auto data = BytesViewData(args[1]);
      if (!data.has_value()) {
        return std::nullopt;
      }
      if (const auto err = FsPathError(FsOp::WriteFile, sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-WriteFile");
        return IoErrorUnion(*err);
      }
      const auto kind = EntryKindOf(sigma.fs_state, resolved->path);
      if (EntryExists(sigma.fs_state, resolved->path) && kind != FsEntryKind::File) {
        SPEC_RULE("Prim-FS-WriteFile");
        return IoErrorUnion(IoErrorKind::AlreadyExists);
      }
      FsEntry entry;
      entry.kind = FsEntryKind::File;
      entry.bytes = *data;
      sigma.fs_state.entries[resolved->path] = entry;
      AddEntryName(sigma.fs_state, resolved->path);
      SPEC_RULE("Prim-FS-WriteFile");
      return UnitUnionValue();
    }
    if (name == "write_stdout") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto text = StringViewPath(args[0]);
      if (!text.has_value()) {
        return std::nullopt;
      }
      sigma.stdout_buffer.append(*text);
      SPEC_RULE("Prim-FS-WriteStdout");
      return UnitUnionValue();
    }
    if (name == "write_stderr") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto text = StringViewPath(args[0]);
      if (!text.has_value()) {
        return std::nullopt;
      }
      sigma.stderr_buffer.append(*text);
      SPEC_RULE("Prim-FS-WriteStderr");
      return UnitUnionValue();
    }
    if (name == "remove") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto resolved = resolve_path(args[0]);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      if (resolved->invalid) {
        SPEC_RULE("Prim-FS-Remove");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      if (const auto err = FsPathError(FsOp::Remove, sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-Remove");
        return IoErrorUnion(*err);
      }
      sigma.fs_state.entries.erase(resolved->path);
      RemoveEntryName(sigma.fs_state, resolved->path);
      SPEC_RULE("Prim-FS-Remove");
      return UnitUnionValue();
    }

    if (name == "create_dir") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto resolved = resolve_path(args[0]);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      if (resolved->invalid) {
        SPEC_RULE("Prim-FS-CreateDir");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      if (const auto err = FsPathError(FsOp::CreateDir, sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-CreateDir");
        return IoErrorUnion(*err);
      }
      if (EntryExists(sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-CreateDir");
        return IoErrorUnion(IoErrorKind::AlreadyExists);
      }
      FsEntry entry;
      entry.kind = FsEntryKind::Dir;
      entry.names.clear();
      sigma.fs_state.entries[resolved->path] = entry;
      AddEntryName(sigma.fs_state, resolved->path);
      SPEC_RULE("Prim-FS-CreateDir");
      return UnitUnionValue();
    }
    if (name == "ensure_dir") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto resolved = resolve_path(args[0]);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      if (resolved->invalid) {
        SPEC_RULE("Prim-FS-EnsureDir");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      if (const auto err = FsPathError(FsOp::EnsureDir, sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-EnsureDir");
        return IoErrorUnion(*err);
      }
      if (EntryExists(sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-EnsureDir");
        return UnitUnionValue();
      }
      FsEntry entry;
      entry.kind = FsEntryKind::Dir;
      entry.names.clear();
      sigma.fs_state.entries[resolved->path] = entry;
      AddEntryName(sigma.fs_state, resolved->path);
      SPEC_RULE("Prim-FS-EnsureDir");
      return UnitUnionValue();
    }
    if (name == "kind") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto resolved = resolve_path(args[0]);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      if (resolved->invalid) {
        SPEC_RULE("Prim-FS-Kind");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      if (const auto err = FsPathError(FsOp::Kind, sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-FS-Kind");
        return IoErrorUnion(*err);
      }
      const FsEntryKind kind = EntryKindOf(sigma.fs_state, resolved->path);
      SPEC_RULE("Prim-FS-Kind");
      return UnionWrap(analysis::MakeTypePath({"FileKind"}), FileKindValue(kind));
    }
    return IoErrorUnion(IoErrorKind::IoFailure);
  }
  if (PathEq(owner, {"File"})) {
    const auto handle = RecordHandleOf(self);
    if (!handle.has_value()) {
      return std::nullopt;
    }
    const auto state = ModalStateOf(self);
    const bool is_append = state.has_value() && StateHas(*state, "Append");
    const bool is_write = state.has_value() && StateHas(*state, "Write");
    if (name == "read_all") {
      std::vector<std::uint8_t> bytes;
      const auto err = FileReadAllBytes(sigma.fs_state, *handle, &bytes);
      if (err.has_value()) {
        SPEC_RULE("Prim-File-ReadAll");
        return IoErrorUnion(*err);
      }
      if (!Utf8Valid(bytes)) {
        SPEC_RULE("Prim-File-ReadAll");
        return IoErrorUnion(IoErrorKind::IoFailure);
      }
      SPEC_RULE("Prim-File-ReadAll");
      return StringUnionManaged(bytes);
    }
    if (name == "read_all_bytes") {
      std::vector<std::uint8_t> bytes;
      const auto err = FileReadAllBytes(sigma.fs_state, *handle, &bytes);
      if (err.has_value()) {
        SPEC_RULE("Prim-File-ReadAllBytes");
        return IoErrorUnion(*err);
      }
      SPEC_RULE("Prim-File-ReadAllBytes");
      return BytesUnionManaged(bytes);
    }
    if (name == "write") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      const auto data = BytesViewData(args[0]);
      if (!data.has_value()) {
        return std::nullopt;
      }
      const auto err = FileWriteData(sigma.fs_state, *handle, *data);
      if (err.has_value()) {
        if (is_append) {
          SPEC_RULE("Prim-File-Write-Append");
        } else {
          SPEC_RULE("Prim-File-Write");
        }
        return IoErrorUnion(*err);
      }
      if (is_append) {
        SPEC_RULE("Prim-File-Write-Append");
      } else {
        SPEC_RULE("Prim-File-Write");
      }
      return UnitUnionValue();
    }
    if (name == "flush") {
      const auto err = FileFlushHandle(sigma.fs_state, *handle);
      if (err.has_value()) {
        if (is_append) {
          SPEC_RULE("Prim-File-Flush-Append");
        } else {
          SPEC_RULE("Prim-File-Flush");
        }
        return IoErrorUnion(*err);
      }
      if (is_append) {
        SPEC_RULE("Prim-File-Flush-Append");
      } else {
        SPEC_RULE("Prim-File-Flush");
      }
      return UnitUnionValue();
    }
    if (name == "close") {
      FileCloseHandle(sigma.fs_state, *handle);
      if (is_append) {
        SPEC_RULE("Prim-File-Close-Append");
      } else if (is_write) {
        SPEC_RULE("Prim-File-Close-Write");
      } else {
        SPEC_RULE("Prim-File-Close-Read");
      }
      return FileClosedValue();
    }
    return IoErrorUnion(IoErrorKind::IoFailure);
  }
  if (PathEq(owner, {"DirIter"})) {
    const auto handle = RecordHandleOf(self);
    if (!handle.has_value()) {
      return std::nullopt;
    }
    if (name == "next") {
      auto it = sigma.fs_state.diriters.find(*handle);
      if (it == sigma.fs_state.diriters.end()) {
        SPEC_RULE("Prim-Dir-Next");
        return IoErrorUnion(IoErrorKind::IoFailure);
      }
      if (it->second.pos >= it->second.entries.size()) {
        SPEC_RULE("Prim-Dir-Next");
        return UnitUnionValue();
      }
      const std::string name_entry = it->second.entries[it->second.pos];
      const std::string full_path = core::Join(it->second.path, name_entry);
      const auto fs_handle = it->second.fs_handle;
      const auto resolved = ResolveFsPathForOp(sigma, fs_handle, full_path);
      if (!resolved.has_value()) {
        return std::nullopt;
      }
      if (resolved->invalid) {
        SPEC_RULE("Prim-Dir-Next");
        return IoErrorUnion(IoErrorKind::InvalidPath);
      }
      if (const auto err = FsPathError(FsOp::Kind, sigma.fs_state, resolved->path)) {
        SPEC_RULE("Prim-Dir-Next");
        return IoErrorUnion(*err);
      }
      const FsEntryKind kind = EntryKindOf(sigma.fs_state, resolved->path);
      it->second.pos += 1;
      SPEC_RULE("Prim-Dir-Next");
      return DirEntryUnionValue(name_entry, resolved->path, kind);
    }
    if (name == "close") {
      sigma.fs_state.diriters.erase(*handle);
      SPEC_RULE("Prim-Dir-Close");
      return DirIterClosedValue();
    }
    return IoErrorUnion(IoErrorKind::IoFailure);
  }
  if (PathEq(owner, {"HeapAllocator"})) {
    if (!HeapValid(self, sigma)) {
      return std::nullopt;
    }
    if (name == "with_quota") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      if (!USizeOfValue(args[0]).has_value()) {
        return std::nullopt;
      }
      return self;
    }
    if (name == "alloc_raw") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      if (!USizeOfValue(args[0]).has_value()) {
        return std::nullopt;
      }
      const Addr addr = AllocAddr(sigma);
      WriteAddr(sigma, addr, Value{UnitVal{}});
      RawPtrVal raw;
      raw.qual = analysis::RawPtrQual::Mut;
      raw.addr = addr;
      return Value{raw};
    }
    if (name == "dealloc_raw") {
      if (args.size() != 2) {
        return std::nullopt;
      }
      if (!USizeOfValue(args[1]).has_value()) {
        return std::nullopt;
      }
      const auto* raw = std::get_if<RawPtrVal>(&args[0].node);
      if (!raw) {
        return std::nullopt;
      }
      sigma.store.erase(raw->addr);
      sigma.addr_views.erase(raw->addr);
      sigma.binding_by_addr.erase(raw->addr);
      return UnitValue();
    }
    return std::nullopt;
  }
  if (PathEq(owner, {"System"})) {
    if (name == "get_env") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      return UnionWrap(analysis::MakeTypePrim("()"), UnitValue());
    }
    if (name == "exit") {
      if (args.size() != 1) {
        return std::nullopt;
      }
      return UnitValue();
    }
    return std::nullopt;
  }


  return std::nullopt;
}


}  // namespace cursive0::eval

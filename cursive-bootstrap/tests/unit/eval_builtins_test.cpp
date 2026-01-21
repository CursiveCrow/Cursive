#undef NDEBUG
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <string_view>
#include <vector>


#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/eval/builtins.h"
#include "cursive0/eval/state.h"
#include "cursive0/eval/value.h"
#include "cursive0/analysis/types/types.h"


namespace {

using namespace cursive0::eval;

Value StringViewValue(std::string_view text) {
  StringVal out;
  out.state = cursive0::analysis::StringState::View;
  out.bytes.assign(text.begin(), text.end());
  return Value{out};
}

Value BytesViewValue(const std::vector<std::uint8_t>& bytes) {
  BytesVal out;
  out.state = cursive0::analysis::BytesState::View;
  out.bytes = bytes;
  return Value{out};
}

std::optional<Value> UnwrapUnion(const Value& value) {
  const auto* uni = std::get_if<UnionVal>(&value.node);
  if (!uni || !uni->value) {
    return std::nullopt;
  }
  return *uni->value;
}

bool IsUnitUnion(const Value& value) {
  const auto inner = UnwrapUnion(value);
  return inner.has_value() && std::holds_alternative<UnitVal>(inner->node);
}

bool IsIoErrorUnion(const Value& value, std::string_view name) {
  const auto inner = UnwrapUnion(value);
  if (!inner.has_value()) {
    return false;
  }
  const auto* err = std::get_if<EnumVal>(&inner->node);
  if (!err || err->path.size() != 2) {
    return false;
  }
  return err->path[0] == "IoError" && err->path[1] == name;
}

std::optional<std::vector<std::uint8_t>> ManagedBytes(const Value& value) {
  const auto inner = UnwrapUnion(value);
  if (!inner.has_value()) {
    return std::nullopt;
  }
  const auto* bytes = std::get_if<BytesVal>(&inner->node);
  if (!bytes || bytes->state != cursive0::analysis::BytesState::Managed) {
    return std::nullopt;
  }
  return bytes->bytes;
}

std::optional<std::vector<std::uint8_t>> ManagedStringBytes(const Value& value) {
  const auto inner = UnwrapUnion(value);
  if (!inner.has_value()) {
    return std::nullopt;
  }
  const auto* str = std::get_if<StringVal>(&inner->node);
  if (!str || str->state != cursive0::analysis::StringState::Managed) {
    return std::nullopt;
  }
  return str->bytes;
}

const Value* FieldValue(const RecordVal& record, std::string_view name) {
  for (const auto& field : record.fields) {
    if (field.first == name) {
      return &field.second;
    }
  }
  return nullptr;
}

Value MakeInt(std::string type, std::uint64_t value) {
  IntVal v;
  v.type = std::move(type);
  v.negative = false;
  v.magnitude = cursive0::core::UInt128FromU64(value);
  return Value{v};
}

}  // namespace

int main() {
  using namespace cursive0::eval;
  namespace sema = cursive0::analysis;

  Sigma sigma;

  ScopeEntry scope;
  assert(PushScope(sigma, &scope));
  const auto scope_id = scope.id;


  const Addr heap_addr = AllocAddr(sigma);
  WriteAddr(sigma, heap_addr, Value{UnitVal{}});
  sigma.addr_tags[heap_addr] = RuntimeTag{RuntimeTagKind::ScopeTag, scope_id};

  RawPtrVal heap_ptr;
  heap_ptr.qual = cursive0::analysis::RawPtrQual::Imm;
  heap_ptr.addr = heap_addr;

  DynamicVal heap_dyn;
  heap_dyn.class_path = {"HeapAllocator"};
  heap_dyn.data = heap_ptr;
  heap_dyn.concrete_type = nullptr;

  Value heap_value{heap_dyn};

  IntVal count_val;
  count_val.type = "usize";
  count_val.negative = false;
  count_val.magnitude = cursive0::core::UInt128FromU64(4);
  Value count_value{count_val};

  auto alloc = PrimCall({"HeapAllocator"}, "alloc_raw", heap_value,
                        {count_value}, sigma);
  assert(alloc.has_value());
  const auto* raw = std::get_if<RawPtrVal>(&alloc->node);
  assert(raw);
  assert(raw->qual == cursive0::analysis::RawPtrQual::Mut);

  auto quota = PrimCall({"HeapAllocator"}, "with_quota", heap_value,
                        {count_value}, sigma);
  assert(quota.has_value());
  assert(std::holds_alternative<DynamicVal>(quota->node));

  auto dealloc = PrimCall({"HeapAllocator"}, "dealloc_raw", heap_value,
                          {*alloc, count_value}, sigma);
  assert(dealloc.has_value());
  assert(std::holds_alternative<UnitVal>(dealloc->node));
  assert(std::holds_alternative<UnitVal>(dealloc->node));

  RecordVal sys;
  sys.record_type = cursive0::analysis::MakeTypePath({"System"});
  Value sys_value{sys};

  StringVal key;
  key.state = cursive0::analysis::StringState::View;
  key.bytes = {0x46};
  Value key_value{key};

  auto env = PrimCall({"System"}, "get_env", sys_value, {key_value}, sigma);
  assert(env.has_value());
  const auto* env_union = std::get_if<UnionVal>(&env->node);
  assert(env_union);
  assert(env_union->value);
  assert(std::holds_alternative<UnitVal>(env_union->value->node));


  // FileSystem primitives (in-memory interpreter semantics).
  const Addr fs_addr = AllocAddr(sigma);
  WriteAddr(sigma, fs_addr, Value{UnitVal{}});
  sigma.addr_tags[fs_addr] = RuntimeTag{RuntimeTagKind::ScopeTag, scope_id};

  RawPtrVal fs_ptr;
  fs_ptr.qual = cursive0::analysis::RawPtrQual::Imm;
  fs_ptr.addr = fs_addr;

  DynamicVal fs_dyn;
  fs_dyn.class_path = {"FileSystem"};
  fs_dyn.data = fs_ptr;
  fs_dyn.concrete_type = nullptr;

  Value fs_value{fs_dyn};
  sigma.fs_handles[fs_addr] = FileSystemHandle{std::nullopt, ""};

  SPEC_COV("Prim-FS-CreateDir");
  auto mk_dir = PrimCall({"FileSystem"}, "create_dir", fs_value,
                         {StringViewValue("root")}, sigma);
  assert(mk_dir.has_value());
  assert(IsUnitUnion(*mk_dir));

  auto ensure_dir = PrimCall({"FileSystem"}, "ensure_dir", fs_value,
                             {StringViewValue("root")}, sigma);
  assert(ensure_dir.has_value());
  assert(IsUnitUnion(*ensure_dir));

  std::vector<std::uint8_t> hello = {'h', 'i'};
  auto write_file = PrimCall({"FileSystem"}, "write_file", fs_value,
                             {StringViewValue("root/hello.txt"), BytesViewValue(hello)},
                             sigma);
  assert(write_file.has_value());
  assert(IsUnitUnion(*write_file));

  SPEC_COV("Prim-FS-ReadBytes");
  auto read_bytes = PrimCall({"FileSystem"}, "read_bytes", fs_value,
                             {StringViewValue("root/hello.txt")}, sigma);
  assert(read_bytes.has_value());
  const auto bytes = ManagedBytes(*read_bytes);
  assert(bytes.has_value());
  assert(*bytes == hello);

  SPEC_COV("Prim-FS-ReadFile");
  auto read_file = PrimCall({"FileSystem"}, "read_file", fs_value,
                            {StringViewValue("root/hello.txt")}, sigma);
  assert(read_file.has_value());
  const auto str_bytes = ManagedStringBytes(*read_file);
  assert(str_bytes.has_value());
  assert(str_bytes.has_value());
  assert(*str_bytes == hello);

  std::vector<std::uint8_t> bad_utf8 = {0xFF};
  auto write_bad = PrimCall({"FileSystem"}, "write_file", fs_value,
                            {StringViewValue("root/bad.txt"), BytesViewValue(bad_utf8)},
                            sigma);
  assert(write_bad.has_value());
  assert(IsUnitUnion(*write_bad));

  auto read_bad = PrimCall({"FileSystem"}, "read_file", fs_value,
                           {StringViewValue("root/bad.txt")}, sigma);
  assert(read_bad.has_value());
  assert(IsIoErrorUnion(*read_bad, "IoFailure"));
  SPEC_COV("Prim-FS-OpenWrite");
  auto open_write_missing = PrimCall({"FileSystem"}, "open_write", fs_value,
                                     {StringViewValue("root/missing.txt")}, sigma);
  assert(open_write_missing.has_value());
  assert(IsIoErrorUnion(*open_write_missing, "NotFound"));
  
  SPEC_COV("Prim-FS-CreateWrite");
  auto create_write = PrimCall({"FileSystem"}, "create_write", fs_value,
                               {StringViewValue("root/log.bin")}, sigma);
  assert(create_write.has_value());

  const auto file_inner = UnwrapUnion(*create_write);
  assert(file_inner.has_value());
  const auto* file_rec = std::get_if<RecordVal>(&file_inner->node);
  assert(file_rec);

  std::vector<std::uint8_t> data = {1, 2, 3};
  SPEC_COV("Prim-File-Write");
  auto file_write = PrimCall({"File"}, "write", *file_inner,
                             {BytesViewValue(data)}, sigma);
  assert(file_write.has_value());
  assert(IsUnitUnion(*file_write));

  SPEC_COV("Prim-File-Flush");
  auto file_flush = PrimCall({"File"}, "flush", *file_inner, {}, sigma);
  assert(file_flush.has_value());
  assert(IsUnitUnion(*file_flush));

  SPEC_COV("Prim-File-Close-Write");
  auto file_close = PrimCall({"File"}, "close", *file_inner, {}, sigma);
  assert(file_close.has_value());

  SPEC_COV("Prim-FS-OpenRead");
  auto open_read = PrimCall({"FileSystem"}, "open_read", fs_value,
                            {StringViewValue("root/log.bin")}, sigma);
  assert(open_read.has_value());
  const auto read_inner = UnwrapUnion(*open_read);
  assert(read_inner.has_value());

  SPEC_COV("Prim-File-ReadAllBytes");
  auto read_all_bytes = PrimCall({"File"}, "read_all_bytes", *read_inner, {}, sigma);
  assert(read_all_bytes.has_value());
  const auto all_bytes = ManagedBytes(*read_all_bytes);
  assert(all_bytes.has_value());
  assert(*all_bytes == data);

  SPEC_COV("Prim-File-ReadAll");
  auto read_all = PrimCall({"File"}, "read_all", *read_inner, {}, sigma);
  assert(read_all.has_value());
  const auto all_text = ManagedStringBytes(*read_all);
  assert(all_text.has_value());
  assert(*all_text == data);

  SPEC_COV("Prim-File-Close-Read");
  auto read_close = PrimCall({"File"}, "close", *read_inner, {}, sigma);
  assert(read_close.has_value());

  SPEC_COV("Prim-FS-OpenAppend");
  auto open_append = PrimCall({"FileSystem"}, "open_append", fs_value,
                              {StringViewValue("root/log.bin")}, sigma);
  assert(open_append.has_value());
  const auto append_inner = UnwrapUnion(*open_append);
  assert(append_inner.has_value());

  std::vector<std::uint8_t> append_bytes = {4, 5};
  SPEC_COV("Prim-File-Write-Append");
  auto append_write = PrimCall({"File"}, "write", *append_inner,
                               {BytesViewValue(append_bytes)}, sigma);
  assert(append_write.has_value());
  assert(IsUnitUnion(*append_write));

  SPEC_COV("Prim-File-Flush-Append");
  auto append_flush = PrimCall({"File"}, "flush", *append_inner, {}, sigma);
  assert(append_flush.has_value());
  assert(IsUnitUnion(*append_flush));

  SPEC_COV("Prim-File-Close-Append");
  auto append_close = PrimCall({"File"}, "close", *append_inner, {}, sigma);
  assert(append_close.has_value());

  auto mk_sub = PrimCall({"FileSystem"}, "create_dir", fs_value,
                         {StringViewValue("root/sub")}, sigma);
  assert(mk_sub.has_value());
  assert(IsUnitUnion(*mk_sub));

  auto open_dir = PrimCall({"FileSystem"}, "open_dir", fs_value,
                           {StringViewValue("root")}, sigma);
  assert(open_dir.has_value());
  const auto dir_inner = UnwrapUnion(*open_dir);
  assert(dir_inner.has_value());
  const auto* dir_rec = std::get_if<RecordVal>(&dir_inner->node);
  assert(dir_rec);

  auto next1 = PrimCall({"DirIter"}, "next", *dir_inner, {}, sigma);
  assert(next1.has_value());
  const auto entry1 = UnwrapUnion(*next1);
  assert(entry1.has_value());
  const auto* entry1_rec = std::get_if<RecordVal>(&entry1->node);
  assert(entry1_rec);
  const Value* name1 = FieldValue(*entry1_rec, "name");
  assert(name1);
  const auto* name1_val = std::get_if<StringVal>(&name1->node);
  assert(name1_val && name1_val->state == cursive0::analysis::StringState::Managed);
  const std::string name1_text(name1_val->bytes.begin(), name1_val->bytes.end());
  assert(name1_text == "bad.txt");
  const Value* kind1 = FieldValue(*entry1_rec, "kind");
  assert(kind1);
  const auto* kind1_enum = std::get_if<EnumVal>(&kind1->node);
  assert(kind1_enum && kind1_enum->path.size() == 2);
  assert(kind1_enum->path[0] == "FileKind" && kind1_enum->path[1] == "File");

  auto next2 = PrimCall({"DirIter"}, "next", *dir_inner, {}, sigma);
  assert(next2.has_value());
  const auto entry2 = UnwrapUnion(*next2);
  assert(entry2.has_value());
  const auto* entry2_rec = std::get_if<RecordVal>(&entry2->node);
  assert(entry2_rec);
  const Value* name2 = FieldValue(*entry2_rec, "name");
  assert(name2);
  const auto* name2_val = std::get_if<StringVal>(&name2->node);
  assert(name2_val && name2_val->state == cursive0::analysis::StringState::Managed);
  const std::string name2_text(name2_val->bytes.begin(), name2_val->bytes.end());
  assert(name2_text == "hello.txt");
  const Value* kind2 = FieldValue(*entry2_rec, "kind");
  assert(kind2);
  const auto* kind2_enum = std::get_if<EnumVal>(&kind2->node);
  assert(kind2_enum && kind2_enum->path.size() == 2);
  assert(kind2_enum->path[0] == "FileKind" && kind2_enum->path[1] == "File");

  auto next3 = PrimCall({"DirIter"}, "next", *dir_inner, {}, sigma);
  assert(next3.has_value());
  const auto entry3 = UnwrapUnion(*next3);
  assert(entry3.has_value());
  const auto* entry3_rec = std::get_if<RecordVal>(&entry3->node);
  assert(entry3_rec);
  const Value* name3 = FieldValue(*entry3_rec, "name");
  assert(name3);
  const auto* name3_val = std::get_if<StringVal>(&name3->node);
  assert(name3_val && name3_val->state == cursive0::analysis::StringState::Managed);
  const std::string name3_text(name3_val->bytes.begin(), name3_val->bytes.end());
  assert(name3_text == "log.bin");
  const Value* kind3 = FieldValue(*entry3_rec, "kind");
  assert(kind3);
  const auto* kind3_enum = std::get_if<EnumVal>(&kind3->node);
  assert(kind3_enum && kind3_enum->path.size() == 2);
  assert(kind3_enum->path[0] == "FileKind" && kind3_enum->path[1] == "File");

  auto next4 = PrimCall({"DirIter"}, "next", *dir_inner, {}, sigma);
  assert(next4.has_value());
  const auto entry4 = UnwrapUnion(*next4);
  assert(entry4.has_value());
  const auto* entry4_rec = std::get_if<RecordVal>(&entry4->node);
  assert(entry4_rec);
  const Value* name4 = FieldValue(*entry4_rec, "name");
  assert(name4);
  const auto* name4_val = std::get_if<StringVal>(&name4->node);
  assert(name4_val && name4_val->state == cursive0::analysis::StringState::Managed);
  const std::string name4_text(name4_val->bytes.begin(), name4_val->bytes.end());
  assert(name4_text == "sub");
  const Value* kind4 = FieldValue(*entry4_rec, "kind");
  assert(kind4);
  const auto* kind4_enum = std::get_if<EnumVal>(&kind4->node);
  assert(kind4_enum && kind4_enum->path.size() == 2);
  assert(kind4_enum->path[0] == "FileKind" && kind4_enum->path[1] == "Dir");

  auto next5 = PrimCall({"DirIter"}, "next", *dir_inner, {}, sigma);
  assert(next5.has_value());
  assert(IsUnitUnion(*next5));

  auto dir_close = PrimCall({"DirIter"}, "close", *dir_inner, {}, sigma);
  assert(dir_close.has_value());

  SPEC_COV("Prim-FS-Remove");
  auto fs_remove = PrimCall({"FileSystem"}, "remove", fs_value,
                            {StringViewValue("root/bad.txt")}, sigma);
  assert(fs_remove.has_value());
  assert(IsUnitUnion(*fs_remove));

  SPEC_COV("Prim-FS-WriteStdout");
  auto fs_stdout = PrimCall({"FileSystem"}, "write_stdout", fs_value,
                            {StringViewValue("hello")}, sigma);
  assert(fs_stdout.has_value());
  assert(IsUnitUnion(*fs_stdout));

  SPEC_COV("Prim-FS-WriteStderr");
  auto fs_stderr = PrimCall({"FileSystem"}, "write_stderr", fs_value,
                            {StringViewValue("err")}, sigma);
  assert(fs_stderr.has_value());
  assert(IsUnitUnion(*fs_stderr));

  SPEC_COV("Prim-FS-Restrict");
  auto fs_restrict = PrimCall({"FileSystem"}, "restrict", fs_value,
                              {StringViewValue("root")}, sigma);
  assert(fs_restrict.has_value());

  const auto* fs_restrict_dyn = std::get_if<DynamicVal>(&fs_restrict->node);
  assert(fs_restrict_dyn);

  auto restricted_read = PrimCall({"FileSystem"}, "read_bytes", *fs_restrict,
                                  {StringViewValue("hello.txt")}, sigma);
  assert(restricted_read.has_value());
  const auto restricted_bytes = ManagedBytes(*restricted_read);
  assert(restricted_bytes.has_value());
  assert(*restricted_bytes == hello);

  auto restricted_invalid = PrimCall({"FileSystem"}, "open_read", *fs_restrict,
                                     {StringViewValue("../escape")}, sigma);
  assert(restricted_invalid.has_value());
  assert(IsIoErrorUnion(*restricted_invalid, "InvalidPath"));

  auto restricted_exists = PrimCall({"FileSystem"}, "exists", *fs_restrict,
                                    {StringViewValue("../escape")}, sigma);
  assert(restricted_exists.has_value());
  const auto* exists_val = std::get_if<BoolVal>(&restricted_exists->node);
  assert(exists_val && !exists_val->value);

  {

    SPEC_COV("Region-New-Scoped");
    SPEC_COV("Region-Alloc-Proc");
    SPEC_COV("Region-Reset-Proc");
    SPEC_COV("Region-Freeze-Proc");
    SPEC_COV("Region-Thaw-Proc");
    SPEC_COV("Region-Free-Proc");

    const cursive0::analysis::TypePath region_path = {"Region"};
    Value opts{UnitVal{}};
    std::vector<BindingValue> args;
    args.push_back(opts);
    auto region = BuiltinCall(region_path, "new_scoped", args, sigma);
    assert(region.has_value());

    Value region_val = *region;
    std::vector<BindingValue> alloc_args;
    alloc_args.push_back(region_val);
    alloc_args.push_back(MakeInt("i32", 7));
    auto alloc = BuiltinCall(region_path, "alloc", alloc_args, sigma);

    assert(alloc.has_value());

    std::vector<BindingValue> handle_args;
    handle_args.push_back(region_val);
    auto reset = BuiltinCall(region_path, "reset_unchecked", handle_args, sigma);
    assert(reset.has_value());
    auto freeze = BuiltinCall(region_path, "freeze", handle_args, sigma);
    assert(freeze.has_value());
    auto thaw = BuiltinCall(region_path, "thaw", handle_args, sigma);
    assert(thaw.has_value());
    auto free_r = BuiltinCall(region_path, "free_unchecked", handle_args, sigma);
    assert(free_r.has_value());
  }

  return 0;
}

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/scopes_lookup.h"
#include "cursive0/semantics/address.h"
#include "cursive0/semantics/value.h"

namespace cursive0 {
namespace syntax {
struct Block;
}  // namespace syntax
}  // namespace cursive0

namespace cursive0::semantics {

using ScopeId = std::uint64_t;
using BindId = std::uint64_t;
using RegionTag = std::uint64_t;
using RegionTarget = std::uint64_t;

enum class Movability {
  Mov,
  Immov,
};

enum class Responsibility {
  Resp,
  Alias,
};

struct BindState {
  enum class Kind {
    Valid,
    Moved,
    PartiallyMoved,
  };
  Kind kind = Kind::Valid;
  std::set<std::string> fields;
};

struct BindInfo {
  Movability mov = Movability::Mov;
  Responsibility resp = Responsibility::Alias;
};

struct Binding {
  ScopeId scope_id = 0;
  BindId bind_id = 0;
  std::string name;
  Addr addr = 0;
};

struct Alias {
  Addr addr = 0;
};

using BindingValue = std::variant<Value, Alias>;

struct DropBinding {
  Binding binding;
};

struct DropStatic {
  sema::PathKey path;
  std::string name;
};

struct DeferBlock {
  std::shared_ptr<syntax::Block> block;
};

using CleanupItem = std::variant<DropBinding, DropStatic, DeferBlock>;

struct ScopeEntry {
  ScopeId id = 0;
  std::vector<CleanupItem> cleanup;
  std::map<std::string, std::vector<BindId>> names;
  std::map<BindId, BindingValue> vals;
  std::map<BindId, BindState> states;
  std::map<BindId, BindInfo> infos;
  std::map<BindId, Addr> addrs;
  BindId next_bind_id = 0;
};

struct RegionEntry {
  RegionTag tag = 0;
  RegionTarget target = 0;
  ScopeId scope = 0;
  std::optional<std::size_t> mark;
};

enum class RuntimeTagKind {
  RegionTag,
  ScopeTag,
};

struct RuntimeTag {
  RuntimeTagKind kind = RuntimeTagKind::RegionTag;
  std::uint64_t id = 0;
};

enum class IoErrorKind {
  NotFound,
  PermissionDenied,
  AlreadyExists,
  InvalidPath,
  Busy,
  IoFailure,
};

enum class FsOp {
  OpenRead,
  OpenWrite,
  OpenAppend,
  CreateWrite,
  ReadFile,
  ReadBytes,
  WriteFile,
  WriteStdout,
  WriteStderr,
  Exists,
  Remove,
  OpenDir,
  CreateDir,
  EnsureDir,
  Kind,
};

enum class FsEntryKind {
  File,
  Dir,
  Other,
};

enum class FsHandleState {
  OpenRead,
  OpenWrite,
  OpenAppend,
  Closed,
};

struct FsEntry {
  FsEntryKind kind = FsEntryKind::Other;
  std::vector<std::uint8_t> bytes;
  std::vector<std::string> names;
};

struct FsHandle {
  FsHandleState state = FsHandleState::Closed;
  std::size_t pos = 0;
  std::size_t len = 0;
  std::string path;
};

struct DirIterState {
  Addr fs_handle = 0;
  std::string path;
  std::vector<std::string> entries;
  std::size_t pos = 0;
};

struct FsState {
  std::map<std::string, FsEntry> entries;
  std::map<std::uint64_t, FsHandle> handles;
  std::map<std::uint64_t, DirIterState> diriters;
  std::set<std::uint64_t> flushed;
  std::map<std::pair<FsOp, std::string>, IoErrorKind> failmap;
  std::uint64_t next_handle = 1;
};

struct FileSystemHandle {
  std::optional<Addr> parent;
  std::string base;
};

enum class AddrViewKind {
  Field,
  Tuple,
  Index,
};

struct AddrView {
  Addr base = 0;
  AddrViewKind kind = AddrViewKind::Field;
  std::string field;
  std::size_t index = 0;
};

struct SemanticsContext {
  const sema::ScopeContext* sema = nullptr;
  const sema::NameMapTable* name_maps = nullptr;
  const sema::ModuleNames* module_names = nullptr;
  sema::TypeRef ret_type = nullptr;
};

struct Sigma {
  std::map<Addr, Value> store;
  std::vector<ScopeEntry> scope_stack;
  std::vector<RegionEntry> region_stack;
  std::map<Addr, RuntimeTag> addr_tags;
  std::map<Addr, AddrView> addr_views;
  FsState fs_state;
  std::map<Addr, FileSystemHandle> fs_handles;
  std::map<Addr, Binding> binding_by_addr;
  std::map<std::pair<sema::PathKey, std::string>, Addr> static_addrs;
  std::map<sema::PathKey, Addr> poison_flags;
  Addr next_addr = 1;
  ScopeId next_scope_id = 1;
  RegionTag next_region_tag = 1;
  RegionTarget next_region_target = 1;
};

ScopeEntry ScopeEmpty(ScopeId sid);
ScopeEntry* CurrentScope(Sigma& sigma);
const ScopeEntry* CurrentScope(const Sigma& sigma);
ScopeId CurrentScopeId(const Sigma& sigma);

bool PushScope(Sigma& sigma, ScopeEntry* out_scope);
bool PopScope(Sigma& sigma, ScopeEntry* out_scope);
bool AppendCleanup(Sigma& sigma, const CleanupItem& item);

std::optional<ScopeEntry> ScopeById(const std::vector<ScopeEntry>& stack,
                                    ScopeId sid);
std::optional<std::vector<ScopeEntry>> ReplaceScopeById(
    const std::vector<ScopeEntry>& stack,
    ScopeId sid,
    const ScopeEntry& scope);

std::optional<Binding> LookupBind(const Sigma& sigma,
                                  std::string_view name);
std::optional<Value> LookupVal(const SemanticsContext& ctx,
                               const Sigma& sigma,
                               std::string_view name);
std::optional<Value> LookupValPath(const SemanticsContext& ctx,
                                   const Sigma& sigma,
                                   const sema::PathKey& path,
                                   std::string_view name);

std::optional<Value> BindingValueOf(const Sigma& sigma,
                                    const Binding& binding);
std::optional<BindState> BindStateOf(const Sigma& sigma,
                                     const Binding& binding);
std::optional<BindInfo> BindInfoOf(const Sigma& sigma,
                                   const Binding& binding);

Addr AllocAddr(Sigma& sigma);
Addr BindAddr(const Binding& binding);
std::optional<Addr> AddrOfBind(const Sigma& sigma, std::string_view name);

std::optional<Value> ReadAddr(const Sigma& sigma, Addr addr);
bool WriteAddr(Sigma& sigma, Addr addr, const Value& value);

bool UpdateVal(Sigma& sigma, const Binding& binding, const Value& value);
bool SetState(Sigma& sigma, const Binding& binding, const BindState& state);

bool BindVal(Sigma& sigma,
             std::string_view name,
             const BindingValue& value,
             const BindInfo& info,
             Binding* out_binding);

std::optional<RuntimeTag> AddrTag(const Sigma& sigma, Addr addr);
bool TagActive(const Sigma& sigma, const RuntimeTag& tag);

bool PoisonedModule(const Sigma& sigma, const sema::PathKey& path);
std::set<sema::PathKey> PoisonedModules(const Sigma& sigma);

}  // namespace cursive0::semantics

#include "cursive0/semantics/state.h"

#include <algorithm>
#include <type_traits>

#include "cursive0/core/int128.h"
#include "cursive0/sema/resolver.h"
#include "cursive0/sema/resolve_qual.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/string_bytes.h"
#include "cursive0/sema/visibility.h"
#include "cursive0/semantics/builtins.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::semantics {

namespace {

const ScopeEntry* FindScopeById(const std::vector<ScopeEntry>& stack,
                                ScopeId sid) {
  for (const auto& scope : stack) {
    if (scope.id == sid) {
      return &scope;
    }
  }
  return nullptr;
}

ScopeEntry* FindScopeById(std::vector<ScopeEntry>& stack, ScopeId sid) {
  for (auto& scope : stack) {
    if (scope.id == sid) {
      return &scope;
    }
  }
  return nullptr;
}

const syntax::ProcedureDecl* FindProcedureDecl(const sema::ScopeContext& ctx,
                                               const sema::PathKey& module,
                                               std::string_view name) {
  for (const auto& mod : ctx.sigma.mods) {
    if (sema::PathKeyOf(mod.path) != module) {
      continue;
    }
    for (const auto& item : mod.items) {
      if (const auto* proc = std::get_if<syntax::ProcedureDecl>(&item)) {
        if (proc->name == name) {
          return proc;
        }
      }
    }
  }
  return nullptr;
}

sema::ResolveTypePathResult ResolveTypePathAdapter(
    const sema::ScopeContext& ctx,
    const sema::NameMapTable& name_maps,
    const sema::ModuleNames& module_names,
    const syntax::TypePath& path) {
  sema::ResolveContext resolve_ctx;
  resolve_ctx.ctx = const_cast<sema::ScopeContext*>(&ctx);
  resolve_ctx.name_maps = &name_maps;
  resolve_ctx.module_names = &module_names;
  resolve_ctx.can_access = sema::CanAccess;
  const auto resolved = sema::ResolveTypePath(resolve_ctx, path);
  if (!resolved.ok) {
    return {false, resolved.diag_id, {}};
  }
  return {true, std::nullopt, resolved.value};
}

std::optional<Value> FieldValue(const Value& value, std::string_view name) {
  const auto* record = std::get_if<RecordVal>(&value.node);
  if (!record) {
    return std::nullopt;
  }
  for (const auto& [field_name, field_value] : record->fields) {
    if (sema::IdEq(field_name, name)) {
      return field_value;
    }
  }
  return std::nullopt;
}

std::optional<std::size_t> IndexNum(const Value& value) {
  const auto* int_val = std::get_if<IntVal>(&value.node);
  if (!int_val || int_val->negative) {
    return std::nullopt;
  }
  if (!core::UInt128FitsU64(int_val->magnitude)) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(core::UInt128ToU64(int_val->magnitude));
}

std::optional<std::size_t> IncIndex(const Value& value) {
  const auto idx = IndexNum(value);
  if (!idx.has_value()) {
    return std::nullopt;
  }
  return *idx + 1;
}

std::optional<std::pair<std::size_t, std::size_t>> SliceBounds(
    const RangeVal& range,
    std::size_t len) {
  std::optional<std::size_t> start;
  std::optional<std::size_t> end;
  if (range.lo) {
    start = IndexNum(*range.lo);
  }
  if (range.hi) {
    end = IndexNum(*range.hi);
  }
  switch (range.kind) {
    case RangeKind::Exclusive:
      break;
    case RangeKind::Inclusive: {
      if (!end.has_value()) {
        return std::nullopt;
      }
      const auto inc = IncIndex(*range.hi);
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
      const auto inc = IncIndex(*range.hi);
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

std::optional<Value> TupleValue(const Value& value, std::size_t index) {
  const auto* tuple = std::get_if<TupleVal>(&value.node);
  if (!tuple || index >= tuple->elements.size()) {
    return std::nullopt;
  }
  return tuple->elements[index];
}

std::optional<Value> IndexValue(const Value& value, std::size_t index) {
  if (const auto* array = std::get_if<ArrayVal>(&value.node)) {
    if (index >= array->elements.size()) {
      return std::nullopt;
    }
    return array->elements[index];
  }
  if (const auto* slice = std::get_if<SliceVal>(&value.node)) {
    const auto bounds = SliceBounds(slice->range, slice->base.size());
    if (!bounds.has_value()) {
      return std::nullopt;
    }
    const auto start = bounds->first;
    const auto end = bounds->second;
    if (index >= end - start) {
      return std::nullopt;
    }
    return slice->base[start + index];
  }
  return std::nullopt;
}

bool UpdateField(Value& value,
                 std::string_view name,
                 const Value& field_value) {
  auto* record = std::get_if<RecordVal>(&value.node);
  if (!record) {
    return false;
  }
  for (auto& [field_name, elem] : record->fields) {
    if (sema::IdEq(field_name, name)) {
      elem = field_value;
      return true;
    }
  }
  return false;
}

bool UpdateTuple(Value& value, std::size_t index, const Value& elem) {
  auto* tuple = std::get_if<TupleVal>(&value.node);
  if (!tuple || index >= tuple->elements.size()) {
    return false;
  }
  tuple->elements[index] = elem;
  return true;
}

bool UpdateIndex(Value& value, std::size_t index, const Value& elem) {
  if (auto* array = std::get_if<ArrayVal>(&value.node)) {
    if (index >= array->elements.size()) {
      return false;
    }
    array->elements[index] = elem;
    return true;
  }
  if (auto* slice = std::get_if<SliceVal>(&value.node)) {
    const auto bounds = SliceBounds(slice->range, slice->base.size());
    if (!bounds.has_value()) {
      return false;
    }
    const auto start = bounds->first;
    const auto end = bounds->second;
    if (index >= end - start) {
      return false;
    }
    slice->base[start + index] = elem;
    return true;
  }
  return false;
}

std::optional<Value> ReadAddrView(const Sigma& sigma, const AddrView& view) {
  const auto base = ReadAddr(sigma, view.base);
  if (!base.has_value()) {
    return std::nullopt;
  }
  switch (view.kind) {
    case AddrViewKind::Field:
      return FieldValue(*base, view.field);
    case AddrViewKind::Tuple:
      return TupleValue(*base, view.index);
    case AddrViewKind::Index:
      return IndexValue(*base, view.index);
  }
  return std::nullopt;
}

bool WriteAddrView(Sigma& sigma, const AddrView& view, const Value& value) {
  const auto base = ReadAddr(sigma, view.base);
  if (!base.has_value()) {
    return false;
  }
  Value updated = *base;
  bool ok = false;
  switch (view.kind) {
    case AddrViewKind::Field:
      ok = UpdateField(updated, view.field, value);
      break;
    case AddrViewKind::Tuple:
      ok = UpdateTuple(updated, view.index, value);
      break;
    case AddrViewKind::Index:
      ok = UpdateIndex(updated, view.index, value);
      break;
  }
  if (!ok) {
    return false;
  }
  return WriteAddr(sigma, view.base, updated);
}

bool IsNonZero(const Value& value) {
  return std::visit(
      [&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, BoolVal>) {
          return node.value;
        } else if constexpr (std::is_same_v<T, IntVal>) {
          return node.negative ||
                 core::UInt128High(node.magnitude) != 0 ||
                 core::UInt128Low(node.magnitude) != 0;
        }
        return false;
      },
      value.node);
}

}  // namespace

ScopeEntry ScopeEmpty(ScopeId sid) {
  ScopeEntry scope;
  scope.id = sid;
  return scope;
}

ScopeEntry* CurrentScope(Sigma& sigma) {
  if (sigma.scope_stack.empty()) {
    return nullptr;
  }
  return &sigma.scope_stack.back();
}

const ScopeEntry* CurrentScope(const Sigma& sigma) {
  if (sigma.scope_stack.empty()) {
    return nullptr;
  }
  return &sigma.scope_stack.back();
}

ScopeId CurrentScopeId(const Sigma& sigma) {
  const auto* scope = CurrentScope(sigma);
  return scope ? scope->id : 0;
}

bool PushScope(Sigma& sigma, ScopeEntry* out_scope) {
  const ScopeId sid = sigma.next_scope_id++;
  ScopeEntry scope = ScopeEmpty(sid);
  sigma.scope_stack.push_back(scope);
  if (out_scope) {
    *out_scope = scope;
  }
  return true;
}

bool PopScope(Sigma& sigma, ScopeEntry* out_scope) {
  if (sigma.scope_stack.empty()) {
    return false;
  }
  ScopeEntry scope = sigma.scope_stack.back();
  sigma.scope_stack.pop_back();
  for (const auto& [bind_id, addr] : scope.addrs) {
    sigma.binding_by_addr.erase(addr);
  }
  if (out_scope) {
    *out_scope = scope;
  }
  return true;
}

bool AppendCleanup(Sigma& sigma, const CleanupItem& item) {
  auto* scope = CurrentScope(sigma);
  if (!scope) {
    return false;
  }
  scope->cleanup.push_back(item);
  return true;
}

std::optional<ScopeEntry> ScopeById(const std::vector<ScopeEntry>& stack,
                                    ScopeId sid) {
  const auto* scope = FindScopeById(stack, sid);
  if (!scope) {
    return std::nullopt;
  }
  return *scope;
}

std::optional<std::vector<ScopeEntry>> ReplaceScopeById(
    const std::vector<ScopeEntry>& stack,
    ScopeId sid,
    const ScopeEntry& scope) {
  std::vector<ScopeEntry> out = stack;
  for (auto& entry : out) {
    if (entry.id == sid) {
      entry = scope;
      return out;
    }
  }
  return std::nullopt;
}

std::optional<Binding> LookupBind(const Sigma& sigma, std::string_view name) {
  for (auto it = sigma.scope_stack.rbegin();
       it != sigma.scope_stack.rend();
       ++it) {
    const auto name_it = it->names.find(std::string(name));
    if (name_it == it->names.end() || name_it->second.empty()) {
      continue;
    }
    const BindId bind_id = name_it->second.back();
    const auto addr_it = it->addrs.find(bind_id);
    if (addr_it == it->addrs.end()) {
      return std::nullopt;
    }
    Binding binding;
    binding.scope_id = it->id;
    binding.bind_id = bind_id;
    binding.name = std::string(name);
    binding.addr = addr_it->second;
    return binding;
  }
  return std::nullopt;
}

std::optional<Value> BindingValueOf(const Sigma& sigma,
                                    const Binding& binding) {
  const auto* scope = FindScopeById(sigma.scope_stack, binding.scope_id);
  if (!scope) {
    return std::nullopt;
  }
  const auto it = scope->vals.find(binding.bind_id);
  if (it == scope->vals.end()) {
    return std::nullopt;
  }
  if (std::holds_alternative<Value>(it->second)) {
    return std::get<Value>(it->second);
  }
  const auto alias = std::get<Alias>(it->second);
  return ReadAddr(sigma, alias.addr);
}

std::optional<BindState> BindStateOf(const Sigma& sigma,
                                     const Binding& binding) {
  const auto* scope = FindScopeById(sigma.scope_stack, binding.scope_id);
  if (!scope) {
    return std::nullopt;
  }
  const auto it = scope->states.find(binding.bind_id);
  if (it == scope->states.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<BindInfo> BindInfoOf(const Sigma& sigma,
                                   const Binding& binding) {
  const auto* scope = FindScopeById(sigma.scope_stack, binding.scope_id);
  if (!scope) {
    return std::nullopt;
  }
  const auto it = scope->infos.find(binding.bind_id);
  if (it == scope->infos.end()) {
    return std::nullopt;
  }
  return it->second;
}

Addr AllocAddr(Sigma& sigma) {
  const Addr addr = sigma.next_addr++;
  return addr;
}

Addr BindAddr(const Binding& binding) {
  return binding.addr;
}

std::optional<Addr> AddrOfBind(const Sigma& sigma, std::string_view name) {
  const auto binding = LookupBind(sigma, name);
  if (!binding.has_value()) {
    return std::nullopt;
  }
  const auto* scope = FindScopeById(sigma.scope_stack, binding->scope_id);
  if (!scope) {
    return std::nullopt;
  }
  const auto val_it = scope->vals.find(binding->bind_id);
  if (val_it == scope->vals.end()) {
    return std::nullopt;
  }
  if (std::holds_alternative<Alias>(val_it->second)) {
    return std::get<Alias>(val_it->second).addr;
  }
  return binding->addr;
}

std::optional<Value> ReadAddr(const Sigma& sigma, Addr addr) {
  const auto bind_it = sigma.binding_by_addr.find(addr);
  if (bind_it != sigma.binding_by_addr.end()) {
    const auto* scope = FindScopeById(sigma.scope_stack, bind_it->second.scope_id);
    if (!scope) {
      return std::nullopt;
    }
    const auto val_it = scope->vals.find(bind_it->second.bind_id);
    if (val_it == scope->vals.end()) {
      return std::nullopt;
    }
    if (std::holds_alternative<Value>(val_it->second)) {
      return std::get<Value>(val_it->second);
    }
    const auto alias = std::get<Alias>(val_it->second);
    return ReadAddr(sigma, alias.addr);
  }
  const auto it = sigma.store.find(addr);
  if (it == sigma.store.end()) {
    const auto view_it = sigma.addr_views.find(addr);
    if (view_it != sigma.addr_views.end()) {
      return ReadAddrView(sigma, view_it->second);
    }
    return std::nullopt;
  }
  return it->second;
}

bool WriteAddr(Sigma& sigma, Addr addr, const Value& value) {
  const auto view_it = sigma.addr_views.find(addr);
  if (view_it != sigma.addr_views.end()) {
    return WriteAddrView(sigma, view_it->second, value);
  }
  const auto bind_it = sigma.binding_by_addr.find(addr);
  if (bind_it != sigma.binding_by_addr.end()) {
    const auto* scope = FindScopeById(sigma.scope_stack, bind_it->second.scope_id);
    if (!scope) {
      return false;
    }
    const auto val_it = scope->vals.find(bind_it->second.bind_id);
    if (val_it == scope->vals.end()) {
      return false;
    }
    if (std::holds_alternative<Alias>(val_it->second)) {
      const auto alias = std::get<Alias>(val_it->second);
      return WriteAddr(sigma, alias.addr, value);
    }
    auto* scope_mut = FindScopeById(sigma.scope_stack, bind_it->second.scope_id);
    if (!scope_mut) {
      return false;
    }
    scope_mut->vals[bind_it->second.bind_id] = value;
    return true;
  }
  sigma.store[addr] = value;
  return true;
}

bool UpdateVal(Sigma& sigma, const Binding& binding, const Value& value) {
  auto* scope = FindScopeById(sigma.scope_stack, binding.scope_id);
  if (!scope) {
    return false;
  }
  auto it = scope->vals.find(binding.bind_id);
  if (it == scope->vals.end()) {
    return false;
  }
  if (std::holds_alternative<Alias>(it->second)) {
    const auto alias = std::get<Alias>(it->second);
    return WriteAddr(sigma, alias.addr, value);
  }
  it->second = value;
  return true;
}

bool SetState(Sigma& sigma, const Binding& binding, const BindState& state) {
  auto* scope = FindScopeById(sigma.scope_stack, binding.scope_id);
  if (!scope) {
    return false;
  }
  auto it = scope->states.find(binding.bind_id);
  if (it == scope->states.end()) {
    return false;
  }
  it->second = state;
  return true;
}

bool BindVal(Sigma& sigma,
             std::string_view name,
             const BindingValue& value,
             const BindInfo& info,
             Binding* out_binding) {
  auto* scope = CurrentScope(sigma);
  if (!scope) {
    return false;
  }
  const BindId bind_id = scope->next_bind_id++;
  const Addr addr = AllocAddr(sigma);
  scope->names[std::string(name)].push_back(bind_id);
  scope->vals[bind_id] = value;
  scope->states[bind_id] = BindState{};
  scope->infos[bind_id] = info;
  scope->addrs[bind_id] = addr;
  Binding binding;
  binding.scope_id = scope->id;
  binding.bind_id = bind_id;
  binding.name = std::string(name);
  binding.addr = addr;
  sigma.binding_by_addr[addr] = binding;
  if (info.resp == Responsibility::Resp) {
    AppendCleanup(sigma, DropBinding{binding});
  }
  if (out_binding) {
    *out_binding = binding;
  }
  return true;
}

std::optional<RuntimeTag> AddrTag(const Sigma& sigma, Addr addr) {
  const auto bind_it = sigma.binding_by_addr.find(addr);
  if (bind_it != sigma.binding_by_addr.end()) {
    return RuntimeTag{RuntimeTagKind::ScopeTag, bind_it->second.scope_id};
  }
  const auto tag_it = sigma.addr_tags.find(addr);
  if (tag_it != sigma.addr_tags.end()) {
    return tag_it->second;
  }
  return std::nullopt;
}

bool TagActive(const Sigma& sigma, const RuntimeTag& tag) {
  if (tag.kind == RuntimeTagKind::RegionTag) {
    for (const auto& entry : sigma.region_stack) {
      if (entry.tag == tag.id) {
        return true;
      }
    }
    return false;
  }
  for (const auto& scope : sigma.scope_stack) {
    if (scope.id == tag.id) {
      return true;
    }
  }
  return false;
}

bool PoisonedModule(const Sigma& sigma, const sema::PathKey& path) {
  const auto it = sigma.poison_flags.find(path);
  if (it == sigma.poison_flags.end()) {
    return false;
  }
  const auto value = ReadAddr(sigma, it->second);
  if (!value.has_value()) {
    return false;
  }
  return IsNonZero(*value);
}

std::set<sema::PathKey> PoisonedModules(const Sigma& sigma) {
  std::set<sema::PathKey> out;
  for (const auto& [path, addr] : sigma.poison_flags) {
    const auto value = ReadAddr(sigma, addr);
    if (value.has_value() && IsNonZero(*value)) {
      out.insert(path);
    }
  }
  return out;
}

std::optional<Value> LookupVal(const SemanticsContext& ctx,
                               const Sigma& sigma,
                               std::string_view name) {
  if (const auto binding = LookupBind(sigma, name)) {
    const auto* scope = FindScopeById(sigma.scope_stack, binding->scope_id);
    if (!scope) {
      return std::nullopt;
    }
    const auto it = scope->vals.find(binding->bind_id);
    if (it == scope->vals.end()) {
      return std::nullopt;
    }
    if (std::holds_alternative<Value>(it->second)) {
      SPEC_RULE("LookupVal-Bind-Value");
      return std::get<Value>(it->second);
    }
    const auto alias = std::get<Alias>(it->second);
    const auto val = ReadAddr(sigma, alias.addr);
    if (val.has_value()) {
      SPEC_RULE("LookupVal-Bind-Alias");
    }
    return val;
  }

  if (!ctx.sema || !ctx.name_maps || !ctx.module_names) {
    return std::nullopt;
  }

  const auto ent = sema::ResolveValueName(*ctx.sema, name);
  if (ent.has_value() && ent->origin_opt.has_value()) {
    const auto& module_path = *ent->origin_opt;
    const auto key = sema::PathKeyOf(module_path);
    if (PoisonedModule(sigma, key)) {
      return std::nullopt;
    }
    const std::string resolved_name =
        ent->target_opt.has_value() ? *ent->target_opt : std::string(name);
    const auto value =
        LookupValPath(ctx, sigma, key, resolved_name);
    if (value.has_value()) {
      SPEC_RULE("LookupVal-Path");
    }
    return value;
  }

  const auto type_ent = sema::ResolveTypeName(*ctx.sema, name);
  if (!type_ent.has_value() || !type_ent->origin_opt.has_value()) {
    return std::nullopt;
  }
  const auto module_key = sema::PathKeyOf(*type_ent->origin_opt);
  if (PoisonedModule(sigma, module_key)) {
    return std::nullopt;
  }
  const std::string resolved_name =
      type_ent->target_opt.has_value() ? *type_ent->target_opt
                                       : std::string(name);
  sema::PathKey full_path = module_key;
  full_path.push_back(resolved_name);
  const auto it = ctx.sema->sigma.types.find(full_path);
  if (it == ctx.sema->sigma.types.end()) {
    return std::nullopt;
  }
  if (!std::holds_alternative<syntax::RecordDecl>(it->second)) {
    return std::nullopt;
  }
  SPEC_RULE("LookupVal-RecordCtor");
  return Value{RecordCtorVal{full_path}};
}

std::optional<Value> LookupValPath(const SemanticsContext& ctx,
                                   const Sigma& sigma,
                                   const sema::PathKey& path,
                                   std::string_view name) {
  if (!ctx.sema || !ctx.name_maps || !ctx.module_names) {
    return std::nullopt;
  }
  syntax::ModulePath module_path = path;
  const auto resolved =
      sema::ResolveQualified(*ctx.sema, *ctx.name_maps, *ctx.module_names,
                             module_path, name, sema::EntityKind::Value,
                             sema::CanAccess);
  if (resolved.ok && resolved.entity.has_value() &&
      resolved.entity->origin_opt.has_value()) {
    const auto& origin = *resolved.entity->origin_opt;
    const auto key = sema::PathKeyOf(origin);
    if (PoisonedModule(sigma, key)) {
      return std::nullopt;
    }
    const std::string resolved_name =
        resolved.entity->target_opt.has_value()
            ? *resolved.entity->target_opt
            : std::string(name);
    const auto static_it =
        sigma.static_addrs.find({key, resolved_name});
    if (static_it != sigma.static_addrs.end()) {
      const auto val = ReadAddr(sigma, static_it->second);
      if (val.has_value()) {
        SPEC_RULE("LookupValPath-Static");
      }
      return val;
    }
    if (const auto* proc =
            FindProcedureDecl(*ctx.sema, key, resolved_name)) {
      (void)proc;
      SPEC_RULE("LookupValPath-Proc");
      return Value{ProcRefVal{key, resolved_name}};
    }
    return std::nullopt;
  }

  if (path.size() == 1) {
    if (sema::IdEq(path[0], "string") &&
        sema::IsStringBuiltinName(name)) {
      SPEC_RULE("LookupValPath-Proc");
      return Value{ProcRefVal{path, std::string(name)}};
    }
    if (sema::IdEq(path[0], "bytes") &&
        sema::IsBytesBuiltinName(name)) {
      SPEC_RULE("LookupValPath-Proc");
      return Value{ProcRefVal{path, std::string(name)}};
    }
    if (sema::IdEq(path[0], "Region") && IsRegionProcName(name)) {
      SPEC_RULE("LookupValPath-Proc");
      return Value{ProcRefVal{path, std::string(name)}};
    }
  }

  sema::ResolveQualContext qual_ctx;
  qual_ctx.ctx = ctx.sema;
  qual_ctx.name_maps = ctx.name_maps;
  qual_ctx.module_names = ctx.module_names;
  qual_ctx.can_access = sema::CanAccess;
  qual_ctx.resolve_type_path = ResolveTypePathAdapter;
  const auto record = sema::ResolveRecordPath(qual_ctx, module_path, name);
  if (!record.ok) {
    return std::nullopt;
  }
  const auto& record_path = record.path;
  sema::PathKey record_key = sema::PathKeyOf(record_path);
  if (!record_key.empty()) {
    sema::PathKey module_key = record_key;
    module_key.pop_back();
    if (PoisonedModule(sigma, module_key)) {
      return std::nullopt;
    }
  }
  SPEC_RULE("LookupValPath-RecordCtor");
  return Value{RecordCtorVal{record_key}};
}

}  // namespace cursive0::semantics

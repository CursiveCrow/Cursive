#include "cursive0/eval/cleanup.h"

#include <algorithm>
#include <utility>
#include "cursive0/analysis/composite/classes.h"
#include "cursive0/analysis/composite/function_types.h"
#include "cursive0/analysis/modal/modal.h"
#include "cursive0/analysis/composite/record_methods.h"
#include "cursive0/analysis/types/type_expr.h"
#include "cursive0/analysis/types/type_equiv.h"
#include "cursive0/analysis/resolve/visibility.h"
#include "cursive0/eval/apply.h"
#include "cursive0/eval/exec.h"

#include "cursive0/analysis/resolve/collect_toplevel.h"
#include "cursive0/analysis/resolve/resolver.h"
#include "cursive0/eval/eval.h"

namespace cursive0::eval {

namespace {

Outcome PanicOutcome() {
  Control ctrl;
  ctrl.kind = ControlKind::Panic;
  return MakeCtrl(ctrl);
}

bool IsPanicCtrl(const Outcome& out) {
  if (!IsCtrl(out)) {
    return false;
  }
  return std::get<Control>(out.node).kind == ControlKind::Panic;
}

std::optional<ScopeEntry*> MutableScopeById(Sigma& sigma, ScopeId sid) {
  for (auto& scope : sigma.scope_stack) {
    if (scope.id == sid) {
      return &scope;
    }
  }
  return std::nullopt;
}

bool IsMoveExpr(const syntax::Expr& expr) {
  return std::holds_alternative<syntax::MoveExpr>(expr.node);
}

bool IsPlaceExpr(const syntax::Expr& expr) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::IdentifierExpr>) {
          return true;
        } else if constexpr (std::is_same_v<T, syntax::FieldAccessExpr>) {
          return node.base ? IsPlaceExpr(*node.base) : false;
        } else if constexpr (std::is_same_v<T, syntax::TupleAccessExpr>) {
          return node.base ? IsPlaceExpr(*node.base) : false;
        } else if constexpr (std::is_same_v<T, syntax::IndexAccessExpr>) {
          return node.base ? IsPlaceExpr(*node.base) : false;
        } else if constexpr (std::is_same_v<T, syntax::DerefExpr>) {
          return node.value ? IsPlaceExpr(*node.value) : false;
        }
        return false;
      },
      expr.node);
}

Responsibility RespOfInit(const syntax::Expr& init) {
  if (!IsPlaceExpr(init)) {
    return Responsibility::Resp;
  }
  if (IsMoveExpr(init)) {
    return Responsibility::Resp;
  }
  return Responsibility::Alias;
}

Movability MovOf(const syntax::Token& op) {
  if (op.lexeme == ":=") {
    return Movability::Immov;
  }
  return Movability::Mov;
}

std::optional<BindInfo> StaticBindInfo(const SemanticsContext& ctx,
                                       const analysis::PathKey& path,
                                       std::string_view name) {
  if (!ctx.sema) {
    return std::nullopt;
  }
  for (const auto& mod : ctx.sema->sigma.mods) {
    if (analysis::PathKeyOf(mod.path) != path) {
      continue;
    }
    for (const auto& item : mod.items) {
      const auto* decl = std::get_if<syntax::StaticDecl>(&item);
      if (!decl) {
        continue;
      }
      const auto names = analysis::PatNames(*decl->binding.pat);
      bool matches = false;
      for (const auto& n : names) {
        if (n == name) {
          matches = true;
          break;
        }
      }
      if (!matches) {
        continue;
      }
      BindInfo info;
      info.mov = MovOf(decl->binding.op);
      if (decl->binding.init) {
        info.resp = RespOfInit(*decl->binding.init);
      }
      return info;
    }
  }
  return std::nullopt;
}

bool DropOnAssign(const Sigma& sigma, const Binding& binding) {
  const auto info = BindInfoOf(sigma, binding);
  if (!info.has_value()) {
    return false;
  }
  return info->mov == Movability::Immov &&
         info->resp == Responsibility::Resp;
}

bool DropOnAssignStatic(const SemanticsContext& ctx,
                        const analysis::PathKey& path,
                        std::string_view name) {
  const auto info = StaticBindInfo(ctx, path, name);
  if (!info.has_value()) {
    return false;
  }
  return info->mov == Movability::Immov &&
         info->resp == Responsibility::Resp;
}

struct RootBinding {
  enum class Kind {
    None,
    Local,
    Static,
  };
  Kind kind = Kind::None;
  Binding binding;
  analysis::PathKey path;
  std::string name;
};

RootBinding RootBindingOf(const SemanticsContext& ctx,
                          const syntax::Expr& place,
                          const Sigma& sigma) {
  RootBinding out;
  const auto root = PlaceRoot(place);
  if (!root.has_value()) {
    return out;
  }
  const auto binding = LookupBind(sigma, *root);
  if (binding.has_value()) {
    out.kind = RootBinding::Kind::Local;
    out.binding = *binding;
    return out;
  }
  if (!ctx.sema) {
    return out;
  }
  const auto ent = analysis::ResolveValueName(*ctx.sema, *root);
  if (!ent.has_value() || !ent->origin_opt.has_value()) {
    return out;
  }
  const std::string resolved =
      ent->target_opt.has_value() ? *ent->target_opt : *root;
  out.kind = RootBinding::Kind::Static;
  out.path = analysis::PathKeyOf(*ent->origin_opt);
  out.name = resolved;
  return out;
}

bool RootMoved(const SemanticsContext& ctx,
               const syntax::Expr& place,
               const Sigma& sigma) {
  const auto root = RootBindingOf(ctx, place, sigma);
  if (root.kind != RootBinding::Kind::Local) {
    return false;
  }
  const auto state = BindStateOf(sigma, root.binding);
  if (!state.has_value()) {
    return false;
  }
  return state->kind == BindState::Kind::Moved;
}

bool DropOnAssignRoot(const SemanticsContext& ctx,
                      const syntax::Expr& place,
                      const Sigma& sigma) {
  const auto root = RootBindingOf(ctx, place, sigma);
  if (root.kind == RootBinding::Kind::Local) {
    return DropOnAssign(sigma, root.binding);
  }
  if (root.kind == RootBinding::Kind::Static) {
    return DropOnAssignStatic(ctx, root.path, root.name);
  }
  return false;
}

analysis::TypeRef TypeOfValue(const Value& value) {
  return std::visit(
      [&](const auto& node) -> analysis::TypeRef {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, BoolVal>) {
          return analysis::MakeTypePrim("bool");
        } else if constexpr (std::is_same_v<T, CharVal>) {
          return analysis::MakeTypePrim("char");
        } else if constexpr (std::is_same_v<T, UnitVal>) {
          return analysis::MakeTypePrim("()");
        } else if constexpr (std::is_same_v<T, IntVal>) {
          return analysis::MakeTypePrim(node.type);
        } else if constexpr (std::is_same_v<T, FloatVal>) {
          return analysis::MakeTypePrim(node.type);
        } else if constexpr (std::is_same_v<T, PtrVal>) {
          return analysis::MakeTypePtr(nullptr, node.state);
        } else if constexpr (std::is_same_v<T, RawPtrVal>) {
          return analysis::MakeTypeRawPtr(node.qual, nullptr);
        } else if constexpr (std::is_same_v<T, TupleVal>) {
          std::vector<analysis::TypeRef> elems;
          elems.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            elems.push_back(TypeOfValue(elem));
          }
          return analysis::MakeTypeTuple(std::move(elems));
        } else if constexpr (std::is_same_v<T, ArrayVal>) {
          analysis::TypeRef elem_ty = nullptr;
          if (!node.elements.empty()) {
            elem_ty = TypeOfValue(node.elements.front());
          }
          return analysis::MakeTypeArray(elem_ty, node.elements.size());
        } else if constexpr (std::is_same_v<T, RecordVal>) {
          return node.record_type;
        } else if constexpr (std::is_same_v<T, EnumVal>) {
          return analysis::MakeTypePath(node.path);
        } else if constexpr (std::is_same_v<T, ModalVal>) {
          return nullptr;
        } else if constexpr (std::is_same_v<T, UnionVal>) {
          return node.member;
        } else if constexpr (std::is_same_v<T, DynamicVal>) {
          return analysis::MakeTypeDynamic(node.class_path);
        } else if constexpr (std::is_same_v<T, StringVal>) {
          return analysis::MakeTypeString(node.state);
        } else if constexpr (std::is_same_v<T, BytesVal>) {
          return analysis::MakeTypeBytes(node.state);
        } else if constexpr (std::is_same_v<T, ProcRefVal>) {
          return nullptr;
        } else if constexpr (std::is_same_v<T, RecordCtorVal>) {
          return nullptr;
        } else if constexpr (std::is_same_v<T, RangeVal>) {
          return analysis::MakeTypeRange();
        }
        return nullptr;
      },
      value.node);
}


std::optional<Value> FieldValueForDrop(const Value& value,
                                       std::string_view name) {
  const auto* record = std::get_if<RecordVal>(&value.node);
  if (!record) {
    return std::nullopt;
  }
  for (const auto& [field, field_value] : record->fields) {
    if (field == name) {
      return field_value;
    }
  }
  return std::nullopt;
}

syntax::Path SyntaxPathOf(const analysis::TypePath& path) {
  syntax::Path out;
  out.reserve(path.size());
  for (const auto& comp : path) {
    out.push_back(comp);
  }
  return out;
}

analysis::PathKey PathKeyFromTypePath(const analysis::TypePath& path) {
  return analysis::PathKeyOf(SyntaxPathOf(path));
}

syntax::ModulePath ModulePathFromKey(const analysis::PathKey& path) {
  syntax::ModulePath out;
  out.reserve(path.size());
  for (const auto& comp : path) {
    out.push_back(comp);
  }
  return out;
}

std::optional<analysis::Permission> LowerPermission(syntax::TypePerm perm) {
  switch (perm) {
    case syntax::TypePerm::Const:
      return analysis::Permission::Const;
    case syntax::TypePerm::Unique:
      return analysis::Permission::Unique;
    case syntax::TypePerm::Shared:
      return analysis::Permission::Shared;
  }
  return std::nullopt;
}

std::optional<analysis::RawPtrQual> LowerRawPtrQual(syntax::RawPtrQual qual) {
  switch (qual) {
    case syntax::RawPtrQual::Imm:
      return analysis::RawPtrQual::Imm;
    case syntax::RawPtrQual::Mut:
      return analysis::RawPtrQual::Mut;
  }
  return std::nullopt;
}

std::optional<analysis::PtrState> LowerPtrState(
    const std::optional<syntax::PtrState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::PtrState::Valid:
      return analysis::PtrState::Valid;
    case syntax::PtrState::Null:
      return analysis::PtrState::Null;
    case syntax::PtrState::Expired:
      return analysis::PtrState::Expired;
  }
  return std::nullopt;
}

std::optional<analysis::StringState> LowerStringState(
    const std::optional<syntax::StringState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::StringState::Managed:
      return analysis::StringState::Managed;
    case syntax::StringState::View:
      return analysis::StringState::View;
  }
  return std::nullopt;
}

std::optional<analysis::BytesState> LowerBytesState(
    const std::optional<syntax::BytesState>& state) {
  if (!state.has_value()) {
    return std::nullopt;
  }
  switch (*state) {
    case syntax::BytesState::Managed:
      return analysis::BytesState::Managed;
    case syntax::BytesState::View:
      return analysis::BytesState::View;
  }
  return std::nullopt;
}

std::optional<analysis::TypePath> ResolveTypePath(const SemanticsContext& ctx,
                                              const syntax::TypePath& path) {
  if (!ctx.sema || !ctx.name_maps || !ctx.module_names) {
    analysis::TypePath out;
    out.reserve(path.size());
    for (const auto& part : path) {
      out.push_back(part);
    }
    return out;
  }
  analysis::ResolveContext resolve_ctx;
  resolve_ctx.ctx = const_cast<analysis::ScopeContext*>(ctx.sema);
  resolve_ctx.name_maps = ctx.name_maps;
  resolve_ctx.module_names = ctx.module_names;
  resolve_ctx.can_access = analysis::CanAccess;
  const auto resolved = analysis::ResolveTypePath(resolve_ctx, path);
  if (!resolved.ok) {
    return std::nullopt;
  }
  analysis::TypePath out;
  out.reserve(resolved.value.size());
  for (const auto& part : resolved.value) {
    out.push_back(part);
  }
  return out;
}

std::optional<analysis::TypeRef> LowerType(
    const SemanticsContext& ctx,
    const std::shared_ptr<syntax::Type>& type) {
  if (!type) {
    return std::nullopt;
  }
  return std::visit(
      [&](const auto& node) -> std::optional<analysis::TypeRef> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, syntax::TypePrim>) {
          return analysis::MakeTypePrim(node.name);
        } else if constexpr (std::is_same_v<T, syntax::TypePermType>) {
          const auto perm = LowerPermission(node.perm);
          const auto base = LowerType(ctx, node.base);
          if (!perm.has_value() || !base.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypePerm(*perm, *base);
        } else if constexpr (std::is_same_v<T, syntax::TypeUnion>) {
          std::vector<analysis::TypeRef> members;
          members.reserve(node.types.size());
          for (const auto& member : node.types) {
            const auto lowered = LowerType(ctx, member);
            if (!lowered.has_value()) {
              return std::nullopt;
            }
            members.push_back(*lowered);
          }
          return analysis::MakeTypeUnion(std::move(members));
        } else if constexpr (std::is_same_v<T, syntax::TypeFunc>) {
          std::vector<analysis::TypeFuncParam> params;
          params.reserve(node.params.size());
          for (const auto& param : node.params) {
            const auto lowered = LowerType(ctx, param.type);
            if (!lowered.has_value()) {
              return std::nullopt;
            }
            analysis::TypeFuncParam out_param;
            if (param.mode.has_value()) {
              out_param.mode = analysis::ParamMode::Move;
            }
            out_param.type = *lowered;
            params.push_back(std::move(out_param));
          }
          const auto ret = LowerType(ctx, node.ret);
          if (!ret.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypeFunc(std::move(params), *ret);
        } else if constexpr (std::is_same_v<T, syntax::TypeTuple>) {
          std::vector<analysis::TypeRef> elements;
          elements.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            const auto lowered = LowerType(ctx, elem);
            if (!lowered.has_value()) {
              return std::nullopt;
            }
            elements.push_back(*lowered);
          }
          return analysis::MakeTypeTuple(std::move(elements));
        } else if constexpr (std::is_same_v<T, syntax::TypeArray>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.has_value() || !ctx.sema) {
            return std::nullopt;
          }
          const auto len = analysis::ConstLen(*ctx.sema, node.length);
          if (!len.ok || !len.value.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypeArray(*elem, *len.value);
        } else if constexpr (std::is_same_v<T, syntax::TypeSlice>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypeSlice(*elem);
        } else if constexpr (std::is_same_v<T, syntax::TypePtr>) {
          const auto elem = LowerType(ctx, node.element);
          if (!elem.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypePtr(*elem, LowerPtrState(node.state));
        } else if constexpr (std::is_same_v<T, syntax::TypeRawPtr>) {
          const auto elem = LowerType(ctx, node.element);
          const auto qual = LowerRawPtrQual(node.qual);
          if (!elem.has_value() || !qual.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypeRawPtr(*qual, *elem);
        } else if constexpr (std::is_same_v<T, syntax::TypeString>) {
          return analysis::MakeTypeString(LowerStringState(node.state));
        } else if constexpr (std::is_same_v<T, syntax::TypeBytes>) {
          return analysis::MakeTypeBytes(LowerBytesState(node.state));
        } else if constexpr (std::is_same_v<T, syntax::TypeDynamic>) {
          analysis::TypePath path(node.path.begin(), node.path.end());
          return analysis::MakeTypeDynamic(std::move(path));
        } else if constexpr (std::is_same_v<T, syntax::TypeModalState>) {
          const auto path = ResolveTypePath(ctx, node.path);
          if (!path.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypeModalState(*path, node.state);
        } else if constexpr (std::is_same_v<T, syntax::TypePathType>) {
          const auto path = ResolveTypePath(ctx, node.path);
          if (!path.has_value()) {
            return std::nullopt;
          }
          return analysis::MakeTypePath(*path);
        } else {
          return std::nullopt;
        }
      },
      type->node);
}

void ReleaseValue(const analysis::TypeRef& /*type*/,
                  const Value& /*value*/,
                  Sigma& /*sigma*/) {}

struct DropChild {
  analysis::TypeRef type;
  Value value;
};


std::vector<DropChild> DropChildren(const SemanticsContext& ctx,
                                    const analysis::TypeRef& type,
                                    const Value& value,
                                    const std::set<std::string>& fields) {
  std::vector<DropChild> out;
  if (!type) {
    return out;
  }

  if (const auto* path = std::get_if<analysis::TypePathType>(&type->node)) {
    if (!ctx.sema) {
      return out;
    }
    const auto key = analysis::PathKeyOf(path->path);
    const auto it = ctx.sema->sigma.types.find(key);
    if (it != ctx.sema->sigma.types.end()) {
      if (const auto* record = std::get_if<syntax::RecordDecl>(&it->second)) {
        for (auto it_member = record->members.rbegin();
             it_member != record->members.rend();
             ++it_member) {
          const auto* field = std::get_if<syntax::FieldDecl>(&*it_member);
          if (!field) {
            continue;
          }
          if (fields.find(field->name) != fields.end()) {
            continue;
          }
          const auto field_value = FieldValueForDrop(value, field->name);
          if (!field_value.has_value()) {
            continue;
          }
          DropChild child;
          child.value = *field_value;
          const auto field_type = LowerType(ctx, field->type);
          child.type = field_type.value_or(TypeOfValue(child.value));
          out.push_back(std::move(child));
        }
        return out;
      }
      if (const auto* modal = std::get_if<syntax::ModalDecl>(&it->second)) {
        (void)modal;
        const auto* modal_val = std::get_if<ModalVal>(&value.node);
        if (!modal_val || !modal_val->payload) {
          return out;
        }
        DropChild child;
        child.value = *modal_val->payload;
        child.type = analysis::MakeTypeModalState(path->path, modal_val->state);
        out.push_back(std::move(child));
        return out;
      }
    }
  }

  if (const auto* perm = std::get_if<analysis::TypePerm>(&type->node)) {
    return DropChildren(ctx, perm->base, value, fields);
  }

  if (const auto* tuple = std::get_if<analysis::TypeTuple>(&type->node)) {
    const auto* tuple_val = std::get_if<TupleVal>(&value.node);
    if (!tuple_val) {
      return out;
    }
    const std::size_t count = std::min(tuple->elements.size(),
                                       tuple_val->elements.size());
    for (std::size_t i = count; i-- > 0;) {
      DropChild child;
      child.value = tuple_val->elements[i];
      child.type = tuple->elements[i];
      out.push_back(std::move(child));
    }
    return out;
  }

  if (const auto* array = std::get_if<analysis::TypeArray>(&type->node)) {
    const auto* array_val = std::get_if<ArrayVal>(&value.node);
    if (!array_val) {
      return out;
    }
    const std::size_t count = std::min<std::size_t>(
        array->length, array_val->elements.size());
    for (std::size_t i = count; i-- > 0;) {
      DropChild child;
      child.value = array_val->elements[i];
      child.type = array->element;
      out.push_back(std::move(child));
    }
    return out;
  }

  if (const auto* uni = std::get_if<analysis::TypeUnion>(&type->node)) {
    (void)uni;
    if (const auto* uni_val = std::get_if<UnionVal>(&value.node)) {
      DropChild child;
      child.value = *uni_val->value;
      child.type = uni_val->member;
      out.push_back(std::move(child));
    }
    return out;
  }

  if (const auto* modal_state = std::get_if<analysis::TypeModalState>(&type->node)) {
    if (!ctx.sema) {
      return out;
    }
    const auto* decl = analysis::LookupModalDecl(*ctx.sema, modal_state->path);
    if (!decl) {
      return out;
    }
    const auto* state = analysis::LookupModalState(*decl, modal_state->state);
    if (!state) {
      return out;
    }
    const Value* record_value = &value;
    Value payload_value;
    if (!std::holds_alternative<RecordVal>(value.node)) {
      const auto* modal_val = std::get_if<ModalVal>(&value.node);
      if (!modal_val || !modal_val->payload) {
        return out;
      }
      payload_value = *modal_val->payload;
      record_value = &payload_value;
    }
    const auto* record = std::get_if<RecordVal>(&record_value->node);
    if (!record) {
      return out;
    }
    for (const auto& member : state->members) {
      const auto* field = std::get_if<syntax::StateFieldDecl>(&member);
      if (!field) {
        continue;
      }
      const auto field_value = FieldValueForDrop(*record_value, field->name);
      if (!field_value.has_value()) {
        continue;
      }
      DropChild child;
      child.value = *field_value;
      const auto field_type = LowerType(ctx, field->type);
      child.type = field_type.value_or(TypeOfValue(child.value));
      out.push_back(std::move(child));
    }
    return out;
  }

  return out;
}

DropStatus DropList(const SemanticsContext& ctx,
                    const std::vector<DropChild>& items,
                    Sigma& sigma) {
  if (items.empty()) {
    return DropStatus::Ok;
  }
  for (const auto& item : items) {
    const auto status =
        DropValueOut(ctx, item.type, item.value, {}, sigma);
    if (status == DropStatus::Panic) {
      return DropStatus::Panic;
    }
  }
  return DropStatus::Ok;
}

bool RecordType(const SemanticsContext& ctx, const analysis::TypeRef& type) {
  if (!type || !ctx.sema) {
    return false;
  }
  const auto* path = std::get_if<analysis::TypePathType>(&type->node);
  if (!path) {
    return false;
  }
  const auto key = analysis::PathKeyOf(path->path);
  const auto it = ctx.sema->sigma.types.find(key);
  if (it == ctx.sema->sigma.types.end()) {
    return false;
  }
  return std::holds_alternative<syntax::RecordDecl>(it->second);
}


bool NonRecordFOk(const SemanticsContext& ctx,
                  const analysis::TypeRef& type,
                  const std::set<std::string>& fields) {
  if (fields.empty()) {
    return true;
  }
  return RecordType(ctx, type);
}


Outcome DropCall(const SemanticsContext& ctx,
                 const analysis::TypeRef& type,
                 const Value& value,
                 Sigma& sigma) {
  if (!type) {
    return MakeVal(Value{UnitVal{}});
  }
  if (!ctx.sema) {
    return MakeVal(Value{UnitVal{}});
  }
  const auto stripped = analysis::StripPerm(type);
  if (!stripped) {
    return MakeVal(Value{UnitVal{}});
  }
  const syntax::ClassPath drop_path = {"Drop"};
  if (!analysis::TypeImplementsClass(*ctx.sema, stripped, drop_path)) {
    return MakeVal(Value{UnitVal{}});
  }
  if (const auto* str = std::get_if<analysis::TypeString>(&stripped->node)) {
    if (str->state.has_value() && *str->state == analysis::StringState::Managed) {
      return MakeVal(Value{UnitVal{}});
    }
  }
  if (const auto* bytes = std::get_if<analysis::TypeBytes>(&stripped->node)) {
    if (bytes->state.has_value() && *bytes->state == analysis::BytesState::Managed) {
      return MakeVal(Value{UnitVal{}});
    }
  }

  const auto lookup = analysis::LookupMethodStatic(*ctx.sema, stripped, "drop");
  if (!lookup.ok) {
    return PanicOutcome();
  }
  MethodTarget target;
  if (lookup.record_method) {
    target.kind = MethodTarget::Kind::Record;
    target.record_method = lookup.record_method;
  } else if (lookup.class_method) {
    target.kind = MethodTarget::Kind::Class;
    target.class_method = lookup.class_method;
    target.owner_class = lookup.owner_class;
  } else {
    return PanicOutcome();
  }

  syntax::Expr base;
  base.node = syntax::IdentifierExpr{"self"};
  base.span = {};
  BindingValue self_arg = value;
  std::vector<BindingValue> args;
  return ApplyMethodSigma(ctx, base, "drop", value, self_arg,
                          args, target, sigma);
}

}  // namespace

DropStatus DropActionOut(const SemanticsContext& ctx,
                         const Binding& binding,
                         Sigma& sigma) {
  const auto state = BindStateOf(sigma, binding);
  if (!state.has_value()) {
    return DropStatus::Panic;
  }
  if (state->kind == BindState::Kind::Moved) {
    SPEC_RULE("DropAction-Moved");
    return DropStatus::Ok;
  }
  const auto value = BindingValueOf(sigma, binding);
  if (!value.has_value()) {
    return DropStatus::Panic;
  }
  const analysis::TypeRef type = TypeOfValue(*value);
  if (state->kind == BindState::Kind::PartiallyMoved) {
    SPEC_RULE("DropAction-Partial");
    return DropValueOut(ctx, type, *value, state->fields, sigma);
  }
  SPEC_RULE("DropAction-Valid");
  return DropValueOut(ctx, type, *value, {}, sigma);
}


DropStatus DropStaticActionOut(const SemanticsContext& ctx,
                               const analysis::PathKey& path,
                               std::string_view name,
                               Sigma& sigma) {
  const auto it = sigma.static_addrs.find({path, std::string(name)});
  if (it == sigma.static_addrs.end()) {
    return DropStatus::Panic;
  }
  const auto value = ReadAddr(sigma, it->second);
  if (!value.has_value()) {
    return DropStatus::Panic;
  }
  analysis::TypeRef type;
  if (ctx.sema) {
    const auto module_path = ModulePathFromKey(path);
    const auto static_type = analysis::ValuePathType(*ctx.sema, module_path, name);
    if (!static_type.ok || !static_type.type) {
      return DropStatus::Panic;
    }
    type = static_type.type;
  } else {
    type = TypeOfValue(*value);
  }
  SPEC_RULE("DropStaticAction");
  return DropValueOut(ctx, type, *value, {}, sigma);
}


DropStatus DropValueOut(const SemanticsContext& ctx,
                        const analysis::TypeRef& type,
                        const Value& value,
                        const std::set<std::string>& fields,
                        Sigma& sigma) {
  if (!NonRecordFOk(ctx, type, fields)) {
    return DropStatus::Panic;
  }
  const Outcome drop_out = DropCall(ctx, type, value, sigma);
  if (IsPanicCtrl(drop_out)) {
    SPEC_RULE("DropValueOut-DropPanic");
    return DropStatus::Panic;
  }
  const auto children = DropChildren(ctx, type, value, fields);
  const auto child_status = DropList(ctx, children, sigma);
  if (child_status == DropStatus::Panic) {
    SPEC_RULE("DropValueOut-ChildPanic");
    return DropStatus::Panic;
  }
  ReleaseValue(type, value, sigma);
  SPEC_RULE("DropValueOut-Ok");
  return DropStatus::Ok;
}

DropStatus DropSubvalue(const SemanticsContext& ctx,
                        const syntax::Expr& place,
                        const analysis::TypeRef& type,
                        const Value& value,
                        Sigma& sigma) {
  if (!DropOnAssignRoot(ctx, place, sigma) || RootMoved(ctx, place, sigma)) {
    SPEC_RULE("DropSubvalue-Skip");
    return DropStatus::Ok;
  }
  const analysis::TypeRef use_type = type ? type : TypeOfValue(value);
  const auto status = DropValueOut(ctx, use_type, value, {}, sigma);
  if (status == DropStatus::Ok) {
    SPEC_RULE("DropSubvalue-Do");
  }
  return status;
}


[[maybe_unused]] void DestroyBindingsRec(const SemanticsContext& ctx,
                                         const std::vector<Binding>& bindings,
                                         std::size_t index,
                                         Sigma& sigma) {
  if (index >= bindings.size()) {
    SPEC_RULE("Destroy-Empty");
    return;
  }
  DropActionOut(ctx, bindings[index], sigma);
  SPEC_RULE("Destroy-Cons");
  DestroyBindingsRec(ctx, bindings, index + 1, sigma);
}

[[maybe_unused]] void DestroyBindings(const SemanticsContext& ctx,
                                      const std::vector<Binding>& bindings,
                                      Sigma& sigma) {
  DestroyBindingsRec(ctx, bindings, 0, sigma);
}


CleanupStatus Cleanup(const SemanticsContext& ctx,
                      const std::vector<CleanupItem>& items,
                      Sigma& sigma) {
  if (items.empty()) {
    SPEC_RULE("Cleanup-Empty");
    return CleanupStatus::Ok;
  }
  CleanupStatus status = CleanupStatus::Ok;
  for (const auto& item : items) {
    bool item_panic = false;
    if (const auto* drop = std::get_if<DropBinding>(&item)) {
      item_panic = DropActionOut(ctx, drop->binding, sigma) ==
                   DropStatus::Panic;
      if (item_panic) {
        SPEC_RULE("Cleanup-Cons-Drop-Panic");
      } else {
        SPEC_RULE("Cleanup-Cons-Drop");
      }
    } else if (const auto* drop_static = std::get_if<DropStatic>(&item)) {
      item_panic = DropStaticActionOut(ctx,
                                       drop_static->path,
                                       drop_static->name,
                                       sigma) == DropStatus::Panic;
      if (item_panic) {
        SPEC_RULE("Cleanup-Cons-DropStatic-Panic");
      } else {
        SPEC_RULE("Cleanup-Cons-DropStatic");
      }
    } else if (const auto* defer = std::get_if<DeferBlock>(&item)) {
      SemanticsContext inner_ctx = ctx;
      inner_ctx.temp_ctx = nullptr;
      Outcome out = EvalBlockSigma(inner_ctx, *defer->block, sigma);
      item_panic = IsCtrl(out) ? IsPanicCtrl(out) : false;
      if (item_panic) {
        SPEC_RULE("Cleanup-Cons-Defer-Panic");
      } else {
        SPEC_RULE("Cleanup-Cons-Defer-Ok");
      }
    }
    if (item_panic) {
      status = CleanupStatus::Panic;
    }
  }
  return status;
}

CleanupStatus CleanupScope(const SemanticsContext& ctx,
                           const ScopeEntry& scope,
                           Sigma& sigma) {
  if (!MutableScopeById(sigma, scope.id).has_value()) {
    return CleanupStatus::Abort;
  }
  SPEC_RULE("Cleanup-Start");
  CleanupStatus status = CleanupStatus::Ok;
  while (true) {
    auto scope_opt = MutableScopeById(sigma, scope.id);
    if (!scope_opt.has_value()) {
      break;
    }
    auto* target_scope = *scope_opt;
    if (target_scope->cleanup.empty()) {
      break;
    }
    const CleanupItem item = target_scope->cleanup.back();
    target_scope->cleanup.pop_back();

    bool item_panic = false;
    if (const auto* drop = std::get_if<DropBinding>(&item)) {
      item_panic = DropActionOut(ctx, drop->binding, sigma) ==
                   DropStatus::Panic;
      if (item_panic) {
        if (status == CleanupStatus::Panic) {
          SPEC_RULE("Cleanup-Step-Drop-Abort");
          return CleanupStatus::Abort;
        }
        SPEC_RULE("Cleanup-Step-Drop-Panic");
      } else {
        SPEC_RULE("Cleanup-Step-Drop-Ok");
      }
    } else if (const auto* drop_static = std::get_if<DropStatic>(&item)) {
      item_panic = DropStaticActionOut(ctx,
                                       drop_static->path,
                                       drop_static->name,
                                       sigma) == DropStatus::Panic;
      if (item_panic) {
        if (status == CleanupStatus::Panic) {
          SPEC_RULE("Cleanup-Step-DropStatic-Abort");
          return CleanupStatus::Abort;
        }
        SPEC_RULE("Cleanup-Step-DropStatic-Panic");
      } else {
        SPEC_RULE("Cleanup-Step-DropStatic-Ok");
      }
    } else if (const auto* defer = std::get_if<DeferBlock>(&item)) {
      SemanticsContext inner_ctx = ctx;
      inner_ctx.temp_ctx = nullptr;
      Outcome out = EvalBlockSigma(inner_ctx, *defer->block, sigma);
      item_panic = IsCtrl(out) ? IsPanicCtrl(out) : false;
      if (item_panic) {
        if (status == CleanupStatus::Panic) {
          SPEC_RULE("Cleanup-Step-Defer-Abort");
          return CleanupStatus::Abort;
        }
        SPEC_RULE("Cleanup-Step-Defer-Panic");
      } else {
        SPEC_RULE("Cleanup-Step-Defer-Ok");
      }
    }
    if (item_panic) {
      status = CleanupStatus::Panic;
    }
  }
  SPEC_RULE("Cleanup-Done");
  SPEC_RULE("CleanupScope-From-SmallStep");
  return status;
}

CleanupStatus Unwind(const SemanticsContext& ctx,
                     const std::vector<ScopeEntry>& frames,
                     Sigma& sigma) {
  for (const auto& frame : frames) {
    const CleanupStatus status = CleanupScope(ctx, frame, sigma);
    if (status != CleanupStatus::Ok) {
      SPEC_RULE("Unwind-Abort");
      return CleanupStatus::Abort;
    }
    SPEC_RULE("Unwind-Step");
  }
  return CleanupStatus::Ok;
}

}  // namespace cursive0::eval

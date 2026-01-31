// C0X Extension: Structured Concurrency Built-in Types (§18)

#include "cursive0/03_analysis/caps/cap_concurrency.h"

#include <utility>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/resolve/scopes.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsConcurrency() {
  SPEC_DEF("ExecutionDomainClass", "18.2.4");
  SPEC_DEF("SpawnedModal", "18.4.2");
  SPEC_DEF("CancelTokenModal", "18.6.1");
  SPEC_DEF("TrackedModal", "5.4.4");
  SPEC_DEF("AsyncModal", "5.4.5");
  SPEC_DEF("ReactorClass", "5.9.2");
}

static std::shared_ptr<syntax::Type> MakeTypeNode(const syntax::TypeNode& node) {
  auto ty = std::make_shared<syntax::Type>();
  ty->span = core::Span{};
  ty->node = node;
  return ty;
}

static std::shared_ptr<syntax::Type> MakeTypePrimAst(std::string_view name) {
  return MakeTypeNode(syntax::TypePrim{syntax::Identifier{name}});
}

static std::shared_ptr<syntax::Type> MakeTypePathAst(
    std::initializer_list<std::string_view> comps) {
  syntax::TypePath path;
  for (const auto comp : comps) {
    path.emplace_back(comp);
  }
  return MakeTypeNode(syntax::TypePathType{std::move(path)});
}

static std::shared_ptr<syntax::Type> MakeTypeUnionAst(
    std::vector<std::shared_ptr<syntax::Type>> members) {
  syntax::TypeUnion node;
  node.types = std::move(members);
  return MakeTypeNode(node);
}

static syntax::Param MakeParam(std::string_view name,
                               std::shared_ptr<syntax::Type> type) {
  syntax::Param param{};
  param.mode = std::nullopt;
  param.name = std::string(name);
  param.type = std::move(type);
  return param;
}

static std::shared_ptr<syntax::Type> MakeTypeModalStateAst(
    std::initializer_list<std::string_view> comps,
    std::string_view state) {
  syntax::TypeModalState modal;
  for (const auto comp : comps) {
    modal.path.emplace_back(comp);
  }
  modal.state = syntax::Identifier{state};
  return MakeTypeNode(modal);
}

static std::shared_ptr<syntax::Block> MakeEmptyBlock() {
  auto block = std::make_shared<syntax::Block>();
  block->stmts = {};
  block->tail_opt = nullptr;
  return block;
}

static syntax::TypeParam MakeTypeParam(std::string_view name,
                                       std::shared_ptr<syntax::Type> default_type) {
  syntax::TypeParam param{};
  param.name = std::string(name);
  param.default_type = std::move(default_type);
  param.span = core::Span{};
  return param;
}

static std::optional<syntax::GenericParams> MakeGenericParams(
    std::vector<syntax::TypeParam> params) {
  syntax::GenericParams gen{};
  gen.params = std::move(params);
  gen.span = core::Span{};
  return gen;
}

static syntax::StateFieldDecl MakeStateField(std::string_view name,
                                             std::shared_ptr<syntax::Type> type) {
  syntax::StateFieldDecl field{};
  field.vis = syntax::Visibility::Public;
  field.name = std::string(name);
  field.type = std::move(type);
  field.span = core::Span{};
  field.doc_opt = std::nullopt;
  return field;
}

static syntax::StateMethodDecl MakeStateMethod(std::string_view name,
                                               std::shared_ptr<syntax::Type> ret) {
  syntax::StateMethodDecl method;
  method.vis = syntax::Visibility::Public;
  method.name = std::string(name);
  method.params = {};
  method.return_type_opt = std::move(ret);
  method.body = MakeEmptyBlock();
  method.span = core::Span{};
  method.doc_opt = std::nullopt;
  return method;
}

static TypeRef TypeUnit() {
  return MakeTypeTuple({});
}

static TypeRef TypeBool() {
  return MakeTypePrim("bool");
}

static TypeRef TypeUsize() {
  return MakeTypePrim("usize");
}

// Helper for string comparison
static bool StrEq(std::string_view a, std::string_view b) {
  return a == b;
}

}  // namespace

// -----------------------------------------------------------------------------
// ExecutionDomain class (§18.2.4)
// -----------------------------------------------------------------------------

bool IsExecutionDomainClassPath(const syntax::ClassPath& path) {
  SpecDefsConcurrency();
  return path.size() == 1 && IdEq(path[0], "ExecutionDomain");
}

bool IsExecutionDomainTypePath(const syntax::TypePath& path) {
  SpecDefsConcurrency();
  if (path.size() != 1) {
    return false;
  }
  return IdEq(path[0], "ExecutionDomain") ||
         IdEq(path[0], "CpuDomain") ||
         IdEq(path[0], "GpuDomain") ||
         IdEq(path[0], "InlineDomain");
}

std::optional<ExecutionDomainMethodSig> LookupExecutionDomainMethodSig(
    std::string_view name) {
  SpecDefsConcurrency();
  ExecutionDomainMethodSig sig{};

  // §18.2.4: procedure name(~) -> string
  if (StrEq(name, "name")) {
    sig.recv_perm = Permission::Const;
    sig.params = {};
    sig.ret = MakeTypeString(StringState::View);
    return sig;
  }

  // §18.2.4: procedure max_concurrency(~) -> usize
  if (StrEq(name, "max_concurrency")) {
    sig.recv_perm = Permission::Const;
    sig.params = {};
    sig.ret = TypeUsize();
    return sig;
  }

  return std::nullopt;
}

static syntax::ClassDecl BuildDomainClassDecl(std::string_view name) {
  syntax::ClassDecl decl;
  decl.vis = syntax::Visibility::Public;
  decl.name = std::string(name);
  decl.supers = {{"ExecutionDomain"}};
  decl.items = {};
  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

syntax::ClassDecl BuildCpuDomainClassDecl() {
  SpecDefsConcurrency();
  return BuildDomainClassDecl("CpuDomain");
}

syntax::ClassDecl BuildGpuDomainClassDecl() {
  SpecDefsConcurrency();
  return BuildDomainClassDecl("GpuDomain");
}

syntax::ClassDecl BuildInlineDomainClassDecl() {
  SpecDefsConcurrency();
  return BuildDomainClassDecl("InlineDomain");
}

// -----------------------------------------------------------------------------
// Spawned<T> modal type (§18.4.2)
// -----------------------------------------------------------------------------

bool IsSpawnedTypePath(const syntax::TypePath& path) {
  SpecDefsConcurrency();
  return path.size() == 1 && IdEq(path[0], "Spawned");
}

bool IsValidSpawnedState(std::string_view state) {
  SpecDefsConcurrency();
  return StrEq(state, "Pending") || StrEq(state, "Ready");
}

TypeRef MakeSpawnedType(const TypeRef& inner_type) {
  SpecDefsConcurrency();
  TypePathType path_type;
  path_type.path = {"Spawned"};
  path_type.generic_args = {inner_type};
  return MakeType(path_type);
}

TypeRef MakeSpawnedTypeWithState(const TypeRef& inner_type,
                                 std::string_view state) {
  SpecDefsConcurrency();
  // For modal state types, we need to use TypeModalState
  // Spawned<T>@State
  TypeModalState modal;
  modal.path = {"Spawned"};
  modal.state = std::string(state);
  modal.generic_args = {inner_type};
  return MakeType(modal);
}

std::optional<TypeRef> ExtractSpawnedInner(const TypeRef& type) {
  SpecDefsConcurrency();
  if (!type) return std::nullopt;

  // Check for TypePathType with Spawned path
  const auto* path = std::get_if<TypePathType>(&type->node);
  if (path && !path->path.empty() && IdEq(path->path.back(), "Spawned")) {
    if (!path->generic_args.empty()) {
      return path->generic_args[0];
    }
    return std::nullopt;
  }

  // Check for TypeModalState with Spawned path
  const auto* modal = std::get_if<TypeModalState>(&type->node);
  if (modal && !modal->path.empty() && IdEq(modal->path.back(), "Spawned")) {
    if (!modal->generic_args.empty()) {
      return modal->generic_args[0];
    }
    return std::nullopt;
  }

  return std::nullopt;
}

// -----------------------------------------------------------------------------
// Tracked<T, E> modal type (§5.4.4)
// -----------------------------------------------------------------------------

bool IsTrackedTypePath(const syntax::TypePath& path) {
  SpecDefsConcurrency();
  return path.size() == 1 && IdEq(path[0], "Tracked");
}

bool IsValidTrackedState(std::string_view state) {
  SpecDefsConcurrency();
  return StrEq(state, "Pending") || StrEq(state, "Ready");
}

TypeRef MakeTrackedType(const TypeRef& value_type,
                        const TypeRef& err_type) {
  SpecDefsConcurrency();
  TypePathType path_type;
  path_type.path = {"Tracked"};
  path_type.generic_args = {value_type, err_type};
  return MakeType(path_type);
}

TypeRef MakeTrackedTypeWithState(const TypeRef& value_type,
                                 const TypeRef& err_type,
                                 std::string_view state) {
  SpecDefsConcurrency();
  TypeModalState modal;
  modal.path = {"Tracked"};
  modal.state = std::string(state);
  modal.generic_args = {value_type, err_type};
  return MakeType(modal);
}

std::optional<std::pair<TypeRef, TypeRef>> ExtractTrackedArgs(
    const TypeRef& type) {
  SpecDefsConcurrency();
  if (!type) {
    return std::nullopt;
  }
  const auto* path = std::get_if<TypePathType>(&type->node);
  if (path && IsTrackedTypePath(path->path)) {
    if (path->generic_args.size() == 2) {
      return std::make_pair(path->generic_args[0], path->generic_args[1]);
    }
    return std::nullopt;
  }
  const auto* modal = std::get_if<TypeModalState>(&type->node);
  if (modal && IsTrackedTypePath(modal->path)) {
    if (modal->generic_args.size() == 2) {
      return std::make_pair(modal->generic_args[0], modal->generic_args[1]);
    }
    return std::nullopt;
  }
  return std::nullopt;
}

// -----------------------------------------------------------------------------
// CancelToken modal type (§18.6.1)
// -----------------------------------------------------------------------------

bool IsCancelTokenTypePath(const syntax::TypePath& path) {
  SpecDefsConcurrency();
  return path.size() == 1 && IdEq(path[0], "CancelToken");
}

bool IsValidCancelTokenState(std::string_view state) {
  SpecDefsConcurrency();
  return StrEq(state, "Active") || StrEq(state, "Cancelled");
}

std::optional<CancelTokenMethodSig> LookupCancelTokenMethodSig(
    std::string_view name,
    std::string_view state) {
  SpecDefsConcurrency();
  CancelTokenMethodSig sig{};

  // §18.6.1: procedure cancel(~%) - only in @Active state
  if (StrEq(name, "cancel")) {
    if (!StrEq(state, "Active") && !state.empty()) {
      return std::nullopt;  // Only valid in @Active state
    }
    sig.recv_perm = Permission::Shared;
    sig.params = {};
    sig.ret = TypeUnit();
    sig.valid_states = "Active";
    return sig;
  }

  // §18.6.1: procedure is_cancelled(~) -> bool - in both states
  if (StrEq(name, "is_cancelled")) {
    sig.recv_perm = Permission::Const;
    sig.params = {};
    sig.ret = TypeBool();
    sig.valid_states = "Active,Cancelled";
    return sig;
  }

  // §18.6.1: procedure child(~) -> CancelToken@Active - only in @Active state
  if (StrEq(name, "child")) {
    if (!StrEq(state, "Active") && !state.empty()) {
      return std::nullopt;  // Only valid in @Active state
    }
    sig.recv_perm = Permission::Const;
    sig.params = {};
    sig.ret = MakeCancelTokenTypeWithState("Active");
    sig.valid_states = "Active";
    return sig;
  }

  // §18.6.1: procedure new() -> CancelToken@Active - static constructor
  if (StrEq(name, "new")) {
    sig.recv_perm = Permission::Const;  // Not used for static
    sig.params = {};
    sig.ret = MakeCancelTokenTypeWithState("Active");
    sig.valid_states = "";  // Static method
    return sig;
  }

  return std::nullopt;
}

TypeRef MakeCancelTokenType() {
  SpecDefsConcurrency();
  TypePathType path_type;
  path_type.path = {"CancelToken"};
  path_type.generic_args = {};
  return MakeType(path_type);
}

TypeRef MakeCancelTokenTypeWithState(std::string_view state) {
  SpecDefsConcurrency();
  TypeModalState modal;
  modal.path = {"CancelToken"};
  modal.state = std::string(state);
  modal.generic_args = {};
  return MakeType(modal);
}

// -----------------------------------------------------------------------------
// Built-in type declarations for sigma population
// -----------------------------------------------------------------------------

syntax::ModalDecl BuildSpawnedModalDecl() {
  SpecDefsConcurrency();
  syntax::ModalDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Spawned";
  decl.generic_params = MakeGenericParams({MakeTypeParam("T", nullptr)});
  decl.implements = {};

  // State @Pending - task has been created but not completed
  {
    syntax::StateBlock pending_state{};
    pending_state.name = "Pending";
    pending_state.members = {};
    decl.states.push_back(pending_state);
  }

  // State @Ready - task has completed, value is available
  {
    syntax::StateBlock ready_state{};
    ready_state.name = "Ready";
    ready_state.members = {
        MakeStateField("value", MakeTypePathAst({"T"})),
    };
    decl.states.push_back(ready_state);
  }

  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

syntax::ModalDecl BuildCancelTokenModalDecl() {
  SpecDefsConcurrency();
  syntax::ModalDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "CancelToken";
  decl.implements = {};

  // State @Active
  {
    syntax::StateBlock active_state{};
    active_state.name = "Active";
    active_state.members = {
        MakeStateMethod("cancel", MakeTypePrimAst("()")),
        MakeStateMethod("is_cancelled", MakeTypePrimAst("bool")),
        MakeStateMethod("child", MakeTypeModalStateAst({"CancelToken"}, "Active")),
    };
    decl.states.push_back(active_state);
  }

  // State @Cancelled
  {
    syntax::StateBlock cancelled_state{};
    cancelled_state.name = "Cancelled";
    cancelled_state.members = {
        MakeStateMethod("is_cancelled", MakeTypePrimAst("bool")),
    };
    decl.states.push_back(cancelled_state);
  }

  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

syntax::ModalDecl BuildTrackedModalDecl() {
  SpecDefsConcurrency();
  syntax::ModalDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Tracked";
  decl.generic_params =
      MakeGenericParams({MakeTypeParam("T", nullptr), MakeTypeParam("E", nullptr)});
  decl.implements = {};

  {
    syntax::StateBlock pending_state{};
    pending_state.name = "Pending";
    pending_state.members = {};
    decl.states.push_back(pending_state);
  }
  {
    syntax::StateBlock ready_state{};
    ready_state.name = "Ready";
    std::vector<std::shared_ptr<syntax::Type>> union_members;
    union_members.push_back(MakeTypePathAst({"T"}));
    union_members.push_back(MakeTypePathAst({"E"}));
    ready_state.members = {
        MakeStateField("value", MakeTypeUnionAst(std::move(union_members))),
    };
    decl.states.push_back(ready_state);
  }

  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

syntax::ModalDecl BuildAsyncModalDecl() {
  SpecDefsConcurrency();
  syntax::ModalDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Async";
  decl.generic_params = MakeGenericParams({
      MakeTypeParam("Out", nullptr),
      MakeTypeParam("In", MakeTypePrimAst("()")),
      MakeTypeParam("Result", MakeTypePrimAst("()")),
      MakeTypeParam("E", MakeTypePrimAst("!")),
  });
  decl.implements = {};

  // Build Async<Out, In, Result, E>@State modal state refs for resume return type
  auto async_state = [&](std::string_view state) {
    syntax::TypeModalState modal;
    modal.path = {"Async"};
    modal.state = syntax::Identifier{state};
    modal.generic_args = {
        MakeTypePathAst({"Out"}),
        MakeTypePathAst({"In"}),
        MakeTypePathAst({"Result"}),
        MakeTypePathAst({"E"}),
    };
    return MakeTypeNode(modal);
  };

  // @Suspended state
  {
    syntax::StateBlock suspended{};
    suspended.name = "Suspended";
    std::vector<std::shared_ptr<syntax::Type>> resume_union;
    resume_union.push_back(async_state("Suspended"));
    resume_union.push_back(async_state("Completed"));
    resume_union.push_back(async_state("Failed"));
    syntax::StateMethodDecl resume;
    resume.vis = syntax::Visibility::Public;
    resume.name = "resume";
    resume.params = {MakeParam("input", MakeTypePathAst({"In"}))};
    resume.return_type_opt = MakeTypeUnionAst(std::move(resume_union));
    resume.body = MakeEmptyBlock();
    resume.span = core::Span{};
    resume.doc_opt = std::nullopt;
    suspended.members = {
        MakeStateField("output", MakeTypePathAst({"Out"})),
        resume,
    };
    decl.states.push_back(suspended);
  }

  // @Completed state
  {
    syntax::StateBlock completed{};
    completed.name = "Completed";
    completed.members = {
        MakeStateField("value", MakeTypePathAst({"Result"})),
    };
    decl.states.push_back(completed);
  }

  // @Failed state
  {
    syntax::StateBlock failed{};
    failed.name = "Failed";
    failed.members = {
        MakeStateField("error", MakeTypePathAst({"E"})),
    };
    decl.states.push_back(failed);
  }

  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

// -----------------------------------------------------------------------------
// Built-in async aliases (§5.4.5)
// -----------------------------------------------------------------------------

syntax::TypeAliasDecl BuildSequenceAliasDecl() {
  SpecDefsConcurrency();
  syntax::TypeAliasDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Sequence";
  decl.generic_params = MakeGenericParams({MakeTypeParam("T", nullptr)});
  syntax::TypePathType body{};
  body.path = {"Async"};
  body.generic_args = {MakeTypePathAst({"T"}),
                       MakeTypePrimAst("()"),
                       MakeTypePrimAst("()"),
                       MakeTypePrimAst("!")};
  decl.type = MakeTypeNode(body);
  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

syntax::TypeAliasDecl BuildFutureAliasDecl() {
  SpecDefsConcurrency();
  syntax::TypeAliasDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Future";
  decl.generic_params =
      MakeGenericParams({MakeTypeParam("T", nullptr),
                         MakeTypeParam("E", MakeTypePrimAst("!"))});
  syntax::TypePathType body{};
  body.path = {"Async"};
  body.generic_args = {MakeTypePrimAst("()"),
                       MakeTypePrimAst("()"),
                       MakeTypePathAst({"T"}),
                       MakeTypePathAst({"E"})};
  decl.type = MakeTypeNode(body);
  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

syntax::TypeAliasDecl BuildStreamAliasDecl() {
  SpecDefsConcurrency();
  syntax::TypeAliasDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Stream";
  decl.generic_params =
      MakeGenericParams({MakeTypeParam("T", nullptr),
                         MakeTypeParam("E", nullptr)});
  syntax::TypePathType body{};
  body.path = {"Async"};
  body.generic_args = {MakeTypePathAst({"T"}),
                       MakeTypePrimAst("()"),
                       MakeTypePrimAst("()"),
                       MakeTypePathAst({"E"})};
  decl.type = MakeTypeNode(body);
  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

syntax::TypeAliasDecl BuildPipeAliasDecl() {
  SpecDefsConcurrency();
  syntax::TypeAliasDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Pipe";
  decl.generic_params =
      MakeGenericParams({MakeTypeParam("In", nullptr),
                         MakeTypeParam("Out", nullptr)});
  syntax::TypePathType body{};
  body.path = {"Async"};
  body.generic_args = {MakeTypePathAst({"Out"}),
                       MakeTypePathAst({"In"}),
                       MakeTypePrimAst("()"),
                       MakeTypePrimAst("!")};
  decl.type = MakeTypeNode(body);
  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

syntax::TypeAliasDecl BuildExchangeAliasDecl() {
  SpecDefsConcurrency();
  syntax::TypeAliasDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Exchange";
  decl.generic_params = MakeGenericParams({MakeTypeParam("T", nullptr)});
  syntax::TypePathType body{};
  body.path = {"Async"};
  body.generic_args = {MakeTypePathAst({"T"}),
                       MakeTypePathAst({"T"}),
                       MakeTypePathAst({"T"}),
                       MakeTypePrimAst("!")};
  decl.type = MakeTypeNode(body);
  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

syntax::ClassDecl BuildExecutionDomainClassDecl() {
  SpecDefsConcurrency();
  syntax::ClassDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "ExecutionDomain";
  decl.modal = false;
  decl.generic_params = std::nullopt;
  decl.supers = {};
  decl.where_clause = std::nullopt;
  decl.items = {};
  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

bool IsReactorClassPath(const syntax::ClassPath& path) {
  SpecDefsConcurrency();
  return path.size() == 1 && IdEq(path[0], "Reactor");
}

syntax::ClassDecl BuildReactorClassDecl() {
  SpecDefsConcurrency();
  syntax::ClassDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "Reactor";
  decl.modal = false;
  decl.generic_params = std::nullopt;
  decl.supers = {};
  decl.where_clause = std::nullopt;
  decl.items = {};
  decl.span = core::Span{};
  decl.doc = {};
  return decl;
}

}  // namespace cursive0::analysis

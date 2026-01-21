#include "cursive0/sema/cap_system.h"

#include <utility>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/span.h"
#include "cursive0/sema/scopes.h"

namespace cursive0::sema {

namespace {

static inline void SpecDefsCapSystem() {
  SPEC_DEF("SystemInterface", "5.9.4");
  SPEC_DEF("SystemMethodSig", "5.9.4");
  SPEC_DEF("BuiltinRecord", "5.9.4");
  SPEC_DEF("Fields(Context)", "5.9.4");
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

static std::shared_ptr<syntax::Type> MakeTypeStringAst(
    std::optional<syntax::StringState> state) {
  syntax::TypeString node;
  node.state = state;
  return MakeTypeNode(node);
}

static std::shared_ptr<syntax::Type> MakeTypeDynamicAst(
    std::initializer_list<std::string_view> comps) {
  syntax::TypeDynamic node;
  for (const auto comp : comps) {
    node.path.emplace_back(comp);
  }
  return MakeTypeNode(node);
}

static std::shared_ptr<syntax::Type> MakeTypePathAst(
    std::initializer_list<std::string_view> comps) {
  syntax::TypePath path;
  for (const auto comp : comps) {
    path.emplace_back(comp);
  }
  return MakeTypeNode(syntax::TypePathType{std::move(path)});
}

static syntax::Param MakeParam(std::string_view name,
                               std::shared_ptr<syntax::Type> type) {
  syntax::Param param{};
  param.mode = std::nullopt;
  param.name = std::string(name);
  param.type = std::move(type);
  return param;
}

static syntax::FieldDecl MakeField(std::string_view name,
                                   std::shared_ptr<syntax::Type> type) {
  syntax::FieldDecl field{};
  field.vis = syntax::Visibility::Public;
  field.name = std::string(name);
  field.type = std::move(type);
  field.init_opt = nullptr;
  field.span = core::Span{};
  field.doc_opt = std::nullopt;
  return field;
}

static TypeRef TypeUnit() {
  return MakeTypePrim("()");
}

static TypeRef TypeNever() {
  return MakeTypePrim("!");
}

static TypeRef TypeStringAny() {
  return MakeTypeString(std::nullopt);
}

static TypeRef Union2(TypeRef a, TypeRef b) {
  std::vector<TypeRef> members;
  members.reserve(2);
  members.push_back(std::move(a));
  members.push_back(std::move(b));
  return MakeTypeUnion(std::move(members));
}

}  // namespace

std::optional<SystemMethodSig> LookupSystemMethodSig(std::string_view name) {
  SpecDefsCapSystem();
  SystemMethodSig sig{};
  sig.recv_perm = Permission::Const;

  if (IdEq(name, "exit")) {
    sig.params = {MakeParam("code", MakeTypePrimAst("i32"))};
    sig.ret = TypeNever();
    return sig;
  }
  if (IdEq(name, "get_env")) {
    sig.params = {MakeParam("key", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeStringAny(), TypeUnit());
    return sig;
  }

  return std::nullopt;
}

syntax::RecordDecl BuildContextRecordDecl() {
  SpecDefsCapSystem();
  syntax::RecordDecl record;
  record.vis = syntax::Visibility::Public;
  record.name = "Context";
  record.implements = {{"Bitcopy"}};
  record.members = {
      MakeField("fs", MakeTypeDynamicAst({"FileSystem"})),
      MakeField("heap", MakeTypeDynamicAst({"HeapAllocator"})),
      MakeField("sys", MakeTypePathAst({"System"})),
  };
  record.span = core::Span{};
  record.doc = {};
  return record;
}

syntax::RecordDecl BuildSystemRecordDecl() {
  SpecDefsCapSystem();
  syntax::RecordDecl record;
  record.vis = syntax::Visibility::Public;
  record.name = "System";
  record.implements = {{"Bitcopy"}};
  record.members = {};
  record.span = core::Span{};
  record.doc = {};
  return record;
}

}  // namespace cursive0::sema

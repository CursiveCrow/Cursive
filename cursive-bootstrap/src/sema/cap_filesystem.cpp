#include "cursive0/sema/cap_filesystem.h"

#include <utility>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/scopes.h"

namespace cursive0::sema {

namespace {

static inline void SpecDefsCapFileSystem() {
  SPEC_DEF("CapClass", "5.9.1");
  SPEC_DEF("CapType", "5.9.1");
  SPEC_DEF("CapMethodSig", "5.9.1");
  SPEC_DEF("CapRecv", "5.9.1");
  SPEC_DEF("FileSystemInterface", "5.9.2");
  SPEC_DEF("BuiltinTypes_FS", "5.9.2");
  SPEC_DEF("DirIterStateMembers", "5.9.2");
  SPEC_DEF("FileStateMembers", "5.9.2");
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

static std::shared_ptr<syntax::Type> MakeTypeStringAst(
    std::optional<syntax::StringState> state) {
  syntax::TypeString node;
  node.state = state;
  return MakeTypeNode(node);
}

static std::shared_ptr<syntax::Type> MakeTypeBytesAst(
    std::optional<syntax::BytesState> state) {
  syntax::TypeBytes node;
  node.state = state;
  return MakeTypeNode(node);
}

static std::shared_ptr<syntax::Type> MakeTypeModalStateAst(
    std::initializer_list<std::string_view> comps,
    std::string_view state) {
  syntax::TypeModalState node;
  for (const auto comp : comps) {
    node.path.emplace_back(comp);
  }
  node.state = syntax::Identifier{state};
  return MakeTypeNode(node);
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

static std::shared_ptr<syntax::Block> MakeEmptyBlock() {
  auto block = std::make_shared<syntax::Block>();
  block->span = core::Span{};
  return block;
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
                                               std::vector<syntax::Param> params,
                                               std::shared_ptr<syntax::Type> ret) {
  syntax::StateMethodDecl method{};
  method.vis = syntax::Visibility::Public;
  method.name = std::string(name);
  method.params = std::move(params);
  method.return_type_opt = std::move(ret);
  method.body = MakeEmptyBlock();
  method.span = core::Span{};
  method.doc_opt = std::nullopt;
  return method;
}

static syntax::TransitionDecl MakeTransition(std::string_view name,
                                             std::vector<syntax::Param> params,
                                             std::string_view target_state) {
  syntax::TransitionDecl trans{};
  trans.vis = syntax::Visibility::Public;
  trans.name = std::string(name);
  trans.params = std::move(params);
  trans.target_state = std::string(target_state);
  trans.body = MakeEmptyBlock();
  trans.span = core::Span{};
  trans.doc_opt = std::nullopt;
  return trans;
}

static TypeRef TypeStringView() {
  return MakeTypeString(StringState::View);
}

static TypeRef TypeBytesView() {
  return MakeTypeBytes(BytesState::View);
}

static TypeRef TypeBytesManaged() {
  return MakeTypeBytes(BytesState::Managed);
}

static TypeRef TypeStringManaged() {
  return MakeTypeString(StringState::Managed);
}

static TypeRef TypeIoError() {
  return MakeTypePath({"IoError"});
}

static TypeRef TypeFileKind() {
  return MakeTypePath({"FileKind"});
}

static TypeRef TypeUnique(TypeRef base) {
  return MakeTypePerm(Permission::Unique, std::move(base));
}

static TypeRef TypeFileState(std::string_view state) {
  return TypeUnique(MakeTypeModalState({"File"}, std::string(state)));
}

static TypeRef TypeDirIterState(std::string_view state) {
  return TypeUnique(MakeTypeModalState({"DirIter"}, std::string(state)));
}

static TypeRef TypeDirEntry() {
  return MakeTypePath({"DirEntry"});
}

static TypeRef TypeFileSystemDynamic() {
  return MakeTypeDynamic({"FileSystem"});
}

static TypeRef TypeUnit() {
  return MakeTypePrim("()");
}

static TypeRef TypeBool() {
  return MakeTypePrim("bool");
}

static TypeRef Union2(TypeRef a, TypeRef b) {
  std::vector<TypeRef> members;
  members.reserve(2);
  members.push_back(std::move(a));
  members.push_back(std::move(b));
  return MakeTypeUnion(std::move(members));
}

static TypeRef Union3(TypeRef a, TypeRef b, TypeRef c) {
  std::vector<TypeRef> members;
  members.reserve(3);
  members.push_back(std::move(a));
  members.push_back(std::move(b));
  members.push_back(std::move(c));
  return MakeTypeUnion(std::move(members));
}

}  // namespace

bool IsFileSystemBuiltinTypePath(const syntax::TypePath& path) {
  SpecDefsCapFileSystem();
  return path.size() == 1 &&
         (IdEq(path[0], "File") || IdEq(path[0], "DirIter") ||
          IdEq(path[0], "DirEntry") || IdEq(path[0], "FileKind") ||
          IdEq(path[0], "IoError"));
}

bool IsFileSystemClassPath(const syntax::ClassPath& path) {
  SpecDefsCapFileSystem();
  return path.size() == 1 && IdEq(path[0], "FileSystem");
}

std::optional<FileSystemMethodSig> LookupFileSystemMethodSig(
    std::string_view name) {
  SpecDefsCapFileSystem();
  FileSystemMethodSig sig{};
  sig.recv_perm = Permission::Const;

  if (IdEq(name, "open_read")) {
    sig.params = {MakeParam("path", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeFileState("Read"), TypeIoError());
    return sig;
  }
  if (IdEq(name, "open_write")) {
    sig.params = {MakeParam("path", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeFileState("Write"), TypeIoError());
    return sig;
  }
  if (IdEq(name, "open_append")) {
    sig.params = {MakeParam("path", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeFileState("Append"), TypeIoError());
    return sig;
  }
  if (IdEq(name, "create_write")) {
    sig.params = {MakeParam("path", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeFileState("Write"), TypeIoError());
    return sig;
  }
  if (IdEq(name, "read_file")) {
    sig.params = {MakeParam("path", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeStringManaged(), TypeIoError());
    return sig;
  }
  if (IdEq(name, "read_bytes")) {
    sig.params = {MakeParam("path", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeBytesManaged(), TypeIoError());
    return sig;
  }
  if (IdEq(name, "write_file")) {
    sig.params = {
        MakeParam("path", MakeTypeStringAst(syntax::StringState::View)),
        MakeParam("data", MakeTypeBytesAst(syntax::BytesState::View)),
    };
    sig.ret = Union2(TypeUnit(), TypeIoError());
    return sig;
  }
  if (IdEq(name, "write_stdout")) {
    sig.params = {MakeParam("data", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeUnit(), TypeIoError());
    return sig;
  }
  if (IdEq(name, "write_stderr")) {
    sig.params = {MakeParam("data", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeUnit(), TypeIoError());
    return sig;
  }
  if (IdEq(name, "exists")) {
    sig.params = {MakeParam("path", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = TypeBool();
    return sig;
  }
  if (IdEq(name, "remove")) {
    sig.params = {MakeParam("path", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeUnit(), TypeIoError());
    return sig;
  }
  if (IdEq(name, "open_dir")) {
    sig.params = {MakeParam("path", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeDirIterState("Open"), TypeIoError());
    return sig;
  }
  if (IdEq(name, "create_dir")) {
    sig.params = {MakeParam("path", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeUnit(), TypeIoError());
    return sig;
  }
  if (IdEq(name, "ensure_dir")) {
    sig.params = {MakeParam("path", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeUnit(), TypeIoError());
    return sig;
  }
  if (IdEq(name, "kind")) {
    sig.params = {MakeParam("path", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = Union2(TypeFileKind(), TypeIoError());
    return sig;
  }
  if (IdEq(name, "restrict")) {
    sig.params = {MakeParam("path", MakeTypeStringAst(syntax::StringState::View))};
    sig.ret = TypeFileSystemDynamic();
    return sig;
  }

  return std::nullopt;
}

syntax::ModalDecl BuildFileModalDecl() {
  SpecDefsCapFileSystem();
  syntax::ModalDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "File";
  decl.implements = {};
  decl.doc = {};

  auto make_state = [](std::string_view name,
                       std::vector<syntax::StateMember> members) {
    syntax::StateBlock state{};
    state.name = std::string(name);
    state.members = std::move(members);
    state.span = core::Span{};
    state.doc_opt = std::nullopt;
    return state;
  };

  auto handle_field = MakeStateField("handle", MakeTypePrimAst("usize"));

  std::vector<syntax::StateMember> read_members;
  read_members.push_back(handle_field);
  read_members.push_back(MakeStateMethod(
      "read_all", {},
      MakeTypeUnionAst({MakeTypeStringAst(syntax::StringState::Managed),
                        MakeTypePathAst({"IoError"})})));
  read_members.push_back(MakeStateMethod(
      "read_all_bytes", {},
      MakeTypeUnionAst({MakeTypeBytesAst(syntax::BytesState::Managed),
                        MakeTypePathAst({"IoError"})})));
  read_members.push_back(MakeTransition("close", {}, "Closed"));

  std::vector<syntax::StateMember> write_members;
  write_members.push_back(handle_field);
  write_members.push_back(MakeStateMethod(
      "write", {MakeParam("data", MakeTypeBytesAst(syntax::BytesState::View))},
      MakeTypeUnionAst({MakeTypePrimAst("()"), MakeTypePathAst({"IoError"})})));
  write_members.push_back(MakeStateMethod(
      "flush", {},
      MakeTypeUnionAst({MakeTypePrimAst("()"), MakeTypePathAst({"IoError"})})));
  write_members.push_back(MakeTransition("close", {}, "Closed"));

  std::vector<syntax::StateMember> append_members;
  append_members.push_back(handle_field);
  append_members.push_back(MakeStateMethod(
      "write", {MakeParam("data", MakeTypeBytesAst(syntax::BytesState::View))},
      MakeTypeUnionAst({MakeTypePrimAst("()"), MakeTypePathAst({"IoError"})})));
  append_members.push_back(MakeStateMethod(
      "flush", {},
      MakeTypeUnionAst({MakeTypePrimAst("()"), MakeTypePathAst({"IoError"})})));
  append_members.push_back(MakeTransition("close", {}, "Closed"));

  std::vector<syntax::StateMember> closed_members;

  decl.states = {
      make_state("Read", std::move(read_members)),
      make_state("Write", std::move(write_members)),
      make_state("Append", std::move(append_members)),
      make_state("Closed", std::move(closed_members)),
  };

  return decl;
}

syntax::ModalDecl BuildDirIterModalDecl() {
  SpecDefsCapFileSystem();
  syntax::ModalDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "DirIter";
  decl.implements = {};
  decl.doc = {};

  auto make_state = [](std::string_view name,
                       std::vector<syntax::StateMember> members) {
    syntax::StateBlock state{};
    state.name = std::string(name);
    state.members = std::move(members);
    state.span = core::Span{};
    state.doc_opt = std::nullopt;
    return state;
  };

  auto handle_field = MakeStateField("handle", MakeTypePrimAst("usize"));

  std::vector<syntax::StateMember> open_members;
  open_members.push_back(handle_field);
  open_members.push_back(MakeStateMethod(
      "next", {},
      MakeTypeUnionAst({MakeTypePathAst({"DirEntry"}),
                        MakeTypePrimAst("()"),
                        MakeTypePathAst({"IoError"})})));
  open_members.push_back(MakeTransition("close", {}, "Closed"));

  std::vector<syntax::StateMember> closed_members;

  decl.states = {
      make_state("Open", std::move(open_members)),
      make_state("Closed", std::move(closed_members)),
  };

  return decl;
}

syntax::RecordDecl BuildDirEntryRecordDecl() {
  SpecDefsCapFileSystem();
  syntax::RecordDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "DirEntry";
  decl.implements = {};
  decl.span = core::Span{};
  decl.doc = {};

  syntax::FieldDecl name_field{};
  name_field.vis = syntax::Visibility::Public;
  name_field.name = "name";
  name_field.type = MakeTypeStringAst(syntax::StringState::Managed);
  name_field.init_opt = nullptr;
  name_field.span = core::Span{};
  name_field.doc_opt = std::nullopt;

  syntax::FieldDecl path_field{};
  path_field.vis = syntax::Visibility::Public;
  path_field.name = "path";
  path_field.type = MakeTypeStringAst(syntax::StringState::Managed);
  path_field.init_opt = nullptr;
  path_field.span = core::Span{};
  path_field.doc_opt = std::nullopt;

  syntax::FieldDecl kind_field{};
  kind_field.vis = syntax::Visibility::Public;
  kind_field.name = "kind";
  kind_field.type = MakeTypePathAst({"FileKind"});
  kind_field.init_opt = nullptr;
  kind_field.span = core::Span{};
  kind_field.doc_opt = std::nullopt;

  decl.members = {name_field, path_field, kind_field};
  return decl;
}

syntax::EnumDecl BuildFileKindEnumDecl() {
  SpecDefsCapFileSystem();
  syntax::EnumDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "FileKind";
  decl.implements = {{"Bitcopy"}};
  decl.span = core::Span{};
  decl.doc = {};

  auto make_variant = [](std::string_view name) {
    syntax::VariantDecl variant{};
    variant.name = std::string(name);
    variant.payload_opt = std::nullopt;
    variant.discriminant_opt = std::nullopt;
    variant.span = core::Span{};
    variant.doc_opt = std::nullopt;
    return variant;
  };

  decl.variants = {make_variant("File"), make_variant("Dir"),
                   make_variant("Other")};
  return decl;
}

syntax::EnumDecl BuildIoErrorEnumDecl() {
  SpecDefsCapFileSystem();
  syntax::EnumDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "IoError";
  decl.implements = {{"Bitcopy"}};
  decl.span = core::Span{};
  decl.doc = {};

  auto make_variant = [](std::string_view name) {
    syntax::VariantDecl variant{};
    variant.name = std::string(name);
    variant.payload_opt = std::nullopt;
    variant.discriminant_opt = std::nullopt;
    variant.span = core::Span{};
    variant.doc_opt = std::nullopt;
    return variant;
  };

  decl.variants = {make_variant("NotFound"), make_variant("PermissionDenied"),
                   make_variant("AlreadyExists"), make_variant("InvalidPath"),
                   make_variant("Busy"), make_variant("IoFailure")};
  return decl;
}

}  // namespace cursive0::sema

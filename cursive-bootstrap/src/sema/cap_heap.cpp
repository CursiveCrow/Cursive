#include "cursive0/sema/cap_heap.h"

#include <utility>

#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/scopes.h"

namespace cursive0::sema {

namespace {

static inline void SpecDefsCapHeap() {
  SPEC_DEF("CapClass", "5.9.1");
  SPEC_DEF("CapType", "5.9.1");
  SPEC_DEF("CapMethodSig", "5.9.1");
  SPEC_DEF("CapRecv", "5.9.1");
  SPEC_DEF("HeapAllocatorInterface", "5.9.3");
  SPEC_DEF("AllocationError", "5.9.3");
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

static std::shared_ptr<syntax::Type> MakeTypeRawPtrAst(
    syntax::RawPtrQual qual,
    std::shared_ptr<syntax::Type> elem) {
  syntax::TypeRawPtr node;
  node.qual = qual;
  node.element = std::move(elem);
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

static TypeRef TypeHeapAllocatorDynamic() {
  return MakeTypeDynamic({"HeapAllocator"});
}

static TypeRef TypeUnit() {
  return MakeTypePrim("()");
}


}  // namespace

bool IsHeapAllocatorBuiltinTypePath(const syntax::TypePath& path) {
  SpecDefsCapHeap();
  return path.size() == 1 && IdEq(path[0], "AllocationError");
}

bool IsHeapAllocatorClassPath(const syntax::ClassPath& path) {
  SpecDefsCapHeap();
  return path.size() == 1 && IdEq(path[0], "HeapAllocator");
}

std::optional<HeapAllocatorMethodSig> LookupHeapAllocatorMethodSig(
    std::string_view name) {
  SpecDefsCapHeap();
  HeapAllocatorMethodSig sig{};

  if (IdEq(name, "with_quota")) {
    sig.recv_perm = Permission::Unique;
    sig.params = {MakeParam("size", MakeTypePrimAst("usize"))};
    sig.ret = TypeHeapAllocatorDynamic();
    return sig;
  }
  if (IdEq(name, "alloc_raw")) {
    sig.recv_perm = Permission::Const;
    sig.params = {MakeParam("count", MakeTypePrimAst("usize"))};
    sig.ret = MakeTypeRawPtr(RawPtrQual::Mut, MakeTypePrim("u8"));
    sig.kind = HeapAllocatorMethodKind::AllocRaw;
    return sig;
  }
  if (IdEq(name, "dealloc_raw")) {
    sig.recv_perm = Permission::Const;
    sig.params = {
        MakeParam("ptr",
                  MakeTypeRawPtrAst(syntax::RawPtrQual::Mut,
                                    MakeTypePrimAst("u8"))),
        MakeParam("count", MakeTypePrimAst("usize")),
    };
    sig.ret = TypeUnit();
    sig.kind = HeapAllocatorMethodKind::DeallocRaw;
    return sig;
  }

  return std::nullopt;
}

syntax::EnumDecl BuildAllocationErrorEnumDecl() {
  SpecDefsCapHeap();
  syntax::EnumDecl decl{};
  decl.vis = syntax::Visibility::Public;
  decl.name = "AllocationError";
  decl.implements = {};
  decl.span = core::Span{};
  decl.doc = {};

  auto make_variant = [](std::string_view name) {
    syntax::VariantDecl variant{};
    variant.name = std::string(name);
    variant.discriminant_opt = std::nullopt;
    variant.span = core::Span{};
    variant.doc_opt = std::nullopt;

    syntax::VariantPayloadTuple tuple{};
    tuple.elements.push_back(MakeTypePrimAst("usize"));
    variant.payload_opt = syntax::VariantPayload{std::move(tuple)};
    return variant;
  };

  decl.variants = {make_variant("OutOfMemory"), make_variant("QuotaExceeded")};
  return decl;
}

}  // namespace cursive0::sema

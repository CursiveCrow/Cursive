#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

enum class HeapAllocatorMethodKind {
  Normal,
  AllocRaw,
  DeallocRaw,
};

struct HeapAllocatorMethodSig {
  Permission recv_perm;
  std::vector<syntax::Param> params;
  TypeRef ret;
  HeapAllocatorMethodKind kind = HeapAllocatorMethodKind::Normal;
};

bool IsHeapAllocatorBuiltinTypePath(const syntax::TypePath& path);
bool IsHeapAllocatorClassPath(const syntax::ClassPath& path);

std::optional<HeapAllocatorMethodSig> LookupHeapAllocatorMethodSig(
    std::string_view name);

syntax::EnumDecl BuildAllocationErrorEnumDecl();

}  // namespace cursive0::sema

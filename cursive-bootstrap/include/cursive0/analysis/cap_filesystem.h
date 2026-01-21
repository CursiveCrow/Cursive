#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

struct FileSystemMethodSig {
  Permission recv_perm;
  std::vector<syntax::Param> params;
  TypeRef ret;
};

bool IsFileSystemBuiltinTypePath(const syntax::TypePath& path);
bool IsFileSystemClassPath(const syntax::ClassPath& path);

std::optional<FileSystemMethodSig> LookupFileSystemMethodSig(
    std::string_view name);

syntax::ModalDecl BuildFileModalDecl();
syntax::ModalDecl BuildDirIterModalDecl();
syntax::RecordDecl BuildDirEntryRecordDecl();
syntax::EnumDecl BuildFileKindEnumDecl();
syntax::EnumDecl BuildIoErrorEnumDecl();

}  // namespace cursive0::sema

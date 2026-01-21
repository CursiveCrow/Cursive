#pragma once

#include <optional>
#include <string_view>

#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::sema {

bool IsStringBytesBuiltinPath(const syntax::ModulePath& path);
bool IsStringBuiltinName(std::string_view name);
bool IsBytesBuiltinName(std::string_view name);

std::optional<TypeRef> LookupStringBytesBuiltinType(
    const syntax::ModulePath& path,
    std::string_view name);

}  // namespace cursive0::sema

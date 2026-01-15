#pragma once

#include <string_view>

namespace cursive0::project {

bool IsIdentifier(std::string_view ident);
bool IsKeyword(std::string_view ident);
bool IsName(std::string_view ident);

}  // namespace cursive0::project

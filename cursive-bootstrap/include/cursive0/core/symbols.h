#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace cursive0::core {

std::string StringOfPath(const std::vector<std::string>& comps);
std::string StringOfPath(std::initializer_list<std::string_view> comps);

std::string PathToPrefix(std::string_view s);
std::string Mangle(std::string_view s);
std::string PathSig(std::initializer_list<std::string_view> comps);
std::string MangleModulePath(std::string_view module_path);

}  // namespace cursive0::core

#include "cursive0/project/ident.h"

#include <cstdint>
#include <vector>

#include "cursive0/core/keywords.h"
#include "cursive0/core/source_text.h"
#include "cursive0/core/unicode.h"

namespace cursive0::project {

namespace {

std::vector<std::uint8_t> ToBytes(std::string_view ident) {
  return std::vector<std::uint8_t>(ident.begin(), ident.end());
}

}  // namespace

bool IsKeyword(std::string_view ident) {
  return core::IsKeyword(ident);
}

bool IsIdentifier(std::string_view ident) {
  if (ident.empty()) {
    return false;
  }
  const core::DecodeResult decoded = core::Decode(ToBytes(ident));
  if (!decoded.ok || decoded.scalars.empty()) {
    return false;
  }
  if (!core::IsIdentStart(decoded.scalars[0])) {
    return false;
  }
  for (std::size_t i = 1; i < decoded.scalars.size(); ++i) {
    if (!core::IsIdentContinue(decoded.scalars[i])) {
      return false;
    }
  }
  return true;
}

bool IsName(std::string_view ident) {
  return IsIdentifier(ident) && !IsKeyword(ident);
}

}  // namespace cursive0::project

#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/composite/enums.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::analysis::EnumDiscriminants;
using cursive0::syntax::EnumDecl;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::VariantDecl;
using cursive0::syntax::Visibility;

Token MakeToken(TokenKind kind, std::string_view lexeme) {
  Token tok;
  tok.kind = kind;
  tok.lexeme = std::string(lexeme);
  tok.span = {};
  return tok;
}

VariantDecl MakeVariant(std::string_view name,
                        std::optional<Token> disc_opt) {
  VariantDecl var{};
  var.name = std::string(name);
  var.payload_opt = std::nullopt;
  var.discriminant_opt = std::move(disc_opt);
  var.span = {};
  var.doc_opt = std::nullopt;
  return var;
}

EnumDecl MakeEnum(std::vector<VariantDecl> variants) {
  EnumDecl decl{};
  decl.vis = Visibility::Public;
  decl.name = "E";
  decl.implements = {};
  decl.variants = std::move(variants);
  decl.span = {};
  decl.doc = {};
  return decl;
}

}  // namespace

int main() {
  {
    SPEC_COV("Enum-Disc-NotInt");
    auto decl = MakeEnum({MakeVariant(
        "A", MakeToken(TokenKind::FloatLiteral, "1.0"))});
    const auto result = EnumDiscriminants(decl);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Enum-Disc-NotInt"));
  }

  {
    SPEC_COV("Enum-Disc-Negative");
    auto decl = MakeEnum({MakeVariant(
        "A", MakeToken(TokenKind::IntLiteral, "-1"))});
    const auto result = EnumDiscriminants(decl);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Enum-Disc-Negative"));
  }

  {
    SPEC_COV("Enum-Disc-Invalid");
    auto decl = MakeEnum({MakeVariant(
        "A", MakeToken(TokenKind::IntLiteral, "18446744073709551616"))});
    const auto result = EnumDiscriminants(decl);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Enum-Disc-Invalid"));
  }

  {
    SPEC_COV("Enum-Disc-Dup");
    auto decl = MakeEnum({
        MakeVariant("A", MakeToken(TokenKind::IntLiteral, "1")),
        MakeVariant("B", std::nullopt),
        MakeVariant("C", MakeToken(TokenKind::IntLiteral, "2")),
    });
    const auto result = EnumDiscriminants(decl);
    assert(!result.ok);
    assert(result.diag_id ==
           std::optional<std::string_view>("Enum-Disc-Dup"));
  }

  return 0;
}

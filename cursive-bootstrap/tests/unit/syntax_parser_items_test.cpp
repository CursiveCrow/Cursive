#include <cassert>
#include <string>
#include <vector>
#include <variant>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/source_text.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/lexer.h"
#include "cursive0/syntax/parser.h"

namespace {

using cursive0::core::LineStarts;
using cursive0::core::SourceFile;
using cursive0::core::UnicodeScalar;
using cursive0::syntax::ASTFile;
using cursive0::syntax::ASTItem;
using cursive0::syntax::Binding;
using cursive0::syntax::ClassDecl;
using cursive0::syntax::ClassFieldDecl;
using cursive0::syntax::ClassItem;
using cursive0::syntax::ClassMethodDecl;
using cursive0::syntax::EnumDecl;
using cursive0::syntax::FieldDecl;
using cursive0::syntax::ModalDecl;
using cursive0::syntax::MethodDecl;
using cursive0::syntax::Mutability;
using cursive0::syntax::Param;
using cursive0::syntax::ParamMode;
using cursive0::syntax::ParseFile;
using cursive0::syntax::MakeParser;
using cursive0::syntax::Parser;
using cursive0::syntax::Receiver;
using cursive0::syntax::ReceiverPerm;
using cursive0::syntax::ReceiverShorthand;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::RecordMember;
using cursive0::syntax::StateBlock;
using cursive0::syntax::StateFieldDecl;
using cursive0::syntax::StateMember;
using cursive0::syntax::StateMethodDecl;
using cursive0::syntax::StaticDecl;
using cursive0::syntax::TransitionDecl;
using cursive0::syntax::TypeAliasDecl;
using cursive0::syntax::UsingDecl;
using cursive0::syntax::UsingList;
using cursive0::syntax::UsingPath;
using cursive0::syntax::Visibility;
using cursive0::syntax::VariantDecl;
using cursive0::syntax::VariantPayload;
using cursive0::syntax::VariantPayloadRecord;
using cursive0::syntax::VariantPayloadTuple;
using cursive0::syntax::SyncItem;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;

static std::vector<UnicodeScalar> AsScalars(const std::string& text) {
  std::vector<UnicodeScalar> out;
  out.reserve(text.size());
  for (unsigned char c : text) {
    out.push_back(static_cast<UnicodeScalar>(c));
  }
  return out;
}

static SourceFile MakeSourceFile(const std::string& path,
                                 const std::string& text) {
  SourceFile s;
  s.path = path;
  s.scalars = AsScalars(text);
  s.text = cursive0::core::EncodeUtf8(s.scalars);
  s.bytes.assign(s.text.begin(), s.text.end());
  s.byte_len = s.text.size();
  s.line_starts = LineStarts(s.scalars);
  s.line_count = s.line_starts.size();
  return s;
}

static Token MakeToken(TokenKind kind, const char* lexeme) {
  Token tok;
  tok.kind = kind;
  tok.lexeme = std::string(lexeme);
  return tok;
}

}  // namespace

int main() {
  SPEC_COV("Parse-AliasOpt-None");
  SPEC_COV("Parse-AliasOpt-Yes");
  SPEC_COV("Parse-Attribute-Unsupported");
  SPEC_COV("Parse-BindingAfterLetVar");
  SPEC_COV("Parse-Class");
  SPEC_COV("Parse-ClassBody");
  SPEC_COV("Parse-ClassItem-Field");
  SPEC_COV("Parse-ClassItem-Method");
  SPEC_COV("Parse-ClassItemList-Cons");
  SPEC_COV("Parse-ClassItemList-End");
  SPEC_COV("Parse-ClassList-Cons");
  SPEC_COV("Parse-ClassListTail-Comma");
  SPEC_COV("Parse-ClassListTail-End");
  SPEC_COV("Parse-ClassMethodBody-Abstract");
  SPEC_COV("Parse-ClassMethodBody-Concrete");
  SPEC_COV("ConsumeTerminatorReq-Yes");
  SPEC_COV("ConsumeTerminatorReq-No");
  SPEC_COV("Parse-ClassPath");
  SPEC_COV("Parse-Enum");
  SPEC_COV("Parse-EnumBody");
  SPEC_COV("Parse-Extern-Unsupported");
  SPEC_COV("Parse-FieldDecl");
  SPEC_COV("Parse-FieldDeclList-Cons");
  SPEC_COV("Parse-FieldDeclList-Empty");
  SPEC_COV("Parse-FieldDeclTail-Comma");
  SPEC_COV("Parse-FieldDeclTail-End");
  SPEC_COV("Parse-FieldDeclTail-TrailingComma");
  SPEC_COV("Parse-Generic-Header-Unsupported");
  SPEC_COV("Parse-Ident");
  SPEC_COV("Parse-Ident-Err");
  SPEC_COV("Parse-Implements-None");
  SPEC_COV("Parse-Implements-Yes");
  SPEC_COV("Parse-Import-Unsupported");
  SPEC_COV("Parse-Item-Err");
  SPEC_COV("Parse-MethodDefAfterVis");
  SPEC_COV("Parse-MethodParams-Comma");
  SPEC_COV("Parse-MethodParams-None");
  SPEC_COV("Parse-MethodSignature");
  SPEC_COV("Parse-Modal");
  SPEC_COV("Parse-Modal-Class-Unsupported");
  SPEC_COV("Parse-ModalBody");
  SPEC_COV("Parse-ModulePath");
  SPEC_COV("Parse-ModulePathTail-Cons");
  SPEC_COV("Parse-ModulePathTail-End");
  SPEC_COV("Parse-Override-No");
  SPEC_COV("Parse-Override-Yes");
  SPEC_COV("Parse-Param");
  SPEC_COV("Parse-ParamList-Cons");
  SPEC_COV("Parse-ParamList-Empty");
  SPEC_COV("Parse-ParamMode-Move");
  SPEC_COV("Parse-ParamMode-None");
  SPEC_COV("Parse-ParamTail-Comma");
  SPEC_COV("Parse-ParamTail-End");
  SPEC_COV("Parse-ParamTail-TrailingComma");
  SPEC_COV("Parse-Procedure");
  SPEC_COV("Parse-QualifiedHead");
  SPEC_COV("Parse-Receiver-Explicit");
  SPEC_COV("Parse-Receiver-Short-Const");
  SPEC_COV("Parse-Receiver-Short-Unique");
  SPEC_COV("Parse-Record");
  SPEC_COV("Parse-RecordBody");
  SPEC_COV("Parse-RecordFieldDecl");
  SPEC_COV("Parse-RecordFieldDeclAfterVis");
  SPEC_COV("Parse-RecordFieldDeclList-Cons");
  SPEC_COV("Parse-RecordFieldDeclList-Empty");
  SPEC_COV("Parse-RecordFieldDeclTail-Comma");
  SPEC_COV("Parse-RecordFieldDeclTail-End");
  SPEC_COV("Parse-RecordFieldDeclTail-TrailingComma");
  SPEC_COV("Parse-RecordFieldInitOpt-None");
  SPEC_COV("Parse-RecordFieldInitOpt-Yes");
  SPEC_COV("Parse-RecordMember-Field");
  SPEC_COV("Parse-RecordMember-Method");
  SPEC_COV("Parse-RecordMemberList-Cons");
  SPEC_COV("Parse-RecordMemberList-End");
  SPEC_COV("Parse-RecordMemberTail-Comma");
  SPEC_COV("Parse-RecordMemberTail-End");
  SPEC_COV("Parse-RecordMemberTail-TrailingComma");
  SPEC_COV("Parse-ReturnOpt-Arrow");
  SPEC_COV("Parse-ReturnOpt-None");
  SPEC_COV("Parse-ShadowBinding");
  SPEC_COV("Parse-Signature");
  SPEC_COV("Parse-StateBlock");
  SPEC_COV("Parse-StateBlockList-Cons");
  SPEC_COV("Parse-StateBlockList-Empty");
  SPEC_COV("Parse-StateMember-Field");
  SPEC_COV("Parse-StateMember-Method");
  SPEC_COV("Parse-StateMember-Transition");
  SPEC_COV("Parse-StateMemberList-Cons");
  SPEC_COV("Parse-StateMemberList-End");
  SPEC_COV("Parse-Static-Decl");
  SPEC_COV("Parse-Superclass-None");
  SPEC_COV("Parse-Superclass-Yes");
  SPEC_COV("Parse-SuperclassBounds-Cons");
  SPEC_COV("Parse-SuperclassBoundsTail-End");
  SPEC_COV("Parse-SuperclassBoundsTail-Plus");
  SPEC_COV("Parse-Type-Alias");
  SPEC_COV("Parse-TypeAnnotOpt-None");
  SPEC_COV("Parse-TypeAnnotOpt-Yes");
  SPEC_COV("Parse-Type-Generic-Unsupported");
  SPEC_COV("Parse-TypeList-Cons");
  SPEC_COV("Parse-TypeList-Empty");
  SPEC_COV("Parse-TypeListTail-Comma");
  SPEC_COV("Parse-TypeListTail-End");
  SPEC_COV("Parse-TypeListTail-TrailingComma");
  SPEC_COV("Parse-TypePath");
  SPEC_COV("Parse-TypePathTail-Cons");
  SPEC_COV("Parse-TypePathTail-End");
  SPEC_COV("Parse-Qualified-Generic-Unsupported");
  SPEC_COV("Parse-Comptime-Unsupported");
  SPEC_COV("Parse-Use-Unsupported");
  SPEC_COV("Parse-Using-List");
  SPEC_COV("Parse-Using-Path");
  SPEC_COV("Parse-UsingList-Cons");
  SPEC_COV("Parse-UsingList-Empty");
  SPEC_COV("Parse-UsingListTail-Comma");
  SPEC_COV("Parse-UsingListTail-End");
  SPEC_COV("Parse-UsingListTail-TrailingComma");
  SPEC_COV("Parse-UsingSpec");
  SPEC_COV("Parse-Variant");
  SPEC_COV("Parse-VariantDiscriminantOpt-None");
  SPEC_COV("Parse-VariantDiscriminantOpt-Yes");
  SPEC_COV("Parse-VariantList-Cons");
  SPEC_COV("Parse-VariantList-Empty");
  SPEC_COV("Parse-VariantPayloadOpt-None");
  SPEC_COV("Parse-VariantPayloadOpt-Record");
  SPEC_COV("Parse-VariantPayloadOpt-Tuple");
  SPEC_COV("Parse-VariantTail-Comma");
  SPEC_COV("Parse-VariantTail-End");
  SPEC_COV("Parse-VariantTail-TrailingComma");
  SPEC_COV("Parse-Vis-Default");
  SPEC_COV("Parse-Vis-Opt");
  SPEC_COV("Sync-Item-Advance");
  SPEC_COV("Sync-Item-Stop");
  SPEC_COV("Sync-Type-Stop");
  SPEC_COV("Sync-Type-Consume");
  SPEC_COV("Sync-Type-Advance");
  SPEC_COV("Return-At-Module-Err");

  {
    const SourceFile source = MakeSourceFile(
        "using_path.cursive", "using alpha::beta as gamma");
    auto result = ParseFile(source);
    assert(result.file.has_value());
    assert(result.diags.empty());
    const ASTFile& file = *result.file;
    assert(file.items.size() == 1);
    const auto& decl = std::get<UsingDecl>(file.items[0]);
    assert(decl.vis == Visibility::Internal);
    const auto* clause = std::get_if<UsingPath>(&decl.clause);
    assert(clause != nullptr);
    assert(clause->path.size() == 2);
    assert(clause->path[0] == "alpha");
    assert(clause->path[1] == "beta");
    assert(clause->alias_opt.has_value());
    assert(*clause->alias_opt == "gamma");
  }

  {
    const SourceFile source = MakeSourceFile(
        "using_list.cursive",
        "public using mod { Foo, Bar as Baz }");
    auto result = ParseFile(source);
    assert(result.file.has_value());
    assert(result.diags.empty());
    const ASTFile& file = *result.file;
    assert(file.items.size() == 1);
    const auto& decl = std::get<UsingDecl>(file.items[0]);
    assert(decl.vis == Visibility::Public);
    const auto* clause = std::get_if<UsingList>(&decl.clause);
    assert(clause != nullptr);
    assert(clause->module_path.size() == 1);
    assert(clause->module_path[0] == "mod");
    assert(clause->specs.size() == 2);
    assert(clause->specs[0].name == "Foo");
    assert(!clause->specs[0].alias_opt.has_value());
    assert(clause->specs[1].name == "Bar");
    assert(clause->specs[1].alias_opt.has_value());
    assert(*clause->specs[1].alias_opt == "Baz");
  }

  {
    const SourceFile source = MakeSourceFile(
        "static_decl.cursive", "private let x: T = y");
    auto result = ParseFile(source);
    assert(result.file.has_value());
    assert(result.diags.empty());
    const ASTFile& file = *result.file;
    assert(file.items.size() == 1);
    const auto& decl = std::get<StaticDecl>(file.items[0]);
    assert(decl.vis == Visibility::Private);
    assert(decl.mut == Mutability::Let);
    const Binding& bind = decl.binding;
    assert(bind.pat != nullptr);
    assert(bind.type_opt != nullptr);
    assert(bind.init != nullptr);
    assert(bind.op.lexeme == "=");
    const auto* ident = std::get_if<cursive0::syntax::IdentifierPattern>(&bind.pat->node);
    assert(ident != nullptr);
    assert(ident->name == "x");
  }

  {
    const SourceFile source = MakeSourceFile(
        "procedure_decl.cursive", "procedure f(move a: T) -> U {}");
    auto result = ParseFile(source);
    assert(result.file.has_value());
    assert(result.diags.empty());
    const ASTFile& file = *result.file;
    assert(file.items.size() == 1);
    const auto& decl = std::get<cursive0::syntax::ProcedureDecl>(file.items[0]);
    assert(decl.vis == Visibility::Internal);
    assert(decl.name == "f");
    assert(decl.params.size() == 1);
    assert(decl.params[0].mode.has_value());
    assert(decl.params[0].mode.value() == ParamMode::Move);
    assert(decl.return_type_opt != nullptr);
    assert(decl.body != nullptr);
  }

  {
    const SourceFile source = MakeSourceFile(
        "record_decl.cursive",
        "record R <: A, B { x: T, procedure m(~) {} }");
    auto result = ParseFile(source);
    assert(result.file.has_value());
    assert(result.diags.empty());
    const ASTFile& file = *result.file;
    assert(file.items.size() == 1);
    const auto& decl = std::get<RecordDecl>(file.items[0]);
    assert(decl.name == "R");
    assert(decl.implements.size() == 2);
    assert(decl.implements[0].size() == 1);
    assert(decl.implements[0][0] == "A");
    assert(decl.implements[1][0] == "B");
    assert(decl.members.size() == 2);
    assert(std::holds_alternative<FieldDecl>(decl.members[0]));
    assert(std::holds_alternative<MethodDecl>(decl.members[1]));
    const auto& method = std::get<MethodDecl>(decl.members[1]);
    assert(method.name == "m");
    const auto* recv = std::get_if<ReceiverShorthand>(&method.receiver);
    assert(recv != nullptr);
    assert(recv->perm == ReceiverPerm::Const);
  }

  {
    const SourceFile source = MakeSourceFile(
        "enum_decl.cursive",
        "enum E { A, B(T), C { x: T }, D = 1 }");
    auto result = ParseFile(source);
    assert(result.file.has_value());
    assert(result.diags.empty());
    const ASTFile& file = *result.file;
    assert(file.items.size() == 1);
    const auto& decl = std::get<EnumDecl>(file.items[0]);
    assert(decl.name == "E");
    assert(decl.variants.size() == 4);
    const VariantDecl& v0 = decl.variants[0];
    const VariantDecl& v1 = decl.variants[1];
    const VariantDecl& v2 = decl.variants[2];
    const VariantDecl& v3 = decl.variants[3];
    assert(v0.name == "A");
    assert(!v0.payload_opt.has_value());
    assert(v1.name == "B");
    assert(v1.payload_opt.has_value());
    assert(std::holds_alternative<VariantPayloadTuple>(*v1.payload_opt));
    const auto& tuple = std::get<VariantPayloadTuple>(*v1.payload_opt);
    assert(tuple.elements.size() == 1);
    assert(v2.name == "C");
    assert(v2.payload_opt.has_value());
    assert(std::holds_alternative<VariantPayloadRecord>(*v2.payload_opt));
    const auto& rec = std::get<VariantPayloadRecord>(*v2.payload_opt);
    assert(rec.fields.size() == 1);
    assert(v3.name == "D");
    assert(v3.discriminant_opt.has_value());
    assert(v3.discriminant_opt->lexeme == "1");
  }

  {
    const SourceFile source = MakeSourceFile(
        "modal_decl.cursive",
        "modal M { @S { field: T procedure sm(a: T) {} transition go(a: T) -> @S {} } }");
    auto result = ParseFile(source);
    assert(result.file.has_value());
    assert(result.diags.empty());
    const ASTFile& file = *result.file;
    assert(file.items.size() == 1);
    const auto& decl = std::get<ModalDecl>(file.items[0]);
    assert(decl.name == "M");
    assert(decl.states.size() == 1);
    const StateBlock& state = decl.states[0];
    assert(state.name == "S");
    assert(state.members.size() == 3);
    assert(std::holds_alternative<StateFieldDecl>(state.members[0]));
    assert(std::holds_alternative<StateMethodDecl>(state.members[1]));
    assert(std::holds_alternative<TransitionDecl>(state.members[2]));
    const auto& trans = std::get<TransitionDecl>(state.members[2]);
    assert(trans.target_state == "S");
  }

  {
    const SourceFile source = MakeSourceFile(
        "class_decl.cursive",
        "class C <: A + B { f: T; procedure m(~) {} procedure n(~) ; }");
    auto result = ParseFile(source);
    assert(result.file.has_value());
    assert(result.diags.empty());
    const ASTFile& file = *result.file;
    assert(file.items.size() == 1);
    const auto& decl = std::get<ClassDecl>(file.items[0]);
    assert(decl.name == "C");
    assert(decl.supers.size() == 2);
    assert(decl.items.size() == 3);
    assert(std::holds_alternative<ClassFieldDecl>(decl.items[0]));
    assert(std::holds_alternative<ClassMethodDecl>(decl.items[1]));
    assert(std::holds_alternative<ClassMethodDecl>(decl.items[2]));
    const auto& m0 = std::get<ClassMethodDecl>(decl.items[1]);
    const auto& m1 = std::get<ClassMethodDecl>(decl.items[2]);
    assert(m0.body_opt != nullptr);
    assert(m1.body_opt == nullptr);
  }

  {
    const SourceFile source = MakeSourceFile(
        "type_alias.cursive", "protected type Alias = T");
    auto result = ParseFile(source);
    assert(result.file.has_value());
    assert(result.diags.empty());
    const ASTFile& file = *result.file;
    assert(file.items.size() == 1);
    const auto& decl = std::get<TypeAliasDecl>(file.items[0]);
    assert(decl.vis == Visibility::Protected);
    assert(decl.name == "Alias");
    assert(decl.type != nullptr);
  }

  {
    const SourceFile source = MakeSourceFile(
        "class_decl_missing_term.cursive",
        "class C { f: T procedure m(~) ; }");
    auto result = ParseFile(source);
    assert(result.file.has_value());
    assert(!result.diags.empty());
  }

  {
    const SourceFile source = MakeSourceFile(
        "sync_item.cursive", "x procedure");
    std::vector<Token> tokens;
    tokens.push_back(MakeToken(TokenKind::Identifier, "x"));
    tokens.push_back(MakeToken(TokenKind::Keyword, "procedure"));
    Parser parser = MakeParser(tokens, source);
    SyncItem(parser);
    assert(parser.index == 1);
  }

  return 0;
}

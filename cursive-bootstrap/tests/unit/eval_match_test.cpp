#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/eval/match.h"
#include "cursive0/eval/value.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"

using cursive0::eval::BindEnv;
using cursive0::eval::MatchPattern;
using cursive0::eval::MatchRecord;
using cursive0::eval::SemanticsContext;
using cursive0::eval::Value;
using cursive0::eval::ValueEqual;

namespace {

using cursive0::syntax::Pattern;
using cursive0::syntax::PatternPtr;

static cursive0::syntax::Token MakeToken(cursive0::syntax::TokenKind kind,
                                         std::string lexeme) {
  cursive0::syntax::Token tok;
  tok.kind = kind;
  tok.lexeme = std::move(lexeme);
  return tok;
}

static PatternPtr MakePattern(cursive0::syntax::PatternNode node) {
  auto pattern = std::make_shared<Pattern>();
  pattern->node = std::move(node);
  return pattern;
}

static PatternPtr MakeLiteralPattern(cursive0::syntax::TokenKind kind,
                                     std::string lexeme) {
  cursive0::syntax::LiteralPattern lit;
  lit.literal = MakeToken(kind, std::move(lexeme));
  return MakePattern(lit);
}

static PatternPtr MakeIdentPattern(std::string name) {
  return MakePattern(cursive0::syntax::IdentifierPattern{std::move(name)});
}

static PatternPtr MakeWildcardPattern() {
  return MakePattern(cursive0::syntax::WildcardPattern{});
}

static PatternPtr MakeTuplePattern(std::vector<PatternPtr> elements) {
  cursive0::syntax::TuplePattern tuple;
  tuple.elements = std::move(elements);
  return MakePattern(std::move(tuple));
}

static cursive0::syntax::FieldPattern MakeField(std::string name,
                                                PatternPtr pat = nullptr) {
  cursive0::syntax::FieldPattern field;
  field.name = std::move(name);
  field.pattern_opt = std::move(pat);
  return field;
}

static PatternPtr MakeRecordPattern(std::vector<cursive0::syntax::FieldPattern> fields) {
  cursive0::syntax::RecordPattern rec;
  rec.path = {"Point"};
  rec.fields = std::move(fields);
  return MakePattern(std::move(rec));
}

static PatternPtr MakeEnumPatternUnit(cursive0::syntax::TypePath path,
                                      std::string name) {
  cursive0::syntax::EnumPattern pat;
  pat.path = std::move(path);
  pat.name = std::move(name);
  pat.payload_opt = std::nullopt;
  return MakePattern(std::move(pat));
}

static PatternPtr MakeEnumPatternTuple(cursive0::syntax::TypePath path,
                                       std::string name,
                                       std::vector<PatternPtr> elements) {
  cursive0::syntax::TuplePayloadPattern payload;
  payload.elements = std::move(elements);
  cursive0::syntax::EnumPattern pat;
  pat.path = std::move(path);
  pat.name = std::move(name);
  pat.payload_opt = cursive0::syntax::EnumPayloadPattern{std::move(payload)};
  return MakePattern(std::move(pat));
}

static PatternPtr MakeEnumPatternRecord(cursive0::syntax::TypePath path,
                                        std::string name,
                                        std::vector<cursive0::syntax::FieldPattern> fields) {
  cursive0::syntax::RecordPayloadPattern payload;
  payload.fields = std::move(fields);
  cursive0::syntax::EnumPattern pat;
  pat.path = std::move(path);
  pat.name = std::move(name);
  pat.payload_opt = cursive0::syntax::EnumPayloadPattern{std::move(payload)};
  return MakePattern(std::move(pat));
}

static PatternPtr MakeModalPattern(std::string state,
                                   std::optional<std::vector<cursive0::syntax::FieldPattern>> fields) {
  cursive0::syntax::ModalPattern pat;
  pat.state = std::move(state);
  if (fields.has_value()) {
    cursive0::syntax::ModalRecordPayload payload;
    payload.fields = std::move(*fields);
    pat.fields_opt = std::move(payload);
  }
  return MakePattern(std::move(pat));
}

static PatternPtr MakeRangePattern(cursive0::syntax::RangeKind kind,
                                   PatternPtr lo,
                                   PatternPtr hi) {
  cursive0::syntax::RangePattern range;
  range.kind = kind;
  range.lo = std::move(lo);
  range.hi = std::move(hi);
  return MakePattern(std::move(range));
}

static std::shared_ptr<cursive0::syntax::Type> MakePrimType(std::string name) {
  auto type = std::make_shared<cursive0::syntax::Type>();
  type->node = cursive0::syntax::TypePrim{std::move(name)};
  return type;
}

static Value MakeInt(std::string type, std::uint64_t value) {
  cursive0::eval::IntVal v;
  v.type = std::move(type);
  v.negative = false;
  v.magnitude = cursive0::core::UInt128FromU64(value);
  return Value{v};
}

static Value MakeTuple(std::vector<Value> elements) {
  cursive0::eval::TupleVal v;
  v.elements = std::move(elements);
  return Value{v};
}

static Value MakeRecord(std::vector<std::pair<std::string, Value>> fields) {
  cursive0::eval::RecordVal v;
  v.record_type = cursive0::analysis::MakeTypePath({"Point"});
  v.fields = std::move(fields);
  return Value{v};
}

static Value MakeEnumUnit(cursive0::analysis::TypePath path) {
  cursive0::eval::EnumVal v;
  v.path = std::move(path);
  return Value{v};
}

static Value MakeEnumTuple(cursive0::analysis::TypePath path,
                           std::vector<Value> elements) {
  cursive0::eval::EnumVal v;
  v.path = std::move(path);
  cursive0::eval::EnumPayloadTupleVal payload;
  payload.elements = std::move(elements);
  v.payload = std::move(payload);
  return Value{v};
}

static Value MakeEnumRecord(cursive0::analysis::TypePath path,
                            std::vector<std::pair<std::string, Value>> fields) {
  cursive0::eval::EnumVal v;
  v.path = std::move(path);
  cursive0::eval::EnumPayloadRecordVal payload;
  payload.fields = std::move(fields);
  v.payload = std::move(payload);
  return Value{v};
}

static Value MakeModal(std::string state, Value payload) {
  cursive0::eval::ModalVal v;
  v.state = std::move(state);
  v.payload = std::make_shared<Value>(std::move(payload));
  return Value{v};
}

static Value MakeUnionValue(cursive0::analysis::TypeRef member, Value payload) {
  cursive0::eval::UnionVal v;
  v.member = std::move(member);
  v.value = std::make_shared<Value>(std::move(payload));
  return Value{v};
}

}  // namespace

int main() {
  SPEC_COV("Match-Modal-Empty");
  SPEC_COV("Match-Modal-Record");
  SPEC_COV("Match-Wildcard");
  SPEC_COV("Match-Ident");
  SPEC_COV("Match-Typed");
  SPEC_COV("Match-Literal");
  SPEC_COV("Match-Tuple");
  SPEC_COV("Match-Record");
  SPEC_COV("Match-Enum-Unit");
  SPEC_COV("Match-Enum-Tuple");
  SPEC_COV("Match-Enum-Record");
  SPEC_COV("Match-Modal-General");
  SPEC_COV("Match-Modal-State");
  SPEC_COV("Match-Range");
  SPEC_COV("MatchRecord-Empty");
  SPEC_COV("MatchRecord-Cons-Implicit");
  SPEC_COV("MatchRecord-Cons-Explicit");

  SemanticsContext ctx;

  Value bool_value{cursive0::eval::BoolVal{true}};
  auto wild = MakeWildcardPattern();
  auto res = MatchPattern(ctx, *wild, bool_value);
  assert(res.has_value());
  assert(res->empty());

  auto ident = MakeIdentPattern("x");
  Value int_one = MakeInt("i32", 1);
  res = MatchPattern(ctx, *ident, int_one);
  assert(res.has_value());
  assert(res->size() == 1);
  assert(ValueEqual(res->at("x"), int_one));

  auto typed = MakePattern(cursive0::syntax::TypedPattern{"y", MakePrimType("i32")});
  Value union_value = MakeUnionValue(cursive0::analysis::MakeTypePrim("i32"), int_one);
  res = MatchPattern(ctx, *typed, union_value);
  assert(res.has_value());
  assert(res->size() == 1);
  assert(ValueEqual(res->at("y"), int_one));

  auto lit = MakeLiteralPattern(cursive0::syntax::TokenKind::IntLiteral, "42");
  Value int_42 = MakeInt("i32", 42);
  res = MatchPattern(ctx, *lit, int_42);
  assert(res.has_value());
  assert(res->empty());

  auto tuple_pat = MakeTuplePattern({MakeIdentPattern("a"),
                                     MakeLiteralPattern(cursive0::syntax::TokenKind::IntLiteral, "5")});
  Value tuple_val = MakeTuple({MakeInt("i32", 7), MakeInt("i32", 5)});
  res = MatchPattern(ctx, *tuple_pat, tuple_val);
  assert(res.has_value());
  assert(ValueEqual(res->at("a"), MakeInt("i32", 7)));

  auto record_val = MakeRecord({{"x", MakeInt("i32", 1)}, {"y", MakeInt("i32", 2)}});
  std::vector<cursive0::syntax::FieldPattern> empty_fields;
  auto rec_empty = MatchRecord(ctx, empty_fields, record_val);
  assert(rec_empty.has_value());
  assert(rec_empty->empty());

  std::vector<cursive0::syntax::FieldPattern> implicit_fields;
  implicit_fields.push_back(MakeField("x"));
  auto rec_implicit = MatchRecord(ctx, implicit_fields, record_val);
  assert(rec_implicit.has_value());
  assert(ValueEqual(rec_implicit->at("x"), MakeInt("i32", 1)));

  std::vector<cursive0::syntax::FieldPattern> explicit_fields;
  explicit_fields.push_back(MakeField("x",
                                      MakeLiteralPattern(cursive0::syntax::TokenKind::IntLiteral, "1")));
  auto rec_explicit = MatchRecord(ctx, explicit_fields, record_val);
  assert(rec_explicit.has_value());
  assert(rec_explicit->empty());

  auto record_pat = MakeRecordPattern({MakeField("x")});
  res = MatchPattern(ctx, *record_pat, record_val);
  assert(res.has_value());
  assert(ValueEqual(res->at("x"), MakeInt("i32", 1)));

  Value enum_unit = MakeEnumUnit({"Color", "Red"});
  auto enum_unit_pat = MakeEnumPatternUnit({"Color"}, "Red");
  res = MatchPattern(ctx, *enum_unit_pat, enum_unit);
  assert(res.has_value());
  assert(res->empty());

  Value enum_tuple = MakeEnumTuple({"Option", "Some"}, {MakeInt("i32", 9)});
  auto enum_tuple_pat =
      MakeEnumPatternTuple({"Option"}, "Some", {MakeIdentPattern("v")});
  res = MatchPattern(ctx, *enum_tuple_pat, enum_tuple);
  assert(res.has_value());
  assert(ValueEqual(res->at("v"), MakeInt("i32", 9)));

  Value enum_record = MakeEnumRecord({"Msg", "Data"}, {{"value", MakeInt("i32", 7)}});
  auto enum_record_pat =
      MakeEnumPatternRecord({"Msg"}, "Data", {MakeField("value")});
  res = MatchPattern(ctx, *enum_record_pat, enum_record);
  assert(res.has_value());
  assert(ValueEqual(res->at("value"), MakeInt("i32", 7)));

  Value modal_payload = MakeRecord({{"x", MakeInt("i32", 3)}});
  Value modal_val = MakeModal("Open", modal_payload);
  auto modal_empty = MakeModalPattern("Open", std::nullopt);
  res = MatchPattern(ctx, *modal_empty, modal_val);
  assert(res.has_value());
  assert(res->empty());

  auto modal_fields = MakeModalPattern("Open", std::vector<cursive0::syntax::FieldPattern>{MakeField("x")});
  res = MatchPattern(ctx, *modal_fields, modal_val);
  assert(res.has_value());
  assert(ValueEqual(res->at("x"), MakeInt("i32", 3)));

  res = MatchPattern(ctx, *modal_fields, modal_payload);
  assert(res.has_value());
  assert(ValueEqual(res->at("x"), MakeInt("i32", 3)));

  auto range_pat = MakeRangePattern(cursive0::syntax::RangeKind::Inclusive,
                                    MakeLiteralPattern(cursive0::syntax::TokenKind::IntLiteral, "1"),
                                    MakeLiteralPattern(cursive0::syntax::TokenKind::IntLiteral, "5"));
  res = MatchPattern(ctx, *range_pat, MakeInt("i32", 3));
  assert(res.has_value());
  assert(res->empty());

  return 0;
}

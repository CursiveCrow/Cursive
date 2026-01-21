#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/codegen/layout/layout.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/int128.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"

namespace {

using cursive0::codegen::AlignOf;
using cursive0::codegen::EncodeConst;
using cursive0::codegen::EnumLayoutOf;
using cursive0::codegen::LayoutOf;
using cursive0::codegen::ModalLayoutOf;
using cursive0::codegen::RangeLayoutOf;
using cursive0::codegen::RecordLayoutOf;
using cursive0::codegen::SizeOf;
using cursive0::codegen::UnionLayoutOf;
using cursive0::codegen::ValidValue;
using cursive0::codegen::ValueBits;
using cursive0::codegen::Value;
using cursive0::codegen::BoolVal;
using cursive0::codegen::BytesVal;
using cursive0::codegen::CharVal;
using cursive0::codegen::DynamicVal;
using cursive0::codegen::EnumPayloadTupleVal;
using cursive0::codegen::EnumVal;
using cursive0::codegen::IntVal;
using cursive0::codegen::ModalVal;
using cursive0::codegen::PtrVal;
using cursive0::codegen::RawPtrVal;
using cursive0::codegen::ValueRangeKind;
using cursive0::codegen::ValueRangeVal;
using cursive0::codegen::RecordVal;
using cursive0::codegen::SliceVal;
using cursive0::codegen::StringVal;
using cursive0::codegen::TupleVal;
using cursive0::codegen::UnionVal;
using cursive0::analysis::MakeTypeArray;
using cursive0::analysis::MakeTypeFunc;
using cursive0::analysis::MakeTypeModalState;
using cursive0::analysis::MakeTypePath;
using cursive0::analysis::MakeTypePerm;
using cursive0::analysis::MakeTypePrim;
using cursive0::analysis::MakeTypePtr;
using cursive0::analysis::MakeTypeRange;
using cursive0::analysis::MakeTypeRawPtr;
using cursive0::analysis::MakeTypeSlice;
using cursive0::analysis::MakeTypeString;
using cursive0::analysis::MakeTypeBytes;
using cursive0::analysis::MakeTypeTuple;
using cursive0::analysis::MakeTypeUnion;
using cursive0::analysis::MakeTypeDynamic;
using cursive0::analysis::ParamMode;
using cursive0::analysis::PathKeyOf;
using cursive0::analysis::Permission;
using cursive0::analysis::PtrState;
using cursive0::analysis::RawPtrQual;
using cursive0::analysis::ScopeContext;
using cursive0::analysis::StringState;
using cursive0::analysis::BytesState;
using cursive0::analysis::TypeFuncParam;
using cursive0::analysis::TypeUnion;
using cursive0::syntax::EnumDecl;
using cursive0::syntax::FieldDecl;
using cursive0::syntax::ModalDecl;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::StateBlock;
using cursive0::syntax::StateFieldDecl;
using cursive0::syntax::Token;
using cursive0::syntax::TokenKind;
using cursive0::syntax::Type;
using cursive0::syntax::TypePrim;
using cursive0::syntax::TypePtr;
using cursive0::syntax::VariantDecl;
using cursive0::syntax::VariantPayloadTuple;
using cursive0::syntax::VariantPayloadRecord;
using cursive0::syntax::Visibility;

std::shared_ptr<Type> MakePrimType(const char* name) {
  auto type = std::make_shared<Type>();
  type->node = TypePrim{std::string(name)};
  return type;
}

std::shared_ptr<Type> MakePtrType(const char* name,
                                  cursive0::syntax::PtrState state) {
  auto type = std::make_shared<Type>();
  TypePtr ptr;
  ptr.element = MakePrimType(name);
  ptr.state = state;
  type->node = ptr;
  return type;
}

FieldDecl MakeField(const char* name, std::shared_ptr<Type> type) {
  FieldDecl field;
  field.vis = Visibility::Public;
  field.name = std::string(name);
  field.type = std::move(type);
  field.init_opt = nullptr;
  field.span = {};
  field.doc_opt = std::nullopt;
  return field;
}

StateFieldDecl MakeStateField(const char* name, std::shared_ptr<Type> type) {
  StateFieldDecl field;
  field.vis = Visibility::Public;
  field.name = std::string(name);
  field.type = std::move(type);
  field.span = {};
  field.doc_opt = std::nullopt;
  return field;
}

Token MakeToken(TokenKind kind, const char* lexeme) {
  Token tok;
  tok.kind = kind;
  tok.lexeme = std::string(lexeme);
  return tok;
}

}  // namespace

int main() {
  SPEC_COV("Size-Prim");
  SPEC_COV("Align-Prim");
  SPEC_COV("Layout-Prim");
  SPEC_COV("Encode-Bool");
  SPEC_COV("Encode-Char");
  SPEC_COV("Encode-Int");
  SPEC_COV("Encode-Float");
  SPEC_COV("Encode-Unit");
  SPEC_COV("Encode-Never");
  SPEC_COV("Encode-RawPtr-Null");
  SPEC_COV("Valid-Bool");
  SPEC_COV("Valid-Char");
  SPEC_COV("Valid-Scalar");
  SPEC_COV("Valid-Unit");
  SPEC_COV("Valid-Never");
  SPEC_COV("Layout-Perm");
  SPEC_COV("Size-Perm");
  SPEC_COV("Align-Perm");
  SPEC_COV("Size-Ptr");
  SPEC_COV("Align-Ptr");
  SPEC_COV("Layout-Ptr");
  SPEC_COV("Size-RawPtr");
  SPEC_COV("Align-RawPtr");
  SPEC_COV("Layout-RawPtr");
  SPEC_COV("Size-Func");
  SPEC_COV("Align-Func");
  SPEC_COV("Layout-Func");
  SPEC_COV("Layout-Record-Empty");
  SPEC_COV("Layout-Record-Cons");
  SPEC_COV("Size-Record");
  SPEC_COV("Align-Record");
  SPEC_COV("Layout-Record");
  SPEC_COV("Size-Alias");
  SPEC_COV("Align-Alias");
  SPEC_COV("Layout-Alias");
  SPEC_COV("Layout-Union-Niche");
  SPEC_COV("Layout-Union-Tagged");
  SPEC_COV("Size-Union");
  SPEC_COV("Align-Union");
  SPEC_COV("Layout-Union");
  SPEC_COV("Size-String-Managed");
  SPEC_COV("Align-String-Managed");
  SPEC_COV("Layout-String-Managed");
  SPEC_COV("Size-String-View");
  SPEC_COV("Align-String-View");
  SPEC_COV("Layout-String-View");
  SPEC_COV("Size-Bytes-Managed");
  SPEC_COV("Align-Bytes-Managed");
  SPEC_COV("Layout-Bytes-Managed");
  SPEC_COV("Size-Bytes-View");
  SPEC_COV("Align-Bytes-View");
  SPEC_COV("Layout-Bytes-View");
  SPEC_COV("Size-String-Modal");
  SPEC_COV("Align-String-Modal");
  SPEC_COV("Size-Bytes-Modal");
  SPEC_COV("Align-Bytes-Modal");
  SPEC_COV("Layout-Tuple-Empty");
  SPEC_COV("Layout-Tuple-Cons");
  SPEC_COV("Size-Tuple");
  SPEC_COV("Align-Tuple");
  SPEC_COV("Layout-Tuple");
  SPEC_COV("Size-Array");
  SPEC_COV("Align-Array");
  SPEC_COV("Layout-Array");
  SPEC_COV("Size-Slice");
  SPEC_COV("Align-Slice");
  SPEC_COV("Layout-Slice");
  SPEC_COV("Layout-Range");
  SPEC_COV("Size-Range");
  SPEC_COV("Align-Range");
  SPEC_COV("Layout-Range-SizeAlign");
  SPEC_COV("Layout-Enum-Tagged");
  SPEC_COV("Size-Enum");
  SPEC_COV("Align-Enum");
  SPEC_COV("Layout-Enum");
  SPEC_COV("Layout-Modal-Niche");
  SPEC_COV("Layout-Modal-Tagged");
  SPEC_COV("Size-Modal");
  SPEC_COV("Align-Modal");
  SPEC_COV("Layout-Modal");
  SPEC_COV("Size-ModalState");
  SPEC_COV("Align-ModalState");
  SPEC_COV("Layout-ModalState");
  SPEC_COV("Layout-DynamicClass");
  SPEC_COV("Size-DynamicClass");
  SPEC_COV("Align-DynamicClass");

  ScopeContext ctx;
  ctx.current_module = {"Test"};

  RecordDecl point;
  point.vis = Visibility::Public;
  point.name = "Point";
  point.implements = {};
  point.members = {MakeField("x", MakePrimType("i32")),
                   MakeField("y", MakePrimType("u8"))};
  point.span = {};
  point.doc = {};

  cursive0::syntax::TypeAliasDecl alias;
  alias.vis = Visibility::Public;
  alias.name = "AliasI64";
  alias.type = MakePrimType("i64");
  alias.span = {};
  alias.doc = {};

  VariantDecl var_a;
  var_a.name = "A";
  var_a.payload_opt = std::nullopt;
  var_a.discriminant_opt = std::nullopt;
  var_a.span = {};
  var_a.doc_opt = std::nullopt;

  VariantDecl var_b;
  var_b.name = "B";
  var_b.payload_opt = VariantPayloadTuple{std::vector<std::shared_ptr<Type>>{
      MakePrimType("u8")}};
  var_b.discriminant_opt = std::nullopt;
  var_b.span = {};
  var_b.doc_opt = std::nullopt;

  VariantDecl var_c;
  var_c.name = "C";
  VariantPayloadRecord rec_payload;
  rec_payload.fields.push_back(MakeField("payload", MakePrimType("u64")));
  var_c.payload_opt = rec_payload;
  var_c.discriminant_opt = std::nullopt;
  var_c.span = {};
  var_c.doc_opt = std::nullopt;

  EnumDecl enum_decl;
  enum_decl.vis = Visibility::Public;
  enum_decl.name = "E";
  enum_decl.implements = {};
  enum_decl.variants = {var_a, var_b, var_c};
  enum_decl.span = {};
  enum_decl.doc = {};

  StateBlock some_state;
  some_state.name = "Some";
  some_state.members = {MakeStateField("ptr", MakePtrType("u8", cursive0::syntax::PtrState::Valid))};
  some_state.span = {};
  some_state.doc_opt = std::nullopt;

  StateBlock none_state;
  none_state.name = "None";
  none_state.members = {};
  none_state.span = {};
  none_state.doc_opt = std::nullopt;

  ModalDecl opt_ptr;
  opt_ptr.vis = Visibility::Public;
  opt_ptr.name = "OptPtr";
  opt_ptr.implements = {};
  opt_ptr.states = {some_state, none_state};
  opt_ptr.span = {};
  opt_ptr.doc = {};

  StateBlock on_state;
  on_state.name = "On";
  on_state.members = {MakeStateField("x", MakePrimType("u8"))};
  on_state.span = {};
  on_state.doc_opt = std::nullopt;

  StateBlock off_state;
  off_state.name = "Off";
  off_state.members = {MakeStateField("y", MakePrimType("u64"))};
  off_state.span = {};
  off_state.doc_opt = std::nullopt;

  ModalDecl toggle;
  toggle.vis = Visibility::Public;
  toggle.name = "Toggle";
  toggle.implements = {};
  toggle.states = {on_state, off_state};
  toggle.span = {};
  toggle.doc = {};

  ctx.sigma.types[PathKeyOf({"Point"})] = point;
  ctx.sigma.types[PathKeyOf({"AliasI64"})] = alias;
  ctx.sigma.types[PathKeyOf({"E"})] = enum_decl;
  ctx.sigma.types[PathKeyOf({"OptPtr"})] = opt_ptr;
  ctx.sigma.types[PathKeyOf({"Toggle"})] = toggle;

  const auto i32 = MakeTypePrim("i32");
  const auto unit = MakeTypePrim("()");
  const auto perm_i32 = MakeTypePerm(Permission::Const, i32);

  assert(SizeOf(ctx, i32).value() == 4);
  assert(AlignOf(ctx, i32).value() == 4);
  auto layout = LayoutOf(ctx, i32);
  assert(layout.has_value() && layout->size == 4 && layout->align == 4);

  assert(SizeOf(ctx, perm_i32).value() == 4);
  assert(AlignOf(ctx, perm_i32).value() == 4);
  auto perm_layout = LayoutOf(ctx, perm_i32);
  assert(perm_layout.has_value() && perm_layout->size == 4 && perm_layout->align == 4);

  auto ptr = MakeTypePtr(MakeTypePrim("u8"), PtrState::Valid);
  assert(SizeOf(ctx, ptr).value() == 8);
  assert(AlignOf(ctx, ptr).value() == 8);

  auto raw = MakeTypeRawPtr(RawPtrQual::Imm, MakeTypePrim("u8"));
  assert(SizeOf(ctx, raw).value() == 8);
  assert(AlignOf(ctx, raw).value() == 8);

  auto func = MakeTypeFunc(std::vector<TypeFuncParam>{}, i32);
  assert(SizeOf(ctx, func).value() == 8);
  assert(AlignOf(ctx, func).value() == 8);

  auto record_layout = RecordLayoutOf(ctx, {i32, MakeTypePrim("u8")});
  assert(record_layout.has_value());
  assert(record_layout->layout.size == 8);
  assert(record_layout->layout.align == 4);
  assert(record_layout->offsets.size() == 2);
  assert(record_layout->offsets[0] == 0);
  assert(record_layout->offsets[1] == 4);

  auto empty_layout = RecordLayoutOf(ctx, {});
  assert(empty_layout.has_value());
  assert(empty_layout->layout.size == 0);
  assert(empty_layout->layout.align == 1);

  auto record_type = MakeTypePath({"Point"});
  auto record_size = SizeOf(ctx, record_type);
  assert(record_size.has_value() && *record_size == 8);
  auto record_align = AlignOf(ctx, record_type);
  assert(record_align.has_value() && *record_align == 4);
  auto record_layout_type = LayoutOf(ctx, record_type);
  assert(record_layout_type.has_value());
  assert(record_layout_type->size == 8);
  assert(record_layout_type->align == 4);

  auto alias_type = MakeTypePath({"AliasI64"});
  assert(SizeOf(ctx, alias_type).value() == 8);
  assert(AlignOf(ctx, alias_type).value() == 8);
  auto alias_layout = LayoutOf(ctx, alias_type);
  assert(alias_layout.has_value());
  assert(alias_layout->size == 8);
  assert(alias_layout->align == 8);

  auto range_layout = RangeLayoutOf(ctx);
  assert(range_layout.has_value());
  assert(range_layout->layout.size == 24);
  assert(range_layout->layout.align == 8);
  auto range_type = MakeTypeRange();
  assert(SizeOf(ctx, range_type).value() == 24);
  assert(AlignOf(ctx, range_type).value() == 8);
  auto range_layout_type = LayoutOf(ctx, range_type);
  assert(range_layout_type.has_value());
  assert(range_layout_type->size == 24);
  assert(range_layout_type->align == 8);

  auto tuple_empty = MakeTypeTuple({});
  auto tuple_two = MakeTypeTuple({MakeTypePrim("u8"), MakeTypePrim("u32")});
  assert(LayoutOf(ctx, tuple_empty).has_value());
  auto tuple_size = SizeOf(ctx, tuple_two);
  assert(tuple_size.has_value() && *tuple_size == 8);
  auto tuple_align = AlignOf(ctx, tuple_two);
  assert(tuple_align.has_value() && *tuple_align == 4);
  auto tuple_layout = LayoutOf(ctx, tuple_two);
  assert(tuple_layout.has_value());
  assert(tuple_layout->size == 8);
  assert(tuple_layout->align == 4);

  auto array_type = MakeTypeArray(MakeTypePrim("u16"), 3);
  assert(SizeOf(ctx, array_type).value() == 6);
  assert(AlignOf(ctx, array_type).value() == 2);
  auto array_layout = LayoutOf(ctx, array_type);
  assert(array_layout.has_value());
  assert(array_layout->size == 6);
  assert(array_layout->align == 2);

  auto slice_type = MakeTypeSlice(MakeTypePrim("u8"));
  assert(SizeOf(ctx, slice_type).value() == 16);
  assert(AlignOf(ctx, slice_type).value() == 8);
  auto slice_layout = LayoutOf(ctx, slice_type);
  assert(slice_layout.has_value());
  assert(slice_layout->size == 16);
  assert(slice_layout->align == 8);

  auto union_niche = MakeTypeUnion({ptr, unit});
  const auto* union_niche_node = std::get_if<TypeUnion>(&union_niche->node);
  assert(union_niche_node);
  const auto union_niche_layout = UnionLayoutOf(ctx, *union_niche_node);
  assert(union_niche_layout.has_value());
  assert(union_niche_layout->niche);
  assert(union_niche_layout->layout.size == 8);

  auto union_tagged = MakeTypeUnion({MakeTypePrim("i32"), MakeTypePrim("bool")});
  const auto* union_tagged_node = std::get_if<TypeUnion>(&union_tagged->node);
  assert(union_tagged_node);
  const auto union_tagged_layout = UnionLayoutOf(ctx, *union_tagged_node);
  assert(union_tagged_layout.has_value());
  assert(!union_tagged_layout->niche);
  assert(union_tagged_layout->layout.size == 8);
  assert(union_tagged_layout->layout.align == 4);
  auto union_layout_type = LayoutOf(ctx, union_tagged);
  assert(union_layout_type.has_value());
  assert(union_layout_type->size == 8);
  assert(union_layout_type->align == 4);

  UnionVal union_tagged_bool;
  union_tagged_bool.member = MakeTypePrim("bool");
  union_tagged_bool.value = std::make_shared<Value>(Value{BoolVal{true}});
  const auto union_tagged_bits = ValueBits(ctx, union_tagged, Value{union_tagged_bool});
  assert(union_tagged_bits.has_value());
  auto union_tagged_mut = *union_tagged_bits;
  if (union_tagged_mut.size() > 1) {
    union_tagged_mut[1] = 0xAB;
  }
  assert(ValidValue(ctx, union_tagged, union_tagged_mut));

  assert(SizeOf(ctx, MakeTypeString(StringState::Managed)).value() == 24);
  assert(AlignOf(ctx, MakeTypeString(StringState::Managed)).value() == 8);
  auto str_layout = LayoutOf(ctx, MakeTypeString(StringState::Managed));
  assert(str_layout.has_value());
  assert(str_layout->size == 24);
  assert(str_layout->align == 8);
  assert(SizeOf(ctx, MakeTypeString(StringState::View)).value() == 16);
  assert(AlignOf(ctx, MakeTypeString(StringState::View)).value() == 8);
  assert(LayoutOf(ctx, MakeTypeString(StringState::View)).has_value());
  assert(SizeOf(ctx, MakeTypeString(std::nullopt)).value() == 32);
  assert(AlignOf(ctx, MakeTypeString(std::nullopt)).value() == 8);

  assert(SizeOf(ctx, MakeTypeBytes(BytesState::Managed)).value() == 24);
  assert(AlignOf(ctx, MakeTypeBytes(BytesState::Managed)).value() == 8);
  auto bytes_layout = LayoutOf(ctx, MakeTypeBytes(BytesState::Managed));
  assert(bytes_layout.has_value());
  assert(bytes_layout->size == 24);
  assert(bytes_layout->align == 8);
  assert(SizeOf(ctx, MakeTypeBytes(BytesState::View)).value() == 16);
  assert(AlignOf(ctx, MakeTypeBytes(BytesState::View)).value() == 8);
  assert(LayoutOf(ctx, MakeTypeBytes(BytesState::View)).has_value());
  assert(SizeOf(ctx, MakeTypeBytes(std::nullopt)).value() == 32);
  assert(AlignOf(ctx, MakeTypeBytes(std::nullopt)).value() == 8);

  const auto enum_layout = EnumLayoutOf(ctx, enum_decl);
  assert(enum_layout.has_value());
  assert(enum_layout->layout.size == 16);
  assert(enum_layout->layout.align == 8);
  auto enum_type = MakeTypePath({"E"});
  assert(SizeOf(ctx, enum_type).value() == 16);
  assert(AlignOf(ctx, enum_type).value() == 8);
  auto enum_layout_type = LayoutOf(ctx, enum_type);
  assert(enum_layout_type.has_value());
  assert(enum_layout_type->size == 16);
  assert(enum_layout_type->align == 8);

  const auto modal_niche_layout = ModalLayoutOf(ctx, opt_ptr);
  assert(modal_niche_layout.has_value());
  assert(modal_niche_layout->niche);
  assert(modal_niche_layout->layout.size == 8);
  assert(modal_niche_layout->layout.align == 8);
  auto modal_layout_type = LayoutOf(ctx, MakeTypePath({"OptPtr"}));
  assert(modal_layout_type.has_value());
  assert(modal_layout_type->size == 8);
  assert(modal_layout_type->align == 8);

  const auto modal_tagged_layout = ModalLayoutOf(ctx, toggle);
  assert(modal_tagged_layout.has_value());
  assert(!modal_tagged_layout->niche);
  assert(modal_tagged_layout->layout.size == 16);
  assert(modal_tagged_layout->layout.align == 8);

  auto modal_state_type = MakeTypeModalState({"OptPtr"}, "Some");
  assert(SizeOf(ctx, modal_state_type).value() == 8);
  assert(AlignOf(ctx, modal_state_type).value() == 8);

  auto dyn_type = MakeTypeDynamic({"Widget"});
  assert(SizeOf(ctx, dyn_type).value() == 16);
  assert(AlignOf(ctx, dyn_type).value() == 8);
  auto dyn_layout = LayoutOf(ctx, dyn_type);
  assert(dyn_layout.has_value());
  assert(dyn_layout->size == 16);
  assert(dyn_layout->align == 8);

  const auto bool_bits = EncodeConst(MakeTypePrim("bool"),
                                     MakeToken(TokenKind::BoolLiteral, "true"));
  assert(bool_bits.has_value());
  assert(bool_bits->size() == 1 && (*bool_bits)[0] == 1);

  const auto char_bits = EncodeConst(MakeTypePrim("char"),
                                     MakeToken(TokenKind::CharLiteral, "'A'"));
  assert(char_bits.has_value());
  assert(char_bits->size() == 4 && (*char_bits)[0] == 65);

  const auto char_hex_bits = EncodeConst(MakeTypePrim("char"),
                                      MakeToken(TokenKind::CharLiteral, "'\\x41'"));
  assert(char_hex_bits.has_value());
  assert(*char_hex_bits == std::vector<std::uint8_t>({0x41, 0x00, 0x00, 0x00}));

  const auto char_uni_bits = EncodeConst(MakeTypePrim("char"),
                                      MakeToken(TokenKind::CharLiteral, "'\\u{1F600}'"));
  assert(char_uni_bits.has_value());
  assert(*char_uni_bits == std::vector<std::uint8_t>({0x00, 0xF6, 0x01, 0x00}));

  const auto int_bits = EncodeConst(MakeTypePrim("u8"),
                                    MakeToken(TokenKind::IntLiteral, "42"));
  assert(int_bits.has_value());
  assert(int_bits->size() == 1 && (*int_bits)[0] == 42);

  const auto float_bits = EncodeConst(MakeTypePrim("f32"),
                                      MakeToken(TokenKind::FloatLiteral, "1.0f32"));
  assert(float_bits.has_value());
  assert(float_bits->size() == 4);

  const auto unit_bits = EncodeConst(MakeTypePrim("()"),
                                     MakeToken(TokenKind::Unknown, ""));
  assert(unit_bits.has_value() && unit_bits->empty());

  const auto never_bits = EncodeConst(MakeTypePrim("!"),
                                      MakeToken(TokenKind::Unknown, ""));
  assert(never_bits.has_value() && never_bits->empty());

  const auto null_bits = EncodeConst(MakeTypeRawPtr(RawPtrQual::Imm, MakeTypePrim("u8")),
                                     MakeToken(TokenKind::NullLiteral, "null"));
  assert(null_bits.has_value() && null_bits->size() == 8);

  assert(ValidValue(ctx, MakeTypePrim("bool"), {0x00}));
  assert(ValidValue(ctx, MakeTypePrim("bool"), {0x01}));
  assert(!ValidValue(ctx, MakeTypePrim("bool"), {0x02}));

  assert(ValidValue(ctx, MakeTypePrim("char"), {0x41, 0x00, 0x00, 0x00}));
  assert(!ValidValue(ctx, MakeTypePrim("char"), {0x00, 0xD8, 0x00, 0x00}));

  assert(ValidValue(ctx, MakeTypePrim("i32"), {0x00, 0x00, 0x00, 0x00}));
  assert(!ValidValue(ctx, MakeTypePrim("i32"), {0x00, 0x00, 0x00}));

  assert(ValidValue(ctx, MakeTypePrim("()"), {}));
  assert(!ValidValue(ctx, MakeTypePrim("!"), {}));

  assert(!ValidValue(ctx, ptr, std::vector<std::uint8_t>(8, 0)));
  assert(ValidValue(ctx, ptr, {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}));

  RecordVal point_val;
  point_val.fields.push_back({"x", Value{IntVal{"i32", cursive0::core::UInt128FromU64(1)}}});
  point_val.fields.push_back({"y", Value{IntVal{"u8", cursive0::core::UInt128FromU64(2)}}});
  const auto point_bits = ValueBits(ctx, record_type, Value{point_val});
  assert(point_bits.has_value());
  assert(point_bits->size() == 8);
  assert((*point_bits)[0] == 1);
  assert((*point_bits)[4] == 2);
  assert(ValidValue(ctx, record_type, *point_bits));
  auto bad_point_bits = *point_bits;
  bad_point_bits[5] = 0xFF;
  assert(!ValidValue(ctx, record_type, bad_point_bits));


  ValueRangeVal range_val;
  range_val.kind = ValueRangeKind::ToInclusive;
  range_val.lo = 1;
  range_val.hi = 2;
  const auto range_bits = ValueBits(ctx, range_type, Value{range_val});
  assert(range_bits.has_value());
  assert(range_bits->size() == 24);
  assert((*range_bits)[0] == 1);
  assert(ValidValue(ctx, range_type, *range_bits));
  auto bad_range_bits = *range_bits;
  bad_range_bits[0] = 6;
  assert(!ValidValue(ctx, range_type, bad_range_bits));


  EnumVal enum_val;
  enum_val.variant = "B";
  EnumPayloadTupleVal payload_tuple;
  payload_tuple.elements.push_back(Value{IntVal{"u8", cursive0::core::UInt128FromU64(7)}});
  enum_val.payload = payload_tuple;
  const auto enum_bits = ValueBits(ctx, enum_type, Value{enum_val});
  assert(enum_bits.has_value());
  assert(enum_bits->size() == 16);
  assert((*enum_bits)[0] == 1);
  assert((*enum_bits)[8] == 7);
  assert(ValidValue(ctx, enum_type, *enum_bits));
  auto bad_enum_bits = *enum_bits;
  bad_enum_bits[0] = 3;
  assert(!ValidValue(ctx, enum_type, bad_enum_bits));


  UnionVal union_payload;
  union_payload.member = ptr;
  union_payload.value = std::make_shared<Value>(Value{PtrVal{PtrState::Valid, 0x1000}});
  const auto union_bits = ValueBits(ctx, union_niche, Value{union_payload});
  assert(union_bits.has_value());
  assert(union_bits->size() == 8);
  assert(ValidValue(ctx, union_niche, *union_bits));


  UnionVal union_empty;
  union_empty.member = unit;
  union_empty.value = std::make_shared<Value>(Value{cursive0::codegen::UnitVal{}});
  const auto union_empty_bits = ValueBits(ctx, union_niche, Value{union_empty});
  assert(union_empty_bits.has_value());
  assert(*union_empty_bits == std::vector<std::uint8_t>(8, 0));
  assert(ValidValue(ctx, union_niche, *union_empty_bits));


  ModalVal modal_some;
  modal_some.state = "Some";
  RecordVal modal_payload;
  modal_payload.fields.push_back({"ptr", Value{PtrVal{PtrState::Valid, 0x2000}}});
  modal_some.payload = std::make_shared<Value>(Value{modal_payload});
  const auto modal_bits = ValueBits(ctx, MakeTypePath({"OptPtr"}), Value{modal_some});
  assert(modal_bits.has_value());
  assert(modal_bits->size() == 8);
  assert(ValidValue(ctx, MakeTypePath({"OptPtr"}), *modal_bits));


  ModalVal modal_none;
  modal_none.state = "None";
  modal_none.payload = std::make_shared<Value>(Value{cursive0::codegen::UnitVal{}});
  const auto modal_none_bits = ValueBits(ctx, MakeTypePath({"OptPtr"}), Value{modal_none});
  assert(modal_none_bits.has_value());
  assert(*modal_none_bits == std::vector<std::uint8_t>(8, 0));
  assert(ValidValue(ctx, MakeTypePath({"OptPtr"}), *modal_none_bits));


  ModalVal modal_on;
  modal_on.state = "On";
  RecordVal on_payload;
  on_payload.fields.push_back({"x", Value{IntVal{"u8", cursive0::core::UInt128FromU64(1)}}});
  modal_on.payload = std::make_shared<Value>(Value{on_payload});
  const auto modal_on_bits = ValueBits(ctx, MakeTypePath({"Toggle"}), Value{modal_on});
  assert(modal_on_bits.has_value());
  assert(modal_on_bits->size() == 16);
  assert((*modal_on_bits)[0] == 0);
  assert((*modal_on_bits)[8] == 1);
  assert(ValidValue(ctx, MakeTypePath({"Toggle"}), *modal_on_bits));
  auto bad_modal_bits = *modal_on_bits;
  bad_modal_bits[0] = 2;
  assert(!ValidValue(ctx, MakeTypePath({"Toggle"}), bad_modal_bits));


  Value string_val{StringVal{}};
  const auto string_bits = ValueBits(ctx, MakeTypeString(StringState::Managed), string_val);
  assert(string_bits.has_value());
  assert(string_bits->size() == 24);
  assert(ValidValue(ctx, MakeTypeString(StringState::Managed), *string_bits));


  Value bytes_val{BytesVal{}};
  const auto bytes_bits = ValueBits(ctx, MakeTypeBytes(BytesState::View), bytes_val);
  assert(bytes_bits.has_value());
  assert(bytes_bits->size() == 16);
  assert(ValidValue(ctx, MakeTypeBytes(BytesState::View), *bytes_bits));


  return 0;
}

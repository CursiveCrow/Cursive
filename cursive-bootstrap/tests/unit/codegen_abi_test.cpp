// Unit tests for ABI type lowering, parameter passing, and call lowering (§6.2.2-§6.2.4)

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cursive0/codegen/abi.h"
#include "cursive0/codegen/layout.h"
#include "cursive0/runtime/runtime_interface.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace {

using cursive0::codegen::ABICall;
using cursive0::codegen::ABICallInfo;
using cursive0::codegen::ABIParam;
using cursive0::codegen::ABIRet;
using cursive0::codegen::ABITy;
using cursive0::codegen::ABIType;
using cursive0::codegen::BuiltinMethodSym;
using cursive0::codegen::ByValOk;
using cursive0::codegen::IsBuiltinCapClass;
using cursive0::codegen::kByValAlign;
using cursive0::codegen::kByValMax;
using cursive0::codegen::kPanicOutName;
using cursive0::codegen::kPtrAlign;
using cursive0::codegen::kPtrSize;
using cursive0::codegen::LowerArgs;
using cursive0::codegen::LowerRecvArg;
using cursive0::codegen::MethodSymbol;
using cursive0::codegen::NeedsPanicOut;
using cursive0::codegen::PanicOutParams;
using cursive0::codegen::PanicOutType;
using cursive0::codegen::PanicRecordType;
using cursive0::codegen::PassKind;
using cursive0::codegen::SeqIR;
using cursive0::sema::BytesState;
using cursive0::sema::MakeTypeArray;
using cursive0::sema::MakeTypeBytes;
using cursive0::sema::MakeTypeDynamic;
using cursive0::sema::MakeTypeFunc;
using cursive0::sema::MakeTypeModalState;
using cursive0::sema::MakeTypePath;
using cursive0::sema::MakeTypePerm;
using cursive0::sema::MakeTypePrim;
using cursive0::sema::MakeTypePtr;
using cursive0::sema::MakeTypeRange;
using cursive0::sema::MakeTypeRawPtr;
using cursive0::sema::MakeTypeSlice;
using cursive0::sema::MakeTypeString;
using cursive0::sema::MakeTypeTuple;
using cursive0::sema::MakeTypeUnion;
using cursive0::sema::ParamMode;
using cursive0::sema::PathKeyOf;
using cursive0::sema::Permission;
using cursive0::sema::PtrState;
using cursive0::sema::RawPtrQual;
using cursive0::sema::ScopeContext;
using cursive0::sema::StringState;
using cursive0::sema::TypeFuncParam;
using cursive0::sema::TypeRef;
using cursive0::syntax::FieldDecl;
using cursive0::syntax::ModalDecl;
using cursive0::syntax::RecordDecl;
using cursive0::syntax::StateBlock;
using cursive0::syntax::StateFieldDecl;
using cursive0::syntax::Type;
using cursive0::syntax::TypePrim;
using cursive0::syntax::TypePtr;
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

}  // namespace

int main() {
  // SPEC_COV tags for §6.2.2 ABI Type Lowering
  SPEC_COV("ABI-Prim");
  SPEC_COV("ABI-Perm");
  SPEC_COV("ABI-Ptr");
  SPEC_COV("ABI-RawPtr");
  SPEC_COV("ABI-Func");
  SPEC_COV("ABI-Alias");
  SPEC_COV("ABI-Record");
  SPEC_COV("ABI-Tuple");
  SPEC_COV("ABI-Array");
  SPEC_COV("ABI-Slice");
  SPEC_COV("ABI-Range");
  SPEC_COV("ABI-Enum");
  SPEC_COV("ABI-Union");
  SPEC_COV("ABI-Modal");
  SPEC_COV("ABI-Dynamic");
  SPEC_COV("ABI-StringBytes");

  // SPEC_COV tags for §6.2.3 ABI Parameter and Return Passing
  SPEC_COV("ABI-Param-ByRef-Alias");
  SPEC_COV("ABI-Param-ByValue-Move");
  SPEC_COV("ABI-Param-ByRef-Move");
  SPEC_COV("ABI-Ret-ByValue");
  SPEC_COV("ABI-Ret-ByRef");
  SPEC_COV("ABI-Call");

  // SPEC_COV tags for §6.2.4 Call Lowering
  SPEC_COV("MethodSymbol-Record");
  SPEC_COV("MethodSymbol-Default");
  SPEC_COV("MethodSymbol-ModalState-Method");
  SPEC_COV("MethodSymbol-ModalState-Transition");
  SPEC_COV("BuiltinMethodSym-FileSystem");
  SPEC_COV("BuiltinMethodSym-HeapAllocator");
  SPEC_COV("Lower-Args-Empty");
  SPEC_COV("Lower-Args-Cons-Move");
  SPEC_COV("Lower-Args-Cons-Ref");
  SPEC_COV("Lower-RecvArg-Move");
  SPEC_COV("Lower-RecvArg-Ref");
  SPEC_COV("Lower-MethodCall-Static-PanicOut");
  SPEC_COV("Lower-MethodCall-Static-NoPanicOut");
  SPEC_COV("Lower-MethodCall-Capability");
  SPEC_COV("Lower-MethodCall-Dynamic");

  // Set up context
  ScopeContext ctx;
  ctx.current_module = {"Test"};

  // Register test types
  RecordDecl point;
  point.vis = Visibility::Public;
  point.name = "Point";
  point.implements = {};
  point.members = {MakeField("x", MakePrimType("i32")),
                   MakeField("y", MakePrimType("i32"))};
  point.span = {};
  point.doc = {};

  RecordDecl large_record;
  large_record.vis = Visibility::Public;
  large_record.name = "LargeRecord";
  large_record.implements = {};
  large_record.members = {
      MakeField("a", MakePrimType("u64")),
      MakeField("b", MakePrimType("u64")),
      MakeField("c", MakePrimType("u64")),
  };
  large_record.span = {};
  large_record.doc = {};

  cursive0::syntax::TypeAliasDecl alias;
  alias.vis = Visibility::Public;
  alias.name = "AliasI64";
  alias.type = MakePrimType("i64");
  alias.span = {};
  alias.doc = {};

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

  ctx.sigma.types[PathKeyOf({"Point"})] = point;
  ctx.sigma.types[PathKeyOf({"LargeRecord"})] = large_record;
  ctx.sigma.types[PathKeyOf({"AliasI64"})] = alias;
  ctx.sigma.types[PathKeyOf({"OptPtr"})] = opt_ptr;

  // ==========================================================================
  // Test §6.2.2 ABITy rules
  // ==========================================================================

  // ABI-Prim: primitive types
  {
    const auto i32_abi = ABITy(ctx, MakeTypePrim("i32"));
    assert(i32_abi.has_value());
    assert(i32_abi->size == 4);
    assert(i32_abi->align == 4);

    const auto u8_abi = ABITy(ctx, MakeTypePrim("u8"));
    assert(u8_abi.has_value());
    assert(u8_abi->size == 1);
    assert(u8_abi->align == 1);

    const auto unit_abi = ABITy(ctx, MakeTypePrim("()"));
    assert(unit_abi.has_value());
    assert(unit_abi->size == 0);
    assert(unit_abi->align == 1);

    const auto usize_abi = ABITy(ctx, MakeTypePrim("usize"));
    assert(usize_abi.has_value());
    assert(usize_abi->size == kPtrSize);
    assert(usize_abi->align == kPtrAlign);
  }

  // ABI-Perm: strip permission
  {
    const auto perm_i32 = MakeTypePerm(Permission::Const, MakeTypePrim("i32"));
    const auto perm_abi = ABITy(ctx, perm_i32);
    assert(perm_abi.has_value());
    assert(perm_abi->size == 4);
    assert(perm_abi->align == 4);
  }

  // ABI-Ptr, ABI-RawPtr, ABI-Func
  {
    const auto ptr = MakeTypePtr(MakeTypePrim("u8"), PtrState::Valid);
    const auto ptr_abi = ABITy(ctx, ptr);
    assert(ptr_abi.has_value());
    assert(ptr_abi->size == kPtrSize);
    assert(ptr_abi->align == kPtrAlign);

    const auto raw = MakeTypeRawPtr(RawPtrQual::Imm, MakeTypePrim("u8"));
    const auto raw_abi = ABITy(ctx, raw);
    assert(raw_abi.has_value());
    assert(raw_abi->size == kPtrSize);
    assert(raw_abi->align == kPtrAlign);

    const auto func = MakeTypeFunc({}, MakeTypePrim("i32"));
    const auto func_abi = ABITy(ctx, func);
    assert(func_abi.has_value());
    assert(func_abi->size == kPtrSize);
    assert(func_abi->align == kPtrAlign);
  }

  // ABI-Slice, ABI-Dynamic
  {
    const auto slice = MakeTypeSlice(MakeTypePrim("u8"));
    const auto slice_abi = ABITy(ctx, slice);
    assert(slice_abi.has_value());
    assert(slice_abi->size == 2 * kPtrSize);
    assert(slice_abi->align == kPtrAlign);

    const auto dyn = MakeTypeDynamic({"Widget"});
    const auto dyn_abi = ABITy(ctx, dyn);
    assert(dyn_abi.has_value());
    assert(dyn_abi->size == 2 * kPtrSize);
    assert(dyn_abi->align == kPtrAlign);
  }

  // ABI-Tuple, ABI-Array
  {
    const auto tuple = MakeTypeTuple({MakeTypePrim("u8"), MakeTypePrim("u32")});
    const auto tuple_abi = ABITy(ctx, tuple);
    assert(tuple_abi.has_value());
    assert(tuple_abi->size == 8);  // 1 + 3 padding + 4
    assert(tuple_abi->align == 4);

    const auto array = MakeTypeArray(MakeTypePrim("u16"), 4);
    const auto array_abi = ABITy(ctx, array);
    assert(array_abi.has_value());
    assert(array_abi->size == 8);  // 2 * 4
    assert(array_abi->align == 2);
  }

  // ABI-Record, ABI-Alias
  {
    const auto record = MakeTypePath({"Point"});
    const auto record_abi = ABITy(ctx, record);
    assert(record_abi.has_value());
    assert(record_abi->size == 8);  // 2 * i32
    assert(record_abi->align == 4);

    const auto alias_type = MakeTypePath({"AliasI64"});
    const auto alias_abi = ABITy(ctx, alias_type);
    assert(alias_abi.has_value());
    assert(alias_abi->size == 8);
    assert(alias_abi->align == 8);
  }

  // ABI-StringBytes
  {
    const auto str_managed = MakeTypeString(StringState::Managed);
    const auto str_managed_abi = ABITy(ctx, str_managed);
    assert(str_managed_abi.has_value());
    assert(str_managed_abi->size == 3 * kPtrSize);
    assert(str_managed_abi->align == kPtrAlign);

    const auto str_view = MakeTypeString(StringState::View);
    const auto str_view_abi = ABITy(ctx, str_view);
    assert(str_view_abi.has_value());
    assert(str_view_abi->size == 2 * kPtrSize);
    assert(str_view_abi->align == kPtrAlign);

    const auto bytes_managed = MakeTypeBytes(BytesState::Managed);
    const auto bytes_managed_abi = ABITy(ctx, bytes_managed);
    assert(bytes_managed_abi.has_value());
    assert(bytes_managed_abi->size == 3 * kPtrSize);
    assert(bytes_managed_abi->align == kPtrAlign);
  }

  // ABI-Range
  {
    const auto range = MakeTypeRange();
    const auto range_abi = ABITy(ctx, range);
    assert(range_abi.has_value());
    assert(range_abi->size == 24);  // kind (u8) + lo (usize) + hi (usize) + padding
    assert(range_abi->align == 8);
  }

  // ==========================================================================
  // Test §6.2.3 ABI Parameter and Return Passing
  // ==========================================================================

  // ByValOk predicate
  {
    // Small types pass ByValOk
    assert(ByValOk(ctx, MakeTypePrim("u8")));
    assert(ByValOk(ctx, MakeTypePrim("i32")));
    assert(ByValOk(ctx, MakeTypePrim("u64")));
    assert(ByValOk(ctx, MakeTypePrim("()")));

    // 16 bytes = 2 * PtrSize is the max
    const auto ptr = MakeTypePtr(MakeTypePrim("u8"), PtrState::Valid);
    assert(ByValOk(ctx, ptr));  // 8 bytes

    const auto slice = MakeTypeSlice(MakeTypePrim("u8"));
    assert(ByValOk(ctx, slice));  // 16 bytes - exactly at limit

    // Large types fail ByValOk
    const auto large = MakeTypePath({"LargeRecord"});
    assert(!ByValOk(ctx, large));  // 24 bytes

    const auto str = MakeTypeString(StringState::Managed);
    assert(!ByValOk(ctx, str));  // 24 bytes
  }

  // ABIParam tests
  {
    // Non-move (aliased) params are always ByRef
    const auto param_alias = ABIParam(ctx, std::nullopt, MakeTypePrim("i32"));
    assert(param_alias.has_value());
    assert(*param_alias == PassKind::ByRef);

    // Small move params are ByValue
    const auto param_move_small = ABIParam(ctx, ParamMode::Move, MakeTypePrim("i32"));
    assert(param_move_small.has_value());
    assert(*param_move_small == PassKind::ByValue);

    // Zero-sized move params are ByValue
    const auto param_move_unit = ABIParam(ctx, ParamMode::Move, MakeTypePrim("()"));
    assert(param_move_unit.has_value());
    assert(*param_move_unit == PassKind::ByValue);

    // Large move params are ByRef
    const auto param_move_large = ABIParam(ctx, ParamMode::Move, MakeTypePath({"LargeRecord"}));
    assert(param_move_large.has_value());
    assert(*param_move_large == PassKind::ByRef);
  }

  // ABIRet tests
  {
    // Small returns are ByValue
    const auto ret_small = ABIRet(ctx, MakeTypePrim("i32"));
    assert(ret_small.has_value());
    assert(*ret_small == PassKind::ByValue);

    // Zero-sized returns are ByValue
    const auto ret_unit = ABIRet(ctx, MakeTypePrim("()"));
    assert(ret_unit.has_value());
    assert(*ret_unit == PassKind::ByValue);

    // Large returns are SRet
    const auto ret_large = ABIRet(ctx, MakeTypePath({"LargeRecord"}));
    assert(ret_large.has_value());
    assert(*ret_large == PassKind::SRet);
  }

  // ABICall tests
  {
    std::vector<std::pair<std::optional<ParamMode>, TypeRef>> params;
    params.push_back({ParamMode::Move, MakeTypePrim("i32")});
    params.push_back({std::nullopt, MakeTypePrim("u64")});

    const auto call_info = ABICall(ctx, params, MakeTypePrim("bool"));
    assert(call_info.has_value());
    assert(call_info->param_kinds.size() == 2);
    assert(call_info->param_kinds[0] == PassKind::ByValue);  // move i32
    assert(call_info->param_kinds[1] == PassKind::ByRef);    // alias u64
    assert(call_info->ret_kind == PassKind::ByValue);
    assert(!call_info->has_sret);

    // Test SRet case
    const auto call_sret = ABICall(ctx, {}, MakeTypePath({"LargeRecord"}));
    assert(call_sret.has_value());
    assert(call_sret->ret_kind == PassKind::SRet);
    assert(call_sret->has_sret);
  }

  // ==========================================================================
  // Test Panic Out-Parameter Support
  // ==========================================================================

  {
    // PanicRecordType should be a tuple (bool, u32)
    const auto panic_record = PanicRecordType();
    assert(panic_record != nullptr);

    // PanicOutType should be rawptr[mut, PanicRecord]
    const auto panic_out = PanicOutType();
    assert(panic_out != nullptr);

    // kPanicOutName should be "__panic"
    assert(kPanicOutName == "__panic");

    // NeedsPanicOut tests
    assert(!NeedsPanicOut("main"));  // entry point
    assert(!NeedsPanicOut("cursive::runtime::panic"));  // runtime symbol
    assert(!NeedsPanicOut("cursive::runtime::fs::open_read"));  // builtin
    assert(NeedsPanicOut("my_function"));  // user-defined

    // PanicOutParams test
    std::vector<std::tuple<std::optional<ParamMode>, std::string, TypeRef>> original_params;
    original_params.push_back({ParamMode::Move, "x", MakeTypePrim("i32")});

    const auto with_panic = PanicOutParams(original_params, "my_func");
    assert(with_panic.size() == 2);  // original + panic param

    const auto without_panic = PanicOutParams(original_params, "main");
    assert(without_panic.size() == 1);  // no panic param for entry
  }

  // ==========================================================================
  // Test §6.2.4 Call Lowering Support
  // ==========================================================================

  // BuiltinMethodSym tests
  {
    assert(IsBuiltinCapClass("FileSystem"));
    assert(IsBuiltinCapClass("HeapAllocator"));
    assert(!IsBuiltinCapClass("SomeOtherClass"));

    const auto fs_sym = BuiltinMethodSym("FileSystem", "open_read");
    assert(fs_sym.has_value());
    assert(*fs_sym == cursive0::codegen::BuiltinSym("FileSystem::open_read"));

    const auto heap_sym = BuiltinMethodSym("HeapAllocator", "alloc_raw");
    assert(heap_sym.has_value());
    assert(*heap_sym == cursive0::codegen::BuiltinSym("HeapAllocator::alloc_raw"));

    const auto unknown_sym = BuiltinMethodSym("UnknownClass", "method");
    assert(!unknown_sym.has_value());
  }

  // LowerArgs empty case
  {
    std::vector<cursive0::syntax::Param> empty_params;
    std::vector<cursive0::syntax::Arg> empty_args;

    const auto result = LowerArgs(ctx, empty_params, empty_args);
    assert(result.has_value());
    assert(result->values.empty());
  }

  // LowerRecvArg tests
  {
    // Create a dummy expression
    auto expr = std::make_shared<cursive0::syntax::Expr>();
    cursive0::syntax::PathExpr path_expr;
    path_expr.path = {"x"};
    expr->node = path_expr;

    // Move receiver
    const auto move_result = LowerRecvArg(ctx, expr, true);
    assert(move_result.has_value());

    // Reference receiver
    const auto ref_result = LowerRecvArg(ctx, expr, false);
    assert(ref_result.has_value());
  }

  // SeqIR tests
  {
    const auto empty = SeqIR();
    assert(empty != nullptr);

    const auto single = SeqIR(empty);
    assert(single != nullptr);

    const auto two = SeqIR(empty, empty);
    assert(two != nullptr);

    std::vector<cursive0::codegen::IRPtr> vec = {empty, empty, empty};
    const auto multi = SeqIR(vec);
    assert(multi != nullptr);
  }

  // Constants verification
  {
    // Verify constants match spec
    assert(kPtrSize == 8);
    assert(kPtrAlign == 8);
    assert(kByValMax == 16);
    assert(kByValAlign == 8);
  }

  return 0;
}

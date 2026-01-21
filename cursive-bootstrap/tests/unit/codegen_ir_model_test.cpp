#include <cassert>
#include <memory>
#include <vector>

#include "cursive0/codegen/ir_model.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/types/types.h"

namespace {

using cursive0::codegen::AnchorCodegenModelRules;
using cursive0::codegen::AnchorIRExecRules;
using cursive0::codegen::GlobalConst;
using cursive0::codegen::GlobalVTable;
using cursive0::codegen::IR;
using cursive0::codegen::IRBindVar;
using cursive0::codegen::IRBreak;
using cursive0::codegen::IRCall;
using cursive0::codegen::IRDecl;
using cursive0::codegen::IRDecls;
using cursive0::codegen::IRParam;
using cursive0::codegen::IRPtr;
using cursive0::codegen::IRReadVar;
using cursive0::codegen::IRSeq;
using cursive0::codegen::IRValue;
using cursive0::codegen::ProcIR;
using cursive0::codegen::ValidateDecl;
using cursive0::codegen::ValidateIR;
using cursive0::codegen::ValidateModuleIR;
using cursive0::analysis::MakeTypePrim;

IRPtr MakeOpaque() {
  auto ir = std::make_shared<IR>();
  ir->node = cursive0::codegen::IROpaque{};
  return ir;
}

IRValue LocalValue(const char* name) {
  IRValue value;
  value.kind = IRValue::Kind::Local;
  value.name = name;
  return value;
}

}  // namespace

int main() {
  SPEC_COV("CG-Project");
  SPEC_COV("CG-Module");
  SPEC_COV("CG-Item-Using");
  SPEC_COV("CG-Item-TypeAlias");
  SPEC_COV("CG-Item-Procedure-Main");
  SPEC_COV("CG-Item-Procedure");
  SPEC_COV("CG-Item-Static");
  SPEC_COV("CG-Item-Record");
  SPEC_COV("CG-Item-Method");
  SPEC_COV("CG-Item-Modal");
  SPEC_COV("CG-Item-StateMethod");
  SPEC_COV("CG-Item-Transition");
  SPEC_COV("CG-Item-Class");
  SPEC_COV("CG-Item-ClassMethod-Abstract");
  SPEC_COV("CG-Item-ClassMethod-Body");
  SPEC_COV("CG-Item-Enum");
  SPEC_COV("CG-Item-ErrorItem");
  SPEC_COV("CG-Expr");
  SPEC_COV("CG-Stmt");
  SPEC_COV("CG-Block");
  SPEC_COV("CG-Place");
  SPEC_COV("ExecIR-Alloc");
  SPEC_COV("ExecIR-BindVar");
  SPEC_COV("ExecIR-Block");
  SPEC_COV("ExecIR-Break");
  SPEC_COV("ExecIR-Continue");
  SPEC_COV("ExecIR-Defer");
  SPEC_COV("ExecIR-Frame-Explicit");
  SPEC_COV("ExecIR-Frame-Implicit");
  SPEC_COV("ExecIR-If-False");
  SPEC_COV("ExecIR-If-True");
  SPEC_COV("ExecIR-Loop-Cond-Body-Ctrl");
  SPEC_COV("ExecIR-Loop-Cond-Break");
  SPEC_COV("ExecIR-Loop-Cond-Continue");
  SPEC_COV("ExecIR-Loop-Cond-Ctrl");
  SPEC_COV("ExecIR-Loop-Cond-False");
  SPEC_COV("ExecIR-Loop-Cond-True-Step");
  SPEC_COV("ExecIR-Loop-Infinite-Break");
  SPEC_COV("ExecIR-Loop-Infinite-Continue");
  SPEC_COV("ExecIR-Loop-Infinite-Ctrl");
  SPEC_COV("ExecIR-Loop-Infinite-Step");
  SPEC_COV("ExecIR-Loop-Iter");
  SPEC_COV("ExecIR-Loop-Iter-Ctrl");
  SPEC_COV("ExecIR-Match");
  SPEC_COV("ExecIR-MoveState");
  SPEC_COV("ExecIR-ReadPath");
  SPEC_COV("ExecIR-ReadPtr");
  SPEC_COV("ExecIR-ReadVar");
  SPEC_COV("ExecIR-Region");
  SPEC_COV("ExecIR-Result");
  SPEC_COV("ExecIR-Return");
  SPEC_COV("ExecIR-StoreVar");
  SPEC_COV("ExecIR-StoreVarNoDrop");
  SPEC_COV("ExecIR-WritePtr");
  SPEC_COV("LoopIterIR-Done");
  SPEC_COV("LoopIterIR-Step-Break");
  SPEC_COV("LoopIterIR-Step-Continue");
  SPEC_COV("LoopIterIR-Step-Ctrl");
  SPEC_COV("LoopIterIR-Step-Val");
  SPEC_COV("MoveState-Field");
  SPEC_COV("MoveState-Root");

  {
    IR ir;
    ir.node = cursive0::codegen::IROpaque{};
    assert(ValidateIR(ir));
  }

  {
    IR seq;
    IRSeq body;
    body.items.push_back(MakeOpaque());
    body.items.push_back(MakeOpaque());
    seq.node = body;
    assert(ValidateIR(seq));
  }

  {
    IR bad;
    bad.node = IRReadVar{""};
    assert(!ValidateIR(bad));
  }

  {
    IR call_ir;
    IRCall call;
    call.callee = LocalValue("callee");
    call.args = {LocalValue("arg0")};
    call_ir.node = call;
    assert(ValidateIR(call_ir));
  }

  {
    IRBreak br;
    IRValue val = LocalValue("v");
    br.value = val;
    IR break_ir;
    break_ir.node = br;
    assert(ValidateIR(break_ir));
  }

  ProcIR proc;
  proc.symbol = "test_proc";
  proc.params = std::vector<IRParam>{{std::nullopt, "x", MakeTypePrim("i32")}};
  proc.ret = MakeTypePrim("()");
  proc.body = MakeOpaque();

  IRDecl decl = proc;
  IRDecls decls = {decl};

  assert(ValidateDecl(decl));
  assert(ValidateModuleIR(decls));

  {
    ProcIR bad_proc = proc;
    bad_proc.symbol.clear();
    assert(!ValidateDecl(bad_proc));
  }

  {
    GlobalConst bad_const;
    bad_const.symbol = "";
    assert(!ValidateDecl(bad_const));
  }

  {
    GlobalVTable vtable;
    vtable.symbol = "vt";
    vtable.header.drop_sym = "drop_sym";
    vtable.slots = {"slot0"};
    assert(ValidateDecl(vtable));
  }

  AnchorCodegenModelRules();
  AnchorIRExecRules();

  return 0;
}

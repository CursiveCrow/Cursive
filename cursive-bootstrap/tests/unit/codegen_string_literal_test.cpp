// Unit test for string literal lowering and emission (T-DYN-006 / §7.5)

#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "cursive0/codegen/layout.h"
#include "cursive0/codegen/llvm_emit.h"
#include "cursive0/codegen/mangle.h"
#include "cursive0/codegen/lower_expr.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/sema/types.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

using cursive0::codegen::DecodeStringLiteralBytes;
using cursive0::codegen::IRDecls;
using cursive0::codegen::IRValue;
using cursive0::codegen::LLVMEmitter;
using cursive0::codegen::LowerCtx;
using cursive0::codegen::MangleLiteral;
using cursive0::sema::MakeTypeString;
using cursive0::sema::StringState;

int main() {
  SPEC_COV("StringLiteralVal");
  SPEC_COV("EmitLiteral-String");
  SPEC_COV("EmitLiteralData-Decl");
  SPEC_COV("EmitLiteralData-Bytes");
  SPEC_COV("UniqueEmits-Literal");

  const auto decoded = DecodeStringLiteralBytes("\"hi\"");
  assert(decoded.has_value());

  IRValue lit;
  lit.kind = IRValue::Kind::Immediate;
  lit.name = "\"hi\"";
  lit.bytes = *decoded;

  LowerCtx ctx;
  ctx.RegisterValueType(lit, MakeTypeString(StringState::View));

  llvm::LLVMContext llvm_ctx;
  LLVMEmitter emitter(llvm_ctx, "string_literal_test");
  IRDecls decls;
  emitter.EmitModule(decls, ctx);

  llvm::Value* val = emitter.EvaluateIRValue(lit);
  assert(val != nullptr);
  auto* struct_val = llvm::dyn_cast<llvm::ConstantStruct>(val);
  assert(struct_val != nullptr);
  assert(struct_val->getNumOperands() == 2);

  auto* len_const = llvm::dyn_cast<llvm::ConstantInt>(struct_val->getOperand(1));
  assert(len_const != nullptr);
  assert(len_const->getZExtValue() == decoded->size());

  const std::string sym = MangleLiteral("string", *decoded);
  auto* gv = emitter.GetModule().getGlobalVariable(sym, true);
  assert(gv != nullptr);
  assert(gv->isConstant());

  return 0;
}

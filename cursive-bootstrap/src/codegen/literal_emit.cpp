#include "cursive0/codegen/llvm_emit.h"
#include "cursive0/codegen/layout.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/core/symbols.h"
#include "cursive0/sema/scopes.h"
#include "cursive0/sema/types.h"

// LLVM Includes
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <vector>

namespace cursive0::codegen {

namespace {

sema::ScopeContext BuildScope(const LowerCtx* ctx) {
  sema::ScopeContext scope;
  if (ctx && ctx->sigma) {
    scope.sigma = *ctx->sigma;
    scope.current_module = ctx->module_path;
  }
  return scope;
}

sema::TypeRef StripPerm(const sema::TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<sema::TypePerm>(&type->node)) {
    return StripPerm(perm->base);
  }
  return type;
}

bool IsIntTypeName(std::string_view name) {
  static constexpr std::string_view kInts[] = {
      "i8", "i16", "i32", "i64", "i128", "isize",
      "u8", "u16", "u32", "u64", "u128", "usize"};
  for (const auto& t : kInts) {
    if (t == name) {
      return true;
    }
  }
  return false;
}

bool IsFloatTypeName(std::string_view name) {
  static constexpr std::string_view kFloats[] = {"f16", "f32", "f64"};
  for (const auto& t : kFloats) {
    if (t == name) {
      return true;
    }
  }
  return false;
}

llvm::APInt APIntFromBytes(const std::vector<std::uint8_t>& bytes) {
  const unsigned bitwidth = static_cast<unsigned>(bytes.size() * 8);
  if (bitwidth == 0) {
    return llvm::APInt(1, 0);
  }
  std::vector<std::uint64_t> words;
  words.reserve((bytes.size() + 7) / 8);
  for (std::size_t i = 0; i < bytes.size(); i += 8) {
    std::uint64_t word = 0;
    const std::size_t limit = std::min<std::size_t>(8, bytes.size() - i);
    for (std::size_t j = 0; j < limit; ++j) {
      word |= static_cast<std::uint64_t>(bytes[i + j]) << (8 * j);
    }
    words.push_back(word);
  }
  return llvm::APInt(bitwidth, words);
}

bool AllZero(const std::vector<std::uint8_t>& bytes) {
  for (auto b : bytes) {
    if (b != 0) {
      return false;
    }
  }
  return true;
}

llvm::Constant* ConstBytes(llvm::Type* ty,
                           const std::vector<std::uint8_t>& bytes,
                           LLVMEmitter& emitter,
                           LowerCtx* lower_ctx) {
  if (!ty) {
    return nullptr;
  }

  if (bytes.empty()) {
    return llvm::Constant::getNullValue(ty);
  }

  if (auto* arr = llvm::dyn_cast<llvm::ArrayType>(ty)) {
    if (arr->getElementType()->isIntegerTy(8) &&
        arr->getNumElements() == bytes.size()) {
      return llvm::ConstantDataArray::get(
          emitter.GetContext(), llvm::ArrayRef<uint8_t>(bytes.data(), bytes.size()));
    }
  }

  if (auto* int_ty = llvm::dyn_cast<llvm::IntegerType>(ty)) {
    const unsigned bits = int_ty->getBitWidth();
    if (bits == bytes.size() * 8) {
      return llvm::ConstantInt::get(emitter.GetContext(), APIntFromBytes(bytes));
    }
  }

  if (ty->isHalfTy() || ty->isFloatTy() || ty->isDoubleTy()) {
    const unsigned bits = ty->getPrimitiveSizeInBits();
    if (bits == bytes.size() * 8) {
      auto int_ty = llvm::IntegerType::get(emitter.GetContext(), bits);
      auto int_c = llvm::ConstantInt::get(emitter.GetContext(), APIntFromBytes(bytes));
      return llvm::ConstantExpr::getBitCast(int_c, ty);
    }
  }

  if (ty->isPointerTy() && AllZero(bytes)) {
    return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ty));
  }

  if (lower_ctx) {
    lower_ctx->ReportCodegenFailure();
  }
  return llvm::Constant::getNullValue(ty);
}

sema::TypeRef StaticTypeForConst(const GlobalConst& global, const LowerCtx* ctx) {
  if (ctx) {
    auto type = ctx->LookupStaticType(global.symbol);
    if (type) {
      return type;
    }
  }
  return sema::MakeTypeArray(sema::MakeTypePrim("u8"), global.bytes.size());
}

bool IsLiteralSymbol(const std::string& symbol) {
  static const std::string kLiteralPrefix =
      core::Mangle(core::StringOfPath({"cursive", "runtime", "literal"}));
  return symbol.rfind(kLiteralPrefix, 0) == 0;
}

}  // namespace

// T-LLVM-013: Literal Data Emission
void LLVMEmitter::EmitGlobalConst(const GlobalConst& global) {
  SPEC_RULE("EmitLiteralData-Decl");
  SPEC_RULE("EmitLiteralData-Bytes");
  SPEC_RULE("UniqueEmits-Literal");
  SPEC_RULE("LowerIRDecl-GlobalConst");

  const auto scope = BuildScope(current_ctx_);
  sema::TypeRef type = StaticTypeForConst(global, current_ctx_);
  type = StripPerm(type);
  const bool was_failed = current_ctx_ && current_ctx_->codegen_failed;
  llvm::Type* llvm_ty = GetLLVMType(type);
  llvm::Constant* init = ConstBytes(llvm_ty, global.bytes, *this, current_ctx_);
  if (!init) {
    init = llvm::Constant::getNullValue(llvm_ty);
  }

  const bool is_literal = IsLiteralSymbol(global.symbol);
  if (is_literal && type) {
    if (const auto* prim = std::get_if<sema::TypePrim>(&type->node)) {
      if (prim->name == "char") {
        SPEC_RULE("EmitLiteral-Char");
      } else if (IsIntTypeName(prim->name)) {
        SPEC_RULE("EmitLiteral-Int");
      } else if (IsFloatTypeName(prim->name)) {
        SPEC_RULE("EmitLiteral-Float");
      }
    }
  }
  if (is_literal && current_ctx_ && current_ctx_->codegen_failed && !was_failed) {
    SPEC_RULE("EmitLiteral-Err");
  }
  auto* gvar = new llvm::GlobalVariable(
      *module_,
      llvm_ty,
      is_literal,
      is_literal ? llvm::GlobalValue::InternalLinkage
                 : llvm::GlobalValue::ExternalLinkage,
      init,
      global.symbol);

  const auto align = AlignOf(scope, type).value_or(1);
  gvar->setAlignment(llvm::Align(align));
  if (is_literal) {
    gvar->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
  }

  globals_[global.symbol] = gvar;
}

void LLVMEmitter::EmitGlobalZero(const GlobalZero& global) {
  SPEC_RULE("LowerIRDecl-GlobalZero");
  const auto scope = BuildScope(current_ctx_);

  sema::TypeRef type;
  if (current_ctx_) {
    type = current_ctx_->LookupStaticType(global.symbol);
  }
  if (!type) {
    type = sema::MakeTypeArray(sema::MakeTypePrim("u8"), global.size);
  }

  llvm::Type* llvm_ty = GetLLVMType(type);
  llvm::Constant* init = llvm::Constant::getNullValue(llvm_ty);

  auto* gvar = new llvm::GlobalVariable(
      *module_,
      llvm_ty,
      false,
      llvm::GlobalValue::ExternalLinkage,
      init,
      global.symbol);

  const auto align = AlignOf(scope, type).value_or(1);
  gvar->setAlignment(llvm::Align(align));

  globals_[global.symbol] = gvar;
}

} // namespace cursive0::codegen

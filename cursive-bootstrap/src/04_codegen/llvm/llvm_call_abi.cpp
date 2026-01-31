#include "cursive0/04_codegen/llvm/llvm_emit.h"
#include "cursive0/04_codegen/abi/abi.h"
#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/resolve/scopes.h"
#include "cursive0/03_analysis/types/types.h"

// LLVM Includes
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"

#include <cstdlib>
#include <iostream>

namespace cursive0::codegen {

namespace {

analysis::ScopeContext BuildScope(const LowerCtx* ctx) {
  analysis::ScopeContext scope;
  if (ctx && ctx->sigma) {
    scope.sigma = *ctx->sigma;
    scope.current_module = ctx->module_path;
  }
  return scope;
}

analysis::TypeRef StripPerm(const analysis::TypeRef& type) {
  if (!type) {
    return type;
  }
  if (const auto* perm = std::get_if<analysis::TypePerm>(&type->node)) {
    return StripPerm(perm->base);
  }
  return type;
}

bool IsValidPtrType(const analysis::TypeRef& type) {
  const auto stripped = StripPerm(type);
  if (!stripped) {
    return false;
  }
  if (const auto* ptr = std::get_if<analysis::TypePtr>(&stripped->node)) {
    return ptr->state.has_value() && *ptr->state == analysis::PtrState::Valid;
  }
  return false;
}

}  // namespace

// T-LLVM-011: Call ABI Mapping
ABICallResult LLVMEmitter::ComputeCallABI(const std::vector<IRParam>& params,
                                          analysis::TypeRef ret_type) {
  SPEC_RULE("LLVMCall-ByValue");
  SPEC_RULE("LLVMCall-SRet");

  ABICallResult result;

  const analysis::ScopeContext scope = BuildScope(current_ctx_);

  std::vector<std::pair<std::optional<analysis::ParamMode>, analysis::TypeRef>> abi_params;
  abi_params.reserve(params.size());
  for (const auto& param : params) {
    abi_params.push_back({param.mode, param.type});
  }

  const auto call_info = ABICall(scope, abi_params, ret_type);
  if (!call_info.has_value()) {
    SPEC_RULE("LLVMCall-Err");
    if (current_ctx_) {
      if (std::getenv("CURSIVE0_DEBUG_OBJ")) {
        std::cerr << "[cursivec0] ABI call failed: ret="
                  << analysis::TypeToString(ret_type) << " params=[";
        for (std::size_t i = 0; i < params.size(); ++i) {
          if (i > 0) {
            std::cerr << ", ";
          }
          const auto& param = params[i];
          if (param.mode.has_value() &&
              *param.mode == analysis::ParamMode::Move) {
            std::cerr << "move ";
          }
          std::cerr << analysis::TypeToString(param.type);
        }
        std::cerr << "]\n";
      }
      current_ctx_->ReportCodegenFailure();
    }
    result.ret_type = llvm::Type::getVoidTy(context_);
    result.func_type = llvm::FunctionType::get(result.ret_type, {}, false);
    return result;
  }

  result.param_kinds = call_info->param_kinds;
  result.has_sret = call_info->has_sret;

  // Compute return type.
  if (call_info->ret_kind == PassKind::SRet) {
    SPEC_RULE("LLVMRetLower-SRet");
    result.ret_type = llvm::Type::getVoidTy(context_);
  } else {
    const auto size = SizeOf(scope, ret_type);
    if (!size.has_value()) {
      SPEC_RULE("LLVMRetLower-Err");
      if (current_ctx_) {
        current_ctx_->ReportCodegenFailure();
      }
      result.ret_type = llvm::Type::getVoidTy(context_);
    } else if (*size == 0) {
      SPEC_RULE("LLVMRetLower-ByValue-ZST");
      result.ret_type = llvm::Type::getVoidTy(context_);
    } else {
      SPEC_RULE("LLVMRetLower-ByValue");
      result.ret_type = GetLLVMType(ret_type);
      if (!result.ret_type) {
        SPEC_RULE("LLVMRetLower-Err");
        if (current_ctx_) {
          current_ctx_->ReportCodegenFailure();
        }
        result.ret_type = llvm::Type::getVoidTy(context_);
      }
    }
  }

  result.param_indices.assign(params.size(), std::nullopt);

  // Compute parameter list.
  if (result.has_sret) {
    result.param_types.push_back(GetOpaquePtr());
  }

  unsigned llvm_index = result.has_sret ? 1u : 0u;
  for (std::size_t i = 0; i < params.size(); ++i) {
    const auto kind = result.param_kinds[i];
    if (kind == PassKind::ByRef) {
      SPEC_RULE("LLVMArgLower-ByRef");
      result.param_types.push_back(GetOpaquePtr());
      result.param_indices[i] = llvm_index++;
      continue;
    }

    const auto size = SizeOf(scope, params[i].type);
    if (!size.has_value()) {
      SPEC_RULE("LLVMArgLower-Err");
      if (current_ctx_) {
        current_ctx_->ReportCodegenFailure();
      }
      result.param_types.push_back(GetOpaquePtr());
      result.param_indices[i] = llvm_index++;
      continue;
    }
    if (*size == 0) {
      result.param_indices[i] = std::nullopt;
      continue;
    }

    if (IsValidPtrType(params[i].type)) {
      SPEC_RULE("LLVMArgLower-ByValue-PtrValid");
    } else {
      SPEC_RULE("LLVMArgLower-ByValue-Other");
    }
    llvm::Type* llvm_ty = GetLLVMType(params[i].type);
    if (!llvm_ty) {
      SPEC_RULE("LLVMArgLower-Err");
      if (current_ctx_) {
        current_ctx_->ReportCodegenFailure();
      }
      llvm_ty = GetOpaquePtr();
    }
    result.param_types.push_back(llvm_ty);
    result.param_indices[i] = llvm_index++;
  }

  result.func_type = llvm::FunctionType::get(result.ret_type, result.param_types, false);
  return result;
}

} // namespace cursive0::codegen

// =============================================================================
// File: 05_codegen/llvm/llvm_call.h
// Construct: LLVM Call ABI and Calling Convention Handling
// Spec Section: 6.12.9, 6.2
// Spec Rules: LLVMCall-ByValue, LLVMCall-SRet, LLVMArgLower-*, LLVMRetLower-*
// =============================================================================
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "04_analysis/typing/types.h"
#include "05_codegen/abi/abi.h"
#include "05_codegen/ir/ir_model.h"
#include "05_codegen/llvm/llvm_attr.h"

// Forward declarations for LLVM types
namespace llvm {
class AllocaInst;
class CallInst;
class Function;
class FunctionType;
class IRBuilderBase;
class Type;
class Value;
}  // namespace llvm

namespace cursive::codegen {

// Forward declarations
class LLVMEmitter;
struct LowerCtx;

// =============================================================================
// §6.12.9 LLVM Call Signature Lowering
// =============================================================================

// LLVMCallSig(params, R) ⇓ sig
// Maps Cursive procedure signatures to LLVM function types with ABI lowering

// Result of LLVM call signature computation (extends ABICallResult in llvm_emit.h)
// ABICallResult is already declared in llvm_emit.h, additional helpers here

// -----------------------------------------------------------------------------
// Call ABI Helpers
// -----------------------------------------------------------------------------

// IsValidPtrType(T) - checks if type is a valid pointer (Ptr<T>@Valid)
bool IsValidPtrType(const analysis::TypeRef& type);

enum class ByRefAccessKind {
  ReadOnly,
  ReadWrite,
};

ByRefAccessKind ByRefAccess(const analysis::TypeRef& type);

AttrSet ComputeLoweredParamAttrs(const std::string& param_name,
                                 const analysis::TypeRef& type,
                                 PassKind pass_kind,
                                 const LowerCtx* ctx);

AttrSet ComputeSRetParamAttrs(const analysis::TypeRef& ret_type,
                              llvm::Type* ret_llvm_type,
                              const LowerCtx* ctx);

// Create an entry-block alloca instruction for temporary storage
llvm::AllocaInst* CreateEntryAlloca(llvm::Function* func,
                                    llvm::Type* ty,
                                    const std::string& name);

// Coerce a value to target type (bitcast, extend, truncate as needed)
llvm::Value* CoerceValue(llvm::IRBuilderBase* builder,
                         llvm::Value* value,
                         llvm::Type* target);

// -----------------------------------------------------------------------------
// Argument Lowering (§6.12.9)
// -----------------------------------------------------------------------------

// (LLVMArgLower-ByValue-PtrValid)
// StripPerm(T) = TypePtr(U, Valid) ∧ PassKind = ByValue
// ⇒ LLVM arg is ptr type
//
// (LLVMArgLower-ByValue-Other)
// PassKind = ByValue ∧ not PtrValid
// ⇒ LLVM arg is LLVMTy(T)
//
// (LLVMArgLower-ByRef)
// PassKind = ByRef
// ⇒ LLVM arg is opaque ptr
//
// (LLVMArgLower-Err)
// Size computation fails
// ⇒ CodegenError

// Compute LLVM parameter types for a procedure signature
std::optional<std::vector<llvm::Type*>> ComputeLLVMParamTypes(
    LLVMEmitter& emitter,
    const std::vector<IRParam>& params,
    const std::vector<PassKind>& param_kinds,
    bool has_sret);

// -----------------------------------------------------------------------------
// Return Value Lowering (§6.12.9)
// -----------------------------------------------------------------------------

// (LLVMRetLower-SRet)
// PassKind = SRet
// ⇒ LLVM return type is void, first param is sret ptr
//
// (LLVMRetLower-ByValue-ZST)
// sizeof(R) = 0
// ⇒ LLVM return type is void
//
// (LLVMRetLower-ByValue)
// sizeof(R) > 0 ∧ PassKind = ByValue
// ⇒ LLVM return type is LLVMTy(R)
//
// (LLVMRetLower-Err)
// Size computation fails
// ⇒ CodegenError

// Compute LLVM return type for a procedure
std::optional<llvm::Type*> ComputeLLVMReturnType(LLVMEmitter& emitter,
                                                 const analysis::TypeRef& ret_type,
                                                 PassKind ret_kind);

// -----------------------------------------------------------------------------
// Call Emission
// -----------------------------------------------------------------------------

// Emit an ABI-compliant call instruction
// Handles sret allocation, by-ref temporaries, and type coercion
llvm::Value* EmitABICall(LLVMEmitter& emitter,
                         llvm::IRBuilderBase* builder,
                         llvm::Value* callee,
                         const std::vector<IRParam>& params,
                         const analysis::TypeRef& ret_type,
                         const std::vector<llvm::Value*>& args,
                         bool use_c_abi_aggregate_sret = false,
                         bool ffi_import_boundary = false,
                         bool ffi_import_catch = false,
                         std::optional<unsigned> call_conv_override = std::nullopt,
                         const std::vector<IRValue>* source_args = nullptr,
                         llvm::Value** result_storage_out = nullptr,
                         llvm::Value* preferred_result_storage = nullptr,
                         bool foreign_boundary_mode_independent = false);

// -----------------------------------------------------------------------------
// Calling Convention
// -----------------------------------------------------------------------------

// Get the LLVM calling convention ID for Cursive's default ABI
// CallConvDefault = Cursive0ABI = Win64 C calling convention
unsigned GetCursiveCallingConv();

// Check if a function should use the C calling convention
bool IsCCallingConv(std::string_view symbol);

// Check if a function is an extern "C" declaration
bool IsExternC(std::string_view symbol);

}  // namespace cursive::codegen

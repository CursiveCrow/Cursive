// =================================================================
// File: 04_codegen/llvm/llvm_ir_panic.h
// Construct: LLVM IR Panic Emission Utilities
// Spec Section: 6.12
// Spec Rules: PanicRecord, PoisonFlag
// =================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "llvm/IR/IRBuilder.h"

// Forward declarations
namespace llvm {
class GlobalVariable;
class Value;
}  // namespace llvm

namespace cursive0::codegen {

// Forward declarations
class LLVMEmitter;
struct LowerCtx;

// Convert panic reason string to numeric code
std::uint16_t PanicCodeFromString(const std::string& reason);

// Load the panic output pointer from the current function's local
llvm::Value* LoadPanicOutPtr(LLVMEmitter& emitter,
                             llvm::IRBuilder<>* builder);

// Store a panic record with the given code
void StorePanicRecord(LLVMEmitter& emitter,
                      llvm::IRBuilder<>* builder,
                      std::uint16_t code);

// Clear the panic record (set to non-panic state)
void ClearPanicRecord(LLVMEmitter& emitter,
                      llvm::IRBuilder<>* builder);

// Emit a return instruction with default value
void EmitReturn(LLVMEmitter& emitter, llvm::IRBuilder<>* builder);

// Emit conditional panic (sets panic record if condition is false)
void EmitPanicIfFalse(LLVMEmitter& emitter,
                      llvm::IRBuilder<>* builder,
                      llvm::Value* ok,
                      std::uint16_t code);

// Emit conditional panic and return (panics and returns if condition is false)
void EmitPanicReturnIfFalse(LLVMEmitter& emitter,
                            llvm::IRBuilder<>* builder,
                            llvm::Value* ok,
                            std::uint16_t code);

// Get or create a poison flag global for a module
llvm::GlobalVariable* GetOrCreatePoisonFlag(LLVMEmitter& emitter,
                                            const std::vector<std::string>& module_path);

// Split a module path string (e.g. "foo::bar::baz") into components
std::vector<std::string> SplitModulePathString(const std::string& module);

// Check if a function is an init function
bool IsInitFunction(LLVMEmitter& emitter, llvm::Function* func);

// Compute the set of modules to poison on init failure
std::vector<std::string> PoisonSetForInit(const LowerCtx& ctx);

// Store panic record for init function failures
void StoreInitPanicRecord(LLVMEmitter& emitter,
                          llvm::IRBuilder<>* builder);

}  // namespace cursive0::codegen

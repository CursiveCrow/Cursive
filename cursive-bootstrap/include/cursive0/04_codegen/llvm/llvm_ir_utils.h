// =================================================================
// File: 04_codegen/llvm/llvm_ir_utils.h
// Construct: LLVM IR Emission Utilities
// Spec Section: 6.12
// Spec Rules: ConstBytes, TypeUtils
// =================================================================
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/types.h"
#include "cursive0/04_codegen/lower/lower_expr.h"

// LLVM includes for function signatures
#include "llvm/IR/IRBuilder.h"
#include "llvm/ADT/APInt.h"

// Forward declare remaining LLVM types
namespace llvm {
class Constant;
class AllocaInst;
}  // namespace llvm

namespace cursive0::codegen {

// Forward declarations
class LLVMEmitter;
struct LowerCtx;

// Build a ScopeContext from a LowerCtx
analysis::ScopeContext BuildScope(const LowerCtx* ctx);

// Align a value up to a given alignment
std::uint64_t AlignUp(std::uint64_t value, std::uint64_t align);

// Create an APInt from raw bytes (little-endian)
llvm::APInt APIntFromBytes(const std::vector<std::uint8_t>& bytes);

// Check if all bytes are zero
bool AllZero(const std::vector<std::uint8_t>& bytes);

// Create a constant from raw bytes
llvm::Constant* ConstBytes(llvm::Type* ty,
                           const std::vector<std::uint8_t>& bytes,
                           LLVMEmitter& emitter,
                           LowerCtx* lower_ctx);

// Strip permission wrapper from a type
analysis::TypeRef StripPerm(const analysis::TypeRef& type);

// Check if a primitive type name is unsigned
bool IsUnsignedPrim(std::string_view name);

// Check if a type is unsigned
bool IsUnsignedType(const analysis::TypeRef& type);

// Check if a type is unit type ()
bool IsUnitType(const analysis::TypeRef& type);

// Check if a type is never type !
bool IsNeverType(const analysis::TypeRef& type);

// Get element type from pointer type
analysis::TypeRef PtrElementType(const analysis::TypeRef& type);

// Convert integer value to usize (i64)
llvm::Value* AsUSize(llvm::IRBuilder<>* builder,
                     llvm::Value* value,
                     llvm::LLVMContext& ctx);

// Create a byte-offset GEP
llvm::Value* ByteGEP(LLVMEmitter& emitter,
                     llvm::IRBuilder<>* builder,
                     llvm::Value* ptr,
                     std::uint64_t offset);

// Create a byte-offset GEP with runtime offset
llvm::Value* ByteGEP(LLVMEmitter& emitter,
                     llvm::IRBuilder<>* builder,
                     llvm::Value* ptr,
                     llvm::Value* offset);

// Load a value at a byte offset from a pointer
llvm::Value* LoadAtOffset(LLVMEmitter& emitter,
                          llvm::IRBuilder<>* builder,
                          llvm::Value* ptr,
                          std::uint64_t offset,
                          llvm::Type* ty);

// Store a value at a byte offset to a pointer
void StoreAtOffset(LLVMEmitter& emitter,
                   llvm::IRBuilder<>* builder,
                   llvm::Value* ptr,
                   std::uint64_t offset,
                   llvm::Value* value);

// Create an alloca in the entry block
llvm::AllocaInst* CreateEntryAlloca(LLVMEmitter& emitter,
                                    llvm::IRBuilder<>* builder,
                                    llvm::Type* ty,
                                    const std::string& name);

// Get length from a slice/array value
llvm::Value* SliceLenFromValue(LLVMEmitter& emitter,
                               llvm::IRBuilder<>* builder,
                               const IRValue& value,
                               const analysis::TypeRef& type);

// Extract pointer from slice value
llvm::Value* ExtractSlicePtr(LLVMEmitter& emitter,
                             llvm::IRBuilder<>* builder,
                             llvm::Value* slice_val);

// Extract length from slice value
llvm::Value* ExtractSliceLen(LLVMEmitter& emitter,
                             llvm::IRBuilder<>* builder,
                             llvm::Value* slice_val);

}  // namespace cursive0::codegen

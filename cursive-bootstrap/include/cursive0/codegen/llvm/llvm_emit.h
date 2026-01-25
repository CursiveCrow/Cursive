#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <system_error>

#include "cursive0/codegen/ir_model.h"
#include "cursive0/codegen/abi/abi.h"
#include "cursive0/codegen/lower/lower_expr.h"

// Forward declare LLVM types to avoid pulling in all headers here
namespace llvm {
  class LLVMContext;
  class Module;
  class IRBuilderBase;
  template <typename T, typename Inserter> class IRBuilder;
  class Type;

  class Value;
  class Function;
  class BasicBlock;
  class Value;
  class Function;
  class BasicBlock;
  class FunctionType;
  class AttributeList;
}

namespace cursive0::codegen {

struct ABICallResult {
    llvm::FunctionType* func_type = nullptr;
    std::vector<llvm::Type*> param_types;
    llvm::Type* ret_type = nullptr;
    bool has_sret = false;
    std::vector<PassKind> param_kinds;
    std::vector<std::optional<unsigned>> param_indices;
};


class LLVMEmitter {
public:
  // Construct with a context and target info
  LLVMEmitter(llvm::LLVMContext& ctx, const std::string& module_name);
  ~LLVMEmitter();

  // §6.12.9 LLVM IR Emission Pipeline
  
  // T-LLVM-001: Set module header (triple, datalayout)
  void SetupModule();

  // T-LLVM-008: Emit the full module
  // Returns the generated LLVM module on success, or error on failure
  llvm::Module* EmitModule(const IRDecls& decls, LowerCtx& ctx);

  // T-LLVM-007: Type Mapping helpers
  llvm::Type* GetLLVMType(analysis::TypeRef type);
  llvm::Type* GetOpaquePtr(); // T-LLVM-002

  // T-LLVM-006: Runtime Declarations
  void DeclareRuntime();

  // T-LLVM-009 / T-LLVM-010 Helpers
  // Evaluate an IRValue to an llvm::Value*
  llvm::Value* EvaluateIRValue(const IRValue& val);
  
  // T-LLVM-010: Bind local variable
  void EmitBindVar(const IRBindVar& bind);
  
  // T-LLVM-009: Instruction emission
  void EmitIR(const IRPtr& ir);



  // Accessors
  llvm::Module& GetModule() { return *module_; }
  llvm::LLVMContext& GetContext() { return context_; }
  LowerCtx* GetCurrentCtx() { return current_ctx_; }
  const LowerCtx* GetCurrentCtx() const { return current_ctx_; }
  
  // Ownership transfer
  std::unique_ptr<llvm::Module> ReleaseModule();
  
  // T-LLVM-011: Call ABI
  ABICallResult ComputeCallABI(const std::vector<IRParam>& params, analysis::TypeRef ret_type);




  // Builder access (needs casting in impl)
  void* GetBuilderRaw() { return builder_.get(); }
  
  // Local variable management
  void SetLocal(const std::string& name, llvm::Value* val) { locals_[name] = val; }
  llvm::Value* GetLocal(const std::string& name) { return locals_.count(name) ? locals_[name] : nullptr; }
  llvm::Value* GetGlobal(const std::string& name) { return globals_.count(name) ? globals_[name] : nullptr; }
  llvm::Function* GetFunction(const std::string& name) { return functions_.count(name) ? functions_[name] : nullptr; }
  void RemoveLocal(const std::string& name) { locals_.erase(name); }
  void ClearLocals() { locals_.clear(); }

  // Temporary IRValue materialization cache
  void SetTempValue(const IRValue& value, llvm::Value* llvm_value) {
    if (value.kind == IRValue::Kind::Opaque) {
      values_[value.name] = llvm_value;
    }
  }
  llvm::Value* GetTempValue(const IRValue& value) const {
    if (value.kind != IRValue::Kind::Opaque) {
      return nullptr;
    }
    auto it = values_.find(value.name);
    return it != values_.end() ? it->second : nullptr;
  }
  void ClearTempValues() { values_.clear(); }

  // Symbol aliases for IRReadPath resolution
  void SetSymbolAlias(const std::string& name, const std::string& symbol) {
    symbol_aliases_[name] = symbol;
  }
  std::optional<std::string> LookupSymbolAlias(const std::string& name) const {
    auto it = symbol_aliases_.find(name);
    if (it == symbol_aliases_.end()) {
      return std::nullopt;
    }
    return it->second;
  }
  void ClearSymbolAliases() { symbol_aliases_.clear(); }

  // Active region tracking for implicit region allocation
  void PushActiveRegion(const IRValue& region) { active_regions_.push_back(region); }
  void PopActiveRegion() { if (!active_regions_.empty()) active_regions_.pop_back(); }
  const IRValue* CurrentActiveRegion() const {
    return active_regions_.empty() ? nullptr : &active_regions_.back();
  }

  // C0X Extension: Parallel context tracking for spawn/dispatch (§18)
  void PushParallelContext(const IRValue& ctx) { parallel_contexts_.push_back(ctx); }
  void PopParallelContext() { if (!parallel_contexts_.empty()) parallel_contexts_.pop_back(); }
  const IRValue* CurrentParallelContext() const {
    return parallel_contexts_.empty() ? nullptr : &parallel_contexts_.back();
  }

  // T-LLVM-004: Checked Arithmetic (UB/Poison Avoidance)
  llvm::Value* EmitCheckedAdd(llvm::Value* lhs, llvm::Value* rhs, bool is_signed);
  llvm::Value* EmitCheckedSub(llvm::Value* lhs, llvm::Value* rhs, bool is_signed);
  llvm::Value* EmitCheckedMul(llvm::Value* lhs, llvm::Value* rhs, bool is_signed);
  llvm::Value* EmitCheckedDiv(llvm::Value* lhs, llvm::Value* rhs, bool is_signed);
  llvm::Value* EmitCheckedRem(llvm::Value* lhs, llvm::Value* rhs, bool is_signed);
  llvm::Value* EmitCheckedShl(llvm::Value* lhs, llvm::Value* rhs);
  llvm::Value* EmitCheckedShr(llvm::Value* lhs, llvm::Value* rhs, bool is_signed);

  // T-LLVM-005: Memory Intrinsics
  void EmitMemCpy(llvm::Value* dst, llvm::Value* src, llvm::Value* size, uint64_t align = 1);
  void EmitMemSet(llvm::Value* dst, llvm::Value* val, llvm::Value* size, uint64_t align = 1);
  void EmitMemMove(llvm::Value* dst, llvm::Value* src, llvm::Value* size, uint64_t align = 1);


private:
  llvm::LLVMContext& context_;
  std::unique_ptr<llvm::Module> module_;
  std::unique_ptr<llvm::IRBuilderBase> builder_; // Use base class to hide template
  
  LowerCtx* current_ctx_ = nullptr;

  // Track declarations

  std::unordered_map<std::string, llvm::Function*> functions_;
  std::unordered_map<std::string, llvm::Value*> globals_;
  std::unordered_map<std::string, llvm::Value*> locals_;

  std::unordered_map<std::string, llvm::Value*> values_;
  std::unordered_map<std::string, std::string> symbol_aliases_;
  std::vector<IRValue> active_regions_;
  std::vector<IRValue> parallel_contexts_;  // C0X Extension: §18 parallel context stack


  // Type cache
  std::unordered_map<analysis::TypeRef, llvm::Type*> type_cache_;

  // Internal helpers
public:
  void EmitDecl(const IRDecl& decl);
  void EmitProc(const ProcIR& proc);
  void EmitGlobalConst(const GlobalConst& global);
  void EmitGlobalZero(const GlobalZero& global);
  void EmitVTable(const GlobalVTable& vtable);
  
  // T-LLVM-015: Entrypoint Generation
  void EmitEntryPoint();
  
  // T-LLVM-016: Poison Instrumentation
  void EmitPoisonCheck(const std::string& module_name);


  // Attribute helpers (T-LLVM-003)
  void AddPtrAttributes(llvm::Function* func, unsigned arg_idx, analysis::TypeRef type);
  
};
  


// Main entry point for T-LLVM-008
llvm::Module* EmitLLVM(const IRDecls& decls, LowerCtx& ctx, llvm::LLVMContext& llvm_ctx);

} // namespace cursive0::codegen

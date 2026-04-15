#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "05_codegen/ir/ir_model.h"
#include "05_codegen/lower/lower_expr.h"
#include "04_analysis/typing/types.h"
#include "02_source/ast/ast.h"

namespace cursive::codegen {

// ============================================================================
// §6.7 Globals and Initialization
// ============================================================================
// GlobalsJudg = {EmitGlobal, InitFn, DeinitFn, Lower-StaticInit,
//                Lower-StaticInitItem, Lower-StaticInitItems, InitCallIR,
//                Lower-StaticDeinit, Lower-StaticDeinitNames, Lower-StaticDeinitItem,
//                Lower-StaticDeinitItems, DeinitCallIR, EmitInitPlan, EmitDeinitPlan,
//                EmitStringLit, EmitBytesLit, InitPanicHandle}
// ConstInitJudg = {ConstInit}

// ============================================================================
// §6.7 Static Name Extraction
// ============================================================================

// StaticName(binding) - Extract the name from a static binding pattern
// Returns nullopt if the pattern is not a simple identifier or typed pattern
std::optional<std::string> StaticName(const ast::Binding& binding);

// StaticBindList(binding) - Get the list of names bound by a static declaration
std::vector<std::string> StaticBindList(const ast::Binding& binding);

// StaticBindTypes(binding) - Map bound names to their types
std::vector<std::pair<std::string, analysis::TypeRef>> StaticBindTypes(
    const ast::Binding& binding,
    const ast::ModulePath& module_path,
    LowerCtx& ctx);

// ============================================================================
// §6.7 Symbol Generation
// ============================================================================

// StaticSym(item, x) - Compute the mangled symbol for a static binding
// StaticSym(StaticDecl(..., binding,...), x) =
//   Mangle(StaticDecl(...)) if StaticName(binding) = x
//   Mangle(StaticBinding(StaticDecl(...), x)) otherwise
std::string StaticSym(const ast::StaticDecl& decl,
                      const ast::ModulePath& module_path,
                      const std::string& name);

// StaticSymPath(path, name) - Symbol for a static by module path and name
std::string StaticSymPath(const ast::ModulePath& path,
                          const std::string& name);

// ============================================================================
// §6.7 Global Emission
// ============================================================================

// EmitGlobalResult - result of EmitGlobal
struct EmitGlobalResult {
  std::vector<IRDecl> decls;
  bool needs_runtime_init;
};

// (Emit-Static-Const): Constant initializer -> GlobalConst
// (Emit-Static-Init): Non-constant initializer -> GlobalZero
// (Emit-Static-Multi): Destructuring pattern -> multiple globals
EmitGlobalResult EmitGlobal(const ast::StaticDecl& item,
                            const ast::ModulePath& module_path,
                            LowerCtx& ctx);

// ============================================================================
// §6.7 Static Store IR Generation
// ============================================================================

// StaticStoreIR(item, binds) - Generate IR to store pattern case-analysis results
// into static globals
IRPtr StaticStoreIR(const ast::StaticDecl& item,
                    const ast::ModulePath& module_path,
                    const std::vector<std::pair<std::string, IRValue>>& binds);

// ============================================================================
// §6.12.14 String/Bytes Literal Emission
// ============================================================================

// EmitStringLit - Emit a string literal as a global constant
IRDecl EmitStringLit(const std::string& contents);

// EmitBytesLit - Emit a bytes literal as a global constant
IRDecl EmitBytesLit(const std::vector<std::uint8_t>& contents);

// ============================================================================
// §6.7 Static Items Query
// ============================================================================

// StaticItems(P, m) - Get all static declarations in a module
std::vector<const ast::StaticDecl*> StaticItems(
    const ast::ASTModule& module);

// ============================================================================
// §6.7 Init/Deinit Symbol Generation
// ============================================================================

// InitSym(m) = PathSig(["cursive", "runtime", "init"] ++ PathOfModule(m))
std::string InitSym(const ast::ModulePath& module_path);

// DeinitSym(m) = PathSig(["cursive", "runtime", "deinit"] ++ PathOfModule(m))
std::string DeinitSym(const ast::ModulePath& module_path);

// (InitFn): Returns the init function symbol for a module
std::string InitFn(const ast::ModulePath& module_path);

// (DeinitFn): Returns the deinit function symbol for a module
std::string DeinitFn(const ast::ModulePath& module_path);

// ============================================================================
// §6.7 Static Init/Deinit Lowering
// ============================================================================

// (Lower-StaticInitItem): Lower a single static declaration's initializer
IRPtr LowerStaticInitItem(const ast::ModulePath& module_path,
                          const ast::StaticDecl& item,
                          LowerCtx& ctx);

// (Lower-StaticInitItems): Sequence static init items
IRPtr LowerStaticInitItems(const ast::ModulePath& module_path,
                           const std::vector<const ast::StaticDecl*>& items,
                           LowerCtx& ctx);

// (Lower-StaticInit): Entry point for module static initialization
IRPtr LowerStaticInit(const ast::ModulePath& module_path,
                      const ast::ASTModule& module,
                      LowerCtx& ctx);

// (Lower-StaticDeinitItem): Lower a single static declaration's deinitializer
IRPtr LowerStaticDeinitItem(const ast::ModulePath& module_path,
                            const ast::StaticDecl& item,
                            LowerCtx& ctx);

// (Lower-StaticDeinitItems): Sequence static deinit items
IRPtr LowerStaticDeinitItems(const ast::ModulePath& module_path,
                             const std::vector<const ast::StaticDecl*>& items,
                             LowerCtx& ctx);

// (Lower-StaticDeinit): Entry point for module static deinitialization
IRPtr LowerStaticDeinit(const ast::ModulePath& module_path,
                        const ast::ASTModule& module,
                        LowerCtx& ctx);

// (Lower-StaticDeinitNames): Lower deinitialization for named bindings
IRPtr LowerStaticDeinitNames(const ast::ModulePath& module_path,
                             const ast::StaticDecl& item,
                             const std::vector<std::string>& names,
                             LowerCtx& ctx);

// ============================================================================
// §6.7 Init/Deinit Call IR
// ============================================================================

// (InitCallIR): Emit IR to call a module's init function
// InitCallIR(m) = SeqIR(CallIR(InitFn(m), [PanicOutName]), PanicCheck)
IRPtr InitCallIR(const ast::ModulePath& module_path, LowerCtx& ctx);

// (DeinitCallIR): Emit IR to call a module's deinit function
// DeinitCallIR(m) = SeqIR(CallIR(DeinitFn(m), [PanicOutName]), PanicCheck)
IRPtr DeinitCallIR(const ast::ModulePath& module_path, LowerCtx& ctx);

// ============================================================================
// §6.7 Init/Deinit Plan Generation
// ============================================================================

// (EmitInitPlan): Emit initialization calls for all modules in InitOrder
IRPtr EmitInitPlan(const std::vector<ast::ModulePath>& init_order,
                   LowerCtx& ctx);

// (EmitDeinitPlan): Emit deinitialization calls in reverse InitOrder
IRPtr EmitDeinitPlan(const std::vector<ast::ModulePath>& init_order,
                     LowerCtx& ctx);

// ============================================================================
// §6.7 Module Init/Deinit Function IR
// ============================================================================

// Generate the complete IR for a module's init function
ProcIR EmitModuleInitFn(const ast::ModulePath& module_path,
                        const ast::ASTModule& module,
                        LowerCtx& ctx);

// Generate the complete IR for a module's deinit function
ProcIR EmitModuleDeinitFn(const ast::ModulePath& module_path,
                          const ast::ASTModule& module,
                          LowerCtx& ctx);

// ============================================================================
// Spec Rule Anchors
// ============================================================================

void AnchorGlobalsRules();
void AnchorInitRules();

}  // namespace cursive::codegen

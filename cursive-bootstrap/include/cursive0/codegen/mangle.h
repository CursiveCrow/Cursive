#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "cursive0/sema/context.h"
#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::codegen {

// MangleContext provides the scope context needed for symbol mangling.
struct MangleContext {
  const sema::ScopeContext* scope_ctx = nullptr;
};

// =============================================================================
// Item Path Computation
// =============================================================================

// ItemPath(proc) = PathOfModule(ModuleOf(proc)) ++ [name]
std::vector<std::string> ItemPathProc(const syntax::ModulePath& module_path,
                                      const syntax::ProcedureDecl& proc);

// ItemPath(method) = RecordPath(R) ++ [method.name]
std::vector<std::string> ItemPathMethod(const sema::TypePath& record_path,
                                        const syntax::MethodDecl& method);

// ItemPath(class_method) = ClassPath(Cl) ++ [method.name]
std::vector<std::string> ItemPathClassMethod(const sema::TypePath& class_path,
                                             const syntax::ClassMethodDecl& method);

// ItemPath(state_method) = ModalPath(M) ++ [S] ++ [method.name]
std::vector<std::string> ItemPathStateMethod(const sema::TypePath& modal_path,
                                             const std::string& state,
                                             const syntax::StateMethodDecl& method);

// ItemPath(transition) = ModalPath(M) ++ [S] ++ [trans.name]
std::vector<std::string> ItemPathTransition(const sema::TypePath& modal_path,
                                            const std::string& state,
                                            const syntax::TransitionDecl& trans);

// ItemPath(static) = PathOfModule(ModuleOf(static)) ++ [StaticName(binding)]
std::vector<std::string> ItemPathStatic(const syntax::ModulePath& module_path,
                                        const syntax::StaticDecl& decl);

// ItemPath(StaticBinding(decl, x)) = PathOfModule(ModuleOf(decl)) ++ [x]
std::vector<std::string> ItemPathStaticBinding(const syntax::ModulePath& module_path,
                                               const std::string& binding_name);

// ItemPath(VTableDecl(T, Cl)) = ["vtable"] ++ PathOfType(T) ++ ["cl"] ++ ClassPath(Cl)
std::vector<std::string> ItemPathVTable(const sema::TypeRef& type,
                                        const sema::TypePath& class_path);

// ItemPath(DefaultImpl(T, m)) = ["default"] ++ PathOfType(T) ++ ["cl"] ++ ClassPath(Cl) ++ [m.name]
std::vector<std::string> ItemPathDefaultImpl(const sema::TypeRef& type,
                                             const sema::TypePath& class_path,
                                             const std::string& method_name);

// =============================================================================
// PathOfType: Type path encoding for vtable/default impl mangling
// =============================================================================

// Per §6.3.1:
//   PathOfType(TypePrim(name)) = ["prim", name]
//   PathOfType(TypeString(st)) = ["string", TypeStateName(st)]
//   PathOfType(TypeBytes(st)) = ["bytes", TypeStateName(st)]
//   PathOfType(TypePath(p)) = p
//   PathOfType(TypeModalState(p, S)) = p ++ [S]
//   PathOfType(T) = ⊥ otherwise
std::vector<std::string> PathOfType(const sema::TypeRef& type);

// =============================================================================
// Mangle Functions (§6.3.1 Rules)
// =============================================================================

// ScopedSym(item) = PathSig(ItemPath(item))
std::string ScopedSym(const std::vector<std::string>& item_path);

// (Mangle-Proc): Mangle non-main procedure
// (Mangle-Main): Mangle main procedure
std::string MangleProc(const syntax::ModulePath& module_path,
                       const syntax::ProcedureDecl& proc);

// (Mangle-Record-Method): Mangle record method
std::string MangleMethod(const sema::TypePath& record_path,
                         const syntax::MethodDecl& method);

// (Mangle-Class-Method): Mangle class method declaration
std::string MangleClassMethod(const sema::TypePath& class_path,
                              const syntax::ClassMethodDecl& method);

// (Mangle-State-Method): Mangle modal state method
std::string MangleStateMethod(const sema::TypePath& modal_path,
                              const std::string& state,
                              const syntax::StateMethodDecl& method);

// (Mangle-Transition): Mangle modal transition
std::string MangleTransition(const sema::TypePath& modal_path,
                             const std::string& state,
                             const syntax::TransitionDecl& trans);

// (Mangle-Static): Mangle static declaration
std::string MangleStatic(const syntax::ModulePath& module_path,
                         const syntax::StaticDecl& decl);

// (Mangle-StaticBinding): Mangle individual static binding
std::string MangleStaticBinding(const syntax::ModulePath& module_path,
                                const std::string& binding_name);

// (Mangle-VTable): Mangle vtable symbol
std::string MangleVTable(const sema::TypeRef& type,
                         const sema::TypePath& class_path);

// (Mangle-Literal): Mangle literal data symbol
// Per §6.3.1:
//   Mangle(LiteralData(kind, contents)) = PathSig(["cursive", "runtime", "literal", LiteralID(kind, contents)])
std::string MangleLiteral(const std::string& kind,
                          std::span<const std::uint8_t> contents);

// (Mangle-DefaultImpl): Mangle default implementation symbol
std::string MangleDefaultImpl(const sema::TypeRef& type,
                              const sema::TypePath& class_path,
                              const std::string& method_name);

// =============================================================================
// Spec Rule Anchors
// =============================================================================

// Emits SPEC_RULE anchors for §6.3.1 mangle rules.
void AnchorMangleRules();

}  // namespace cursive0::codegen

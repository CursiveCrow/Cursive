#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::codegen {

// MangleContext provides the scope context needed for symbol mangling.
struct MangleContext {
  const analysis::ScopeContext* scope_ctx = nullptr;
};

// =============================================================================
// Item Path Computation
// =============================================================================

// ItemPath(proc) = PathOfModule(ModuleOf(proc)) ++ [name]
std::vector<std::string> ItemPathProc(const syntax::ModulePath& module_path,
                                      const syntax::ProcedureDecl& proc);

// ItemPath(method) = RecordPath(R) ++ [method.name]
std::vector<std::string> ItemPathMethod(const analysis::TypePath& record_path,
                                        const syntax::MethodDecl& method);

// ItemPath(class_method) = ClassPath(Cl) ++ [method.name]
std::vector<std::string> ItemPathClassMethod(const analysis::TypePath& class_path,
                                             const syntax::ClassMethodDecl& method);

// ItemPath(state_method) = ModalPath(M) ++ [S] ++ [method.name]
std::vector<std::string> ItemPathStateMethod(const analysis::TypePath& modal_path,
                                             const std::string& state,
                                             const syntax::StateMethodDecl& method);

// ItemPath(transition) = ModalPath(M) ++ [S] ++ [trans.name]
std::vector<std::string> ItemPathTransition(const analysis::TypePath& modal_path,
                                            const std::string& state,
                                            const syntax::TransitionDecl& trans);

// ItemPath(static) = PathOfModule(ModuleOf(static)) ++ [StaticName(binding)]
std::vector<std::string> ItemPathStatic(const syntax::ModulePath& module_path,
                                        const syntax::StaticDecl& decl);

// ItemPath(StaticBinding(decl, x)) = PathOfModule(ModuleOf(decl)) ++ [x]
std::vector<std::string> ItemPathStaticBinding(const syntax::ModulePath& module_path,
                                               const std::string& binding_name);

// ItemPath(VTableDecl(T, Cl)) = ["vtable"] ++ PathOfType(T) ++ ["cl"] ++ ClassPath(Cl)
std::vector<std::string> ItemPathVTable(const analysis::TypeRef& type,
                                        const analysis::TypePath& class_path);

// ItemPath(DefaultImpl(T, m)) = ["default"] ++ PathOfType(T) ++ ["cl"] ++ ClassPath(Cl) ++ [m.name]
std::vector<std::string> ItemPathDefaultImpl(const analysis::TypeRef& type,
                                             const analysis::TypePath& class_path,
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
std::vector<std::string> PathOfType(const analysis::TypeRef& type);

// =============================================================================
// §6.3.1 LinkName - FFI attribute-aware symbol resolution
// =============================================================================

// LinkName resolves the link symbol name for a declaration based on attributes:
//   - If [[symbol("name")]] present, return specified name
//   - If [[no_mangle]] present, return raw identifier
//   - If [[export(link_name="name")]] present, return link_name
//   - Otherwise, return std::nullopt (caller should use mangled name)
std::optional<std::string> LinkName(const syntax::AttributeList& attrs,
                                    const std::string& raw_name);

// =============================================================================
// Mangle Functions (§6.3.1 Rules)
// =============================================================================

// ScopedSym(item) = PathSig(ItemPath(item))
std::string ScopedSym(const std::vector<std::string>& item_path);

// (Mangle-Proc): Mangle non-main procedure
// (Mangle-Main): Mangle main procedure
// With attribute-aware LinkName resolution
std::string MangleProc(const syntax::ModulePath& module_path,
                       const syntax::ProcedureDecl& proc);

// (Mangle-Record-Method): Mangle record method
std::string MangleMethod(const analysis::TypePath& record_path,
                         const syntax::MethodDecl& method);

// (Mangle-Class-Method): Mangle class method declaration
std::string MangleClassMethod(const analysis::TypePath& class_path,
                              const syntax::ClassMethodDecl& method);

// (Mangle-State-Method): Mangle modal state method
std::string MangleStateMethod(const analysis::TypePath& modal_path,
                              const std::string& state,
                              const syntax::StateMethodDecl& method);

// (Mangle-Transition): Mangle modal transition
std::string MangleTransition(const analysis::TypePath& modal_path,
                             const std::string& state,
                             const syntax::TransitionDecl& trans);

// (Mangle-Static): Mangle static declaration
std::string MangleStatic(const syntax::ModulePath& module_path,
                         const syntax::StaticDecl& decl);

// (Mangle-StaticBinding): Mangle individual static binding
std::string MangleStaticBinding(const syntax::ModulePath& module_path,
                                const std::string& binding_name);

// (Mangle-VTable): Mangle vtable symbol
std::string MangleVTable(const analysis::TypeRef& type,
                         const analysis::TypePath& class_path);

// (Mangle-Literal): Mangle literal data symbol
// Per §6.3.1:
//   Mangle(LiteralData(kind, contents)) = PathSig(["cursive", "runtime", "literal", LiteralID(kind, contents)])
std::string MangleLiteral(const std::string& kind,
                          std::span<const std::uint8_t> contents);

// (Mangle-DefaultImpl): Mangle default implementation symbol
std::string MangleDefaultImpl(const analysis::TypeRef& type,
                              const analysis::TypePath& class_path,
                              const std::string& method_name);

// =============================================================================
// Spec Rule Anchors
// =============================================================================

// Emits SPEC_RULE anchors for §6.3.1 mangle rules.
void AnchorMangleRules();

}  // namespace cursive0::codegen

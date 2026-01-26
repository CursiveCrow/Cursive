#include "cursive0/codegen/mangle.h"

#include <optional>
#include <variant>

#include "cursive0/core/assert_spec.h"
#include "cursive0/core/hash.h"
#include "cursive0/core/symbols.h"

namespace cursive0::codegen {

// =============================================================================
// §6.3.1 LinkName - FFI attribute-aware symbol resolution
// =============================================================================

std::optional<std::string> LinkName(const syntax::AttributeList& attrs,
                                    const std::string& raw_name) {
  SPEC_DEF("LinkName", "6.3.1");

  for (const auto& attr : attrs) {
    // [[symbol("name")]] - use exact specified symbol name
    if (attr.name == "symbol") {
      SPEC_RULE("LinkName-Symbol");
      for (const auto& arg : attr.args) {
        if (const auto* tok = std::get_if<syntax::Token>(&arg.value)) {
          if (tok->kind == syntax::TokenKind::StringLiteral) {
            // Remove quotes from string literal
            std::string_view sv = tok->lexeme;
            if (sv.size() >= 2 && sv.front() == '"' && sv.back() == '"') {
              return std::string(sv.substr(1, sv.size() - 2));
            }
            return std::string(sv);
          }
        }
      }
    }

    // [[no_mangle]] - use raw identifier as symbol
    if (attr.name == "no_mangle") {
      SPEC_RULE("LinkName-NoMangle");
      return raw_name;
    }

    // [[export(link_name="name")]] - use link_name value
    if (attr.name == "export") {
      for (const auto& arg : attr.args) {
        if (arg.key.has_value() && *arg.key == "link_name") {
          SPEC_RULE("LinkName-Export");
          if (const auto* tok = std::get_if<syntax::Token>(&arg.value)) {
            if (tok->kind == syntax::TokenKind::StringLiteral) {
              std::string_view sv = tok->lexeme;
              if (sv.size() >= 2 && sv.front() == '"' && sv.back() == '"') {
                return std::string(sv.substr(1, sv.size() - 2));
              }
              return std::string(sv);
            }
          }
        }
      }
    }
  }

  // No FFI attribute found - use standard mangling
  return std::nullopt;
}

namespace {

// TypeStateName per §6.3.1:
//   TypeStateName(View) = "view"
//   TypeStateName(Managed) = "managed"
std::string TypeStateName(analysis::StringState state) {
  switch (state) {
    case analysis::StringState::View:
      return "view";
    case analysis::StringState::Managed:
      return "managed";
  }
  return "";
}

std::string TypeStateName(analysis::BytesState state) {
  switch (state) {
    case analysis::BytesState::View:
      return "view";
    case analysis::BytesState::Managed:
      return "managed";
  }
  return "";
}

// Extract name from a simple binding pattern.
// StaticName(binding) = name if IdentifierPattern(name) or TypedPattern(name, _)
// StaticName(binding) = ⊥ otherwise
std::optional<std::string> StaticName(const syntax::Binding& binding) {
  SPEC_DEF("StaticName", "6.7.2");

  if (!binding.pat) {
    return std::nullopt;
  }

  if (const auto* ident =
          std::get_if<syntax::IdentifierPattern>(&binding.pat->node)) {
    return ident->name;
  }

  if (const auto* typed =
          std::get_if<syntax::TypedPattern>(&binding.pat->node)) {
    return typed->name;
  }

  return std::nullopt;
}

}  // namespace

// =============================================================================
// Item Path Computation
// =============================================================================

std::vector<std::string> ItemPathProc(const syntax::ModulePath& module_path,
                                      const syntax::ProcedureDecl& proc) {
  SPEC_DEF("ItemPath", "6.3.1");
  SPEC_DEF("PathOfModule", "3.4.1");

  std::vector<std::string> path = module_path;
  path.push_back(proc.name);
  return path;
}

std::vector<std::string> ItemPathMethod(const analysis::TypePath& record_path,
                                        const syntax::MethodDecl& method) {
  SPEC_DEF("ItemPath", "6.3.1");
  SPEC_DEF("RecordPath", "5.3.1");

  std::vector<std::string> path = record_path;
  path.push_back(method.name);
  return path;
}

std::vector<std::string> ItemPathClassMethod(const analysis::TypePath& class_path,
                                             const syntax::ClassMethodDecl& method) {
  SPEC_DEF("ItemPath", "6.3.1");
  SPEC_DEF("ClassPath", "5.4.1");

  std::vector<std::string> path = class_path;
  path.push_back(method.name);
  return path;
}

std::vector<std::string> ItemPathStateMethod(const analysis::TypePath& modal_path,
                                             const std::string& state,
                                             const syntax::StateMethodDecl& method) {
  SPEC_DEF("ItemPath", "6.3.1");
  SPEC_DEF("ModalPath", "5.4.2");

  std::vector<std::string> path = modal_path;
  path.push_back(state);
  path.push_back(method.name);
  return path;
}

std::vector<std::string> ItemPathTransition(const analysis::TypePath& modal_path,
                                            const std::string& state,
                                            const syntax::TransitionDecl& trans) {
  SPEC_DEF("ItemPath", "6.3.1");
  SPEC_DEF("ModalPath", "5.4.2");

  std::vector<std::string> path = modal_path;
  path.push_back(state);
  path.push_back(trans.name);
  return path;
}

std::vector<std::string> ItemPathStatic(const syntax::ModulePath& module_path,
                                        const syntax::StaticDecl& decl) {
  SPEC_DEF("ItemPath", "6.3.1");

  std::vector<std::string> path = module_path;
  const auto name = StaticName(decl.binding);
  if (name.has_value()) {
    path.push_back(*name);
  }
  return path;
}

std::vector<std::string> ItemPathStaticBinding(const syntax::ModulePath& module_path,
                                               const std::string& binding_name) {
  SPEC_DEF("ItemPath", "6.3.1");
  SPEC_DEF("StaticBinding", "6.7.2");

  std::vector<std::string> path = module_path;
  path.push_back(binding_name);
  return path;
}

std::vector<std::string> ItemPathVTable(const analysis::TypeRef& type,
                                        const analysis::TypePath& class_path) {
  SPEC_DEF("ItemPath", "6.3.1");
  SPEC_DEF("VTableDecl", "6.3.1");

  // ItemPath(VTableDecl(T, Cl)) = ["vtable"] ++ PathOfType(T) ++ ["cl"] ++ ClassPath(Cl)
  std::vector<std::string> path;
  path.push_back("vtable");

  const auto type_path = PathOfType(type);
  path.insert(path.end(), type_path.begin(), type_path.end());

  path.push_back("cl");
  path.insert(path.end(), class_path.begin(), class_path.end());

  return path;
}

std::vector<std::string> ItemPathDefaultImpl(const analysis::TypeRef& type,
                                             const analysis::TypePath& class_path,
                                             const std::string& method_name) {
  SPEC_DEF("ItemPath", "6.3.1");
  SPEC_DEF("DefaultImpl", "6.3.1");

  // ItemPath(DefaultImpl(T, m)) = ["default"] ++ PathOfType(T) ++ ["cl"] ++ ClassPath(Cl) ++ [m.name]
  std::vector<std::string> path;
  path.push_back("default");

  const auto type_path = PathOfType(type);
  path.insert(path.end(), type_path.begin(), type_path.end());

  path.push_back("cl");
  path.insert(path.end(), class_path.begin(), class_path.end());

  path.push_back(method_name);

  return path;
}

// =============================================================================
// PathOfType
// =============================================================================

std::vector<std::string> PathOfType(const analysis::TypeRef& type) {
  SPEC_DEF("PathOfType", "6.3.1");
  SPEC_DEF("TypeStateName", "6.3.1");

  if (!type) {
    return {};
  }

  // PathOfType(TypePrim(name)) = ["prim", name]
  if (const auto* prim = std::get_if<analysis::TypePrim>(&type->node)) {
    return {"prim", prim->name};
  }

  // PathOfType(TypeString(st)) = ["string", TypeStateName(st)]
  if (const auto* str = std::get_if<analysis::TypeString>(&type->node)) {
    if (str->state.has_value()) {
      return {"string", TypeStateName(*str->state)};
    }
    // If state is not specified, use a default representation
    return {"string", "modal"};
  }

  // PathOfType(TypeBytes(st)) = ["bytes", TypeStateName(st)]
  if (const auto* bytes = std::get_if<analysis::TypeBytes>(&type->node)) {
    if (bytes->state.has_value()) {
      return {"bytes", TypeStateName(*bytes->state)};
    }
    return {"bytes", "modal"};
  }

  // PathOfType(TypePath(p)) = p
  if (const auto* path_type = std::get_if<analysis::TypePathType>(&type->node)) {
    return path_type->path;
  }

  // PathOfType(TypeModalState(p, S)) = p ++ [S]
  if (const auto* modal_state = std::get_if<analysis::TypeModalState>(&type->node)) {
    std::vector<std::string> result = modal_state->path;
    result.push_back(modal_state->state);
    return result;
  }

  // PathOfType(T) = ⊥ otherwise
  return {};
}

// =============================================================================
// ScopedSym
// =============================================================================

std::string ScopedSym(const std::vector<std::string>& item_path) {
  SPEC_DEF("ScopedSym", "6.3.1");
  SPEC_DEF("PathSig", "6.3.1");

  // ScopedSym(item) = PathSig(ItemPath(item))
  // PathSig(p) = mangle(StringOfPath(p))
  return core::Mangle(core::StringOfPath(item_path));
}

// =============================================================================
// Mangle Functions
// =============================================================================

std::string MangleProc(const syntax::ModulePath& module_path,
                       const syntax::ProcedureDecl& proc) {
  // §6.3.1 Check FFI attributes first
  if (auto link_name = LinkName(proc.attrs, proc.name)) {
    SPEC_RULE("Mangle-Proc-LinkName");
    return *link_name;
  }

  if (proc.name == "main") {
    SPEC_RULE("Mangle-Main");
  } else {
    SPEC_RULE("Mangle-Proc");
  }

  return ScopedSym(ItemPathProc(module_path, proc));
}

std::string MangleMethod(const analysis::TypePath& record_path,
                         const syntax::MethodDecl& method) {
  SPEC_RULE("Mangle-Record-Method");

  return ScopedSym(ItemPathMethod(record_path, method));
}

std::string MangleClassMethod(const analysis::TypePath& class_path,
                              const syntax::ClassMethodDecl& method) {
  SPEC_RULE("Mangle-Class-Method");

  return ScopedSym(ItemPathClassMethod(class_path, method));
}

std::string MangleStateMethod(const analysis::TypePath& modal_path,
                              const std::string& state,
                              const syntax::StateMethodDecl& method) {
  SPEC_RULE("Mangle-State-Method");

  return ScopedSym(ItemPathStateMethod(modal_path, state, method));
}

std::string MangleTransition(const analysis::TypePath& modal_path,
                             const std::string& state,
                             const syntax::TransitionDecl& trans) {
  SPEC_RULE("Mangle-Transition");

  return ScopedSym(ItemPathTransition(modal_path, state, trans));
}

std::string MangleStatic(const syntax::ModulePath& module_path,
                         const syntax::StaticDecl& decl) {
  SPEC_RULE("Mangle-Static");

  return ScopedSym(ItemPathStatic(module_path, decl));
}

std::string MangleStaticBinding(const syntax::ModulePath& module_path,
                                const std::string& binding_name) {
  SPEC_RULE("Mangle-StaticBinding");

  return ScopedSym(ItemPathStaticBinding(module_path, binding_name));
}

std::string MangleVTable(const analysis::TypeRef& type,
                         const analysis::TypePath& class_path) {
  SPEC_RULE("Mangle-VTable");

  return ScopedSym(ItemPathVTable(type, class_path));
}

std::string MangleLiteral(const std::string& kind,
                          std::span<const std::uint8_t> contents) {
  SPEC_RULE("Mangle-Literal");
  SPEC_DEF("LiteralData", "6.3.1");

  // Mangle(LiteralData(kind, contents)) = PathSig(["cursive", "runtime", "literal", LiteralID(kind, contents)])
  const std::string literal_id = core::LiteralID(kind, contents);
  return ScopedSym({"cursive", "runtime", "literal", literal_id});
}

std::string MangleDefaultImpl(const analysis::TypeRef& type,
                              const analysis::TypePath& class_path,
                              const std::string& method_name) {
  SPEC_RULE("Mangle-DefaultImpl");

  return ScopedSym(ItemPathDefaultImpl(type, class_path, method_name));
}

// =============================================================================
// Spec Rule Anchors
// =============================================================================

void AnchorMangleRules() {
  // §6.3.1 Mangle Rules
  SPEC_RULE("Mangle-Proc");
  SPEC_RULE("Mangle-Main");
  SPEC_RULE("Mangle-Record-Method");
  SPEC_RULE("Mangle-Class-Method");
  SPEC_RULE("Mangle-State-Method");
  SPEC_RULE("Mangle-Transition");
  SPEC_RULE("Mangle-Static");
  SPEC_RULE("Mangle-StaticBinding");
  SPEC_RULE("Mangle-VTable");
  SPEC_RULE("Mangle-Literal");
  SPEC_RULE("Mangle-DefaultImpl");
}

}  // namespace cursive0::codegen

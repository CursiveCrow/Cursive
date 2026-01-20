#include "cursive0/codegen/globals.h"

#include <cstring>
#include <variant>

#include "cursive0/codegen/mangle.h"
#include "cursive0/codegen/layout.h"
#include "cursive0/core/symbols.h"
#include "cursive0/sema/type_pattern.h"
#include "cursive0/core/assert_spec.h"

namespace cursive0::codegen {

// ============================================================================
// Helper functions
// ============================================================================

// Helpers moved to lower_expr.h
static std::optional<std::vector<std::uint8_t>> ConstInit(sema::TypeRef type,
                                                          const syntax::Expr& expr) {
  if (!type) {
    return std::nullopt;
  }
  if (const auto* lit = std::get_if<syntax::LiteralExpr>(&expr.node)) {
    SPEC_RULE("ConstInit");
    return EncodeConst(type, lit->literal);
  }
  return std::nullopt;
}

// ============================================================================
// ยง6.7 Static Name Extraction
// ============================================================================

std::optional<std::string> StaticName(const syntax::Binding& binding) {
  SPEC_DEF("StaticName", "ยง6.7");
  
  if (!binding.pat) {
    return std::nullopt;
  }
  
  // StaticName(binding) = name if IdentifierPattern(name)
  if (const auto* ident =
          std::get_if<syntax::IdentifierPattern>(&binding.pat->node)) {
    return ident->name;
  }
  
  // StaticName(binding) = name if TypedPattern(name, _)
  if (const auto* typed =
          std::get_if<syntax::TypedPattern>(&binding.pat->node)) {
    return typed->name;
  }
  
  // StaticName(binding) = โ�ฅ otherwise
  return std::nullopt;
}

// Helper to recursively extract names from a pattern
static void CollectPatternNames(const syntax::Pattern& pat,
                                std::vector<std::string>& names) {
  if (const auto* ident =
          std::get_if<syntax::IdentifierPattern>(&pat.node)) {
    names.push_back(ident->name);
    return;
  }
  
  if (const auto* typed =
          std::get_if<syntax::TypedPattern>(&pat.node)) {
    names.push_back(typed->name);
    return;
  }
  
  if (const auto* tuple =
          std::get_if<syntax::TuplePattern>(&pat.node)) {
    for (const auto& elem : tuple->elements) {
      if (elem) {
        CollectPatternNames(*elem, names);
      }
    }
    return;
  }
  
  if (const auto* record =
          std::get_if<syntax::RecordPattern>(&pat.node)) {
    for (const auto& field : record->fields) {
      if (field.pattern_opt) {
        CollectPatternNames(*field.pattern_opt, names);
      }
    }
    return;
  }
  
  // Wildcard and other patterns don't bind names
}

std::vector<std::string> StaticBindList(const syntax::Binding& binding) {
  SPEC_DEF("StaticBindList", "ยง6.7");
  
  std::vector<std::string> names;
  if (binding.pat) {
    CollectPatternNames(*binding.pat, names);
  }
  return names;
}
// StaticBindTypes(binding) = B when BindType(binding) matches pattern
std::vector<std::pair<std::string, sema::TypeRef>> StaticBindTypes(
    const syntax::Binding& binding,
    const syntax::ModulePath& module_path,
    LowerCtx& ctx) {

  if (!binding.pat) {
    return {};
  }

  sema::TypeRef bind_type;
  if (binding.init && ctx.expr_type) {
    bind_type = ctx.expr_type(*binding.init);
  }
  if (!bind_type && binding.type_opt && ctx.sigma) {
    sema::ScopeContext scope;
    scope.sigma = *ctx.sigma;
    scope.current_module = module_path;
    if (auto lowered = LowerTypeForLayout(scope, binding.type_opt)) {
      bind_type = *lowered;
    }
  }
  if (!bind_type) {
    return {};
  }

  sema::ScopeContext scope;
  if (ctx.sigma) {
    scope.sigma = *ctx.sigma;
    scope.current_module = module_path;
  }
  const auto match = sema::TypeMatchPattern(scope, binding.pat, bind_type);
  if (!match.ok) {
    return {};
  }
  return match.bindings;
}


// ============================================================================
// ยง6.7 Symbol Generation
// ============================================================================

std::string StaticSym(const syntax::StaticDecl& decl,
                      const syntax::ModulePath& module_path,
                      const std::string& name) {
  SPEC_DEF("StaticSym", "ยง6.7");
  
  // StaticSym(StaticDecl(..., binding,...), x) =
  //   Mangle(StaticDecl(...)) if StaticName(binding) = x
  //   Mangle(StaticBinding(StaticDecl(...), x)) otherwise
  
  auto static_name = StaticName(decl.binding);
  if (static_name.has_value() && *static_name == name) {
    return MangleStatic(module_path, decl);
  }
  
  return MangleStaticBinding(module_path, name);
}

std::string StaticSymPath(const syntax::ModulePath& path,
                          const std::string& name) {
  SPEC_DEF("StaticSymPath", "ยง6.7");
  
  // StaticSymPath(path, name) uses MangleStaticBinding
  return MangleStaticBinding(path, name);
}

// ============================================================================
// ยง6.7 Global Emission
// ============================================================================

EmitGlobalResult EmitGlobal(const syntax::StaticDecl& item,
                            const syntax::ModulePath& module_path,
                            LowerCtx& ctx) {
  EmitGlobalResult result;
  result.needs_runtime_init = false;

  const auto& binding = item.binding;
  auto static_name = StaticName(binding);

  sema::TypeRef init_type;
  if (binding.init && ctx.expr_type) {
    init_type = ctx.expr_type(*binding.init);
  }
  if (!init_type && binding.type_opt && ctx.sigma) {
    sema::ScopeContext scope;
    scope.sigma = *ctx.sigma;
    scope.current_module = module_path;
    if (auto lowered = LowerTypeForLayout(scope, binding.type_opt)) {
      init_type = *lowered;
    }
  }

  sema::ScopeContext layout_scope;
  if (ctx.sigma) {
    layout_scope.sigma = *ctx.sigma;
    layout_scope.current_module = module_path;
  }

  // Check if this is a simple binding
  if (static_name.has_value()) {
    std::string sym = StaticSym(item, module_path, *static_name);

    if (init_type) {
      ctx.RegisterStaticType(sym, init_type);
      ctx.RegisterStaticModule(sym, module_path);
    }

    if (binding.init) {
      if (auto bytes = ConstInit(init_type, *binding.init)) {
        SPEC_RULE("Emit-Static-Const");
        GlobalConst gc;
        gc.symbol = sym;
        gc.bytes = std::move(*bytes);
        result.decls.push_back(std::move(gc));
        return result;
      }

      SPEC_RULE("Emit-Static-Init");
      GlobalZero gz;
      gz.symbol = sym;
      const auto size = SizeOf(layout_scope, init_type);
      if (!size.has_value()) {
        ctx.ReportCodegenFailure();
        return result;
      }
      gz.size = *size;
      result.decls.push_back(std::move(gz));
      result.needs_runtime_init = true;
      return result;
    }

    SPEC_RULE("Emit-Static-Init");
    GlobalZero gz;
    gz.symbol = sym;
    const auto size = SizeOf(layout_scope, init_type);
    if (!size.has_value()) {
      ctx.ReportCodegenFailure();
      return result;
    }
    gz.size = *size;
    result.decls.push_back(std::move(gz));
    return result;
  }

  // (Emit-Static-Multi) - destructuring pattern with multiple bindings
  SPEC_RULE("Emit-Static-Multi");

  auto names = StaticBindList(binding);
  auto bind_types = StaticBindTypes(binding, module_path, ctx);
  for (const auto& name : names) {
    std::string sym = StaticSym(item, module_path, name);
    sema::TypeRef type;
    for (const auto& [bind_name, bind_type] : bind_types) {
      if (bind_name == name) {
        type = bind_type;
        break;
      }
    }
    if (type) {
      ctx.RegisterStaticType(sym, type);
      ctx.RegisterStaticModule(sym, module_path);
    }
    GlobalZero gz;
    gz.symbol = sym;
    const auto size = SizeOf(layout_scope, type);
    if (!size.has_value()) {
      ctx.ReportCodegenFailure();
      result.decls.clear();
      return result;
    }
    gz.size = *size;
    result.decls.push_back(std::move(gz));
  }

  result.needs_runtime_init = true;
  return result;
}

// ============================================================================
// ยง6.7 Static Store IR Generation
// ============================================================================

IRPtr StaticStoreIR(const syntax::StaticDecl& item,
                    const syntax::ModulePath& module_path,
                    const std::vector<std::pair<std::string, IRValue>>& binds) {
  SPEC_DEF("StaticStoreIR", "ยง6.7");
  
  // StaticStoreIR(item, []) = ฮต
  if (binds.empty()) {
    return EmptyIR();
  }
  
  // StaticStoreIR(item, [โ�จx, vโ�ฉ] ++ bs) = SeqIR(StoreGlobal(StaticSym(item, x), v), StaticStoreIR(item, bs))
  std::vector<IRPtr> stores;
  stores.reserve(binds.size());
  
  for (const auto& [name, value] : binds) {
    std::string sym = StaticSym(item, module_path, name);
    IRStoreGlobal store;
    store.symbol = sym;
    store.value = value;
    stores.push_back(MakeIR(std::move(store)));
  }
  
  return SeqIR(std::move(stores));
}

// ============================================================================
// ยง6.12.14 String/Bytes Literal Emission
// ============================================================================

IRDecl EmitStringLit(const std::string& contents) {
  SPEC_DEF("EmitStringLit", "ยง6.12.14");
  
  // Generate a unique symbol for this string literal
  std::vector<std::uint8_t> bytes(contents.begin(), contents.end());
  std::string sym = MangleLiteral("string", bytes);
  
  GlobalConst gc;
  gc.symbol = sym;
  gc.bytes = std::move(bytes);
  return gc;
}

IRDecl EmitBytesLit(const std::vector<std::uint8_t>& contents) {
  SPEC_DEF("EmitBytesLit", "ยง6.12.14");
  SPEC_RULE("EmitLiteral-Bytes");
  
  // Generate a unique symbol for this bytes literal
  std::string sym = MangleLiteral("bytes", contents);
  
  GlobalConst gc;
  gc.symbol = sym;
  gc.bytes = contents;
  return gc;
}

// ============================================================================
// ยง6.7 Static Items Query
// ============================================================================

std::vector<const syntax::StaticDecl*> StaticItems(
    const syntax::ASTModule& module) {
  SPEC_DEF("StaticItems", "ยง6.7");
  
  std::vector<const syntax::StaticDecl*> items;
  
  for (const auto& item : module.items) {
    if (const auto* static_decl = std::get_if<syntax::StaticDecl>(&item)) {
      items.push_back(static_decl);
    }
  }
  
  return items;
}

// ============================================================================
// Spec Rule Anchors
// ============================================================================

void AnchorGlobalsRules() {
  // ยง6.7 Globals and Initialization
  SPEC_RULE("Emit-Static-Const");
  SPEC_RULE("Emit-Static-Init");
  SPEC_RULE("Emit-Static-Multi");
  
  // Definitions
  SPEC_DEF("StaticName", "ยง6.7");
  SPEC_DEF("StaticBindList", "ยง6.7");
  SPEC_DEF("StaticBindTypes", "ยง6.7");
  SPEC_DEF("StaticSym", "ยง6.7");
  SPEC_DEF("StaticSymPath", "ยง6.7");
  SPEC_DEF("StaticStoreIR", "ยง6.7");
  SPEC_DEF("StaticItems", "ยง6.7");
  SPEC_DEF("EmitStringLit", "ยง6.12.14");
  SPEC_DEF("EmitBytesLit", "ยง6.12.14");
}

}  // namespace cursive0::codegen

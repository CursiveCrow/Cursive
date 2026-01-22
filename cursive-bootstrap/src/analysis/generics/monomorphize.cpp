#include "cursive0/analysis/generics/monomorphize.h"

#include <algorithm>

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/types/type_equiv.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsMonomorphize() {
  SPEC_DEF("Instantiate", "C0X.5.Y");
  SPEC_DEF("InstGraph", "C0X.5.Y");
  SPEC_DEF("Monomorphize", "C0X.5.Y");
  SPEC_DEF("TypeSubst", "C0X.5.Y");
}

// Compare type keys for ordering
bool TypeKeyLessForInst(const TypeRef& a, const TypeRef& b) {
  if (!a && !b) return false;
  if (!a) return true;
  if (!b) return false;
  return TypeKeyLess(TypeKeyOf(a), TypeKeyOf(b));
}

}  // namespace

bool InstantiationKey::operator<(const InstantiationKey& other) const {
  if (decl_path < other.decl_path) return true;
  if (other.decl_path < decl_path) return false;
  
  if (args.size() != other.args.size()) {
    return args.size() < other.args.size();
  }
  
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (TypeKeyLessForInst(args[i], other.args[i])) return true;
    if (TypeKeyLessForInst(other.args[i], args[i])) return false;
  }
  
  return false;
}

bool InstantiationKey::operator==(const InstantiationKey& other) const {
  if (decl_path != other.decl_path) return false;
  if (args.size() != other.args.size()) return false;
  
  for (std::size_t i = 0; i < args.size(); ++i) {
    auto equiv = TypeEquiv(args[i], other.args[i]);
    if (!equiv.equiv) return false;
  }
  
  return true;
}

void MonomorphizeContext::Demand(const InstantiationKey& key) {
  SpecDefsMonomorphize();
  SPEC_RULE("Mono-Demand");
  
  auto it = instantiations_.find(key);
  if (it != instantiations_.end()) {
    return;  // Already demanded
  }
  
  InstantiationEntry entry;
  entry.key = key;
  entry.processed = false;
  instantiations_[key] = entry;
  worklist_.push_back(key);
}

bool MonomorphizeContext::HasInstantiation(const InstantiationKey& key) const {
  return instantiations_.find(key) != instantiations_.end();
}

bool MonomorphizeContext::ProcessToFixedPoint() {
  SpecDefsMonomorphize();
  SPEC_RULE("Mono-FixedPoint");
  
  while (!worklist_.empty()) {
    if (current_depth_ >= kMaxDepth) {
      // Non-terminating type-level recursion
      SPEC_RULE("Mono-NonTerminating");
      return false;
    }
    
    InstantiationKey key = worklist_.back();
    worklist_.pop_back();
    
    auto it = instantiations_.find(key);
    if (it == instantiations_.end() || it->second.processed) {
      continue;
    }
    
    ++current_depth_;
    
    // Mark as processed
    it->second.processed = true;
    
    // Dependencies would be discovered during instantiation
    // (not implemented here - would require full AST traversal)
    
    --current_depth_;
  }
  
  return true;
}

TypeRef InstantiateType(const TypeRef& type, const TypeSubst& subst) {
  SpecDefsMonomorphize();
  SPEC_RULE("Instantiate-Type");
  
  if (!type) {
    return type;
  }
  
  return std::visit(
      [&](const auto& node) -> TypeRef {
        using T = std::decay_t<decltype(node)>;
        
        if constexpr (std::is_same_v<T, TypePathType>) {
          // Check if this is a type parameter
          if (node.path.size() == 1) {
            auto it = subst.find(node.path[0]);
            if (it != subst.end()) {
              return it->second;
            }
          }
          
          // Instantiate generic args if any
          if (node.generic_args.empty()) {
            return type;
          }
          
          std::vector<TypeRef> new_args;
          new_args.reserve(node.generic_args.size());
          for (const auto& arg : node.generic_args) {
            new_args.push_back(InstantiateType(arg, subst));
          }
          
          TypePathType new_node = node;
          new_node.generic_args = std::move(new_args);
          return MakeType(new_node);
        }
        else if constexpr (std::is_same_v<T, TypePerm>) {
          return MakeTypePerm(node.perm, InstantiateType(node.base, subst));
        }
        else if constexpr (std::is_same_v<T, TypeTuple>) {
          std::vector<TypeRef> new_elems;
          new_elems.reserve(node.elements.size());
          for (const auto& elem : node.elements) {
            new_elems.push_back(InstantiateType(elem, subst));
          }
          return MakeTypeTuple(std::move(new_elems));
        }
        else if constexpr (std::is_same_v<T, TypeArray>) {
          return MakeTypeArray(InstantiateType(node.element, subst), node.length);
        }
        else if constexpr (std::is_same_v<T, TypeSlice>) {
          return MakeTypeSlice(InstantiateType(node.element, subst));
        }
        else if constexpr (std::is_same_v<T, TypePtr>) {
          return MakeTypePtr(InstantiateType(node.element, subst), node.state);
        }
        else if constexpr (std::is_same_v<T, TypeRawPtr>) {
          return MakeTypeRawPtr(node.qual, InstantiateType(node.element, subst));
        }
        else if constexpr (std::is_same_v<T, TypeUnion>) {
          std::vector<TypeRef> new_members;
          new_members.reserve(node.members.size());
          for (const auto& member : node.members) {
            new_members.push_back(InstantiateType(member, subst));
          }
          return MakeTypeUnion(std::move(new_members));
        }
        else if constexpr (std::is_same_v<T, TypeFunc>) {
          std::vector<TypeFuncParam> new_params;
          new_params.reserve(node.params.size());
          for (const auto& param : node.params) {
            new_params.push_back(TypeFuncParam{
                param.mode,
                InstantiateType(param.type, subst)});
          }
          return MakeTypeFunc(std::move(new_params),
                              InstantiateType(node.ret, subst));
        }
        else {
          return type;
        }
      },
      type->node);
}

TypeSubst BuildSubstitution(
    const std::vector<syntax::TypeParam>& params,
    const std::vector<TypeRef>& args) {
  SpecDefsMonomorphize();
  SPEC_RULE("Build-Subst");
  
  TypeSubst subst;
  
  for (std::size_t i = 0; i < params.size() && i < args.size(); ++i) {
    subst[params[i].name] = args[i];
  }
  
  // Fill in defaults for missing args
  for (std::size_t i = args.size(); i < params.size(); ++i) {
    if (params[i].default_type) {
      // Lower AST type to analysis type (simplified - real impl would use full lowering)
      // For now, just skip defaults
    }
  }
  
  return subst;
}

BoundCheckResult CheckBoundsSatisfied(
    const std::vector<syntax::TypeParam>& params,
    const std::vector<TypeRef>& args) {
  SpecDefsMonomorphize();
  SPEC_RULE("Check-Bounds");
  
  BoundCheckResult result;
  
  // For each parameter with bounds, check the corresponding argument satisfies them
  for (std::size_t i = 0; i < params.size() && i < args.size(); ++i) {
    const auto& param = params[i];
    
    for (const auto& bound : param.bounds) {
      // Check if args[i] implements bound.class_path
      // This requires class implementation lookup (simplified here)
      // Full implementation would check Implements(Γ, args[i], bound.class_path)
      
      // For now, always succeed (real impl needs class resolution)
    }
  }
  
  return result;
}

InferResult InferTypeArguments(
    const std::vector<syntax::TypeParam>& params,
    const std::vector<TypeRef>& expected_param_types,
    const std::vector<TypeRef>& actual_arg_types,
    const std::optional<TypeRef>& expected_return) {
  SpecDefsMonomorphize();
  SPEC_RULE("Infer-TypeArgs");
  
  InferResult result;
  result.inferred_args.resize(params.size());
  
  // Simple unification-based inference
  // For each parameter position, try to unify expected with actual
  // and extract type variable bindings
  
  std::map<std::string, TypeRef> bindings;
  
  // Infer from arguments
  for (std::size_t i = 0; i < expected_param_types.size() && 
                          i < actual_arg_types.size(); ++i) {
    // Match expected against actual to extract bindings
    // This is a simplified version - real impl needs full unification
    
    const auto& expected = expected_param_types[i];
    const auto& actual = actual_arg_types[i];
    
    if (!expected || !actual) continue;
    
    // If expected is a type parameter, bind it
    if (const auto* path = std::get_if<TypePathType>(&expected->node)) {
      if (path->path.size() == 1) {
        // Check if it's a type parameter
        for (const auto& param : params) {
          if (param.name == path->path[0]) {
            bindings[param.name] = actual;
            break;
          }
        }
      }
    }
  }
  
  // Build inferred args from bindings and defaults
  for (std::size_t i = 0; i < params.size(); ++i) {
    auto it = bindings.find(params[i].name);
    if (it != bindings.end()) {
      result.inferred_args[i] = it->second;
    } else if (params[i].default_type) {
      // Use default (would need lowering)
      result.inferred_args[i] = nullptr;
    } else {
      // Could not infer
      result.ok = false;
      result.diag_id = "E-TYP-INFER";
    }
  }
  
  return result;
}

}  // namespace cursive0::analysis

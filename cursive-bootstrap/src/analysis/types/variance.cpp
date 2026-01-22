#include "cursive0/analysis/types/variance.h"

#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/types/type_equiv.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsVariance() {
  SPEC_DEF("Variance", "C0X.5.Y");
  SPEC_DEF("VarMut", "C0X.5.Y");
  SPEC_DEF("VarConst", "C0X.5.Y");
  SPEC_DEF("T-Gen-Sub", "C0X.5.Y");
}

}  // namespace

Variance CombineVariance(Variance outer, Variance inner) {
  SpecDefsVariance();
  SPEC_RULE("Var-Combine");
  
  // Bivariant dominates
  if (outer == Variance::Bivariant || inner == Variance::Bivariant) {
    return Variance::Bivariant;
  }
  
  // Invariant absorbs
  if (outer == Variance::Invariant || inner == Variance::Invariant) {
    return Variance::Invariant;
  }
  
  // Co + Co = Co, Contra + Contra = Co
  if (outer == inner) {
    return Variance::Covariant;
  }
  
  // Co + Contra = Contra, Contra + Co = Contra
  return Variance::Contravariant;
}

Variance InvertVariance(Variance v) {
  SpecDefsVariance();
  SPEC_RULE("Var-Invert");
  
  switch (v) {
    case Variance::Covariant:
      return Variance::Contravariant;
    case Variance::Contravariant:
      return Variance::Covariant;
    case Variance::Invariant:
      return Variance::Invariant;
    case Variance::Bivariant:
      return Variance::Bivariant;
  }
  return Variance::Invariant;
}

Variance VarianceOf(const TypeRef& type, const std::string& param_name) {
  SpecDefsVariance();
  SPEC_RULE("Var-Of");
  
  if (!type) {
    return Variance::Bivariant;  // Unused
  }
  
  return std::visit(
      [&](const auto& node) -> Variance {
        using T = std::decay_t<decltype(node)>;
        
        if constexpr (std::is_same_v<T, TypePathType>) {
          // Check if this is the type parameter itself
          if (node.path.size() == 1 && node.path[0] == param_name) {
            return Variance::Covariant;
          }
          
          // Check generic arguments
          Variance result = Variance::Bivariant;
          for (const auto& arg : node.generic_args) {
            Variance arg_var = VarianceOf(arg, param_name);
            if (arg_var != Variance::Bivariant) {
              if (result == Variance::Bivariant) {
                result = arg_var;
              } else if (result != arg_var) {
                result = Variance::Invariant;
              }
            }
          }
          return result;
        }
        else if constexpr (std::is_same_v<T, TypePerm>) {
          // Permission qualifiers: unique/shared make it invariant
          if (node.perm == Permission::Unique || node.perm == Permission::Shared) {
            return VarMut(node.base, param_name);
          }
          return VarConst(node.base, param_name);
        }
        else if constexpr (std::is_same_v<T, TypeFunc>) {
          // Function types: parameters are contravariant, return is covariant
          Variance result = Variance::Bivariant;
          
          // Parameters (contravariant)
          for (const auto& param : node.params) {
            Variance param_var = VarianceOf(param.type, param_name);
            param_var = InvertVariance(param_var);
            result = CombineVariance(result, param_var);
          }
          
          // Return type (covariant)
          Variance ret_var = VarianceOf(node.ret, param_name);
          result = CombineVariance(result, ret_var);
          
          return result;
        }
        else if constexpr (std::is_same_v<T, TypeTuple>) {
          // Tuple elements are covariant
          Variance result = Variance::Bivariant;
          for (const auto& elem : node.elements) {
            result = CombineVariance(result, VarianceOf(elem, param_name));
          }
          return result;
        }
        else if constexpr (std::is_same_v<T, TypeArray>) {
          // Array element is invariant (can read and write)
          return VarMut(node.element, param_name);
        }
        else if constexpr (std::is_same_v<T, TypeSlice>) {
          // Slice element is invariant
          return VarMut(node.element, param_name);
        }
        else if constexpr (std::is_same_v<T, TypePtr>) {
          // Pointer element variance depends on state
          return VarianceOf(node.element, param_name);
        }
        else if constexpr (std::is_same_v<T, TypeUnion>) {
          // Union members are covariant
          Variance result = Variance::Bivariant;
          for (const auto& member : node.members) {
            result = CombineVariance(result, VarianceOf(member, param_name));
          }
          return result;
        }
        else {
          return Variance::Bivariant;
        }
      },
      type->node);
}

Variance VarMut(const TypeRef& type, const std::string& param_name) {
  SpecDefsVariance();
  SPEC_RULE("Var-Mut");
  
  // Mutable storage position is invariant
  Variance base = VarianceOf(type, param_name);
  if (base == Variance::Bivariant) {
    return Variance::Bivariant;
  }
  return Variance::Invariant;
}

Variance VarConst(const TypeRef& type, const std::string& param_name) {
  SpecDefsVariance();
  SPEC_RULE("Var-Const");
  
  // Immutable storage is covariant
  return VarianceOf(type, param_name);
}

VarianceContext ComputeVarianceContext(
    const std::vector<syntax::TypeParam>& params,
    const std::vector<TypeRef>& member_types) {
  SpecDefsVariance();
  SPEC_RULE("Var-Context");
  
  VarianceContext ctx;
  
  for (const auto& param : params) {
    Variance combined = Variance::Bivariant;
    
    for (const auto& member_type : member_types) {
      Variance member_var = VarianceOf(member_type, param.name);
      combined = CombineVariance(combined, member_var);
    }
    
    ctx.param_variance[param.name] = combined;
  }
  
  return ctx;
}

bool CheckGenericSubtyping(
    const VarianceContext& ctx,
    const std::vector<TypeRef>& args1,
    const std::vector<TypeRef>& args2) {
  SpecDefsVariance();
  SPEC_RULE("T-Gen-Sub");
  
  if (args1.size() != args2.size()) {
    return false;
  }
  
  std::size_t i = 0;
  for (const auto& [param_name, variance] : ctx.param_variance) {
    if (i >= args1.size()) {
      break;
    }
    
    const auto& a1 = args1[i];
    const auto& a2 = args2[i];
    
    switch (variance) {
      case Variance::Covariant: {
        // a1 <: a2 required (checked elsewhere)
        break;
      }
      case Variance::Contravariant: {
        // a2 <: a1 required (checked elsewhere)
        break;
      }
      case Variance::Invariant: {
        auto equiv = TypeEquiv(a1, a2);
        if (!equiv.equiv) {
          return false;
        }
        break;
      }
      case Variance::Bivariant: {
        // Always ok
        break;
      }
    }
    
    ++i;
  }
  
  return true;
}

}  // namespace cursive0::analysis

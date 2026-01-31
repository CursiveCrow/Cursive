// =================================================================
// File: 03_analysis/types/type_predicates.h
// Construct: Type Predicates (Bitcopy, Clone, Eq, Ord, Cast)
// Spec Section: 5.2.12
// Spec Rules: BitcopyType, CloneType, EqType, OrdType, CastValid
// =================================================================
#pragma once

#include "cursive0/03_analysis/types/context.h"
#include "cursive0/03_analysis/types/types.h"

namespace cursive0::analysis {

// Check if a type satisfies Bitcopy predicate
// §12884-12887: Structural derivation for Records, Enums, Modals
bool BitcopyType(const ScopeContext& ctx, const TypeRef& type);

// Check if a type satisfies Clone predicate
bool CloneType(const ScopeContext& ctx, const TypeRef& type);

// Check if a type supports equality comparison
bool EqType(const TypeRef& type);

// Check if a type supports ordering comparison
bool OrdType(const TypeRef& type);

// Check if a cast from source to target is valid
bool CastValid(const TypeRef& source, const TypeRef& target);

// Strip permission wrapper from a type
TypeRef StripPerm(const TypeRef& type);

}  // namespace cursive0::analysis

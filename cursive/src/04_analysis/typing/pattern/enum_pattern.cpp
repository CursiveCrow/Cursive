// =============================================================================
// MIGRATION MAPPING: pattern/enum_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.4: Pattern Matching
//   - Enum pattern typing
//   - Variant matching
//   - Payload extraction
//   - Let-Refutable-Pattern-Err (line 9773)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_pattern.cpp
//   Enum pattern typing
//
// KEY CONTENT TO MIGRATE:
//   ENUM PATTERN (Enum::Variant | Enum::Variant(p1, p2)):
//   1. Resolve enum and variant
//   2. Check scrutinee type matches enum
//   3. For unit variant: no payload
//   4. For tuple variant: check payload patterns
//   5. Collect bindings from payload patterns
//   6. Pattern is refutable (variant may not match)
//
//   TYPING:
//   Enum = scrutinee type
//   Variant in Enum.variants
//   Payload patterns match variant payload types
//   --------------------------------------------------
//   Gamma |- Enum::Variant(p1, ...) <= EnumType => ok, bindings
//
//   UNIT VARIANT PATTERN:
//   Enum::Variant  // no payload
//   - Matches variant with no associated data
//   - No bindings from variant itself
//
//   TUPLE VARIANT PATTERN:
//   Enum::Variant(p1, p2, ...)
//   - Matches variant with payload
//   - Sub-patterns typed against payload types
//   - Bindings collected from sub-patterns
//
//   REFUTABILITY:
//   - Enum patterns are refutable
//   - Cannot be used in let/var directly
//   - Must be in match expression
//   - Other variants must be covered
//
//   EXHAUSTIVENESS:
//   - All enum variants must be covered
//   - Or wildcard pattern as catch-all
//   - See match_check.cpp for exhaustiveness
//
//   EXAMPLE:
//   match result {
//       Result::Ok(value) => { use(value) },
//       Result::Err(e) => { handle(e) }
//   }
//
// DEPENDENCIES:
//   - EnumLookup for variant resolution
//   - PatternTypeFn for payload patterns
//   - RefutableContext verification
//   - BindingCollection for payload bindings
//
// SPEC RULES:
//   - Enum pattern semantics
//   - Let-Refutable-Pattern-Err (line 9773)
//
// RESULT TYPE:
//   PatternResult { ok: true, bindings: from_payloads }
//
// NOTES:
//   - Refutable - requires match context
//   - Payload patterns can bind values
//   - Exhaustiveness checked at match level
//
// =============================================================================


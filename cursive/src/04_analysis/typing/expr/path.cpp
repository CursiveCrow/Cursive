// =============================================================================
// MIGRATION MAPPING: expr/path.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2: Static Semantics - Path Resolution
//   - Qualified name resolution
//   - Module path lookup
//   - Type path vs value path
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Path expression typing
//
// KEY CONTENT TO MIGRATE:
//   PATH EXPRESSION (module::item | Type::method):
//   1. Parse path components
//   2. Resolve each segment
//   3. Determine final item kind
//   4. Return appropriate type
//
//   PATH FORMS:
//   - module::function - procedure reference
//   - module::Type - type reference
//   - Type::method - associated function
//   - module::CONSTANT - static value
//
//   RESOLUTION:
//   1. Start at current scope or absolute root
//   2. For each segment:
//      a. Look up in current namespace
//      b. Check visibility
//      c. Advance to nested scope
//   3. Final segment determines type
//
//   TYPE VS VALUE PATHS:
//   - Type path: resolves to type
//   - Value path: resolves to value/function
//   - Context determines which
//
//   VISIBILITY:
//   - Each segment must be visible from caller
//   - Private items block path resolution
//
//   EXAMPLE:
//   std::io::read_file       // procedure path
//   cursive::runtime::string // type path
//   MyType::new              // associated function
//
// DEPENDENCIES:
//   - NameResolver for segment lookup
//   - VisibilityCheck for access validation
//   - TypeOrValueContext determination
//
// SPEC RULES:
//   - Path resolution semantics
//   - Visibility rules
//
// RESULT TYPE:
//   Type of resolved item
//
// =============================================================================


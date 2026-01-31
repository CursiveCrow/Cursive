// =============================================================================
// MIGRATION MAPPING: expr/enum_literal.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.2: Static Semantics - Enum Literals
//   - Enum variant construction
//   - Qualified name for variants
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_infer.cpp
//   Enum literal typing
//
// KEY CONTENT TO MIGRATE:
//   ENUM LITERAL (Enum::Variant | Enum::Variant(args)):
//   1. Resolve enum type and variant
//   2. For unit variant: no arguments
//   3. For tuple variant: type arguments against payload types
//   4. Result is the enum type
//
//   TYPING:
//   EnumDecl(path) = E
//   Variant in E.variants
//   For tuple variant: ArgsOk(args, payload_types)
//   --------------------------------------------------
//   Gamma |- Enum::Variant(args) : EnumType
//
//   UNIT VARIANT:
//   Direction::North
//   - No payload, no parentheses
//   - Type is just the enum type
//
//   TUPLE VARIANT:
//   Result::Ok(42)
//   - Arguments match payload types
//   - Positional, not named
//
//   TYPE INFERENCE:
//   - Enum type may be inferred from context
//   - Variant name must be unambiguous
//
//   EXAMPLE:
//   let d: Direction = Direction::North
//   let r: Result = Result::Ok(42)
//   let e: Option = Option::Some(value)
//
// DEPENDENCIES:
//   - EnumLookup for resolution
//   - VariantPayloadTypes for argument checking
//   - ArgsOk for argument typing
//
// SPEC RULES:
//   - Enum literal semantics
//   - ArgsOk for payload
//
// RESULT TYPE:
//   Enum type
//
// =============================================================================


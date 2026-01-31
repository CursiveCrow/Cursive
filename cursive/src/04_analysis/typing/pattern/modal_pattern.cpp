// =============================================================================
// MIGRATION MAPPING: pattern/modal_pattern.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.4: Pattern Matching
//   - Modal state pattern typing
//   - State-specific field extraction
//   - Let-Refutable-Pattern-Err (line 9773)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_pattern.cpp
//   Modal pattern typing
//
// KEY CONTENT TO MIGRATE:
//   MODAL PATTERN (@StateName | @StateName{ fields }):
//   1. Resolve modal type and state
//   2. Check scrutinee type is the modal type
//   3. For simple: just state match
//   4. For with fields: check field patterns
//   5. Collect bindings from field patterns
//   6. Pattern is refutable (state may not match)
//
//   TYPING:
//   Modal = scrutinee type
//   @StateName in Modal.states
//   Field patterns match state-specific fields
//   --------------------------------------------------
//   Gamma |- @StateName{ fields } <= ModalType => ok, bindings
//
//   SIMPLE STATE PATTERN:
//   @Connected  // just check state
//   - Matches modal in that state
//   - No bindings from state itself
//
//   STATE WITH FIELDS:
//   @Connected{ socket }
//   - Matches state and extracts fields
//   - Fields are state-specific
//   - Bindings from field patterns
//
//   REFUTABILITY:
//   - Modal patterns are refutable
//   - Cannot be used in let/var directly
//   - Must be in match expression
//   - Other states must be covered
//
//   EXAMPLE:
//   match connection {
//       @Connected{ socket } => { use(socket) },
//       @Disconnected{ host } => { reconnect(host) }
//   }
//
// DEPENDENCIES:
//   - ModalLookup for state resolution
//   - StateFieldLookup for field types
//   - PatternTypeFn for field patterns
//   - RefutableContext verification
//
// SPEC RULES:
//   - Modal pattern semantics
//   - Let-Refutable-Pattern-Err (line 9773)
//
// RESULT TYPE:
//   PatternResult { ok: true, bindings: from_fields }
//
// NOTES:
//   - Refutable - requires match context
//   - State-specific fields only accessible in that arm
//   - All states must be covered for exhaustiveness
//
// =============================================================================


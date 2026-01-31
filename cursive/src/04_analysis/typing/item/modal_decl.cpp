// =============================================================================
// MIGRATION MAPPING: item/modal_decl.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.3.4: Modal Declarations
//   - WF-ModalDecl (line 10554): Modal well-formedness
//   - WF-ModalState (line 12444): State well-formedness
//   - WF-ModalState-ArgCount-Err (line 12449): State argument error
//   - WF-Modal-Payload (line 12300): Payload well-formedness
//   - modal_decl grammar (line 3103)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_decls.cpp
//   Modal declaration typing
//
// KEY CONTENT TO MIGRATE:
//   MODAL DECLARATION:
//   1. Process generic parameters (if any)
//   2. Process class implementations (<:)
//   3. Process where clauses
//   4. Process state blocks:
//      a. Each @StateName block
//      b. State-specific fields
//      c. State-specific methods
//      d. Transition procedures
//   5. Process type invariant (if any)
//   6. Register modal in environment
//
//   TYPING RULE (WF-ModalDecl):
//   Generic params well-formed
//   All state blocks well-formed
//   State names distinct
//   Transitions valid
//   --------------------------------------------------
//   Gamma |- modal Name<params> <: classes where {...} { states }
//
//   STATE BLOCK:
//   @StateName {
//       field: Type
//       procedure method(~) -> T { ... }
//       transition next_state() -> @OtherState { ... }
//   }
//
//   STATE-SPECIFIC FIELDS:
//   - Fields declared in state only accessible in that state
//   - Different states may have different fields
//   - Field access requires modal to be in correct state
//
//   TRANSITIONS:
//   - transition keyword for state-changing procedures
//   - Return type is @NewState
//   - Consumes current state, produces new state
//   - Must construct new state with required fields
//
//   BUILT-IN MODALS:
//   - Region: @Active, @Frozen, @Freed
//   - CancelToken: @Active, @Cancelled
//   - Spawned<T>: @Pending, @Ready
//   - Async<Out, In, Result, E>: @Suspended, @Completed, @Failed
//
// DEPENDENCIES:
//   - ProcessGenericParams()
//   - StateBlockTyping()
//   - TransitionTyping()
//   - FieldTyping()
//   - MethodTyping()
//   - TypeInvariantCheck()
//
// SPEC RULES:
//   - WF-ModalDecl (line 10554)
//   - WF-ModalState (line 12444)
//   - WF-Modal-Payload (line 12300)
//
// RESULT:
//   Modal declaration added to environment
//   Diagnostics for any errors
//
// =============================================================================


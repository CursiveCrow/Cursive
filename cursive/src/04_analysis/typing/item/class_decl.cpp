// =============================================================================
// MIGRATION MAPPING: item/class_decl.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.3.5: Class Declarations
//   - WF-ClassDecl (line 10564): Class well-formedness
//   - WF-Class (line 11964, 22625): Class requirements
//   - WF-Class-Method (line 11949): Method well-formedness
//   - WF-Class-Self (line 22652): Self type in classes
//   - class_decl grammar (line 3104)
//   - class_item grammar (line 3130)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/types/type_decls.cpp
//   Class declaration typing
//
// KEY CONTENT TO MIGRATE:
//   CLASS DECLARATION:
//   1. Process generic parameters (if any)
//   2. Process superclass bounds (<: SuperClass)
//   3. Process where clauses
//   4. Process class items:
//      a. Method declarations (abstract or with default)
//      b. Associated type declarations
//      c. Abstract field declarations
//      d. Abstract state declarations (for modal classes)
//   5. Register class in environment
//
//   TYPING RULE (WF-ClassDecl):
//   Generic params well-formed
//   Superclass bounds valid
//   All methods well-formed
//   Associated types declared
//   --------------------------------------------------
//   Gamma |- class Name<params> <: super where {...} { items }
//
//   CLASS ITEMS:
//   - Method decl: procedure name(~, ...) -> T
//   - Associated type: type Item
//   - Abstract field: field_name: T
//   - Abstract state: @StateName
//
//   SELF TYPE (line 22652):
//   - Within class, Self denotes the implementing type
//   - Self is unknown/abstract
//   - Used in method signatures
//
//   METHOD REQUIREMENTS:
//   - Receiver shorthand required (~, ~!, ~%)
//   - Return type specified
//   - May have default implementation
//
//   MODAL CLASSES:
//   - modal class keyword for state machine classes
//   - Declares abstract states
//   - Implementers must provide concrete states
//
//   SUPERCLASS BOUNDS:
//   - Class may extend other classes (<: SuperClass)
//   - Inherits method requirements
//   - Must satisfy all superclass methods
//
// DEPENDENCIES:
//   - ProcessGenericParams()
//   - ProcessSuperclassBounds()
//   - MethodDeclTyping()
//   - AssociatedTypeDecl()
//   - ModalClassState()
//
// SPEC RULES:
//   - WF-ClassDecl (line 10564)
//   - WF-Class (line 11964, 22625)
//   - WF-Class-Method (line 11949)
//   - WF-Class-Self (line 22652)
//
// RESULT:
//   Class declaration added to environment
//   Diagnostics for any errors
//
// =============================================================================


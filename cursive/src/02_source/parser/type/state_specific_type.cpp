// =============================================================================
// MIGRATION MAPPING: state_specific_type.cpp
// =============================================================================
// This file should contain parsing logic for state-specific modal types:
// ModalType@State representing a modal type in a specific state.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.7, Lines 4803-4806
// Lines 4688-4690 (Modal Type References)
// =============================================================================
//
// FORMAL RULES FROM SPEC:
// -----------------------------------------------------------------------------
//
// **(Parse-Modal-State-Type)** Lines 4803-4806
// Γ ⊢ ParseModalTypeRef(P) ⇓ (P_1, modal_ref)
// IsOp(Tok(P_1), "@")
// Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, state)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseNonPermType(P) ⇓ (P_2, TypeModalState(modal_ref, state))
//
// MODAL TYPE REFERENCE (Lines 4688-4690):
// -----------------------------------------------------------------------------
// ParseModalTypeRef(P) ⇓ (P_2, tr) ⇔
//   Γ ⊢ ParseTypePath(P) ⇓ (P_1, path) ∧
//   Γ ⊢ ParseGenericArgsOpt(P_1) ⇓ (P_2, args_opt) ∧
//   tr = (TypePath(path) if args_opt = ⊥ else TypeApply(path, args_opt))
//
// SEMANTICS:
// - State-specific types pin a modal type to a known state
// - Syntax: ModalType@StateName, e.g., Connection@Connected
// - With generics: Container<T>@Loaded
// - The state must match a state defined in the modal type
// - Examples: Connection@Connected, CancelToken@Active, Region@Frozen
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_types.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. Modal state type parsing in ParseNonPermType (Lines 636-647)
//    ─────────────────────────────────────────────────────────────────────────
//    const Token* after_args = Tok(cur);
//    if (after_args && IsOpTok(*after_args, "@")) {
//      SPEC_RULE("Parse-Modal-State-Type");
//      Parser after_at = cur;
//      Advance(after_at);  // consume @
//      ParseElemResult<Identifier> state = ParseIdent(after_at);
//      TypeModalState modal;
//      modal.path = std::move(path.elem);
//      modal.generic_args = std::move(args);
//      modal.state = state.elem;
//      return {state.parser, MakeTypeNode(SpanBetween(start, state.parser), modal)};
//    }
//
// 2. Generic args parsing before state check (Lines 607-634)
//    ─────────────────────────────────────────────────────────────────────────
//    // Parse optional generic args, then optional modal state.
//    std::vector<std::shared_ptr<Type>> args;
//    Parser cur = path.parser;
//    const Token* next = Tok(cur);
//    if (next && IsOpTok(*next, "<")) {
//      SPEC_RULE("Parse-Type-Generic-Args");
//      Parser after_lt = cur;
//      Advance(after_lt);  // consume <
//
//      // Parse first type arg
//      ParseElemResult<std::shared_ptr<Type>> first_arg = ParseType(after_lt);
//      args.push_back(first_arg.elem);
//      cur = first_arg.parser;
//
//      // Parse additional args separated by ; or ,
//      while (IsPunc(cur, ";") || IsPunc(cur, ",")) {
//        Advance(cur);
//        ParseElemResult<std::shared_ptr<Type>> arg = ParseType(cur);
//        args.push_back(arg.elem);
//        cur = arg.parser;
//      }
//
//      // Expect >
//      if (!IsOp(cur, ">")) {
//        EmitParseSyntaxErr(cur, TokSpan(cur));
//      } else {
//        Advance(cur);
//      }
//    }
//
// PARSING FLOW:
// -----------------------------------------------------------------------------
//   1. Parse type path (e.g., Connection, my_module::MyModal)
//   2. Parse optional generic arguments (e.g., <T>)
//   3. Check for "@" operator
//   4. If "@" found:
//      a. Advance past "@"
//      b. Parse state name as identifier
//      c. Construct TypeModalState node
//   5. If no "@", fall through to TypePath handling
//
// DEPENDENCIES:
// =============================================================================
// - ParseTypePath function (type_path.cpp)
// - ParseGenericArgsOpt function (type_args.cpp)
// - ParseIdent function (common identifier parsing)
// - MakeTypeNode helper (type_common.cpp)
// - TypeModalState AST node type
// - Identifier type
// - Token predicates: IsOpTok, IsPunc
// - Parser utilities: Tok, Advance, SpanBetween, TokSpan
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - The "@" operator is overloaded for multiple uses (modal state, ptr state)
// - Context determines whether @ is state-specific or pointer state
// - Modal types without @State represent any-state (runtime checked)
// - State names are identifiers, not strings
// - Generic args use , for arguments (not ; like parameters in declarations)
// - Built-in modals: Region, CancelToken, Spawned<T>, Tracked<T,E>, Async<...>
// - Span covers from type path start to end of state identifier
// =============================================================================

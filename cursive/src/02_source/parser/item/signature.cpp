// =============================================================================
// MIGRATION MAPPING: Procedure Signature Parsing
// =============================================================================
// Destination: cursive/src/02_source/parser/item/signature.cpp
// Source:      cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 592-777)
//
// PURPOSE:
// Parse procedure signatures including parameters and return types.
// Handles both regular procedures and method signatures with receivers.
// =============================================================================

// =============================================================================
// SPEC RULES IMPLEMENTED
// =============================================================================
// From C0updated.md Section 3.3.6.13:
//
// (Parse-Signature)
// IsPunc(Tok(P), "(")    Γ ⊢ ParseParamList(Advance(P)) ⇓ (P_1, params)
// IsPunc(Tok(P_1), ")")    Γ ⊢ ParseReturnOpt(Advance(P_1)) ⇓ (P_2, ret_opt)
// ────────────────────────────────────────────────────────────────────────────
// Γ ⊢ ParseSignature(P) ⇓ (P_2, params, ret_opt)
//
// (Parse-ReturnOpt-None)
// ¬ IsOp(Tok(P), "->")
// ──────────────────────────────────────────────
// Γ ⊢ ParseReturnOpt(P) ⇓ (P, ⊥)
//
// (Parse-ReturnOpt-Arrow)
// IsOp(Tok(P), "->")    Γ ⊢ ParseType(Advance(P)) ⇓ (P_1, ty)
// ────────────────────────────────────────────────────────────────
// Γ ⊢ ParseReturnOpt(P) ⇓ (P_1, ty)

// =============================================================================
// SOURCE CODE MAPPING
// =============================================================================

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 592-602)
FUNCTION: ParseReturnOpt

ORIGINAL:
ParseElemResult<std::shared_ptr<Type>> ParseReturnOpt(Parser parser) {
  if (!IsOp(parser, "->")) {
    SPEC_RULE("Parse-ReturnOpt-None");
    return {parser, nullptr};
  }
  SPEC_RULE("Parse-ReturnOpt-Arrow");
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(next);
  return {ty.parser, ty.elem};
}

IMPLEMENTATION NOTES:
- Returns nullptr if no -> arrow present
- Otherwise parses type after ->
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 604-613)
FUNCTION: ParseParamModeOpt

ORIGINAL:
ParseElemResult<std::optional<ParamMode>> ParseParamModeOpt(Parser parser) {
  if (IsKw(parser, "move")) {
    SPEC_RULE("Parse-ParamMode-Move");
    Parser next = parser;
    Advance(next);
    return {next, ParamMode::Move};
  }
  SPEC_RULE("Parse-ParamMode-None");
  return {parser, std::nullopt};
}

IMPLEMENTATION NOTES:
- Only "move" mode is currently supported
- Returns nullopt for non-move (default pass by reference)
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 615-632)
FUNCTION: ParseParam

ORIGINAL:
ParseElemResult<Param> ParseParam(Parser parser) {
  SPEC_RULE("Parse-Param");
  Parser start = parser;
  ParseElemResult<std::optional<ParamMode>> mode = ParseParamModeOpt(parser);
  ParseElemResult<Identifier> name = ParseIdent(mode.parser);
  if (!IsPunc(name.parser, ":")) {
    EmitParseSyntaxErr(name.parser, TokSpan(name.parser));
  } else {
    Advance(name.parser);
  }
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(name.parser);
  Param param;
  param.mode = mode.elem;
  param.name = name.elem;
  param.type = ty.elem;
  param.span = SpanBetween(start, ty.parser);
  return {ty.parser, param};
}

IMPLEMENTATION NOTES:
- Parameter format: [move] name: Type
- The mode is optional (defaults to non-consuming)
- Colon is required between name and type
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 634-672)
FUNCTIONS: ParseParamTail, ParseParamList

IMPLEMENTATION NOTES:
- Parameters are comma-separated
- Handles trailing commas with error (disallowed on single line)
- Empty parameter list is valid
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 689-727)
FUNCTION: ParseReceiver

ORIGINAL:
ParseElemResult<Receiver> ParseReceiver(Parser parser) {
  if (IsOp(parser, "~")) {
    SPEC_RULE("Parse-Receiver-Short-Const");
    Parser next = parser;
    Advance(next);
    return {next, ReceiverShorthand{ReceiverPerm::Const}};
  }
  if (IsOp(parser, "~!")) {
    SPEC_RULE("Parse-Receiver-Short-Unique");
    Parser next = parser;
    Advance(next);
    return {next, ReceiverShorthand{ReceiverPerm::Unique}};
  }
  // C0X Extension: shared receiver shorthand
  if (IsOp(parser, "~%")) {
    SPEC_RULE("Parse-Receiver-Short-Shared");
    Parser next = parser;
    Advance(next);
    return {next, ReceiverShorthand{ReceiverPerm::Shared}};
  }

  SPEC_RULE("Parse-Receiver-Explicit");
  Parser start = parser;
  ParseElemResult<std::optional<ParamMode>> mode = ParseParamModeOpt(parser);
  ParseElemResult<Identifier> name = ParseIdent(mode.parser);
  if (!(name.elem == "self")) {
    EmitParseSyntaxErr(start, TokSpan(start));
  }
  if (!IsPunc(name.parser, ":")) {
    EmitParseSyntaxErr(name.parser, TokSpan(name.parser));
  } else {
    Advance(name.parser);
  }
  ParseElemResult<std::shared_ptr<Type>> ty = ParseType(name.parser);
  ReceiverExplicit recv;
  recv.mode_opt = mode.elem;
  recv.type = ty.elem;
  return {ty.parser, recv};
}

IMPLEMENTATION NOTES:
- Shorthand receivers: ~ (const), ~! (unique), ~% (shared)
- Explicit receiver: [move] self: Type
- Shorthand binds "self" automatically
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 736-753)
FUNCTION: ParseMethodSignature

IMPLEMENTATION NOTES:
- Method signature = receiver + parameters + optional return
- Format: (receiver, param1, param2) -> ReturnType
- Receiver comes first, followed by comma and regular parameters
*/

/*
SOURCE: cursive-bootstrap/src/02_syntax/parser_items.cpp (lines 761-777)
FUNCTION: ParseSignature

ORIGINAL:
SignatureResult ParseSignature(Parser parser) {
  SPEC_RULE("Parse-Signature");
  if (!IsPunc(parser, "(")) {
    EmitParseSyntaxErr(parser, TokSpan(parser));
    return {parser, {}, nullptr};
  }
  Parser next = parser;
  Advance(next);
  ParseElemResult<std::vector<Param>> params = ParseParamList(next);
  if (!IsPunc(params.parser, ")")) {
    EmitParseSyntaxErr(params.parser, TokSpan(params.parser));
  } else {
    Advance(params.parser);
  }
  ParseElemResult<std::shared_ptr<Type>> ret = ParseReturnOpt(params.parser);
  return {ret.parser, params.elem, ret.elem};
}

IMPLEMENTATION NOTES:
- Regular signature for top-level procedures
- No receiver, just parameters and return type
- Format: (param1, param2) -> ReturnType
*/

// =============================================================================
// TYPE DEFINITIONS USED
// =============================================================================

/*
ENUM: ParamMode
enum class ParamMode {
  Move  // Parameter takes ownership
};

STRUCT: Param
struct Param {
  std::optional<ParamMode> mode;  // None = borrow (default)
  Identifier name;
  std::shared_ptr<Type> type;
  core::Span span;
};

ENUM: ReceiverPerm
enum class ReceiverPerm {
  Const,   // ~ - read-only
  Unique,  // ~! - exclusive mutable
  Shared   // ~% - synchronized shared
};

STRUCT: ReceiverShorthand
struct ReceiverShorthand {
  ReceiverPerm perm;
};

STRUCT: ReceiverExplicit
struct ReceiverExplicit {
  std::optional<ParamMode> mode_opt;
  std::shared_ptr<Type> type;
};

VARIANT: Receiver
using Receiver = std::variant<ReceiverShorthand, ReceiverExplicit>;

STRUCT: SignatureResult
struct SignatureResult {
  Parser parser;
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;  // nullable
};

STRUCT: MethodSignatureResult
struct MethodSignatureResult {
  Parser parser;
  Receiver receiver;
  std::vector<Param> params;
  std::shared_ptr<Type> return_type_opt;  // nullable
};
*/

// =============================================================================
// GRAMMAR
// =============================================================================

/*
Signature ::= '(' ParamList ')' ReturnOpt
MethodSignature ::= '(' Receiver MethodParams ')' ReturnOpt

ReturnOpt ::= '->' Type | ε
ParamList ::= (Param (',' Param)*)?
MethodParams ::= (',' ParamList)?

Param ::= ParamMode? IDENTIFIER ':' Type
ParamMode ::= 'move'

Receiver ::= ReceiverShorthand | ReceiverExplicit
ReceiverShorthand ::= '~' | '~!' | '~%'
ReceiverExplicit ::= ParamMode? 'self' ':' Type
*/

// =============================================================================
// RECEIVER SHORTHAND EXPANSION
// =============================================================================

/*
| Shorthand | Expands To     | Meaning               |
| --------- | -------------- | --------------------- |
| ~         | self: const Self  | Read-only access      |
| ~!        | self: unique Self | Exclusive mutable     |
| ~%        | self: shared Self | Synchronized shared   |

Receiver Compatibility:
| Caller Permission | May Call ~ | May Call ~% | May Call ~! |
| ----------------- | ---------- | ----------- | ----------- |
| const             | Yes        | No          | No          |
| shared            | Yes        | Yes         | No          |
| unique            | Yes        | Yes         | Yes         |
*/

// =============================================================================
// EXAMPLES
// =============================================================================

/*
// Regular procedure
procedure add(a: i32, b: i32) -> i32

// With move parameter
procedure consume(move data: unique Buffer) -> ()

// Method with const receiver
procedure get(~) -> i32

// Method with unique receiver (mutating)
procedure set(~!, value: i32) -> ()

// Method with shared receiver (synchronized)
procedure synchronized_read(~%) -> i32

// Explicit receiver
procedure custom(self: const MyType, extra: i32) -> ()
*/

// =============================================================================
// IMPLEMENTATION CHECKLIST
// =============================================================================
// [ ] ParseReturnOpt function
//     - Check for -> operator
//     - Parse type if present
//     - Return nullptr if not present
// [ ] ParseParamModeOpt function
//     - Check for "move" keyword
//     - Return nullopt if not present
// [ ] ParseParam function
//     - Parse optional mode
//     - Parse identifier name
//     - Expect colon
//     - Parse type
// [ ] ParseParamList function
//     - Handle empty list
//     - Parse comma-separated parameters
//     - Handle trailing comma error
// [ ] ParseReceiver function
//     - Check for shorthand: ~, ~!, ~%
//     - Otherwise parse explicit receiver with self
// [ ] ParseMethodParams function
//     - Handle comma after receiver
//     - Parse remaining parameters
// [ ] ParseMethodSignature function
//     - Expect (
//     - Parse receiver
//     - Parse method params
//     - Expect )
//     - Parse return type
// [ ] ParseSignature function
//     - Expect (
//     - Parse param list
//     - Expect )
//     - Parse return type

// =============================================================================
// PSEUDO-IMPLEMENTATION
// =============================================================================

/*
SignatureResult ParseSignature(Parser parser) {
  if (!IsPunc(parser, "(")) {
    EmitError(parser);
    return {parser, {}, nullptr};
  }
  Advance(parser);

  auto params = ParseParamList(parser);
  parser = params.parser;

  if (!IsPunc(parser, ")")) {
    EmitError(parser);
  } else {
    Advance(parser);
  }

  auto ret = ParseReturnOpt(parser);
  return {ret.parser, params.elem, ret.elem};
}
*/

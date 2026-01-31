// =============================================================================
// MIGRATION MAPPING: enum_pattern.cpp
// =============================================================================
// This file should contain parsing logic for enum patterns, which match enum
// variants and optionally destructure their payloads.
//
// SPEC REFERENCE: C0updated.md, Section 3.3.9, Lines 6117-6120
// HELPER RULES: Section 3.3.9.1, Lines 6219-6234
// =============================================================================
//
// FORMAL RULES:
// -----------------------------------------------------------------------------
// **(Parse-Pattern-Enum)** Lines 6117-6120
// Gamma |- ParseTypePath(P) => (P_1, path)
// IsOp(Tok(P_1), "::")
// Gamma |- ParseIdent(Advance(P_1)) => (P_2, name)
// Gamma |- ParseEnumPatternPayloadOpt(P_2) => (P_3, payload_opt)
// --------------------------------------------------------------------------
// Gamma |- ParsePatternAtom(P) => (P_3, EnumPattern(path, name, payload_opt))
//
// ENUM PAYLOAD PARSING RULES (Lines 6221-6234):
// -----------------------------------------------------------------------------
// **(Parse-EnumPatternPayloadOpt-None)** Lines 6221-6224
// Tok(P) NOT IN {Punctuator("("), Punctuator("{")}
// --------------------------------------------------------------------------
// Gamma |- ParseEnumPatternPayloadOpt(P) => (P, null)
//
// **(Parse-EnumPatternPayloadOpt-Tuple)** Lines 6226-6229
// IsPunc(Tok(P), "(")
// Gamma |- ParseTuplePatternElems(Advance(P)) => (P_1, ps)
// IsPunc(Tok(P_1), ")")
// --------------------------------------------------------------------------
// Gamma |- ParseEnumPatternPayloadOpt(P) => (Advance(P_1), TuplePayloadPattern(ps))
//
// **(Parse-EnumPatternPayloadOpt-Record)** Lines 6231-6234
// IsPunc(Tok(P), "{")
// Gamma |- ParseFieldPatternList(Advance(P)) => (P_1, fs)
// IsPunc(Tok(P_1), "}")
// --------------------------------------------------------------------------
// Gamma |- ParseEnumPatternPayloadOpt(P) => (Advance(P_1), RecordPayloadPattern(fs))
//
// SEMANTICS:
// - `Result::Ok` - match variant without payload
// - `Result::Ok(v)` - match variant with tuple payload
// - `Result::Err{ code, msg }` - match variant with record payload
// - The path prefix identifies the enum type
// - The name after `::` identifies the variant
//
// EXAMPLES:
// - `Option::Some(x)` - destructure Some variant
// - `Option::None` - match None variant
// - `Error::IoError{ code: c }` - destructure named fields
//
// =============================================================================
// SOURCE FILE: cursive-bootstrap/src/02_syntax/parser_patterns.cpp
// =============================================================================
//
// CONTENT TO MIGRATE:
// -----------------------------------------------------------------------------
//
// 1. EnumPayloadOptResult struct (Lines 66-69)
//    -------------------------------------------------------------------------
//    struct EnumPayloadOptResult {
//      Parser parser;
//      std::optional<EnumPayloadPattern> payload_opt;
//    };
//
// 2. ParseEnumPatternPayloadOpt function (Lines 240-273)
//    -------------------------------------------------------------------------
//    EnumPayloadOptResult ParseEnumPatternPayloadOpt(Parser parser) {
//      if (!IsPunc(parser, "(") && !IsPunc(parser, "{")) {
//        SPEC_RULE("Parse-EnumPatternPayloadOpt-None");
//        return {parser, std::nullopt};
//      }
//      if (IsPunc(parser, "(")) {
//        SPEC_RULE("Parse-EnumPatternPayloadOpt-Tuple");
//        Parser next = parser;
//        Advance(next);
//        ParseElemResult<std::vector<PatternPtr>> elems = ParseTuplePatternElems(next);
//        if (!IsPunc(elems.parser, ")")) {
//          EmitParseSyntaxErr(elems.parser, TokSpan(elems.parser));
//          return {elems.parser, std::nullopt};
//        }
//        Parser after = elems.parser;
//        Advance(after);
//        TuplePayloadPattern payload;
//        payload.elements = std::move(elems.elem);
//        return {after, EnumPayloadPattern{std::move(payload)}};
//      }
//      SPEC_RULE("Parse-EnumPatternPayloadOpt-Record");
//      Parser next = parser;
//      Advance(next);
//      ParseElemResult<std::vector<FieldPattern>> fields = ParseFieldPatternList(next);
//      if (!IsPunc(fields.parser, "}")) {
//        EmitParseSyntaxErr(fields.parser, TokSpan(fields.parser));
//        return {fields.parser, std::nullopt};
//      }
//      Parser after = fields.parser;
//      Advance(after);
//      RecordPayloadPattern payload;
//      payload.fields = std::move(fields.elem);
//      return {after, EnumPayloadPattern{std::move(payload)}};
//    }
//
// 3. EnumPattern branch in ParsePatternAtom (Lines 336-349)
//    -------------------------------------------------------------------------
//    if (IsIdentTok(*tok)) {
//      Parser next = parser;
//      Advance(next);
//      if (IsOp(next, "::")) {
//        SPEC_RULE("Parse-Pattern-Enum");
//        ParseQualifiedHeadResult head = ParseQualifiedHead(parser);
//        EnumPayloadOptResult payload = ParseEnumPatternPayloadOpt(head.parser);
//        EnumPattern pat;
//        pat.path = head.module_path;
//        pat.name = head.name;
//        pat.payload_opt = payload.payload_opt;
//        return {payload.parser, MakePattern(SpanBetween(parser, payload.parser), pat)};
//      }
//    }
//
// NOTE: The implementation uses ParseQualifiedHead which combines path and name
// parsing. This is equivalent to ParseTypePath + "::" + ParseIdent from spec.
//
// DEPENDENCIES:
// =============================================================================
// - EnumPattern AST node type:
//   - path: ModulePath (or TypePath segments before the variant)
//   - name: Identifier (the variant name)
//   - payload_opt: optional<EnumPayloadPattern>
// - EnumPayloadPattern: variant type holding TuplePayloadPattern | RecordPayloadPattern
// - TuplePayloadPattern: contains elements: vector<PatternPtr>
// - RecordPayloadPattern: contains fields: vector<FieldPattern>
// - ParseQualifiedHead or (ParseTypePath + ParseIdent) functions
// - ParseTuplePatternElems function (tuple_pattern.cpp)
// - ParseFieldPatternList function (record_pattern.cpp)
// - MakePattern, SpanBetween helpers
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
// - EnumPayloadPattern is a sum type (variant) of tuple or record payload
// - The lookahead for `::` distinguishes enum patterns from record patterns
// - Payload is optional - unit variants have no payload
// - Tuple payloads use `()`, record payloads use `{}`
// - Reuses ParseTuplePatternElems and ParseFieldPatternList helpers
// - Span covers from path start to end of payload (or variant name if no payload)
// =============================================================================

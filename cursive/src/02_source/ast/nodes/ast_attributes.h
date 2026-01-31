// ===========================================================================
// MIGRATION MAPPING: ast_attributes.h
// ===========================================================================
//
// PURPOSE:
//   Attribute AST node definitions. Contains struct definitions for the
//   [[attr]] annotation system used on declarations and expressions.
//   This is a C0X extension.
//
// ---------------------------------------------------------------------------
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ---------------------------------------------------------------------------
//   Section 3.3.2.2 - Attributes (Lines 2671-2678)
//
//   Attribute system:
//   - AttrName = Identifier                             (2672)
//   - AttrArg = (key?: Identifier, value: Token | Expr) (2673)
//   - AttributeSpec = (name: AttrName, args: [AttrArg]) (2675)
//   - AttributeList = [AttributeSpec]                   (2677)
//
//   Attribute syntax: [[name]] or [[name(arg, key=value)]]
//
//   Standard attributes (per spec):
//   - [[inline]], [[inline(always)]], [[inline(never)]], [[inline(hint)]]
//   - [[cold]]
//   - [[export]]
//   - [[no_mangle]]
//   - [[symbol("name")]]
//   - [[unwind]]
//   - [[layout(C)]]
//   - [[ffi_pass_by_value]]
//   - [[weak]]
//   - [[dynamic]]
//   - [[static_dispatch_only]]
//
// ---------------------------------------------------------------------------
// SOURCE FILE: C:\Dev\Cursive\cursive-bootstrap\include\cursive0\02_syntax\ast.h
// ---------------------------------------------------------------------------
//
// 1. ATTRIBUTE ARGUMENT (Lines 58-61)
//    ─────────────────────────────────────────────────────────────────────────
//    struct AttributeArg {
//      optional<Identifier> key;  // Named arg: key = value
//      variant<Token, ExprPtr> value;  // Token literal or expression
//    };
//
// 2. ATTRIBUTE ITEM (Lines 63-68)
//    ─────────────────────────────────────────────────────────────────────────
//    struct AttributeItem {
//      Identifier name;
//      vector<AttributeArg> args;
//      Span span;
//    };
//
// 3. ATTRIBUTE LIST (Lines 70-71)
//    ─────────────────────────────────────────────────────────────────────────
//    using AttributeList = vector<AttributeItem>;
//
// ---------------------------------------------------------------------------
// ATTRIBUTE USAGE CONTEXTS
// ---------------------------------------------------------------------------
//   Attributes can appear on:
//
//   DECLARATIONS (items.h):
//   - ProcedureDecl.attrs
//   - ExternBlock.attrs
//   - ExternProcDecl.attrs
//   - RecordDecl.attrs
//   - FieldDecl.attrs
//   - MethodDecl.attrs
//   - EnumDecl.attrs
//
//   EXPRESSIONS (exprs.h):
//   - AttributedExpr.attrs (e.g., [[dynamic]] contract_expr)
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   From fwd.h:
//     - Identifier, ExprPtr
//
//   From 01_core/span.h:
//     - core::Span
//
//   From lexer:
//     - Token (for literal attribute arguments)
//
// ---------------------------------------------------------------------------
// ATTRIBUTE ARGUMENT VALUES
// ---------------------------------------------------------------------------
//   Attribute arguments can be:
//
//   1. Positional token literals:
//      [[inline(always)]]  -> args[0] = {key: none, value: Token("always")}
//      [[symbol("name")]]  -> args[0] = {key: none, value: Token("name")}
//
//   2. Named token literals:
//      [[foo(bar="baz")]]  -> args[0] = {key: "bar", value: Token("baz")}
//
//   3. Expression values (rare):
//      [[foo(size=4*1024)]] -> args[0] = {key: "size", value: BinaryExpr}
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Attribute validation is done in analysis phase, not parsing.
//      The AST just stores the raw attribute syntax.
//
//   2. Consider adding convenience accessors:
//      bool has_attr(const AttributeList&, string_view name);
//      const AttributeItem* find_attr(const AttributeList&, string_view name);
//      optional<Token> get_attr_arg(const AttributeItem&, size_t index);
//      optional<Token> get_attr_arg(const AttributeItem&, string_view key);
//
//   3. Some attributes affect code generation:
//      - [[inline]] -> inlining hints
//      - [[layout(C)]] -> C ABI layout
//      - [[export]] -> symbol visibility
//      Document these effects in the analysis/codegen phases.
//
//   4. The [[dynamic]] attribute affects contract verification mode.
//      It's used on expressions (AttributedExpr) and procedures.
//
//   5. Consider interning attribute names for faster comparison:
//      enum class AttrKind { Inline, Cold, Export, NoMangle, ... };
//      optional<AttrKind> classify_attr(string_view name);
//
//   6. Attribute arguments using ExprPtr create a dependency on
//      expressions. This is rare but supported for computed values.
//
// ===========================================================================

// TODO: Migrate attribute definitions from ast.h lines 55-71
//
// namespace cursive::ast {
//
// // Attribute argument (positional or named)
// struct AttributeArg {
//   std::optional<Identifier> key;  // Named: key = value
//   std::variant<lexer::Token, ExprPtr> value;
// };
//
// // Single attribute: [[name]] or [[name(args)]]
// struct AttributeItem {
//   Identifier name;
//   std::vector<AttributeArg> args;
//   core::Span span;
// };
//
// // List of attributes on a declaration or expression
// using AttributeList = std::vector<AttributeItem>;
//
// // Helper functions
// bool has_attribute(const AttributeList& attrs, std::string_view name);
// const AttributeItem* find_attribute(const AttributeList& attrs, std::string_view name);
//
// } // namespace cursive::ast


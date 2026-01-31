// ===========================================================================
// MIGRATION MAPPING: validate_common.cpp
// ===========================================================================
//
// PURPOSE:
//   Common validation infrastructure, result types, and visitor pattern base.
//   Provides foundational types and utilities used by all category-specific
//   validators.
//
// ===========================================================================
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ===========================================================================
//   Section 3.3.2 - AST Node Catalog
//   Defines the AST structure that validation operates on.
//
//   Section 3.3.11 - Documentation Association (line 2639)
//   Validation phases include documentation attachment.
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Internal:
//   - ast/nodes/ast_fwd.h: Forward declarations for all AST types
//   - 00_core/diagnostics.h: DiagnosticStream for error reporting
//   - 00_core/span.h: Span for error locations
//
//   External:
//   - <variant>: std::visit for variant traversal
//   - <functional>: std::function for composable validators
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. ValidationResult is the common return type for all validators
//   2. ASTValidator base class provides visitor pattern infrastructure
//   3. Validation phases can be composed using functional composition
//   4. Span validation utilities are shared across all node types
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above
//
// namespace cursive::ast::validate {
//
// // ---------------------------------------------------------------------------
// // VALIDATION RESULT
// // ---------------------------------------------------------------------------
//
// /// Result of a validation operation.
// /// Contains success/failure status and any emitted diagnostics.
// struct ValidationResult {
//     bool valid = true;
//     DiagnosticStream diags;
//
//     /// Combine two validation results.
//     /// Result is valid only if both inputs are valid.
//     ValidationResult& operator+=(const ValidationResult& other);
//
//     /// Check if validation passed.
//     explicit operator bool() const { return valid; }
// };
//
// /// Combine multiple validation results.
// ValidationResult combine(std::initializer_list<ValidationResult> results);
//
// // ---------------------------------------------------------------------------
// // VALIDATION CONTEXT
// // ---------------------------------------------------------------------------
//
// /// Context passed through validation traversal.
// /// Tracks current scope, parent nodes, and validation state.
// struct ValidationContext {
//     DiagnosticStream& diags;
//
//     // Scope tracking
//     bool in_method_context = false;
//     bool in_modal_state = false;
//     bool in_unsafe_block = false;
//     bool in_region_block = false;
//     bool in_loop = false;
//     bool in_parallel = false;
//
//     // Key tracking for yield validation
//     bool keys_held = false;
//
//     // Parent tracking for context-dependent validation
//     const ASTItem* current_item = nullptr;
//     const Stmt* current_stmt = nullptr;
//     const Expr* current_expr = nullptr;
// };
//
// // ---------------------------------------------------------------------------
// // VISITOR BASE CLASS
// // ---------------------------------------------------------------------------
//
// /// Base class for AST validation visitors.
// /// Provides default implementations that recursively validate children.
// struct ASTValidator {
//     ValidationContext& ctx;
//
//     explicit ASTValidator(ValidationContext& ctx) : ctx(ctx) {}
//     virtual ~ASTValidator() = default;
//
//     // Type validation
//     virtual ValidationResult visit(const Type& type);
//     virtual ValidationResult visit(const TypePrim& type);
//     virtual ValidationResult visit(const TypePerm& type);
//     virtual ValidationResult visit(const TypeUnion& type);
//     // ... etc for all type variants
//
//     // Expression validation
//     virtual ValidationResult visit(const Expr& expr);
//     virtual ValidationResult visit(const LiteralExpr& expr);
//     virtual ValidationResult visit(const BinaryExpr& expr);
//     // ... etc for all expression variants
//
//     // Pattern validation
//     virtual ValidationResult visit(const Pattern& pattern);
//     // ... etc for all pattern variants
//
//     // Statement validation
//     virtual ValidationResult visit(const Stmt& stmt);
//     virtual ValidationResult visit(const Block& block);
//     // ... etc for all statement variants
//
//     // Declaration validation
//     virtual ValidationResult visit(const ASTItem& item);
//     // ... etc for all item variants
//
//     // Module validation
//     virtual ValidationResult visit(const ASTFile& file);
//     virtual ValidationResult visit(const ASTModule& module);
// };
//
// // ---------------------------------------------------------------------------
// // COMPOSABLE VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Type alias for validation functions.
// using ValidatorFn = std::function<ValidationResult(const ASTModule&, ValidationContext&)>;
//
// /// Compose multiple validators into a single validator.
// /// Runs each validator in sequence, combining results.
// ValidatorFn compose(std::initializer_list<ValidatorFn> validators);
//
// /// Create a validator that runs only if a condition is met.
// ValidatorFn when(bool condition, ValidatorFn validator);
//
// // ---------------------------------------------------------------------------
// // SPAN VALIDATION UTILITIES
// // ---------------------------------------------------------------------------
//
// /// Validates that a span has correct ordering (start <= end).
// bool validate_span_ordering(const Span& span, ValidationContext& ctx);
//
// /// Validates that child span is contained within parent span.
// bool validate_span_containment(const Span& parent, const Span& child, ValidationContext& ctx);
//
// /// Validates that sibling spans do not overlap.
// bool validate_span_no_overlap(const Span& a, const Span& b, ValidationContext& ctx);
//
// /// Validates that all nodes in a tree have valid spans.
// template<typename Node>
// ValidationResult validate_spans(const Node& root, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // DIAGNOSTIC HELPERS
// // ---------------------------------------------------------------------------
//
// /// Emit a validation error.
// void emit_error(ValidationContext& ctx, Span span, DiagnosticCode code, std::string message);
//
// /// Emit a validation warning.
// void emit_warning(ValidationContext& ctx, Span span, DiagnosticCode code, std::string message);
//
// /// Emit a validation note (additional context for previous diagnostic).
// void emit_note(ValidationContext& ctx, Span span, std::string message);
//
// } // namespace cursive::ast::validate


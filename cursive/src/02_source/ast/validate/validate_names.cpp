// ===========================================================================
// MIGRATION MAPPING: validate_names.cpp
// ===========================================================================
//
// PURPOSE:
//   Name validation functions. Checks for reserved keywords, protected names,
//   reserved prefixes and namespaces, and naming constraint violations.
//
// ===========================================================================
// SPEC REFERENCE: C:\Dev\Cursive\C0updated.md
// ===========================================================================
//   Section 3.2.3 - Reserved Names (lines 2086-2102)
//
//   Reserved Keywords (line ~2086):
//   all, as, break, class, continue, dispatch, else, enum, false, defer,
//   frame, from, if, imm, import, internal, let, loop, match, modal, move,
//   mut, null, parallel, private, procedure, protected, public, race, record,
//   region, return, shadow, shared, spawn, sync, transition, transmute, true,
//   type, unique, unsafe, var, widen, where, using, yield, const, override
//
//   Contextual Keywords (line ~2092):
//   - in: loop iteration context (loop x in range)
//   - key: dispatch key clause (dispatch i in range key path mode)
//   - wait: wait expression context
//
//   Universe-Protected Names (line ~2095):
//   Self, Drop, Bitcopy, Clone, Eq, Hash, Hasher, Iterator, Step, FfiSafe,
//   string, bytes, Modal, Region, RegionOptions, CancelToken, Context,
//   System, ExecutionDomain, CpuSet, Priority, Reactor
//
//   Reserved Namespace Prefixes (line ~2100):
//   - cursive, cursive::*
//   - std
//
//   Reserved Identifier Prefix (line ~2102):
//   - gen_ (reserved for generated identifiers)
//
// ---------------------------------------------------------------------------
// DEPENDENCIES
// ---------------------------------------------------------------------------
//   Internal:
//   - ast/nodes/ast_common.h: Identifier
//   - ast/nodes/ast_module.h: ASTModule
//   - validate_common.cpp: ValidationResult, ValidationContext
//   - 00_core/span.h: Span for error locations
//
// ---------------------------------------------------------------------------
// REFACTORING NOTES
// ---------------------------------------------------------------------------
//   1. Name validation is used by multiple other validators
//   2. Contextual keywords need context to determine if reserved
//   3. Universe-protected names cannot be shadowed in certain contexts
//   4. Reserved prefix checking should be efficient (trie or hash)
//
// ===========================================================================

// TODO: Migrate implementation from source files listed above
//
// namespace cursive::ast::validate {
//
// // ---------------------------------------------------------------------------
// // TOP-LEVEL NAME VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates all names in a module.
// /// @param module Module to validate
// /// @param ctx Validation context
// ValidationResult validate_names(const ASTModule& module, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // RESERVED KEYWORD CHECKING
// // ---------------------------------------------------------------------------
//
// /// Checks if a name is a reserved keyword.
// /// Reserved keywords cannot be used as identifiers.
// /// @param name Name to check
// bool is_reserved_keyword(std::string_view name);
//
// /// Checks if a name is a contextual keyword.
// /// Contextual keywords are only reserved in specific syntactic positions.
// /// @param name Name to check
// bool is_contextual_keyword(std::string_view name);
//
// /// Checks if a contextual keyword is reserved in the current context.
// /// @param name Name to check
// /// @param ctx Current validation context
// bool is_contextual_keyword_reserved(std::string_view name, const ValidationContext& ctx);
//
// /// List of all reserved keywords.
// extern const std::vector<std::string_view> RESERVED_KEYWORDS;
//
// /// List of contextual keywords.
// extern const std::vector<std::string_view> CONTEXTUAL_KEYWORDS;
//
// // ---------------------------------------------------------------------------
// // UNIVERSE-PROTECTED NAME CHECKING
// // ---------------------------------------------------------------------------
//
// /// Checks if a name is universe-protected.
// /// Universe-protected names cannot be shadowed in certain contexts.
// /// @param name Name to check
// bool is_universe_protected(std::string_view name);
//
// /// Checks if shadowing a universe-protected name is allowed in context.
// /// @param name Name being shadowed
// /// @param ctx Current validation context
// bool can_shadow_universe_protected(std::string_view name, const ValidationContext& ctx);
//
// /// List of universe-protected names.
// extern const std::vector<std::string_view> UNIVERSE_PROTECTED_NAMES;
//
// // ---------------------------------------------------------------------------
// // RESERVED PREFIX CHECKING
// // ---------------------------------------------------------------------------
//
// /// Checks if a name uses a reserved prefix.
// /// Reserved prefixes: gen_
// /// @param name Name to check
// bool is_reserved_prefix(std::string_view name);
//
// /// Checks if a name starts with gen_ prefix.
// /// @param name Name to check
// bool has_gen_prefix(std::string_view name);
//
// // ---------------------------------------------------------------------------
// // RESERVED NAMESPACE CHECKING
// // ---------------------------------------------------------------------------
//
// /// Checks if a path uses a reserved namespace.
// /// Reserved namespaces: cursive, cursive::*, std
// /// @param path Path to check
// bool is_reserved_namespace(std::string_view path);
//
// /// Checks if a module path starts with cursive::.
// /// @param path Path to check
// bool is_cursive_namespace(std::string_view path);
//
// /// Checks if a module path is std.
// /// @param path Path to check
// bool is_std_namespace(std::string_view path);
//
// // ---------------------------------------------------------------------------
// // IDENTIFIER VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates an identifier is legal.
// /// Checks:
// /// - Not a reserved keyword
// /// - Not using reserved prefix
// /// - Valid Unicode identifier per spec
// /// @param name Identifier to validate
// /// @param ctx Validation context
// bool is_valid_identifier(const Identifier& name, ValidationContext& ctx);
//
// /// Validates an identifier is legal for declaration.
// /// Additional checks:
// /// - Not a universe-protected name (if applicable)
// /// @param name Identifier to validate
// /// @param ctx Validation context
// bool is_valid_declaration_name(const Identifier& name, ValidationContext& ctx);
//
// /// Validates a module path is legal.
// /// Checks:
// /// - Not a reserved namespace
// /// - All segments are valid identifiers
// /// @param path Path to validate
// /// @param ctx Validation context
// bool is_valid_module_path_name(const Path& path, ValidationContext& ctx);
//
// // ---------------------------------------------------------------------------
// // UNICODE VALIDATION
// // ---------------------------------------------------------------------------
//
// /// Validates identifier follows Unicode rules.
// /// Identifiers are NFC-normalized for equality.
// /// @param name Name to check
// bool is_valid_unicode_identifier(std::string_view name);
//
// /// Checks for disallowed Unicode characters.
// /// Disallowed outside unsafe: bidirectional controls, zero-width joiners.
// /// @param name Name to check
// /// @param in_unsafe Whether currently in unsafe context
// bool has_disallowed_unicode(std::string_view name, bool in_unsafe);
//
// // ---------------------------------------------------------------------------
// // DIAGNOSTIC MESSAGES
// // ---------------------------------------------------------------------------
//
// /// Generates error message for reserved keyword usage.
// std::string reserved_keyword_message(std::string_view name);
//
// /// Generates error message for reserved prefix usage.
// std::string reserved_prefix_message(std::string_view name);
//
// /// Generates error message for reserved namespace usage.
// std::string reserved_namespace_message(std::string_view path);
//
// /// Generates "did you mean" suggestion for common mistakes.
// std::optional<std::string> suggest_alternative(std::string_view name);
//
// } // namespace cursive::ast::validate


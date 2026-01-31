// =============================================================================
// MIGRATION MAPPING: lexer_security.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 3.2.10 - Lexical Security (lines 2446-2498)
//
// SOURCE FILE: cursive-bootstrap/src/02_syntax/lexer_security.cpp
//   Lines 1-142 (entire file)
//
// DEPENDENCIES:
//   - cursive/include/02_source/lexer.h (LexSecureResult, Token)
//   - cursive/src/00_core/unicode.cpp (Utf8Offsets)
//   - cursive/src/00_core/span.cpp (SpanOf, SpanRange)
//   - cursive/src/00_core/diagnostic_messages.cpp (MakeDiagnostic, Emit)
//
// =============================================================================
// CONTENT TO MIGRATE:
// =============================================================================
//
// INTERNAL HELPERS (anonymous namespace, source lines 14-98):
//
// 1. SpanOfText() (lines 16-23)
//    Convert scalar indices to Span
//
// 2. IsLBrace(), IsRBrace() (lines 25-31)
//    Implements spec predicates from lines 2464-2465:
//    IsLBrace(t) = t.kind = Punctuator("{")
//    IsRBrace(t) = t.kind = Punctuator("}")
//
// 3. NextNonNewline() (lines 33-41)
//    Implements spec NextNonNewline from lines 2467-2468:
//    NextNonNewline(K, i) = min{j | j >= i AND K[j].kind != Newline}
//
// 4. MatchBrace() (lines 43-57)
//    Implements spec MatchBrace from lines 2470-2473:
//    Find matching closing brace using depth counting
//    Balance(K, j, m) counts open minus close braces
//
// 5. SpanFrom() (lines 59-65)
//    Construct span from two tokens (start to end)
//    Implements spec SpanFrom from line 2475
//
// 6. ComputeUnsafeSpans() (lines 67-85)
//    Implements spec UnsafeSpans from line 2477:
//    UnsafeSpans(K) = {SpanFrom(K[j], K[k]) |
//      K[i].kind = Keyword("unsafe"),
//      j = NextNonNewline(K, i+1),
//      K[j].kind = Punctuator("{"),
//      k = MatchBrace(K, j),
//      k != bottom}
//
//    Algorithm:
//    - Scan for "unsafe" keyword
//    - Find next non-newline token (must be "{")
//    - Find matching "}"
//    - Record span from "{" to "}"
//
// 7. UnsafeAtByte() (lines 87-96)
//    Implements spec UnsafeAtByte from line 2479:
//    UnsafeAtByte(b) = exists sp in UnsafeSpans(K). b in SpanRange(sp)
//
// MAIN FUNCTIONS:
//
// 8. UnsafeSpans() (lines 100-102)
//    Public wrapper for ComputeUnsafeSpans
//
// 9. LexSecure() (lines 104-138)
//    Implements spec rules from 3.2.10:
//
//    (LexSecure-Err) - spec lines 2486-2489:
//      If any sensitive position is NOT inside unsafe block:
//        Emit E-SRC-0308 at that position
//        Return error
//
//    (LexSecure-Warn) - spec lines 2491-2494:
//      If all sensitive positions ARE inside unsafe blocks:
//        Emit W-SRC-0308 for each (warning only)
//        Return ok
//
//    Algorithm:
//    a. If no sensitive positions, return ok
//    b. Compute unsafe block spans from token stream
//    c. For each sensitive position:
//       - If NOT in unsafe span: error E-SRC-0308, return fail
//    d. For all sensitive positions (all in unsafe):
//       - Emit warning W-SRC-0308 for each
//    e. Return ok
//
// =============================================================================
// SPEC DEFINITIONS:
// =============================================================================
//
// Sensitive characters (line 2084):
//   Sensitive(c) = c in {U+202A...U+202E, U+2066...U+2069, U+200C, U+200D}
//   These are bidirectional control characters and zero-width joiners
//   (Trojan Source attack vectors)
//
// Unsafe span mode (line 2481):
//   UnsafeSpanMode = TokenOnly
//   Only token-level detection, not AST-level
//
// LexSecureErrSpan (line 2497):
//   Error span is single character at sensitive position
//
// =============================================================================
// REFACTORING NOTES:
// =============================================================================
//
// - Current implementation uses TokenOnly mode as spec requires
// - Consider caching UnsafeSpans computation if called multiple times
// - SpanRange returns (start_offset, end_offset) pair for range check
//
// - The spec allows sensitive chars ONLY inside unsafe {} blocks
// - Even inside unsafe, a warning is emitted (security audit trail)
//
// - Sensitive character list is security-critical:
//   U+202A-U+202E: Bidirectional embedding controls
//   U+2066-U+2069: Bidirectional isolate controls
//   U+200C: Zero-width non-joiner
//   U+200D: Zero-width joiner
//
// - These characters can cause "Trojan Source" attacks where code
//   visually appears different from its actual semantics
//
// =============================================================================

#include "cursive/include/02_source/lexer.h"

// TODO: Migrate from cursive-bootstrap/src/02_syntax/lexer_security.cpp

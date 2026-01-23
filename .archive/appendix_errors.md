# Appendix B: Error Code Catalog

This appendix catalogs all normative error codes referenced throughout the specification. Implementations MUST emit these codes for the corresponding violations.

## B.1 Parsing Errors (E10xx)

| Code | Description | Section |
|------|-------------|---------|
| E1001 | Unexpected token | §2 |
| E1002 | Unterminated string literal | §2.4.5 |
| E1003 | Unterminated block comment | §2.2 |
| E1004 | Invalid escape sequence | §2.4.4-5 |
| E1005 | Invalid integer literal | §2.4.1 |
| E1006 | Invalid float literal | §2.4.2 |
| E1007 | Unclosed delimiter (expected closing `(`, `[`, `<`, or `{`) | §2 |
| E1008 | Unexpected newline in expression | §2 |

## B.2 Name Resolution Errors (E20xx)

| Code | Description | Section |
|------|-------------|---------|
| E2001 | Undefined identifier | §30-33 |
| E2002 | Ambiguous name | §31.2 |
| E2003 | Circular import | §31 |
| E2004 | Module not found | §33.1 |
| E2005 | Private item access violation | §32 |

## B.3 Type Errors (E2xxx)

| Code | Description | Section |
|------|-------------|---------|
| E2101 | Type mismatch | §4-9 |
| E2102 | Unknown type | §4 |
| E2103 | Trait not implemented | §8 |
| E2104 | Conflicting trait implementations | §8 |
| E2105 | Type parameter mismatch | §9 |
| E2106 | Const generic mismatch | §9.6 |
| E2107 | Array size mismatch | §5.1 |
| E2108 | Infinite type recursion | §4 |

## B.4 Modal Errors (E30xx)

| Code | Description | Section |
|------|-------------|---------|
| E3001 | Invalid state transition | §10-12 |
| E3002 | Procedure called in wrong state | §10.3 |
| E3003 | Unreachable state | §11.6 |
| E3004 | Non-exhaustive state coverage | §12.3 |
| E3005 | Modal state invariant violation | §19.4 |

## B.5 Contract Errors (E40xx)

| Code | Description | Section |
|------|-------------|---------|
| E4001 | Precondition may fail | §17.2 |
| E4002 | Postcondition cannot be proven | §17.3 |
| E4003 | Invariant violation | §19 |
| E4004 | Loop invariant not maintained | §19.5 |
| E4005 | Invalid @old reference | §17.6 |
| E4006 | Contract references private field | §17 |

## B.6 Region/Memory Errors (E50xx)

| Code | Description | Section |
|------|-------------|---------|
| E5001 | Region escape detected | §29.4 |
| E5002 | Region outlived allocation | §28 |
| E5003 | Invalid region nesting | §28.3 |
| E5004 | Region allocation without declaration | §28.5 |

## B.7 Permission Errors (E60xx)

| Code | Description | Section |
|------|-------------|---------|
| E6001 | Permission violation | §26 |
| E6002 | Moved value used | §26.3 |
| E6003 | Mutable reference to immutable data | §26 |
| E6004 | Isolated permission violation | §26.5 |

## B.8 Effect Errors (E90xx)

| Code | Description | Section |
|------|-------------|---------|
| E9001 | Missing effect declaration | §21-24 |
| E9002 | Effect exceeds declared needs | §22 |
| E9003 | Forbidden effect used | §22.4 |
| E9004 | Effect budget exceeded | §23 |
| E9005 | Async effect mask violation | §22.7 |
| E9201 | Callback exceeds effect mask | §36.5, §60 |

## B.9 Typed Holes (E95xx)

| Code | Description | Section |
|------|-------------|---------|
| E9501 | Unfilled typed hole | §14.10 |
| E9502 | Typed hole elaborated to trap (warning) | §14.10 |
| E9503 | Hole effects exceed function effects | §14.10 |

## B.10 FFI Errors (E97xx)

| Code | Description | Section |
|------|-------------|---------|
| E9701 | FFI call without ffi.call effect | §57 |
| E9702 | Invalid type across FFI boundary | §58 |
| E9703 | Mismatched calling convention | §57 |
| E9704 | Region value crosses FFI boundary | §59 |

## B.11 Concurrency Errors (E78xx)

| Code | Description | Section |
|------|-------------|---------|
| E7801 | JoinHandle escape from scope | §35.5 |

## B.12 Warnings (W0xxx-W9xxx)

| Code | Description | Section |
|------|-------------|---------|
| W0101 | Alias collides with real name | §2.6 |
| W7800 | Implicit join at scope end | §35.5 |

## B.13 Implementation-Defined Diagnostics

Implementations MAY define additional error codes in the following ranges:
- **I1000-I1999**: Implementation-specific parsing warnings
- **I2000-I2999**: Implementation-specific type system warnings
- **I9000-I9999**: Implementation-specific optimization hints

These codes SHOULD be documented in the implementation's reference manual and MUST NOT conflict with normative codes defined in this specification.

---

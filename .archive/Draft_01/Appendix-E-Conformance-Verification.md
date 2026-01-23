# Appendix E – Conformance Verification

This appendix collects the conformance test suite, certification, and conformance claim requirements that were previously specified in Part I §6.6.

## E.1 Test Suite Structure

> A normative conformance test suite **SHOULD** be maintained alongside this specification to verify implementation conformance.
>
> The test suite **MUST** be organized by specification part and chapter, with tests clearly labeled by the requirement they verify (using section anchors, e.g., `[conformance.obligations.implementations]`).
>
> Each test **MUST** include:
>
> 1. A description of the requirement being tested.
> 2. Source code demonstrating the tested construct.
> 3. Expected behavior (successful compilation, specific diagnostic, runtime outcome).
> 4. Any necessary environment or configuration requirements.

## E.2 Test Categories

> Conformance tests **MUST** be categorized as:
>
> 1. **Positive tests**: Well-formed programs that implementations MUST accept and execute according to specified semantics.
> 2. **Negative tests**: Ill-formed programs that implementations MUST reject with appropriate diagnostics (§7), each verifying a specific error condition.
> 3. **Behavioral tests**: Programs verifying runtime semantics, memory safety properties, or concurrency guarantees.
>
> Negative tests **MUST** specify the expected diagnostic code(s) (§7.2). Implementations passing a negative test **MUST** issue at least one of the expected diagnostics.
>
> Test coverage **SHOULD** include at least one positive and one negative test for every normative MUST/MUST NOT requirement in this specification.

### E.2.1 Diagnostic Expectation Sidecar Format

> To make negative tests machine-readable and interoperable across implementations, each negative test file **SHOULD** be accompanied by a diagnostic expectation sidecar file in JSON format (for example, `test_name.diag.json`) with the following shape:
>
> ```json
> {
>   "expected": [
>     {
>       "code": "E-SRC-0101",           // REQUIRED: Acceptable diagnostic code
>       "severity": "error",            // OPTIONAL: Expected severity
>       "file": "path/to/test.cursive", // OPTIONAL: File path (defaults to test file)
>       "line": 12,                     // OPTIONAL: 1-based line number
>       "column": 5                     // OPTIONAL: 1-based column number
>     }
>   ]
> }
> ```
>
> A negative test is considered passed when the implementation emits at least one diagnostic whose code matches an entry in `expected` and whose source location (if specified) matches the given file and position within an implementation-defined tolerance (for example, allowing off-by-one column differences when tabs or normalizations differ). Additional diagnostics **MAY** be emitted but **MUST NOT** be treated as satisfying the test unless they also match the sidecar’s `expected` entries.

## E.3 Certification Process

> Implementations **MAY** seek conformance certification by demonstrating that they pass the normative test suite and satisfy all conformance obligations (§6.2).
>
> Certification **MUST** be version-specific, indicating which specification version the implementation conforms to. Implementations **MUST** provide a complete conformance dossier (§6.2.4) as part of certification.
>
> Certified implementations **MUST** document any test suite exclusions (e.g., for partial conformance per §6.6.5) with justifications for why specific tests cannot be satisfied.

## E.4 Conformance Claims

> Conformance claims **MUST** include:
>
> 1. Specification version conformed to (§8.1.1).
> 2. Implementation name and version.
> 3. Supported target platforms and their conformance dossiers.
> 4. Test suite version used and test results summary (pass/fail/excluded counts).
> 5. Any partial conformance declarations (§E.5).
>
> Conformance claims **MUST** be publicly accessible and **SHOULD** be machine-readable (JSON or TOML format preferred).

## E.5 Partial Conformance

> Implementations targeting embedded systems, restricted environments, or specialized domains **MAY** claim partial conformance by explicitly documenting which specification requirements they do not satisfy.
>
> Partial conformance claims **MUST** enumerate:
>
> 1. Unsupported language features with specification section references.
> 2. Unsupported standard library components (if core library requirements exist).
> 3. Reduced implementation limits below the minimums specified in §6.4.1.
> 4. Rationale for each exclusion (platform constraints, domain requirements).
>
> Partial conformance implementations **MUST** still satisfy all conformance obligations for the subset of the language they support. Features not supported **MUST** be diagnosed with appropriate error codes.
>
> When a conformance dossier is used to substantiate partial conformance claims, it **MUST** include a `partial_conformance` section as specified in Appendix C, §C.2.8. Each entry in that section **MUST** correspond to at least one of the enumerated exclusions above so that tools can consume partial conformance information in a machine‑readable way.

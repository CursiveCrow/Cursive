# Appendix C – Conformance Dossier Schema (Normative)

**Status**: Normative
**Related Sections**: §6.1.4.1 (Documentation Requirements), §6.2.4 (Conformance Dossier), §6.4.2 (Limit Documentation)

---

## C.1 Purpose and Scope [appendix.c.purpose]

This appendix defines the normative schema for conformance dossiers required by §6.2.4. Conformance dossiers document implementation-defined behaviors, implementation limits, UVB attestations, and extension support, enabling portability analysis and conformance verification.

### C.1.1 Format Requirements [appendix.c.purpose.format]

> Conformance dossiers **MUST** be provided in one of the following structured formats:
>
> 1. **JSON** conforming to the JSON Schema defined in §C.3
> 2. **TOML** conforming to the structure defined in §C.4
>
> Implementations **MAY** provide dossiers in both formats. When both formats are provided, they **MUST** contain equivalent information.
>
> Dossiers **MUST** use UTF-8 encoding. File names **SHOULD** follow the pattern `conformance-dossier-{target-triple}.{json|toml}` where `{target-triple}` identifies the target platform (e.g., `x86_64-unknown-linux-gnu`, `aarch64-apple-darwin`).

### C.1.2 Key Stability [appendix.c.purpose.stability]

> All keys defined in this appendix **MUST** be treated as stable identifiers across specification versions within the same MAJOR version. Implementations **MUST NOT** remove or rename required keys within a MAJOR version.
>
> New optional keys **MAY** be added in MINOR version increments. Implementations and tooling **MUST** ignore unknown keys to support forward compatibility.

### C.1.3 Multiple Target Support [appendix.c.purpose.targets]

> Implementations supporting multiple target platforms **MUST** provide a separate conformance dossier for each target triple, or provide a single dossier with target-specific sections clearly delineated by target triple keys.

---

## C.2 Required Top-Level Structure [appendix.c.structure]

> Conformance dossiers **MUST** contain the following top-level sections:
>
> 1. `implementation` - Implementation identification and metadata (§C.2.1)
> 2. `target` - Target platform specification (§C.2.2)
> 3. `implementation_defined_behaviors` - IDB documentation (§C.2.3)
> 4. `implementation_limits` - Limit documentation (§C.2.4)
> 5. `extensions` - Extension support table (§C.2.5)
> 6. `diagnostics` - Diagnostic system information (§C.2.6)
> 7. `uvb_attestations` - Unverifiable behavior attestations (§C.2.7)
>
> All sections except `uvb_attestations` and `partial_conformance` **MUST** be present. The `uvb_attestations` section **MUST** be present if and only if the dossier documents a specific program or library (program-specific dossiers); it **MAY** be omitted from general implementation dossiers. The optional `partial_conformance` section (§C.2.8) **MUST** be present when the implementation claims partial conformance as described in Appendix E, §E.5.

### C.2.1 Implementation Section [appendix.c.structure.implementation]

> The `implementation` section **MUST** contain the following fields:
>
> ```
> implementation {
>   name: string                    // REQUIRED: Implementation name
>   version: string                 // REQUIRED: Implementation version (SemVer format)
>   vendor: string                  // REQUIRED: Vendor/organization name
>   release_date: string            // REQUIRED: ISO 8601 date (YYYY-MM-DD)
>   specification_version: string   // REQUIRED: Target spec version (SemVer format)
>   homepage: string                // OPTIONAL: Implementation homepage URL
>   documentation_url: string       // OPTIONAL: Online documentation URL
>   contact: string                 // OPTIONAL: Support contact (email or URL)
>   build_identifier: string        // OPTIONAL: Build hash or identifier
>   build_config: object?           // OPTIONAL: Representative build configuration (§C.2.1.1)
> }
> ```
>
> The `specification_version` field **MUST** indicate which version of this specification the implementation conforms to. Implementations **MUST** reject programs declaring incompatible specification versions per §7.1.3.

#### C.2.7.1 UVB Categories [appendix.c.structure.uvb.categories]

> The `uvb_category` field **MUST** use one of the versioned category identifiers defined for this specification version. For version 1.0 of this specification, the following categories are reserved:
>
> - `"ffi_pointer_deref"` – Unverifiable behavior arising from dereferencing pointers obtained from or passed to foreign code.  
> - `"unsafe_block"` – Operations inside `unsafe` blocks whose correctness depends on external invariants not modeled by the type system or permission system.  
> - `"contract_unverifiable"` – Contract clauses whose truth cannot be established within the verification model of Part VIII but whose violation would compromise safety or correctness.  
> - `"platform_undefined"` – Interactions with platform facilities whose guarantees are weaker than the language’s (for example, reliance on implementation-defined signal behavior) and which require external justification.
>
> Future specification versions **MAY** extend this list with additional category strings but **MUST NOT** change the meaning of existing categories. Conformance tools **MUST** reject dossiers whose `uvb_category` values are not members of the allowed set for the referenced specification version.

**_Example:_**

```json
"implementation": {
  "name": "cursivec",
  "version": "1.2.3",
  "vendor": "Cursive Foundation",
  "release_date": "2025-11-13",
  "specification_version": "1.0.0",
  "homepage": "https://cursive-lang.org",
  "documentation_url": "https://docs.cursive-lang.org/conformance/1.2.3",
  "contact": "support@cursive-lang.org",
  "build_identifier": "a1b2c3d4e5f67890",
  "build_config": {
    "optimization": "release",
    "conformance_mode": "strict",
    "feature_flags": ["simd_intrinsics"],
    "lto": "full"
  }
}
```

#### C.2.1.1 Build Configuration Object [appendix.c.structure.implementation.buildconfig]

> When present, the optional `build_config` object **MUST** describe a representative build configuration for which the dossier’s guarantees are intended to hold. It **MUST** have the following shape:
>
> ```
> implementation.build_config {
>   optimization: string            // OPTIONAL: Optimization level (e.g., "debug", "release", "size")
>   conformance_mode: string        // OPTIONAL: Named conformance mode (e.g., "strict", "permissive")
>   feature_flags: array?           // OPTIONAL: Enabled feature flags (identifiers as in §7.3.2)
>   lto: string?                    // OPTIONAL: Link-time optimization mode (e.g., "off", "thin", "full")
> }
> ```
>
> When a conformance claim (§E.4) refers to a specific build configuration, the `build_config` object **SHOULD** match the configuration used for testing and certification, including the selected conformance mode and enabled feature flags.

### C.2.2 Target Section [appendix.c.structure.target]

> The `target` section **MUST** contain the following fields:
>
> ```
> target {
>   triple: string                  // REQUIRED: Target triple identifier
>   architecture: string            // REQUIRED: CPU architecture (e.g., "x86_64", "aarch64")
>   operating_system: string        // REQUIRED: OS identifier (e.g., "linux", "windows", "darwin")
>   environment: string             // OPTIONAL: ABI environment (e.g., "gnu", "msvc", "musl")
>   pointer_width: integer          // REQUIRED: Pointer size in bits (32, 64, 128)
>   endianness: string              // REQUIRED: "little" or "big"
>   default_alignment: integer      // REQUIRED: Default alignment in bytes
>   max_alignment: integer          // REQUIRED: Maximum supported alignment in bytes
> }
> ```

**_Example:_**

```json
"target": {
  "triple": "x86_64-unknown-linux-gnu",
  "architecture": "x86_64",
  "operating_system": "linux",
  "environment": "gnu",
  "pointer_width": 64,
  "endianness": "little",
  "default_alignment": 8,
  "max_alignment": 4096
}
```

### C.2.3 Implementation-Defined Behaviors Section [appendix.c.structure.idb]

> The `implementation_defined_behaviors` section **MUST** document all implementation-defined behaviors enumerated in §6.1.4.1. This section **MUST** contain the following subsections:

#### C.2.3.1 Primitive Types [appendix.c.structure.idb.primitives]

> ```
> implementation_defined_behaviors.primitive_types: array {
>   type: string                    // REQUIRED: Type name (e.g., "i32", "f64", "usize")
>   size: integer                   // REQUIRED: Size in bytes
>   alignment: integer              // REQUIRED: Alignment in bytes
>   representation: string          // REQUIRED: Binary representation description
>   min_value: string?              // OPTIONAL: Minimum value (for numeric types)
>   max_value: string?              // OPTIONAL: Maximum value (for numeric types)
>   notes: string?                  // OPTIONAL: Additional notes
> }
> ```

**_Example:_**

```json
"primitive_types": [
  {
    "type": "i32",
    "size": 4,
    "alignment": 4,
    "representation": "Two's complement, IEEE 754 compatible",
    "min_value": "-2147483648",
    "max_value": "2147483647"
  },
  {
    "type": "usize",
    "size": 8,
    "alignment": 8,
    "representation": "Unsigned integer, platform word size",
    "min_value": "0",
    "max_value": "18446744073709551615"
  },
  {
    "type": "f64",
    "size": 8,
    "alignment": 8,
    "representation": "IEEE 754 binary64"
  }
]
```

#### C.2.3.2 Pointer Semantics [appendix.c.structure.idb.pointers]

> ```
> implementation_defined_behaviors.pointer_semantics {
>   null_representation: string     // REQUIRED: How null pointers are represented
>   provenance_tracking: string     // REQUIRED: "strict" | "permissive" | "none"
>   comparison_semantics: string    // REQUIRED: Description of pointer comparison behavior
>   arithmetic_behavior: string     // REQUIRED: Description of pointer arithmetic behavior
>   notes: string?                  // OPTIONAL: Additional notes
> }
> ```

**_Example:_**

```json
"pointer_semantics": {
  "null_representation": "All-bits-zero (0x0)",
  "provenance_tracking": "strict",
  "comparison_semantics": "Pointers from different allocations are unordered; comparison within same allocation follows address order",
  "arithmetic_behavior": "Pointer arithmetic is checked in debug builds, unchecked in release builds",
  "notes": "Provenance violations may trigger runtime errors in debug mode"
}
```

#### C.2.3.3 Foreign Function Interface [appendix.c.structure.idb.ffi]

> ```
> implementation_defined_behaviors.ffi {
>   calling_conventions: array {     // REQUIRED: At least one entry
>     name: string                   // REQUIRED: Convention name (e.g., "C", "system")
>     abi_standard: string           // REQUIRED: Reference to ABI standard
>     parameter_passing: string      // REQUIRED: Description of parameter passing
>     return_value_handling: string  // REQUIRED: Description of return value handling
>     stack_alignment: integer       // REQUIRED: Required stack alignment in bytes
>     notes: string?                 // OPTIONAL: Additional notes
>   }
>   type_mappings: array {           // REQUIRED: FFI type mappings
>     cursive_type: string           // REQUIRED: Cursive type name
>     foreign_type: string           // REQUIRED: Corresponding foreign type
>     notes: string?                 // OPTIONAL: Mapping notes
>   }
> }
> ```

**_Example:_**

```json
"ffi": {
  "calling_conventions": [
    {
      "name": "C",
      "abi_standard": "System V AMD64 ABI",
      "parameter_passing": "First 6 integer args in registers (RDI, RSI, RDX, RCX, R8, R9), remainder on stack",
      "return_value_handling": "Integer/pointer returns in RAX, floating-point in XMM0",
      "stack_alignment": 16
    }
  ],
  "type_mappings": [
    {"cursive_type": "i32", "foreign_type": "int32_t"},
    {"cursive_type": "u64", "foreign_type": "uint64_t"},
    {"cursive_type": "usize", "foreign_type": "size_t"},
    {"cursive_type": "*const T", "foreign_type": "const T*"},
    {"cursive_type": "*mut T", "foreign_type": "T*"}
  ]
}
```

#### C.2.3.4 Memory Layout [appendix.c.structure.idb.layout]

> ```
> implementation_defined_behaviors.memory_layout {
>   struct_padding: string          // REQUIRED: Padding policy description
>   field_reordering: boolean       // REQUIRED: Whether field reordering is performed
>   union_layout: string            // REQUIRED: Union layout policy
>   enum_discriminant: string       // REQUIRED: Enum discriminant representation
>   vtable_layout: string           // REQUIRED: Virtual table layout description
>   notes: string?                  // OPTIONAL: Additional notes
> }
> ```

**_Example:_**

```json
"memory_layout": {
  "struct_padding": "Fields aligned to their natural alignment; padding inserted as needed; no trailing padding unless #[repr(C)]",
  "field_reordering": false,
  "union_layout": "All variants share offset 0; size is maximum variant size rounded to alignment",
  "enum_discriminant": "Smallest integer type sufficient to represent all variants; placed at offset 0",
  "vtable_layout": "Function pointers in declaration order; drop glue at index 0",
  "notes": "Layout guarantees apply only to #[repr(C)] and #[repr(transparent)] types"
}
```

#### C.2.3.5 Signed Integer Overflow [appendix.c.structure.idb.overflow]

> ```
> implementation_defined_behaviors.signed_integer_overflow {
>   debug_mode: string              // REQUIRED: Behavior in debug mode
>   release_mode: string            // REQUIRED: Behavior in release mode
>   detection_method: string        // REQUIRED: How overflow is detected (if applicable)
>   notes: string?                  // OPTIONAL: Additional notes
> }
> ```

**_Example:_**

```json
"signed_integer_overflow": {
  "debug_mode": "Panic on overflow with diagnostic message",
  "release_mode": "Two's complement wrapping",
  "detection_method": "LLVM overflow intrinsics in debug mode",
  "notes": "Use checked_* methods for explicit overflow handling"
}
```

#### C.2.3.6 Concurrency and Memory Ordering [appendix.c.structure.idb.concurrency]

> ```
> implementation_defined_behaviors.concurrency {
>   memory_model: string            // REQUIRED: Memory model description
>   atomic_support: array {         // REQUIRED: Supported atomic operations
>     type: string                  // REQUIRED: Atomic type (e.g., "AtomicI32")
>     supported_orderings: array    // REQUIRED: Array of supported orderings
>     lock_free: boolean            // REQUIRED: Whether lock-free
>   }
>   thread_scheduling: string       // REQUIRED: Thread scheduling policy
>   synchronization_primitives: array  // REQUIRED: Available primitives
>   notes: string?                  // OPTIONAL: Additional notes
> }
> ```

**_Example:_**

```json
"concurrency": {
  "memory_model": "C++11-style memory model with sequential consistency by default",
  "atomic_support": [
    {
      "type": "AtomicI32",
      "supported_orderings": ["Relaxed", "Acquire", "Release", "AcqRel", "SeqCst"],
      "lock_free": true
    },
    {
      "type": "AtomicI64",
      "supported_orderings": ["Relaxed", "Acquire", "Release", "AcqRel", "SeqCst"],
      "lock_free": true
    }
  ],
  "thread_scheduling": "OS-scheduled preemptive multithreading",
  "synchronization_primitives": ["Mutex", "RwLock", "Condvar", "Barrier", "Channel"],
  "notes": "Lock-free guarantees depend on target hardware support"
}
```

#### C.2.3.7 Panic and Unwind Behavior [appendix.c.structure.idb.panic]

> ```
> implementation_defined_behaviors.panic_behavior {
>   default_strategy: string        // REQUIRED: "unwind" | "abort"
>   unwind_mechanism: string?       // REQUIRED if unwind: Description of unwind mechanism
>   cross_ffi_unwind: string        // REQUIRED: Behavior when unwinding across FFI boundaries
>   panic_hook_support: boolean     // REQUIRED: Whether custom panic hooks are supported
>   notes: string?                  // OPTIONAL: Additional notes
> }
> ```

**_Example:_**

```json
"panic_behavior": {
  "default_strategy": "unwind",
  "unwind_mechanism": "DWARF-based unwinding compatible with C++ exceptions",
  "cross_ffi_unwind": "Unwinding across extern \"C\" boundaries is undefined behavior; use extern \"C-unwind\" for safe unwinding",
  "panic_hook_support": true,
  "notes": "Use -C panic=abort to disable unwinding"
}
```

#### C.2.3.8 Additional IDB Categories [appendix.c.structure.idb.additional]

> Implementations **MAY** include additional IDB categories beyond those required by §6.1.4.1. Additional categories **MUST** follow the same structured documentation approach and **SHOULD** include a `notes` field explaining why the behavior is implementation-defined.

### C.2.4 Implementation Limits Section [appendix.c.structure.limits]

> The `implementation_limits` section **MUST** document all limits that differ from the minimum guaranteed limits specified in §6.4.1. Limits that match the specification minimum **MAY** be omitted.
>
> ```
> implementation_limits {
>   source_text: object {
>     max_file_size_bytes: integer?
>     max_identifier_length: integer?
>     max_string_literal_length: integer?
>   }
>   syntactic_nesting: object {
>     max_block_nesting: integer?
>     max_expression_nesting: integer?
>     max_type_nesting: integer?
>     max_delimiter_nesting: integer?
>   }
>   declarations: object {
>     max_procedure_parameters: integer?
>     max_record_fields: integer?
>     max_enum_variants: integer?
>     max_modal_states: integer?
>     max_generic_parameters: integer?
>   }
>   comptime: object {
>     max_recursion_depth: integer?
>     max_evaluation_steps: integer?
>     max_memory_bytes: integer?
>     max_string_size_bytes: integer?
>     max_collection_elements: integer?
>   }
>   name_resolution: object {
>     max_qualified_name_components: integer?
>     max_scope_depth: integer?
>   }
>   generics: object {
>     max_monomorphization_instantiations: integer?
>     max_instantiation_nesting: integer?
>     max_unification_variables: integer?
>   }
>   platform_specific: object?       // OPTIONAL: Platform-dependent limits
>   notes: string?                   // OPTIONAL: General notes about limits
> }
> ```
>
> Each limit field is **OPTIONAL**. Omitted fields indicate the implementation supports the specification minimum (§6.4.1).

**_Example:_**

```json
"implementation_limits": {
  "source_text": {
    "max_file_size_bytes": 16777216,
    "max_identifier_length": 2047
  },
  "comptime": {
    "max_recursion_depth": 512,
    "max_evaluation_steps": 10000000,
    "max_memory_bytes": 134217728
  },
  "generics": {
    "max_monomorphization_instantiations": 4096
  },
  "platform_specific": {
    "max_stack_size_bytes": 8388608,
    "max_thread_count": 1024
  },
  "notes": "Stack size and thread count limits are configurable via environment variables"
}
```

### C.2.5 Extensions Section [appendix.c.structure.extensions]

> The `extensions` section **MUST** enumerate all supported extensions (§7.3).
>
> ```
> extensions {
>   supported: array {               // REQUIRED: Array of supported extensions
>     identifier: string             // REQUIRED: Extension identifier (dotted format)
>     stability: string              // REQUIRED: "stable" | "preview" | "experimental"
>     specification_version: string? // OPTIONAL: Spec version introducing the extension
>     target_stabilization: string?  // REQUIRED for preview: Target stabilization version
>     description: string            // REQUIRED: Brief description
>     documentation_url: string?     // OPTIONAL: Extension documentation URL
>     feature_flag_required: boolean // REQUIRED: Whether explicit opt-in is required
>   }
>   namespaces: array?               // OPTIONAL: Reserved vendor namespaces
>   notes: string?                   // OPTIONAL: General notes about extensions
> }
> ```

**_Example:_**

```json
"extensions": {
  "supported": [
    {
      "identifier": "inline_assembly",
      "stability": "stable",
      "specification_version": "1.0.0",
      "description": "Inline assembly support for target architecture",
      "documentation_url": "https://docs.cursive-lang.org/asm",
      "feature_flag_required": false
    },
    {
      "identifier": "simd_intrinsics",
      "stability": "preview",
      "target_stabilization": "1.1.0",
      "description": "SIMD intrinsics for vectorized operations",
      "documentation_url": "https://docs.cursive-lang.org/simd",
      "feature_flag_required": true
    },
    {
      "identifier": "cursivefoundation.com.wasm_extensions",
      "stability": "experimental",
      "description": "WebAssembly-specific extensions for WASI",
      "feature_flag_required": true
    }
  ],
  "namespaces": ["cursivefoundation.com"],
  "notes": "Enable extensions via [features] section in project manifest"
}
```

### C.2.6 Diagnostics Section [appendix.c.structure.diagnostics]

> The `diagnostics` section **MUST** document diagnostic system capabilities and characteristics.
>
> ```
> diagnostics {
>   taxonomy_version: string        // REQUIRED: Version of diagnostic taxonomy (Appendix B)
>   supported_categories: array     // REQUIRED: Supported diagnostic categories
>   output_formats: array           // REQUIRED: Supported output formats
>   diagnostic_limit: integer?      // OPTIONAL: Maximum diagnostics before stopping
>   json_schema_url: string?        // OPTIONAL: URL for JSON diagnostic schema
>   notes: string?                  // OPTIONAL: Additional notes
> }
> ```

**_Example:_**

```json
"diagnostics": {
  "taxonomy_version": "1.0.0",
  "supported_categories": ["SRC", "TYP", "MEM", "EXP", "GRN", "FFI", "DIA", "CNF"],
  "output_formats": ["human", "json", "json-streaming", "sarif"],
  "diagnostic_limit": 100,
  "json_schema_url": "https://docs.cursive-lang.org/schemas/diagnostics-1.0.json",
  "notes": "Use --diagnostic-format to select output format; --diagnostic-limit to override limit"
}
```

> The `taxonomy_version` **MUST** correspond to the MAJOR.MINOR version of the diagnostic taxonomy defined in Appendix B for the specification version indicated by `implementation.specification_version`. Validators **MUST** treat a dossier as invalid when `taxonomy_version` does not match the taxonomy version associated with the referenced specification version, and such mismatches **SHOULD** be reported using `E-DIA-0301` or `E-DIA-0302` as appropriate.
>
> When `"sarif"` appears in `output_formats`, the SARIF output **MUST** conform to a pinned SARIF version (such as 2.1.0) that is documented either in the dossier or in implementation documentation; consumers **SHOULD** assume SARIF 2.1.0 unless a different version is explicitly stated. When `"json"` or `"json-streaming"` appears, the `json_schema_url` field **SHOULD** reference a JSON Schema that describes the structure of the diagnostic payload.

### C.2.7 UVB Attestations Section [appendix.c.structure.uvb]

> The `uvb_attestations` section **MUST** be included in program-specific or library-specific conformance dossiers when Unverifiable Behavior operations are present. This section **MUST** enumerate all UVB operations that are reachable in the link‑closed program image together with their external verification artifacts (§6.2.2).
>
> ```
> uvb_attestations {
>   project_identifier: string      // REQUIRED: Project name and version
>   attestation_date: string        // REQUIRED: ISO 8601 date of attestation
>   attestations: array {           // REQUIRED: Array of individual attestations
>     id: string                    // REQUIRED: Unique attestation identifier
>     source_location: object {     // REQUIRED: Source location of UVB operation
>       file: string                // REQUIRED: Relative file path
>       line: integer               // REQUIRED: Line number
>       column: integer             // REQUIRED: Column number
>       span_start: integer?        // OPTIONAL: Character offset start
>       span_end: integer?          // OPTIONAL: Character offset end
>     }
>     uvb_category: string          // REQUIRED: Category of UVB (see §C.2.7.1 for allowed values)
>     operation_description: string // REQUIRED: Human-readable description of operation
>     verification_method: string   // REQUIRED: "formal_proof" | "manual_audit" | "test_coverage" | "external_contract"
>     proof_artifact: object {      // REQUIRED: Proof artifact reference
>       type: string                // REQUIRED: "uri" | "embedded_hash" | "external_reference"
>       value: string               // REQUIRED: URI, hash, or reference
>       hash_algorithm: string?     // REQUIRED if type=embedded_hash: Hash algorithm (e.g., "SHA256")
>     }
>     verifier_identity: string?    // OPTIONAL: Identity of verifier
>     verification_date: string?    // OPTIONAL: ISO 8601 date of verification
>     expiration_date: string?      // OPTIONAL: ISO 8601 date when attestation expires
>     notes: string?                // OPTIONAL: Additional context
>   }
>   aggregate_proof_uri: string?    // OPTIONAL: URI to aggregate proof document
>   attestation_policy: string?     // OPTIONAL: Description of project attestation policy
>   notes: string?                  // OPTIONAL: General notes about attestations
> }
> ```
>
> **Conformance tools** (build systems, CI/CD pipelines) **MUST** verify that every UVB site that is reachable in the link‑closed program image (including all linked artifacts that contribute to the final executable image) has a corresponding attestation entry before producing executable artifacts (§6.2.4). Attestations recorded in this section **MUST** cover UVB sites in both the primary program and any linked libraries or foreign modules that the conformance claim includes.

**_Example:_**

```json
"uvb_attestations": {
  "project_identifier": "secure-crypto-lib v2.3.1",
  "attestation_date": "2025-11-10",
  "attestations": [
    {
      "id": "uvb-001",
      "source_location": {
        "file": "src/ffi/openssl_bindings.cursive",
        "line": 127,
        "column": 9,
        "span_start": 4832,
        "span_end": 4891
      },
      "uvb_category": "ffi_pointer_deref",
      "operation_description": "Dereferencing pointer returned from OpenSSL EVP_CIPHER_CTX_new()",
      "verification_method": "manual_audit",
      "proof_artifact": {
        "type": "uri",
        "value": "https://audits.example.com/secure-crypto-lib/2025/uvb-001-audit-report.pdf"
      },
      "verifier_identity": "Jane Security <jane@security-auditors.com>",
      "verification_date": "2025-11-08",
      "notes": "Verified that OpenSSL contract guarantees non-null return for successful allocation"
    },
    {
      "id": "uvb-002",
      "source_location": {
        "file": "src/unsafe/simd_operations.cursive",
        "line": 89,
        "column": 5
      },
      "uvb_category": "unsafe_block",
      "operation_description": "Unsafe pointer arithmetic for SIMD buffer access",
      "verification_method": "formal_proof",
      "proof_artifact": {
        "type": "embedded_hash",
        "value": "a3f7c2e9d1b4f8e6c5a7d9f2e1c8b4a6f3e7d2c9a1b8f4e6c7d9a2f1e8b5c3d7",
        "hash_algorithm": "SHA256"
      },
      "verifier_identity": "Formal Methods Team",
      "verification_date": "2025-11-09",
      "notes": "Proof verified using Coq; see internal proof repository"
    }
  ],
  "aggregate_proof_uri": "https://docs.example.com/secure-crypto-lib/safety-proof-v2.3.1.pdf",
  "attestation_policy": "All FFI operations require manual security audit; unsafe blocks require formal verification or comprehensive test coverage",
  "notes": "This attestation set covers release v2.3.1; re-verification required for subsequent releases"
}
```

---

### C.2.8 Partial Conformance Section [appendix.c.structure.partial]

> When an implementation claims partial conformance (Appendix E, §E.5), the dossier **MUST** include a `partial_conformance` section that enumerates the deviations from full conformance and their rationale.
>
> ```
> partial_conformance {
>   entries: array {               // REQUIRED: Array of partial-conformance entries
>     section: string              // REQUIRED: Section or anchor identifier (e.g., "§6.4.1 [conformance.limits.minimum]")
>     anchor: string?              // OPTIONAL: Stable semantic anchor (e.g., "conformance.limits.minimum")
>     kind: string?                // OPTIONAL: "unsupported" | "restricted" | "relaxed"
>     reason: string               // REQUIRED: Human-readable explanation
>     notes: string?               // OPTIONAL: Additional context or migration guidance
>   }
> }
> ```
>
> Each entry **MUST** identify the affected specification requirement by section number and, where available, semantic anchor, and **MUST** clearly state whether the feature is entirely unsupported, supported with stricter limits, or supported with relaxed guarantees. Tooling that consumes dossiers **MAY** use this section to filter or annotate diagnostics for features outside the claimed conformance subset.

**_Example:_**

```json
"partial_conformance": {
  "entries": [
    {
      "section": "§11.6 [translation.comptime]",
      "anchor": "translation.comptime",
      "kind": "restricted",
      "reason": "Comptime recursion depth limited to 64 frames on this embedded target",
      "notes": "Diagnostic E-SRC-0201 is issued when exceeding this lower limit"
    },
    {
      "section": "Part XI [concurrency.model]",
      "anchor": "concurrency.model",
      "kind": "unsupported",
      "reason": "Target runtime is single-threaded; no shared-memory concurrency is provided"
    }
  ]
}
```

---

## C.3 Normative JSON Schema [appendix.c.json]

> The following JSON Schema formally specifies the conformance dossier structure for implementations choosing JSON format. This schema is **normative**.
>
> Implementations **MUST** produce dossiers that validate against this schema. Validation tools **SHOULD** use this schema to verify dossier conformance.

**_JSON Schema Specification:_**

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://cursive-lang.org/schemas/conformance-dossier/1.0.json",
  "title": "Cursive Conformance Dossier",
  "description": "Normative schema for Cursive language implementation conformance dossiers",
  "type": "object",
  "required": [
    "implementation",
    "target",
    "implementation_defined_behaviors",
    "implementation_limits",
    "extensions",
    "diagnostics"
  ],
  "properties": {
    "implementation": {
      "type": "object",
      "required": ["name", "version", "vendor", "release_date", "specification_version"],
      "properties": {
        "name": {"type": "string"},
        "version": {"type": "string", "pattern": "^(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)(?:-[0-9A-Za-z-.]+)?(?:\\+[0-9A-Za-z-.]+)?$"},
        "vendor": {"type": "string"},
        "release_date": {"type": "string", "format": "date"},
        "specification_version": {"type": "string", "pattern": "^(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)(?:-[0-9A-Za-z-.]+)?(?:\\+[0-9A-Za-z-.]+)?$"},
        "homepage": {"type": "string", "format": "uri"},
        "documentation_url": {"type": "string", "format": "uri"},
        "contact": {"type": "string"},
        "build_identifier": {"type": "string"},
        "build_config": {
          "type": "object",
          "properties": {
            "optimization": {"type": "string"},
            "conformance_mode": {"type": "string"},
            "feature_flags": {
              "type": "array",
              "items": {"type": "string"}
            },
            "lto": {"type": "string"}
          }
        }
      }
    },
    "target": {
      "type": "object",
      "required": ["triple", "architecture", "operating_system", "pointer_width", "endianness", "default_alignment", "max_alignment"],
      "properties": {
        "triple": {"type": "string"},
        "architecture": {"type": "string"},
        "operating_system": {"type": "string"},
        "environment": {"type": "string"},
        "pointer_width": {"type": "integer", "enum": [32, 64, 128]},
        "endianness": {"type": "string", "enum": ["little", "big"]},
        "default_alignment": {"type": "integer", "minimum": 1},
        "max_alignment": {"type": "integer", "minimum": 1}
      }
    },
    "implementation_defined_behaviors": {
      "type": "object",
      "required": ["primitive_types", "pointer_semantics", "ffi", "memory_layout", "signed_integer_overflow", "concurrency", "panic_behavior"],
      "properties": {
        "primitive_types": {
          "type": "array",
          "items": {
            "type": "object",
            "required": ["type", "size", "alignment", "representation"],
            "properties": {
              "type": {"type": "string"},
              "size": {"type": "integer", "minimum": 1},
              "alignment": {"type": "integer", "minimum": 1},
              "representation": {"type": "string"},
              "min_value": {"type": "string"},
              "max_value": {"type": "string"},
              "notes": {"type": "string"}
            }
          }
        },
        "pointer_semantics": {
          "type": "object",
          "required": ["null_representation", "provenance_tracking", "comparison_semantics", "arithmetic_behavior"],
          "properties": {
            "null_representation": {"type": "string"},
            "provenance_tracking": {"type": "string", "enum": ["strict", "permissive", "none"]},
            "comparison_semantics": {"type": "string"},
            "arithmetic_behavior": {"type": "string"},
            "notes": {"type": "string"}
          }
        },
        "ffi": {
          "type": "object",
          "required": ["calling_conventions", "type_mappings"],
          "properties": {
            "calling_conventions": {
              "type": "array",
              "minItems": 1,
              "items": {
                "type": "object",
                "required": ["name", "abi_standard", "parameter_passing", "return_value_handling", "stack_alignment"],
                "properties": {
                  "name": {"type": "string"},
                  "abi_standard": {"type": "string"},
                  "parameter_passing": {"type": "string"},
                  "return_value_handling": {"type": "string"},
                  "stack_alignment": {"type": "integer", "minimum": 1},
                  "notes": {"type": "string"}
                }
              }
            },
            "type_mappings": {
              "type": "array",
              "items": {
                "type": "object",
                "required": ["cursive_type", "foreign_type"],
                "properties": {
                  "cursive_type": {"type": "string"},
                  "foreign_type": {"type": "string"},
                  "notes": {"type": "string"}
                }
              }
            }
          }
        },
        "memory_layout": {
          "type": "object",
          "required": ["struct_padding", "field_reordering", "union_layout", "enum_discriminant", "vtable_layout"],
          "properties": {
            "struct_padding": {"type": "string"},
            "field_reordering": {"type": "boolean"},
            "union_layout": {"type": "string"},
            "enum_discriminant": {"type": "string"},
            "vtable_layout": {"type": "string"},
            "notes": {"type": "string"}
          }
        },
        "signed_integer_overflow": {
          "type": "object",
          "required": ["debug_mode", "release_mode", "detection_method"],
          "properties": {
            "debug_mode": {"type": "string"},
            "release_mode": {"type": "string"},
            "detection_method": {"type": "string"},
            "notes": {"type": "string"}
          }
        },
        "concurrency": {
          "type": "object",
          "required": ["memory_model", "atomic_support", "thread_scheduling", "synchronization_primitives"],
          "properties": {
            "memory_model": {"type": "string"},
            "atomic_support": {
              "type": "array",
              "items": {
                "type": "object",
                "required": ["type", "supported_orderings", "lock_free"],
                "properties": {
                  "type": {"type": "string"},
                  "supported_orderings": {"type": "array", "items": {"type": "string"}},
                  "lock_free": {"type": "boolean"}
                }
              }
            },
            "thread_scheduling": {"type": "string"},
            "synchronization_primitives": {"type": "array", "items": {"type": "string"}},
            "notes": {"type": "string"}
          }
        },
        "panic_behavior": {
          "type": "object",
          "required": ["default_strategy", "cross_ffi_unwind", "panic_hook_support"],
          "properties": {
            "default_strategy": {"type": "string", "enum": ["unwind", "abort"]},
            "unwind_mechanism": {"type": "string"},
            "cross_ffi_unwind": {"type": "string"},
            "panic_hook_support": {"type": "boolean"},
            "notes": {"type": "string"}
          }
        }
      }
    },
    "implementation_limits": {
      "type": "object",
      "properties": {
        "source_text": {
          "type": "object",
          "properties": {
            "max_file_size_bytes": {"type": "integer", "minimum": 1048576},
            "max_identifier_length": {"type": "integer", "minimum": 1023},
            "max_string_literal_length": {"type": "integer", "minimum": 65535}
          }
        },
        "syntactic_nesting": {
          "type": "object",
          "properties": {
            "max_block_nesting": {"type": "integer", "minimum": 256},
            "max_expression_nesting": {"type": "integer", "minimum": 256},
            "max_type_nesting": {"type": "integer", "minimum": 128},
            "max_delimiter_nesting": {"type": "integer", "minimum": 256}
          }
        },
        "declarations": {
          "type": "object",
          "properties": {
            "max_procedure_parameters": {"type": "integer", "minimum": 255},
            "max_record_fields": {"type": "integer", "minimum": 1024},
            "max_enum_variants": {"type": "integer", "minimum": 1024},
            "max_modal_states": {"type": "integer", "minimum": 64},
            "max_generic_parameters": {"type": "integer", "minimum": 256}
          }
        },
        "comptime": {
          "type": "object",
          "properties": {
            "max_recursion_depth": {"type": "integer", "minimum": 256},
            "max_evaluation_steps": {"type": "integer", "minimum": 1000000},
            "max_memory_bytes": {"type": "integer", "minimum": 67108864},
            "max_string_size_bytes": {"type": "integer", "minimum": 1048576},
            "max_collection_elements": {"type": "integer", "minimum": 10000}
          }
        },
        "name_resolution": {
          "type": "object",
          "properties": {
            "max_qualified_name_components": {"type": "integer", "minimum": 32},
            "max_scope_depth": {"type": "integer", "minimum": 256}
          }
        },
        "generics": {
          "type": "object",
          "properties": {
            "max_monomorphization_instantiations": {"type": "integer", "minimum": 1024},
            "max_instantiation_nesting": {"type": "integer", "minimum": 32},
            "max_unification_variables": {"type": "integer", "minimum": 1024}
          }
        },
        "platform_specific": {"type": "object"},
        "notes": {"type": "string"}
      }
    },
    "extensions": {
      "type": "object",
      "required": ["supported"],
      "properties": {
        "supported": {
          "type": "array",
          "items": {
            "type": "object",
            "required": ["identifier", "stability", "description", "feature_flag_required"],
            "properties": {
              "identifier": {"type": "string"},
              "stability": {"type": "string", "enum": ["stable", "preview", "experimental"]},
              "specification_version": {"type": "string", "pattern": "^(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)(?:-[0-9A-Za-z-.]+)?(?:\\+[0-9A-Za-z-.]+)?$"},
              "target_stabilization": {"type": "string", "pattern": "^(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)(?:-[0-9A-Za-z-.]+)?(?:\\+[0-9A-Za-z-.]+)?$"},
              "description": {"type": "string"},
              "documentation_url": {"type": "string", "format": "uri"},
              "feature_flag_required": {"type": "boolean"}
            },
            "allOf": [
              {
                "if": {
                  "properties": { "stability": { "const": "preview" } }
                },
                "then": {
                  "required": ["target_stabilization"]
                }
              }
            ]
          }
        },
        "namespaces": {"type": "array", "items": {"type": "string"}},
        "notes": {"type": "string"}
      }
    },
    "diagnostics": {
      "type": "object",
      "required": ["taxonomy_version", "supported_categories", "output_formats"],
      "properties": {
        "taxonomy_version": {"type": "string", "pattern": "^(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)(?:-[0-9A-Za-z-.]+)?(?:\\+[0-9A-Za-z-.]+)?$"},
        "supported_categories": {"type": "array", "items": {"type": "string"}},
        "output_formats": {"type": "array", "items": {"type": "string"}},
        "diagnostic_limit": {"type": "integer", "minimum": 1},
        "json_schema_url": {"type": "string", "format": "uri"},
        "notes": {"type": "string"}
      }
    },
    "uvb_attestations": {
      "type": "object",
      "required": ["project_identifier", "attestation_date", "attestations"],
      "properties": {
        "project_identifier": {"type": "string"},
        "attestation_date": {"type": "string", "format": "date"},
        "attestations": {
          "type": "array",
          "items": {
            "type": "object",
            "required": ["id", "source_location", "uvb_category", "operation_description", "verification_method", "proof_artifact"],
            "properties": {
              "id": {"type": "string"},
              "source_location": {
                "type": "object",
                "required": ["file", "line", "column"],
                "properties": {
                  "file": {"type": "string"},
                  "line": {"type": "integer", "minimum": 1},
                  "column": {"type": "integer", "minimum": 1},
                  "span_start": {"type": "integer", "minimum": 0},
                  "span_end": {"type": "integer", "minimum": 0}
                }
              },
              "uvb_category": {
                "type": "string",
                "enum": ["ffi_pointer_deref", "unsafe_block", "contract_unverifiable", "platform_undefined"]
              },
              "operation_description": {"type": "string"},
              "verification_method": {"type": "string", "enum": ["formal_proof", "manual_audit", "test_coverage", "external_contract"]},
              "proof_artifact": {
                "type": "object",
                "required": ["type", "value"],
                "properties": {
                  "type": {"type": "string", "enum": ["uri", "embedded_hash", "external_reference"]},
                  "value": {"type": "string"},
                  "hash_algorithm": {"type": "string"}
                }
              },
              "verifier_identity": {"type": "string"},
              "verification_date": {"type": "string", "format": "date"},
              "expiration_date": {"type": "string", "format": "date"},
              "notes": {"type": "string"}
            }
          }
        },
        "aggregate_proof_uri": {"type": "string", "format": "uri"},
        "attestation_policy": {"type": "string"},
        "notes": {"type": "string"}
      }
    },
    "partial_conformance": {
      "type": "object",
      "properties": {
        "entries": {
          "type": "array",
          "items": {
            "type": "object",
            "required": ["section", "reason"],
            "properties": {
              "section": {"type": "string"},
              "anchor": {"type": "string"},
              "kind": {"type": "string", "enum": ["unsupported", "restricted", "relaxed"]},
              "reason": {"type": "string"},
              "notes": {"type": "string"}
            }
          }
        }
      }
    }
  }
}
```

---

## C.4 TOML Format Specification [appendix.c.toml]

> Implementations **MAY** provide conformance dossiers in TOML format as an alternative to JSON. The TOML structure **MUST** follow the same semantic organization as the JSON schema (§C.3), with appropriate TOML syntax adaptations.

**_TOML Format Example:_**

```toml
# Cursive Conformance Dossier - TOML Format
# Specification Version: 1.0.0

[implementation]
name = "cursivec"
version = "1.2.3"
vendor = "Cursive Foundation"
release_date = "2025-11-13"
specification_version = "1.0.0"
homepage = "https://cursive-lang.org"
documentation_url = "https://docs.cursive-lang.org/conformance/1.2.3"
contact = "support@cursive-lang.org"
build_identifier = "a1b2c3d4e5f67890"

[target]
triple = "x86_64-unknown-linux-gnu"
architecture = "x86_64"
operating_system = "linux"
environment = "gnu"
pointer_width = 64
endianness = "little"
default_alignment = 8
max_alignment = 4096

[implementation_defined_behaviors.pointer_semantics]
null_representation = "All-bits-zero (0x0)"
provenance_tracking = "strict"
comparison_semantics = "Pointers from different allocations are unordered"
arithmetic_behavior = "Checked in debug, unchecked in release"

[[implementation_defined_behaviors.primitive_types]]
type = "i32"
size = 4
alignment = 4
representation = "Two's complement"
min_value = "-2147483648"
max_value = "2147483647"

[[implementation_defined_behaviors.primitive_types]]
type = "usize"
size = 8
alignment = 8
representation = "Unsigned integer, platform word size"
min_value = "0"
max_value = "18446744073709551615"

[implementation_defined_behaviors.ffi]

[[implementation_defined_behaviors.ffi.calling_conventions]]
name = "C"
abi_standard = "System V AMD64 ABI"
parameter_passing = "First 6 integer args in registers"
return_value_handling = "Integer returns in RAX"
stack_alignment = 16

[[implementation_defined_behaviors.ffi.type_mappings]]
cursive_type = "i32"
foreign_type = "int32_t"

[[implementation_defined_behaviors.ffi.type_mappings]]
cursive_type = "usize"
foreign_type = "size_t"

[implementation_defined_behaviors.memory_layout]
struct_padding = "Fields aligned to natural alignment"
field_reordering = false
union_layout = "All variants share offset 0"
enum_discriminant = "Smallest sufficient integer type"
vtable_layout = "Function pointers in declaration order"

[implementation_defined_behaviors.signed_integer_overflow]
debug_mode = "Panic on overflow"
release_mode = "Two's complement wrapping"
detection_method = "LLVM overflow intrinsics"

[implementation_defined_behaviors.concurrency]
memory_model = "C++11-style sequential consistency"
thread_scheduling = "OS-scheduled preemptive"
synchronization_primitives = ["Mutex", "RwLock", "Condvar"]

[[implementation_defined_behaviors.concurrency.atomic_support]]
type = "AtomicI32"
supported_orderings = ["Relaxed", "Acquire", "Release", "SeqCst"]
lock_free = true

[implementation_defined_behaviors.panic_behavior]
default_strategy = "unwind"
unwind_mechanism = "DWARF-based"
cross_ffi_unwind = "Undefined across extern C boundaries"
panic_hook_support = true

[implementation_limits]

[implementation_limits.source_text]
max_file_size_bytes = 16777216
max_identifier_length = 2047

[implementation_limits.comptime]
max_recursion_depth = 512
max_evaluation_steps = 10000000
max_memory_bytes = 134217728

[extensions]

[[extensions.supported]]
identifier = "inline_assembly"
stability = "stable"
description = "Inline assembly support"
feature_flag_required = false

[[extensions.supported]]
identifier = "simd_intrinsics"
stability = "preview"
target_stabilization = "1.1.0"
description = "SIMD intrinsics"
feature_flag_required = true

[diagnostics]
taxonomy_version = "1.0.0"
supported_categories = ["SRC", "TYP", "MEM", "EXP", "GRN", "FFI"]
output_formats = ["human", "json", "sarif"]
diagnostic_limit = 100

# UVB attestations section (program-specific dossiers only)
[uvb_attestations]
project_identifier = "secure-crypto-lib v2.3.1"
attestation_date = "2025-11-10"

[[uvb_attestations.attestations]]
id = "uvb-001"
uvb_category = "ffi_pointer_deref"
operation_description = "OpenSSL pointer dereference"
verification_method = "manual_audit"
verifier_identity = "Jane Security"
verification_date = "2025-11-08"

[uvb_attestations.attestations.source_location]
file = "src/ffi/openssl_bindings.cursive"
line = 127
column = 9

[uvb_attestations.attestations.proof_artifact]
type = "uri"
value = "https://audits.example.com/report.pdf"
```

---

## C.5 Validation and Tooling Requirements [appendix.c.validation]

### C.5.1 Schema Validation [appendix.c.validation.schema]

> Implementations **SHOULD** provide tooling to validate conformance dossiers against the normative schema (§C.3).
>
> Conformance verification tools (build systems, CI/CD pipelines) **SHOULD** validate dossiers before relying on their contents.
>
> Invalid dossiers **SHOULD** trigger warnings or errors as appropriate for the use case.

### C.5.2 Attestation Verification [appendix.c.validation.attestation]

> Build tools processing programs with UVB operations **MUST** verify that:
>
> 1. The `uvb_attestations` section exists in the project-specific dossier
> 2. Every UVB site that is reachable in the link‑closed program image has a corresponding attestation entry
> 3. Each attestation includes a valid `proof_artifact` reference
> 4. Attestations have not expired (if `expiration_date` is present)
>
> Verification failures **MUST** prevent executable artifact production and **MUST** issue error diagnostics identifying missing or expired attestations.

### C.5.3 Dossier Generation [appendix.c.validation.generation]

> Implementations **SHOULD** provide automated tooling to generate conformance dossiers from implementation metadata.
>
> Generated dossiers **MUST** be valid according to the normative schema (§C.3).
>
> Implementations **MAY** support dossier templates or partial generation with manual completion required for certain fields (e.g., UVB attestations).

### C.5.4 Interoperability Testing [appendix.c.validation.interoperability]

> Conformance test suites **SHOULD** include tests verifying:
>
> 1. Dossier schema validity
> 2. Completeness of required sections
> 3. Consistency between documented behaviors and actual implementation behavior
> 4. Proper encoding and accessibility of dossier files

---

## C.6 Examples and Reference Implementations [appendix.c.examples]

### C.6.1 Complete JSON Dossier Example [appendix.c.examples.complete]

> A complete, normative example conformance dossier is available at:
> `https://cursive-lang.org/conformance/examples/complete-dossier-1.0.json`
>
> This example **SHOULD** be used as a reference for implementation authors and tool developers.

### C.6.2 Minimal Valid Dossier [appendix.c.examples.minimal]

> The following represents a minimal valid conformance dossier meeting all REQUIRED field obligations for a single‑threaded, abort‑on‑panic embedded profile. A second, more feature‑rich example for a multi‑threaded target with unwinding enabled is available as a separate reference artifact alongside the complete example dossier (§C.6.1).

```json
{
  "implementation": {
    "name": "minimal-cursive",
    "version": "1.0.0",
    "vendor": "Example Vendor",
    "release_date": "2025-11-13",
    "specification_version": "1.0.0"
  },
  "target": {
    "triple": "x86_64-unknown-linux-gnu",
    "architecture": "x86_64",
    "operating_system": "linux",
    "pointer_width": 64,
    "endianness": "little",
    "default_alignment": 8,
    "max_alignment": 4096
  },
  "implementation_defined_behaviors": {
    "primitive_types": [
      {"type": "i32", "size": 4, "alignment": 4, "representation": "Two's complement"}
    ],
    "pointer_semantics": {
      "null_representation": "0x0",
      "provenance_tracking": "strict",
      "comparison_semantics": "Address-based",
      "arithmetic_behavior": "Checked"
    },
    "ffi": {
      "calling_conventions": [{
        "name": "C",
        "abi_standard": "System V AMD64",
        "parameter_passing": "Register and stack",
        "return_value_handling": "Register",
        "stack_alignment": 16
      }],
      "type_mappings": [
        {"cursive_type": "i32", "foreign_type": "int32_t"}
      ]
    },
    "memory_layout": {
      "struct_padding": "Natural alignment",
      "field_reordering": false,
      "union_layout": "Shared offset",
      "enum_discriminant": "Smallest integer",
      "vtable_layout": "Declaration order"
    },
    "signed_integer_overflow": {
      "debug_mode": "panic",
      "release_mode": "wrap",
      "detection_method": "compiler intrinsics"
    },
    "concurrency": {
      "memory_model": "single-threaded (no shared-memory concurrency)",
      "atomic_support": [],
      "thread_scheduling": "none",
      "synchronization_primitives": []
    },
    "panic_behavior": {
      "default_strategy": "abort",
      "cross_ffi_unwind": "undefined",
      "panic_hook_support": false
    }
  },
  "implementation_limits": {},
  "extensions": {
    "supported": []
  },
  "diagnostics": {
    "taxonomy_version": "1.0.0",
    "supported_categories": ["SRC", "TYP"],
    "output_formats": ["human"]
  }
}
```

---

## C.7 Conformance and Forward Compatibility [appendix.c.conformance]

### C.7.1 Schema Versioning [appendix.c.conformance.versioning]

> The conformance dossier schema version **MUST** track the specification MAJOR and MINOR versions. Schema version `X.Y.Z` is compatible with specification version `X.Y.*`.
>
> MINOR version increments to the schema **MAY** add new optional fields but **MUST NOT** remove or alter existing required fields.
>
> MAJOR version increments **MAY** introduce breaking changes to the schema structure.

### C.7.2 Unknown Field Handling [appendix.c.conformance.unknown]

> Conformance tools and validators **MUST** ignore unknown fields in dossiers to support forward compatibility.
>
> Implementations **MAY** include vendor-specific extensions as additional fields, provided they do not conflict with normative field names.
>
> Extension fields **SHOULD** use namespaced keys (e.g., `"vendor.com:custom_field"`) to avoid conflicts with future specification additions.

### C.7.3 Deprecation Policy [appendix.c.conformance.deprecation]

> Fields in the conformance dossier schema follow the same deprecation policy as language features (§7.2.3).
>
> Deprecated fields **MUST** remain valid for at least one MINOR version after deprecation.
>
> Implementations **SHOULD** issue warnings when generating or consuming dossiers using deprecated fields.

---

**End of Appendix C**

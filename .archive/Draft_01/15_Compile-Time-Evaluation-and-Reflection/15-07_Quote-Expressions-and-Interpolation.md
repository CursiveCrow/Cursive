# Cursive Language Specification

## Clause 15 — Compile-Time Evaluation and Reflection

**Part**: XV — Compile-Time Evaluation and Reflection  
**File**: 16-7_Quote-Expressions-and-Interpolation.md  
**Section**: §15.7 Quote Expressions and Interpolation  
**Stable label**: [comptime.quote]  
**Forward references**: §16.2 [comptime.procedures], §16.3 [comptime.blocks], §16.8 [comptime.codegen.api], Clause 8 [expr], Annex A §A.4 [grammar.expression]

---

### §15.7 Quote Expressions and Interpolation [comptime.quote]

#### §15.7.1 Overview [comptime.quote.overview]

[1] _Quote expressions_ capture Cursive code as data for later emission during code generation. They enable type-safe code templates with compile-time value interpolation, supporting metaprogramming patterns without textual macros.

[2] Quote expressions use the `quote` keyword to delimit code and the `$(...)` syntax to interpolate compile-time values into the quoted code.

[3] This subclause specifies quote expression syntax, interpolation semantics, the QuotedBlock and QuotedExpr types, and integration with the code generation API (§16.8).

#### §15.7.2 Syntax [comptime.quote.syntax]

[4] Quote expression syntax:

**Quote expressions** take one of the following forms:
```
"quote" <block_expr>
"quote" <expression>
```

**Interpolations** take one of the following forms:
```
"$" "(" <expression> ")"
"$" <identifier>
```

[ Note: See Annex A §A.4 [grammar.expression] for the normative `quote_expr` production.
— end note ]

[5] **Quote forms:**

- `quote { statements }` — Quote block for procedure bodies, statement sequences
- `quote expression` — Quote single expression for inline values

[6] **Interpolation syntax:**

- `$(expression)` — Evaluate `expression` at compile time and splice result as literal value
- `$(identifier)` — When `identifier` binds to a string value, splice as identifier in generated code

[ Note: Quote interpolation uses the `$` (dollar sign) character. This is the designated interpolation syntax in Cursive. The `$` glyph is specifically reserved for this purpose.
— end note ]

#### §15.7.3 Constraints [comptime.quote.constraints]

##### §15.7.3.1 Context Requirements

[7] _Comptime-only._ Quote expressions shall appear only in comptime contexts:

- Within `comptime { ... }` blocks
- Within comptime procedure bodies
- As arguments to codegen API procedures

[8] Using quote expressions in runtime contexts produces diagnostic E16-200.

##### §15.7.3.2 Interpolation Constraints

[9] _Compile-time evaluability._ Expressions within `$(...)` shall be compile-time evaluable. They may reference:

- Comptime bindings (variables in comptime scope)
- Comptime constants
- Results of comptime procedures
- Comptime intrinsics

[10] Runtime-only expressions within `$(...)` produce diagnostic E16-201.

[11] _Interpolated value types._ Interpolated expressions shall produce values that can be embedded as literals:

- Primitive values (integers, floats, booleans, characters)
- String values (for both literal strings and identifier interpolation)
- Arrays of embeddable values
- Tuples of embeddable values

[12] Attempting to interpolate non-embeddable values (procedures, types, witnesses) produces diagnostic E16-202.

##### §15.7.3.3 Quoted Code Constraints

[13] _Syntactic validity._ The code within `quote { ... }` shall be syntactically valid Cursive code, though it need not be type-correct at quotation time.

[14] _Post-interpolation correctness._ After interpolation resolution, the generated code shall be type-correct and well-formed. Type errors in generated code are detected during type checking phase (§2.2.4.3) and produce diagnostic E16-203.

#### §15.7.4 Semantics [comptime.quote.semantics]

##### §15.7.4.1 Quote Evaluation

[15] Quote expressions evaluate to opaque QuotedBlock or QuotedExpr values:

[ Given: Quote expression `quote c` where `c` is code ]

$$
\frac{\Gamma_{\text{ct}} \vdash c : \text{syntactically valid}}{\Gamma_{\text{ct}} \vdash \texttt{quote } c : \texttt{QuotedBlock}}
\tag{T-Quote}
$$

[16] The quoted code `c` is captured as data without execution. Interpolations are not evaluated at quotation time.

##### §15.7.4.2 Interpolation Resolution

[17] When a quote is used in code generation, interpolations are evaluated and their values spliced into the code:

[ Given: Quote `quote { ... $(e_1) ... $(e_n) ... }` ]

$$
\frac{\forall i.\, \langle e_i, \sigma_{\text{ct}} \rangle \Downarrow_{\text{ct}} \langle v_i, \sigma_{\text{ct}} \rangle}{\texttt{quote } \{ \ldots $(e_i) \ldots \} \text{ resolves to code with } v_i \text{ spliced}}
\tag{E-Quote-Interpolate}
$$

[18] Each interpolated expression evaluates to a compile-time value. The value is converted to source code syntax and inserted at the interpolation site.

##### §15.7.4.3 Value Interpolation

[19] **Literal value splicing.** Interpolating a primitive value embeds that value as a literal:

```cursive
let multiplier: const usize = 10

let body = quote {
    result x * $(multiplier)
}

// After interpolation: result x * 10
```

[20] The compile-time value `10` is spliced as the literal `10` in the generated code.

##### §15.7.4.4 Identifier Interpolation

[21] **Identifier splicing.** Interpolating a string value as an identifier uses that string as an actual identifier in generated code:

```cursive
let field_name: const string@View = "balance"

let getter_body = quote {
    result self.$(field_name)
}

// After interpolation: result self.balance
```

[22] The string `"balance"` is spliced as the identifier `balance` (not as a string literal).

[23] **Identifier context detection.** The compiler detects whether an interpolation appears in identifier position (field access, variable reference) or value position (literal) and interprets accordingly:

- **Identifier position**: `self.$(name)`, `$(function_name)(args)` — Splice as identifier
- **Value position**: `x + $(value)`, `$(array)[index]` — Splice as literal value

##### §15.7.4.5 Array and Tuple Interpolation

[24] Arrays and tuples of embeddable values may be interpolated:

```cursive
let values: const [i32; 3] = [1, 2, 3]

let body = quote {
    let data = $(values)
}

// After interpolation: let data = [1, 2, 3]
```

#### §15.7.5 Examples [comptime.quote.examples]

##### §15.7.5.1 Simple Value Interpolation

**Example 16.7.5.1** (Numeric constant):

```cursive
comptime {
    let buffer_size: const usize = 4096

    let init_body = quote {
        let buffer: [u8; $(buffer_size)] = [0; $(buffer_size)]
        result buffer
    }

    // Generates: let buffer: [u8; 4096] = [0; 4096]
}
```

##### §15.7.5.2 Identifier Interpolation

**Example 16.7.5.2** (Field access generation):

```cursive
comptime procedure generate_field_accessor(field_name: string@View, field_type: string@View): QuotedBlock
    [[ |- field_name.len() > 0 => true ]]
{
    result quote {
        result self.$(field_name)
    }
}

comptime {
    let accessor = generate_field_accessor("position", "Vector3")

    // Generated: result self.position
}
```

##### §15.7.5.3 Combined Interpolation

**Example 16.7.5.3** (Multiple interpolations):

```cursive
comptime procedure generate_setter(field_name: string@View, field_type: string@View): QuotedBlock
    [[ comptime::alloc |- field_name.len() > 0 => true ]]
{
    let temp_name <- gensym("old_value")

    result quote {
        let $(temp_name) = self.$(field_name)
        self.$(field_name) = new_value

        // Optional: log change
        if DEBUG_MODE {
            log_field_change($(field_name), $(temp_name), new_value)
        }
    }
}
```

##### §15.7.5.4 Quote in Procedure Spec

**Example 16.7.5.4** (Quote as procedure body):

```cursive
comptime {
    let max_value: const i32 = 100

    codegen::declare_procedure(codegen::ProcedureSpec {
        name: "clamp_to_max",
        visibility: codegen::Visibility::Public,
        receiver: codegen::ReceiverSpec::None,
        params: [
            codegen::ParamSpec {
                name: "value",
                ty: TypeRef::Named("i32"),
                permission: Permission::Const,
                responsible: false,
            },
        ],
        return_type: TypeRef::Named("i32"),
        sequent: codegen::sequent_pure(),
        body: quote {
            if value > $(max_value) {
                result $(max_value)
            } else {
                result value
            }
        },
    })
}

// Generated:
// procedure clamp_to_max(value: const i32): i32 {
//     if value > 100 {
//         result 100
//     } else {
//         result value
//     }
// }
```

#### §15.7.6 Interpolation Type Safety [comptime.quote.typesafety]

##### §15.7.6.1 Type Preservation

[25] Interpolation preserves type information where applicable. Interpolating typed values maintains their types in generated code:

```cursive
let count: const u32 = 50

let body = quote {
    let items: [Item; $(count)] = allocate()
}

// count is u32, interpolates as 50u32 (preserving type)
```

##### §15.7.6.2 Type Coercion

[26] When necessary, interpolated values may undergo compile-time type coercion to match context:

```cursive
let size: const u32 = 1024

let body = quote {
    let buffer: [u8; $(size as usize)] = [0; $(size as usize)]
}
```

[27] Explicit `as` casts within interpolations are evaluated at compile time.

#### §15.7.7 Diagnostics [comptime.quote.diagnostics]

[28] [Note: Diagnostics defined in this subsection are cataloged in Annex E §E.5.1.15. — end note]

#### §15.7.8 Conformance Requirements [comptime.quote.requirements]

[29] A conforming implementation SHALL:

1. Support `quote { ... }` and `quote expr` syntax
2. Implement `$(...)` interpolation syntax (dollar sign)
3. Evaluate interpolations at compile time during code generation
4. Splice interpolated values as literals or identifiers based on context
5. Restrict quote expressions to comptime contexts (E16-200)
6. Enforce compile-time evaluability of interpolated expressions (E16-201)
7. Type-check generated code after interpolation resolution
8. Support identifier, value, array, and tuple interpolation
9. Detect and report type errors in generated code (E16-203)
10. Preserve type information during value interpolation

[30] A conforming implementation MAY:

1. Optimize quote representation for memory efficiency
2. Cache quoted code structures

[31] A conforming implementation SHALL NOT:

1. Use syntax other than `$(...)` for interpolation
2. Allow quote expressions in runtime contexts
3. Evaluate interpolations non-deterministically
4. Generate syntactically invalid code from well-formed quotes

---

**Previous**: §16.6 Type Reflection and Metadata Queries (§16.6 [comptime.reflect.queries]) | **Next**: §16.8 Code Generation API and Specifications (§16.8 [comptime.codegen.api])

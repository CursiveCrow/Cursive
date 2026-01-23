# Cursive Language Specification

## Clause 12 — Witness System

**Clause**: 12 — Witness System
**File**: 13-7_Grammar-References.md
**Section**: §12.7 Grammar Hooks and References
**Stable label**: [witness.grammar]  
**Forward references**: Annex A §A.2 [grammar.type], Annex A §A.4 [grammar.expression]

---

### §12.7 Grammar Hooks and References [witness.grammar]

#### §12.7.1 Overview

[1] This subclause consolidates the grammar for witness types, witness expressions, and witness-related constructs. The authoritative grammar appears in Annex A; this section provides a readable summary with cross-references.

#### §12.7.2 Witness Type Grammar [witness.grammar.type]

[2] Witness type syntax (simplified notation):

**Witness types** match the pattern:
```
"witness" "<" <WitnessProperty> ">" [ <AllocationState> ]
```

where **witness properties** take one of the following forms:
```
<BehaviorIdentifier>
<ContractIdentifier>
<Type> "@" <StateIdentifier>
```

and **allocation states** match the pattern:
```
"@" ( "Stack" | "Region" | "Heap" )
```

[3] The witness property (in angle brackets) specifies what is being witnessed. The allocation state (optional, defaults to @Stack) specifies where the witness is allocated.

[ Note: See Annex A §A.2 [grammar.type] for authoritative witness type grammar.
— end note ]

#### §12.7.3 Witness Construction Grammar [witness.grammar.construction]

[4] Witness construction via type coercion:

**Witness construction expressions** are simply:
```
<Expression>

// Where expression has type T
// And target type is witness<B>@State
// Compiler inserts coercion automatically
```

[5] No explicit construction syntax is needed. Type annotations trigger automatic coercion.

#### §12.7.4 Witness Method Call Grammar [witness.grammar.methodcall]

[6] Witness method invocation:

**Witness method calls** match the pattern:
```
<WitnessExpression> "::" <Identifier> "(" [ <ArgumentList> ] ")"
```

where **witness expressions** are any `<Expression>` with type `witness<B>`.

[7] Method calls use the scope operator `::` consistently with all Cursive procedure calls.

[ Note: See Annex A §A.4 [grammar.expression] for complete expression grammar including method calls.
— end note ]

#### §12.7.5 Witness Transition Grammar [witness.grammar.transitions]

[8] Witness state transitions:

**Witness transition calls** match the pattern:
```
<WitnessExpression> "::" <TransitionMethod> "(" ")"
```

where **transition methods** take one of the following forms:
```
"to_region"
"to_heap"
```

[9] Transitions are built-in methods on witness types, following modal transition syntax (§7.6).

#### §12.7.6 Complete Grammar Example [witness.grammar.example]

**Example 13.7.6.1 (All witness grammar elements)**:

```cursive
// Type annotations
procedure demo(
    stack_w: witness<Display>,               // Defaults to @Stack
    region_w: witness<Processor>@Region,     // Explicit @Region
    modal_w: witness<FileHandle@Open>@Heap   // Modal witness, @Heap
)
    [[ alloc::heap, io::write |- true => true ]]
{
    // Construction via coercion
    let shape = Circle { radius: 5.0 }
    let w1: witness<Display> = shape                  // Stack witness
    let w2: witness<Display>@Heap = move Circle::new()  // Heap witness

    // Method calls
    w1::show()
    w2::render(canvas)

    // Transitions
    let w3 = w1::to_heap()

    // Modal witness methods
    modal_w::read(buffer)

    // Heterogeneous array
    let witnesses: [witness<Display>] = [w1, stack_w]
}
```

#### §12.7.7 Integration with Annex A [witness.grammar.annex]

[10] Witness grammar integrates with Annex A productions:

**Type Grammar** (Annex A §A.2):

- `witness_type` added to `type` production
- Modal state annotations reused for allocation states

**Expression Grammar** (Annex A §A.4):

- Witness construction via type coercion (implicit in assignment)
- Method calls follow postfix expression syntax

**Pattern Grammar** (Annex A §A.3):

- Witnesses can be pattern matched (future extension)
- Currently: witnesses are opaque, no pattern matching on content

#### §12.7.8 Conformance Requirements [witness.grammar.requirements]

[11] Implementations shall:

1. Parse witness types with property in angle brackets
2. Support optional allocation state annotations (@Stack/@Region/@Heap)
3. Default omitted allocation state to @Stack
4. Parse witness method calls using `::` operator
5. Support transition methods (to_region, to_heap)
6. Integrate witness grammar with Annex A type and expression grammars
7. Emit syntax diagnostics for malformed witness types
8. Maintain grammar consistency with modal types and generic syntax

---

**Previous**: §13.6 Regions, Permissions, and Grants Interplay (§13.6 [witness.memory]) | **Next**: §13.8 Diagnostics (§13.8 [witness.diagnostics])

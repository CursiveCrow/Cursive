# Cursive Language Specification

## Clause 5 — Names, Scopes, and Resolution

**Clause**: 5 — Names, Scopes, and Resolution
**File**: 06-2_Scope-Formation.md
**Section**: §5.2 Scope Formation
**Stable label**: [name.scope]  
**Forward references**: §2.5 [lex.units], §4.2 [decl.variable], §4.6 [decl.visibility], §5.3 [name.shadow], §5.4 [name.lookup], §10.3 [memory.region]

---

### §5.2 Scope Formation [name.scope]

#### §5.2.1 Overview [name.scope.overview]

[1] This subclause defines the lexical structures that create scopes, the hierarchy relating module, procedure, and block scopes, and the rules by which declarations enter those scopes. Scope formation underpins the unified namespace model: each scope provides exactly one binding table shared by all name categories.

[2] The rules complement the binding semantics in §6.3 and the lookup algorithm in §6.4. Together they ensure deterministic, two-phase name resolution that aligns with Cursive’s goals of explicitness and AI-friendly predictability.

#### §6.2.2 Syntax [name.scope.syntax]

[3] Scope boundaries coincide with the following syntactic constructs:

**Scopes** take one of the following forms:
```
<module_scope>
<procedure_scope>
<block_scope>
```

**Module scopes** are:
```
<compilation_unit>
```

**Procedure scopes** take one of the following forms:
```
<procedure_body>
<comptime_callable_body>
```

**Block scopes** take one of the following forms:
```
"{" <statement>* "}"
<region_block>
```

**Region blocks** match the pattern:
```
"region" [ <identifier> ] "{" <statement>* "}"
```

[ Note: See Clause 9 for `procedure_body` and `statement` grammar; §2.5.2 for `compilation_unit`; §11.3 [memory.region] for region semantics.
— end note ]

[4] The canonical forms for procedure bodies and statements appear in Clause 9; compilation units are defined in §2.5.2; region blocks are elaborated in §11.3 [memory.region].

#### §6.2.3 Constraints [name.scope.constraints]

[5] Every scope shall have a single parent except the implicit root scope associated with the compilation unit. The parent relation shall form a tree—cycles and multiple parents for a single scope are forbidden (diagnostic E06-201).

[6] Module scope is the outermost scope for each compilation unit. All top-level declarations (`module`, `import`, `use`, `let`, `var`, `record`, `enum`, `modal`, `contract`, `predicate`, `grant`) enter this scope and become visible throughout the unit subject to visibility modifiers (§5.6).

[7] Procedure scope is established when a callable body begins. Parameters and labels have procedure-wide visibility within that scope regardless of nested blocks. Nested procedure declarations create new scopes following the rules in §5.4.

[8] Block scope begins at each `{` and ends at the matching `}`. Declarations inside a block are visible from the point of declaration to the end of the block and may shadow outer bindings only when marked with `shadow` (§6.3). Region blocks (`region`) create block scopes that are constrained additionally by the region stack (§11.3), but their name-visibility behaviour is identical to ordinary blocks.

[9] Control-flow constructs that introduce implicit blocks (`if`, `while`, `for`, `match`) behave as if they contained explicit braces. Implementations shall normalise such constructs to block scopes before name-resolution analysis.

[10] `use` declarations augment the surrounding module scope; they do not create nested lexical scopes. Imported bindings therefore share the module’s binding table and obey the single-namespace rule. Treating a `use` clause as a nested scope is ill-formed (diagnostic E06-202).

#### §6.2.4 Semantics [name.scope.semantics]

[11] Scope formation proceeds during parsing. Two-phase compilation (§2.2) records every scope boundary before semantic analysis so that forward references within a compilation unit are valid. Each scope owns a binding table populated as declarations are encountered.

[12] Shadowing is resolved lexically: when a nested scope declares an identifier marked with `shadow`, it inserts a new binding that temporarily overrides the ancestor binding for the duration of the nested scope. Without `shadow`, redeclaration in the same or nested scope triggers E06-300 (§6.3).

[13] Function scope exposes parameters and labels to all nested blocks. Block-local declarations do not escape their block even when references to them are captured by closures; closures capture the value, not the scope entry, and the captured value obeys the permission rules of Clause 11.

[14] Region blocks attach lifetime semantics to the bindings created within them; when the region closes, any region-allocated storage is reclaimed. Name visibility nonetheless follows ordinary block rules—bindings cease to exist after the closing brace.

#### §6.2.5 Examples (Informative) [name.scope.examples]

**Example 6.2.5.1 (Scope hierarchy with region blocks):**

```cursive
// Module scope: MODULE_CONST, helper, and main share the module binding table
let MODULE_CONST = 42

procedure helper(x: i32): i32
    [[ |- true => true ]]
{
    // Procedure scope begins here; parameter `x` and label `'retry` are visible throughout
    'retry: loop {
        let temp = x + MODULE_CONST  // `temp` is block-scoped within the loop body
        if temp > 100 {
            break 'retry
        }
        shadow let x = temp  // Allowed shadowing within the loop block
    }

    // Block scope ends; the shadowed `x` is gone, the original parameter `x` is still visible
    result x
}

public procedure main(): i32
    [[ |- true => true ]]
{
    let value = helper(10)
    {
        region session {
            let handle = open_resource()
            // `handle` visible only inside region block
        }
        // `handle` is out of scope here (and the resource has been reclaimed)
    }
    result value
}
```

#### §6.2.6 Conformance Requirements [name.scope.requirements]

[15] Implementations shall construct the scope tree described in this subclause during parsing, ensuring parent/child relationships are acyclic and well-defined (diagnostic E06-201 when violated).

[16] Implementations shall treat implicit blocks in control-flow constructs as lexical scopes equivalent to explicit braces before performing name lookup.

[17] Implementations shall enforce the procedure-scope visibility of parameters and labels and the block-scope visibility of local declarations, raising diagnostics when references escape their scope (E06-410 or context-specific errors).

[18] Implementations shall ensure region blocks behave as block scopes for name visibility while honouring the additional lifetime semantics defined in §11.3.

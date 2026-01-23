# Cursive Language Specification

## Clause 12 — Witness System

**Clause**: 12 — Witness System
**File**: 13-8_Diagnostics.md
**Section**: §12.8 Diagnostics
**Stable label**: [witness.diagnostics]  
**Forward references**: §12.1-12.7 (all witness subclauses), Annex E §E.5 [implementation.diagnostics]

---

### §12.8 Diagnostics [witness.diagnostics]

#### §12.8.1 Overview

[1] This subclause consolidates all diagnostics for the witness system, provides conformance requirements, and specifies integration with other language subsystems.

[2] Witness system diagnostics cover formation errors, dispatch failures, permission violations, region escapes, and grant violations. All diagnostics follow the canonical format `E12-[NNN]` as specified in §1.6.6 [intro.document.diagnostics].

#### §12.8.2 Complete Diagnostic Catalog [witness.diagnostics.catalog]

[3] Clause 12 defines the following diagnostic codes:

##### §12.8.2.1 Formation and Construction Diagnostics (E12-001 through E12-012)

| Code    | Description                                                 | Section    |
| ------- | ----------------------------------------------------------- | ---------- |
| E12-001 | Type does not satisfy behavior/satisfy contract/have state  | §12.3.6[1] |
| E12-010 | Missing `move` for responsible witness (@Heap/@Region)      | §12.3.6[2] |
| E12-011 | Region witness escapes region                               | §12.3.6[5] |
| E12-012 | Grant missing for witness construction (alloc::heap/region) | §12.3.6[4] |

##### §12.8.2.2 Dispatch Diagnostics (E12-020 through E12-022)

| Code    | Description                                 | Section     |
| ------- | ------------------------------------------- | ----------- |
| E12-020 | Method not found in behavior/contract       | §12.5.8[7]  |
| E12-021 | Method not available in modal state         | §12.5.5[10] |
| E12-022 | Insufficient grants for witness method call | §12.5.7[14] |

##### §12.8.2.3 Memory Integration Diagnostics (E12-030 through E12-032)

| Code    | Description                                 | Section     |
| ------- | ------------------------------------------- | ----------- |
| E12-030 | Permission upgrade on witness               | §12.6.4[6]  |
| E12-031 | Witness transition requires grant           | §12.6.7[20] |
| E12-032 | Invalid witness transition (no such method) | §12.6.7[19] |

#### §12.8.3 Diagnostic Payloads [witness.diagnostics.payloads]

[4] Witness diagnostics shall follow payload schemas in Annex E §E.5.3, with required fields including source/target types, witness type, method names, modal states, permissions, and grants as applicable. Diagnostic codes E12-001 through E12-032 are defined in this clause.

#### §12.8.4 Consolidated Conformance Requirements [witness.diagnostics.conformance]

[5] A conforming implementation of the witness system shall:

**Type System** (§12.1-§12.2):

1. Provide `witness<Property>@State` as built-in modal type
2. Support behavior, contract, and modal state witnesses
3. Default witness types to @Stack allocation state
4. Unify all witness kinds with same dense pointer representation

**Formation** (§12.3): 5. Support automatic witness coercion from implementing types 6. Verify type implements/satisfies witness property 7. Create non-responsible @Stack witnesses without `move` 8. Require explicit `move` for @Heap/@Region witnesses 9. Enforce grant requirements for witness construction 10. Apply region escape analysis to region witnesses

**Representation** (§12.4): 11. Implement witnesses as dense pointers (16 bytes on 64-bit) 12. Structure witness tables with type ID, size, align, state tag, drop, vtable 13. Share witness tables across instances 14. Support type erasure hiding concrete types

**Dispatch** (§12.5): 15. Support method calls using `::` operator 16. Implement vtable-based dynamic dispatch 17. Type-check method existence and availability 18. Propagate grants from method sequents 19. Optimize to static dispatch when concrete type known (optional)

**Memory Integration** (§12.6): 20. Map allocation states to cleanup responsibility 21. Integrate with RAII for destructor calls 22. Enforce region escape prevention 23. Inherit permissions from source values 24. Support witness transitions between allocation states 25. Enforce grant requirements for transitions

**Grammar** (§12.7): 26. Parse witness types with angle bracket syntax 27. Support allocation state annotations 28. Integrate with Annex A type and expression grammars

**Diagnostics**: 29. Emit all diagnostics E12-001 through E12-032 30. Provide structured diagnostic payloads per Annex E §E.5 31. Maintain witness metadata for reflection and tooling

#### §12.8.5 Integration Summary [witness.diagnostics.integration]

[6] The witness system integrates with:

**Behaviors (Clause 8)**:

- Behavior declarations compile to witness tables
- Behavior implementations provide vtable entries
- Generic bounds (`<T: B>`) vs witness types (`witness<B>`) trade static/dynamic

**Contracts (Clause 10)**:

- Contract declarations compile to witness tables
- Contract implementations provide vtable entries
- Dynamic verification hooks generate witness evidence

**Modal Types (§6.6)**:

- Modal states trackable dynamically via witnesses
- State-specific procedures in modal witness vtables
- Witness state obligations in transition sequents

**Memory Model (Clause 9)**:

- Witness allocation states map to cleanup responsibility
- Region witnesses subject to escape analysis
- Permissions inherited from values
- Move semantics apply to witness construction

**Type System (Clause 5)**:

- Witnesses are modal types with allocation states
- Dense pointer representation defined
- Method dispatch typed through behavior/contract signatures

#### §12.8.6 Complete Example [witness.diagnostics.example]

**Example 13.8.6.1 (Comprehensive witness usage)**:

```cursive
behavior Drawable {
    procedure draw(~)
        [[ io::write |- true => true ]]
    { }

    procedure bounds(~): Rectangle
    { result Rectangle { x: 0.0, y: 0.0, width: 0.0, height: 0.0 } }
}

record Point with Drawable {
    x: f64,
    y: f64,

    procedure draw(~)
        [[ io::write |- true => true ]]
    {
        println("Point({}, {})", self.x, self.y)
    }

    procedure bounds(~): Rectangle
    {
        result Rectangle { x: self.x, y: self.y, width: 0.0, height: 0.0 }
    }
}

record Circle with Drawable {
    center: Point,
    radius: f64,

    procedure draw(~)
        [[ io::write |- true => true ]]
    {
        println("Circle(center: {}, radius: {})", self.center, self.radius)
    }

    procedure bounds(~): Rectangle
    {
        result Rectangle {
            x: self.center.x - self.radius,
            y: self.center.y - self.radius,
            width: self.radius * 2.0,
            height: self.radius * 2.0
        }
    }
}

// Witness construction and usage
procedure render_shapes(shapes: [witness<Drawable>])
    [[ io::write |- shapes.len() > 0 => true ]]
{
    loop shape: witness<Drawable> in shapes {
        shape::draw()                          // Dynamic dispatch
        let bounds = shape::bounds()          // Dynamic dispatch
        println("Bounds: {}x{}", bounds.width, bounds.height)
    }
}

// Usage
procedure main(): i32
    [[ alloc::heap, io::write |- true => true ]]
{
    let point = Point { x: 1.0, y: 2.0 }
    let circle = Circle {
        center: Point { x: 5.0, y: 5.0 },
        radius: 3.0
    }

    // Create heap witnesses explicitly (heterogeneous collection)
    let point_witness: witness<Drawable>@Heap = move point
    let circle_witness: witness<Drawable>@Heap = move circle

    // Array elements now have identical type
    let shapes: [witness<Drawable>@Heap] = [point_witness, circle_witness]

    render_shapes(shapes)

    result 0
}
```

#### §12.8.7 Conformance Requirements [witness.grammar.requirements]

[7] Implementations shall:

1. Parse witness types per §12.7.2 grammar
2. Support witness method calls per §12.7.4 grammar
3. Integrate witness grammar with Annex A
4. Emit syntax diagnostics for malformed witness types or calls
5. Maintain AST representation of witnesses for type checking

---

**Previous**: §12.6 Regions, Permissions, and Grants Interplay (§12.6 [witness.memory]) | **Next**: Clause 13 — Concurrency and Memory Ordering (§13 [concurrency])

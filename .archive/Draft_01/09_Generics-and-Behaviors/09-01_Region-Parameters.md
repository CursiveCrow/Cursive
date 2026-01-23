# Cursive Language Specification

## Clause 9 — Generics and Behaviors

**Clause**: 9 — Generics and Behaviors
**File**: 10-01_Region-Parameters.md
**Section**: §9.1 Region Parameters Overview
**Stable label**: [generic.regions]  
**Forward references**: §8.2 [generic.parameter], §8.3 [generic.bounds], §8.6 [generic.resolution], §9.3 [memory.region], §6.5 [type.pointer]

---

### §9.1 Region Parameters Overview [generic.regions]

#### §9.1.1 Overview

[1] This subclause specifies how region allocation integrates with the generic type system. Region type parameters are not supported in version 1.0; allocation uses explicit region blocks with the `^` operator (§10.3) and provenance tracking rather than type-level parameterization.

#### §9.1.2 Current Design: Provenance Without Parameterization

[4] Under the current design, region allocation uses the `^` prefix operator within explicit `region` blocks:

```cursive
region session {
    let data = ^Buffer::new()
    process(data)
}
// All allocations in 'session' freed at O(1) when block exits
```

[5] The compiler tracks provenance metadata (Stack, Region(r), Heap) for each allocated object as described in §9.3.4 and §7.5.3. Escape analysis prevents region-allocated values from outliving their region without requiring type-level region parameters.

[6] Generic types currently do not expose region parameters. A type such as `List<T>` may contain region-allocated elements, but the region is determined by allocation sites rather than by parameterization. The provenance of contained elements is tracked via the existing escape analysis mechanism.

#### §9.1.3 Syntax (Current Edition)

[7] Region allocation syntax (defined in §9.3.2):

**Region blocks** match the pattern:
```
"region" [ <identifier> ] <block_stmt>
```

**Region allocation expressions** take one of the following forms:
```
"^" <expression>
"^" "^" ... <expression>    // Caret stacking for parent regions
```

[ Note: Complete region allocation semantics are specified in §9.3.2 [memory.region]. — end note ]

[8] Generic parameter lists (§8.2.2) currently support type, const, and grant parameters but not region parameters:

**Generic parameters** match the pattern:
```
"<" <generic_param_list> ">"
```

where **generic parameter lists** match:
```
<generic_param> [ "," <generic_param> ... ]
```

and each **generic parameter** takes one of the following forms:
```
<type_param>
<const_param>
<grant_param>
// region_param reserved for future edition
```

[ Note: See §8.2.2 [generic.params] and Annex A §A.6 for the complete generic parameter grammar. — end note ]

#### §9.1.4 Constraints (Current Edition)

[9] Region allocation requires an active region block. Using `^` outside a region context produces diagnostic E10-103 (defined in §9.3).

[10] Escape analysis (§9.3.3, §7.5.3) prevents region-allocated values from escaping their originating region. This constraint is enforced through provenance tracking without requiring type-level region annotations.

[11] Generic types containing region-allocated elements shall not expose those elements in ways that would allow escape. The prohibition is enforced through the same provenance and escape rules that apply to non-generic types.

#### §9.1.5 Semantics (Current Edition)

[12] When a generic type `Container<T>` is instantiated with region-allocated elements:

1. The element type `T` carries no region annotation
2. Elements allocated with `^` have provenance Region(r) where r is the active region
3. Escape analysis prevents storing those elements in containers with longer lifetimes
4. The container itself may be allocated in any region or on the heap

[13] The lack of region parameterization means that containers cannot abstract over region lifetimes. Each container instance is associated with a specific allocation site, and region provenance is determined by that site rather than by type parameters.

#### §9.1.6 Examples (Current Edition)

**Example 9.1.6.1 (Region allocation without parameterization):**

```cursive
record List<T> {
    data: Ptr<T>@Valid,
    len: usize,
    cap: usize,
}

procedure demo()
    [[ alloc::region, alloc::heap |- true => true ]]
{
    region temp {
        let items = ^List::new()        // List allocated in temp
        items.push(^Item::new())         // Item allocated in temp
        // Both List and Items freed when temp exits
    }

    let permanent = List::new()          // List allocated on heap
    permanent.push(Item::new())          // Item allocated on heap
    // Separate lifetime from temp region
}
```

**Example 9.1.6.2 (Escape prevention through provenance):**

```cursive
procedure invalid_escape(): List<Data>
    [[ alloc::region, alloc::heap |- true => true ]]
{
    region temp {
        let list = ^List::new()
        list.push(^Data::new())
        result list  // error[E10-101]: region-allocated value cannot escape
    }
}

procedure valid_heap_return(): List<Data>
    [[ alloc::region, alloc::heap |- true => true ]]
{
    region temp {
        let list = ^List::new()
        list.push(^Data::new())
        result list.to_heap()  // Explicit heap conversion before escape
    }
}
```

#### §9.1.7 Design Decision: No Region Type Parameters

[14] This specification deliberately does **not** introduce region type parameters (syntax like `List<^r, T>`). Region allocation is managed through the `Arena` modal type (§9.3.2) and provenance metadata, not type-level parameterization.

[ Rationale: Region type parameters would reintroduce complexity similar to lifetime parameters, contradicting Cursive's explicit allocation philosophy. Regions are concrete arenas (§10.3), not abstract bounds. Named arena bindings provide necessary control without type-level pollution. — end rationale ]

[15] Region type parameters are not supported in version 1.0. Future editions may reconsider based on field experience if the Arena modal type proves insufficient for all use cases. If additional arena control is needed beyond named bindings and modal states, extensions will prioritize first-class arena values (library types, collection patterns) rather than type-level parameterization.

#### §9.1.8 Integration with Other Systems

[17] Region allocation integrates with:

- **Type system (Clause 7)**: Pointer provenance (§7.5.3) tracks allocation source
- **Memory model (Clause 9)**: Region blocks (§9.3), escape analysis (§9.3.3)
- **Permissions (§9.4)**: Orthogonal—region allocation works with all permissions
- **Generics (§10.6)**: Monomorphization preserves region semantics

[18] These integrations ensure that region allocation remains safe and deterministic even when combined with generic types, permissions, and contracts.

#### §9.1.9 Conformance Requirements

[19] Implementations shall integrate region allocation with monomorphization (§8.6) so that generic instantiations preserve region semantics, and shall not introduce region parameterization syntax without specification approval.

---

**Previous**: Clause 8 — Statements and Control Flow (§8.4 [stmt.order]) | **Next**: §8.2 Type Parameters (§8.2 [generic.parameter])

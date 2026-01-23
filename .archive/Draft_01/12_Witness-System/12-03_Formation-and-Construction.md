# Cursive Language Specification

## Clause 12 — Witness System

**Clause**: 12 — Witness System
**File**: 13-3_Formation-and-Construction.md
**Section**: §12.3 Formation and Construction
**Stable label**: [witness.formation]  
**Forward references**: §13.4 [witness.representation], §13.5 [witness.dispatch], §13.6 [witness.memory], Clause 10 [generic], Clause 11 §11.2 [memory.object], Clause 11 §11.5 [memory.move]

---

### §12.3 Formation and Construction [witness.formation]

#### §12.3.1 Overview

[1] Witness construction creates dense pointer values from concrete types that satisfy behaviors, satisfy contracts, or inhabit modal states. Construction is type-driven: witness types in annotations trigger automatic coercion from concrete values to witness values.

[2] This subclause specifies witness formation rules, construction semantics for each allocation state, cleanup responsibility, grant requirements, and the explicit `move` requirement for responsible witnesses.

#### §12.3.2 Witness Type Formation [witness.formation.types]

##### §12.3.2.1 Syntax

[3] Witness types follow the modal type syntax (§7.6) with the property in angle brackets:

**Witness types** match the pattern:
```
"witness" "<" <witness_property> ">" [ <witness_allocation_state> ]
```

where **witness properties** take one of the following forms:
```
<behavior_identifier>
<contract_identifier>
<type_identifier> "@" <state_identifier>
```

and **witness allocation states** match the pattern:
```
"@" ( "Stack" | "Region" | "Heap" )
```

[4] Allocation state is optional; omitted state defaults to `@Stack`.

**Example 13.3.2.1 (Witness type forms)**:

```cursive
witness<Display>                     // Defaults to witness<Display>@Stack
witness<Display>@Stack               // Explicit stack allocation
witness<Display>@Region              // Region allocation
witness<Display>@Heap                // Heap allocation

witness<Serializable>                // Contract witness, defaults to @Stack
witness<FileHandle@Open>             // Modal state witness, defaults to @Stack
witness<FileHandle@Open>@Heap        // Modal state witness, heap allocated
```

##### §12.3.2.2 Well-Formedness

[5] A witness type is well-formed when:

$$
\frac{\Gamma \vdash B : \text{Behavior} \quad S \in \{\text{@Stack}, \text{@Region}, \text{@Heap}\}}
     {\Gamma \vdash \texttt{witness}\langle B \rangle@S : \text{Type}}
\tag{WF-Witness-Behavior}
$$

$$
\frac{\Gamma \vdash C : \text{Contract} \quad S \in \{\text{@Stack}, \text{@Region}, \text{@Heap}\}}
     {\Gamma \vdash \texttt{witness}\langle C \rangle@S : \text{Type}}
\tag{WF-Witness-Contract}
$$

$$
\frac{\Gamma \vdash T@State : \text{Type} \quad S \in \{\text{@Stack}, \text{@Region}, \text{@Heap}\}}
     {\Gamma \vdash \texttt{witness}\langle T@State \rangle@S : \text{Type}}
\tag{WF-Witness-Modal}
$$

[6] The property (`B`, `C`, or `T@State`) must be visible and well-formed in the current context.

#### §12.3.3 Witness Construction [witness.formation.construction]

##### §12.3.3.1 Automatic Coercion

[7] Witness construction occurs via automatic coercion when a value is assigned to a witness-typed binding:

$$
\frac{\Gamma \vdash v : T \quad \Gamma \vdash T : B \quad \text{target type: } \texttt{witness}\langle B \rangle@S}
     {\text{Coerce } v \text{ to } \texttt{witness}\langle B \rangle@S}
\tag{Coerce-Witness}
$$

[8] The coercion creates a dense pointer containing the value and its behavior/contract vtable or modal state tag.

**Example 13.3.3.1 (Automatic witness construction)**:

```cursive
let point = Point { x: 1.0, y: 2.0 }

// Type annotation triggers coercion
let w: witness<Display> = point
// Compiler inserts witness construction automatically
```

##### §12.3.3.2 Stack Witness Construction (Non-Responsible)

[9] Stack witnesses (`@Stack`) are non-responsible bindings. They reference the original value without transferring cleanup responsibility. No `move` keyword is required.

**Construction rule**:

[ Given: Value $v$ of type $T$ where $T : B$ ]

$$
\frac{\Gamma \vdash v : T \quad T : B}
     {\texttt{let } w : \texttt{witness}\langle B \rangle@\texttt{Stack} = v \text{ creates non-responsible witness}}
\tag{E-Witness-Stack}
$$

[10] The original binding remains valid and retains cleanup responsibility. The stack witness becomes invalid when the original binding goes out of scope.

**Example 13.3.3.2 (Stack witness - non-responsible)**:

```cursive
procedure demo()
    [[ io::write |- true => true ]]
{
    let shape = Point { x: 1.0, y: 2.0 }

    let w: witness<Display>@Stack = shape
    // No move needed
    // shape is still valid

    shape::show()  // ✅ OK: shape not moved
    w::show()      // ✅ OK: witness references shape

    // At scope exit: shape's destructor called
    //                w becomes invalid (non-responsible)
}
```

##### §12.3.3.3 Heap Witness Construction (Responsible)

[11] Heap witnesses (`@Heap`) are responsible bindings. They take cleanup responsibility from the source value via explicit `move`. The witness allocates heap storage and becomes responsible for calling the destructor.

**Construction rule**:

[ Given: Value $v$ of type $T$ where $T : B$ ]

$$
\frac{\Gamma \vdash v : T \quad T : B \quad v \text{ transferable}}
     {\texttt{let } w : \texttt{witness}\langle B \rangle@\texttt{Heap} = \texttt{move } v}
\tag{E-Witness-Heap}
$$

[12] Omitting `move` for heap witness construction produces diagnostic E13-010 (missing move for responsible witness). After construction, the source binding is invalid (moved).

[13] **Grant requirement**: Heap witness construction requires `alloc::heap` grant in the procedure's sequent.

**Example 13.3.3.3 (Heap witness - responsible)**:

```cursive
procedure create_heap_witness(): witness<Display>@Heap
    [[ alloc::heap |- true => true ]]
{
    let shape = Circle { radius: 5.0 }

    let w: witness<Display>@Heap = move shape  // Explicit move required
    // shape is now invalid (moved)
    // shape::show()  // ❌ ERROR: use of moved value

    result w  // Witness escapes scope (heap allocation)
}
// Caller receives responsible witness
// Caller's scope exit calls destructor
```

##### §12.3.3.4 Region Witness Construction (Responsible)

[14] Region witnesses (`@Region`) are responsible bindings allocated in the active region. They require explicit `move` and the `alloc::region` grant. The witness is destroyed when the region exits.

**Construction rule**:

[ Given: Value $v$ in active region $r$ ]

$$
\frac{\Gamma \vdash v : T \quad T : B \quad v \text{ transferable} \quad r \text{ active}}
     {\texttt{let } w : \texttt{witness}\langle B \rangle@\texttt{Region} = \texttt{move } v}
\tag{E-Witness-Region}
$$

[15] Region witnesses cannot escape their region. Attempting to return or store a region witness outside its region produces diagnostic E13-011 (witness escape violation), consistent with region escape analysis (§11.3.3).

**Example 13.3.3.4 (Region witness - responsible)**:

```cursive
procedure process_in_region()
    [[ alloc::region, io::write |- true => true ]]
{
    region temp {
        let shape = Rectangle { width: 10.0, height: 5.0 }

        let w: witness<Display>@Region = move shape  // Explicit move
        // shape is invalid

        w::show()  // OK within region

        // return w  // ❌ ERROR E13-011: witness escapes region
    }
    // Region exit: w destroyed
}
```

#### §12.3.4 Witness Construction from Different Source Types [witness.formation.sources]

##### §12.3.4.1 From Owned Values

[16] Responsible witnesses can be created from owned values (bindings created with `=`):

```cursive
let value = Type::new()
let w: witness<B>@Heap = move value  // Takes responsibility
```

##### §12.3.4.2 From Non-Responsible Bindings

[17] Stack witnesses can reference non-responsible bindings:

```cursive
let owner = Type::new()
let ref <- owner  // Non-responsible binding

let w: witness<B>@Stack = ref  // Stack witness references ref
// Both ref and w are non-responsible
// owner retains responsibility
```

[18] Heap/region witnesses cannot be created from non-responsible bindings because non-responsible bindings cannot transfer responsibility (§11.5.2.3). Attempting to move from non-responsible bindings produces diagnostic E11-502.

**Example 13.3.4.1 (Witness from non-responsible binding)**:

```cursive
let data = Buffer::new()
let view <- data  // Non-responsible

let w1: witness<Display>@Stack = view  // ✅ OK: stack witness
// let w2: witness<Display>@Heap = move view  // ❌ E11-502: cannot move non-responsible
```

#### §12.3.5 Permission Inheritance [witness.formation.permissions]

[19] Witnesses inherit the permission of the underlying value:

$$
\frac{v : \texttt{perm } T \quad \texttt{witness}\langle B \rangle@S \text{ from } v}
     {\texttt{witness}\langle B \rangle@S \text{ has permission perm}}
\tag{Witness-Permission-Inherit}
$$

[20] Permission qualifiers (const, unique, shared) attach to the witness type, not the witness itself:

**Example 13.3.5.1 (Permission inheritance)**:

```cursive
let point: unique Point = Point::new()
let w: unique witness<Display> = point
// Witness has unique permission (inherited from point)

w::mutate()  // ✅ OK if Display has mutating methods

let const_point: const Point = get_point()
let cw: const witness<Display> = const_point
// Witness has const permission
// cw::mutate()  // ❌ ERROR: const permission
```

#### §12.3.6 Constraints [witness.formation.constraints]

[1] _Satisfaction requirement._ The source value's type must satisfy the behavior, satisfy the contract, or be in the modal state specified by the witness property. Violations produce diagnostic E13-001.

[2] _Move requirement for responsible witnesses._ Creating `@Heap` or `@Region` witnesses requires explicit `move` keyword. Omitting `move` produces diagnostic E13-010.

[3] _Non-responsible limitation._ Stack witnesses may reference non-responsible bindings. Heap/region witnesses require transferable bindings. Attempting to create responsible witnesses from non-responsible bindings produces diagnostic E11-502.

[4] _Grant requirements._ Witness construction requires grants based on allocation state:

- `@Stack`: No grants (references only)
- `@Region`: `alloc::region` grant required
- `@Heap`: `alloc::heap` grant required

[5] _Escape restrictions._ Region witnesses are subject to region escape analysis (§11.3.3). They cannot escape their region. Violations produce diagnostic E13-011.

[6] _Permission compatibility._ The source value's permission must be compatible with the witness type's permission annotation. Permission upgrades are forbidden (§11.4.3).

#### §12.3.7 Diagnostics [witness.formation.diagnostics]

[7] Witness formation diagnostics:

| Code    | Condition                                                   | Constraint |
| ------- | ----------------------------------------------------------- | ---------- |
| E13-001 | Type does not satisfy behavior/satisfy contract/have state  | [1]        |
| E13-010 | Missing `move` for responsible witness (@Heap/@Region)      | [2]        |
| E13-011 | Region witness escapes region                               | [5]        |
| E13-012 | Grant missing for witness construction (alloc::heap/region) | [4]        |

#### §12.3.8 Conformance Requirements [witness.formation.requirements]

[8] Implementations shall:

1. Support automatic witness coercion from implementing types
2. Verify source type implements/satisfies witness property
3. Create non-responsible stack witnesses without `move`
4. Require explicit `move` for heap and region witnesses
5. Enforce grant requirements: @Region needs alloc::region, @Heap needs alloc::heap
6. Apply region escape analysis to region witnesses
7. Inherit permissions from source values to witnesses
8. Track cleanup responsibility per allocation state
9. Invalidate source bindings after move to responsible witnesses
10. Emit diagnostics E13-001, E13-010, E13-011, E13-012 for violations

---

**Previous**: §13.2 Witness Kinds (§13.2 [witness.kind]) | **Next**: §13.4 Representation and Erasure (§13.4 [witness.representation])

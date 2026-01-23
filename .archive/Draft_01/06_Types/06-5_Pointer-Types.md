# Cursive Language Specification

## Clause 6 — Types

**Part**: VI — Type System  
**File**: 07-5_Pointer-Types.md
**Section**: §6.5 Pointer and Reference Types  
**Stable label**: [type.pointer]  
**Forward references**: §6.6 [type.modal], §6.7 [type.relation], Clause 7 [expr], Clause 8 [stmt], Clause 9 [generic], Clause 10 [memory], Clause 11 [contract]

---

### §6.5 Pointer and Reference Types [type.pointer]

[1] Cursive provides two pointer families:

- **Safe modal pointers** `Ptr<T>@State`, which are first-class values with compile-time state tracking, provenance, and aliasing guarantees.
- **Raw pointers** `*const T` and `*mut T`, which bypass safety guarantees for FFI and low-level code.

Reference bindings (`let name <- value`) and procedure receivers desugar to safe pointers described in this clause. General modal types are specified in §7.6; `Ptr<T>` is a built-in modal definition.

#### §7.5.1 Syntax [type.pointer.syntax]

[2] Pointer types follow Annex A §A.3:

```
PointerType    ::= SafePtrType | RawPtrType
SafePtrType    ::= 'Ptr' '<' Type '>' ('@' Ident)?
RawPtrType     ::= '*const' Type | '*mut' Type
```

[3] `Ptr<T>` without a state annotation refers to the unconstrained pointer type. Annotated forms `Ptr<T>@State` restrict the pointer to a particular modal state (paragraph [9]). Safe pointer types support optional provenance attributes (informative tooling metadata) but provenance is primarily tracked via the typing rules in §7.5.3.

#### §7.5.2 Safe Modal Pointers [type.pointer.safe]

##### §7.5.2.1 Modal States [type.pointer.safe.states]

[4] The modal declaration for pointers is defined by the language core:

```cursive
modal Ptr<T> { … }
```

[5] The modal declaration follows the general rules in §7.6.1. The built-in pointer states are:

| State      | Meaning                             |
| ---------- | ----------------------------------- |
| `@Null`    | explicit null pointer               |
| `@Valid`   | dereferenceable pointer             |
| `@Weak`    | weak reference (non-owning)         |
| `@Expired` | weak reference whose target is gone |

[6] State ordering: `Ptr<T>@Null`, `Ptr<T>@Valid`, `Ptr<T>@Weak`, `Ptr<T>@Expired` ⊑ `Ptr<T>`. Widening to `Ptr<T>` is implicit; narrowing requires state refinement (pattern matching or transition results). For modal syntax, witness semantics, and transition guarantees, see §7.6.

##### §7.5.2.2 Formation [type.pointer.safe.formation]

[6] Safe pointers are well-formed when their pointee type is well-formed and the state is one of the built-in identifiers:

$$
\dfrac{\Gamma \vdash \tau : \text{Type} \quad S \in \{\text{@Null}, \text{@Valid}, \text{@Weak}, \text{@Expired}\}}{\Gamma \vdash \text{Ptr}\langle\tau\rangle@S : \text{Type}}
\tag{WF-Ptr-State}
$$

$$
\dfrac{\Gamma \vdash \tau : \text{Type}}{\Gamma \vdash \text{Ptr}\langle\tau\rangle : \text{Type}}
\tag{WF-Ptr}
$$

##### §7.5.2.3 Pointer Creation [type.pointer.safe.creation]

[7] Address-of operator:

$$
\dfrac{\Gamma \vdash e : \tau \quad \text{storage}(e)=\text{location}}{\Gamma \vdash \&e : \text{Ptr}\langle\tau\rangle@\text{Valid}}
\tag{T-Addr}
$$

The operand must denote a storage location (lvalue). Taking the address of aliases, temporaries, or computed expressions is ill-formed (diagnostic E07-302). Pointer declarations must include explicit types; pointer types are not inferred (E07-303).

[8] `Ptr::null<T>()` yields `Ptr<T>@Null`. Other constructors (`to_heap`, region allocation) are defined in Clause 12 but always return pointers with known state and provenance.

##### §7.5.2.4 Dereference and Access [type.pointer.safe.deref]

[9] Dereference is permitted only in `@Valid` state:

$$
\dfrac{\Gamma \vdash p : \text{Ptr}\langle\tau\rangle@\text{Valid}}{\Gamma \vdash *p : \tau}
\tag{T-Deref}
$$

The dynamic semantics evaluate the pointer to an address and read from the pointed location:

```
[E-Deref]
⟨p, σ⟩ ⇓ ⟨Ptr⟨τ⟩@Valid { addr: ℓ }, σ'⟩
σ'(ℓ) = v
--------------------------------
⟨*p, σ⟩ ⇓ ⟨v, σ'⟩
```

[10] Compilation rejects dereferences in other states:

- `Ptr<T>@Null`: E07-301 — “cannot dereference null pointer”.
- `Ptr<T>@Weak`: E07-304 — must upgrade to `@Valid` first.
- `Ptr<T>@Expired`: E07-305 — pointer refers to destroyed object.

##### §7.5.2.5 Weak References and Upgrades [type.pointer.safe.weak]

[11] State transitions are provided via pointer methods:

```
Procedure   Signature                               Effect
-----------------------------------------------------------------------
Ptr::downgrade  (self: const Ptr<T>@Valid) -> Ptr<T>@Weak
Ptr::upgrade    (self: const Ptr<T>@Weak)  -> Ptr<T>        // returns Ptr<T>@Valid or Ptr<T>@Null at runtime
Ptr::is_null    (self: const Ptr<T>@Valid) -> bool          // always false
Ptr::is_expired (self: const Ptr<T>@Weak)  -> bool
```

Typing rules:

$$
\dfrac{\Gamma \vdash p : \text{Ptr}\langle\tau\rangle@\text{Valid}}{\Gamma \vdash p.\text{downgrade}() : \text{Ptr}\langle\tau\rangle@\text{Weak}}
\tag{T-Downgrade}
$$

$$
\dfrac{\Gamma \vdash p : \text{Ptr}\langle\tau\rangle@\text{Weak}}{\Gamma \vdash p.\text{upgrade}() : \text{Ptr}\langle\tau\rangle}
\tag{T-Upgrade}
$$

The result of `upgrade` must be pattern matched to recover `@Valid` or `@Null` states.

##### §7.5.2.6 Pattern Matching and Refinement [type.pointer.safe.pattern]

[12] Pattern matching on pointer states refines the pointer type within each branch. Example:

```cursive
match ptr {
    @Null => println("optional value missing"),
    @Valid => println("value: {}", *ptr),
    @Weak => match ptr.upgrade() {
        @Valid => println("weak upgraded: {}", *ptr),
        @Null => println("target destroyed"),
    },
    @Expired => println("stale weak reference"),
}
```

Typing rule:

$$
\dfrac{\Gamma \vdash e : \text{Ptr}\langle\tau\rangle}{\Gamma \vdash \text{match } e \text{ with states}} \quad \text{patterns cover all states}
\tag{T-Match-Ptr}
$$

##### §7.5.2.7 State Transitions [type.pointer.safe.transitions]

[13] Safe pointers follow the deterministic transition table in Table 7.5.1. No other state transitions are permitted.

**Table 7.5.1 — `Ptr<T>` transition catalogue**

| Operation                     | Source state              | Target state | Conditions / notes                                                                            |
| ----------------------------- | ------------------------- | ------------ | --------------------------------------------------------------------------------------------- |
| `&e` (address-of)             | storage-backed expression | `@Valid`     | `e` denotes a live storage location; provenance recorded per §7.5.3                           |
| `Ptr::null<T>()`              | —                         | `@Null`      | Produces an explicit null pointer                                                             |
| `Ptr::downgrade()`            | `@Valid`                  | `@Weak`      | Pointer retains provenance and no longer proves liveness                                      |
| `Ptr::upgrade()` success path | `@Weak`                   | `@Valid`     | Only when the referent is still live; caller SHALL pattern match the result                   |
| `Ptr::upgrade()` failure path | `@Weak`                   | `@Null`      | Indicates the referent has been destroyed; dereference must not follow                        |
| Owning scope/region drop      | `@Weak`                   | `@Expired`   | Occurs automatically when the final owning `@Valid` pointer drops; dereference raises E07-305 |

[14] Implementations shall diagnose any attempt to forge a transition not listed above. In particular, there is no transition that converts `@Expired` back to `@Valid`; a stale pointer can only become usable again by obtaining a fresh pointer via the safe constructors above.

#### §7.5.3 Provenance and Escape Analysis [type.pointer.safe.provenance]

##### §7.5.3.1 Provenance Model

[13] Provenance metadata tracks allocation source:

| Provenance  | Description                                     |
| ----------- | ----------------------------------------------- |
| `Stack`     | Stack frame local                               |
| `Region(r)` | Region `r` allocated via `region` block         |
| `Heap`      | Explicit heap allocation (`to_heap`, allocator) |

Rules:

$$
\text{prov}(\&e) = \text{storage\_class}(e)
\tag{Prov-Addr}
$$

$$
\text{prov}(p') = \text{prov}(p) \quad \text{(copy/move)}
\tag{Prov-Copy}
$$

##### §7.5.3.2 Escape Restrictions

[14] Escape analysis forbids pointer escape when provenance would become invalid:

- **Stack escape**: returning or storing a stack pointer beyond procedure scope → E07-300.
- **Region escape**: pointer leaves region lifetime without explicit heap escape → E07-300.
- **Heap**: unrestricted (subject to ownership checks in Clause 12).

Pseudo algorithm:

```
CHECK_ESCAPE(ptr, target_scope):
    match prov(ptr):
        Stack  -> deny if target_scope ≠ current_procedure
        Region(r) -> deny if target_scope escapes r
        Heap   -> allow
```

Diagnostics include allocation site, escape site, and suggested remedies (e.g., `.to_heap()` or returning by move).

#### §7.5.4 Copy Predicate and Layout [type.pointer.safe.copy]

[15] Theorem 7.5.1 (_Pointer Copy_): `Ptr<T>@S` is `Copy` for all states `S`. Proof sketch: pointer values are machine addresses independent of pointee semantics.

[16] Size and alignment equal machine word size and alignment:

$$
\text{size}(\text{Ptr}\langle\tau\rangle@S) = \text{size}(usize) \quad \text{align}(\text{Ptr}\langle\tau\rangle@S) = \text{align}(usize)
\tag{Ptr-Size}
$$

#### §7.5.5 Raw Pointers [type.pointer.raw]

##### §7.5.5.1 Formation and Copy

[17] Raw pointers are well-formed when the pointee type is well-formed:

$$
\dfrac{\Gamma \vdash \tau : \text{Type}}{\Gamma \vdash *\text{const}\; \tau : \text{Type}} \qquad \dfrac{\Gamma \vdash \tau : \text{Type}}{\Gamma \vdash *\text{mut}\; \tau : \text{Type}}
\tag{WF-Raw}
$$

They are always `Copy`, with the same size/alignment as safe pointers.

##### §7.5.5.2 Operations

[18] Dereferencing and writing require the grant `unsafe.ptr`:

$$
\dfrac{\Gamma \vdash p : *\text{const}\; \tau}{\Gamma \vdash *p : \tau \; ! \{\text{unsafe.ptr}\}}
\tag{T-Raw-Deref}
$$

$$
\dfrac{\Gamma \vdash p : *\text{mut}\; \tau \quad \Gamma \vdash v : \tau}{\Gamma \vdash (*p = v) : () \; ! \{\text{unsafe.ptr}\}}
\tag{T-Raw-Write}
$$

Pointer arithmetic and casts also require `unsafe.ptr` and are otherwise unchecked. Undefined behaviour occurs if alignment, lifetime, or aliasing rules are violated; the spec imposes no compile-time enforcement beyond the `unsafe` requirement.

##### §7.5.5.3 Interoperation

[19] Coercions:

- `Ptr<T>@Valid` → `*const T`/`*mut T` (explicit cast).
- `*const T`/`*mut T` → `Ptr<T>` forbidden without proving state (requires user-provided witness or safe wrapper).
- Capturing closures may not be cast to raw procedure pointers (E07-206).

#### §7.5.6 Diagnostics [type.pointer.diagnostics]

[20] Mandatory diagnostics:

| Code    | Condition                                        |
| ------- | ------------------------------------------------ |
| E07-300 | Pointer escape violating provenance rules        |
| E07-301 | Dereference of `Ptr<T>@Null`                     |
| E07-302 | Address-of expression without storage            |
| E07-303 | Missing explicit pointer type annotation         |
| E07-304 | Dereference of `Ptr<T>@Weak`                     |
| E07-305 | Dereference of `Ptr<T>@Expired`                  |
| E07-400 | Raw pointer dereference outside `unsafe` context |
| E07-401 | Raw pointer assignment outside `unsafe` context  |
| E07-402 | Cast between incompatible raw pointer types      |

Diagnostics follow Annex E §E.5 for payload structure (allocation sites, provenance, and suggested fixes).

Diagnostics shall include allocation site, pointer provenance, and hints consistent with Annex E §E.5.

#### §7.5.7 Examples (Informative) [type.pointer.examples]

**Example 7.5.7.1 (Safe pointer workflow):**

```cursive
record Node {
    parent: Ptr<Node>@Weak,
    children: [Ptr<Node>@Valid],
}

procedure attach_child(parent: Ptr<Node>@Valid, child: Ptr<Node>@Valid) {
    (*child).parent = parent.downgrade()
    (*parent).children.push(child)
}

procedure print_parent(child: Ptr<Node>) {
    match child {
        @Valid => match (*child).parent.upgrade() {
            @Valid => println("parent children: {}", (*parent).children.len()),
            @Null  => println("no parent"),
        },
        _ => println("invalid node pointer"),
    }
}
```

**Example 7.5.7.2 (Escape violation):**

```cursive
procedure dangling(): Ptr<i32> {
    region r {
        let value = ^42
        result &value    // error[E07-300]
    }
}
```

**Example 7.5.7.3 (Raw pointer usage):**

```cursive
procedure bump(ptr: *mut i32)
    [[ unsafe.ptr |- true => true ]]
{
    unsafe {
        *ptr = *ptr + 1
    }
}
```

#### §7.5.8 Conformance Requirements [type.pointer.requirements]

[21] A conforming implementation shall:

1. Support the pointer syntax in §7.5.1 and states in §7.5.2.1.
2. Enforce address-of, dereference, downgrade/upgrade, and pattern-matching rules, rejecting invalid state operations with diagnostics E07-301, E07-304, E07-305.
3. Track provenance (Stack, Region, Heap) and reject illegal pointer escapes, reporting E07-300 with contextual notes.
4. Require explicit pointer type annotations (E07-303) and prohibit address-of on non-storage expressions (E07-302).
5. Treat all safe and raw pointers as `Copy` with machine word size and alignment.
6. Require `unsafe.ptr` grants for raw pointer dereference, writes, arithmetic, and casts, issuing E07-400–E07-402 when missing.
7. Maintain pointer metadata through type inference and ABI lowering so introspection (§7.7) reflects accurate state and provenance information.

[22] Deviations render an implementation non-conforming unless another clause explicitly designates the behaviour as implementation-defined.

---

**Previous**: §7.4 Function and Procedure Types (§7.4 [type.function]) | **Next**: §7.6 Modal Types (§7.6 [type.modal])

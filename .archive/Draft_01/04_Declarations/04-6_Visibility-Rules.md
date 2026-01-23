# Cursive Language Specification

## Clause 4 — Declarations

**Clause**: 4 — Declarations
**File**: 05-6_Visibility-Rules.md
**Section**: §4.6 Visibility Rules
**Stable label**: [decl.visibility]  
**Forward references**: Clause 3 [module], §4.2 [decl.variable], §4.5 [decl.type], Clause 9 [generic]

---

### §4.6 Visibility Rules [decl.visibility]

#### §4.6.1 Overview

[1] Visibility modifiers determine which scopes may refer to a declaration. Cursive recognises four modifiers: `public`, `internal`, `private`, and `protected`.

[2] This subclause states where modifiers may appear, the defaults that apply when no modifier is written, and the limits imposed when re-exporting or aliasing declarations.

#### §5.6.2 Syntax

**Visibility modifiers** take one of the following forms:
```
"public"
"internal"
"private"
"protected"
```

[ Note: See Annex A §A.6 [grammar.declaration] for the normative `visibility` production.
— end note ]

#### §5.6.3 Defaults and Placement

[1] Module-scope declarations default to `internal` visibility. Writing `private` or `protected` at module scope is ill-formed (diagnostic E05-601); authors shall use `internal` explicitly when they wish to emphasise the default.

[2] Fields inside records, tuple records, enum variants, and modal state payloads inherit the containing type’s visibility unless overridden by an explicit modifier.

[3] Procedures nested inside types (including modal state transition blocks) inherit the type’s visibility. Procedures declared at module scope obey the module default.

[4] Visibility modifiers shall not appear on local (block-scoped) bindings. Any attempt to do so is ill-formed (diagnostic E05-602).

#### §5.6.4 Modifier Semantics

[1] **public** — Accessible from any module once imported. `public use` re-exports preserve this visibility.

[2] **internal** — Accessible only within the declaring module. Importers cannot name the entity directly; any attempt to re-export it as `public` is ill-formed (diagnostic E05-603).

[3] **private** — Accessible only within the smallest enclosing declaration:

- For type members, visibility is limited to the declaring type’s own procedures.
- For modal state payloads, visibility is limited to transition procedures defined in that state’s block.
- Module-scope `private` is rejected by [1].

[4] **protected** — May be applied only to members declared inside a type or modal state block. It grants visibility to the declaring type and to behaviors or contracts (Clause 10 and Clause 12) that provide implementations for that type. Outside those contexts `protected` behaves like `internal`. Module-scope or stand-alone `protected` declarations are rejected by [1].

#### §5.6.5 Re-export and Aliasing Rules

[1] `public use module::item` is valid only when `item` was `public` in its defining module. Attempting to widen visibility produces diagnostic E05-603.

[2] Visibility narrowing during re-export is disallowed. Modifiers other than `public` shall not be written with `use`; statements such as `private use`, `internal use`, or `protected use` are ill-formed (diagnostic E05-604).

[3] Type aliases adopt the visibility written on the alias declaration. They do not inherit the aliased type's visibility.

[4] _Nested visibility._ When a type member has explicit visibility that differs from the containing type's visibility, the more restrictive modifier applies. For example, a `private` field in a `public` record remains `private`; a `protected` procedure in a `public` type remains `protected`. This ensures that explicit modifiers are never widened by inheritance.

[5] _Protected access context._ `protected` members are accessible only within: - The declaring type's own procedures - Behavior implementations (`behavior X for Y`) where `Y` matches the declaring type - Contract implementations (`contract X for Y`) where `Y` matches the declaring type - Modal state transition procedures defined within the same state block (for state-specific `protected` members)

    Attempts to access `protected` members from other contexts (e.g., external modules, unrelated behaviors) emit diagnostic E05-605 (protected member access violation).

[6] _Visibility inheritance in type hierarchies._ When a type implements a behavior or contract, `protected` members of the implementing type become accessible within the behavior/contract implementation, but not within the behavior/contract declaration itself. This allows implementations to access internal helpers while keeping the interface clean.

#### §5.6.6 Examples (Informative)

**Example 5.6.6.1 (Module defaults and errors):**

```cursive
let cache_size = 1024          // internal by default
public procedure fetch(id: UserId): Record
{ ... }
private let temp = 0           // error[E05-601]
```

**Example 5.6.6.2 (Type members):**

```cursive
public record User {
    public id: UserId
    private password_hash: string

    protected procedure helper(~) { ... }
}
```

User behavior implementations may reference `helper`, whereas external modules cannot.

**Example 5.6.6.3 - invalid (Visibility widening on re-export):**

```cursive
public use auth::internal_helper  // error[E05-603]
```

**Example 5.6.6.4 - invalid (Visibility narrowing on re-export):**

```cursive
private use auth::Session         // error[E05-604]
```

**Example 5.6.6.5 (Protected access through behavior):**

```cursive
behavior UserStorage for User {
    procedure seed(~%, data: Seed)
 {
        User.helper(data.initial_password)
    }
}
```

### §5.6.7 Conformance Requirements [decl.visibility.requirements]

[1] Implementations shall reject attempts to apply `private` or `protected` at module scope (E05-601) and forbid visibility modifiers on local bindings (E05-602).

[2] Compilers shall prevent re-exports from widening or narrowing visibility (E05-603–E05-604) and ensure that `protected` members are accessible only within the contexts described in §5.6.4.
[ Note: Behavior `UserStorage` may call the protected `helper`, while unrelated modules cannot.
— end note ]
